/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/***************************************************************************
*            search.c
*
*  dim mai 22 11:20:54 2005
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

#include <string.h>

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <glib-object.h>

#include <gio/gio.h>

#include <gtk/gtk.h>

#include "rejilla-search.h"
#include "rejilla-search-engine.h"

#include "rejilla-misc.h"

#include "rejilla-app.h"
#include "rejilla-utils.h"
#include "rejilla-search-entry.h"
#include "rejilla-mime-filter.h"
#include "rejilla-search-engine.h"
#include "eggtreemultidnd.h"

#include "rejilla-uri-container.h"
#include "rejilla-layout-object.h"


struct RejillaSearchPrivate {
	GtkTreeViewColumn *sort_column;

	GtkWidget *tree;
	GtkWidget *entry;
	GtkWidget *filter;
	GtkWidget *filters;
	GtkWidget *right;
	GtkWidget *left;
	GtkWidget *results_label;

	RejillaSearchEngine *engine;
	gint first_hit;

	gint max_results;

	int id;
};

enum {
	TARGET_URIS_LIST,
};

static GtkTargetEntry ntables_find[] = {
	{"text/uri-list", 0, TARGET_URIS_LIST}
};
static guint nb_ntables_find = sizeof (ntables_find) / sizeof (ntables_find[0]);

#define REJILLA_SEARCH_SPACING 6

static void rejilla_search_iface_uri_container_init (RejillaURIContainerIFace *iface);
static void rejilla_search_iface_layout_object_init (RejillaLayoutObjectIFace *iface);

G_DEFINE_TYPE_WITH_CODE (RejillaSearch,
			 rejilla_search,
			 GTK_TYPE_VBOX,
			 G_IMPLEMENT_INTERFACE (REJILLA_TYPE_URI_CONTAINER,
					        rejilla_search_iface_uri_container_init)
			 G_IMPLEMENT_INTERFACE (REJILLA_TYPE_LAYOUT_OBJECT,
					        rejilla_search_iface_layout_object_init));



static void
rejilla_search_column_icon_cb (GtkTreeViewColumn *tree_column,
                               GtkCellRenderer *cell,
                               GtkTreeModel *model,
                               GtkTreeIter *iter,
                               gpointer data)
{
	GIcon *icon;
	const gchar *mime;
	gpointer hit = NULL;

	gtk_tree_model_get (model, iter,
	                    REJILLA_SEARCH_TREE_HIT_COL, &hit,
	                    -1);

	mime = rejilla_search_engine_mime_from_hit (REJILLA_SEARCH (data)->priv->engine, hit);
	if (!mime)
		return;
	
	if (!strcmp (mime, "inode/directory"))
		mime = "x-directory/normal";

	icon = g_content_type_get_icon (mime);
	g_object_set (G_OBJECT (cell),
		      "gicon", icon,
		      NULL);
	g_object_unref (icon);
}

static gchar *
rejilla_search_name_from_hit (RejillaSearch *search,
			      gpointer hit)
{
	gchar *name;
	const gchar *uri;
	gchar *unescaped_uri;

	uri = rejilla_search_engine_uri_from_hit (search->priv->engine, hit);

	/* beagle can return badly formed uri not
	 * encoded in UTF-8 locale charset so we
	 * check them just in case */
	unescaped_uri = g_uri_unescape_string (uri, NULL);
	if (!g_utf8_validate (unescaped_uri, -1, NULL)) {
		g_free (unescaped_uri);
		return NULL;
	}

	name = g_path_get_basename (unescaped_uri);
	g_free (unescaped_uri);
	return name;
}

static void
rejilla_search_column_name_cb (GtkTreeViewColumn *tree_column,
                               GtkCellRenderer *cell,
                               GtkTreeModel *model,
                               GtkTreeIter *iter,
                               gpointer data)
{
	gchar *name;
	gpointer hit = NULL;

	gtk_tree_model_get (model, iter,
	                    REJILLA_SEARCH_TREE_HIT_COL, &hit,
	                    -1);

	name = rejilla_search_name_from_hit (data, hit);
	g_object_set (G_OBJECT (cell),
		      "text", name,
		      NULL);
	g_free (name);
}

