/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Librejilla-media
 * Copyright (C) Philippe Rouquier 2005-2009 <bonfire-app@wanadoo.fr>
 *
 * Librejilla-media is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * The Librejilla-media authors hereby grant permission for non-GPL compatible
 * GStreamer plugins to be used and distributed together with GStreamer
 * and Librejilla-media. This permission is above and beyond the permissions granted
 * by the GPL license by which Librejilla-media is covered. If you modify this code
 * you may extend this exception to your version of the code, but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version.
 * 
 * Librejilla-media is distributed in the hope that it will be useful,
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
#include <stdio.h>

#include <glib.h>
#include <glib/gi18n-lib.h>

#include "rejilla-media.h"
#include "rejilla-media-private.h"

static gboolean debug = 0;

#define REJILLA_MEDIUM_TRUE_RANDOM_WRITABLE(media)				\
	(REJILLA_MEDIUM_IS (media, REJILLA_MEDIUM_DVDRW_RESTRICTED) ||		\
	 REJILLA_MEDIUM_IS (media, REJILLA_MEDIUM_DVDRW_PLUS) ||		\
	 REJILLA_MEDIUM_IS (media, REJILLA_MEDIUM_DVD_RAM) || 			\
	 REJILLA_MEDIUM_IS (media, REJILLA_MEDIUM_BDRE))

static const GOptionEntry options [] = {
	{ "rejilla-media-debug", 0, 0, G_OPTION_ARG_NONE, &debug,
	  N_("Display debug statements on stdout for Rejilla media library"),
	  NULL },
	{ NULL }
};

void
rejilla_media_library_set_debug (gboolean value)
{
	debug = value;
}

static GSList *
rejilla_media_add_to_list (GSList *retval,
			   RejillaMedia media)
{
	retval = g_slist_prepend (retval, GINT_TO_POINTER (media));
	return retval;
}

static GSList *
rejilla_media_new_status (GSList *retval,
			  RejillaMedia media,
			  RejillaMedia type)
{
	if ((type & REJILLA_MEDIUM_BLANK)
	&& !(media & REJILLA_MEDIUM_ROM)) {
		/* If media is blank there is no other possible property. */
		retval = rejilla_media_add_to_list (retval,
						    media|
						    REJILLA_MEDIUM_BLANK);

		/* NOTE about BR-R they can be "formatted" but they are never
		 * unformatted since by default they'll be used as sequential.
		 * We don't consider DVD-RW as unformatted as in this case
		 * they are treated as DVD-RW in sequential mode and therefore
		 * don't require any formatting. */
		if (!(media & REJILLA_MEDIUM_RAM)
		&&   (REJILLA_MEDIUM_IS (media, REJILLA_MEDIUM_DVDRW_PLUS)
		||    REJILLA_MEDIUM_IS (media, REJILLA_MEDIUM_BD|REJILLA_MEDIUM_REWRITABLE))) {
			if (type & REJILLA_MEDIUM_UNFORMATTED)
				retval = rejilla_media_add_to_list (retval,
								    media|
								    REJILLA_MEDIUM_BLANK|
								    REJILLA_MEDIUM_UNFORMATTED);
		}
	}

	if (type & REJILLA_MEDIUM_CLOSED) {
		if (media & (REJILLA_MEDIUM_DVD|REJILLA_MEDIUM_BD))
			retval = rejilla_media_add_to_list (retval,
							    media|
							    REJILLA_MEDIUM_CLOSED|
							    (type & REJILLA_MEDIUM_HAS_DATA)|
							    (type & REJILLA_MEDIUM_PROTECTED));
		else {
			if (type & REJILLA_MEDIUM_HAS_AUDIO)
				retval = rejilla_media_add_to_list (retval,
								    media|
								    REJILLA_MEDIUM_CLOSED|
								    REJILLA_MEDIUM_HAS_AUDIO);
			if (type & REJILLA_MEDIUM_HAS_DATA)
				retval = rejilla_media_add_to_list (retval,
								    media|
								    REJILLA_MEDIUM_CLOSED|
								    REJILLA_MEDIUM_HAS_DATA);
			if (REJILLA_MEDIUM_IS (type, REJILLA_MEDIUM_HAS_AUDIO|REJILLA_MEDIUM_HAS_DATA))
				retval = rejilla_media_add_to_list (retval,
								    media|
								    REJILLA_MEDIUM_CLOSED|
								    REJILLA_MEDIUM_HAS_DATA|
								    REJILLA_MEDIUM_HAS_AUDIO);
		}
	}

	if ((type & REJILLA_MEDIUM_APPENDABLE)
	&& !(media & REJILLA_MEDIUM_ROM)
	&& !REJILLA_MEDIUM_TRUE_RANDOM_WRITABLE (media)) {
		if (media & (REJILLA_MEDIUM_BD|REJILLA_MEDIUM_DVD))
			retval = rejilla_media_add_to_list (retval,
							    media|
							    REJILLA_MEDIUM_APPENDABLE|
							    REJILLA_MEDIUM_HAS_DATA);
		else {
			if (type & REJILLA_MEDIUM_HAS_AUDIO)
				retval = rejilla_media_add_to_list (retval,
								    media|
								    REJILLA_MEDIUM_APPENDABLE|
								    REJILLA_MEDIUM_HAS_AUDIO);
			if (type & REJILLA_MEDIUM_HAS_DATA)
				retval = rejilla_media_add_to_list (retval,
								    media|
								    REJILLA_MEDIUM_APPENDABLE|
								    REJILLA_MEDIUM_HAS_DATA);
			if (REJILLA_MEDIUM_IS (type, REJILLA_MEDIUM_HAS_AUDIO|REJILLA_MEDIUM_HAS_DATA))
				retval = rejilla_media_add_to_list (retval,
								    media|
								    REJILLA_MEDIUM_HAS_DATA|
								    REJILLA_MEDIUM_APPENDABLE|
								    REJILLA_MEDIUM_HAS_AUDIO);
		}
	}

	return retval;
}

