/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * rejilla
 * Copyright (C) Philippe Rouquier 2005-2008 <bonfire-app@wanadoo.fr>
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

#include <string.h>

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>

#include <gtk/gtk.h>

#include "rejilla-file-filtered.h"
#include "rejilla-filter-option.h"
#include "rejilla-utils.h"

#include "rejilla-track-data-cfg.h"


typedef struct _RejillaFileFilteredPrivate RejillaFileFilteredPrivate;
struct _RejillaFileFilteredPrivate
{
	GtkWidget *tree;
	GtkWidget *restore;
	GtkWidget *options;

	RejillaTrackDataCfg *track;
};

#define REJILLA_FILE_FILTERED_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REJILLA_TYPE_FILE_FILTERED, RejillaFileFilteredPrivate))

G_DEFINE_TYPE (RejillaFileFiltered, rejilla_file_filtered, GTK_TYPE_EXPANDER);

enum {
	PROP_0,
	PROP_TRACK
};

static gchar *
rejilla_file_filtered_get_label_text (RejillaFileFiltered *self)
{
	guint num;
	gchar *label;
	GtkTreeModel *model;
	RejillaFileFilteredPrivate *priv;

	priv = REJILLA_FILE_FILTERED_PRIVATE (self);
	model = gtk_tree_view_get_model (GTK_TREE_VIEW (priv->tree));
	num = gtk_tree_model_iter_n_children (model, NULL);

	if (gtk_expander_get_expanded (GTK_EXPANDER (self))) {
		if (!num)
			label = g_strdup (_("No file filtered"));
		else
			label = g_strdup_printf (ngettext ("Hide the _filtered file list (%d file)", "Hide the _filtered file list (%d files)", num), num);
	}
	else {
		if (!num)
			label = g_strdup (_("No file filtered"));
		else
			label = g_strdup_printf (ngettext ("Show the _filtered file list (%d file)", "Show the _filtered file list (%d files)", num), num);
	}

	return label;
}

static void
rejilla_file_filtered_update (RejillaFileFiltered *self)
{
	RejillaFileFilteredPrivate *priv;
	GtkWidget *widget;
	gchar *markup;

	priv = REJILLA_FILE_FILTERED_PRIVATE (self);

	markup = rejilla_file_filtered_get_label_text (self);

	widget = gtk_expander_get_label_widget (GTK_EXPANDER (self));
 	gtk_label_set_markup_with_mnemonic (GTK_LABEL (widget), markup);
	g_free (markup);
}

static void
rejilla_file_filtered_activate (GtkExpander *self)
{
	GTK_EXPANDER_CLASS (rejilla_file_filtered_parent_class)->activate (self);
	rejilla_file_filtered_update (REJILLA_FILE_FILTERED (self));
}

static void
rejilla_file_filtered_row_inserted (GtkTreeModel *model,
				    GtkTreePath *treepath,
				    GtkTreeIter *iter,
				    RejillaFileFiltered *self)
{
	rejilla_file_filtered_update (self);
}

static void
rejilla_file_filtered_row_deleted (GtkTreeModel *model,
				   GtkTreePath *treepath,
				   RejillaFileFiltered *self)
{
	rejilla_file_filtered_update (self);
}

static void
rejilla_file_filtered_option_pressed_cb (GtkButton *button,
					 RejillaFileFiltered *self)
{
	GtkWidget *option;
	GtkWidget *dialog;
	GtkWidget *toplevel;

	toplevel = gtk_widget_get_toplevel (GTK_WIDGET (self));
	dialog = gtk_dialog_new_with_buttons (_("Filter Options"),
					      GTK_WINDOW (toplevel),
					      GTK_DIALOG_DESTROY_WITH_PARENT |
					      GTK_DIALOG_MODAL,
					      GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
					      NULL);
	option = rejilla_filter_option_new ();
	gtk_widget_show (option);
	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (dialog))), option, FALSE, FALSE, 0);
	gtk_widget_show (dialog);

	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);	
}

