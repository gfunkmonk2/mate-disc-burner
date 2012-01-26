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
#include <glib/gi18n-lib.h>

#include <gst/gst.h>

#include "rejilla-media-private.h"

#include "rejilla-media.h"

#include "burn-basics.h"
#include "burn-debug.h"
#include "rejilla-plugin.h"
#include "rejilla-plugin-private.h"
#include "rejilla-plugin-information.h"
#include "rejilla-plugin-registration.h"
#include "burn-caps.h"

#define REJILLA_SCHEMA_PLUGINS				"org.mate.rejilla.plugins"
#define REJILLA_PROPS_PRIORITY_KEY			"priority"

typedef struct _RejillaPluginFlagPair RejillaPluginFlagPair;

struct _RejillaPluginFlagPair {
	RejillaPluginFlagPair *next;
	RejillaBurnFlag supported;
	RejillaBurnFlag compulsory;
};

struct _RejillaPluginFlags {
	RejillaMedia media;
	RejillaPluginFlagPair *pairs;
};
typedef struct _RejillaPluginFlags RejillaPluginFlags;

struct _RejillaPluginConfOption {
	gchar *key;
	gchar *description;
	RejillaPluginConfOptionType type;

	union {
		struct {
			guint max;
			guint min;
		} range;

		GSList *suboptions;

		GSList *choices;
	} specifics;
};

typedef struct _RejillaPluginPrivate RejillaPluginPrivate;
struct _RejillaPluginPrivate
{
	GSettings *settings;

	gboolean active;
	guint group;

	GSList *options;

	GSList *errors;

	GType type;
	gchar *path;
	GModule *handle;

	gchar *name;
	gchar *display_name;
	gchar *author;
	gchar *description;
	gchar *copyright;
	gchar *website;

	guint notify_priority;
	guint priority_original;
	gint priority;

	GSList *flags;
	GSList *blank_flags;

	RejillaPluginProcessFlag process_flags;

	guint compulsory:1;
};

static const gchar *default_icon = "gtk-cdrom";

#define REJILLA_PLUGIN_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REJILLA_TYPE_PLUGIN, RejillaPluginPrivate))
G_DEFINE_TYPE (RejillaPlugin, rejilla_plugin, G_TYPE_TYPE_MODULE);

enum
{
	PROP_0,
	PROP_PATH,
	PROP_PRIORITY
};

enum
{
	LOADED_SIGNAL,
	ACTIVATED_SIGNAL,
	LAST_SIGNAL
};

static GTypeModuleClass* parent_class = NULL;
static guint plugin_signals [LAST_SIGNAL] = { 0 };

static void
rejilla_plugin_error_free (RejillaPluginError *error)
{
	g_free (error->detail);
	g_free (error);
}

void
rejilla_plugin_add_error (RejillaPlugin *plugin,
                          RejillaPluginErrorType type,
                          const gchar *detail)
{
	RejillaPluginError *error;
	RejillaPluginPrivate *priv;

	g_return_if_fail (REJILLA_IS_PLUGIN (plugin));

	priv = REJILLA_PLUGIN_PRIVATE (plugin);

	error = g_new0 (RejillaPluginError, 1);
	error->detail = g_strdup (detail);
	error->type = type;

	priv->errors = g_slist_prepend (priv->errors, error);
}

void
rejilla_plugin_test_gstreamer_plugin (RejillaPlugin *plugin,
                                      const gchar *name)
{
	GstElement *element;

	/* Let's see if we've got the plugins we need */
	element = gst_element_factory_make (name, NULL);
	if (!element)
		rejilla_plugin_add_error (plugin,
		                          REJILLA_PLUGIN_ERROR_MISSING_GSTREAMER_PLUGIN,
		                          name);
	else
		gst_object_unref (element);
}

