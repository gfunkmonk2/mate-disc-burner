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

#ifndef _REJILLA_MEDIUM_MONITOR_H_
#define _REJILLA_MEDIUM_MONITOR_H_

#include <glib-object.h>

#include <rejilla-medium.h>
#include <rejilla-drive.h>

G_BEGIN_DECLS

#define REJILLA_TYPE_MEDIUM_MONITOR             (rejilla_medium_monitor_get_type ())
#define REJILLA_MEDIUM_MONITOR(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_MEDIUM_MONITOR, RejillaMediumMonitor))
#define REJILLA_MEDIUM_MONITOR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_MEDIUM_MONITOR, RejillaMediumMonitorClass))
#define REJILLA_IS_MEDIUM_MONITOR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_MEDIUM_MONITOR))
#define REJILLA_IS_MEDIUM_MONITOR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_MEDIUM_MONITOR))
#define REJILLA_MEDIUM_MONITOR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_MEDIUM_MONITOR, RejillaMediumMonitorClass))

typedef struct _RejillaMediumMonitorClass RejillaMediumMonitorClass;
typedef struct _RejillaMediumMonitor RejillaMediumMonitor;


struct _RejillaMediumMonitorClass
{
	GObjectClass parent_class;

	/* Signals */
	void		(* drive_added)		(RejillaMediumMonitor *monitor,
						 RejillaDrive *drive);

	void		(* drive_removed)	(RejillaMediumMonitor *monitor,
						 RejillaDrive *drive);

	void		(* medium_added)	(RejillaMediumMonitor *monitor,
						 RejillaMedium *medium);

	void		(* medium_removed)	(RejillaMediumMonitor *monitor,
						 RejillaMedium *medium);
};

struct _RejillaMediumMonitor
{
	GObject parent_instance;
};

GType rejilla_medium_monitor_get_type (void) G_GNUC_CONST;

RejillaMediumMonitor *
rejilla_medium_monitor_get_default (void);

typedef enum {
	REJILLA_MEDIA_TYPE_NONE				= 0,
	REJILLA_MEDIA_TYPE_FILE				= 1,
	REJILLA_MEDIA_TYPE_DATA				= 1 << 1,
	REJILLA_MEDIA_TYPE_AUDIO			= 1 << 2,
	REJILLA_MEDIA_TYPE_WRITABLE			= 1 << 3,
	REJILLA_MEDIA_TYPE_REWRITABLE			= 1 << 4,
	REJILLA_MEDIA_TYPE_ANY_IN_BURNER		= 1 << 5,

	/* If combined with other flags it will filter.
	 * if alone all CDs are returned.
	 * It can't be combined with FILE type. */
	REJILLA_MEDIA_TYPE_CD					= 1 << 6,

	REJILLA_MEDIA_TYPE_ALL_BUT_FILE			= 0xFE,
	REJILLA_MEDIA_TYPE_ALL				= 0xFF
} RejillaMediaType;

typedef enum {
	REJILLA_DRIVE_TYPE_NONE				= 0,
	REJILLA_DRIVE_TYPE_FILE				= 1,
	REJILLA_DRIVE_TYPE_WRITER			= 1 << 1,
	REJILLA_DRIVE_TYPE_READER			= 1 << 2,
	REJILLA_DRIVE_TYPE_ALL_BUT_FILE			= 0xFE,
	REJILLA_DRIVE_TYPE_ALL				= 0xFF
} RejillaDriveType;

GSList *
rejilla_medium_monitor_get_media (RejillaMediumMonitor *monitor,
				  RejillaMediaType type);

GSList *
rejilla_medium_monitor_get_drives (RejillaMediumMonitor *monitor,
				   RejillaDriveType type);

RejillaDrive *
rejilla_medium_monitor_get_drive (RejillaMediumMonitor *monitor,
				  const gchar *device);

gboolean
rejilla_medium_monitor_is_probing (RejillaMediumMonitor *monitor);

G_END_DECLS

#endif /* _REJILLA_MEDIUM_MONITOR_H_ */
