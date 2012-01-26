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

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>

#include "burn-debug.h"
#include "burn-basics.h"
#include "rejilla-track-stream.h"

typedef struct _RejillaTrackStreamPrivate RejillaTrackStreamPrivate;
struct _RejillaTrackStreamPrivate
{
        GFileMonitor *monitor;
	gchar *uri;

	RejillaStreamFormat format;

	guint64 gap;
	guint64 start;
	guint64 end;
};

#define REJILLA_TRACK_STREAM_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REJILLA_TYPE_TRACK_STREAM, RejillaTrackStreamPrivate))

G_DEFINE_TYPE (RejillaTrackStream, rejilla_track_stream, REJILLA_TYPE_TRACK);

static RejillaBurnResult
rejilla_track_stream_set_source_real (RejillaTrackStream *track,
				      const gchar *uri)
{
	RejillaTrackStreamPrivate *priv;

	priv = REJILLA_TRACK_STREAM_PRIVATE (track);

	if (priv->uri)
		g_free (priv->uri);

	priv->uri = g_strdup (uri);

	/* Since that's a new URI chances are, the end point is different */
	priv->end = 0;

	return REJILLA_BURN_OK;
}

/**
 * rejilla_track_stream_set_source:
 * @track: a #RejillaTrackStream
 * @uri: a #gchar
 *
 * Sets the stream (song or video) uri.
 *
 * Note: it resets the end point of the track to 0 but keeps start point and gap
 * unchanged.
 *
 * Return value: a #RejillaBurnResult. REJILLA_BURN_OK if it is successful.
 **/

RejillaBurnResult
rejilla_track_stream_set_source (RejillaTrackStream *track,
				 const gchar *uri)
{
	RejillaTrackStreamClass *klass;
	RejillaBurnResult res;

	g_return_val_if_fail (REJILLA_IS_TRACK_STREAM (track), REJILLA_BURN_ERR);

	klass = REJILLA_TRACK_STREAM_GET_CLASS (track);
	if (!klass->set_source)
		return REJILLA_BURN_ERR;

	res = klass->set_source (track, uri);
	if (res != REJILLA_BURN_OK)
		return res;

	rejilla_track_changed (REJILLA_TRACK (track));
	return REJILLA_BURN_OK;
}

/**
 * rejilla_track_stream_get_format:
 * @track: a #RejillaTrackStream
 *
 * This function returns the format of the stream.
 *
 * Return value: a #RejillaStreamFormat.
 **/

RejillaStreamFormat
rejilla_track_stream_get_format (RejillaTrackStream *track)
{
	RejillaTrackStreamPrivate *priv;

	g_return_val_if_fail (REJILLA_IS_TRACK_STREAM (track), REJILLA_AUDIO_FORMAT_NONE);

	priv = REJILLA_TRACK_STREAM_PRIVATE (track);

	return priv->format;
}

static RejillaBurnResult
rejilla_track_stream_set_format_real (RejillaTrackStream *track,
				      RejillaStreamFormat format)
{
	RejillaTrackStreamPrivate *priv;

	priv = REJILLA_TRACK_STREAM_PRIVATE (track);

	if (format == REJILLA_AUDIO_FORMAT_NONE)
		REJILLA_BURN_LOG ("Setting a NONE audio format with a valid uri");

	priv->format = format;
	return REJILLA_BURN_OK;
}

/**
 * rejilla_track_stream_set_format:
 * @track: a #RejillaTrackStream
 * @format: a #RejillaStreamFormat
 *
 * Sets the format of the stream.
 *
 * Return value: a #RejillaBurnResult. REJILLA_BURN_OK if it is successful.
 **/

