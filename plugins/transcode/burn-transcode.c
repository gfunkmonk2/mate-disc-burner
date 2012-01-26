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
#include <math.h>
#include <unistd.h>
#include <fcntl.h>

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>
#include <gmodule.h>

#include <gst/gst.h>

#include "burn-basics.h"
#include "rejilla-medium.h"
#include "rejilla-tags.h"
#include "burn-job.h"
#include "rejilla-plugin-registration.h"
#include "burn-normalize.h"


#define REJILLA_TYPE_TRANSCODE         (rejilla_transcode_get_type ())
#define REJILLA_TRANSCODE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), REJILLA_TYPE_TRANSCODE, RejillaTranscode))
#define REJILLA_TRANSCODE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), REJILLA_TYPE_TRANSCODE, RejillaTranscodeClass))
#define REJILLA_IS_TRANSCODE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), REJILLA_TYPE_TRANSCODE))
#define REJILLA_IS_TRANSCODE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), REJILLA_TYPE_TRANSCODE))
#define REJILLA_TRANSCODE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), REJILLA_TYPE_TRANSCODE, RejillaTranscodeClass))

REJILLA_PLUGIN_BOILERPLATE (RejillaTranscode, rejilla_transcode, REJILLA_TYPE_JOB, RejillaJob);

static gboolean rejilla_transcode_bus_messages (GstBus *bus,
						GstMessage *msg,
						RejillaTranscode *transcode);
static void rejilla_transcode_new_decoded_pad_cb (GstElement *decode,
						  GstPad *pad,
						  gboolean arg2,
						  RejillaTranscode *transcode);

struct RejillaTranscodePrivate {
	GstElement *pipeline;
	GstElement *convert;
	GstElement *source;
	GstElement *decode;
	GstElement *sink;

	/* element to link decode to */
	GstElement *link;

	gint pad_size;
	gint pad_fd;
	gint pad_id;

	gint64 size;
	gint64 pos;

	gulong probe;
	gint64 segment_start;
	gint64 segment_end;

	guint set_active_state:1;
	guint mp3_size_pipeline:1;
};
typedef struct RejillaTranscodePrivate RejillaTranscodePrivate;

#define REJILLA_TRANSCODE_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REJILLA_TYPE_TRANSCODE, RejillaTranscodePrivate))

static GObjectClass *parent_class = NULL;

static gboolean
rejilla_transcode_buffer_handler (GstPad *pad,
				  GstBuffer *buffer,
				  RejillaTranscode *self)
{
	RejillaTranscodePrivate *priv;
	GstPad *peer;
	gint64 size;

	priv = REJILLA_TRANSCODE_PRIVATE (self);

	size = GST_BUFFER_SIZE (buffer);

	if (priv->segment_start <= 0 && priv->segment_end <= 0)
		return TRUE;

	/* what we do here is more or less what gstreamer does when seeking:
	 * it reads and process from 0 to the seek position (I tried).
	 * It even forwards the data before the seek position to the sink (which
	 * is a problem in our case as it would be written) */
	if (priv->size > priv->segment_end) {
		priv->size += size;
		return FALSE;
	}

	if (priv->size + size > priv->segment_end) {
		GstBuffer *new_buffer;
		int data_size;

		/* the entire the buffer is not interesting for us */
		/* create a new buffer and push it on the pad:
		 * NOTE: we're going to receive it ... */
		data_size = priv->segment_end - priv->size;
		new_buffer = gst_buffer_new_and_alloc (data_size);
		memcpy (GST_BUFFER_DATA (new_buffer), GST_BUFFER_DATA (buffer), data_size);

		/* Recursive: the following calls ourselves BEFORE we finish */
		peer = gst_pad_get_peer (pad);
		gst_pad_push (peer, new_buffer);

		priv->size += size - data_size;

		/* post an EOS event to stop pipeline */
		gst_pad_push_event (peer, gst_event_new_eos ());
		gst_object_unref (peer);
		return FALSE;
	}

	/* see if the buffer is in the segment */
	if (priv->size < priv->segment_start) {
		GstBuffer *new_buffer;
		gint data_size;

		/* see if all the buffer is interesting for us */
		if (priv->size + size < priv->segment_start) {
			priv->size += size;
			return FALSE;
		}

		/* create a new buffer and push it on the pad:
		 * NOTE: we're going to receive it ... */
		data_size = priv->size + size - priv->segment_start;
		new_buffer = gst_buffer_new_and_alloc (data_size);
		memcpy (GST_BUFFER_DATA (new_buffer),
			GST_BUFFER_DATA (buffer) +
			GST_BUFFER_SIZE (buffer) -
			data_size,
			data_size);
		GST_BUFFER_TIMESTAMP (new_buffer) = GST_BUFFER_TIMESTAMP (buffer) + data_size;

		/* move forward by the size of bytes we dropped */
		priv->size += size - data_size;

		/* this is recursive the following calls ourselves 
		 * BEFORE we finish */
		peer = gst_pad_get_peer (pad);
		gst_pad_push (peer, new_buffer);
		gst_object_unref (peer);

		return FALSE;
	}

	priv->size += size;
	priv->pos += size;

	return TRUE;
}

static RejillaBurnResult
rejilla_transcode_set_boundaries (RejillaTranscode *transcode)
{
	RejillaTranscodePrivate *priv;
	RejillaTrack *track;
	gint64 start;
	gint64 end;

	priv = REJILLA_TRANSCODE_PRIVATE (transcode);

	/* we need to reach the song start and set a possible end; this is only
	 * needed when it is decoding a song. Otherwise*/
	rejilla_job_get_current_track (REJILLA_JOB (transcode), &track);
	start = rejilla_track_stream_get_start (REJILLA_TRACK_STREAM (track));
	end = rejilla_track_stream_get_end (REJILLA_TRACK_STREAM (track));

	priv->segment_start = REJILLA_DURATION_TO_BYTES (start);
	priv->segment_end = REJILLA_DURATION_TO_BYTES (end);

	REJILLA_JOB_LOG (transcode, "settings track boundaries time = %lli %lli / bytes = %lli %lli",
			 start, end,
			 priv->segment_start, priv->segment_end);

	return REJILLA_BURN_OK;
}

static void
rejilla_transcode_send_volume_event (RejillaTranscode *transcode)
{
	RejillaTranscodePrivate *priv;
	gdouble track_peak = 0.0;
	gdouble track_gain = 0.0;
	GstTagList *tag_list;
	RejillaTrack *track;
	GstEvent *event;
	GValue *value;

	priv = REJILLA_TRANSCODE_PRIVATE (transcode);

	rejilla_job_get_current_track (REJILLA_JOB (transcode), &track);

	REJILLA_JOB_LOG (transcode, "Sending audio levels tags");
	if (rejilla_track_tag_lookup (track, REJILLA_TRACK_PEAK_VALUE, &value) == REJILLA_BURN_OK)
		track_peak = g_value_get_double (value);

	if (rejilla_track_tag_lookup (track, REJILLA_TRACK_GAIN_VALUE, &value) == REJILLA_BURN_OK)
		track_gain = g_value_get_double (value);

	/* it's possible we fail */
	tag_list = gst_tag_list_new ();
	gst_tag_list_add (tag_list, GST_TAG_MERGE_REPLACE,
			  GST_TAG_TRACK_GAIN, track_gain,
			  GST_TAG_TRACK_PEAK, track_peak,
			  NULL);

	/* NOTE: that event is goind downstream */
	event = gst_event_new_tag (tag_list);
	if (!gst_element_send_event (priv->convert, event))
		REJILLA_JOB_LOG (transcode, "Couldn't send tags to rgvolume");

	REJILLA_JOB_LOG (transcode, "Set volume level %lf %lf", track_gain, track_peak);
}

