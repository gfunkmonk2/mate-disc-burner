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

#include <gio/gio.h>

#include "rejilla-misc.h"

#include "burn-basics.h"

#include "rejilla-file-node.h"
#include "rejilla-io.h"


RejillaFileNode *
rejilla_file_node_root_new (void)
{
	RejillaFileNode *root;

	root = g_new0 (RejillaFileNode, 1);
	root->is_root = TRUE;
	root->is_imported = TRUE;

	root->union3.stats = g_new0 (RejillaFileTreeStats, 1);
	return root;
}

RejillaFileNode *
rejilla_file_node_get_root (RejillaFileNode *node,
			    guint *depth_retval)
{
	RejillaFileNode *parent;
	guint depth = 0;

	parent = node;
	while (parent) {
		if (parent->is_root) {
			if (depth_retval)
				*depth_retval = depth;

			return parent;
		}

		depth ++;
		parent = parent->parent;
	}

	return NULL;
}


guint
rejilla_file_node_get_depth (RejillaFileNode *node)
{
	guint depth = 0;

	while (node) {
		if (node->is_root)
			return depth;

		depth ++;
		node = node->parent;
	}

	return 0;
}

RejillaFileTreeStats *
rejilla_file_node_get_tree_stats (RejillaFileNode *node,
				  guint *depth)
{
	RejillaFileTreeStats *stats;
	RejillaFileNode *root;

	stats = REJILLA_FILE_NODE_STATS (node);
	if (stats)
		return stats;

	root = rejilla_file_node_get_root (node, depth);
	stats = REJILLA_FILE_NODE_STATS (root);

	return stats;
}

gint
rejilla_file_node_sort_default_cb (gconstpointer obj_a, gconstpointer obj_b)
{
	const RejillaFileNode *a = obj_a;
	const RejillaFileNode *b = obj_b;

	if (a->is_file == b->is_file)
		return 0;

	if (b->is_file)
		return -1;
	
	return 1;
}

gint
rejilla_file_node_sort_name_cb (gconstpointer obj_a, gconstpointer obj_b)
{
	gint res;
	const RejillaFileNode *a = obj_a;
	const RejillaFileNode *b = obj_b;


	res = rejilla_file_node_sort_default_cb (a, b);
	if (res)
		return res;

	return strcmp (REJILLA_FILE_NODE_NAME (a), REJILLA_FILE_NODE_NAME (b));
}

gint
rejilla_file_node_sort_size_cb (gconstpointer obj_a, gconstpointer obj_b)
{
	gint res;
	gint num_a, num_b;
	const RejillaFileNode *a = obj_a;
	const RejillaFileNode *b = obj_b;


	res = rejilla_file_node_sort_default_cb (a, b);
	if (res)
		return res;

	if (a->is_file)
		return REJILLA_FILE_NODE_SECTORS (a) - REJILLA_FILE_NODE_SECTORS (b);

	/* directories */
	num_a = rejilla_file_node_get_n_children (a);
	num_b = rejilla_file_node_get_n_children (b);
	return num_a - num_b;
}

gint 
rejilla_file_node_sort_mime_cb (gconstpointer obj_a, gconstpointer obj_b)
{
	gint res;
	const RejillaFileNode *a = obj_a;
	const RejillaFileNode *b = obj_b;


	res = rejilla_file_node_sort_default_cb (a, b);
	if (res)
		return res;

	return strcmp (REJILLA_FILE_NODE_NAME (a), REJILLA_FILE_NODE_NAME (b));
}