void
rejilla_plugin_test_app (RejillaPlugin *plugin,
                         const gchar *name,
                         const gchar *version_arg,
                         const gchar *version_format,
                         gint version [3])
{
	gchar *standard_output = NULL;
	gchar *standard_error = NULL;
	guint major, minor, sub;
	gchar *prog_path;
	GPtrArray *argv;
	gboolean res;
	int i;

	/* First see if this plugin can be used, i.e. if cdrecord is in
	 * the path */
	prog_path = g_find_program_in_path (name);
	if (!prog_path) {
		rejilla_plugin_add_error (plugin,
		                          REJILLA_PLUGIN_ERROR_MISSING_APP,
		                          name);
		return;
	}

	if (!g_file_test (prog_path, G_FILE_TEST_IS_EXECUTABLE)) {
		g_free (prog_path);
		rejilla_plugin_add_error (plugin,
		                          REJILLA_PLUGIN_ERROR_MISSING_APP,
		                          name);
		return;
	}

	/* make sure that's not a symlink pointing to something with another
	 * name like wodim.
	 * NOTE: we used to test the target and see if it had the same name as
	 * the symlink with GIO. The problem is, when the symlink pointed to
	 * another symlink, then GIO didn't follow that other symlink. And in
	 * the end it didn't work. So forbid all symlink. */
	if (g_file_test (prog_path, G_FILE_TEST_IS_SYMLINK)) {
		rejilla_plugin_add_error (plugin,
		                          REJILLA_PLUGIN_ERROR_SYMBOLIC_LINK_APP,
		                          name);
		g_free (prog_path);
		return;
	}
	/* Make sure it's a regular file */
	else if (!g_file_test (prog_path, G_FILE_TEST_IS_REGULAR)) {
		rejilla_plugin_add_error (plugin,
		                          REJILLA_PLUGIN_ERROR_MISSING_APP,
		                          name);
		g_free (prog_path);
		return;
	}

	if (!version_arg) {
		g_free (prog_path);
		return;
	}

	/* Check version */
	argv = g_ptr_array_new ();
	g_ptr_array_add (argv, prog_path);
	g_ptr_array_add (argv, (gchar *) version_arg);
	g_ptr_array_add (argv, NULL);

	res = g_spawn_sync (NULL,
	                    (gchar **) argv->pdata,
	                    NULL,
	                    0,
	                    NULL,
	                    NULL,
	                    &standard_output,
	                    &standard_error,
	                    NULL,
	                    NULL);

	g_ptr_array_free (argv, TRUE);
	g_free (prog_path);

	if (!res) {
		rejilla_plugin_add_error (plugin,
		                          REJILLA_PLUGIN_ERROR_WRONG_APP_VERSION,
		                          name);
		return;
	}

	for (i = 0; i < 3 && version [i] >= 0; i++);

	if ((standard_output && sscanf (standard_output, version_format, &major, &minor, &sub) == i)
	||  (standard_error && sscanf (standard_error, version_format, &major, &minor, &sub) == i)) {
		if (major < version [0]
		||  (version [1] >= 0 && minor < version [1])
		||  (version [2] >= 0 && sub < version [2]))
			rejilla_plugin_add_error (plugin,
						  REJILLA_PLUGIN_ERROR_WRONG_APP_VERSION,
						  name);
	}
	else
		rejilla_plugin_add_error (plugin,
		                          REJILLA_PLUGIN_ERROR_WRONG_APP_VERSION,
		                          name);

	g_free (standard_output);
	g_free (standard_error);
}

void
rejilla_plugin_set_compulsory (RejillaPlugin *self,
			       gboolean compulsory)
{
	RejillaPluginPrivate *priv;

	priv = REJILLA_PLUGIN_PRIVATE (self);
	priv->compulsory = compulsory;
}

gboolean
rejilla_plugin_get_compulsory (RejillaPlugin *self)
{
	RejillaPluginPrivate *priv;

	priv = REJILLA_PLUGIN_PRIVATE (self);
	return priv->compulsory;
}

void
rejilla_plugin_set_active (RejillaPlugin *self, gboolean active)
{
	RejillaPluginPrivate *priv;
	gboolean was_active;
	gboolean now_active;

	priv = REJILLA_PLUGIN_PRIVATE (self);

	was_active = rejilla_plugin_get_active (self, FALSE);
	priv->active = active;

	now_active = rejilla_plugin_get_active (self, FALSE);
	if (was_active == now_active)
		return;

	REJILLA_BURN_LOG ("Plugin %s is %s",
			  rejilla_plugin_get_name (self),
			  now_active?"active":"inactive");

	g_signal_emit (self,
		       plugin_signals [ACTIVATED_SIGNAL],
		       0,
		       now_active);
}

gboolean
rejilla_plugin_get_active (RejillaPlugin *plugin,
                           gboolean ignore_errors)
{
	RejillaPluginPrivate *priv;

	priv = REJILLA_PLUGIN_PRIVATE (plugin);

	if (priv->type == G_TYPE_NONE)
		return FALSE;

	if (priv->priority < 0)
		return FALSE;

	if (priv->errors) {
		if (!ignore_errors)
			return FALSE;
	}

	return priv->active;
}

static void
rejilla_plugin_cleanup_definition (RejillaPlugin *self)
{
	RejillaPluginPrivate *priv;

	priv = REJILLA_PLUGIN_PRIVATE (self);

	g_free (priv->name);
	priv->name = NULL;
	g_free (priv->author);
	priv->author = NULL;
	g_free (priv->description);
	priv->description = NULL;
	g_free (priv->copyright);
	priv->copyright = NULL;
	g_free (priv->website);
	priv->website = NULL;
}

/**
 * Plugin configure options
 */

static void
rejilla_plugin_conf_option_choice_pair_free (RejillaPluginChoicePair *pair)
{
	g_free (pair->string);
	g_free (pair);
}

static void
rejilla_plugin_conf_option_free (RejillaPluginConfOption *option)
{
	if (option->type == REJILLA_PLUGIN_OPTION_BOOL)
		g_slist_free (option->specifics.suboptions);

	if (option->type == REJILLA_PLUGIN_OPTION_CHOICE) {
		g_slist_foreach (option->specifics.choices, (GFunc) rejilla_plugin_conf_option_choice_pair_free, NULL);
		g_slist_free (option->specifics.choices);
	}

	g_free (option->key);
	g_free (option->description);

	g_free (option);
}