static gchar*
rejilla_search_description_from_hit (RejillaSearch *search,
				     gpointer hit)
{
	const gchar *mime;

	mime = rejilla_search_engine_mime_from_hit (search->priv->engine, hit);
	if (!mime)
		return NULL;

	return g_content_type_get_description (mime);
}

static void
rejilla_search_column_description_cb (GtkTreeViewColumn *tree_column,
                                      GtkCellRenderer *cell,
                                      GtkTreeModel *model,
                                      GtkTreeIter *iter,
                                      gpointer data)
{
	gchar *description;
	gpointer hit = NULL;

	gtk_tree_model_get (model, iter,
	                    REJILLA_SEARCH_TREE_HIT_COL, &hit,
	                    -1);

	description = rejilla_search_description_from_hit (data, hit);
	g_object_set (G_OBJECT (cell),
		      "text", description,
		      NULL);
	g_free (description);
}

static void
rejilla_search_increase_activity (RejillaSearch *search)
{
	GdkCursor *cursor;

	cursor = gdk_cursor_new (GDK_WATCH);
	gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (search)), cursor);
	gdk_cursor_unref (cursor);
}

static void
rejilla_search_decrease_activity (RejillaSearch *search)
{
	gdk_window_set_cursor (gtk_widget_get_window (GTK_WIDGET (search)), NULL);
}

static void
rejilla_search_update_header (RejillaSearch *search)
{
	gchar *string;
	gint num_hits;

	num_hits = rejilla_search_engine_num_hits (search->priv->engine);
	if (num_hits) {
		gint last;
		gchar *tmp;

		last = search->priv->first_hit + search->priv->max_results;
		last = MIN (last, num_hits);

		tmp = g_strdup_printf (_("Results %i–%i (out of %i)"),
				       search->priv->first_hit + 1,
				       last,
				       num_hits);
		string = g_strdup_printf ("<b>%s</b>", tmp);
		g_free (tmp);
	}
	else
		string = g_strdup_printf ("<b>%s</b>", _("No results"));

	gtk_label_set_markup (GTK_LABEL (search->priv->results_label), string);
	g_free (string);

	if (search->priv->first_hit + search->priv->max_results < num_hits)
		gtk_widget_set_sensitive (search->priv->right, TRUE);
	else
		gtk_widget_set_sensitive (search->priv->right, FALSE);

	if (search->priv->first_hit > 0)
		gtk_widget_set_sensitive (search->priv->left, TRUE);
	else
		gtk_widget_set_sensitive (search->priv->left, FALSE);
}

static void
rejilla_search_empty_tree (RejillaSearch *search)
{
	GtkTreeModel *model;
	GtkTreeModel *sort;
	GtkTreeIter row;

	sort = gtk_tree_view_get_model (GTK_TREE_VIEW (search->priv->tree));
	model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (sort));

	if (gtk_tree_model_get_iter_first (model, &row)) {
		do {
			gpointer hit;
			const gchar *mime;

			hit = NULL;
			gtk_tree_model_get (model, &row,
					    REJILLA_SEARCH_TREE_HIT_COL, &hit,
					    -1);

			if (!hit)
				continue;

			mime = rejilla_search_engine_mime_from_hit (search->priv->engine, hit);
			if (!mime)
				continue;

			rejilla_mime_filter_unref_mime (REJILLA_MIME_FILTER (search->priv->filter), mime);
		} while (gtk_list_store_remove (GTK_LIST_STORE (model), &row));
	}
}

static void
rejilla_search_row_inserted (GtkTreeModel *model,
                             GtkTreePath *path,
                             GtkTreeIter *iter,
                             RejillaSearch *search)
{
	const gchar *mime;
	gpointer hit = NULL;

	gtk_tree_model_get (model, iter,
	                    REJILLA_SEARCH_TREE_HIT_COL, &hit,
	                    -1);

	if (!hit)
		return;

	mime = rejilla_search_engine_mime_from_hit (search->priv->engine, hit);

	if (mime) {
		/* add the mime type to the filter combo */
		rejilla_mime_filter_add_mime (REJILLA_MIME_FILTER (search->priv->filter), mime);
	}
}

