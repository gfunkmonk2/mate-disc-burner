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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/param.h>
#include <unistd.h>
#include <fcntl.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>

#include <gmodule.h>

#include "rejilla-plugin-registration.h"
#include "burn-job.h"
#include "burn-volume.h"
#include "rejilla-drive.h"
#include "rejilla-track-disc.h"
#include "rejilla-track-image.h"
#include "rejilla-tags.h"


#define REJILLA_TYPE_CHECKSUM_IMAGE		(rejilla_checksum_image_get_type ())
#define REJILLA_CHECKSUM_IMAGE(o)		(G_TYPE_CHECK_INSTANCE_CAST ((o), REJILLA_TYPE_CHECKSUM_IMAGE, RejillaChecksumImage))
#define REJILLA_CHECKSUM_CLASS(k)		(G_TYPE_CHECK_CLASS_CAST((k), REJILLA_TYPE_CHECKSUM_IMAGE, RejillaChecksumImageClass))
#define REJILLA_IS_CHECKSUM_IMAGE(o)		(G_TYPE_CHECK_INSTANCE_TYPE ((o), REJILLA_TYPE_CHECKSUM_IMAGE))
#define REJILLA_IS_CHECKSUM_IMAGE_CLASS(k)	(G_TYPE_CHECK_CLASS_TYPE ((k), REJILLA_TYPE_CHECKSUM_IMAGE))
#define REJILLA_CHECKSUM_GET_CLASS(o)		(G_TYPE_INSTANCE_GET_CLASS ((o), REJILLA_TYPE_CHECKSUM_IMAGE, RejillaChecksumImageClass))

REJILLA_PLUGIN_BOILERPLATE (RejillaChecksumImage, rejilla_checksum_image, REJILLA_TYPE_JOB, RejillaJob);

struct _RejillaChecksumImagePrivate {
	GChecksum *checksum;
	RejillaChecksumType checksum_type;

	/* That's for progress reporting */
	goffset total;
	goffset bytes;

	/* this is for the thread and the end of it */
	GThread *thread;
	GMutex *mutex;
	GCond *cond;
	gint end_id;

	guint cancel;
};
typedef struct _RejillaChecksumImagePrivate RejillaChecksumImagePrivate;

#define REJILLA_CHECKSUM_IMAGE_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REJILLA_TYPE_CHECKSUM_IMAGE, RejillaChecksumImagePrivate))

#define REJILLA_SCHEMA_CONFIG		"org.mate.rejilla.config"
#define REJILLA_PROPS_CHECKSUM_IMAGE	"checksum-image"

static RejillaJobClass *parent_class = NULL;

