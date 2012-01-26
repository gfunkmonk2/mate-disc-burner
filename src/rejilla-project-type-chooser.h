/***************************************************************************
*            cd-type-chooser.h
*
*  ven mai 27 17:33:12 2005
*  Copyright  2005  Philippe Rouquier
*  rejilla-app@wanadoo.fr
****************************************************************************/

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

#ifndef CD_TYPE_CHOOSER_H
#define CD_TYPE_CHOOSER_H

#include <glib.h>
#include <glib-object.h>

#include <gtk/gtk.h>

#include "rejilla-project-parse.h"

G_BEGIN_DECLS

#define REJILLA_TYPE_PROJECT_TYPE_CHOOSER         (rejilla_project_type_chooser_get_type ())
#define REJILLA_PROJECT_TYPE_CHOOSER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), REJILLA_TYPE_PROJECT_TYPE_CHOOSER, RejillaProjectTypeChooser))
#define REJILLA_PROJECT_TYPE_CHOOSER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k),REJILLA_TYPE_PROJECT_TYPE_CHOOSER, RejillaProjectTypeChooserClass))
#define REJILLA_IS_PROJECT_TYPE_CHOOSER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), REJILLA_TYPE_PROJECT_TYPE_CHOOSER))
#define REJILLA_IS_PROJECT_TYPE_CHOOSER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), REJILLA_TYPE_PROJECT_TYPE_CHOOSER))
#define REJILLA_PROJECT_TYPE_CHOOSER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), REJILLA_TYPE_PROJECT_TYPE_CHOOSER, RejillaProjectTypeChooserClass))

typedef struct RejillaProjectTypeChooserPrivate RejillaProjectTypeChooserPrivate;

typedef struct {
	GtkEventBox parent;
	RejillaProjectTypeChooserPrivate *priv;
} RejillaProjectTypeChooser;

typedef struct {
	GtkEventBoxClass parent_class;

	void	(*last_saved_clicked)	(RejillaProjectTypeChooser *chooser,
					 const gchar *path);
	void	(*recent_clicked)	(RejillaProjectTypeChooser *chooser,
					 const gchar *uri);
	void	(*chosen)		(RejillaProjectTypeChooser *chooser,
					 RejillaProjectType type);
} RejillaProjectTypeChooserClass;

GType rejilla_project_type_chooser_get_type (void);
GtkWidget *rejilla_project_type_chooser_new (void);

G_END_DECLS

G_END_DECLS

#endif				/* CD_TYPE_CHOOSER_H */
