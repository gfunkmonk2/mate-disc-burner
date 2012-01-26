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

#include <string.h>
#include <stdlib.h>

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>
#include <gmodule.h>

#include "burn-cdrtools.h"
#include "burn-process.h"
#include "burn-job.h"
#include "rejilla-plugin-registration.h"
#include "rejilla-tags.h"
#include "rejilla-track-disc.h"

#include "burn-volume.h"
#include "rejilla-drive.h"


#define REJILLA_TYPE_READCD         (rejilla_readcd_get_type ())
#define REJILLA_READCD(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), REJILLA_TYPE_READCD, RejillaReadcd))
#define REJILLA_READCD_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), REJILLA_TYPE_READCD, RejillaReadcdClass))
#define REJILLA_IS_READCD(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), REJILLA_TYPE_READCD))
#define REJILLA_IS_READCD_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), REJILLA_TYPE_READCD))
#define REJILLA_READCD_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), REJILLA_TYPE_READCD, RejillaReadcdClass))

REJILLA_PLUGIN_BOILERPLATE (RejillaReadcd, rejilla_readcd, REJILLA_TYPE_PROCESS, RejillaProcess);
static GObjectClass *parent_class = NULL;

static RejillaBurnResult
rejilla_readcd_read_stderr (RejillaProcess *process, const gchar *line)
{
	RejillaReadcd *readcd;
	gint dummy1;
	gint dummy2;
	gchar *pos;

	readcd = REJILLA_READCD (process);

	if ((pos = strstr (line, "addr:"))) {
		gint sector;
		gint64 written;
		RejillaImageFormat format;
		RejillaTrackType *output = NULL;

		pos += strlen ("addr:");
		sector = strtoll (pos, NULL, 10);

		output = rejilla_track_type_new ();
		rejilla_job_get_output_type (REJILLA_JOB (readcd), output);

		format = rejilla_track_type_get_image_format (output);
		if (format == REJILLA_IMAGE_FORMAT_BIN)
			written = (gint64) ((gint64) sector * 2048ULL);
		else if (format == REJILLA_IMAGE_FORMAT_CLONE)
			written = (gint64) ((gint64) sector * 2448ULL);
		else
			written = (gint64) ((gint64) sector * 2048ULL);

		rejilla_track_type_free (output);

		rejilla_job_set_written_track (REJILLA_JOB (readcd), written);

		if (sector > 10)
			rejilla_job_start_progress (REJILLA_JOB (readcd), FALSE);
	}
	else if ((pos = strstr (line, "Capacity:"))) {
		rejilla_job_set_current_action (REJILLA_JOB (readcd),
							REJILLA_BURN_ACTION_DRIVE_COPY,
							NULL,
							FALSE);
	}
	else if (strstr (line, "Device not ready.")) {
		rejilla_job_error (REJILLA_JOB (readcd),
				   g_error_new (REJILLA_BURN_ERROR,
						REJILLA_BURN_ERROR_DRIVE_BUSY,
						_("The drive is busy")));
	}
	else if (strstr (line, "Cannot open SCSI driver.")) {
		rejilla_job_error (REJILLA_JOB (readcd),
				   g_error_new (REJILLA_BURN_ERROR,
						REJILLA_BURN_ERROR_PERMISSION,
						_("You do not have the required permissions to use this drive")));		
	}
	else if (strstr (line, "Cannot send SCSI cmd via ioctl")) {
		rejilla_job_error (REJILLA_JOB (readcd),
				   g_error_new (REJILLA_BURN_ERROR,
						REJILLA_BURN_ERROR_PERMISSION,
						_("You do not have the required permissions to use this drive")));
	}
	/* we scan for this error as in this case readcd returns success */
	else if (sscanf (line, "Input/output error. Error on sector %d not corrected. Total of %d error", &dummy1, &dummy2) == 2) {
		rejilla_job_error (REJILLA_JOB (process),
				   g_error_new (REJILLA_BURN_ERROR,
						REJILLA_BURN_ERROR_GENERAL,
						_("An internal error occurred")));
	}
	else if (strstr (line, "No space left on device")) {
		/* This is necessary as readcd won't return an error code on exit */
		rejilla_job_error (REJILLA_JOB (readcd),
				   g_error_new (REJILLA_BURN_ERROR,
						REJILLA_BURN_ERROR_DISK_SPACE,
						_("The location you chose to store the image on does not have enough free space for the disc image")));
	}

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_readcd_argv_set_iso_boundary (RejillaReadcd *readcd,
				      GPtrArray *argv,
				      GError **error)
{
	goffset nb_blocks;
	RejillaTrack *track;
	GValue *value = NULL;
	RejillaTrackType *output = NULL;

	rejilla_job_get_current_track (REJILLA_JOB (readcd), &track);

	output = rejilla_track_type_new ();
	rejilla_job_get_output_type (REJILLA_JOB (readcd), output);

	rejilla_track_tag_lookup (track,
				  REJILLA_TRACK_MEDIUM_ADDRESS_START_TAG,
				  &value);
	if (value) {
		guint64 start, end;

		/* we were given an address to start */
		start = g_value_get_uint64 (value);

		/* get the length now */
		value = NULL;
		rejilla_track_tag_lookup (track,
					  REJILLA_TRACK_MEDIUM_ADDRESS_END_TAG,
					  &value);

		end = g_value_get_uint64 (value);

		REJILLA_JOB_LOG (readcd,
				 "reading from sector %"G_GINT64_FORMAT" to %"G_GINT64_FORMAT,
				 start,
				 end);
		g_ptr_array_add (argv, g_strdup_printf ("-sectors=%"G_GINT64_FORMAT"-%"G_GINT64_FORMAT,
							start,
							end));
	}
	/* 0 means all disc, -1 problem */
	else if (rejilla_track_disc_get_track_num (REJILLA_TRACK_DISC (track)) > 0) {
		goffset start;
		RejillaDrive *drive;
		RejillaMedium *medium;

		drive = rejilla_track_disc_get_drive (REJILLA_TRACK_DISC (track));
		medium = rejilla_drive_get_medium (drive);
		rejilla_medium_get_track_space (medium,
						rejilla_track_disc_get_track_num (REJILLA_TRACK_DISC (track)),
						NULL,
						&nb_blocks);
		rejilla_medium_get_track_address (medium,
						  rejilla_track_disc_get_track_num (REJILLA_TRACK_DISC (track)),
						  NULL,
						  &start);

		REJILLA_JOB_LOG (readcd,
				 "reading %i from sector %"G_GINT64_FORMAT" to %"G_GINT64_FORMAT,
				 rejilla_track_disc_get_track_num (REJILLA_TRACK_DISC (track)),
				 start,
				 start + nb_blocks);
		g_ptr_array_add (argv, g_strdup_printf ("-sectors=%"G_GINT64_FORMAT"-%"G_GINT64_FORMAT,
							start,
							start + nb_blocks));
	}
	/* if it's BIN output just read the last track */
	else if (rejilla_track_type_get_image_format (output) == REJILLA_IMAGE_FORMAT_BIN) {
		goffset start;
		RejillaDrive *drive;
		RejillaMedium *medium;

		drive = rejilla_track_disc_get_drive (REJILLA_TRACK_DISC (track));
		medium = rejilla_drive_get_medium (drive);
		rejilla_medium_get_last_data_track_space (medium,
							  NULL,
							  &nb_blocks);
		rejilla_medium_get_last_data_track_address (medium,
							    NULL,
							    &start);
		REJILLA_JOB_LOG (readcd,
				 "reading last track from sector %"G_GINT64_FORMAT" to %"G_GINT64_FORMAT,
				 start,
				 start + nb_blocks);
		g_ptr_array_add (argv, g_strdup_printf ("-sectors=%"G_GINT64_FORMAT"-%"G_GINT64_FORMAT,
							start,
							start + nb_blocks));
	}
	else {
		rejilla_track_get_size (track, &nb_blocks, NULL);
		g_ptr_array_add (argv, g_strdup_printf ("-sectors=0-%"G_GINT64_FORMAT, nb_blocks));
	}

	rejilla_track_type_free (output);

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_readcd_get_size (RejillaReadcd *self,
			 GError **error)
{
	goffset blocks;
	GValue *value = NULL;
	RejillaImageFormat format;
	RejillaTrack *track = NULL;
	RejillaTrackType *output = NULL;

	output = rejilla_track_type_new ();
	rejilla_job_get_output_type (REJILLA_JOB (self), output);

	if (!rejilla_track_type_get_has_image (output)) {
		rejilla_track_type_free (output);
		return REJILLA_BURN_ERR;
	}

	format = rejilla_track_type_get_image_format (output);
	rejilla_track_type_free (output);

	rejilla_job_get_current_track (REJILLA_JOB (self), &track);
	rejilla_track_tag_lookup (track,
				  REJILLA_TRACK_MEDIUM_ADDRESS_START_TAG,
				  &value);

	if (value) {
		guint64 start, end;

		/* we were given an address to start */
		start = g_value_get_uint64 (value);

		/* get the length now */
		value = NULL;
		rejilla_track_tag_lookup (track,
					  REJILLA_TRACK_MEDIUM_ADDRESS_END_TAG,
					  &value);

		end = g_value_get_uint64 (value);
		blocks = end - start;
	}
	else if (rejilla_track_disc_get_track_num (REJILLA_TRACK_DISC (track)) > 0) {
		RejillaDrive *drive;
		RejillaMedium *medium;

		drive = rejilla_track_disc_get_drive (REJILLA_TRACK_DISC (track));
		medium = rejilla_drive_get_medium (drive);
		rejilla_medium_get_track_space (medium,
						rejilla_track_disc_get_track_num (REJILLA_TRACK_DISC (track)),
						NULL,
						&blocks);
	}
	else if (format == REJILLA_IMAGE_FORMAT_BIN) {
		RejillaDrive *drive;
		RejillaMedium *medium;

		drive = rejilla_track_disc_get_drive (REJILLA_TRACK_DISC (track));
		medium = rejilla_drive_get_medium (drive);
		rejilla_medium_get_last_data_track_space (medium,
							  NULL,
							  &blocks);
	}
	else
		rejilla_track_get_size (track, &blocks, NULL);

	if (format == REJILLA_IMAGE_FORMAT_BIN) {
		rejilla_job_set_output_size_for_current_track (REJILLA_JOB (self),
							       blocks,
							       blocks * 2048ULL);
	}
	else if (format == REJILLA_IMAGE_FORMAT_CLONE) {
		rejilla_job_set_output_size_for_current_track (REJILLA_JOB (self),
							       blocks,
							       blocks * 2448ULL);
	}
	else
		return REJILLA_BURN_NOT_SUPPORTED;

	/* no need to go any further */
	return REJILLA_BURN_NOT_RUNNING;
}

static RejillaBurnResult
rejilla_readcd_set_argv (RejillaProcess *process,
			 GPtrArray *argv,
			 GError **error)
{
	RejillaBurnResult result = FALSE;
	RejillaTrackType *output = NULL;
	RejillaImageFormat format;
	RejillaJobAction action;
	RejillaReadcd *readcd;
	RejillaMedium *medium;
	RejillaTrack *track;
	RejillaDrive *drive;
	RejillaMedia media;
	gchar *outfile_arg;
	gchar *dev_str;
	gchar *device;

	readcd = REJILLA_READCD (process);

	/* This is a kind of shortcut */
	rejilla_job_get_action (REJILLA_JOB (process), &action);
	if (action == REJILLA_JOB_ACTION_SIZE)
		return rejilla_readcd_get_size (readcd, error);

	g_ptr_array_add (argv, g_strdup ("readcd"));

	rejilla_job_get_current_track (REJILLA_JOB (readcd), &track);
	drive = rejilla_track_disc_get_drive (REJILLA_TRACK_DISC (track));

	/* NOTE: that function returns either bus_target_lun or the device path
	 * according to OSes. Basically it returns bus/target/lun only for FreeBSD
	 * which is the only OS in need for that. For all others it returns the device
	 * path. */
	device = rejilla_drive_get_bus_target_lun_string (drive);

	if (!device)
		return REJILLA_BURN_ERR;

	dev_str = g_strdup_printf ("dev=%s", device);
	g_ptr_array_add (argv, dev_str);
	g_free (device);

	g_ptr_array_add (argv, g_strdup ("-nocorr"));

	medium = rejilla_drive_get_medium (drive);
	media = rejilla_medium_get_status (medium);

	output = rejilla_track_type_new ();
	rejilla_job_get_output_type (REJILLA_JOB (readcd), output);
	format = rejilla_track_type_get_image_format (output);
	rejilla_track_type_free (output);

	if ((media & REJILLA_MEDIUM_DVD) && format != REJILLA_IMAGE_FORMAT_BIN) {
		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_GENERAL,
			     _("An internal error occurred"));
		return REJILLA_BURN_ERR;
	}

	if (format == REJILLA_IMAGE_FORMAT_CLONE) {
		/* NOTE: with this option the sector size is 2448 
		 * because it is raw96 (2352+96) otherwise it is 2048  */
		g_ptr_array_add (argv, g_strdup ("-clone"));
	}
	else if (format == REJILLA_IMAGE_FORMAT_BIN) {
		g_ptr_array_add (argv, g_strdup ("-noerror"));

		/* don't do it for clone since we need the entire disc */
		result = rejilla_readcd_argv_set_iso_boundary (readcd, argv, error);
		if (result != REJILLA_BURN_OK)
			return result;
	}
	else
		REJILLA_JOB_NOT_SUPPORTED (readcd);

	if (rejilla_job_get_fd_out (REJILLA_JOB (readcd), NULL) != REJILLA_BURN_OK) {
		gchar *image;

		if (format != REJILLA_IMAGE_FORMAT_CLONE
		&&  format != REJILLA_IMAGE_FORMAT_BIN)
			REJILLA_JOB_NOT_SUPPORTED (readcd);

		result = rejilla_job_get_image_output (REJILLA_JOB (readcd),
						       &image,
						       NULL);
		if (result != REJILLA_BURN_OK)
			return result;

		outfile_arg = g_strdup_printf ("-f=%s", image);
		g_ptr_array_add (argv, outfile_arg);
		g_free (image);
	}
	else if (format == REJILLA_IMAGE_FORMAT_BIN) {
		outfile_arg = g_strdup ("-f=-");
		g_ptr_array_add (argv, outfile_arg);
	}
	else 	/* unfortunately raw images can't be piped out */
		REJILLA_JOB_NOT_SUPPORTED (readcd);

	rejilla_job_set_use_average_rate (REJILLA_JOB (process), TRUE);
	return REJILLA_BURN_OK;
}

static void
rejilla_readcd_class_init (RejillaReadcdClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	RejillaProcessClass *process_class = REJILLA_PROCESS_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = rejilla_readcd_finalize;

	process_class->stderr_func = rejilla_readcd_read_stderr;
	process_class->set_argv = rejilla_readcd_set_argv;
}

static void
rejilla_readcd_init (RejillaReadcd *obj)
{ }

static void
rejilla_readcd_finalize (GObject *object)
{
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
rejilla_readcd_export_caps (RejillaPlugin *plugin)
{
	GSList *output;
	GSList *input;

	rejilla_plugin_define (plugin,
			       "readcd",
	                       NULL,
			       _("Copies any disc to a disc image"),
			       "Philippe Rouquier",
			       0);

	/* that's for clone mode only The only one to copy audio */
	output = rejilla_caps_image_new (REJILLA_PLUGIN_IO_ACCEPT_FILE,
					 REJILLA_IMAGE_FORMAT_CLONE);

	input = rejilla_caps_disc_new (REJILLA_MEDIUM_CD|
				       REJILLA_MEDIUM_ROM|
				       REJILLA_MEDIUM_WRITABLE|
				       REJILLA_MEDIUM_REWRITABLE|
				       REJILLA_MEDIUM_APPENDABLE|
				       REJILLA_MEDIUM_CLOSED|
				       REJILLA_MEDIUM_HAS_AUDIO|
				       REJILLA_MEDIUM_HAS_DATA);

	rejilla_plugin_link_caps (plugin, output, input);
	g_slist_free (output);
	g_slist_free (input);

	/* that's for regular mode: it accepts the previous type of discs 
	 * plus the DVDs types as well */
	output = rejilla_caps_image_new (REJILLA_PLUGIN_IO_ACCEPT_FILE|
					 REJILLA_PLUGIN_IO_ACCEPT_PIPE,
					 REJILLA_IMAGE_FORMAT_BIN);

	input = rejilla_caps_disc_new (REJILLA_MEDIUM_CD|
				       REJILLA_MEDIUM_DVD|
				       REJILLA_MEDIUM_DUAL_L|
				       REJILLA_MEDIUM_PLUS|
				       REJILLA_MEDIUM_SEQUENTIAL|
				       REJILLA_MEDIUM_RESTRICTED|
				       REJILLA_MEDIUM_ROM|
				       REJILLA_MEDIUM_WRITABLE|
				       REJILLA_MEDIUM_REWRITABLE|
				       REJILLA_MEDIUM_CLOSED|
				       REJILLA_MEDIUM_APPENDABLE|
				       REJILLA_MEDIUM_HAS_DATA);

	rejilla_plugin_link_caps (plugin, output, input);
	g_slist_free (output);
	g_slist_free (input);

	rejilla_plugin_register_group (plugin, _(CDRTOOLS_DESCRIPTION));
}

G_MODULE_EXPORT void
rejilla_plugin_check_config (RejillaPlugin *plugin)
{
	gint version [3] = { 2, 0, -1};
	rejilla_plugin_test_app (plugin,
	                         "readcd",
	                         "--version",
	                         "readcd %d.%d",
	                         version);
}
