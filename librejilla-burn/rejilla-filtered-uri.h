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

#ifndef _REJILLA_FILTERED_URI_H_
#define _REJILLA_FILTERED_URI_H_

#include <glib-object.h>

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef enum {
	REJILLA_FILTER_NONE			= 0,
	REJILLA_FILTER_HIDDEN			= 1,
	REJILLA_FILTER_UNREADABLE,
	REJILLA_FILTER_BROKEN_SYM,
	REJILLA_FILTER_RECURSIVE_SYM,
	REJILLA_FILTER_UNKNOWN,
} RejillaFilterStatus;

#define REJILLA_TYPE_FILTERED_URI             (rejilla_filtered_uri_get_type ())
#define REJILLA_FILTERED_URI(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_FILTERED_URI, RejillaFilteredUri))
#define REJILLA_FILTERED_URI_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_FILTERED_URI, RejillaFilteredUriClass))
#define REJILLA_IS_FILTERED_URI(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_FILTERED_URI))
#define REJILLA_IS_FILTERED_URI_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_FILTERED_URI))
#define REJILLA_FILTERED_URI_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_FILTERED_URI, RejillaFilteredUriClass))

typedef struct _RejillaFilteredUriClass RejillaFilteredUriClass;
typedef struct _RejillaFilteredUri RejillaFilteredUri;

struct _RejillaFilteredUriClass
{
	GtkListStoreClass parent_class;
};

struct _RejillaFilteredUri
{
	GtkListStore parent_instance;
};

GType rejilla_filtered_uri_get_type (void) G_GNUC_CONST;

RejillaFilteredUri *
rejilla_filtered_uri_new (void);

gchar *
rejilla_filtered_uri_restore (RejillaFilteredUri *filtered,
			      GtkTreePath *treepath);

RejillaFilterStatus
rejilla_filtered_uri_lookup_restored (RejillaFilteredUri *filtered,
				      const gchar *uri);

GSList *
rejilla_filtered_uri_get_restored_list (RejillaFilteredUri *filtered);

void
rejilla_filtered_uri_filter (RejillaFilteredUri *filtered,
			     const gchar *uri,
			     RejillaFilterStatus status);
void
rejilla_filtered_uri_dont_filter (RejillaFilteredUri *filtered,
				  const gchar *uri);

void
rejilla_filtered_uri_clear (RejillaFilteredUri *filtered);

void
rejilla_filtered_uri_remove_with_children (RejillaFilteredUri *filtered,
					   const gchar *uri);

G_END_DECLS

#endif /* _REJILLA_FILTERED_URI_H_ */