static gboolean
rejilla_search_update_tree (RejillaSearch *search)
{
	GtkTreeModel *model;
	GtkTreeModel *sort;
	gint max_hits;
	gint last_hit;

	if (search->priv->first_hit < 0)
		search->priv->first_hit = 0;

	max_hits = rejilla_search_engine_num_hits (search->priv->engine);
	if (search->priv->first_hit > max_hits) {
		search->priv->first_hit = max_hits;
		return FALSE;
	}

	last_hit = MIN (max_hits, search->priv->max_results + search->priv->first_hit);

	rejilla_search_empty_tree (search);

	sort = gtk_tree_view_get_model (GTK_TREE_VIEW (search->priv->tree));
	model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (sort));

	rejilla_search_engine_add_hits (search->priv->engine,
	                                model,
	                                search->priv->first_hit,
	                                last_hit);

	rejilla_search_update_header (search);
	return TRUE;
}

static void
rejilla_search_left_button_clicked_cb (GtkButton *button,
				       RejillaSearch *search)
{
	search->priv->first_hit -= search->priv->max_results;
	rejilla_search_update_tree (search);
}

static void
rejilla_search_right_button_clicked_cb (GtkButton *button,
					RejillaSearch *search)
{
	search->priv->first_hit += search->priv->max_results;
	rejilla_search_update_tree (search);
}

static void
rejilla_search_max_results_num_changed_cb (GtkComboBox *combo,
					   RejillaSearch *search)
{
	gint index;
	gint page_num;

	if (search->priv->max_results)
		page_num = search->priv->first_hit / search->priv->max_results;
	else
		page_num = 0;

	index = gtk_combo_box_get_active (combo);
	switch (index) {
	case 0:
		search->priv->max_results = 20;
		break;
	case 1:
		search->priv->max_results = 50;
		break;
	case 2:
		search->priv->max_results = 100;
		break;
	}

	search->priv->first_hit = page_num * search->priv->max_results;
	rejilla_search_update_tree (search);
}

static void
rejilla_search_finished_cb (RejillaSearchEngine *engine,
                            RejillaSearch *search)
{
	rejilla_search_decrease_activity (search);
}

static void
rejilla_search_error_cb (RejillaSearchEngine *engine,
                         GError *error,
                         RejillaSearch *search)
{
	rejilla_search_update_header (search);
	if (error)
		rejilla_app_alert (rejilla_app_get_default (),
				   _("Error querying for keywords."),
				   error->message,
				   GTK_MESSAGE_ERROR);

	rejilla_search_decrease_activity (search);
}

static void
rejilla_search_hit_added_cb (RejillaSearchEngine *engine,
                             gpointer hit,
                             RejillaSearch *search)
{
	gint num;
	gint hit_num;
	GtkTreeIter iter;
	const gchar *mime;
	GtkTreeModel *model;

	hit_num = rejilla_search_engine_num_hits (search->priv->engine);
	if (hit_num < search->priv->first_hit
	&& hit_num >= search->priv->first_hit + search->priv->max_results) {
		rejilla_search_update_header (search);
		return;
	}

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (search->priv->tree));
	model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (model));

	num = gtk_tree_model_iter_n_children (model, NULL);
	if (num >= search->priv->max_results) {
		rejilla_search_update_header (search);
		return;
	}

	gtk_list_store_insert_with_values (GTK_LIST_STORE (model), &iter, -1,
	                                   REJILLA_SEARCH_TREE_HIT_COL, hit,
	                                   -1);

	mime = rejilla_search_engine_mime_from_hit (search->priv->engine, hit);
	rejilla_search_update_header (search);
}

static void
rejilla_search_hit_removed_cb (RejillaSearchEngine *engine,
                               gpointer hit,
                               RejillaSearch *search)
{
	int num = 0;
	int range_end;
	int range_start;
	GtkTreeIter iter;
	GtkTreeModel *model;

	model = gtk_tree_view_get_model (GTK_TREE_VIEW (search->priv->tree));
	model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (model));

	if (!gtk_tree_model_get_iter_first (model, &iter))
		return;

	do {
		gpointer model_hit;

		model_hit = NULL;
		gtk_tree_model_get (model, &iter,
				    REJILLA_SEARCH_TREE_HIT_COL, &model_hit,
				    -1);

		if (hit == model_hit) {
			const gchar *mime;

			mime = rejilla_search_engine_mime_from_hit (search->priv->engine, hit);
			rejilla_mime_filter_unref_mime (REJILLA_MIME_FILTER (search->priv->filter), mime);

			gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
			break;
		}

		num ++;
	} while (gtk_tree_model_iter_next (model, &iter));

	if (num < search->priv->first_hit
	&& num >= search->priv->first_hit + search->priv->max_results) {
		rejilla_search_update_header (search);
		return;
	}

	range_start = search->priv->first_hit + search->priv->max_results - 1;
	range_end = search->priv->first_hit + search->priv->max_results;
	rejilla_search_engine_add_hits (search->priv->engine,
	                                model,
	                                range_start,
	                                range_end);

	rejilla_search_update_header (search);
}

