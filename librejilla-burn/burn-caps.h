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

#ifndef BURN_CAPS_H
#define BURN_CAPS_H

#include <glib.h>
#include <glib-object.h>

#include "burn-basics.h"
#include "rejilla-track-type.h"
#include "rejilla-track-type-private.h"
#include "rejilla-plugin.h"
#include "rejilla-plugin-information.h"
#include "rejilla-plugin-registration.h"

G_BEGIN_DECLS

#define REJILLA_TYPE_BURNCAPS         (rejilla_burn_caps_get_type ())
#define REJILLA_BURNCAPS(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), REJILLA_TYPE_BURNCAPS, RejillaBurnCaps))
#define REJILLA_BURNCAPS_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), REJILLA_TYPE_BURNCAPS, RejillaBurnCapsClass))
#define REJILLA_IS_BURNCAPS(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), REJILLA_TYPE_BURNCAPS))
#define REJILLA_IS_BURNCAPS_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), REJILLA_TYPE_BURNCAPS))
#define REJILLA_BURNCAPS_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), REJILLA_TYPE_BURNCAPS, RejillaBurnCapsClass))

struct _RejillaCaps {
	GSList *links;			/* RejillaCapsLink */
	GSList *modifiers;		/* RejillaPlugin */
	RejillaTrackType type;
	RejillaPluginIOFlag flags;
};
typedef struct _RejillaCaps RejillaCaps;

struct _RejillaCapsLink {
	GSList *plugins;
	RejillaCaps *caps;
};
typedef struct _RejillaCapsLink RejillaCapsLink;

struct _RejillaCapsTest {
	GSList *links;
	RejillaChecksumType type;
};
typedef struct _RejillaCapsTest RejillaCapsTest;

typedef struct RejillaBurnCapsPrivate RejillaBurnCapsPrivate;
struct RejillaBurnCapsPrivate {
	GSList *caps_list;		/* RejillaCaps */
	GSList *tests;			/* RejillaCapsTest */

	GHashTable *groups;

	gchar *group_str;
	guint group_id;
};

typedef struct {
	GObject parent;
	RejillaBurnCapsPrivate *priv;
} RejillaBurnCaps;

typedef struct {
	GObjectClass parent_class;
} RejillaBurnCapsClass;

GType rejilla_burn_caps_get_type (void);

RejillaBurnCaps *rejilla_burn_caps_get_default (void);

RejillaPlugin *
rejilla_caps_link_need_download (RejillaCapsLink *link);

gboolean
rejilla_caps_link_active (RejillaCapsLink *link,
                          gboolean ignore_plugin_errors);

gboolean
rejilla_burn_caps_is_input (RejillaBurnCaps *self,
			    RejillaCaps *input);

RejillaCaps *
rejilla_burn_caps_find_start_caps (RejillaBurnCaps *self,
				   RejillaTrackType *output);

gboolean
rejilla_caps_is_compatible_type (const RejillaCaps *caps,
				 const RejillaTrackType *type);

RejillaBurnResult
rejilla_caps_link_check_recorder_flags_for_input (RejillaCapsLink *link,
                                                  RejillaBurnFlag session_flags);

G_END_DECLS

#endif /* BURN_CAPS_H */
