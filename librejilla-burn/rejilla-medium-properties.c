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

#include <string.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>

#include "rejilla-medium-properties.h"
#include "rejilla-drive-properties.h"
#include "rejilla-image-properties.h"
#include "rejilla-session-cfg.h"
#include "rejilla-session-helper.h"

#include "burn-basics.h"
#include "rejilla-track.h"
#include "rejilla-medium.h"
#include "rejilla-session.h"
#include "burn-image-format.h"

typedef struct _RejillaMediumPropertiesPrivate RejillaMediumPropertiesPrivate;
struct _RejillaMediumPropertiesPrivate
{
	RejillaBurnSession *session;

	GtkWidget *medium_prop;
};

#define REJILLA_MEDIUM_PROPERTIES_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REJILLA_TYPE_MEDIUM_PROPERTIES, RejillaMediumPropertiesPrivate))

enum {
	PROP_0,
	PROP_SESSION
};

G_DEFINE_TYPE (RejillaMediumProperties, rejilla_medium_properties, GTK_TYPE_BUTTON);

static void
rejilla_medium_properties_drive_properties (RejillaMediumProperties *self)
{
	RejillaMediumPropertiesPrivate *priv;
	GtkWidget *medium_prop;
	RejillaDrive *drive;
	GtkWidget *toplevel;
	gchar *display_name;
	GtkWidget *dialog;
	GtkWidget *box;
	gchar *header;

	priv = REJILLA_MEDIUM_PROPERTIES_PRIVATE (self);

	/* Build dialog */
	medium_prop = rejilla_drive_properties_new (REJILLA_SESSION_CFG (priv->session));
	gtk_widget_show (medium_prop);

	drive = rejilla_burn_session_get_burner (priv->session);
	display_name = rejilla_drive_get_display_name (drive);

	header = g_strdup_printf (_("Properties of %s"), display_name);
	g_free (display_name);

	toplevel = gtk_widget_get_toplevel (GTK_WIDGET (self));
	dialog = gtk_dialog_new_with_buttons (header,
					      GTK_WINDOW (toplevel),
					      GTK_DIALOG_MODAL|
					      GTK_DIALOG_DESTROY_WITH_PARENT,
					      GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
					      NULL);
	g_free (header);

	gtk_window_set_icon_name (GTK_WINDOW (dialog),
	                          gtk_window_get_icon_name (GTK_WINDOW (toplevel)));

	box = gtk_dialog_get_content_area (GTK_DIALOG (dialog));
	gtk_box_pack_start (GTK_BOX (box), medium_prop, TRUE, TRUE, 0);

	/* launch the dialog */
	priv->medium_prop = dialog;
	gtk_widget_show (dialog);
	gtk_dialog_run (GTK_DIALOG (dialog));
	priv->medium_prop = NULL;
	gtk_widget_destroy (dialog);
}

static gboolean
rejilla_medium_properties_wrong_extension (RejillaSessionCfg *session,
					   RejillaMediumProperties *self)
{
	GtkWidget *dialog;
	GtkWidget *toplevel;
	GtkResponseType answer;
	RejillaMediumPropertiesPrivate *priv;

	priv = REJILLA_MEDIUM_PROPERTIES_PRIVATE (self);

	toplevel = gtk_widget_get_toplevel (GTK_WIDGET (self));
	dialog = gtk_message_dialog_new (GTK_WINDOW (toplevel),
					 GTK_DIALOG_DESTROY_WITH_PARENT |
					 GTK_DIALOG_MODAL,
					 GTK_MESSAGE_WARNING,
					 GTK_BUTTONS_NONE,
					 _("Do you really want to keep the current extension for the disc image name?"));

	gtk_window_set_icon_name (GTK_WINDOW (dialog),
	                          gtk_window_get_icon_name (GTK_WINDOW (toplevel)));
		
	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
						  _("If you choose to keep it, programs may not be able to recognize the file type properly."));

	gtk_dialog_add_button (GTK_DIALOG (dialog),
			       _("_Keep Current Extension"),
			       GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button (GTK_DIALOG (dialog),
			       _("Change _Extension"),
			       GTK_RESPONSE_YES);

	answer = gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);

	if (answer == GTK_RESPONSE_YES)
		return TRUE;

	return FALSE;
}

static void
rejilla_medium_properties_image_properties (RejillaMediumProperties *self)
{
	RejillaMediumPropertiesPrivate *priv;
	GtkWindow *toplevel;

	priv = REJILLA_MEDIUM_PROPERTIES_PRIVATE (self);

	priv->medium_prop = rejilla_image_properties_new ();
	rejilla_image_properties_set_session (REJILLA_IMAGE_PROPERTIES (priv->medium_prop),
					      REJILLA_SESSION_CFG (priv->session));

	gtk_dialog_add_buttons (GTK_DIALOG (priv->medium_prop),
				GTK_STOCK_CLOSE, GTK_RESPONSE_OK,
				NULL);

	toplevel = GTK_WINDOW (gtk_widget_get_toplevel (GTK_WIDGET (self)));
	gtk_window_set_transient_for (GTK_WINDOW (priv->medium_prop), GTK_WINDOW (toplevel));
	gtk_window_set_destroy_with_parent (GTK_WINDOW (priv->medium_prop), TRUE);
	gtk_window_set_position (GTK_WINDOW (toplevel), GTK_WIN_POS_CENTER_ON_PARENT);

	gtk_window_set_icon_name (GTK_WINDOW (priv->medium_prop),
	                          gtk_window_get_icon_name (GTK_WINDOW (toplevel)));

	/* and here we go ... run the thing */
	gtk_widget_show (priv->medium_prop);
	gtk_dialog_run (GTK_DIALOG (priv->medium_prop));
	gtk_widget_destroy (priv->medium_prop);
	priv->medium_prop = NULL;
}

