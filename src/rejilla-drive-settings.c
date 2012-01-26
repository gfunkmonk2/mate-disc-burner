/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Rejilla
 * Copyright (C) Philippe Rouquier 2005-2010 <bonfire-app@wanadoo.fr>
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

#include "rejilla-drive-settings.h"
#include "rejilla-session.h"
#include "rejilla-session-helper.h"
#include "rejilla-drive-properties.h"

typedef struct _RejillaDriveSettingsPrivate RejillaDriveSettingsPrivate;
struct _RejillaDriveSettingsPrivate
{
	RejillaMedia dest_media;
	RejillaDrive *dest_drive;
	RejillaTrackType *src_type;

	GSettings *settings;
	GSettings *config_settings;
	RejillaBurnSession *session;
};

#define REJILLA_DRIVE_SETTINGS_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REJILLA_TYPE_DRIVE_SETTINGS, RejillaDriveSettingsPrivate))

#define REJILLA_SCHEMA_DRIVES			"org.mate.rejilla.drives"
#define REJILLA_DRIVE_PROPERTIES_PATH		"/apps/rejilla/drives/"
#define REJILLA_PROPS_FLAGS			"flags"
#define REJILLA_PROPS_SPEED			"speed"

#define REJILLA_SCHEMA_CONFIG			"org.mate.rejilla.config"
#define REJILLA_PROPS_TMP_DIR			"tmpdir"

#define REJILLA_DEST_SAVED_FLAGS		(REJILLA_DRIVE_PROPERTIES_FLAGS|REJILLA_BURN_FLAG_MULTI)

G_DEFINE_TYPE (RejillaDriveSettings, rejilla_drive_settings, G_TYPE_OBJECT);

static GVariant *
rejilla_drive_settings_set_mapping_speed (const GValue *value,
                                          const GVariantType *variant_type,
                                          gpointer user_data)
{
	return g_variant_new_int32 (g_value_get_int64 (value) / 1000);
}

static gboolean
rejilla_drive_settings_get_mapping_speed (GValue *value,
                                          GVariant *variant,
                                          gpointer user_data)
{
	if (!g_variant_get_int32 (variant)) {
		RejillaDriveSettingsPrivate *priv;
		RejillaMedium *medium;
		RejillaDrive *drive;

		priv = REJILLA_DRIVE_SETTINGS_PRIVATE (user_data);
		drive = rejilla_burn_session_get_burner (priv->session);
		medium = rejilla_drive_get_medium (drive);

		/* Must not be NULL since we only bind when a medium is available */
		g_assert (medium != NULL);

		g_value_set_int64 (value, rejilla_medium_get_max_write_speed (medium));
	}
	else
		g_value_set_int64 (value, g_variant_get_int32 (variant) * 1000);

	return TRUE;
}

static GVariant *
rejilla_drive_settings_set_mapping_flags (const GValue *value,
                                          const GVariantType *variant_type,
                                          gpointer user_data)
{
	return g_variant_new_int32 (g_value_get_int (value) & REJILLA_DEST_SAVED_FLAGS);
}

static gboolean
rejilla_drive_settings_get_mapping_flags (GValue *value,
                                          GVariant *variant,
                                          gpointer user_data)
{
	RejillaBurnFlag flags;
	RejillaBurnFlag current_flags;
	RejillaDriveSettingsPrivate *priv;

	priv = REJILLA_DRIVE_SETTINGS_PRIVATE (user_data);

	flags = g_variant_get_int32 (variant);
	if (rejilla_burn_session_same_src_dest_drive (priv->session)) {
		/* Special case */
		if (flags == 1) {
			flags = REJILLA_BURN_FLAG_EJECT|
				REJILLA_BURN_FLAG_BURNPROOF;
		}
		else
			flags &= REJILLA_DEST_SAVED_FLAGS;

		flags |= REJILLA_BURN_FLAG_BLANK_BEFORE_WRITE|
			 REJILLA_BURN_FLAG_FAST_BLANK;
	}
	/* This is for the default value when the user has never used it */
	else if (flags == 1) {
		RejillaTrackType *source;

		flags = REJILLA_BURN_FLAG_EJECT|
			REJILLA_BURN_FLAG_BURNPROOF;

		source = rejilla_track_type_new ();
		rejilla_burn_session_get_input_type (REJILLA_BURN_SESSION (priv->session), source);

		if (!rejilla_track_type_get_has_medium (source))
			flags |= REJILLA_BURN_FLAG_NO_TMP_FILES;

		rejilla_track_type_free (source);
	}
	else
		flags &= REJILLA_DEST_SAVED_FLAGS;

	current_flags = rejilla_burn_session_get_flags (REJILLA_BURN_SESSION (priv->session));
	current_flags &= (~REJILLA_DEST_SAVED_FLAGS);

	g_value_set_int (value, flags|current_flags);
	return TRUE;
}