static RejillaFileNode *
rejilla_file_node_insert (RejillaFileNode *head,
			  RejillaFileNode *node,
			  GCompareFunc sort_func,
			  guint *newpos)
{
	RejillaFileNode *iter;
	guint n = 0;

	/* check for some special cases */
	if (!head) {
		node->next = NULL;
		return node;
	}

	/* Set hidden nodes (whether virtual or not) always last */
	if (head->is_hidden) {
		node->next = head;
		if (newpos)
			*newpos = 0;

		return node;
	}

	if (node->is_hidden) {
		iter = head;
		n = 1;
		while (iter->next)  {
			iter = iter->next;
			n ++;
		}

		iter->next = node;

		if (newpos)
			*newpos = n;

		return head;
	}

	/* regular node, regular head node */
	if (sort_func (head, node) > 0) {
		/* head is after node */
		node->next = head;
		if (newpos)
			*newpos = 0;

		return node;
	}

	n = 1;
	for (iter = head; iter->next; iter = iter->next) {
		if (sort_func (iter->next, node) > 0) {
			/* iter->next should be located after node */
			node->next = iter->next;
			iter->next = node;

			if (newpos)
				*newpos = n;

			return head;
		}
		n ++;
	}

	/* append it */
	iter->next = node;
	node->next = NULL;

	if (newpos)
		*newpos = n;

	return head;
}

gint *
rejilla_file_node_need_resort (RejillaFileNode *node,
			       GCompareFunc sort_func)
{
	RejillaFileNode *previous;
	RejillaFileNode *parent;
	RejillaFileNode *head;
	gint *array = NULL;
	guint newpos = 0;
	guint oldpos;
	guint size;

	if (node->is_hidden)
		return NULL;

	parent = node->parent;
	head = REJILLA_FILE_NODE_CHILDREN (parent);

	/* find previous node and get old position */
	if (head != node) {
		previous = head;
		oldpos = 0;
		while (previous->next != node) {
			previous = previous->next;
			oldpos ++;
		}
		oldpos ++;
	}
	else {
		previous = NULL;
		oldpos = 0;
	}

	/* see where we should start from head or from node->next */
	if (previous && sort_func (previous, node) > 0) {
		gint i;

		/* move on the left */

		previous->next = node->next;

		head = rejilla_file_node_insert (head, node, sort_func, &newpos);
		parent->union2.children = head;

		/* create an array to reflect the changes */
		/* NOTE: hidden nodes are not taken into account. */
		size = rejilla_file_node_get_n_children (parent);
		array = g_new0 (gint, size);

		for (i = 0; i < size; i ++) {
			if (i == newpos)
				array [i] = oldpos;
			else if (i > newpos && i <= oldpos)
				array [i] = i - 1;
			else
				array [i] = i;
		}
	}
	/* Hidden nodes stay at the end, hence the !node->next->is_hidden */
	else if (node->next && !node->next->is_hidden && sort_func (node, node->next) > 0) {
		gint i;

		/* move on the right */

		if (previous)
			previous->next = node->next;
		else
			parent->union2.children = node->next;

		/* NOTE: here we're sure head hasn't changed since we checked 
		 * that node should go after node->next (given as head for the
		 * insertion here) */
		rejilla_file_node_insert (node->next, node, sort_func, &newpos);

		/* we started from oldpos so newpos needs updating */
		newpos += oldpos;

		/* create an array to reflect the changes. */
		/* NOTE: hidden nodes are not taken into account. */
		size = rejilla_file_node_get_n_children (parent);
		array = g_new0 (gint, size);

		for (i = 0; i < size; i ++) {
			if (i == newpos)
				array [i] = oldpos;
			else if (i >= oldpos && i < newpos)
				array [i] = i + 1;
			else
				array [i] = i;
		}
	}

	return array;
}

gint *
rejilla_file_node_sort_children (RejillaFileNode *parent,
				 GCompareFunc sort_func)
{
	RejillaFileNode *new_order = NULL;
	RejillaFileNode *iter;
	RejillaFileNode *next;
	gint *array = NULL;
	gint num_children;
	guint oldpos = 1;
	guint newpos;

	/* check for some special cases */
	if (parent->is_hidden)
		return NULL;

	new_order = REJILLA_FILE_NODE_CHILDREN (parent);
	if (!new_order)
		return NULL;

	if (!new_order->next)
		return NULL;

	/* make the array */
	num_children = rejilla_file_node_get_n_children (parent);
	array = g_new (gint, num_children);

	next = new_order->next;
	new_order->next = NULL;
	array [0] = 0;

	for (iter = next; iter; iter = next, oldpos ++) {
		/* unlink iter */
		next = iter->next;
		iter->next = NULL;

		newpos = 0;
		new_order = rejilla_file_node_insert (new_order,
						      iter,
						      sort_func,
						      &newpos);

		if (newpos < oldpos)
			memmove (array + newpos + 1, array + newpos, (oldpos - newpos) * sizeof (guint));

		array [newpos] = oldpos;
	}

	/* set the new order */
	parent->union2.children = new_order;

	return array;
}

