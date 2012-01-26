/***************************************************************************
*            player.h
*
*  lun mai 30 08:15:01 2005
*  Copyright  2005  Philippe Rouquier
*  rejilla-app@wanadoo.fr
****************************************************************************/

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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifndef PLAYER_H
#define PLAYER_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define REJILLA_TYPE_PLAYER         (rejilla_player_get_type ())
#define REJILLA_PLAYER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), REJILLA_TYPE_PLAYER, RejillaPlayer))
#define REJILLA_PLAYER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), REJILLA_TYPE_PLAYER, RejillaPlayerClass))
#define REJILLA_IS_PLAYER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), REJILLA_TYPE_PLAYER))
#define REJILLA_IS_PLAYER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), REJILLA_TYPE_PLAYER))
#define REJILLA_PLAYER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), REJILLA_TYPE_PLAYER, RejillaPlayerClass))

typedef struct RejillaPlayerPrivate RejillaPlayerPrivate;

typedef struct {
	GtkAlignment parent;
	RejillaPlayerPrivate *priv;
} RejillaPlayer;

typedef struct {
	GtkAlignmentClass parent_class;

	void		(*error)	(RejillaPlayer *player);
	void		(*ready)	(RejillaPlayer *player);
} RejillaPlayerClass;

GType rejilla_player_get_type (void);
GtkWidget *rejilla_player_new (void);

void
rejilla_player_set_uri (RejillaPlayer *player,
			const gchar *uri);
void
rejilla_player_set_boundaries (RejillaPlayer *player, 
			       gint64 start,
			       gint64 end);

G_END_DECLS

#endif
