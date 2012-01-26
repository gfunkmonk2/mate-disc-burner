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

#ifndef _METADATA_H
#define _METADATA_H

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>

#include <gdk-pixbuf/gdk-pixbuf.h>

#include <gst/gst.h>

G_BEGIN_DECLS

typedef enum {
	REJILLA_METADATA_FLAG_NONE			= 0,
	REJILLA_METADATA_FLAG_SILENCES			= 1 << 1,
	REJILLA_METADATA_FLAG_MISSING			= 1 << 2,
	REJILLA_METADATA_FLAG_THUMBNAIL			= 1 << 3
} RejillaMetadataFlag;

#define REJILLA_TYPE_METADATA         (rejilla_metadata_get_type ())
#define REJILLA_METADATA(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), REJILLA_TYPE_METADATA, RejillaMetadata))
#define REJILLA_METADATA_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), REJILLA_TYPE_METADATA, RejillaMetadataClass))
#define REJILLA_IS_METADATA(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), REJILLA_TYPE_METADATA))
#define REJILLA_IS_METADATA_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), REJILLA_TYPE_METADATA))
#define REJILLA_METADATA_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), REJILLA_TYPE_METADATA, RejillaMetadataClass))

typedef struct {
	gint64 start;
	gint64 end;
} RejillaMetadataSilence;

typedef struct {
	gchar *uri;
	gchar *type;
	gchar *title;
	gchar *artist;
	gchar *album;
	gchar *genre;
	gchar *composer;
	gchar *musicbrainz_id;
	int isrc;
	guint64 len;

	gint channels;
	gint rate;

	GSList *silences;

	GdkPixbuf *snapshot;

	guint is_seekable:1;
	guint has_audio:1;
	guint has_video:1;
	guint has_dts:1;
} RejillaMetadataInfo;

void
rejilla_metadata_info_copy (RejillaMetadataInfo *dest,
			    RejillaMetadataInfo *src);
void
rejilla_metadata_info_clear (RejillaMetadataInfo *info);
void
rejilla_metadata_info_free (RejillaMetadataInfo *info);

typedef struct _RejillaMetadataClass RejillaMetadataClass;
typedef struct _RejillaMetadata RejillaMetadata;

struct _RejillaMetadataClass {
	GObjectClass parent_class;

	void		(*completed)	(RejillaMetadata *meta,
					 const GError *error);
	void		(*progress)	(RejillaMetadata *meta,
					 gdouble progress);

};

struct _RejillaMetadata {
	GObject parent;
};

GType rejilla_metadata_get_type (void) G_GNUC_CONST;

RejillaMetadata *rejilla_metadata_new (void);

gboolean
rejilla_metadata_get_info_async (RejillaMetadata *metadata,
				 const gchar *uri,
				 RejillaMetadataFlag flags);

void
rejilla_metadata_cancel (RejillaMetadata *metadata);

gboolean
rejilla_metadata_set_uri (RejillaMetadata *metadata,
			  RejillaMetadataFlag flags,
			  const gchar *uri,
			  GError **error);

void
rejilla_metadata_wait (RejillaMetadata *metadata,
		       GCancellable *cancel);
void
rejilla_metadata_increase_listener_number (RejillaMetadata *metadata);

gboolean
rejilla_metadata_decrease_listener_number (RejillaMetadata *metadata);

const gchar *
rejilla_metadata_get_uri (RejillaMetadata *metadata);

RejillaMetadataFlag
rejilla_metadata_get_flags (RejillaMetadata *metadata);

gboolean
rejilla_metadata_get_result (RejillaMetadata *metadata,
			     RejillaMetadataInfo *info,
			     GError **error);

typedef int	(*RejillaMetadataGetXidCb)	(gpointer user_data);

void
rejilla_metadata_set_get_xid_callback (RejillaMetadata *metadata,
                                       RejillaMetadataGetXidCb callback,
                                       gpointer user_data);
G_END_DECLS

#endif				/* METADATA_H */
