/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * rejilla
 * Copyright (C) Philippe Rouquier 2007 <bonfire-app@wanadoo.fr>
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
#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>

#include "rejilla-units.h"

#include "rejilla-session-cfg.h"
#include "rejilla-tags.h"
#include "rejilla-track-stream-cfg.h"

#include "rejilla-utils.h"
#include "rejilla-video-tree-model.h"

#include "eggtreemultidnd.h"

typedef struct _RejillaVideoTreeModelPrivate RejillaVideoTreeModelPrivate;
struct _RejillaVideoTreeModelPrivate
{
	RejillaSessionCfg *session;

	GSList *gaps;

	guint stamp;
	GtkIconTheme *theme;
};

#define REJILLA_VIDEO_TREE_MODEL_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REJILLA_TYPE_VIDEO_TREE_MODEL, RejillaVideoTreeModelPrivate))

static void
rejilla_video_tree_model_multi_drag_source_iface_init (gpointer g_iface, gpointer data);
static void
rejilla_video_tree_model_drag_source_iface_init (gpointer g_iface, gpointer data);
static void
rejilla_video_tree_model_drag_dest_iface_init (gpointer g_iface, gpointer data);
static void
rejilla_video_tree_model_iface_init (gpointer g_iface, gpointer data);

G_DEFINE_TYPE_WITH_CODE (RejillaVideoTreeModel,
			 rejilla_video_tree_model,
			 G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_MODEL,
					        rejilla_video_tree_model_iface_init)
			 G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_DRAG_DEST,
					        rejilla_video_tree_model_drag_dest_iface_init)
			 G_IMPLEMENT_INTERFACE (GTK_TYPE_TREE_DRAG_SOURCE,
					        rejilla_video_tree_model_drag_source_iface_init)
			 G_IMPLEMENT_INTERFACE (EGG_TYPE_TREE_MULTI_DRAG_SOURCE,
					        rejilla_video_tree_model_multi_drag_source_iface_init));

enum {
	REJILLA_STREAM_ROW_NORMAL	= 0,
	REJILLA_STREAM_ROW_GAP		= 1,
};

/**
 * This is mainly a list so the following functions are not implemented.
 * But we may need them for AUDIO models when we display GAPs
 */
static gboolean
rejilla_video_tree_model_iter_parent (GtkTreeModel *model,
				      GtkTreeIter *iter,
				      GtkTreeIter *child)
{
	return FALSE;
}

static gboolean
rejilla_video_tree_model_iter_nth_child (GtkTreeModel *model,
					 GtkTreeIter *iter,
					 GtkTreeIter *parent,
					 gint n)
{
	return FALSE;
}

static gint
rejilla_video_tree_model_iter_n_children (GtkTreeModel *model,
					  GtkTreeIter *iter)
{
	if (!iter) {
		guint num = 0;
		GSList *iter;
		GSList * tracks;
		RejillaVideoTreeModelPrivate *priv;

		priv = REJILLA_VIDEO_TREE_MODEL_PRIVATE (model);

		/* This is a special case in which we return the number
		 * of rows that are in the model. */
		tracks = rejilla_burn_session_get_tracks (REJILLA_BURN_SESSION (priv->session));
		for (iter = tracks; iter; iter = iter->next) {
			RejillaTrackStream *track;

			track = iter->data;
			num ++;

			if (rejilla_track_stream_get_gap (track) > 0)
				num ++;
		}

		return num;
	}

	return 0;
}

static gboolean
rejilla_video_tree_model_iter_has_child (GtkTreeModel *model,
					 GtkTreeIter *iter)
{
	return FALSE;
}

static gboolean
rejilla_video_tree_model_iter_children (GtkTreeModel *model,
				        GtkTreeIter *iter,
				        GtkTreeIter *parent)
{
	return FALSE;
}

