/***************************************************************************
 *            rejilla-uri-container.c
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

#include <glib.h>
#include <gtk/gtk.h>

#include "rejilla-uri-container.h"
 
static void rejilla_uri_container_base_init (gpointer g_class);

typedef enum {
	URI_ACTIVATED_SIGNAL,
	URI_SELECTED_SIGNAL,
	LAST_SIGNAL
} RejillaURIContainerSignalType;

static guint rejilla_uri_container_signals[LAST_SIGNAL] = { 0 };

GType
rejilla_uri_container_get_type ()
{
	static GType type = 0;

	if(type == 0) {
		static const GTypeInfo our_info = {
			sizeof (RejillaURIContainerIFace),
			rejilla_uri_container_base_init,
			NULL,
			NULL,
			NULL,
			NULL,
			0,
			0,
			NULL,
		};

		type = g_type_register_static (G_TYPE_INTERFACE, 
					       "RejillaURIContainer",
					       &our_info,
					       0);

		g_type_interface_add_prerequisite (type, G_TYPE_OBJECT);
	}

	return type;
}

static void
rejilla_uri_container_base_init (gpointer g_class)
{
	static gboolean initialized = FALSE;

	if (initialized)
		return;

	rejilla_uri_container_signals [URI_SELECTED_SIGNAL] =
	    g_signal_new ("uri_selected",
			  REJILLA_TYPE_URI_CONTAINER,
			  G_SIGNAL_RUN_LAST|G_SIGNAL_NO_RECURSE,
			  G_STRUCT_OFFSET (RejillaURIContainerIFace, uri_selected),
			  NULL,
			  NULL,
			  g_cclosure_marshal_VOID__VOID,
			  G_TYPE_NONE,
			  0);
	rejilla_uri_container_signals [URI_ACTIVATED_SIGNAL] =
	    g_signal_new ("uri_activated",
			  REJILLA_TYPE_URI_CONTAINER,
			  G_SIGNAL_RUN_LAST|G_SIGNAL_NO_RECURSE,
			  G_STRUCT_OFFSET (RejillaURIContainerIFace, uri_activated),
			  NULL,
			  NULL,
			  g_cclosure_marshal_VOID__VOID,
			  G_TYPE_NONE,
			  0);
	initialized = TRUE;
}

gboolean
rejilla_uri_container_get_boundaries (RejillaURIContainer *container,
				      gint64 *start,
				      gint64 *end)
{
	RejillaURIContainerIFace *iface;

	g_return_val_if_fail (REJILLA_IS_URI_CONTAINER (container), FALSE);

	if (!gtk_widget_get_mapped (GTK_WIDGET (container)))
		return FALSE;

	iface = REJILLA_URI_CONTAINER_GET_IFACE (container);
	if (iface->get_boundaries)
		return (* iface->get_boundaries) (container, start, end);

	return FALSE;
}

gchar *
rejilla_uri_container_get_selected_uri (RejillaURIContainer *container)
{
	RejillaURIContainerIFace *iface;

	g_return_val_if_fail (REJILLA_IS_URI_CONTAINER (container), NULL);

	if (!gtk_widget_get_mapped (GTK_WIDGET (container)))
		return NULL;

	iface = REJILLA_URI_CONTAINER_GET_IFACE (container);
	if (iface->get_selected_uri)
		return (* iface->get_selected_uri) (container);

	return NULL;
}

gchar **
rejilla_uri_container_get_selected_uris (RejillaURIContainer *container)
{
	RejillaURIContainerIFace *iface;

	g_return_val_if_fail (REJILLA_IS_URI_CONTAINER (container), NULL);

	if (!gtk_widget_get_mapped (GTK_WIDGET (container)))
		return NULL;

	iface = REJILLA_URI_CONTAINER_GET_IFACE (container);
	if (iface->get_selected_uris)
		return (* iface->get_selected_uris) (container);

	return NULL;
}

void
rejilla_uri_container_uri_selected (RejillaURIContainer *container)
{
	g_return_if_fail (REJILLA_IS_URI_CONTAINER (container));
	g_signal_emit (container,
		       rejilla_uri_container_signals [URI_SELECTED_SIGNAL],
		       0);
}

void
rejilla_uri_container_uri_activated (RejillaURIContainer *container)
{
	g_return_if_fail (REJILLA_IS_URI_CONTAINER (container));
	g_signal_emit (container,
		       rejilla_uri_container_signals [URI_ACTIVATED_SIGNAL],
		       0);
}
