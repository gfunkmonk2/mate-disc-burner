/*
 *  Rejilla is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Rejilla is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */
/***************************************************************************
*            play-list.h
*
*  mer mai 25 22:22:53 2005
*  Copyright  2005  Philippe Rouquier
*  rejilla-app@wanadoo.fr
****************************************************************************/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef BUILD_PLAYLIST

#ifndef PLAY_LIST_H
#define PLAY_LIST_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define REJILLA_TYPE_PLAYLIST         (rejilla_playlist_get_type ())
#define REJILLA_PLAYLIST(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), REJILLA_TYPE_PLAYLIST, RejillaPlaylist))
#define REJILLA_PLAYLIST_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), REJILLA_TYPE_PLAYLIST, RejillaPlaylistClass))
#define REJILLA_IS_PLAY_LIST(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), REJILLA_TYPE_PLAYLIST))
#define REJILLA_IS_PLAY_LIST_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), REJILLA_TYPE_PLAYLIST))
#define REJILLA_PLAYLIST_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), REJILLA_TYPE_PLAYLIST, RejillaPlaylistClass))
typedef struct RejillaPlaylistPrivate RejillaPlaylistPrivate;

typedef struct {
	GtkVBox parent;
	RejillaPlaylistPrivate *priv;
} RejillaPlaylist;

typedef struct {
	GtkVBoxClass parent_class;
} RejillaPlaylistClass;

GType rejilla_playlist_get_type (void);
GtkWidget *rejilla_playlist_new (void);

G_END_DECLS

#endif				/* PLAY_LIST_H */

#endif
