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

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>
#include <gmodule.h>

#include <libxml/xmlerror.h>
#include <libxml/xmlwriter.h>
#include <libxml/parser.h>
#include <libxml/xmlstring.h>
#include <libxml/uri.h>

#include "rejilla-plugin-registration.h"
#include "burn-job.h"
#include "burn-process.h"
#include "rejilla-track-data.h"
#include "rejilla-track-stream.h"


#define REJILLA_TYPE_DVD_AUTHOR             (rejilla_dvd_author_get_type ())
#define REJILLA_DVD_AUTHOR(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_DVD_AUTHOR, RejillaDvdAuthor))
#define REJILLA_DVD_AUTHOR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_DVD_AUTHOR, RejillaDvdAuthorClass))
#define REJILLA_IS_DVD_AUTHOR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_DVD_AUTHOR))
#define REJILLA_IS_DVD_AUTHOR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_DVD_AUTHOR))
#define REJILLA_DVD_AUTHOR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_DVD_AUTHOR, RejillaDvdAuthorClass))

REJILLA_PLUGIN_BOILERPLATE (RejillaDvdAuthor, rejilla_dvd_author, REJILLA_TYPE_PROCESS, RejillaProcess);

typedef struct _RejillaDvdAuthorPrivate RejillaDvdAuthorPrivate;
struct _RejillaDvdAuthorPrivate
{
	gchar *output;
};

#define REJILLA_DVD_AUTHOR_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REJILLA_TYPE_DVD_AUTHOR, RejillaDvdAuthorPrivate))

static RejillaProcessClass *parent_class = NULL;

