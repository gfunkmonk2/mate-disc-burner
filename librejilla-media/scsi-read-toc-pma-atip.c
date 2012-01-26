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
#include "scsi-mmc3.h"

#include "scsi-error.h"
#include "scsi-utils.h"
#include "scsi-base.h"
#include "scsi-command.h"
#include "scsi-opcodes.h"
#include "scsi-read-toc-pma-atip.h"

#if G_BYTE_ORDER == G_LITTLE_ENDIAN

struct _RejillaRdTocPmaAtipCDB {
	uchar opcode;

	uchar reserved0		:1;
	uchar msf		:1;
	uchar reserved1		:6;

	uchar format		:4;
	uchar reserved2		:4;

	uchar reserved3		[3];
	uchar track_session_num;

	uchar alloc_len		[2];
	uchar ctl;
};

#else

struct _RejillaRdTocPmaAtipCDB {
	uchar opcode;

	uchar reserved1		:6;
	uchar msf		:1;
	uchar reserved0		:1;

	uchar reserved2		:4;
	uchar format		:4;

	uchar reserved3		[3];
	uchar track_session_num;

	uchar alloc_len		[2];
	uchar ctl;
};

#endif

typedef struct _RejillaRdTocPmaAtipCDB RejillaRdTocPmaAtipCDB;

REJILLA_SCSI_COMMAND_DEFINE (RejillaRdTocPmaAtipCDB,
			     READ_TOC_PMA_ATIP,
			     REJILLA_SCSI_READ);

typedef enum {
REJILLA_RD_TAP_FORMATTED_TOC		= 0x00,
REJILLA_RD_TAP_MULTISESSION_INFO	= 0x01,
REJILLA_RD_TAP_RAW_TOC			= 0x02,
REJILLA_RD_TAP_PMA			= 0x03,
REJILLA_RD_TAP_ATIP			= 0x04,
REJILLA_RD_TAP_CD_TEXT			= 0x05	/* Introduced in MMC3 */
} RejillaReadTocPmaAtipType;

static RejillaScsiResult
rejilla_read_toc_pma_atip (RejillaRdTocPmaAtipCDB *cdb,
			   int desc_size,
			   RejillaScsiTocPmaAtipHdr **data,
			   int *size,
			   RejillaScsiErrCode *error)
{
	RejillaScsiTocPmaAtipHdr *buffer;
	RejillaScsiTocPmaAtipHdr hdr;
	RejillaScsiResult res;
	int request_size;
	int buffer_size;

	if (!data || !size) {
		REJILLA_SCSI_SET_ERRCODE (error, REJILLA_SCSI_BAD_ARGUMENT);
		return REJILLA_SCSI_FAILURE;
	}

	REJILLA_SET_16 (cdb->alloc_len, sizeof (RejillaScsiTocPmaAtipHdr));

	memset (&hdr, 0, sizeof (RejillaScsiTocPmaAtipHdr));
	res = rejilla_scsi_command_issue_sync (cdb,
					       &hdr,
					       sizeof (RejillaScsiTocPmaAtipHdr),
					       error);
	if (res) {
		*size = 0;
		return res;
	}

	request_size = REJILLA_GET_16 (hdr.len) + sizeof (hdr.len);

	/* NOTE: if size is not valid use the maximum possible size */
	if ((request_size - sizeof (hdr)) % desc_size) {
		REJILLA_MEDIA_LOG ("Unaligned data (%i) setting to max (65530)", request_size);
		request_size = 65530;
	}
	else if (request_size - sizeof (hdr) < desc_size) {
		REJILLA_MEDIA_LOG ("Undersized data (%i) setting to max (65530)", request_size);
		request_size = 65530;
	}

	buffer = (RejillaScsiTocPmaAtipHdr *) g_new0 (uchar, request_size);

	REJILLA_SET_16 (cdb->alloc_len, request_size);
	res = rejilla_scsi_command_issue_sync (cdb, buffer, request_size, error);
	if (res) {
		g_free (buffer);
		*size = 0;
		return res;
	}

	buffer_size = REJILLA_GET_16 (buffer->len) + sizeof (buffer->len);

	*data = buffer;
	*size = MIN (buffer_size, request_size);

	return res;
}

