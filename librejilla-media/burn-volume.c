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

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <glib.h>
#include <glib/gi18n-lib.h>

#include "burn-volume-source.h"
#include "burn-volume.h"
#include "burn-iso9660.h"
#include "rejilla-media.h"
#include "rejilla-media-private.h"
#include "rejilla-units.h"

struct _RejillaTagDesc {
	guint16 id;
	guint16 version;
	guchar checksum;
	guchar reserved;
	guint16 serial;
	guint16 crc;
	guint16 crc_len;
	guint32 location;
};
typedef struct _RejillaTagDesc RejillaTagDesc;

struct _RejillaAnchorDesc {
	RejillaTagDesc tag;

	guchar main_extent		[8];
	guchar reserve_extent		[8];
};
typedef struct _RejillaAnchorDesc RejillaAnchorDesc;

#define SYSTEM_AREA_SECTORS		16
#define ANCHOR_AREA_SECTORS		256


void
rejilla_volume_file_free (RejillaVolFile *file)
{
	if (!file)
		return;

	if (file->isdir) {
		GList *iter;

		if (file->isdir_loaded) {
			for (iter = file->specific.dir.children; iter; iter = iter->next)
				rejilla_volume_file_free (iter->data);

			g_list_free (file->specific.dir.children);
		}
	}
	else {
		g_slist_foreach (file->specific.file.extents,
				 (GFunc) g_free,
				 NULL);
		g_slist_free (file->specific.file.extents);
	}

	g_free (file->rr_name);
	g_free (file->name);
	g_free (file);
}

static gboolean
rejilla_volume_get_primary_from_file (RejillaVolSrc *vol,
				      gchar *primary_vol,
				      GError **error)
{
	RejillaVolDesc *desc;

	/* skip the first 16 blocks */
	if (REJILLA_VOL_SRC_SEEK (vol, SYSTEM_AREA_SECTORS, SEEK_CUR, error) == -1)
		return FALSE;

	if (!REJILLA_VOL_SRC_READ (vol, primary_vol, 1, error))
		return FALSE;

	/* make a few checks to ensure this is an ECMA volume */
	desc = (RejillaVolDesc *) primary_vol;
	if (memcmp (desc->id, "CD001", 5)
	&&  memcmp (desc->id, "BEA01", 5)
	&&  memcmp (desc->id, "BOOT2", 5)
	&&  memcmp (desc->id, "CDW02", 5)
	&&  memcmp (desc->id, "NSR02", 5)	/* usually UDF */
	&&  memcmp (desc->id, "NSR03", 5)	/* usually UDF */
	&&  memcmp (desc->id, "TEA01", 5)) {
		g_set_error (error,
			     REJILLA_MEDIA_ERROR,
			     REJILLA_MEDIA_ERROR_IMAGE_INVALID,
			     _("It does not appear to be a valid ISO image"));
		REJILLA_MEDIA_LOG ("Wrong volume descriptor, got %.5s", desc->id);
		return FALSE;
	}

	return TRUE;
}

gboolean
rejilla_volume_get_size (RejillaVolSrc *vol,
			 gint64 block,
			 gint64 *nb_blocks,
			 GError **error)
{
	gboolean result;
	gchar buffer [ISO9660_BLOCK_SIZE];

	if (block && REJILLA_VOL_SRC_SEEK (vol, block, SEEK_SET, error) == -1)
		return FALSE;

	result = rejilla_volume_get_primary_from_file (vol, buffer, error);
	if (!result)
		return FALSE;

	if (!rejilla_iso9660_is_primary_descriptor (buffer, error))
		return FALSE;

	return rejilla_iso9660_get_size (buffer, nb_blocks, error);
}

RejillaVolFile *
rejilla_volume_get_files (RejillaVolSrc *vol,
			  gint64 block,
			  gchar **label,
			  gint64 *nb_blocks,
			  gint64 *data_blocks,
			  GError **error)
{
	gchar buffer [ISO9660_BLOCK_SIZE];

	if (REJILLA_VOL_SRC_SEEK (vol, block, SEEK_SET, error) == -1)
		return FALSE;

	if (!rejilla_volume_get_primary_from_file (vol, buffer, error))
		return NULL;

	if (!rejilla_iso9660_is_primary_descriptor (buffer, error))
		return NULL;

	if (label
	&& !rejilla_iso9660_get_label (buffer, label, error))
		return NULL;

	if (nb_blocks
	&& !rejilla_iso9660_get_size (buffer, nb_blocks, error))
		return NULL;

	return rejilla_iso9660_get_contents (vol,
					     buffer,
					     data_blocks,
					     error);
}

