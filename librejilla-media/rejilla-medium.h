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

#include <glib-object.h>

#include <rejilla-media.h>

#ifndef _BURN_MEDIUM_H_
#define _BURN_MEDIUM_H_

G_BEGIN_DECLS

#define REJILLA_TYPE_MEDIUM             (rejilla_medium_get_type ())
#define REJILLA_MEDIUM(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_MEDIUM, RejillaMedium))
#define REJILLA_MEDIUM_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_MEDIUM, RejillaMediumClass))
#define REJILLA_IS_MEDIUM(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_MEDIUM))
#define REJILLA_IS_MEDIUM_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_MEDIUM))
#define REJILLA_MEDIUM_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_MEDIUM, RejillaMediumClass))

typedef struct _RejillaMediumClass RejillaMediumClass;

/**
 * RejillaMedium:
 *
 * Represents a physical medium currently inserted in a #RejillaDrive.
 **/
typedef struct _RejillaMedium RejillaMedium;

/**
 * RejillaDrive:
 *
 * Represents a physical drive currently connected.
 **/
typedef struct _RejillaDrive RejillaDrive;

struct _RejillaMediumClass
{
	GObjectClass parent_class;
};

struct _RejillaMedium
{
	GObject parent_instance;
};

GType rejilla_medium_get_type (void) G_GNUC_CONST;


RejillaMedia
rejilla_medium_get_status (RejillaMedium *medium);

guint64
rejilla_medium_get_max_write_speed (RejillaMedium *medium);

guint64 *
rejilla_medium_get_write_speeds (RejillaMedium *medium);

void
rejilla_medium_get_free_space (RejillaMedium *medium,
			       goffset *bytes,
			       goffset *blocks);

void
rejilla_medium_get_capacity (RejillaMedium *medium,
			     goffset *bytes,
			     goffset *blocks);

void
rejilla_medium_get_data_size (RejillaMedium *medium,
			      goffset *bytes,
			      goffset *blocks);

gint64
rejilla_medium_get_next_writable_address (RejillaMedium *medium);

gboolean
rejilla_medium_can_be_rewritten (RejillaMedium *medium);

gboolean
rejilla_medium_can_be_written (RejillaMedium *medium);

const gchar *
rejilla_medium_get_CD_TEXT_title (RejillaMedium *medium);

const gchar *
rejilla_medium_get_type_string (RejillaMedium *medium);

gchar *
rejilla_medium_get_tooltip (RejillaMedium *medium);

RejillaDrive *
rejilla_medium_get_drive (RejillaMedium *medium);

guint
rejilla_medium_get_track_num (RejillaMedium *medium);

gboolean
rejilla_medium_get_last_data_track_space (RejillaMedium *medium,
					  goffset *bytes,
					  goffset *sectors);

gboolean
rejilla_medium_get_last_data_track_address (RejillaMedium *medium,
					    goffset *bytes,
					    goffset *sectors);

gboolean
rejilla_medium_get_track_space (RejillaMedium *medium,
				guint num,
				goffset *bytes,
				goffset *sectors);

gboolean
rejilla_medium_get_track_address (RejillaMedium *medium,
				  guint num,
				  goffset *bytes,
				  goffset *sectors);

gboolean
rejilla_medium_can_use_dummy_for_sao (RejillaMedium *medium);

gboolean
rejilla_medium_can_use_dummy_for_tao (RejillaMedium *medium);

gboolean
rejilla_medium_can_use_burnfree (RejillaMedium *medium);

gboolean
rejilla_medium_can_use_sao (RejillaMedium *medium);

gboolean
rejilla_medium_can_use_tao (RejillaMedium *medium);

G_END_DECLS

#endif /* _BURN_MEDIUM_H_ */
