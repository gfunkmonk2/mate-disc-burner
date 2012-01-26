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

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>
#include <gmodule.h>

#include "rejilla-error.h"
#include "rejilla-plugin-registration.h"
#include "burn-job.h"
#include "burn-process.h"
#include "rejilla-track-disc.h"
#include "rejilla-track-image.h"
#include "rejilla-drive.h"
#include "rejilla-medium.h"

#define CDRDAO_DESCRIPTION		N_("cdrdao burning suite")

#define REJILLA_TYPE_CDRDAO         (rejilla_cdrdao_get_type ())
#define REJILLA_CDRDAO(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), REJILLA_TYPE_CDRDAO, RejillaCdrdao))
#define REJILLA_CDRDAO_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), REJILLA_TYPE_CDRDAO, RejillaCdrdaoClass))
#define REJILLA_IS_CDRDAO(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), REJILLA_TYPE_CDRDAO))
#define REJILLA_IS_CDRDAO_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), REJILLA_TYPE_CDRDAO))
#define REJILLA_CDRDAO_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), REJILLA_TYPE_CDRDAO, RejillaCdrdaoClass))

REJILLA_PLUGIN_BOILERPLATE (RejillaCdrdao, rejilla_cdrdao, REJILLA_TYPE_PROCESS, RejillaProcess);

struct _RejillaCdrdaoPrivate {
 	gchar *tmp_toc_path;
	guint use_raw:1;
};
typedef struct _RejillaCdrdaoPrivate RejillaCdrdaoPrivate;
#define REJILLA_CDRDAO_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), REJILLA_TYPE_CDRDAO, RejillaCdrdaoPrivate)) 

static GObjectClass *parent_class = NULL;

#define REJILLA_SCHEMA_CONFIG		"org.mate.rejilla.config"
#define REJILLA_KEY_RAW_FLAG		"raw-flag"

static gboolean
rejilla_cdrdao_read_stderr_image (RejillaCdrdao *cdrdao, const gchar *line)
{
	int min, sec, sub, s1;

	if (sscanf (line, "%d:%d:%d", &min, &sec, &sub) == 3) {
		guint64 secs = min * 60 + sec;

		rejilla_job_set_written_track (REJILLA_JOB (cdrdao), secs * 75 * 2352);
		if (secs > 2)
			rejilla_job_start_progress (REJILLA_JOB (cdrdao), FALSE);
	}
	else if (sscanf (line, "Leadout %*s %*d %d:%d:%*d(%i)", &min, &sec, &s1) == 3) {
		RejillaJobAction action;

		rejilla_job_get_action (REJILLA_JOB (cdrdao), &action);
		if (action == REJILLA_JOB_ACTION_SIZE) {
			/* get the number of sectors. As we added -raw sector = 2352 bytes */
			rejilla_job_set_output_size_for_current_track (REJILLA_JOB (cdrdao), s1, (gint64) s1 * 2352ULL);
			rejilla_job_finished_session (REJILLA_JOB (cdrdao));
		}
	}
	else if (strstr (line, "Copying audio tracks")) {
		rejilla_job_set_current_action (REJILLA_JOB (cdrdao),
						REJILLA_BURN_ACTION_DRIVE_COPY,
						_("Copying audio track"),
						FALSE);
	}
	else if (strstr (line, "Copying data track")) {
		rejilla_job_set_current_action (REJILLA_JOB (cdrdao),
						REJILLA_BURN_ACTION_DRIVE_COPY,
						_("Copying data track"),
						FALSE);
	}
	else
		return FALSE;

	return TRUE;
}

