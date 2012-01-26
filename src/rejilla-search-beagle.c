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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>

#include <gio/gio.h>

#include <gtk/gtk.h>

#include <beagle/beagle.h>

#include "rejilla-search-beagle.h"
#include "rejilla-search-engine.h"


typedef struct _RejillaSearchBeaglePrivate RejillaSearchBeaglePrivate;
struct _RejillaSearchBeaglePrivate
{
	BeagleClient *client;
	BeagleQuery *query;

	GSList *hits;
};

#define REJILLA_SEARCH_BEAGLE_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REJILLA_TYPE_SEARCH_BEAGLE, RejillaSearchBeaglePrivate))

static void rejilla_search_beagle_init_engine (RejillaSearchEngineIface *iface);

G_DEFINE_TYPE_WITH_CODE (RejillaSearchBeagle,
			 rejilla_search_beagle,
			 G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (REJILLA_TYPE_SEARCH_ENGINE,
					        rejilla_search_beagle_init_engine));

static gboolean
rejilla_search_beagle_is_available (RejillaSearchEngine *engine)
{
	RejillaSearchBeaglePrivate *priv;

	priv = REJILLA_SEARCH_BEAGLE_PRIVATE (engine);
	if (priv->client)
		return TRUE;

	priv->client = beagle_client_new (NULL);
	return (priv->client != NULL);
}

static gint
rejilla_search_beagle_num_hits (RejillaSearchEngine *engine)
{
	RejillaSearchBeaglePrivate *priv;

	priv = REJILLA_SEARCH_BEAGLE_PRIVATE (engine);
	return g_slist_length (priv->hits);
}

static const gchar *
rejilla_search_beagle_uri_from_hit (RejillaSearchEngine *engine,
                                     gpointer hit)
{
	return beagle_hit_get_uri (BEAGLE_HIT (hit));
}

static const gchar *
rejilla_search_beagle_mime_from_hit (RejillaSearchEngine *engine,
                                     gpointer hit)
{
	return beagle_hit_get_mime_type (BEAGLE_HIT (hit));
}

static int
rejilla_search_beagle_score_from_hit (RejillaSearchEngine *engine,
                                      gpointer hit)
{
	return (int) (beagle_hit_get_score (BEAGLE_HIT (hit)) * 100);
}

static gboolean
rejilla_search_beagle_add_hit_to_tree (RejillaSearchEngine *search,
                                       GtkTreeModel *model,
                                       gint range_start,
                                       gint range_end)
{
	RejillaSearchBeaglePrivate *priv;
	GSList *iter;
	gint num;

	priv = REJILLA_SEARCH_BEAGLE_PRIVATE (search);

	num = 0;
	iter = g_slist_nth (priv->hits, range_start);

	for (; iter && num < range_end - range_start; iter = iter->next, num ++) {
		BeagleHit *hit;
		GtkTreeIter row;

		hit = iter->data;

		gtk_list_store_insert_with_values (GTK_LIST_STORE (model), &row, -1,
		                                   REJILLA_SEARCH_TREE_HIT_COL, hit,
		                                   -1);
	}

	return TRUE;
}

static void
rejilla_search_beagle_hit_added_cb (BeagleQuery *query,
				    BeagleHitsAddedResponse *response,
				    RejillaSearchBeagle *search)
{
	GSList *list;
	GSList *iter;
	RejillaSearchBeaglePrivate *priv;

	priv = REJILLA_SEARCH_BEAGLE_PRIVATE (search);

	/* NOTE : list must not be modified nor freed */
	list = beagle_hits_added_response_get_hits (response);
	list = g_slist_copy (list);

	if (priv->hits)
		priv->hits = g_slist_concat (priv->hits, list);
	else
		priv->hits = list;

	for (iter = list; iter; iter = iter->next) {
		BeagleHit *hit;

		hit = iter->data;
		beagle_hit_ref (hit);

		rejilla_search_engine_hit_added (REJILLA_SEARCH_ENGINE (search), hit);
	}
}

static void
rejilla_search_beagle_hit_substracted_cb (BeagleQuery *query,
					  BeagleHitsSubtractedResponse *response,
					  RejillaSearchBeagle *search)
{
	GSList *list, *iter;
	RejillaSearchBeaglePrivate *priv;

	priv = REJILLA_SEARCH_BEAGLE_PRIVATE (search);

	list = beagle_hits_subtracted_response_get_uris (response);
	for (iter = list; iter; iter = iter->next) {
		GSList *next, *hit_iter;
		const gchar *removed_uri;

		removed_uri = iter->data;

		/* see if it isn't in the hits that are still waiting */
		for (hit_iter = priv->hits; hit_iter; hit_iter = next) {
			BeagleHit *hit;
			const char *hit_uri;
	
			next = hit_iter->next;
			hit = hit_iter->data;

			hit_uri = beagle_hit_get_uri (hit);
			if (!strcmp (hit_uri, removed_uri)) {
				priv->hits = g_slist_remove (priv->hits, hit);
				rejilla_search_engine_hit_removed (REJILLA_SEARCH_ENGINE (search), hit);
				beagle_hit_unref (hit);
			}
		}
	}
}

