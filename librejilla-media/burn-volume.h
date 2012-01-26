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

#include <glib.h>

#ifndef BURN_VOLUME_H
#define BURN_VOLUME_H

G_BEGIN_DECLS

#include "burn-volume-source.h"

struct _RejillaVolDesc {
	guchar type;
	gchar id			[5];
	guchar version;
};
typedef struct _RejillaVolDesc RejillaVolDesc;

struct _RejillaVolFileExtent {
	guint block;
	guint size;
};
typedef struct _RejillaVolFileExtent RejillaVolFileExtent;

typedef struct _RejillaVolFile RejillaVolFile;
struct _RejillaVolFile {
	RejillaVolFile *parent;

	gchar *name;
	gchar *rr_name;

	union {

	struct {
		GSList *extents;
		guint64 size_bytes;
	} file;

	struct {
		GList *children;
		guint address;
	} dir;

	} specific;

	guint isdir:1;
	guint isdir_loaded:1;

	/* mainly used internally */
	guint has_RR:1;
	guint relocated:1;
};

gboolean
rejilla_volume_is_valid (RejillaVolSrc *src,
			 GError **error);

gboolean
rejilla_volume_get_size (RejillaVolSrc *src,
			 gint64 block,
			 gint64 *nb_blocks,
			 GError **error);

RejillaVolFile *
rejilla_volume_get_files (RejillaVolSrc *src,
			  gint64 block,
			  gchar **label,
			  gint64 *nb_blocks,
			  gint64 *data_blocks,
			  GError **error);

RejillaVolFile *
rejilla_volume_get_file (RejillaVolSrc *src,
			 const gchar *path,
			 gint64 volume_start_block,
			 GError **error);

GList *
rejilla_volume_load_directory_contents (RejillaVolSrc *vol,
					gint64 session_block,
					gint64 block,
					GError **error);


#define REJILLA_VOLUME_FILE_NAME(file)			((file)->rr_name?(file)->rr_name:(file)->name)
#define REJILLA_VOLUME_FILE_SIZE(file)			((file)->isdir?0:(file)->specific.file.size_bytes)

void
rejilla_volume_file_free (RejillaVolFile *file);

gchar *
rejilla_volume_file_to_path (RejillaVolFile *file);

RejillaVolFile *
rejilla_volume_file_from_path (const gchar *ptr,
			       RejillaVolFile *parent);

gint64
rejilla_volume_file_size (RejillaVolFile *file);

RejillaVolFile *
rejilla_volume_file_merge (RejillaVolFile *file1,
			   RejillaVolFile *file2);

G_END_DECLS

#endif /* BURN_VOLUME_H */
