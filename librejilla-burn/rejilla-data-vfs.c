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

#include <string.h>

#include <glib.h>
#include <glib/gi18n-lib.h>

#include "rejilla-misc.h"

#include "rejilla-data-vfs.h"
#include "rejilla-data-project.h"
#include "rejilla-file-node.h"
#include "rejilla-io.h"
#include "rejilla-filtered-uri.h"

#include "librejilla-marshal.h"

#include "burn-debug.h"

typedef struct _RejillaDataVFSPrivate RejillaDataVFSPrivate;
struct _RejillaDataVFSPrivate
{
	/* In this hash there are all URIs currently loading. Every
	 * time we want to load a new URI, we should ask this table if
	 * that URI is currently loading; if so, add the associated
	 * node to the list in the hash table. */
	GHashTable *loading;
	GHashTable *directories;

	RejillaFilteredUri *filtered;

	RejillaIOJobBase *load_uri;
	RejillaIOJobBase *load_contents;

	GSettings *settings;

	guint replace_sym:1;
	guint filter_hidden:1;
	guint filter_broken_sym:1;
};

#define REJILLA_DATA_VFS_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REJILLA_TYPE_DATA_VFS, RejillaDataVFSPrivate))

enum {
	UNREADABLE_SIGNAL,
	RECURSIVE_SIGNAL,
	IMAGE_SIGNAL,
	ACTIVITY_SIGNAL,
	UNKNOWN_SIGNAL,
	LAST_SIGNAL
};

static gulong rejilla_data_vfs_signals [LAST_SIGNAL] = { 0 };


G_DEFINE_TYPE (RejillaDataVFS, rejilla_data_vfs, REJILLA_TYPE_DATA_SESSION);

RejillaFilteredUri *
rejilla_data_vfs_get_filtered_model (RejillaDataVFS *vfs)
{
	RejillaDataVFSPrivate *priv;

	priv = REJILLA_DATA_VFS_PRIVATE (vfs);

	return priv->filtered;
}

gboolean
rejilla_data_vfs_is_active (RejillaDataVFS *self)
{
	RejillaDataVFSPrivate *priv;

	priv = REJILLA_DATA_VFS_PRIVATE (self);
	return (g_hash_table_size (priv->loading) != 0 || g_hash_table_size (priv->directories) != 0);
}

gboolean
rejilla_data_vfs_is_loading_uri (RejillaDataVFS *self)
{
	RejillaDataVFSPrivate *priv;

	priv = REJILLA_DATA_VFS_PRIVATE (self);
	return (g_hash_table_size (priv->loading) != 0);
}

static RejillaBurnResult
rejilla_data_vfs_emit_image_signal (RejillaDataVFS *self,
				    const gchar *uri)
{
	GValue instance_and_params [2];
	GValue return_value;
	GValue *params;

	/* object which signalled */
	instance_and_params->g_type = 0;
	g_value_init (instance_and_params, G_TYPE_FROM_INSTANCE (self));
	g_value_set_instance (instance_and_params, self);

	/* arguments of signal (name) */
	params = instance_and_params + 1;
	params->g_type = 0;
	g_value_init (params, G_TYPE_STRING);
	g_value_set_string (params, uri);

	/* default to CANCEL */
	return_value.g_type = 0;
	g_value_init (&return_value, G_TYPE_INT);
	g_value_set_int (&return_value, REJILLA_BURN_CANCEL);

	g_signal_emitv (instance_and_params,
			rejilla_data_vfs_signals [IMAGE_SIGNAL],
			0,
			&return_value);

	g_value_unset (instance_and_params);
	g_value_unset (params);

	return g_value_get_int (&return_value);
}

