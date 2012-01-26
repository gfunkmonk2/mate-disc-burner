/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Rejilla
 * Copyright (C) Philippe Rouquier 2005-2009 <bonfire-app@wanadoo.fr>
 * 
 *  Rejilla is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 * rejilla is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with rejilla.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifndef _REJILLA_RENAME_H_
#define _REJILLA_RENAME_H_

#include <glib-object.h>

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef gboolean (*RejillaRenameCallback)	(GtkTreeModel *model,
						 GtkTreeIter *iter,
						 GtkTreePath *treepath,
						 const gchar *old_name,
						 const gchar *new_name);

#define REJILLA_TYPE_RENAME             (rejilla_rename_get_type ())
#define REJILLA_RENAME(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_RENAME, RejillaRename))
#define REJILLA_RENAME_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_RENAME, RejillaRenameClass))
#define REJILLA_IS_RENAME(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_RENAME))
#define REJILLA_IS_RENAME_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_RENAME))
#define REJILLA_RENAME_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_RENAME, RejillaRenameClass))

typedef struct _RejillaRenameClass RejillaRenameClass;
typedef struct _RejillaRename RejillaRename;

struct _RejillaRenameClass
{
	GtkVBoxClass parent_class;
};

struct _RejillaRename
{
	GtkVBox parent_instance;
};

GType rejilla_rename_get_type (void) G_GNUC_CONST;

GtkWidget *
rejilla_rename_new (void);

gboolean
rejilla_rename_do (RejillaRename *self,
		   GtkTreeSelection *selection,
		   guint column_num,
		   RejillaRenameCallback callback);

void
rejilla_rename_set_show_keep_default (RejillaRename *self,
				      gboolean show);

G_END_DECLS

#endif /* _REJILLA_RENAME_H_ */