gint *
rejilla_file_node_reverse_children (RejillaFileNode *parent)
{
	RejillaFileNode *previous;
	RejillaFileNode *last;
	RejillaFileNode *iter;
	gint firstfile = 0;
	gint *array;
	gint size;
	gint i;
	
	/* when reversing the list of children the only thing we must pay 
	 * attention to is to keep directories first; so we do it in two passes
	 * first order the directories and then the files */
	last = REJILLA_FILE_NODE_CHILDREN (parent);

	/* special case */
	if (!last || !last->next)
		return NULL;

	previous = last;
	iter = last->next;
	size = 1;

	if (!last->is_file) {
		while (!iter->is_file) {
			RejillaFileNode *next;

			next = iter->next;
			iter->next = previous;

			size ++;
			if (!next) {
				/* No file afterwards */
				parent->union2.children = iter;
				last->next = NULL;
				firstfile = size;
				goto end;
			}

			previous = iter;
			iter = next;
		}

		/* the new head is the last processed node */
		parent->union2.children = previous;
		firstfile = size;

		previous = iter;
		iter = iter->next;
		previous->next = NULL;
	}

	while (iter) {
		RejillaFileNode *next;

		next = iter->next;
		iter->next = previous;

		size ++;

		previous = iter;
		iter = next;
	}

	/* NOTE: iter is NULL here */
	if (last->is_file) {
		last->next = NULL;
		parent->union2.children = previous;
	}
	else
		last->next = previous;

end:

	array = g_new (gint, size);

	for (i = 0; i < firstfile; i ++)
		array [i] = firstfile - i - 1;

	for (i = firstfile; i < size; i ++)
		array [i] = size - i + firstfile - 1;

	return array;
}

RejillaFileNode *
rejilla_file_node_nth_child (RejillaFileNode *parent,
			     guint nth)
{
	RejillaFileNode *peers;
	guint pos;

	if (!parent)
		return NULL;

	peers = REJILLA_FILE_NODE_CHILDREN (parent);
	for (pos = 0; pos < nth && peers; pos ++)
		peers = peers->next;

	return peers;
}

guint
rejilla_file_node_get_n_children (const RejillaFileNode *node)
{
	RejillaFileNode *children;
	guint num = 0;

	if (!node)
		return 0;

	for (children = REJILLA_FILE_NODE_CHILDREN (node); children; children = children->next) {
		if (children->is_hidden)
			continue;
		num ++;
	}

	return num;
}

guint
rejilla_file_node_get_pos_as_child (RejillaFileNode *node)
{
	RejillaFileNode *parent;
	RejillaFileNode *peers;
	guint pos = 0;

	if (!node)
		return 0;

	parent = node->parent;
	for (peers = REJILLA_FILE_NODE_CHILDREN (parent); peers; peers = peers->next) {
		if (peers == node)
			break;
		pos ++;
	}

	return pos;
}

gboolean
rejilla_file_node_is_ancestor (RejillaFileNode *parent,
			       RejillaFileNode *node)
{
	while (node && node != parent)
		node = node->parent;

	if (!node)
		return FALSE;

	return TRUE;
}

RejillaFileNode *
rejilla_file_node_check_name_existence_case (RejillaFileNode *parent,
					     const gchar *name)
{
	RejillaFileNode *iter;

	if (name && name [0] == '\0')
		return NULL;

	iter = REJILLA_FILE_NODE_CHILDREN (parent);
	for (; iter; iter = iter->next) {
		if (!strcasecmp (name, REJILLA_FILE_NODE_NAME (iter)))
			return iter;
	}

	return NULL;
}

