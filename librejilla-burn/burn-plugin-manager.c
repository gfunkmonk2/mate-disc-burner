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

#include <glib.h>
#include <glib-object.h>
#include <gmodule.h>
#include <glib/gi18n.h>

#include "burn-basics.h"
#include "burn-debug.h"
#include "rejilla-track.h"
#include "rejilla-plugin.h"
#include "rejilla-plugin-private.h"
#include "rejilla-plugin-information.h"
#include "burn-plugin-manager.h"

static RejillaPluginManager *default_manager = NULL;

#define REJILLA_PLUGIN_MANAGER_NOT_SUPPORTED_LOG(caps, error)			\
{										\
	REJILLA_BURN_LOG ("Unsupported operation");				\
	g_set_error (error,							\
		     REJILLA_BURN_ERROR,					\
		     REJILLA_BURN_ERROR_GENERAL,				\
		     _("An internal error occurred"),				\
		     G_STRLOC);							\
	return REJILLA_BURN_NOT_SUPPORTED;					\
}

typedef struct _RejillaPluginManagerPrivate RejillaPluginManagerPrivate;
struct _RejillaPluginManagerPrivate {
	GSList *plugins;
	GSettings *settings;
};

#define REJILLA_PLUGIN_MANAGER_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REJILLA_TYPE_PLUGIN_MANAGER, RejillaPluginManagerPrivate))

G_DEFINE_TYPE (RejillaPluginManager, rejilla_plugin_manager, G_TYPE_OBJECT);

enum
{
	CAPS_CHANGED_SIGNAL,
	LAST_SIGNAL
};
static guint caps_signals [LAST_SIGNAL] = { 0 };

#define REJILLA_SCHEMA_CONFIG		       "org.mate.rejilla.config"
#define REJILLA_PROPS_PLUGINS_KEY	       "plugins"

static GObjectClass* parent_class = NULL;

GSList *
rejilla_plugin_manager_get_plugins_list (RejillaPluginManager *self)
{
	RejillaPluginManagerPrivate *priv;
	GSList *retval = NULL;
	GSList *iter;

	priv = REJILLA_PLUGIN_MANAGER_PRIVATE (self);

	for (iter = priv->plugins; iter; iter = iter->next) {
		RejillaPlugin *plugin;

		plugin = iter->data;
		g_object_ref (plugin);
		retval = g_slist_prepend (retval, plugin);
	}

	return retval;
}

static void
rejilla_plugin_manager_plugin_state_changed (RejillaPlugin *plugin,
					     gboolean active,
					     RejillaPluginManager *self)
{
	RejillaPluginManagerPrivate *priv;
	GPtrArray *array;
	GSList *iter;

	priv = REJILLA_PLUGIN_MANAGER_PRIVATE (self);

	/* build a list of all active plugins */
	array = g_ptr_array_new ();
	for (iter = priv->plugins; iter; iter = iter->next) {
		RejillaPlugin *plugin;
		const gchar *name;

		plugin = iter->data;

		if (rejilla_plugin_get_gtype (plugin) == G_TYPE_NONE)
			continue;

		if (!rejilla_plugin_get_active (plugin, 0))
			continue;

		if (rejilla_plugin_can_burn (plugin) == REJILLA_BURN_OK
		||  rejilla_plugin_can_convert (plugin) == REJILLA_BURN_OK
		||  rejilla_plugin_can_image (plugin) == REJILLA_BURN_OK)
			continue;

		name = rejilla_plugin_get_name (plugin);
		if (name)
			g_ptr_array_add (array, (gchar *) name);
	}

	if (array->len) {
		g_ptr_array_add (array, NULL);
		g_settings_set_strv (priv->settings,
		                     REJILLA_PROPS_PLUGINS_KEY,
		                     (const gchar * const *) array->pdata);
	}
	else {
		gchar *none = "none";

		g_ptr_array_add (array, none);
		g_ptr_array_add (array, NULL);
		g_settings_set_strv (priv->settings,
		                     REJILLA_PROPS_PLUGINS_KEY,
		                     (const gchar * const *) array->pdata);
	}
	g_ptr_array_free (array, TRUE);

	/* tell the rest of the world */
	g_signal_emit (self,
		       caps_signals [CAPS_CHANGED_SIGNAL],
		       0);
}