static void
rejilla_drive_settings_bind_session (RejillaDriveSettings *self)
{
	RejillaDriveSettingsPrivate *priv;
	gchar *display_name;
	gchar *path;
	gchar *tmp;

	priv = REJILLA_DRIVE_SETTINGS_PRIVATE (self);

	/* Get the drive name: it's done this way to avoid escaping */
	tmp = rejilla_drive_get_display_name (priv->dest_drive);
	display_name = g_strdup_printf ("drive-%u", g_str_hash (tmp));
	g_free (tmp);

	if (rejilla_track_type_get_has_medium (priv->src_type))
		path = g_strdup_printf ("%s%s/disc-%i/",
		                        REJILLA_DRIVE_PROPERTIES_PATH,
		                        display_name,
		                        priv->dest_media);
	else if (rejilla_track_type_get_has_data (priv->src_type))
		path = g_strdup_printf ("%s%s/data-%i/",
		                        REJILLA_DRIVE_PROPERTIES_PATH,
		                        display_name,
		                        priv->dest_media);
	else if (rejilla_track_type_get_has_image (priv->src_type))
		path = g_strdup_printf ("%s%s/image-%i/",
		                        REJILLA_DRIVE_PROPERTIES_PATH,
		                        display_name,
		                        priv->dest_media);
	else if (rejilla_track_type_get_has_stream (priv->src_type))
		path = g_strdup_printf ("%s%s/audio-%i/",
		                        REJILLA_DRIVE_PROPERTIES_PATH,
		                        display_name,
		                        priv->dest_media);
	else {
		g_free (display_name);
		return;
	}
	g_free (display_name);

	priv->settings = g_settings_new_with_path (REJILLA_SCHEMA_DRIVES, path);
	g_free (path);

	g_settings_bind_with_mapping (priv->settings, REJILLA_PROPS_SPEED,
	                              priv->session, "speed", G_SETTINGS_BIND_DEFAULT,
	                              rejilla_drive_settings_get_mapping_speed,
	                              rejilla_drive_settings_set_mapping_speed,
	                              self,
	                              NULL);

	g_settings_bind_with_mapping (priv->settings, REJILLA_PROPS_FLAGS,
	                              priv->session, "flags", G_SETTINGS_BIND_DEFAULT,
	                              rejilla_drive_settings_get_mapping_flags,
	                              rejilla_drive_settings_set_mapping_flags,
	                              self,
	                              NULL);
}

static void
rejilla_drive_settings_unbind_session (RejillaDriveSettings *self)
{
	RejillaDriveSettingsPrivate *priv;

	priv = REJILLA_DRIVE_SETTINGS_PRIVATE (self);

	if (priv->settings) {
		rejilla_track_type_free (priv->src_type);
		priv->src_type = NULL;

		g_object_unref (priv->dest_drive);
		priv->dest_drive = NULL;

		priv->dest_media = REJILLA_MEDIUM_NONE;

		g_settings_unbind (priv->settings, "speed");
		g_settings_unbind (priv->settings, "flags");

		g_object_unref (priv->settings);
		priv->settings = NULL;
	}
}

