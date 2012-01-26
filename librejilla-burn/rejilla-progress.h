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

#ifndef PROGRESS_H
#define PROGRESS_H

#include <glib.h>
#include <glib-object.h>

#include <gtk/gtk.h>

#include "burn-basics.h"
#include "rejilla-media.h"

G_BEGIN_DECLS

#define REJILLA_TYPE_BURN_PROGRESS         (rejilla_burn_progress_get_type ())
#define REJILLA_BURN_PROGRESS(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), REJILLA_TYPE_BURN_PROGRESS, RejillaBurnProgress))
#define REJILLA_BURN_PROGRESS_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), REJILLA_TYPE_BURN_PROGRESS, RejillaBurnProgressClass))
#define REJILLA_IS_BURN_PROGRESS(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), REJILLA_TYPE_BURN_PROGRESS))
#define REJILLA_IS_BURN_PROGRESS_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), REJILLA_TYPE_BURN_PROGRESS))
#define REJILLA_BURN_PROGRESS_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), REJILLA_TYPE_BURN_PROGRESS, RejillaBurnProgressClass))

typedef struct RejillaBurnProgressPrivate RejillaBurnProgressPrivate;

typedef struct {
	GtkVBox parent;
	RejillaBurnProgressPrivate *priv;
} RejillaBurnProgress;

typedef struct {
	GtkVBoxClass parent_class;
} RejillaBurnProgressClass;

GType rejilla_burn_progress_get_type (void);

GtkWidget *rejilla_burn_progress_new (void);

void
rejilla_burn_progress_reset (RejillaBurnProgress *progress);

void
rejilla_burn_progress_set_status (RejillaBurnProgress *progress,
				  RejillaMedia media,
				  gdouble overall_progress,
				  gdouble action_progress,
				  glong remaining,
				  gint mb_isosize,
				  gint mb_written,
				  gint64 rate);
void
rejilla_burn_progress_display_session_info (RejillaBurnProgress *progress,
					    glong time,
					    gint64 rate,
					    RejillaMedia media,
					    gint mb_written);

void
rejilla_burn_progress_set_action (RejillaBurnProgress *progress,
				  RejillaBurnAction action,
				  const gchar *string);

G_END_DECLS

#endif /* PROGRESS_H */
