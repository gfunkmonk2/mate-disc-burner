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

#include "burn-basics.h"
#include "burn-plugin-manager.h"
#include "rejilla-medium-selection-priv.h"
#include "rejilla-session-helper.h"

#include "rejilla-dest-selection.h"

#include "rejilla-drive.h"
#include "rejilla-medium.h"
#include "rejilla-volume.h"

#include "rejilla-burn-lib.h"
#include "rejilla-tags.h"
#include "rejilla-track.h"
#include "rejilla-session.h"
#include "rejilla-session-cfg.h"

typedef struct _RejillaDestSelectionPrivate RejillaDestSelectionPrivate;
struct _RejillaDestSelectionPrivate
{
	RejillaBurnSession *session;

	RejillaDrive *locked_drive;

	guint user_changed:1;
};

#define REJILLA_DEST_SELECTION_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REJILLA_TYPE_DEST_SELECTION, RejillaDestSelectionPrivate))

enum {
	PROP_0,
	PROP_SESSION
};

G_DEFINE_TYPE (RejillaDestSelection, rejilla_dest_selection, REJILLA_TYPE_MEDIUM_SELECTION);

static void
rejilla_dest_selection_lock (RejillaDestSelection *self,
			     gboolean locked)
{
	RejillaDestSelectionPrivate *priv;

	priv = REJILLA_DEST_SELECTION_PRIVATE (self);

	if (locked == (priv->locked_drive != NULL))
		return;

	gtk_widget_set_sensitive (GTK_WIDGET (self), (locked != TRUE));
	gtk_widget_queue_draw (GTK_WIDGET (self));

	if (priv->locked_drive) {
		rejilla_drive_unlock (priv->locked_drive);
		g_object_unref (priv->locked_drive);
		priv->locked_drive = NULL;
	}

	if (locked) {
		RejillaMedium *medium;

		medium = rejilla_medium_selection_get_active (REJILLA_MEDIUM_SELECTION (self));
		priv->locked_drive = rejilla_medium_get_drive (medium);

		if (priv->locked_drive) {
			g_object_ref (priv->locked_drive);
			rejilla_drive_lock (priv->locked_drive,
					    _("Ongoing burning process"),
					    NULL);
		}

		if (medium)
			g_object_unref (medium);
	}
}

static void
rejilla_dest_selection_valid_session (RejillaSessionCfg *session,
				      RejillaDestSelection *self)
{
	rejilla_medium_selection_update_media_string (REJILLA_MEDIUM_SELECTION (self));
}

static void
rejilla_dest_selection_output_changed (RejillaSessionCfg *session,
				       RejillaMedium *former,
				       RejillaDestSelection *self)
{
	RejillaDestSelectionPrivate *priv;
	RejillaMedium *medium;
	RejillaDrive *burner;

	priv = REJILLA_DEST_SELECTION_PRIVATE (self);

	/* make sure the current displayed drive reflects that */
	burner = rejilla_burn_session_get_burner (priv->session);
	medium = rejilla_medium_selection_get_active (REJILLA_MEDIUM_SELECTION (self));
	if (burner != rejilla_medium_get_drive (medium))
		rejilla_medium_selection_set_active (REJILLA_MEDIUM_SELECTION (self),
						     rejilla_drive_get_medium (burner));

	if (medium)
		g_object_unref (medium);
}

static void
rejilla_dest_selection_flags_changed (RejillaBurnSession *session,
                                      GParamSpec *pspec,
				      RejillaDestSelection *self)
{
	RejillaDestSelectionPrivate *priv;

	priv = REJILLA_DEST_SELECTION_PRIVATE (self);

	rejilla_dest_selection_lock (self, (rejilla_burn_session_get_flags (REJILLA_BURN_SESSION (priv->session)) & REJILLA_BURN_FLAG_MERGE) != 0);
}

