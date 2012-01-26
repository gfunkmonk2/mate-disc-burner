/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */

/***************************************************************************
 *            project.c
 *
 *  mar nov 29 09:32:17 2005
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>

#include <gio/gio.h>

#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <gst/gst.h>

#include "rejilla-units.h"

#include "rejilla-misc.h"
#include "rejilla-jacket-edit.h"
#include "rejilla-pk.h"

#include "rejilla-tags.h"
#include "rejilla-session.h"

#include "rejilla-setting.h"

#ifdef BUILD_PREVIEW
#include "rejilla-player.h"
#endif

#include "rejilla-track-data.h"
#include "rejilla-track-data-cfg.h"
#include "rejilla-track-stream-cfg.h"
#include "rejilla-session-cfg.h"

/* These includes are not in the exported *.h files by 
 * librejilla-burn. */
#include "rejilla-medium-selection-priv.h"
#include "rejilla-session-helper.h"
#include "rejilla-dest-selection.h"
#include "rejilla-cover.h"
#include "rejilla-status-dialog.h"
#include "rejilla-video-options.h"
#include "rejilla-drive-properties.h"
#include "rejilla-image-properties.h"
#include "burn-plugin-manager.h"

#include "rejilla-project-type-chooser.h"
#include "rejilla-app.h"
#include "rejilla-project.h"
#include "rejilla-disc.h"
#include "rejilla-data-disc.h"
#include "rejilla-audio-disc.h"
#include "rejilla-video-disc.h"
#include "rejilla-uri-container.h"
#include "rejilla-layout-object.h"
#include "rejilla-disc-message.h"
#include "rejilla-file-chooser.h"
#include "rejilla-notify.h"
#include "rejilla-project-parse.h"
#include "rejilla-project-name.h"
#include "rejilla-drive-settings.h"

static void rejilla_project_class_init (RejillaProjectClass *klass);
static void rejilla_project_init (RejillaProject *sp);
static void rejilla_project_iface_uri_container_init (RejillaURIContainerIFace *iface);
static void rejilla_project_iface_layout_object_init (RejillaLayoutObjectIFace *iface);
static void rejilla_project_finalize (GObject *object);

static void
rejilla_project_save_cb (GtkAction *action, RejillaProject *project);
static void
rejilla_project_save_as_cb (GtkAction *action, RejillaProject *project);

static void
rejilla_project_add_uris_cb (GtkAction *action, RejillaProject *project);
static void
rejilla_project_remove_selected_uris_cb (GtkAction *action, RejillaProject *project);
static void
rejilla_project_empty_cb (GtkAction *action, RejillaProject *project);

static void
rejilla_project_burn_cb (GtkAction *action, RejillaProject *project);

static void
rejilla_project_burn_clicked_cb (GtkButton *button, RejillaProject *project);

static void
rejilla_project_selection_changed_cb (RejillaDisc *disc,
				      RejillaProject *project);

static gchar *
rejilla_project_get_selected_uri (RejillaURIContainer *container);
static gboolean
rejilla_project_get_boundaries (RejillaURIContainer *container,
				gint64 *start,
				gint64 *end);

static void
rejilla_project_get_proportion (RejillaLayoutObject *object,
				gint *header,
				gint *center,
				gint *footer);

static void
rejilla_project_get_proportion (RejillaLayoutObject *object,
				gint *header,
				gint *center,
				gint *footer);

struct RejillaProjectPrivate {
	RejillaSessionCfg *session;

	GtkWidget *help;
	GtkWidget *selection;
	GtkWidget *name_display;
	GtkWidget *discs;
	GtkWidget *audio;
	GtkWidget *data;
	GtkWidget *video;

	GtkWidget *message;

	GtkUIManager *manager;

	guint status_ctx;

	GtkWidget *project_status;

	/* header */
	GtkWidget *burn;

	GtkActionGroup *project_group;
	guint merge_id;

	gchar *project;

	gchar *cover;

	gint64 sectors;
	RejillaDisc *current;

	RejillaURIContainer *current_source;

	GCancellable *cancel;

	GtkWidget *chooser;
	gulong selected_id;
	gulong activated_id;

	guint is_burning:1;

    	guint burnt:1;

	guint empty:1;
	guint modified:1;
	guint has_focus:1;
	guint oversized:1;
	guint selected_uris:1;
};

static GtkActionEntry entries [] = {
	{"Save", GTK_STOCK_SAVE, NULL, NULL,
	 N_("Save current project"), G_CALLBACK (rejilla_project_save_cb)},
	{"SaveAs", GTK_STOCK_SAVE_AS, N_("Save _As…"), NULL,
	 N_("Save current project to a different location"), G_CALLBACK (rejilla_project_save_as_cb)},
	{"Add", GTK_STOCK_ADD, N_("_Add Files"), NULL,
	 N_("Add files to the project"), G_CALLBACK (rejilla_project_add_uris_cb)},
	{"DeleteProject", GTK_STOCK_REMOVE, N_("_Remove Files"), NULL,
	 N_("Remove the selected files from the project"), G_CALLBACK (rejilla_project_remove_selected_uris_cb)},
	/* Translators: "empty" is a verb here */
	{"DeleteAll", GTK_STOCK_CLEAR, N_("E_mpty Project"), NULL,
	 N_("Remove all files from the project"), G_CALLBACK (rejilla_project_empty_cb)},
	{"Burn", "media-optical-burn", N_("_Burn…"), NULL,
	 N_("Burn the disc"), G_CALLBACK (rejilla_project_burn_cb)},
};

static const gchar *description = {
	"<ui>"
	    "<menubar name='menubar' >"
		"<menu action='ProjectMenu'>"
			"<placeholder name='ProjectPlaceholder'>"
			    "<menuitem action='Save'/>"
			    "<menuitem action='SaveAs'/>"
			    "<separator/>"
			"<menuitem action='Burn'/>"
			"</placeholder>"
		"</menu>"
		
		"<menu action='EditMenu'>"
			"<placeholder name='EditPlaceholder'>"
			    "<menuitem action='Add'/>"
			    "<menuitem action='DeleteProject'/>"
			    "<menuitem action='DeleteAll'/>"
			    "<separator/>"
			"</placeholder>"
		"</menu>"

		"<menu action='ViewMenu'>"
		"</menu>"

		"<menu action='ToolMenu'>"
			"<placeholder name='DiscPlaceholder'/>"
		"</menu>"
	    "</menubar>"
	    "<toolbar name='Toolbar'>"
		"<separator/>"
		"<toolitem action='Add'/>"
		"<toolitem action='DeleteProject'/>"
		"<toolitem action='DeleteAll'/>"
		"<placeholder name='DiscButtonPlaceholder'/>"
	     "</toolbar>"
	"</ui>"
};

static GObjectClass *parent_class = NULL;

#define REJILLA_PROJECT_SIZE_WIDGET_BORDER	1

#define REJILLA_PROJECT_VERSION "0.2"

#define REJILLA_RESPONSE_ADD			1976

enum {
	TREE_MODEL_ROW = 150,
	FILE_NODES_ROW,
	TARGET_URIS_LIST,
};

static GtkTargetEntry ntables_cd[] = {
	{"GTK_TREE_MODEL_ROW", GTK_TARGET_SAME_WIDGET, TREE_MODEL_ROW},
	{"GTK_TREE_MODEL_ROW", GTK_TARGET_SAME_WIDGET, FILE_NODES_ROW},
	{"text/uri-list", 0, TARGET_URIS_LIST}
};
static guint nb_targets_cd = sizeof (ntables_cd) / sizeof (ntables_cd [0]);

GType
rejilla_project_get_type ()
{
	static GType type = 0;

	if(type == 0) {
		static const GTypeInfo our_info = {
			sizeof (RejillaProjectClass),
			NULL,
			NULL,
			(GClassInitFunc)rejilla_project_class_init,
			NULL,
			NULL,
			sizeof (RejillaProject),
			0,
			(GInstanceInitFunc) rejilla_project_init,
		};

		static const GInterfaceInfo uri_container_info =
		{
			(GInterfaceInitFunc) rejilla_project_iface_uri_container_init,
			NULL,
			NULL
		};
		static const GInterfaceInfo layout_object =
		{
			(GInterfaceInitFunc) rejilla_project_iface_layout_object_init,
			NULL,
			NULL
		};

		type = g_type_register_static (GTK_TYPE_VBOX, 
					       "RejillaProject",
					       &our_info, 0);

		g_type_add_interface_static (type,
					     REJILLA_TYPE_URI_CONTAINER,
					     &uri_container_info);
		g_type_add_interface_static (type,
					     REJILLA_TYPE_LAYOUT_OBJECT,
					     &layout_object);
	}

	return type;
}

static void
rejilla_project_class_init (RejillaProjectClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = rejilla_project_finalize;
}

static void
rejilla_project_iface_uri_container_init (RejillaURIContainerIFace *iface)
{
	iface->get_selected_uri = rejilla_project_get_selected_uri;
	iface->get_boundaries = rejilla_project_get_boundaries;
}

static void
rejilla_project_iface_layout_object_init (RejillaLayoutObjectIFace *iface)
{
	iface->get_proportion = rejilla_project_get_proportion;
}

static void
rejilla_project_get_proportion (RejillaLayoutObject *object,
				gint *header,
				gint *center,
				gint *footer)
{
	GtkAllocation allocation;

	if (!REJILLA_PROJECT (object)->priv->name_display)
		return;

	gtk_widget_get_allocation (gtk_widget_get_parent (REJILLA_PROJECT (object)->priv->name_display),
				   &allocation);
	*footer = allocation.height;
}

static void
rejilla_project_set_remove_button_state (RejillaProject *project)
{
	GtkAction *action;
	gboolean sensitive;

	sensitive = (project->priv->has_focus &&
	             project->priv->selected_uris);

	action = gtk_action_group_get_action (project->priv->project_group, "DeleteProject");
	gtk_action_set_sensitive (action, sensitive);
}

static void
rejilla_project_set_add_button_state (RejillaProject *project)
{
	GtkAction *action;
	GtkWidget *widget;
	gboolean sensitive;
	GtkWidget *toplevel;

	sensitive = ((!project->priv->current_source || !project->priv->has_focus) &&
		      !project->priv->oversized && !project->priv->chooser);

	action = gtk_action_group_get_action (project->priv->project_group, "Add");
	gtk_action_set_sensitive (action, sensitive);

	/* set the Add button to be the default for the whole window. That fixes 
	 * #465175 – Location field not working. GtkFileChooser needs a default
	 * widget to be activated. */
	toplevel = gtk_widget_get_toplevel (GTK_WIDGET (project));
	if (!sensitive) {
		gtk_window_set_default (GTK_WINDOW (toplevel), NULL);
		return;
	}

	widget = gtk_ui_manager_get_widget (project->priv->manager, "/Toolbar/Add");
	if (!widget)
		return;

	widget = gtk_bin_get_child (GTK_BIN (widget));
	gtk_widget_set_can_default (widget, TRUE);

	gtk_window_set_default (GTK_WINDOW (toplevel), widget);
}

static void
rejilla_project_focus_changed_cb (GtkContainer *container,
				  GtkWidget *widget,
				  gpointer NULL_data)
{
	RejillaProject *project;

	project = REJILLA_PROJECT (container);
	project->priv->has_focus = (widget != NULL);

	rejilla_project_set_remove_button_state (project);
	rejilla_project_set_add_button_state (project);
}

static void
rejilla_project_name_changed_cb (RejillaProjectName *name,
				 RejillaProject *project)
{
	GtkAction *action;

	project->priv->modified = TRUE;

	/* the state of the following depends on the existence of an opened project */
	action = gtk_action_group_get_action (project->priv->project_group, "Save");
	if (project->priv->modified)
		gtk_action_set_sensitive (action, TRUE);
	else
		gtk_action_set_sensitive (action, FALSE);
}