static gint
rejilla_checksum_image_read (RejillaChecksumImage *self,
			     int fd,
			     guchar *buffer,
			     gint bytes,
			     GError **error)
{
	gint total = 0;
	gint read_bytes;
	RejillaChecksumImagePrivate *priv;

	priv = REJILLA_CHECKSUM_IMAGE_PRIVATE (self);

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
rejilla_checksum_image_write (RejillaChecksumImage *self,
			      int fd,
			      guchar *buffer,
			      gint bytes,
			      GError **error)
{
	gint bytes_remaining;
	gint bytes_written = 0;
	RejillaChecksumImagePrivate *priv;

	priv = REJILLA_CHECKSUM_IMAGE_PRIVATE (self);

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
rejilla_checksum_image_checksum (RejillaChecksumImage *self,
				 GChecksumType checksum_type,
				 int fd_in,
				 int fd_out,
				 GError **error)
{
	gint read_bytes;
	guchar buffer [2048];
	RejillaBurnResult result;
	RejillaChecksumImagePrivate *priv;

	priv = REJILLA_CHECKSUM_IMAGE_PRIVATE (self);

	priv->checksum = g_checksum_new (checksum_type);
	result = REJILLA_BURN_OK;
	while (1) {
		read_bytes = rejilla_checksum_image_read (self,
							  fd_in,
							  buffer,
							  sizeof (buffer),
							  error);
		if (read_bytes == -2)
			return REJILLA_BURN_CANCEL;

		if (read_bytes == -1)
			return REJILLA_BURN_ERR;

		if (!read_bytes)
			break;

		/* it can happen when we're just asked to generate a checksum
		 * that we don't need to output the received data */
		if (fd_out > 0) {
			result = rejilla_checksum_image_write (self,
							       fd_out,
							       buffer,
							       read_bytes, error);
			if (result != REJILLA_BURN_OK)
				break;
		}

		g_checksum_update (priv->checksum,
				   buffer,
				   read_bytes);

		priv->bytes += read_bytes;
	}

	return result;
}

static RejillaBurnResult
rejilla_checksum_image_checksum_fd_input (RejillaChecksumImage *self,
					  GChecksumType checksum_type,
					  GError **error)
{
	int fd_in = -1;
	int fd_out = -1;
	RejillaBurnResult result;
	RejillaChecksumImagePrivate *priv;

	priv = REJILLA_CHECKSUM_IMAGE_PRIVATE (self);

	REJILLA_JOB_LOG (self, "Starting checksum generation live (size = %lli)", priv->total);
	result = rejilla_job_set_nonblocking (REJILLA_JOB (self), error);
	if (result != REJILLA_BURN_OK)
		return result;

	rejilla_job_get_fd_in (REJILLA_JOB (self), &fd_in);
	rejilla_job_get_fd_out (REJILLA_JOB (self), &fd_out);

	return rejilla_checksum_image_checksum (self, checksum_type, fd_in, fd_out, error);
}

static RejillaBurnResult
rejilla_checksum_image_checksum_file_input (RejillaChecksumImage *self,
					    GChecksumType checksum_type,
					    GError **error)
{
	RejillaChecksumImagePrivate *priv;
	RejillaBurnResult result;
	RejillaTrack *track;
	int fd_out = -1;
	int fd_in = -1;
	gchar *path;

	priv = REJILLA_CHECKSUM_IMAGE_PRIVATE (self);

	/* get all information */
	rejilla_job_get_current_track (REJILLA_JOB (self), &track);
	path = rejilla_track_image_get_source (REJILLA_TRACK_IMAGE (track), FALSE);
	if (!path) {
		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_FILE_NOT_LOCAL,
			     _("The file is not stored locally"));
		return REJILLA_BURN_ERR;
	}

	REJILLA_JOB_LOG (self,
			 "Starting checksuming file %s (size = %"G_GOFFSET_FORMAT")",
			 path,
			 priv->total);

	fd_in = open (path, O_RDONLY);
	if (!fd_in) {
                int errsv;
		gchar *name = NULL;

		if (errno == ENOENT)
			return REJILLA_BURN_RETRY;

		name = g_path_get_basename (path);

                errsv = errno;

		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_GENERAL,
			     /* Translators: first %s is the filename, second %s
			      * is the error generated from errno */
			     _("\"%s\" could not be opened (%s)"),
			     name,
			     g_strerror (errsv));
		g_free (name);
		g_free (path);

		return REJILLA_BURN_ERR;
	}

	/* and here we go */
	rejilla_job_get_fd_out (REJILLA_JOB (self), &fd_out);
	result = rejilla_checksum_image_checksum (self, checksum_type, fd_in, fd_out, error);
	g_free (path);
	close (fd_in);

	return result;
}