RejillaPluginConfOption *
rejilla_plugin_get_next_conf_option (RejillaPlugin *self,
				     RejillaPluginConfOption *current)
{
	RejillaPluginPrivate *priv;
	GSList *node;

	priv = REJILLA_PLUGIN_PRIVATE (self);
	if (!priv->options)
		return NULL;

	if (!current)
		return priv->options->data;

	node = g_slist_find (priv->options, current);
	if (!node)
		return NULL;

	if (!node->next)
		return NULL;

	return node->next->data;
}

RejillaBurnResult
rejilla_plugin_conf_option_get_info (RejillaPluginConfOption *option,
				     gchar **key,
				     gchar **description,
				     RejillaPluginConfOptionType *type)
{
	g_return_val_if_fail (option != NULL, REJILLA_BURN_ERR);

	if (key)
		*key = g_strdup (option->key);

	if (description)
		*description = g_strdup (option->description);

	if (type)
		*type = option->type;

	return REJILLA_BURN_OK;
}

RejillaPluginConfOption *
rejilla_plugin_conf_option_new (const gchar *key,
				const gchar *description,
				RejillaPluginConfOptionType type)
{
	RejillaPluginConfOption *option;

	g_return_val_if_fail (key != NULL, NULL);
	g_return_val_if_fail (description != NULL, NULL);
	g_return_val_if_fail (type != REJILLA_PLUGIN_OPTION_NONE, NULL);

	option = g_new0 (RejillaPluginConfOption, 1);
	option->key = g_strdup (key);
	option->description = g_strdup (description);
	option->type = type;

	return option;
}

RejillaBurnResult
rejilla_plugin_add_conf_option (RejillaPlugin *self,
				RejillaPluginConfOption *option)
{
	RejillaPluginPrivate *priv;

	priv = REJILLA_PLUGIN_PRIVATE (self);
	priv->options = g_slist_append (priv->options, option);

	return REJILLA_BURN_OK;
}

RejillaBurnResult
rejilla_plugin_conf_option_bool_add_suboption (RejillaPluginConfOption *option,
					       RejillaPluginConfOption *suboption)
{
	if (option->type != REJILLA_PLUGIN_OPTION_BOOL)
		return REJILLA_BURN_ERR;

	option->specifics.suboptions = g_slist_prepend (option->specifics.suboptions,
						        suboption);
	return REJILLA_BURN_OK;
}

RejillaBurnResult
rejilla_plugin_conf_option_int_set_range (RejillaPluginConfOption *option,
					  gint min,
					  gint max)
{
	if (option->type != REJILLA_PLUGIN_OPTION_INT)
		return REJILLA_BURN_ERR;

	option->specifics.range.max = max;
	option->specifics.range.min = min;
	return REJILLA_BURN_OK;
}

RejillaBurnResult
rejilla_plugin_conf_option_choice_add (RejillaPluginConfOption *option,
				       const gchar *string,
				       gint value)
{
	RejillaPluginChoicePair *pair;

	if (option->type != REJILLA_PLUGIN_OPTION_CHOICE)
		return REJILLA_BURN_ERR;

	pair = g_new0 (RejillaPluginChoicePair, 1);
	pair->value = value;
	pair->string = g_strdup (string);
	option->specifics.choices = g_slist_append (option->specifics.choices, pair);

	return REJILLA_BURN_OK;
}

GSList *
rejilla_plugin_conf_option_bool_get_suboptions (RejillaPluginConfOption *option)
{
	if (option->type != REJILLA_PLUGIN_OPTION_BOOL)
		return NULL;
	return option->specifics.suboptions;
}

gint
rejilla_plugin_conf_option_int_get_max (RejillaPluginConfOption *option) 
{
	if (option->type != REJILLA_PLUGIN_OPTION_INT)
		return -1;
	return option->specifics.range.max;
}

gint
rejilla_plugin_conf_option_int_get_min (RejillaPluginConfOption *option)
{
	if (option->type != REJILLA_PLUGIN_OPTION_INT)
		return -1;
	return option->specifics.range.min;
}

GSList *
rejilla_plugin_conf_option_choice_get (RejillaPluginConfOption *option)
{
	if (option->type != REJILLA_PLUGIN_OPTION_CHOICE)
		return NULL;
	return option->specifics.choices;
}

/**
 * Used to set the caps of plugin
 */

void
rejilla_plugin_define (RejillaPlugin *self,
		       const gchar *name,
                       const gchar *display_name,
                       const gchar *description,
		       const gchar *author,
		       guint priority)
{
	RejillaPluginPrivate *priv;

	priv = REJILLA_PLUGIN_PRIVATE (self);

	rejilla_plugin_cleanup_definition (self);

	priv->name = g_strdup (name);
	priv->display_name = g_strdup (display_name);
	priv->author = g_strdup (author);
	priv->description = g_strdup (description);
	priv->priority_original = priority;
}

void
rejilla_plugin_set_group (RejillaPlugin *self,
			  gint group_id)
{
	RejillaPluginPrivate *priv;

	priv = REJILLA_PLUGIN_PRIVATE (self);
	priv->group = group_id;
}

guint
rejilla_plugin_get_group (RejillaPlugin *self)
{
	RejillaPluginPrivate *priv;

	priv = REJILLA_PLUGIN_PRIVATE (self);
	return priv->group;
}