static GstElement *
rejilla_transcode_create_volume (RejillaTranscode *transcode,
				 RejillaTrack *track)
{
	GstElement *volume = NULL;

	/* see if we need a volume object */
	if (rejilla_track_tag_lookup (track, REJILLA_TRACK_PEAK_VALUE, NULL) == REJILLA_BURN_OK
	||  rejilla_track_tag_lookup (track, REJILLA_TRACK_GAIN_VALUE, NULL) == REJILLA_BURN_OK) {
		REJILLA_JOB_LOG (transcode, "Found audio levels tags");
		volume = gst_element_factory_make ("rgvolume", NULL);
		if (volume)
			g_object_set (volume,
				      "album-mode", FALSE,
				      NULL);
		else
			REJILLA_JOB_LOG (transcode, "rgvolume object couldn't be created");
	}

	return volume;
}

static gboolean
rejilla_transcode_create_pipeline_size_mp3 (RejillaTranscode *transcode,
					    GstElement *pipeline,
					    GstElement *source,
                                            GError **error)
{
	RejillaTranscodePrivate *priv;
	GstElement *parse;
	GstElement *sink;

	priv = REJILLA_TRANSCODE_PRIVATE (transcode);

	REJILLA_JOB_LOG (transcode, "Creating specific pipeline for MP3s");

	parse = gst_element_factory_make ("mp3parse", NULL);
	if (!parse) {
		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_GENERAL,
			     _("%s element could not be created"),
			     "\"Mp3parse\"");
		g_object_unref (pipeline);
		return FALSE;
	}
	gst_bin_add (GST_BIN (pipeline), parse);

	sink = gst_element_factory_make ("fakesink", NULL);
	if (!sink) {
		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_GENERAL,
			     _("%s element could not be created"),
			     "\"Fakesink\"");
		g_object_unref (pipeline);
		return FALSE;
	}
	gst_bin_add (GST_BIN (pipeline), sink);

	/* Link */
	if (!gst_element_link_many (source, parse, sink, NULL)) {
		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_GENERAL,
			     _("Impossible to link plugin pads"));
		REJILLA_JOB_LOG (transcode, "Impossible to link elements");
		g_object_unref (pipeline);
		return FALSE;
	}

	priv->convert = NULL;

	priv->sink = sink;
	priv->source = source;
	priv->pipeline = pipeline;

	/* Get it going */
	gst_element_set_state (priv->pipeline, GST_STATE_PLAYING);
	return TRUE;
}

static void
rejilla_transcode_error_on_pad_linking (RejillaTranscode *self,
                                        const gchar *function_name)
{
	RejillaTranscodePrivate *priv;
	GstMessage *message;
	GstBus *bus;

	priv = REJILLA_TRANSCODE_PRIVATE (self);

	REJILLA_JOB_LOG (self, "Error on pad linking");
	message = gst_message_new_error (GST_OBJECT (priv->pipeline),
					 g_error_new (REJILLA_BURN_ERROR,
						      REJILLA_BURN_ERROR_GENERAL,
						      /* Translators: This message is sent
						       * when rejilla could not link together
						       * two gstreamer plugins so that one
						       * sends its data to the second for further
						       * processing. This data transmission is
						       * done through a pad. Maybe this is a bit
						       * too technical and should be removed? */
						      _("Impossible to link plugin pads")),
					 function_name);

	bus = gst_pipeline_get_bus (GST_PIPELINE (priv->pipeline));
	gst_bus_post (bus, message);
	g_object_unref (bus);
}

static void
rejilla_transcode_wavparse_pad_added_cb (GstElement *wavparse,
                                         GstPad *new_pad,
                                         gpointer user_data)
{
	GstPad *pad = NULL;
	RejillaTranscodePrivate *priv;

	priv = REJILLA_TRANSCODE_PRIVATE (user_data);

	pad = gst_element_get_static_pad (priv->sink, "sink");
	if (!pad) 
		goto error;

	if (gst_pad_link (new_pad, pad) != GST_PAD_LINK_OK)
		goto error;

	gst_element_set_state (priv->sink, GST_STATE_PLAYING);
	return;

error:

	if (pad)
		gst_object_unref (pad);

	rejilla_transcode_error_on_pad_linking (REJILLA_TRANSCODE (user_data), "Sent by rejilla_transcode_wavparse_pad_added_cb");
}

