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
#include <gio/gio.h>

#ifndef _BURN_DRIVE_H_
#define _BURN_DRIVE_H_

#include <rejilla-medium.h>

G_BEGIN_DECLS

typedef enum {
	REJILLA_DRIVE_CAPS_NONE			= 0,
	REJILLA_DRIVE_CAPS_CDR			= 1,
	REJILLA_DRIVE_CAPS_CDRW			= 1 << 1,
	REJILLA_DRIVE_CAPS_DVDR			= 1 << 2,
	REJILLA_DRIVE_CAPS_DVDRW		= 1 << 3,
	REJILLA_DRIVE_CAPS_DVDR_PLUS		= 1 << 4,
	REJILLA_DRIVE_CAPS_DVDRW_PLUS		= 1 << 5,
	REJILLA_DRIVE_CAPS_DVDR_PLUS_DL		= 1 << 6,
	REJILLA_DRIVE_CAPS_DVDRW_PLUS_DL	= 1 << 7,
	REJILLA_DRIVE_CAPS_DVDRAM		= 1 << 10,
	REJILLA_DRIVE_CAPS_BDR			= 1 << 8,
	REJILLA_DRIVE_CAPS_BDRW			= 1 << 9
} RejillaDriveCaps;

#define REJILLA_TYPE_DRIVE             (rejilla_drive_get_type ())
#define REJILLA_DRIVE(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_DRIVE, RejillaDrive))
#define REJILLA_DRIVE_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_DRIVE, RejillaDriveClass))
#define REJILLA_IS_DRIVE(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_DRIVE))
#define REJILLA_IS_DRIVE_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_DRIVE))
#define REJILLA_DRIVE_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_DRIVE, RejillaDriveClass))

typedef struct _RejillaDriveClass RejillaDriveClass;

struct _RejillaDriveClass
{
	GObjectClass parent_class;

	/* Signals */
	void		(* medium_added)	(RejillaDrive *drive,
						 RejillaMedium *medium);

	void		(* medium_removed)	(RejillaDrive *drive,
						 RejillaMedium *medium);
};

struct _RejillaDrive
{
	GObject parent_instance;
};

GType rejilla_drive_get_type (void) G_GNUC_CONST;

void
rejilla_drive_reprobe (RejillaDrive *drive);

RejillaMedium *
rejilla_drive_get_medium (RejillaDrive *drive);

GDrive *
rejilla_drive_get_gdrive (RejillaDrive *drive);

const gchar *
rejilla_drive_get_udi (RejillaDrive *drive);

gboolean
rejilla_drive_is_fake (RejillaDrive *drive);

gchar *
rejilla_drive_get_display_name (RejillaDrive *drive);

const gchar *
rejilla_drive_get_device (RejillaDrive *drive);

const gchar *
rejilla_drive_get_block_device (RejillaDrive *drive);

gchar *
rejilla_drive_get_bus_target_lun_string (RejillaDrive *drive);

RejillaDriveCaps
rejilla_drive_get_caps (RejillaDrive *drive);

gboolean
rejilla_drive_can_write_media (RejillaDrive *drive,
                               RejillaMedia media);

gboolean
rejilla_drive_can_write (RejillaDrive *drive);

gboolean
rejilla_drive_can_eject (RejillaDrive *drive);

gboolean
rejilla_drive_eject (RejillaDrive *drive,
		     gboolean wait,
		     GError **error);

void
rejilla_drive_cancel_current_operation (RejillaDrive *drive);

gboolean
rejilla_drive_is_door_open (RejillaDrive *drive);

gboolean
rejilla_drive_can_use_exclusively (RejillaDrive *drive);

gboolean
rejilla_drive_lock (RejillaDrive *drive,
		    const gchar *reason,
		    gchar **reason_for_failure);
gboolean
rejilla_drive_unlock (RejillaDrive *drive);

gboolean
rejilla_drive_is_locked (RejillaDrive *drive,
                         gchar **reason);

G_END_DECLS

#endif /* _BURN_DRIVE_H_ */