/**
 * rejilla_plugin_get_errors:
 * @plugin: a #RejillaPlugin.
 *
 * This function returns a list of all errors that
 * prevents the plugin from working properly.
 *
 * Returns: a #GSList of #RejillaPluginError structures or %NULL.
 * It must not be freed.
 **/

GSList *
rejilla_plugin_get_errors (RejillaPlugin *plugin)
{
	RejillaPluginPrivate *priv;

	g_return_val_if_fail (REJILLA_IS_PLUGIN (plugin), NULL);
	priv = REJILLA_PLUGIN_PRIVATE (plugin);
	return priv->errors;
}

gchar *
rejilla_plugin_get_error_string (RejillaPlugin *plugin)
{
	gchar *error_string = NULL;
	RejillaPluginPrivate *priv;
	GString *string;
	GSList *iter;

	g_return_val_if_fail (REJILLA_IS_PLUGIN (plugin), NULL);

	priv = REJILLA_PLUGIN_PRIVATE (plugin);

	string = g_string_new (NULL);
	for (iter = priv->errors; iter; iter = iter->next) {
		RejillaPluginError *error;

		error = iter->data;
		switch (error->type) {
			case REJILLA_PLUGIN_ERROR_MISSING_APP:
				g_string_append_c (string, '\n');
				g_string_append_printf (string, _("\"%s\" could not be found in the path"), error->detail);
				break;
			case REJILLA_PLUGIN_ERROR_MISSING_GSTREAMER_PLUGIN:
				g_string_append_c (string, '\n');
				g_string_append_printf (string, _("\"%s\" GStreamer plugin could not be found"), error->detail);
				break;
			case REJILLA_PLUGIN_ERROR_WRONG_APP_VERSION:
				g_string_append_c (string, '\n');
				g_string_append_printf (string, _("The version of \"%s\" is too old"), error->detail);
				break;
			case REJILLA_PLUGIN_ERROR_SYMBOLIC_LINK_APP:
				g_string_append_c (string, '\n');
				g_string_append_printf (string, _("\"%s\" is a symbolic link pointing to another program"), error->detail);
				break;
			case REJILLA_PLUGIN_ERROR_MISSING_LIBRARY:
				g_string_append_c (string, '\n');
				g_string_append_printf (string, _("\"%s\" could not be found"), error->detail);
				break;
			case REJILLA_PLUGIN_ERROR_LIBRARY_VERSION:
				g_string_append_c (string, '\n');
				g_string_append_printf (string, _("The version of \"%s\" is too old"), error->detail);
				break;
			case REJILLA_PLUGIN_ERROR_MODULE:
				g_string_append_c (string, '\n');
				g_string_append (string, error->detail);
				break;

			default:
				break;
		}
	}

	error_string = string->str;
	g_string_free (string, FALSE);
	return error_string;
}

static RejillaPluginFlags *
rejilla_plugin_get_flags (GSList *flags,
			  RejillaMedia media)
{
	GSList *iter;

	for (iter = flags; iter; iter = iter->next) {
		RejillaPluginFlags *flags;

		flags = iter->data;
		if ((media & flags->media) == media)
			return flags;
	}

	return NULL;
}

static GSList *
rejilla_plugin_set_flags_real (GSList *flags_list,
			       RejillaMedia media,
			       RejillaBurnFlag supported,
			       RejillaBurnFlag compulsory)
{
	RejillaPluginFlags *flags;
	RejillaPluginFlagPair *pair;

	flags = rejilla_plugin_get_flags (flags_list, media);
	if (!flags) {
		flags = g_new0 (RejillaPluginFlags, 1);
		flags->media = media;
		flags_list = g_slist_prepend (flags_list, flags);
	}
	else for (pair = flags->pairs; pair; pair = pair->next) {
		/* have a look at the RejillaPluginFlagPair to see if there
		 * is an exactly similar pair of flags or at least which
		 * encompasses it to avoid redundancy. */
		if ((pair->supported & supported) == supported
		&&  (pair->compulsory & compulsory) == compulsory)
			return flags_list;
	}

	pair = g_new0 (RejillaPluginFlagPair, 1);
	pair->supported = supported;
	pair->compulsory = compulsory;

	pair->next = flags->pairs;
	flags->pairs = pair;

	return flags_list;
}

void
rejilla_plugin_set_flags (RejillaPlugin *self,
			  RejillaMedia media,
			  RejillaBurnFlag supported,
			  RejillaBurnFlag compulsory)
{
	RejillaPluginPrivate *priv;
	GSList *list;
	GSList *iter;

	priv = REJILLA_PLUGIN_PRIVATE (self);

	list = rejilla_media_get_all_list (media);
	for (iter = list; iter; iter = iter->next) {
		RejillaMedia medium;

		medium = GPOINTER_TO_INT (iter->data);
		priv->flags = rejilla_plugin_set_flags_real (priv->flags,
							     medium,
							     supported,
							     compulsory);
	}
	g_slist_free (list);
}