static void
rejilla_plugin_manager_set_plugins_state (RejillaPluginManager *self)
{
	GSList *iter;
	int name_num;
	gchar **names = NULL;
	RejillaPluginManagerPrivate *priv;

	priv = REJILLA_PLUGIN_MANAGER_PRIVATE (self);

	/* get the list of user requested plugins. while at it we add a watch
	 * on the key so as to be warned whenever the user changes prefs. */
	REJILLA_BURN_LOG ("Getting list of plugins to be loaded");
	names = g_settings_get_strv (priv->settings, REJILLA_PROPS_PLUGINS_KEY);
	name_num = g_strv_length (names);

	for (iter = priv->plugins; iter; iter = iter->next) {
		gboolean active;
		RejillaPlugin *plugin;

		plugin = iter->data;

		if (rejilla_plugin_get_compulsory (plugin)) {
			g_signal_handlers_block_matched (plugin,
							 G_SIGNAL_MATCH_FUNC,
							 0,
							 0,
							 0,
							 rejilla_plugin_manager_plugin_state_changed,
							 NULL);
			rejilla_plugin_set_active (plugin, TRUE);
			g_signal_handlers_unblock_matched (plugin,
							   G_SIGNAL_MATCH_FUNC,
							   0,
							   0,
							   0,
							   rejilla_plugin_manager_plugin_state_changed,
							   NULL);
			REJILLA_BURN_LOG ("Plugin set to active. %s is %s",
					  rejilla_plugin_get_name (plugin),
					  rejilla_plugin_get_active (plugin, 0)? "active":"inactive");
			continue;
		}

		/* See if this plugin is in the names list. If not, de-activate it. */
		if (name_num) {
			int i;

			active = FALSE;
			for (i = 0; i < name_num; i++) {
				/* This allows to be able to support the old way
				 * rejilla had to save which plugins should be
				 * used */
				if (!g_strcmp0 (rejilla_plugin_get_name (plugin), names [i])
				||  !g_strcmp0 (rejilla_plugin_get_display_name (plugin), names [i])) {
					active = TRUE;
					break;
				}
			}
		}
		else
			active = TRUE;

		/* we don't want to receive a signal from this plugin if its 
		 * active state changes */
		g_signal_handlers_block_matched (plugin,
						 G_SIGNAL_MATCH_FUNC,
						 0,
						 0,
						 0,
						 rejilla_plugin_manager_plugin_state_changed,
						 NULL);
		rejilla_plugin_set_active (plugin, active);
		g_signal_handlers_unblock_matched (plugin,
						   G_SIGNAL_MATCH_FUNC,
						   0,
						   0,
						   0,
						   rejilla_plugin_manager_plugin_state_changed,
						   NULL);

		REJILLA_BURN_LOG ("Setting plugin %s %s",
				  rejilla_plugin_get_name (plugin),
				  rejilla_plugin_get_active (plugin, 0)? "active":"inactive");
	}
	g_strfreev (names);
}

static void
rejilla_plugin_manager_plugin_list_changed_cb (GSettings *settings,
                                               const gchar *key,
                                               gpointer user_data)
{
	rejilla_plugin_manager_set_plugins_state (REJILLA_PLUGIN_MANAGER (user_data));
}

#if 0

/**
 * This function is only for debugging purpose. It allows to load plugins in a
 * particular order which is useful since sometimes it triggers some new bugs.
 */

static void
rejilla_plugin_manager_init (RejillaPluginManager *self)
{
	guint i = 0;
	const gchar *name [] = {
				"librejilla-transcode.so",
				"librejilla-checksum.so",
				"librejilla-dvdcss.so",
				"librejilla-checksum-file.so",
				"librejilla-local-track.so",
				"librejilla-toc2cue.so",
				"librejilla-wodim.so",
				"librejilla-readom.so",
				"librejilla-dvdrwformat.so",
				"librejilla-genisoimage.so",
				"librejilla-mkisofs.so",
				//"librejilla-normalize.so",
				"librejilla-cdrdao.so",
				//"librejilla-readcd.so",
				//"librejilla-cdrecord.so",
				"librejilla-growisofs.so",
				//"librejilla-libburn.so",
				//"librejilla-libisofs.so",
				//"librejilla-vcdimager.so",
				//"librejilla-dvdauthor.so",
				//"librejilla-vob.so"
				NULL};
	RejillaPluginManagerPrivate *priv;

	priv = REJILLA_PLUGIN_MANAGER_PRIVATE (self);

	/* open the plugin directory */
	REJILLA_BURN_LOG ("opening plugin directory %s", REJILLA_PLUGIN_DIRECTORY);

	/* load all plugins from directory */
	for (i = 0; name [i] != NULL; i++) {
		RejillaPluginRegisterType function;
		RejillaPlugin *plugin;
		GModule *handle;
		gchar *path;

		/* the name must end with *.so */
		if (!g_str_has_suffix (name [i], G_MODULE_SUFFIX))
			continue;

		path = g_module_build_path (REJILLA_PLUGIN_DIRECTORY, name [i]);
		REJILLA_BURN_LOG ("loading %s", path);

		handle = g_module_open (path, 0);
		if (!handle) {
			g_free (path);
			REJILLA_BURN_LOG ("Module can't be loaded: g_module_open failed (%s)",
					  g_module_error ());
			continue;
		}

		if (!g_module_symbol (handle, "rejilla_plugin_register", (gpointer) &function)) {
			g_free (path);
			g_module_close (handle);
			REJILLA_BURN_LOG ("Module can't be loaded: no register function");
			continue;
		}

		/* now we can create the plugin */
		plugin = rejilla_plugin_new (path);
		g_module_close (handle);
		g_free (path);

		if (!plugin) {
			REJILLA_BURN_LOG ("Load failure");
			continue;
		}

		if (rejilla_plugin_get_gtype (plugin) == G_TYPE_NONE) {
			REJILLA_BURN_LOG ("Load failure, no GType was returned %s",
					  rejilla_plugin_get_error (plugin));
		}
		else
			g_signal_connect (plugin,
					  "activated",
					  G_CALLBACK (rejilla_plugin_manager_plugin_state_changed),
					  self);

		priv->plugins = g_slist_prepend (priv->plugins, plugin);
	}

	rejilla_plugin_manager_set_plugins_state (self);
}