static gboolean
rejilla_data_vfs_check_uri_result (RejillaDataVFS *self,
				   const gchar *uri,
				   GError *error,
				   GFileInfo *info)
{
	RejillaDataVFSPrivate *priv;

	priv = REJILLA_DATA_VFS_PRIVATE (self);

	/* Only signal errors if the node was specifically added by the user
	 * that is if it is loading. So check the loading GHashTable to know 
	 * that. Otherwise this URI comes from directory exploration.
	 * The problem is the URI returned by rejilla-io could be different
	 * from the one set in the loading hash if there are parent symlinks.
	 * That's one of the readon why we passed the orignal URI as a
	 * registered string in the callback. Of course that's not true when
	 * we're loading directory contents. */

	if (error) {
		if (error->domain == G_IO_ERROR && error->code == G_IO_ERROR_NOT_FOUND) {
			if (g_hash_table_lookup (priv->loading, uri))
				g_signal_emit (self,
					       rejilla_data_vfs_signals [UNKNOWN_SIGNAL],
					       0,
					       uri);
		}
		else if (error->domain == REJILLA_UTILS_ERROR && error->code == REJILLA_UTILS_ERROR_SYMLINK_LOOP) {
			rejilla_data_project_exclude_uri (REJILLA_DATA_PROJECT (self),
							  uri);

			if (g_hash_table_lookup (priv->loading, uri))
				g_signal_emit (self,
					       rejilla_data_vfs_signals [RECURSIVE_SIGNAL],
					       0,
					       uri);
		}
		else {
			rejilla_data_project_exclude_uri (REJILLA_DATA_PROJECT (self),
							  uri);

			if (g_hash_table_lookup (priv->loading, uri))
				g_signal_emit (self,
					       rejilla_data_vfs_signals [UNREADABLE_SIGNAL],
					       0,
					       error,
					       uri);
		}

		REJILLA_BURN_LOG ("VFS information retrieval error %s : %s\n",
				  uri,
				  error->message);

		return FALSE;
	}

	if (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_ACCESS_CAN_READ)
	&& !g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_READ)) {
		rejilla_data_project_exclude_uri (REJILLA_DATA_PROJECT (self), uri);

		if (g_hash_table_lookup (priv->loading, uri)) {
			GError *error;

			error = g_error_new (REJILLA_UTILS_ERROR,
					     REJILLA_UTILS_ERROR_GENERAL,
					     _("\"%s\" cannot be read"),
					     g_file_info_get_name (info));
			g_signal_emit (self,
				       rejilla_data_vfs_signals [UNREADABLE_SIGNAL],
				       0,
				       error,
				       uri);
		}
		return FALSE;
	}

	return TRUE;
}

static void
rejilla_data_vfs_remove_from_hash (RejillaDataVFS *self,
				   GHashTable *h_table,
				   const gchar *uri)
{
	GSList *nodes;
	GSList *iter;

	/* data is the reference to the node explored */
	nodes = g_hash_table_lookup (h_table, uri);
	for (iter = nodes; iter; iter = iter->next) {
		guint reference;

		reference = GPOINTER_TO_INT (iter->data);
		rejilla_data_project_reference_free (REJILLA_DATA_PROJECT (self), reference);
	}
	g_slist_free (nodes);
	g_hash_table_remove (h_table, uri);
}

/**
 * Explore and add the contents of a directory already loaded
 */
static void
rejilla_data_vfs_directory_load_end (GObject *object,
				     gboolean cancelled,
				     gpointer data)
{
	RejillaDataVFSPrivate *priv = REJILLA_DATA_VFS_PRIVATE (object);
	RejillaDataVFS *self = REJILLA_DATA_VFS (object);
	gchar *uri = data;
	GSList *nodes;

	priv = REJILLA_DATA_VFS_PRIVATE (self);

	nodes = g_hash_table_lookup (priv->directories, uri);
	for (; nodes; nodes = nodes->next) {
		RejillaFileNode *parent;
		guint reference;

		reference = GPOINTER_TO_INT (nodes->data);
		parent = rejilla_data_project_reference_get (REJILLA_DATA_PROJECT (self), reference);
		if (!parent)
			continue;

		rejilla_data_project_directory_node_loaded (REJILLA_DATA_PROJECT (self), parent);
	}

	rejilla_data_vfs_remove_from_hash (self, priv->directories, uri);
	rejilla_utils_unregister_string (uri);

	if (cancelled)
		return;

	/* Only emit a signal if state changed. Some widgets need to know if 
	 * either directories loading or uri loading state has changed to signal
	 * it even if there were some uri loading. */
	if (!g_hash_table_size (priv->directories))
		g_signal_emit (self,
			       rejilla_data_vfs_signals [ACTIVITY_SIGNAL],
			       0,
			       g_hash_table_size (priv->loading));
}