static GSList *
rejilla_media_new_attribute (GSList *retval,
			     RejillaMedia media,
			     RejillaMedia type)
{
	/* NOTE: never reached by BDs, ROMs (any) or Restricted Overwrite
	 * and DVD- dual layer */

	/* NOTE: there is no dual layer DVD-RW */
	if (type & REJILLA_MEDIUM_REWRITABLE)
		retval = rejilla_media_new_status (retval,
						   media|REJILLA_MEDIUM_REWRITABLE,
						   type);

	if (type & REJILLA_MEDIUM_WRITABLE)
		retval = rejilla_media_new_status (retval,
						   media|REJILLA_MEDIUM_WRITABLE,
						   type);

	return retval;
}

static GSList *
rejilla_media_new_subtype (GSList *retval,
			   RejillaMedia media,
			   RejillaMedia type)
{
	if (media & REJILLA_MEDIUM_BD) {
		/* There seems to be Dual layers BDs as well */

		if (type & REJILLA_MEDIUM_ROM) {
			retval = rejilla_media_new_status (retval,
							   media|
							   REJILLA_MEDIUM_ROM,
							   type);
			if (type & REJILLA_MEDIUM_DUAL_L)
				retval = rejilla_media_new_status (retval,
								   media|
								   REJILLA_MEDIUM_ROM|
								   REJILLA_MEDIUM_DUAL_L,
								   type);
		}

		if (type & REJILLA_MEDIUM_RANDOM) {
			retval = rejilla_media_new_status (retval,
							   media|
							   REJILLA_MEDIUM_RANDOM|
							   REJILLA_MEDIUM_WRITABLE,
							   type);
			if (type & REJILLA_MEDIUM_DUAL_L)
				retval = rejilla_media_new_status (retval,
								   media|
								   REJILLA_MEDIUM_RANDOM|
								   REJILLA_MEDIUM_WRITABLE|
								   REJILLA_MEDIUM_DUAL_L,
								   type);
		}

		if (type & REJILLA_MEDIUM_SRM) {
			retval = rejilla_media_new_status (retval,
							   media|
							   REJILLA_MEDIUM_SRM|
							   REJILLA_MEDIUM_WRITABLE,
							   type);
			if (type & REJILLA_MEDIUM_DUAL_L)
				retval = rejilla_media_new_status (retval,
								   media|
								   REJILLA_MEDIUM_SRM|
								   REJILLA_MEDIUM_WRITABLE|
								   REJILLA_MEDIUM_DUAL_L,
								   type);
		}

		if (type & REJILLA_MEDIUM_POW) {
			retval = rejilla_media_new_status (retval,
							   media|
							   REJILLA_MEDIUM_POW|
							   REJILLA_MEDIUM_WRITABLE,
							   type);
			if (type & REJILLA_MEDIUM_DUAL_L)
				retval = rejilla_media_new_status (retval,
								   media|
								   REJILLA_MEDIUM_POW|
								   REJILLA_MEDIUM_WRITABLE|
								   REJILLA_MEDIUM_DUAL_L,
								   type);
		}

		/* BD-RE */
		if (type & REJILLA_MEDIUM_REWRITABLE) {
			retval = rejilla_media_new_status (retval,
							   media|
							   REJILLA_MEDIUM_REWRITABLE,
							   type);
			if (type & REJILLA_MEDIUM_DUAL_L)
				retval = rejilla_media_new_status (retval,
								   media|
								   REJILLA_MEDIUM_REWRITABLE|
								   REJILLA_MEDIUM_DUAL_L,
								   type);
		}
	}

	if (media & REJILLA_MEDIUM_DVD) {
		/* There is no such thing as DVD-RW DL nor DVD-RAM DL*/

		/* The following is always a DVD-R dual layer */
		if (type & REJILLA_MEDIUM_JUMP)
			retval = rejilla_media_new_status (retval,
							   media|
							   REJILLA_MEDIUM_JUMP|
							   REJILLA_MEDIUM_DUAL_L|
							   REJILLA_MEDIUM_WRITABLE,
							   type);

		if (type & REJILLA_MEDIUM_SEQUENTIAL) {
			retval = rejilla_media_new_attribute (retval,
							      media|
							      REJILLA_MEDIUM_SEQUENTIAL,
							      type);

			/* This one has to be writable only, no RW */
			if (type & REJILLA_MEDIUM_DUAL_L)
				retval = rejilla_media_new_status (retval,
								   media|
								   REJILLA_MEDIUM_SEQUENTIAL|
								   REJILLA_MEDIUM_WRITABLE|
								   REJILLA_MEDIUM_DUAL_L,
								   type);
		}

		/* Restricted Overwrite media are always rewritable */
		if (type & REJILLA_MEDIUM_RESTRICTED)
			retval = rejilla_media_new_status (retval,
							   media|
							   REJILLA_MEDIUM_RESTRICTED|
							   REJILLA_MEDIUM_REWRITABLE,
							   type);

		if (type & REJILLA_MEDIUM_PLUS) {
			retval = rejilla_media_new_attribute (retval,
							      media|
							      REJILLA_MEDIUM_PLUS,
							      type);

			if (type & REJILLA_MEDIUM_DUAL_L)
				retval = rejilla_media_new_attribute (retval,
								      media|
								      REJILLA_MEDIUM_PLUS|
								      REJILLA_MEDIUM_DUAL_L,
								      type);

		}

		if (type & REJILLA_MEDIUM_ROM) {
			retval = rejilla_media_new_status (retval,
							   media|
							   REJILLA_MEDIUM_ROM,
							   type);

			if (type & REJILLA_MEDIUM_DUAL_L)
				retval = rejilla_media_new_status (retval,
								   media|
								   REJILLA_MEDIUM_ROM|
								   REJILLA_MEDIUM_DUAL_L,
								   type);
		}

		/* RAM media are always rewritable */
		if (type & REJILLA_MEDIUM_RAM)
			retval = rejilla_media_new_status (retval,
							   media|
							   REJILLA_MEDIUM_RAM|
							   REJILLA_MEDIUM_REWRITABLE,
							   type);
	}

	return retval;
}

