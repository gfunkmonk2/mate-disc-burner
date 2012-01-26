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

#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>

#include "rejilla-src-selection.h"
#include "rejilla-medium-selection.h"

#include "rejilla-track.h"
#include "rejilla-session.h"
#include "rejilla-track-disc.h"

#include "rejilla-drive.h"
#include "rejilla-volume.h"

typedef struct _RejillaSrcSelectionPrivate RejillaSrcSelectionPrivate;
struct _RejillaSrcSelectionPrivate
{
	RejillaBurnSession *session;
	RejillaTrackDisc *track;
};

#define REJILLA_SRC_SELECTION_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REJILLA_TYPE_SRC_SELECTION, RejillaSrcSelectionPrivate))

enum {
	PROP_0,
	PROP_SESSION
};

G_DEFINE_TYPE (RejillaSrcSelection, rejilla_src_selection, REJILLA_TYPE_MEDIUM_SELECTION);

static void
rejilla_src_selection_medium_changed (RejillaMediumSelection *selection,
				      RejillaMedium *medium)
{
	RejillaSrcSelectionPrivate *priv;
	RejillaDrive *drive = NULL;

	priv = REJILLA_SRC_SELECTION_PRIVATE (selection);

	if (priv->session && priv->track) {
		drive = rejilla_medium_get_drive (medium);
		rejilla_track_disc_set_drive (priv->track, drive);
	}

	gtk_widget_set_sensitive (GTK_WIDGET (selection), drive != NULL);

	if (REJILLA_MEDIUM_SELECTION_CLASS (rejilla_src_selection_parent_class)->medium_changed)
		REJILLA_MEDIUM_SELECTION_CLASS (rejilla_src_selection_parent_class)->medium_changed (selection, medium);
}

GtkWidget *
rejilla_src_selection_new (RejillaBurnSession *session)
{
	g_return_val_if_fail (REJILLA_IS_BURN_SESSION (session), NULL);
	return GTK_WIDGET (g_object_new (REJILLA_TYPE_SRC_SELECTION,
					 "session", session,
					 NULL));
}

static void
rejilla_src_selection_init (RejillaSrcSelection *object)
{
	/* only show media with something to be read on them */
	rejilla_medium_selection_show_media_type (REJILLA_MEDIUM_SELECTION (object),
						  REJILLA_MEDIA_TYPE_AUDIO|
						  REJILLA_MEDIA_TYPE_DATA);
}

static void
rejilla_src_selection_finalize (GObject *object)
{
	RejillaSrcSelectionPrivate *priv;

	priv = REJILLA_SRC_SELECTION_PRIVATE (object);

	if (priv->session) {
		g_object_unref (priv->session);
		priv->session = NULL;
	}

	if (priv->track) {
		g_object_unref (priv->track);
		priv->track = NULL;
	}

	G_OBJECT_CLASS (rejilla_src_selection_parent_class)->finalize (object);
}

static RejillaTrack *
_get_session_disc_track (RejillaBurnSession *session)
{
	RejillaTrack *track;
	GSList *tracks;
	guint num;

	tracks = rejilla_burn_session_get_tracks (session);
	num = g_slist_length (tracks);

	if (num != 1)
		return NULL;

	track = tracks->data;
	if (REJILLA_IS_TRACK_DISC (track))
		return track;

	return NULL;
}

static void
rejilla_src_selection_set_property (GObject *object,
				    guint property_id,
				    const GValue *value,
				    GParamSpec *pspec)
{
	RejillaSrcSelectionPrivate *priv;
	RejillaBurnSession *session;

	priv = REJILLA_SRC_SELECTION_PRIVATE (object);

	switch (property_id) {
	case PROP_SESSION:
	{
		RejillaMedium *medium;
		RejillaDrive *drive;
		RejillaTrack *track;

		session = g_value_get_object (value);

		priv->session = session;
		g_object_ref (session);

		if (priv->track)
			g_object_unref (priv->track);

		/* See if there was a track set; if so then use it */
		track = _get_session_disc_track (session);
		if (track) {
			priv->track = REJILLA_TRACK_DISC (track);
			g_object_ref (track);
		}
		else {
			priv->track = rejilla_track_disc_new ();
			rejilla_burn_session_add_track (priv->session,
							REJILLA_TRACK (priv->track),
							NULL);
		}

		drive = rejilla_track_disc_get_drive (priv->track);
		medium = rejilla_drive_get_medium (drive);
		if (!medium) {
			/* No medium set use set session medium source as the
			 * one currently active in the selection widget */
			medium = rejilla_medium_selection_get_active (REJILLA_MEDIUM_SELECTION (object));
			rejilla_src_selection_medium_changed (REJILLA_MEDIUM_SELECTION (object), medium);
		}
		else
			rejilla_medium_selection_set_active (REJILLA_MEDIUM_SELECTION (object), medium);

		break;
	}

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
rejilla_src_selection_get_property (GObject *object,
				    guint property_id,
				    GValue *value,
				    GParamSpec *pspec)
{
	RejillaSrcSelectionPrivate *priv;

	priv = REJILLA_SRC_SELECTION_PRIVATE (object);

	switch (property_id) {
	case PROP_SESSION:
		g_value_set_object (value, priv->session);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
rejilla_src_selection_class_init (RejillaSrcSelectionClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	RejillaMediumSelectionClass *medium_selection_class = REJILLA_MEDIUM_SELECTION_CLASS (klass);

	g_type_class_add_private (klass, sizeof (RejillaSrcSelectionPrivate));

	object_class->finalize = rejilla_src_selection_finalize;
	object_class->set_property = rejilla_src_selection_set_property;
	object_class->get_property = rejilla_src_selection_get_property;

	medium_selection_class->medium_changed = rejilla_src_selection_medium_changed;

	g_object_class_install_property (object_class,
					 PROP_SESSION,
					 g_param_spec_object ("session",
							      "The session to work with",
							      "The session to work with",
							      REJILLA_TYPE_BURN_SESSION,
							      G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));
}
