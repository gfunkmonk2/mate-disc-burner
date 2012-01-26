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

#include "rejilla-tags.h"
#include "rejilla-plugin-registration.h"
#include "burn-job.h"
#include "burn-process.h"
#include "rejilla-track-stream.h"


#define REJILLA_TYPE_VCD_IMAGER             (rejilla_vcd_imager_get_type ())
#define REJILLA_VCD_IMAGER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_VCD_IMAGER, RejillaVcdImager))
#define REJILLA_VCD_IMAGER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_VCD_IMAGER, RejillaVcdImagerClass))
#define REJILLA_IS_VCD_IMAGER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_VCD_IMAGER))
#define REJILLA_IS_VCD_IMAGER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_VCD_IMAGER))
#define REJILLA_VCD_IMAGER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_VCD_IMAGER, RejillaVcdImagerClass))

REJILLA_PLUGIN_BOILERPLATE (RejillaVcdImager, rejilla_vcd_imager, REJILLA_TYPE_PROCESS, RejillaProcess);

typedef struct _RejillaVcdImagerPrivate RejillaVcdImagerPrivate;
struct _RejillaVcdImagerPrivate
{
	guint num_tracks;

	guint svcd:1;
};

#define REJILLA_VCD_IMAGER_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REJILLA_TYPE_VCD_IMAGER, RejillaVcdImagerPrivate))

static RejillaProcessClass *parent_class = NULL;

