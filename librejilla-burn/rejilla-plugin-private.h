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
 
#ifndef _BURN_PLUGIN_PRIVATE_H
#define _BURN_PLUGIN_PRIVATE_H

#include <glib.h>

#include "rejilla-media.h"

#include "rejilla-enums.h"
#include "rejilla-plugin.h"

G_BEGIN_DECLS

RejillaPlugin *
rejilla_plugin_new (const gchar *path);

void
rejilla_plugin_set_group (RejillaPlugin *plugin, gint group_id);

gboolean
rejilla_plugin_get_image_flags (RejillaPlugin *plugin,
			        RejillaMedia media,
				RejillaBurnFlag current,
			        RejillaBurnFlag *supported,
			        RejillaBurnFlag *compulsory);
gboolean
rejilla_plugin_get_blank_flags (RejillaPlugin *plugin,
				RejillaMedia media,
				RejillaBurnFlag current,
				RejillaBurnFlag *supported,
				RejillaBurnFlag *compulsory);
gboolean
rejilla_plugin_get_record_flags (RejillaPlugin *plugin,
				 RejillaMedia media,
				 RejillaBurnFlag current,
				 RejillaBurnFlag *supported,
				 RejillaBurnFlag *compulsory);

gboolean
rejilla_plugin_get_process_flags (RejillaPlugin *plugin,
				  RejillaPluginProcessFlag *flags);

gboolean
rejilla_plugin_check_image_flags (RejillaPlugin *plugin,
				  RejillaMedia media,
				  RejillaBurnFlag current);
gboolean
rejilla_plugin_check_blank_flags (RejillaPlugin *plugin,
				  RejillaMedia media,
				  RejillaBurnFlag current);
gboolean
rejilla_plugin_check_record_flags (RejillaPlugin *plugin,
				   RejillaMedia media,
				   RejillaBurnFlag current);
gboolean
rejilla_plugin_check_media_restrictions (RejillaPlugin *plugin,
					 RejillaMedia media);

void
rejilla_plugin_check_plugin_ready (RejillaPlugin *plugin);

G_END_DECLS

#endif
