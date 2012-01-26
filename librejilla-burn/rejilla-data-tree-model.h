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

#ifndef _REJILLA_DATA_TREE_MODEL_H_
#define _REJILLA_DATA_TREE_MODEL_H_

#include <glib-object.h>

#include "rejilla-data-vfs.h"
#include "rejilla-file-node.h"

G_BEGIN_DECLS

#define REJILLA_TYPE_DATA_TREE_MODEL             (rejilla_data_tree_model_get_type ())
#define REJILLA_DATA_TREE_MODEL(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_DATA_TREE_MODEL, RejillaDataTreeModel))
#define REJILLA_DATA_TREE_MODEL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_DATA_TREE_MODEL, RejillaDataTreeModelClass))
#define REJILLA_IS_DATA_TREE_MODEL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_DATA_TREE_MODEL))
#define REJILLA_IS_DATA_TREE_MODEL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_DATA_TREE_MODEL))
#define REJILLA_DATA_TREE_MODEL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_DATA_TREE_MODEL, RejillaDataTreeModelClass))

typedef struct _RejillaDataTreeModelClass RejillaDataTreeModelClass;
typedef struct _RejillaDataTreeModel RejillaDataTreeModel;

struct _RejillaDataTreeModelClass
{
	RejillaDataVFSClass parent_class;
};

struct _RejillaDataTreeModel
{
	RejillaDataVFS parent_instance;

	/* Signals */
	void		(*row_added)		(RejillaDataTreeModel *model,
						 RejillaFileNode *node);
	void		(*row_changed)		(RejillaDataTreeModel *model,
						 RejillaFileNode *node);
	void		(*row_removed)		(RejillaDataTreeModel *model,
						 RejillaFileNode *former_parent,
						 guint former_position,
						 RejillaFileNode *node);
	void		(*rows_reordered)	(RejillaDataTreeModel *model,
						 RejillaFileNode *parent,
						 guint *new_order);
};

GType rejilla_data_tree_model_get_type (void) G_GNUC_CONST;

RejillaDataTreeModel *
rejilla_data_tree_model_new (void);

G_END_DECLS

#endif /* _REJILLA_DATA_TREE_MODEL_H_ */
