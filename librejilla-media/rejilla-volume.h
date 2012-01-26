/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Rejilla
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

#ifndef _REJILLA_VOLUME_H_
#define _REJILLA_VOLUME_H_

#include <glib-object.h>
#include <gio/gio.h>
#include <gtk/gtk.h>
#include <rejilla-drive.h>

G_BEGIN_DECLS

#define REJILLA_TYPE_VOLUME             (rejilla_volume_get_type ())
#define REJILLA_VOLUME(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_VOLUME, RejillaVolume))
#define REJILLA_VOLUME_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_VOLUME, RejillaVolumeClass))
#define REJILLA_IS_VOLUME(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_VOLUME))
#define REJILLA_IS_VOLUME_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_VOLUME))
#define REJILLA_VOLUME_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_VOLUME, RejillaVolumeClass))

typedef struct _RejillaVolumeClass RejillaVolumeClass;
typedef struct _RejillaVolume RejillaVolume;

struct _RejillaVolumeClass
{
	RejillaMediumClass parent_class;
};

struct _RejillaVolume
{
	RejillaMedium parent_instance;
};

GType rejilla_volume_get_type (void) G_GNUC_CONST;

gchar *
rejilla_volume_get_name (RejillaVolume *volume);

GIcon *
rejilla_volume_get_icon (RejillaVolume *volume);

GVolume *
rejilla_volume_get_gvolume (RejillaVolume *volume);

gboolean
rejilla_volume_is_mounted (RejillaVolume *volume);

gchar *
rejilla_volume_get_mount_point (RejillaVolume *volume,
				GError **error);

gboolean
rejilla_volume_umount (RejillaVolume *volume,
		       gboolean wait,
		       GError **error);

gboolean
rejilla_volume_mount (RejillaVolume *volume,
		      GtkWindow *parent_window,
		      gboolean wait,
		      GError **error);

void
rejilla_volume_cancel_current_operation (RejillaVolume *volume);

G_END_DECLS

#endif /* _REJILLA_VOLUME_H_ */
