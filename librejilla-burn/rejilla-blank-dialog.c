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
#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>

#include <canberra-gtk.h>

#include "rejilla-misc.h"

#include "burn-basics.h"

#include "rejilla-session.h"
#include "rejilla-session-helper.h"
#include "rejilla-burn.h"

#include "burn-plugin-manager.h"
#include "rejilla-tool-dialog.h"
#include "rejilla-tool-dialog-private.h"
#include "rejilla-blank-dialog.h"

G_DEFINE_TYPE (RejillaBlankDialog, rejilla_blank_dialog, REJILLA_TYPE_TOOL_DIALOG);

struct RejillaBlankDialogPrivate {
	RejillaBurnSession *session;

	GtkWidget *fast;

	guint caps_sig;
	guint output_sig;

	guint fast_saved;
	guint dummy_saved;
};
typedef struct RejillaBlankDialogPrivate RejillaBlankDialogPrivate;

#define REJILLA_BLANK_DIALOG_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REJILLA_TYPE_BLANK_DIALOG, RejillaBlankDialogPrivate))

static RejillaToolDialogClass *parent_class = NULL;

static guint
rejilla_blank_dialog_set_button (RejillaBurnSession *session,
				 guint saved,
				 GtkWidget *button,
				 RejillaBurnFlag flag,
				 RejillaBurnFlag supported,
				 RejillaBurnFlag compulsory)
{
	if (flag & supported) {
		if (compulsory & flag) {
			if (gtk_widget_get_sensitive (button))
				saved = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

			gtk_widget_set_sensitive (button, FALSE);
			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), TRUE);

			rejilla_burn_session_add_flag (session, flag);
		}
		else {
			if (!gtk_widget_get_sensitive (button)) {
				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), saved);

				if (saved)
					rejilla_burn_session_add_flag (session, flag);
				else
					rejilla_burn_session_remove_flag (session, flag);
			}

			gtk_widget_set_sensitive (button, TRUE);
		}
	}
	else {
		if (gtk_widget_get_sensitive (button))
			saved = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (button));

		gtk_widget_set_sensitive (button, FALSE);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), FALSE);

		rejilla_burn_session_remove_flag (session, flag);
	}

	return saved;
}

static void
rejilla_blank_dialog_device_opts_setup (RejillaBlankDialog *self)
{
	RejillaBurnFlag supported;
	RejillaBurnFlag compulsory;
	RejillaBlankDialogPrivate *priv;

	priv = REJILLA_BLANK_DIALOG_PRIVATE (self);

	/* set the options */
	rejilla_burn_session_get_blank_flags (priv->session,
					      &supported,
					      &compulsory);

	priv->fast_saved = rejilla_blank_dialog_set_button (priv->session,
							    priv->fast_saved,
							    priv->fast,
							    REJILLA_BURN_FLAG_FAST_BLANK,
							    supported,
							    compulsory);

	/* This must be done afterwards, once the session flags were updated */
	rejilla_tool_dialog_set_valid (REJILLA_TOOL_DIALOG (self),
				       (rejilla_burn_session_can_blank (priv->session) == REJILLA_BURN_OK));
}

static void
rejilla_blank_dialog_caps_changed (RejillaPluginManager *manager,
				   RejillaBlankDialog *dialog)
{
	rejilla_blank_dialog_device_opts_setup (dialog);
}

static void
rejilla_blank_dialog_output_changed (RejillaBurnSession *session,
				     RejillaMedium *former,
				     RejillaBlankDialog *dialog)
{
	rejilla_blank_dialog_device_opts_setup (dialog);
}

static void
rejilla_blank_dialog_fast_toggled (GtkToggleButton *toggle,
				   RejillaBlankDialog *self)
{
	RejillaBlankDialogPrivate *priv;

	priv = REJILLA_BLANK_DIALOG_PRIVATE (self);
	if (gtk_toggle_button_get_active (toggle))
		rejilla_burn_session_add_flag (priv->session, REJILLA_BURN_FLAG_FAST_BLANK);
	else
		rejilla_burn_session_remove_flag (priv->session, REJILLA_BURN_FLAG_FAST_BLANK);
}

