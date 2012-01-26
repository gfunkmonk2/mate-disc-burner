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

#ifndef BLANK_DIALOG_H
#define BLANK_DIALOG_H

#include <glib.h>
#include <glib-object.h>

#include <gtk/gtk.h>

#include <rejilla-tool-dialog.h>

G_BEGIN_DECLS

#define REJILLA_TYPE_BLANK_DIALOG         (rejilla_blank_dialog_get_type ())
#define REJILLA_BLANK_DIALOG(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), REJILLA_TYPE_BLANK_DIALOG, RejillaBlankDialog))
#define REJILLA_BLANK_DIALOG_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), REJILLA_TYPE_BLANK_DIALOG, RejillaBlankDialogClass))
#define REJILLA_IS_BLANK_DIALOG(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), REJILLA_TYPE_BLANK_DIALOG))
#define REJILLA_IS_BLANK_DIALOG_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), REJILLA_TYPE_BLANK_DIALOG))
#define REJILLA_BLANK_DIALOG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), REJILLA_TYPE_BLANK_DIALOG, RejillaBlankDialogClass))

typedef struct {
	RejillaToolDialog parent;
} RejillaBlankDialog;

typedef struct {
	RejillaToolDialogClass parent_class;
} RejillaBlankDialogClass;

GType rejilla_blank_dialog_get_type (void);

RejillaBlankDialog *rejilla_blank_dialog_new (void);

G_END_DECLS

#endif				/* BLANK_DIALOG_H */
