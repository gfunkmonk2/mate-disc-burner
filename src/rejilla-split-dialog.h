/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * rejilla
 * Copyright (C) Philippe Rouquier 2007-2008 <bonfire-app@wanadoo.fr>
 * 
 *  Rejilla is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 * rejilla is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with rejilla.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifndef _REJILLA_SPLIT_DIALOG_H_
#define _REJILLA_SPLIT_DIALOG_H_

#include <glib-object.h>

#include <gtk/gtk.h>

G_BEGIN_DECLS

struct _RejillaAudioSlice {
	gint64 start;
	gint64 end;
};
typedef struct _RejillaAudioSlice RejillaAudioSlice;

#define REJILLA_TYPE_SPLIT_DIALOG             (rejilla_split_dialog_get_type ())
#define REJILLA_SPLIT_DIALOG(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_SPLIT_DIALOG, RejillaSplitDialog))
#define REJILLA_SPLIT_DIALOG_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_SPLIT_DIALOG, RejillaSplitDialogClass))
#define REJILLA_IS_SPLIT_DIALOG(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_SPLIT_DIALOG))
#define REJILLA_IS_SPLIT_DIALOG_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_SPLIT_DIALOG))
#define REJILLA_SPLIT_DIALOG_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_SPLIT_DIALOG, RejillaSplitDialogClass))

typedef struct _RejillaSplitDialogClass RejillaSplitDialogClass;
typedef struct _RejillaSplitDialog RejillaSplitDialog;

struct _RejillaSplitDialogClass
{
	GtkDialogClass parent_class;
};

struct _RejillaSplitDialog
{
	GtkDialog parent_instance;
};

GType rejilla_split_dialog_get_type (void) G_GNUC_CONST;

GtkWidget *
rejilla_split_dialog_new (void);

void
rejilla_split_dialog_set_uri (RejillaSplitDialog *dialog,
			      const gchar *uri,
                              const gchar *title,
                              const gchar *artist);
void
rejilla_split_dialog_set_boundaries (RejillaSplitDialog *dialog,
				     gint64 start,
				     gint64 end);

GSList *
rejilla_split_dialog_get_slices (RejillaSplitDialog *self);

G_END_DECLS

#endif /* _REJILLA_SPLIT_DIALOG_H_ */