static void
rejilla_search_entry_activated_cb (RejillaSearchEntry *entry,
				   RejillaSearch *search)
{
	rejilla_search_increase_activity (search);

	/* we first empty everything including the filter box */
	rejilla_search_empty_tree (search);
	rejilla_search_entry_set_query (entry, search->priv->engine);
	rejilla_search_engine_start_query (search->priv->engine);
	rejilla_search_update_header (search);
}

static gboolean
rejilla_search_is_visible_cb (GtkTreeModel *model,
			      GtkTreeIter *iter,
			      RejillaSearch *search)
{
	const gchar *uri, *mime;
	gpointer hit = NULL;
	gboolean result;
	gchar *name;

	gtk_tree_model_get (model, iter,
	                    REJILLA_SEARCH_TREE_HIT_COL, &hit,
	                    -1);

	name = rejilla_search_name_from_hit (search, hit);
	uri = rejilla_search_engine_uri_from_hit (search->priv->engine, hit);
	mime = rejilla_search_engine_mime_from_hit (search->priv->engine, hit);
	result = rejilla_mime_filter_filter (REJILLA_MIME_FILTER (search->priv->filter),
					     name,
					     uri,
					     name,
					     mime);

	g_free (name);
	return result;
}

static void
rejilla_search_mime_filter_changed (GtkComboBox *combo,
				    RejillaSearch *search)
{
	gtk_tree_model_filter_refilter (GTK_TREE_MODEL_FILTER (gtk_tree_view_get_model (GTK_TREE_VIEW (search->priv->tree))));
}

static void
rejilla_search_tree_activated_cb (GtkTreeView *tree,
				  GtkTreeIter *row,
				  GtkTreeViewColumn *column,
				  RejillaSearch *search)
{
	rejilla_uri_container_uri_activated (REJILLA_URI_CONTAINER (search));
}

static char **
rejilla_search_get_selected_rows (RejillaSearch *search)
{
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	gchar **uris = NULL;
	GList *rows, *iter;
	GtkTreeIter row;
	int i;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (search->priv->tree));
	rows = gtk_tree_selection_get_selected_rows (selection, &model);
	if (rows == NULL)
		return NULL;

	uris = g_new0 (char *, g_list_length (rows) + 1);
	for (iter = rows, i = 0; iter != NULL; iter = iter->next, i++) {
		gpointer hit;

		gtk_tree_model_get_iter (model,
					 &row,
					 (GtkTreePath *) iter->data);
		gtk_tree_path_free (iter->data);

		hit = NULL;
		gtk_tree_model_get (model, &row,
				    REJILLA_SEARCH_TREE_HIT_COL, &hit,
				    -1);
		uris[i] = g_strdup (rejilla_search_engine_uri_from_hit (search->priv->engine, hit));
	}

	g_list_free (rows);
	return uris;
}

static void
rejilla_search_drag_data_get_cb (GtkTreeView *tree,
				 GdkDragContext *drag_context,
				 GtkSelectionData *selection_data,
				 guint info,
				 guint time, RejillaSearch *search)
{
	gchar **uris;

	uris = rejilla_search_get_selected_rows (search);
	gtk_selection_data_set_uris (selection_data, uris);
	g_strfreev (uris);
}

static gchar *
rejilla_search_get_selected_uri (RejillaURIContainer *container)
{
	RejillaSearch *search;
	gchar **uris = NULL;
	gchar *uri;

	search = REJILLA_SEARCH (container);
	uris = rejilla_search_get_selected_rows (search);

	if (uris) {
		uri = g_strdup (uris [0]);
		g_strfreev (uris);
		return uri;
	}

	return NULL;
}

static gchar **
rejilla_search_get_selected_uris (RejillaURIContainer *container)
{
	RejillaSearch *search;

	search = REJILLA_SEARCH (container);
	return rejilla_search_get_selected_rows (search);
}