static void
rejilla_video_tree_model_get_value (GtkTreeModel *model,
				    GtkTreeIter *iter,
				    gint column,
				    GValue *value)
{
	RejillaVideoTreeModelPrivate *priv;
	RejillaVideoTreeModel *self;
	RejillaBurnResult result;
	RejillaStatus *status;
	RejillaTrack *track;
	const gchar *string;
	GdkPixbuf *pixbuf;
	GValue *value_tag;
	GSList *tracks;
	gchar *text;

	self = REJILLA_VIDEO_TREE_MODEL (model);
	priv = REJILLA_VIDEO_TREE_MODEL_PRIVATE (model);

	/* make sure that iter comes from us */
	g_return_if_fail (priv->stamp == iter->stamp);
	g_return_if_fail (iter->user_data != NULL);

	track = iter->user_data;
	if (!REJILLA_IS_TRACK_STREAM (track))
		return;

	if (GPOINTER_TO_INT (iter->user_data2) == REJILLA_STREAM_ROW_GAP) {
		switch (column) {
		case REJILLA_VIDEO_TREE_MODEL_WEIGHT:
			g_value_init (value, PANGO_TYPE_STYLE);
			g_value_set_enum (value, PANGO_WEIGHT_BOLD);
			return;
		case REJILLA_VIDEO_TREE_MODEL_STYLE:
			g_value_init (value, PANGO_TYPE_STYLE);
			g_value_set_enum (value, PANGO_STYLE_ITALIC);
			return;
		case REJILLA_VIDEO_TREE_MODEL_NAME:
			g_value_init (value, G_TYPE_STRING);
			g_value_set_string (value, _("Pause"));
			break;
		case REJILLA_VIDEO_TREE_MODEL_ICON_NAME:
			g_value_init (value, G_TYPE_STRING);
			g_value_set_string (value, GTK_STOCK_MEDIA_PAUSE);
			break;
		case REJILLA_VIDEO_TREE_MODEL_EDITABLE:
		case REJILLA_VIDEO_TREE_MODEL_SELECTABLE:
			g_value_init (value, G_TYPE_BOOLEAN);
			g_value_set_boolean (value, FALSE);
			break;
		case REJILLA_VIDEO_TREE_MODEL_IS_GAP:
			g_value_init (value, G_TYPE_BOOLEAN);
			g_value_set_boolean (value, TRUE);
			break;

		case REJILLA_VIDEO_TREE_MODEL_SIZE:
			g_value_init (value, G_TYPE_STRING);
			text = rejilla_units_get_time_string (rejilla_track_stream_get_gap (REJILLA_TRACK_STREAM (track)), TRUE, FALSE);
			g_value_set_string (value, text);
			g_free (text);
			break;

		case REJILLA_VIDEO_TREE_MODEL_INDEX:
		case REJILLA_VIDEO_TREE_MODEL_ARTIST:
			g_value_init (value, G_TYPE_STRING);
			g_value_set_string (value, NULL);
			break;

		case REJILLA_VIDEO_TREE_MODEL_THUMBNAIL:
		case REJILLA_VIDEO_TREE_MODEL_INDEX_NUM:
		default:
			g_value_init (value, G_TYPE_INVALID);
			break;
		}

		return;
	}

	switch (column) {
	case REJILLA_VIDEO_TREE_MODEL_WEIGHT:
		g_value_init (value, PANGO_TYPE_STYLE);
		g_value_set_enum (value, PANGO_WEIGHT_NORMAL);
		return;
	case REJILLA_VIDEO_TREE_MODEL_STYLE:
		g_value_init (value, PANGO_TYPE_STYLE);
		g_value_set_enum (value, PANGO_STYLE_NORMAL);
		return;
	case REJILLA_VIDEO_TREE_MODEL_NAME:
		g_value_init (value, G_TYPE_STRING);

		string = rejilla_track_tag_lookup_string (track, REJILLA_TRACK_STREAM_TITLE_TAG);
		if (string) {
			g_value_set_string (value, string);
		}
		else {
			GFile *file;
			gchar *uri;
			gchar *name;
			gchar *unescaped;

			uri = rejilla_track_stream_get_source (REJILLA_TRACK_STREAM (track), TRUE);
			unescaped = g_uri_unescape_string (uri, NULL);
			g_free (uri);

			file = g_file_new_for_uri (unescaped);
			g_free (unescaped);

			name = g_file_get_basename (file);
			g_object_unref (file);

			g_value_set_string (value, name);
			g_free (name);
		}

		return;

	case REJILLA_VIDEO_TREE_MODEL_ARTIST:
		g_value_init (value, G_TYPE_STRING);

		string = rejilla_track_tag_lookup_string (track, REJILLA_TRACK_STREAM_ARTIST_TAG);
		if (string) 
			g_value_set_string (value, string);

		return;

	case REJILLA_VIDEO_TREE_MODEL_ICON_NAME:
		status = rejilla_status_new ();
		rejilla_track_get_status (track, status);
		g_value_init (value, G_TYPE_STRING);

		value_tag = NULL;
		result = rejilla_status_get_result (status);
		if (result == REJILLA_BURN_NOT_READY || result == REJILLA_BURN_RUNNING)
			g_value_set_string (value, "image-loading");
		else if (rejilla_track_tag_lookup (track, REJILLA_TRACK_STREAM_MIME_TAG, &value_tag) == REJILLA_BURN_OK)
			g_value_set_string (value, g_value_get_string (value_tag));
		else
			g_value_set_string (value, "image-missing");

		g_object_unref (status);
		return;

	case REJILLA_VIDEO_TREE_MODEL_THUMBNAIL:
		g_value_init (value, GDK_TYPE_PIXBUF);

		status = rejilla_status_new ();
		rejilla_track_get_status (track, status);
		result = rejilla_status_get_result (status);

		if (result == REJILLA_BURN_NOT_READY || result == REJILLA_BURN_RUNNING)
			pixbuf = gtk_icon_theme_load_icon (priv->theme,
							   "image-loading",
							   48,
							   0,
							   NULL);
		else {
			value_tag = NULL;
			rejilla_track_tag_lookup (track,
						  REJILLA_TRACK_STREAM_THUMBNAIL_TAG,
						  &value_tag);

			if (value_tag)
				pixbuf = g_value_dup_object (value_tag);
			else
				pixbuf = gtk_icon_theme_load_icon (priv->theme,
								   "image-missing",
								   48,
								   0,
								   NULL);
		}

		g_value_set_object (value, pixbuf);
		g_object_unref (pixbuf);

		g_object_unref (status);
		return;

	case REJILLA_VIDEO_TREE_MODEL_SIZE:
		status = rejilla_status_new ();
		rejilla_track_get_status (track, status);

		g_value_init (value, G_TYPE_STRING);

		result = rejilla_status_get_result (status);
		if (result == REJILLA_BURN_OK) {
			guint64 len = 0;

			rejilla_track_stream_get_length (REJILLA_TRACK_STREAM (track), &len);
			len -= rejilla_track_stream_get_gap (REJILLA_TRACK_STREAM (track));
			text = rejilla_units_get_time_string (len, TRUE, FALSE);
			g_value_set_string (value, text);
			g_free (text);
		}
		else
			g_value_set_string (value, _("(loading…)"));

		g_object_unref (status);
		return;

	case REJILLA_VIDEO_TREE_MODEL_EDITABLE:
		g_value_init (value, G_TYPE_BOOLEAN);
		/* This can be used for gap lines */
		g_value_set_boolean (value, TRUE);
		//g_value_set_boolean (value, file->editable);
		return;

	case REJILLA_VIDEO_TREE_MODEL_SELECTABLE:
		g_value_init (value, G_TYPE_BOOLEAN);
		/* This can be used for gap lines */
		g_value_set_boolean (value, TRUE);
		//g_value_set_boolean (value, file->editable);
		return;

	case REJILLA_VIDEO_TREE_MODEL_IS_GAP:
		g_value_init (value, G_TYPE_BOOLEAN);
		g_value_set_boolean (value, FALSE);
		return;

	case REJILLA_VIDEO_TREE_MODEL_INDEX:
		tracks = rejilla_burn_session_get_tracks (REJILLA_BURN_SESSION (priv->session));
		g_value_init (value, G_TYPE_STRING);
		text = g_strdup_printf ("%02i", g_slist_index (tracks, track) + 1);
		g_value_set_string (value, text);
		g_free (text);
		return;

	case REJILLA_VIDEO_TREE_MODEL_INDEX_NUM:
		tracks = rejilla_burn_session_get_tracks (REJILLA_BURN_SESSION (priv->session));
		g_value_init (value, G_TYPE_UINT);
		g_value_set_uint (value, g_slist_index (tracks, track) + 1);
		return;

	default:
		break;
	}
}