static void
rejilla_file_filtered_restore_pressed_cb (GtkButton *button,
					  RejillaFileFiltered *self)
{
	RejillaFileFilteredPrivate *priv;
	GtkTreeSelection *selection;
	GtkTreeModel *model;
	GList *selected;
	GList *iter;

	priv = REJILLA_FILE_FILTERED_PRIVATE (self);

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->tree));
	selected = gtk_tree_selection_get_selected_rows (selection, &model);

	selected = g_list_reverse (selected);

	for (iter = selected; iter; iter = iter->next) {
		GtkTreePath *treepath;

		treepath = iter->data;
		rejilla_track_data_cfg_restore (priv->track, treepath);
	}
	g_list_free (selected);

	/* update label */
	rejilla_file_filtered_update (self);
}

static void
rejilla_file_filtered_selection_changed_cb (GtkTreeSelection *selection,
					    RejillaFileFiltered *self)
{
	RejillaFileFilteredPrivate *priv;

	priv = REJILLA_FILE_FILTERED_PRIVATE (self);

	if (gtk_tree_selection_count_selected_rows (selection))
		gtk_widget_set_sensitive (priv->restore, TRUE);
	else
		gtk_widget_set_sensitive (priv->restore, FALSE);
}

void
rejilla_file_filtered_set_right_button_group (RejillaFileFiltered *self,
					      GtkSizeGroup *group)
{
	RejillaFileFilteredPrivate *priv;

	priv = REJILLA_FILE_FILTERED_PRIVATE (self);
	gtk_size_group_add_widget (group, priv->restore);
	gtk_size_group_add_widget (group, priv->options);
}

static void
rejilla_file_filtered_init (RejillaFileFiltered *object)
{
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *button;
	GtkWidget *scroll;
	GtkWidget *mainbox;
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	RejillaFileFilteredPrivate *priv;

	priv = REJILLA_FILE_FILTERED_PRIVATE (object);

	gtk_widget_set_tooltip_text (GTK_WIDGET (object), _("Select the files you want to restore and click on the \"Restore\" button"));

	mainbox = gtk_vbox_new (FALSE, 10);
	gtk_widget_show (mainbox);
	gtk_container_add (GTK_CONTAINER (object), mainbox);

	hbox = gtk_hbox_new (FALSE, 6);
	gtk_widget_show (hbox);

	priv->tree = gtk_tree_view_new ();
	gtk_widget_show (priv->tree);
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (priv->tree), TRUE);
	gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->tree)),
				     GTK_SELECTION_MULTIPLE);
	gtk_tree_view_set_rubber_banding (GTK_TREE_VIEW (priv->tree), TRUE);

	g_signal_connect (gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->tree)),
	                  "changed",
	                  G_CALLBACK (rejilla_file_filtered_selection_changed_cb),
	                  object);

	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_expand (column, TRUE);

	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_add_attribute (column, renderer,
					    "stock-id", REJILLA_FILTERED_STOCK_ID_COL);
	gtk_tree_view_column_add_attribute (column, renderer,
					    "sensitive", REJILLA_FILTERED_FATAL_ERROR_COL);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_end (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer,
					    "text", REJILLA_FILTERED_URI_COL);
	gtk_tree_view_column_add_attribute (column, renderer,
					    "sensitive", REJILLA_FILTERED_FATAL_ERROR_COL);

	gtk_tree_view_column_set_title (column, _("Files"));
	gtk_tree_view_append_column (GTK_TREE_VIEW (priv->tree), column);
	gtk_tree_view_column_set_sort_column_id (column, REJILLA_FILTERED_URI_COL);
	gtk_tree_view_column_set_clickable (column, TRUE);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_min_width (column, 450);

	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Type"), renderer,
							   "text", REJILLA_FILTERED_STATUS_COL,
							   "sensitive", REJILLA_FILTERED_FATAL_ERROR_COL,
							   NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (priv->tree), column);
	gtk_tree_view_column_set_sort_column_id (column, REJILLA_FILTERED_STATUS_COL);
	gtk_tree_view_column_set_clickable (column, TRUE);

	scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_show (scroll);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll),
					     GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (scroll), priv->tree);

	gtk_box_pack_start (GTK_BOX (hbox),
			    scroll,
			    TRUE,
			    TRUE,
			    0);

	vbox = gtk_vbox_new (FALSE, 6);
	gtk_widget_show (vbox);
	gtk_box_pack_start (GTK_BOX (hbox), vbox, FALSE, FALSE, 0);

	button = gtk_button_new_with_mnemonic (_("_Restore"));
	gtk_widget_show (button);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	g_signal_connect (button,
			  "clicked",
			  G_CALLBACK (rejilla_file_filtered_restore_pressed_cb),
			  object);
	priv->restore = button;
	gtk_widget_set_sensitive (priv->restore, FALSE);
	gtk_widget_set_tooltip_text (priv->restore, _("Restore the selected files"));

	button = gtk_button_new_with_mnemonic (_("_Options…"));
	gtk_widget_show (button);
	gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, FALSE, 0);
	g_signal_connect (button,
			  "clicked",
			  G_CALLBACK (rejilla_file_filtered_option_pressed_cb),
			  object);
	priv->options = button;
	gtk_widget_set_tooltip_text (priv->options, _("Set the options for file filtering"));

	gtk_box_pack_start (GTK_BOX (mainbox),
			    hbox,
			    TRUE,
			    TRUE,
			    0);
}

