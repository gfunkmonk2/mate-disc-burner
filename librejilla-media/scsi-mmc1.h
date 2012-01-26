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

#include "scsi-base.h"
#include "scsi-device.h"
#include "scsi-error.h"
#include "scsi-read-cd.h"
#include "scsi-read-disc-info.h"
#include "scsi-read-toc-pma-atip.h"
#include "scsi-read-track-information.h"
#include "scsi-mech-status.h"

#ifndef _BURN_MMC1_H
#define _BURN_MMC1_H

G_BEGIN_DECLS

RejillaScsiResult
rejilla_mmc1_read_disc_information_std (RejillaDeviceHandle *handle,
					RejillaScsiDiscInfoStd **info_return,
					int *size,
					RejillaScsiErrCode *error);

RejillaScsiResult
rejilla_mmc1_read_toc_formatted (RejillaDeviceHandle *handle,
				 int track_num,
				 RejillaScsiFormattedTocData **data,
				 int *size,
				 RejillaScsiErrCode *error);
RejillaScsiResult
rejilla_mmc1_read_toc_raw (RejillaDeviceHandle *handle,
			   int session_num,
			   RejillaScsiRawTocData **data,
			   int *size,
			   RejillaScsiErrCode *error);
RejillaScsiResult
rejilla_mmc1_read_atip (RejillaDeviceHandle *handle,
			RejillaScsiAtipData **data,
			int *size,
			RejillaScsiErrCode *error);

RejillaScsiResult
rejilla_mmc1_read_track_info (RejillaDeviceHandle *handle,
			      int track_num,
			      RejillaScsiTrackInfo *track_info,
			      int *size,
			      RejillaScsiErrCode *error);

RejillaScsiResult
rejilla_mmc1_read_block (RejillaDeviceHandle *handle,
			 gboolean user_data,
			 RejillaScsiBlockType type,
			 RejillaScsiBlockHeader header,
			 RejillaScsiBlockSubChannel channel,
			 int start,
			 int size,
			 unsigned char *buffer,
			 int buffer_len,
			 RejillaScsiErrCode *error);
RejillaScsiResult
rejilla_mmc1_mech_status (RejillaDeviceHandle *handle,
			  RejillaScsiMechStatusHdr *hdr,
			  RejillaScsiErrCode *error);

G_END_DECLS

#endif /* _BURN_MMC1_H */

 