static RejillaBurnResult
rejilla_checksum_image_create_checksum (RejillaChecksumImage *self,
					GError **error)
{
	RejillaBurnResult result;
	RejillaTrack *track = NULL;
	GChecksumType checksum_type;
	RejillaChecksumImagePrivate *priv;

	priv = REJILLA_CHECKSUM_IMAGE_PRIVATE (self);

	/* get the checksum type */
	switch (priv->checksum_type) {
		case REJILLA_CHECKSUM_MD5:
			checksum_type = G_CHECKSUM_MD5;
			break;
		case REJILLA_CHECKSUM_SHA1:
			checksum_type = G_CHECKSUM_SHA1;
			break;
		case REJILLA_CHECKSUM_SHA256:
			checksum_type = G_CHECKSUM_SHA256;
			break;
		default:
			return REJILLA_BURN_ERR;
	}

	rejilla_job_set_current_action (REJILLA_JOB (self),
					REJILLA_BURN_ACTION_CHECKSUM,
					_("Creating image checksum"),
					FALSE);
	rejilla_job_start_progress (REJILLA_JOB (self), FALSE);
	rejilla_job_get_current_track (REJILLA_JOB (self), &track);

	/* see if another plugin is sending us data to checksum
	 * or if we do it ourself (and then that must be from an
	 * image file only). */
	if (rejilla_job_get_fd_in (REJILLA_JOB (self), NULL) == REJILLA_BURN_OK) {
		RejillaMedium *medium;
		GValue *value = NULL;
		RejillaDrive *drive;
		guint64 start, end;
		goffset sectors;
		goffset bytes;

		rejilla_track_tag_lookup (track,
					  REJILLA_TRACK_MEDIUM_ADDRESS_START_TAG,
					  &value);

		/* we were given an address to start */
		start = g_value_get_uint64 (value);

		/* get the length now */
		value = NULL;
		rejilla_track_tag_lookup (track,
					  REJILLA_TRACK_MEDIUM_ADDRESS_END_TAG,
					  &value);

		end = g_value_get_uint64 (value);

		priv->total = end - start;

		/* we're only able to checksum ISO format at the moment so that
		 * means we can only handle last session */
		drive = rejilla_track_disc_get_drive (REJILLA_TRACK_DISC (track));
		medium = rejilla_drive_get_medium (drive);
		rejilla_medium_get_last_data_track_space (medium,
							  &bytes,
							  &sectors);

		/* That's the only way to get the sector size */
		priv->total *= bytes / sectors;

		return rejilla_checksum_image_checksum_fd_input (self, checksum_type, error);
	}
	else {
		result = rejilla_track_get_size (track,
						 NULL,
						 &priv->total);
		if (result != REJILLA_BURN_OK)
			return result;

		return rejilla_checksum_image_checksum_file_input (self, checksum_type, error);
	}

	return REJILLA_BURN_OK;
}

static RejillaChecksumType
rejilla_checksum_get_checksum_type (void)
{
	GSettings *settings;
	GChecksumType checksum_type;

	settings = g_settings_new (REJILLA_SCHEMA_CONFIG);
	checksum_type = g_settings_get_int (settings, REJILLA_PROPS_CHECKSUM_IMAGE);
	g_object_unref (settings);

	return checksum_type;
}

static RejillaBurnResult
rejilla_checksum_image_image_and_checksum (RejillaChecksumImage *self,
					   GError **error)
{
	RejillaBurnResult result;
	GChecksumType checksum_type;
	RejillaChecksumImagePrivate *priv;

	priv = REJILLA_CHECKSUM_IMAGE_PRIVATE (self);

	priv->checksum_type = rejilla_checksum_get_checksum_type ();

	if (priv->checksum_type & REJILLA_CHECKSUM_MD5)
		checksum_type = G_CHECKSUM_MD5;
	else if (priv->checksum_type & REJILLA_CHECKSUM_SHA1)
		checksum_type = G_CHECKSUM_SHA1;
	else if (priv->checksum_type & REJILLA_CHECKSUM_SHA256)
		checksum_type = G_CHECKSUM_SHA256;
	else {
		checksum_type = G_CHECKSUM_MD5;
		priv->checksum_type = REJILLA_CHECKSUM_MD5;
	}

	rejilla_job_set_current_action (REJILLA_JOB (self),
					REJILLA_BURN_ACTION_CHECKSUM,
					_("Creating image checksum"),
					FALSE);
	rejilla_job_start_progress (REJILLA_JOB (self), FALSE);

	if (rejilla_job_get_fd_in (REJILLA_JOB (self), NULL) != REJILLA_BURN_OK) {
		RejillaTrack *track;

		rejilla_job_get_current_track (REJILLA_JOB (self), &track);
		result = rejilla_track_get_size (track,
						 NULL,
						 &priv->total);
		if (result != REJILLA_BURN_OK)
			return result;

		result = rejilla_checksum_image_checksum_file_input (self,
								     checksum_type,
								     error);
	}
	else
		result = rejilla_checksum_image_checksum_fd_input (self,
								   checksum_type,
								   error);

	return result;
}