static gboolean
rejilla_transcode_create_pipeline (RejillaTranscode *transcode,
				   GError **error)
{
	gchar *uri;
	gboolean keep_dts;
	GstElement *decode;
	GstElement *source;
	GstBus *bus = NULL;
	GstCaps *filtercaps;
	GValue *value = NULL;
	GstElement *pipeline;
	GstElement *sink = NULL;
	RejillaJobAction action;
	GstElement *filter = NULL;
	GstElement *volume = NULL;
	GstElement *convert = NULL;
	RejillaTrack *track = NULL;
	GstElement *resample = NULL;
	RejillaTranscodePrivate *priv;

	priv = REJILLA_TRANSCODE_PRIVATE (transcode);

	REJILLA_JOB_LOG (transcode, "Creating new pipeline");

	priv->set_active_state = 0;

	/* free the possible current pipeline and create a new one */
	if (priv->pipeline) {
		gst_element_set_state (priv->pipeline, GST_STATE_NULL);
		gst_object_unref (G_OBJECT (priv->pipeline));
		priv->link = NULL;
		priv->sink = NULL;
		priv->source = NULL;
		priv->convert = NULL;
		priv->pipeline = NULL;
	}

	/* create three types of pipeline according to the needs: (possibly adding grvolume)
	 * - filesrc ! decodebin ! audioconvert ! fakesink (find size) and filesrc!mp3parse!fakesink for mp3s
	 * - filesrc ! decodebin ! audioresample ! audioconvert ! audio/x-raw-int,rate=44100,width=16,depth=16,endianness=4321,signed ! filesink
	 * - filesrc ! decodebin ! audioresample ! audioconvert ! audio/x-raw-int,rate=44100,width=16,depth=16,endianness=4321,signed ! fdsink
	 */
	pipeline = gst_pipeline_new (NULL);

	bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
	gst_bus_add_watch (bus,
			   (GstBusFunc) rejilla_transcode_bus_messages,
			   transcode);
	gst_object_unref (bus);

	/* source */
	rejilla_job_get_current_track (REJILLA_JOB (transcode), &track);
	uri = rejilla_track_stream_get_source (REJILLA_TRACK_STREAM (track), TRUE);
	source = gst_element_make_from_uri (GST_URI_SRC, uri, NULL);
	g_free (uri);

	if (source == NULL) {
		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_GENERAL,
			     /* Translators: %s is the name of the object (as in
			      * GObject) from the Gstreamer library that could
			      * not be created */
			     _("%s element could not be created"),
			     "\"Source\"");
		goto error;
	}
	gst_bin_add (GST_BIN (pipeline), source);
	g_object_set (source,
		      "typefind", FALSE,
		      NULL);

	/* sink */
	rejilla_job_get_action (REJILLA_JOB (transcode), &action);
	switch (action) {
	case REJILLA_JOB_ACTION_SIZE:
		if (priv->mp3_size_pipeline)
			return rejilla_transcode_create_pipeline_size_mp3 (transcode, pipeline, source, error);

		sink = gst_element_factory_make ("fakesink", NULL);
		break;

	case REJILLA_JOB_ACTION_IMAGE:
		volume = rejilla_transcode_create_volume (transcode, track);

		if (rejilla_job_get_fd_out (REJILLA_JOB (transcode), NULL) != REJILLA_BURN_OK) {
			gchar *output;

			rejilla_job_get_image_output (REJILLA_JOB (transcode),
						      &output,
						      NULL);
			sink = gst_element_factory_make ("filesink", NULL);
			g_object_set (sink,
				      "location", output,
				      NULL);
			g_free (output);
		}
		else {
			int fd;

			rejilla_job_get_fd_out (REJILLA_JOB (transcode), &fd);
			sink = gst_element_factory_make ("fdsink", NULL);
			g_object_set (sink,
				      "fd", fd,
				      NULL);
		}
		break;

	default:
		goto error;
	}

	if (!sink) {
		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_GENERAL,
			     _("%s element could not be created"),
			     "\"Sink\"");
		goto error;
	}

	gst_bin_add (GST_BIN (pipeline), sink);
	g_object_set (sink,
		      "sync", FALSE,
		      NULL);

	rejilla_job_tag_lookup (REJILLA_JOB (transcode),
				REJILLA_SESSION_STREAM_AUDIO_FORMAT,
				&value);
	if (value)
		keep_dts = (g_value_get_int (value) & REJILLA_AUDIO_FORMAT_DTS) != 0;
	else
		keep_dts = FALSE;

	if (keep_dts
	&&  action == REJILLA_JOB_ACTION_IMAGE
	&& (rejilla_track_stream_get_format (REJILLA_TRACK_STREAM (track)) & REJILLA_AUDIO_FORMAT_DTS) != 0) {
		GstElement *wavparse;
		GstPad *sinkpad;

		REJILLA_JOB_LOG (transcode, "DTS wav pipeline");

		/* FIXME: volume normalization won't work here. We'd need to 
		 * reencode it afterwards otherwise. */
		/* This is a special case. This is DTS wav. So we only decode wav. */
		wavparse = gst_element_factory_make ("wavparse", NULL);
		if (wavparse == NULL) {
			g_set_error (error,
				     REJILLA_BURN_ERROR,
				     REJILLA_BURN_ERROR_GENERAL,
				     _("%s element could not be created"),
				     "\"Wavparse\"");
			goto error;
		}
		gst_bin_add (GST_BIN (pipeline), wavparse);

		if (!gst_element_link (source, wavparse)) {
			g_set_error (error,
				     REJILLA_BURN_ERROR,
				     REJILLA_BURN_ERROR_GENERAL,
			             _("Impossible to link plugin pads"));
			goto error;
		}

		g_signal_connect (wavparse,
		                  "pad-added",
		                  G_CALLBACK (rejilla_transcode_wavparse_pad_added_cb),
		                  transcode);

		/* This is an ugly workaround for the lack of accuracy with
		 * gstreamer. Yet this is unfortunately a necessary evil. */
		priv->pos = 0;
		priv->size = 0;
		sinkpad = gst_element_get_pad (sink, "sink");
		priv->probe = gst_pad_add_buffer_probe (sinkpad,
							G_CALLBACK (rejilla_transcode_buffer_handler),
							transcode);
		gst_object_unref (sinkpad);


		priv->link = NULL;
		priv->sink = sink;
		priv->decode = NULL;
		priv->source = source;
		priv->convert = NULL;
		priv->pipeline = pipeline;

		gst_element_set_state (pipeline, GST_STATE_PLAYING);
		return TRUE;
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

	if (action == REJILLA_JOB_ACTION_IMAGE) {
		RejillaStreamFormat session_format;
		RejillaTrackType *output_type;

		output_type = rejilla_track_type_new ();
		rejilla_job_get_output_type (REJILLA_JOB (transcode), output_type);
		session_format = rejilla_track_type_get_stream_format (output_type);
		rejilla_track_type_free (output_type);

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

		/* filter */
		filter = gst_element_factory_make ("capsfilter", NULL);
		if (!filter) {
			g_set_error (error,
				     REJILLA_BURN_ERROR,
				     REJILLA_BURN_ERROR_GENERAL,
				     _("%s element could not be created"),
				     "\"Filter\"");
			goto error;
		}
		gst_bin_add (GST_BIN (pipeline), filter);
		filtercaps = gst_caps_new_full (gst_structure_new ("audio/x-raw-int",
								   "channels", G_TYPE_INT, 2,
								   "width", G_TYPE_INT, 16,
								   "depth", G_TYPE_INT, 16,
								   /* NOTE: we use little endianness only for libburn which requires little */
								   "endianness", G_TYPE_INT, (session_format & REJILLA_AUDIO_FORMAT_RAW_LITTLE_ENDIAN) != 0 ? 1234:4321,
								   "rate", G_TYPE_INT, 44100,
								   "signed", G_TYPE_BOOLEAN, TRUE,
								   NULL),
						NULL);
		g_object_set (GST_OBJECT (filter), "caps", filtercaps, NULL);
		gst_caps_unref (filtercaps);
	}

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

	if (action == REJILLA_JOB_ACTION_IMAGE) {
		GstPad *sinkpad;
		gboolean res;

		if (!gst_element_link (source, decode)) {
			REJILLA_JOB_LOG (transcode, "Impossible to link plugin pads");
			g_set_error (error,
				     REJILLA_BURN_ERROR,
				     REJILLA_BURN_ERROR_GENERAL,
			             _("Impossible to link plugin pads"));
			goto error;
		}

		priv->link = resample;
		g_signal_connect (G_OBJECT (decode),
				  "new-decoded-pad",
				  G_CALLBACK (rejilla_transcode_new_decoded_pad_cb),
				  transcode);

		if (volume) {
			gst_bin_add (GST_BIN (pipeline), volume);
			res = gst_element_link_many (resample,
			                             volume,
						     convert,
			                             filter,
			                             sink,
			                             NULL);
		}
		else
			res = gst_element_link_many (resample,
			                             convert,
			                             filter,
			                             sink,
			                             NULL);

		if (!res) {
			REJILLA_JOB_LOG (transcode, "Impossible to link plugin pads");
			g_set_error (error,
				     REJILLA_BURN_ERROR,
				     REJILLA_BURN_ERROR_GENERAL,
				     _("Impossible to link plugin pads"));
			goto error;
		}

		/* This is an ugly workaround for the lack of accuracy with
		 * gstreamer. Yet this is unfortunately a necessary evil. */
		priv->pos = 0;
		priv->size = 0;
		sinkpad = gst_element_get_pad (sink, "sink");
		priv->probe = gst_pad_add_buffer_probe (sinkpad,
							G_CALLBACK (rejilla_transcode_buffer_handler),
							transcode);
		gst_object_unref (sinkpad);
	}
	else {
		if (!gst_element_link (source, decode)
		||  !gst_element_link (convert, sink)) {
			REJILLA_JOB_LOG (transcode, "Impossible to link plugin pads");
			g_set_error (error,
				     REJILLA_BURN_ERROR,
				     REJILLA_BURN_ERROR_GENERAL,
				     _("Impossible to link plugin pads"));
			goto error;
		}

		priv->link = convert;
		g_signal_connect (G_OBJECT (decode),
				  "new-decoded-pad",
				  G_CALLBACK (rejilla_transcode_new_decoded_pad_cb),
				  transcode);
	}

	priv->sink = sink;
	priv->decode = decode;
	priv->source = source;
	priv->convert = convert;
	priv->pipeline = pipeline;

	gst_element_set_state (pipeline, GST_STATE_PLAYING);
	return TRUE;

error:

	if (error && (*error))
		REJILLA_JOB_LOG (transcode,
				 "can't create object : %s \n",
				 (*error)->message);

	gst_object_unref (GST_OBJECT (pipeline));
	return FALSE;
}

