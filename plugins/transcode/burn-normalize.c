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
#include <glib/gi18n-lib.h>
#include <gmodule.h>

#include <gst/gst.h>

#include "rejilla-tags.h"

#include "burn-job.h"
#include "burn-normalize.h"
#include "rejilla-plugin-registration.h"


#define REJILLA_TYPE_NORMALIZE             (rejilla_normalize_get_type ())
#define REJILLA_NORMALIZE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_NORMALIZE, RejillaNormalize))
#define REJILLA_NORMALIZE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_NORMALIZE, RejillaNormalizeClass))
#define REJILLA_IS_NORMALIZE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_NORMALIZE))
#define REJILLA_IS_NORMALIZE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_NORMALIZE))
#define REJILLA_NORMALIZE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_NORMALIZE, RejillaNormalizeClass))

REJILLA_PLUGIN_BOILERPLATE (RejillaNormalize, rejilla_normalize, REJILLA_TYPE_JOB, RejillaJob);

typedef struct _RejillaNormalizePrivate RejillaNormalizePrivate;
struct _RejillaNormalizePrivate
{
	GstElement *pipeline;
	GstElement *analysis;
	GstElement *decode;
	GstElement *resample;

	GSList *tracks;
	RejillaTrack *track;

	gdouble album_peak;
	gdouble album_gain;
	gdouble track_peak;
	gdouble track_gain;
};

#define REJILLA_NORMALIZE_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REJILLA_TYPE_NORMALIZE, RejillaNormalizePrivate))

static GObjectClass *parent_class = NULL;


static gboolean
rejilla_normalize_bus_messages (GstBus *bus,
				GstMessage *msg,
				RejillaNormalize *normalize);

static void
rejilla_normalize_stop_pipeline (RejillaNormalize *normalize)
{
	RejillaNormalizePrivate *priv;

	priv = REJILLA_NORMALIZE_PRIVATE (normalize);
	if (!priv->pipeline)
		return;

	gst_element_set_state (priv->pipeline, GST_STATE_NULL);
	gst_object_unref (GST_OBJECT (priv->pipeline));
	priv->pipeline = NULL;
	priv->resample = NULL;
	priv->analysis = NULL;
	priv->decode = NULL;
}

static void
rejilla_normalize_new_decoded_pad_cb (GstElement *decode,
				      GstPad *pad,
				      gboolean arg2,
				      RejillaNormalize *normalize)
{
	GstPad *sink;
	GstCaps *caps;
	GstStructure *structure;
	RejillaNormalizePrivate *priv;

	priv = REJILLA_NORMALIZE_PRIVATE (normalize);

	sink = gst_element_get_pad (priv->resample, "sink");
	if (GST_PAD_IS_LINKED (sink)) {
		REJILLA_JOB_LOG (normalize, "New decoded pad already linked");
		return;
	}

	/* make sure we only have audio */
	caps = gst_pad_get_caps (pad);
	if (!caps)
		return;

	structure = gst_caps_get_structure (caps, 0);
	if (structure && g_strrstr (gst_structure_get_name (structure), "audio")) {
		if (gst_pad_link (pad, sink) != GST_PAD_LINK_OK) {
			REJILLA_JOB_LOG (normalize, "New decoded pad can't be linked");
			rejilla_job_error (REJILLA_JOB (normalize), NULL);
		}
		else
			REJILLA_JOB_LOG (normalize, "New decoded pad linked");
	}
	else
		REJILLA_JOB_LOG (normalize, "New decoded pad with unsupported stream time");

	gst_object_unref (sink);
	gst_caps_unref (caps);
}

  static gboolean
