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

#include <glib.h>
#include <glib/gi18n-lib.h>

#include "rejilla-units.h"
#include "rejilla-media.h"
#include "rejilla-media-private.h"
#include "burn-iso9660.h"
#include "burn-iso-field.h"
#include "burn-susp.h"
#include "rejilla-media.h"
#include "burn-volume.h"

struct _RejillaIsoCtx {
	gint num_blocks;

	gchar buffer [ISO9660_BLOCK_SIZE];
	gint offset;
	RejillaVolSrc *vol;

	gchar *spare_record;

	guint64 data_blocks;
	GError *error;

	guchar susp_skip;

	guint is_root:1;
	guint has_susp:1;
	guint has_RR:1;
};
typedef struct _RejillaIsoCtx RejillaIsoCtx;

typedef enum {
REJILLA_ISO_FILE_EXISTENCE		= 1,
REJILLA_ISO_FILE_DIRECTORY		= 1 << 1,
REJILLA_ISO_FILE_ASSOCIATED		= 1 << 2,
REJILLA_ISO_FILE_RECORD			= 1 << 3,
REJILLA_ISO_FILE_PROTECTION		= 1 << 4,
	/* Reserved */
REJILLA_ISO_FILE_MULTI_EXTENT_FINAL	= 1 << 7
} RejillaIsoFileFlag;

struct _RejillaIsoDirRec {
	guchar record_size;
	guchar x_attr_size;
	guchar address			[8];
	guchar file_size		[8];
	guchar date_time		[7];
	guchar flags;
	guchar file_unit;
	guchar gap_size;
	guchar volseq_num		[4];
	guchar id_size;
	gchar id			[0];
};
typedef struct _RejillaIsoDirRec RejillaIsoDirRec;

struct _RejillaIsoPrimary {
	guchar type;
	gchar id			[5];
	guchar version;

	guchar unused_0;

	gchar system_id			[32];
	gchar vol_id			[32];

	guchar unused_1			[8];

	guchar vol_size			[8];

	guchar escapes			[32];
	guchar volset_size		[4];
	guchar sequence_num		[4];
	guchar block_size		[4];
	guchar path_table_size		[8];
	guchar L_table_loc		[4];
	guchar opt_L_table_loc		[4];
	guchar M_table_loc		[4];
	guchar opt_M_table_loc		[4];

	/* the following has a fixed size of 34 bytes */
	RejillaIsoDirRec root_rec	[0];

	/* to be continued if needed */
};
typedef struct _RejillaIsoPrimary RejillaIsoPrimary;

typedef enum {
	REJILLA_ISO_OK,
	REJILLA_ISO_END,
	REJILLA_ISO_ERROR
} RejillaIsoResult;

#define ISO9660_BYTES_TO_BLOCKS(size)			REJILLA_BYTES_TO_SECTORS ((size), ISO9660_BLOCK_SIZE)

static GList *
rejilla_iso9660_load_directory_records (RejillaIsoCtx *ctx,
					RejillaVolFile *parent,
					RejillaIsoDirRec *record,
					gboolean recursive);

static RejillaVolFile *
rejilla_iso9660_lookup_directory_records (RejillaIsoCtx *ctx,
					  const gchar *path,
					  gint address);

gboolean
rejilla_iso9660_is_primary_descriptor (const char *buffer,
				       GError **error)
{
	RejillaVolDesc *vol;

	/* must be CD001 */
	vol = (RejillaVolDesc *) buffer;
	if (memcmp (vol->id, "CD001", 5)) {
		g_set_error (error,
			     REJILLA_MEDIA_ERROR,
			     REJILLA_MEDIA_ERROR_IMAGE_INVALID,
			     _("It does not appear to be a valid ISO image"));
		return FALSE;
	}

	/* must be "1" for primary volume */
	if (vol->type != 1) {
		g_set_error (error,
			     REJILLA_MEDIA_ERROR,
			     REJILLA_MEDIA_ERROR_IMAGE_INVALID,
			     _("It does not appear to be a valid ISO image"));
		return FALSE;
	}

	return TRUE;
}