#endif

static void
rejilla_plugin_manager_init (RejillaPluginManager *self)
{
	GDir *directory;
	const gchar *name;
	GError *error = NULL;
	RejillaPluginManagerPrivate *priv;

	priv = REJILLA_PLUGIN_MANAGER_PRIVATE (self);

	priv->settings = g_settings_new (REJILLA_SCHEMA_CONFIG);
	g_signal_connect (priv->settings,
	                  "changed",
	                  G_CALLBACK (rejilla_plugin_manager_plugin_list_changed_cb),
	                  self);

	/* open the plugin directory */
	REJILLA_BURN_LOG ("opening plugin directory %s", REJILLA_PLUGIN_DIRECTORY);
	directory = g_dir_open (REJILLA_PLUGIN_DIRECTORY, 0, &error);
	if (!directory) {
		if (error) {
			REJILLA_BURN_LOG ("Error opening plugin directory %s", error->message);
			g_error_free (error);
			return;
		}
	}

	/* load all plugins from directory */
	while ((name = g_dir_read_name (directory))) {
		RejillaPluginRegisterType function;
		RejillaPlugin *plugin;
		GModule *handle;
		gchar *path;

		/* the name must end with *.so */
		if (!g_str_has_suffix (name, G_MODULE_SUFFIX))
			continue;

		path = g_module_build_path (REJILLA_PLUGIN_DIRECTORY, name);
		REJILLA_BURN_LOG ("loading %s", path);

		handle = g_module_open (path, 0);
		if (!handle) {
			g_free (path);
			REJILLA_BURN_LOG ("Module can't be loaded: g_module_open failed (%s)",
					  g_module_error ());
			continue;
		}

		if (!g_module_symbol (handle, "rejilla_plugin_register", (gpointer) &function)) {
			g_free (path);
			g_module_close (handle);
			REJILLA_BURN_LOG ("Module can't be loaded: no register function");
			continue;
		}

		/* now we can create the plugin */
		plugin = rejilla_plugin_new (path);
		g_module_close (handle);
		g_free (path);

		if (!plugin) {
			REJILLA_BURN_LOG ("Load failure");
			continue;
		}

		if (rejilla_plugin_get_gtype (plugin) == G_TYPE_NONE) {
			gchar *error_string;

			error_string = rejilla_plugin_get_error_string (plugin);
			REJILLA_BURN_LOG ("Load failure, no GType was returned %s", error_string);
			g_free (error_string);
		}

		g_signal_connect (plugin,
		                  "activated",
		                  G_CALLBACK (rejilla_plugin_manager_plugin_state_changed),
		                  self);

		g_assert (rejilla_plugin_get_name (plugin));
		priv->plugins = g_slist_prepend (priv->plugins, plugin);
	}
	g_dir_close (directory);

	rejilla_plugin_manager_set_plugins_state (self);
}

static void
rejilla_plugin_manager_finalize (GObject *object)
{
	RejillaPluginManagerPrivate *priv;

	priv = REJILLA_PLUGIN_MANAGER_PRIVATE (object);

	if (priv->settings) {
		g_object_unref (priv->settings);
		priv->settings = NULL;
	}

	if (priv->plugins) {
		g_slist_free (priv->plugins);
		priv->plugins = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
	default_manager = NULL;
}

static void
rejilla_plugin_manager_class_init (RejillaPluginManagerClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	parent_class = G_OBJECT_CLASS (g_type_class_peek_parent (klass));

	g_type_class_add_private (klass, sizeof (RejillaPluginManagerPrivate));

	object_class->finalize = rejilla_plugin_manager_finalize;

	caps_signals [CAPS_CHANGED_SIGNAL] =
		g_signal_new ("caps_changed",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE,
		              0,
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0);
}

RejillaPluginManager *
rejilla_plugin_manager_get_default (void)
{
	if (!default_manager)
		default_manager = REJILLA_PLUGIN_MANAGER (g_object_new (REJILLA_TYPE_PLUGIN_MANAGER, NULL));

	return default_manager;
}