static gboolean
rejilla_data_vfs_directory_check_symlink_loop (RejillaDataVFS *self,
					       RejillaFileNode *parent,
					       const gchar *uri,
					       GFileInfo *info)
{
	RejillaDataVFSPrivate *priv;
	const gchar *target_uri;
	guint target_len;
	guint uri_len;

	priv = REJILLA_DATA_VFS_PRIVATE (self);

	/* Of course for a loop to exist, it must be a directory */
	if (g_file_info_get_file_type (info) != G_FILE_TYPE_DIRECTORY)
		return FALSE;

	target_uri = g_file_info_get_symlink_target (info);
	if (!target_uri)
		return FALSE;

	/* if target points to a child that's OK */
	uri_len = strlen (uri);
	if (!strncmp (target_uri, uri, uri_len)
	&&   target_uri [uri_len] == G_DIR_SEPARATOR)
		return FALSE;

	target_len = strlen (target_uri);
	while (parent && !parent->is_root) {
		RejillaFileNode *next;
		gchar *parent_uri;
		guint parent_len;
		gchar *next_uri;
		guint next_len;

		/* if the file is not grafted carry on */
		if (!parent->is_grafted) {
			parent = parent->parent;
			continue;
		}

		/* if the file is a symlink, carry on since that's why it was
		 * grafted. It can't have been added by the user since in this
		 * case we replace the symlink by the target. */
		if (parent->is_symlink) {
			parent = parent->parent;
			continue;
		}

		parent_uri = rejilla_data_project_node_to_uri (REJILLA_DATA_PROJECT (self), parent);
		parent_len = strlen (parent_uri);

		/* see if target is a parent of that file */
		if (!strncmp (target_uri, parent_uri, target_len)
		&&   parent_uri [target_len] == G_DIR_SEPARATOR) {
			g_free (parent_uri);
			return TRUE;
		}

		/* see if the graft is also the parent of the target */
		if (!strncmp (parent_uri, target_uri, parent_len)
		&&   target_uri [parent_len] == G_DIR_SEPARATOR) {
			g_free (parent_uri);
			return TRUE;
		}

		/* The next graft point must be the natural parent of this one */
		next = parent->parent;
		if (!next || next->is_root || next->is_fake) {
			/* It's not we're done */
			g_free (parent_uri);
			break;
		}

		next_uri = rejilla_data_project_node_to_uri (REJILLA_DATA_PROJECT (self), next);
		next_len = strlen (next_uri);

		if (!strncmp (next_uri, parent_uri, next_len)
		&&   parent_uri [next_len] == G_DIR_SEPARATOR) {
			/* It's not the natural parent. We're done */
			g_free (parent_uri);
			break;
		}

		/* retry with the next parent graft point */
		g_free (parent_uri);
		parent = next;
	}

	return FALSE;
}

static void
rejilla_data_vfs_directory_load_result (GObject *owner,
					GError *error,
					const gchar *uri,
					GFileInfo *info,
					gpointer data)
{
	RejillaDataVFS *self = REJILLA_DATA_VFS (owner);
	RejillaDataVFSPrivate *priv;
	gchar *parent_uri = data;
	const gchar *name;
	GSList *nodes;
	GSList *iter;

	priv = REJILLA_DATA_VFS_PRIVATE (self);

	/* check the status of the operation.
	 * NOTE: no need to remove the nodes. */
	if (!rejilla_data_vfs_check_uri_result (self, uri, error, info))
		return;

	/* Filtering part */
	name = g_file_info_get_name (info);

	/* See if it's a broken symlink */
	if (g_file_info_get_is_symlink (info)
	&& !g_file_info_get_symlink_target (info)) {
		/* See if this file is already in restored or if we should filter */
		if (priv->filter_broken_sym
		    && !rejilla_filtered_uri_lookup_restored (priv->filtered, uri)) {
			rejilla_filtered_uri_filter (priv->filtered,
						     uri,
						     REJILLA_FILTER_BROKEN_SYM);
			rejilla_data_project_exclude_uri (REJILLA_DATA_PROJECT (self), uri);
			return;
		}
	}
	/* A new hidden file ? */
	else if (name [0] == '.') {
		/* See if this file is already in restored or if we should filter */
		if (priv->filter_hidden
		&& !rejilla_filtered_uri_lookup_restored (priv->filtered, uri)) {
			rejilla_filtered_uri_filter (priv->filtered,
						     uri,
						     REJILLA_FILTER_HIDDEN);
			rejilla_data_project_exclude_uri (REJILLA_DATA_PROJECT (self), uri);
			return;
		}
	}

	/* add node for all parents */
	nodes = g_hash_table_lookup (priv->directories, parent_uri);
	for (iter = nodes; iter; iter = iter->next) {
		guint reference;
		RejillaFileNode *parent;

		reference = GPOINTER_TO_INT (iter->data);
		parent = rejilla_data_project_reference_get (REJILLA_DATA_PROJECT (self), reference);
		if (!parent)
			continue;

		if (parent->is_root) {
			/* This may be true in some rare situations (when the root of a
			 * volume has been added like burn:/// */
			rejilla_data_project_add_loading_node (REJILLA_DATA_PROJECT (self),
							       uri,
							       parent);
			 continue;
		}

		if (g_file_info_get_is_symlink (info)) {
			if (rejilla_data_vfs_directory_check_symlink_loop (self, parent, uri, info)) {
				rejilla_data_project_exclude_uri (REJILLA_DATA_PROJECT (self), uri);
				if (g_hash_table_lookup (priv->loading, uri))
					g_signal_emit (self,
						       rejilla_data_vfs_signals [RECURSIVE_SIGNAL],
						       0,
						       uri);
				return;
			}

			if (!priv->replace_sym) {
				/* This is to workaround a small inconsistency
				 * in GVFS burn:// backend. When there is a
				 * symlink in burn:// and we are asked not to 
				 * follow symlinks then the file type is not
				 * G_FILE_TYPE_SYMBOLIC_LINK */
				g_file_info_set_file_type (info, G_FILE_TYPE_SYMBOLIC_LINK);
			}
		}

		rejilla_data_project_add_node_from_info (REJILLA_DATA_PROJECT (self),
							 uri,
							 info,
							 parent);
	}
}