rejilla_normalize_build_pipeline (RejillaNormalize *normalize,
                                  const gchar *uri,
                                  GstElement *analysis,
                                  GError **error)
{
	GstBus *bus = NULL;
	GstElement *source;
	GstElement *decode;
	GstElement *pipeline;
	GstElement *sink = NULL;
	GstElement *convert = NULL;
	GstElement *resample = NULL;
	RejillaNormalizePrivate *priv;

	priv = REJILLA_NORMALIZE_PRIVATE (normalize);

	REJILLA_JOB_LOG (normalize, "Creating new pipeline");

	/* create filesrc ! decodebin ! audioresample ! audioconvert ! rganalysis ! fakesink */
	pipeline = gst_pipeline_new (NULL);
	priv->pipeline = pipeline;

	/* a new source is created */
	source = gst_element_make_from_uri (GST_URI_SRC, uri, NULL);
	if (source == NULL) {
		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_GENERAL,
			     _("%s element could not be created"),
			     "\"Source\"");
		goto error;
	}
	gst_bin_add (GST_BIN (priv->pipeline), source);
	g_object_set (source,
		      "typefind", FALSE,
		      NULL);

	/* decode */
	decode = gst_element_factory_make ("decodebin", NULL);
	if (decode == NULL) {
		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_GENERAL,
			     _("%s element could not be created"),
			     "\"Decodebin\"");
		goto error;
	}
	gst_bin_add (GST_BIN (pipeline), decode);
	priv->decode = decode;

	if (!gst_element_link (source, decode)) {
		REJILLA_JOB_LOG (normalize, "Elements could not be linked");
		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_GENERAL,
		             _("Impossible to link plugin pads"));
		goto error;
	}

	/* audioconvert */
	convert = gst_element_factory_make ("audioconvert", NULL);
	if (convert == NULL) {
		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_GENERAL,
			     _("%s element could not be created"),
			     "\"Audioconvert\"");
		goto error;
	}
	gst_bin_add (GST_BIN (pipeline), convert);

	/* audioresample */
	resample = gst_element_factory_make ("audioresample", NULL);
	if (resample == NULL) {
		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_GENERAL,
			     _("%s element could not be created"),
			     "\"Audioresample\"");
		goto error;
	}
	gst_bin_add (GST_BIN (pipeline), resample);
	priv->resample = resample;

	/* rganalysis: set the number of tracks to be expected */
	priv->analysis = analysis;
	gst_bin_add (GST_BIN (pipeline), analysis);

	/* sink */
	sink = gst_element_factory_make ("fakesink", NULL);
	if (!sink) {
		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_GENERAL,
			     _("%s element could not be created"),
			     "\"Fakesink\"");
		goto error;
	}
	gst_bin_add (GST_BIN (pipeline), sink);
	g_object_set (sink,
		      "sync", FALSE,
		      NULL);

	/* link everything */
	g_signal_connect (G_OBJECT (decode),
	                  "new-decoded-pad",
	                  G_CALLBACK (rejilla_normalize_new_decoded_pad_cb),
	                  normalize);
	if (!gst_element_link_many (resample,
	                            convert,
	                            analysis,
	                            sink,
	                            NULL)) {
		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_GENERAL,
		             _("Impossible to link plugin pads"));
	}

	/* connect to the bus */	
	bus = gst_pipeline_get_bus (GST_PIPELINE (priv->pipeline));
	gst_bus_add_watch (bus,
			   (GstBusFunc) rejilla_normalize_bus_messages,
			   normalize);
	gst_object_unref (bus);

	gst_element_set_state (priv->pipeline, GST_STATE_PLAYING);

	return TRUE;

error:

	if (error && (*error))
		REJILLA_JOB_LOG (normalize,
				 "can't create object : %s \n",
				 (*error)->message);

	gst_object_unref (GST_OBJECT (pipeline));
	return FALSE;
}

