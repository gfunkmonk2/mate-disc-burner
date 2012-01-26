/***************************************************************************
 *            rejilla-layout-object.h
 *
 *  dim oct 15 17:15:58 2006
 *  Copyright  2006  Philippe Rouquier
 *  bonfire-app@wanadoo.fr
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

#ifndef REJILLA_LAYOUT_OBJECT_H
#define REJILLA_LAYOUT_OBJECT_H

#include <glib.h>
#include <glib-object.h>

#include "rejilla-layout.h"

G_BEGIN_DECLS

#define REJILLA_TYPE_LAYOUT_OBJECT         (rejilla_layout_object_get_type ())
#define REJILLA_LAYOUT_OBJECT(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), REJILLA_TYPE_LAYOUT_OBJECT, RejillaLayoutObject))
#define REJILLA_IS_LAYOUT_OBJECT(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), REJILLA_TYPE_LAYOUT_OBJECT))
#define REJILLA_LAYOUT_OBJECT_GET_IFACE(o) (G_TYPE_INSTANCE_GET_INTERFACE ((o), REJILLA_TYPE_LAYOUT_OBJECT, RejillaLayoutObjectIFace))

typedef struct _RejillaLayoutObject RejillaLayoutObject;
typedef struct _RejillaLayoutIFace RejillaLayoutObjectIFace;

struct _RejillaLayoutIFace {
	GTypeInterface g_iface;

	void	(*get_proportion)	(RejillaLayoutObject *self,
					 gint *header,
					 gint *center,
					 gint *footer);
	void	(*set_context)		(RejillaLayoutObject *self,
					 RejillaLayoutType type);
};

GType rejilla_layout_object_get_type (void);

void rejilla_layout_object_get_proportion (RejillaLayoutObject *self,
					   gint *header,
					   gint *center,
					   gint *footer);

void rejilla_layout_object_set_context (RejillaLayoutObject *self,
					RejillaLayoutType type);

G_END_DECLS

#endif /* REJILLA_LAYOUT_OBJECT_H */