static void
rejilla_drive_settings_rebind_session (RejillaDriveSettings *self)
{
	RejillaDriveSettingsPrivate *priv;
	RejillaTrackType *type;
	RejillaMedia new_media;
	RejillaMedium *medium;
	RejillaDrive *drive;

	priv = REJILLA_DRIVE_SETTINGS_PRIVATE (self);

	/* See if we really need to do that:
	 * - check the source type has changed 
	 * - check the output type has changed */
	drive = rejilla_burn_session_get_burner (priv->session);
	medium = rejilla_drive_get_medium (drive);
	type = rejilla_track_type_new ();

	if (!drive
	||  !medium
	||   rejilla_drive_is_fake (drive)
	||  !REJILLA_MEDIUM_VALID (rejilla_medium_get_status (medium))
	||   rejilla_burn_session_get_input_type (REJILLA_BURN_SESSION (priv->session), type) != REJILLA_BURN_OK) {
		rejilla_drive_settings_unbind_session (self);
		return;
	}

	new_media = REJILLA_MEDIUM_TYPE (rejilla_medium_get_status (medium));

	if (priv->dest_drive == drive
	&&  priv->dest_media == new_media
	&&  priv->src_type && rejilla_track_type_equal (priv->src_type, type)) {
		rejilla_track_type_free (type);
		return;
	}

	rejilla_track_type_free (priv->src_type);
	priv->src_type = type;

	if (priv->dest_drive)
		g_object_unref (priv->dest_drive);

	priv->dest_drive = g_object_ref (drive);

	priv->dest_media = new_media;

	rejilla_drive_settings_bind_session (self);
}

static void
rejilla_drive_settings_output_changed_cb (RejillaBurnSession *session,
                                          RejillaMedium *former_medium,
                                          RejillaDriveSettings *self)
{
	rejilla_drive_settings_rebind_session (self);
}

static void
rejilla_drive_settings_track_added_cb (RejillaBurnSession *session,
                                       RejillaTrack *track,
                                       RejillaDriveSettings *self)
{
	rejilla_drive_settings_rebind_session (self);
}

static void
rejilla_drive_settings_track_removed_cb (RejillaBurnSession *session,
                                         RejillaTrack *track,
                                         guint former_position,
                                         RejillaDriveSettings *self)
{
	rejilla_drive_settings_rebind_session (self);
}

static void
rejilla_drive_settings_unset_session (RejillaDriveSettings *self)
{
	RejillaDriveSettingsPrivate *priv;

	priv = REJILLA_DRIVE_SETTINGS_PRIVATE (self);

	rejilla_drive_settings_unbind_session (self);

	if (priv->session) {
		g_signal_handlers_disconnect_by_func (priv->session,
		                                      rejilla_drive_settings_track_added_cb,
		                                      self);
		g_signal_handlers_disconnect_by_func (priv->session,
		                                      rejilla_drive_settings_track_removed_cb,
		                                      self);
		g_signal_handlers_disconnect_by_func (priv->session,
		                                      rejilla_drive_settings_output_changed_cb,
		                                      self);

		g_settings_unbind (priv->config_settings, "tmpdir");
		g_object_unref (priv->config_settings);

		g_object_unref (priv->session);
		priv->session = NULL;
	}
}

void
rejilla_drive_settings_set_session (RejillaDriveSettings *self,
                                    RejillaBurnSession *session)
{
	RejillaDriveSettingsPrivate *priv;

	priv = REJILLA_DRIVE_SETTINGS_PRIVATE (self);

	rejilla_drive_settings_unset_session (self);

	priv->session = g_object_ref (session);
	g_signal_connect (session,
	                  "track-added",
	                  G_CALLBACK (rejilla_drive_settings_track_added_cb),
	                  self);
	g_signal_connect (session,
	                  "track-removed",
	                  G_CALLBACK (rejilla_drive_settings_track_removed_cb),
	                  self);
	g_signal_connect (session,
	                  "output-changed",
	                  G_CALLBACK (rejilla_drive_settings_output_changed_cb),
	                  self);
	rejilla_drive_settings_rebind_session (self);

	priv->config_settings = g_settings_new (REJILLA_SCHEMA_CONFIG);
	g_settings_bind (priv->config_settings,
	                 REJILLA_PROPS_TMP_DIR, session,
	                 "tmpdir", G_SETTINGS_BIND_DEFAULT);
}

static void
rejilla_drive_settings_init (RejillaDriveSettings *object)
{ }

static void
rejilla_drive_settings_finalize (GObject *object)
{
	rejilla_drive_settings_unset_session (REJILLA_DRIVE_SETTINGS (object));
	G_OBJECT_CLASS (rejilla_drive_settings_parent_class)->finalize (object);
}

static void
rejilla_drive_settings_class_init (RejillaDriveSettingsClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (RejillaDriveSettingsPrivate));

	object_class->finalize = rejilla_drive_settings_finalize;
}

RejillaDriveSettings *
rejilla_drive_settings_new (void)
{
	return g_object_new (REJILLA_TYPE_DRIVE_SETTINGS, NULL);
}
