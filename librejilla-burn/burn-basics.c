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
#include <string.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>

#include <gst/gst.h>
#include <gst/pbutils/pbutils.h>

#include "rejilla-io.h"

#include "burn-basics.h"
#include "burn-debug.h"
#include "burn-caps.h"
#include "burn-plugin-manager.h"
#include "rejilla-plugin-information.h"

#include "rejilla-drive.h"
#include "rejilla-medium-monitor.h"

#include "rejilla-burn-lib.h"
#include "burn-caps.h"

static RejillaPluginManager *plugin_manager = NULL;
static RejillaMediumMonitor *medium_manager = NULL;
static RejillaBurnCaps *default_caps = NULL;


GQuark
rejilla_burn_quark (void)
{
	static GQuark quark = 0;

	if (!quark)
		quark = g_quark_from_static_string ("RejillaBurnError");

	return quark;
}
 
const gchar *
rejilla_burn_action_to_string (RejillaBurnAction action)
{
	gchar *strings [REJILLA_BURN_ACTION_LAST] = { 	"",
							N_("Getting size"),
							N_("Creating image"),
							N_("Writing"),
							N_("Blanking"),
							N_("Creating checksum"),
							N_("Copying disc"),
							N_("Copying file"),
							N_("Analysing audio files"),
							N_("Transcoding song"),
							N_("Preparing to write"),
							N_("Writing leadin"),
							N_("Writing CD-Text information"),
							N_("Finalizing"),
							N_("Writing leadout"),
						        N_("Starting to record"),
							N_("Success"),
							N_("Ejecting medium")};
	return _(strings [action]);
}

/**
 * utility functions
 */

gboolean
rejilla_check_flags_for_drive (RejillaDrive *drive,
			       RejillaBurnFlag flags)
{
	RejillaMedia media;
	RejillaMedium *medium;

	if (!drive)
		return TRUE;

	if (rejilla_drive_is_fake (drive))
		return TRUE;

	medium = rejilla_drive_get_medium (drive);
	if (!medium)
		return TRUE;

	media = rejilla_medium_get_status (medium);
	if (flags & REJILLA_BURN_FLAG_DUMMY) {
		/* This is always FALSE */
		if (media & REJILLA_MEDIUM_PLUS)
			return FALSE;

		if (media & REJILLA_MEDIUM_DVD) {
			if (!rejilla_medium_can_use_dummy_for_sao (medium))
				return FALSE;
		}
		else if (flags & REJILLA_BURN_FLAG_DAO) {
			if (!rejilla_medium_can_use_dummy_for_sao (medium))
				return FALSE;
		}
		else if (!rejilla_medium_can_use_dummy_for_tao (medium))
			return FALSE;
	}

	if (flags & REJILLA_BURN_FLAG_BURNPROOF) {
		if (!rejilla_medium_can_use_burnfree (medium))
			return FALSE;
	}

	return TRUE;
}

gchar *
rejilla_string_get_localpath (const gchar *uri)
{
	gchar *localpath;
	gchar *realuri;
	GFile *file;

	if (!uri)
		return NULL;

	if (uri [0] == '/')
		return g_strdup (uri);

	if (strncmp (uri, "file://", 7))
		return NULL;

	file = g_file_new_for_commandline_arg (uri);
	realuri = g_file_get_uri (file);
	g_object_unref (file);

	localpath = g_filename_from_uri (realuri, NULL, NULL);
	g_free (realuri);

	return localpath;
}

gchar *
rejilla_string_get_uri (const gchar *uri)
{
	gchar *uri_return;
	GFile *file;

	if (!uri)
		return NULL;

	if (uri [0] != '/')
		return g_strdup (uri);

	file = g_file_new_for_commandline_arg (uri);
	uri_return = g_file_get_uri (file);
	g_object_unref (file);

	return uri_return;
}