static gboolean
rejilla_data_vfs_load_directory (RejillaDataVFS *self,
				 RejillaFileNode *node,
				 const gchar *uri)
{
	RejillaDataVFSPrivate *priv;
	gchar *registered;
	guint reference;
	GSList *nodes;

	priv = REJILLA_DATA_VFS_PRIVATE (self);

	/* Start exploration of directory*/
	reference = rejilla_data_project_reference_new (REJILLA_DATA_PROJECT (self), node);

	nodes = g_hash_table_lookup (priv->directories, uri);
	if (nodes) {
		/* It's loading, wait for the results */
		nodes = g_slist_prepend (nodes, GINT_TO_POINTER (reference));
		g_hash_table_insert (priv->directories, (gchar *) uri, nodes);
		return TRUE;
	}

	registered = rejilla_utils_register_string (uri);
	g_hash_table_insert (priv->directories,
			     registered,
			     g_slist_prepend (NULL, GINT_TO_POINTER (reference)));

	if (!priv->load_contents)
		priv->load_contents = rejilla_io_register (G_OBJECT (self),
							   rejilla_data_vfs_directory_load_result,
							   rejilla_data_vfs_directory_load_end,
							   NULL);

	/* no need to require mime types here as these rows won't be visible */
	rejilla_io_load_directory (uri,
				   priv->load_contents,
				   REJILLA_IO_INFO_PERM|
				  (priv->replace_sym ? REJILLA_IO_INFO_FOLLOW_SYMLINK:REJILLA_IO_INFO_NONE),
				   registered);

	/* Only emit a signal if state changed. Some widgets need to know if 
	 * either directories loading or uri loading state has changed to signal
	 * it even if there were some uri loading. */
	if (g_hash_table_size (priv->directories) == 1)
		g_signal_emit (self,
			       rejilla_data_vfs_signals [ACTIVITY_SIGNAL],
			       0,
			       TRUE);

	return TRUE;
}

/**
 * Update a node already in the tree
 */
static void
rejilla_data_vfs_loading_node_end (GObject *object,
				   gboolean cancelled,
				   gpointer data)
{
	RejillaDataVFSPrivate *priv = REJILLA_DATA_VFS_PRIVATE (object);
	RejillaDataVFS *self = REJILLA_DATA_VFS (object);
	gchar *uri = data;

	priv = REJILLA_DATA_VFS_PRIVATE (self);
	rejilla_data_vfs_remove_from_hash (self, priv->loading, uri);
	rejilla_utils_unregister_string (uri);

	/* Only emit a signal if state changed. Some widgets need to know if 
	 * either directories loading or uri loading state has changed to signal
	 * it even if there were some directories loading.
	 * NOTE: we only cancel when we're stopping. That's why there is no need
	 * to emit any signal in this case (cancellation). */
	if (!g_hash_table_size (priv->loading) && !cancelled)
		g_signal_emit (self,
			       rejilla_data_vfs_signals [ACTIVITY_SIGNAL],
			       0,
			       g_hash_table_size (priv->directories));
}