/********************************** help ***************************************/
static GtkWidget *
rejilla_utils_disc_find_tree_view_in_container (GtkContainer *container)
{
	GList *children;
	GList *iter;

	children = gtk_container_get_children (container);
	for (iter = children; iter; iter = iter->next) {
		GtkWidget *widget;

		widget = iter->data;
		if (GTK_IS_TREE_VIEW (widget)) {
			g_list_free (children);
			return widget;
		}

		if (GTK_IS_CONTAINER (widget)) {
			widget = rejilla_utils_disc_find_tree_view_in_container (GTK_CONTAINER (widget));
			if (widget) {
				g_list_free (children);
				return widget;
			}
		}
	}
	g_list_free (children);

	return NULL;
}

static GtkWidget *
rejilla_utils_disc_find_tree_view (RejillaDisc *widget)
{
	return rejilla_utils_disc_find_tree_view_in_container (GTK_CONTAINER (widget));
}

static void
rejilla_utils_disc_hide_use_info_leave_cb (GtkWidget *widget,
					   GdkDragContext *drag_context,
					   guint time,
					   RejillaProject *project)
{
	GtkWidget *other_widget;

	other_widget = rejilla_utils_disc_find_tree_view (project->priv->current);
	if (!other_widget)
		return;

	g_signal_emit_by_name (other_widget,
			       "drag-leave",
			       drag_context,
			       time);
}

static gboolean
rejilla_utils_disc_hide_use_info_drop_cb (GtkWidget *widget,
					  GdkDragContext *drag_context,
					  gint x,
					  gint y,
					  guint time,
					  RejillaProject *project)
{
	GdkAtom target = GDK_NONE;
	GtkWidget *other_widget;

	/* here the treeview is not realized so we'll have a warning message
	 * if we ever try to forward the event */
	other_widget = rejilla_utils_disc_find_tree_view (project->priv->current);
	if (!other_widget)
		return FALSE;

	target = gtk_drag_dest_find_target (other_widget,
					    drag_context,
					    gtk_drag_dest_get_target_list (GTK_WIDGET (other_widget)));

	if (target != GDK_NONE) {
		gboolean return_value = FALSE;

		/* It's necessary to make sure the treeview
		 * is realized already before sending the
		 * signal */
		gtk_widget_realize (other_widget);

		/* The widget must be realized to receive such events. */
		g_signal_emit_by_name (other_widget,
				       "drag-drop",
				       drag_context,
				       x,
				       y,
				       time,
				       &return_value);
		return return_value;
	}

	return FALSE;
}

static void
rejilla_utils_disc_hide_use_info_data_received_cb (GtkWidget *widget,
						   GdkDragContext *drag_context,
						   gint x,
						   gint y,
						   GtkSelectionData *data,
						   guint info,
						   guint time,
						   RejillaProject *project)
{
	GtkWidget *other_widget;

	g_return_if_fail(REJILLA_IS_PROJECT(project));

	other_widget = rejilla_utils_disc_find_tree_view (project->priv->current);
	if (!other_widget)
		return;

	g_signal_emit_by_name (other_widget,
			       "drag-data-received",
			       drag_context,
			       x,
			       y,
			       data,
			       info,
			       time);
}

static gboolean
rejilla_utils_disc_hide_use_info_motion_cb (GtkWidget *widget,
					    GdkDragContext *drag_context,
					    gint x,
					    gint y,
					    guint time,
					    GtkNotebook *notebook)
{
	return TRUE;
}

static gboolean
rejilla_utils_disc_hide_use_info_button_cb (GtkWidget *widget,
					    GdkEventButton *event,
					    RejillaProject *project)
{
	GtkWidget *other_widget;
	gboolean result;

	if (event->button != 3)
		return TRUE;

	other_widget = rejilla_utils_disc_find_tree_view (project->priv->current);
	if (!other_widget)
		return TRUE;

	g_signal_emit_by_name (other_widget,
			       "button-press-event",
			       event,
			       &result);

	return result;
}

static void
rejilla_utils_disc_style_changed_cb (GtkWidget *widget,
				     GtkStyle *previous,
				     GtkWidget *event_box)
{
	/* The widget (a treeview here) needs to be realized to get proper style */
	gtk_widget_realize (widget);
	gtk_widget_modify_bg (event_box, GTK_STATE_NORMAL, &gtk_widget_get_style (widget)->base[GTK_STATE_NORMAL]);
}

static void
rejilla_utils_disc_realized_cb (GtkWidget *event_box,
				GtkWidget *textview)
{
	/* The widget (a treeview here) needs to be realized to get proper style */
	gtk_widget_realize (textview);
	gtk_widget_modify_bg (event_box, GTK_STATE_NORMAL, &gtk_widget_get_style (textview)->base[GTK_STATE_NORMAL]);

	g_signal_handlers_disconnect_by_func (textview,
					      rejilla_utils_disc_style_changed_cb,
					      event_box);
	g_signal_connect (textview,
			  "style-set",
			  G_CALLBACK (rejilla_utils_disc_style_changed_cb),
			  event_box);
}

static GtkWidget *
rejilla_disc_get_use_info_notebook (RejillaProject *project)
{
	GList *chain;
	GtkTextIter iter;
	GtkWidget *frame;
	GtkWidget *textview;
	GtkWidget *notebook;
	GtkWidget *alignment;
	GtkTextBuffer *buffer;
	GtkWidget *event_box;

	notebook = gtk_notebook_new ();
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (notebook), FALSE);
	gtk_notebook_set_show_border (GTK_NOTEBOOK (notebook), FALSE);

	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
				  frame,
				  NULL);

	/* Now this event box must be 'transparent' to have the same background 
	 * color as a treeview */
	event_box = gtk_event_box_new ();
	gtk_event_box_set_visible_window (GTK_EVENT_BOX (event_box), TRUE);
	gtk_event_box_set_above_child (GTK_EVENT_BOX (event_box), TRUE);
	gtk_drag_dest_set (event_box, 
			   GTK_DEST_DEFAULT_MOTION,
			   ntables_cd,
			   nb_targets_cd,
			   GDK_ACTION_COPY|
			   GDK_ACTION_MOVE);

	/* the following signals need to be forwarded to the widget underneath */
	g_signal_connect (event_box,
			  "drag-motion",
			  G_CALLBACK (rejilla_utils_disc_hide_use_info_motion_cb),
			  project);
	g_signal_connect (event_box,
			  "drag-leave",
			  G_CALLBACK (rejilla_utils_disc_hide_use_info_leave_cb),
			  project);
	g_signal_connect (event_box,
			  "drag-drop",
			  G_CALLBACK (rejilla_utils_disc_hide_use_info_drop_cb),
			  project);
	g_signal_connect (event_box,
			  "button-press-event",
			  G_CALLBACK (rejilla_utils_disc_hide_use_info_button_cb),
			  project);
	g_signal_connect (event_box,
			  "drag-data-received",
			  G_CALLBACK (rejilla_utils_disc_hide_use_info_data_received_cb),
			  project);

	gtk_container_add (GTK_CONTAINER (frame), event_box);

	/* The alignment to set properly the position of the GtkTextView */
	alignment = gtk_alignment_new (0.5, 0.5, 1.0, 0.0);
	gtk_container_set_border_width (GTK_CONTAINER (alignment), 10);
	gtk_widget_show (alignment);
	gtk_container_add (GTK_CONTAINER (event_box), alignment);

	/* The TreeView for the message */
	buffer = gtk_text_buffer_new (NULL);
	gtk_text_buffer_create_tag (buffer, "Title",
	                            "scale", 1.1,
	                            "justification", GTK_JUSTIFY_CENTER,
	                            "foreground", "grey50",
	                            "wrap-mode", GTK_WRAP_WORD,
	                            NULL);

	gtk_text_buffer_get_start_iter (buffer, &iter);
	gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, _("To add files to this project click the \"Add\" button or drag files to this area"), -1, "Title", NULL);
	gtk_text_buffer_insert (buffer, &iter, "\n\n\n", -1);
	gtk_text_buffer_insert_with_tags_by_name (buffer, &iter, _("To remove files select them then click on the \"Remove\" button or press \"Delete\" key"), -1, "Title", NULL);

	textview = gtk_text_view_new_with_buffer (buffer);
	gtk_text_view_set_editable (GTK_TEXT_VIEW (textview), FALSE);
	gtk_text_view_set_cursor_visible (GTK_TEXT_VIEW (textview), FALSE);

	gtk_drag_dest_set (textview, 
			   GTK_DEST_DEFAULT_MOTION,
			   ntables_cd,
			   nb_targets_cd,
			   GDK_ACTION_COPY|
			   GDK_ACTION_MOVE);

	/* the following signals need to be forwarded to the widget underneath */
	g_signal_connect (textview,
			  "drag-motion",
			  G_CALLBACK (rejilla_utils_disc_hide_use_info_motion_cb),
			  project);
	g_signal_connect (textview,
			  "drag-leave",
			  G_CALLBACK (rejilla_utils_disc_hide_use_info_leave_cb),
			  project);
	g_signal_connect (textview,
			  "drag-drop",
			  G_CALLBACK (rejilla_utils_disc_hide_use_info_drop_cb),
			  project);
	g_signal_connect (textview,
			  "button-press-event",
			  G_CALLBACK (rejilla_utils_disc_hide_use_info_button_cb),
			  project);
	g_signal_connect (textview,
			  "drag-data-received",
			  G_CALLBACK (rejilla_utils_disc_hide_use_info_data_received_cb),
			  project);

	gtk_container_add (GTK_CONTAINER (alignment), textview);

	g_signal_connect (event_box,
			  "realize",
			  G_CALLBACK (rejilla_utils_disc_realized_cb),
			  project);

	chain = g_list_prepend (NULL, event_box);
	gtk_container_set_focus_chain (GTK_CONTAINER (frame), chain);
	g_list_free (chain);

	chain = g_list_prepend (NULL, alignment);
	gtk_container_set_focus_chain (GTK_CONTAINER (event_box), chain);
	g_list_free (chain);

	chain = g_list_prepend (NULL, textview);
	gtk_container_set_focus_chain (GTK_CONTAINER (alignment), chain);
	g_list_free (chain);

	gtk_widget_show_all (notebook);
	return notebook;
}

/********************************** size ***************************************/
static gchar *
rejilla_project_get_sectors_string (gint64 sectors,
				    RejillaTrackType *type)
{
	gint64 size_bytes;

	if (rejilla_track_type_get_has_stream (type)) {
		if (REJILLA_STREAM_FORMAT_HAS_VIDEO (rejilla_track_type_get_stream_format (type)))
			/* This is an embarassing problem: this is an approximation
			 * based on the fact that 2 hours = 4.3GiB */
			size_bytes = sectors * 2048LL * 72000LL / 47LL;
		else
			size_bytes = sectors * GST_SECOND / 75LL;
		return rejilla_units_get_time_string (size_bytes, TRUE, FALSE);
	}
	else {
		size_bytes = sectors * 2048LL;
		return g_format_size_for_display (size_bytes);
	}
}

static void
rejilla_project_update_project_size (RejillaProject *project)
{
	RejillaTrackType *session_type;
	goffset sectors = 0;
	GtkWidget *status;
	gchar *size_str;
	gchar *string;

	status = rejilla_app_get_statusbar2 (rejilla_app_get_default ());

	if (!project->priv->status_ctx)
		project->priv->status_ctx = gtk_statusbar_get_context_id (GTK_STATUSBAR (status),
									  "size_project");

	gtk_statusbar_pop (GTK_STATUSBAR (status), project->priv->status_ctx);

	rejilla_burn_session_get_size (REJILLA_BURN_SESSION (project->priv->session),
				       &sectors,
				       NULL);

	session_type = rejilla_track_type_new ();
	rejilla_burn_session_get_input_type (REJILLA_BURN_SESSION (project->priv->session), session_type);

	string = rejilla_project_get_sectors_string (sectors, session_type);
	rejilla_track_type_free (session_type);

	size_str = g_strdup_printf (_("Estimated project size: %s"), string);
	g_free (string);

	gtk_statusbar_push (GTK_STATUSBAR (status), project->priv->status_ctx, size_str);
	g_free (size_str);
}

