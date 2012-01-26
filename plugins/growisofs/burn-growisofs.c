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
#include <stdlib.h>
#include <math.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>

#include <gmodule.h>

#include "rejilla-units.h"

#include "rejilla-plugin-registration.h"
#include "burn-job.h"
#include "burn-process.h"
#include "rejilla-drive.h"
#include "burn-growisofs-common.h"
#include "rejilla-track-data.h"
#include "rejilla-track-image.h"


#define REJILLA_TYPE_GROWISOFS         (rejilla_growisofs_get_type ())
#define REJILLA_GROWISOFS(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), REJILLA_TYPE_GROWISOFS, RejillaGrowisofs))
#define REJILLA_GROWISOFS_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), REJILLA_TYPE_GROWISOFS, RejillaGrowisofsClass))
#define REJILLA_IS_GROWISOFS(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), REJILLA_TYPE_GROWISOFS))
#define REJILLA_IS_GROWISOFS_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), REJILLA_TYPE_GROWISOFS))
#define REJILLA_GROWISOFS_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), REJILLA_TYPE_GROWISOFS, RejillaGrowisofsClass))

REJILLA_PLUGIN_BOILERPLATE (RejillaGrowisofs, rejilla_growisofs, REJILLA_TYPE_PROCESS, RejillaProcess);

struct RejillaGrowisofsPrivate {
	guint use_utf8:1;
	guint use_genisoimage:1;
  	guint use_dao:1;
};
typedef struct RejillaGrowisofsPrivate RejillaGrowisofsPrivate;

#define REJILLA_GROWISOFS_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REJILLA_TYPE_GROWISOFS, RejillaGrowisofsPrivate))

static GObjectClass *parent_class = NULL;

#define REJILLA_SCHEMA_CONFIG		"org.mate.rejilla.config"
#define REJILLA_KEY_DAO_FLAG		"dao-flag"