static void
rejilla_search_beagle_finished_cb (BeagleQuery *query,
				   BeagleFinishedResponse *response,
				   RejillaSearchBeagle *search)
{
	rejilla_search_engine_query_finished (REJILLA_SEARCH_ENGINE (search));
}

static void
rejilla_search_beagle_error_cb (BeagleRequest *request,
				GError *error,
				RejillaSearchEngine *search)
{
	rejilla_search_engine_query_error (REJILLA_SEARCH_ENGINE (search), error);
}

static gboolean
rejilla_search_beagle_query_start (RejillaSearchEngine *search)
{
	RejillaSearchBeaglePrivate *priv;
	GError *error = NULL;

	priv = REJILLA_SEARCH_BEAGLE_PRIVATE (search);

	/* search itself */
	if (!priv->query) {
		g_warning ("No query");
		return FALSE;
	}

	beagle_query_set_max_hits (priv->query, 10000);
	g_signal_connect (G_OBJECT (priv->query), "hits-added",
			  G_CALLBACK (rejilla_search_beagle_hit_added_cb),
			  search);
	g_signal_connect (G_OBJECT (priv->query), "hits-subtracted",
			  G_CALLBACK
			  (rejilla_search_beagle_hit_substracted_cb),
			  search);
	g_signal_connect (G_OBJECT (priv->query), "finished",
			  G_CALLBACK (rejilla_search_beagle_finished_cb),
			  search);
	g_signal_connect (G_OBJECT (priv->query), "error",
			  G_CALLBACK (rejilla_search_beagle_error_cb),
			  search);

	beagle_client_send_request_async (priv->client,
					  BEAGLE_REQUEST (priv->query),
					  &error);
	if (error) {
		rejilla_search_engine_query_error (REJILLA_SEARCH_ENGINE (search), error);
		g_error_free (error);
		return FALSE;
	}

	return TRUE;
}

static void
rejilla_search_beagle_clean (RejillaSearchBeagle *beagle)
{
	RejillaSearchBeaglePrivate *priv;

	priv = REJILLA_SEARCH_BEAGLE_PRIVATE (beagle);

	if (priv->query) {
		g_object_unref (priv->query);
		priv->query = NULL;
	}

	if (priv->hits) {
		g_slist_foreach (priv->hits, (GFunc) beagle_hit_unref, NULL);
		g_slist_free (priv->hits);
		priv->hits = NULL;
	}
}

static gboolean
rejilla_search_beagle_query_new (RejillaSearchEngine *search,
				 const gchar *keywords)
{
	RejillaSearchBeaglePrivate *priv;
	BeagleQueryPartHuman *text;

	priv = REJILLA_SEARCH_BEAGLE_PRIVATE (search);

	rejilla_search_beagle_clean (REJILLA_SEARCH_BEAGLE (search));
	priv->query = beagle_query_new ();

	if (keywords) {
		BeagleQueryPartHuman *text;

		text = beagle_query_part_human_new ();
		beagle_query_part_human_set_string (text, keywords);
		beagle_query_part_set_logic (BEAGLE_QUERY_PART (text),
					     BEAGLE_QUERY_PART_LOGIC_REQUIRED);

		beagle_query_add_part (priv->query, BEAGLE_QUERY_PART (text));
	}

	text = beagle_query_part_human_new ();
	beagle_query_part_human_set_string (text, "type:File");
	beagle_query_add_part (priv->query, BEAGLE_QUERY_PART (text));

	return TRUE;
}

