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

#ifndef _REJILLA_FILE_MONITOR_H_
#define _REJILLA_FILE_MONITOR_H_

#include <glib-object.h>

G_BEGIN_DECLS

typedef enum {
	REJILLA_FILE_MONITOR_FILE,
	REJILLA_FILE_MONITOR_FOLDER
} RejillaFileMonitorType;

typedef	gboolean	(*RejillaMonitorFindFunc)	(gpointer data, gpointer callback_data);

#define REJILLA_TYPE_FILE_MONITOR             (rejilla_file_monitor_get_type ())
#define REJILLA_FILE_MONITOR(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_FILE_MONITOR, RejillaFileMonitor))
#define REJILLA_FILE_MONITOR_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_FILE_MONITOR, RejillaFileMonitorClass))
#define REJILLA_IS_FILE_MONITOR(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_FILE_MONITOR))
#define REJILLA_IS_FILE_MONITOR_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_FILE_MONITOR))
#define REJILLA_FILE_MONITOR_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_FILE_MONITOR, RejillaFileMonitorClass))

typedef struct _RejillaFileMonitorClass RejillaFileMonitorClass;
typedef struct _RejillaFileMonitor RejillaFileMonitor;

struct _RejillaFileMonitorClass
{
	GObjectClass parent_class;

	/* Virtual functions */

	/* if name is NULL then it's happening on the callback_data */
	void		(*file_added)		(RejillaFileMonitor *monitor,
						 gpointer callback_data,
						 const gchar *name);

	/* NOTE: there is no dest_type here as it must be a FOLDER type */
	void		(*file_moved)		(RejillaFileMonitor *self,
						 RejillaFileMonitorType src_type,
						 gpointer callback_src,
						 const gchar *name_src,
						 gpointer callback_dest,
						 const gchar *name_dest);
	void		(*file_renamed)		(RejillaFileMonitor *monitor,
						 RejillaFileMonitorType type,
						 gpointer callback_data,
						 const gchar *old_name,
						 const gchar *new_name);
	void		(*file_removed)		(RejillaFileMonitor *monitor,
						 RejillaFileMonitorType type,
						 gpointer callback_data,
						 const gchar *name);
	void		(*file_modified)	(RejillaFileMonitor *monitor,
						 gpointer callback_data,
						 const gchar *name);
};

struct _RejillaFileMonitor
{
	GObject parent_instance;
};

GType rejilla_file_monitor_get_type (void) G_GNUC_CONST;

gboolean
rejilla_file_monitor_single_file (RejillaFileMonitor *monitor,
				  const gchar *uri,
				  gpointer callback_data);
gboolean
rejilla_file_monitor_directory_contents (RejillaFileMonitor *monitor,
				  	 const gchar *uri,
				       	 gpointer callback_data);
void
rejilla_file_monitor_reset (RejillaFileMonitor *monitor);

void
rejilla_file_monitor_foreach_cancel (RejillaFileMonitor *self,
				     RejillaMonitorFindFunc func,
				     gpointer callback_data);

G_END_DECLS

#endif /* _REJILLA_FILE_MONITOR_H_ */