GSList *
rejilla_media_get_all_list (RejillaMedia type)
{
	GSList *retval = NULL;

	if (type & REJILLA_MEDIUM_FILE)
		retval = rejilla_media_add_to_list (retval, REJILLA_MEDIUM_FILE);					       

	if (type & REJILLA_MEDIUM_CD) {
		if (type & REJILLA_MEDIUM_ROM)
			retval = rejilla_media_new_status (retval,
							   REJILLA_MEDIUM_CD|
							   REJILLA_MEDIUM_ROM,
							   type);

		retval = rejilla_media_new_attribute (retval,
						      REJILLA_MEDIUM_CD,
						      type);
	}

	if (type & REJILLA_MEDIUM_DVD)
		retval = rejilla_media_new_subtype (retval,
						    REJILLA_MEDIUM_DVD,
						    type);


	if (type & REJILLA_MEDIUM_BD)
		retval = rejilla_media_new_subtype (retval,
						    REJILLA_MEDIUM_BD,
						    type);

	return retval;
}

GQuark
rejilla_media_quark (void)
{
	static GQuark quark = 0;

	if (!quark)
		quark = g_quark_from_static_string ("RejillaMediaError");

	return quark;
}

void
rejilla_media_to_string (RejillaMedia media,
			 gchar *buffer)
{
	if (media & REJILLA_MEDIUM_FILE)
		strcat (buffer, "file ");

	if (media & REJILLA_MEDIUM_CD)
		strcat (buffer, "CD ");

	if (media & REJILLA_MEDIUM_DVD)
		strcat (buffer, "DVD ");

	if (media & REJILLA_MEDIUM_RAM)
		strcat (buffer, "RAM ");

	if (media & REJILLA_MEDIUM_BD)
		strcat (buffer, "BD ");

	if (media & REJILLA_MEDIUM_DUAL_L)
		strcat (buffer, "DL ");

	/* DVD subtypes */
	if (media & REJILLA_MEDIUM_PLUS)
		strcat (buffer, "+ ");

	if (media & REJILLA_MEDIUM_SEQUENTIAL)
		strcat (buffer, "- (sequential) ");

	if (media & REJILLA_MEDIUM_RESTRICTED)
		strcat (buffer, "- (restricted) ");

	if (media & REJILLA_MEDIUM_JUMP)
		strcat (buffer, "- (jump) ");

	/* BD subtypes */
	if (media & REJILLA_MEDIUM_SRM)
		strcat (buffer, "SRM ");

	if (media & REJILLA_MEDIUM_POW)
		strcat (buffer, "POW ");

	if (media & REJILLA_MEDIUM_RANDOM)
		strcat (buffer, "RANDOM ");

	/* discs attributes */
	if (media & REJILLA_MEDIUM_REWRITABLE)
		strcat (buffer, "RW ");

	if (media & REJILLA_MEDIUM_WRITABLE)
		strcat (buffer, "W ");

	if (media & REJILLA_MEDIUM_ROM)
		strcat (buffer, "ROM ");

	/* status of the disc */
	if (media & REJILLA_MEDIUM_CLOSED)
		strcat (buffer, "closed ");

	if (media & REJILLA_MEDIUM_BLANK)
		strcat (buffer, "blank ");

	if (media & REJILLA_MEDIUM_APPENDABLE)
		strcat (buffer, "appendable ");

	if (media & REJILLA_MEDIUM_PROTECTED)
		strcat (buffer, "protected ");

	if (media & REJILLA_MEDIUM_HAS_DATA)
		strcat (buffer, "with data ");

	if (media & REJILLA_MEDIUM_HAS_AUDIO)
		strcat (buffer, "with audio ");

	if (media & REJILLA_MEDIUM_UNFORMATTED)
		strcat (buffer, "Unformatted ");
}

