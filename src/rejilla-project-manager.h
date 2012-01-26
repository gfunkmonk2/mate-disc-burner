/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/***************************************************************************
 *            rejilla-project-manager.h
 *
 *  mer mai 24 14:22:56 2006
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

#ifndef REJILLA_PROJECT_MANAGER_H
#define REJILLA_PROJECT_MANAGER_H

#include <glib.h>
#include <glib-object.h>

#include <gtk/gtk.h>

#include "rejilla-medium.h"
#include "rejilla-project-parse.h"
#include "rejilla-project-type-chooser.h"
#include "rejilla-session-cfg.h"

G_BEGIN_DECLS

#define REJILLA_TYPE_PROJECT_MANAGER         (rejilla_project_manager_get_type ())
#define REJILLA_PROJECT_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), REJILLA_TYPE_PROJECT_MANAGER, RejillaProjectManager))
#define REJILLA_PROJECT_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), REJILLA_TYPE_PROJECT_MANAGER, RejillaProjectManagerClass))
#define REJILLA_IS_PROJECT_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), REJILLA_TYPE_PROJECT_MANAGER))
#define REJILLA_IS_PROJECT_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), REJILLA_TYPE_PROJECT_MANAGER))
#define REJILLA_PROJECT_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), REJILLA_TYPE_PROJECT_MANAGER, RejillaProjectManagerClass))

typedef struct RejillaProjectManagerPrivate RejillaProjectManagerPrivate;

typedef struct {
	GtkNotebook parent;
	RejillaProjectManagerPrivate *priv;
} RejillaProjectManager;

typedef struct {
	GtkNotebookClass parent_class;	
} RejillaProjectManagerClass;

GType rejilla_project_manager_get_type (void);
GtkWidget *rejilla_project_manager_new (void);

gboolean
rejilla_project_manager_open_session (RejillaProjectManager *manager,
                                      RejillaSessionCfg *session);

void
rejilla_project_manager_empty (RejillaProjectManager *manager);

/**
 * returns the path of the project that was saved. NULL otherwise.
 */

gboolean
rejilla_project_manager_save_session (RejillaProjectManager *manager,
				      const gchar *path,
				      gchar **saved_uri,
				      gboolean cancellable);

void
rejilla_project_manager_register_ui (RejillaProjectManager *manager,
				     GtkUIManager *ui_manager);

void
rejilla_project_manager_switch (RejillaProjectManager *manager,
				RejillaProjectType type,
				gboolean reset);

G_END_DECLS

#endif /* REJILLA_PROJECT_MANAGER_H */