GtkTreePath *
rejilla_video_tree_model_track_to_path (RejillaVideoTreeModel *self,
				        RejillaTrack *track_arg)
{
	RejillaVideoTreeModelPrivate *priv;
	GSList *tracks;
	gint nth = 0;

	if (!REJILLA_IS_TRACK_STREAM (track_arg))
		return NULL;

	priv = REJILLA_VIDEO_TREE_MODEL_PRIVATE (self);

	tracks = rejilla_burn_session_get_tracks (REJILLA_BURN_SESSION (priv->session));
	for (; tracks; tracks = tracks->next) {
		RejillaTrackStream *track;

		track = tracks->data;
		if (track == REJILLA_TRACK_STREAM (track_arg))
			break;

		nth ++;

		if (rejilla_track_stream_get_gap (track) > 0)
			nth ++;

	}

	return gtk_tree_path_new_from_indices (nth, -1);
}

static GtkTreePath *
rejilla_video_tree_model_get_path (GtkTreeModel *model,
				   GtkTreeIter *iter)
{
	RejillaVideoTreeModelPrivate *priv;
	GSList *tracks;
	gint nth = 0;

	priv = REJILLA_VIDEO_TREE_MODEL_PRIVATE (model);

	/* make sure that iter comes from us */
	g_return_val_if_fail (priv->stamp == iter->stamp, NULL);
	g_return_val_if_fail (iter->user_data != NULL, NULL);

	tracks = rejilla_burn_session_get_tracks (REJILLA_BURN_SESSION (priv->session));
	for (; tracks; tracks = tracks->next) {
		RejillaTrackStream *track;

		track = tracks->data;
		if (GPOINTER_TO_INT (iter->user_data2) == REJILLA_STREAM_ROW_NORMAL
		&&  track == iter->user_data)
			break;

		nth ++;

		if (rejilla_track_stream_get_gap (track) > 0) {
			if (GPOINTER_TO_INT (iter->user_data2) == REJILLA_STREAM_ROW_GAP
			&&  track == iter->user_data)
				break;

			nth ++;
		}
	}

	/* NOTE: there is only one single file without a name: root */
	return gtk_tree_path_new_from_indices (nth, -1);
}

RejillaTrack *
rejilla_video_tree_model_path_to_track (RejillaVideoTreeModel *self,
				        GtkTreePath *path)
{
	RejillaVideoTreeModelPrivate *priv;
	const gint *indices;
	GSList *tracks;
	guint depth;
	gint index;

	priv = REJILLA_VIDEO_TREE_MODEL_PRIVATE (self);

	indices = gtk_tree_path_get_indices (path);
	depth = gtk_tree_path_get_depth (path);

	/* NOTE: it can happen that paths are depth 2 when there is DND but then
	 * only the first index is relevant. */
	if (depth > 2)
		return NULL;

	/* Whether it is a GAP or a NORMAL row is of no importance */
	tracks = rejilla_burn_session_get_tracks (REJILLA_BURN_SESSION (priv->session));
	index = indices [0];
	for (; tracks; tracks = tracks->next) {
		RejillaTrackStream *track;

		track = tracks->data;
		if (index <= 0)
			return REJILLA_TRACK (track);

		index --;

		if (rejilla_track_stream_get_gap (track) > 0) {
			if (index <= 0)
				return REJILLA_TRACK (track);

				index --;
		}
	}

	return NULL;
}

