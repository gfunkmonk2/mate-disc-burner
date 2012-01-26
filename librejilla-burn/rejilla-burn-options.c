/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Librejilla-burn
 * Copyright (C) Philippe Rouquier 2005-2009 <bonfire-app@wanadoo.fr>
 *
 * Librejilla-burn is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * The Librejilla-burn authors hereby grant permission for non-GPL compatible
 * GStreamer plugins to be used and distributed together with GStreamer
 * and Librejilla-burn. This permission is above and beyond the permissions granted
 * by the GPL license by which Librejilla-burn is covered. If you modify this code
 * you may extend this exception to your version of the code, but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version.
 * 
 * Librejilla-burn is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Library General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to:
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

#include <gdk/gdkx.h>
#include <gtk/gtk.h>

#include "burn-basics.h"

#include "rejilla-medium.h"
#include "rejilla-medium-selection-priv.h"

#include "burn-debug.h"
#include "rejilla-session.h"
#include "rejilla-session-helper.h"
#include "rejilla-burn-options.h"
#include "rejilla-video-options.h"
#include "rejilla-src-image.h"
#include "rejilla-src-selection.h"
#include "rejilla-session-cfg.h"
#include "rejilla-dest-selection.h"
#include "rejilla-medium-properties.h"
#include "rejilla-status-dialog.h"
#include "rejilla-track-stream.h"
#include "rejilla-track-data-cfg.h"
#include "rejilla-track-image-cfg.h"

#include "rejilla-notify.h"
#include "rejilla-misc.h"
#include "rejilla-pk.h"

typedef struct _RejillaBurnOptionsPrivate RejillaBurnOptionsPrivate;
struct _RejillaBurnOptionsPrivate
{
	RejillaSessionCfg *session;

	GtkSizeGroup *size_group;

	GtkWidget *source;
	GtkWidget *source_placeholder;
	GtkWidget *message_input;
	GtkWidget *selection;
	GtkWidget *properties;
	GtkWidget *message_output;
	GtkWidget *options;
	GtkWidget *options_placeholder;
	GtkWidget *button;

	GtkWidget *burn;
	GtkWidget *burn_multi;

	guint not_ready_id;
	GtkWidget *status_dialog;

	GCancellable *cancel;

	guint is_valid:1;

	guint has_image:1;
	guint has_audio:1;
	guint has_video:1;
	guint has_data:1;
	guint has_disc:1;
};

#define REJILLA_BURN_OPTIONS_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REJILLA_TYPE_BURN_OPTIONS, RejillaBurnOptionsPrivate))

enum {
	PROP_0,
	PROP_SESSION
};

G_DEFINE_TYPE (RejillaBurnOptions, rejilla_burn_options, GTK_TYPE_DIALOG);

static void
rejilla_burn_options_add_source (RejillaBurnOptions *self,
				 const gchar *title,
				 ...)
{
	va_list vlist;
	GtkWidget *child;
	GSList *list = NULL;
	RejillaBurnOptionsPrivate *priv;

	priv = REJILLA_BURN_OPTIONS_PRIVATE (self);

	/* create message queue for input */
	priv->message_input = rejilla_notify_new ();
	list = g_slist_prepend (list, priv->message_input);

	va_start (vlist, title);
	while ((child = va_arg (vlist, GtkWidget *))) {
		GtkWidget *hbox;
		GtkWidget *alignment;

		hbox = gtk_hbox_new (FALSE, 12);
		gtk_widget_show (hbox);

		gtk_box_pack_start (GTK_BOX (hbox), child, TRUE, TRUE, 0);

		alignment = gtk_alignment_new (0.0, 0.5, 0., 0.);
		gtk_widget_show (alignment);
		gtk_size_group_add_widget (priv->size_group, alignment);
		gtk_box_pack_start (GTK_BOX (hbox), alignment, FALSE, FALSE, 0);

		list = g_slist_prepend (list, hbox);
	}
	va_end (vlist);

	priv->source = rejilla_utils_pack_properties_list (title, list);
	g_slist_free (list);

	gtk_container_add (GTK_CONTAINER (priv->source_placeholder), priv->source);
	gtk_widget_show (priv->source_placeholder);
}

/**
 * rejilla_burn_options_add_options:
 * @dialog: a #RejillaBurnOptions
 * @options: a #GtkWidget
 *
 * Adds some new options to be displayed in the dialog.
 **/

void
rejilla_burn_options_add_options (RejillaBurnOptions *self,
				  GtkWidget *options)
{
	RejillaBurnOptionsPrivate *priv;

	priv = REJILLA_BURN_OPTIONS_PRIVATE (self);
	gtk_box_pack_start (GTK_BOX (priv->options), options, FALSE, TRUE, 0);
	gtk_widget_show (priv->options);
}

static void
rejilla_burn_options_set_type_shown (RejillaBurnOptions *self,
				     RejillaMediaType type)
{
	RejillaBurnOptionsPrivate *priv;

	priv = REJILLA_BURN_OPTIONS_PRIVATE (self);
	rejilla_medium_selection_show_media_type (REJILLA_MEDIUM_SELECTION (priv->selection), type);
}