static gboolean
rejilla_plugin_get_all_flags (GSList *flags_list,
			      gboolean check_compulsory,
			      RejillaMedia media,
			      RejillaBurnFlag mask,
			      RejillaBurnFlag current,
			      RejillaBurnFlag *supported_retval,
			      RejillaBurnFlag *compulsory_retval)
{
	gboolean found;
	RejillaPluginFlags *flags;
	RejillaPluginFlagPair *iter;
	RejillaBurnFlag supported = REJILLA_BURN_FLAG_NONE;
	RejillaBurnFlag compulsory = (REJILLA_BURN_FLAG_ALL & mask);

	flags = rejilla_plugin_get_flags (flags_list, media);
	if (!flags) {
		if (supported_retval)
			*supported_retval = REJILLA_BURN_FLAG_NONE;
		if (compulsory_retval)
			*compulsory_retval = REJILLA_BURN_FLAG_NONE;
		return FALSE;
	}

	/* Find all sets of flags that support the current flags */
	found = FALSE;
	for (iter = flags->pairs; iter; iter = iter->next) {
		RejillaBurnFlag compulsory_masked;

		if ((current & iter->supported) != current)
			continue;

		compulsory_masked = (iter->compulsory & mask);
		if (check_compulsory
		&& (current & compulsory_masked) != compulsory_masked)
			continue;

		supported |= iter->supported;
		compulsory &= compulsory_masked;
		found = TRUE;
	}

	if (!found) {
		if (supported_retval)
			*supported_retval = REJILLA_BURN_FLAG_NONE;
		if (compulsory_retval)
			*compulsory_retval = REJILLA_BURN_FLAG_NONE;
		return FALSE;
	}

	if (supported_retval)
		*supported_retval = supported;
	if (compulsory_retval)
		*compulsory_retval = compulsory;

	return TRUE;
}

gboolean
rejilla_plugin_check_record_flags (RejillaPlugin *self,
				   RejillaMedia media,
				   RejillaBurnFlag current)
{
	RejillaPluginPrivate *priv;

	priv = REJILLA_PLUGIN_PRIVATE (self);
	current &= REJILLA_PLUGIN_BURN_FLAG_MASK;

	return rejilla_plugin_get_all_flags (priv->flags,
					     TRUE,
					     media,
					     REJILLA_PLUGIN_BURN_FLAG_MASK,
					     current,
					     NULL,
					     NULL);
}

gboolean
rejilla_plugin_check_image_flags (RejillaPlugin *self,
				  RejillaMedia media,
				  RejillaBurnFlag current)
{
	RejillaPluginPrivate *priv;

	priv = REJILLA_PLUGIN_PRIVATE (self);

	current &= REJILLA_PLUGIN_IMAGE_FLAG_MASK;

	/* If there is no flag that's no use checking anything. If there is no
	 * flag we don't care about the media and therefore it's always possible
	 * NOTE: that's no the case for other operation like burn/blank. */
	if (current == REJILLA_BURN_FLAG_NONE)
		return TRUE;

	return rejilla_plugin_get_all_flags (priv->flags,
					     TRUE,
					     media,
					     REJILLA_PLUGIN_IMAGE_FLAG_MASK,
					     current,
					     NULL,
					     NULL);
}

gboolean
rejilla_plugin_check_media_restrictions (RejillaPlugin *self,
					 RejillaMedia media)
{
	RejillaPluginPrivate *priv;

	priv = REJILLA_PLUGIN_PRIVATE (self);

	/* no restrictions */
	if (!priv->flags)
		return TRUE;

	return (rejilla_plugin_get_flags (priv->flags, media) != NULL);
}

gboolean
rejilla_plugin_get_record_flags (RejillaPlugin *self,
				 RejillaMedia media,
				 RejillaBurnFlag current,
				 RejillaBurnFlag *supported,
				 RejillaBurnFlag *compulsory)
{
	RejillaPluginPrivate *priv;
	gboolean result;

	priv = REJILLA_PLUGIN_PRIVATE (self);
	current &= REJILLA_PLUGIN_BURN_FLAG_MASK;

	result = rejilla_plugin_get_all_flags (priv->flags,
					       FALSE,
					       media,
					       REJILLA_PLUGIN_BURN_FLAG_MASK,
					       current,
					       supported,
					       compulsory);
	if (!result)
		return FALSE;

	if (supported)
		*supported &= REJILLA_PLUGIN_BURN_FLAG_MASK;
	if (compulsory)
		*compulsory &= REJILLA_PLUGIN_BURN_FLAG_MASK;

	return TRUE;
}

gboolean
rejilla_plugin_get_image_flags (RejillaPlugin *self,
				RejillaMedia media,
				RejillaBurnFlag current,
				RejillaBurnFlag *supported,
				RejillaBurnFlag *compulsory)
{
	RejillaPluginPrivate *priv;
	gboolean result;

	priv = REJILLA_PLUGIN_PRIVATE (self);
	current &= REJILLA_PLUGIN_IMAGE_FLAG_MASK;

	result = rejilla_plugin_get_all_flags (priv->flags,
					       FALSE,
					       media,
					       REJILLA_PLUGIN_IMAGE_FLAG_MASK,
					       current,
					       supported,
					       compulsory);
	if (!result)
		return FALSE;

	if (supported)
		*supported &= REJILLA_PLUGIN_IMAGE_FLAG_MASK;
	if (compulsory)
		*compulsory &= REJILLA_PLUGIN_IMAGE_FLAG_MASK;

	return TRUE;
}

