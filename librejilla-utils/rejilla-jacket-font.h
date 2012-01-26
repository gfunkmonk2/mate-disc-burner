/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Librejilla-misc
 * Copyright (C) Philippe Rouquier 2005-2009 <bonfire-app@wanadoo.fr>
 *
 * Librejilla-misc is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * The Librejilla-misc authors hereby grant permission for non-GPL compatible
 * GStreamer plugins to be used and distributed together with GStreamer
 * and Librejilla-misc. This permission is above and beyond the permissions granted
 * by the GPL license by which Librejilla-burn is covered. If you modify this code
 * you may extend this exception to your version of the code, but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version.
 * 
 * Librejilla-misc is distributed in the hope that it will be useful,
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

#ifndef _REJILLA_JACKET_FONT_H_
#define _REJILLA_JACKET_FONT_H_

#include <glib-object.h>

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define REJILLA_TYPE_JACKET_FONT             (rejilla_jacket_font_get_type ())
#define REJILLA_JACKET_FONT(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_JACKET_FONT, RejillaJacketFont))
#define REJILLA_JACKET_FONT_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_JACKET_FONT, RejillaJacketFontClass))
#define REJILLA_IS_JACKET_FONT(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_JACKET_FONT))
#define REJILLA_IS_JACKET_FONT_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_JACKET_FONT))
#define REJILLA_JACKET_FONT_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_JACKET_FONT, RejillaJacketFontClass))

typedef struct _RejillaJacketFontClass RejillaJacketFontClass;
typedef struct _RejillaJacketFont RejillaJacketFont;

struct _RejillaJacketFontClass
{
	GtkHBoxClass parent_class;
};

struct _RejillaJacketFont
{
	GtkHBox parent_instance;
};

GType rejilla_jacket_font_get_type (void) G_GNUC_CONST;

GtkWidget *
rejilla_jacket_font_new (void);

void
rejilla_jacket_font_set_name (RejillaJacketFont *font,
			      const gchar *name);

gchar *
rejilla_jacket_font_get_family (RejillaJacketFont *font);

guint
rejilla_jacket_font_get_size (RejillaJacketFont *font);

G_END_DECLS

#endif /* _REJILLA_JACKET_FONT_H_ */