static void
rejilla_burn_options_message_response_cb (RejillaDiscMessage *message,
					  GtkResponseType response,
					  RejillaBurnOptions *self)
{
	if (response == GTK_RESPONSE_OK) {
		RejillaBurnOptionsPrivate *priv;

		priv = REJILLA_BURN_OPTIONS_PRIVATE (self);
		rejilla_session_cfg_add_flags (priv->session, REJILLA_BURN_FLAG_OVERBURN);
	}
}

static void
rejilla_burn_options_message_response_span_cb (RejillaDiscMessage *message,
					       GtkResponseType response,
					       RejillaBurnOptions *self)
{
	if (response == GTK_RESPONSE_OK) {
		RejillaBurnOptionsPrivate *priv;

		priv = REJILLA_BURN_OPTIONS_PRIVATE (self);
		rejilla_session_span_start (REJILLA_SESSION_SPAN (priv->session));
		if (rejilla_session_span_next (REJILLA_SESSION_SPAN (priv->session)) == REJILLA_BURN_ERR)
			REJILLA_BURN_LOG ("Spanning failed\n");
	}
}

#define REJILLA_BURN_OPTIONS_NO_MEDIUM_WARNING	1000

static void
rejilla_burn_options_update_no_medium_warning (RejillaBurnOptions *self)
{
	RejillaBurnOptionsPrivate *priv;

	priv = REJILLA_BURN_OPTIONS_PRIVATE (self);

	if (!priv->is_valid
	||  !rejilla_burn_session_is_dest_file (REJILLA_BURN_SESSION (priv->session))
	||   rejilla_medium_selection_get_media_num (REJILLA_MEDIUM_SELECTION (priv->selection)) != 1) {
		rejilla_notify_message_remove (priv->message_output,
					       REJILLA_BURN_OPTIONS_NO_MEDIUM_WARNING);
		return;
	}

	/* The user may have forgotten to insert a disc so remind him of that if
	 * there aren't any other possibility in the selection */
	rejilla_notify_message_add (priv->message_output,
				    _("Please insert a writable CD or DVD if you don't want to write to an image file."),
				    NULL,
				    -1,
				    REJILLA_BURN_OPTIONS_NO_MEDIUM_WARNING);
}

static void
rejilla_burn_options_not_ready_dialog_cancel_cb (GtkDialog *dialog,
						 guint response,
						 gpointer data)
{
	RejillaBurnOptionsPrivate *priv;

	priv = REJILLA_BURN_OPTIONS_PRIVATE (data);

	if (priv->not_ready_id) {
		g_source_remove (priv->not_ready_id);
		priv->not_ready_id = 0;
	}
	gtk_widget_set_sensitive (GTK_WIDGET (data), TRUE);

	if (response != GTK_RESPONSE_OK)
		gtk_dialog_response (GTK_DIALOG (data),
				     GTK_RESPONSE_CANCEL);
	else {
		priv->status_dialog = NULL;
		gtk_widget_destroy (GTK_WIDGET (dialog));
	}		
}

static gboolean
rejilla_burn_options_not_ready_dialog_show_cb (gpointer data)
{
	RejillaBurnOptionsPrivate *priv;

	priv = REJILLA_BURN_OPTIONS_PRIVATE (data);

	/* icon should be set by now */
	gtk_window_set_icon_name (GTK_WINDOW (priv->status_dialog),
	                          gtk_window_get_icon_name (GTK_WINDOW (data)));

	gtk_widget_show (priv->status_dialog);
	priv->not_ready_id = 0;
	return FALSE;
}

static void
rejilla_burn_options_not_ready_dialog_shown_cb (GtkWidget *widget,
                                                gpointer data)
{
	RejillaBurnOptionsPrivate *priv;

	priv = REJILLA_BURN_OPTIONS_PRIVATE (data);
	if (priv->not_ready_id) {
		g_source_remove (priv->not_ready_id);
		priv->not_ready_id = 0;
	}
}

static void
rejilla_burn_options_setup_buttons (RejillaBurnOptions *self)
{
	RejillaBurnOptionsPrivate *priv;
	RejillaTrackType *type;
	gchar *label_m = "";
	gchar *label;
	gchar *icon;

	priv = REJILLA_BURN_OPTIONS_PRIVATE (self);

	/* add the new widgets */
	type = rejilla_track_type_new ();
	rejilla_burn_session_get_input_type (REJILLA_BURN_SESSION (priv->session), type);
	if (rejilla_burn_session_is_dest_file (REJILLA_BURN_SESSION (priv->session))) {
		label = _("Create _Image");
		icon = "iso-image-new";
	}
	else if (rejilla_track_type_get_has_medium (type)) {
		/* Translators: This is a verb, an action */
		label = _("_Copy");
		icon = "media-optical-copy";
	
		label_m = _("Make _Several Copies");
	}
	else {
		/* Translators: This is a verb, an action */
		label = _("_Burn");
		icon = "media-optical-burn";

		label_m = _("Burn _Several Copies");
	}

	if (priv->burn_multi)
		gtk_button_set_label (GTK_BUTTON (priv->burn_multi), label_m);
	else
		priv->burn_multi = gtk_dialog_add_button (GTK_DIALOG (self),
							  label_m,
							  GTK_RESPONSE_ACCEPT);

	if (rejilla_burn_session_is_dest_file (REJILLA_BURN_SESSION (priv->session)))
		gtk_widget_hide (priv->burn_multi);
	else
		gtk_widget_show (priv->burn_multi);

	if (priv->burn)
		gtk_button_set_label (GTK_BUTTON (priv->burn), label);
	else
		priv->burn = gtk_dialog_add_button (GTK_DIALOG (self),
						    label,
						    GTK_RESPONSE_OK);

	gtk_button_set_image (GTK_BUTTON (priv->burn), gtk_image_new_from_icon_name (icon, GTK_ICON_SIZE_BUTTON));

	gtk_widget_set_sensitive (priv->burn, priv->is_valid);
	gtk_widget_set_sensitive (priv->burn_multi, priv->is_valid);

	rejilla_track_type_free (type);
}