static void
rejilla_blank_dialog_drive_changed (RejillaToolDialog *dialog,
				    RejillaMedium *medium)
{
	RejillaBlankDialogPrivate *priv;
	RejillaDrive *drive;

	priv = REJILLA_BLANK_DIALOG_PRIVATE (dialog);

	if (medium)
		drive = rejilla_medium_get_drive (medium);
	else
		drive = NULL;

	/* it can happen that the drive changed while initializing and that
	 * session hasn't been created yet. */
	if (priv->session)
		rejilla_burn_session_set_burner (priv->session, drive);
}

static gboolean
rejilla_blank_dialog_activate (RejillaToolDialog *dialog,
			       RejillaMedium *medium)
{
	RejillaBlankDialogPrivate *priv;
	RejillaBlankDialog *self;
	RejillaBurnResult result;
	GError *error = NULL;
	RejillaBurn *burn;

	self = REJILLA_BLANK_DIALOG (dialog);
	priv = REJILLA_BLANK_DIALOG_PRIVATE (self);

	burn = rejilla_tool_dialog_get_burn (dialog);
	rejilla_burn_session_start (priv->session);
	result = rejilla_burn_blank (burn,
				     priv->session,
				     &error);

	/* Tell the user the result of the operation */
	if (result == REJILLA_BURN_ERR || error) {
		GtkResponseType answer;
		GtkWidget *message;
		GtkWidget *button;

		message =  gtk_message_dialog_new (GTK_WINDOW (self),
						   GTK_DIALOG_DESTROY_WITH_PARENT|
						   GTK_DIALOG_MODAL,
						   GTK_MESSAGE_ERROR,
						   GTK_BUTTONS_CLOSE,
						   _("Error while blanking."));

		gtk_window_set_icon_name (GTK_WINDOW (message),
					  gtk_window_get_icon_name (GTK_WINDOW (self)));

		button = rejilla_utils_make_button (_("Blank _Again"),
						    NULL,
						    "media-optical-blank",
						    GTK_ICON_SIZE_BUTTON);
		gtk_widget_show (button);
		gtk_dialog_add_action_widget (GTK_DIALOG (message),
					      button,
					      GTK_RESPONSE_OK);

		if (error) {
			gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (message),
								  "%s.",
								  error->message);
			g_error_free (error);
		}
		else
			gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (message),
								  _("Unknown error."));

		answer = gtk_dialog_run (GTK_DIALOG (message));
		gtk_widget_destroy (message);

		if (answer == GTK_RESPONSE_OK) {
			rejilla_blank_dialog_device_opts_setup (self);
			return FALSE;
		}
	}
	else if (result == REJILLA_BURN_OK) {
		GtkResponseType answer;
		GtkWidget *message;
		GtkWidget *button;

		message = gtk_message_dialog_new (GTK_WINDOW (self),
						  GTK_DIALOG_DESTROY_WITH_PARENT|
						  GTK_DIALOG_MODAL,
						  GTK_MESSAGE_INFO,
						  GTK_BUTTONS_NONE,
						  _("The disc was successfully blanked."));

		gtk_window_set_icon_name (GTK_WINDOW (message),
					  gtk_window_get_icon_name (GTK_WINDOW (self)));

		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (message),
							  _("The disc is ready for use."));

		button = rejilla_utils_make_button (_("Blank _Again"),
						    NULL,
						    "media-optical-blank",
						    GTK_ICON_SIZE_BUTTON);
		gtk_widget_show (button);
		gtk_dialog_add_action_widget (GTK_DIALOG (message),
					      button,
					      GTK_RESPONSE_OK);

		gtk_dialog_add_button (GTK_DIALOG (message),
				       GTK_STOCK_CLOSE,
				       GTK_RESPONSE_CLOSE);

		gtk_widget_show (GTK_WIDGET (message));
		ca_gtk_play_for_widget (GTK_WIDGET (message), 0,
					CA_PROP_EVENT_ID, "complete-media-format",
					CA_PROP_EVENT_DESCRIPTION, _("The disc was successfully blanked."),
					NULL);

		answer = gtk_dialog_run (GTK_DIALOG (message));
		gtk_widget_destroy (message);

		if (answer == GTK_RESPONSE_OK) {
			rejilla_blank_dialog_device_opts_setup (self);
			return FALSE;
		}
	}
	else if (result == REJILLA_BURN_NOT_SUPPORTED) {
		g_warning ("operation not supported");
	}
	else if (result == REJILLA_BURN_NOT_READY) {
		g_warning ("operation not ready");
	}
	else if (result == REJILLA_BURN_NOT_RUNNING) {
		g_warning ("job not running");
	}
	else if (result == REJILLA_BURN_RUNNING) {
		g_warning ("job running");
	}

	return TRUE;
}