RejillaFileNode *
rejilla_file_node_check_name_existence (RejillaFileNode *parent,
				        const gchar *name)
{
	RejillaFileNode *iter;

	if (name && name [0] == '\0')
		return NULL;

	iter = REJILLA_FILE_NODE_CHILDREN (parent);
	for (; iter; iter = iter->next) {
		if (!strcmp (name, REJILLA_FILE_NODE_NAME (iter)))
			return iter;
	}

	return NULL;
}

RejillaFileNode *
rejilla_file_node_get_from_path (RejillaFileNode *root,
				 const gchar *path)
{
	gchar **array;
	gchar **iter;

	if (!path)
		return NULL;

	/* If we don't do that array[0] == '\0' */
	if (g_str_has_prefix (path, G_DIR_SEPARATOR_S))
		array = g_strsplit (path + 1, G_DIR_SEPARATOR_S, 0);
	else
		array = g_strsplit (path, G_DIR_SEPARATOR_S, 0);

	if (!array)
		return NULL;

	for (iter = array; iter && *iter; iter++) {
		root = rejilla_file_node_check_name_existence (root, *iter);
		if (!root)
			break;
	}
	g_strfreev (array);

	return root;
}

RejillaFileNode *
rejilla_file_node_check_imported_sibling (RejillaFileNode *node)
{
	RejillaFileNode *parent;
	RejillaFileNode *iter;
	RejillaImport *import;

	parent = node->parent;

	/* That could happen if a node is moved to a location where another node
	 * (to be removed) has the same name and is a parent of this node */
	if (!parent)
		return NULL;

	/* See if among the imported children of the parent one of them
	 * has the same name as the node being removed. If so, restore
	 * it with all its imported children (provided that's a
	 * directory). */
	import = REJILLA_FILE_NODE_IMPORT (parent);
	if (!import)
		return NULL;

	iter = import->replaced;
	if (!strcmp (REJILLA_FILE_NODE_NAME (iter), REJILLA_FILE_NODE_NAME (node))) {
		/* A match, remove it from the list and return it */
		import->replaced = iter->next;
		if (!import->replaced) {
			/* no more imported saved import structure */
			parent->union1.name = import->name;
			parent->has_import = FALSE;
			g_free (import);
		}

		iter->next = NULL;
		return iter;			
	}

	for (; iter->next; iter = iter->next) {
		if (!strcmp (REJILLA_FILE_NODE_NAME (iter->next), REJILLA_FILE_NODE_NAME (node))) {
			RejillaFileNode *removed;
			/* There is one match, remove it from the list */
			removed = iter->next;
			iter->next = removed->next;
			removed->next = NULL;
			return removed;
		}
	}

	return NULL;
}

void
rejilla_file_node_graft (RejillaFileNode *file_node,
			 RejillaURINode *uri_node)
{
	RejillaGraft *graft;

	if (!file_node->is_grafted) {
		RejillaFileNode *parent;

		graft = g_new (RejillaGraft, 1);
		graft->name = file_node->union1.name;
		file_node->union1.graft = graft;
		file_node->is_grafted = TRUE;

		/* since it wasn't grafted propagate the size change; that is
		 * substract the current node size from the parent nodes until
		 * the parent graft point. */
		for (parent = file_node->parent; parent && !parent->is_root; parent = parent->parent) {
			parent->union3.sectors -= REJILLA_FILE_NODE_SECTORS (file_node);
			if (parent->is_grafted)
				break;
		}
	}
	else {
		RejillaURINode *old_uri_node;

		graft = REJILLA_FILE_NODE_GRAFT (file_node);
		old_uri_node = graft->node;
		if (old_uri_node == uri_node)
			return;

		old_uri_node->nodes = g_slist_remove (old_uri_node->nodes, file_node);
	}

	graft->node = uri_node;
	uri_node->nodes = g_slist_prepend (uri_node->nodes, file_node);
}

