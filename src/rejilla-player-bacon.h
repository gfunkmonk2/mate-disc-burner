/***************************************************************************
 *            player-bacon.h
 *
 *  ven déc 30 11:29:33 2005
 *  Copyright  2005  Rouquier Philippe
 *  rejilla-app@wanadoo.fr
 ***************************************************************************/

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

#ifndef PLAYER_BACON_H
#define PLAYER_BACON_H

#include <glib.h>
#include <glib-object.h>

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define REJILLA_TYPE_PLAYER_BACON         (rejilla_player_bacon_get_type ())
#define REJILLA_PLAYER_BACON(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), REJILLA_TYPE_PLAYER_BACON, RejillaPlayerBacon))
#define REJILLA_PLAYER_BACON_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), REJILLA_TYPE_PLAYER_BACON, RejillaPlayerBaconClass))
#define REJILLA_IS_PLAYER_BACON(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), REJILLA_TYPE_PLAYER_BACON))
#define REJILLA_IS_PLAYER_BACON_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), REJILLA_TYPE_PLAYER_BACON))
#define REJILLA_PLAYER_BACON_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), REJILLA_TYPE_PLAYER_BACON, RejillaPlayerBaconClass))

#define	PLAYER_BACON_WIDTH 120
#define	PLAYER_BACON_HEIGHT 90

typedef struct RejillaPlayerBaconPrivate RejillaPlayerBaconPrivate;

typedef enum {
	BACON_STATE_ERROR,
	BACON_STATE_READY,
	BACON_STATE_PAUSED,
	BACON_STATE_PLAYING
} RejillaPlayerBaconState;

typedef struct {
	GtkWidget parent;
	RejillaPlayerBaconPrivate *priv;
} RejillaPlayerBacon;

typedef struct {
	GtkWidgetClass parent_class;

	void	(*state_changed)	(RejillaPlayerBacon *bacon,
					 RejillaPlayerBaconState state);

	void	(*eof)			(RejillaPlayerBacon *bacon);

} RejillaPlayerBaconClass;

GType rejilla_player_bacon_get_type (void);
GtkWidget *rejilla_player_bacon_new (void);

void rejilla_player_bacon_set_uri (RejillaPlayerBacon *bacon, const gchar *uri);
void rejilla_player_bacon_set_volume (RejillaPlayerBacon *bacon, gdouble volume);
gboolean rejilla_player_bacon_set_boundaries (RejillaPlayerBacon *bacon, gint64 start, gint64 end);
gboolean rejilla_player_bacon_play (RejillaPlayerBacon *bacon);
gboolean rejilla_player_bacon_stop (RejillaPlayerBacon *bacon);
gboolean rejilla_player_bacon_set_pos (RejillaPlayerBacon *bacon, gdouble pos);
gboolean rejilla_player_bacon_get_pos (RejillaPlayerBacon *bacon, gint64 *pos);
gdouble  rejilla_player_bacon_get_volume (RejillaPlayerBacon *bacon);
gboolean rejilla_player_bacon_forward (RejillaPlayerBacon *bacon, gint64 value);
gboolean rejilla_player_bacon_backward (RejillaPlayerBacon *bacon, gint64 value);
G_END_DECLS

#endif /* PLAYER_BACON_H */