void
rejilla_plugin_set_blank_flags (RejillaPlugin *self,
				RejillaMedia media,
				RejillaBurnFlag supported,
				RejillaBurnFlag compulsory)
{
	RejillaPluginPrivate *priv;

	priv = REJILLA_PLUGIN_PRIVATE (self);
	priv->blank_flags = rejilla_plugin_set_flags_real (priv->blank_flags,
							   media,
							   supported,
							   compulsory);
}

gboolean
rejilla_plugin_check_blank_flags (RejillaPlugin *self,
				  RejillaMedia media,
				  RejillaBurnFlag current)
{
	RejillaPluginPrivate *priv;

	priv = REJILLA_PLUGIN_PRIVATE (self);
	current &= REJILLA_PLUGIN_BLANK_FLAG_MASK;

	return rejilla_plugin_get_all_flags (priv->blank_flags,
					     TRUE,
					     media,
					     REJILLA_PLUGIN_BLANK_FLAG_MASK,
					     current,
					     NULL,
					     NULL);
}

gboolean
rejilla_plugin_get_blank_flags (RejillaPlugin *self,
				RejillaMedia media,
				RejillaBurnFlag current,
			        RejillaBurnFlag *supported,
			        RejillaBurnFlag *compulsory)
{
	RejillaPluginPrivate *priv;
	gboolean result;

	priv = REJILLA_PLUGIN_PRIVATE (self);
	current &= REJILLA_PLUGIN_BLANK_FLAG_MASK;

	result = rejilla_plugin_get_all_flags (priv->blank_flags,
					       FALSE,
					       media,
					       REJILLA_PLUGIN_BLANK_FLAG_MASK,
					       current,
					       supported,
					       compulsory);
	if (!result)
		return FALSE;

	if (supported)
		*supported &= REJILLA_PLUGIN_BLANK_FLAG_MASK;
	if (compulsory)
		*compulsory &= REJILLA_PLUGIN_BLANK_FLAG_MASK;

	return TRUE;
}

void
rejilla_plugin_set_process_flags (RejillaPlugin *plugin,
				  RejillaPluginProcessFlag flags)
{
	RejillaPluginPrivate *priv;

	priv = REJILLA_PLUGIN_PRIVATE (plugin);
	priv->process_flags = flags;
}

gboolean
rejilla_plugin_get_process_flags (RejillaPlugin *plugin,
				  RejillaPluginProcessFlag *flags)
{
	RejillaPluginPrivate *priv;

	g_return_val_if_fail (flags != NULL, FALSE);

	priv = REJILLA_PLUGIN_PRIVATE (plugin);
	*flags = priv->process_flags;
	return TRUE;
}

const gchar *
rejilla_plugin_get_name (RejillaPlugin *plugin)
{
	RejillaPluginPrivate *priv;

	priv = REJILLA_PLUGIN_PRIVATE (plugin);
	return priv->name;
}

const gchar *
rejilla_plugin_get_display_name (RejillaPlugin *plugin)
{
	RejillaPluginPrivate *priv;

	priv = REJILLA_PLUGIN_PRIVATE (plugin);
	return priv->display_name ? priv->display_name:priv->name;

}

const gchar *
rejilla_plugin_get_author (RejillaPlugin *plugin)
{
	RejillaPluginPrivate *priv;

	priv = REJILLA_PLUGIN_PRIVATE (plugin);
	return priv->author;
}

const gchar *
rejilla_plugin_get_copyright (RejillaPlugin *plugin)
{
	RejillaPluginPrivate *priv;

	priv = REJILLA_PLUGIN_PRIVATE (plugin);
	return priv->copyright;
}

const gchar *
rejilla_plugin_get_website (RejillaPlugin *plugin)
{
	RejillaPluginPrivate *priv;

	priv = REJILLA_PLUGIN_PRIVATE (plugin);
	return priv->website;
}

const gchar *
rejilla_plugin_get_description (RejillaPlugin *plugin)
{
	RejillaPluginPrivate *priv;

	priv = REJILLA_PLUGIN_PRIVATE (plugin);
	return priv->description;
}

const gchar *
rejilla_plugin_get_icon_name (RejillaPlugin *plugin)
{
	RejillaPluginPrivate *priv;

	priv = REJILLA_PLUGIN_PRIVATE (plugin);
	return default_icon;
}

guint
rejilla_plugin_get_priority (RejillaPlugin *self)
{
	RejillaPluginPrivate *priv;

	priv = REJILLA_PLUGIN_PRIVATE (self);

	if (priv->priority > 0)
		return priv->priority;

	return priv->priority_original;
}

GType
rejilla_plugin_get_gtype (RejillaPlugin *self)
{
	RejillaPluginPrivate *priv;

	priv = REJILLA_PLUGIN_PRIVATE (self);

	if (priv->errors)
		return G_TYPE_NONE;

	return priv->type;
}

/**
 * Function to initialize and load
 */

static void
rejilla_plugin_unload (GTypeModule *module)
{
	RejillaPluginPrivate *priv;

	priv = REJILLA_PLUGIN_PRIVATE (module);
	if (!priv->handle)
		return;

	g_module_close (priv->handle);
	priv->handle = NULL;
}