void
rejilla_file_node_ungraft (RejillaFileNode *node)
{
	RejillaGraft *graft;
	RejillaFileNode *parent;

	if (!node->is_grafted)
		return;

	graft = node->union1.graft;

	/* Remove it from the URINode list of grafts */
	graft->node->nodes = g_slist_remove (graft->node->nodes, node);

	/* The name must be exactly the one of the URI */
	node->is_grafted = FALSE;
	node->union1.name = graft->name;

	/* Removes the graft */
	g_free (graft);

	/* Propagate the size change up the parents to the next
	 * grafted parent in the tree (if any). */
	for (parent = node->parent; parent && !parent->is_root; parent = parent->parent) {
		parent->union3.sectors += REJILLA_FILE_NODE_SECTORS (node);
		if (parent->is_grafted)
			break;
	}
}

void
rejilla_file_node_rename (RejillaFileNode *node,
			  const gchar *name)
{
	g_free (REJILLA_FILE_NODE_NAME (node));
	if (node->is_grafted)
		node->union1.graft->name = g_strdup (name);
	else
		node->union1.name = g_strdup (name);
}

void
rejilla_file_node_add (RejillaFileNode *parent,
		       RejillaFileNode *node,
		       GCompareFunc sort_func)
{
	RejillaFileTreeStats *stats;
	guint depth = 0;

	parent->union2.children = rejilla_file_node_insert (REJILLA_FILE_NODE_CHILDREN (parent),
							    node,
							    sort_func,
							    NULL);
	node->parent = parent;

	if (REJILLA_FILE_NODE_VIRTUAL (node))
		return;

	stats = rejilla_file_node_get_tree_stats (node->parent, &depth);
	if (!node->is_imported) {
		/* book keeping */
		if (!node->is_file)
			stats->num_dir ++;
		else
			stats->children ++;

		/* NOTE: parent will be changed afterwards !!! */
		if (!node->is_grafted) {
			/* propagate the size change*/
			for (; parent && !parent->is_root; parent = parent->parent) {
				parent->union3.sectors += REJILLA_FILE_NODE_SECTORS (node);
				if (parent->is_grafted)
					break;
			}
		}
	}

	/* Even imported should be included. The only type of nodes that are not
	 * heeded are the virtual nodes. */
	if (node->is_file) {
		if (depth < 6)
			return;
	}
	else if (depth < 5)
		return;

	stats->num_deep ++;
	node->is_deep = TRUE;
}

void
rejilla_file_node_set_from_info (RejillaFileNode *node,
				 RejillaFileTreeStats *stats,
				 GFileInfo *info)
{
	/* NOTE: the name will never be replaced here since that means
	 * we could replace a previously set name (that triggered the
	 * creation of a graft). If someone wants to set a new name,
	 * then rename_node is the function. */

	if (node->parent) {
		/* update the stats since a file could have been added to the tree but
		 * at this point we didn't know what it was (a file or a directory).
		 * Only do this if it wasn't a file before.
		 * Do this only if it's in the tree. */
		if (!node->is_file && (g_file_info_get_file_type (info) != G_FILE_TYPE_DIRECTORY)) {
			stats->children ++;
			stats->num_dir --;
		}
		else if (node->is_file && (g_file_info_get_file_type (info) == G_FILE_TYPE_DIRECTORY)) {
			stats->children --;
			stats->num_dir ++;
		}
	}

	if (!node->is_symlink
	&& (g_file_info_get_file_type (info) == G_FILE_TYPE_SYMBOLIC_LINK)) {
		/* only count files */
		stats->num_sym ++;
	}

	/* update :
	 * - the mime type
	 * - the size (and possibly the one of his parent)
	 * - the type */
	node->is_file = (g_file_info_get_file_type (info) != G_FILE_TYPE_DIRECTORY);
	node->is_fake = FALSE;
	node->is_loading = FALSE;
	node->is_imported = FALSE;
	node->is_reloading = FALSE;
	node->is_symlink = (g_file_info_get_file_type (info) == G_FILE_TYPE_SYMBOLIC_LINK);

	if (node->is_file) {
		guint sectors;
		gint sectors_diff;

		/* register mime type string */
		if (g_file_info_has_attribute (info, G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE)) {
			const gchar *mime;

			if (REJILLA_FILE_NODE_MIME (node))
				rejilla_utils_unregister_string (REJILLA_FILE_NODE_MIME (node));

			mime = g_file_info_get_content_type (info);
			node->union2.mime = rejilla_utils_register_string (mime);
		}

		sectors = REJILLA_BYTES_TO_SECTORS (g_file_info_get_size (info), 2048);

		if (sectors > REJILLA_FILE_2G_LIMIT && REJILLA_FILE_NODE_SECTORS (node) <= REJILLA_FILE_2G_LIMIT) {
			node->is_2GiB = 1;
			stats->num_2GiB ++;
		}
		else if (sectors <= REJILLA_FILE_2G_LIMIT && REJILLA_FILE_NODE_SECTORS (node) > REJILLA_FILE_2G_LIMIT) {
			node->is_2GiB = 0;
			stats->num_2GiB --;
		}

		/* The node isn't grafted and it's a file. So we must propagate
		 * its size up to the parent graft node. */
		/* NOTE: we used to accumulate all the directory contents till
		 * the end and process all of entries at once, when it was
		 * finished. We had to do that to calculate the whole size. */
		sectors_diff = sectors - REJILLA_FILE_NODE_SECTORS (node);
		for (; node; node = node->parent) {
			node->union3.sectors += sectors_diff;
			if (node->is_grafted)
				break;
		}
	}
	else	/* since that's directory then it must be explored now */
		node->is_exploring = TRUE;
}