static void
rejilla_caps_list_dump (void)
{
	GSList *iter;
	RejillaBurnCaps *self;

	self = rejilla_burn_caps_get_default ();
	for (iter = self->priv->caps_list; iter; iter = iter->next) {
		RejillaCaps *caps;

		caps = iter->data;
		REJILLA_BURN_LOG_WITH_TYPE (&caps->type,
					    caps->flags,
					    "Created %i links pointing to",
					    g_slist_length (caps->links));
	}

	g_object_unref (self);
}

/**
 * rejilla_burn_library_start:
 * @argc: an #int.
 * @argv: a #char **.
 *
 * Starts the library. This function must be called
 * before using any of the functions.
 *
 * Rename to: init
 *
 * Returns: a #gboolean
 **/

gboolean
rejilla_burn_library_start (int *argc,
                            char **argv [])
{
	REJILLA_BURN_LOG ("Initializing Rejilla-burn %i.%i.%i",
			  REJILLA_MAJOR_VERSION,
			  REJILLA_MINOR_VERSION,
			  REJILLA_SUB);

#if defined(HAVE_STRUCT_USCSI_CMD)
	/* Work around: because on OpenSolaris rejilla possibly be run
	 * as root for a user with 'Primary Administrator' profile,
	 * a root dbus session will be autospawned at that time.
	 * This fix is to work around
	 * http://bugzilla.mate.org/show_bug.cgi?id=526454
	 */
	g_setenv ("DBUS_SESSION_BUS_ADDRESS", "autolaunch:", TRUE);
#endif

	/* Initialize external libraries (threads...) */
	if (!g_thread_supported ())
		g_thread_init (NULL);

	/* ... and GStreamer) */
	if (!gst_init_check (argc, argv, NULL))
		return FALSE;

	/* This is for missing codec automatic install */
	gst_pb_utils_init ();

	/* initialize the media library */
	rejilla_media_library_start ();

	/* initialize all device list */
	if (!medium_manager)
		medium_manager = rejilla_medium_monitor_get_default ();

	/* initialize plugins */
	if (!default_caps)
		default_caps = REJILLA_BURNCAPS (g_object_new (REJILLA_TYPE_BURNCAPS, NULL));

	if (!plugin_manager)
		plugin_manager = rejilla_plugin_manager_get_default ();

	rejilla_caps_list_dump ();
	return TRUE;
}

RejillaBurnCaps *
rejilla_burn_caps_get_default ()
{
	if (!default_caps)
		g_error ("You must call rejilla_burn_library_start () before using API from librejilla-burn");

	g_object_ref (default_caps);
	return default_caps;
}

/**
 * rejilla_burn_library_get_plugins_list:
 * 
 * This function returns the list of plugins that 
 * are available to librejilla-burn.
 *
 * Returns: (element-type GObject.Object) (transfer full):a #GSList that must be destroyed when not needed and each object unreffed.
 **/

GSList *
rejilla_burn_library_get_plugins_list (void)
{
	plugin_manager = rejilla_plugin_manager_get_default ();
	return rejilla_plugin_manager_get_plugins_list (plugin_manager);
}

/**
 * rejilla_burn_library_stop:
 *
 * Stop the library. Don't use any of the functions or
 * objects afterwards
 *
 * Rename to: deinit
 *
 **/
void
rejilla_burn_library_stop (void)
{
	if (plugin_manager) {
		g_object_unref (plugin_manager);
		plugin_manager = NULL;
	}

	if (medium_manager) {
		g_object_unref (medium_manager);
		medium_manager = NULL;
	}

	if (default_caps) {
		g_object_unref (default_caps);
		default_caps = NULL;
	}

	/* Cleanup the io thing */
	rejilla_io_shutdown ();
}

/**
 * rejilla_burn_library_can_checksum:
 *
 * Checks whether the library can do any kind of
 * checksum at all.
 *
 * Returns: a #gboolean
 */