static void
rejilla_project_update_controls (RejillaProject *project)
{
	GtkAction *action;

	rejilla_project_set_remove_button_state (project);
	rejilla_project_set_add_button_state (project);

	action = gtk_action_group_get_action (project->priv->project_group, "DeleteAll");
	gtk_action_set_sensitive (action, (rejilla_disc_is_empty (REJILLA_DISC (project->priv->current))));
}

static void
rejilla_project_modified (RejillaProject *project)
{
	GtkAction *action;

	rejilla_project_update_controls (project);
	rejilla_project_update_project_size (project);

	/* the state of the following depends on the existence of an opened project */
	action = gtk_action_group_get_action (project->priv->project_group, "Save");
	gtk_action_set_sensitive (action, TRUE);
	project->priv->modified = TRUE;
}

static void
rejilla_project_track_removed (RejillaBurnSession *session,
			       RejillaTrack *track,
			       guint former_position,
			       RejillaProject *project)
{
	rejilla_project_modified (project);
}

static void
rejilla_project_track_changed (RejillaBurnSession *session,
			       RejillaTrack *track,
			       RejillaProject *project)
{
	rejilla_project_modified (project);
}

static void
rejilla_project_track_added (RejillaBurnSession *session,
			     RejillaTrack *track,
			     RejillaProject *project)
{
	rejilla_project_modified (project);
}

static void
rejilla_project_message_response_span_cb (RejillaDiscMessage *message,
					  GtkResponseType response,
					  RejillaProject *project)
{
	if (response == GTK_RESPONSE_OK)
		rejilla_session_span_start (REJILLA_SESSION_SPAN (project->priv->session));
}

static void
rejilla_project_message_response_overburn_cb (RejillaDiscMessage *message,
					      GtkResponseType response,
					      RejillaProject *project)
{
	if (response == GTK_RESPONSE_OK)
		rejilla_session_cfg_add_flags (project->priv->session, REJILLA_BURN_FLAG_OVERBURN);
}

static void
rejilla_project_is_valid (RejillaSessionCfg *session,
			  RejillaProject *project)
{
	RejillaSessionError valid;
	GdkWindow *window;
	GdkCursor *cursor;
	GtkAction *action;

	/* Update the cursor */
	window = gtk_widget_get_window (GTK_WIDGET (project));
	if (window) {
		RejillaStatus *status;

		status = rejilla_status_new ();
		rejilla_burn_session_get_status (REJILLA_BURN_SESSION (session), status);
		if (rejilla_status_get_result (status) == REJILLA_BURN_NOT_READY
		||  rejilla_status_get_result (status) == REJILLA_BURN_RUNNING) {
			cursor = gdk_cursor_new (GDK_WATCH);
			gdk_window_set_cursor (window, cursor);
			gdk_cursor_unref (cursor);
		}
		else
			gdk_window_set_cursor (window, NULL);

		g_object_unref (status);
	}

	valid = rejilla_session_cfg_get_error (project->priv->session);

	/* Update burn button state */
	action = gtk_action_group_get_action (project->priv->project_group, "Burn");
	gtk_action_set_sensitive (action, REJILLA_SESSION_IS_VALID (valid));
	gtk_widget_set_sensitive (project->priv->burn, REJILLA_SESSION_IS_VALID (valid));

	/* FIXME: update option button state as well */

	/* NOTE: empty error is the first checked error; so if another errors comes up
	 * that means that file selection is not empty */

	/* Clean any message */
	rejilla_notify_message_remove (project->priv->message,
				       REJILLA_NOTIFY_CONTEXT_SIZE);

	if (valid == REJILLA_SESSION_EMPTY) {
		project->priv->empty = TRUE;
		project->priv->oversized = FALSE;
	}
	else if (valid == REJILLA_SESSION_INSUFFICIENT_SPACE) {
		goffset min_disc_size;
		goffset available_space;

		project->priv->oversized = TRUE;
		project->priv->empty = FALSE;

		min_disc_size = rejilla_session_span_get_max_space (REJILLA_SESSION_SPAN (session));

		/* One rule should be that the maximum batch size should not exceed the disc size
		 * FIXME! we could change it into a dialog telling the user what is the maximum
		 * size required. */
		available_space = rejilla_burn_session_get_available_medium_space (REJILLA_BURN_SESSION (session));

		/* Here there is an alternative: we may be able to span the data
		 * across multiple media. So try that. */
		if (available_space > min_disc_size
		&& rejilla_session_span_possible (REJILLA_SESSION_SPAN (session)) == REJILLA_BURN_RETRY) {
			GtkWidget *message;

			message = rejilla_notify_message_add (project->priv->message,
							      _("Would you like to burn the selection of files across several media?"),
							      _("The project is too large for the disc even with the overburn option."),
							      -1,
							      REJILLA_NOTIFY_CONTEXT_SIZE);
			gtk_widget_set_tooltip_text (gtk_info_bar_add_button (GTK_INFO_BAR (message),
									      _("_Burn Several Discs"),
								    	      GTK_RESPONSE_OK),
						     _("Burn the selection of files across several media"));

			g_signal_connect (message,
					  "response",
					  G_CALLBACK (rejilla_project_message_response_span_cb),
					  project);
		}
		else
			rejilla_notify_message_add (project->priv->message,
						    _("Please choose another CD or DVD or insert a new one."),
						    _("The project is too large for the disc even with the overburn option."),
						    -1,
						    REJILLA_NOTIFY_CONTEXT_SIZE);
	}
	else if (valid == REJILLA_SESSION_OVERBURN_NECESSARY) {
		GtkWidget *message;

		project->priv->empty = FALSE;
		project->priv->oversized = TRUE;
		message = rejilla_notify_message_add (project->priv->message,
						      _("Would you like to burn beyond the disc's reported capacity?"),
						      _("The project is too large for the disc and you must remove files from it."
							"\nYou may want to use this option if you're using 90 or 100 min CD-R(W) which cannot be properly recognized and therefore needs the overburn option."
							"\nNote: This option might cause failure."),
						      -1,
						      REJILLA_NOTIFY_CONTEXT_SIZE);
		gtk_widget_set_tooltip_text (gtk_info_bar_add_button (GTK_INFO_BAR (message),
								      _("_Overburn"),
							    	      GTK_RESPONSE_OK),
					     _("Burn beyond the disc's reported capacity"));

		g_signal_connect (message,
				  "response",
				  G_CALLBACK (rejilla_project_message_response_overburn_cb),
				  project);
	}
	else if (valid == REJILLA_SESSION_NO_OUTPUT) {
		project->priv->empty = FALSE;
		rejilla_notify_message_add (project->priv->message,
					    _("Please insert a writable CD or DVD."),
					    NULL,
					    -1,
					    REJILLA_NOTIFY_CONTEXT_SIZE);
	}
	else if (valid == REJILLA_SESSION_NOT_SUPPORTED) {
		project->priv->empty = FALSE;
		rejilla_notify_message_add (project->priv->message,
					    _("Please replace the disc with a supported CD or DVD."),
					    NULL,
					    -1,
					    REJILLA_NOTIFY_CONTEXT_SIZE);
	}
	else if (valid == REJILLA_SESSION_NO_CD_TEXT) {
		rejilla_notify_message_add (project->priv->message,
					    _("No track information (artist, title, ...) will be written to the disc."),
					    _("This is not supported by the current active burning backend."),
					    -1,
					    REJILLA_NOTIFY_CONTEXT_SIZE);
	}
	else if (rejilla_burn_session_is_dest_file (REJILLA_BURN_SESSION (project->priv->session))
	     &&  rejilla_medium_selection_get_media_num (REJILLA_MEDIUM_SELECTION (project->priv->selection)) == 1) {
		/* The user may have forgotten to insert a disc so remind him of that if
		 * there aren't any other possibility in the selection */
		rejilla_notify_message_add (project->priv->message,
					    _("Please insert a writable CD or DVD if you don't want to write to an image file."),
					    NULL,
					    10000,
					    REJILLA_NOTIFY_CONTEXT_SIZE);
	}

	if (REJILLA_SESSION_IS_VALID (valid) || valid == REJILLA_SESSION_NOT_READY)
		gtk_notebook_set_current_page (GTK_NOTEBOOK (project->priv->help), 0);

	if (REJILLA_SESSION_IS_VALID (valid)) {
		project->priv->empty = FALSE;
		project->priv->oversized = FALSE;
	}

	gtk_notebook_set_current_page (GTK_NOTEBOOK (project->priv->help),
	                               rejilla_disc_is_empty (REJILLA_DISC (project->priv->current))? 0:1);
}

static void
rejilla_project_init (RejillaProject *obj)
{
	GtkSizeGroup *name_size_group;
	GtkSizeGroup *size_group;
	GtkWidget *alignment;
	GtkWidget *selector;
	GtkWidget *table;

	obj->priv = g_new0 (RejillaProjectPrivate, 1);

	g_signal_connect (G_OBJECT (obj),
			  "set-focus-child",
			  G_CALLBACK (rejilla_project_focus_changed_cb),
			  NULL);

	obj->priv->message = rejilla_notify_new ();
	gtk_box_pack_start (GTK_BOX (obj), obj->priv->message, FALSE, TRUE, 0);
	gtk_widget_show (obj->priv->message);

	/* bottom */
	table = gtk_table_new (3, 2, FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (table), 6);
	gtk_table_set_col_spacings (GTK_TABLE (table), 6);
	gtk_table_set_row_spacings (GTK_TABLE (table), 6);
	gtk_widget_show (table);
	gtk_box_pack_end (GTK_BOX (obj), table, FALSE, TRUE, 0);

	/* Media selection widget */
	name_size_group = gtk_size_group_new (GTK_SIZE_GROUP_VERTICAL);
	selector = rejilla_dest_selection_new (NULL);
	gtk_size_group_add_widget (GTK_SIZE_GROUP (name_size_group), selector);
	g_object_unref (name_size_group);

	gtk_widget_show (selector);
	obj->priv->selection = selector;

	gtk_table_attach (GTK_TABLE (table), selector,
			  0, 2,
			  1, 2,
			  GTK_FILL|GTK_EXPAND,
			  GTK_FILL|GTK_EXPAND,
			  0, 0);

	size_group = gtk_size_group_new (GTK_SIZE_GROUP_BOTH);

	obj->priv->burn = rejilla_utils_make_button (_("_Burn…"),
						     NULL,
						     "media-optical-burn",
						     GTK_ICON_SIZE_BUTTON);
	gtk_widget_show (obj->priv->burn);
	gtk_widget_set_sensitive (obj->priv->burn, FALSE);
	gtk_button_set_focus_on_click (GTK_BUTTON (obj->priv->burn), FALSE);
	g_signal_connect (obj->priv->burn,
			  "clicked",
			  G_CALLBACK (rejilla_project_burn_clicked_cb),
			  obj);
	gtk_widget_set_tooltip_text (obj->priv->burn,
				     _("Start to burn the contents of the selection"));
	gtk_size_group_add_widget (GTK_SIZE_GROUP (size_group), obj->priv->burn);

	alignment = gtk_alignment_new (1.0, 0.5, 0.0, 0.0);
	gtk_widget_show (alignment);
	gtk_container_add (GTK_CONTAINER (alignment), obj->priv->burn);
	gtk_table_attach (GTK_TABLE (table), alignment,
			  2, 3,
			  1, 2,
			  GTK_FILL,
			  GTK_EXPAND,
			  0, 0);

	/* Name widget */
	obj->priv->name_display = rejilla_project_name_new (REJILLA_BURN_SESSION (obj->priv->session));
	gtk_size_group_add_widget (GTK_SIZE_GROUP (name_size_group), obj->priv->name_display);
	gtk_widget_show (obj->priv->name_display);
	gtk_table_attach (GTK_TABLE (table), obj->priv->name_display,
			  0, 2,
			  0, 1,
			  GTK_EXPAND|GTK_FILL,
			  GTK_EXPAND|GTK_FILL,
			  0, 0);
	obj->priv->empty = 1;

	g_signal_connect (obj->priv->name_display,
			  "name-changed",
			  G_CALLBACK (rejilla_project_name_changed_cb),
			  obj);

	/* The three panes to put into the notebook */
	obj->priv->audio = rejilla_audio_disc_new ();
	gtk_widget_show (obj->priv->audio);
	g_signal_connect (G_OBJECT (obj->priv->audio),
			  "selection-changed",
			  G_CALLBACK (rejilla_project_selection_changed_cb),
			  obj);

	obj->priv->data = rejilla_data_disc_new ();
	gtk_widget_show (obj->priv->data);
	rejilla_data_disc_set_right_button_group (REJILLA_DATA_DISC (obj->priv->data), size_group);
	g_signal_connect (G_OBJECT (obj->priv->data),
			  "selection-changed",
			  G_CALLBACK (rejilla_project_selection_changed_cb),
			  obj);

	obj->priv->video = rejilla_video_disc_new ();
	gtk_widget_show (obj->priv->video);
	g_signal_connect (G_OBJECT (obj->priv->video),
			  "selection-changed",
			  G_CALLBACK (rejilla_project_selection_changed_cb),
			  obj);

	obj->priv->help = rejilla_disc_get_use_info_notebook (obj);
	gtk_widget_show (obj->priv->help);

	obj->priv->discs = gtk_notebook_new ();
	gtk_widget_show (obj->priv->discs);
	gtk_notebook_set_show_border (GTK_NOTEBOOK (obj->priv->discs), FALSE);
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (obj->priv->discs), FALSE);

	gtk_notebook_prepend_page (GTK_NOTEBOOK (obj->priv->discs),
				   obj->priv->video, NULL);
	gtk_notebook_prepend_page (GTK_NOTEBOOK (obj->priv->discs),
				   obj->priv->data, NULL);
	gtk_notebook_prepend_page (GTK_NOTEBOOK (obj->priv->discs),
				   obj->priv->audio, NULL);

	gtk_notebook_prepend_page (GTK_NOTEBOOK (obj->priv->help),
				   obj->priv->discs, NULL);

	gtk_box_pack_start (GTK_BOX (obj),
			    obj->priv->help,
			    TRUE,
			    TRUE,
			    0);

	g_object_unref (size_group);
}