static void
rejilla_data_vfs_loading_node_result (GObject *owner,
				      GError *error,
				      const gchar *uri,
				      GFileInfo *info,
				      gpointer callback_data)
{
	GSList *iter;
	GSList *nodes;
	RejillaFileNode *root;
	RejillaFileTreeStats *stats;
	gchar *registered = callback_data;
	RejillaDataVFS *self = REJILLA_DATA_VFS (owner);
	RejillaDataVFSPrivate *priv = REJILLA_DATA_VFS_PRIVATE (self);

	nodes = g_hash_table_lookup (priv->loading, registered);

	/* check the status of the operation */
	if (!rejilla_data_vfs_check_uri_result (self, registered, error, info)) {
		/* we need to remove the loading node that is waiting */
		for (iter = nodes; iter; iter = iter->next) {
			RejillaFileNode *node;
			guint reference;

			reference = GPOINTER_TO_INT (iter->data);
			node = rejilla_data_project_reference_get (REJILLA_DATA_PROJECT (self), reference);

			/* the node could have been removed in the mean time */
			if (node)
				rejilla_data_project_remove_node (REJILLA_DATA_PROJECT (self), node);
		}

		return;
	}

	/* It can happen that the user made a mistake out of ignorance or for
	 * whatever other reason and dropped an image he wanted to burn.
	 * So if our file is the only one in the project and if that's an image
	 * check it is an image. If so, ask him if that's he really want to do. */
	root = rejilla_data_project_get_root (REJILLA_DATA_PROJECT (self));
	stats = REJILLA_FILE_NODE_STATS (root);

	if (stats && !stats->children
	&&  rejilla_file_node_get_n_children (root) <= 1
	&& (!strcmp (g_file_info_get_content_type (info), "application/x-toc")
	||  !strcmp (g_file_info_get_content_type (info), "application/x-cdrdao-toc")
	||  !strcmp (g_file_info_get_content_type (info), "application/x-cue")
	||  !strcmp (g_file_info_get_content_type (info), "application/x-cd-image"))) {
		RejillaBurnResult result;

		result = rejilla_data_vfs_emit_image_signal (self, uri);
		if (result == REJILLA_BURN_CANCEL) {
			/* recheck the node as a reset may have been done */
			nodes = g_hash_table_lookup (priv->loading, registered);
			for (iter = nodes; iter; iter = iter->next) {
				RejillaFileNode *node;
				guint reference;

				reference = GPOINTER_TO_INT (iter->data);
				node = rejilla_data_project_reference_get (REJILLA_DATA_PROJECT (self), reference);

				/* the node could have been removed in the mean time */
				if (node)
					rejilla_data_project_remove_node (REJILLA_DATA_PROJECT (self), node);
			}

			return;
		}
	}

	/* NOTE: we don't check for a broken symlink here since the user chose
	 * to add it. So even if it were we would have to add it. The same for
	 * hidden files. */
	for (iter = nodes; iter; iter = iter->next) {
		guint reference;
		gboolean added;
		RejillaFileNode *node;

		reference = GPOINTER_TO_INT (iter->data);

		/* check if the node still exists */
		node = rejilla_data_project_reference_get (REJILLA_DATA_PROJECT (self), reference);
		rejilla_data_project_reference_free (REJILLA_DATA_PROJECT (self), reference);

		if (!node)
			continue;

		/* if the node is reloading but not loading that means we just
		 * want to know if it is still readable and or if its size (if
		 * it's a file) changed; if we reached this point no need to go
		 * further.
		 * Yet, another case is when we need their mime type. They are
		 * also set as reloading. */

		/* NOTE: check is loading here on purpose. Otherwise directories
		 * that replace a temp parent wouldn't load since they are also
		 * reloading. */
		if (g_file_info_get_is_symlink (info)
		&& !priv->replace_sym) {
			/* This is to workaround a small inconsistency
			 * in GVFS burn:// backend. When there is a
			 * symlink in burn:// and we are asked not to 
			 * follow symlinks then the file type is not
			 * G_FILE_TYPE_SYMBOLIC_LINK */
			g_file_info_set_file_type (info, G_FILE_TYPE_SYMBOLIC_LINK);
		}

		if (!node->is_loading) {
			rejilla_data_project_node_reloaded (REJILLA_DATA_PROJECT (self),
							    node,
							    uri,
							    info);
			continue;
		}

		/* update node */
		added = rejilla_data_project_node_loaded (REJILLA_DATA_PROJECT (self),
		                                          node,
		                                          uri,
		                                          info);

		if (!added)
			continue;

		/* See what type of file it is. If that's a directory then 
		 * explore it right away */
		if (node->is_file)
			continue;

		/* starts exploring its contents */
		rejilla_data_vfs_load_directory (self, node, uri);
	}
}

static gboolean
rejilla_data_vfs_load_node (RejillaDataVFS *self,
			    RejillaIOFlags flags,
			    guint reference,
			    const gchar *uri)
{
	RejillaDataVFSPrivate *priv;
	gchar *registered;

	priv = REJILLA_DATA_VFS_PRIVATE (self);

	registered = rejilla_utils_register_string (uri);
	g_hash_table_insert (priv->loading,
			     registered,
			     g_slist_prepend (NULL, GINT_TO_POINTER (reference)));

	if (!priv->load_uri)
		priv->load_uri = rejilla_io_register (G_OBJECT (self),
						      rejilla_data_vfs_loading_node_result,
						      rejilla_data_vfs_loading_node_end,
						      NULL);

	rejilla_io_get_file_info (uri,
				  priv->load_uri,
				  flags|
				  (priv->replace_sym ? REJILLA_IO_INFO_FOLLOW_SYMLINK:REJILLA_IO_INFO_NONE),
				  registered);

	/* Only emit a signal if state changed. Some widgets need to know if 
	 * either directories loading or uri loading state has changed to signal
	 * it even if there were some directories loading. */
	if (g_hash_table_size (priv->loading) == 1)
		g_signal_emit (self,
			       rejilla_data_vfs_signals [ACTIVITY_SIGNAL],
			       0,
			       TRUE);

	return TRUE;
}

