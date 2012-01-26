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

#ifndef _BURN_TRACK_IMAGE_CFG_H_
#define _BURN_TRACK_IMAGE_CFG_H_

#include <glib-object.h>

#include <rejilla-track.h>
#include <rejilla-track-image.h>

G_BEGIN_DECLS

#define REJILLA_TYPE_TRACK_IMAGE_CFG             (rejilla_track_image_cfg_get_type ())
#define REJILLA_TRACK_IMAGE_CFG(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_TRACK_IMAGE_CFG, RejillaTrackImageCfg))
#define REJILLA_TRACK_IMAGE_CFG_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_TRACK_IMAGE_CFG, RejillaTrackImageCfgClass))
#define REJILLA_IS_TRACK_IMAGE_CFG(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_TRACK_IMAGE_CFG))
#define REJILLA_IS_TRACK_IMAGE_CFG_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_TRACK_IMAGE_CFG))
#define REJILLA_TRACK_IMAGE_CFG_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_TRACK_IMAGE_CFG, RejillaTrackImageCfgClass))

typedef struct _RejillaTrackImageCfgClass RejillaTrackImageCfgClass;
typedef struct _RejillaTrackImageCfg RejillaTrackImageCfg;

struct _RejillaTrackImageCfgClass
{
	RejillaTrackImageClass parent_class;
};

struct _RejillaTrackImageCfg
{
	RejillaTrackImage parent_instance;
};

GType rejilla_track_image_cfg_get_type (void) G_GNUC_CONST;

RejillaTrackImageCfg *
rejilla_track_image_cfg_new (void);

RejillaBurnResult
rejilla_track_image_cfg_set_source (RejillaTrackImageCfg *track,
				    const gchar *uri);

RejillaBurnResult
rejilla_track_image_cfg_force_format (RejillaTrackImageCfg *track,
				      RejillaImageFormat format);

RejillaImageFormat
rejilla_track_image_cfg_get_forced_format (RejillaTrackImageCfg *track);

G_END_DECLS

#endif /* _BURN_TRACK_IMAGE_CFG_H_ */
