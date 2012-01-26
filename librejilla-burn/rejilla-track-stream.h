/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Librejilla-media
 * Copyright (C) Philippe Rouquier 2005-2009 <bonfire-app@wanadoo.fr>
 *
 * Librejilla-media is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * The Librejilla-media authors hereby grant permission for non-GPL compatible
 * GStreamer plugins to be used and distributed together with GStreamer
 * and Librejilla-media. This permission is above and beyond the permissions granted
 * by the GPL license by which Librejilla-media is covered. If you modify this code
 * you may extend this exception to your version of the code, but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version.
 * 
 * Librejilla-media is distributed in the hope that it will be useful,
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

#ifndef _REJILLA_TRACK_STREAM_H_
#define _REJILLA_TRACK_STREAM_H_

#include <glib-object.h>

#include <rejilla-enums.h>
#include <rejilla-track.h>

G_BEGIN_DECLS

#define REJILLA_TYPE_TRACK_STREAM             (rejilla_track_stream_get_type ())
#define REJILLA_TRACK_STREAM(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_TRACK_STREAM, RejillaTrackStream))
#define REJILLA_TRACK_STREAM_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_TRACK_STREAM, RejillaTrackStreamClass))
#define REJILLA_IS_TRACK_STREAM(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_TRACK_STREAM))
#define REJILLA_IS_TRACK_STREAM_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_TRACK_STREAM))
#define REJILLA_TRACK_STREAM_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_TRACK_STREAM, RejillaTrackStreamClass))

typedef struct _RejillaTrackStreamClass RejillaTrackStreamClass;
typedef struct _RejillaTrackStream RejillaTrackStream;

struct _RejillaTrackStreamClass
{
	RejillaTrackClass parent_class;

	/* Virtual functions */
	RejillaBurnResult       (*set_source)		(RejillaTrackStream *track,
							 const gchar *uri);

	RejillaBurnResult       (*set_format)		(RejillaTrackStream *track,
							 RejillaStreamFormat format);

	RejillaBurnResult       (*set_boundaries)       (RejillaTrackStream *track,
							 gint64 start,
							 gint64 end,
							 gint64 gap);
};

struct _RejillaTrackStream
{
	RejillaTrack parent_instance;
};

GType rejilla_track_stream_get_type (void) G_GNUC_CONST;

RejillaTrackStream *
rejilla_track_stream_new (void);

RejillaBurnResult
rejilla_track_stream_set_source (RejillaTrackStream *track,
				 const gchar *uri);

RejillaBurnResult
rejilla_track_stream_set_format (RejillaTrackStream *track,
				 RejillaStreamFormat format);

RejillaBurnResult
rejilla_track_stream_set_boundaries (RejillaTrackStream *track,
				     gint64 start,
				     gint64 end,
				     gint64 gap);

gchar *
rejilla_track_stream_get_source (RejillaTrackStream *track,
				 gboolean uri);

RejillaBurnResult
rejilla_track_stream_get_length (RejillaTrackStream *track,
				 guint64 *length);

guint64
rejilla_track_stream_get_start (RejillaTrackStream *track);

guint64
rejilla_track_stream_get_end (RejillaTrackStream *track);

guint64
rejilla_track_stream_get_gap (RejillaTrackStream *track);

RejillaStreamFormat
rejilla_track_stream_get_format (RejillaTrackStream *track);

G_END_DECLS

#endif /* _REJILLA_TRACK_STREAM_H_ */