static void
rejilla_search_tree_selection_changed_cb (GtkTreeSelection *selection,
					  RejillaSearch *search)
{
	rejilla_uri_container_uri_selected (REJILLA_URI_CONTAINER (search));
}

static void
rejilla_search_get_proportion (RejillaLayoutObject *object,
			       gint *header,
			       gint *center,
			       gint *footer) 
{
	GtkRequisition requisition;

	gtk_widget_size_request (REJILLA_SEARCH (object)->priv->filters,
				 &requisition);
	*footer = requisition.height + REJILLA_SEARCH_SPACING;
}

static void
rejilla_search_set_context (RejillaLayoutObject *object,
			    RejillaLayoutType type)
{
	RejillaSearch *self;

	self = REJILLA_SEARCH (object);
	rejilla_search_entry_set_context (REJILLA_SEARCH_ENTRY (self->priv->entry), type);
}

static gint
rejilla_search_sort_name (GtkTreeModel *model,
                          GtkTreeIter  *iter1,
                          GtkTreeIter  *iter2,
                          gpointer user_data)
{
	gint res;
	gpointer hit1, hit2;
	gchar *name1, *name2;
	RejillaSearch *search = REJILLA_SEARCH (user_data);

	gtk_tree_model_get (model, iter1,
	                    REJILLA_SEARCH_TREE_HIT_COL, &hit1,
	                    -1);
	gtk_tree_model_get (model, iter2,
	                    REJILLA_SEARCH_TREE_HIT_COL, &hit2,
	                    -1);

	name1 = rejilla_search_name_from_hit (search, hit1);
	name2 = rejilla_search_name_from_hit (search, hit2);

	res = g_strcmp0 (name1, name2);
	g_free (name1);
	g_free (name2);

	return res;
}

static gint
rejilla_search_sort_description (GtkTreeModel *model,
                                 GtkTreeIter  *iter1,
                                 GtkTreeIter  *iter2,
                                 gpointer user_data)
{
	gpointer hit1, hit2;
	RejillaSearch *search = REJILLA_SEARCH (user_data);

	gtk_tree_model_get (model, iter1,
	                    REJILLA_SEARCH_TREE_HIT_COL, &hit1,
	                    -1);
	gtk_tree_model_get (model, iter2,
	                    REJILLA_SEARCH_TREE_HIT_COL, &hit2,
	                    -1);

	return g_strcmp0 (rejilla_search_description_from_hit (search, hit1),
	                  rejilla_search_description_from_hit (search, hit2));
}

static gint
rejilla_search_sort_score (GtkTreeModel *model,
                           GtkTreeIter  *iter1,
                           GtkTreeIter  *iter2,
                           gpointer user_data)
{
	gpointer hit1, hit2;
	RejillaSearch *search = REJILLA_SEARCH (user_data);

	gtk_tree_model_get (model, iter1,
	                    REJILLA_SEARCH_TREE_HIT_COL, &hit1,
	                    -1);
	gtk_tree_model_get (model, iter2,
	                    REJILLA_SEARCH_TREE_HIT_COL, &hit2,
	                    -1);

	return rejilla_search_engine_score_from_hit (search->priv->engine, hit2) -
	       rejilla_search_engine_score_from_hit (search->priv->engine, hit1);
}

