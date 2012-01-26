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

#include "burn-debug.h"
#include "burn-job.h"
#include "burn-process.h"
#include "rejilla-plugin-registration.h"
#include "burn-cdrtools.h"
#include "rejilla-track-data.h"


#define REJILLA_TYPE_MKISOFS         (rejilla_mkisofs_get_type ())
#define REJILLA_MKISOFS(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), REJILLA_TYPE_MKISOFS, RejillaMkisofs))
#define REJILLA_MKISOFS_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), REJILLA_TYPE_MKISOFS, RejillaMkisofsClass))
#define REJILLA_IS_MKISOFS(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), REJILLA_TYPE_MKISOFS))
#define REJILLA_IS_MKISOFS_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), REJILLA_TYPE_MKISOFS))
#define REJILLA_MKISOFS_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), REJILLA_TYPE_MKISOFS, RejillaMkisofsClass))

REJILLA_PLUGIN_BOILERPLATE (RejillaMkisofs, rejilla_mkisofs, REJILLA_TYPE_PROCESS, RejillaProcess);

struct _RejillaMkisofsPrivate {
	guint use_utf8:1;
};
typedef struct _RejillaMkisofsPrivate RejillaMkisofsPrivate;

#define REJILLA_MKISOFS_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REJILLA_TYPE_MKISOFS, RejillaMkisofsPrivate))
static GObjectClass *parent_class = NULL;

