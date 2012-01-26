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

#ifndef _REJILLA_PROJECT_NAME_H_
#define _REJILLA_PROJECT_NAME_H_

#include <glib-object.h>

#include <gtk/gtk.h>

#include "rejilla-session.h"

#include "rejilla-project-type-chooser.h"

G_BEGIN_DECLS

#define REJILLA_TYPE_PROJECT_NAME             (rejilla_project_name_get_type ())
#define REJILLA_PROJECT_NAME(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_PROJECT_NAME, RejillaProjectName))
#define REJILLA_PROJECT_NAME_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_PROJECT_NAME, RejillaProjectNameClass))
#define REJILLA_IS_PROJECT_NAME(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_PROJECT_NAME))
#define REJILLA_IS_PROJECT_NAME_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_PROJECT_NAME))
#define REJILLA_PROJECT_NAME_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_PROJECT_NAME, RejillaProjectNameClass))

typedef struct _RejillaProjectNameClass RejillaProjectNameClass;
typedef struct _RejillaProjectName RejillaProjectName;

struct _RejillaProjectNameClass
{
	GtkEntryClass parent_class;
};

struct _RejillaProjectName
{
	GtkEntry parent_instance;
};

GType rejilla_project_name_get_type (void) G_GNUC_CONST;

GtkWidget *
rejilla_project_name_new (RejillaBurnSession *session);

void
rejilla_project_name_set_session (RejillaProjectName *project,
				  RejillaBurnSession *session);

G_END_DECLS

#endif /* _REJILLA_PROJECT_NAME_H_ */