static RejillaBurnResult
rejilla_normalize_set_next_track (RejillaJob *job,
                                  GError **error)
{
	gchar *uri;
	GValue *value;
	GstElement *analysis;
	RejillaTrackType *type;
	RejillaTrack *track = NULL;
	gboolean dts_allowed = FALSE;
	RejillaNormalizePrivate *priv;

	priv = REJILLA_NORMALIZE_PRIVATE (job);

	/* See if dts is allowed */
	value = NULL;
	rejilla_job_tag_lookup (job, REJILLA_SESSION_STREAM_AUDIO_FORMAT, &value);
	if (value)
		dts_allowed = (g_value_get_int (value) & REJILLA_AUDIO_FORMAT_DTS) != 0;

	type = rejilla_track_type_new ();
	while (priv->tracks && priv->tracks->data) {
		track = priv->tracks->data;
		priv->tracks = g_slist_remove (priv->tracks, track);

		rejilla_track_get_track_type (track, type);
		if (rejilla_track_type_get_has_stream (type)) {
			if (!dts_allowed)
				break;

			/* skip DTS tracks as we won't modify them */
			if ((rejilla_track_type_get_stream_format (type) & REJILLA_AUDIO_FORMAT_DTS) == 0) 
				break;

			REJILLA_JOB_LOG (job, "Skipped DTS track");
		}

		track = NULL;
	}
	rejilla_track_type_free (type);

	if (!track)
		return REJILLA_BURN_OK;

	if (!priv->analysis) {
		analysis = gst_element_factory_make ("rganalysis", NULL);
		if (analysis == NULL) {
			g_set_error (error,
				     REJILLA_BURN_ERROR,
				     REJILLA_BURN_ERROR_GENERAL,
				     _("%s element could not be created"),
				     "\"Rganalysis\"");
			return REJILLA_BURN_ERR;
		}

		g_object_set (analysis,
			      "num-tracks", g_slist_length (priv->tracks),
			      NULL);
	}
	else {
		/* destroy previous pipeline but save our plugin */
		analysis = g_object_ref (priv->analysis);

		/* NOTE: why lock state? because otherwise analysis would lose all 
		 * information about tracks already analysed by going into the NULL
		 * state. */
		gst_element_set_locked_state (analysis, TRUE);
		gst_bin_remove (GST_BIN (priv->pipeline), analysis);
		rejilla_normalize_stop_pipeline (REJILLA_NORMALIZE (job));
		gst_element_set_locked_state (analysis, FALSE);
	}

	/* create a new one */
	priv->track = track;
	uri = rejilla_track_stream_get_source (REJILLA_TRACK_STREAM (track), TRUE);
	REJILLA_JOB_LOG (job, "Analysing track %s", uri);

	if (!rejilla_normalize_build_pipeline (REJILLA_NORMALIZE (job), uri, analysis, error)) {
		g_free (uri);
		return REJILLA_BURN_ERR;
	}

	g_free (uri);
	return REJILLA_BURN_RETRY;
}

static RejillaBurnResult
rejilla_normalize_stop (RejillaJob *job,
			GError **error)
{
	RejillaNormalizePrivate *priv;

	priv = REJILLA_NORMALIZE_PRIVATE (job);

	rejilla_normalize_stop_pipeline (REJILLA_NORMALIZE (job));
	if (priv->tracks) {
		g_slist_free (priv->tracks);
		priv->tracks = NULL;
	}

	priv->track = NULL;

	return REJILLA_BURN_OK;
}

static void
foreach_tag (const GstTagList *list,
	     const gchar *tag,
	     RejillaNormalize *normalize)
{
	gdouble value = 0.0;
	RejillaNormalizePrivate *priv;

	priv = REJILLA_NORMALIZE_PRIVATE (normalize);

	/* Those next two are generated at the end only */
	if (!strcmp (tag, GST_TAG_ALBUM_GAIN)) {
		gst_tag_list_get_double (list, tag, &value);
		priv->album_gain = value;
	}
	else if (!strcmp (tag, GST_TAG_ALBUM_PEAK)) {
		gst_tag_list_get_double (list, tag, &value);
		priv->album_peak = value;
	}
	else if (!strcmp (tag, GST_TAG_TRACK_PEAK)) {
		gst_tag_list_get_double (list, tag, &value);
		priv->track_peak = value;
	}
	else if (!strcmp (tag, GST_TAG_TRACK_GAIN)) {
		gst_tag_list_get_double (list, tag, &value);
		priv->track_gain = value;
	}
}