/**
 * Returns TOC data for all the sessions starting with track_num
 */

RejillaScsiResult
rejilla_mmc1_read_toc_formatted (RejillaDeviceHandle *handle,
				 int track_num,
				 RejillaScsiFormattedTocData **data,
				 int *size,
				 RejillaScsiErrCode *error)
{
	RejillaRdTocPmaAtipCDB *cdb;
	RejillaScsiResult res;

	g_return_val_if_fail (handle != NULL, REJILLA_SCSI_FAILURE);

	cdb = rejilla_scsi_command_new (&info, handle);
	cdb->format = REJILLA_RD_TAP_FORMATTED_TOC;

	/* first track for which this function will return information */
	cdb->track_session_num = track_num;

	res = rejilla_read_toc_pma_atip (cdb,
					 sizeof (RejillaScsiTocDesc),
					(RejillaScsiTocPmaAtipHdr **) data,
					 size,
					 error);
	rejilla_scsi_command_free (cdb);
	return res;
}

/**
 * Returns RAW TOC data
 */

RejillaScsiResult
rejilla_mmc1_read_toc_raw (RejillaDeviceHandle *handle,
			   int session_num,
			   RejillaScsiRawTocData **data,
			   int *size,
			   RejillaScsiErrCode *error)
{
	RejillaRdTocPmaAtipCDB *cdb;
	RejillaScsiResult res;

	g_return_val_if_fail (handle != NULL, REJILLA_SCSI_FAILURE);

	cdb = rejilla_scsi_command_new (&info, handle);
	cdb->format = REJILLA_RD_TAP_RAW_TOC;

	/* first session for which this function will return information */
	cdb->track_session_num = session_num;

	res = rejilla_read_toc_pma_atip (cdb,
					 sizeof (RejillaScsiRawTocDesc),
					(RejillaScsiTocPmaAtipHdr **) data,
					 size,
					 error);
	rejilla_scsi_command_free (cdb);
	return res;
}

/**
 * Returns CD-TEXT information recorded in the leadin area as R-W sub-channel
 */

RejillaScsiResult
rejilla_mmc3_read_cd_text (RejillaDeviceHandle *handle,
			   RejillaScsiCDTextData **data,
			   int *size,
			   RejillaScsiErrCode *error)
{
	RejillaRdTocPmaAtipCDB *cdb;
	RejillaScsiResult res;

	g_return_val_if_fail (handle != NULL, REJILLA_SCSI_FAILURE);

	cdb = rejilla_scsi_command_new (&info, handle);
	cdb->format = REJILLA_RD_TAP_CD_TEXT;

	res = rejilla_read_toc_pma_atip (cdb,
					 sizeof (RejillaScsiCDTextPackData),
					(RejillaScsiTocPmaAtipHdr **) data,
					 size,
					 error);
	rejilla_scsi_command_free (cdb);
	return res;
}

/**
 * Returns ATIP information
 */

RejillaScsiResult
rejilla_mmc1_read_atip (RejillaDeviceHandle *handle,
			RejillaScsiAtipData **data,
			int *size,
			RejillaScsiErrCode *error)
{
	RejillaRdTocPmaAtipCDB *cdb;
	RejillaScsiResult res;

	g_return_val_if_fail (handle != NULL, REJILLA_SCSI_FAILURE);

	/* In here we have to ask how many bytes the drive wants to return first
	 * indeed there is a difference in the descriptor size between MMC1/MMC2
	 * and MMC3. */
	cdb = rejilla_scsi_command_new (&info, handle);
	cdb->format = REJILLA_RD_TAP_ATIP;
	cdb->msf = 1;				/* specs says it's compulsory */

	/* FIXME: sizeof (RejillaScsiTocDesc) is not really good here but that
	 * avoids the unaligned message */
	res = rejilla_read_toc_pma_atip (cdb,
					 sizeof (RejillaScsiTocDesc),
					(RejillaScsiTocPmaAtipHdr **) data,
					 size,
					 error);
	rejilla_scsi_command_free (cdb);
	return res;
}