static gboolean
rejilla_video_tree_model_get_iter (GtkTreeModel *model,
				   GtkTreeIter *iter,
				   GtkTreePath *path)
{
	RejillaVideoTreeModelPrivate *priv;
	const gint *indices;
	GSList *tracks;
	guint depth;
	gint index;

	priv = REJILLA_VIDEO_TREE_MODEL_PRIVATE (model);

	depth = gtk_tree_path_get_depth (path);

	/* NOTE: it can happen that paths are depth 2 when there is DND but then
	 * only the first index is relevant. */
	if (depth > 2)
		return FALSE;

	/* Whether it is a GAP or a NORMAL row is of no importance */
	indices = gtk_tree_path_get_indices (path);
	index = indices [0];
	tracks = rejilla_burn_session_get_tracks (REJILLA_BURN_SESSION (priv->session));
	for (; tracks; tracks = tracks->next) {
		RejillaTrackStream *track;

		track = tracks->data;
		if (index <= 0) {
			iter->stamp = priv->stamp;
			iter->user_data2 = GINT_TO_POINTER (REJILLA_STREAM_ROW_NORMAL);
			iter->user_data = track;
			return TRUE;
		}
		index --;

		if (rejilla_track_stream_get_gap (track) > 0) {
			if (index <= 0) {
				iter->stamp = priv->stamp;
				iter->user_data2 = GINT_TO_POINTER (REJILLA_STREAM_ROW_GAP);
				iter->user_data = track;
				return TRUE;
			}
			index --;
		}
	}

	return FALSE;
}

static RejillaTrack *
rejilla_video_tree_model_track_next (RejillaVideoTreeModel *model,
				     RejillaTrack *track)
{
	RejillaVideoTreeModelPrivate *priv;
	GSList *tracks;
	GSList *node;

	priv = REJILLA_VIDEO_TREE_MODEL_PRIVATE (model);

	tracks = rejilla_burn_session_get_tracks (REJILLA_BURN_SESSION (priv->session));
	node = g_slist_find (tracks, track);
	if (!node || !node->next)
		return NULL;

	return node->next->data;
}

static RejillaTrack *
rejilla_video_tree_model_track_previous (RejillaVideoTreeModel *model,
					 RejillaTrack *track)
{
	RejillaVideoTreeModelPrivate *priv;
	GSList *tracks;

	priv = REJILLA_VIDEO_TREE_MODEL_PRIVATE (model);

	tracks = rejilla_burn_session_get_tracks (REJILLA_BURN_SESSION (priv->session));
	while (tracks && tracks->next) {
		if (tracks->next->data == track)
			return tracks->data;

		tracks = tracks->next;
	}

	return NULL;
}

static gboolean
rejilla_video_tree_model_iter_next (GtkTreeModel *model,
				    GtkTreeIter *iter)
{
	RejillaVideoTreeModelPrivate *priv;
	RejillaTrackStream *track;
	GSList *tracks;
	GSList *node;

	priv = REJILLA_VIDEO_TREE_MODEL_PRIVATE (model);

	/* make sure that iter comes from us */
	g_return_val_if_fail (priv->stamp == iter->stamp, FALSE);
	g_return_val_if_fail (iter->user_data != NULL, FALSE);

	track = REJILLA_TRACK_STREAM (iter->user_data);
	if (!track)
		return FALSE;

	if (GPOINTER_TO_INT (iter->user_data2) == REJILLA_STREAM_ROW_NORMAL
	&&  rejilla_track_stream_get_gap (track) > 0) {
		iter->user_data2 = GINT_TO_POINTER (REJILLA_STREAM_ROW_GAP);
		return TRUE;
	}

	tracks = rejilla_burn_session_get_tracks (REJILLA_BURN_SESSION (priv->session));
	node = g_slist_find (tracks, track);
	if (!node || !node->next)
		return FALSE;

	iter->user_data2 = GINT_TO_POINTER (REJILLA_STREAM_ROW_NORMAL);
	iter->user_data = node->next->data;
	return TRUE;
}

static GType
rejilla_video_tree_model_get_column_type (GtkTreeModel *model,
					 gint index)
{
	switch (index) {
	case REJILLA_VIDEO_TREE_MODEL_NAME:
		return G_TYPE_STRING;

	case REJILLA_VIDEO_TREE_MODEL_ARTIST:
		return G_TYPE_STRING;

	case REJILLA_VIDEO_TREE_MODEL_THUMBNAIL:
		return GDK_TYPE_PIXBUF;

	case REJILLA_VIDEO_TREE_MODEL_ICON_NAME:
		return G_TYPE_STRING;

	case REJILLA_VIDEO_TREE_MODEL_SIZE:
		return G_TYPE_STRING;

	case REJILLA_VIDEO_TREE_MODEL_EDITABLE:
		return G_TYPE_BOOLEAN;

	case REJILLA_VIDEO_TREE_MODEL_SELECTABLE:
		return G_TYPE_BOOLEAN;

	case REJILLA_VIDEO_TREE_MODEL_INDEX:
		return G_TYPE_STRING;

	case REJILLA_VIDEO_TREE_MODEL_INDEX_NUM:
		return G_TYPE_UINT;

	case REJILLA_VIDEO_TREE_MODEL_IS_GAP:
		return G_TYPE_STRING;

	case REJILLA_VIDEO_TREE_MODEL_WEIGHT:
		return PANGO_TYPE_WEIGHT;

	case REJILLA_VIDEO_TREE_MODEL_STYLE:
		return PANGO_TYPE_STYLE;

	default:
		break;
	}

	return G_TYPE_INVALID;
}

static gint
rejilla_video_tree_model_get_n_columns (GtkTreeModel *model)
{
	return REJILLA_VIDEO_TREE_MODEL_COL_NUM;
}

