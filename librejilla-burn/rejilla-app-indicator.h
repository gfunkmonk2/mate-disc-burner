/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Librejilla-burn
 * Copyright (C) Canonical 2010
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

#ifndef APP_INDICATOR_H
#define APP_INDICATOR_H

#include <glib.h>
#include <glib-object.h>

#include <gtk/gtk.h>

#include "burn-basics.h"

G_BEGIN_DECLS

#define REJILLA_TYPE_APPINDICATOR         (rejilla_app_indicator_get_type ())
#define REJILLA_APPINDICATOR(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), REJILLA_TYPE_APPINDICATOR, RejillaAppIndicator))
#define REJILLA_APPINDICATOR_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), REJILLA_TYPE_APPINDICATOR, RejillaAppIndicatorClass))
#define REJILLA_IS_APPINDICATOR(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), REJILLA_TYPE_APPINDICATOR))
#define REJILLA_IS_APPINDICATOR_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), REJILLA_TYPE_APPINDICATOR))
#define REJILLA_APPINDICATOR_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), REJILLA_TYPE_APPINDICATOR, RejillaAppIndicatorClass))

typedef struct RejillaAppIndicatorPrivate RejillaAppIndicatorPrivate;

typedef struct {
	GObject parent;
	RejillaAppIndicatorPrivate *priv;
} RejillaAppIndicator;

typedef struct {
	GObjectClass parent_class;

	void		(*show_dialog)		(RejillaAppIndicator *indicator, gboolean show);
	void		(*close_after)		(RejillaAppIndicator *indicator, gboolean close);
	void		(*cancel)		(RejillaAppIndicator *indicator);

} RejillaAppIndicatorClass;

GType rejilla_app_indicator_get_type (void);
RejillaAppIndicator *rejilla_app_indicator_new (void);

void
rejilla_app_indicator_set_progress (RejillaAppIndicator *indicator,
				    gdouble fraction,
				    long remaining);

void
rejilla_app_indicator_set_action (RejillaAppIndicator *indicator,
				  RejillaBurnAction action,
				  const gchar *string);

void
rejilla_app_indicator_set_show_dialog (RejillaAppIndicator *indicator,
				       gboolean show);

void
rejilla_app_indicator_hide (RejillaAppIndicator *indicator);

G_END_DECLS

#endif /* APP_INDICATOR_H */