static void
rejilla_medium_properties_clicked (GtkButton *button)
{
	RejillaMediumPropertiesPrivate *priv;
	RejillaDrive *drive;

	priv = REJILLA_MEDIUM_PROPERTIES_PRIVATE (button);

	drive = rejilla_burn_session_get_burner (priv->session);
	if (!drive)
		return;

	if (rejilla_drive_is_fake (drive))
		rejilla_medium_properties_image_properties (REJILLA_MEDIUM_PROPERTIES (button));
	else
		rejilla_medium_properties_drive_properties (REJILLA_MEDIUM_PROPERTIES (button));
}

static void
rejilla_medium_properties_output_changed (RejillaBurnSession *session,
					  RejillaMedium *former,
					  RejillaMediumProperties *self)
{
	RejillaMediumPropertiesPrivate *priv;
	RejillaDrive *burner;

	priv = REJILLA_MEDIUM_PROPERTIES_PRIVATE (self);

	/* make sure that's an actual change of medium
	 * as it could also be a change of path for image */
	burner = rejilla_burn_session_get_burner (session);
	if (former == rejilla_drive_get_medium (burner))
		return;

	/* close properties dialog */
	if (priv->medium_prop) {
		gtk_dialog_response (GTK_DIALOG (priv->medium_prop),
				     GTK_RESPONSE_CANCEL);
		priv->medium_prop = NULL;
	}
}

static void
rejilla_medium_properties_init (RejillaMediumProperties *object)
{
	RejillaMediumPropertiesPrivate *priv;

	priv = REJILLA_MEDIUM_PROPERTIES_PRIVATE (object);

	gtk_widget_set_tooltip_text (GTK_WIDGET (object), _("Configure recording options"));
}

static void
rejilla_medium_properties_finalize (GObject *object)
{
	RejillaMediumPropertiesPrivate *priv;

	priv = REJILLA_MEDIUM_PROPERTIES_PRIVATE (object);

	if (priv->session) {
		g_signal_handlers_disconnect_by_func (priv->session,
						      rejilla_medium_properties_output_changed,
						      object);
		g_signal_handlers_disconnect_by_func (priv->session,
						      rejilla_medium_properties_wrong_extension,
						      object);
		g_object_unref (priv->session);
		priv->session = NULL;
	}

	G_OBJECT_CLASS (rejilla_medium_properties_parent_class)->finalize (object);
}

static void
rejilla_medium_properties_set_property (GObject *object,
					guint property_id,
					const GValue *value,
					GParamSpec *pspec)
{
	RejillaMediumPropertiesPrivate *priv;
	RejillaBurnSession *session;

	priv = REJILLA_MEDIUM_PROPERTIES_PRIVATE (object);

	switch (property_id) {
	case PROP_SESSION:
		if (priv->session)
			g_object_unref (priv->session);

		session = g_value_get_object (value);

		/* NOTE: no need to unref a potential previous session since
		 * it's only set at construct time */
		priv->session = session;
		g_object_ref (session);

		g_signal_connect (session,
				  "output-changed",
				  G_CALLBACK (rejilla_medium_properties_output_changed),
				  object);
		g_signal_connect (session,
				  "wrong-extension",
				  G_CALLBACK (rejilla_medium_properties_wrong_extension),
				  object);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
rejilla_medium_properties_get_property (GObject *object,
				     guint property_id,
				     GValue *value,
				     GParamSpec *pspec)
{
	RejillaMediumPropertiesPrivate *priv;

	priv = REJILLA_MEDIUM_PROPERTIES_PRIVATE (object);

	switch (property_id) {
	case PROP_SESSION:
		g_value_set_object (value, priv->session);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
rejilla_medium_properties_class_init (RejillaMediumPropertiesClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GtkButtonClass* button_class = GTK_BUTTON_CLASS (klass);

	g_type_class_add_private (klass, sizeof (RejillaMediumPropertiesPrivate));

	object_class->finalize = rejilla_medium_properties_finalize;
	object_class->set_property = rejilla_medium_properties_set_property;
	object_class->get_property = rejilla_medium_properties_get_property;

	button_class->clicked = rejilla_medium_properties_clicked;
	g_object_class_install_property (object_class,
					 PROP_SESSION,
					 g_param_spec_object ("session",
							      "The session to work with",
							      "The session to work with",
							      REJILLA_TYPE_SESSION_CFG,
							      G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));

}

GtkWidget *
rejilla_medium_properties_new (RejillaSessionCfg *session)
{
	return g_object_new (REJILLA_TYPE_MEDIUM_PROPERTIES,
			     "session", session,
			     "use-stock", TRUE,
			     "label", GTK_STOCK_PROPERTIES,
			     "focus-on-click", FALSE,
			     NULL);
}