static void
rejilla_project_finalize (GObject *object)
{
	RejillaProject *cobj;
	cobj = REJILLA_PROJECT(object);

	if (cobj->priv->cancel) {
		g_cancellable_cancel (cobj->priv->cancel);
		cobj->priv->cancel = NULL;
	}

	if (cobj->priv->session) {
		g_object_unref (cobj->priv->session);
		cobj->priv->session = NULL;
	}

	if (cobj->priv->project)
		g_free (cobj->priv->project);

	if (cobj->priv->cover)
		g_free (cobj->priv->cover);

	g_free (cobj->priv);
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

GtkWidget *
rejilla_project_new ()
{
	RejillaProject *obj;
	
	obj = REJILLA_PROJECT (g_object_new (REJILLA_TYPE_PROJECT, NULL));
	gtk_notebook_set_current_page (GTK_NOTEBOOK (obj->priv->discs), 0);

	return GTK_WIDGET (obj);
}

/***************************** URIContainer ************************************/
static void
rejilla_project_selection_changed_cb (RejillaDisc *disc,
				      RejillaProject *project)
{
	project->priv->selected_uris = rejilla_disc_get_selected_uri (project->priv->current, NULL);
	rejilla_project_set_remove_button_state (project);
	rejilla_uri_container_uri_selected (REJILLA_URI_CONTAINER (project));
}

static gchar *
rejilla_project_get_selected_uri (RejillaURIContainer *container)
{
	RejillaProject *project;
	gchar *uri = NULL;

	project = REJILLA_PROJECT (container);

	/* if we are burning we better not return anything so as to stop 
	 * preview widget from carrying on to play */
	if (project->priv->is_burning)
		return NULL;

	if (rejilla_disc_get_selected_uri (project->priv->current, &uri))
		return uri;

	return NULL;
}

static gboolean
rejilla_project_get_boundaries (RejillaURIContainer *container,
				gint64 *start,
				gint64 *end)
{
	RejillaProject *project;

	project = REJILLA_PROJECT (container);

	/* if we are burning we better not return anything so as to stop 
	 * preview widget from carrying on to play */
	if (project->priv->is_burning)
		return FALSE;

	return rejilla_disc_get_boundaries (project->priv->current,
					    start,
					    end);
}

static void
rejilla_project_no_song_dialog (RejillaProject *project)
{
	rejilla_app_alert (rejilla_app_get_default (),
			   _("Please add songs to the project."),
			   _("There are no songs to write to disc"),
			   GTK_MESSAGE_WARNING);
}

static void
rejilla_project_no_file_dialog (RejillaProject *project)
{
	rejilla_app_alert (rejilla_app_get_default (),
			   _("Please add files to the project."),
			   _("There are no files to write to disc"),
			   GTK_MESSAGE_WARNING);
}

static RejillaBurnResult
rejilla_project_check_status (RejillaProject *project)
{
        GtkWidget *dialog;
        RejillaStatus *status;
	GtkResponseType response;
	RejillaBurnResult result;

        status = rejilla_status_new ();
        rejilla_burn_session_get_status (REJILLA_BURN_SESSION (project->priv->session), status);
        result = rejilla_status_get_result (status);
        g_object_unref (status);

        if (result == REJILLA_BURN_ERR) {
                /* At the moment the only error possible is an empty project */
		if (REJILLA_IS_AUDIO_DISC (project->priv->current))
			rejilla_project_no_song_dialog (project);
		else
			rejilla_project_no_file_dialog (project);

		return REJILLA_BURN_ERR;
	}

	if (result == REJILLA_BURN_OK)
		return REJILLA_BURN_OK;

        dialog = rejilla_status_dialog_new (REJILLA_BURN_SESSION (project->priv->session),
                                            gtk_widget_get_toplevel (GTK_WIDGET (project)));

	gtk_widget_show (dialog);
        response = gtk_dialog_run (GTK_DIALOG (dialog));
        gtk_widget_destroy (dialog);

        return (response == GTK_RESPONSE_OK)? REJILLA_BURN_OK:REJILLA_BURN_CANCEL;
}

static RejillaBurnResult
rejilla_project_install_missing (RejillaPluginErrorType type,
                                 const gchar *detail,
                                 gpointer user_data)
{
	RejillaProject *project = REJILLA_PROJECT (user_data);
	GCancellable *cancel;
	RejillaPK *package;
	GtkWidget *parent;
	gboolean res;
	int xid = 0;

	/* Get the xid */
	parent = gtk_widget_get_toplevel (GTK_WIDGET (project));
	xid = gdk_x11_drawable_get_xid (GDK_DRAWABLE (gtk_widget_get_window (parent)));

	package = rejilla_pk_new ();
	cancel = g_cancellable_new ();
	project->priv->cancel = cancel;
	switch (type) {
		case REJILLA_PLUGIN_ERROR_MISSING_APP:
			res = rejilla_pk_install_missing_app (package, detail, xid, cancel);
			break;

		case REJILLA_PLUGIN_ERROR_MISSING_LIBRARY:
			res = rejilla_pk_install_missing_library (package, detail, xid, cancel);
			break;

		case REJILLA_PLUGIN_ERROR_MISSING_GSTREAMER_PLUGIN:
			res = rejilla_pk_install_gstreamer_plugin (package, detail, xid, cancel);
			break;

		default:
			res = FALSE;
			break;
	}

	if (package) {
		g_object_unref (package);
		package = NULL;
	}

	if (g_cancellable_is_cancelled (cancel)) {
		g_object_unref (cancel);
		return REJILLA_BURN_CANCEL;
	}

	project->priv->cancel = NULL;
	g_object_unref (cancel);

	if (!res)
		return REJILLA_BURN_ERR;

	return REJILLA_BURN_RETRY;
}

static RejillaBurnResult
rejilla_project_list_missing (RejillaPluginErrorType type,
                              const gchar *detail,
                              gpointer user_data)
{
	GString *string = user_data;

	if (type == REJILLA_PLUGIN_ERROR_MISSING_APP ||
	    type == REJILLA_PLUGIN_ERROR_SYMBOLIC_LINK_APP ||
	    type == REJILLA_PLUGIN_ERROR_WRONG_APP_VERSION) {
		g_string_append_c (string, '\n');
		/* Translators: %s is the name of a missing application */
		g_string_append_printf (string, _("%s (application)"), detail);
	}
	else if (type == REJILLA_PLUGIN_ERROR_MISSING_LIBRARY ||
	         type == REJILLA_PLUGIN_ERROR_LIBRARY_VERSION) {
		g_string_append_c (string, '\n');
		/* Translators: %s is the name of a missing library */
		g_string_append_printf (string, _("%s (library)"), detail);
	}
	else if (type == REJILLA_PLUGIN_ERROR_MISSING_GSTREAMER_PLUGIN) {
		g_string_append_c (string, '\n');
		/* Translators: %s is the name of a missing GStreamer plugin */
		g_string_append_printf (string, _("%s (GStreamer plugin)"), detail);
	}

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_project_check_plugins_not_ready (RejillaProject *project,
                                         RejillaBurnSession *session)
{
	RejillaBurnResult result;
	GtkWidget *parent;
	GString *string;

	parent = gtk_widget_get_toplevel (GTK_WIDGET (project));
	gtk_widget_set_sensitive (parent, FALSE);

	rejilla_burn_session_set_strict_support (REJILLA_BURN_SESSION (session), TRUE);
	result = rejilla_burn_session_can_burn (session, FALSE);
	rejilla_burn_session_set_strict_support (REJILLA_BURN_SESSION (session), FALSE);

	if (result == REJILLA_BURN_OK) {
		gtk_widget_set_sensitive (parent, TRUE);
		return result;
	}

	result = rejilla_burn_session_can_burn (session, FALSE);
	if (result != REJILLA_BURN_OK) {
		gtk_widget_set_sensitive (parent, TRUE);
		return result;
	}

	result = rejilla_session_foreach_plugin_error (session,
	                                               rejilla_project_install_missing,
	                                               project);
	gtk_widget_set_sensitive (parent, TRUE);

	if (result == REJILLA_BURN_CANCEL)
		return result;

	if (result == REJILLA_BURN_OK)
		return result;

	string = g_string_new (_("Please install the following manually and try again:"));
	rejilla_session_foreach_plugin_error (session,
	                                      rejilla_project_list_missing,
	                                      string);

	rejilla_utils_message_dialog (parent,
	                              _("All required applications and libraries are not installed."),
	                              string->str,
	                              GTK_MESSAGE_ERROR);
	g_string_free (string, TRUE);

	return REJILLA_BURN_ERR;
}

/******************************** burning **************************************/
static void
rejilla_project_setup_session (RejillaProject *project,
			       RejillaBurnSession *session)
{
	const gchar *label;

	label = gtk_entry_get_text (GTK_ENTRY (project->priv->name_display));
	rejilla_burn_session_set_label (session, label);

	if (project->priv->cover) {
		GValue *value;

		value = g_new0 (GValue, 1);
		g_value_init (value, G_TYPE_STRING);
		g_value_set_string (value, project->priv->cover);
		rejilla_burn_session_tag_add (session,
					      REJILLA_COVER_URI,
					      value);
	}
}

static void
rejilla_project_output_changed (RejillaBurnSession *session,
                                RejillaMedium *former_medium,
                                GtkDialog *dialog)
{
	gtk_dialog_response (dialog, GTK_RESPONSE_CANCEL);
}

static RejillaBurnResult
rejilla_project_drive_properties (RejillaProject *project)
{
	RejillaTrackType *track_type;
	GtkWidget *medium_prop;
	GtkResponseType answer;
	RejillaDrive *drive;
	gchar *display_name;
	GtkWidget *options;
	GtkWidget *button;
	GtkWidget *dialog;
	glong cancel_sig;
	GtkWidget *box;
	gchar *header;
	gchar *string;

	/* Build dialog */
	drive = rejilla_burn_session_get_burner (REJILLA_BURN_SESSION (project->priv->session));

	display_name = rejilla_drive_get_display_name (drive);
	header = g_strdup_printf (_("Properties of %s"), display_name);
	g_free (display_name);

	dialog = gtk_dialog_new_with_buttons (header,
					      NULL,
					      GTK_DIALOG_MODAL|
					      GTK_DIALOG_DESTROY_WITH_PARENT,
					      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					      NULL);
	g_free (header);

	/* This is in case the medium gets ejected instead of our locking it */
	cancel_sig = g_signal_connect (project->priv->session,
	                               "output-changed",
	                               G_CALLBACK (rejilla_project_output_changed),
	                               dialog);

	gtk_dialog_add_button (GTK_DIALOG (dialog),
			       _("Burn _Several Copies"),
			       GTK_RESPONSE_ACCEPT);

	button = rejilla_utils_make_button (_("_Burn"),
					    NULL,
					    "media-optical-burn",
					    GTK_ICON_SIZE_BUTTON);
	gtk_widget_show (button);
	gtk_dialog_add_action_widget (GTK_DIALOG (dialog),
				      button,
				      GTK_RESPONSE_OK);

	box = gtk_dialog_get_content_area (GTK_DIALOG (dialog));

	track_type = rejilla_track_type_new ();

	rejilla_burn_session_get_input_type (REJILLA_BURN_SESSION (project->priv->session), track_type);
	if (rejilla_track_type_get_has_stream (track_type)
	&& REJILLA_STREAM_FORMAT_HAS_VIDEO (rejilla_track_type_get_stream_format (track_type))) {
		/* Special case for video project */
		options = rejilla_video_options_new (REJILLA_BURN_SESSION (project->priv->session));
		gtk_widget_show (options);

		string = g_strdup_printf ("<b>%s</b>", _("Video Options"));
		options = rejilla_utils_pack_properties (string,
							 options,
							 NULL);
		g_free (string);
		gtk_box_pack_start (GTK_BOX (box), options, FALSE, TRUE, 0);
	}

	rejilla_track_type_free (track_type);

	medium_prop = rejilla_drive_properties_new (project->priv->session);
	gtk_box_pack_start (GTK_BOX (box), medium_prop, TRUE, TRUE, 0);
	gtk_widget_show (medium_prop);

	rejilla_app_set_toplevel (rejilla_app_get_default (), GTK_WINDOW (dialog));

	/* launch the dialog */
	answer = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);

	g_signal_handler_disconnect (project->priv->session, cancel_sig);

	if (answer == GTK_RESPONSE_OK)
		return REJILLA_BURN_OK;

	if (answer == GTK_RESPONSE_ACCEPT)
		return REJILLA_BURN_RETRY;

	return REJILLA_BURN_CANCEL;
}

static gboolean
rejilla_project_image_properties (RejillaProject *project)
{
	RejillaTrackType *track_type;
	GtkResponseType answer;
	GtkWidget *button;
	GtkWidget *dialog;

	/* Build dialog */
	dialog = rejilla_image_properties_new ();

	rejilla_app_set_toplevel (rejilla_app_get_default (), GTK_WINDOW (dialog));
	gtk_window_set_destroy_with_parent (GTK_WINDOW (dialog), TRUE);
	gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER_ON_PARENT);

	gtk_dialog_add_button (GTK_DIALOG (dialog),
			       GTK_STOCK_CANCEL,
			       GTK_RESPONSE_CANCEL);

	button = gtk_dialog_add_button (GTK_DIALOG (dialog),
					_("Create _Image"),
				       GTK_RESPONSE_OK);
	gtk_button_set_image (GTK_BUTTON (button), gtk_image_new_from_icon_name ("iso-image-new", GTK_ICON_SIZE_BUTTON));

	rejilla_image_properties_set_session (REJILLA_IMAGE_PROPERTIES (dialog), project->priv->session);

	track_type = rejilla_track_type_new ();

	rejilla_burn_session_get_input_type (REJILLA_BURN_SESSION (project->priv->session), track_type);
	if (rejilla_track_type_get_has_stream (track_type)
	&& REJILLA_STREAM_FORMAT_HAS_VIDEO (rejilla_track_type_get_stream_format (track_type))) {
		GtkWidget *box;
		GtkWidget *options;

		/* create video widget */
		options = rejilla_video_options_new (REJILLA_BURN_SESSION (project->priv->session));
		gtk_widget_show (options);

		box = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
		gtk_box_pack_end (GTK_BOX (box), options, FALSE, TRUE, 0);
	}

	rejilla_track_type_free (track_type);

	gtk_dialog_set_default_response (GTK_DIALOG (dialog), GTK_RESPONSE_OK);

	/* launch the dialog */
	answer = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);

	return (answer == GTK_RESPONSE_OK) ? REJILLA_BURN_OK:REJILLA_BURN_ERR;
}