gboolean
rejilla_iso9660_get_size (const gchar *block,
			  gint64 *nb_blocks,
			  GError **error)
{
	RejillaIsoPrimary *vol;

	/* read the size of the volume */
	vol = (RejillaIsoPrimary *) block;
	*nb_blocks = (gint64) rejilla_iso9660_get_733_val (vol->vol_size);

	return TRUE;
}

gboolean
rejilla_iso9660_get_label (const gchar *block,
			   gchar **label,
			   GError **error)
{
	RejillaIsoPrimary *vol;

	/* read the identifier */
	vol = (RejillaIsoPrimary *) block;
	*label = g_strndup (vol->vol_id, sizeof (vol->vol_id));

	return TRUE;	
}

static RejillaIsoResult
rejilla_iso9660_seek (RejillaIsoCtx *ctx, gint address)
{
	ctx->offset = 0;
	ctx->num_blocks = 1;

	/* The size of all the records is given by size member and its location
	 * by its address member. In a set of directory records the first two 
	 * records are: '.' (id == 0) and '..' (id == 1). So since we've got
	 * the address of the set load the block. */
	if (REJILLA_VOL_SRC_SEEK (ctx->vol, address, SEEK_SET, &(ctx->error)) == -1)
		return REJILLA_ISO_ERROR;

	if (!REJILLA_VOL_SRC_READ (ctx->vol, ctx->buffer, 1, &(ctx->error)))
		return REJILLA_ISO_ERROR;

	return REJILLA_ISO_OK;
}

static RejillaIsoResult
rejilla_iso9660_next_block (RejillaIsoCtx *ctx)
{
	ctx->offset = 0;
	ctx->num_blocks ++;

	if (!REJILLA_VOL_SRC_READ (ctx->vol, ctx->buffer, 1, &(ctx->error)))
		return REJILLA_ISO_ERROR;

	return REJILLA_ISO_OK;
}

static gboolean
rejilla_iso9660_read_susp (RejillaIsoCtx *ctx,
			   RejillaSuspCtx *susp_ctx,
			   gchar *susp,
			   gint susp_len)
{
	gboolean result = TRUE;
	guint64 current_position = -1;

	memset (susp_ctx, 0, sizeof (RejillaSuspCtx));
	if (!rejilla_susp_read (susp_ctx, susp, susp_len)) {
		REJILLA_MEDIA_LOG ("Could not read susp area");
		return FALSE;
	}

	while (susp_ctx->CE_address) {
		gchar CE_block [ISO9660_BLOCK_SIZE];
		gint64 seek_res;
		guint32 offset;
		guint32 len;

		REJILLA_MEDIA_LOG ("Continuation Area");

		/* we need to move to another block */
		seek_res = REJILLA_VOL_SRC_SEEK (ctx->vol, susp_ctx->CE_address, SEEK_SET, NULL);
		if (seek_res == -1) {
			REJILLA_MEDIA_LOG ("Could not seek to continuation area");
			result = FALSE;
			break;
		}

		if (current_position == -1)
			current_position = seek_res;

		if (!REJILLA_VOL_SRC_READ (ctx->vol, CE_block, 1, NULL)) {
			REJILLA_MEDIA_LOG ("Could not get continuation area");
			result = FALSE;
			break;
		}

		offset = susp_ctx->CE_offset;
		len = MIN (susp_ctx->CE_len, sizeof (CE_block) - offset);

		/* reset information about the CE area */
		memset (&susp_ctx->CE_address, 0, sizeof (susp_ctx->CE_address));
		memset (&susp_ctx->CE_offset, 0, sizeof (susp_ctx->CE_offset));
		memset (&susp_ctx->CE_len, 0, sizeof (susp_ctx->CE_len));

		/* read all information contained in the CE area */
		if (!rejilla_susp_read (susp_ctx, CE_block + offset, len)) {
			REJILLA_MEDIA_LOG ("Could not read continuation area");
			result = FALSE;
			break;
		}
	}

	/* reset the reading address properly */
	if (current_position != -1
	&&  REJILLA_VOL_SRC_SEEK (ctx->vol, current_position, SEEK_SET, NULL) == -1) {
		REJILLA_MEDIA_LOG ("Could not rewind to previous position");
		result = FALSE;
	}

	return result;
}