static void
rejilla_normalize_song_end_reached (RejillaNormalize *normalize)
{
	GValue *value;
	GError *error = NULL;
	RejillaBurnResult result;
	RejillaNormalizePrivate *priv;

	priv = REJILLA_NORMALIZE_PRIVATE (normalize);
	
	/* finished track: set tags */
	REJILLA_JOB_LOG (normalize,
			 "Setting track peak (%lf) and gain (%lf)",
			 priv->track_peak,
			 priv->track_gain);

	value = g_new0 (GValue, 1);
	g_value_init (value, G_TYPE_DOUBLE);
	g_value_set_double (value, priv->track_peak);
	rejilla_track_tag_add (priv->track,
			       REJILLA_TRACK_PEAK_VALUE,
			       value);

	value = g_new0 (GValue, 1);
	g_value_init (value, G_TYPE_DOUBLE);
	g_value_set_double (value, priv->track_gain);
	rejilla_track_tag_add (priv->track,
			       REJILLA_TRACK_GAIN_VALUE,
			       value);

	priv->track_peak = 0.0;
	priv->track_gain = 0.0;

	result = rejilla_normalize_set_next_track (REJILLA_JOB (normalize), &error);
	if (result == REJILLA_BURN_OK) {
		REJILLA_JOB_LOG (normalize,
				 "Setting album peak (%lf) and gain (%lf)",
				 priv->album_peak,
				 priv->album_gain);

		/* finished: set tags */
		value = g_new0 (GValue, 1);
		g_value_init (value, G_TYPE_DOUBLE);
		g_value_set_double (value, priv->album_peak);
		rejilla_job_tag_add (REJILLA_JOB (normalize),
				     REJILLA_ALBUM_PEAK_VALUE,
				     value);

		value = g_new0 (GValue, 1);
		g_value_init (value, G_TYPE_DOUBLE);
		g_value_set_double (value, priv->album_gain);
		rejilla_job_tag_add (REJILLA_JOB (normalize),
				     REJILLA_ALBUM_GAIN_VALUE,
				     value);

		rejilla_job_finished_session (REJILLA_JOB (normalize));
		return;
	}

	/* jump to next track */
	if (result == REJILLA_BURN_ERR) {
		rejilla_job_error (REJILLA_JOB (normalize), error);
		return;
	}
}

static gboolean
rejilla_normalize_bus_messages (GstBus *bus,
				GstMessage *msg,
				RejillaNormalize *normalize)
{
	RejillaNormalizePrivate *priv;
	GstTagList *tags = NULL;
	GError *error = NULL;
	gchar *debug;

	priv = REJILLA_NORMALIZE_PRIVATE (normalize);
	switch (GST_MESSAGE_TYPE (msg)) {
	case GST_MESSAGE_TAG:
		/* This is the information we've been waiting for.
		 * NOTE: levels for whole album is delivered at the end */
		gst_message_parse_tag (msg, &tags);
		gst_tag_list_foreach (tags, (GstTagForeachFunc) foreach_tag, normalize);
		gst_tag_list_free (tags);
		return TRUE;

	case GST_MESSAGE_ERROR:
		gst_message_parse_error (msg, &error, &debug);
		REJILLA_JOB_LOG (normalize, debug);
		g_free (debug);

	        rejilla_job_error (REJILLA_JOB (normalize), error);
		return FALSE;

	case GST_MESSAGE_EOS:
		rejilla_normalize_song_end_reached (normalize);
		return FALSE;

	case GST_MESSAGE_STATE_CHANGED:
		break;

	default:
		return TRUE;
	}

	return TRUE;
}