static void
rejilla_search_column_clicked (GtkTreeViewColumn *column,
			       RejillaSearch *search)
{
	GtkTreeModel *sort;
	GtkTreeModel *model;
	GtkSortType model_order;

	sort = gtk_tree_view_get_model (GTK_TREE_VIEW (search->priv->tree));
	model = gtk_tree_model_filter_get_model (GTK_TREE_MODEL_FILTER (sort));

	gtk_tree_sortable_get_sort_column_id (GTK_TREE_SORTABLE (model),
					      NULL,
					      &model_order);

	if (!gtk_tree_view_column_get_sort_indicator (column)) {
		GtkTreeIterCompareFunc sort_func;

		if (search->priv->sort_column)
			gtk_tree_view_column_set_sort_indicator (search->priv->sort_column, FALSE);

		search->priv->sort_column = column;
		gtk_tree_view_column_set_sort_indicator (column, TRUE);

		gtk_tree_view_column_set_sort_order (column, GTK_SORT_ASCENDING);
		gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (model),
		                                      GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
		                                      GTK_SORT_ASCENDING);

		sort_func = g_object_get_data (G_OBJECT (column), "SortFunc");
		gtk_tree_sortable_set_default_sort_func (GTK_TREE_SORTABLE (model),
							 sort_func,
							 search,
							 NULL);
	}
	else if (model_order == GTK_SORT_DESCENDING) {
		gtk_tree_view_column_set_sort_indicator (column, FALSE);
		gtk_tree_sortable_set_default_sort_func (GTK_TREE_SORTABLE (model),
							 rejilla_search_sort_score,
							 search,
							 NULL);
		gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (model),
		                                      GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
		                                      GTK_SORT_ASCENDING);
	}
	else {
		gtk_tree_view_column_set_sort_order (column, GTK_SORT_DESCENDING);
		gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (model),
		                                      GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
		                                      GTK_SORT_DESCENDING);
	}

	g_signal_stop_emission_by_name (column, "clicked");
}

static gboolean
rejilla_search_try_again (RejillaSearch *search)
{
	if (rejilla_search_engine_is_available (search->priv->engine)) {
		gtk_widget_set_sensitive (GTK_WIDGET (search), TRUE);
		search->priv->id = 0;
		return FALSE;
	}

	return TRUE;
}

