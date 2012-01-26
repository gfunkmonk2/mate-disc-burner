/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Librejilla-misc
 * Copyright (C) Philippe Rouquier 2005-2009 <bonfire-app@wanadoo.fr>
 *
 * Librejilla-misc is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * The Librejilla-misc authors hereby grant permission for non-GPL compatible
 * GStreamer plugins to be used and distributed together with GStreamer
 * and Librejilla-misc. This permission is above and beyond the permissions granted
 * by the GPL license by which Librejilla-burn is covered. If you modify this code
 * you may extend this exception to your version of the code, but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version.
 * 
 * Librejilla-misc is distributed in the hope that it will be useful,
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

#include <sys/types.h>
#include <unistd.h>

#include <glib.h>
#include <gtk/gtk.h>

#ifndef _REJILLA_MISC_H
#define _REJILLA_MISC_H

G_BEGIN_DECLS

GQuark rejilla_utils_error_quark (void);

#define REJILLA_UTILS_ERROR				rejilla_utils_error_quark()

typedef enum {
	REJILLA_UTILS_ERROR_NONE,
	REJILLA_UTILS_ERROR_GENERAL,
	REJILLA_UTILS_ERROR_SYMLINK_LOOP
} RejillaUtilsErrors;

#define REJILLA_UTILS_LOG_DOMAIN			"RejillaUtils"

void
rejilla_utils_set_use_debug (gboolean active);

void
rejilla_utils_debug_message (const gchar *domain,
			     const gchar *location,
			     const gchar *format,
			     ...);

#define REJILLA_UTILS_LOG(format, ...)						\
	rejilla_utils_debug_message (REJILLA_UTILS_LOG_DOMAIN,			\
				     G_STRLOC,					\
				     format,					\
				     ##__VA_ARGS__);


#define REJILLA_GET_BASENAME_FOR_DISPLAY(uri, name)				\
{										\
    	gchar *escaped_basename;						\
	escaped_basename = g_path_get_basename (uri);				\
    	name = g_uri_unescape_string (escaped_basename, NULL);			\
	g_free (escaped_basename);						\
}

void
rejilla_utils_init (void);

gchar *
rejilla_utils_get_uri_name (const gchar *uri);
gchar*
rejilla_utils_validate_utf8 (const gchar *name);

gchar *
rejilla_utils_register_string (const gchar *string);
void
rejilla_utils_unregister_string (const gchar *string);

GtkWidget *
rejilla_utils_properties_get_label (GtkWidget *widget);

GtkWidget *
rejilla_utils_pack_properties (const gchar *title, ...);
GtkWidget *
rejilla_utils_pack_properties_list (const gchar *title, GSList *list);
GtkWidget *
rejilla_utils_make_button (const gchar *text,
			   const gchar *stock,
			   const gchar *theme,
			   GtkIconSize size);

GtkWidget *
rejilla_utils_create_message_dialog (GtkWidget *parent,
				     const gchar *primary_message,
				     const gchar *secondary_message,
				     GtkMessageType type);

void
rejilla_utils_message_dialog (GtkWidget *parent,
			      const gchar *primary_message,
			      const gchar *secondary_message,
			      GtkMessageType type);

G_END_DECLS

#endif				/* _MISC_H */
