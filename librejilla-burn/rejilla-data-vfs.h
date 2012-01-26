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

#ifndef _REJILLA_DATA_VFS_H_
#define _REJILLA_DATA_VFS_H_

#include <glib-object.h>
#include <gtk/gtk.h>

#include "rejilla-data-session.h"
#include "rejilla-filtered-uri.h"

G_BEGIN_DECLS

#define REJILLA_SCHEMA_FILTER			"org.mate.rejilla.filter"
#define REJILLA_PROPS_FILTER_HIDDEN	        "hidden"
#define REJILLA_PROPS_FILTER_BROKEN	        "broken-sym"
#define REJILLA_PROPS_FILTER_REPLACE_SYMLINK    "replace-sym"

#define REJILLA_TYPE_DATA_VFS             (rejilla_data_vfs_get_type ())
#define REJILLA_DATA_VFS(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_DATA_VFS, RejillaDataVFS))
#define REJILLA_DATA_VFS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_DATA_VFS, RejillaDataVFSClass))
#define REJILLA_IS_DATA_VFS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_DATA_VFS))
#define REJILLA_IS_DATA_VFS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_DATA_VFS))
#define REJILLA_DATA_VFS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_DATA_VFS, RejillaDataVFSClass))

typedef struct _RejillaDataVFSClass RejillaDataVFSClass;
typedef struct _RejillaDataVFS RejillaDataVFS;

struct _RejillaDataVFSClass
{
	RejillaDataSessionClass parent_class;

	void	(*activity_changed)	(RejillaDataVFS *vfs,
					 gboolean active);
};

struct _RejillaDataVFS
{
	RejillaDataSession parent_instance;
};

GType rejilla_data_vfs_get_type (void) G_GNUC_CONST;

gboolean
rejilla_data_vfs_is_active (RejillaDataVFS *vfs);

gboolean
rejilla_data_vfs_is_loading_uri (RejillaDataVFS *vfs);

gboolean
rejilla_data_vfs_load_mime (RejillaDataVFS *vfs,
			    RejillaFileNode *node);

gboolean
rejilla_data_vfs_require_node_load (RejillaDataVFS *vfs,
				    RejillaFileNode *node);

gboolean
rejilla_data_vfs_require_directory_contents (RejillaDataVFS *vfs,
					     RejillaFileNode *node);

RejillaFilteredUri *
rejilla_data_vfs_get_filtered_model (RejillaDataVFS *vfs);

G_END_DECLS

#endif /* _REJILLA_DATA_VFS_H_ */
