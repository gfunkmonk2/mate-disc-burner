/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Librejilla-burn
 * Copyright (C) Philippe Rouquier 2005-2009 <bonfire-app@wanadoo.fr>
 *
 * Librejilla-burn is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * The Librejilla-burn authors hereby grant permission for non-GPL compatible
 * GStreamer plugins to be used and distributed together with GStreamer
 * and Librejilla-burn. This permission is above and beyond the permissions granted
 * by the GPL license by which Librejilla-burn is covered. If you modify this code
 * you may extend this exception to your version of the code, but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version.
 * 
 * Librejilla-burn is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>
#include <gmodule.h>

#include "rejilla-plugin-registration.h"
#include "burn-job.h"
#include "rejilla-tags.h"
#include "rejilla-track-image.h"


#define REJILLA_TYPE_AUDIO2CUE         (rejilla_audio2cue_get_type ())
#define REJILLA_AUDIO2CUE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), REJILLA_TYPE_AUDIO2CUE, RejillaAudio2Cue))
#define REJILLA_AUDIO2CUE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), REJILLA_TYPE_AUDIO2CUE, RejillaAudio2CueClass))
#define REJILLA_IS_AUDIO2CUE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), REJILLA_TYPE_AUDIO2CUE))
#define REJILLA_IS_AUDIO2CUE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), REJILLA_TYPE_AUDIO2CUE))
#define REJILLA_AUDIO2CUE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), REJILLA_TYPE_AUDIO2CUE, RejillaAudio2CueClass))

REJILLA_PLUGIN_BOILERPLATE (RejillaAudio2Cue, rejilla_audio2cue, REJILLA_TYPE_JOB, RejillaJob);

struct _RejillaAudio2CuePrivate {
	goffset total;
	goffset bytes;

	GThread *thread;
	GMutex *mutex;
	GCond *cond;
	GError *error;
	gint thread_id;

	guint cancel:1;
	guint success:1;
};
typedef struct _RejillaAudio2CuePrivate RejillaAudio2CuePrivate;

#define REJILLA_AUDIO2CUE_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REJILLA_TYPE_AUDIO2CUE, RejillaAudio2CuePrivate))

static RejillaAudio2CueClass *parent_class = NULL;


static void
rejilla_audio2cue_stop_real (RejillaAudio2Cue *self)
{
	RejillaAudio2CuePrivate *priv;

	priv = REJILLA_AUDIO2CUE_PRIVATE (self);

	g_mutex_lock (priv->mutex);
	if (priv->thread) {
		priv->cancel = 1;
		g_cond_wait (priv->cond, priv->mutex);
		priv->cancel = 0;
	}
	g_mutex_unlock (priv->mutex);

	if (priv->thread_id) {
		g_source_remove (priv->thread_id);
		priv->thread_id = 0;
	}

	if (priv->error) {
		g_error_free (priv->error);
		priv->error = NULL;
	}
}