struct _RejillaChecksumImageThreadCtx {
	RejillaChecksumImage *sum;
	RejillaBurnResult result;
	GError *error;
};
typedef struct _RejillaChecksumImageThreadCtx RejillaChecksumImageThreadCtx;

static gboolean
rejilla_checksum_image_end (gpointer data)
{
	RejillaChecksumImage *self;
	RejillaTrack *track;
	const gchar *checksum;
	RejillaBurnResult result;
	RejillaChecksumImagePrivate *priv;
	RejillaChecksumImageThreadCtx *ctx;

	ctx = data;
	self = ctx->sum;
	priv = REJILLA_CHECKSUM_IMAGE_PRIVATE (self);

	/* NOTE ctx/data is destroyed in its own callback */
	priv->end_id = 0;

	if (ctx->result != REJILLA_BURN_OK) {
		GError *error;

		error = ctx->error;
		ctx->error = NULL;

		g_checksum_free (priv->checksum);
		priv->checksum = NULL;

		rejilla_job_error (REJILLA_JOB (self), error);
		return FALSE;
	}

	/* we were asked to check the sum of the track so get the type
	 * of the checksum first to see what to do */
	track = NULL;
	rejilla_job_get_current_track (REJILLA_JOB (self), &track);

	/* Set the checksum for the track and at the same time compare it to a
	 * potential previous one. */
	checksum = g_checksum_get_string (priv->checksum);
	REJILLA_JOB_LOG (self,
			 "Setting new checksum (type = %i) %s (%s before)",
			 priv->checksum_type,
			 checksum,
			 rejilla_track_get_checksum (track));
	result = rejilla_track_set_checksum (track,
					     priv->checksum_type,
					     checksum);
	g_checksum_free (priv->checksum);
	priv->checksum = NULL;

	if (result != REJILLA_BURN_OK)
		goto error;

	rejilla_job_finished_track (REJILLA_JOB (self));
	return FALSE;

error:
{
	GError *error = NULL;

	error = g_error_new (REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_BAD_CHECKSUM,
			     _("Some files may be corrupted on the disc"));
	rejilla_job_error (REJILLA_JOB (self), error);
	return FALSE;
}
}

static void
rejilla_checksum_image_destroy (gpointer data)
{
	RejillaChecksumImageThreadCtx *ctx;

	ctx = data;
	if (ctx->error) {
		g_error_free (ctx->error);
		ctx->error = NULL;
	}

	g_free (ctx);
}

static gpointer
rejilla_checksum_image_thread (gpointer data)
{
	GError *error = NULL;
	RejillaJobAction action;
	RejillaTrack *track = NULL;
	RejillaChecksumImage *self;
	RejillaChecksumImagePrivate *priv;
	RejillaChecksumImageThreadCtx *ctx;
	RejillaBurnResult result = REJILLA_BURN_NOT_SUPPORTED;

	self = REJILLA_CHECKSUM_IMAGE (data);
	priv = REJILLA_CHECKSUM_IMAGE_PRIVATE (self);

	/* check DISC types and add checksums for DATA and IMAGE-bin types */
	rejilla_job_get_action (REJILLA_JOB (self), &action);
	rejilla_job_get_current_track (REJILLA_JOB (self), &track);

	if (action == REJILLA_JOB_ACTION_CHECKSUM) {
		priv->checksum_type = rejilla_track_get_checksum_type (track);
		if (priv->checksum_type & (REJILLA_CHECKSUM_MD5|REJILLA_CHECKSUM_SHA1|REJILLA_CHECKSUM_SHA256))
			result = rejilla_checksum_image_create_checksum (self, &error);
		else
			result = REJILLA_BURN_ERR;
	}
	else if (action == REJILLA_JOB_ACTION_IMAGE) {
		RejillaTrackType *input;

		input = rejilla_track_type_new ();
		rejilla_job_get_input_type (REJILLA_JOB (self), input);

		if (rejilla_track_type_get_has_image (input))
			result = rejilla_checksum_image_image_and_checksum (self, &error);
		else
			result = REJILLA_BURN_ERR;

		rejilla_track_type_free (input);
	}

	if (result != REJILLA_BURN_CANCEL) {
		ctx = g_new0 (RejillaChecksumImageThreadCtx, 1);
		ctx->sum = self;
		ctx->error = error;
		ctx->result = result;
		priv->end_id = g_idle_add_full (G_PRIORITY_HIGH_IDLE,
						rejilla_checksum_image_end,
						ctx,
						rejilla_checksum_image_destroy);
	}

	/* End thread */
	g_mutex_lock (priv->mutex);
	priv->thread = NULL;
	g_cond_signal (priv->cond);
	g_mutex_unlock (priv->mutex);

	g_thread_exit (NULL);
	return NULL;
}

