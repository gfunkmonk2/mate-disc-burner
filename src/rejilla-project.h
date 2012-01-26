/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/***************************************************************************
 *            project.h
 *
 *  mar nov 29 09:32:17 2005
 *  Copyright  2005  Rouquier Philippe
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

#ifndef PROJECT_H
#define PROJECT_H

#include <glib.h>
#include <glib-object.h>

#include <gtk/gtk.h>

#include "rejilla-session-cfg.h"

#include "rejilla-disc.h"
#include "rejilla-uri-container.h"
#include "rejilla-project-type-chooser.h"
#include "rejilla-jacket-edit.h"

G_BEGIN_DECLS

#define REJILLA_TYPE_PROJECT         (rejilla_project_get_type ())
#define REJILLA_PROJECT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), REJILLA_TYPE_PROJECT, RejillaProject))
#define REJILLA_PROJECT_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), REJILLA_TYPE_PROJECT, RejillaProjectClass))
#define REJILLA_IS_PROJECT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), REJILLA_TYPE_PROJECT))
#define REJILLA_IS_PROJECT_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), REJILLA_TYPE_PROJECT))
#define REJILLA_PROJECT_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), REJILLA_TYPE_PROJECT, RejillaProjectClass))

typedef struct RejillaProjectPrivate RejillaProjectPrivate;

typedef struct {
	GtkVBox parent;
	RejillaProjectPrivate *priv;
} RejillaProject;

typedef struct {
	GtkVBoxClass parent_class;

	void	(*add_pressed)	(RejillaProject *project);
} RejillaProjectClass;

GType rejilla_project_get_type (void);
GtkWidget *rejilla_project_new (void);

RejillaBurnResult
rejilla_project_confirm_switch (RejillaProject *project,
				gboolean keep_files);

void
rejilla_project_set_audio (RejillaProject *project);
void
rejilla_project_set_data (RejillaProject *project);
void
rejilla_project_set_video (RejillaProject *project);
void
rejilla_project_set_none (RejillaProject *project);

void
rejilla_project_set_source (RejillaProject *project,
			    RejillaURIContainer *source);

RejillaProjectType
rejilla_project_convert_to_data (RejillaProject *project);

RejillaProjectType
rejilla_project_convert_to_stream (RejillaProject *project,
				   gboolean is_video);

RejillaProjectType
rejilla_project_open_session (RejillaProject *project,
			      RejillaSessionCfg *session);

gboolean
rejilla_project_save_project (RejillaProject *project);
gboolean
rejilla_project_save_project_as (RejillaProject *project);

gboolean
rejilla_project_save_session (RejillaProject *project,
			      const gchar *uri,
			      gchar **saved_uri,
			      gboolean show_cancel);

void
rejilla_project_register_ui (RejillaProject *project,
			     GtkUIManager *manager);

void
rejilla_project_create_audio_cover (RejillaProject *project);

G_END_DECLS

#endif /* PROJECT_H */
