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

#ifndef _REJILLA_TRACK_DISC_H_
#define _REJILLA_TRACK_DISC_H_

#include <glib-object.h>

#include <rejilla-drive.h>

#include <rejilla-track.h>

G_BEGIN_DECLS

#define REJILLA_TYPE_TRACK_DISC             (rejilla_track_disc_get_type ())
#define REJILLA_TRACK_DISC(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_TRACK_DISC, RejillaTrackDisc))
#define REJILLA_TRACK_DISC_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_TRACK_DISC, RejillaTrackDiscClass))
#define REJILLA_IS_TRACK_DISC(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_TRACK_DISC))
#define REJILLA_IS_TRACK_DISC_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_TRACK_DISC))
#define REJILLA_TRACK_DISC_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_TRACK_DISC, RejillaTrackDiscClass))

typedef struct _RejillaTrackDiscClass RejillaTrackDiscClass;
typedef struct _RejillaTrackDisc RejillaTrackDisc;

struct _RejillaTrackDiscClass
{
	RejillaTrackClass parent_class;
};

struct _RejillaTrackDisc
{
	RejillaTrack parent_instance;
};

GType rejilla_track_disc_get_type (void) G_GNUC_CONST;

RejillaTrackDisc *
rejilla_track_disc_new (void);

RejillaBurnResult
rejilla_track_disc_set_drive (RejillaTrackDisc *track,
			      RejillaDrive *drive);

RejillaDrive *
rejilla_track_disc_get_drive (RejillaTrackDisc *track);

RejillaBurnResult
rejilla_track_disc_set_track_num (RejillaTrackDisc *track,
				  guint num);

guint
rejilla_track_disc_get_track_num (RejillaTrackDisc *track);

RejillaMedia
rejilla_track_disc_get_medium_type (RejillaTrackDisc *track);

G_END_DECLS

#endif /* _REJILLA_TRACK_DISC_H_ */