static RejillaBurnResult
rejilla_mkisofs_read_isosize (RejillaProcess *process, const gchar *line)
{
	gint64 sectors;

	sectors = strtoll (line, NULL, 10);
	if (!sectors)
		return REJILLA_BURN_OK;

	/* mkisofs reports blocks of 2048 bytes */
	rejilla_job_set_output_size_for_current_track (REJILLA_JOB (process),
						       sectors,
						       sectors * 2048ULL);
	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_mkisofs_read_stdout (RejillaProcess *process, const gchar *line)
{
	RejillaJobAction action;

	rejilla_job_get_action (REJILLA_JOB (process), &action);
	if (action == REJILLA_JOB_ACTION_SIZE)
		return rejilla_mkisofs_read_isosize (process, line);

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_mkisofs_read_stderr (RejillaProcess *process, const gchar *line)
{
	gchar fraction_str [7] = { 0, };
	RejillaMkisofs *mkisofs;
	RejillaMkisofsPrivate *priv;

	mkisofs = REJILLA_MKISOFS (process);
	priv = REJILLA_MKISOFS_PRIVATE (process);

	if (strstr (line, "estimate finish")
	&&  sscanf (line, "%6c%% done, estimate finish", fraction_str) == 1) {
		gdouble fraction;
	
		fraction = g_strtod (fraction_str, NULL) / (gdouble) 100.0;
		rejilla_job_set_progress (REJILLA_JOB (mkisofs), fraction);
		rejilla_job_start_progress (REJILLA_JOB (process), FALSE);
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
	else if (strstr (line, "Use mkisofs -help")) {
		rejilla_job_error (REJILLA_JOB (process), 
				   g_error_new_literal (REJILLA_BURN_ERROR,
							REJILLA_BURN_ERROR_GENERAL,
							_("This version of mkisofs is not supported")));
	}
/*	else if ((pos =  strstr (line,"mkisofs: Permission denied. "))) {
		int res = FALSE;
		gboolean isdir = FALSE;
		char *path = NULL;

		pos += strlen ("mkisofs: Permission denied. ");
		if (!strncmp (pos, "Unable to open directory ", 24)) {
			isdir = TRUE;

			pos += strlen ("Unable to open directory ");
			path = g_strdup (pos);
			path[strlen (path) - 1] = 0;
		}
		else if (!strncmp (pos, "File ", 5)) {
			char *end;

			isdir = FALSE;
			pos += strlen ("File ");
			end = strstr (pos, " is not readable - ignoring");
			if (end)
				path = g_strndup (pos, end - pos);
		}
		else
			return TRUE;

		res = rejilla_mkisofs_base_ask_unreadable_file (REJILLA_GENISOIMAGE_BASE (process),
								path,
								isdir);
		if (!res) {
			g_free (path);

			rejilla_job_progress_changed (REJILLA_JOB (process), 1.0, -1);
			rejilla_job_cancel (REJILLA_JOB (process), FALSE);
			return FALSE;
		}
	}*/
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
	else if (strstr (line, "No space left on device")) {
		rejilla_job_error (REJILLA_JOB (process),
				   g_error_new_literal (REJILLA_BURN_ERROR,
							REJILLA_BURN_ERROR_DISK_SPACE,
							_("There is no space left on the device")));

	}
	else if (strstr (line, "Unable to open disc image file")) {
		rejilla_job_error (REJILLA_JOB (process),
				   g_error_new_literal (REJILLA_BURN_ERROR,
							REJILLA_BURN_ERROR_PERMISSION,
							_("You do not have the required permission to write at this location")));

	}
	else if (strstr (line, "Value too large for defined data type")) {
		rejilla_job_error (REJILLA_JOB (process),
				   g_error_new_literal (REJILLA_BURN_ERROR,
							REJILLA_BURN_ERROR_MEDIUM_SPACE,
							_("Not enough space available on the disc")));
	}

	/** REMINDER: these should not be necessary

	else if (strstr (line, "Resource temporarily unavailable")) {
		rejilla_job_error (REJILLA_JOB (process),
				   g_error_new_literal (REJILLA_BURN_ERROR,
							REJILLA_BURN_ERROR_INPUT,
							_("Data could not be written")));
	}
	else if (strstr (line, "Bad file descriptor.")) {
		rejilla_job_error (REJILLA_JOB (process),
				   g_error_new_literal (REJILLA_BURN_ERROR,
							REJILLA_BURN_ERROR_INPUT,
							_("Internal error: bad file descriptor")));
	}

	**/

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_mkisofs_set_argv_image (RejillaMkisofs *mkisofs,
				GPtrArray *argv,
				GError **error)
{
	gchar *label = NULL;
	RejillaTrack *track;
	RejillaBurnFlag flags;
	gchar *videodir = NULL;
	gchar *emptydir = NULL;
	RejillaJobAction action;
	RejillaImageFS image_fs;
	RejillaBurnResult result;
	gchar *grafts_path = NULL;
	gchar *excluded_path = NULL;

	/* set argv */
	g_ptr_array_add (argv, g_strdup ("-r"));

	result = rejilla_job_get_current_track (REJILLA_JOB (mkisofs), &track);
	if (result != REJILLA_BURN_OK)
		REJILLA_JOB_NOT_READY (mkisofs);

	image_fs = rejilla_track_data_get_fs (REJILLA_TRACK_DATA (track));
	if (image_fs & REJILLA_IMAGE_FS_JOLIET)
		g_ptr_array_add (argv, g_strdup ("-J"));

	if ((image_fs & REJILLA_IMAGE_FS_ISO)
	&&  (image_fs & REJILLA_IMAGE_ISO_FS_LEVEL_3)) {
		g_ptr_array_add (argv, g_strdup ("-iso-level"));
		g_ptr_array_add (argv, g_strdup ("3"));
	}

	if (image_fs & REJILLA_IMAGE_FS_UDF)
		g_ptr_array_add (argv, g_strdup ("-udf"));

	if (image_fs & REJILLA_IMAGE_FS_VIDEO) {
		g_ptr_array_add (argv, g_strdup ("-dvd-video"));

		result = rejilla_job_get_tmp_dir (REJILLA_JOB (mkisofs),
						  &videodir,
						  error);
		if (result != REJILLA_BURN_OK)
			return result;
	}

	g_ptr_array_add (argv, g_strdup ("-graft-points"));

	if (image_fs & REJILLA_IMAGE_ISO_FS_DEEP_DIRECTORY)
		g_ptr_array_add (argv, g_strdup ("-D"));	// This is dangerous the manual says but apparently it works well

	result = rejilla_job_get_tmp_file (REJILLA_JOB (mkisofs),
					   NULL,
					   &grafts_path,
					   error);
	if (result != REJILLA_BURN_OK) {
		g_free (videodir);
		return result;
	}

	result = rejilla_job_get_tmp_file (REJILLA_JOB (mkisofs),
					   NULL,
					   &excluded_path,
					   error);
	if (result != REJILLA_BURN_OK) {
		g_free (grafts_path);
		g_free (videodir);
		return result;
	}

	result = rejilla_job_get_tmp_dir (REJILLA_JOB (mkisofs),
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

	rejilla_job_get_data_label (REJILLA_JOB (mkisofs), &label);
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
#if defined(HAVE_STRUCT_USCSI_CMD)
	g_ptr_array_add (argv, g_strdup ("SOLARIS"));
#else
	g_ptr_array_add (argv, g_strdup ("LINUX"));
#endif
	
	/* FIXME! -sort is an interesting option allowing to decide where the 
	* files are written on the disc and therefore to optimize later reading */
	/* FIXME: -hidden --hidden-list -hide-jolie -hide-joliet-list will allow to hide
	* some files when we will display the contents of a disc we will want to merge */
	/* FIXME: support preparer publisher options */

	rejilla_job_get_flags (REJILLA_JOB (mkisofs), &flags);
	if (flags & (REJILLA_BURN_FLAG_APPEND|REJILLA_BURN_FLAG_MERGE)) {
		goffset last_session = 0, next_wr_add = 0;
		gchar *startpoint = NULL;

		rejilla_job_get_last_session_address (REJILLA_JOB (mkisofs), &last_session);
		rejilla_job_get_next_writable_address (REJILLA_JOB (mkisofs), &next_wr_add);
		if (last_session == -1 || next_wr_add == -1) {
			g_free (videodir);
			REJILLA_JOB_LOG (mkisofs, "Failed to get the start point of the track. Make sure the media allow to add files (it is not closed)"); 
			g_set_error (error,
				     REJILLA_BURN_ERROR,
				     REJILLA_BURN_ERROR_GENERAL,
				     _("An internal error occurred"));
			return REJILLA_BURN_ERR;
		}

		startpoint = g_strdup_printf ("%"G_GINT64_FORMAT",%"G_GINT64_FORMAT,
					      last_session,
					      next_wr_add);

		g_ptr_array_add (argv, g_strdup ("-C"));
		g_ptr_array_add (argv, startpoint);

		if (flags & REJILLA_BURN_FLAG_MERGE) {
		        gchar *device = NULL;

			g_ptr_array_add (argv, g_strdup ("-M"));

			/* NOTE: that function returns either bus_target_lun or the device path
			 * according to OSes. Basically it returns bus/target/lun only for FreeBSD
			 * which is the only OS in need for that. For all others it returns the device
			 * path. */
			rejilla_job_get_bus_target_lun (REJILLA_JOB (mkisofs), &device);
			g_ptr_array_add (argv, device);
		}
	}

	rejilla_job_get_action (REJILLA_JOB (mkisofs), &action);
	if (action == REJILLA_JOB_ACTION_SIZE) {
		g_ptr_array_add (argv, g_strdup ("-quiet"));
		g_ptr_array_add (argv, g_strdup ("-print-size"));

		rejilla_job_set_current_action (REJILLA_JOB (mkisofs),
						REJILLA_BURN_ACTION_GETTING_SIZE,
						NULL,
						FALSE);
		rejilla_job_start_progress (REJILLA_JOB (mkisofs), FALSE);

		if (videodir) {
			g_ptr_array_add (argv, g_strdup ("-f"));
			g_ptr_array_add (argv, videodir);
		}

		return REJILLA_BURN_OK;
	}

	if (rejilla_job_get_fd_out (REJILLA_JOB (mkisofs), NULL) != REJILLA_BURN_OK) {
		gchar *output = NULL;

		result = rejilla_job_get_image_output (REJILLA_JOB (mkisofs),
						      &output,
						       NULL);
		if (result != REJILLA_BURN_OK) {
			g_free (videodir);
			return result;
		}

		g_ptr_array_add (argv, g_strdup ("-o"));
		g_ptr_array_add (argv, output);
	}

	if (videodir) {
		g_ptr_array_add (argv, g_strdup ("-f"));
		g_ptr_array_add (argv, videodir);
	}

	rejilla_job_set_current_action (REJILLA_JOB (mkisofs),
					REJILLA_BURN_ACTION_CREATING_IMAGE,
					NULL,
					FALSE);

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_mkisofs_set_argv (RejillaProcess *process,
			  GPtrArray *argv,
			  GError **error)
{
	gchar *prog_name;
	RejillaJobAction action;
	RejillaMkisofs *mkisofs;
	RejillaBurnResult result;
	RejillaMkisofsPrivate *priv;

	mkisofs = REJILLA_MKISOFS (process);
	priv = REJILLA_MKISOFS_PRIVATE (process);

	prog_name = g_find_program_in_path ("mkisofs");
	if (prog_name && g_file_test (prog_name, G_FILE_TEST_IS_EXECUTABLE))
		g_ptr_array_add (argv, prog_name);
	else
		g_ptr_array_add (argv, g_strdup ("mkisofs"));

	if (priv->use_utf8) {
		g_ptr_array_add (argv, g_strdup ("-input-charset"));
		g_ptr_array_add (argv, g_strdup ("utf8"));
	}

	rejilla_job_get_action (REJILLA_JOB (mkisofs), &action);
	if (action == REJILLA_JOB_ACTION_SIZE)
		result = rejilla_mkisofs_set_argv_image (mkisofs, argv, error);
	else if (action == REJILLA_JOB_ACTION_IMAGE)
		result = rejilla_mkisofs_set_argv_image (mkisofs, argv, error);
	else
		REJILLA_JOB_NOT_SUPPORTED (mkisofs);

	return result;
}

static void
rejilla_mkisofs_class_init (RejillaMkisofsClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	RejillaProcessClass *process_class = REJILLA_PROCESS_CLASS (klass);

	g_type_class_add_private (klass, sizeof (RejillaMkisofsPrivate));

	parent_class = g_type_class_peek_parent(klass);
	object_class->finalize = rejilla_mkisofs_finalize;

	process_class->stdout_func = rejilla_mkisofs_read_stdout;
	process_class->stderr_func = rejilla_mkisofs_read_stderr;
	process_class->set_argv = rejilla_mkisofs_set_argv;
}

static void
rejilla_mkisofs_init (RejillaMkisofs *obj)
{
	RejillaMkisofsPrivate *priv;
	gchar *standard_error;
	gboolean res;

	priv = REJILLA_MKISOFS_PRIVATE (obj);

	/* this code used to be ncb_mkisofs_supports_utf8 */
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
rejilla_mkisofs_finalize (GObject *object)
{
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
rejilla_mkisofs_export_caps (RejillaPlugin *plugin)
{
	GSList *output;
	GSList *input;

	rejilla_plugin_define (plugin,
			       "mkisofs",
	                       NULL,
			       _("Creates disc images from a file selection"),
			       "Philippe Rouquier",
			       2);

	rejilla_plugin_set_flags (plugin,
				  REJILLA_MEDIUM_CDR|
				  REJILLA_MEDIUM_CDRW|
				  REJILLA_MEDIUM_DVDR|
				  REJILLA_MEDIUM_DVDRW|
				  REJILLA_MEDIUM_DVDR_PLUS|
				  REJILLA_MEDIUM_DUAL_L|
				  REJILLA_MEDIUM_APPENDABLE|
				  REJILLA_MEDIUM_HAS_AUDIO|
				  REJILLA_MEDIUM_HAS_DATA,
				  REJILLA_BURN_FLAG_APPEND|
				  REJILLA_BURN_FLAG_MERGE,
				  REJILLA_BURN_FLAG_NONE);

	rejilla_plugin_set_flags (plugin,
				  REJILLA_MEDIUM_DUAL_L|
				  REJILLA_MEDIUM_DVDRW_PLUS|
				  REJILLA_MEDIUM_RESTRICTED|
				  REJILLA_MEDIUM_APPENDABLE|
				  REJILLA_MEDIUM_CLOSED|
				  REJILLA_MEDIUM_HAS_DATA,
				  REJILLA_BURN_FLAG_APPEND|
				  REJILLA_BURN_FLAG_MERGE,
				  REJILLA_BURN_FLAG_NONE);

	/* Caps */
	output = rejilla_caps_image_new (REJILLA_PLUGIN_IO_ACCEPT_FILE|
					 REJILLA_PLUGIN_IO_ACCEPT_PIPE,
					 REJILLA_IMAGE_FORMAT_BIN);

	input = rejilla_caps_data_new (REJILLA_IMAGE_FS_ISO|
				       REJILLA_IMAGE_FS_UDF|
				       REJILLA_IMAGE_ISO_FS_LEVEL_3|
				       REJILLA_IMAGE_ISO_FS_DEEP_DIRECTORY|
				       REJILLA_IMAGE_FS_JOLIET|
				       REJILLA_IMAGE_FS_VIDEO);
	rejilla_plugin_link_caps (plugin, output, input);
	g_slist_free (input);

	input = rejilla_caps_data_new (REJILLA_IMAGE_FS_ISO|
				       REJILLA_IMAGE_ISO_FS_LEVEL_3|
				       REJILLA_IMAGE_ISO_FS_DEEP_DIRECTORY|
				       REJILLA_IMAGE_FS_SYMLINK);
	rejilla_plugin_link_caps (plugin, output, input);
	g_slist_free (input);

	g_slist_free (output);

	rejilla_plugin_register_group (plugin, _(CDRTOOLS_DESCRIPTION));
}

G_MODULE_EXPORT void
rejilla_plugin_check_config (RejillaPlugin *plugin)
{
	gint version [3] = { 2, 0, -1};
	rejilla_plugin_test_app (plugin,
	                         "mkisofs",
	                         "--version",
	                         "mkisofs %d.%d",
	                         version);
}
