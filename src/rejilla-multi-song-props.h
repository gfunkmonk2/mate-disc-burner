/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Rejilla
 * Copyright (C) Philippe Rouquier 2005-2009 <bonfire-app@wanadoo.fr>
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

#ifndef _REJILLA_MULTI_SONG_PROPS_H_
#define _REJILLA_MULTI_SONG_PROPS_H_

#include <glib.h>
#include <glib-object.h>

#include <gtk/gtk.h>

#include "rejilla-rename.h"

G_BEGIN_DECLS

#define REJILLA_TYPE_MULTI_SONG_PROPS             (rejilla_multi_song_props_get_type ())
#define REJILLA_MULTI_SONG_PROPS(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_MULTI_SONG_PROPS, RejillaMultiSongProps))
#define REJILLA_MULTI_SONG_PROPS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_MULTI_SONG_PROPS, RejillaMultiSongPropsClass))
#define REJILLA_IS_MULTI_SONG_PROPS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_MULTI_SONG_PROPS))
#define REJILLA_IS_MULTI_SONG_PROPS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_MULTI_SONG_PROPS))
#define REJILLA_MULTI_SONG_PROPS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_MULTI_SONG_PROPS, RejillaMultiSongPropsClass))

typedef struct _RejillaMultiSongPropsClass RejillaMultiSongPropsClass;
typedef struct _RejillaMultiSongProps RejillaMultiSongProps;

struct _RejillaMultiSongPropsClass
{
	GtkDialogClass parent_class;
};

struct _RejillaMultiSongProps
{
	GtkDialog parent_instance;
};

GType rejilla_multi_song_props_get_type (void) G_GNUC_CONST;

GtkWidget *
rejilla_multi_song_props_new (void);

void
rejilla_multi_song_props_set_show_gap (RejillaMultiSongProps *props,
				       gboolean show);

void
rejilla_multi_song_props_set_rename_callback (RejillaMultiSongProps *props,
					      GtkTreeSelection *selection,
					      gint column_num,
					      RejillaRenameCallback callback);
void
rejilla_multi_song_props_get_properties (RejillaMultiSongProps *props,
					 gchar **artist,
					 gchar **composer,
					 gint *isrc,
					 gint64 *gap);

G_END_DECLS

#endif /* _REJILLA_MULTI_SONG_PROPS_H_ */