static gchar *
rejilla_iso9660_get_susp (RejillaIsoCtx *ctx,
			  RejillaIsoDirRec *record,
			  guint *susp_len)
{
	gchar *susp_block;
	gint start;
	gint len;

	start = sizeof (RejillaIsoDirRec) + record->id_size;
	/* padding byte when id_size is an even number */
	if (start & 1)
		start ++;

	if (ctx->susp_skip)
		start += ctx->susp_skip;

	/* we don't want to go beyond end of buffer */
	if (start >= record->record_size)
		return NULL;

	len = record->record_size - start;

	if (len <= 0)
		return NULL;

	if (susp_len)
		*susp_len = len;

	susp_block = ((gchar *) record) + start;

	REJILLA_MEDIA_LOG ("Got susp block");
	return susp_block;
}

static RejillaIsoResult
rejilla_iso9660_next_record (RejillaIsoCtx *ctx, RejillaIsoDirRec **retval)
{
	RejillaIsoDirRec *record;

	if (ctx->offset > sizeof (ctx->buffer)) {
		REJILLA_MEDIA_LOG ("Invalid record size");
		goto error;
	}

	if (ctx->offset == sizeof (ctx->buffer)) {
		REJILLA_MEDIA_LOG ("No next record");
		return REJILLA_ISO_END;
	}

	/* record_size already checked last time function was called */
	record = (RejillaIsoDirRec *) (ctx->buffer + ctx->offset);
	if (!record->record_size) {
		REJILLA_MEDIA_LOG ("Last record");
		return REJILLA_ISO_END;
	}

	if (record->record_size > (sizeof (ctx->buffer) - ctx->offset)) {
		gint part_one, part_two;

		/* This is for cross sector boundary records */
		REJILLA_MEDIA_LOG ("Cross sector boundary record");

		/* some implementations write across block boundary which is
		 * "forbidden" by ECMA-119. But linux kernel accepts it, so ...
		 */
/*		ctx->error = g_error_new (REJILLA_MEDIA_ERROR,
					  REJILLA_MEDIA_ERROR_IMAGE_INVALID,
					  _("It does not appear to be a valid ISO image"));
		goto error;
*/
		if (ctx->spare_record)
			g_free (ctx->spare_record);

		ctx->spare_record = g_new0 (gchar, record->record_size);

		part_one = sizeof (ctx->buffer) - ctx->offset;
		part_two = record->record_size - part_one;
		
		memcpy (ctx->spare_record,
			ctx->buffer + ctx->offset,
			part_one);
		
		if (rejilla_iso9660_next_block (ctx) == REJILLA_ISO_ERROR)
			goto error;

		memcpy (ctx->spare_record + part_one,
			ctx->buffer,
			part_two);
		ctx->offset = part_two;

		record = (RejillaIsoDirRec *) ctx->spare_record;
	}
	else
		ctx->offset += record->record_size;

	*retval = record;
	return REJILLA_ISO_OK;

error:
	if (!ctx->error)
		ctx->error = g_error_new (REJILLA_MEDIA_ERROR,
					  REJILLA_MEDIA_ERROR_IMAGE_INVALID,
					  _("It does not appear to be a valid ISO image"));
	return REJILLA_ISO_ERROR;
}

static RejillaIsoResult
rejilla_iso9660_get_first_directory_record (RejillaIsoCtx *ctx,
					    RejillaIsoDirRec **record,
					    gint address)
{
	RejillaIsoResult result;

	REJILLA_MEDIA_LOG ("Reading directory record");

	result = rejilla_iso9660_seek (ctx, address);
	if (result != REJILLA_ISO_OK)
		return REJILLA_ISO_ERROR;

	/* load "." */
	result = rejilla_iso9660_next_record (ctx, record);
	if (result != REJILLA_ISO_OK)
		return REJILLA_ISO_ERROR;

	return REJILLA_ISO_OK;
}

