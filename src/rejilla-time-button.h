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

#ifndef _REJILLA_TIME_BUTTON_H_
#define _REJILLA_TIME_BUTTON_H_

#include <glib-object.h>

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define REJILLA_TYPE_TIME_BUTTON             (rejilla_time_button_get_type ())
#define REJILLA_TIME_BUTTON(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_TIME_BUTTON, RejillaTimeButton))
#define REJILLA_TIME_BUTTON_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_TIME_BUTTON, RejillaTimeButtonClass))
#define REJILLA_IS_TIME_BUTTON(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_TIME_BUTTON))
#define REJILLA_IS_TIME_BUTTON_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_TIME_BUTTON))
#define REJILLA_TIME_BUTTON_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_TIME_BUTTON, RejillaTimeButtonClass))

typedef struct _RejillaTimeButtonClass RejillaTimeButtonClass;
typedef struct _RejillaTimeButton RejillaTimeButton;

struct _RejillaTimeButtonClass
{
	GtkHBoxClass parent_class;

	void		(*value_changed)	(RejillaTimeButton *self);
};

struct _RejillaTimeButton
{
	GtkHBox parent_instance;
};

GType rejilla_time_button_get_type (void) G_GNUC_CONST;

GtkWidget *
rejilla_time_button_new (void);

gint64
rejilla_time_button_get_value (RejillaTimeButton *time);

void
rejilla_time_button_set_value (RejillaTimeButton *time,
			       gint64 value);
void
rejilla_time_button_set_max (RejillaTimeButton *time,
			     gint64 max);

void
rejilla_time_button_set_show_frames (RejillaTimeButton *time,
				     gboolean show);

G_END_DECLS

#endif /* _REJILLA_TIME_BUTTON_H_ */