static void
rejilla_transcode_set_track_size (RejillaTranscode *transcode,
				  gint64 duration)
{
	gchar *uri;
	RejillaTrack *track;

	rejilla_job_get_current_track (REJILLA_JOB (transcode), &track);
	rejilla_track_stream_set_boundaries (REJILLA_TRACK_STREAM (track), -1, duration, -1);
	duration += rejilla_track_stream_get_gap (REJILLA_TRACK_STREAM (track));

	/* if transcoding on the fly we should add some length just to make
	 * sure we won't be too short (gstreamer duration discrepancy) */
	rejilla_job_set_output_size_for_current_track (REJILLA_JOB (transcode),
						       REJILLA_DURATION_TO_SECTORS (duration),
						       REJILLA_DURATION_TO_BYTES (duration));

	uri = rejilla_track_stream_get_source (REJILLA_TRACK_STREAM (track), FALSE);
	REJILLA_JOB_LOG (transcode,
			 "Song %s"
			 "\nsectors %" G_GINT64_FORMAT
			 "\ntime %" G_GINT64_FORMAT, 
			 uri,
			 REJILLA_DURATION_TO_SECTORS (duration),
			 duration);
	g_free (uri);
}

/**
 * These functions are to deal with siblings
 */

static RejillaBurnResult
rejilla_transcode_create_sibling_size (RejillaTranscode *transcode,
				        RejillaTrack *src,
				        GError **error)
{
	RejillaTrack *dest;
	guint64 duration;

	/* it means the same file uri is in the selection and was already
	 * checked. Simply get the values for the length and other information
	 * and copy them. */
	/* NOTE: no need to copy the length since if they are sibling that means
	 * that they have the same length */
	rejilla_track_stream_get_length (REJILLA_TRACK_STREAM (src), &duration);
	rejilla_job_set_output_size_for_current_track (REJILLA_JOB (transcode),
						       REJILLA_DURATION_TO_SECTORS (duration),
						       REJILLA_DURATION_TO_BYTES (duration));

	/* copy the info we are missing */
	rejilla_job_get_current_track (REJILLA_JOB (transcode), &dest);
	rejilla_track_tag_copy_missing (dest, src);

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_transcode_create_sibling_image (RejillaTranscode *transcode,
					RejillaTrack *src,
					GError **error)
{
	RejillaTrackStream *dest;
	RejillaTrack *track;
	guint64 length = 0;
	gchar *path_dest;
	gchar *path_src;

	/* it means the file is already in the selection. Simply create a 
	 * symlink pointing to first file in the selection with the same uri */
	path_src = rejilla_track_stream_get_source (REJILLA_TRACK_STREAM (src), FALSE);
	rejilla_job_get_audio_output (REJILLA_JOB (transcode), &path_dest);

	if (symlink (path_src, path_dest) == -1) {
                int errsv = errno;

		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_GENERAL,
			     /* Translators: the %s is the error message from errno */
			     _("An internal error occurred (%s)"),
			     g_strerror (errsv));

		goto error;
	}

	dest = rejilla_track_stream_new ();
	rejilla_track_stream_set_source (dest, path_dest);

	/* FIXME: what if input had metadata ?*/
	rejilla_track_stream_set_format (dest, REJILLA_AUDIO_FORMAT_RAW);

	/* NOTE: there is no gap and start = 0 since these tracks are the result
	 * of the transformation of previous ones */
	rejilla_track_stream_get_length (REJILLA_TRACK_STREAM (src), &length);
	rejilla_track_stream_set_boundaries (dest, 0, length, 0);

	/* copy all infos but from the current track */
	rejilla_job_get_current_track (REJILLA_JOB (transcode), &track);
	rejilla_track_tag_copy_missing (REJILLA_TRACK (dest), track);
	rejilla_job_add_track (REJILLA_JOB (transcode), REJILLA_TRACK (dest));

	/* It's good practice to unref the track afterwards as we don't need it
	 * anymore. RejillaTaskCtx refs it. */
	g_object_unref (dest);

	g_free (path_src);
	g_free (path_dest);

	return REJILLA_BURN_NOT_RUNNING;

error:
	g_free (path_src);
	g_free (path_dest);

	return REJILLA_BURN_ERR;
}

static RejillaTrack *
rejilla_transcode_search_for_sibling (RejillaTranscode *transcode)
{
	RejillaJobAction action;
	GSList *iter, *songs;
	RejillaTrack *track;
	gint64 start;
	gint64 end;
	gchar *uri;

	rejilla_job_get_action (REJILLA_JOB (transcode), &action);

	rejilla_job_get_current_track (REJILLA_JOB (transcode), &track);
	start = rejilla_track_stream_get_start (REJILLA_TRACK_STREAM (track));
	end = rejilla_track_stream_get_end (REJILLA_TRACK_STREAM (track));
	uri = rejilla_track_stream_get_source (REJILLA_TRACK_STREAM (track), TRUE);

	rejilla_job_get_done_tracks (REJILLA_JOB (transcode), &songs);

	for (iter = songs; iter; iter = iter->next) {
		gchar *iter_uri;
		gint64 iter_end;
		gint64 iter_start;
		RejillaTrack *iter_track;

		iter_track = iter->data;
		iter_uri = rejilla_track_stream_get_source (REJILLA_TRACK_STREAM (iter_track), TRUE);

		if (strcmp (iter_uri, uri))
			continue;

		iter_end = rejilla_track_stream_get_end (REJILLA_TRACK_STREAM (iter_track));
		if (!iter_end)
			continue;

		if (iter_end != end)
			continue;

		iter_start = rejilla_track_stream_get_start (REJILLA_TRACK_STREAM (track));
		if (iter_start == start) {
			g_free (uri);
			return iter_track;
		}
	}

	g_free (uri);
	return NULL;
}

