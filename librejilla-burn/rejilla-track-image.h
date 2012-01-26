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

#ifndef _BURN_TRACK_IMAGE_H_
#define _BURN_TRACK_IMAGE_H_

#include <glib-object.h>

#include <rejilla-enums.h>
#include <rejilla-track.h>

G_BEGIN_DECLS

#define REJILLA_TYPE_TRACK_IMAGE             (rejilla_track_image_get_type ())
#define REJILLA_TRACK_IMAGE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_TRACK_IMAGE, RejillaTrackImage))
#define REJILLA_TRACK_IMAGE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_TRACK_IMAGE, RejillaTrackImageClass))
#define REJILLA_IS_TRACK_IMAGE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_TRACK_IMAGE))
#define REJILLA_IS_TRACK_IMAGE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_TRACK_IMAGE))
#define REJILLA_TRACK_IMAGE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_TRACK_IMAGE, RejillaTrackImageClass))

typedef struct _RejillaTrackImageClass RejillaTrackImageClass;
typedef struct _RejillaTrackImage RejillaTrackImage;

struct _RejillaTrackImageClass
{
	RejillaTrackClass parent_class;

	RejillaBurnResult	(*set_source)		(RejillaTrackImage *track,
							 const gchar *image,
							 const gchar *toc,
							 RejillaImageFormat format);

	RejillaBurnResult       (*set_block_num)	(RejillaTrackImage *track,
							 goffset blocks);
};

struct _RejillaTrackImage
{
	RejillaTrack parent_instance;
};

GType rejilla_track_image_get_type (void) G_GNUC_CONST;

RejillaTrackImage *
rejilla_track_image_new (void);

RejillaBurnResult
rejilla_track_image_set_source (RejillaTrackImage *track,
				const gchar *image,
				const gchar *toc,
				RejillaImageFormat format);

RejillaBurnResult
rejilla_track_image_set_block_num (RejillaTrackImage *track,
				   goffset blocks);

gchar *
rejilla_track_image_get_source (RejillaTrackImage *track,
				gboolean uri);

gchar *
rejilla_track_image_get_toc_source (RejillaTrackImage *track,
				    gboolean uri);

RejillaImageFormat
rejilla_track_image_get_format (RejillaTrackImage *track);

gboolean
rejilla_track_image_need_byte_swap (RejillaTrackImage *track);

G_END_DECLS

#endif /* _BURN_TRACK_IMAGE_H_ */