static GtkTreeModelFlags
rejilla_video_tree_model_get_flags (GtkTreeModel *model)
{
	return GTK_TREE_MODEL_LIST_ONLY;
}

static gboolean
rejilla_video_tree_model_multi_row_draggable (EggTreeMultiDragSource *drag_source,
					      GList *path_list)
{
	/* All rows are draggable so return TRUE */
	return TRUE;
}

static gboolean
rejilla_video_tree_model_multi_drag_data_get (EggTreeMultiDragSource *drag_source,
					      GList *path_list,
					      GtkSelectionData *selection_data)
{
	if (gtk_selection_data_get_target (selection_data) == gdk_atom_intern (REJILLA_DND_TARGET_SELF_FILE_NODES, TRUE)) {
		RejillaDNDVideoContext context;

		context.model = GTK_TREE_MODEL (drag_source);
		context.references = path_list;

		gtk_selection_data_set (selection_data,
					gdk_atom_intern_static_string (REJILLA_DND_TARGET_SELF_FILE_NODES),
					8,
					(void *) &context,
					sizeof (context));
	}
	else
		return FALSE;

	return TRUE;
}

static gboolean
rejilla_video_tree_model_multi_drag_data_delete (EggTreeMultiDragSource *drag_source,
						 GList *path_list)
{
	/* NOTE: it's not the data in the selection_data here that should be
	 * deleted but rather the rows selected when there is a move. FALSE
	 * here means that we didn't delete anything. */
	/* return TRUE to stop other handlers */
	return TRUE;
}

void
rejilla_video_tree_model_move_before (RejillaVideoTreeModel *self,
				      GtkTreeIter *iter,
				      GtkTreePath *dest_before)
{
	RejillaTrack *track;
	GtkTreeIter sibling;
	RejillaTrack *track_sibling;
	RejillaVideoTreeModelPrivate *priv;

	priv = REJILLA_VIDEO_TREE_MODEL_PRIVATE (self);

	track = REJILLA_TRACK (iter->user_data);
	if (!dest_before || !rejilla_video_tree_model_get_iter (GTK_TREE_MODEL (self), &sibling, dest_before)) {
		if (GPOINTER_TO_INT (iter->user_data2) == REJILLA_STREAM_ROW_GAP) {
			guint64 gap;
			GSList *tracks;

			gap = rejilla_track_stream_get_gap (REJILLA_TRACK_STREAM (track));
			rejilla_track_stream_set_boundaries (REJILLA_TRACK_STREAM (track),
							     -1,
							     -1,
							     0);
			rejilla_track_changed (track);

			/* Get last track */
			tracks = rejilla_burn_session_get_tracks (REJILLA_BURN_SESSION (priv->session));
			tracks = g_slist_last (tracks);
			track_sibling = tracks->data;

			gap += rejilla_track_stream_get_gap (REJILLA_TRACK_STREAM (track_sibling));
			rejilla_track_stream_set_boundaries (REJILLA_TRACK_STREAM (track_sibling),
							     -1,
							     -1,
							     gap);
			rejilla_track_changed (track_sibling);
			return;
		}

		rejilla_burn_session_move_track (REJILLA_BURN_SESSION (priv->session),
						 track,
						 NULL);
		return;
	}

	track_sibling = REJILLA_TRACK (sibling.user_data);

	if (GPOINTER_TO_INT (iter->user_data2) == REJILLA_STREAM_ROW_GAP) {
		guint64 gap;
		RejillaTrack *previous_sibling;

		/* Merge the gaps or add it */
		gap = rejilla_track_stream_get_gap (REJILLA_TRACK_STREAM (track));
		rejilla_track_stream_set_boundaries (REJILLA_TRACK_STREAM (track),
						     -1,
						     -1,
						     0);
		rejilla_track_changed (track);

		if (GPOINTER_TO_INT (sibling.user_data2) == REJILLA_STREAM_ROW_GAP) {
			gap += rejilla_track_stream_get_gap (REJILLA_TRACK_STREAM (track_sibling));
			rejilla_track_stream_set_boundaries (REJILLA_TRACK_STREAM (track_sibling),
							     -1,
							     -1,
							     gap);
			rejilla_track_changed (track_sibling);
			return;
		}

		/* get the track before track_sibling */
		previous_sibling = rejilla_video_tree_model_track_previous (self, track_sibling);
		if (previous_sibling)
			track_sibling = previous_sibling;

		gap += rejilla_track_stream_get_gap (REJILLA_TRACK_STREAM (track_sibling));
		rejilla_track_stream_set_boundaries (REJILLA_TRACK_STREAM (track_sibling),
						     -1,
						     -1,
						     gap);
		rejilla_track_changed (track_sibling);
		return;
	}

	if (GPOINTER_TO_INT (sibling.user_data2) == REJILLA_STREAM_ROW_GAP) {
		guint64 gap;

		/* merge */
		gap = rejilla_track_stream_get_gap (REJILLA_TRACK_STREAM (track_sibling));
		rejilla_track_stream_set_boundaries (REJILLA_TRACK_STREAM (track_sibling),
						     -1,
						     -1,
						     0);
		rejilla_track_changed (track_sibling);

		gap += rejilla_track_stream_get_gap (REJILLA_TRACK_STREAM (track));
		rejilla_track_stream_set_boundaries (REJILLA_TRACK_STREAM (track),
						     -1,
						     -1,
						     gap);
		rejilla_track_changed (track);

		/* Track sibling is now the next track of current track_sibling */
		track_sibling = rejilla_video_tree_model_track_next (self, track_sibling);
	}

	rejilla_burn_session_move_track (REJILLA_BURN_SESSION (priv->session),
					 track,
					 track_sibling);
}

