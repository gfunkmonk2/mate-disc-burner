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

#ifndef _BURN_TRACK_TYPE_H
#define _BURN_TRACK_TYPE_H

#include <glib.h>

#include <rejilla-enums.h>
#include <rejilla-media.h>

G_BEGIN_DECLS

typedef struct _RejillaTrackType RejillaTrackType;

RejillaTrackType *
rejilla_track_type_new (void);

void
rejilla_track_type_free (RejillaTrackType *type);

gboolean
rejilla_track_type_is_empty (const RejillaTrackType *type);
gboolean
rejilla_track_type_get_has_data (const RejillaTrackType *type);
gboolean
rejilla_track_type_get_has_image (const RejillaTrackType *type);
gboolean
rejilla_track_type_get_has_stream (const RejillaTrackType *type);
gboolean
rejilla_track_type_get_has_medium (const RejillaTrackType *type);

void
rejilla_track_type_set_has_data (RejillaTrackType *type);
void
rejilla_track_type_set_has_image (RejillaTrackType *type);
void
rejilla_track_type_set_has_stream (RejillaTrackType *type);
void
rejilla_track_type_set_has_medium (RejillaTrackType *type);

RejillaStreamFormat
rejilla_track_type_get_stream_format (const RejillaTrackType *type);
RejillaImageFormat
rejilla_track_type_get_image_format (const RejillaTrackType *type);
RejillaMedia
rejilla_track_type_get_medium_type (const RejillaTrackType *type);
RejillaImageFS
rejilla_track_type_get_data_fs (const RejillaTrackType *type);

void
rejilla_track_type_set_stream_format (RejillaTrackType *type,
				      RejillaStreamFormat format);
void
rejilla_track_type_set_image_format (RejillaTrackType *type,
				     RejillaImageFormat format);
void
rejilla_track_type_set_medium_type (RejillaTrackType *type,
				    RejillaMedia media);
void
rejilla_track_type_set_data_fs (RejillaTrackType *type,
				RejillaImageFS fs_type);

gboolean
rejilla_track_type_equal (const RejillaTrackType *type_A,
			  const RejillaTrackType *type_B);

G_END_DECLS

#endif
