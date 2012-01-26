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

#ifndef _REJILLA_JACKET_VIEW_H_
#define _REJILLA_JACKET_VIEW_H_

#include <glib-object.h>

#include <gtk/gtk.h>

#include "rejilla-jacket-background.h"

G_BEGIN_DECLS

typedef enum {
	REJILLA_JACKET_FRONT		= 0,
	REJILLA_JACKET_BACK		= 1,
} RejillaJacketSide;

#define COVER_HEIGHT_FRONT_MM		120.0
#define COVER_WIDTH_FRONT_MM		120.0
#define COVER_WIDTH_FRONT_INCH		4.724
#define COVER_HEIGHT_FRONT_INCH		4.724

#define COVER_HEIGHT_BACK_MM		117.5
#define COVER_WIDTH_BACK_MM		152.0
#define COVER_HEIGHT_BACK_INCH		4.646
#define COVER_WIDTH_BACK_INCH		5.984

#define COVER_HEIGHT_SIDE_MM		COVER_HEIGHT_BACK_MM
#define COVER_WIDTH_SIDE_MM		6.0
#define COVER_HEIGHT_SIDE_INCH		COVER_HEIGHT_BACK_INCH
#define COVER_WIDTH_SIDE_INCH		0.235

#define COVER_TEXT_MARGIN		/*1.*/0.03 //0.079

#define REJILLA_TYPE_JACKET_VIEW             (rejilla_jacket_view_get_type ())
#define REJILLA_JACKET_VIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_JACKET_VIEW, RejillaJacketView))
#define REJILLA_JACKET_VIEW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_JACKET_VIEW, RejillaJacketViewClass))
#define REJILLA_IS_JACKET_VIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_JACKET_VIEW))
#define REJILLA_IS_JACKET_VIEW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_JACKET_VIEW))
#define REJILLA_JACKET_VIEW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_JACKET_VIEW, RejillaJacketViewClass))

typedef struct _RejillaJacketViewClass RejillaJacketViewClass;
typedef struct _RejillaJacketView RejillaJacketView;

struct _RejillaJacketViewClass
{
	GtkContainerClass parent_class;
};

struct _RejillaJacketView
{
	GtkContainer parent_instance;
};

GType rejilla_jacket_view_get_type (void) G_GNUC_CONST;

GtkWidget *
rejilla_jacket_view_new (void);

void
rejilla_jacket_view_add_default_tag (RejillaJacketView *self,
				     GtkTextTag *tag);

void
rejilla_jacket_view_set_side (RejillaJacketView *view,
			      RejillaJacketSide side);

void
rejilla_jacket_view_set_image_style (RejillaJacketView *view,
				     RejillaJacketImageStyle style);

void
rejilla_jacket_view_set_color_background (RejillaJacketView *view,
					  GdkColor *color,
					  GdkColor *color2);
void
rejilla_jacket_view_set_color_style (RejillaJacketView *view,
				     RejillaJacketColorStyle style);

const gchar *
rejilla_jacket_view_get_image (RejillaJacketView *self);

const gchar *
rejilla_jacket_view_set_image (RejillaJacketView *view,
			       const gchar *path);

void
rejilla_jacket_view_configure_background (RejillaJacketView *view);

guint
rejilla_jacket_view_print (RejillaJacketView *view,
			   GtkPrintContext *context,
			   gdouble x,
			   gdouble y);

GtkTextBuffer *
rejilla_jacket_view_get_active_buffer (RejillaJacketView *view);

GtkTextBuffer *
rejilla_jacket_view_get_body_buffer (RejillaJacketView *view);

GtkTextBuffer *
rejilla_jacket_view_get_side_buffer (RejillaJacketView *view);

GtkTextAttributes *
rejilla_jacket_view_get_attributes (RejillaJacketView *view,
				    GtkTextIter *iter);

G_END_DECLS

#endif /* _REJILLA_JACKET_VIEW_H_ */