static void
rejilla_dest_selection_medium_changed (RejillaMediumSelection *selection,
				       RejillaMedium *medium)
{
	RejillaDestSelectionPrivate *priv;

	priv = REJILLA_DEST_SELECTION_PRIVATE (selection);

	if (!priv->session)
		goto chain;

	if (!medium) {
	    	gtk_widget_set_sensitive (GTK_WIDGET (selection), FALSE);
		goto chain;
	}

	if (rejilla_medium_get_drive (medium) == rejilla_burn_session_get_burner (priv->session))
		goto chain;

	if (priv->locked_drive && priv->locked_drive != rejilla_medium_get_drive (medium)) {
		rejilla_medium_selection_set_active (selection, medium);
		goto chain;
	}

	rejilla_burn_session_set_burner (priv->session, rejilla_medium_get_drive (medium));
	gtk_widget_set_sensitive (GTK_WIDGET (selection), (priv->locked_drive == NULL));

chain:

	if (REJILLA_MEDIUM_SELECTION_CLASS (rejilla_dest_selection_parent_class)->medium_changed)
		REJILLA_MEDIUM_SELECTION_CLASS (rejilla_dest_selection_parent_class)->medium_changed (selection, medium);
}

static void
rejilla_dest_selection_user_change (RejillaDestSelection *selection,
                                    GParamSpec *pspec,
                                    gpointer NULL_data)
{
	gboolean shown = FALSE;
	RejillaDestSelectionPrivate *priv;

	/* we are only interested when the menu is shown */
	g_object_get (selection,
	              "popup-shown", &shown,
	              NULL);

	if (!shown)
		return;

	priv = REJILLA_DEST_SELECTION_PRIVATE (selection);
	priv->user_changed = TRUE;
}

static void
rejilla_dest_selection_medium_removed (GtkTreeModel *model,
                                       GtkTreePath *path,
                                       gpointer user_data)
{
	RejillaDestSelectionPrivate *priv;

	priv = REJILLA_DEST_SELECTION_PRIVATE (user_data);
	if (priv->user_changed)
		return;

	if (gtk_combo_box_get_active (GTK_COMBO_BOX (user_data)) == -1)
		rejilla_dest_selection_choose_best (REJILLA_DEST_SELECTION (user_data));
}

static void
rejilla_dest_selection_medium_added (GtkTreeModel *model,
                                     GtkTreePath *path,
                                     GtkTreeIter *iter,
                                     gpointer user_data)
{
	RejillaDestSelectionPrivate *priv;

	priv = REJILLA_DEST_SELECTION_PRIVATE (user_data);
	if (priv->user_changed)
		return;

	rejilla_dest_selection_choose_best (REJILLA_DEST_SELECTION (user_data));
}

static void
rejilla_dest_selection_init (RejillaDestSelection *object)
{
	RejillaDestSelectionPrivate *priv;
	GtkTreeModel *model;

	priv = REJILLA_DEST_SELECTION_PRIVATE (object);

	model = gtk_combo_box_get_model (GTK_COMBO_BOX (object));
	g_signal_connect (model,
	                  "row-inserted",
	                  G_CALLBACK (rejilla_dest_selection_medium_added),
	                  object);
	g_signal_connect (model,
	                  "row-deleted",
	                  G_CALLBACK (rejilla_dest_selection_medium_removed),
	                  object);

	/* Only show media on which we can write and which are in a burner.
	 * There is one exception though, when we're copying media and when the
	 * burning device is the same as the dest device. */
	rejilla_medium_selection_show_media_type (REJILLA_MEDIUM_SELECTION (object),
						  REJILLA_MEDIA_TYPE_WRITABLE|
						  REJILLA_MEDIA_TYPE_FILE);

	/* This is to know when the user changed it on purpose */
	g_signal_connect (object,
	                  "notify::popup-shown",
	                  G_CALLBACK (rejilla_dest_selection_user_change),
	                  NULL);
}