static void
rejilla_blank_dialog_finalize (GObject *object)
{
	RejillaBlankDialogPrivate *priv;

	priv = REJILLA_BLANK_DIALOG_PRIVATE (object);

	if (priv->caps_sig) {
		RejillaPluginManager *manager;

		manager = rejilla_plugin_manager_get_default ();
		g_signal_handler_disconnect (manager, priv->caps_sig);
		priv->caps_sig = 0;
	}

	if (priv->output_sig) {
		g_signal_handler_disconnect (priv->session, priv->output_sig);
		priv->output_sig = 0;
	}

	if (priv->session) {
		g_object_unref (priv->session);
		priv->session = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
rejilla_blank_dialog_class_init (RejillaBlankDialogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	RejillaToolDialogClass *tool_dialog_class = REJILLA_TOOL_DIALOG_CLASS (klass);

	g_type_class_add_private (klass, sizeof (RejillaBlankDialogPrivate));

	parent_class = g_type_class_peek_parent (klass);

	object_class->finalize = rejilla_blank_dialog_finalize;

	tool_dialog_class->activate = rejilla_blank_dialog_activate;
	tool_dialog_class->medium_changed = rejilla_blank_dialog_drive_changed;
}

static void
rejilla_blank_dialog_init (RejillaBlankDialog *obj)
{
	RejillaBlankDialogPrivate *priv;
	RejillaPluginManager *manager;
	RejillaMedium *medium;
	RejillaDrive *drive;

	priv = REJILLA_BLANK_DIALOG_PRIVATE (obj);

	rejilla_tool_dialog_set_button (REJILLA_TOOL_DIALOG (obj),
					/* Translators: This is a verb, an action */
					_("_Blank"),
					NULL,
					"media-optical-blank");

	/* only media that can be rewritten with or without data */
	rejilla_tool_dialog_set_medium_type_shown (REJILLA_TOOL_DIALOG (obj),
						   REJILLA_MEDIA_TYPE_REWRITABLE);

	medium = rejilla_tool_dialog_get_medium (REJILLA_TOOL_DIALOG (obj));
	drive = rejilla_medium_get_drive (medium);

	priv->session = rejilla_burn_session_new ();
	rejilla_burn_session_set_flags (priv->session,
				        REJILLA_BURN_FLAG_EJECT|
				        REJILLA_BURN_FLAG_NOGRACE);
	rejilla_burn_session_set_burner (priv->session, drive);

	if (medium)
		g_object_unref (medium);

	priv->output_sig = g_signal_connect (priv->session,
					     "output-changed",
					     G_CALLBACK (rejilla_blank_dialog_output_changed),
					     obj);

	manager = rejilla_plugin_manager_get_default ();
	priv->caps_sig = g_signal_connect (manager,
					   "caps-changed",
					   G_CALLBACK (rejilla_blank_dialog_caps_changed),
					   obj);

	priv->fast = gtk_check_button_new_with_mnemonic (_("_Fast blanking"));
	gtk_widget_set_tooltip_text (priv->fast, _("Activate fast blanking, as opposed to a longer, thorough blanking"));
	g_signal_connect (priv->fast,
			  "clicked",
			  G_CALLBACK (rejilla_blank_dialog_fast_toggled),
			  obj);

	rejilla_tool_dialog_pack_options (REJILLA_TOOL_DIALOG (obj),
					  priv->fast,
					  NULL);

	rejilla_blank_dialog_device_opts_setup (obj);

	/* if fast blank is supported check it by default */
	if (gtk_widget_is_sensitive (priv->fast))
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->fast), TRUE);
}

/**
 * rejilla_blank_dialog_new:
 *
 * Creates a new #RejillaBlankDialog object
 *
 * Return value: a #RejillaBlankDialog. Unref when it is not needed anymore.
 **/
RejillaBlankDialog *
rejilla_blank_dialog_new ()
{
	RejillaBlankDialog *obj;

	obj = REJILLA_BLANK_DIALOG (g_object_new (REJILLA_TYPE_BLANK_DIALOG,
						  "title", _("Disc Blanking"),
						  NULL));
	return obj;
}