static void
rejilla_burn_options_update_valid (RejillaBurnOptions *self)
{
	RejillaBurnOptionsPrivate *priv;
	RejillaSessionError valid;

	priv = REJILLA_BURN_OPTIONS_PRIVATE (self);

	valid = rejilla_session_cfg_get_error (priv->session);
	priv->is_valid = REJILLA_SESSION_IS_VALID (valid);

	rejilla_burn_options_setup_buttons (self);

	gtk_widget_set_sensitive (priv->options, priv->is_valid);
	gtk_widget_set_sensitive (priv->properties, priv->is_valid);

	if (priv->message_input) {
		gtk_widget_hide (priv->message_input);
		rejilla_notify_message_remove (priv->message_input,
					       REJILLA_NOTIFY_CONTEXT_SIZE);
	}

	rejilla_notify_message_remove (priv->message_output,
				       REJILLA_NOTIFY_CONTEXT_SIZE);

	if (valid == REJILLA_SESSION_NOT_READY) {
		if (!priv->not_ready_id && !priv->status_dialog) {
			gtk_widget_set_sensitive (GTK_WIDGET (self), FALSE);
			priv->status_dialog = rejilla_status_dialog_new (REJILLA_BURN_SESSION (priv->session),  GTK_WIDGET (self));
			g_signal_connect (priv->status_dialog,
					  "response", 
					  G_CALLBACK (rejilla_burn_options_not_ready_dialog_cancel_cb),
					  self);

			g_signal_connect (priv->status_dialog,
					  "show", 
					  G_CALLBACK (rejilla_burn_options_not_ready_dialog_shown_cb),
					  self);
			g_signal_connect (priv->status_dialog,
					  "user-interaction", 
					  G_CALLBACK (rejilla_burn_options_not_ready_dialog_shown_cb),
					  self);

			priv->not_ready_id = g_timeout_add_seconds (1,
								    rejilla_burn_options_not_ready_dialog_show_cb,
								    self);
		}
	}
	else {
		gtk_widget_set_sensitive (GTK_WIDGET (self), TRUE);
		if (priv->status_dialog) {
			gtk_widget_destroy (priv->status_dialog);
			priv->status_dialog = NULL;
		}

		if (priv->not_ready_id) {
			g_source_remove (priv->not_ready_id);
			priv->not_ready_id = 0;
		}
	}

	if (valid == REJILLA_SESSION_INSUFFICIENT_SPACE) {
		goffset min_disc_size;
		goffset available_space;

		min_disc_size = rejilla_session_span_get_max_space (REJILLA_SESSION_SPAN (priv->session));

		/* One rule should be that the maximum batch size should not exceed the disc size
		 * FIXME: we could change it into a dialog telling the user what is the maximum
		 * size required. */
		available_space = rejilla_burn_session_get_available_medium_space (REJILLA_BURN_SESSION (priv->session));

		/* Here there is an alternative: we may be able to span the data
		 * across multiple media. So try that. */
		if (available_space > min_disc_size
		&&  rejilla_session_span_possible (REJILLA_SESSION_SPAN (priv->session)) == REJILLA_BURN_RETRY) {
			GtkWidget *message;

			message = rejilla_notify_message_add (priv->message_output,
							      _("Would you like to burn the selection of files across several media?"),
							      _("The data size is too large for the disc even with the overburn option."),
							      -1,
							      REJILLA_NOTIFY_CONTEXT_SIZE);

			gtk_widget_set_tooltip_text (gtk_info_bar_add_button (GTK_INFO_BAR (message),
									      _("_Burn Several Discs"),
									      GTK_RESPONSE_OK),
						    _("Burn the selection of files across several media"));

			g_signal_connect (message,
					  "response",
					  G_CALLBACK (rejilla_burn_options_message_response_span_cb),
					  self);
		}
		else
			rejilla_notify_message_add (priv->message_output,
						    _("Please choose another CD or DVD or insert a new one."),
						    _("The data size is too large for the disc even with the overburn option."),
						    -1,
						    REJILLA_NOTIFY_CONTEXT_SIZE);
	}
	else if (valid == REJILLA_SESSION_NO_OUTPUT) {
		rejilla_notify_message_add (priv->message_output,
					    _("Please insert a writable CD or DVD."),
					    NULL,
					    -1,
					    REJILLA_NOTIFY_CONTEXT_SIZE);
	}
	else if (valid == REJILLA_SESSION_NO_CD_TEXT) {
		rejilla_notify_message_add (priv->message_output,
					    _("No track information (artist, title, ...) will be written to the disc."),
					    _("This is not supported by the current active burning backend."),
					    -1,
					    REJILLA_NOTIFY_CONTEXT_SIZE);
	}
	else if (valid == REJILLA_SESSION_EMPTY) {
		RejillaTrackType *type;
		
		type = rejilla_track_type_new ();
		rejilla_burn_session_get_input_type (REJILLA_BURN_SESSION (priv->session), type);

		if (rejilla_track_type_get_has_data (type))
			rejilla_notify_message_add (priv->message_output,
						    _("Please add files."),
						    _("There are no files to write to disc"),
						    -1,
						    REJILLA_NOTIFY_CONTEXT_SIZE);
		else if (!REJILLA_STREAM_FORMAT_HAS_VIDEO (rejilla_track_type_get_stream_format (type)))
			rejilla_notify_message_add (priv->message_output,
						    _("Please add songs."),
						    _("There are no songs to write to disc"),
						    -1,
						    REJILLA_NOTIFY_CONTEXT_SIZE);
		else
			rejilla_notify_message_add (priv->message_output,
						     _("Please add videos."),
						    _("There are no videos to write to disc"),
						    -1,
						    REJILLA_NOTIFY_CONTEXT_SIZE);
		rejilla_track_type_free (type);
		gtk_window_resize (GTK_WINDOW (self), 10, 10);
		return;		      
	}
	else if (valid == REJILLA_SESSION_NO_INPUT_MEDIUM) {
		GtkWidget *message;

		if (priv->message_input) {
			gtk_widget_show (priv->message_input);
			message = rejilla_notify_message_add (priv->message_input,
							      _("Please insert a disc holding data."),
							      _("There is no inserted disc to copy."),
							      -1,
							      REJILLA_NOTIFY_CONTEXT_SIZE);
		}
	}
	else if (valid == REJILLA_SESSION_NO_INPUT_IMAGE) {
		GtkWidget *message;

		if (priv->message_input) {
			gtk_widget_show (priv->message_input);
			message = rejilla_notify_message_add (priv->message_input,
							      _("Please select a disc image."),
							      _("There is no selected disc image."),
							      -1,
							      REJILLA_NOTIFY_CONTEXT_SIZE);
		}
	}
	else if (valid == REJILLA_SESSION_UNKNOWN_IMAGE) {
		GtkWidget *message;

		if (priv->message_input) {
			gtk_widget_show (priv->message_input);
			message = rejilla_notify_message_add (priv->message_input,
							      /* Translators: this is a disc image not a picture */
							      C_("disc", "Please select another image."),
							      _("It doesn't appear to be a valid disc image or a valid cue file."),
							      -1,
							      REJILLA_NOTIFY_CONTEXT_SIZE);
		}
	}
	else if (valid == REJILLA_SESSION_DISC_PROTECTED) {
		GtkWidget *message;

		if (priv->message_input) {
			gtk_widget_show (priv->message_input);
			message = rejilla_notify_message_add (priv->message_input,
							      _("Please insert a disc that is not copy protected."),
							      _("All required applications and libraries are not installed."),
							      -1,
							      REJILLA_NOTIFY_CONTEXT_SIZE);
		}
	}
	else if (valid == REJILLA_SESSION_NOT_SUPPORTED) {
		rejilla_notify_message_add (priv->message_output,
		                            _("Please replace the disc with a supported CD or DVD."),
		                            NULL,
		                            -1,
		                            REJILLA_NOTIFY_CONTEXT_SIZE);
	}
	else if (valid == REJILLA_SESSION_OVERBURN_NECESSARY) {
		GtkWidget *message;

		message = rejilla_notify_message_add (priv->message_output,
						      _("Would you like to burn beyond the disc's reported capacity?"),
						      _("The data size is too large for the disc and you must remove files from the selection otherwise."
							"\nYou may want to use this option if you're using 90 or 100 min CD-R(W) which cannot be properly recognised and therefore need overburn option."
							"\nNOTE: This option might cause failure."),
						      -1,
						      REJILLA_NOTIFY_CONTEXT_SIZE);

		gtk_widget_set_tooltip_text (gtk_info_bar_add_button (GTK_INFO_BAR (message),
								      _("_Overburn"),
								      GTK_RESPONSE_OK),
					     _("Burn beyond the disc's reported capacity"));

		g_signal_connect (message,
				  "response",
				  G_CALLBACK (rejilla_burn_options_message_response_cb),
				  self);
	}
	else if (rejilla_burn_session_same_src_dest_drive (REJILLA_BURN_SESSION (priv->session))) {
		/* The medium is valid but it's a special case */
		rejilla_notify_message_add (priv->message_output,
					    _("The drive that holds the source disc will also be the one used to record."),
					    _("A new writable disc will be required once the currently loaded one has been copied."),
					    -1,
					    REJILLA_NOTIFY_CONTEXT_SIZE);
		gtk_widget_show_all (priv->message_output);
	}

	rejilla_burn_options_update_no_medium_warning (self);
	gtk_window_resize (GTK_WINDOW (self), 10, 10);
}