static void
rejilla_project_burn (RejillaProject *project)
{
	RejillaBurnResult res;
	RejillaDisc *current_disc;
	RejillaDriveSettings *settings;

	/* Check that we are ready */
	if (rejilla_project_check_status (project) != REJILLA_BURN_OK)
		return;

	/* Check that we are not missing any plugin */
	if (rejilla_project_check_plugins_not_ready (project, REJILLA_BURN_SESSION (project->priv->session)) != REJILLA_BURN_OK)
		return;

	/* Set saved parameters for the session */
	settings = rejilla_drive_settings_new ();
	rejilla_drive_settings_set_session (settings, REJILLA_BURN_SESSION (project->priv->session));

	if (!rejilla_burn_session_is_dest_file (REJILLA_BURN_SESSION (project->priv->session)))
		res = rejilla_project_drive_properties (project);
	else
		res = rejilla_project_image_properties (project);

	if (res != REJILLA_BURN_OK
	&&  res != REJILLA_BURN_RETRY) {
		g_object_unref (settings);
		return;
	}

	project->priv->is_burning = 1;

	/* This is to avoid having the settings being wrongly reflected or changed */
	current_disc = project->priv->current;
	rejilla_disc_set_session_contents (current_disc, NULL);
	project->priv->current = NULL;

	rejilla_dest_selection_set_session (REJILLA_DEST_SELECTION (project->priv->selection), NULL);
	rejilla_project_setup_session (project, REJILLA_BURN_SESSION (project->priv->session));

	/* This is to stop the preview widget from playing */
	rejilla_uri_container_uri_selected (REJILLA_URI_CONTAINER (project));

	/* now setup the burn dialog */
	project->priv->burnt = rejilla_app_burn (rejilla_app_get_default (),
						 REJILLA_BURN_SESSION (project->priv->session),
						 res == REJILLA_BURN_RETRY);

	g_object_unref (settings);

	/* empty the stack of temporary tracks */
	while (rejilla_burn_session_pop_tracks (REJILLA_BURN_SESSION (project->priv->session)) == REJILLA_BURN_RETRY);

	project->priv->current = current_disc;
	rejilla_disc_set_session_contents (current_disc, REJILLA_BURN_SESSION (project->priv->session));
	rejilla_dest_selection_set_session (REJILLA_DEST_SELECTION (project->priv->selection),
					    REJILLA_BURN_SESSION (project->priv->session));

	project->priv->is_burning = 0;

	rejilla_project_update_controls (project);
}

/******************************** cover ****************************************/
void
rejilla_project_create_audio_cover (RejillaProject *project)
{
	GtkWidget *window;

	if (!REJILLA_IS_AUDIO_DISC (project->priv->current))
		return;

	rejilla_project_setup_session (project, REJILLA_BURN_SESSION (project->priv->session));
	window = rejilla_session_edit_cover (REJILLA_BURN_SESSION (project->priv->session),
					     gtk_widget_get_toplevel (GTK_WIDGET (project)));

	/* This strange hack is a way to workaround #568358.
	 * At one point we'll need to hide the dialog which means it
	 * will anwer with a GTK_RESPONSE_NONE */
	while (gtk_dialog_run (GTK_DIALOG (window)) == GTK_RESPONSE_NONE)
		gtk_widget_show (GTK_WIDGET (window));

	gtk_widget_destroy (window);
}

/********************************     ******************************************/
static void
rejilla_project_reset (RejillaProject *project)
{
	gtk_notebook_set_current_page (GTK_NOTEBOOK (project->priv->help), 1);

	if (project->priv->project_status) {
		gtk_widget_hide (project->priv->project_status);
		gtk_dialog_response (GTK_DIALOG (project->priv->project_status), GTK_RESPONSE_CANCEL);
		project->priv->project_status = NULL;
	}

	if (project->priv->current) {
		rejilla_disc_set_session_contents (project->priv->current, NULL);
		project->priv->current = NULL;
	}

	if (project->priv->chooser) {
		gtk_widget_destroy (project->priv->chooser);
		project->priv->chooser = NULL;
	}

	if (project->priv->project) {
		g_free (project->priv->project);
		project->priv->project = NULL;
	}

	if (project->priv->cover) {
		g_free (project->priv->cover);
		project->priv->cover = NULL;
	}

	/* remove the buttons from the "toolbar" */
	if (project->priv->merge_id > 0)
		gtk_ui_manager_remove_ui (project->priv->manager,
					  project->priv->merge_id);

	project->priv->empty = 1;
    	project->priv->burnt = 0;
	project->priv->modified = 0;

	if (project->priv->session) {
		g_signal_handlers_disconnect_by_func (project->priv->session,
						      rejilla_project_is_valid,
						      project);
		g_signal_handlers_disconnect_by_func (project->priv->session,
						      rejilla_project_track_added,
						      project);
		g_signal_handlers_disconnect_by_func (project->priv->session,
						      rejilla_project_track_changed,
						      project);
		g_signal_handlers_disconnect_by_func (project->priv->session,
						      rejilla_project_track_removed,
						      project);

		/* unref session to force it to remove all temporary files */
		g_object_unref (project->priv->session);
		project->priv->session = NULL;
	}

	rejilla_notify_message_remove (project->priv->message, REJILLA_NOTIFY_CONTEXT_SIZE);
	rejilla_notify_message_remove (project->priv->message, REJILLA_NOTIFY_CONTEXT_LOADING);
	rejilla_notify_message_remove (project->priv->message, REJILLA_NOTIFY_CONTEXT_MULTISESSION);
}

static void
rejilla_project_new_session (RejillaProject *project,
                             RejillaSessionCfg *session)
{
	if (project->priv->session)
		rejilla_project_reset (project);

	if (session)
		project->priv->session = g_object_ref (session);
	else
		project->priv->session = rejilla_session_cfg_new ();

	rejilla_burn_session_set_strict_support (REJILLA_BURN_SESSION (project->priv->session), FALSE);

	/* NOTE: "is-valid" is emitted whenever there is a change in the
	 * contents of the session. So no need to connect to track-added, ... */
	g_signal_connect (project->priv->session,
			  "is-valid",
			  G_CALLBACK (rejilla_project_is_valid),
			  project);
	g_signal_connect (project->priv->session,
			  "track-added",
			  G_CALLBACK (rejilla_project_track_added),
			  project);
	g_signal_connect (project->priv->session,
			  "track-changed",
			  G_CALLBACK (rejilla_project_track_changed),
			  project);
	g_signal_connect (project->priv->session,
			  "track-removed",
			  G_CALLBACK (rejilla_project_track_removed),
			  project);

	rejilla_dest_selection_set_session (REJILLA_DEST_SELECTION (project->priv->selection),
					    REJILLA_BURN_SESSION (project->priv->session));
	rejilla_project_name_set_session (REJILLA_PROJECT_NAME (project->priv->name_display),
					  REJILLA_BURN_SESSION (project->priv->session));
}