RejillaFileNode *
rejilla_file_node_new_loading (const gchar *name)
{
	RejillaFileNode *node;

	node = g_new0 (RejillaFileNode, 1);
	node->union1.name = g_strdup (name);
	node->is_loading = TRUE;

	return node;
}

RejillaFileNode *
rejilla_file_node_new_virtual (const gchar *name)
{
	RejillaFileNode *node;

	/* virtual nodes are nodes that "don't exist". They appear as temporary
	 * parents (and therefore replacable) and hidden (not displayed in the
	 * GtkTreeModel). They are used as 'placeholders' to trigger
	 * name-collision signal. */
	node = g_new0 (RejillaFileNode, 1);
	node->union1.name = g_strdup (name);
	node->is_fake = TRUE;
	node->is_hidden = TRUE;

	return node;
}

RejillaFileNode *
rejilla_file_node_new (const gchar *name)
{
	RejillaFileNode *node;

	node = g_new0 (RejillaFileNode, 1);
	node->union1.name = g_strdup (name);

	return node;
}

RejillaFileNode *
rejilla_file_node_new_imported_session_file (GFileInfo *info)
{
	RejillaFileNode *node;

	/* Create the node information */
	node = g_new0 (RejillaFileNode, 1);
	node->union1.name = g_strdup (g_file_info_get_name (info));
	node->is_file = (g_file_info_get_file_type (info) != G_FILE_TYPE_DIRECTORY);
	node->is_imported = TRUE;

	if (!node->is_file) {
		node->is_fake = TRUE;
		node->union3.imported_address = g_file_info_get_attribute_int64 (info, REJILLA_IO_DIR_CONTENTS_ADDR);
	}
	else
		node->union3.sectors = REJILLA_BYTES_TO_SECTORS (g_file_info_get_size (info), 2048);

	return node;
}

RejillaFileNode *
rejilla_file_node_new_empty_folder (const gchar *name)
{
	RejillaFileNode *node;

	/* Create the node information */
	node = g_new0 (RejillaFileNode, 1);
	node->union1.name = g_strdup (name);
	node->is_fake = TRUE;

	return node;
}