static RejillaVolFile *
rejilla_iso9660_read_file_record (RejillaIsoCtx *ctx,
				  RejillaIsoDirRec *record,
				  RejillaSuspCtx *susp_ctx)
{
	RejillaVolFile *file;
	RejillaVolFileExtent *extent;

	if (record->id_size > record->record_size - sizeof (RejillaIsoDirRec)) {
		REJILLA_MEDIA_LOG ("Filename is too long");
		ctx->error = g_error_new (REJILLA_MEDIA_ERROR,
					  REJILLA_MEDIA_ERROR_IMAGE_INVALID,
					  _("It does not appear to be a valid ISO image"));
		return NULL;
	}

	file = g_new0 (RejillaVolFile, 1);
	file->isdir = 0;
	file->name = g_new0 (gchar, record->id_size + 1);
	memcpy (file->name, record->id, record->id_size);

	file->specific.file.size_bytes = rejilla_iso9660_get_733_val (record->file_size);

	/* NOTE: a file can be in multiple places */
	extent = g_new (RejillaVolFileExtent, 1);
	extent->block = rejilla_iso9660_get_733_val (record->address);
	extent->size = rejilla_iso9660_get_733_val (record->file_size);
	file->specific.file.extents = g_slist_prepend (file->specific.file.extents, extent);

	/* see if we've got a susp area */
	if (!susp_ctx) {
		REJILLA_MEDIA_LOG ("New file %s", file->name);
		return file;
	}

	REJILLA_MEDIA_LOG ("New file %s with a suspend area", file->name);

	if (susp_ctx->rr_name) {
		REJILLA_MEDIA_LOG ("Got a susp (RR) %s", susp_ctx->rr_name);
		file->rr_name = susp_ctx->rr_name;
		susp_ctx->rr_name = NULL;
	}

	return file;
}

static RejillaVolFile *
rejilla_iso9660_read_directory_record (RejillaIsoCtx *ctx,
				       RejillaIsoDirRec *record,
				       gboolean recursive)
{
	gchar *susp;
	gint address;
	guint susp_len = 0;
	RejillaSuspCtx susp_ctx;
	RejillaVolFile *directory;

	if (record->id_size > record->record_size - sizeof (RejillaIsoDirRec)) {
		REJILLA_MEDIA_LOG ("Filename is too long");
		ctx->error = g_error_new (REJILLA_MEDIA_ERROR,
					  REJILLA_MEDIA_ERROR_IMAGE_INVALID,
					  _("It does not appear to be a valid ISO image"));
		return NULL;
	}

	/* create the directory and set information */
	directory = g_new0 (RejillaVolFile, 1);
	directory->isdir = TRUE;
	directory->isdir_loaded = FALSE;
	directory->name = g_new0 (gchar, record->id_size + 1);
	memcpy (directory->name, record->id, record->id_size);

	if (ctx->has_susp && ctx->has_RR) {
		/* See if we've got a susp area. Do it now to see if it has a CL
		 * entry. The rest will be checked later after reading contents.
		 */
		susp = rejilla_iso9660_get_susp (ctx, record, &susp_len);
		if (!rejilla_iso9660_read_susp (ctx, &susp_ctx, susp, susp_len)) {
			REJILLA_MEDIA_LOG ("Could not read susp area");
			rejilla_volume_file_free (directory);
			return NULL;
		}

		/* look for a "CL" SUSP entry in case the directory was relocated */
		if (susp_ctx.CL_address) {
			REJILLA_MEDIA_LOG ("Entry has a CL entry");
			address = susp_ctx.CL_address;
		}
		else
			address = rejilla_iso9660_get_733_val (record->address);

		REJILLA_MEDIA_LOG ("New directory %s with susp area", directory->name);

		/* if this directory has a "RE" susp entry then drop it; it's 
		 * not at the right place in the Rock Ridge file hierarchy. It
		 * will probably be skipped */
		if (susp_ctx.has_RE) {
			REJILLA_MEDIA_LOG ("Rock Ridge relocated directory. Skipping entry.");
			directory->relocated = TRUE;
		}

		if (susp_ctx.rr_name) {
			REJILLA_MEDIA_LOG ("Got a susp (RR) %s", susp_ctx.rr_name);
			directory->rr_name = susp_ctx.rr_name;
			susp_ctx.rr_name = NULL;
		}

		rejilla_susp_ctx_clean (&susp_ctx);
	}
	else
		address = rejilla_iso9660_get_733_val (record->address);

	/* load contents if recursive */
	if (recursive) {
		GList *children;

		rejilla_iso9660_get_first_directory_record (ctx,
							    &record,
							    address);
		children = rejilla_iso9660_load_directory_records (ctx,
								   directory,
								   record,
								   TRUE);
		if (!children && ctx->error) {
			rejilla_volume_file_free (directory);
			if (ctx->has_susp && ctx->has_RR)
				rejilla_susp_ctx_clean (&susp_ctx);

			return NULL;
		}

		directory->isdir_loaded = TRUE;
		directory->specific.dir.children = children;
	}
	else	/* store the address of contents for later use */
		directory->specific.dir.address = address;

	REJILLA_MEDIA_LOG ("New directory %s", directory->name);
	return directory;
}