static gboolean
rejilla_search_beagle_query_set_scope (RejillaSearchEngine *search,
                                       RejillaSearchScope scope)
{
	BeagleQueryPartOr *or_part = NULL;
	RejillaSearchBeaglePrivate *priv;

	priv = REJILLA_SEARCH_BEAGLE_PRIVATE (search);

	if (!priv->query)
		return FALSE;

	if (scope & REJILLA_SEARCH_SCOPE_DOCUMENTS) {
		BeagleQueryPartProperty *filetype;

		if (!or_part)
			or_part = beagle_query_part_or_new ();

		filetype = beagle_query_part_property_new ();
		beagle_query_part_property_set_property_type (filetype, BEAGLE_PROPERTY_TYPE_KEYWORD);
		beagle_query_part_property_set_key (filetype, "beagle:FileType");
		beagle_query_part_property_set_value (filetype, "document");
		beagle_query_part_or_add_subpart (or_part, BEAGLE_QUERY_PART (filetype));
	}

	if (scope & REJILLA_SEARCH_SCOPE_PICTURES) {
		BeagleQueryPartProperty *filetype;

		if (!or_part)
			or_part = beagle_query_part_or_new ();

		filetype = beagle_query_part_property_new ();
		beagle_query_part_property_set_property_type (filetype, BEAGLE_PROPERTY_TYPE_KEYWORD);
		beagle_query_part_property_set_key (filetype, "beagle:FileType");
		beagle_query_part_property_set_value (filetype, "image");
		beagle_query_part_or_add_subpart (or_part, BEAGLE_QUERY_PART (filetype));
	}

	if (scope & REJILLA_SEARCH_SCOPE_MUSIC) {
		BeagleQueryPartProperty *filetype;

		if (!or_part)
			or_part = beagle_query_part_or_new ();

		filetype = beagle_query_part_property_new ();
		beagle_query_part_property_set_property_type (filetype, BEAGLE_PROPERTY_TYPE_KEYWORD);
		beagle_query_part_property_set_key (filetype, "beagle:FileType");
		beagle_query_part_property_set_value (filetype, "audio");
		beagle_query_part_or_add_subpart (or_part, BEAGLE_QUERY_PART (filetype));
	}

	if (scope & REJILLA_SEARCH_SCOPE_VIDEO) {
		BeagleQueryPartProperty *filetype;

		if (!or_part)
			or_part = beagle_query_part_or_new ();

		filetype = beagle_query_part_property_new ();
		beagle_query_part_property_set_property_type (filetype, BEAGLE_PROPERTY_TYPE_KEYWORD);
		beagle_query_part_property_set_key (filetype, "beagle:FileType");
		beagle_query_part_property_set_value (filetype, "video");
		beagle_query_part_or_add_subpart (or_part, BEAGLE_QUERY_PART (filetype));
	}

	if (!or_part)
		return FALSE;

	beagle_query_add_part (priv->query, BEAGLE_QUERY_PART (or_part));
	return TRUE;
}

static gboolean
rejilla_search_beagle_set_query_mime (RejillaSearchEngine *search,
                                      const gchar **mimes)
{
	int i;
	BeagleQueryPartOr *or_part;
	RejillaSearchBeaglePrivate *priv;

	priv = REJILLA_SEARCH_BEAGLE_PRIVATE (search);

	if (!priv->query)
		return FALSE;

	or_part = beagle_query_part_or_new ();
	for (i = 0; mimes [i]; i ++) {
		BeagleQueryPartProperty *filetype;

		filetype = beagle_query_part_property_new ();
		beagle_query_part_property_set_property_type (filetype, BEAGLE_PROPERTY_TYPE_KEYWORD);
		beagle_query_part_property_set_key (filetype, "beagle:MimeType");
		beagle_query_part_property_set_value (filetype, mimes [i]);
		beagle_query_part_or_add_subpart (or_part, BEAGLE_QUERY_PART (filetype));
	}

	beagle_query_add_part (priv->query, BEAGLE_QUERY_PART (or_part));

	return TRUE;
}

static void
rejilla_search_beagle_init_engine (RejillaSearchEngineIface *iface)
{
	iface->is_available = rejilla_search_beagle_is_available;
	iface->uri_from_hit = rejilla_search_beagle_uri_from_hit;
	iface->score_from_hit = rejilla_search_beagle_score_from_hit;
	iface->mime_from_hit = rejilla_search_beagle_mime_from_hit;
	iface->query_new = rejilla_search_beagle_query_new;
	iface->query_set_scope = rejilla_search_beagle_query_set_scope;
	iface->query_set_mime = rejilla_search_beagle_set_query_mime;
	iface->query_start = rejilla_search_beagle_query_start;
	iface->add_hits = rejilla_search_beagle_add_hit_to_tree;
	iface->num_hits = rejilla_search_beagle_num_hits;
}

static void
rejilla_search_beagle_init (RejillaSearchBeagle *object)
{
	RejillaSearchBeaglePrivate *priv;

	priv = REJILLA_SEARCH_BEAGLE_PRIVATE (object);

	priv->client = beagle_client_new (NULL);
}

static void
rejilla_search_beagle_finalize (GObject *object)
{
	RejillaSearchBeaglePrivate *priv;

	priv = REJILLA_SEARCH_BEAGLE_PRIVATE (object);

	if (priv->client) {
		g_object_unref (priv->client);
		priv->client = NULL;
	}

	rejilla_search_beagle_clean (REJILLA_SEARCH_BEAGLE (object));
	G_OBJECT_CLASS (rejilla_search_beagle_parent_class)->finalize (object);
}

static void
rejilla_search_beagle_class_init (RejillaSearchBeagleClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (RejillaSearchBeaglePrivate));

	object_class->finalize = rejilla_search_beagle_finalize;
}
