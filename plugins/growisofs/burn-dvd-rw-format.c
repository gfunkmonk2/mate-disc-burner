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

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>

#include <gmodule.h>

#include "burn-basics.h"
#include "rejilla-plugin.h"
#include "rejilla-plugin-registration.h"
#include "burn-job.h"
#include "burn-process.h"
#include "rejilla-medium.h"
#include "burn-growisofs-common.h"


#define REJILLA_TYPE_DVD_RW_FORMAT         (rejilla_dvd_rw_format_get_type ())
#define REJILLA_DVD_RW_FORMAT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), REJILLA_TYPE_DVD_RW_FORMAT, RejillaDvdRwFormat))
#define REJILLA_DVD_RW_FORMAT_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), REJILLA_TYPE_DVD_RW_FORMAT, RejillaDvdRwFormatClass))
#define REJILLA_IS_DVD_RW_FORMAT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), REJILLA_TYPE_DVD_RW_FORMAT))
#define REJILLA_IS_DVD_RW_FORMAT_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), REJILLA_TYPE_DVD_RW_FORMAT))
#define REJILLA_DVD_RW_FORMAT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), REJILLA_TYPE_DVD_RW_FORMAT, RejillaDvdRwFormatClass))

REJILLA_PLUGIN_BOILERPLATE (RejillaDvdRwFormat, rejilla_dvd_rw_format, REJILLA_TYPE_PROCESS, RejillaProcess);

static GObjectClass *parent_class = NULL;

