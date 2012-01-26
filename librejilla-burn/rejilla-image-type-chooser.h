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

#ifndef REJILLA_IMAGE_TYPE_CHOOSER_H
#define REJILLA_IMAGE_TYPE_CHOOSER_H

#include <glib.h>
#include <glib-object.h>

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define REJILLA_TYPE_IMAGE_TYPE_CHOOSER         (rejilla_image_type_chooser_get_type ())
#define REJILLA_IMAGE_TYPE_CHOOSER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), REJILLA_TYPE_IMAGE_TYPE_CHOOSER, RejillaImageTypeChooser))
#define REJILLA_IMAGE_TYPE_CHOOSER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), REJILLA_TYPE_IMAGE_TYPE_CHOOSER, RejillaImageTypeChooserClass))
#define REJILLA_IS_IMAGE_TYPE_CHOOSER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), REJILLA_TYPE_IMAGE_TYPE_CHOOSER))
#define REJILLA_IS_IMAGE_TYPE_CHOOSER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), REJILLA_TYPE_IMAGE_TYPE_CHOOSER))
#define REJILLA_IMAGE_TYPE_CHOOSER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), REJILLA_TYPE_IMAGE_TYPE_CHOOSER, RejillaImageTypeChooserClass))

typedef struct _RejillaImageTypeChooser RejillaImageTypeChooser;
typedef struct _RejillaImageTypeChooserPrivate RejillaImageTypeChooserPrivate;
typedef struct _RejillaImageTypeChooserClass RejillaImageTypeChooserClass;

struct _RejillaImageTypeChooser {
	GtkHBox parent;
};

struct _RejillaImageTypeChooserClass {
	GtkHBoxClass parent_class;
};

GType rejilla_image_type_chooser_get_type (void);
GtkWidget *rejilla_image_type_chooser_new (void);

guint
rejilla_image_type_chooser_set_formats (RejillaImageTypeChooser *self,
				        RejillaImageFormat formats,
                                        gboolean show_autodetect,
                                        gboolean is_video);
void
rejilla_image_type_chooser_set_format (RejillaImageTypeChooser *self,
				       RejillaImageFormat format);
void
rejilla_image_type_chooser_get_format (RejillaImageTypeChooser *self,
				       RejillaImageFormat *format);
gboolean
rejilla_image_type_chooser_get_VCD_type (RejillaImageTypeChooser *chooser);

void
rejilla_image_type_chooser_set_VCD_type (RejillaImageTypeChooser *chooser,
                                         gboolean is_svcd);

G_END_DECLS

#endif /* REJILLA_IMAGE_TYPE_CHOOSER_H */
