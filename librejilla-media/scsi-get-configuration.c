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

#include "scsi-mmc2.h"

#include "scsi-error.h"
#include "scsi-utils.h"
#include "scsi-base.h"
#include "scsi-command.h"
#include "scsi-opcodes.h"
#include "scsi-get-configuration.h"

#if G_BYTE_ORDER == G_LITTLE_ENDIAN

struct _RejillaGetConfigCDB {
	uchar opcode		:8;

	uchar returned_data	:2;
	uchar reserved0		:6;

	uchar feature_num	[2];

	uchar reserved1		[3];

	uchar alloc_len		[2];

	uchar ctl;
};

#else

struct _RejillaGetConfigCDB {
	uchar opcode		:8;

	uchar reserved1		:6;
	uchar returned_data	:2;

	uchar feature_num	[2];

	uchar reserved0		[3];

	uchar alloc_len		[2];

	uchar ctl;
};

#endif

typedef struct _RejillaGetConfigCDB RejillaGetConfigCDB;

REJILLA_SCSI_COMMAND_DEFINE (RejillaGetConfigCDB,
			     GET_CONFIGURATION,
			     REJILLA_SCSI_READ);

typedef enum {
REJILLA_GET_CONFIG_RETURN_ALL_FEATURES	= 0x00,
REJILLA_GET_CONFIG_RETURN_ALL_CURRENT	= 0x01,
REJILLA_GET_CONFIG_RETURN_ONLY_FEATURE	= 0x02,
} RejillaGetConfigReturnedData;

static RejillaScsiResult
rejilla_get_configuration (RejillaGetConfigCDB *cdb,
			   RejillaScsiGetConfigHdr **data,
			   int *size,
			   RejillaScsiErrCode *error)
{
	RejillaScsiGetConfigHdr *buffer;
	RejillaScsiGetConfigHdr hdr;
	RejillaScsiResult res;
	int request_size;
	int buffer_size;

	if (!data || !size) {
		REJILLA_SCSI_SET_ERRCODE (error, REJILLA_SCSI_BAD_ARGUMENT);
		return REJILLA_SCSI_FAILURE;
	}

	/* first issue the command once ... */
	memset (&hdr, 0, sizeof (hdr));
	REJILLA_SET_16 (cdb->alloc_len, sizeof (hdr));
	res = rejilla_scsi_command_issue_sync (cdb, &hdr, sizeof (hdr), error);
	if (res)
		return res;

	/* ... check the size returned ... */
	request_size = REJILLA_GET_32 (hdr.len) +
		       G_STRUCT_OFFSET (RejillaScsiGetConfigHdr, len) +
		       sizeof (hdr.len);

	/* NOTE: if size is not valid use the maximum possible size */
	if ((request_size - sizeof (hdr)) % 8) {
		REJILLA_MEDIA_LOG ("Unaligned data (%i) setting to max (65530)", request_size);
		request_size = 65530;
	}
	else if (request_size <= sizeof (hdr)) {
		/* NOTE: if there is a feature, the size must be larger than the
		 * header size. */
		REJILLA_MEDIA_LOG ("Undersized data (%i) setting to max (65530)", request_size);
		request_size = 65530;
	}

	/* ... allocate a buffer and re-issue the command */
	buffer = (RejillaScsiGetConfigHdr *) g_new0 (uchar, request_size);

	REJILLA_SET_16 (cdb->alloc_len, request_size);
	res = rejilla_scsi_command_issue_sync (cdb, buffer, request_size, error);
	if (res) {
		g_free (buffer);
		return res;
	}

	/* make sure the response has the requested size */
	buffer_size = REJILLA_GET_32 (buffer->len) +
		      G_STRUCT_OFFSET (RejillaScsiGetConfigHdr, len) +
		      sizeof (hdr.len);

	if (buffer_size < sizeof (RejillaScsiGetConfigHdr) + 2) {
		/* we can't have a size less or equal to that of the header */
		REJILLA_MEDIA_LOG ("Size of buffer is less or equal to size of header");
		g_free (buffer);
		return REJILLA_SCSI_FAILURE;
	}

	if (buffer_size != request_size)
		REJILLA_MEDIA_LOG ("Sizes mismatch asked %i / received %i",
				  request_size,
				  buffer_size);

	*data = buffer;
	*size = MIN (buffer_size, request_size);
	return REJILLA_SCSI_OK;
}

RejillaScsiResult
rejilla_mmc2_get_configuration_feature (RejillaDeviceHandle *handle,
					RejillaScsiFeatureType type,
					RejillaScsiGetConfigHdr **data,
					int *size,
					RejillaScsiErrCode *error)
{
	RejillaScsiGetConfigHdr *hdr = NULL;
	RejillaGetConfigCDB *cdb;
	RejillaScsiResult res;
	int hdr_size = 0;

	g_return_val_if_fail (handle != NULL, REJILLA_SCSI_FAILURE);
	g_return_val_if_fail (data != NULL, REJILLA_SCSI_FAILURE);
	g_return_val_if_fail (size != NULL, REJILLA_SCSI_FAILURE);

	cdb = rejilla_scsi_command_new (&info, handle);
	REJILLA_SET_16 (cdb->feature_num, type);
	cdb->returned_data = REJILLA_GET_CONFIG_RETURN_ONLY_FEATURE;

	res = rejilla_get_configuration (cdb, &hdr, &hdr_size, error);
	rejilla_scsi_command_free (cdb);

	/* make sure the desc is the one we want */
	if (hdr && REJILLA_GET_16 (hdr->desc->code) != type) {
		REJILLA_MEDIA_LOG ("Wrong type returned %d", hdr->desc->code);
		REJILLA_SCSI_SET_ERRCODE (error, REJILLA_SCSI_TYPE_MISMATCH);

		g_free (hdr);
		return REJILLA_SCSI_FAILURE;
	}

	*data = hdr;
	*size = hdr_size;
	return res;
}

RejillaScsiResult
rejilla_mmc2_get_profile (RejillaDeviceHandle *handle,
			  RejillaScsiProfile *profile,
			  RejillaScsiErrCode *error)
{
	RejillaScsiGetConfigHdr hdr;
	RejillaGetConfigCDB *cdb;
	RejillaScsiResult res;

	g_return_val_if_fail (handle != NULL, REJILLA_SCSI_FAILURE);
	g_return_val_if_fail (profile != NULL, REJILLA_SCSI_FAILURE);

	cdb = rejilla_scsi_command_new (&info, handle);
	REJILLA_SET_16 (cdb->feature_num, REJILLA_SCSI_FEAT_CORE);
	cdb->returned_data = REJILLA_GET_CONFIG_RETURN_ONLY_FEATURE;

	memset (&hdr, 0, sizeof (hdr));
	REJILLA_SET_16 (cdb->alloc_len, sizeof (hdr));
	res = rejilla_scsi_command_issue_sync (cdb, &hdr, sizeof (hdr), error);
	rejilla_scsi_command_free (cdb);

	if (res)
		return res;

	*profile = REJILLA_GET_16 (hdr.current_profile);
	return REJILLA_SCSI_OK;
}