static GList *
rejilla_iso9660_load_directory_records (RejillaIsoCtx *ctx,
					RejillaVolFile *parent,
					RejillaIsoDirRec *record,
					gboolean recursive)
{
	GSList *iter;
	gint max_block;
	gint max_record_size;
	RejillaVolFile *entry;
	GList *children = NULL;
	RejillaIsoResult result;
	GSList *directories = NULL;

	max_record_size = rejilla_iso9660_get_733_val (record->file_size);
	max_block = ISO9660_BYTES_TO_BLOCKS (max_record_size);
	REJILLA_MEDIA_LOG ("Maximum directory record length %i block (= %i bytes)", max_block, max_record_size);

	/* skip ".." */
	result = rejilla_iso9660_next_record (ctx, &record);
	if (result != REJILLA_ISO_OK)
		return NULL;

	REJILLA_MEDIA_LOG ("Skipped '.' and '..'");

	while (1) {
		RejillaIsoResult result;

		result = rejilla_iso9660_next_record (ctx, &record);
		if (result == REJILLA_ISO_END) {
			if (ctx->num_blocks >= max_block)
				break;

			result = rejilla_iso9660_next_block (ctx);
			if (result != REJILLA_ISO_OK)
				goto error;

			continue;
		}
		else if (result == REJILLA_ISO_ERROR)
			goto error;

		if (!record)
			break;

		/* if it's a directory, keep the record for later (we don't 
		 * want to change the reading offset for the moment) */
		if (record->flags & REJILLA_ISO_FILE_DIRECTORY) {
			gpointer copy;

			copy = g_new0 (gchar, record->record_size);
			memcpy (copy, record, record->record_size);
			directories = g_slist_prepend (directories, copy);
			continue;
		}

		if (ctx->has_RR) {
			RejillaSuspCtx susp_ctx = { NULL, };
			guint susp_len = 0;
			gchar *susp;

			/* See if we've got a susp area. Do it now to see if it
			 * has a CL entry. The rest will be checked later after
			 * reading contents. Otherwise we wouldn't be able to 
			 * get deep directories that are flagged as files. */
			susp = rejilla_iso9660_get_susp (ctx, record, &susp_len);
			if (!rejilla_iso9660_read_susp (ctx, &susp_ctx, susp, susp_len)) {
				REJILLA_MEDIA_LOG ("Could not read susp area");
				goto error;
			}

			/* look for a "CL" SUSP entry in case the directory was
			 * relocated. If it has, it's a directory and keep it
			 * for later. */
			if (susp_ctx.CL_address) {
				gpointer copy;

				REJILLA_MEDIA_LOG ("Entry has a CL entry, keeping for later");
				copy = g_new0 (gchar, record->record_size);
				memcpy (copy, record, record->record_size);
				directories = g_slist_prepend (directories, copy);

				rejilla_susp_ctx_clean (&susp_ctx);
				memset (&susp_ctx, 0, sizeof (RejillaSuspCtx));
				continue;
			}

			entry = rejilla_iso9660_read_file_record (ctx, record, &susp_ctx);
			rejilla_susp_ctx_clean (&susp_ctx);
		}
		else
			entry = rejilla_iso9660_read_file_record (ctx, record, NULL);

		if (!entry)
			goto error;

		entry->parent = parent;

		/* check that we don't have another file record for the
		 * same file (usually files > 4G). It always follows
		 * its sibling */
		if (children) {
			RejillaVolFile *last;

			last = children->data;
			if (!last->isdir && !strcmp (REJILLA_VOLUME_FILE_NAME (last), REJILLA_VOLUME_FILE_NAME (entry))) {
				/* add size and addresses */
				ctx->data_blocks += ISO9660_BYTES_TO_BLOCKS (entry->specific.file.size_bytes);
				last = rejilla_volume_file_merge (last, entry);
				REJILLA_MEDIA_LOG ("Multi extent file");
				continue;
			}
		}

		children = g_list_prepend (children, entry);
		ctx->data_blocks += ISO9660_BYTES_TO_BLOCKS (entry->specific.file.size_bytes);
	}

	/* Takes care of the directories: we accumulate them not to change the
	 * offset of file descriptor FILE */
	for (iter = directories; iter; iter = iter->next) {
		record = iter->data;

		entry = rejilla_iso9660_read_directory_record (ctx, record, recursive);
		if (!entry)
			goto error;

		if (entry->relocated) {
			rejilla_volume_file_free (entry);
			continue;
		}

		entry->parent = parent;
		children = g_list_prepend (children, entry);
	}
	g_slist_foreach (directories, (GFunc) g_free, NULL);
	g_slist_free (directories);

	return children;

error:

	g_list_foreach (children, (GFunc) rejilla_volume_file_free, NULL);
	g_list_free (children);

	g_slist_foreach (directories, (GFunc) g_free, NULL);
	g_slist_free (directories);

	return NULL;
}