static RejillaBurnResult
rejilla_transcode_has_track_sibling (RejillaTranscode *transcode,
				     GError **error)
{
	RejillaJobAction action;
	RejillaTrack *sibling = NULL;
	RejillaBurnResult result = REJILLA_BURN_OK;

	if (rejilla_job_get_fd_out (REJILLA_JOB (transcode), NULL) == REJILLA_BURN_OK)
		return REJILLA_BURN_OK;

	sibling = rejilla_transcode_search_for_sibling (transcode);
	if (!sibling)
		return REJILLA_BURN_OK;

	REJILLA_JOB_LOG (transcode, "found sibling: skipping");
	rejilla_job_get_action (REJILLA_JOB (transcode), &action);
	if (action == REJILLA_JOB_ACTION_IMAGE)
		result = rejilla_transcode_create_sibling_image (transcode,
								 sibling,
								 error);
	else if (action == REJILLA_JOB_ACTION_SIZE)
		result = rejilla_transcode_create_sibling_size (transcode,
								sibling,
								error);

	return result;
}

static RejillaBurnResult
rejilla_transcode_start (RejillaJob *job,
			 GError **error)
{
	RejillaTranscode *transcode;
	RejillaBurnResult result;
	RejillaJobAction action;

	transcode = REJILLA_TRANSCODE (job);

	rejilla_job_get_action (job, &action);
	rejilla_job_set_use_average_rate (job, TRUE);

	if (action == REJILLA_JOB_ACTION_SIZE) {
		RejillaTrack *track;

		/* see if the track size was already set since then no need to 
		 * carry on with a lengthy get size and the library will do it
		 * itself. */
		rejilla_job_get_current_track (job, &track);
		if (rejilla_track_stream_get_end (REJILLA_TRACK_STREAM (track)) > 0)
			return REJILLA_BURN_NOT_SUPPORTED;

		if (!rejilla_transcode_create_pipeline (transcode, error))
			return REJILLA_BURN_ERR;

		rejilla_job_set_current_action (job,
						REJILLA_BURN_ACTION_GETTING_SIZE,
						NULL,
						TRUE);

		rejilla_job_start_progress (job, FALSE);
		return REJILLA_BURN_OK;
	}

	if (action == REJILLA_JOB_ACTION_IMAGE) {
		/* Look for a sibling to avoid transcoding twice. In this case
		 * though start and end of this track must be inside start and
		 * end of the previous track. Of course if we are piping that
		 * operation is simply impossible. */
		if (rejilla_job_get_fd_out (job, NULL) != REJILLA_BURN_OK) {
			result = rejilla_transcode_has_track_sibling (REJILLA_TRANSCODE (job), error);
			if (result != REJILLA_BURN_OK)
				return result;
		}

		rejilla_transcode_set_boundaries (transcode);
		if (!rejilla_transcode_create_pipeline (transcode, error))
			return REJILLA_BURN_ERR;
	}
	else
		REJILLA_JOB_NOT_SUPPORTED (transcode);

	return REJILLA_BURN_OK;
}

static void
rejilla_transcode_stop_pipeline (RejillaTranscode *transcode)
{
	RejillaTranscodePrivate *priv;
	GstPad *sinkpad;

	priv = REJILLA_TRANSCODE_PRIVATE (transcode);
	if (!priv->pipeline)
		return;

	sinkpad = gst_element_get_pad (priv->sink, "sink");
	if (priv->probe)
		gst_pad_remove_buffer_probe (sinkpad, priv->probe);

	gst_object_unref (sinkpad);

	gst_element_set_state (priv->pipeline, GST_STATE_NULL);
	gst_object_unref (GST_OBJECT (priv->pipeline));

	priv->link = NULL;
	priv->sink = NULL;
	priv->source = NULL;
	priv->convert = NULL;
	priv->pipeline = NULL;

	priv->set_active_state = 0;
}

static RejillaBurnResult
rejilla_transcode_stop (RejillaJob *job,
			GError **error)
{
	RejillaTranscodePrivate *priv;

	priv = REJILLA_TRANSCODE_PRIVATE (job);

	priv->mp3_size_pipeline = 0;

	if (priv->pad_id) {
		g_source_remove (priv->pad_id);
		priv->pad_id = 0;
	}

	rejilla_transcode_stop_pipeline (REJILLA_TRANSCODE (job));
	return REJILLA_BURN_OK;
}

/* we must make sure that the track size is a multiple
 * of 2352 to be burnt by cdrecord with on the fly */

static gint64
rejilla_transcode_pad_real (RejillaTranscode *transcode,
			    int fd,
			    gint64 bytes2write,
			    GError **error)
{
	const int buffer_size = 512;
	char buffer [buffer_size];
	gint64 b_written;
	gint64 size;

	b_written = 0;
	bzero (buffer, sizeof (buffer));
	for (; bytes2write; bytes2write -= b_written) {
		size = bytes2write > buffer_size ? buffer_size : bytes2write;
		b_written = write (fd, buffer, (int) size);

		REJILLA_JOB_LOG (transcode,
				 "written %" G_GINT64_FORMAT " bytes for padding",
				 b_written);

		/* we should not handle EINTR and EAGAIN as errors */
		if (b_written < 0) {
			if (errno == EINTR || errno == EAGAIN) {
				REJILLA_JOB_LOG (transcode, "got EINTR / EAGAIN, retrying");
	
				/* we'll try later again */
				return bytes2write;
			}
		}

		if (size != b_written) {
                        int errsv = errno;

			g_set_error (error,
				     REJILLA_BURN_ERROR,
				     REJILLA_BURN_ERROR_GENERAL,
				     /* Translators: %s is the string error from errno */
				     _("Error while padding file (%s)"),
				     g_strerror (errsv));
			return -1;
		}
	}

	return 0;
}

static void
rejilla_transcode_push_track (RejillaTranscode *transcode)
{
	guint64 length = 0;
	gchar *output = NULL;
	RejillaTrack *src = NULL;
	RejillaTrackStream *track;

	rejilla_job_get_audio_output (REJILLA_JOB (transcode), &output);
	rejilla_job_get_current_track (REJILLA_JOB (transcode), &src);

	rejilla_track_stream_get_length (REJILLA_TRACK_STREAM (src), &length);

	track = rejilla_track_stream_new ();
	rejilla_track_stream_set_source (track, output);
	g_free (output);

	/* FIXME: what if input had metadata ?*/
	rejilla_track_stream_set_format (track, REJILLA_AUDIO_FORMAT_RAW);
	rejilla_track_stream_set_boundaries (track, 0, length, 0);
	rejilla_track_tag_copy_missing (REJILLA_TRACK (track), src);

	rejilla_job_add_track (REJILLA_JOB (transcode), REJILLA_TRACK (track));
	
	/* It's good practice to unref the track afterwards as we don't need it
	 * anymore. RejillaTaskCtx refs it. */
	g_object_unref (track);

	rejilla_job_finished_track (REJILLA_JOB (transcode));
}

static gboolean
rejilla_transcode_pad_idle (RejillaTranscode *transcode)
{
	gint64 bytes2write;
	GError *error = NULL;
	RejillaTranscodePrivate *priv;

	priv = REJILLA_TRANSCODE_PRIVATE (transcode);
	bytes2write = rejilla_transcode_pad_real (transcode,
						  priv->pad_fd,
						  priv->pad_size,
						  &error);

	if (bytes2write == -1) {
		priv->pad_id = 0;
		rejilla_job_error (REJILLA_JOB (transcode), error);
		return FALSE;
	}

	if (bytes2write) {
		priv->pad_size = bytes2write;
		return TRUE;
	}

	/* we are finished with padding */
	priv->pad_id = 0;
	close (priv->pad_fd);
	priv->pad_fd = -1;

	/* set the next song or finish */
	rejilla_transcode_push_track (transcode);
	return FALSE;
}