static gboolean
rejilla_cdrdao_read_stderr_record (RejillaCdrdao *cdrdao, const gchar *line)
{
	int fifo, track, min, sec;
	guint written, total;

	if (sscanf (line, "Wrote %u of %u (Buffers %d%%  %*s", &written, &total, &fifo) >= 2) {
		rejilla_job_set_dangerous (REJILLA_JOB (cdrdao), TRUE);

		rejilla_job_set_written_session (REJILLA_JOB (cdrdao), written * 1048576);
		rejilla_job_set_current_action (REJILLA_JOB (cdrdao),
						REJILLA_BURN_ACTION_RECORDING,
						NULL,
						FALSE);

		rejilla_job_start_progress (REJILLA_JOB (cdrdao), FALSE);
	}
	else if (sscanf (line, "Wrote %*s blocks. Buffer fill min") == 1) {
		/* this is for fixating phase */
		rejilla_job_set_current_action (REJILLA_JOB (cdrdao),
						REJILLA_BURN_ACTION_FIXATING,
						NULL,
						FALSE);
	}
	else if (sscanf (line, "Analyzing track %d %*s start %d:%d:%*d, length %*d:%*d:%*d", &track, &min, &sec) == 3) {
		gchar *string;

		string = g_strdup_printf (_("Analysing track %02i"), track);
		rejilla_job_set_current_action (REJILLA_JOB (cdrdao),
						REJILLA_BURN_ACTION_ANALYSING,
						string,
						TRUE);
		g_free (string);
	}
	else if (sscanf (line, "%d:%d:%*d", &min, &sec) == 2) {
		gint64 written;
		guint64 secs = min * 60 + sec;

		if (secs > 2)
			rejilla_job_start_progress (REJILLA_JOB (cdrdao), FALSE);

		written = secs * 75 * 2352;
		rejilla_job_set_written_session (REJILLA_JOB (cdrdao), written);
	}
	else if (strstr (line, "Writing track")) {
		rejilla_job_set_dangerous (REJILLA_JOB (cdrdao), TRUE);
	}
	else if (strstr (line, "Writing finished successfully")
	     ||  strstr (line, "On-the-fly CD copying finished successfully")) {
		rejilla_job_set_dangerous (REJILLA_JOB (cdrdao), FALSE);
	}
	else if (strstr (line, "Blanking disk...")) {
		rejilla_job_set_current_action (REJILLA_JOB (cdrdao),
						REJILLA_BURN_ACTION_BLANKING,
						NULL,
						FALSE);
		rejilla_job_start_progress (REJILLA_JOB (cdrdao), FALSE);
		rejilla_job_set_dangerous (REJILLA_JOB (cdrdao), TRUE);
	}
	else {
		gchar *name = NULL;
		gchar *cuepath = NULL;
		RejillaTrack *track = NULL;
		RejillaJobAction action;

		/* Try to catch error could not find cue file */

		/* Track could be NULL here if we're simply blanking a medium */
		rejilla_job_get_action (REJILLA_JOB (cdrdao), &action);
		if (action == REJILLA_JOB_ACTION_ERASE)
			return TRUE;

		rejilla_job_get_current_track (REJILLA_JOB (cdrdao), &track);
		if (!track)
			return FALSE;

		cuepath = rejilla_track_image_get_toc_source (REJILLA_TRACK_IMAGE (track), FALSE);

		if (!cuepath)
			return FALSE;

		if (!strstr (line, "ERROR: Could not find input file")) {
			g_free (cuepath);
			return FALSE;
		}

		name = g_path_get_basename (cuepath);
		g_free (cuepath);

		rejilla_job_error (REJILLA_JOB (cdrdao),
				   g_error_new (REJILLA_BURN_ERROR,
						REJILLA_BURN_ERROR_FILE_NOT_FOUND,
						/* Translators: %s is a filename */
						_("\"%s\" could not be found"),
						name));
		g_free (name);
	}

	return FALSE;
}

static RejillaBurnResult
rejilla_cdrdao_read_stderr (RejillaProcess *process, const gchar *line)
{
	RejillaCdrdao *cdrdao;
	gboolean result = FALSE;
	RejillaJobAction action;

	cdrdao = REJILLA_CDRDAO (process);

	rejilla_job_get_action (REJILLA_JOB (cdrdao), &action);
	if (action == REJILLA_JOB_ACTION_RECORD
	||  action == REJILLA_JOB_ACTION_ERASE)
		result = rejilla_cdrdao_read_stderr_record (cdrdao, line);
	else if (action == REJILLA_JOB_ACTION_IMAGE
	     ||  action == REJILLA_JOB_ACTION_SIZE)
		result = rejilla_cdrdao_read_stderr_image (cdrdao, line);

	if (result)
		return REJILLA_BURN_OK;

	if (strstr (line, "Cannot setup device")) {
		rejilla_job_error (REJILLA_JOB (cdrdao),
				   g_error_new (REJILLA_BURN_ERROR,
						REJILLA_BURN_ERROR_DRIVE_BUSY,
						_("The drive is busy")));
	}
	else if (strstr (line, "Operation not permitted. Cannot send SCSI")) {
		rejilla_job_error (REJILLA_JOB (cdrdao),
				   g_error_new (REJILLA_BURN_ERROR,
						REJILLA_BURN_ERROR_PERMISSION,
						_("You do not have the required permissions to use this drive")));
	}

	return REJILLA_BURN_OK;
}