static void
rejilla_dest_selection_clean (RejillaDestSelection *self)
{
	RejillaDestSelectionPrivate *priv;

	priv = REJILLA_DEST_SELECTION_PRIVATE (self);

	if (priv->session) {
		g_signal_handlers_disconnect_by_func (priv->session,
						      rejilla_dest_selection_valid_session,
						      self);
		g_signal_handlers_disconnect_by_func (priv->session,
						      rejilla_dest_selection_output_changed,
						      self);
		g_signal_handlers_disconnect_by_func (priv->session,
						      rejilla_dest_selection_flags_changed,
						      self);

		g_object_unref (priv->session);
		priv->session = NULL;
	}

	if (priv->locked_drive) {
		rejilla_drive_unlock (priv->locked_drive);
		g_object_unref (priv->locked_drive);
		priv->locked_drive = NULL;
	}
}

static void
rejilla_dest_selection_finalize (GObject *object)
{
	rejilla_dest_selection_clean (REJILLA_DEST_SELECTION (object));
	G_OBJECT_CLASS (rejilla_dest_selection_parent_class)->finalize (object);
}

static goffset
_get_medium_free_space (RejillaMedium *medium,
                        goffset session_blocks)
{
	RejillaMedia media;
	goffset blocks = 0;

	media = rejilla_medium_get_status (medium);
	media = rejilla_burn_library_get_media_capabilities (media);

	/* NOTE: we always try to blank a medium when we can */
	rejilla_medium_get_free_space (medium,
				       NULL,
				       &blocks);

	if ((media & REJILLA_MEDIUM_REWRITABLE)
	&& blocks < session_blocks)
		rejilla_medium_get_capacity (medium,
		                             NULL,
		                             &blocks);

	return blocks;
}

static gboolean
rejilla_dest_selection_foreach_medium (RejillaMedium *medium,
				       gpointer callback_data)
{
	RejillaBurnSession *session;
	goffset session_blocks = 0;
	goffset burner_blocks = 0;
	goffset medium_blocks;
	RejillaDrive *burner;

	session = callback_data;
	burner = rejilla_burn_session_get_burner (session);
	if (!burner) {
		rejilla_burn_session_set_burner (session, rejilla_medium_get_drive (medium));
		return TRUE;
	}

	/* no need to deal with this case */
	if (rejilla_drive_get_medium (burner) == medium)
		return TRUE;

	/* The rule is:
	 * - blank media are our favourite since it avoids hiding/blanking data
	 * - take the medium that is closest to the size we need to burn
	 * - try to avoid a medium that is already our source for copying */
	/* NOTE: we could check if medium is bigger */
	if ((rejilla_burn_session_get_dest_media (session) & REJILLA_MEDIUM_BLANK)
	&&  (rejilla_medium_get_status (medium) & REJILLA_MEDIUM_BLANK))
		goto choose_closest_size;

	if (rejilla_burn_session_get_dest_media (session) & REJILLA_MEDIUM_BLANK)
		return TRUE;

	if (rejilla_medium_get_status (medium) & REJILLA_MEDIUM_BLANK) {
		rejilla_burn_session_set_burner (session, rejilla_medium_get_drive (medium));
		return TRUE;
	}

	/* In case it is the same source/same destination, choose it this new
	 * medium except if the medium is a file. */
	if (rejilla_burn_session_same_src_dest_drive (session)
	&& (rejilla_medium_get_status (medium) & REJILLA_MEDIUM_FILE) == 0) {
		rejilla_burn_session_set_burner (session, rejilla_medium_get_drive (medium));
		return TRUE;
	}

	/* Any possible medium is better than file even if it means copying to
	 * the same drive with a new medium later. */
	if (rejilla_drive_is_fake (burner)
	&& (rejilla_medium_get_status (medium) & REJILLA_MEDIUM_FILE) == 0) {
		rejilla_burn_session_set_burner (session, rejilla_medium_get_drive (medium));
		return TRUE;
	}


choose_closest_size:

	rejilla_burn_session_get_size (session, &session_blocks, NULL);
	medium_blocks = _get_medium_free_space (medium, session_blocks);

	if (medium_blocks - session_blocks <= 0)
		return TRUE;

	burner_blocks = _get_medium_free_space (rejilla_drive_get_medium (burner), session_blocks);
	if (burner_blocks - session_blocks <= 0)
		rejilla_burn_session_set_burner (session, rejilla_medium_get_drive (medium));
	else if (burner_blocks - session_blocks > medium_blocks - session_blocks)
		rejilla_burn_session_set_burner (session, rejilla_medium_get_drive (medium));

	return TRUE;
}