void
rejilla_file_node_unlink (RejillaFileNode *node)
{
	RejillaFileNode *iter;
	RejillaImport *import;

	if (!node->parent)
		return;

	iter = REJILLA_FILE_NODE_CHILDREN (node->parent);

	/* handle the size change for previous parent */
	if (!node->is_grafted
	&&  !node->is_imported
	&&  !REJILLA_FILE_NODE_VIRTUAL (node)) {
		RejillaFileNode *parent;

		/* handle the size change if it wasn't grafted */
		for (parent = node->parent; parent && !parent->is_root; parent = parent->parent) {
			parent->union3.sectors -= REJILLA_FILE_NODE_SECTORS (node);
			if (parent->is_grafted)
				break;
		}
	}

	node->is_deep = FALSE;

	if (iter == node) {
		node->parent->union2.children = node->next;
		node->parent = NULL;
		node->next = NULL;
		return;
	}

	for (; iter->next; iter = iter->next) {
		if (iter->next == node) {
			iter->next = node->next;
			node->parent = NULL;
			node->next = NULL;
			return;
		}
	}

	if (!node->is_imported || !node->parent->has_import)
		return;

	/* It wasn't found among the parent children. If parent is imported and
	 * the node is imported as well then check if it isn't among the import
	 * children */
	import = REJILLA_FILE_NODE_IMPORT (node->parent);
	iter = import->replaced;

	if (iter == node) {
		import->replaced = iter->next;
		node->parent = NULL;
		node->next = NULL;
		return;
	}

	for (; iter->next; iter = iter->next) {
		if (iter->next == node) {
			iter->next = node->next;
			node->parent = NULL;
			node->next = NULL;
			return;
		}
	}
}

void
rejilla_file_node_move_from (RejillaFileNode *node,
			     RejillaFileTreeStats *stats)
{
	/* NOTE: for the time being no backend supports moving imported files */
	if (node->is_imported)
		return;

	if (node->is_deep)
		stats->num_deep --;

	rejilla_file_node_unlink (node);
}

void
rejilla_file_node_move_to (RejillaFileNode *node,
			   RejillaFileNode *parent,
			   GCompareFunc sort_func)
{
	RejillaFileTreeStats *stats;
	guint depth = 0;

	/* NOTE: for the time being no backend supports moving imported files */
	if (node->is_imported)
		return;

	/* reinsert it now at the new location */
	parent->union2.children = rejilla_file_node_insert (REJILLA_FILE_NODE_CHILDREN (parent),
							    node,
							    sort_func,
							    NULL);
	node->parent = parent;

	if (!node->is_grafted) {
		RejillaFileNode *parent;

		/* propagate the size change for new parent */
		for (parent = node->parent; parent && !parent->is_root; parent = parent->parent) {
			parent->union3.sectors += REJILLA_FILE_NODE_SECTORS (node);
			if (parent->is_grafted)
				break;
		}
	}

	/* NOTE: here stats about the tree can change if the parent has a depth
	 * > 6 and if previous didn't. Other stats remains unmodified. */
	stats = rejilla_file_node_get_tree_stats (node->parent, &depth);
	if (node->is_file) {
		if (depth < 6)
			return;
	}
	else if (depth < 5)
		return;

	stats->num_deep ++;
	node->is_deep = TRUE;
}