static void
rejilla_project_switch (RejillaProject *project, RejillaProjectType type)
{
	GtkAction *action;

	if (type == REJILLA_PROJECT_TYPE_AUDIO) {
		project->priv->current = REJILLA_DISC (project->priv->audio);
		gtk_notebook_set_current_page (GTK_NOTEBOOK (project->priv->discs), 0);
		rejilla_medium_selection_show_media_type (REJILLA_MEDIUM_SELECTION (project->priv->selection),
							  REJILLA_MEDIA_TYPE_WRITABLE|
							  REJILLA_MEDIA_TYPE_FILE|
		                                          REJILLA_MEDIA_TYPE_CD);
	}
	else if (type == REJILLA_PROJECT_TYPE_DATA) {
		project->priv->current = REJILLA_DISC (project->priv->data);
		gtk_notebook_set_current_page (GTK_NOTEBOOK (project->priv->discs), 1);
		rejilla_medium_selection_show_media_type (REJILLA_MEDIUM_SELECTION (project->priv->selection),
							  REJILLA_MEDIA_TYPE_WRITABLE|
							  REJILLA_MEDIA_TYPE_FILE);
	}
	else if (type == REJILLA_PROJECT_TYPE_VIDEO) {
		project->priv->current = REJILLA_DISC (project->priv->video);
		gtk_notebook_set_current_page (GTK_NOTEBOOK (project->priv->discs), 2);
		rejilla_medium_selection_show_media_type (REJILLA_MEDIUM_SELECTION (project->priv->selection),
							  REJILLA_MEDIA_TYPE_WRITABLE|
							  REJILLA_MEDIA_TYPE_FILE);
	}

	if (project->priv->current) {
		project->priv->merge_id = rejilla_disc_add_ui (project->priv->current,
							       project->priv->manager,
							       project->priv->message);
		rejilla_disc_set_session_contents (project->priv->current,
						   REJILLA_BURN_SESSION (project->priv->session));
	}

	rejilla_notify_message_remove (project->priv->message, REJILLA_NOTIFY_CONTEXT_SIZE);

	/* update the menus */
	action = gtk_action_group_get_action (project->priv->project_group, "Add");
	gtk_action_set_visible (action, TRUE);
	gtk_action_set_sensitive (action, TRUE);
	action = gtk_action_group_get_action (project->priv->project_group, "DeleteProject");
	gtk_action_set_visible (action, TRUE);
	gtk_action_set_sensitive (action, FALSE);
	action = gtk_action_group_get_action (project->priv->project_group, "DeleteAll");
	gtk_action_set_visible (action, TRUE);
	gtk_action_set_sensitive (action, FALSE);
	action = gtk_action_group_get_action (project->priv->project_group, "SaveAs");
	gtk_action_set_sensitive (action, TRUE);
	action = gtk_action_group_get_action (project->priv->project_group, "Save");
	gtk_action_set_sensitive (action, FALSE);
}

void
rejilla_project_set_audio (RejillaProject *project)
{
	rejilla_project_new_session (project, NULL);
	rejilla_project_switch (project, REJILLA_PROJECT_TYPE_AUDIO);
}

void
rejilla_project_set_data (RejillaProject *project)
{
	rejilla_project_new_session (project, NULL);
	rejilla_project_switch (project, REJILLA_PROJECT_TYPE_DATA);
}

void
rejilla_project_set_video (RejillaProject *project)
{
	rejilla_project_new_session (project, NULL);
	rejilla_project_switch (project, REJILLA_PROJECT_TYPE_VIDEO);
}

RejillaBurnResult
rejilla_project_confirm_switch (RejillaProject *project,
				gboolean keep_files)
{
	GtkWidget *dialog;
	GtkResponseType answer;

	if (project->priv->project) {
		if (!project->priv->modified)
			return TRUE;

		dialog = rejilla_app_dialog (rejilla_app_get_default (),
					     _("Do you really want to create a new project and discard the current one?"),
					     GTK_BUTTONS_CANCEL,
					     GTK_MESSAGE_WARNING);

		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
							  _("If you choose to create a new empty project, all changes will be lost."));

		gtk_dialog_add_button (GTK_DIALOG (dialog),
				       _("_Discard Changes"),
				       GTK_RESPONSE_OK);
	}
	else if (keep_files) {
		if (project->priv->empty)
			return TRUE;

		dialog = rejilla_app_dialog (rejilla_app_get_default (),
					     _("Do you want to discard the file selection or add it to the new project?"),
					     GTK_BUTTONS_CANCEL,
					     GTK_MESSAGE_WARNING);

		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
							  _("If you choose to create a new empty project, the file selection will be discarded."));
		gtk_dialog_add_button (GTK_DIALOG (dialog),
				       _("_Discard File Selection"),
				       GTK_RESPONSE_OK);

		gtk_dialog_add_button (GTK_DIALOG (dialog),
				       _("_Keep File Selection"),
				       GTK_RESPONSE_ACCEPT);
	}
	else {
		if (project->priv->empty)
			return TRUE;

		dialog = rejilla_app_dialog (rejilla_app_get_default (),
					     _("Do you really want to create a new project and discard the current one?"),
					     GTK_BUTTONS_CANCEL,
					     GTK_MESSAGE_WARNING);

		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
							  _("If you choose to create a new empty project, the file selection will be discarded."));
		gtk_dialog_add_button (GTK_DIALOG (dialog),
				       _("_Discard Project"),
				       GTK_RESPONSE_OK);
	}

	answer = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);

	if (answer == GTK_RESPONSE_ACCEPT)
		return REJILLA_BURN_RETRY;

	if (answer != GTK_RESPONSE_OK)
		return REJILLA_BURN_CANCEL;

	return REJILLA_BURN_OK;
}

void
rejilla_project_set_none (RejillaProject *project)
{
	GtkAction *action;
	GtkWidget *status;

	rejilla_project_reset (project);

	/* update buttons/menus */
	action = gtk_action_group_get_action (project->priv->project_group, "Add");
	gtk_action_set_visible (action, FALSE);
	action = gtk_action_group_get_action (project->priv->project_group, "DeleteProject");
	gtk_action_set_visible (action, FALSE);
	action = gtk_action_group_get_action (project->priv->project_group, "DeleteAll");
	gtk_action_set_visible (action, FALSE);
	action = gtk_action_group_get_action (project->priv->project_group, "SaveAs");
	gtk_action_set_sensitive (action, FALSE);
	action = gtk_action_group_get_action (project->priv->project_group, "Save");
	gtk_action_set_sensitive (action, FALSE);

	if (project->priv->merge_id)
		gtk_ui_manager_remove_ui (project->priv->manager,
					  project->priv->merge_id);

	status = rejilla_app_get_statusbar2 (rejilla_app_get_default ());

	if (project->priv->status_ctx)
		gtk_statusbar_pop (GTK_STATUSBAR (status), project->priv->status_ctx);
}

/**************************** manage the relations with the sources ************/
static void
rejilla_project_transfer_uris_from_src (RejillaProject *project)
{
	gchar **uris;
	gchar **uri;

	if (!project->priv->current_source)
		return;

	uris = rejilla_uri_container_get_selected_uris (project->priv->current_source);
	if (!uris)
		return;

	uri = uris;
	while (*uri) {
		rejilla_disc_add_uri (project->priv->current, *uri);
		uri ++;
	}

	g_strfreev (uris);
}

static void
rejilla_project_source_uri_activated_cb (RejillaURIContainer *container,
					 RejillaProject *project)
{
	rejilla_project_transfer_uris_from_src (project);
}

static void
rejilla_project_source_uri_selected_cb (RejillaURIContainer *container,
					RejillaProject *project)
{
	rejilla_project_set_add_button_state (project);
}

void
rejilla_project_set_source (RejillaProject *project,
			    RejillaURIContainer *source)
{
	if (project->priv->chooser)
		gtk_dialog_response (GTK_DIALOG (project->priv->chooser), GTK_RESPONSE_CANCEL);

	if (project->priv->activated_id) {
		g_signal_handler_disconnect (project->priv->current_source,
					     project->priv->activated_id);
		project->priv->activated_id = 0;
	}

	if (project->priv->selected_id) {
		g_signal_handler_disconnect (project->priv->current_source,
					     project->priv->selected_id);
		project->priv->selected_id = 0;
	}

	project->priv->current_source = source;

	if (source) {
		project->priv->activated_id = g_signal_connect (source,
							        "uri-activated",
							        G_CALLBACK (rejilla_project_source_uri_activated_cb),
							        project);
		project->priv->selected_id = g_signal_connect (source,
							       "uri-selected",
							       G_CALLBACK (rejilla_project_source_uri_selected_cb),
							       project);
	}

	rejilla_project_set_add_button_state (project);
}

/******************************* menus/buttons *********************************/
static void
rejilla_project_save_cb (GtkAction *action, RejillaProject *project)
{
	rejilla_project_save_project (project);
}

static void
rejilla_project_save_as_cb (GtkAction *action, RejillaProject *project)
{
	rejilla_project_save_project_as (project);
}

static void
rejilla_project_file_chooser_activated_cb (GtkWidget *chooser,
					   RejillaProject *project)
{
	gboolean sensitive;
	GtkAction *action;
	GSList *uris;
	GSList *iter;

	if (!project->priv->chooser)
		return;

	project->priv->chooser = NULL;
	uris = gtk_file_chooser_get_uris (GTK_FILE_CHOOSER (chooser));
	gtk_widget_destroy (GTK_WIDGET (chooser));

	sensitive = ((!project->priv->current_source || !project->priv->has_focus) &&
		      !project->priv->oversized);

	action = gtk_action_group_get_action (project->priv->project_group, "Add");
	gtk_action_set_sensitive (action, sensitive);

	for (iter = uris; iter; iter = iter->next) {
		gchar *uri;

		uri = iter->data;
		rejilla_disc_add_uri (project->priv->current, uri);
	}
	g_slist_foreach (uris, (GFunc) g_free, NULL);
	g_slist_free (uris);
}

static void
rejilla_project_file_chooser_response_cb (GtkWidget *chooser,
					  GtkResponseType response,
					  RejillaProject *project)
{
	gboolean sensitive;
	GtkAction *action;
	GSList *uris;
	GSList *iter;

	if (!project->priv->chooser)
		return;

	sensitive = ((!project->priv->current_source || !project->priv->has_focus) &&
		      !project->priv->oversized);

	action = gtk_action_group_get_action (project->priv->project_group, "Add");
	gtk_action_set_sensitive (action, sensitive);

	if (response != REJILLA_RESPONSE_ADD) {
		gtk_widget_destroy (chooser);
		project->priv->chooser = NULL;
		return;
	}

	project->priv->chooser = NULL;
	uris = gtk_file_chooser_get_uris (GTK_FILE_CHOOSER (chooser));
	gtk_widget_destroy (GTK_WIDGET (chooser));

	for (iter = uris; iter; iter = iter->next) {
		gchar *uri;

		uri = iter->data;
		rejilla_disc_add_uri (project->priv->current, uri);
	}
	g_slist_foreach (uris, (GFunc) g_free, NULL);
	g_slist_free (uris);
}

static void
rejilla_project_preview_ready (RejillaPlayer *player,
			       GtkFileChooser *chooser)
{
	gtk_file_chooser_set_preview_widget_active (chooser, TRUE);
}

static void
rejilla_project_update_preview (GtkFileChooser *chooser,
				RejillaPlayer *player)
{
	gchar *uri;

	gtk_file_chooser_set_preview_widget_active (chooser, TRUE);

	uri = gtk_file_chooser_get_preview_uri (chooser);
	rejilla_player_set_uri (player, uri);
	g_free (uri);
}