static gboolean
rejilla_iso9660_check_SUSP_RR_use (RejillaIsoCtx *ctx,
				   RejillaIsoDirRec *record)
{
	RejillaSuspCtx susp_ctx;
	guint susp_len = 0;
	gchar *susp;

	susp = rejilla_iso9660_get_susp (ctx, record, &susp_len);
	rejilla_iso9660_read_susp (ctx, &susp_ctx, susp, susp_len);

	ctx->has_susp = susp_ctx.has_SP;
	ctx->has_RR = susp_ctx.has_RockRidge;
	ctx->susp_skip = susp_ctx.skip;
	ctx->is_root = FALSE;

	if (ctx->has_susp)
		REJILLA_MEDIA_LOG ("File system supports system use sharing protocol");

	if (ctx->has_RR)
		REJILLA_MEDIA_LOG ("File system has Rock Ridge extension");

	rejilla_susp_ctx_clean (&susp_ctx);
	return TRUE;
}

static void
rejilla_iso9660_ctx_init (RejillaIsoCtx *ctx, RejillaVolSrc *vol)
{
	memset (ctx, 0, sizeof (RejillaIsoCtx));

	ctx->is_root = TRUE;
	ctx->vol = vol;
	ctx->offset = 0;

	/* to fully initialize the context we need the root directory record */
}

RejillaVolFile *
rejilla_iso9660_get_contents (RejillaVolSrc *vol,
			      const gchar *block,
			      gint64 *data_blocks,
			      GError **error)
{
	RejillaIsoPrimary *primary;
	RejillaIsoDirRec *record;
	RejillaVolFile *volfile;
	RejillaIsoDirRec *root;
	RejillaIsoCtx ctx;
	GList *children;
	gint address;

	primary = (RejillaIsoPrimary *) block;
	root = primary->root_rec;

	/* check settings */
	address = rejilla_iso9660_get_733_val (root->address);
	rejilla_iso9660_ctx_init (&ctx, vol);
	rejilla_iso9660_get_first_directory_record (&ctx, &record, address);
	rejilla_iso9660_check_SUSP_RR_use (&ctx, record);

	/* create volume file */
	volfile = g_new0 (RejillaVolFile, 1);
	volfile->isdir = TRUE;
	volfile->isdir_loaded = FALSE;

	children = rejilla_iso9660_load_directory_records (&ctx,
							   volfile,
							   record,
							   TRUE);
	volfile->specific.dir.children = children;

	if (ctx.spare_record)
		g_free (ctx.spare_record);

	if (data_blocks)
		*data_blocks = ctx.data_blocks;

	if (!children && ctx.error) {
		if (error)
			g_propagate_error (error, ctx.error);

		rejilla_volume_file_free (volfile);
		volfile = NULL;
	}

	return volfile;
}