static RejillaBurnResult
rejilla_normalize_start (RejillaJob *job,
			 GError **error)
{
	RejillaNormalizePrivate *priv;
	RejillaBurnResult result;

	priv = REJILLA_NORMALIZE_PRIVATE (job);

	priv->album_gain = -1.0;
	priv->album_peak = -1.0;

	/* get tracks */
	rejilla_job_get_tracks (job, &priv->tracks);
	if (!priv->tracks)
		return REJILLA_BURN_ERR;

	priv->tracks = g_slist_copy (priv->tracks);

	result = rejilla_normalize_set_next_track (job, error);
	if (result == REJILLA_BURN_ERR)
		return REJILLA_BURN_ERR;

	if (result == REJILLA_BURN_OK)
		return REJILLA_BURN_NOT_RUNNING;

	/* ready to go */
	rejilla_job_set_current_action (job,
					REJILLA_BURN_ACTION_ANALYSING,
					_("Normalizing tracks"),
					FALSE);

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_normalize_activate (RejillaJob *job,
			    GError **error)
{
	GSList *tracks;
	RejillaJobAction action;

	rejilla_job_get_action (job, &action);
	if (action != REJILLA_JOB_ACTION_IMAGE)
		return REJILLA_BURN_NOT_RUNNING;

	/* check we have more than one track */
	rejilla_job_get_tracks (job, &tracks);
	if (g_slist_length (tracks) < 2)
		return REJILLA_BURN_NOT_RUNNING;

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_normalize_clock_tick (RejillaJob *job)
{
	gint64 position = 0.0;
	gint64 duration = 0.0;
	RejillaNormalizePrivate *priv;
	GstFormat format = GST_FORMAT_TIME;

	priv = REJILLA_NORMALIZE_PRIVATE (job);

	gst_element_query_duration (priv->pipeline, &format, &duration);
	gst_element_query_position (priv->pipeline, &format, &position);

	if (duration > 0) {
		GSList *tracks;
		gdouble progress;

		rejilla_job_get_tracks (job, &tracks);
		progress = (gdouble) position / (gdouble) duration;

		if (tracks) {
			gdouble num_tracks;

			num_tracks = g_slist_length (tracks);
			progress = (gdouble) (num_tracks - 1.0 - (gdouble) g_slist_length (priv->tracks) + progress) / (gdouble) num_tracks;
			rejilla_job_set_progress (job, progress);
		}
	}

	return REJILLA_BURN_OK;
}

static void
rejilla_normalize_init (RejillaNormalize *object)
{}

static void
rejilla_normalize_finalize (GObject *object)
{
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
rejilla_normalize_class_init (RejillaNormalizeClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	RejillaJobClass *job_class = REJILLA_JOB_CLASS (klass);

	g_type_class_add_private (klass, sizeof (RejillaNormalizePrivate));

	object_class->finalize = rejilla_normalize_finalize;

	job_class->activate = rejilla_normalize_activate;
	job_class->start = rejilla_normalize_start;
	job_class->clock_tick = rejilla_normalize_clock_tick;
	job_class->stop = rejilla_normalize_stop;
}

static void
rejilla_normalize_export_caps (RejillaPlugin *plugin)
{
	GSList *input;

	rejilla_plugin_define (plugin,
	                       "normalize",
			       N_("Normalization"),
			       _("Sets consistent sound levels between tracks"),
			       "Philippe Rouquier",
			       0);

	/* Add dts to make sure that when they are mixed with regular songs
	 * this plugin will be called for the regular tracks */
	input = rejilla_caps_audio_new (REJILLA_PLUGIN_IO_ACCEPT_FILE,
					REJILLA_AUDIO_FORMAT_UNDEFINED|
	                                REJILLA_AUDIO_FORMAT_DTS|
					REJILLA_METADATA_INFO);
	rejilla_plugin_process_caps (plugin, input);
	g_slist_free (input);

	/* Add dts to make sure that when they are mixed with regular songs
	 * this plugin will be called for the regular tracks */
	input = rejilla_caps_audio_new (REJILLA_PLUGIN_IO_ACCEPT_FILE,
					REJILLA_AUDIO_FORMAT_UNDEFINED|
	                                REJILLA_AUDIO_FORMAT_DTS);
	rejilla_plugin_process_caps (plugin, input);
	g_slist_free (input);

	/* We should run first */
	rejilla_plugin_set_process_flags (plugin, REJILLA_PLUGIN_RUN_PREPROCESSING);

	rejilla_plugin_set_compulsory (plugin, FALSE);
}

G_MODULE_EXPORT void
rejilla_plugin_check_config (RejillaPlugin *plugin)
{
	rejilla_plugin_test_gstreamer_plugin (plugin, "rgvolume");
	rejilla_plugin_test_gstreamer_plugin (plugin, "rganalysis");
}