void
rejilla_dest_selection_choose_best (RejillaDestSelection *self)
{
	RejillaDestSelectionPrivate *priv;

	priv = REJILLA_DEST_SELECTION_PRIVATE (self);

	priv->user_changed = FALSE;
	if (!priv->session)
		return;

	if (!(rejilla_burn_session_get_flags (priv->session) & REJILLA_BURN_FLAG_MERGE)) {
		RejillaDrive *drive;

		/* Select the best fitting media */
		rejilla_medium_selection_foreach (REJILLA_MEDIUM_SELECTION (self),
						  rejilla_dest_selection_foreach_medium,
						  priv->session);

		drive = rejilla_burn_session_get_burner (REJILLA_BURN_SESSION (priv->session));
		if (drive)
			rejilla_medium_selection_set_active (REJILLA_MEDIUM_SELECTION (self),
							     rejilla_drive_get_medium (drive));
	}
}

void
rejilla_dest_selection_set_session (RejillaDestSelection *selection,
				    RejillaBurnSession *session)
{
	RejillaDestSelectionPrivate *priv;

	priv = REJILLA_DEST_SELECTION_PRIVATE (selection);

	if (priv->session)
		rejilla_dest_selection_clean (selection);

	if (!session)
		return;

	priv->session = g_object_ref (session);
	if (rejilla_burn_session_get_flags (session) & REJILLA_BURN_FLAG_MERGE) {
		RejillaDrive *drive;

		/* Prevent automatic resetting since a drive was set */
		priv->user_changed = TRUE;

		drive = rejilla_burn_session_get_burner (session);
		rejilla_medium_selection_set_active (REJILLA_MEDIUM_SELECTION (selection),
						     rejilla_drive_get_medium (drive));
	}
	else {
		RejillaDrive *burner;

		/* Only try to set a better drive if there isn't one already set */
		burner = rejilla_burn_session_get_burner (REJILLA_BURN_SESSION (priv->session));
		if (burner) {
			RejillaMedium *medium;

			/* Prevent automatic resetting since a drive was set */
			priv->user_changed = TRUE;

			medium = rejilla_drive_get_medium (burner);
			rejilla_medium_selection_set_active (REJILLA_MEDIUM_SELECTION (selection), medium);
		}
		else
			rejilla_dest_selection_choose_best (REJILLA_DEST_SELECTION (selection));
	}

	g_signal_connect (session,
			  "is-valid",
			  G_CALLBACK (rejilla_dest_selection_valid_session),
			  selection);
	g_signal_connect (session,
			  "output-changed",
			  G_CALLBACK (rejilla_dest_selection_output_changed),
			  selection);
	g_signal_connect (session,
			  "notify::flags",
			  G_CALLBACK (rejilla_dest_selection_flags_changed),
			  selection);

	rejilla_medium_selection_update_media_string (REJILLA_MEDIUM_SELECTION (selection));
}