static void
rejilla_burn_options_valid_cb (RejillaSessionCfg *session,
			       RejillaBurnOptions *self)
{
	rejilla_burn_options_update_valid (self);
}

static void
rejilla_burn_options_init (RejillaBurnOptions *object)
{
}

/**
 * To build anything we need to have the session set first
 */

static void
rejilla_burn_options_build_contents (RejillaBurnOptions *object)
{
	RejillaBurnOptionsPrivate *priv;
	GtkWidget *content_area;
	GtkWidget *selection;
	GtkWidget *alignment;
	gchar *string;

	priv = REJILLA_BURN_OPTIONS_PRIVATE (object);

	priv->size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

	/* Sets default flags for the session */
	rejilla_burn_session_add_flag (REJILLA_BURN_SESSION (priv->session),
				       REJILLA_BURN_FLAG_NOGRACE|
				       REJILLA_BURN_FLAG_CHECK_SIZE);

	/* Create a cancel button */
	gtk_dialog_add_button (GTK_DIALOG (object),
			       GTK_STOCK_CANCEL,
			       GTK_RESPONSE_CANCEL);

	/* Create an upper box for sources */
	priv->source_placeholder = gtk_alignment_new (0.0, 0.5, 1.0, 1.0);
	content_area = gtk_dialog_get_content_area (GTK_DIALOG (object));
	gtk_box_pack_start (GTK_BOX (content_area),
			    priv->source_placeholder,
			    FALSE,
			    TRUE,
			    0);

	/* Medium selection box */
	selection = gtk_hbox_new (FALSE, 12);
	gtk_widget_show (selection);

	alignment = gtk_alignment_new (0.0, 0.5, 1.0, 1.0);
	gtk_widget_show (alignment);
	gtk_box_pack_start (GTK_BOX (selection),
			    alignment,
			    TRUE,
			    TRUE,
			    0);

	priv->selection = rejilla_dest_selection_new (REJILLA_BURN_SESSION (priv->session));
	gtk_widget_show (priv->selection);
	gtk_container_add (GTK_CONTAINER (alignment), priv->selection);

	priv->properties = rejilla_medium_properties_new (priv->session);
	gtk_size_group_add_widget (priv->size_group, priv->properties);
	gtk_widget_show (priv->properties);
	gtk_box_pack_start (GTK_BOX (selection),
			    priv->properties,
			    FALSE,
			    FALSE,
			    0);

	/* Box to display warning messages */
	priv->message_output = rejilla_notify_new ();
	gtk_widget_show (priv->message_output);

	string = g_strdup_printf ("<b>%s</b>", _("Select a disc to write to"));
	selection = rejilla_utils_pack_properties (string,
						   priv->message_output,
						   selection,
						   NULL);
	g_free (string);
	gtk_widget_show (selection);

	gtk_box_pack_start (GTK_BOX (content_area),
			    selection,
			    FALSE,
			    TRUE,
			    0);

	/* Create a lower box for options */
	alignment = gtk_alignment_new (0.0, 0.5, 1.0, 1.0);
	gtk_widget_show (alignment);
	gtk_box_pack_start (GTK_BOX (content_area),
			    alignment,
			    FALSE,
			    TRUE,
			    0);
	priv->options_placeholder = alignment;

	priv->options = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (alignment), priv->options);

	g_signal_connect (priv->session,
			  "is-valid",
			  G_CALLBACK (rejilla_burn_options_valid_cb),
			  object);

	rejilla_burn_options_update_valid (object);
}

