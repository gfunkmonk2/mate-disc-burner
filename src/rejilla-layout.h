/***************************************************************************
 *            rejilla-layout.h
 *
 *  mer mai 24 15:14:42 2006
 *  Copyright  2006  Rouquier Philippe
 *  rejilla-app@wanadoo.fr
 ***************************************************************************/

/*
 *  Rejilla is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Rejilla is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifndef REJILLA_LAYOUT_H
#define REJILLA_LAYOUT_H

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib.h>
#include <glib-object.h>

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define REJILLA_TYPE_LAYOUT         (rejilla_layout_get_type ())
#define REJILLA_LAYOUT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), REJILLA_TYPE_LAYOUT, RejillaLayout))
#define REJILLA_LAYOUT_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), REJILLA_TYPE_LAYOUT, RejillaLayoutClass))
#define REJILLA_IS_LAYOUT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), REJILLA_TYPE_LAYOUT))
#define REJILLA_IS_LAYOUT_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), REJILLA_TYPE_LAYOUT))
#define REJILLA_LAYOUT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), REJILLA_TYPE_LAYOUT, RejillaLayoutClass))

typedef struct RejillaLayoutPrivate RejillaLayoutPrivate;

typedef enum {
	REJILLA_LAYOUT_NONE		= 0,
	REJILLA_LAYOUT_AUDIO		= 1,
	REJILLA_LAYOUT_DATA		= 1 << 1,
	REJILLA_LAYOUT_VIDEO		= 1 << 2
} RejillaLayoutType;

typedef struct {
	GtkHPaned parent;
	RejillaLayoutPrivate *priv;
} RejillaLayout;

typedef struct {
	GtkHPanedClass parent_class;
} RejillaLayoutClass;

GType rejilla_layout_get_type (void);
GtkWidget *rejilla_layout_new (void);

void
rejilla_layout_add_project (RejillaLayout *layout,
			    GtkWidget *project);
void
rejilla_layout_add_preview (RejillaLayout*layout,
			    GtkWidget *preview);

void
rejilla_layout_add_source (RejillaLayout *layout,
			   GtkWidget *child,
			   const gchar *id,
			   const gchar *subtitle,
			   const gchar *icon_name,
			   RejillaLayoutType types);
void
rejilla_layout_load (RejillaLayout *layout,
		     RejillaLayoutType type);

void
rejilla_layout_register_ui (RejillaLayout *layout,
			    GtkUIManager *manager);

G_END_DECLS

#endif /* REJILLA_LAYOUT_H */
