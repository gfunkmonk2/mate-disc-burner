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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <time.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>
#include <gmodule.h>

#include "rejilla-units.h"

#include "burn-debug.h"
#include "burn-job.h"
#include "burn-process.h"
#include "rejilla-plugin-registration.h"
#include "burn-cdrtools.h"
#include "rejilla-track-disc.h"
#include "rejilla-track-stream.h"

#define REJILLA_TYPE_CDDA2WAV         (rejilla_cdda2wav_get_type ())
#define REJILLA_CDDA2WAV(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), REJILLA_TYPE_CDDA2WAV, RejillaCdda2wav))
#define REJILLA_CDDA2WAV_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), REJILLA_TYPE_CDDA2WAV, RejillaCdda2wavClass))
#define REJILLA_IS_CDDA2WAV(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), REJILLA_TYPE_CDDA2WAV))
#define REJILLA_IS_CDDA2WAV_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), REJILLA_TYPE_CDDA2WAV))
#define REJILLA_CDDA2WAV_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), REJILLA_TYPE_CDDA2WAV, RejillaCdda2wavClass))

REJILLA_PLUGIN_BOILERPLATE (RejillaCdda2wav, rejilla_cdda2wav, REJILLA_TYPE_PROCESS, RejillaProcess);

struct _RejillaCdda2wavPrivate {
	gchar *file_pattern;

	guint track_num;
	guint track_nb;

	guint is_inf	:1;
};
typedef struct _RejillaCdda2wavPrivate RejillaCdda2wavPrivate;

#define REJILLA_CDDA2WAV_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REJILLA_TYPE_CDDA2WAV, RejillaCdda2wavPrivate))
static GObjectClass *parent_class = NULL;


static RejillaBurnResult
rejilla_cdda2wav_post (RejillaJob *job)
{
	RejillaCdda2wavPrivate *priv;
	RejillaMedium *medium;
	RejillaJobAction action;
	RejillaDrive *drive;
	RejillaTrack *track;
	int track_num;
	int i;

	priv = REJILLA_CDDA2WAV_PRIVATE (job);

	rejilla_job_get_action (job, &action);
	if (action == REJILLA_JOB_ACTION_SIZE)
		return REJILLA_BURN_OK;

	/* we add the tracks */
	track = NULL;
	rejilla_job_get_current_track (job, &track);

	drive = rejilla_track_disc_get_drive (REJILLA_TRACK_DISC (track));
	medium = rejilla_drive_get_medium (drive);

	track_num = rejilla_medium_get_track_num (medium);
	for (i = 0; i < track_num; i ++) {
		RejillaTrackStream *track_stream;
		goffset block_num = 0;

		rejilla_medium_get_track_space (medium, i + 1, NULL, &block_num);
		track_stream = rejilla_track_stream_new ();
		rejilla_track_stream_set_boundaries (track_stream,
		                                     0,
		                                     REJILLA_BYTES_TO_DURATION (block_num * 2352),
		                                     0);

		rejilla_track_stream_set_format (track_stream,
		                                 REJILLA_AUDIO_FORMAT_RAW|
		                                 REJILLA_METADATA_INFO);

		REJILLA_JOB_LOG (job, "Adding new audio track of size %" G_GOFFSET_FORMAT, REJILLA_BYTES_TO_DURATION (block_num * 2352));

		/* either add .inf or .cdr files */
		if (!priv->is_inf) {
			gchar *uri;
			gchar *filename;

			if (track_num == 1)
				filename = g_strdup_printf ("%s.cdr", priv->file_pattern);
			else
				filename = g_strdup_printf ("%s_%02i.cdr", priv->file_pattern, i + 1);

			uri = g_filename_to_uri (filename, NULL, NULL);
			g_free (filename);

			rejilla_track_stream_set_source (track_stream, uri);
			g_free (uri);

			/* signal to cdrecord that we have an .inf file */
			if (i != 0)
				filename = g_strdup_printf ("%s_%02i.inf", priv->file_pattern, i);
			else
				filename = g_strdup_printf ("%s.inf", priv->file_pattern);

			rejilla_track_tag_add_string (REJILLA_TRACK (track_stream),
			                              REJILLA_CDRTOOLS_TRACK_INF_FILE,
			                              filename);
			g_free (filename);
		}

		rejilla_job_add_track (job, REJILLA_TRACK (track_stream));
		g_object_unref (track_stream);
	}

	return rejilla_job_finished_session (job);
}

static gboolean
rejilla_cdda2wav_get_output_filename_pattern (RejillaCdda2wav *cdda2wav,
                                              GError **error)
{
	gchar *path;
	gchar *file_pattern;
	RejillaCdda2wavPrivate *priv;

	priv = REJILLA_CDDA2WAV_PRIVATE (cdda2wav);

	if (priv->file_pattern) {
		g_free (priv->file_pattern);
		priv->file_pattern = NULL;
	}

	/* Create a tmp directory so cdda2wav can
	 * put all its stuff in there */
	path = NULL;
	rejilla_job_get_tmp_dir (REJILLA_JOB (cdda2wav), &path, error);
	if (!path)
		return FALSE;

	file_pattern = g_strdup_printf ("%s/cd_file", path);
	g_free (path);

	/* NOTE: this file pattern is used to
	 * name all wav and inf files. It is followed
	 * by underscore/number of the track/extension */

	priv->file_pattern = file_pattern;
	return TRUE;
}