static RejillaBurnResult
rejilla_vcd_imager_read_stdout (RejillaProcess *process,
				const gchar *line)
{
	gint percent = 0;
	guint track_num = 0;
	RejillaVcdImagerPrivate *priv;

	priv = REJILLA_VCD_IMAGER_PRIVATE (process);

	if (sscanf (line, "#scan[track-%d]: %*d/%*d (%d)", &track_num, &percent) == 2) {
		rejilla_job_start_progress (REJILLA_JOB (process), FALSE);
		rejilla_job_set_progress (REJILLA_JOB (process),
					  (gdouble) ((gdouble) percent) /
					  100.0 /
					  (gdouble) (priv->num_tracks + 1) +
					  (gdouble) (track_num) /
					  (gdouble) (priv->num_tracks + 1));
	}
	else if (sscanf (line, "#write[%*d/%*d]: %*d/%*d (%d)", &percent) == 1) {
		gdouble progress;

		/* NOTE: percent can be over 100% ???? */
		rejilla_job_start_progress (REJILLA_JOB (process), FALSE);
		progress = (gdouble) ((gdouble) percent) /
			   100.0 /
			   (gdouble) (priv->num_tracks + 1) +
			   (gdouble) (priv->num_tracks) /
			   (gdouble) (priv->num_tracks + 1);

		if (progress > 1.0)
			progress = 1.0;

		rejilla_job_set_progress (REJILLA_JOB (process), progress);
	}

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_vcd_imager_read_stderr (RejillaProcess *process,
				const gchar *line)
{
	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_vcd_imager_generate_xml_file (RejillaProcess *process,
				      const gchar *path,
				      GError **error)
{
	RejillaVcdImagerPrivate *priv;
	GSList *tracks = NULL;
	xmlTextWriter *xml;
	gchar buffer [64];
	gint success;
	GSList *iter;
	gchar *name;
	gint i;

	REJILLA_JOB_LOG (process, "Creating (S)VCD layout xml file (%s)", path);

	xml = xmlNewTextWriterFilename (path, 0);
	if (!xml)
		return REJILLA_BURN_ERR;

	priv = REJILLA_VCD_IMAGER_PRIVATE (process);

	xmlTextWriterSetIndent (xml, 1);
	xmlTextWriterSetIndentString (xml, (xmlChar *) "\t");

	success = xmlTextWriterStartDocument (xml,
					      NULL,
					      "UTF8",
					      NULL);
	if (success < 0)
		goto error;

	success = xmlTextWriterWriteDTD (xml,
					(xmlChar *) "videocd",
					(xmlChar *) "-//GNU//DTD VideoCD//EN",
					(xmlChar *) "http://www.gnu.org/software/vcdimager/videocd.dtd",
					(xmlChar *) NULL);
	if (success < 0)
		goto error;

	/* let's start */
	success = xmlTextWriterStartElement (xml, (xmlChar *) "videocd");
	if (success < 0)
		goto error;

	success = xmlTextWriterWriteAttribute (xml,
					       (xmlChar *) "xmlns",
					       (xmlChar *) "http://www.gnu.org/software/vcdimager/1.0/");
	if (success < 0)
		goto error;

	if (priv->svcd)
		success = xmlTextWriterWriteAttribute (xml,
						       (xmlChar *) "class",
						       (xmlChar *) "svcd");
	else
		success = xmlTextWriterWriteAttribute (xml,
						       (xmlChar *) "class",
						       (xmlChar *) "vcd");

	if (success < 0)
		goto error;

	if (priv->svcd)
		success = xmlTextWriterWriteAttribute (xml,
						       (xmlChar *) "version",
						       (xmlChar *) "1.0");
	else
		success = xmlTextWriterWriteAttribute (xml,
						       (xmlChar *) "version",
						       (xmlChar *) "2.0");
	if (success < 0)
		goto error;

	/* info part */
	success = xmlTextWriterStartElement (xml, (xmlChar *) "info");
	if (success < 0)
		goto error;

	/* name of the volume */
	name = NULL;
	rejilla_job_get_audio_title (REJILLA_JOB (process), &name);
	success = xmlTextWriterWriteElement (xml,
					     (xmlChar *) "album-id",
					     (xmlChar *) name);
	g_free (name);
	if (success < 0)
		goto error;

	/* number of CDs */
	success = xmlTextWriterWriteElement (xml,
					     (xmlChar *) "volume-count",
					     (xmlChar *) "1");
	if (success < 0)
		goto error;

	/* CD number */
	success = xmlTextWriterWriteElement (xml,
					     (xmlChar *) "volume-number",
					     (xmlChar *) "1");
	if (success < 0)
		goto error;

	/* close info part */
	success = xmlTextWriterEndElement (xml);
	if (success < 0)
		goto error;

	/* Primary Volume descriptor */
	success = xmlTextWriterStartElement (xml, (xmlChar *) "pvd");
	if (success < 0)
		goto error;

	/* NOTE: no need to convert a possible non compliant name as this will 
	 * be done by vcdimager. */
	name = NULL;
	rejilla_job_get_audio_title (REJILLA_JOB (process), &name);
	success = xmlTextWriterWriteElement (xml,
					     (xmlChar *) "volume-id",
					     (xmlChar *) name);
	g_free (name);
	if (success < 0)
		goto error;

	/* Makes it CD-i compatible */
	success = xmlTextWriterWriteElement (xml,
					     (xmlChar *) "system-id",
					     (xmlChar *) "CD-RTOS CD-BRIDGE");
	if (success < 0)
		goto error;

	/* Close Primary Volume descriptor */
	success = xmlTextWriterEndElement (xml);
	if (success < 0)
		goto error;

	/* the tracks */
	success = xmlTextWriterStartElement (xml, (xmlChar *) "sequence-items");
	if (success < 0)
		goto error;

	/* get all tracks */
	rejilla_job_get_tracks (REJILLA_JOB (process), &tracks);
	priv->num_tracks = g_slist_length (tracks);
	for (i = 0, iter = tracks; iter; iter = iter->next, i++) {
		RejillaTrack *track;
		gchar *video;

		track = iter->data;
		success = xmlTextWriterStartElement (xml, (xmlChar *) "sequence-item");
		if (success < 0)
			goto error;

		video = rejilla_track_stream_get_source (REJILLA_TRACK_STREAM (track), FALSE);
		success = xmlTextWriterWriteAttribute (xml,
						       (xmlChar *) "src",
						       (xmlChar *) video);
		g_free (video);

		if (success < 0)
			goto error;

		sprintf (buffer, "track-%i", i);
		success = xmlTextWriterWriteAttribute (xml,
						       (xmlChar *) "id",
						       (xmlChar *) buffer);
		if (success < 0)
			goto error;

		/* close sequence-item */
		success = xmlTextWriterEndElement (xml);
		if (success < 0)
			goto error;
	}

	/* sequence-items */
	success = xmlTextWriterEndElement (xml);
	if (success < 0)
		goto error;

	/* the navigation */
	success = xmlTextWriterStartElement (xml, (xmlChar *) "pbc");
	if (success < 0)
		goto error;

	/* get all tracks */
	rejilla_job_get_tracks (REJILLA_JOB (process), &tracks);
	for (i = 0, iter = tracks; iter; iter = iter->next, i++) {
		RejillaTrack *track;

		track = iter->data;

		sprintf (buffer, "playlist-%i", i);
		success = xmlTextWriterStartElement (xml, (xmlChar *) "playlist");
		if (success < 0)
			goto error;

		success = xmlTextWriterWriteAttribute (xml,
						       (xmlChar *) "id",
						       (xmlChar *) buffer);
		if (success < 0)
			goto error;

		success = xmlTextWriterWriteElement (xml,
						     (xmlChar *) "wait",
						     (xmlChar *) "0");
		if (success < 0)
			goto error;

		success = xmlTextWriterStartElement (xml, (xmlChar *) "play-item");
		if (success < 0)
			goto error;

		sprintf (buffer, "track-%i", i);
		success = xmlTextWriterWriteAttribute (xml,
						       (xmlChar *) "ref",
						       (xmlChar *) buffer);
		if (success < 0)
			goto error;

		/* play-item */
		success = xmlTextWriterEndElement (xml);
		if (success < 0)
			goto error;

		/* playlist */
		success = xmlTextWriterEndElement (xml);
		if (success < 0)
			goto error;
	}

	/* pbc */
	success = xmlTextWriterEndElement (xml);
	if (success < 0)
		goto error;

	/* close videocd */
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
rejilla_vcd_imager_set_argv (RejillaProcess *process,
			     GPtrArray *argv,
			     GError **error)
{
	RejillaVcdImagerPrivate *priv;
	RejillaBurnResult result;
	RejillaJobAction action;
	RejillaMedia medium;
	gchar *output;
	gchar *image;
	gchar *toc;

	priv = REJILLA_VCD_IMAGER_PRIVATE (process);

	rejilla_job_get_action (REJILLA_JOB (process), &action);
	if (action != REJILLA_JOB_ACTION_IMAGE)
		REJILLA_JOB_NOT_SUPPORTED (process);

	g_ptr_array_add (argv, g_strdup ("vcdxbuild"));

	g_ptr_array_add (argv, g_strdup ("--progress"));
	g_ptr_array_add (argv, g_strdup ("-v"));

	/* specifies output */
	image = toc = NULL;
	rejilla_job_get_image_output (REJILLA_JOB (process),
				      &image,
				      &toc);

	g_ptr_array_add (argv, g_strdup ("-c"));
	g_ptr_array_add (argv, toc);
	g_ptr_array_add (argv, g_strdup ("-b"));
	g_ptr_array_add (argv, image);

	/* get temporary file to write XML */
	result = rejilla_job_get_tmp_file (REJILLA_JOB (process),
					   NULL,
					   &output,
					   error);
	if (result != REJILLA_BURN_OK)
		return result;

	g_ptr_array_add (argv, output);

	rejilla_job_get_media (REJILLA_JOB (process), &medium);
	if (medium & REJILLA_MEDIUM_CD) {
		GValue *value = NULL;

		rejilla_job_tag_lookup (REJILLA_JOB (process),
					REJILLA_VCD_TYPE,
					&value);
		if (value)
			priv->svcd = (g_value_get_int (value) == REJILLA_SVCD);
	}

	result = rejilla_vcd_imager_generate_xml_file (process, output, error);
	if (result != REJILLA_BURN_OK)
		return result;
	
	rejilla_job_set_current_action (REJILLA_JOB (process),
					REJILLA_BURN_ACTION_CREATING_IMAGE,
					_("Creating file layout"),
					FALSE);
	return REJILLA_BURN_OK;
}

static void
rejilla_vcd_imager_init (RejillaVcdImager *object)
{}

static void
rejilla_vcd_imager_finalize (GObject *object)
{
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
rejilla_vcd_imager_class_init (RejillaVcdImagerClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	RejillaProcessClass* process_class = REJILLA_PROCESS_CLASS (klass);

	g_type_class_add_private (klass, sizeof (RejillaVcdImagerPrivate));

	object_class->finalize = rejilla_vcd_imager_finalize;
	process_class->stdout_func = rejilla_vcd_imager_read_stdout;
	process_class->stderr_func = rejilla_vcd_imager_read_stderr;
	process_class->set_argv = rejilla_vcd_imager_set_argv;
	process_class->post = rejilla_job_finished_session;
}

static void
rejilla_vcd_imager_export_caps (RejillaPlugin *plugin)
{
	GSList *output;
	GSList *input;

	/* NOTE: it seems that cdrecord can burn cue files on the fly */
	rejilla_plugin_define (plugin,
			       "vcdimager",
	                       NULL,
			       _("Creates disc images suitable for SVCDs"),
			       "Philippe Rouquier",
			       1);

	input = rejilla_caps_audio_new (REJILLA_PLUGIN_IO_ACCEPT_FILE,
					REJILLA_AUDIO_FORMAT_MP2|
					REJILLA_AUDIO_FORMAT_44100|
					REJILLA_VIDEO_FORMAT_VCD|
					REJILLA_METADATA_INFO);

	output = rejilla_caps_image_new (REJILLA_PLUGIN_IO_ACCEPT_FILE,
					 REJILLA_IMAGE_FORMAT_CUE);

	rejilla_plugin_link_caps (plugin, output, input);
	g_slist_free (input);

	input = rejilla_caps_audio_new (REJILLA_PLUGIN_IO_ACCEPT_FILE,
					REJILLA_AUDIO_FORMAT_MP2|
					REJILLA_AUDIO_FORMAT_44100|
					REJILLA_VIDEO_FORMAT_VCD);

	rejilla_plugin_link_caps (plugin, output, input);
	g_slist_free (output);
	g_slist_free (input);

	/* we only support CDs they must be blank */
	rejilla_plugin_set_flags (plugin,
				  REJILLA_MEDIUM_CDRW|
				  REJILLA_MEDIUM_BLANK|
				  REJILLA_MEDIUM_CLOSED|
				  REJILLA_MEDIUM_APPENDABLE|
				  REJILLA_MEDIUM_HAS_DATA|
				  REJILLA_MEDIUM_HAS_AUDIO,
				  REJILLA_BURN_FLAG_NONE,
				  REJILLA_BURN_FLAG_NONE);

	rejilla_plugin_set_flags (plugin,
				  REJILLA_MEDIUM_FILE|
				  REJILLA_MEDIUM_CDR|
				  REJILLA_MEDIUM_BLANK|
				  REJILLA_MEDIUM_APPENDABLE|
				  REJILLA_MEDIUM_HAS_DATA|
				  REJILLA_MEDIUM_HAS_AUDIO,
				  REJILLA_BURN_FLAG_NONE,
				  REJILLA_BURN_FLAG_NONE);
}

G_MODULE_EXPORT void
rejilla_plugin_check_config (RejillaPlugin *plugin)
{
	gint version [3] = { 0, 7, 0};
	rejilla_plugin_test_app (plugin,
	                         "vcdimager",
	                         "--version",
	                         "vcdimager (GNU VCDImager) %d.%d.%d",
	                         version);
}
