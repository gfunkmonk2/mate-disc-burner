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

#ifndef _BURN_PLUGIN_H_
#define _BURN_PLUGIN_H_

#include <glib.h>
#include <glib-object.h>
#include <gmodule.h>

G_BEGIN_DECLS

#define REJILLA_TYPE_PLUGIN             (rejilla_plugin_get_type ())
#define REJILLA_PLUGIN(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_PLUGIN, RejillaPlugin))
#define REJILLA_PLUGIN_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_PLUGIN, RejillaPluginClass))
#define REJILLA_IS_PLUGIN(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_PLUGIN))
#define REJILLA_IS_PLUGIN_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_PLUGIN))
#define REJILLA_PLUGIN_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_PLUGIN, RejillaPluginClass))

typedef struct _RejillaPluginClass RejillaPluginClass;
typedef struct _RejillaPlugin RejillaPlugin;

struct _RejillaPluginClass {
	GTypeModuleClass parent_class;

	/* Signals */
	void	(* loaded)	(RejillaPlugin *plugin);
	void	(* activated)	(RejillaPlugin *plugin,
			                  gboolean active);
};

struct _RejillaPlugin {
	GTypeModule parent_instance;
};

GType rejilla_plugin_get_type (void) G_GNUC_CONST;

GType
rejilla_plugin_get_gtype (RejillaPlugin *self);

/**
 * Plugin configure options
 */

typedef struct _RejillaPluginConfOption RejillaPluginConfOption;
typedef enum {
	REJILLA_PLUGIN_OPTION_NONE	= 0,
	REJILLA_PLUGIN_OPTION_BOOL,
	REJILLA_PLUGIN_OPTION_INT,
	REJILLA_PLUGIN_OPTION_STRING,
	REJILLA_PLUGIN_OPTION_CHOICE
} RejillaPluginConfOptionType;

typedef enum {
	REJILLA_PLUGIN_RUN_NEVER		= 0,

	/* pre-process initial track */
	REJILLA_PLUGIN_RUN_PREPROCESSING	= 1,

	/* run before final image/disc is created */
	REJILLA_PLUGIN_RUN_BEFORE_TARGET	= 1 << 1,

	/* run after final image/disc is created: post-processing */
	REJILLA_PLUGIN_RUN_AFTER_TARGET		= 1 << 2,
} RejillaPluginProcessFlag;

G_END_DECLS

#endif /* _BURN_PLUGIN_H_ */