static gboolean
rejilla_transcode_pad (RejillaTranscode *transcode, int fd, GError **error)
{
	guint64 length = 0;
	gint64 bytes2write = 0;
	RejillaTrack *track = NULL;
	RejillaTranscodePrivate *priv;

	priv = REJILLA_TRANSCODE_PRIVATE (transcode);
	if (priv->pos < 0)
		return TRUE;

	/* Padding is important for two reasons:
	 * - first if didn't output enough bytes compared to what we should have
	 * - second we must output a multiple of 2352 to respect sector
	 *   boundaries */
	rejilla_job_get_current_track (REJILLA_JOB (transcode), &track);
	rejilla_track_stream_get_length (REJILLA_TRACK_STREAM (track), &length);

	if (priv->pos < REJILLA_DURATION_TO_BYTES (length)) {
		gint64 b_written = 0;

		/* Check bytes boundary for length */
		b_written = REJILLA_DURATION_TO_BYTES (length);
		b_written += (b_written % 2352) ? 2352 - (b_written % 2352):0;
		bytes2write = b_written - priv->pos;

		REJILLA_JOB_LOG (transcode,
				 "wrote %lli bytes (= %lli ns) out of %lli (= %lli ns)"
				 "\n=> padding %lli bytes",
				 priv->pos,
				 REJILLA_BYTES_TO_DURATION (priv->pos),
				 REJILLA_DURATION_TO_BYTES (length),
				 length,
				 bytes2write);
	}
	else {
		gint64 b_written = 0;

		/* wrote more or the exact amount of bytes. Check bytes boundary */
		b_written = priv->pos;
		bytes2write = (b_written % 2352) ? 2352 - (b_written % 2352):0;
		REJILLA_JOB_LOG (transcode,
				 "wrote %lli bytes (= %lli ns)"
				 "\n=> padding %lli bytes",
				 b_written,
				 priv->pos,
				 bytes2write);
	}

	if (!bytes2write)
		return TRUE;

	bytes2write = rejilla_transcode_pad_real (transcode,
						  fd,
						  bytes2write,
						  error);
	if (bytes2write == -1)
		return TRUE;

	if (bytes2write) {
		RejillaTranscodePrivate *priv;

		priv = REJILLA_TRANSCODE_PRIVATE (transcode);
		/* when writing to a pipe it can happen that its buffer is full
		 * because cdrecord is not fast enough. Therefore we couldn't
		 * write/pad it and we'll have to wait for the pipe to become
		 * available again */
		priv->pad_fd = fd;
		priv->pad_size = bytes2write;
		priv->pad_id = g_timeout_add (50,
					     (GSourceFunc) rejilla_transcode_pad_idle,
					      transcode);
		return FALSE;		
	}

	return TRUE;
}

static gboolean
rejilla_transcode_pad_pipe (RejillaTranscode *transcode, GError **error)
{
	int fd;
	gboolean result;

	rejilla_job_get_fd_out (REJILLA_JOB (transcode), &fd);
	fd = dup (fd);

	result = rejilla_transcode_pad (transcode, fd, error);
	if (result)
		close (fd);

	return result;
}

static gboolean
rejilla_transcode_pad_file (RejillaTranscode *transcode, GError **error)
{
	int fd;
	gchar *output;
	gboolean result;

	output = NULL;
	rejilla_job_get_audio_output (REJILLA_JOB (transcode), &output);
	fd = open (output, O_WRONLY | O_CREAT | O_APPEND, S_IRWXU | S_IRGRP | S_IROTH);
	g_free (output);

	if (fd == -1) {
                int errsv = errno;

		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_GENERAL,
			     /* Translators: %s is the string error from errno */
			     _("Error while padding file (%s)"),
			     g_strerror (errsv));
		return FALSE;
	}

	result = rejilla_transcode_pad (transcode, fd, error);
	if (result)
		close (fd);

	return result;
}

static gboolean
rejilla_transcode_is_mp3 (RejillaTranscode *transcode)
{
	RejillaTranscodePrivate *priv;
	GstElement *typefind;
	GstCaps *caps = NULL;
	const gchar *mime;

	priv = REJILLA_TRANSCODE_PRIVATE (transcode);

	/* find the type of the file */
	typefind = gst_bin_get_by_name (GST_BIN (priv->decode), "typefind");
	g_object_get (typefind, "caps", &caps, NULL);
	if (!caps) {
		gst_object_unref (typefind);
		return TRUE;
	}

	if (caps && gst_caps_get_size (caps) > 0) {
		mime = gst_structure_get_name (gst_caps_get_structure (caps, 0));
		gst_object_unref (typefind);

		if (mime && !strcmp (mime, "application/x-id3"))
			return TRUE;

		if (!strcmp (mime, "audio/mpeg"))
			return TRUE;
	}
	else
		gst_object_unref (typefind);

	return FALSE;
}

static gint64
rejilla_transcode_get_duration (RejillaTranscode *transcode)
{
	gint64 duration = -1;
	RejillaTranscodePrivate *priv;
	GstFormat format = GST_FORMAT_TIME;

	priv = REJILLA_TRANSCODE_PRIVATE (transcode);

	/* This part is specific to MP3s */
	if (priv->mp3_size_pipeline) {
		/* This is the most reliable way to get the duration for mp3
		 * read them till the end and get the position. */
		gst_element_query_position (priv->pipeline,
					    &format,
					    &duration);
	}

	/* This is for any sort of files */
	if (duration == -1 || duration == 0)
		gst_element_query_duration (priv->pipeline,
					    &format,
					    &duration);

	REJILLA_JOB_LOG (transcode, "got duration %"G_GINT64_FORMAT, duration);

	if (duration == -1 || duration == 0)	
	    rejilla_job_error (REJILLA_JOB (transcode),
			       g_error_new (REJILLA_BURN_ERROR,
					    REJILLA_BURN_ERROR_GENERAL,
					    _("Error while getting duration")));
	return duration;
}

static gboolean
rejilla_transcode_song_end_reached (RejillaTranscode *transcode)
{
	GError *error = NULL;
	RejillaJobAction action;

	rejilla_job_get_action (REJILLA_JOB (transcode), &action);
	if (action == REJILLA_JOB_ACTION_SIZE) {
		gint64 duration;

		/* this is when we need to write infs:
		 * - when asked to create infs
		 * - when decoding to a file */
		duration = rejilla_transcode_get_duration (transcode);
		if (duration == -1)
			return FALSE;

		rejilla_transcode_set_track_size (transcode, duration);
		rejilla_job_finished_track (REJILLA_JOB (transcode));
		return TRUE;
	}

	if (action == REJILLA_JOB_ACTION_IMAGE) {
		gboolean result;

		/* pad file so it is a multiple of 2352 (= 1 sector) */
		if (rejilla_job_get_fd_out (REJILLA_JOB (transcode), NULL) == REJILLA_BURN_OK)
			result = rejilla_transcode_pad_pipe (transcode, &error);
		else
			result = rejilla_transcode_pad_file (transcode, &error);
	
		if (error) {
			rejilla_job_error (REJILLA_JOB (transcode), error);
			return FALSE;
		}

		if (!result) {
			rejilla_transcode_stop_pipeline (transcode);
			return FALSE;
		}
	}

	rejilla_transcode_push_track (transcode);
	return TRUE;
}

