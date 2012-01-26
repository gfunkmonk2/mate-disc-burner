/***************************************************************************
 *            rejilla-disc.h
 *
 *  dim nov 27 14:58:13 2005
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

#ifndef DISC_H
#define DISC_H

#include <glib.h>
#include <glib-object.h>

#include <gtk/gtk.h>

#include "rejilla-project-parse.h"
#include "rejilla-session.h"

G_BEGIN_DECLS

#define REJILLA_TYPE_DISC         (rejilla_disc_get_type ())
#define REJILLA_DISC(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), REJILLA_TYPE_DISC, RejillaDisc))
#define REJILLA_IS_DISC(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), REJILLA_TYPE_DISC))
#define REJILLA_DISC_GET_IFACE(o) (G_TYPE_INSTANCE_GET_INTERFACE ((o), REJILLA_TYPE_DISC, RejillaDiscIface))

#define REJILLA_DISC_ACTION "DiscAction"


typedef enum {
	REJILLA_DISC_OK = 0,
	REJILLA_DISC_NOT_IN_TREE,
	REJILLA_DISC_NOT_READY,
	REJILLA_DISC_LOADING,
	REJILLA_DISC_BROKEN_SYMLINK,
	REJILLA_DISC_CANCELLED,
	REJILLA_DISC_ERROR_SIZE,
	REJILLA_DISC_ERROR_EMPTY_SELECTION,
	REJILLA_DISC_ERROR_FILE_NOT_FOUND,
	REJILLA_DISC_ERROR_UNREADABLE,
	REJILLA_DISC_ERROR_ALREADY_IN_TREE,
	REJILLA_DISC_ERROR_JOLIET,
	REJILLA_DISC_ERROR_FILE_TYPE,
	REJILLA_DISC_ERROR_THREAD,
	REJILLA_DISC_ERROR_UNKNOWN
} RejillaDiscResult;

typedef struct _RejillaDisc        RejillaDisc;
typedef struct _RejillaDiscIface   RejillaDiscIface;

struct _RejillaDiscIface {
	GTypeInterface g_iface;

	/* signals */
	void	(*selection_changed)			(RejillaDisc *disc);

	/* virtual functions */
	RejillaDiscResult	(*set_session_contents)	(RejillaDisc *disc,
							 RejillaBurnSession *session);

	RejillaDiscResult	(*add_uri)		(RejillaDisc *disc,
							 const gchar *uri);

	gboolean		(*is_empty)		(RejillaDisc *disc);
	gboolean		(*get_selected_uri)	(RejillaDisc *disc,
							 gchar **uri);
	gboolean		(*get_boundaries)	(RejillaDisc *disc,
							 gint64 *start,
							 gint64 *end);

	void			(*delete_selected)	(RejillaDisc *disc);
	void			(*clear)		(RejillaDisc *disc);

	guint			(*add_ui)		(RejillaDisc *disc,
							 GtkUIManager *manager,
							 GtkWidget *message);
};

GType rejilla_disc_get_type (void);

guint
rejilla_disc_add_ui (RejillaDisc *disc,
		     GtkUIManager *manager,
		     GtkWidget *message);

RejillaDiscResult
rejilla_disc_add_uri (RejillaDisc *disc, const gchar *escaped_uri);

gboolean
rejilla_disc_get_selected_uri (RejillaDisc *disc, gchar **uri);

gboolean
rejilla_disc_get_boundaries (RejillaDisc *disc,
			     gint64 *start,
			     gint64 *end);

void
rejilla_disc_delete_selected (RejillaDisc *disc);

gboolean
rejilla_disc_clear (RejillaDisc *disc);

RejillaDiscResult
rejilla_disc_get_status (RejillaDisc *disc,
			 gint *remaining,
			 gchar **current_task);

RejillaDiscResult
rejilla_disc_set_session_contents (RejillaDisc *disc,
				   RejillaBurnSession *session);

gboolean
rejilla_disc_is_empty (RejillaDisc *disc);

void
rejilla_disc_selection_changed (RejillaDisc *disc);

G_END_DECLS

#endif /* DISC_H */