static RejillaBurnResult
rejilla_dvd_rw_format_read_stderr (RejillaProcess *process, const gchar *line)
{
	int perc_1 = 0, perc_2 = 0;
	float percent;

	if (strstr (line, "unable to proceed with format")
	||  strstr (line, "media is not blank")
	||  strstr (line, "media is already formatted")
	||  strstr (line, "you have the option to re-run")) {
		rejilla_job_error (REJILLA_JOB (process),
				   g_error_new (REJILLA_BURN_ERROR,
						REJILLA_BURN_ERROR_MEDIUM_INVALID,
						_("The disc is not supported")));
		return REJILLA_BURN_OK;
	}
	else if (strstr (line, "unable to umount")) {
		rejilla_job_error (REJILLA_JOB (process),
				   g_error_new (REJILLA_BURN_ERROR,
						REJILLA_BURN_ERROR_DRIVE_BUSY,
						_("The drive is busy")));
		return REJILLA_BURN_OK;
	}

	if ((sscanf (line, "* blanking %d.%1d%%,", &perc_1, &perc_2) == 2)
	||  (sscanf (line, "* formatting %d.%1d%%,", &perc_1, &perc_2) == 2)
	||  (sscanf (line, "* relocating lead-out %d.%1d%%,", &perc_1, &perc_2) == 2))
		rejilla_job_set_dangerous (REJILLA_JOB (process), TRUE);
	else 
		sscanf (line, "%d.%1d%%", &perc_1, &perc_2);

	percent = (float) perc_1 / 100.0 + (float) perc_2 / 1000.0;
	if (percent) {
		rejilla_job_start_progress (REJILLA_JOB (process), FALSE);
		rejilla_job_set_progress (REJILLA_JOB (process), percent);
	}

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_dvd_rw_format_set_argv (RejillaProcess *process,
				GPtrArray *argv,
				GError **error)
{
	RejillaMedia media;
	RejillaBurnFlag flags;
	gchar *device;

	g_ptr_array_add (argv, g_strdup ("dvd+rw-format"));

	/* undocumented option to show progress */
	g_ptr_array_add (argv, g_strdup ("-gui"));

	rejilla_job_get_media (REJILLA_JOB (process), &media);
	rejilla_job_get_flags (REJILLA_JOB (process), &flags);
        if (!REJILLA_MEDIUM_IS (media, REJILLA_MEDIUM_BDRE)
	&&  !REJILLA_MEDIUM_IS (media, REJILLA_MEDIUM_DVDRW_PLUS)
	&&  !REJILLA_MEDIUM_IS (media, REJILLA_MEDIUM_DVDRW_RESTRICTED)
	&&  (flags & REJILLA_BURN_FLAG_FAST_BLANK)) {
		gchar *blank_str;

		/* This creates a sequential DVD-RW */
		blank_str = g_strdup_printf ("-blank%s",
					     (flags & REJILLA_BURN_FLAG_FAST_BLANK) ? "" : "=full");
		g_ptr_array_add (argv, blank_str);
	}
	else {
		gchar *format_str;

		/* This creates a restricted overwrite DVD-RW or reformat a + */
		format_str = g_strdup ("-force");
		g_ptr_array_add (argv, format_str);
	}

	rejilla_job_get_device (REJILLA_JOB (process), &device);
	g_ptr_array_add (argv, device);

	rejilla_job_set_current_action (REJILLA_JOB (process),
					REJILLA_BURN_ACTION_BLANKING,
					NULL,
					FALSE);
	return REJILLA_BURN_OK;
}

static void
rejilla_dvd_rw_format_class_init (RejillaDvdRwFormatClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	RejillaProcessClass *process_class = REJILLA_PROCESS_CLASS (klass);

	parent_class = g_type_class_peek_parent(klass);
	object_class->finalize = rejilla_dvd_rw_format_finalize;

	process_class->set_argv = rejilla_dvd_rw_format_set_argv;
	process_class->stderr_func = rejilla_dvd_rw_format_read_stderr;
	process_class->post = rejilla_job_finished_session;
}

static void
rejilla_dvd_rw_format_init (RejillaDvdRwFormat *obj)
{ }

static void
rejilla_dvd_rw_format_finalize (GObject *object)
{
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
rejilla_dvd_rw_format_export_caps (RejillaPlugin *plugin)
{
	/* NOTE: sequential and restricted are added later on demand */
	const RejillaMedia media = REJILLA_MEDIUM_DVD|
				   REJILLA_MEDIUM_DUAL_L|
				   REJILLA_MEDIUM_REWRITABLE|
				   REJILLA_MEDIUM_APPENDABLE|
				   REJILLA_MEDIUM_CLOSED|
				   REJILLA_MEDIUM_HAS_DATA|
				   REJILLA_MEDIUM_UNFORMATTED|
				   REJILLA_MEDIUM_BLANK;
	GSList *output;

	rejilla_plugin_define (plugin,
			       "dvd-rw-format",
	                       NULL,
			       _("Blanks and formats rewritable DVDs and BDs"),
			       "Philippe Rouquier",
			       4);

	output = rejilla_caps_disc_new (media|
					REJILLA_MEDIUM_BDRE|
					REJILLA_MEDIUM_PLUS|
					REJILLA_MEDIUM_RESTRICTED|
					REJILLA_MEDIUM_SEQUENTIAL);
	rejilla_plugin_blank_caps (plugin, output);
	g_slist_free (output);

	rejilla_plugin_set_blank_flags (plugin,
					media|
					REJILLA_MEDIUM_BDRE|
					REJILLA_MEDIUM_PLUS|
					REJILLA_MEDIUM_RESTRICTED,
					REJILLA_BURN_FLAG_NOGRACE,
					REJILLA_BURN_FLAG_NONE);
	rejilla_plugin_set_blank_flags (plugin,
					media|
					REJILLA_MEDIUM_SEQUENTIAL,
					REJILLA_BURN_FLAG_NOGRACE|
					REJILLA_BURN_FLAG_FAST_BLANK,
					REJILLA_BURN_FLAG_NONE);

	rejilla_plugin_register_group (plugin, _(GROWISOFS_DESCRIPTION));
}

G_MODULE_EXPORT void
rejilla_plugin_check_config (RejillaPlugin *plugin)
{
	gint version [3] = { 5, 0, -1};
	rejilla_plugin_test_app (plugin,
	                         "dvd+rw-format",
	                         "-v",
	                         "* BD/DVD±RW/-RAM format utility by <appro@fy.chalmers.se>, version %d.%d",
	                         version);
}