static RejillaVolFile *
rejilla_iso9660_lookup_directory_record_RR (RejillaIsoCtx *ctx,
					    const gchar *path,
					    guint len,
					    RejillaIsoDirRec *record)
{
	RejillaVolFile *entry = NULL;
	RejillaSuspCtx susp_ctx;
	gchar record_name [256];
	guint susp_len = 0;
	gchar *susp;

	/* See if we've got a susp area. Do it now to see if it
	 * has a CL entry and rr_name. */
	susp = rejilla_iso9660_get_susp (ctx, record, &susp_len);
	if (!rejilla_iso9660_read_susp (ctx, &susp_ctx, susp, susp_len)) {
		REJILLA_MEDIA_LOG ("Could not read susp area");
		return NULL;
	}

	/* set name */
	if (!susp_ctx.rr_name) {
		memcpy (record_name, record->id, record->id_size);
		record_name [record->id_size] = '\0';
	}
	else
		strcpy (record_name, susp_ctx.rr_name);

	if (!(record->flags & REJILLA_ISO_FILE_DIRECTORY)) {
		if (len) {
			/* Look for a "CL" SUSP entry in case it was
			 * relocated. If it has, it's a directory. */
			if (susp_ctx.CL_address && !strncmp (record_name, path, len)) {
				/* move path forward */
				path += len;
				path ++;

				entry = rejilla_iso9660_lookup_directory_records (ctx,
										  path,
										  susp_ctx.CL_address);
			}
		}
		else if (!strcmp (record_name, path))
			entry = rejilla_iso9660_read_file_record (ctx,
								  record,
								  &susp_ctx);
	}
	else if (len && !strncmp (record_name, path, len)) {
		gint address;

		/* move path forward */
		path += len;
		path ++;

		address = rejilla_iso9660_get_733_val (record->address);
		entry = rejilla_iso9660_lookup_directory_records (ctx,
								  path,
								  address);
	}

	rejilla_susp_ctx_clean (&susp_ctx);
	return entry;
}

static RejillaVolFile *
rejilla_iso9660_lookup_directory_record_ISO (RejillaIsoCtx *ctx,
					     const gchar *path,
					     guint len,
					     RejillaIsoDirRec *record)
{
	RejillaVolFile *entry = NULL;

	if (!(record->flags & REJILLA_ISO_FILE_DIRECTORY)) {
		if (!len && !strncmp (record->id, path, record->id_size))
			entry = rejilla_iso9660_read_file_record (ctx,
								  record,
								  NULL);
	}
	else if (len && !strncmp (record->id, path, record->id_size)) {
		gint address;

		/* move path forward */
		path += len;
		path ++;

		address = rejilla_iso9660_get_733_val (record->address);
		entry = rejilla_iso9660_lookup_directory_records (ctx,
								  path,
								  address);
	}

	return entry;
}