static gboolean
rejilla_plugin_load_real (RejillaPlugin *plugin) 
{
	RejillaPluginPrivate *priv;
	RejillaPluginRegisterType register_func;

	priv = REJILLA_PLUGIN_PRIVATE (plugin);

	if (!priv->path)
		return FALSE;

	if (priv->handle)
		return TRUE;

	priv->handle = g_module_open (priv->path, G_MODULE_BIND_LAZY);
	if (!priv->handle) {
		rejilla_plugin_add_error (plugin, REJILLA_PLUGIN_ERROR_MODULE, g_module_error ());
		REJILLA_BURN_LOG ("Module %s can't be loaded: g_module_open failed ()", priv->name);
		return FALSE;
	}

	if (!g_module_symbol (priv->handle, "rejilla_plugin_register", (gpointer) &register_func)) {
		REJILLA_BURN_LOG ("it doesn't appear to be a valid rejilla plugin");
		rejilla_plugin_unload (G_TYPE_MODULE (plugin));
		return FALSE;
	}

	priv->type = register_func (plugin);
	rejilla_burn_debug_setup_module (priv->handle);
	return TRUE;
}

static gboolean
rejilla_plugin_load (GTypeModule *module) 
{
	if (!rejilla_plugin_load_real (REJILLA_PLUGIN (module)))
		return FALSE;

	g_signal_emit (REJILLA_PLUGIN (module),
		       plugin_signals [LOADED_SIGNAL],
		       0);
	return TRUE;
}

static void
rejilla_plugin_priority_changed (GSettings *settings,
                                 const gchar *key,
                                 RejillaPlugin *self)
{
	RejillaPluginPrivate *priv;
	gboolean is_active;

	priv = REJILLA_PLUGIN_PRIVATE (self);

	/* At the moment it can only be the priority key */
	priv->priority = g_settings_get_int (settings, REJILLA_PROPS_PRIORITY_KEY);

	is_active = rejilla_plugin_get_active (self, FALSE);

	g_object_notify (G_OBJECT (self), "priority");
	if (is_active != rejilla_plugin_get_active (self, FALSE))
		g_signal_emit (self,
			       plugin_signals [ACTIVATED_SIGNAL],
			       0,
			       is_active);
}

typedef void	(* RejillaPluginCheckConfig)	(RejillaPlugin *plugin);

/**
 * rejilla_plugin_check_plugin_ready:
 * @plugin: a #RejillaPlugin.
 *
 * Ask a plugin to check whether it can operate.
 * rejilla_plugin_can_operate () should be called
 * afterwards to know whether it can operate or not.
 *
 **/
void
rejilla_plugin_check_plugin_ready (RejillaPlugin *plugin)
{
	GModule *handle;
	RejillaPluginPrivate *priv;
	RejillaPluginCheckConfig function = NULL;

	g_return_if_fail (REJILLA_IS_PLUGIN (plugin));
	priv = REJILLA_PLUGIN_PRIVATE (plugin);

	if (priv->errors) {
		g_slist_foreach (priv->errors, (GFunc) rejilla_plugin_error_free, NULL);
		g_slist_free (priv->errors);
		priv->errors = NULL;
	}

	handle = g_module_open (priv->path, 0);
	if (!handle) {
		rejilla_plugin_add_error (plugin, REJILLA_PLUGIN_ERROR_MODULE, g_module_error ());
		REJILLA_BURN_LOG ("Module %s can't be loaded: g_module_open failed ()", priv->name);
		return;
	}

	if (!g_module_symbol (handle, "rejilla_plugin_check_config", (gpointer) &function)) {
		g_module_close (handle);
		REJILLA_BURN_LOG ("Module %s has no check config function", priv->name);
		return;
	}

	function (REJILLA_PLUGIN (plugin));
	g_module_close (handle);
}

static void
rejilla_plugin_init_real (RejillaPlugin *object)
{
	GModule *handle;
	gchar *settings_path;
	RejillaPluginPrivate *priv;
	RejillaPluginRegisterType function = NULL;

	priv = REJILLA_PLUGIN_PRIVATE (object);

	g_type_module_set_name (G_TYPE_MODULE (object), priv->name);

	handle = g_module_open (priv->path, 0);
	if (!handle) {
		rejilla_plugin_add_error (object, REJILLA_PLUGIN_ERROR_MODULE, g_module_error ());
		REJILLA_BURN_LOG ("Module %s (at %s) can't be loaded: g_module_open failed ()", priv->name, priv->path);
		return;
	}

	if (!g_module_symbol (handle, "rejilla_plugin_register", (gpointer) &function)) {
		g_module_close (handle);
		REJILLA_BURN_LOG ("Module %s can't be loaded: no register function, priv->name", priv->name);
		return;
	}

	priv->type = function (object);
	if (priv->type == G_TYPE_NONE) {
		g_module_close (handle);
		REJILLA_BURN_LOG ("Module %s encountered an error while registering its capabilities", priv->name);
		return;
	}

	/* now see if we need to override the hardcoded priority of the plugin */
	settings_path = g_strconcat ("/apps/rejilla/plugins/",
	                             priv->name,
	                             G_DIR_SEPARATOR_S,
	                             NULL);
	priv->settings = g_settings_new_with_path (REJILLA_SCHEMA_PLUGINS,
	                                           settings_path);
	g_free (settings_path);

	priv->priority = g_settings_get_int (priv->settings, REJILLA_PROPS_PRIORITY_KEY);

	/* Make sure a key is created for each plugin */
	if (!priv->priority)
		g_settings_set_int (priv->settings, REJILLA_PROPS_PRIORITY_KEY, 0);

	g_signal_connect (priv->settings,
	                  "changed",
	                  G_CALLBACK (rejilla_plugin_priority_changed),
	                  object);

	/* Check if it can operate */
	rejilla_plugin_check_plugin_ready (object);

	g_module_close (handle);
}