static gboolean
rejilla_data_vfs_loading_node (RejillaDataVFS *self,
			       RejillaFileNode *node,
			       const gchar *uri)
{
	RejillaDataVFSPrivate *priv;
	guint reference;
	GSList *nodes;

	/* NOTE: this function receives URIs only from utf8 origins (not from
	 * GIO for example) so we can assume that this is safe */

	priv = REJILLA_DATA_VFS_PRIVATE (self);

	if (!node->is_reloading
	&&   REJILLA_FILE_NODE_NAME (node)
	&&  !strcmp (REJILLA_FILE_NODE_NAME (node), G_DIR_SEPARATOR_S)) {
		/* This is a root directory: we don't add it since a
		 * child of the root directory can't be a root itself.
		 * So we add all its contents under its parent. Remove
		 * the loading node as well. 
		 * Be careful in the next functions not to use node. */
		rejilla_data_vfs_load_directory (self, node->parent, uri);

		/* node was invalidated: return FALSE */
		rejilla_data_project_remove_node (REJILLA_DATA_PROJECT (self), node);
		return FALSE;
	}

	/* FIXME: we could know right from the start if that node is is_loading */
	/* add a reference on the node to update it when we have all information */
	reference = rejilla_data_project_reference_new (REJILLA_DATA_PROJECT (self), node);
	nodes = g_hash_table_lookup (priv->loading, uri);
	if (nodes) {
		/* It's loading, wait for the results */
		nodes = g_slist_prepend (nodes, GINT_TO_POINTER (reference));
		g_hash_table_insert (priv->loading, (gchar *) uri, nodes);
		return TRUE;
	}

	/* loading nodes are almost always visible already so get mime type */
	return rejilla_data_vfs_load_node (self,
					   REJILLA_IO_INFO_PERM|
					   REJILLA_IO_INFO_MIME|
					   REJILLA_IO_INFO_CHECK_PARENT_SYMLINK,
					   reference,
					   uri);
}

static gboolean
rejilla_data_vfs_increase_priority_cb (gpointer data, gpointer user_data)
{
	if (data == user_data)
		return TRUE;

	return FALSE;
}

static gboolean
rejilla_data_vfs_require_higher_priority (RejillaDataVFS *self,
					  RejillaFileNode *node,
					  RejillaIOJobBase *type)
{
	gchar *registered;
	gchar *uri;

	uri = rejilla_data_project_node_to_uri (REJILLA_DATA_PROJECT (self), node);
	registered = rejilla_utils_register_string (uri);
	g_free (uri);

	rejilla_io_find_urgent (type,
				rejilla_data_vfs_increase_priority_cb,
				registered);

	rejilla_utils_unregister_string (registered);
	return TRUE;
}

gboolean
rejilla_data_vfs_require_directory_contents (RejillaDataVFS *self,
					     RejillaFileNode *node)
{
	RejillaDataVFSPrivate *priv;

	priv = REJILLA_DATA_VFS_PRIVATE (self);
	return rejilla_data_vfs_require_higher_priority (self,
							 node,
							 priv->load_contents);
}

gboolean
rejilla_data_vfs_require_node_load (RejillaDataVFS *self,
				    RejillaFileNode *node)
{
	RejillaDataVFSPrivate *priv;

	priv = REJILLA_DATA_VFS_PRIVATE (self);
	return rejilla_data_vfs_require_higher_priority (self,
							 node,
							 priv->load_uri);
}

gboolean
rejilla_data_vfs_load_mime (RejillaDataVFS *self,
			    RejillaFileNode *node)
{
	RejillaDataVFSPrivate *priv;
	guint reference;
	gboolean result;
	GSList *nodes;
	gchar *uri;

	priv = REJILLA_DATA_VFS_PRIVATE (self);

	if (node->is_loading || node->is_reloading) {
		rejilla_data_vfs_require_node_load (self, node);
		return TRUE;
	}

	uri = rejilla_data_project_node_to_uri (REJILLA_DATA_PROJECT (self), node);
	node->is_reloading = TRUE;

	/* make sure the node is not already in the loading table */
	nodes = g_hash_table_lookup (priv->loading, uri);
	if (nodes) {
		gchar *registered;
		GSList *iter;

		registered = rejilla_utils_register_string (uri);
		g_free (uri);

		for (iter = nodes; iter; iter = iter->next) {
			guint reference;
			RejillaFileNode *ref_node;

			reference = GPOINTER_TO_INT (iter->data);
			ref_node = rejilla_data_project_reference_get (REJILLA_DATA_PROJECT (self), reference);
			if (ref_node == node) {
				/* Ask for a higher priority */
				rejilla_io_find_urgent (priv->load_uri,
							rejilla_data_vfs_increase_priority_cb,
							registered);
				rejilla_utils_unregister_string (registered);
				return TRUE;
			}
		}

		/* It's loading, wait for the results */
		reference = rejilla_data_project_reference_new (REJILLA_DATA_PROJECT (self), node);
		nodes = g_slist_prepend (nodes, GINT_TO_POINTER (reference));
		g_hash_table_insert (priv->loading, registered, nodes);

		/* Yet, ask for a higher priority */
		rejilla_io_find_urgent (priv->load_uri,
					rejilla_data_vfs_increase_priority_cb,
					registered);
		rejilla_utils_unregister_string (registered);
		return TRUE;
	}

	reference = rejilla_data_project_reference_new (REJILLA_DATA_PROJECT (self), node);
	result = rejilla_data_vfs_load_node (self,
					     REJILLA_IO_INFO_MIME|
					     REJILLA_IO_INFO_URGENT|
					     (priv->replace_sym ? REJILLA_IO_INFO_FOLLOW_SYMLINK:REJILLA_IO_INFO_NONE),
					     reference,
					     uri);
	g_free (uri);

	return result;
}