static RejillaVolFile *
rejilla_iso9660_lookup_directory_records (RejillaIsoCtx *ctx,
					  const gchar *path,
					  gint address)
{
	guint len;
	gchar *end;
	gint max_block;
	gint max_record_size;
	RejillaIsoResult result;
	RejillaIsoDirRec *record;
	RejillaVolFile *file = NULL;

	REJILLA_MEDIA_LOG ("Reading directory record");

	result = rejilla_iso9660_seek (ctx, address);
	if (result != REJILLA_ISO_OK)
		return NULL;

	/* "." */
	result = rejilla_iso9660_next_record (ctx, &record);
	if (result != REJILLA_ISO_OK)
		return NULL;

	/* Look for "SP" SUSP if it's root directory. Also look for "ER" which
	 * should tell us whether Rock Ridge could be used. */
	if (ctx->is_root) {
		RejillaSuspCtx susp_ctx;
		guint susp_len = 0;
		gchar *susp;

		susp = rejilla_iso9660_get_susp (ctx, record, &susp_len);
		rejilla_iso9660_read_susp (ctx, &susp_ctx, susp, susp_len);

		ctx->has_susp = susp_ctx.has_SP;
		ctx->has_RR = susp_ctx.has_RockRidge;
		ctx->susp_skip = susp_ctx.skip;
		ctx->is_root = FALSE;

		rejilla_susp_ctx_clean (&susp_ctx);

		if (ctx->has_susp)
			REJILLA_MEDIA_LOG ("File system supports system use sharing protocol");

		if (ctx->has_RR)
			REJILLA_MEDIA_LOG ("File system has Rock Ridge extension");
	}

	max_record_size = rejilla_iso9660_get_733_val (record->file_size);
	max_block = ISO9660_BYTES_TO_BLOCKS (max_record_size);
	REJILLA_MEDIA_LOG ("Maximum directory record length %i block (= %i bytes)", max_block, max_record_size);

	/* skip ".." */
	result = rejilla_iso9660_next_record (ctx, &record);
	if (result != REJILLA_ISO_OK)
		return NULL;

	REJILLA_MEDIA_LOG ("Skipped '.' and '..'");

	end = strchr (path, '/');
	if (!end)
		/* reached the final file */
		len = 0;
	else
		len = end - path;

	while (1) {
		RejillaIsoResult result;
		RejillaVolFile *entry;

		result = rejilla_iso9660_next_record (ctx, &record);
		if (result == REJILLA_ISO_END) {
			if (ctx->num_blocks >= max_block) {
				REJILLA_MEDIA_LOG ("Reached the end of directory record");
				break;
			}

			result = rejilla_iso9660_next_block (ctx);
			if (result != REJILLA_ISO_OK) {
				REJILLA_MEDIA_LOG ("Failed to load next block");
				return NULL;
			}

			continue;
		}
		else if (result == REJILLA_ISO_ERROR) {
			REJILLA_MEDIA_LOG ("Error retrieving next record");
			return NULL;
		}

		if (!record) {
			REJILLA_MEDIA_LOG ("No record !!!");
			break;
		}

		if (ctx->has_RR)
			entry = rejilla_iso9660_lookup_directory_record_RR (ctx,
									    path,
									    len,
									    record);
		else
			entry = rejilla_iso9660_lookup_directory_record_ISO (ctx,
									     path,
									     len,
									     record);

		if (!entry)
			continue;

		if (file) {
			/* add size and addresses */
			file = rejilla_volume_file_merge (file, entry);
			REJILLA_MEDIA_LOG ("Multi extent file");
		}
		else
			file = entry;

		/* carry on in case that's a multi extent file */
	}

	return file;
}

RejillaVolFile *
rejilla_iso9660_get_file (RejillaVolSrc *vol,
			  const gchar *path,
			  const gchar *block,
			  GError **error)
{
	RejillaIsoPrimary *primary;
	RejillaIsoDirRec *root;
	RejillaVolFile *entry;
	RejillaIsoCtx ctx;
	gint address;

	primary = (RejillaIsoPrimary *) block;
	root = primary->root_rec;

	address = rejilla_iso9660_get_733_val (root->address);
	rejilla_iso9660_ctx_init (&ctx, vol);

	/* now that we have root block address, skip first "/" and go. */
	path ++;
	entry = rejilla_iso9660_lookup_directory_records (&ctx,
							  path,
							  address);

	/* clean context */
	if (ctx.spare_record)
		g_free (ctx.spare_record);

	if (error && ctx.error)
		g_propagate_error (error, ctx.error);

	return entry;
}

GList *
rejilla_iso9660_get_directory_contents (RejillaVolSrc *vol,
					const gchar *vol_desc,
					gint address,
					GError **error)
{
	RejillaIsoDirRec *record = NULL;
	RejillaIsoPrimary *primary;
	RejillaIsoDirRec *root;
	RejillaIsoCtx ctx;
	GList *children;

	/* Check root "." for use of RR and things like that */
	primary = (RejillaIsoPrimary *) vol_desc;
	root = primary->root_rec;

	rejilla_iso9660_ctx_init (&ctx, vol);
	rejilla_iso9660_get_first_directory_record (&ctx,
						    &record,
						    rejilla_iso9660_get_733_val (root->address));
	rejilla_iso9660_check_SUSP_RR_use (&ctx, record);

	/* Seek up to the contents of the directory */
	if (address > 0)
		rejilla_iso9660_get_first_directory_record (&ctx,
							    &record,
							    address);

	/* load */
	children = rejilla_iso9660_load_directory_records (&ctx,
							   NULL,
							   record,
							   FALSE);
	if (ctx.error && error)
		g_propagate_error (error, ctx.error);

	return children;
}
