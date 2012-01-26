/***************************************************************************
 *            rejilla-uri-container.h
 *
 *  lun mai 22 08:54:18 2006
 *  Copyright  2006  Rouquier Philippe
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

#ifndef REJILLA_URI_CONTAINER_H
#define REJILLA_URI_CONTAINER_H

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define REJILLA_TYPE_URI_CONTAINER         (rejilla_uri_container_get_type ())
#define REJILLA_URI_CONTAINER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), REJILLA_TYPE_URI_CONTAINER, RejillaURIContainer))
#define REJILLA_IS_URI_CONTAINER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), REJILLA_TYPE_URI_CONTAINER))
#define REJILLA_URI_CONTAINER_GET_IFACE(o) (G_TYPE_INSTANCE_GET_INTERFACE ((o), REJILLA_TYPE_URI_CONTAINER, RejillaURIContainerIFace))


typedef struct _RejillaURIContainer RejillaURIContainer;

typedef struct {
	GTypeInterface g_iface;

	/* signals */
	void		(*uri_selected)		(RejillaURIContainer *container);
	void		(*uri_activated)	(RejillaURIContainer *container);

	/* virtual functions */
	gboolean	(*get_boundaries)	(RejillaURIContainer *container,
						 gint64 *start,
						 gint64 *end);
	gchar*		(*get_selected_uri)	(RejillaURIContainer *container);
	gchar**		(*get_selected_uris)	(RejillaURIContainer *container);

} RejillaURIContainerIFace;


GType rejilla_uri_container_get_type (void);

gboolean
rejilla_uri_container_get_boundaries (RejillaURIContainer *container,
				      gint64 *start,
				      gint64 *end);
gchar *
rejilla_uri_container_get_selected_uri (RejillaURIContainer *container);
gchar **
rejilla_uri_container_get_selected_uris (RejillaURIContainer *container);

void
rejilla_uri_container_uri_selected (RejillaURIContainer *container);
void
rejilla_uri_container_uri_activated (RejillaURIContainer *container);

G_END_DECLS

#endif /* REJILLA_URI_CONTAINER_H */
