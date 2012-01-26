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

#ifndef _REJILLA_DATA_DISC_H_
#define _REJILLA_DATA_DISC_H_

#include <glib-object.h>

#include <gtk/gtk.h>

#include "rejilla-medium.h"

G_BEGIN_DECLS

#define REJILLA_TYPE_DATA_DISC             (rejilla_data_disc_get_type ())
#define REJILLA_DATA_DISC(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_DATA_DISC, RejillaDataDisc))
#define REJILLA_DATA_DISC_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_DATA_DISC, RejillaDataDiscClass))
#define REJILLA_IS_DATA_DISC(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_DATA_DISC))
#define REJILLA_IS_DATA_DISC_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_DATA_DISC))
#define REJILLA_DATA_DISC_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_DATA_DISC, RejillaDataDiscClass))

typedef struct _RejillaDataDiscClass RejillaDataDiscClass;
typedef struct _RejillaDataDisc RejillaDataDisc;

struct _RejillaDataDiscClass
{
	GtkVBoxClass parent_class;
};

struct _RejillaDataDisc
{
	GtkVBox parent_instance;
};

GType rejilla_data_disc_get_type (void) G_GNUC_CONST;

GtkWidget *
rejilla_data_disc_new (void);

void
rejilla_data_disc_set_right_button_group (RejillaDataDisc *disc,
					  GtkSizeGroup *size_group);

RejillaMedium *
rejilla_data_disc_get_loaded_medium (RejillaDataDisc *disc);

G_END_DECLS

#endif /* _REJILLA_DATA_DISC_H_ */
