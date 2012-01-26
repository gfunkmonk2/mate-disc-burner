/*
 * rejilla-plugin-manager.h
 * This file is part of rejilla
 *
 * Copyright (C) 2007 Philippe Rouquier
 *
 * Based on rejilla code (rejilla/rejilla-plugin-manager.c) by: 
 * 	- Paolo Maggi <paolo@mate.org>
 *
 * Librejilla-media is free software; you can redistribute it and/or modify
fy
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Rejilla is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifndef __REJILLA_PLUGIN_MANAGER_UI_H__
#define __REJILLA_PLUGIN_MANAGER_UI_H__

#include <gtk/gtk.h>

G_BEGIN_DECLS

/*
 * Type checking and casting macros
 */
#define REJILLA_TYPE_PLUGIN_MANAGER_UI              (rejilla_plugin_manager_ui_get_type())
#define REJILLA_PLUGIN_MANAGER_UI(obj)              (G_TYPE_CHECK_INSTANCE_CAST((obj), REJILLA_TYPE_PLUGIN_MANAGER_UI, RejillaPluginManagerUI))
#define REJILLA_PLUGIN_MANAGER_UI_CLASS(klass)      (G_TYPE_CHECK_CLASS_CAST((klass), REJILLA_TYPE_PLUGIN_MANAGER_UI, RejillaPluginManagerUIClass))
#define REJILLA_IS_PLUGIN_MANAGER_UI(obj)           (G_TYPE_CHECK_INSTANCE_TYPE((obj), REJILLA_TYPE_PLUGIN_MANAGER_UI))
#define REJILLA_IS_PLUGIN_MANAGER_UI_CLASS(klass)   (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_PLUGIN_MANAGER_UI))
#define REJILLA_PLUGIN_MANAGER_UI_GET_CLASS(obj)    (G_TYPE_INSTANCE_GET_CLASS((obj), REJILLA_TYPE_PLUGIN_MANAGER_UI, RejillaPluginManagerUIClass))

/* Private structure type */
typedef struct _RejillaPluginManagerUIPrivate RejillaPluginManagerUIPrivate;

/*
 * Main object structure
 */
typedef struct _RejillaPluginManagerUI RejillaPluginManagerUI;

struct _RejillaPluginManagerUI 
{
	GtkVBox vbox;
};

/*
 * Class definition
 */
typedef struct _RejillaPluginManagerUIClass RejillaPluginManagerUIClass;

struct _RejillaPluginManagerUIClass 
{
	GtkVBoxClass parent_class;
};

/*
 * Public methods
 */
GType		 rejilla_plugin_manager_ui_get_type		(void) G_GNUC_CONST;

GtkWidget	*rejilla_plugin_manager_ui_new		(void);
   
G_END_DECLS

#endif  /* __REJILLA_PLUGIN_MANAGER_UI_H__  */