static void
rejilla_burn_options_reset (RejillaBurnOptions *self)
{
	RejillaBurnOptionsPrivate *priv;

	priv = REJILLA_BURN_OPTIONS_PRIVATE (self);

	priv->has_image = FALSE;
	priv->has_audio = FALSE;
	priv->has_video = FALSE;
	priv->has_data = FALSE;
	priv->has_disc = FALSE;

	/* reset all the dialog */
	if (priv->message_input) {
		gtk_widget_destroy (priv->message_input);
		priv->message_input = NULL;
	}

	if (priv->options) {
		gtk_widget_destroy (priv->options);
		priv->options = NULL;
	}

	priv->options = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (priv->options_placeholder), priv->options);

	if (priv->source) {
		gtk_widget_destroy (priv->source);
		priv->source = NULL;
	}

	gtk_widget_hide (priv->source_placeholder);
}

static void
rejilla_burn_options_setup_audio (RejillaBurnOptions *self)
{
	RejillaBurnOptionsPrivate *priv;

	priv = REJILLA_BURN_OPTIONS_PRIVATE (self);

	rejilla_burn_options_reset (self);

	priv->has_audio = TRUE;
	gtk_window_set_title (GTK_WINDOW (self), _("Disc Burning Setup"));
	rejilla_burn_options_set_type_shown (REJILLA_BURN_OPTIONS (self),
					     REJILLA_MEDIA_TYPE_WRITABLE|
					     REJILLA_MEDIA_TYPE_FILE);
}