static void
rejilla_file_filtered_set_property (GObject *object,
				    guint property_id,
				    const GValue *value,
				    GParamSpec *pspec)
{
	RejillaFileFilteredPrivate *priv;
	GtkTreeModel *model;

	priv = REJILLA_FILE_FILTERED_PRIVATE (object);

	switch (property_id) {
	case PROP_TRACK: /* Readable and only writable at creation time */
		priv->track = g_object_ref (g_value_get_object (value));
		model = rejilla_track_data_cfg_get_filtered_model (priv->track);
		gtk_tree_view_set_model (GTK_TREE_VIEW (priv->tree), model);

		g_signal_connect (model,
				  "row-deleted",
				  G_CALLBACK (rejilla_file_filtered_row_deleted),
				  object);
		g_signal_connect (model,
				  "row-inserted",
				  G_CALLBACK (rejilla_file_filtered_row_inserted),
				  object);

		g_object_unref (model);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
rejilla_file_filtered_get_property (GObject *object,
				    guint property_id,
				    GValue *value,
				    GParamSpec *pspec)
{
	RejillaFileFilteredPrivate *priv;

	priv = REJILLA_FILE_FILTERED_PRIVATE (object);

	switch (property_id) {
	case PROP_TRACK:
		g_value_set_object (value, G_OBJECT (priv->track));
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
rejilla_file_filtered_finalize (GObject *object)
{
	RejillaFileFilteredPrivate *priv;

	priv = REJILLA_FILE_FILTERED_PRIVATE (object);
	if (priv->track) {
		GtkTreeModel *model;

		model = rejilla_track_data_cfg_get_filtered_model (priv->track);
		g_signal_handlers_disconnect_by_func (model,
		                                      rejilla_file_filtered_row_deleted,
		                                      object);
		g_signal_handlers_disconnect_by_func (model,
		                                      rejilla_file_filtered_row_inserted,
		                                      object);
		g_object_unref (model);

		g_object_unref (priv->track);
		priv->track = NULL;
	}

	G_OBJECT_CLASS (rejilla_file_filtered_parent_class)->finalize (object);
}

static void
rejilla_file_filtered_class_init (RejillaFileFilteredClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkExpanderClass *expander_class = GTK_EXPANDER_CLASS (klass);

	g_type_class_add_private (klass, sizeof (RejillaFileFilteredPrivate));

	object_class->finalize = rejilla_file_filtered_finalize;
	object_class->set_property = rejilla_file_filtered_set_property;
	object_class->get_property = rejilla_file_filtered_get_property;

	expander_class->activate = rejilla_file_filtered_activate;

	g_object_class_install_property (object_class,
					 PROP_TRACK,
					 g_param_spec_object ("track",
							      "A RejillaTrackDataCfg",
							      "The RejillaTrackDataCfg used by the internal GtkTreeView",
							      REJILLA_TYPE_TRACK_DATA_CFG,
							      G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));
}

GtkWidget*
rejilla_file_filtered_new (RejillaTrackDataCfg *track)
{
	GtkWidget *object;

	object = g_object_new (REJILLA_TYPE_FILE_FILTERED,
			       "track", track,
			       "label", "",
			       "use-markup", TRUE,
			       "use-underline", TRUE,
			       NULL);

	rejilla_file_filtered_update (REJILLA_FILE_FILTERED (object));
	return object;
}
