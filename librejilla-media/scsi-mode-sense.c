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

#include <glib.h>

#include "rejilla-media-private.h"

#include "scsi-spc1.h"

#include "scsi-error.h"
#include "scsi-utils.h"
#include "scsi-base.h"
#include "scsi-command.h"
#include "scsi-opcodes.h"
#include "scsi-mode-pages.h"

/**
 * MODE SENSE command description (defined in SPC, Scsi Primary Commands) 
 */

#if G_BYTE_ORDER == G_LITTLE_ENDIAN

struct _RejillaModeSenseCDB {
	uchar opcode		:8;

	uchar reserved1		:3;
	uchar dbd		:1;
	uchar llbaa		:1;
	uchar reserved0		:3;

	uchar page_code		:8;
	uchar subpage_code	:8;

	uchar reserved2		[3];

	uchar alloc_len		[2];
	uchar ctl;
};

#else

struct _RejillaModeSenseCDB {
	uchar opcode		:8;

	uchar reserved0		:3;
	uchar llbaa		:1;
	uchar dbd		:1;
	uchar reserved1		:3;

	uchar page_code		:8;
	uchar subpage_code	:8;

	uchar reserved2		[3];

	uchar alloc_len		[2];
	uchar ctl;
};

#endif

typedef struct _RejillaModeSenseCDB RejillaModeSenseCDB;

REJILLA_SCSI_COMMAND_DEFINE (RejillaModeSenseCDB,
			     MODE_SENSE,
			     REJILLA_SCSI_READ);

#define REJILLA_MODE_DATA(data)			((RejillaScsiModeData *) (data))

RejillaScsiResult
rejilla_spc1_mode_sense_get_page (RejillaDeviceHandle *handle,
				  RejillaSPCPageType num,
				  RejillaScsiModeData **data,
				  int *data_size,
				  RejillaScsiErrCode *error)
{
	int page_size;
	int buffer_size;
	int request_size;
	RejillaScsiResult res;
	RejillaModeSenseCDB *cdb;
	RejillaScsiModeData header;
	RejillaScsiModeData *buffer;

	g_return_val_if_fail (handle != NULL, REJILLA_SCSI_FAILURE);

	if (!data || !data_size) {
		REJILLA_SCSI_SET_ERRCODE (error, REJILLA_SCSI_BAD_ARGUMENT);
		return REJILLA_SCSI_FAILURE;
	}

	/* issue a first command to get the size of the page ... */
	cdb = rejilla_scsi_command_new (&info, handle);
	cdb->dbd = 1;
	cdb->page_code = num;
	REJILLA_SET_16 (cdb->alloc_len, sizeof (header));
	bzero (&header, sizeof (header));

	REJILLA_MEDIA_LOG ("Getting page size");
	res = rejilla_scsi_command_issue_sync (cdb, &header, sizeof (header), error);
	if (res)
		goto end;

	if (!header.hdr.len) {
		REJILLA_SCSI_SET_ERRCODE (error, REJILLA_SCSI_SIZE_MISMATCH);
		res = REJILLA_SCSI_FAILURE;
		goto end;
	}

	/* Paranoïa, make sure:
	 * - the size given in header, the one of the page returned are coherent
	 * - the block descriptors are actually disabled */
	if (REJILLA_GET_16 (header.hdr.bdlen)) {
		REJILLA_SCSI_SET_ERRCODE (error, REJILLA_SCSI_BAD_ARGUMENT);
		REJILLA_MEDIA_LOG ("Block descriptors not disabled %i", REJILLA_GET_16 (header.hdr.bdlen));
		res = REJILLA_SCSI_FAILURE;
		goto end;
	}

	request_size = REJILLA_GET_16 (header.hdr.len) +
		       G_STRUCT_OFFSET (RejillaScsiModeHdr, len) +
		       sizeof (header.hdr.len);

	page_size = header.page.len +
		    G_STRUCT_OFFSET (RejillaScsiModePage, len) +
		    sizeof (header.page.len);

	if (request_size != page_size + sizeof (RejillaScsiModeHdr)) {
		REJILLA_SCSI_SET_ERRCODE (error, REJILLA_SCSI_SIZE_MISMATCH);
		REJILLA_MEDIA_LOG ("Incoherent answer sizes: request %i, page %i", request_size, page_size);
		res = REJILLA_SCSI_FAILURE;
		goto end;
	}

	/* ... allocate an appropriate buffer ... */
	buffer = (RejillaScsiModeData *) g_new0 (uchar, request_size);

	/* ... re-issue the command */
	REJILLA_MEDIA_LOG("Getting page (size = %i)", request_size);

	REJILLA_SET_16 (cdb->alloc_len, request_size);
	res = rejilla_scsi_command_issue_sync (cdb, buffer, request_size, error);
	if (res) {
		g_free (buffer);
		goto end;
	}

	/* Paranoïa, some more checks:
	 * - the size of the page returned is the one we requested
	 * - block descriptors are actually disabled
	 * - header claimed size == buffer size
	 * - header claimed size == sizeof (header) + sizeof (page) */
	buffer_size = REJILLA_GET_16 (buffer->hdr.len) +
		      G_STRUCT_OFFSET (RejillaScsiModeHdr, len) +
		      sizeof (buffer->hdr.len);

	page_size = buffer->page.len +
		    G_STRUCT_OFFSET (RejillaScsiModePage, len) +
		    sizeof (buffer->page.len);

	if (request_size != buffer_size
	||  REJILLA_GET_16 (buffer->hdr.bdlen)
	||  buffer_size != page_size + sizeof (RejillaScsiModeHdr)) {
		g_free (buffer);

		REJILLA_SCSI_SET_ERRCODE (error, REJILLA_SCSI_SIZE_MISMATCH);
		res = REJILLA_SCSI_FAILURE;
		goto end;
	}

	*data = buffer;
	*data_size = request_size;

end:
	rejilla_scsi_command_free (cdb);
	return res;
}