static RejillaBurnResult
rejilla_dvd_author_add_track (RejillaJob *job)
{
	gchar *path;
	GSList *grafts = NULL;
	RejillaGraftPt *graft;
	RejillaTrackData *track;
	RejillaDvdAuthorPrivate *priv;

	priv = REJILLA_DVD_AUTHOR_PRIVATE (job);

	/* create the track */
	track = rejilla_track_data_new ();

	/* audio */
	graft = g_new (RejillaGraftPt, 1);
	path = g_build_path (G_DIR_SEPARATOR_S,
			     priv->output,
			     "AUDIO_TS",
			     NULL);
	graft->uri = g_filename_to_uri (path, NULL, NULL);
	g_free (path);

	graft->path = g_strdup ("/AUDIO_TS");
	grafts = g_slist_prepend (grafts, graft);

	REJILLA_JOB_LOG (job, "Adding graft point for %s", graft->uri);

	/* video */
	graft = g_new (RejillaGraftPt, 1);
	path = g_build_path (G_DIR_SEPARATOR_S,
			     priv->output,
			     "VIDEO_TS",
			     NULL);
	graft->uri = g_filename_to_uri (path, NULL, NULL);
	g_free (path);

	graft->path = g_strdup ("/VIDEO_TS");
	grafts = g_slist_prepend (grafts, graft);

	REJILLA_JOB_LOG (job, "Adding graft point for %s", graft->uri);

	rejilla_track_data_add_fs (track,
				   REJILLA_IMAGE_FS_ISO|
				   REJILLA_IMAGE_FS_UDF|
				   REJILLA_IMAGE_FS_VIDEO);
	rejilla_track_data_set_source (track,
				       grafts,
				       NULL);
	rejilla_job_add_track (job, REJILLA_TRACK (track));
	g_object_unref (track);

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_dvd_author_read_stdout (RejillaProcess *process,
				const gchar *line)
{
	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_dvd_author_read_stderr (RejillaProcess *process,
				const gchar *line)
{
	gint percent = 0;

	if (sscanf (line, "STAT: fixing VOBU at %*s (%*d/%*d, %d%%)", &percent) == 1) {
		rejilla_job_start_progress (REJILLA_JOB (process), FALSE);
		rejilla_job_set_progress (REJILLA_JOB (process),
					  (gdouble) ((gdouble) percent) / 100.0);
	}

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_dvd_author_generate_xml_file (RejillaProcess *process,
				      const gchar *path,
				      GError **error)
{
	RejillaDvdAuthorPrivate *priv;
	RejillaBurnResult result;
	GSList *tracks = NULL;
	xmlTextWriter *xml;
	gint success;
	GSList *iter;

	REJILLA_JOB_LOG (process, "Creating DVD layout xml file(%s)", path);

	xml = xmlNewTextWriterFilename (path, 0);
	if (!xml)
		return REJILLA_BURN_ERR;

	priv = REJILLA_DVD_AUTHOR_PRIVATE (process);

	xmlTextWriterSetIndent (xml, 1);
	xmlTextWriterSetIndentString (xml, (xmlChar *) "\t");

	success = xmlTextWriterStartDocument (xml,
					      NULL,
					      "UTF8",
					      NULL);
	if (success < 0)
		goto error;

	result = rejilla_job_get_tmp_dir (REJILLA_JOB (process),
					  &priv->output,
					  error);
	if (result != REJILLA_BURN_OK)
		return result;

	/* let's start */
	success = xmlTextWriterStartElement (xml, (xmlChar *) "dvdauthor");
	if (success < 0)
		goto error;

	success = xmlTextWriterWriteAttribute (xml,
					       (xmlChar *) "dest",
					       (xmlChar *) priv->output);
	if (success < 0)
		goto error;

	/* This is needed to finalize */
	success = xmlTextWriterWriteElement (xml, (xmlChar *) "vmgm", (xmlChar *) "");
	if (success < 0)
		goto error;

	/* the tracks */
	success = xmlTextWriterStartElement (xml, (xmlChar *) "titleset");
	if (success < 0)
		goto error;

	success = xmlTextWriterStartElement (xml, (xmlChar *) "titles");
	if (success < 0)
		goto error;

	/* get all tracks */
	rejilla_job_get_tracks (REJILLA_JOB (process), &tracks);
	for (iter = tracks; iter; iter = iter->next) {
		RejillaTrack *track;
		gchar *video;

		track = iter->data;
		success = xmlTextWriterStartElement (xml, (xmlChar *) "pgc");
		if (success < 0)
			goto error;

		success = xmlTextWriterStartElement (xml, (xmlChar *) "vob");
		if (success < 0)
			goto error;

		video = rejilla_track_stream_get_source (REJILLA_TRACK_STREAM (track), FALSE);
		success = xmlTextWriterWriteAttribute (xml,
						       (xmlChar *) "file",
						       (xmlChar *) video);
		g_free (video);

		if (success < 0)
			goto error;

		/* vob */
		success = xmlTextWriterEndElement (xml);
		if (success < 0)
			goto error;

		/* pgc */
		success = xmlTextWriterEndElement (xml);
		if (success < 0)
			goto error;
	}

	/* titles */
	success = xmlTextWriterEndElement (xml);
	if (success < 0)
		goto error;

	/* titleset */
	success = xmlTextWriterEndElement (xml);
	if (success < 0)
		goto error;

	/* close dvdauthor */
	success = xmlTextWriterEndElement (xml);
	if (success < 0)
		goto error;

	xmlTextWriterEndDocument (xml);
	xmlFreeTextWriter (xml);

	return REJILLA_BURN_OK;

error:

	REJILLA_JOB_LOG (process, "Error");

	/* close everything */
	xmlTextWriterEndDocument (xml);
	xmlFreeTextWriter (xml);

	/* FIXME: get the error */

	return REJILLA_BURN_ERR;
}

static RejillaBurnResult
rejilla_dvd_author_set_argv (RejillaProcess *process,
			     GPtrArray *argv,
			     GError **error)
{
	RejillaDvdAuthorPrivate *priv;
	RejillaBurnResult result;
	RejillaJobAction action;
	gchar *output;

	priv = REJILLA_DVD_AUTHOR_PRIVATE (process);

	rejilla_job_get_action (REJILLA_JOB (process), &action);
	if (action != REJILLA_JOB_ACTION_IMAGE)
		REJILLA_JOB_NOT_SUPPORTED (process);

	g_ptr_array_add (argv, g_strdup ("dvdauthor"));
	
	/* get all arguments to write XML file */
	result = rejilla_job_get_tmp_file (REJILLA_JOB (process),
					   NULL,
					   &output,
					   error);
	if (result != REJILLA_BURN_OK)
		return result;

	g_ptr_array_add (argv, g_strdup ("-x"));
	g_ptr_array_add (argv, output);

	result = rejilla_dvd_author_generate_xml_file (process, output, error);
	if (result != REJILLA_BURN_OK)
		return result;

	rejilla_job_set_current_action (REJILLA_JOB (process),
					REJILLA_BURN_ACTION_CREATING_IMAGE,
					_("Creating file layout"),
					FALSE);
	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_dvd_author_post (RejillaJob *job)
{
	RejillaDvdAuthorPrivate *priv;

	priv = REJILLA_DVD_AUTHOR_PRIVATE (job);

	rejilla_dvd_author_add_track (job);

	if (priv->output) {
		g_free (priv->output);
		priv->output = NULL;
	}

	return rejilla_job_finished_session (job);
}

static void
rejilla_dvd_author_init (RejillaDvdAuthor *object)
{}

static void
rejilla_dvd_author_finalize (GObject *object)
{
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
rejilla_dvd_author_class_init (RejillaDvdAuthorClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	RejillaProcessClass* process_class = REJILLA_PROCESS_CLASS (klass);

	g_type_class_add_private (klass, sizeof (RejillaDvdAuthorPrivate));

	object_class->finalize = rejilla_dvd_author_finalize;

	process_class->stdout_func = rejilla_dvd_author_read_stdout;
	process_class->stderr_func = rejilla_dvd_author_read_stderr;
	process_class->set_argv = rejilla_dvd_author_set_argv;
	process_class->post = rejilla_dvd_author_post;
}

static void
rejilla_dvd_author_export_caps (RejillaPlugin *plugin)
{
	GSList *output;
	GSList *input;

	/* NOTE: it seems that cdrecord can burn cue files on the fly */
	rejilla_plugin_define (plugin,
			       "dvdauthor",
	                       NULL,
			       _("Creates disc images suitable for video DVDs"),
			       "Philippe Rouquier",
			       1);

	input = rejilla_caps_audio_new (REJILLA_PLUGIN_IO_ACCEPT_FILE,
					REJILLA_AUDIO_FORMAT_AC3|
					REJILLA_AUDIO_FORMAT_MP2|
					REJILLA_AUDIO_FORMAT_RAW|
					REJILLA_METADATA_INFO|
					REJILLA_VIDEO_FORMAT_VIDEO_DVD);

	output = rejilla_caps_data_new (REJILLA_IMAGE_FS_ISO|
					REJILLA_IMAGE_FS_UDF|
					REJILLA_IMAGE_FS_VIDEO);

	rejilla_plugin_link_caps (plugin, output, input);
	g_slist_free (input);

	input = rejilla_caps_audio_new (REJILLA_PLUGIN_IO_ACCEPT_FILE,
					REJILLA_AUDIO_FORMAT_AC3|
					REJILLA_AUDIO_FORMAT_MP2|
					REJILLA_AUDIO_FORMAT_RAW|
					REJILLA_VIDEO_FORMAT_VIDEO_DVD);

	rejilla_plugin_link_caps (plugin, output, input);
	g_slist_free (output);
	g_slist_free (input);

	/* we only support DVDs */
	rejilla_plugin_set_flags (plugin,
  				  REJILLA_MEDIUM_FILE|
				  REJILLA_MEDIUM_DVDR|
				  REJILLA_MEDIUM_DVDR_PLUS|
				  REJILLA_MEDIUM_DUAL_L|
				  REJILLA_MEDIUM_BLANK|
				  REJILLA_MEDIUM_APPENDABLE|
				  REJILLA_MEDIUM_HAS_DATA,
				  REJILLA_BURN_FLAG_NONE,
				  REJILLA_BURN_FLAG_NONE);

	rejilla_plugin_set_flags (plugin,
				  REJILLA_MEDIUM_DVDRW|
				  REJILLA_MEDIUM_DVDRW_PLUS|
				  REJILLA_MEDIUM_DVDRW_RESTRICTED|
				  REJILLA_MEDIUM_DUAL_L|
				  REJILLA_MEDIUM_BLANK|
				  REJILLA_MEDIUM_CLOSED|
				  REJILLA_MEDIUM_APPENDABLE|
				  REJILLA_MEDIUM_HAS_DATA,
				  REJILLA_BURN_FLAG_NONE,
				  REJILLA_BURN_FLAG_NONE);
}

G_MODULE_EXPORT void
rejilla_plugin_check_config (RejillaPlugin *plugin)
{
	gint version [3] = { 0, 6, 0};
	rejilla_plugin_test_app (plugin,
	                         "dvdauthor",
	                         "-h",
	                         "DVDAuthor::dvdauthor, version %d.%d.%d.",
	                         version);
}