static void
rejilla_cdrdao_set_argv_device (RejillaCdrdao *cdrdao,
				GPtrArray *argv)
{
	gchar *device = NULL;

	g_ptr_array_add (argv, g_strdup ("--device"));

	/* NOTE: that function returns either bus_target_lun or the device path
	 * according to OSes. Basically it returns bus/target/lun only for FreeBSD
	 * which is the only OS in need for that. For all others it returns the device
	 * path. */
	rejilla_job_get_bus_target_lun (REJILLA_JOB (cdrdao), &device);
	g_ptr_array_add (argv, device);
}

static void
rejilla_cdrdao_set_argv_common_rec (RejillaCdrdao *cdrdao,
				    GPtrArray *argv)
{
	RejillaBurnFlag flags;
	gchar *speed_str;
	guint speed;

	rejilla_job_get_flags (REJILLA_JOB (cdrdao), &flags);
	if (flags & REJILLA_BURN_FLAG_DUMMY)
		g_ptr_array_add (argv, g_strdup ("--simulate"));

	g_ptr_array_add (argv, g_strdup ("--speed"));

	rejilla_job_get_speed (REJILLA_JOB (cdrdao), &speed);
	speed_str = g_strdup_printf ("%d", speed);
	g_ptr_array_add (argv, speed_str);

	if (flags & REJILLA_BURN_FLAG_OVERBURN)
		g_ptr_array_add (argv, g_strdup ("--overburn"));
	if (flags & REJILLA_BURN_FLAG_MULTI)
		g_ptr_array_add (argv, g_strdup ("--multi"));
}

static void
rejilla_cdrdao_set_argv_common (RejillaCdrdao *cdrdao,
				GPtrArray *argv)
{
	RejillaBurnFlag flags;

	rejilla_job_get_flags (REJILLA_JOB (cdrdao), &flags);

	/* cdrdao manual says it is a similar option to gracetime */
	if (flags & REJILLA_BURN_FLAG_NOGRACE)
		g_ptr_array_add (argv, g_strdup ("-n"));

	g_ptr_array_add (argv, g_strdup ("-v"));
	g_ptr_array_add (argv, g_strdup ("2"));
}