static RejillaBurnResult
rejilla_checksum_image_start (RejillaJob *job,
			      GError **error)
{
	RejillaChecksumImagePrivate *priv;
	GError *thread_error = NULL;
	RejillaJobAction action;

	rejilla_job_get_action (job, &action);
	if (action == REJILLA_JOB_ACTION_SIZE) {
		/* say we won't write to disc if we're just checksuming "live" */
		if (rejilla_job_get_fd_in (job, NULL) == REJILLA_BURN_OK)
			return REJILLA_BURN_NOT_SUPPORTED;

		/* otherwise return an output of 0 since we're not actually 
		 * writing anything to the disc. That will prevent a disc space
		 * failure. */
		rejilla_job_set_output_size_for_current_track (job, 0, 0);
		return REJILLA_BURN_NOT_RUNNING;
	}

	/* we start a thread for the exploration of the graft points */
	priv = REJILLA_CHECKSUM_IMAGE_PRIVATE (job);
	g_mutex_lock (priv->mutex);
	priv->thread = g_thread_create (rejilla_checksum_image_thread,
					REJILLA_CHECKSUM_IMAGE (job),
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

static RejillaBurnResult
rejilla_checksum_image_activate (RejillaJob *job,
				 GError **error)
{
	RejillaBurnFlag flags = REJILLA_BURN_FLAG_NONE;
	RejillaTrack *track = NULL;
	RejillaJobAction action;

	rejilla_job_get_current_track (job, &track);
	rejilla_job_get_action (job, &action);

	if (action == REJILLA_JOB_ACTION_IMAGE
	&&  rejilla_track_get_checksum_type (track) != REJILLA_CHECKSUM_NONE
	&&  rejilla_track_get_checksum_type (track) == rejilla_checksum_get_checksum_type ()) {
		REJILLA_JOB_LOG (job,
				 "There is a checksum already %d",
				 rejilla_track_get_checksum_type (track));
		/* if there is a checksum already, if so no need to redo one */
		return REJILLA_BURN_NOT_RUNNING;
	}

	flags = REJILLA_BURN_FLAG_NONE;
	rejilla_job_get_flags (job, &flags);
	if (flags & REJILLA_BURN_FLAG_DUMMY) {
		REJILLA_JOB_LOG (job, "Dummy operation, skipping");
		return REJILLA_BURN_NOT_RUNNING;
	}

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_checksum_image_clock_tick (RejillaJob *job)
{
	RejillaChecksumImagePrivate *priv;

	priv = REJILLA_CHECKSUM_IMAGE_PRIVATE (job);

	if (!priv->checksum)
		return REJILLA_BURN_OK;

	if (!priv->total)
		return REJILLA_BURN_OK;

	rejilla_job_start_progress (job, FALSE);
	rejilla_job_set_progress (job,
				  (gdouble) priv->bytes /
				  (gdouble) priv->total);

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_checksum_image_stop (RejillaJob *job,
			     GError **error)
{
	RejillaChecksumImagePrivate *priv;

	priv = REJILLA_CHECKSUM_IMAGE_PRIVATE (job);

	g_mutex_lock (priv->mutex);
	if (priv->thread) {
		priv->cancel = 1;
		g_cond_wait (priv->cond, priv->mutex);
		priv->cancel = 0;
		priv->thread = NULL;
	}
	g_mutex_unlock (priv->mutex);

	if (priv->end_id) {
		g_source_remove (priv->end_id);
		priv->end_id = 0;
	}

	if (priv->checksum) {
		g_checksum_free (priv->checksum);
		priv->checksum = NULL;
	}

	return REJILLA_BURN_OK;
}

static void
rejilla_checksum_image_init (RejillaChecksumImage *obj)
{
	RejillaChecksumImagePrivate *priv;

	priv = REJILLA_CHECKSUM_IMAGE_PRIVATE (obj);

	priv->mutex = g_mutex_new ();
	priv->cond = g_cond_new ();
}

static void
rejilla_checksum_image_finalize (GObject *object)
{
	RejillaChecksumImagePrivate *priv;
	
	priv = REJILLA_CHECKSUM_IMAGE_PRIVATE (object);

	g_mutex_lock (priv->mutex);
	if (priv->thread) {
		priv->cancel = 1;
		g_cond_wait (priv->cond, priv->mutex);
		priv->cancel = 0;
		priv->thread = NULL;
	}
	g_mutex_unlock (priv->mutex);

	if (priv->end_id) {
		g_source_remove (priv->end_id);
		priv->end_id = 0;
	}

	if (priv->checksum) {
		g_checksum_free (priv->checksum);
		priv->checksum = NULL;
	}

	if (priv->mutex) {
		g_mutex_free (priv->mutex);
		priv->mutex = NULL;
	}

	if (priv->cond) {
		g_cond_free (priv->cond);
		priv->cond = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
rejilla_checksum_image_class_init (RejillaChecksumImageClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	RejillaJobClass *job_class = REJILLA_JOB_CLASS (klass);

	g_type_class_add_private (klass, sizeof (RejillaChecksumImagePrivate));

	parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = rejilla_checksum_image_finalize;

	job_class->activate = rejilla_checksum_image_activate;
	job_class->start = rejilla_checksum_image_start;
	job_class->stop = rejilla_checksum_image_stop;
	job_class->clock_tick = rejilla_checksum_image_clock_tick;
}

static void
rejilla_checksum_image_export_caps (RejillaPlugin *plugin)
{
	GSList *input;
	RejillaPluginConfOption *checksum_type;

	rejilla_plugin_define (plugin,
	                       "image-checksum",
			       /* Translators: this is the name of the plugin
				* which will be translated only when it needs
				* displaying. */
			       N_("Image Checksum"),
			       _("Checks disc integrity after it is burnt"),
			       "Philippe Rouquier",
			       0);

	/* For images we can process (thus generating a sum on the fly or simply
	 * test images). */
	input = rejilla_caps_image_new (REJILLA_PLUGIN_IO_ACCEPT_FILE|
					REJILLA_PLUGIN_IO_ACCEPT_PIPE,
					REJILLA_IMAGE_FORMAT_BIN);
	rejilla_plugin_process_caps (plugin, input);

	rejilla_plugin_set_process_flags (plugin,
					  REJILLA_PLUGIN_RUN_PREPROCESSING|
					  REJILLA_PLUGIN_RUN_BEFORE_TARGET);

	rejilla_plugin_check_caps (plugin,
				   REJILLA_CHECKSUM_MD5|
				   REJILLA_CHECKSUM_SHA1|
				   REJILLA_CHECKSUM_SHA256,
				   input);
	g_slist_free (input);

	/* add some configure options */
	checksum_type = rejilla_plugin_conf_option_new (REJILLA_PROPS_CHECKSUM_IMAGE,
							_("Hashing algorithm to be used:"),
							REJILLA_PLUGIN_OPTION_CHOICE);
	rejilla_plugin_conf_option_choice_add (checksum_type,
					       _("MD5"), REJILLA_CHECKSUM_MD5);
	rejilla_plugin_conf_option_choice_add (checksum_type,
					       _("SHA1"), REJILLA_CHECKSUM_SHA1);
	rejilla_plugin_conf_option_choice_add (checksum_type,
					       _("SHA256"), REJILLA_CHECKSUM_SHA256);

	rejilla_plugin_add_conf_option (plugin, checksum_type);

	rejilla_plugin_set_compulsory (plugin, FALSE);
}