static void
rejilla_project_add_uris_cb (GtkAction *action,
			     RejillaProject *project)
{
	GtkWidget *toplevel;
	GtkFileFilter *filter;

	if (project->priv->current_source) {
		rejilla_project_transfer_uris_from_src (project);
		return;
	}

	/* Set the Add button grey as we don't want
	 * the user to be able to click again until the
	 * dialog has been closed */
	gtk_action_set_sensitive (action, FALSE);

	toplevel = gtk_widget_get_toplevel (GTK_WIDGET (project));
	project->priv->chooser = gtk_file_chooser_dialog_new (_("Select Files"),
							      GTK_WINDOW (toplevel),
							      GTK_FILE_CHOOSER_ACTION_OPEN,
							      GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
							      NULL);
	gtk_file_chooser_set_select_multiple (GTK_FILE_CHOOSER (project->priv->chooser), TRUE);
	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (project->priv->chooser), FALSE);
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (project->priv->chooser), g_get_home_dir ());
	rejilla_file_chooser_customize (project->priv->chooser, NULL);
	gtk_widget_show (project->priv->chooser);

	/* This is to work around a bug in GTK+ which doesn't want to add "Add"
	 * button or anything that is not "Open" or "Cancel" buttons */
	/* Just for the record, file chooser creation uses all GtkResponseType
	 * that are already defined for internal use like GTK_RESPONSE_OK,
	 * *_APPLY and so on (usually to open directories, not add them). So we
	 * have to define on custom here. */
	gtk_dialog_add_button (GTK_DIALOG (project->priv->chooser),
			       GTK_STOCK_ADD,
			       REJILLA_RESPONSE_ADD);
	gtk_dialog_set_default_response (GTK_DIALOG (project->priv->chooser),
					 REJILLA_RESPONSE_ADD);

	g_signal_connect (project->priv->chooser,
			  "file-activated",
			  G_CALLBACK (rejilla_project_file_chooser_activated_cb),
			  project);
	g_signal_connect (project->priv->chooser,
			  "response",
			  G_CALLBACK (rejilla_project_file_chooser_response_cb),
			  project);
	g_signal_connect (project->priv->chooser,
			  "close",
			  G_CALLBACK (rejilla_project_file_chooser_activated_cb),
			  project);

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("All files"));
	gtk_file_filter_add_pattern (filter, "*");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (project->priv->chooser), filter);

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("Audio files"));
	gtk_file_filter_add_mime_type (filter, "audio/*");
	gtk_file_filter_add_mime_type (filter, "application/ogg");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (project->priv->chooser), filter);

	if (REJILLA_IS_AUDIO_DISC (project->priv->current))
		gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (project->priv->chooser), filter);

	filter = gtk_file_filter_new ();
	gtk_file_filter_set_name (filter, _("Movies"));
	gtk_file_filter_add_mime_type (filter, "video/*");
	gtk_file_filter_add_mime_type (filter, "application/ogg");
	gtk_file_filter_add_mime_type (filter, "application/x-flash-video");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (project->priv->chooser), filter);

	if (REJILLA_IS_VIDEO_DISC (project->priv->current))
		gtk_file_chooser_set_filter (GTK_FILE_CHOOSER (project->priv->chooser), filter);

	filter = gtk_file_filter_new ();
	/* Translators: this is an image, a picture, not a "Disc Image" */
	gtk_file_filter_set_name (filter, C_("picture", "Image files"));
	gtk_file_filter_add_mime_type (filter, "image/*");
	gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (project->priv->chooser), filter);

#ifdef BUILD_PREVIEW

	GtkWidget *player;
	gpointer value;

	rejilla_setting_get_value (rejilla_setting_get_default (),
	                           REJILLA_SETTING_SHOW_PREVIEW,
	                           &value);

	if (!GPOINTER_TO_INT (value))
		return;

	/* if preview is activated add it */
	player = rejilla_player_new ();

	gtk_widget_show (player);
	gtk_file_chooser_set_preview_widget_active (GTK_FILE_CHOOSER (project->priv->chooser), TRUE);
	gtk_file_chooser_set_use_preview_label (GTK_FILE_CHOOSER (project->priv->chooser), FALSE);
	gtk_file_chooser_set_preview_widget (GTK_FILE_CHOOSER (project->priv->chooser), player);

	g_signal_connect (project->priv->chooser,
			  "update-preview",
			  G_CALLBACK (rejilla_project_update_preview),
			  player);

	g_signal_connect (player,
			  "ready",
			  G_CALLBACK (rejilla_project_preview_ready),
			  project->priv->chooser);
#endif

}

static void
rejilla_project_remove_selected_uris_cb (GtkAction *action, RejillaProject *project)
{
	rejilla_disc_delete_selected (REJILLA_DISC (project->priv->current));
}

static void
rejilla_project_empty_cb (GtkAction *action, RejillaProject *project)
{
	if (!project->priv->empty) {
		GtkWidget *dialog;
		GtkResponseType answer;

		dialog = rejilla_app_dialog (rejilla_app_get_default (),
					      _("Do you really want to empty the current project?"),
					     GTK_BUTTONS_CANCEL,
					     GTK_MESSAGE_WARNING);

		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
							  _("Emptying a project will remove all files already added. "
							    "All the work will be lost. "
							    "Note that files will not be deleted from their own location, "
							    "just no longer listed here."));
		gtk_dialog_add_button (GTK_DIALOG (dialog),
					/* Translators: "empty" is a verb here */
				       _("E_mpty Project"),
				       GTK_RESPONSE_OK);

		answer = gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		if (answer != GTK_RESPONSE_OK)
			return;
	}

	if (!rejilla_disc_clear (REJILLA_DISC (project->priv->current)))
		rejilla_burn_session_add_track (REJILLA_BURN_SESSION (project->priv->session), NULL, NULL);
}

static void
rejilla_project_burn_cb (GtkAction *action, RejillaProject *project)
{
	rejilla_project_burn (project);
}

static void
rejilla_project_burn_clicked_cb (GtkButton *button, RejillaProject *project)
{
	rejilla_project_burn (project);
}

void
rejilla_project_register_ui (RejillaProject *project, GtkUIManager *manager)
{
	GError *error = NULL;
	GtkAction *action;

	/* menus */
	project->priv->project_group = gtk_action_group_new ("ProjectActions1");
	gtk_action_group_set_translation_domain (project->priv->project_group, GETTEXT_PACKAGE);
	gtk_action_group_add_actions (project->priv->project_group,
				      entries,
				      G_N_ELEMENTS (entries),
				      project);

	gtk_ui_manager_insert_action_group (manager, project->priv->project_group, 0);
	if (!gtk_ui_manager_add_ui_from_string (manager,
						description,
						-1,
						&error)) {
		g_message ("building menus/toolbar failed: %s", error->message);
		g_error_free (error);
	}
	
	action = gtk_action_group_get_action (project->priv->project_group, "Save");
	g_object_set (action,
		      "short-label", _("_Save"), /* for toolbar buttons */
		      NULL);
	gtk_action_set_sensitive (action, FALSE);
	action = gtk_action_group_get_action (project->priv->project_group, "SaveAs");
	gtk_action_set_sensitive (action, FALSE);

	action = gtk_action_group_get_action (project->priv->project_group, "Burn");
	gtk_action_set_sensitive (action, FALSE);
	action = gtk_action_group_get_action (project->priv->project_group, "Add");
	gtk_action_set_sensitive (action, FALSE);
	g_object_set (action,
		      "short-label", _("_Add"), /* for toolbar buttons */
		      NULL);
	action = gtk_action_group_get_action (project->priv->project_group, "DeleteProject");
	gtk_action_set_sensitive (action, FALSE);
	g_object_set (action,
		      "short-label", _("_Remove"), /* for toolbar buttons */
		      NULL);
	action = gtk_action_group_get_action (project->priv->project_group, "DeleteAll");
	gtk_action_set_sensitive (action, FALSE);

	project->priv->manager = manager;

	if (gtk_widget_get_realized (project->priv->name_display))
		gtk_widget_grab_focus (project->priv->name_display);
}

/******************************* common to save/open ***************************/
static void
rejilla_project_add_to_recents (RejillaProject *project,
				const gchar *uri,
				gboolean is_project)
{
   	GtkRecentManager *recent;
	gchar *groups [] = { "rejilla", NULL };
	gchar *open_playlist = "rejilla -l %u";
	GtkRecentData recent_data = { NULL,
				      NULL,
				      "application/x-rejilla",
				      "rejilla",
				      "rejilla -p %u",
				      groups,
				      FALSE };

    	recent = gtk_recent_manager_get_default ();

	if (is_project)
		recent_data.app_exec = open_playlist;

    	gtk_recent_manager_add_full (recent, uri, &recent_data);
}

static void
rejilla_project_set_uri (RejillaProject *project,
			 const gchar *uri,
			 RejillaProjectType type)
{
     	gchar *name;
	gchar *title;
	GtkAction *action;
	GtkWidget *toplevel;

	/* possibly reset the name of the project */
	if (uri) {
		if (project->priv->project)
			g_free (project->priv->project);

		project->priv->project = g_strdup (uri);
	}

	uri = uri ? uri : project->priv->project;

	/* add it to recent manager */
	if (rejilla_app_is_running (rejilla_app_get_default ()))
		rejilla_project_add_to_recents (project, uri, TRUE);

	/* update the name of the main window */
    	REJILLA_GET_BASENAME_FOR_DISPLAY (uri, name);
	if (type == REJILLA_PROJECT_TYPE_DATA)
		/* Translators: %s is the name of the project */
		title = g_strdup_printf (_("Rejilla — %s (Data Disc)"), name);
	else if (type == REJILLA_PROJECT_TYPE_AUDIO)
		/* Translators: %s is the name of the project */
		title = g_strdup_printf (_("Rejilla — %s (Audio Disc)"), name);
	else if (type == REJILLA_PROJECT_TYPE_VIDEO)
		/* Translators: %s is the name of the project */
		title = g_strdup_printf (_("Rejilla — %s (Video Disc)"), name);
	else
		title = NULL;
 
	g_free (name);

	toplevel = gtk_widget_get_toplevel (GTK_WIDGET (project));
	if (toplevel)
		gtk_window_set_title (GTK_WINDOW (toplevel), title);
	g_free (title);

	/* update the menus */
	action = gtk_action_group_get_action (project->priv->project_group, "Save");
	gtk_action_set_sensitive (action, FALSE);
}

static RejillaProjectType
rejilla_project_get_session_type (RejillaProject *project)
{
	RejillaTrackType *session_type;
	RejillaProjectType type;

        session_type = rejilla_track_type_new ();
        rejilla_burn_session_get_input_type (REJILLA_BURN_SESSION (project->priv->session), session_type);

	if (rejilla_track_type_get_has_stream (session_type)) {
                if (REJILLA_STREAM_FORMAT_HAS_VIDEO (rejilla_track_type_get_stream_format (session_type)))
		        type = REJILLA_PROJECT_TYPE_VIDEO;
                else
		        type = REJILLA_PROJECT_TYPE_AUDIO;
        }
	else if (rejilla_track_type_get_has_data (session_type))
		type = REJILLA_PROJECT_TYPE_DATA;
	else
		type = REJILLA_PROJECT_TYPE_INVALID;

    	rejilla_track_type_free (session_type);

        return type;
}

/******************************* Projects **************************************/
RejillaProjectType
rejilla_project_open_session (RejillaProject *project,
                              RejillaSessionCfg *session)
{
	GValue *value;
	RejillaProjectType type;

	rejilla_project_new_session (project, session);

	type = rejilla_project_get_session_type (project);
        if (type == REJILLA_PROJECT_TYPE_INVALID)
                return type;

	rejilla_project_switch (project, type);

	if (rejilla_burn_session_get_label (REJILLA_BURN_SESSION (project->priv->session))) {
		g_signal_handlers_block_by_func (project->priv->name_display,
						 rejilla_project_name_changed_cb,
						 project);
		gtk_entry_set_text (GTK_ENTRY (project->priv->name_display),
					       rejilla_burn_session_get_label (REJILLA_BURN_SESSION (project->priv->session)));
		g_signal_handlers_unblock_by_func (project->priv->name_display,
						   rejilla_project_name_changed_cb,
						   project);
	}

	value = NULL;
	rejilla_burn_session_tag_lookup (REJILLA_BURN_SESSION (project->priv->session),
					 REJILLA_COVER_URI,
					 &value);
	if (value) {
		if (project->priv->cover)
			g_free (project->priv->cover);

		project->priv->cover = g_strdup (g_value_get_string (value));
	}

	project->priv->modified = 0;

	return type;
}

