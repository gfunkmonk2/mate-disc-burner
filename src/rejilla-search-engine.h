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

#ifndef _SEARCH_ENGINE_H
#define _SEARCH_ENGINE_H

#include <glib.h>
#include <glib-object.h>

#include <gtk/gtk.h>

G_BEGIN_DECLS

enum {
	REJILLA_SEARCH_TREE_HIT_COL		= 0,
	REJILLA_SEARCH_TREE_NB_COL
};

typedef enum {
	REJILLA_SEARCH_SCOPE_ANY		= 0,
	REJILLA_SEARCH_SCOPE_VIDEO		= 1,
	REJILLA_SEARCH_SCOPE_MUSIC		= 1 << 1,
	REJILLA_SEARCH_SCOPE_PICTURES	= 1 << 2,
	REJILLA_SEARCH_SCOPE_DOCUMENTS	= 1 << 3,
} RejillaSearchScope;

#define REJILLA_TYPE_SEARCH_ENGINE         (rejilla_search_engine_get_type ())
#define REJILLA_SEARCH_ENGINE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), REJILLA_TYPE_SEARCH_ENGINE, RejillaSearchEngine))
#define REJILLA_IS_SEARCH_ENGINE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), REJILLA_TYPE_SEARCH_ENGINE))
#define REJILLA_SEARCH_ENGINE_GET_IFACE(o) (G_TYPE_INSTANCE_GET_INTERFACE ((o), REJILLA_TYPE_SEARCH_ENGINE, RejillaSearchEngineIface))

typedef struct _RejillaSearchEngine        RejillaSearchEngine;
typedef struct _RejillaSearchEngineIface   RejillaSearchEngineIface;

struct _RejillaSearchEngineIface {
	GTypeInterface g_iface;

	/* <Signals> */
	void	(*search_error)				(RejillaSearchEngine *search,
							 GError *error);
	void	(*search_finished)			(RejillaSearchEngine *search);
	void	(*hit_removed)				(RejillaSearchEngine *search,
					                 gpointer hit);
	void	(*hit_added)				(RejillaSearchEngine *search,
						         gpointer hit);

	/* <Virtual functions> */
	gboolean	(*is_available)			(RejillaSearchEngine *search);
	gboolean	(*query_new)			(RejillaSearchEngine *search,
					                 const gchar *keywords);
	gboolean	(*query_set_scope)		(RejillaSearchEngine *search,
					                 RejillaSearchScope scope);
	gboolean	(*query_set_mime)		(RejillaSearchEngine *search,
					                 const gchar **mimes);
	gboolean	(*query_start)			(RejillaSearchEngine *search);

	gboolean	(*add_hits)			(RejillaSearchEngine *search,
					                 GtkTreeModel *model,
					                 gint range_start,
					                 gint range_end);

	gint		(*num_hits)			(RejillaSearchEngine *engine);

	const gchar*	(*uri_from_hit)			(RejillaSearchEngine *engine,
				                         gpointer hit);
	const gchar *	(*mime_from_hit)		(RejillaSearchEngine *engine,
				                	 gpointer hit);
	gint		(*score_from_hit)		(RejillaSearchEngine *engine,
							 gpointer hit);
};

GType rejilla_search_engine_get_type (void);

RejillaSearchEngine *
rejilla_search_engine_new_default (void);

gboolean
rejilla_search_engine_is_available (RejillaSearchEngine *search);

gint
rejilla_search_engine_num_hits (RejillaSearchEngine *search);

gboolean
rejilla_search_engine_new_query (RejillaSearchEngine *search,
                                 const gchar *keywords);

gboolean
rejilla_search_engine_set_query_scope (RejillaSearchEngine *search,
                                       RejillaSearchScope scope);

gboolean
rejilla_search_engine_set_query_mime (RejillaSearchEngine *search,
                                      const gchar **mimes);

gboolean
rejilla_search_engine_start_query (RejillaSearchEngine *search);

void
rejilla_search_engine_query_finished (RejillaSearchEngine *search);

void
rejilla_search_engine_query_error (RejillaSearchEngine *search,
                                   GError *error);

void
rejilla_search_engine_hit_removed (RejillaSearchEngine *search,
                                   gpointer hit);
void
rejilla_search_engine_hit_added (RejillaSearchEngine *search,
                                 gpointer hit);

const gchar *
rejilla_search_engine_uri_from_hit (RejillaSearchEngine *search,
                                     gpointer hit);
const gchar *
rejilla_search_engine_mime_from_hit (RejillaSearchEngine *search,
                                     gpointer hit);
gint
rejilla_search_engine_score_from_hit (RejillaSearchEngine *search,
                                      gpointer hit);

gint
rejilla_search_engine_add_hits (RejillaSearchEngine *search,
                                GtkTreeModel *model,
                                gint range_start,
                                gint range_end);

G_END_DECLS

#endif /* _SEARCH_ENGINE_H */