/* Process start */
static RejillaBurnResult
rejilla_growisofs_read_stdout (RejillaProcess *process, const gchar *line)
{
	int perc_1, perc_2;
	int speed_1, speed_2;
	long long b_written, b_total;

	/* Newer growisofs version have a different line pattern that shows
	 * drive buffer filling. */
	if (sscanf (line, "%10lld/%lld (%4d.%1d%%) @%2d.%1dx, remaining %*d:%*d",
		    &b_written, &b_total, &perc_1, &perc_2, &speed_1, &speed_2) == 6) {
		RejillaJobAction action;

		rejilla_job_get_action (REJILLA_JOB (process), &action);
		if (action == REJILLA_JOB_ACTION_ERASE && b_written >= 65536) {
			/* we nullified 65536 that's enough. A signal SIGTERM
			 * will be sent in process.c. That's not the best way
			 * to do it but it works. */
			rejilla_job_finished_session (REJILLA_JOB (process));
			return REJILLA_BURN_OK;
		}

		rejilla_job_set_written_session (REJILLA_JOB (process), b_written);
		rejilla_job_set_rate (REJILLA_JOB (process), (gdouble) (speed_1 * 10 + speed_2) / 10.0 * (gdouble) DVD_RATE);

		if (action == REJILLA_JOB_ACTION_ERASE) {
			rejilla_job_set_current_action (REJILLA_JOB (process),
							REJILLA_BURN_ACTION_BLANKING,
							NULL,
							FALSE);
		}
		else
			rejilla_job_set_current_action (REJILLA_JOB (process),
							REJILLA_BURN_ACTION_RECORDING,
							NULL,
							FALSE);

		rejilla_job_start_progress (REJILLA_JOB (process), FALSE);
	}
	else if (strstr (line, "About to execute") || strstr (line, "Executing"))
		rejilla_job_set_dangerous (REJILLA_JOB (process), TRUE);

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_growisofs_read_stderr (RejillaProcess *process, const gchar *line)
{
	int perc_1, perc_2;

	if (sscanf (line, " %2d.%2d%% done, estimate finish", &perc_1, &perc_2) == 2) {
		gdouble fraction;
		RejillaBurnAction action;

		fraction = (gdouble) ((gdouble) perc_1 +
			   ((gdouble) perc_2 / (gdouble) 100.0)) /
			   (gdouble) 100.0;

		rejilla_job_set_progress (REJILLA_JOB (process), fraction);
		rejilla_job_get_current_action (REJILLA_JOB (process), &action);

		if (action == REJILLA_BURN_ACTION_BLANKING && fraction >= 0.01) {
			/* we nullified 1% of the medium (more than 65536)
			 * that's enough to make the filesystem unusable and
			 * looking blank. A signal SIGTERM will be sent to stop
			 * us. */
			rejilla_job_finished_session (REJILLA_JOB (process));
			return REJILLA_BURN_OK;
		}

		rejilla_job_set_current_action (REJILLA_JOB (process),
						REJILLA_BURN_ACTION_RECORDING,
						NULL,
						FALSE);
		rejilla_job_start_progress (REJILLA_JOB (process), FALSE);
	}
	else if (strstr (line, "Total extents scheduled to be written = ")) {
		RejillaJobAction action;

		line += strlen ("Total extents scheduled to be written = ");
		rejilla_job_get_action (REJILLA_JOB (process), &action);
		if (action == REJILLA_JOB_ACTION_SIZE) {
			gint64 sectors;

			sectors = strtoll (line, NULL, 10);

			/* NOTE: this has to be a multiple of 2048 */
			rejilla_job_set_output_size_for_current_track (REJILLA_JOB (process),
								       sectors,
								       sectors * 2048ULL);

			/* we better tell growisofs to stop here as it returns 
			 * a value of 1 when mkisofs is run with --print-size */
			rejilla_job_finished_session (REJILLA_JOB (process));
		}
	}
	else if (strstr (line, "flushing cache") != NULL) {
		rejilla_job_set_progress (REJILLA_JOB (process), 1.0);
		rejilla_job_set_current_action (REJILLA_JOB (process),
						REJILLA_BURN_ACTION_FIXATING,
						NULL,
						FALSE);
	}
	else if (strstr (line, "unable to open")
	     ||  strstr (line, "unable to stat")
	     ||  strstr (line, "unable to proceed with recording: unable to unmount")) {
		rejilla_job_error (REJILLA_JOB (process),
				   g_error_new_literal (REJILLA_BURN_ERROR,
							REJILLA_BURN_ERROR_DRIVE_BUSY,
							_("The drive is busy")));
	}
	else if (strstr (line, "not enough space available")
	     ||  strstr (line, "end of user area encountered on this track")
	     ||  strstr (line, "blocks are free")) {
		rejilla_job_error (REJILLA_JOB (process), 
				   g_error_new (REJILLA_BURN_ERROR,
						REJILLA_BURN_ERROR_MEDIUM_SPACE,
						_("Not enough space available on the disc")));
	}
	else if (strstr (line, "Input/output error. Read error on old image")) {
		rejilla_job_error (REJILLA_JOB (process), 
				   g_error_new_literal (REJILLA_BURN_ERROR,
							REJILLA_BURN_ERROR_IMAGE_LAST_SESSION,
							_("Last session import failed")));
	}
	else if (strstr (line, "Unable to sort directory")) {
		rejilla_job_error (REJILLA_JOB (process), 
				   g_error_new_literal (REJILLA_BURN_ERROR,
							REJILLA_BURN_ERROR_WRITE_IMAGE,
							_("An image could not be created")));
	}
	else if (strstr (line, "have the same joliet name")
	     ||  strstr (line, "Joliet tree sort failed.")) {
		rejilla_job_error (REJILLA_JOB (process), 
				   g_error_new_literal (REJILLA_BURN_ERROR,
							REJILLA_BURN_ERROR_IMAGE_JOLIET,
							_("An image could not be created")));
	}
	else if (strstr (line, "Incorrectly encoded string")) {
		rejilla_job_error (REJILLA_JOB (process),
				   g_error_new_literal (REJILLA_BURN_ERROR,
							REJILLA_BURN_ERROR_INPUT_INVALID,
							_("Some files have invalid filenames")));
	}
	else if (strstr (line, "Unknown charset")) {
		rejilla_job_error (REJILLA_JOB (process),
				   g_error_new_literal (REJILLA_BURN_ERROR,
							REJILLA_BURN_ERROR_INPUT_INVALID,
							_("Unknown character encoding")));
	}

	/** REMINDER! removed messages:
	   else if (strstr (line, ":-(") != NULL || strstr (line, "FATAL"))

	   else if (strstr (line, "already carries isofs") && strstr (line, "FATAL:")) {
		rejilla_job_error (REJILLA_JOB (process), 
				   g_error_new (REJILLA_BURN_ERROR,
						REJILLA_BURN_ERROR_MEDIUM_INVALID,
						_("The disc is already burnt")));
	   }
	**/

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_growisofs_set_mkisofs_argv (RejillaGrowisofs *growisofs,
				    GPtrArray *argv,
				    GError **error)
{
	RejillaGrowisofsPrivate *priv;
	RejillaTrack *track = NULL;
	gchar *excluded_path = NULL;
	gchar *grafts_path = NULL;
	RejillaJobAction action;
	RejillaBurnResult result;
	RejillaImageFS fs_type;
	gchar *emptydir = NULL;
	gchar *videodir = NULL;

	priv = REJILLA_GROWISOFS_PRIVATE (growisofs);

	if (priv->use_genisoimage) {
		REJILLA_JOB_LOG (growisofs, "Using genisoimage");
	}
	else {
		REJILLA_JOB_LOG (growisofs, "Using mkisofs");
	}

	g_ptr_array_add (argv, g_strdup ("-r"));

	rejilla_job_get_current_track (REJILLA_JOB (growisofs), &track);
	fs_type = rejilla_track_data_get_fs (REJILLA_TRACK_DATA (track));
	if (fs_type & REJILLA_IMAGE_FS_JOLIET)
		g_ptr_array_add (argv, g_strdup ("-J"));

	if ((fs_type & REJILLA_IMAGE_FS_ISO)
	&&  (fs_type & REJILLA_IMAGE_ISO_FS_LEVEL_3)) {
		/* That's the safest option. A few OS don't support that though,
		 * like MacOSX and freebsd.*/
		g_ptr_array_add (argv, g_strdup ("-iso-level"));
		g_ptr_array_add (argv, g_strdup ("3"));

		/* NOTE: the following is specific to genisoimage
		 * It allows to burn files over 4 GiB.
		 * The only problem here is which are we using? mkisofs or
		 * genisoimage? That's what we determined first. */
		if (priv->use_genisoimage)
			g_ptr_array_add (argv, g_strdup ("-allow-limited-size"));
	}

	if (fs_type & REJILLA_IMAGE_FS_UDF)
		g_ptr_array_add (argv, g_strdup ("-udf"));

	if (fs_type & REJILLA_IMAGE_FS_VIDEO) {
		g_ptr_array_add (argv, g_strdup ("-dvd-video"));

		result = rejilla_job_get_tmp_dir (REJILLA_JOB (growisofs),
						  &videodir,
						  error);
		if (result != REJILLA_BURN_OK)
			return result;
	}

	if (priv->use_utf8) {
		g_ptr_array_add (argv, g_strdup ("-input-charset"));
		g_ptr_array_add (argv, g_strdup ("utf8"));
	}

	g_ptr_array_add (argv, g_strdup ("-graft-points"));

	if (fs_type & REJILLA_IMAGE_ISO_FS_DEEP_DIRECTORY)
		g_ptr_array_add (argv, g_strdup ("-D"));	// This is dangerous the manual says but apparently it works well

	result = rejilla_job_get_tmp_file (REJILLA_JOB (growisofs),
					   NULL,
					   &grafts_path,
					   error);
	if (result != REJILLA_BURN_OK) {
		g_free (videodir);
		return result;
	}

	result = rejilla_job_get_tmp_file (REJILLA_JOB (growisofs),
					   NULL,
					   &excluded_path,
					   error);
	if (result != REJILLA_BURN_OK) {
		g_free (grafts_path);
		g_free (videodir);
		return result;
	}

	result = rejilla_job_get_tmp_dir (REJILLA_JOB (growisofs),
					  &emptydir,
					  error);
	if (result != REJILLA_BURN_OK) {
		g_free (videodir);
		g_free (grafts_path);
		g_free (excluded_path);
		return result;
	}

	result = rejilla_track_data_write_to_paths (REJILLA_TRACK_DATA (track),
	                                            grafts_path,
	                                            excluded_path,
	                                            emptydir,
	                                            videodir,
	                                            error);
	g_free (emptydir);

	if (result != REJILLA_BURN_OK) {
		g_free (videodir);
		g_free (grafts_path);
		g_free (excluded_path);
		return result;
	}

	g_ptr_array_add (argv, g_strdup ("-path-list"));
	g_ptr_array_add (argv, grafts_path);

	g_ptr_array_add (argv, g_strdup ("-exclude-list"));
	g_ptr_array_add (argv, excluded_path);

	rejilla_job_get_action (REJILLA_JOB (growisofs), &action);
	if (action != REJILLA_JOB_ACTION_SIZE) {
		gchar *label = NULL;

		rejilla_job_get_data_label (REJILLA_JOB (growisofs), &label);
		if (label) {
			g_ptr_array_add (argv, g_strdup ("-V"));
			g_ptr_array_add (argv, label);
		}

		g_ptr_array_add (argv, g_strdup ("-A"));
		g_ptr_array_add (argv, g_strdup_printf ("Rejilla-%i.%i.%i",
							REJILLA_MAJOR_VERSION,
							REJILLA_MINOR_VERSION,
							REJILLA_SUB));
	
		g_ptr_array_add (argv, g_strdup ("-sysid"));
		g_ptr_array_add (argv, g_strdup ("LINUX"));
	
		/* FIXME! -sort is an interesting option allowing to decide where the 
		 * files are written on the disc and therefore to optimize later reading */
		/* FIXME: -hidden --hidden-list -hide-jolie -hide-joliet-list will allow to hide
		 * some files when we will display the contents of a disc we will want to merge */
		/* FIXME: support preparer publisher options */

		g_ptr_array_add (argv, g_strdup ("-v"));
	}
	else {
		/* we don't specify -q as there wouldn't be anything */
		g_ptr_array_add (argv, g_strdup ("-print-size"));
	}

	if (videodir) {
		g_ptr_array_add (argv, g_strdup ("-f"));
		g_ptr_array_add (argv, videodir);
	}

	return REJILLA_BURN_OK;
}

/**
 * Some info about use-the-force-luke options
 * dry-run => stops after invoking mkisofs
 * no_tty => avoids fatal error if an isofs exists and an image is piped
 *  	  => skip the five seconds waiting
 * 
 */
 
static RejillaBurnResult
rejilla_growisofs_set_argv_record (RejillaGrowisofs *growisofs,
				   GPtrArray *argv,
				   GError **error)
{
	RejillaBurnResult result;
	RejillaJobAction action;
	RejillaBurnFlag flags;
	goffset sectors = 0;
	gchar *device;
	guint speed;

	/* This seems to help to eject tray after burning (at least with mine) */
	g_ptr_array_add (argv, g_strdup ("growisofs"));
	g_ptr_array_add (argv, g_strdup ("-use-the-force-luke=notray"));

	rejilla_job_get_flags (REJILLA_JOB (growisofs), &flags);
	if (flags & REJILLA_BURN_FLAG_DUMMY)
		g_ptr_array_add (argv, g_strdup ("-use-the-force-luke=dummy"));

	/* NOTE 1: dao is not a good thing if you want to make multisession
	 * DVD+-R. It will close the disc. Which make sense since DAO means
	 * Disc At Once. That's checked in burn-caps.c with coherency checks.
	 * NOTE 2: dao is supported for DL DVD after 6.0 (think about that for
	 * BurnCaps)
	 * Moreover even for single session DVDs it doesn't work properly so
	 * there is a workaround to turn it off entirely. */
	if (flags & REJILLA_BURN_FLAG_DAO)
		g_ptr_array_add (argv, g_strdup ("-use-the-force-luke=dao"));

	/* This is necessary for multi session discs when a new session starts
	 * beyond the 4Gio boundary since it may not be readable afterward.
	 * To work, this requires a kernel > 2.6.8.
	 * FIXME: This would deserve a flag to warn the user. */
	g_ptr_array_add (argv, g_strdup ("-use-the-force-luke=4gms"));

	if (!(flags & REJILLA_BURN_FLAG_MULTI)) {
		/* This option seems to help creating DVD more compatible
		 * with DVD readers.
		 * NOTE: it doesn't work with DVD+RW and DVD-RW in restricted
		 * overwrite mode */
		g_ptr_array_add (argv, g_strdup ("-dvd-compat"));
	}

	rejilla_job_get_speed (REJILLA_JOB (growisofs), &speed);
	if (speed > 0)
		g_ptr_array_add (argv, g_strdup_printf ("-speed=%d", speed));

	/* see if we're asked to merge some new data: in this case we MUST have
	 * a list of grafts. The image can't come through stdin or an already 
	 * made image */
	rejilla_job_get_device (REJILLA_JOB (growisofs), &device);
	rejilla_job_get_action (REJILLA_JOB (growisofs), &action);
	rejilla_job_get_session_output_size (REJILLA_JOB (growisofs),
					     &sectors,
					     NULL);
	if (sectors) {
		/* NOTE: tracksize is in block number (2048 bytes) */
		g_ptr_array_add (argv,
				 g_strdup_printf ("-use-the-force-luke=tracksize:%"
						  G_GINT64_FORMAT,
						  sectors));
	}

	if (flags & REJILLA_BURN_FLAG_MERGE) {
		g_ptr_array_add (argv, g_strdup ("-M"));
		g_ptr_array_add (argv, device);
		
		/* this can only happen if source->type == REJILLA_TRACK_SOURCE_GRAFTS */
		if (action == REJILLA_JOB_ACTION_SIZE)
			g_ptr_array_add (argv, g_strdup ("-dry-run"));

		result = rejilla_growisofs_set_mkisofs_argv (growisofs, 
							     argv,
							     error);
		if (result != REJILLA_BURN_OK)
			return result;
	}
	else {
		RejillaTrack *current = NULL;

		/* apparently we are not merging but growisofs will refuse to 
		 * write a piped image if there is one already on the disc;
		 * except with this option */
		g_ptr_array_add (argv, g_strdup ("-use-the-force-luke=tty"));

		rejilla_job_get_current_track (REJILLA_JOB (growisofs), &current);
		if (rejilla_job_get_fd_in (REJILLA_JOB (growisofs), NULL) == REJILLA_BURN_OK) {
			/* set the buffer. NOTE: apparently this needs to be a power of 2 */
			/* FIXME: is it right to mess with it ? 
			   g_ptr_array_add (argv, g_strdup_printf ("-use-the-force-luke=bufsize:%im", 32)); */

			if (!g_file_test ("/proc/self/fd/0", G_FILE_TEST_EXISTS)) {
				g_set_error (error,
					     REJILLA_BURN_ERROR,
					     REJILLA_BURN_ERROR_FILE_NOT_FOUND,
					     _("\"%s\" could not be found"),
					     "/proc/self/fd/0");
				return REJILLA_BURN_ERR;
			}

			/* FIXME: should we use DAO ? */
			g_ptr_array_add (argv, g_strdup ("-Z"));
			g_ptr_array_add (argv, g_strdup_printf ("%s=/proc/self/fd/0", device));
			g_free (device);
		}
		else if (REJILLA_IS_TRACK_IMAGE (current)) {
			gchar *localpath;

			localpath = rejilla_track_image_get_source (REJILLA_TRACK_IMAGE (current), FALSE);
			if (!localpath) {
				g_set_error (error,
					     REJILLA_BURN_ERROR,
					     REJILLA_BURN_ERROR_FILE_NOT_LOCAL,
					     _("The file is not stored locally"));
				return REJILLA_BURN_ERR;
			}

			g_ptr_array_add (argv, g_strdup ("-Z"));
			g_ptr_array_add (argv, g_strdup_printf ("%s=%s",
								device,
								localpath));

			g_free (device);
			g_free (localpath);
		}
		else if (REJILLA_IS_TRACK_DATA (current)) {
			g_ptr_array_add (argv, g_strdup ("-Z"));
			g_ptr_array_add (argv, device);

			/* this can only happen if source->type == REJILLA_TRACK_SOURCE_DATA */
			if (action == REJILLA_JOB_ACTION_SIZE)
				g_ptr_array_add (argv, g_strdup ("-dry-run"));

			result = rejilla_growisofs_set_mkisofs_argv (growisofs, 
								     argv,
								     error);
			if (result != REJILLA_BURN_OK)
				return result;
		}
		else
			REJILLA_JOB_NOT_SUPPORTED (growisofs);
	}

	if (action == REJILLA_JOB_ACTION_SIZE)
		rejilla_job_set_current_action (REJILLA_JOB (growisofs),
						REJILLA_BURN_ACTION_GETTING_SIZE,
						NULL,
						FALSE);
	else
		rejilla_job_set_current_action (REJILLA_JOB (growisofs),
						REJILLA_BURN_ACTION_START_RECORDING,
						NULL,
						FALSE);

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_growisofs_set_argv_blank (RejillaGrowisofs *growisofs,
				  GPtrArray *argv)
{
	RejillaBurnFlag flags;
	gchar *device;
	guint speed;

	g_ptr_array_add (argv, g_strdup ("growisofs"));
	rejilla_job_get_flags (REJILLA_JOB (growisofs), &flags);
	if (!(flags & REJILLA_BURN_FLAG_FAST_BLANK))
		REJILLA_JOB_NOT_SUPPORTED (growisofs);

	g_ptr_array_add (argv, g_strdup ("-Z"));

	/* NOTE: /dev/zero works but not /dev/null. Why ? */
	rejilla_job_get_device (REJILLA_JOB (growisofs), &device);
	g_ptr_array_add (argv, g_strdup_printf ("%s=%s", device, "/dev/zero"));
	g_free (device);

	/* That should fix a problem where when the DVD had an isofs
	 * growisofs warned that it had an isofs already on the disc */
	g_ptr_array_add (argv, g_strdup ("-use-the-force-luke=tty"));

	/* set maximum write speed */
	rejilla_job_get_max_speed (REJILLA_JOB (growisofs), &speed);
	g_ptr_array_add (argv, g_strdup_printf ("-speed=%d", speed));

	/* we only need to nullify 64 KiB: we'll stop the process when
	 * at least 65536 bytes have been written. We put a little more
	 * so in stdout parsing function remaining time is not negative
	 * if that's too fast. */
	g_ptr_array_add (argv, g_strdup ("-use-the-force-luke=tracksize:1024"));

	if (flags & REJILLA_BURN_FLAG_DUMMY)
		g_ptr_array_add (argv, g_strdup ("-use-the-force-luke=dummy"));

	rejilla_job_set_current_action (REJILLA_JOB (growisofs),
					REJILLA_BURN_ACTION_BLANKING,
					NULL,
					FALSE);
	rejilla_job_start_progress (REJILLA_JOB (growisofs), FALSE);

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_growisofs_set_argv (RejillaProcess *process,
			    GPtrArray *argv,
			    GError **error)
{
	RejillaJobAction action;
	RejillaBurnResult result;

	rejilla_job_get_action (REJILLA_JOB (process), &action);
	if (action == REJILLA_JOB_ACTION_SIZE) {
		RejillaTrack *track = NULL;

		/* only do it if that's DATA as input */
		rejilla_job_get_current_track (REJILLA_JOB (process), &track);
		if (!REJILLA_IS_TRACK_DATA (track))
			return REJILLA_BURN_NOT_SUPPORTED;

		/* If another job is piping data to us leave it to the job to 
		 * retrieve the data size. */
		if (rejilla_job_get_fd_in (REJILLA_JOB (process), NULL) == REJILLA_BURN_OK)
			return REJILLA_BURN_NOT_SUPPORTED;

		result = rejilla_growisofs_set_argv_record (REJILLA_GROWISOFS (process),
							    argv,
							    error);
	}
	else if (action == REJILLA_JOB_ACTION_RECORD)
		result = rejilla_growisofs_set_argv_record (REJILLA_GROWISOFS (process),
							    argv,
							    error);
	else if (action == REJILLA_JOB_ACTION_ERASE)
		result = rejilla_growisofs_set_argv_blank (REJILLA_GROWISOFS (process),
							   argv);
	else
		REJILLA_JOB_NOT_READY (process);

	return result;
}

static void
rejilla_growisofs_class_init (RejillaGrowisofsClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	RejillaProcessClass *process_class = REJILLA_PROCESS_CLASS (klass);

	g_type_class_add_private (klass, sizeof (RejillaGrowisofsPrivate));

	parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = rejilla_growisofs_finalize;

	process_class->stdout_func = rejilla_growisofs_read_stdout;
	process_class->stderr_func = rejilla_growisofs_read_stderr;
	process_class->set_argv = rejilla_growisofs_set_argv;
	process_class->post = rejilla_job_finished_session;
}

static void
rejilla_growisofs_init (RejillaGrowisofs *obj)
{
	RejillaGrowisofsPrivate *priv;
	gchar *standard_error = NULL;
	gchar *prog_name;
	gboolean res;

	priv = REJILLA_GROWISOFS_PRIVATE (obj);

	/* this code (remotely) comes from ncb_mkisofs_supports_utf8 */
	/* Added a way to detect whether we'll use mkisofs or genisoimage */

	prog_name = g_find_program_in_path ("mkisofs");
        if (prog_name && g_file_test (prog_name, G_FILE_TEST_IS_EXECUTABLE)) {
		gchar *standard_output = NULL;

		res = g_spawn_command_line_sync ("mkisofs -version",
						 &standard_output,
						 NULL,
						 NULL,
						 NULL);
		if (res) {
			/* Really make sure it is mkisofs and not a symlink */
			if (standard_output && strstr (standard_output, "genisoimage"))
				priv->use_genisoimage = TRUE;

			if (standard_output)
				g_free (standard_output);
		}
		else
			priv->use_genisoimage = TRUE;
	}
	else
		priv->use_genisoimage = TRUE;

	g_free (prog_name);

	/* Don't use REJILLA_JOB_LOG () here!! */
	if (priv->use_genisoimage)
		res = g_spawn_command_line_sync ("genisoimage -input-charset utf8",
						 NULL,
						 &standard_error,
						 NULL,
						 NULL);
	else
	  	res = g_spawn_command_line_sync ("mkisofs -input-charset utf8",
						 NULL,
						 &standard_error,
						 NULL,
						 NULL);

	if (res && !g_strrstr (standard_error, "Unknown charset"))
		priv->use_utf8 = TRUE;
	else
		priv->use_utf8 = FALSE;

	g_free (standard_error);
}

static void
rejilla_growisofs_finalize (GObject *object)
{
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
rejilla_growisofs_export_caps (RejillaPlugin *plugin)
{
	RejillaPluginConfOption *use_dao_opt;
	GSList *input_symlink;
	GSList *input_joliet;
	GSettings *settings;
	gboolean use_dao;
	GSList *output;
	GSList *input;

	rejilla_plugin_define (plugin,
			       "growisofs",
	                       NULL,
			       _("Burns and blanks DVDs and BDs"),
			       "Philippe Rouquier",
			       7);

	/* growisofs can write images to any type of BD/DVD-R as long as it's blank */
	input = rejilla_caps_image_new (REJILLA_PLUGIN_IO_ACCEPT_PIPE|
					REJILLA_PLUGIN_IO_ACCEPT_FILE,
					REJILLA_IMAGE_FORMAT_BIN);

	output = rejilla_caps_disc_new (REJILLA_MEDIUM_BD|
					REJILLA_MEDIUM_SRM|
					REJILLA_MEDIUM_POW|
					REJILLA_MEDIUM_DVD|
					REJILLA_MEDIUM_DUAL_L|
					REJILLA_MEDIUM_PLUS|
					REJILLA_MEDIUM_JUMP|
					REJILLA_MEDIUM_SEQUENTIAL|
					REJILLA_MEDIUM_WRITABLE|
					REJILLA_MEDIUM_BLANK);

	rejilla_plugin_link_caps (plugin, output, input);
	g_slist_free (output);

	output = rejilla_caps_disc_new (REJILLA_MEDIUM_DVD|
					REJILLA_MEDIUM_SEQUENTIAL|
					REJILLA_MEDIUM_REWRITABLE|
					REJILLA_MEDIUM_UNFORMATTED|
					REJILLA_MEDIUM_BLANK);

	rejilla_plugin_link_caps (plugin, output, input);
	g_slist_free (output);

	/* and images to BD/DVD RW +/-(restricted) whatever the status */
	output = rejilla_caps_disc_new (REJILLA_MEDIUM_BD|
					REJILLA_MEDIUM_DVD|
					REJILLA_MEDIUM_RAM|
					REJILLA_MEDIUM_DUAL_L|
					REJILLA_MEDIUM_PLUS|
					REJILLA_MEDIUM_RESTRICTED|
					REJILLA_MEDIUM_REWRITABLE|
					REJILLA_MEDIUM_UNFORMATTED|
					REJILLA_MEDIUM_BLANK|
					REJILLA_MEDIUM_CLOSED|
					REJILLA_MEDIUM_APPENDABLE|
					REJILLA_MEDIUM_HAS_DATA);
	rejilla_plugin_link_caps (plugin, output, input);
	g_slist_free (output);
	g_slist_free (input);

	/* for DATA type recording discs can be also appended */
	input_joliet = rejilla_caps_data_new (REJILLA_IMAGE_FS_ISO|
					      REJILLA_IMAGE_FS_UDF|
					      REJILLA_IMAGE_ISO_FS_LEVEL_3|
					      REJILLA_IMAGE_ISO_FS_DEEP_DIRECTORY|
					      REJILLA_IMAGE_FS_JOLIET|
					      REJILLA_IMAGE_FS_VIDEO);

	input_symlink = rejilla_caps_data_new (REJILLA_IMAGE_FS_ISO|
					       REJILLA_IMAGE_ISO_FS_LEVEL_3|
					       REJILLA_IMAGE_ISO_FS_DEEP_DIRECTORY|
					       REJILLA_IMAGE_FS_SYMLINK);

	output = rejilla_caps_disc_new (REJILLA_MEDIUM_BD|
					REJILLA_MEDIUM_SRM|
					REJILLA_MEDIUM_POW|
					REJILLA_MEDIUM_DVD|
					REJILLA_MEDIUM_DUAL_L|
					REJILLA_MEDIUM_RAM|
					REJILLA_MEDIUM_PLUS|
					REJILLA_MEDIUM_RESTRICTED|
					REJILLA_MEDIUM_SEQUENTIAL|
					REJILLA_MEDIUM_JUMP|
					REJILLA_MEDIUM_WRITABLE|
					REJILLA_MEDIUM_REWRITABLE|
					REJILLA_MEDIUM_UNFORMATTED|
					REJILLA_MEDIUM_BLANK|
					REJILLA_MEDIUM_APPENDABLE|
					REJILLA_MEDIUM_HAS_DATA);
	rejilla_plugin_link_caps (plugin, output, input_joliet);
	rejilla_plugin_link_caps (plugin, output, input_symlink);
	g_slist_free (output);

	/* growisofs has the possibility to record to closed BD/DVD+RW
    	 * +/-restricted and to append some more data to them which makes them
     	 * unique */
	output = rejilla_caps_disc_new (REJILLA_MEDIUM_BD|
					REJILLA_MEDIUM_DVD|
					REJILLA_MEDIUM_DUAL_L|
					REJILLA_MEDIUM_RAM|
					REJILLA_MEDIUM_PLUS|
					REJILLA_MEDIUM_RESTRICTED|
					REJILLA_MEDIUM_REWRITABLE|
					REJILLA_MEDIUM_CLOSED|
					REJILLA_MEDIUM_HAS_DATA);

	rejilla_plugin_link_caps (plugin, output, input_joliet);
	rejilla_plugin_link_caps (plugin, output, input_symlink);
	g_slist_free (output);
	g_slist_free (input_joliet);
	g_slist_free (input_symlink);

	/* For DVD-RW sequential */
	REJILLA_PLUGIN_ADD_STANDARD_DVDRW_FLAGS (plugin, REJILLA_BURN_FLAG_NONE);

	/* see NOTE for DVD-RW restricted overwrite */
	REJILLA_PLUGIN_ADD_STANDARD_DVDRW_RESTRICTED_FLAGS (plugin, REJILLA_BURN_FLAG_NONE);

	/* DVD+R and DVD-R. DAO and growisofs doesn't always work well with
	 * these types of media and with some drives. So don't allow it if the
	 * workaround is set with GSettings (and it should be by default). */
	settings = g_settings_new (REJILLA_SCHEMA_CONFIG);
	use_dao = g_settings_get_boolean (settings, REJILLA_KEY_DAO_FLAG);
	g_object_unref (settings);

	if (use_dao == TRUE) {
		REJILLA_PLUGIN_ADD_STANDARD_DVDR_FLAGS (plugin, REJILLA_BURN_FLAG_NONE);
		REJILLA_PLUGIN_ADD_STANDARD_DVDR_PLUS_FLAGS (plugin, REJILLA_BURN_FLAG_NONE);
	}
	else {
		/* All above standard flags minus DAO flag support */
		REJILLA_PLUGIN_ADD_STANDARD_DVDR_FLAGS (plugin, REJILLA_BURN_FLAG_DAO);
		REJILLA_PLUGIN_ADD_STANDARD_DVDR_PLUS_FLAGS (plugin, REJILLA_BURN_FLAG_DAO);
	}

	/* for DVD+RW */
	REJILLA_PLUGIN_ADD_STANDARD_DVDRW_PLUS_FLAGS (plugin, REJILLA_BURN_FLAG_NONE);

	/* for BD-RE */
	REJILLA_PLUGIN_ADD_STANDARD_BD_RE_FLAGS (plugin, REJILLA_BURN_FLAG_NONE);

	/* blank caps for +/restricted RW */
	output = rejilla_caps_disc_new (REJILLA_MEDIUM_DVD|
					REJILLA_MEDIUM_DUAL_L|
					REJILLA_MEDIUM_PLUS|
					REJILLA_MEDIUM_RESTRICTED|
					REJILLA_MEDIUM_REWRITABLE|
					REJILLA_MEDIUM_APPENDABLE|
					REJILLA_MEDIUM_CLOSED|
					REJILLA_MEDIUM_HAS_DATA|
					REJILLA_MEDIUM_UNFORMATTED|
					REJILLA_MEDIUM_BLANK);
	rejilla_plugin_blank_caps (plugin, output);
	g_slist_free (output);

	rejilla_plugin_set_blank_flags (plugin,
					REJILLA_MEDIUM_DVD|
					REJILLA_MEDIUM_RESTRICTED|
					REJILLA_MEDIUM_REWRITABLE|
					REJILLA_MEDIUM_APPENDABLE|
					REJILLA_MEDIUM_HAS_DATA|
					REJILLA_MEDIUM_BLANK|
					REJILLA_MEDIUM_UNFORMATTED|
					REJILLA_MEDIUM_CLOSED,
					REJILLA_BURN_FLAG_NOGRACE|
					REJILLA_BURN_FLAG_FAST_BLANK,
					REJILLA_BURN_FLAG_FAST_BLANK);

	/* again DVD+RW don't support dummy */
	rejilla_plugin_set_blank_flags (plugin,
					REJILLA_MEDIUM_DVDRW_PLUS|
					REJILLA_MEDIUM_DUAL_L|
					REJILLA_MEDIUM_APPENDABLE|
					REJILLA_MEDIUM_HAS_DATA|
					REJILLA_MEDIUM_BLANK|
					REJILLA_MEDIUM_UNFORMATTED|
					REJILLA_MEDIUM_CLOSED,
					REJILLA_BURN_FLAG_NOGRACE|
					REJILLA_BURN_FLAG_FAST_BLANK,
					REJILLA_BURN_FLAG_FAST_BLANK);

	use_dao_opt = rejilla_plugin_conf_option_new (REJILLA_KEY_DAO_FLAG,
	                                              _("Allow DAO use"),
	                                              REJILLA_PLUGIN_OPTION_BOOL);
	rejilla_plugin_add_conf_option (plugin, use_dao_opt); 

	rejilla_plugin_register_group (plugin, _(GROWISOFS_DESCRIPTION));
}

G_MODULE_EXPORT void
rejilla_plugin_check_config (RejillaPlugin *plugin)
{
	gint version [3] = { 5, 0, -1};
	rejilla_plugin_test_app (plugin,
	                         "growisofs",
	                         "--version",
	                         "* %*s by <appro@fy.chalmers.se>, version %d.%d,",
	                         version);
}