static gboolean
rejilla_video_tree_model_drag_data_received (GtkTreeDragDest *drag_dest,
					     GtkTreePath *dest_path,
					     GtkSelectionData *selection_data)
{
	RejillaTrack *sibling;
	RejillaVideoTreeModelPrivate *priv;
	GdkAtom target;

	priv = REJILLA_VIDEO_TREE_MODEL_PRIVATE (drag_dest);

	/* The new row(s) must be before dest_path but after our sibling */
	sibling = rejilla_video_tree_model_path_to_track (REJILLA_VIDEO_TREE_MODEL (drag_dest), dest_path);

	/* Received data: see where it comes from:
	 * - from us, then that's a simple move
	 * - from another widget then it's going to be URIS and we add
	 *   them to VideoProject */
	target = gtk_selection_data_get_target (selection_data);
	if (target == gdk_atom_intern (REJILLA_DND_TARGET_SELF_FILE_NODES, TRUE)) {
		RejillaDNDVideoContext *context;
		GtkTreeRowReference *dest;
		GList *iter;

		context = (RejillaDNDVideoContext *) gtk_selection_data_get_data (selection_data);
		if (context->model != GTK_TREE_MODEL (drag_dest))
			return TRUE;

		dest = gtk_tree_row_reference_new (GTK_TREE_MODEL (drag_dest), dest_path);
		/* That's us: move the row and its children. */
		for (iter = context->references; iter; iter = iter->next) {
			GtkTreeRowReference *reference;
			GtkTreePath *destination;
			GtkTreePath *treepath;
			GtkTreeIter tree_iter;

			reference = iter->data;
			treepath = gtk_tree_row_reference_get_path (reference);
			gtk_tree_model_get_iter (GTK_TREE_MODEL (drag_dest),
						 &tree_iter,
						 treepath);
			gtk_tree_path_free (treepath);

			destination = gtk_tree_row_reference_get_path (dest);
			rejilla_video_tree_model_move_before (REJILLA_VIDEO_TREE_MODEL (drag_dest),
							      &tree_iter,
							      destination);
			gtk_tree_path_free (destination);
		}
		gtk_tree_row_reference_free (dest);
	}
	else if (target == gdk_atom_intern ("text/uri-list", TRUE)) {
		gint i;
		gchar **uris = NULL;
		gboolean success = FALSE;
		const guchar *selection_data_raw;

		/* NOTE: for some reason gdk_text_property_to_utf8_list_for_display ()
		 * fails with banshee DND URIs list when calling gtk_selection_data_get_uris ().
		 * so do like caja */

		/* NOTE: there can be many URIs at the same time. One
		 * success is enough to return TRUE. */
		selection_data_raw = gtk_selection_data_get_data (selection_data);
		uris = gtk_selection_data_get_uris (selection_data);
		if (!uris) {
			const guchar *selection_data_raw;

			selection_data_raw = gtk_selection_data_get_data (selection_data);
			uris = g_uri_list_extract_uris ((gchar *) selection_data_raw);
		}

		if (!uris)
			return TRUE;

		success = FALSE;
		for (i = 0; uris [i]; i ++) {
			RejillaTrackStreamCfg *track;

			/* Add the URIs to the project */
			track = rejilla_track_stream_cfg_new ();
			rejilla_track_stream_set_source (REJILLA_TRACK_STREAM (track), uris [i]);
			rejilla_burn_session_add_track (REJILLA_BURN_SESSION (priv->session),
							REJILLA_TRACK (track),
							sibling);
		}
		g_strfreev (uris);
	}
	else
		return FALSE;

	return TRUE;
}

static gboolean
rejilla_video_tree_model_row_drop_possible (GtkTreeDragDest *drag_dest,
					    GtkTreePath *dest_path,
					    GtkSelectionData *selection_data)
{
	/* It's always possible */
	return TRUE;
}

static gboolean
rejilla_video_tree_model_drag_data_delete (GtkTreeDragSource *source,
					   GtkTreePath *treepath)
{
	return TRUE;
}

static void
rejilla_video_tree_model_reindex (RejillaVideoTreeModel *model,
				  RejillaBurnSession *session,
				  RejillaTrack *track_arg,
				  GtkTreeIter *iter,
				  GtkTreePath *path)
{
	GSList *tracks;
	RejillaVideoTreeModelPrivate *priv;

	priv = REJILLA_VIDEO_TREE_MODEL_PRIVATE (model);

	/* tracks (including) after sibling need to be reindexed */
	tracks = rejilla_burn_session_get_tracks (session);
	tracks = g_slist_find (tracks, track_arg);
	if (!tracks)
		return;

	tracks = tracks->next;
	for (; tracks; tracks = tracks->next) {
		RejillaTrack *track;

		track = tracks->data;

		iter->stamp = priv->stamp;
		iter->user_data = track;
		iter->user_data2 = GINT_TO_POINTER (REJILLA_STREAM_ROW_NORMAL);

		gtk_tree_path_next (path);
		gtk_tree_model_row_changed (GTK_TREE_MODEL (model),
					    path,
					    iter);

		/* skip gap rows */
		if (rejilla_track_stream_get_gap (REJILLA_TRACK_STREAM (track)) > 0)
			gtk_tree_path_next (path);
	}
}