static void
rejilla_search_init (RejillaSearch *obj)
{
	gchar *string;
	GtkWidget *box;
	GtkWidget *box1;
	GtkWidget *label;
	GtkWidget *combo;
	GtkWidget *button;
	GtkWidget *scroll;
	GtkListStore *store;
	GtkTreeModel *model;
	GtkWidget *separator;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkFileFilter *file_filter;
	GtkTreeSelection *selection;

	gtk_box_set_spacing (GTK_BOX (obj), REJILLA_SEARCH_SPACING);
	obj->priv = g_new0 (RejillaSearchPrivate, 1);

	obj->priv->engine = rejilla_search_engine_new_default ();
	g_signal_connect (obj->priv->engine,
	                  "search-finished",
	                  G_CALLBACK (rejilla_search_finished_cb),
	                  obj);
	g_signal_connect (obj->priv->engine,
	                  "search-error",
	                  G_CALLBACK (rejilla_search_error_cb),
	                  obj);
	g_signal_connect (obj->priv->engine,
	                  "hit-removed",
	                  G_CALLBACK (rejilla_search_hit_removed_cb),
	                  obj);
	g_signal_connect (obj->priv->engine,
	                  "hit-added",
	                  G_CALLBACK (rejilla_search_hit_added_cb),
	                  obj);

	/* separator */
	separator = gtk_hseparator_new ();
	gtk_box_pack_start (GTK_BOX (obj), separator, FALSE, FALSE, 0);

	/* Entry */
	obj->priv->entry = rejilla_search_entry_new ();
	g_signal_connect (G_OBJECT (obj->priv->entry), 
			  "activated",
			  G_CALLBACK (rejilla_search_entry_activated_cb),
			  obj);
	gtk_box_pack_start (GTK_BOX (obj), obj->priv->entry, FALSE, FALSE, 0);

	/* separator */
	separator = gtk_hseparator_new ();
	gtk_box_pack_start (GTK_BOX (obj), separator, FALSE, FALSE, 0);

	/* results navigation */
	box = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (obj), box, FALSE, FALSE, 0);

	button = rejilla_utils_make_button (_("Previous Results"),
					    GTK_STOCK_GO_BACK,
					    NULL,
					    GTK_ICON_SIZE_BUTTON);
	gtk_button_set_alignment (GTK_BUTTON (button), 0.0, 0.5);
	gtk_widget_set_sensitive (button, FALSE);
	g_signal_connect (G_OBJECT (button), 
			  "clicked",
			  G_CALLBACK (rejilla_search_left_button_clicked_cb),
			  obj);
	gtk_box_pack_start (GTK_BOX (box), button, FALSE, TRUE, 0);
	obj->priv->left = button;

	string = g_strdup_printf ("<b>%s</b>", _("No results"));
	label = gtk_label_new (string);
	g_free (string);

	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_box_pack_start (GTK_BOX (box), label, TRUE, TRUE, 0);
	obj->priv->results_label = label;

	button = rejilla_utils_make_button (_("Next Results"),
					    GTK_STOCK_GO_FORWARD,
					    NULL,
					    GTK_ICON_SIZE_BUTTON);
	gtk_button_set_alignment (GTK_BUTTON (button), 1.0, 0.5);
	gtk_widget_set_sensitive (button, FALSE);
	g_signal_connect (G_OBJECT (button), 
			  "clicked",
			  G_CALLBACK (rejilla_search_right_button_clicked_cb),
			  obj);
	gtk_box_pack_start (GTK_BOX (box), button, FALSE, TRUE, 0);
	obj->priv->right = button;

	/* Tree */
	store = gtk_list_store_new (REJILLA_SEARCH_TREE_NB_COL,
				    G_TYPE_POINTER);

	gtk_tree_sortable_set_default_sort_func (GTK_TREE_SORTABLE (store),
	                                         rejilla_search_sort_score,
        	                                 obj,
                	                         NULL);

	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store),
					      GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
					      GTK_SORT_ASCENDING);

	g_signal_connect (store,
	                  "row-inserted",
	                  G_CALLBACK (rejilla_search_row_inserted),
	                  obj);

	model = gtk_tree_model_filter_new (GTK_TREE_MODEL (store), NULL);
	g_object_unref (G_OBJECT (store));

	gtk_tree_model_filter_set_visible_func (GTK_TREE_MODEL_FILTER (model),
						(GtkTreeModelFilterVisibleFunc) rejilla_search_is_visible_cb,
						obj,
						NULL);

	obj->priv->tree = gtk_tree_view_new_with_model (model);
	g_object_unref (G_OBJECT (model));

	gtk_tree_view_set_rubber_banding (GTK_TREE_VIEW (obj->priv->tree), TRUE);
	egg_tree_multi_drag_add_drag_support (GTK_TREE_VIEW (obj->priv->tree));

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (obj->priv->tree));
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);

	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (obj->priv->tree), TRUE);
	g_signal_connect (G_OBJECT (obj->priv->tree), 
			  "row-activated",
			  G_CALLBACK (rejilla_search_tree_activated_cb),
			  obj);
	g_signal_connect (G_OBJECT (gtk_tree_view_get_selection (GTK_TREE_VIEW (obj->priv->tree))),
			  "changed",
			  G_CALLBACK (rejilla_search_tree_selection_changed_cb),
			  obj);

	gtk_tree_view_set_headers_clickable (GTK_TREE_VIEW (obj->priv->tree), TRUE);

	column = gtk_tree_view_column_new ();

	gtk_tree_view_column_set_clickable (column, TRUE);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_title (column, _("Files"));
	gtk_tree_view_column_set_min_width (column, 128);

	g_object_set_data (G_OBJECT (column), "SortFunc", rejilla_search_sort_name);
	g_signal_connect (column,
			  "clicked",
			  G_CALLBACK (rejilla_search_column_clicked),
			  obj);

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func (column,
	                                         renderer,
	                                         rejilla_search_column_icon_cb,
	                                         obj,
	                                         NULL);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func (column,
	                                         renderer,
	                                         rejilla_search_column_name_cb,
	                                         obj,
	                                         NULL);

	gtk_tree_view_append_column (GTK_TREE_VIEW (obj->priv->tree),
				     column);

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Description"));
	gtk_tree_view_column_set_clickable (column, TRUE);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_min_width (column, 128);

	g_object_set_data (G_OBJECT (column), "SortFunc", rejilla_search_sort_description);
	g_signal_connect (column,
			  "clicked",
			  G_CALLBACK (rejilla_search_column_clicked),
			  obj);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func (column,
	                                         renderer,
	                                         rejilla_search_column_description_cb,
	                                         obj,
	                                         NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (obj->priv->tree),
				     column);
	/* dnd */
	gtk_tree_view_enable_model_drag_source (GTK_TREE_VIEW (obj->priv->tree),
						GDK_BUTTON1_MASK,
						ntables_find,
						nb_ntables_find,
						GDK_ACTION_COPY);

	g_signal_connect (G_OBJECT (obj->priv->tree), 
			  "drag-data-get",
			  G_CALLBACK (rejilla_search_drag_data_get_cb),
			  obj);

	scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_container_add (GTK_CONTAINER (scroll), obj->priv->tree);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll),
					     GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start (GTK_BOX (obj), scroll, TRUE, TRUE, 0);

	/* filter combo */
	box = gtk_hbox_new (FALSE, 32);
	obj->priv->filters = box;
	gtk_box_pack_end (GTK_BOX (obj), box, FALSE, FALSE, 0);

	obj->priv->filter = rejilla_mime_filter_new ();
	g_signal_connect (G_OBJECT (REJILLA_MIME_FILTER (obj->priv->filter)->combo),
			  "changed",
			  G_CALLBACK (rejilla_search_mime_filter_changed),
			  obj);

	file_filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (file_filter, _("All files"));
	gtk_file_filter_add_pattern (file_filter, "*");
	rejilla_mime_filter_add_filter (REJILLA_MIME_FILTER (obj->priv->filter),
					file_filter);
	g_object_unref (file_filter);

	gtk_box_pack_end (GTK_BOX (box), obj->priv->filter, FALSE, FALSE, 0);

	box1 = gtk_hbox_new (FALSE, 6);
	gtk_box_pack_end (GTK_BOX (box), box1, FALSE, FALSE, 0);

	label = gtk_label_new (_("Number of results displayed"));
	gtk_box_pack_start (GTK_BOX (box1), label, FALSE, FALSE, 0);

	combo = gtk_combo_box_new_text ();
	g_signal_connect (combo,
			  "changed",
			  G_CALLBACK (rejilla_search_max_results_num_changed_cb),
			  obj);
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo), "20");
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo), "50");
	gtk_combo_box_append_text (GTK_COMBO_BOX (combo), "100");

	gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 1);
	obj->priv->max_results = 50;

	gtk_box_pack_start (GTK_BOX (box1), combo, FALSE, FALSE, 0);

	if (!rejilla_search_engine_is_available (obj->priv->engine)) {
		gtk_widget_set_sensitive (GTK_WIDGET (obj), FALSE);

		/* we will retry in 10 seconds */
		obj->priv->id = g_timeout_add_seconds (10,
		                                       (GSourceFunc) rejilla_search_try_again,
		                                       obj);
	} 
}