gboolean
rejilla_burn_library_can_checksum (void)
{
	GSList *iter;
	RejillaBurnCaps *self;

	self = rejilla_burn_caps_get_default ();

	if (self->priv->tests == NULL) {
		g_object_unref (self);
		return FALSE;
	}

	for (iter = self->priv->tests; iter; iter = iter->next) {
		RejillaCapsTest *tmp;
		GSList *links;

		tmp = iter->data;
		for (links = tmp->links; links; links = links->next) {
			RejillaCapsLink *link;

			link = links->data;
			if (rejilla_caps_link_active (link, 0)) {
				g_object_unref (self);
				return TRUE;
			}
		}
	}

	g_object_unref (self);
	return FALSE;
}

/**
 * rejilla_burn_library_input_supported:
 * @type: a #RejillaTrackType
 *
 * Checks whether @type can be used as input.
 *
 * Returns: a #RejillaBurnResult
 */

RejillaBurnResult
rejilla_burn_library_input_supported (RejillaTrackType *type)
{
	GSList *iter;
	RejillaBurnCaps *self;

	g_return_val_if_fail (type != NULL, REJILLA_BURN_ERR);

	self = rejilla_burn_caps_get_default ();

	for (iter = self->priv->caps_list; iter; iter = iter->next) {
		RejillaCaps *caps;

		caps = iter->data;

		if (rejilla_caps_is_compatible_type (caps, type)
		&&  rejilla_burn_caps_is_input (self, caps)) {
			g_object_unref (self);
			return REJILLA_BURN_OK;
		}
	}

	g_object_unref (self);
	return REJILLA_BURN_ERR;
}

/**
 * rejilla_burn_library_get_media_capabilities:
 * @media: a #RejillaMedia
 *
 * Used to test what the library can do based on the medium type.
 * Returns REJILLA_MEDIUM_WRITABLE if the disc can be written
 * and / or REJILLA_MEDIUM_REWRITABLE if the disc can be erased.
 *
 * Returns: a #RejillaMedia
 */

RejillaMedia
rejilla_burn_library_get_media_capabilities (RejillaMedia media)
{
	GSList *iter;
	GSList *links;
	RejillaMedia retval;
	RejillaBurnCaps *self;
	RejillaCaps *caps = NULL;

	self = rejilla_burn_caps_get_default ();

	retval = REJILLA_MEDIUM_NONE;
	REJILLA_BURN_LOG_DISC_TYPE (media, "checking media caps for");

	/* we're only interested in DISC caps. There should be only one caps fitting */
	for (iter = self->priv->caps_list; iter; iter = iter->next) {
		caps = iter->data;
		if (caps->type.type != REJILLA_TRACK_TYPE_DISC)
			continue;

		if ((media & caps->type.subtype.media) == media)
			break;

		caps = NULL;
	}

	if (!caps) {
		g_object_unref (self);
		return REJILLA_MEDIUM_NONE;
	}

	/* check the links */
	for (links = caps->links; links; links = links->next) {
		GSList *plugins;
		gboolean active;
		RejillaCapsLink *link;

		link = links->data;

		/* this link must have at least one active plugin to be valid
		 * plugins are not sorted but in this case we don't need them
		 * to be. we just need one active if another is with a better
		 * priority all the better. */
		active = FALSE;
		for (plugins = link->plugins; plugins; plugins = plugins->next) {
			RejillaPlugin *plugin;

			plugin = plugins->data;
			/* Ignore plugin errors */
			if (rejilla_plugin_get_active (plugin, TRUE)) {
				/* this link is valid */
				active = TRUE;
				break;
			}
		}

		if (!active)
			continue;

		if (!link->caps) {
			/* means that it can be blanked */
			retval |= REJILLA_MEDIUM_REWRITABLE;
			continue;
		}

		/* means it can be written. NOTE: if this disc has already some
		 * data on it, it even means it can be appended */
		retval |= REJILLA_MEDIUM_WRITABLE;
	}

	g_object_unref (self);
	return retval;
}
