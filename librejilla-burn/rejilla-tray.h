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

#ifndef TRAY_H
#define TRAY_H

#include <glib.h>
#include <glib-object.h>

#include <gtk/gtk.h>

#include "burn-basics.h"

G_BEGIN_DECLS

#define REJILLA_TYPE_TRAYICON         (rejilla_tray_icon_get_type ())
#define REJILLA_TRAYICON(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), REJILLA_TYPE_TRAYICON, RejillaTrayIcon))
#define REJILLA_TRAYICON_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), REJILLA_TYPE_TRAYICON, RejillaTrayIconClass))
#define REJILLA_IS_TRAYICON(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), REJILLA_TYPE_TRAYICON))
#define REJILLA_IS_TRAYICON_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), REJILLA_TYPE_TRAYICON))
#define REJILLA_TRAYICON_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), REJILLA_TYPE_TRAYICON, RejillaTrayIconClass))

typedef struct RejillaTrayIconPrivate RejillaTrayIconPrivate;

typedef struct {
	GtkStatusIcon parent;
	RejillaTrayIconPrivate *priv;
} RejillaTrayIcon;

typedef struct {
	GtkStatusIconClass parent_class;

	void		(*show_dialog)		(RejillaTrayIcon *tray, gboolean show);
	void		(*close_after)		(RejillaTrayIcon *tray, gboolean close);
	void		(*cancel)		(RejillaTrayIcon *tray);

} RejillaTrayIconClass;

GType rejilla_tray_icon_get_type (void);
RejillaTrayIcon *rejilla_tray_icon_new (void);

void
rejilla_tray_icon_set_progress (RejillaTrayIcon *tray,
				gdouble fraction,
				long remaining);

void
rejilla_tray_icon_set_action (RejillaTrayIcon *tray,
			      RejillaBurnAction action,
			      const gchar *string);

void
rejilla_tray_icon_set_show_dialog (RejillaTrayIcon *tray,
				   gboolean show);

G_END_DECLS

#endif /* TRAY_H */
