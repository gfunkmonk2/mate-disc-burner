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

#include "scsi-mmc1.h"

#include "scsi-error.h"
#include "scsi-utils.h"
#include "scsi-base.h"
#include "scsi-command.h"
#include "scsi-opcodes.h"
#include "scsi-read-disc-info.h"

#if G_BYTE_ORDER == G_LITTLE_ENDIAN

struct _RejillaRdDiscInfoCDB {
	uchar opcode;

	uchar data_type		:3;
	uchar reserved0		:5;

	uchar reserved1		[5];
	uchar alloc_len		[2];

	uchar ctl;
};

#else

struct _RejillaRdDiscInfoCDB {
	uchar opcode;

	uchar reserved0		:5;
	uchar data_type		:3;

	uchar reserved1		[5];
	uchar alloc_len		[2];

	uchar ctl;
};

#endif

typedef struct _RejillaRdDiscInfoCDB RejillaRdDiscInfoCDB;

REJILLA_SCSI_COMMAND_DEFINE (RejillaRdDiscInfoCDB,
			     READ_DISC_INFORMATION,
			     REJILLA_SCSI_READ);

typedef enum {
REJILLA_DISC_INFO_STD		= 0x00,
REJILLA_DISC_INFO_TRACK_RES	= 0x01,
REJILLA_DISC_INFO_POW_RES	= 0x02,
	/* reserved */
} RejillaDiscInfoType;


RejillaScsiResult
rejilla_mmc1_read_disc_information_std (RejillaDeviceHandle *handle,
					RejillaScsiDiscInfoStd **info_return,
					int *size,
					RejillaScsiErrCode *error)
{
	RejillaScsiDiscInfoStd std_info;
	RejillaScsiDiscInfoStd *buffer;
	RejillaRdDiscInfoCDB *cdb;
	RejillaScsiResult res;
	int request_size;
	int buffer_size;

	g_return_val_if_fail (handle != NULL, REJILLA_SCSI_FAILURE);

	if (!info_return || !size) {
		REJILLA_SCSI_SET_ERRCODE (error, REJILLA_SCSI_BAD_ARGUMENT);
		return REJILLA_SCSI_FAILURE;
	}

	cdb = rejilla_scsi_command_new (&info, handle);
	cdb->data_type = REJILLA_DISC_INFO_STD;
	REJILLA_SET_16 (cdb->alloc_len, sizeof (RejillaScsiDiscInfoStd));

	memset (&std_info, 0, sizeof (RejillaScsiDiscInfoStd));
	res = rejilla_scsi_command_issue_sync (cdb,
					       &std_info,
					       sizeof (RejillaScsiDiscInfoStd),
					       error);
	if (res)
		goto end;

	request_size = REJILLA_GET_16 (std_info.len) + 
		       sizeof (std_info.len);
	
	buffer = (RejillaScsiDiscInfoStd *) g_new0 (uchar, request_size);

	REJILLA_SET_16 (cdb->alloc_len, request_size);
	res = rejilla_scsi_command_issue_sync (cdb, buffer, request_size, error);
	if (res) {
		g_free (buffer);
		goto end;
	}

	buffer_size = REJILLA_GET_16 (buffer->len) +
		      sizeof (buffer->len);

	if (request_size != buffer_size)
		REJILLA_MEDIA_LOG ("Sizes mismatch asked %i / received %i",
				  request_size,
				  buffer_size);

	*info_return = buffer;
	*size = MIN (request_size, buffer_size);

end:

	rejilla_scsi_command_free (cdb);
	return res;
}

#if 0

/* These functions are not used for now but may
 * be one day. So keep them around but turn 
 * them off to avoid build warnings */
 
RejillaScsiResult
rejilla_mmc5_read_disc_information_tracks (RejillaDeviceHandle *handle,
					   RejillaScsiTrackResInfo *info_return,
					   int size,
					   RejillaScsiErrCode *error)
{
	RejillaRdDiscInfoCDB *cdb;
	RejillaScsiResult res;

	cdb = rejilla_scsi_command_new (&info, handle);
	cdb->data_type = REJILLA_DISC_INFO_TRACK_RES;
	REJILLA_SET_16 (cdb->alloc_len, size);

	memset (info_return, 0, size);
	res = rejilla_scsi_command_issue_sync (cdb, info_return, size, error);
	rejilla_scsi_command_free (cdb);
	return res;
}

RejillaScsiResult
rejilla_mmc5_read_disc_information_pows (RejillaDeviceHandle *handle,
					 RejillaScsiPOWResInfo *info_return,
					 int size,
					 RejillaScsiErrCode *error)
{
	RejillaRdDiscInfoCDB *cdb;
	RejillaScsiResult res;

	cdb = rejilla_scsi_command_new (&info, handle);
	cdb->data_type = REJILLA_DISC_INFO_POW_RES;
	REJILLA_SET_16 (cdb->alloc_len, size);

	memset (info_return, 0, size);
	res = rejilla_scsi_command_issue_sync (cdb, info_return, size, error);
	rejilla_scsi_command_free (cdb);
	return res;
}

#endif