static void
rejilla_video_tree_model_track_added (RejillaBurnSession *session,
				      RejillaTrack *track,
				      RejillaVideoTreeModel *model)
{
	RejillaVideoTreeModelPrivate *priv;
	GtkTreePath *path;
	GtkTreeIter iter;

	if (!REJILLA_IS_TRACK_STREAM (track))
		return;

	priv = REJILLA_VIDEO_TREE_MODEL_PRIVATE (model);

	iter.stamp = priv->stamp;
	iter.user_data = track;
	iter.user_data2 = GINT_TO_POINTER (REJILLA_STREAM_ROW_NORMAL);

	path = rejilla_video_tree_model_track_to_path (model, track);

	/* if the file is reloading (because of a file system change or because
	 * it was a file that was a tmp folder) then no need to signal an added
	 * signal but a changed one */
	gtk_tree_model_row_inserted (GTK_TREE_MODEL (model),
				     path,
				     &iter);

	if (rejilla_track_stream_get_gap (REJILLA_TRACK_STREAM (track)) > 0) {
		priv->gaps = g_slist_prepend (priv->gaps, track);

		iter.user_data2 = GINT_TO_POINTER (REJILLA_STREAM_ROW_GAP);
		gtk_tree_path_next (path);
		gtk_tree_model_row_inserted (GTK_TREE_MODEL (model),
					     path,
					     &iter);
	}

	/* tracks (including) after sibling need to be reindexed */
	rejilla_video_tree_model_reindex (model, session, track, &iter, path);
	gtk_tree_path_free (path);
}

static void
rejilla_video_tree_model_track_removed (RejillaBurnSession *session,
					RejillaTrack *track,
					guint former_location,
					RejillaVideoTreeModel *model)
{
	RejillaVideoTreeModelPrivate *priv;
	GtkTreePath *path;
	GtkTreeIter iter;

	if (!REJILLA_IS_TRACK_STREAM (track))
		return;

	priv = REJILLA_VIDEO_TREE_MODEL_PRIVATE (model);

	/* remove the file. */
	path = gtk_tree_path_new_from_indices (former_location, -1);
	gtk_tree_model_row_deleted (GTK_TREE_MODEL (model), path);

	if (rejilla_track_stream_get_gap (REJILLA_TRACK_STREAM (track)) > 0) {
		priv->gaps = g_slist_remove (priv->gaps, track);
		gtk_tree_model_row_deleted (GTK_TREE_MODEL (model), path);
	}

	/* tracks (including) after former_location need to be reindexed */
	rejilla_video_tree_model_reindex (model, session, track, &iter, path);
	gtk_tree_path_free (path);
}

static void
rejilla_video_tree_model_track_changed (RejillaBurnSession *session,
					RejillaTrack *track,
					RejillaVideoTreeModel *model)
{
	RejillaVideoTreeModelPrivate *priv;
	GValue *value = NULL;
	GtkTreePath *path;
	GtkTreeIter iter;

	if (!REJILLA_IS_TRACK_STREAM (track))
		return;

	priv = REJILLA_VIDEO_TREE_MODEL_PRIVATE (model);

	/* scale the thumbnail */
	rejilla_track_tag_lookup (REJILLA_TRACK (track),
				  REJILLA_TRACK_STREAM_THUMBNAIL_TAG,
				  &value);
	if (value) {
		GdkPixbuf *scaled;
		GdkPixbuf *snapshot;

		snapshot = g_value_get_object (value);
		scaled = gdk_pixbuf_scale_simple (snapshot,
						  48 * gdk_pixbuf_get_width (snapshot) / gdk_pixbuf_get_height (snapshot),
						  48,
						  GDK_INTERP_BILINEAR);

		value = g_new0 (GValue, 1);
		g_value_init (value, GDK_TYPE_PIXBUF);
		g_value_set_object (value, scaled);
		rejilla_track_tag_add (track,
				       REJILLA_TRACK_STREAM_THUMBNAIL_TAG,
				       value);
	}

	/* Get the iter for the file */
	iter.stamp = priv->stamp;
	iter.user_data2 = GINT_TO_POINTER (REJILLA_STREAM_ROW_NORMAL);
	iter.user_data = track;

	path = rejilla_video_tree_model_track_to_path (model, track);
	gtk_tree_model_row_changed (GTK_TREE_MODEL (model),
				    path,
				    &iter);

	/* Get the iter for a possible gap row.
	 * The problem is to know whether one was added, removed or simply
	 * changed. */
	gtk_tree_path_next (path);
	if (rejilla_track_stream_get_gap (REJILLA_TRACK_STREAM (track)) > 0) {
		iter.user_data2 = GINT_TO_POINTER (REJILLA_STREAM_ROW_GAP);
		if (!g_slist_find (priv->gaps, track)) {
			priv->gaps = g_slist_prepend (priv->gaps,  track);
			gtk_tree_model_row_inserted (GTK_TREE_MODEL (model),
						     path,
						     &iter);
		}
		else
			gtk_tree_model_row_changed (GTK_TREE_MODEL (model),
						    path,
						    &iter);
	}
	else if (g_slist_find (priv->gaps, track)) {
		priv->gaps = g_slist_remove (priv->gaps, track);
		gtk_tree_model_row_deleted (GTK_TREE_MODEL (model), path);		
	}

	gtk_tree_path_free (path);
}

