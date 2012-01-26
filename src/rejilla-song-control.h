/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 4; tab-width: 4 -*- */
/*
 * rejilla
 * Copyright (C) Philippe Rouquier 2010 <bonfire-app@wanadoo.fr>
 * 
 * rejilla is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * rejilla is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _REJILLA_SONG_CONTROL_H_
#define _REJILLA_SONG_CONTROL_H_

#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define REJILLA_TYPE_SONG_CONTROL             (rejilla_song_control_get_type ())
#define REJILLA_SONG_CONTROL(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_SONG_CONTROL, RejillaSongControl))
#define REJILLA_SONG_CONTROL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_SONG_CONTROL, RejillaSongControlClass))
#define REJILLA_IS_SONG_CONTROL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_SONG_CONTROL))
#define REJILLA_IS_SONG_CONTROL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_SONG_CONTROL))
#define REJILLA_SONG_CONTROL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_SONG_CONTROL, RejillaSongControlClass))

typedef struct _RejillaSongControlClass RejillaSongControlClass;
typedef struct _RejillaSongControl RejillaSongControl;

struct _RejillaSongControlClass
{
	GtkAlignmentClass parent_class;
};

struct _RejillaSongControl
{
	GtkAlignment parent_instance;
};

GType rejilla_song_control_get_type (void) G_GNUC_CONST;

GtkWidget *
rejilla_song_control_new (void);

void
rejilla_song_control_set_uri (RejillaSongControl *player,
                              const gchar *uri);

void
rejilla_song_control_set_info (RejillaSongControl *player,
                               const gchar *title,
                               const gchar *artist);

void
rejilla_song_control_set_boundaries (RejillaSongControl *player, 
                                     gsize start,
                                     gsize end);

gint64
rejilla_song_control_get_pos (RejillaSongControl *control);

gint64
rejilla_song_control_get_length (RejillaSongControl *control);

const gchar *
rejilla_song_control_get_uri (RejillaSongControl *control);

G_END_DECLS

#endif /* _REJILLA_SONG_CONTROL_H_ */