/**
 * This function implements the virtual function from data-project
 * It checks the node type and if it is a directory, it explores it
 */
static gboolean
rejilla_data_vfs_node_added (RejillaDataProject *project,
			     RejillaFileNode *node,
			     const gchar *uri)
{
	RejillaDataVFSPrivate *priv;
	RejillaDataVFS *self;

	self = REJILLA_DATA_VFS (project);
	priv = REJILLA_DATA_VFS_PRIVATE (self);

	/* URI can be NULL if it's a created directory or if the node
	 * has just been moved to another location in the tree. */
	if (!uri)
		goto chain;

	/* Is it loading or reloading? if not, only explore directories. */
	if (node->is_loading || node->is_reloading) {
		if (rejilla_data_vfs_loading_node (self, node, uri))
			goto chain;

		/* The node was invalidated. So there's no need to pass it on */
		return FALSE;
	}

	/* NOTE: a symlink pointing to a directory will return TRUE. */
	if (node->is_file)
		goto chain;

	rejilla_data_vfs_load_directory (self, node, uri);

chain:
	/* chain up this function except if we invalidated the node */
	if (REJILLA_DATA_PROJECT_CLASS (rejilla_data_vfs_parent_class)->node_added)
		return REJILLA_DATA_PROJECT_CLASS (rejilla_data_vfs_parent_class)->node_added (project, node, uri);

	return TRUE;
}

static gboolean
rejilla_data_vfs_empty_loading_cb (gpointer key,
				   gpointer data,
				   gpointer callback_data)
{
	RejillaDataProject *project = REJILLA_DATA_PROJECT (callback_data);
	GSList *nodes = data;
	GSList *iter;

	rejilla_utils_unregister_string (key);
	for (iter = nodes; iter; iter = iter->next) {
		guint reference;

		reference = GPOINTER_TO_INT (iter->data);
		rejilla_data_project_reference_free (project, reference);
	}
	g_slist_free (nodes);
	return TRUE;
}

static void
rejilla_data_vfs_clear (RejillaDataVFS *self)
{
	RejillaDataVFSPrivate *priv;

	priv = REJILLA_DATA_VFS_PRIVATE (self);

	/* Stop all VFS operations */
	if (priv->load_uri) {
		rejilla_io_cancel_by_base (priv->load_uri);
		rejilla_io_job_base_free (priv->load_uri);
		priv->load_uri = NULL;
	}

	if (priv->load_contents) {
		rejilla_io_cancel_by_base (priv->load_contents);
		rejilla_io_job_base_free (priv->load_contents);
		priv->load_contents = NULL;
	}

	/* Empty the hash tables */
	g_hash_table_foreach_remove (priv->loading,
				     rejilla_data_vfs_empty_loading_cb,
				     self);
	g_hash_table_foreach_remove (priv->directories,
				     rejilla_data_vfs_empty_loading_cb,
				     self);

	rejilla_filtered_uri_clear (priv->filtered);
}

static void
rejilla_data_vfs_uri_removed (RejillaDataProject *project,
			      const gchar *uri)
{
	RejillaDataVFSPrivate *priv;

	priv = REJILLA_DATA_VFS_PRIVATE (project);

	/* That happens when a graft is removed from the tree, that is when this
	 * graft uri doesn't appear anywhere and when it hasn't got any more 
	 * parent uri grafted. */
	rejilla_filtered_uri_remove_with_children (priv->filtered, uri);
}

static void
rejilla_data_vfs_reset (RejillaDataProject *project,
			guint num_nodes)
{
	rejilla_data_vfs_clear (REJILLA_DATA_VFS (project));

	/* chain up this function except if we invalidated the node */
	if (REJILLA_DATA_PROJECT_CLASS (rejilla_data_vfs_parent_class)->reset)
		REJILLA_DATA_PROJECT_CLASS (rejilla_data_vfs_parent_class)->reset (project, num_nodes);
}

static void
rejilla_data_vfs_settings_changed (GSettings *settings,
                                   const gchar *key,
                                   RejillaDataVFS *self)
{
	RejillaDataVFSPrivate *priv;

	priv = REJILLA_DATA_VFS_PRIVATE (self);

	if (g_strcmp0 (key, REJILLA_PROPS_FILTER_REPLACE_SYMLINK))
		priv->replace_sym = g_settings_get_boolean (settings, REJILLA_PROPS_FILTER_REPLACE_SYMLINK);
	if (g_strcmp0 (key, REJILLA_PROPS_FILTER_BROKEN))
		priv->filter_broken_sym = g_settings_get_boolean (settings, REJILLA_PROPS_FILTER_BROKEN);
	if (g_strcmp0 (key, REJILLA_PROPS_FILTER_HIDDEN))
		priv->filter_hidden = g_settings_get_boolean (settings, REJILLA_PROPS_FILTER_HIDDEN);
}

