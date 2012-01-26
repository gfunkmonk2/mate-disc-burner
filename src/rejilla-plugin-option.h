/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * rejilla
 * Copyright (C) Philippe Rouquier 2007-2008 <bonfire-app@wanadoo.fr>
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

#ifndef _REJILLA_PLUGIN_OPTION_H_
#define _REJILLA_PLUGIN_OPTION_H_

#include <glib-object.h>

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define REJILLA_TYPE_PLUGIN_OPTION             (rejilla_plugin_option_get_type ())
#define REJILLA_PLUGIN_OPTION(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_PLUGIN_OPTION, RejillaPluginOption))
#define REJILLA_PLUGIN_OPTION_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_PLUGIN_OPTION, RejillaPluginOptionClass))
#define REJILLA_IS_PLUGIN_OPTION(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_PLUGIN_OPTION))
#define REJILLA_IS_PLUGIN_OPTION_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_PLUGIN_OPTION))
#define REJILLA_PLUGIN_OPTION_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_PLUGIN_OPTION, RejillaPluginOptionClass))

typedef struct _RejillaPluginOptionClass RejillaPluginOptionClass;
typedef struct _RejillaPluginOption RejillaPluginOption;

struct _RejillaPluginOptionClass
{
	GtkDialogClass parent_class;
};

struct _RejillaPluginOption
{
	GtkDialog parent_instance;
};

GType rejilla_plugin_option_get_type (void) G_GNUC_CONST;

GtkWidget *
rejilla_plugin_option_new (void);

void
rejilla_plugin_option_set_plugin (RejillaPluginOption *dialog,
				  RejillaPlugin *plugin);

G_END_DECLS

#endif /* _REJILLA_PLUGIN_OPTION_H_ */