static void
foreach_tag (const GstTagList *list,
	     const gchar *tag,
	     RejillaTranscode *transcode)
{
	RejillaTrack *track;
	RejillaJobAction action;

	rejilla_job_get_action (REJILLA_JOB (transcode), &action);
	rejilla_job_get_current_track (REJILLA_JOB (transcode), &track);

	REJILLA_JOB_LOG (transcode, "Retrieving tags");

	if (!strcmp (tag, GST_TAG_TITLE)) {
		if (!rejilla_track_tag_lookup_string (track, REJILLA_TRACK_STREAM_TITLE_TAG)) {
			gchar *title = NULL;

			gst_tag_list_get_string (list, tag, &title);
			rejilla_track_tag_add_string (track,
						      REJILLA_TRACK_STREAM_TITLE_TAG,
						      title);
			g_free (title);
		}
	}
	else if (!strcmp (tag, GST_TAG_ARTIST)) {
		if (!rejilla_track_tag_lookup_string (track, REJILLA_TRACK_STREAM_ARTIST_TAG)) {
			gchar *artist = NULL;

			gst_tag_list_get_string (list, tag, &artist);
			rejilla_track_tag_add_string (track,
						      REJILLA_TRACK_STREAM_ARTIST_TAG,
						      artist);
			g_free (artist);
		}
	}
	else if (!strcmp (tag, GST_TAG_ISRC)) {
		if (!rejilla_track_tag_lookup_int (track, REJILLA_TRACK_STREAM_ISRC_TAG)) {
			gchar *isrc = NULL;

			gst_tag_list_get_string (list, tag, &isrc);
			rejilla_track_tag_add_int (track,
						   REJILLA_TRACK_STREAM_ISRC_TAG,
						   (int) g_ascii_strtoull (isrc, NULL, 10));
		}
	}
	else if (!strcmp (tag, GST_TAG_PERFORMER)) {
		if (!rejilla_track_tag_lookup_string (track, REJILLA_TRACK_STREAM_ARTIST_TAG)) {
			gchar *artist = NULL;

			gst_tag_list_get_string (list, tag, &artist);
			rejilla_track_tag_add_string (track,
						      REJILLA_TRACK_STREAM_ARTIST_TAG,
						      artist);
			g_free (artist);
		}
	}
	else if (action == REJILLA_JOB_ACTION_SIZE
	     &&  !strcmp (tag, GST_TAG_DURATION)) {
		guint64 duration;

		/* this is only useful when we try to have the size */
		gst_tag_list_get_uint64 (list, tag, &duration);
		rejilla_track_stream_set_boundaries (REJILLA_TRACK_STREAM (track), 0, duration, -1);
	}
}

/* NOTE: the return value is whether or not we should stop the bus callback */
static gboolean
rejilla_transcode_active_state (RejillaTranscode *transcode)
{
	RejillaTranscodePrivate *priv;
	gchar *name, *string, *uri;
	RejillaJobAction action;
	RejillaTrack *track;

	priv = REJILLA_TRANSCODE_PRIVATE (transcode);

	if (priv->set_active_state)
		return TRUE;

	priv->set_active_state = TRUE;

	rejilla_job_get_current_track (REJILLA_JOB (transcode), &track);
	uri = rejilla_track_stream_get_source (REJILLA_TRACK_STREAM (track), FALSE);

	rejilla_job_get_action (REJILLA_JOB (transcode), &action);
	if (action == REJILLA_JOB_ACTION_SIZE) {
		REJILLA_JOB_LOG (transcode,
				 "Analysing Track %s",
				 uri);

		if (priv->mp3_size_pipeline) {
			gchar *escaped_basename;

			/* Run the pipeline till the end */
			escaped_basename = g_path_get_basename (uri);
			name = g_uri_unescape_string (escaped_basename, NULL);
			g_free (escaped_basename);

			string = g_strdup_printf (_("Analysing \"%s\""), name);
			g_free (name);
		
			rejilla_job_set_current_action (REJILLA_JOB (transcode),
							REJILLA_BURN_ACTION_ANALYSING,
							string,
							TRUE);
			g_free (string);

			rejilla_job_start_progress (REJILLA_JOB (transcode), FALSE);
			g_free (uri);
			return TRUE;
		}

		if (rejilla_transcode_is_mp3 (transcode)) {
			GError *error = NULL;

			/* Rebuild another pipeline which is specific to MP3s. */
			priv->mp3_size_pipeline = TRUE;
			rejilla_transcode_stop_pipeline (transcode);

			if (!rejilla_transcode_create_pipeline (transcode, &error))
				rejilla_job_error (REJILLA_JOB (transcode), error);
		}
		else
			rejilla_transcode_song_end_reached (transcode);

		g_free (uri);
		return FALSE;
	}
	else {
		gchar *escaped_basename;

		escaped_basename = g_path_get_basename (uri);
		name = g_uri_unescape_string (escaped_basename, NULL);
		g_free (escaped_basename);

		string = g_strdup_printf (_("Transcoding \"%s\""), name);
		g_free (name);

		rejilla_job_set_current_action (REJILLA_JOB (transcode),
						REJILLA_BURN_ACTION_TRANSCODING,
						string,
						TRUE);
		g_free (string);
		rejilla_job_start_progress (REJILLA_JOB (transcode), FALSE);

		if (rejilla_job_get_fd_out (REJILLA_JOB (transcode), NULL) != REJILLA_BURN_OK) {
			gchar *dest = NULL;

			rejilla_job_get_audio_output (REJILLA_JOB (transcode), &dest);
			REJILLA_JOB_LOG (transcode,
					 "start decoding %s to %s",
					 uri,
					 dest);
			g_free (dest);
		}
		else
			REJILLA_JOB_LOG (transcode,
					 "start piping %s",
					 uri)
	}

	g_free (uri);
	return TRUE;
}

static gboolean
rejilla_transcode_bus_messages (GstBus *bus,
				GstMessage *msg,
				RejillaTranscode *transcode)
{
	RejillaTranscodePrivate *priv;
	GstTagList *tags = NULL;
	GError *error = NULL;
	GstState state;
	gchar *debug;

	priv = REJILLA_TRANSCODE_PRIVATE (transcode);
	switch (GST_MESSAGE_TYPE (msg)) {
	case GST_MESSAGE_TAG:
		/* we use the information to write an .inf file 
		 * for the time being just store the information */
		gst_message_parse_tag (msg, &tags);
		gst_tag_list_foreach (tags, (GstTagForeachFunc) foreach_tag, transcode);
		gst_tag_list_free (tags);
		return TRUE;

	case GST_MESSAGE_ERROR:
		gst_message_parse_error (msg, &error, &debug);
		REJILLA_JOB_LOG (transcode, debug);
		g_free (debug);

	        rejilla_job_error (REJILLA_JOB (transcode), error);
		return FALSE;

	case GST_MESSAGE_EOS:
		rejilla_transcode_song_end_reached (transcode);
		return FALSE;

	case GST_MESSAGE_STATE_CHANGED: {
		GstStateChangeReturn result;

		result = gst_element_get_state (priv->pipeline,
						&state,
						NULL,
						1);

		if (result != GST_STATE_CHANGE_SUCCESS)
			return TRUE;

		if (state == GST_STATE_PLAYING)
			return rejilla_transcode_active_state (transcode);

		break;
	}

	default:
		return TRUE;
	}

	return TRUE;
}