static RejillaBurnResult
rejilla_cdda2wav_read_stderr (RejillaProcess *process, const gchar *line)
{
	int num;
	RejillaCdda2wav *cdda2wav;
	RejillaCdda2wavPrivate *priv;

	cdda2wav = REJILLA_CDDA2WAV (process);
	priv = REJILLA_CDDA2WAV_PRIVATE (process);

	if (sscanf (line, "100%%  track %d '%*s' recorded successfully", &num) == 1) {
		gchar *string;

		priv->track_nb = num;
		string = g_strdup_printf (_("Copying audio track %02d"), priv->track_nb + 1);
		rejilla_job_set_current_action (REJILLA_JOB (process),
		                                REJILLA_BURN_ACTION_DRIVE_COPY,
		                                string,
		                                TRUE);
		g_free (string);
	}
	else if (strstr (line, "percent_done:")) {
		gchar *string;

		string = g_strdup_printf (_("Copying audio track %02d"), 1);
		rejilla_job_set_current_action (REJILLA_JOB (process),
		                                REJILLA_BURN_ACTION_DRIVE_COPY,
		                                string,
		                                TRUE);
		g_free (string);
	}
	/* we have to do this otherwise with sscanf it will 
	 * match every time it begins with a number */
	else if (strchr (line, '%') && sscanf (line, " %d%%", &num) == 1) {
		gdouble fraction;

		fraction = (gdouble) num / (gdouble) 100.0;
		fraction = ((gdouble) priv->track_nb + fraction) / (gdouble) priv->track_num;
		rejilla_job_set_progress (REJILLA_JOB (cdda2wav), fraction);
		rejilla_job_start_progress (REJILLA_JOB (process), FALSE);
	}

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_cdda2wav_set_argv_image (RejillaCdda2wav *cdda2wav,
				GPtrArray *argv,
				GError **error)
{
	RejillaCdda2wavPrivate *priv;
	int fd_out;

	priv = REJILLA_CDDA2WAV_PRIVATE (cdda2wav);

	/* We want raw output */
	g_ptr_array_add (argv, g_strdup ("output-format=cdr"));

	/* we want all tracks */
	g_ptr_array_add (argv, g_strdup ("-B"));

	priv->is_inf = FALSE;

	if (rejilla_job_get_fd_out (REJILLA_JOB (cdda2wav), &fd_out) == REJILLA_BURN_OK) {
		/* On the fly copying */
		g_ptr_array_add (argv, g_strdup ("-"));
	}
	else {
		if (!rejilla_cdda2wav_get_output_filename_pattern (cdda2wav, error))
			return REJILLA_BURN_ERR;

		g_ptr_array_add (argv, g_strdup (priv->file_pattern));

		rejilla_job_set_current_action (REJILLA_JOB (cdda2wav),
		                                REJILLA_BURN_ACTION_DRIVE_COPY,
		                                _("Preparing to copy audio disc"),
		                                FALSE);
	}

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_cdda2wav_set_argv_size (RejillaCdda2wav *cdda2wav,
                                GPtrArray *argv,
                                GError **error)
{
	RejillaCdda2wavPrivate *priv;
	RejillaMedium *medium;
	RejillaTrack *track;
	RejillaDrive *drive;
	goffset medium_len;
	int i;

	priv = REJILLA_CDDA2WAV_PRIVATE (cdda2wav);

	/* we add the tracks */
	medium_len = 0;
	track = NULL;
	rejilla_job_get_current_track (REJILLA_JOB (cdda2wav), &track);

	drive = rejilla_track_disc_get_drive (REJILLA_TRACK_DISC (track));
	medium = rejilla_drive_get_medium (drive);

	priv->track_num = rejilla_medium_get_track_num (medium);
	for (i = 0; i < priv->track_num; i ++) {
		goffset len = 0;

		rejilla_medium_get_track_space (medium, i, NULL, &len);
		medium_len += len;
	}
	rejilla_job_set_output_size_for_current_track (REJILLA_JOB (cdda2wav), medium_len, medium_len * 2352);

	/* if there isn't any output file then that
	 * means we have to generate all the
	 * .inf files for cdrecord. */
	if (rejilla_job_get_audio_output (REJILLA_JOB (cdda2wav), NULL) != REJILLA_BURN_OK)
		return REJILLA_BURN_NOT_RUNNING;

	/* we want all tracks */
	g_ptr_array_add (argv, g_strdup ("-B"));

	/* since we're running for an on-the-fly burning
	 * we only want the .inf files */
	g_ptr_array_add (argv, g_strdup ("-J"));

	if (!rejilla_cdda2wav_get_output_filename_pattern (cdda2wav, error))
		return REJILLA_BURN_ERR;

	g_ptr_array_add (argv, g_strdup (priv->file_pattern));

	priv->is_inf = TRUE;

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_cdda2wav_set_argv (RejillaProcess *process,
			  GPtrArray *argv,
			  GError **error)
{
	RejillaDrive *drive;
	const gchar *device;
	RejillaTrack *track;
	RejillaJobAction action;
	RejillaBurnResult result;
	RejillaCdda2wav *cdda2wav;
	RejillaCdda2wavPrivate *priv;

	cdda2wav = REJILLA_CDDA2WAV (process);
	priv = REJILLA_CDDA2WAV_PRIVATE (process);

	g_ptr_array_add (argv, g_strdup ("cdda2wav"));

	/* Add the device path */
	track = NULL;
	result = rejilla_job_get_current_track (REJILLA_JOB (process), &track);
	if (!track)
		return result;

	drive = rejilla_track_disc_get_drive (REJILLA_TRACK_DISC (track));
	device = rejilla_drive_get_device (drive);
	g_ptr_array_add (argv, g_strdup_printf ("dev=%s", device));

	/* Have it talking */
	g_ptr_array_add (argv, g_strdup ("-v255"));

	rejilla_job_get_action (REJILLA_JOB (cdda2wav), &action);
	if (action == REJILLA_JOB_ACTION_SIZE)
		result = rejilla_cdda2wav_set_argv_size (cdda2wav, argv, error);
	else if (action == REJILLA_JOB_ACTION_IMAGE)
		result = rejilla_cdda2wav_set_argv_image (cdda2wav, argv, error);
	else
		REJILLA_JOB_NOT_SUPPORTED (cdda2wav);

	return result;
}

static void
rejilla_cdda2wav_class_init (RejillaCdda2wavClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	RejillaProcessClass *process_class = REJILLA_PROCESS_CLASS (klass);

	g_type_class_add_private (klass, sizeof (RejillaCdda2wavPrivate));

	parent_class = g_type_class_peek_parent(klass);
	object_class->finalize = rejilla_cdda2wav_finalize;

	process_class->stderr_func = rejilla_cdda2wav_read_stderr;
	process_class->set_argv = rejilla_cdda2wav_set_argv;
	process_class->post = rejilla_cdda2wav_post;
}

static void
rejilla_cdda2wav_init (RejillaCdda2wav *obj)
{ }

static void
rejilla_cdda2wav_finalize (GObject *object)
{
	RejillaCdda2wavPrivate *priv;

	priv = REJILLA_CDDA2WAV_PRIVATE (object);

	if (priv->file_pattern) {
		g_free (priv->file_pattern);
		priv->file_pattern = NULL;
	}
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
rejilla_cdda2wav_export_caps (RejillaPlugin *plugin)
{
	GSList *output;
	GSList *input;

	rejilla_plugin_define (plugin,
			       "cdda2wav",
	                       NULL,
			       _("Copy tracks from an audio CD with all associated information"),
			       "Philippe Rouquier",
			       1);

	output = rejilla_caps_audio_new (REJILLA_PLUGIN_IO_ACCEPT_FILE /*|REJILLA_PLUGIN_IO_ACCEPT_PIPE*/, /* Keep on the fly on hold until it gets proper testing */
					 REJILLA_AUDIO_FORMAT_RAW|
	                                 REJILLA_METADATA_INFO);

	input = rejilla_caps_disc_new (REJILLA_MEDIUM_CDR|
	                               REJILLA_MEDIUM_CDRW|
	                               REJILLA_MEDIUM_CDROM|
	                               REJILLA_MEDIUM_CLOSED|
	                               REJILLA_MEDIUM_APPENDABLE|
	                               REJILLA_MEDIUM_HAS_AUDIO);

	rejilla_plugin_link_caps (plugin, output, input);
	g_slist_free (output);
	g_slist_free (input);

	rejilla_plugin_register_group (plugin, _(CDRTOOLS_DESCRIPTION));
}

G_MODULE_EXPORT void
rejilla_plugin_check_config (RejillaPlugin *plugin)
{
	gchar *prog_path;

	/* Just check that the program is in the path and executable. */
	prog_path = g_find_program_in_path ("cdda2wav");
	if (!prog_path) {
		rejilla_plugin_add_error (plugin,
		                          REJILLA_PLUGIN_ERROR_MISSING_APP,
		                          "cdda2wav");
		return;
	}

	if (!g_file_test (prog_path, G_FILE_TEST_IS_EXECUTABLE)) {
		g_free (prog_path);
		rejilla_plugin_add_error (plugin,
		                          REJILLA_PLUGIN_ERROR_MISSING_APP,
		                          "cdda2wav");
		return;
	}
	g_free (prog_path);

	/* This is what it should be. Now, as I did not write and icedax plugin
	 * the above is enough so that cdda2wav can use a symlink to icedax.
	 * As for the version checking, it becomes impossible given that with
	 * icedax the string would not start with cdda2wav. So ... */
	/*
	gint version [3] = { 2, 0, -1};
	rejilla_plugin_test_app (plugin,
	                         "cdda2wav",
	                         "--version",
	                         "cdda2wav %d.%d",
	                         version);
	*/
}