static void
rejilla_search_destroy (GtkObject *object)
{
	RejillaSearch *search;

	search = REJILLA_SEARCH (object);
	if (search->priv->tree) {
		g_signal_handlers_disconnect_by_func (gtk_tree_view_get_selection (GTK_TREE_VIEW (search->priv->tree)),
		                                      rejilla_search_tree_selection_changed_cb,
		                                      search);

		g_signal_handlers_disconnect_by_func (search->priv->filter,
						      rejilla_search_mime_filter_changed,
						      search);

		rejilla_search_empty_tree (search);
		search->priv->filter = NULL;
		search->priv->tree = NULL;
	}

	if (search->priv->id) {
		g_source_remove (search->priv->id);
		search->priv->id = 0;
	}

	if (search->priv->engine) {
		g_object_unref (search->priv->engine);
		search->priv->engine = NULL;
	}

	if (GTK_OBJECT_CLASS (rejilla_search_parent_class)->destroy)
		GTK_OBJECT_CLASS (rejilla_search_parent_class)->destroy (object);
}

static void
rejilla_search_finalize (GObject *object)
{
	RejillaSearch *cobj;

	cobj = REJILLA_SEARCH (object);

	g_free (cobj->priv);
	G_OBJECT_CLASS (rejilla_search_parent_class)->finalize (object);
}

static void
rejilla_search_iface_layout_object_init (RejillaLayoutObjectIFace *iface)
{
	iface->get_proportion = rejilla_search_get_proportion;
	iface->set_context = rejilla_search_set_context;
}

static void
rejilla_search_iface_uri_container_init (RejillaURIContainerIFace *iface)
{
	iface->get_selected_uri = rejilla_search_get_selected_uri;
	iface->get_selected_uris = rejilla_search_get_selected_uris;
}

static void
rejilla_search_class_init (RejillaSearchClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkObjectClass *gtkobject_class = GTK_OBJECT_CLASS (klass);

	object_class->finalize = rejilla_search_finalize;
	gtkobject_class->destroy = rejilla_search_destroy;
}

GtkWidget *
rejilla_search_new ()
{
	return g_object_new (REJILLA_TYPE_SEARCH, NULL);
}