static void
rejilla_transcode_new_decoded_pad_cb (GstElement *decode,
				      GstPad *pad,
				      gboolean arg2,
				      RejillaTranscode *transcode)
{
	GstCaps *caps;
	GstStructure *structure;
	RejillaTranscodePrivate *priv;

	priv = REJILLA_TRANSCODE_PRIVATE (transcode);

	REJILLA_JOB_LOG (transcode, "New pad");

	/* make sure we only have audio */
	caps = gst_pad_get_caps (pad);
	if (!caps)
		return;

	structure = gst_caps_get_structure (caps, 0);
	if (structure) {
		if (g_strrstr (gst_structure_get_name (structure), "audio")) {
			GstPad *sink;
			GstElement *queue;
			GstPadLinkReturn res;

			/* before linking pads (before any data reach grvolume), send tags */
			rejilla_transcode_send_volume_event (transcode);

			/* This is necessary in case there is a video stream
			 * (see rejilla-metadata.c). we need to queue to avoid
			 * a deadlock. */
			queue = gst_element_factory_make ("queue", NULL);
			gst_bin_add (GST_BIN (priv->pipeline), queue);
			if (!gst_element_link (queue, priv->link)) {
				rejilla_transcode_error_on_pad_linking (transcode, "Sent by rejilla_transcode_new_decoded_pad_cb");
				goto end;
			}

			sink = gst_element_get_pad (queue, "sink");
			if (GST_PAD_IS_LINKED (sink)) {
				rejilla_transcode_error_on_pad_linking (transcode, "Sent by rejilla_transcode_new_decoded_pad_cb");
				goto end;
			}

			res = gst_pad_link (pad, sink);
			if (res == GST_PAD_LINK_OK)
				gst_element_set_state (queue, GST_STATE_PLAYING);
			else
				rejilla_transcode_error_on_pad_linking (transcode, "Sent by rejilla_transcode_new_decoded_pad_cb");

			gst_object_unref (sink);
		}
		/* For video streams add a fakesink (see rejilla-metadata.c) */
		else if (g_strrstr (gst_structure_get_name (structure), "video")) {
			GstPad *sink;
			GstElement *fakesink;
			GstPadLinkReturn res;

			REJILLA_JOB_LOG (transcode, "Adding a fakesink for video stream");

			fakesink = gst_element_factory_make ("fakesink", NULL);
			if (!fakesink) {
				rejilla_transcode_error_on_pad_linking (transcode, "Sent by rejilla_transcode_new_decoded_pad_cb");
				goto end;
			}

			sink = gst_element_get_static_pad (fakesink, "sink");
			if (!sink) {
				rejilla_transcode_error_on_pad_linking (transcode, "Sent by rejilla_transcode_new_decoded_pad_cb");
				gst_object_unref (fakesink);
				goto end;
			}

			gst_bin_add (GST_BIN (priv->pipeline), fakesink);
			res = gst_pad_link (pad, sink);

			if (res == GST_PAD_LINK_OK)
				gst_element_set_state (fakesink, GST_STATE_PLAYING);
			else
				rejilla_transcode_error_on_pad_linking (transcode, "Sent by rejilla_transcode_new_decoded_pad_cb");

			gst_object_unref (sink);
		}
	}

end:
	gst_caps_unref (caps);
}

static RejillaBurnResult
rejilla_transcode_clock_tick (RejillaJob *job)
{
	RejillaTranscodePrivate *priv;

	priv = REJILLA_TRANSCODE_PRIVATE (job);

	if (!priv->pipeline)
		return REJILLA_BURN_ERR;

	rejilla_job_set_written_track (job, priv->pos);
	return REJILLA_BURN_OK;
}

static void
rejilla_transcode_class_init (RejillaTranscodeClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	RejillaJobClass *job_class = REJILLA_JOB_CLASS (klass);

	g_type_class_add_private (klass, sizeof (RejillaTranscodePrivate));

	parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = rejilla_transcode_finalize;

	job_class->start = rejilla_transcode_start;
	job_class->clock_tick = rejilla_transcode_clock_tick;
	job_class->stop = rejilla_transcode_stop;
}

static void
rejilla_transcode_init (RejillaTranscode *obj)
{ }

static void
rejilla_transcode_finalize (GObject *object)
{
	RejillaTranscodePrivate *priv;

	priv = REJILLA_TRANSCODE_PRIVATE (object);

	if (priv->pad_id) {
		g_source_remove (priv->pad_id);
		priv->pad_id = 0;
	}

	rejilla_transcode_stop_pipeline (REJILLA_TRANSCODE (object));

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
rejilla_transcode_export_caps (RejillaPlugin *plugin)
{
	GSList *input;
	GSList *output;

	rejilla_plugin_define (plugin,
			       "transcode",
	                       NULL,
			       _("Converts any song file into a format suitable for audio CDs"),
			       "Philippe Rouquier",
			       1);

	output = rejilla_caps_audio_new (REJILLA_PLUGIN_IO_ACCEPT_FILE|
					 REJILLA_PLUGIN_IO_ACCEPT_PIPE,
					 REJILLA_AUDIO_FORMAT_RAW|
					 REJILLA_AUDIO_FORMAT_RAW_LITTLE_ENDIAN|
					 REJILLA_METADATA_INFO);

	input = rejilla_caps_audio_new (REJILLA_PLUGIN_IO_ACCEPT_FILE,
					REJILLA_AUDIO_FORMAT_UNDEFINED|
					REJILLA_METADATA_INFO);

	rejilla_plugin_link_caps (plugin, output, input);
	g_slist_free (input);

	input = rejilla_caps_audio_new (REJILLA_PLUGIN_IO_ACCEPT_FILE,
					REJILLA_AUDIO_FORMAT_DTS|
					REJILLA_METADATA_INFO);
	rejilla_plugin_link_caps (plugin, output, input);
	g_slist_free (output);
	g_slist_free (input);

	output = rejilla_caps_audio_new (REJILLA_PLUGIN_IO_ACCEPT_FILE|
					 REJILLA_PLUGIN_IO_ACCEPT_PIPE,
					 REJILLA_AUDIO_FORMAT_RAW|
					 REJILLA_AUDIO_FORMAT_RAW_LITTLE_ENDIAN);

	input = rejilla_caps_audio_new (REJILLA_PLUGIN_IO_ACCEPT_FILE,
					REJILLA_AUDIO_FORMAT_UNDEFINED);

	rejilla_plugin_link_caps (plugin, output, input);
	g_slist_free (input);

	input = rejilla_caps_audio_new (REJILLA_PLUGIN_IO_ACCEPT_FILE,
					REJILLA_AUDIO_FORMAT_DTS);

	rejilla_plugin_link_caps (plugin, output, input);
	g_slist_free (output);
	g_slist_free (input);
}