static void
rejilla_plugin_finalize (GObject *object)
{
	RejillaPluginPrivate *priv;

	priv = REJILLA_PLUGIN_PRIVATE (object);

	if (priv->options) {
		g_slist_foreach (priv->options, (GFunc) rejilla_plugin_conf_option_free, NULL);
		g_slist_free (priv->options);
		priv->options = NULL;
	}

	if (priv->handle) {
		rejilla_plugin_unload (G_TYPE_MODULE (object));
		priv->handle = NULL;
	}

	if (priv->path) {
		g_free (priv->path);
		priv->path = NULL;
	}

	g_free (priv->name);
	g_free (priv->display_name);
	g_free (priv->author);
	g_free (priv->description);

	g_slist_foreach (priv->flags, (GFunc) g_free, NULL);
	g_slist_free (priv->flags);

	g_slist_foreach (priv->blank_flags, (GFunc) g_free, NULL);
	g_slist_free (priv->blank_flags);

	if (priv->settings) {
		g_object_unref (priv->settings);
		priv->settings = NULL;
	}

	if (priv->errors) {
		g_slist_foreach (priv->errors, (GFunc) rejilla_plugin_error_free, NULL);
		g_slist_free (priv->errors);
		priv->errors = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
rejilla_plugin_set_property (GObject *object,
			     guint prop_id,
			     const GValue *value,
			     GParamSpec *pspec)
{
	RejillaPlugin *self;
	RejillaPluginPrivate *priv;

	g_return_if_fail (REJILLA_IS_PLUGIN (object));

	self = REJILLA_PLUGIN (object);
	priv = REJILLA_PLUGIN_PRIVATE (self);

	switch (prop_id)
	{
	case PROP_PATH:
		/* NOTE: this property can only be set once */
		priv->path = g_strdup (g_value_get_string (value));
		rejilla_plugin_init_real (self);
		break;
	case PROP_PRIORITY:
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
rejilla_plugin_get_property (GObject *object,
			     guint prop_id,
			     GValue *value,
			     GParamSpec *pspec)
{
	RejillaPlugin *self;
	RejillaPluginPrivate *priv;

	g_return_if_fail (REJILLA_IS_PLUGIN (object));

	self = REJILLA_PLUGIN (object);
	priv = REJILLA_PLUGIN_PRIVATE (self);

	switch (prop_id)
	{
	case PROP_PATH:
		g_value_set_string (value, priv->path);
		break;
	case PROP_PRIORITY:
		g_value_set_int (value, priv->priority);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
rejilla_plugin_init (RejillaPlugin *object)
{
	RejillaPluginPrivate *priv;

	priv = REJILLA_PLUGIN_PRIVATE (object);
	priv->type = G_TYPE_NONE;
	priv->compulsory = TRUE;
}

static void
rejilla_plugin_class_init (RejillaPluginClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GTypeModuleClass *module_class = G_TYPE_MODULE_CLASS (klass);

	g_type_class_add_private (klass, sizeof (RejillaPluginPrivate));

	object_class->finalize = rejilla_plugin_finalize;
	object_class->set_property = rejilla_plugin_set_property;
	object_class->get_property = rejilla_plugin_get_property;

	module_class->load = rejilla_plugin_load;
	module_class->unload = rejilla_plugin_unload;

	g_object_class_install_property (object_class,
	                                 PROP_PATH,
	                                 g_param_spec_string ("path",
	                                                      "Path",
	                                                      "Path for the module",
	                                                      NULL,
	                                                      G_PARAM_STATIC_NAME|G_PARAM_READABLE|G_PARAM_WRITABLE|G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (object_class,
	                                 PROP_PRIORITY,
	                                 g_param_spec_int ("priority",
							   "Priority",
							   "Priority of the module",
							   1,
							   G_MAXINT,
							   1,
							   G_PARAM_STATIC_NAME|G_PARAM_READABLE));

	plugin_signals [LOADED_SIGNAL] =
		g_signal_new ("loaded",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE,
		              G_STRUCT_OFFSET (RejillaPluginClass, loaded),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0);

	plugin_signals [ACTIVATED_SIGNAL] =
		g_signal_new ("activated",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_LAST | G_SIGNAL_NO_RECURSE,
		              G_STRUCT_OFFSET (RejillaPluginClass, activated),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__BOOLEAN,
		              G_TYPE_NONE, 1,
			      G_TYPE_BOOLEAN);
}

RejillaPlugin *
rejilla_plugin_new (const gchar *path)
{
	if (!path)
		return NULL;

	return REJILLA_PLUGIN (g_object_new (REJILLA_TYPE_PLUGIN,
					     "path", path,
					     NULL));
}