static void
rejilla_burn_options_setup_video (RejillaBurnOptions *self)
{
	gchar *title;
	GtkWidget *options;
	RejillaBurnOptionsPrivate *priv;

	priv = REJILLA_BURN_OPTIONS_PRIVATE (self);

	rejilla_burn_options_reset (self);

	priv->has_video = TRUE;
	gtk_window_set_title (GTK_WINDOW (self), _("Disc Burning Setup"));
	rejilla_burn_options_set_type_shown (REJILLA_BURN_OPTIONS (self),
					     REJILLA_MEDIA_TYPE_WRITABLE|
					     REJILLA_MEDIA_TYPE_FILE);

	/* create the options box */
	options = rejilla_video_options_new (REJILLA_BURN_SESSION (priv->session));
	gtk_widget_show (options);

	title = g_strdup_printf ("<b>%s</b>", _("Video Options"));
	options = rejilla_utils_pack_properties (title,
	                                           options,
	                                           NULL);
	g_free (title);

	gtk_widget_show (options);
	rejilla_burn_options_add_options (self, options);
}

static RejillaBurnResult
rejilla_status_dialog_uri_has_image (RejillaTrackDataCfg *track,
				     const gchar *uri,
				     RejillaBurnOptions *self)
{
	gint answer;
	gchar *name;
	GtkWidget *button;
	GtkWidget *dialog;
	gboolean was_visible = FALSE;
	gboolean was_not_ready = FALSE;
	RejillaTrackImageCfg *track_img;
	RejillaBurnOptionsPrivate *priv;

	priv = REJILLA_BURN_OPTIONS_PRIVATE (self);
	dialog = gtk_message_dialog_new (GTK_WINDOW (self),
					 GTK_DIALOG_MODAL |
					 GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_QUESTION,
					 GTK_BUTTONS_NONE,
					 "%s",
					 _("Do you want to create a disc from the contents of the image or with the image file inside?"));

	gtk_window_set_title (GTK_WINDOW (dialog), "");
	gtk_window_set_icon_name (GTK_WINDOW (dialog),
	                          gtk_window_get_icon_name (GTK_WINDOW (self)));

	name = rejilla_utils_get_uri_name (uri);
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
			/* Translators: %s is the name of the image */
			_("There is only one selected file (\"%s\"). "
			  "It is the image of a disc and its contents can be burned."),
			name);
	g_free (name);

	gtk_dialog_add_button (GTK_DIALOG (dialog), _("Burn as _File"), GTK_RESPONSE_NO);

	button = rejilla_utils_make_button (_("Burn _Contents…"),
	                                    NULL,
	                                    "media-optical-burn",
	                                    GTK_ICON_SIZE_BUTTON);
	gtk_widget_show (button);
	gtk_dialog_add_action_widget (GTK_DIALOG (dialog),
				      button,
				      GTK_RESPONSE_YES);

	if (!priv->not_ready_id && priv->status_dialog) {
		was_visible = TRUE;
		gtk_widget_hide (GTK_WIDGET (priv->status_dialog));
	}
	else if (priv->not_ready_id) {
		g_source_remove (priv->not_ready_id);
		priv->not_ready_id = 0;
		was_not_ready = TRUE;
	}

	gtk_widget_show_all (dialog);
	answer = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);

	if (answer != GTK_RESPONSE_YES) {
		if (was_not_ready)
			priv->not_ready_id = g_timeout_add_seconds (1,
								    rejilla_burn_options_not_ready_dialog_show_cb,
								    self);
		if (was_visible)
			gtk_widget_show (GTK_WIDGET (priv->status_dialog));

		return REJILLA_BURN_OK;
	}

	/* Setup a new track and add it to session */
	track_img = rejilla_track_image_cfg_new ();
	rejilla_track_image_cfg_set_source (track_img, uri);
	rejilla_burn_session_add_track (REJILLA_BURN_SESSION (priv->session),
					REJILLA_TRACK (track_img),
					NULL);

	return REJILLA_BURN_CANCEL;
}

static void
rejilla_burn_options_setup_data (RejillaBurnOptions *self)
{
	GSList *tracks;
	RejillaBurnOptionsPrivate *priv;

	priv = REJILLA_BURN_OPTIONS_PRIVATE (self);

	rejilla_burn_options_reset (self);

	/* NOTE: we don't need to keep a record of the signal as the track will
	 * be destroyed if the user agrees to burn the image directly */
	tracks = rejilla_burn_session_get_tracks (REJILLA_BURN_SESSION (priv->session));
	if (REJILLA_IS_TRACK_DATA_CFG (tracks->data))
		g_signal_connect (tracks->data,
				  "image-uri",
				  G_CALLBACK (rejilla_status_dialog_uri_has_image),
				  self);

	priv->has_data = TRUE;
	gtk_window_set_title (GTK_WINDOW (self), _("Disc Burning Setup"));
	rejilla_burn_options_set_type_shown (REJILLA_BURN_OPTIONS (self),
					     REJILLA_MEDIA_TYPE_WRITABLE|
					     REJILLA_MEDIA_TYPE_FILE);
}