static RejillaBurnResult
rejilla_cdrdao_set_argv_record (RejillaCdrdao *cdrdao,
				GPtrArray *argv)
{
	RejillaTrackType *type = NULL;
	RejillaCdrdaoPrivate *priv;

	priv = REJILLA_CDRDAO_PRIVATE (cdrdao); 

	g_ptr_array_add (argv, g_strdup ("cdrdao"));

	type = rejilla_track_type_new ();
	rejilla_job_get_input_type (REJILLA_JOB (cdrdao), type);

        if (rejilla_track_type_get_has_medium (type)) {
		RejillaDrive *drive;
		RejillaTrack *track;
		RejillaBurnFlag flags;

		g_ptr_array_add (argv, g_strdup ("copy"));
		rejilla_cdrdao_set_argv_device (cdrdao, argv);
		rejilla_cdrdao_set_argv_common (cdrdao, argv);
		rejilla_cdrdao_set_argv_common_rec (cdrdao, argv);

		rejilla_job_get_flags (REJILLA_JOB (cdrdao), &flags);
		if (flags & REJILLA_BURN_FLAG_NO_TMP_FILES)
			g_ptr_array_add (argv, g_strdup ("--on-the-fly"));

		if (priv->use_raw)
		  	g_ptr_array_add (argv, g_strdup ("--driver generic-mmc-raw")); 

		g_ptr_array_add (argv, g_strdup ("--source-device"));

		rejilla_job_get_current_track (REJILLA_JOB (cdrdao), &track);
		drive = rejilla_track_disc_get_drive (REJILLA_TRACK_DISC (track));

		/* NOTE: that function returns either bus_target_lun or the device path
		 * according to OSes. Basically it returns bus/target/lun only for FreeBSD
		 * which is the only OS in need for that. For all others it returns the device
		 * path. */
		g_ptr_array_add (argv, rejilla_drive_get_bus_target_lun_string (drive));
	}
	else if (rejilla_track_type_get_has_image (type)) {
		gchar *cuepath;
		RejillaTrack *track;

		g_ptr_array_add (argv, g_strdup ("write"));
		
		rejilla_job_get_current_track (REJILLA_JOB (cdrdao), &track);

		if (rejilla_track_type_get_image_format (type) == REJILLA_IMAGE_FORMAT_CUE) {
			gchar *parent;

			cuepath = rejilla_track_image_get_toc_source (REJILLA_TRACK_IMAGE (track), FALSE);
			parent = g_path_get_dirname (cuepath);
			rejilla_process_set_working_directory (REJILLA_PROCESS (cdrdao), parent);
			g_free (parent);

			/* This does not work as toc2cue will use BINARY even if
			 * if endianness is big endian */
			/* we need to check endianness */
			/* if (rejilla_track_image_need_byte_swap (REJILLA_TRACK_IMAGE (track)))
				g_ptr_array_add (argv, g_strdup ("--swap")); */
		}
		else if (rejilla_track_type_get_image_format (type) == REJILLA_IMAGE_FORMAT_CDRDAO) {
			/* CDRDAO files are always BIG ENDIAN */
			cuepath = rejilla_track_image_get_toc_source (REJILLA_TRACK_IMAGE (track), FALSE);
		}
		else {
			rejilla_track_type_free (type);
			REJILLA_JOB_NOT_SUPPORTED (cdrdao);
		}

		if (!cuepath) {
			rejilla_track_type_free (type);
			REJILLA_JOB_NOT_READY (cdrdao);
		}

		rejilla_cdrdao_set_argv_device (cdrdao, argv);
		rejilla_cdrdao_set_argv_common (cdrdao, argv);
		rejilla_cdrdao_set_argv_common_rec (cdrdao, argv);

		g_ptr_array_add (argv, cuepath);
	}
	else {
		rejilla_track_type_free (type);
		REJILLA_JOB_NOT_SUPPORTED (cdrdao);
	}

	rejilla_track_type_free (type);
	rejilla_job_set_use_average_rate (REJILLA_JOB (cdrdao), TRUE);
	rejilla_job_set_current_action (REJILLA_JOB (cdrdao),
					REJILLA_BURN_ACTION_START_RECORDING,
					NULL,
					FALSE);
	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_cdrdao_set_argv_blank (RejillaCdrdao *cdrdao,
			       GPtrArray *argv)
{
	RejillaBurnFlag flags;

	g_ptr_array_add (argv, g_strdup ("cdrdao"));
	g_ptr_array_add (argv, g_strdup ("blank"));

	rejilla_cdrdao_set_argv_device (cdrdao, argv);
	rejilla_cdrdao_set_argv_common (cdrdao, argv);

	g_ptr_array_add (argv, g_strdup ("--blank-mode"));
	rejilla_job_get_flags (REJILLA_JOB (cdrdao), &flags);
	if (!(flags & REJILLA_BURN_FLAG_FAST_BLANK))
		g_ptr_array_add (argv, g_strdup ("full"));
	else
		g_ptr_array_add (argv, g_strdup ("minimal"));

	rejilla_job_set_current_action (REJILLA_JOB (cdrdao),
					REJILLA_BURN_ACTION_BLANKING,
					NULL,
					FALSE);

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_cdrdao_post (RejillaJob *job)
{
	RejillaCdrdaoPrivate *priv;

	priv = REJILLA_CDRDAO_PRIVATE (job);
	if (!priv->tmp_toc_path) {
		rejilla_job_finished_session (job);
		return REJILLA_BURN_OK;
	}

	/* we have to run toc2cue now to convert the toc file into a cue file */
	return REJILLA_BURN_RETRY;
}

static RejillaBurnResult
rejilla_cdrdao_start_toc2cue (RejillaCdrdao *cdrdao,
			      GPtrArray *argv,
			      GError **error)
{
	gchar *cue_output;
	RejillaBurnResult result;
	RejillaCdrdaoPrivate *priv;

	priv = REJILLA_CDRDAO_PRIVATE (cdrdao);

	g_ptr_array_add (argv, g_strdup ("toc2cue"));

	g_ptr_array_add (argv, priv->tmp_toc_path);
	priv->tmp_toc_path = NULL;

	result = rejilla_job_get_image_output (REJILLA_JOB (cdrdao),
					       NULL,
					       &cue_output);
	if (result != REJILLA_BURN_OK)
		return result;

	g_ptr_array_add (argv, cue_output);

	/* if there is a file toc2cue will fail */
	g_remove (cue_output);

	rejilla_job_set_current_action (REJILLA_JOB (cdrdao),
					REJILLA_BURN_ACTION_CREATING_IMAGE,
					_("Converting toc file"),
					TRUE);

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_cdrdao_set_argv_image (RejillaCdrdao *cdrdao,
			       GPtrArray *argv,
			       GError **error)
{
	gchar *image = NULL, *toc = NULL;
	RejillaTrackType *output = NULL;
	RejillaCdrdaoPrivate *priv;
	RejillaBurnResult result;
	RejillaJobAction action;
	RejillaDrive *drive;
	RejillaTrack *track;

	priv = REJILLA_CDRDAO_PRIVATE (cdrdao);
	if (priv->tmp_toc_path)
		return rejilla_cdrdao_start_toc2cue (cdrdao, argv, error);

	g_ptr_array_add (argv, g_strdup ("cdrdao"));
	g_ptr_array_add (argv, g_strdup ("read-cd"));
	g_ptr_array_add (argv, g_strdup ("--device"));

	rejilla_job_get_current_track (REJILLA_JOB (cdrdao), &track);
	drive = rejilla_track_disc_get_drive (REJILLA_TRACK_DISC (track));

	/* NOTE: that function returns either bus_target_lun or the device path
	 * according to OSes. Basically it returns bus/target/lun only for FreeBSD
	 * which is the only OS in need for that. For all others it returns the device
	 * path. */
	g_ptr_array_add (argv, rejilla_drive_get_bus_target_lun_string (drive));
	g_ptr_array_add (argv, g_strdup ("--read-raw"));

	/* This is done so that if a cue file is required we first generate
	 * a temporary toc file that will be later converted to a cue file.
	 * The datafile is written where it should be from the start. */
	output = rejilla_track_type_new ();
	rejilla_job_get_output_type (REJILLA_JOB (cdrdao), output);

	if (rejilla_track_type_get_image_format (output) == REJILLA_IMAGE_FORMAT_CDRDAO) {
		result = rejilla_job_get_image_output (REJILLA_JOB (cdrdao),
						       &image,
						       &toc);
		if (result != REJILLA_BURN_OK) {
			rejilla_track_type_free (output);
			return result;
		}
	}
	else if (rejilla_track_type_get_image_format (output) == REJILLA_IMAGE_FORMAT_CUE) {
		/* NOTE: we don't generate the .cue file right away; we'll call
		 * toc2cue right after we finish */
		result = rejilla_job_get_image_output (REJILLA_JOB (cdrdao),
						       &image,
						       NULL);
		if (result != REJILLA_BURN_OK) {
			rejilla_track_type_free (output);
			return result;
		}

		result = rejilla_job_get_tmp_file (REJILLA_JOB (cdrdao),
						   NULL,
						   &toc,
						   error);
		if (result != REJILLA_BURN_OK) {
			g_free (image);
			rejilla_track_type_free (output);
			return result;
		}

		/* save the temporary toc path to resuse it later. */
		priv->tmp_toc_path = g_strdup (toc);
	}

	rejilla_track_type_free (output);

	/* it's safe to remove them: session/task make sure they don't exist 
	 * when there is the proper flag whether it be tmp or real output. */ 
	if (toc)
		g_remove (toc);
	if (image)
		g_remove (image);

	rejilla_job_get_action (REJILLA_JOB (cdrdao), &action);
	if (action == REJILLA_JOB_ACTION_SIZE) {
		rejilla_job_set_current_action (REJILLA_JOB (cdrdao),
						REJILLA_BURN_ACTION_GETTING_SIZE,
						NULL,
						FALSE);
		rejilla_job_start_progress (REJILLA_JOB (cdrdao), FALSE);
	}

	g_ptr_array_add (argv, g_strdup ("--datafile"));
	g_ptr_array_add (argv, image);

	g_ptr_array_add (argv, g_strdup ("-v"));
	g_ptr_array_add (argv, g_strdup ("2"));

	g_ptr_array_add (argv, toc);

	rejilla_job_set_use_average_rate (REJILLA_JOB (cdrdao), TRUE);

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_cdrdao_set_argv (RejillaProcess *process,
			 GPtrArray *argv,
			 GError **error)
{
	RejillaCdrdao *cdrdao;
	RejillaJobAction action;

	cdrdao = REJILLA_CDRDAO (process);

	/* sets the first argv */
	rejilla_job_get_action (REJILLA_JOB (cdrdao), &action);
	if (action == REJILLA_JOB_ACTION_RECORD)
		return rejilla_cdrdao_set_argv_record (cdrdao, argv);
	else if (action == REJILLA_JOB_ACTION_ERASE)
		return rejilla_cdrdao_set_argv_blank (cdrdao, argv);
	else if (action == REJILLA_JOB_ACTION_IMAGE)
		return rejilla_cdrdao_set_argv_image (cdrdao, argv, error);
	else if (action == REJILLA_JOB_ACTION_SIZE) {
		RejillaTrack *track;

		rejilla_job_get_current_track (REJILLA_JOB (cdrdao), &track);
		if (REJILLA_IS_TRACK_DISC (track)) {
			goffset sectors = 0;

			rejilla_track_get_size (track, &sectors, NULL);

			/* cdrdao won't get a track size under 300 sectors */
			if (sectors < 300)
				sectors = 300;

			rejilla_job_set_output_size_for_current_track (REJILLA_JOB (cdrdao),
								       sectors,
								       sectors * 2352ULL);
		}
		else
			return REJILLA_BURN_NOT_SUPPORTED;

		return REJILLA_BURN_NOT_RUNNING;
	}

	REJILLA_JOB_NOT_SUPPORTED (cdrdao);
}

static void
rejilla_cdrdao_class_init (RejillaCdrdaoClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	RejillaProcessClass *process_class = REJILLA_PROCESS_CLASS (klass);

	g_type_class_add_private (klass, sizeof (RejillaCdrdaoPrivate));

	parent_class = g_type_class_peek_parent(klass);
	object_class->finalize = rejilla_cdrdao_finalize;

	process_class->stderr_func = rejilla_cdrdao_read_stderr;
	process_class->set_argv = rejilla_cdrdao_set_argv;
	process_class->post = rejilla_cdrdao_post;
}

static void
rejilla_cdrdao_init (RejillaCdrdao *obj)
{  
	GSettings *settings;
 	RejillaCdrdaoPrivate *priv;
 	
	/* load our "configuration" */
 	priv = REJILLA_CDRDAO_PRIVATE (obj);

	settings = g_settings_new (REJILLA_SCHEMA_CONFIG);
	priv->use_raw = g_settings_get_boolean (settings, REJILLA_KEY_RAW_FLAG);
	g_object_unref (settings);
}

static void
rejilla_cdrdao_finalize (GObject *object)
{
	RejillaCdrdaoPrivate *priv;

	priv = REJILLA_CDRDAO_PRIVATE (object);
	if (priv->tmp_toc_path) {
		g_free (priv->tmp_toc_path);
		priv->tmp_toc_path = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
rejilla_cdrdao_export_caps (RejillaPlugin *plugin)
{
	GSList *input;
	GSList *output;
	RejillaPluginConfOption *use_raw; 
	const RejillaMedia media_w = REJILLA_MEDIUM_CD|
				     REJILLA_MEDIUM_WRITABLE|
				     REJILLA_MEDIUM_REWRITABLE|
				     REJILLA_MEDIUM_BLANK;
	const RejillaMedia media_rw = REJILLA_MEDIUM_CD|
				      REJILLA_MEDIUM_REWRITABLE|
				      REJILLA_MEDIUM_APPENDABLE|
				      REJILLA_MEDIUM_CLOSED|
				      REJILLA_MEDIUM_HAS_DATA|
				      REJILLA_MEDIUM_HAS_AUDIO|
				      REJILLA_MEDIUM_BLANK;

	rejilla_plugin_define (plugin,
			       "cdrdao",
	                       NULL,
			       _("Copies, burns and blanks CDs"),
			       "Philippe Rouquier",
			       0);

	/* that's for cdrdao images: CDs only as input */
	input = rejilla_caps_disc_new (REJILLA_MEDIUM_CD|
				       REJILLA_MEDIUM_ROM|
				       REJILLA_MEDIUM_WRITABLE|
				       REJILLA_MEDIUM_REWRITABLE|
				       REJILLA_MEDIUM_APPENDABLE|
				       REJILLA_MEDIUM_CLOSED|
				       REJILLA_MEDIUM_HAS_AUDIO|
				       REJILLA_MEDIUM_HAS_DATA);

	/* an image can be created ... */
	output = rejilla_caps_image_new (REJILLA_PLUGIN_IO_ACCEPT_FILE,
					 REJILLA_IMAGE_FORMAT_CDRDAO);
	rejilla_plugin_link_caps (plugin, output, input);
	g_slist_free (output);

	output = rejilla_caps_image_new (REJILLA_PLUGIN_IO_ACCEPT_FILE,
					 REJILLA_IMAGE_FORMAT_CUE);
	rejilla_plugin_link_caps (plugin, output, input);
	g_slist_free (output);

	/* ... or a disc */
	output = rejilla_caps_disc_new (media_w);
	rejilla_plugin_link_caps (plugin, output, input);
	g_slist_free (input);

	/* cdrdao can also record these types of images to a disc */
	input = rejilla_caps_image_new (REJILLA_PLUGIN_IO_ACCEPT_FILE,
					REJILLA_IMAGE_FORMAT_CDRDAO|
					REJILLA_IMAGE_FORMAT_CUE);
	
	rejilla_plugin_link_caps (plugin, output, input);
	g_slist_free (output);
	g_slist_free (input);

	/* cdrdao is used to burn images so it can't APPEND and the disc must
	 * have been blanked before (it can't overwrite)
	 * NOTE: REJILLA_MEDIUM_FILE is needed here because of restriction API
	 * when we output an image. */
	rejilla_plugin_set_flags (plugin,
				  media_w|
				  REJILLA_MEDIUM_FILE,
				  REJILLA_BURN_FLAG_DAO|
				  REJILLA_BURN_FLAG_BURNPROOF|
				  REJILLA_BURN_FLAG_OVERBURN|
				  REJILLA_BURN_FLAG_DUMMY|
				  REJILLA_BURN_FLAG_NOGRACE,
				  REJILLA_BURN_FLAG_NONE);

	/* cdrdao can also blank */
	output = rejilla_caps_disc_new (media_rw);
	rejilla_plugin_blank_caps (plugin, output);
	g_slist_free (output);

	rejilla_plugin_set_blank_flags (plugin,
					media_rw,
					REJILLA_BURN_FLAG_NOGRACE|
					REJILLA_BURN_FLAG_FAST_BLANK,
					REJILLA_BURN_FLAG_NONE);

	use_raw = rejilla_plugin_conf_option_new (REJILLA_KEY_RAW_FLAG,
						  _("Enable the \"--driver generic-mmc-raw\" flag (see cdrdao manual)"),
						  REJILLA_PLUGIN_OPTION_BOOL);

	rejilla_plugin_add_conf_option (plugin, use_raw);

	rejilla_plugin_register_group (plugin, _(CDRDAO_DESCRIPTION));
}

G_MODULE_EXPORT void
rejilla_plugin_check_config (RejillaPlugin *plugin)
{
	gint version [3] = { 1, 2, 0};
	rejilla_plugin_test_app (plugin,
	                         "cdrdao",
	                         "version",
	                         "Cdrdao version %d.%d.%d - (C) Andreas Mueller <andreas@daneb.de>",
	                         version);

	rejilla_plugin_test_app (plugin,
	                         "toc2cue",
	                         "-V",
	                         "%d.%d.%d",
	                         version);
}
