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

#include "scsi-mmc2.h"

#include "rejilla-media-private.h"

#include "scsi-error.h"
#include "scsi-base.h"
#include "scsi-command.h"
#include "scsi-opcodes.h"
#include "scsi-read-disc-structure.h"

#if G_BYTE_ORDER == G_LITTLE_ENDIAN

struct _RejillaReadDiscStructureCDB {
	uchar opcode;

	uchar media_type		:4;
	uchar reserved0			:4;

	/* for formats 0x83 */
	uchar address			[4];
	uchar layer_num;

	uchar format;

	uchar alloc_len			[2];

	uchar reserved1			:6;

	/* for formats 0x02, 0x06, 0x07, 0x80, 0x82 */
	uchar agid			:2;

	uchar ctl;
};

#else

struct _RejillaReadDiscStructureCDB {
	uchar opcode;

	uchar reserved0			:4;
	uchar media_type		:4;

	uchar address			[4];

	uchar layer_num;
	uchar format;

	uchar alloc_len			[2];

	uchar agid			:2;
	uchar reserved1			:6;

	uchar ctl;
};

#endif

typedef struct _RejillaReadDiscStructureCDB RejillaReadDiscStructureCDB;

REJILLA_SCSI_COMMAND_DEFINE (RejillaReadDiscStructureCDB,
			     READ_DISC_STRUCTURE,
			     REJILLA_SCSI_READ);

typedef enum {
REJILLA_MEDIA_DVD_HD_DVD			= 0x00,
REJILLA_MEDIA_BD				= 0x01
	/* reserved */
} RejillaScsiMediaType;

static RejillaScsiResult
rejilla_read_disc_structure (RejillaReadDiscStructureCDB *cdb,
			     RejillaScsiReadDiscStructureHdr **data,
			     int *size,
			     RejillaScsiErrCode *error)
{
	RejillaScsiReadDiscStructureHdr *buffer;
	RejillaScsiReadDiscStructureHdr hdr;
	RejillaScsiResult res;
	int request_size;

	if (!data || !size) {
		REJILLA_SCSI_SET_ERRCODE (error, REJILLA_SCSI_BAD_ARGUMENT);
		return REJILLA_SCSI_FAILURE;
	}

	REJILLA_SET_16 (cdb->alloc_len, sizeof (hdr));

	memset (&hdr, 0, sizeof (hdr));
	res = rejilla_scsi_command_issue_sync (cdb, &hdr, sizeof (hdr), error);
	if (res)
		return res;

	request_size = REJILLA_GET_16 (hdr.len) + sizeof (hdr.len);
	buffer = (RejillaScsiReadDiscStructureHdr *) g_new0 (uchar, request_size);

	REJILLA_SET_16 (cdb->alloc_len, request_size);
	res = rejilla_scsi_command_issue_sync (cdb, buffer, request_size, error);
	if (res) {
		g_free (buffer);
		return res;
	}

	if (request_size < REJILLA_GET_16 (buffer->len) + sizeof (buffer->len)) {
		REJILLA_SCSI_SET_ERRCODE (error, REJILLA_SCSI_SIZE_MISMATCH);
		g_free (buffer);
		return REJILLA_SCSI_FAILURE;
	}

	*data = buffer;
	*size = request_size;

	return res;
}

RejillaScsiResult
rejilla_mmc2_read_generic_structure (RejillaDeviceHandle *handle,
				     RejillaScsiGenericFormatType type,
				     RejillaScsiReadDiscStructureHdr **data,
				     int *size,
				     RejillaScsiErrCode *error)
{
	RejillaReadDiscStructureCDB *cdb;
	RejillaScsiResult res;

	g_return_val_if_fail (handle != NULL, REJILLA_SCSI_FAILURE);

	cdb = rejilla_scsi_command_new (&info, handle);
	cdb->format = type;

	res = rejilla_read_disc_structure (cdb, data, size, error);
	rejilla_scsi_command_free (cdb);
	return res;
}

#if 0

/* So far this function only creates a warning at
 * build time and is not used but may be in the
 * future. */

RejillaScsiResult
rejilla_mmc2_read_dvd_structure (RejillaDeviceHandle *handle,
				 int address,
				 RejillaScsiDVDFormatType type,
				 RejillaScsiReadDiscStructureHdr **data,
				 int *size,
				 RejillaScsiErrCode *error)
{
	RejillaReadDiscStructureCDB *cdb;
	RejillaScsiResult res;

	cdb = rejilla_scsi_command_new (&info, handle);
	cdb->format = type;
	cdb->media_type = REJILLA_MEDIA_DVD_HD_DVD;
	REJILLA_SET_32 (cdb->address, address);

	res = rejilla_read_disc_structure (cdb, data, size, error);
	rejilla_scsi_command_free (cdb);
	return res;
}

RejillaScsiResult
rejilla_mmc5_read_bd_structure (RejillaDeviceHandle *handle,
				RejillaScsiBDFormatType type,
				RejillaScsiReadDiscStructureHdr **data,
				int *size,
				RejillaScsiErrCode *error)
{
	RejillaReadDiscStructureCDB *cdb;
	RejillaScsiResult res;

	cdb = rejilla_scsi_command_new (&info, handle);
	cdb->format = type;
	cdb->media_type = REJILLA_MEDIA_BD;

	res = rejilla_read_disc_structure (cdb, data, size, error);
	rejilla_scsi_command_free (cdb);
	return res;
}

#endif
