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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifndef _REJILLA_DATA_PROJECT_H_
#define _REJILLA_DATA_PROJECT_H_

#include <glib-object.h>
#include <gio/gio.h>

#include <gtk/gtk.h>

#include "rejilla-file-node.h"
#include "rejilla-session.h"

#include "rejilla-track-data.h"

#ifdef BUILD_INOTIFY
#include "rejilla-file-monitor.h"
#endif

G_BEGIN_DECLS


#define REJILLA_TYPE_DATA_PROJECT             (rejilla_data_project_get_type ())
#define REJILLA_DATA_PROJECT(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_DATA_PROJECT, RejillaDataProject))
#define REJILLA_DATA_PROJECT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_DATA_PROJECT, RejillaDataProjectClass))
#define REJILLA_IS_DATA_PROJECT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_DATA_PROJECT))
#define REJILLA_IS_DATA_PROJECT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_DATA_PROJECT))
#define REJILLA_DATA_PROJECT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_DATA_PROJECT, RejillaDataProjectClass))

typedef struct _RejillaDataProjectClass RejillaDataProjectClass;
typedef struct _RejillaDataProject RejillaDataProject;

struct _RejillaDataProjectClass
{
#ifdef BUILD_INOTIFY
	RejillaFileMonitorClass parent_class;
#else
	GObjectClass parent_class;
#endif

	/* virtual functions */

	/**
	 * num_nodes is the number of nodes that were at the root of the 
	 * project.
	 */
	void		(*reset)		(RejillaDataProject *project,
						 guint num_nodes);

	/* NOTE: node_added is also called when there is a moved node;
	 * in this case a node_removed is first called and then the
	 * following function is called (mostly to match GtkTreeModel
	 * API). To detect such a case look at uri which will then be
	 * set to NULL.
	 * NULL uri can also happen when it's a created directory.
	 * if return value is FALSE, node was invalidated during call */
	gboolean	(*node_added)		(RejillaDataProject *project,
						 RejillaFileNode *node,
						 const gchar *uri);

	/* This is more an unparent signal. It shouldn't be assumed that the
	 * node was destroyed or not destroyed. Like the above function, it is
	 * also called when a node is moved. */
	void		(*node_removed)		(RejillaDataProject *project,
						 RejillaFileNode *former_parent,
						 guint former_position,
						 RejillaFileNode *node);

	void		(*node_changed)		(RejillaDataProject *project,
						 RejillaFileNode *node);
	void		(*node_reordered)	(RejillaDataProject *project,
						 RejillaFileNode *parent,
						 gint *new_order);

	void		(*uri_removed)		(RejillaDataProject *project,
						 const gchar *uri);
};

struct _RejillaDataProject
{
#ifdef BUILD_INOTIFY
	RejillaFileMonitor parent_instance;
#else
	GObject parent_instance;
#endif
};

GType rejilla_data_project_get_type (void) G_GNUC_CONST;

void
rejilla_data_project_reset (RejillaDataProject *project);

goffset
rejilla_data_project_get_sectors (RejillaDataProject *project);

goffset
rejilla_data_project_improve_image_size_accuracy (goffset blocks,
						  guint64 dir_num,
						  RejillaImageFS fs_type);
goffset
rejilla_data_project_get_folder_sectors (RejillaDataProject *project,
					 RejillaFileNode *node);

gboolean
rejilla_data_project_get_contents (RejillaDataProject *project,
				   GSList **grafts,
				   GSList **unreadable,
				   gboolean hidden_nodes,
				   gboolean joliet_compat,
				   gboolean append_slash);

gboolean
rejilla_data_project_is_empty (RejillaDataProject *project);

gboolean
rejilla_data_project_has_symlinks (RejillaDataProject *project);

gboolean
rejilla_data_project_is_video_project (RejillaDataProject *project);

gboolean
rejilla_data_project_is_joliet_compliant (RejillaDataProject *project);

guint
rejilla_data_project_load_contents (RejillaDataProject *project,
				    GSList *grafts,
				    GSList *excluded);

RejillaFileNode *
rejilla_data_project_add_hidden_node (RejillaDataProject *project,
				      const gchar *uri,
				      const gchar *name,
				      RejillaFileNode *parent);

RejillaFileNode *
rejilla_data_project_add_loading_node (RejillaDataProject *project,
				       const gchar *uri,
				       RejillaFileNode *parent);
RejillaFileNode *
rejilla_data_project_add_node_from_info (RejillaDataProject *project,
					 const gchar *uri,
					 GFileInfo *info,
					 RejillaFileNode *parent);
RejillaFileNode *
rejilla_data_project_add_empty_directory (RejillaDataProject *project,
					  const gchar *name,
					  RejillaFileNode *parent);
RejillaFileNode *
rejilla_data_project_add_imported_session_file (RejillaDataProject *project,
						GFileInfo *info,
						RejillaFileNode *parent);

void
rejilla_data_project_remove_node (RejillaDataProject *project,
				  RejillaFileNode *node);
void
rejilla_data_project_destroy_node (RejillaDataProject *self,
				   RejillaFileNode *node);

gboolean
rejilla_data_project_node_loaded (RejillaDataProject *project,
				  RejillaFileNode *node,
				  const gchar *uri,
				  GFileInfo *info);
void
rejilla_data_project_node_reloaded (RejillaDataProject *project,
				    RejillaFileNode *node,
				    const gchar *uri,
				    GFileInfo *info);
void
rejilla_data_project_directory_node_loaded (RejillaDataProject *project,
					    RejillaFileNode *parent);

gboolean
rejilla_data_project_rename_node (RejillaDataProject *project,
				  RejillaFileNode *node,
				  const gchar *name);

gboolean
rejilla_data_project_move_node (RejillaDataProject *project,
				RejillaFileNode *node,
				RejillaFileNode *parent);

void
rejilla_data_project_restore_uri (RejillaDataProject *project,
				  const gchar *uri);
void
rejilla_data_project_exclude_uri (RejillaDataProject *project,
				  const gchar *uri);

guint
rejilla_data_project_reference_new (RejillaDataProject *project,
				    RejillaFileNode *node);
void
rejilla_data_project_reference_free (RejillaDataProject *project,
				     guint reference);
RejillaFileNode *
rejilla_data_project_reference_get (RejillaDataProject *project,
				    guint reference);

RejillaFileNode *
rejilla_data_project_get_root (RejillaDataProject *project);

gchar *
rejilla_data_project_node_to_uri (RejillaDataProject *project,
				  RejillaFileNode *node);
gchar *
rejilla_data_project_node_to_path (RejillaDataProject *self,
				   RejillaFileNode *node);

void
rejilla_data_project_set_sort_function (RejillaDataProject *project,
					GtkSortType sort_type,
					GCompareFunc sort_func);

RejillaFileNode *
rejilla_data_project_watch_path (RejillaDataProject *project,
				 const gchar *path);

RejillaBurnResult
rejilla_data_project_span (RejillaDataProject *project,
			   goffset max_sectors,
			   gboolean append_slash,
			   gboolean joliet,
			   RejillaTrackData *track);
RejillaBurnResult
rejilla_data_project_span_again (RejillaDataProject *project);

RejillaBurnResult
rejilla_data_project_span_possible (RejillaDataProject *project,
				    goffset max_sectors);
goffset
rejilla_data_project_get_max_space (RejillaDataProject *self);

void
rejilla_data_project_span_stop (RejillaDataProject *project);

G_END_DECLS

#endif /* _REJILLA_DATA_PROJECT_H_ */