static void
rejilla_data_vfs_init (RejillaDataVFS *object)
{
	RejillaDataVFSPrivate *priv;

	priv = REJILLA_DATA_VFS_PRIVATE (object);

	priv->filtered = rejilla_filtered_uri_new ();

	/* load the fitering rules */
	priv->settings = g_settings_new (REJILLA_SCHEMA_FILTER);
	priv->replace_sym = g_settings_get_boolean (priv->settings, REJILLA_PROPS_FILTER_REPLACE_SYMLINK);
	priv->filter_broken_sym = g_settings_get_boolean (priv->settings, REJILLA_PROPS_FILTER_BROKEN);
	priv->filter_hidden = g_settings_get_boolean (priv->settings, REJILLA_PROPS_FILTER_HIDDEN);
	g_signal_connect (priv->settings,
	                  "changed",
	                  G_CALLBACK (rejilla_data_vfs_settings_changed),
	                  object);

	/* create the hash tables */
	priv->loading = g_hash_table_new (g_str_hash, g_str_equal);
	priv->directories = g_hash_table_new (g_str_hash, g_str_equal);
}

static void
rejilla_data_vfs_finalize (GObject *object)
{
	RejillaDataVFSPrivate *priv;

	rejilla_data_vfs_clear (REJILLA_DATA_VFS (object));

	priv = REJILLA_DATA_VFS_PRIVATE (object);

	if (priv->loading) {
		g_hash_table_destroy (priv->loading);
		priv->loading = NULL;
	}

	if (priv->directories) {
		g_hash_table_destroy (priv->directories);
		priv->directories = NULL;
	}

	if (priv->filtered) {
		g_object_unref (priv->filtered);
		priv->filtered = NULL;
	}

	if (priv->settings) {
		g_object_unref (priv->settings);
		priv->settings = NULL;
	}

	G_OBJECT_CLASS (rejilla_data_vfs_parent_class)->finalize (object);
}

static void
rejilla_data_vfs_class_init (RejillaDataVFSClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	RejillaDataProjectClass *data_project_class = REJILLA_DATA_PROJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (RejillaDataVFSPrivate));

	object_class->finalize = rejilla_data_vfs_finalize;

	data_project_class->reset = rejilla_data_vfs_reset;
	data_project_class->node_added = rejilla_data_vfs_node_added;
	data_project_class->uri_removed = rejilla_data_vfs_uri_removed;

	/* There is no need to implement the other virtual functions.
	 * For example, even if we were notified of a node removal it 
	 * would take a lot of time to remove it from the hashes. */

	rejilla_data_vfs_signals [ACTIVITY_SIGNAL] = 
	    g_signal_new ("vfs_activity",
			  G_TYPE_FROM_CLASS (klass),
			  G_SIGNAL_RUN_FIRST|G_SIGNAL_NO_RECURSE,
			  G_STRUCT_OFFSET (RejillaDataVFSClass,
					   activity_changed),
			  NULL, NULL,
			  g_cclosure_marshal_VOID__BOOLEAN,
			  G_TYPE_NONE,
			  1,
			  G_TYPE_BOOLEAN);

	rejilla_data_vfs_signals [IMAGE_SIGNAL] = 
	    g_signal_new ("image_uri",
			  G_TYPE_FROM_CLASS (klass),
			  G_SIGNAL_RUN_LAST|G_SIGNAL_NO_RECURSE,
			  0,
			  NULL, NULL,
			  rejilla_marshal_INT__STRING,
			  G_TYPE_INT,
			  1,
			  G_TYPE_STRING);

	rejilla_data_vfs_signals [UNREADABLE_SIGNAL] = 
	    g_signal_new ("unreadable_uri",
			  G_TYPE_FROM_CLASS (klass),
			  G_SIGNAL_RUN_FIRST,
			  0,
			  NULL, NULL,
			  rejilla_marshal_VOID__POINTER_STRING,
			  G_TYPE_NONE,
			  2,
			  G_TYPE_POINTER,
			  G_TYPE_STRING);

	rejilla_data_vfs_signals [RECURSIVE_SIGNAL] = 
	    g_signal_new ("recursive_sym",
			  G_TYPE_FROM_CLASS (klass),
			  G_SIGNAL_RUN_FIRST,
			  0,
			  NULL, NULL,
			  g_cclosure_marshal_VOID__STRING,
			  G_TYPE_NONE,
			  1,
			  G_TYPE_STRING);

	rejilla_data_vfs_signals [UNKNOWN_SIGNAL] = 
	    g_signal_new ("unknown_uri",
			  G_TYPE_FROM_CLASS (klass),
			  G_SIGNAL_RUN_FIRST,
			  0,
			  NULL, NULL,
			  g_cclosure_marshal_VOID__STRING,
			  G_TYPE_NONE,
			  1,
			  G_TYPE_STRING);
}