static void
rejilla_dest_selection_set_property (GObject *object,
				     guint property_id,
				     const GValue *value,
				     GParamSpec *pspec)
{
	RejillaDestSelectionPrivate *priv;
	RejillaBurnSession *session;

	priv = REJILLA_DEST_SELECTION_PRIVATE (object);

	switch (property_id) {
	case PROP_SESSION: /* Readable and only writable at creation time */
		/* NOTE: no need to unref a potential previous session since
		 * it's only set at construct time */
		session = g_value_get_object (value);
		rejilla_dest_selection_set_session (REJILLA_DEST_SELECTION (object), session);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static void
rejilla_dest_selection_get_property (GObject *object,
				     guint property_id,
				     GValue *value,
				     GParamSpec *pspec)
{
	RejillaDestSelectionPrivate *priv;

	priv = REJILLA_DEST_SELECTION_PRIVATE (object);

	switch (property_id) {
	case PROP_SESSION:
		g_object_ref (priv->session);
		g_value_set_object (value, priv->session);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
	}
}

static gchar *
rejilla_dest_selection_get_output_path (RejillaDestSelection *self)
{
	gchar *path = NULL;
	RejillaImageFormat format;
	RejillaDestSelectionPrivate *priv;

	priv = REJILLA_DEST_SELECTION_PRIVATE (self);

	format = rejilla_burn_session_get_output_format (priv->session);
	switch (format) {
	case REJILLA_IMAGE_FORMAT_BIN:
		rejilla_burn_session_get_output (priv->session,
						 &path,
						 NULL);
		break;

	case REJILLA_IMAGE_FORMAT_CLONE:
	case REJILLA_IMAGE_FORMAT_CDRDAO:
	case REJILLA_IMAGE_FORMAT_CUE:
		rejilla_burn_session_get_output (priv->session,
						 NULL,
						 &path);
		break;

	default:
		break;
	}

	return path;
}

static gchar *
rejilla_dest_selection_format_medium_string (RejillaMediumSelection *selection,
					     RejillaMedium *medium)
{
	guint used;
	gchar *label;
	goffset blocks = 0;
	gchar *medium_name;
	gchar *size_string;
	RejillaMedia media;
	RejillaBurnFlag flags;
	goffset size_bytes = 0;
	goffset data_blocks = 0;
	goffset session_bytes = 0;
	RejillaTrackType *input = NULL;
	RejillaDestSelectionPrivate *priv;

	priv = REJILLA_DEST_SELECTION_PRIVATE (selection);

	if (!priv->session)
		return NULL;

	medium_name = rejilla_volume_get_name (REJILLA_VOLUME (medium));
	if (rejilla_medium_get_status (medium) & REJILLA_MEDIUM_FILE) {
		gchar *path;

		input = rejilla_track_type_new ();
		rejilla_burn_session_get_input_type (priv->session, input);

		/* There should be a special name for image in video context */
		if (rejilla_track_type_get_has_stream (input)
		&&  REJILLA_STREAM_FORMAT_HAS_VIDEO (rejilla_track_type_get_stream_format (input))) {
			RejillaImageFormat format;

			format = rejilla_burn_session_get_output_format (priv->session);
			if (format == REJILLA_IMAGE_FORMAT_CUE) {
				g_free (medium_name);
				if (rejilla_burn_session_tag_lookup_int (priv->session, REJILLA_VCD_TYPE) == REJILLA_SVCD)
					medium_name = g_strdup (_("SVCD image"));
				else
					medium_name = g_strdup (_("VCD image"));
			}
			else if (format == REJILLA_IMAGE_FORMAT_BIN) {
				g_free (medium_name);
				medium_name = g_strdup (_("Video DVD image"));
			}
		}
		rejilla_track_type_free (input);

		/* get the set path for the image file */
		path = rejilla_dest_selection_get_output_path (REJILLA_DEST_SELECTION (selection));
		if (!path)
			return medium_name;

		/* NOTE for translators: the first %s is medium_name ("File
		 * Image") and the second the path for the image file */
		label = g_strdup_printf (_("%s: \"%s\""),
					 medium_name,
					 path);
		g_free (medium_name);
		g_free (path);

		rejilla_medium_selection_update_used_space (REJILLA_MEDIUM_SELECTION (selection),
							    medium,
							    0);
		return label;
	}

	if (!priv->session) {
		g_free (medium_name);
		return NULL;
	}

	input = rejilla_track_type_new ();
	rejilla_burn_session_get_input_type (priv->session, input);
	if (rejilla_track_type_get_has_medium (input)) {
		RejillaMedium *src_medium;

		src_medium = rejilla_burn_session_get_src_medium (priv->session);
		if (src_medium == medium) {
			rejilla_track_type_free (input);

			/* Translators: this string is only used when the user
			 * wants to copy a disc using the same destination and
			 * source drive. It tells him that rejilla will use as
			 * destination disc a new one (once the source has been
			 * copied) which is to be inserted in the drive currently
			 * holding the source disc */
			label = g_strdup_printf (_("New disc in the burner holding the source disc"));
			g_free (medium_name);

			rejilla_medium_selection_update_used_space (REJILLA_MEDIUM_SELECTION (selection),
								    medium,
								    0);
			return label;
		}
	}

	media = rejilla_medium_get_status (medium);
	flags = rejilla_burn_session_get_flags (priv->session);
	rejilla_burn_session_get_size (priv->session,
				       &data_blocks,
				       &session_bytes);

	if (flags & (REJILLA_BURN_FLAG_MERGE|REJILLA_BURN_FLAG_APPEND))
		rejilla_medium_get_free_space (medium, &size_bytes, &blocks);
	else if (rejilla_burn_library_get_media_capabilities (media) & REJILLA_MEDIUM_REWRITABLE)
		rejilla_medium_get_capacity (medium, &size_bytes, &blocks);
	else
		rejilla_medium_get_free_space (medium, &size_bytes, &blocks);

	if (blocks) {
		used = data_blocks * 100 / blocks;
		if (data_blocks && !used)
			used = 1;

		used = MIN (100, used);
	}
	else
		used = 0;

	rejilla_medium_selection_update_used_space (REJILLA_MEDIUM_SELECTION (selection),
						    medium,
						    used);
	blocks -= data_blocks;
	if (blocks <= 0) {
		rejilla_track_type_free (input);

		/* NOTE for translators, the first %s is the medium name */
		label = g_strdup_printf (_("%s: not enough free space"), medium_name);
		g_free (medium_name);
		return label;
	}

	/* format the size */
	if (rejilla_track_type_get_has_stream (input)
	&& REJILLA_STREAM_FORMAT_HAS_VIDEO (rejilla_track_type_get_stream_format (input))) {
		guint64 free_time;

		/* This is an embarassing problem: this is an approximation
		 * based on the fact that 2 hours = 4.3GiB */
		free_time = size_bytes - session_bytes;
		free_time = free_time * 72000LL / 47LL;
		size_string = rejilla_units_get_time_string (free_time,
							     TRUE,
							     TRUE);
	}
	else if (rejilla_track_type_get_has_stream (input)
	|| (rejilla_track_type_get_has_medium (input)
	&& (rejilla_track_type_get_medium_type (input) & REJILLA_MEDIUM_HAS_AUDIO)))
		size_string = rejilla_units_get_time_string (REJILLA_SECTORS_TO_DURATION (blocks),
							     TRUE,
							     TRUE);
	else
		size_string = g_format_size_for_display (size_bytes - session_bytes);

	rejilla_track_type_free (input);

	/* NOTE for translators: the first %s is the medium name, the second %s
	 * is its available free space. "Free" here is the free space available. */
	label = g_strdup_printf (_("%s: %s of free space"), medium_name, size_string);
	g_free (medium_name);
	g_free (size_string);

	return label;
}

static void
rejilla_dest_selection_class_init (RejillaDestSelectionClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	RejillaMediumSelectionClass *medium_selection_class = REJILLA_MEDIUM_SELECTION_CLASS (klass);

	g_type_class_add_private (klass, sizeof (RejillaDestSelectionPrivate));

	object_class->finalize = rejilla_dest_selection_finalize;
	object_class->set_property = rejilla_dest_selection_set_property;
	object_class->get_property = rejilla_dest_selection_get_property;

	medium_selection_class->format_medium_string = rejilla_dest_selection_format_medium_string;
	medium_selection_class->medium_changed = rejilla_dest_selection_medium_changed;
	g_object_class_install_property (object_class,
					 PROP_SESSION,
					 g_param_spec_object ("session",
							      "The session",
							      "The session to work with",
							      REJILLA_TYPE_BURN_SESSION,
							      G_PARAM_READWRITE));
}

GtkWidget *
rejilla_dest_selection_new (RejillaBurnSession *session)
{
	return g_object_new (REJILLA_TYPE_DEST_SELECTION,
			     "session", session,
			     NULL);
}