static RejillaBurnResult
rejilla_audio2cue_stop (RejillaJob *self,
			GError **error)
{
	rejilla_audio2cue_stop_real (REJILLA_AUDIO2CUE (self));
	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_audio2cue_clock_tick (RejillaJob *job)
{
	RejillaAudio2CuePrivate *priv;

	priv = REJILLA_AUDIO2CUE_PRIVATE (job);

	rejilla_job_start_progress (job, FALSE);
	rejilla_job_set_progress (job,
				  (gdouble) priv->bytes /
				  (gdouble) priv->total);

	return REJILLA_BURN_OK;
}

static gboolean
rejilla_audio2cue_create_finished (gpointer user_data)
{
	gchar *toc = NULL;
	goffset blocks = 0;
	gchar *image = NULL;
	RejillaTrackImage *track;

	track = rejilla_track_image_new ();

	rejilla_job_get_session_output_size (user_data, &blocks, NULL);
	rejilla_track_image_set_block_num (track, blocks);
	rejilla_job_get_image_output (user_data,
				      &image,
				      &toc);
	rejilla_track_image_set_source (track,
					image,
					toc,
					REJILLA_IMAGE_FORMAT_CUE);
	g_free (image);
	g_free (toc);

	rejilla_job_add_track (user_data, REJILLA_TRACK (track));
	g_object_unref (track);

	rejilla_job_finished_session (user_data);

	return FALSE;
}

static gint
rejilla_audio2cue_read (RejillaAudio2Cue *self,
			int fd,
			guchar *buffer,
			gint bytes,
			GError **error)
{
	gint total = 0;
	gint read_bytes;
	RejillaAudio2CuePrivate *priv;

	priv = REJILLA_AUDIO2CUE_PRIVATE (self);

	while (1) {
		read_bytes = read (fd, buffer + total, (bytes - total));

		/* maybe that's the end of the stream ... */
		if (!read_bytes)
			return total;

		if (priv->cancel)
			return -2;

		/* ... or an error =( */
		if (read_bytes == -1) {
			if (errno != EAGAIN && errno != EINTR) {
                                int errsv = errno;

				g_set_error (error,
					     REJILLA_BURN_ERROR,
					     REJILLA_BURN_ERROR_GENERAL,
					     _("Data could not be read (%s)"),
					     g_strerror (errsv));
				return -1;
			}
		}
		else {
			total += read_bytes;

			if (total == bytes)
				return total;
		}

		g_usleep (500);
	}

	return total;
}

static RejillaBurnResult
rejilla_audio2cue_write (RejillaAudio2Cue *self,
			 int fd,
			 guchar *buffer,
			 gint bytes,
			 GError **error)
{
	gint bytes_remaining;
	gint bytes_written = 0;
	RejillaAudio2CuePrivate *priv;

	priv = REJILLA_AUDIO2CUE_PRIVATE (self);

	bytes_remaining = bytes;
	while (bytes_remaining) {
		gint written;

		written = write (fd,
				 buffer + bytes_written,
				 bytes_remaining);

		if (priv->cancel)
			return REJILLA_BURN_CANCEL;

		if (written != bytes_remaining) {
			if (errno != EINTR && errno != EAGAIN) {
                                int errsv = errno;

				/* unrecoverable error */
				g_set_error (error,
					     REJILLA_BURN_ERROR,
					     REJILLA_BURN_ERROR_GENERAL,
					     _("Data could not be written (%s)"),
					     g_strerror (errsv));
				return REJILLA_BURN_ERR;
			}
		}

		g_usleep (500);

		if (written > 0) {
			bytes_remaining -= written;
			bytes_written += written;
		}
	}

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_audio2cue_write_bin (RejillaAudio2Cue *self,
			     int fd_in,
			     int fd_out)
{
	RejillaAudio2CuePrivate *priv;

	priv = REJILLA_AUDIO2CUE_PRIVATE (self);

	while (1) {
		gint read_bytes;
		guchar buffer [2352 * 10];
		RejillaBurnResult result;

		read_bytes = rejilla_audio2cue_read (self,
		                                     fd_in,
		                                     buffer,
		                                     sizeof (buffer),
		                                     &priv->error);

		/* This is a simple cancellation */
		if (read_bytes == -2)
			return REJILLA_BURN_CANCEL;

		if (read_bytes == -1) {
			int err_saved = errno;

			priv->error = g_error_new_literal (REJILLA_BURN_ERROR,
							   REJILLA_BURN_ERROR_GENERAL,
							   strerror (err_saved));
			return REJILLA_BURN_ERR;
		}

		if (!read_bytes)
			break;

		result = rejilla_audio2cue_write (self,
		                                  fd_out,
		                                  buffer,
		                                  read_bytes,
		                                  &priv->error);
		if (result != REJILLA_BURN_OK)
			return result;

		priv->bytes += read_bytes;
	}

	return REJILLA_BURN_OK;
}

static gchar *
rejilla_audio2cue_len_to_string (guint64 len)
{
	int sec;
	int min;
	guint64 frame;

	if (len >= 1000000000LL)
		frame = (len % 1000000000LL) * 75;
	else
		frame = len * 75;

	frame = frame / 1000000000 + ((frame % 1000000000LL) ? 1:0);

	len /= 1000000000LL;
	min = len / 60;
	sec = len % 60;

	return g_strdup_printf ("%02i:%02i:%02" G_GINT64_FORMAT, min, sec, frame);
}

static gpointer
rejilla_audio2cue_create_thread (gpointer data)
{
	RejillaAudio2CuePrivate *priv;
	RejillaBurnResult result;
	guint64 total_len = 0;
	GSList *tracks = NULL;
	gchar *image = NULL;
	gchar *album = NULL;
	gchar *toc = NULL;
	int fd_out = -1;
	int fd_in = -1;
	gchar *line;
	int num = 1;

	priv = REJILLA_AUDIO2CUE_PRIVATE (data);
	priv->success = FALSE;

	/* Get all audio data as input and write .bin */
	rejilla_job_get_image_output (data,
	                              &image,
	                              &toc);
	if (!toc || !image)
		goto end;

	fd_out = open (image,
	               O_WRONLY|O_CREAT,
	               S_IWUSR|S_IRUSR);

	if (fd_out < 0) {
		int err_saved = errno;
		priv->error = g_error_new_literal (REJILLA_BURN_ERROR,
						   REJILLA_BURN_ERROR_GENERAL,
						   strerror (err_saved));
		goto end;
	}

	rejilla_job_set_current_action (data,
					REJILLA_BURN_ACTION_CREATING_IMAGE,
					NULL,
					FALSE);

	if (rejilla_job_get_fd_in (data, &fd_in) != REJILLA_BURN_OK) {
		tracks = NULL;
		rejilla_job_get_tracks (data, &tracks);
		for (; tracks; tracks = tracks->next) {
			RejillaTrackStream *track;
			gchar *song_path;

			track = tracks->data;
			song_path = rejilla_track_stream_get_source (track, FALSE);

			REJILLA_JOB_LOG (data, "Writing data from %s", song_path);
			fd_in = open (song_path, O_RDONLY);
			g_free (song_path);

			if (fd_in < 0) {
				int err_saved = errno;

				priv->error = g_error_new_literal (REJILLA_BURN_ERROR,
								   REJILLA_BURN_ERROR_GENERAL,
								   strerror (err_saved));
				goto end;
			}

			result = rejilla_audio2cue_write_bin (data, fd_in, fd_out);

			close (fd_in);
			fd_in = -1;

			if (result != REJILLA_BURN_OK)
				goto end;
		}
	}
	else {
		REJILLA_JOB_LOG (data, "Writing data from fd");
		rejilla_audio2cue_write_bin (data, fd_in, fd_out);
	}

	close (fd_out);
	fd_out = -1;

	/* Write cue file */
	fd_out = open (toc,
	               O_WRONLY|O_CREAT,
	               S_IWUSR|S_IRUSR);

	if (fd_out < 0) {
		int err_saved = errno;
		priv->error = g_error_new_literal (REJILLA_BURN_ERROR,
						   REJILLA_BURN_ERROR_GENERAL,
						   strerror (err_saved));
		goto end;
	}

	/** Most significant byte first (BINARY otherwise as far as I can tell) **/
	line = g_strdup_printf ("FILE \"%s\" MOTOROLA\n", image);
	if (write (fd_out, line, strlen (line)) < 0) {
		int err_saved = errno;
		priv->error = g_error_new_literal (REJILLA_BURN_ERROR,
						   REJILLA_BURN_ERROR_GENERAL,
						   strerror (err_saved));

		g_free (line);
		goto end;
	}
	g_free (line);

	rejilla_job_get_audio_title (data, &album);
	if (album) {
		line = g_strdup_printf ("TITLE \"%s\"\n", album);
		g_free (album);
		album = NULL;

		if (write (fd_out, line, strlen (line)) < 0) {
			int err_saved = errno;
			priv->error = g_error_new_literal (REJILLA_BURN_ERROR,
							   REJILLA_BURN_ERROR_GENERAL,
							   strerror (err_saved));

			g_free (line);
			goto end;
		}
		g_free (line);
	}

	tracks = NULL;
	rejilla_job_get_tracks (data, &tracks);
	for (; tracks; tracks = tracks->next) {
		int isrc;
		guint64 gap;
		guint64 len;
		gchar *string;
		const gchar *title;
		RejillaTrack *track;
		const gchar *performer;
		const gchar *songwriter;

		track = tracks->data;

		string = rejilla_track_stream_get_source (REJILLA_TRACK_STREAM (track), FALSE);
		REJILLA_JOB_LOG (data, "Writing cue entry for \"%s\"", string);
		g_free (string);

		line = g_strdup_printf ("TRACK %02i AUDIO\n", num);
		if (write (fd_out, line, strlen (line)) < 0) {
			int err_saved = errno;
			priv->error = g_error_new_literal (REJILLA_BURN_ERROR,
							   REJILLA_BURN_ERROR_GENERAL,
							   strerror (err_saved));

			g_free (line);
			goto end;
		}
		g_free (line);

		title = rejilla_track_tag_lookup_string (track, REJILLA_TRACK_STREAM_TITLE_TAG);
		if (title) {
			line = g_strdup_printf ("\tTITLE \"%s\"\n", title);
			if (write (fd_out, line, strlen (line)) < 0) {
				int err_saved = errno;
				priv->error = g_error_new_literal (REJILLA_BURN_ERROR,
								   REJILLA_BURN_ERROR_GENERAL,
								   strerror (err_saved));

				g_free (line);
				goto end;
			}
			g_free (line);
		}

		performer = rejilla_track_tag_lookup_string (track, REJILLA_TRACK_STREAM_ARTIST_TAG);
		if (performer) {
			line = g_strdup_printf ("\tPERFORMER \"%s\"\n", performer);
			if (write (fd_out, line, strlen (line)) < 0) {
				int err_saved = errno;
				priv->error = g_error_new_literal (REJILLA_BURN_ERROR,
								   REJILLA_BURN_ERROR_GENERAL,
								   strerror (err_saved));

				g_free (line);
				goto end;
			}
			g_free (line);
		}

		songwriter = rejilla_track_tag_lookup_string (track, REJILLA_TRACK_STREAM_COMPOSER_TAG);
		if (songwriter) {
			line = g_strdup_printf ("\tSONGWRITER \"%s\"\n", songwriter);
			if (write (fd_out, line, strlen (line)) < 0) {
				int err_saved = errno;
				priv->error = g_error_new_literal (REJILLA_BURN_ERROR,
								   REJILLA_BURN_ERROR_GENERAL,
								   strerror (err_saved));

				g_free (line);
				goto end;
			}
			g_free (line);
		}

		isrc = rejilla_track_tag_lookup_int (track, REJILLA_TRACK_STREAM_ISRC_TAG);
		if (isrc > 0) {
			line = g_strdup_printf ("\tISRC %i\n", isrc);
			if (write (fd_out, line, strlen (line)) < 0) {
				int err_saved = errno;
				priv->error = g_error_new_literal (REJILLA_BURN_ERROR,
								   REJILLA_BURN_ERROR_GENERAL,
								   strerror (err_saved));

				g_free (line);
				goto end;
			}
			g_free (line);
		}

		gap = rejilla_track_stream_get_gap (REJILLA_TRACK_STREAM (track));

		string = rejilla_audio2cue_len_to_string (total_len);
		line = g_strdup_printf ("\tINDEX 01 %s\n", string);
		g_free (string);

		if (write (fd_out, line, strlen (line)) < 0) {
			int err_saved = errno;
			priv->error = g_error_new_literal (REJILLA_BURN_ERROR,
							   REJILLA_BURN_ERROR_GENERAL,
							   strerror (err_saved));

			g_free (line);
			goto end;
		}
		g_free (line);

		rejilla_track_stream_get_length (REJILLA_TRACK_STREAM (track), &len);
		total_len += (len - gap);

		/**
		 * This does not work yet but could be useful?
		if (tracks->next) {
			string = rejilla_audio2cue_len_to_string (total_len);
			line = g_strdup_printf ("\tINDEX 02 %s\n", string);
			g_free (string);

			if (write (fd_out, line, strlen (line)) < 0) {
				int err_saved = errno;
				priv->error = g_error_new_literal (REJILLA_BURN_ERROR,
								   REJILLA_BURN_ERROR_GENERAL,
								   strerror (err_saved));

				g_free (line);
				goto end;
			}
			g_free (line);
		}
		**/

		if (gap) {
			string = rejilla_audio2cue_len_to_string (gap);
			line = g_strdup_printf ("\tPOSTGAP %s\n", string);
			g_free (string);

			if (write (fd_out, line, strlen (line)) < 0) {
				int err_saved = errno;
				priv->error = g_error_new_literal (REJILLA_BURN_ERROR,
								   REJILLA_BURN_ERROR_GENERAL,
								   strerror (err_saved));

				g_free (line);
				goto end;
			}
			g_free (line);
		}

		num ++;
	}

	close (fd_out);
	priv->success = TRUE;

end:

	/* Clean */
	if (fd_out > 0)
		close (fd_out);

	if (fd_in > 0)
		close (fd_in);

	if (toc)
		g_free (toc);

	if (image)
		g_free (image);

	/* Get out of the thread */
	if (!priv->cancel)
		priv->thread_id = g_idle_add (rejilla_audio2cue_create_finished, data);

	g_mutex_lock (priv->mutex);
	priv->thread = NULL;
	g_cond_signal (priv->cond);
	g_mutex_unlock (priv->mutex);

	g_thread_exit (NULL);

	return NULL;
}

static RejillaBurnResult
rejilla_audio2cue_start (RejillaJob *job,
			 GError **error)
{
	RejillaAudio2CuePrivate *priv;
	GError *thread_error = NULL;
	RejillaJobAction action;
	goffset bytes = 0;

	priv = REJILLA_AUDIO2CUE_PRIVATE (job);

	rejilla_job_get_action (job, &action);
	if (action == REJILLA_JOB_ACTION_SIZE) {
		RejillaTrack *track = NULL;
		goffset blocks = 0;
		goffset bytes = 0;

		rejilla_job_get_current_track (job, &track);
		rejilla_track_get_size (track, &blocks, &bytes);
		rejilla_job_set_output_size_for_current_track (job, blocks, bytes);
		return REJILLA_BURN_NOT_RUNNING;
	}

	rejilla_job_get_session_output_size (job, NULL, &bytes);
	priv->total = bytes;

	g_mutex_lock (priv->mutex);
	priv->thread = g_thread_create (rejilla_audio2cue_create_thread,
					job,
					FALSE,
					&thread_error);
	g_mutex_unlock (priv->mutex);

	/* Reminder: this is not necessarily an error as the thread may have finished */
	//if (!priv->thread)
	//	return REJILLA_BURN_ERR;
	if (thread_error) {
		g_propagate_error (error, thread_error);
		return REJILLA_BURN_ERR;
	}

	return REJILLA_BURN_OK;
}

static void
rejilla_audio2cue_class_init (RejillaAudio2CueClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	RejillaJobClass *job_class = REJILLA_JOB_CLASS (klass);

	g_type_class_add_private (klass, sizeof (RejillaAudio2CuePrivate));

	parent_class = g_type_class_peek_parent(klass);
	object_class->finalize = rejilla_audio2cue_finalize;

	job_class->start = rejilla_audio2cue_start;
	job_class->stop = rejilla_audio2cue_stop;
	job_class->clock_tick = rejilla_audio2cue_clock_tick;
}

static void
rejilla_audio2cue_init (RejillaAudio2Cue *obj)
{
	RejillaAudio2CuePrivate *priv;

	priv = REJILLA_AUDIO2CUE_PRIVATE (obj);
	priv->mutex = g_mutex_new ();
	priv->cond = g_cond_new ();
}

static void
rejilla_audio2cue_finalize (GObject *object)
{
	rejilla_audio2cue_stop_real (REJILLA_AUDIO2CUE (object));
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
rejilla_audio2cue_export_caps (RejillaPlugin *plugin)
{
	GSList *output;
	GSList *input;

	rejilla_plugin_define (plugin,
			       "audio2cue",
	                       NULL,
			       _("Generates .cue files from audio"),
			       "Philippe Rouquier",
			       0);

	output = rejilla_caps_image_new (REJILLA_PLUGIN_IO_ACCEPT_FILE,
	                                 REJILLA_IMAGE_FORMAT_CUE);

	input = rejilla_caps_audio_new (REJILLA_PLUGIN_IO_ACCEPT_FILE|
					REJILLA_PLUGIN_IO_ACCEPT_PIPE,
	                                REJILLA_AUDIO_FORMAT_RAW|
					REJILLA_METADATA_INFO);

	rejilla_plugin_link_caps (plugin, output, input);
	g_slist_free (output);
	g_slist_free (input);
}