static void
rejilla_burn_options_setup_image (RejillaBurnOptions *self)
{
	gchar *string;
	GtkWidget *file;
	RejillaBurnOptionsPrivate *priv;

	priv = REJILLA_BURN_OPTIONS_PRIVATE (self);

	rejilla_burn_options_reset (self);

	priv->has_image = TRUE;
	gtk_window_set_title (GTK_WINDOW (self), _("Image Burning Setup"));
	rejilla_burn_options_set_type_shown (self, REJILLA_MEDIA_TYPE_WRITABLE);

	/* Image properties */
	file = rejilla_src_image_new (REJILLA_BURN_SESSION (priv->session));
	gtk_widget_show (file);

	/* pack everything */
	string = g_strdup_printf ("<b>%s</b>", _("Select a disc image to write"));
	rejilla_burn_options_add_source (self, 
					 string,
					 file,
					 NULL);
	g_free (string);
}

static void
rejilla_burn_options_setup_disc (RejillaBurnOptions *self)
{
	gchar *title_str;
	GtkWidget *source;
	RejillaBurnOptionsPrivate *priv;

	priv = REJILLA_BURN_OPTIONS_PRIVATE (self);

	rejilla_burn_options_reset (self);

	priv->has_disc = TRUE;
	gtk_window_set_title (GTK_WINDOW (self), _("Copy CD/DVD"));

	/* take care of source media */
	source = rejilla_src_selection_new (REJILLA_BURN_SESSION (priv->session));
	gtk_widget_show (source);

	title_str = g_strdup_printf ("<b>%s</b>", _("Select disc to copy"));
	rejilla_burn_options_add_source (self,
					 title_str,
					 source,
					 NULL);
	g_free (title_str);

	/* only show media with something to be read on them */
	rejilla_medium_selection_show_media_type (REJILLA_MEDIUM_SELECTION (source),
						  REJILLA_MEDIA_TYPE_AUDIO|
						  REJILLA_MEDIA_TYPE_DATA);

	/* This is a special case. When we're copying, someone may want to read
	 * and burn to the same drive so provided that the drive is a burner
	 * then show its contents. */
	rejilla_burn_options_set_type_shown (self,
					     REJILLA_MEDIA_TYPE_ANY_IN_BURNER|
					     REJILLA_MEDIA_TYPE_FILE);
}

static void
rejilla_burn_options_setup (RejillaBurnOptions *self)
{
	RejillaBurnOptionsPrivate *priv;
	RejillaTrackType *type;

	priv = REJILLA_BURN_OPTIONS_PRIVATE (self);

	/* add the new widgets */
	type = rejilla_track_type_new ();
	rejilla_burn_session_get_input_type (REJILLA_BURN_SESSION (priv->session), type);
	if (rejilla_track_type_get_has_medium (type)) {
		if (!priv->has_disc)
			rejilla_burn_options_setup_disc (self);
	}
	else if (rejilla_track_type_get_has_image (type)) {
		if (!priv->has_image)
			rejilla_burn_options_setup_image (self);
	}
	else if (rejilla_track_type_get_has_data (type)) {
		if (!priv->has_data)
			rejilla_burn_options_setup_data (self);
	}
	else if (rejilla_track_type_get_has_stream (type)) {
		if (REJILLA_STREAM_FORMAT_HAS_VIDEO (rejilla_track_type_get_stream_format (type))) {
			if (!priv->has_video)
				rejilla_burn_options_setup_video (self);
		}
		else if (!priv->has_audio)
			rejilla_burn_options_setup_audio (self);
	}
	rejilla_track_type_free (type);

	rejilla_burn_options_setup_buttons (self);
}

static void
rejilla_burn_options_track_added (RejillaBurnSession *session,
				  RejillaTrack *track,
				  RejillaBurnOptions *dialog)
{
	rejilla_burn_options_setup (dialog);
}

static void
rejilla_burn_options_track_removed (RejillaBurnSession *session,
				    RejillaTrack *track,
				    guint former_position,
				    RejillaBurnOptions *dialog)
{
	rejilla_burn_options_setup (dialog);
}

static void
rejilla_burn_options_track_changed (RejillaBurnSession *session,
                                    RejillaTrack *track,
                                    RejillaBurnOptions *dialog)
{
	rejilla_burn_options_setup (dialog);
}