void
rejilla_video_tree_model_set_session (RejillaVideoTreeModel *model,
				      RejillaSessionCfg *session)
{
	RejillaVideoTreeModelPrivate *priv;

	priv = REJILLA_VIDEO_TREE_MODEL_PRIVATE (model);
	if (priv->session) {
		g_signal_handlers_disconnect_by_func (priv->session,
						      rejilla_video_tree_model_track_added,
						      model);
		g_signal_handlers_disconnect_by_func (priv->session,
						      rejilla_video_tree_model_track_removed,
						      model);
		g_signal_handlers_disconnect_by_func (priv->session,
						      rejilla_video_tree_model_track_changed,
						      model);
		g_object_unref (priv->session);
		priv->session = NULL;
	}

	if (!session)
		return;

	priv->session = g_object_ref (session);
	g_signal_connect (session,
			  "track-added",
			  G_CALLBACK (rejilla_video_tree_model_track_added),
			  model);
	g_signal_connect (session,
			  "track-removed",
			  G_CALLBACK (rejilla_video_tree_model_track_removed),
			  model);
	g_signal_connect (session,
			  "track-changed",
			  G_CALLBACK (rejilla_video_tree_model_track_changed),
			  model);
}

RejillaSessionCfg *
rejilla_video_tree_model_get_session (RejillaVideoTreeModel *model)
{
	RejillaVideoTreeModelPrivate *priv;

	priv = REJILLA_VIDEO_TREE_MODEL_PRIVATE (model);
	return priv->session;
}

static void
rejilla_video_tree_model_init (RejillaVideoTreeModel *object)
{
	RejillaVideoTreeModelPrivate *priv;

	priv = REJILLA_VIDEO_TREE_MODEL_PRIVATE (object);

	priv->theme = gtk_icon_theme_get_default ();

	do {
		priv->stamp = g_random_int ();
	} while (!priv->stamp);
}

static void
rejilla_video_tree_model_finalize (GObject *object)
{
	RejillaVideoTreeModelPrivate *priv;

	priv = REJILLA_VIDEO_TREE_MODEL_PRIVATE (object);

	if (priv->session) {
		g_signal_handlers_disconnect_by_func (priv->session,
						      rejilla_video_tree_model_track_added,
						      object);
		g_signal_handlers_disconnect_by_func (priv->session,
						      rejilla_video_tree_model_track_removed,
						      object);
		g_signal_handlers_disconnect_by_func (priv->session,
						      rejilla_video_tree_model_track_changed,
						      object);
		g_object_unref (priv->session);
		priv->session = NULL;
	}

	if (priv->gaps) {
		g_slist_free (priv->gaps);
		priv->gaps = NULL;
	}

	G_OBJECT_CLASS (rejilla_video_tree_model_parent_class)->finalize (object);
}

static void
rejilla_video_tree_model_iface_init (gpointer g_iface, gpointer data)
{
	GtkTreeModelIface *iface = g_iface;
	static gboolean initialized = FALSE;

	if (initialized)
		return;

	initialized = TRUE;

	iface->get_flags = rejilla_video_tree_model_get_flags;
	iface->get_n_columns = rejilla_video_tree_model_get_n_columns;
	iface->get_column_type = rejilla_video_tree_model_get_column_type;
	iface->get_iter = rejilla_video_tree_model_get_iter;
	iface->get_path = rejilla_video_tree_model_get_path;
	iface->get_value = rejilla_video_tree_model_get_value;
	iface->iter_next = rejilla_video_tree_model_iter_next;
	iface->iter_children = rejilla_video_tree_model_iter_children;
	iface->iter_has_child = rejilla_video_tree_model_iter_has_child;
	iface->iter_n_children = rejilla_video_tree_model_iter_n_children;
	iface->iter_nth_child = rejilla_video_tree_model_iter_nth_child;
	iface->iter_parent = rejilla_video_tree_model_iter_parent;
}

static void
rejilla_video_tree_model_multi_drag_source_iface_init (gpointer g_iface, gpointer data)
{
	EggTreeMultiDragSourceIface *iface = g_iface;
	static gboolean initialized = FALSE;

	if (initialized)
		return;

	initialized = TRUE;

	iface->row_draggable = rejilla_video_tree_model_multi_row_draggable;
	iface->drag_data_get = rejilla_video_tree_model_multi_drag_data_get;
	iface->drag_data_delete = rejilla_video_tree_model_multi_drag_data_delete;
}

static void
rejilla_video_tree_model_drag_source_iface_init (gpointer g_iface, gpointer data)
{
	GtkTreeDragSourceIface *iface = g_iface;
	static gboolean initialized = FALSE;

	if (initialized)
		return;

	initialized = TRUE;

	iface->drag_data_delete = rejilla_video_tree_model_drag_data_delete;
}

static void
rejilla_video_tree_model_drag_dest_iface_init (gpointer g_iface, gpointer data)
{
	GtkTreeDragDestIface *iface = g_iface;
	static gboolean initialized = FALSE;

	if (initialized)
		return;

	initialized = TRUE;

	iface->drag_data_received = rejilla_video_tree_model_drag_data_received;
	iface->row_drop_possible = rejilla_video_tree_model_row_drop_possible;
}

static void
rejilla_video_tree_model_class_init (RejillaVideoTreeModelClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (RejillaVideoTreeModelPrivate));

	object_class->finalize = rejilla_video_tree_model_finalize;
}

RejillaVideoTreeModel *
rejilla_video_tree_model_new (void)
{
	return g_object_new (REJILLA_TYPE_VIDEO_TREE_MODEL, NULL);
}