GList *
rejilla_volume_load_directory_contents (RejillaVolSrc *vol,
					gint64 session_block,
					gint64 block,
					GError **error)
{
	gchar buffer [ISO9660_BLOCK_SIZE];

	if (REJILLA_VOL_SRC_SEEK (vol, session_block, SEEK_SET, error) == -1)
		return FALSE;

	if (!rejilla_volume_get_primary_from_file (vol, buffer, error))
		return NULL;

	if (!rejilla_iso9660_is_primary_descriptor (buffer, error))
		return NULL;

	return rejilla_iso9660_get_directory_contents (vol,
						       buffer,
						       block,
						       error);
}

RejillaVolFile *
rejilla_volume_get_file (RejillaVolSrc *vol,
			 const gchar *path,
			 gint64 volume_start_block,
			 GError **error)
{
	gchar buffer [ISO9660_BLOCK_SIZE];

	if (REJILLA_VOL_SRC_SEEK (vol, volume_start_block, SEEK_SET, error) == -1)
		return NULL;

	if (!rejilla_volume_get_primary_from_file (vol, buffer, error))
		return NULL;

	if (!rejilla_iso9660_is_primary_descriptor (buffer, error))
		return NULL;

	return rejilla_iso9660_get_file (vol, path, buffer, error);
}

gchar *
rejilla_volume_file_to_path (RejillaVolFile *file)
{
	GString *path;
	RejillaVolFile *parent;
	GSList *components = NULL, *iter, *next;

	if (!file)
		return NULL;

	/* make a list of all the components of the path by going up to root */
	parent = file->parent;
	while (parent && parent->name) {
		components = g_slist_prepend (components, REJILLA_VOLUME_FILE_NAME (parent));
		parent = parent->parent;
	}

	if (!components)
		return NULL;

	path = g_string_new (NULL);
	for (iter = components; iter; iter = next) {
		gchar *name;

		name = iter->data;
		next = iter->next;
		components = g_slist_remove (components, name);

		g_string_append_c (path, G_DIR_SEPARATOR);
		g_string_append (path, name);
	}

	g_slist_free (components);
	return g_string_free (path, FALSE);
}

RejillaVolFile *
rejilla_volume_file_from_path (const gchar *ptr,
			       RejillaVolFile *parent)
{
	GList *iter;
	gchar *next;
	gint len;

	/* first determine the name of the directory / file to look for */
	if (!ptr || ptr [0] != '/' || !parent)
		return NULL;

	ptr ++;
	next = g_utf8_strchr (ptr, -1, G_DIR_SEPARATOR);
	if (!next)
		len = strlen (ptr);
	else
		len = next - ptr;

	for (iter = parent->specific.dir.children; iter; iter = iter->next) {
		RejillaVolFile *file;

		file = iter->data;
		if (!strncmp (ptr, REJILLA_VOLUME_FILE_NAME (file), len)) {
			/* we've found it seek for the next if any */
			if (!next)
				return file;

			ptr = next;
			return rejilla_volume_file_from_path (ptr, file);
		}
	}

	return NULL;
}

gint64
rejilla_volume_file_size (RejillaVolFile *file)
{
	GList *iter;
	gint64 size = 0;

	if (!file->isdir) {
		GSList *extents;

		for (extents = file->specific.file.extents; extents; extents = extents->next) {
			RejillaVolFileExtent *extent;

			extent = extents->data;
			size += extent->size;
		}
		return REJILLA_BYTES_TO_SECTORS (size, 2048);
	}

	for (iter = file->specific.dir.children; iter; iter = iter->next) {
		file = iter->data;

		if (file->isdir)
			size += rejilla_volume_file_size (file);
		else
			size += REJILLA_BYTES_TO_SECTORS (file->specific.file.size_bytes, 2048);
	}

	return size;
}

RejillaVolFile *
rejilla_volume_file_merge (RejillaVolFile *file1,
			   RejillaVolFile *file2)
{
	file1->specific.file.size_bytes += file2->specific.file.size_bytes;
	file1->specific.file.extents = g_slist_concat (file1->specific.file.extents,
							     file2->specific.file.extents);

	file2->specific.file.extents = NULL;
	rejilla_volume_file_free (file2);

	return file1;
}