static void
rejilla_burn_options_set_property (GObject *object,
				   guint prop_id,
				   const GValue *value,
				   GParamSpec *pspec)
{
	RejillaBurnOptionsPrivate *priv;

	g_return_if_fail (REJILLA_IS_BURN_OPTIONS (object));

	priv = REJILLA_BURN_OPTIONS_PRIVATE (object);

	switch (prop_id)
	{
	case PROP_SESSION: /* Readable and only writable at creation time */
		priv->session = g_object_ref (g_value_get_object (value));

		g_object_notify (object, "session");

		g_signal_connect (priv->session,
				  "track-added",
				  G_CALLBACK (rejilla_burn_options_track_added),
				  object);
		g_signal_connect (priv->session,
				  "track-removed",
				  G_CALLBACK (rejilla_burn_options_track_removed),
				  object);
		g_signal_connect (priv->session,
				  "track-changed",
				  G_CALLBACK (rejilla_burn_options_track_changed),
				  object);
		rejilla_burn_options_build_contents (REJILLA_BURN_OPTIONS (object));
		rejilla_burn_options_setup (REJILLA_BURN_OPTIONS (object));

		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
rejilla_burn_options_get_property (GObject *object,
				   guint prop_id,
				   GValue *value,
				   GParamSpec *pspec)
{
	RejillaBurnOptionsPrivate *priv;

	g_return_if_fail (REJILLA_IS_BURN_OPTIONS (object));

	priv = REJILLA_BURN_OPTIONS_PRIVATE (object);

	switch (prop_id)
	{
	case PROP_SESSION:
		g_value_set_object (value, priv->session);
		g_object_ref (priv->session);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
rejilla_burn_options_finalize (GObject *object)
{
	RejillaBurnOptionsPrivate *priv;

	priv = REJILLA_BURN_OPTIONS_PRIVATE (object);

	if (priv->cancel) {
		g_cancellable_cancel (priv->cancel);
		priv->cancel = NULL;
	}

	if (priv->not_ready_id) {
		g_source_remove (priv->not_ready_id);
		priv->not_ready_id = 0;
	}

	if (priv->status_dialog) {
		gtk_widget_destroy (priv->status_dialog);
		priv->status_dialog = NULL;
	}

	if (priv->session) {
		g_signal_handlers_disconnect_by_func (priv->session,
						      rejilla_burn_options_track_added,
						      object);
		g_signal_handlers_disconnect_by_func (priv->session,
						      rejilla_burn_options_track_removed,
						      object);
		g_signal_handlers_disconnect_by_func (priv->session,
						      rejilla_burn_options_track_changed,
						      object);
		g_signal_handlers_disconnect_by_func (priv->session,
						      rejilla_burn_options_valid_cb,
						      object);

		g_object_unref (priv->session);
		priv->session = NULL;
	}

	if (priv->size_group) {
		g_object_unref (priv->size_group);
		priv->size_group = NULL;
	}

	G_OBJECT_CLASS (rejilla_burn_options_parent_class)->finalize (object);
}

static RejillaBurnResult
rejilla_burn_options_install_missing (RejillaPluginErrorType type,
                                      const gchar *detail,
                                      gpointer user_data)
{
	RejillaBurnOptionsPrivate *priv = REJILLA_BURN_OPTIONS_PRIVATE (user_data);
	GCancellable *cancel;
	RejillaPK *package;
	gboolean res;
	int xid = 0;

	/* Get the xid */
	xid = gdk_x11_drawable_get_xid (GDK_DRAWABLE (gtk_widget_get_window (GTK_WIDGET (user_data))));

	package = rejilla_pk_new ();
	cancel = g_cancellable_new ();
	priv->cancel = cancel;
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

	priv->cancel = NULL;
	g_object_unref (cancel);

	if (!res)
		return REJILLA_BURN_ERR;

	return REJILLA_BURN_RETRY;
}

static RejillaBurnResult
rejilla_burn_options_list_missing (RejillaPluginErrorType type,
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

static void
rejilla_burn_options_response (GtkDialog *dialog,
                               GtkResponseType response)
{
	RejillaBurnOptionsPrivate *priv;
	RejillaBurnResult result;
	GString *string;

	if (response != GTK_RESPONSE_OK)
		return;

	priv = REJILLA_BURN_OPTIONS_PRIVATE (dialog);
	gtk_widget_set_sensitive (GTK_WIDGET (dialog), FALSE);

	result = rejilla_session_foreach_plugin_error (REJILLA_BURN_SESSION (priv->session),
	                                               rejilla_burn_options_install_missing,
	                                               dialog);
	if (result == REJILLA_BURN_CANCEL)
		return;

	gtk_widget_set_sensitive (GTK_WIDGET (dialog), TRUE);

	if (result == REJILLA_BURN_OK)
		return;

	string = g_string_new (_("Please install the following manually and try again:"));
	rejilla_session_foreach_plugin_error (REJILLA_BURN_SESSION (priv->session),
	                                      rejilla_burn_options_list_missing,
	                                      string);

	rejilla_utils_message_dialog (GTK_WIDGET (dialog),
	                              _("All required applications and libraries are not installed."),
	                              string->str,
	                              GTK_MESSAGE_ERROR);
	g_string_free (string, TRUE);

	/* Cancel the rest */
	gtk_dialog_response (dialog, GTK_RESPONSE_CANCEL);
}

static void
rejilla_burn_options_class_init (RejillaBurnOptionsClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	GtkDialogClass *gtk_dialog_class = GTK_DIALOG_CLASS (klass);

	g_type_class_add_private (klass, sizeof (RejillaBurnOptionsPrivate));

	object_class->finalize = rejilla_burn_options_finalize;
	object_class->set_property = rejilla_burn_options_set_property;
	object_class->get_property = rejilla_burn_options_get_property;

	gtk_dialog_class->response = rejilla_burn_options_response;

	g_object_class_install_property (object_class,
					 PROP_SESSION,
					 g_param_spec_object ("session",
							      "The session",
							      "The session to work with",
							      REJILLA_TYPE_BURN_SESSION,
							      G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));
}

/**
 * rejilla_burn_options_new:
 * @session: a #RejillaSessionCfg object
 *
 *  Creates a new #RejillaBurnOptions object.
 *
 * Return value: a #GtkWidget object.
 **/

GtkWidget *
rejilla_burn_options_new (RejillaSessionCfg *session)
{
	return g_object_new (REJILLA_TYPE_BURN_OPTIONS, "session", session, NULL);
}