RejillaBurnResult
rejilla_track_stream_set_format (RejillaTrackStream *track,
				 RejillaStreamFormat format)
{
	RejillaTrackStreamClass *klass;
	RejillaBurnResult res;

	g_return_val_if_fail (REJILLA_IS_TRACK_STREAM (track), REJILLA_BURN_ERR);

	klass = REJILLA_TRACK_STREAM_GET_CLASS (track);
	if (!klass->set_format)
		return REJILLA_BURN_ERR;

	res = klass->set_format (track, format);
	if (res != REJILLA_BURN_OK)
		return res;

	rejilla_track_changed (REJILLA_TRACK (track));
	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_track_stream_set_boundaries_real (RejillaTrackStream *track,
					  gint64 start,
					  gint64 end,
					  gint64 gap)
{
	RejillaTrackStreamPrivate *priv;

	priv = REJILLA_TRACK_STREAM_PRIVATE (track);

	if (gap >= 0)
		priv->gap = gap;

	if (end > 0)
		priv->end = end;

	if (start >= 0)
		priv->start = start;

	return REJILLA_BURN_OK;
}

/**
 * rejilla_track_stream_set_boundaries:
 * @track: a #RejillaTrackStream
 * @start: a #gint64 or -1 to ignore
 * @end: a #gint64 or -1 to ignore
 * @gap: a #gint64 or -1 to ignore
 *
 * Sets the boundaries of the stream (where it starts, ends in the file;
 * how long is the gap with the next track) in nano seconds.
 *
 * Return value: a #RejillaBurnResult. REJILLA_BURN_OK if it is successful.
 **/

RejillaBurnResult
rejilla_track_stream_set_boundaries (RejillaTrackStream *track,
				     gint64 start,
				     gint64 end,
				     gint64 gap)
{
	RejillaTrackStreamClass *klass;
	RejillaBurnResult res;

	g_return_val_if_fail (REJILLA_IS_TRACK_STREAM (track), REJILLA_BURN_ERR);

	klass = REJILLA_TRACK_STREAM_GET_CLASS (track);
	if (!klass->set_boundaries)
		return REJILLA_BURN_ERR;

	res = klass->set_boundaries (track, start, end, gap);
	if (res != REJILLA_BURN_OK)
		return res;

	rejilla_track_changed (REJILLA_TRACK (track));
	return REJILLA_BURN_OK;
}

/**
 * rejilla_track_stream_get_source:
 * @track: a #RejillaTrackStream
 * @uri: a #gboolean
 *
 * This function returns the path or the URI (if @uri is TRUE)
 * of the stream (song or video file).
 *
 * Return value: a #gchar.
 **/

gchar *
rejilla_track_stream_get_source (RejillaTrackStream *track,
				 gboolean uri)
{
	RejillaTrackStreamPrivate *priv;

	g_return_val_if_fail (REJILLA_IS_TRACK_STREAM (track), NULL);

	priv = REJILLA_TRACK_STREAM_PRIVATE (track);
	if (uri)
		return rejilla_string_get_uri (priv->uri);
	else
		return rejilla_string_get_localpath (priv->uri);
}

/**
 * rejilla_track_stream_get_gap:
 * @track: a #RejillaTrackStream
 *
 * This function returns length of the gap (in nano seconds).
 *
 * Return value: a #guint64.
 **/

guint64
rejilla_track_stream_get_gap (RejillaTrackStream *track)
{
	RejillaTrackStreamPrivate *priv;

	g_return_val_if_fail (REJILLA_IS_TRACK_STREAM (track), 0);

	priv = REJILLA_TRACK_STREAM_PRIVATE (track);
	return priv->gap;
}

/**
 * rejilla_track_stream_get_start:
 * @track: a #RejillaTrackStream
 *
 * This function returns start time in the stream (in nano seconds).
 *
 * Return value: a #guint64.
 **/

guint64
rejilla_track_stream_get_start (RejillaTrackStream *track)
{
	RejillaTrackStreamPrivate *priv;

	g_return_val_if_fail (REJILLA_IS_TRACK_STREAM (track), 0);

	priv = REJILLA_TRACK_STREAM_PRIVATE (track);
	return priv->start;
}

/**
 * rejilla_track_stream_get_end:
 * @track: a #RejillaTrackStream
 *
 * This function returns end time in the stream (in nano seconds).
 *
 * Return value: a #guint64.
 **/

guint64
rejilla_track_stream_get_end (RejillaTrackStream *track)
{
	RejillaTrackStreamPrivate *priv;

	g_return_val_if_fail (REJILLA_IS_TRACK_STREAM (track), 0);

	priv = REJILLA_TRACK_STREAM_PRIVATE (track);
	return priv->end;
}

/**
 * rejilla_track_stream_get_length:
 * @track: a #RejillaTrackStream
 * @length: a #guint64
 *
 * This function returns the length of the stream (in nano seconds)
 * taking into account the start and end time as well as the length
 * of the gap. It stores it in @length.
 *
 * Return value: a #RejillaBurnResult. REJILLA_BURN_OK if @length was set.
 **/

RejillaBurnResult
rejilla_track_stream_get_length (RejillaTrackStream *track,
				 guint64 *length)
{
	RejillaTrackStreamPrivate *priv;

	g_return_val_if_fail (REJILLA_IS_TRACK_STREAM (track), REJILLA_BURN_ERR);

	priv = REJILLA_TRACK_STREAM_PRIVATE (track);

	if (priv->start < 0 || priv->end <= 0)
		return REJILLA_BURN_ERR;

	*length = REJILLA_STREAM_LENGTH (priv->start, priv->end + priv->gap);

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_track_stream_get_size (RejillaTrack *track,
			       goffset *blocks,
			       goffset *block_size)
{
	RejillaTrackStreamPrivate *priv;
	RejillaStreamFormat format;

	priv = REJILLA_TRACK_STREAM_PRIVATE (track);

	format = rejilla_track_stream_get_format (REJILLA_TRACK_STREAM (track));
	if (!REJILLA_STREAM_FORMAT_HAS_VIDEO (format)) {
		if (blocks) {
			guint64 length = 0;

			rejilla_track_stream_get_length (REJILLA_TRACK_STREAM (track), &length);
			*blocks = length * 75LL / 1000000000LL;
		}

		if (block_size)
			*block_size = 2352;
	}
	else {
		if (blocks) {
			guint64 length = 0;

			/* This is based on a simple formula:
			 * 4700000000 bytes means 2 hours */
			rejilla_track_stream_get_length (REJILLA_TRACK_STREAM (track), &length);
			*blocks = length * 47LL / 72000LL / 2048LL;
		}

		if (block_size)
			*block_size = 2048;
	}

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_track_stream_get_status (RejillaTrack *track,
				 RejillaStatus *status)
{
	RejillaTrackStreamPrivate *priv;

	priv = REJILLA_TRACK_STREAM_PRIVATE (track);
	if (!priv->uri) {
		if (status)
			rejilla_status_set_error (status,
						  g_error_new (REJILLA_BURN_ERROR,
							       REJILLA_BURN_ERROR_EMPTY,
							       _("There are no files to write to disc")));

		return REJILLA_BURN_ERR;
	}

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_track_stream_get_track_type (RejillaTrack *track,
				     RejillaTrackType *type)
{
	RejillaTrackStreamPrivate *priv;

	priv = REJILLA_TRACK_STREAM_PRIVATE (track);

	rejilla_track_type_set_has_stream (type);
	rejilla_track_type_set_stream_format (type, priv->format);

	return REJILLA_BURN_OK;
}

static void
rejilla_track_stream_init (RejillaTrackStream *object)
{ }

static void
rejilla_track_stream_finalize (GObject *object)
{
	RejillaTrackStreamPrivate *priv;

	priv = REJILLA_TRACK_STREAM_PRIVATE (object);
	if (priv->uri) {
		g_free (priv->uri);
		priv->uri = NULL;
	}

	G_OBJECT_CLASS (rejilla_track_stream_parent_class)->finalize (object);
}

static void
rejilla_track_stream_class_init (RejillaTrackStreamClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	RejillaTrackClass *track_class = REJILLA_TRACK_CLASS (klass);

	g_type_class_add_private (klass, sizeof (RejillaTrackStreamPrivate));

	object_class->finalize = rejilla_track_stream_finalize;

	track_class->get_size = rejilla_track_stream_get_size;
	track_class->get_status = rejilla_track_stream_get_status;
	track_class->get_type = rejilla_track_stream_get_track_type;

	klass->set_source = rejilla_track_stream_set_source_real;
	klass->set_format = rejilla_track_stream_set_format_real;
	klass->set_boundaries = rejilla_track_stream_set_boundaries_real;
}

/**
 * rejilla_track_stream_new:
 *
 *  Creates a new #RejillaTrackStream object.
 *
 * This type of tracks is used to burn audio or
 * video files.
 *
 * Return value: a #RejillaTrackStream object.
 **/

RejillaTrackStream *
rejilla_track_stream_new (void)
{
	return g_object_new (REJILLA_TYPE_TRACK_STREAM, NULL);
}
