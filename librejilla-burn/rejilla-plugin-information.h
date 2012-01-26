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
 
#ifndef _BURN_PLUGIN_REGISTRATION_H
#define _BURN_PLUGIN_REGISTRATION_H

#include <glib.h>
#include <glib-object.h>

#include "rejilla-session.h"
#include "rejilla-plugin.h"

G_BEGIN_DECLS

void
rejilla_plugin_set_active (RejillaPlugin *plugin, gboolean active);

gboolean
rejilla_plugin_get_active (RejillaPlugin *plugin,
                           gboolean ignore_errors);

const gchar *
rejilla_plugin_get_name (RejillaPlugin *plugin);

const gchar *
rejilla_plugin_get_display_name (RejillaPlugin *plugin);

const gchar *
rejilla_plugin_get_author (RejillaPlugin *plugin);

guint
rejilla_plugin_get_group (RejillaPlugin *plugin);

const gchar *
rejilla_plugin_get_copyright (RejillaPlugin *plugin);

const gchar *
rejilla_plugin_get_website (RejillaPlugin *plugin);

const gchar *
rejilla_plugin_get_description (RejillaPlugin *plugin);

const gchar *
rejilla_plugin_get_icon_name (RejillaPlugin *plugin);

typedef struct _RejillaPluginError RejillaPluginError;
struct _RejillaPluginError {
	RejillaPluginErrorType type;
	gchar *detail;
};

GSList *
rejilla_plugin_get_errors (RejillaPlugin *plugin);

gchar *
rejilla_plugin_get_error_string (RejillaPlugin *plugin);

gboolean
rejilla_plugin_get_compulsory (RejillaPlugin *plugin);

guint
rejilla_plugin_get_priority (RejillaPlugin *plugin);

/** 
 * This is to find out what are the capacities of a plugin 
 */

RejillaBurnResult
rejilla_plugin_can_burn (RejillaPlugin *plugin);

RejillaBurnResult
rejilla_plugin_can_image (RejillaPlugin *plugin);

RejillaBurnResult
rejilla_plugin_can_convert (RejillaPlugin *plugin);


/**
 * Plugin configuration options
 */

RejillaPluginConfOption *
rejilla_plugin_get_next_conf_option (RejillaPlugin *plugin,
				     RejillaPluginConfOption *current);

RejillaBurnResult
rejilla_plugin_conf_option_get_info (RejillaPluginConfOption *option,
				     gchar **key,
				     gchar **description,
				     RejillaPluginConfOptionType *type);

GSList *
rejilla_plugin_conf_option_bool_get_suboptions (RejillaPluginConfOption *option);

gint
rejilla_plugin_conf_option_int_get_min (RejillaPluginConfOption *option);
gint
rejilla_plugin_conf_option_int_get_max (RejillaPluginConfOption *option);


struct _RejillaPluginChoicePair {
	gchar *string;
	guint value;
};
typedef struct _RejillaPluginChoicePair RejillaPluginChoicePair;

GSList *
rejilla_plugin_conf_option_choice_get (RejillaPluginConfOption *option);

G_END_DECLS

#endif
 
