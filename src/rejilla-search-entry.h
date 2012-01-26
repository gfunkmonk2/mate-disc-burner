/***************************************************************************
*            search-entry.h
*
*  jeu mai 19 20:06:55 2005
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

#ifndef SEARCH_ENTRY_H
#define SEARCH_ENTRY_H

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "rejilla-layout.h"
#include "rejilla-search-engine.h"

G_BEGIN_DECLS

#define REJILLA_TYPE_SEARCH_ENTRY         (rejilla_search_entry_get_type ())
#define REJILLA_SEARCH_ENTRY(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), REJILLA_TYPE_SEARCH_ENTRY, RejillaSearchEntry))
#define REJILLA_SEARCH_ENTRY_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), REJILLA_TYPE_SEARCH_ENTRY, RejillaSearchEntryClass))
#define REJILLA_IS_SEARCH_ENTRY(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), REJILLA_TYPE_SEARCH_ENTRY))
#define REJILLA_IS_SEARCH_ENTRY_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), REJILLA_TYPE_SEARCH_ENTRY))
#define REJILLA_SEARCH_ENTRY_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), REJILLA_TYPE_SEARCH_ENTRY, RejillaSearchEntryClass))

typedef struct RejillaSearchEntryPrivate RejillaSearchEntryPrivate;

typedef struct {
	GtkTable parent;
	RejillaSearchEntryPrivate *priv;
} RejillaSearchEntry;

typedef struct {
	GtkTableClass parent_class;

	void	(*activated)	(RejillaSearchEntry *entry);

} RejillaSearchEntryClass;

GType rejilla_search_entry_get_type (void);
GtkWidget *rejilla_search_entry_new (void);

gboolean
rejilla_search_entry_set_query (RejillaSearchEntry *entry,
                                RejillaSearchEngine *search);

void
rejilla_search_entry_set_context (RejillaSearchEntry *entry,
				  RejillaLayoutType type);

G_END_DECLS

#endif				/* SEARCH_ENTRY_H */