RejillaProjectType
rejilla_project_convert_to_data (RejillaProject *project)
{
	GSList *tracks;
	RejillaProjectType type;
	RejillaSessionCfg *newsession;
	RejillaTrackDataCfg *data_track;

	newsession = rejilla_session_cfg_new ();
	data_track = rejilla_track_data_cfg_new ();
	rejilla_burn_session_add_track (REJILLA_BURN_SESSION (newsession),
					REJILLA_TRACK (data_track),
					NULL);
	g_object_unref (data_track);

	tracks = rejilla_burn_session_get_tracks (REJILLA_BURN_SESSION (project->priv->session));
	for (; tracks; tracks = tracks->next) {
		RejillaTrackStream *track;
		gchar *uri;

		track = tracks->data;
		uri = rejilla_track_stream_get_source (track, TRUE);
		rejilla_track_data_cfg_add (data_track, uri, NULL);
		g_free (uri);
	}

	type = rejilla_project_open_session (project, newsession);
	g_object_unref (newsession);

	return type;
}

RejillaProjectType
rejilla_project_convert_to_stream (RejillaProject *project,
				   gboolean is_video)
{
	GSList *tracks;
	GtkTreeIter iter;
	RejillaProjectType type;
	RejillaSessionCfg *newsession;
	RejillaTrackDataCfg *data_track;

	tracks = rejilla_burn_session_get_tracks (REJILLA_BURN_SESSION (project->priv->session));
	if (!tracks)
		return REJILLA_PROJECT_TYPE_INVALID;

	data_track = tracks->data;
	if (!gtk_tree_model_get_iter_first (GTK_TREE_MODEL (data_track), &iter))
		return REJILLA_PROJECT_TYPE_INVALID;

	newsession = rejilla_session_cfg_new ();
	do {
		gchar *uri;
		RejillaTrackStreamCfg *track;

		gtk_tree_model_get (GTK_TREE_MODEL (data_track), &iter,
				    REJILLA_DATA_TREE_MODEL_URI, &uri,
				    -1);

		track = rejilla_track_stream_cfg_new ();
		rejilla_track_stream_set_source (REJILLA_TRACK_STREAM (track), uri);
		rejilla_track_stream_set_format (REJILLA_TRACK_STREAM (track),
						 is_video ? REJILLA_VIDEO_FORMAT_UNDEFINED:REJILLA_AUDIO_FORMAT_UNDEFINED);
		g_free (uri);

		rejilla_burn_session_add_track (REJILLA_BURN_SESSION (newsession),
						REJILLA_TRACK (track),
						NULL);
		g_object_unref (track);

	} while (gtk_tree_model_iter_next (GTK_TREE_MODEL (data_track), &iter));

	type = rejilla_project_open_session (project, newsession);
	g_object_unref (newsession);

	return type;
}

/******************************** save project *********************************/
static void
rejilla_project_not_saved_dialog (RejillaProject *project)
{
	xmlError *error;

	error = xmlGetLastError ();
	rejilla_app_alert (rejilla_app_get_default (),
			   _("Your project has not been saved."),
			   error? error->message:_("An unknown error occurred"),
			   GTK_MESSAGE_ERROR);
	xmlResetLastError ();
}

static GtkResponseType
rejilla_project_save_project_dialog (RejillaProject *project,
				     gboolean show_cancel)
{
	GtkWidget *dialog;
	GtkResponseType result;

	dialog = rejilla_app_dialog (rejilla_app_get_default (),
				     _("Save the changes of current project before closing?"),
				     GTK_BUTTONS_NONE,
				     GTK_MESSAGE_WARNING);

	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
						  _("If you don't save, changes will be permanently lost."));

	if (show_cancel)
		gtk_dialog_add_buttons (GTK_DIALOG (dialog),
					_("Cl_ose Without Saving"), GTK_RESPONSE_NO,
					GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					GTK_STOCK_SAVE, GTK_RESPONSE_YES,
					NULL);
	else
		gtk_dialog_add_buttons (GTK_DIALOG (dialog),
					_("Cl_ose Without Saving"), GTK_RESPONSE_NO,
					GTK_STOCK_SAVE, GTK_RESPONSE_YES,
					NULL);

	result = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);

	if (result == GTK_RESPONSE_CANCEL)
		return GTK_RESPONSE_CANCEL;

	if (show_cancel && result == GTK_RESPONSE_DELETE_EVENT)
		return GTK_RESPONSE_CANCEL;

	if (result != GTK_RESPONSE_YES)
		return GTK_RESPONSE_NO;

	return GTK_RESPONSE_YES;
}

static gboolean
rejilla_project_save_project_real (RejillaProject *project,
				   const gchar *uri,
				   RejillaProjectSave save_type)
{
	RejillaBurnResult result;
	RejillaProjectType type;

	g_return_val_if_fail (uri != NULL || project->priv->project != NULL, FALSE);

	result = rejilla_project_check_status (project);
	if (result != REJILLA_BURN_OK)
		return FALSE;

	rejilla_project_setup_session (project, REJILLA_BURN_SESSION (project->priv->session));
        type = rejilla_project_get_session_type (project);

	if (save_type == REJILLA_PROJECT_SAVE_XML
	||  type == REJILLA_PROJECT_TYPE_DATA) {
		rejilla_project_set_uri (project, uri, type);
		if (!rejilla_project_save_project_xml (REJILLA_BURN_SESSION (project->priv->session),
						       uri ? uri : project->priv->project)) {
			rejilla_project_not_saved_dialog (project);
			return FALSE;
		}

		project->priv->modified = 0;
	}
	else if (save_type == REJILLA_PROJECT_SAVE_PLAIN) {
		if (!rejilla_project_save_audio_project_plain_text (REJILLA_BURN_SESSION (project->priv->session),
								    uri)) {
			rejilla_project_not_saved_dialog (project);
			return FALSE;
		}
	}

#ifdef BUILD_PLAYLIST

	else {
		if (!rejilla_project_save_audio_project_playlist (REJILLA_BURN_SESSION (project->priv->session),
								  uri,
								  save_type)) {
			rejilla_project_not_saved_dialog (project);
			return FALSE;
		}
	}

#endif

	return TRUE;
}

static gchar *
rejilla_project_save_project_ask_for_path (RejillaProject *project,
					   RejillaProjectSave *type)
{
	GtkWidget *combo = NULL;
	GtkWidget *toplevel;
	GtkWidget *chooser;
	gchar *uri = NULL;
	gint answer;

	toplevel = gtk_widget_get_toplevel (GTK_WIDGET (project));
	chooser = gtk_file_chooser_dialog_new (_("Save Current Project"),
					       GTK_WINDOW (toplevel),
					       GTK_FILE_CHOOSER_ACTION_SAVE,
					       GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
					       GTK_STOCK_SAVE, GTK_RESPONSE_OK,
					       NULL);

	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (chooser), TRUE);
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (chooser),
					     g_get_home_dir ());

	/* if the file chooser is an audio project offer the possibility to save
	 * in plain text a list of the current displayed songs (only in save as
	 * mode) */
	if (type && REJILLA_IS_AUDIO_DISC (project->priv->current)) {
		combo = gtk_combo_box_new_text ();
		gtk_widget_show (combo);

		gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Save project as a Rejilla audio project"));
		gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Save project as a plain text list"));

#ifdef BUILD_PLAYLIST

		gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Save project as a PLS playlist"));
		gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Save project as an M3U playlist"));
		gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Save project as an XSPF playlist"));
		gtk_combo_box_append_text (GTK_COMBO_BOX (combo), _("Save project as an iriver playlist"));

#endif

		gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
		gtk_file_chooser_set_extra_widget (GTK_FILE_CHOOSER (chooser), combo);
	}

	gtk_widget_show (chooser);
	answer = gtk_dialog_run (GTK_DIALOG (chooser));
	if (answer == GTK_RESPONSE_OK) {
		if (combo)
			*type = gtk_combo_box_get_active (GTK_COMBO_BOX (combo));

		uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (chooser));
		if (*uri == '\0') {
			g_free (uri);
			uri = NULL;
		}
	}

	gtk_widget_destroy (chooser);
	return uri;
}

gboolean
rejilla_project_save_project (RejillaProject *project)
{
	gchar *uri = NULL;
	gboolean result;

	if (!project->priv->session)
		return FALSE;

	if (!project->priv->project && !(uri = rejilla_project_save_project_ask_for_path (project, NULL)))
		return FALSE;

	result = rejilla_project_save_project_real (project, uri, REJILLA_PROJECT_SAVE_XML);
	g_free (uri);

	return result;
}

gboolean
rejilla_project_save_project_as (RejillaProject *project)
{
	RejillaProjectSave type = REJILLA_PROJECT_SAVE_XML;
	gboolean result;
	gchar *uri;

	if (!project->priv->session)
		return FALSE;

	uri = rejilla_project_save_project_ask_for_path (project, &type);
	if (!uri)
		return FALSE;

	result = rejilla_project_save_project_real (project, uri, type);
	g_free (uri);

	return result;
}

/**
 * NOTE: this function returns FALSE if it succeeds and TRUE otherwise.
 * this value is mainly used by the session object to cancel or not the app
 * closing
 */

gboolean
rejilla_project_save_session (RejillaProject *project,
			      const gchar *uri,
			      gchar **saved_uri,
			      gboolean show_cancel)
{
	if (!project->priv->session)
		return FALSE;

	if (!project->priv->current) {
		if (saved_uri)
			*saved_uri = NULL;

		return FALSE;
	}

	if (project->priv->empty) {
		/* the project is empty anyway. No need to ask anything.
		 * return FALSE since this is not a tmp project */
		if (saved_uri)
			*saved_uri = NULL;

		return FALSE;
	}

	if (project->priv->project) {
		GtkResponseType answer;

		if (!project->priv->modified) {
			/* there is a saved project but unmodified.
			 * No need to ask anything */
			if (saved_uri)
				*saved_uri = g_strdup (project->priv->project);

			return FALSE;
		}

		/* ask the user if he wants to save the changes */
		answer = rejilla_project_save_project_dialog (project, show_cancel);
		if (answer == GTK_RESPONSE_CANCEL)
			return TRUE;

		if (answer != GTK_RESPONSE_YES) {
			if (saved_uri)
				*saved_uri = NULL;

			return FALSE;
		}

		if (!rejilla_project_save_project_real (project, NULL, REJILLA_PROJECT_SAVE_XML))
			return TRUE;

		if (saved_uri)
			*saved_uri = g_strdup (project->priv->project);

		return FALSE;
	}

    	if (project->priv->burnt) {
		GtkResponseType answer;

		/* the project wasn't saved but burnt ask if the user wants to
		 * keep it for another time by saving it */
		answer = rejilla_project_save_project_dialog (project, show_cancel);
		if (answer == GTK_RESPONSE_CANCEL)
			return TRUE;

		if (answer != GTK_RESPONSE_YES) {
			if (saved_uri)
				*saved_uri = NULL;

			return FALSE;
		}

		if (!rejilla_project_save_project_as (project))
			return TRUE;

		if (saved_uri)
			*saved_uri = g_strdup (project->priv->project);

		return FALSE;
	}

    	if (!uri) {
		if (saved_uri)
			*saved_uri = NULL;

		return FALSE;
	}

	rejilla_project_setup_session (project, REJILLA_BURN_SESSION (project->priv->session));
	if (!rejilla_project_save_project_xml (REJILLA_BURN_SESSION (project->priv->session), uri)) {
		GtkResponseType response;
		GtkWidget *dialog;

		/* If the automatic saving failed, let the user decide */
		dialog = rejilla_app_dialog (rejilla_app_get_default (),
					      _("Your project has not been saved."),
					     GTK_BUTTONS_NONE,
					     GTK_MESSAGE_WARNING);

		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
							  _("If you don't save, changes will be permanently lost."));

		gtk_dialog_add_buttons (GTK_DIALOG (dialog),
		                        _("Cl_ose Without Saving"), GTK_RESPONSE_NO,
		                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		                        NULL);

		response = gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		if (saved_uri)
			*saved_uri = NULL;

		return (response == GTK_RESPONSE_CANCEL);
	}

	if (saved_uri)
		*saved_uri = g_strdup (uri);

    	return FALSE;
}