/**
 * rejilla_media_get_option_group:
 *
 * Returns a GOptionGroup for the commandline arguments recognized by librejilla-media.
 * You should add this to your GOptionContext if your are using g_option_context_parse ()
 * to parse your commandline arguments.
 *
 * Return value: a #GOptionGroup *
 **/
GOptionGroup *
rejilla_media_get_option_group (void)
{
	GOptionGroup *group;

	group = g_option_group_new ("rejilla-media",
				    N_("Rejilla optical media library"),
				    N_("Display options for Rejilla media library"),
				    NULL,
				    NULL);
	g_option_group_add_entries (group, options);
	return group;
}

void
rejilla_media_message (const gchar *location,
		       const gchar *format,
		       ...)
{
	va_list arg_list;
	gchar *format_real;

	if (!debug)
		return;

	format_real = g_strdup_printf ("RejillaMedia: (at %s) %s\n",
				       location,
				       format);

	va_start (arg_list, format);
	vprintf (format_real, arg_list);
	va_end (arg_list);

	g_free (format_real);
}

#include <gtk/gtk.h>

#include "rejilla-medium-monitor.h"

static RejillaMediumMonitor *default_monitor = NULL;

/**
 * rejilla_media_library_start:
 *
 * Initialize the library.
 *
 * You should call this function before using any other from the library.
 *
 * Rename to: init
 **/
void
rejilla_media_library_start (void)
{
	if (default_monitor) {
		g_object_ref (default_monitor);
		return;
	}

	REJILLA_MEDIA_LOG ("Initializing Rejilla-media %i.%i.%i",
	                    REJILLA_MAJOR_VERSION,
	                    REJILLA_MINOR_VERSION,
	                    REJILLA_SUB);

#if defined(HAVE_STRUCT_USCSI_CMD)
	/* Work around: because on OpenSolaris rejilla posiblely be run
	 * as root for a user with 'Primary Administrator' profile,
	 * a root dbus session will be autospawned at that time.
	 * This fix is to work around
	 * http://bugzilla.mate.org/show_bug.cgi?id=526454
	 */
	g_setenv ("DBUS_SESSION_BUS_ADDRESS", "autolaunch:", TRUE);
#endif

	/* Initialize external libraries (threads... */
	if (!g_thread_supported ())
		g_thread_init (NULL);

	/* Initialize i18n */
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");

	/* Initialize icon-theme */
	gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (),
					   REJILLA_DATADIR "/icons");

	/* Take a reference for the monitoring library */
	default_monitor = rejilla_medium_monitor_get_default ();
}

/**
 * rejilla_media_library_stop:
 *
 * De-initialize the library once you do not need the library anymore.
 *
 * Rename to: deinit
 **/
void
rejilla_media_library_stop (void)
{
	g_object_unref (default_monitor);
	default_monitor = NULL;
}