static void
rejilla_file_node_destroy_with_children (RejillaFileNode *node,
					 RejillaFileTreeStats *stats)
{
	RejillaFileNode *child;
	RejillaFileNode *next;
	RejillaImport *import;
	RejillaGraft *graft;

	/* destroy all children recursively */
	for (child = REJILLA_FILE_NODE_CHILDREN (node); child; child = next) {
		next = child->next;
		rejilla_file_node_destroy_with_children (child, stats);
	}

	/* update all statistics on tree if any */
	if (!REJILLA_FILE_NODE_VIRTUAL (node) && stats) {
		/* check if that's a 2 GiB file */
		if (node->is_2GiB)
			stats->num_2GiB --;

		/* check if that's a deep directory file */
		if (node->is_deep)
			stats->num_deep --;

		/* check if that's a symlink */
		if (node->is_symlink)
			stats->num_sym --;

		/* update file/directory number statistics */
		if (!node->is_imported) {
			if (node->is_file)
				stats->children --;
			else
				stats->num_dir --;
		}
	}

	/* destruction */
	import = REJILLA_FILE_NODE_IMPORT (node);
	graft = REJILLA_FILE_NODE_GRAFT (node);
	if (graft) {
		RejillaURINode *uri_node;

		uri_node = graft->node;

		/* Handle removal from RejillaURINode struct */
		if (uri_node)
			uri_node->nodes = g_slist_remove (uri_node->nodes, node);

		g_free (graft->name);
		g_free (graft);
	}
	else if (import) {
		/* if imported then destroy the saved children */
		for (child = import->replaced; child; child = next) {
			next = child->next;
			rejilla_file_node_destroy_with_children (child, stats);
		}

		g_free (import->name);
		g_free (import);
	}
	else
		g_free (REJILLA_FILE_NODE_NAME (node));

	/* destroy the node */
	if (node->is_file && !node->is_imported && REJILLA_FILE_NODE_MIME (node))
		rejilla_utils_unregister_string (REJILLA_FILE_NODE_MIME (node));

	if (node->is_root)
		g_free (REJILLA_FILE_NODE_STATS (node));

	g_free (node);
}

/**
 * Destroy a node and its children updating the tree stats.
 * If it isn't unlinked yet, it does it.
 */
void
rejilla_file_node_destroy (RejillaFileNode *node,
			   RejillaFileTreeStats *stats)
{
	/* remove from the parent children list or more probably from the 
	 * import list. */
	if (node->parent)
		rejilla_file_node_unlink (node);

	/* traverse the whole tree and free children updating tree stats */
	rejilla_file_node_destroy_with_children (node, stats);
}

/**
 * Pre-remove function that unparent a node (before a possible destruction).
 * If node is imported, it saves it in its parent, destroys all child nodes
 * that are not imported and restore children that were imported.
 * NOTE: tree stats are only updated if the node is imported.
 */

static void
rejilla_file_node_save_imported_children (RejillaFileNode *node,
					  RejillaFileTreeStats *stats,
					  GCompareFunc sort_func)
{
	RejillaFileNode *iter;
	RejillaImport *import;

	/* clean children */
	for (iter = REJILLA_FILE_NODE_CHILDREN (node); iter; iter = iter->next) {
		if (!iter->is_imported)
			rejilla_file_node_destroy_with_children (iter, stats);

		if (!iter->is_file)
			rejilla_file_node_save_imported_children (iter, stats, sort_func);
	}

	/* restore all replaced children */
	import = REJILLA_FILE_NODE_IMPORT (node);
	if (!import)
		return;

	for (iter = import->replaced; iter; iter = iter->next)
		rejilla_file_node_insert (iter, node, sort_func, NULL);

	/* remove import */
	node->union1.name = import->name;
	node->has_import = FALSE;
	g_free (import);
}

void
rejilla_file_node_save_imported (RejillaFileNode *node,
				 RejillaFileTreeStats *stats,
				 RejillaFileNode *parent,
				 GCompareFunc sort_func)
{
	RejillaImport *import;

	/* if it isn't imported return */
	if (!node->is_imported)
		return;

	/* Remove all the children that are not imported. Also restore
	 * all children that were replaced so as to restore the original
	 * order of files. */

	/* that shouldn't happen since root itself is considered imported */
	if (!parent || !parent->is_imported)
		return;

	/* save the node in its parent import structure */
	import = REJILLA_FILE_NODE_IMPORT (parent);
	if (!import) {
		import = g_new0 (RejillaImport, 1);
		import->name = REJILLA_FILE_NODE_NAME (parent);
		parent->union1.import = import;
		parent->has_import = TRUE;
	}

	/* unlink it and add it to the list */
	rejilla_file_node_unlink (node);
	node->next = import->replaced;
	import->replaced = node;
	node->parent = parent;

	/* Explore children and remove not imported ones and restore.
	 * Update the tree stats at the same time.
	 * NOTE: here the tree stats are only used for the grafted children that
	 * are not imported in the tree. */
	rejilla_file_node_save_imported_children (node, stats, sort_func);
}
