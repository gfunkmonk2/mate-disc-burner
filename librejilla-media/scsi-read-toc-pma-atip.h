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

#ifndef _SCSI_READ_TOC_PMA_ATIP_H
#define _SCSI_READ_TOC_PMA_ATIP_H

G_BEGIN_DECLS

#if G_BYTE_ORDER == G_LITTLE_ENDIAN

struct _RejillaScsiTocPmaAtipHdr {
	uchar len			[2];

	uchar first_track_session;
	uchar last_track_session;
};

struct _RejillaScsiTocDesc {
	uchar reserved0;

	uchar control			:4; /* RejillaScsiTrackMode 		*/
	uchar adr			:4; /* RejillaScsiQSubChannelProgAreaMode 	*/

	uchar track_num;
	uchar reserved1;

	uchar track_start		[4];
};

struct _RejillaScsiRawTocDesc {
	uchar session_num;

	uchar control			:4; /* RejillaScsiTrackMode 		*/
	uchar adr			:4; /* RejillaScsiQSubChannelLeadinMode	*/

	/* Q sub-channel data */
	uchar track_num;

	uchar point;			/* RejillaScsiQSubChannelLeadinMode5 or RejillaScsiQSubChannelLeadinMode1 */
	uchar min;
	uchar sec;
	uchar frame;

	uchar zero;
	uchar p_min;
	uchar p_sec;
	uchar p_frame;
};

struct _RejillaScsiPmaDesc {
	uchar reserved0;

	uchar control			:4; /* RejillaScsiTrackMode 		*/
	uchar adr			:4; /* RejillaScsiQSubChannelPmaMode 	*/

	uchar track_num;			/* always 0 */

	uchar point;				/* see RejillaScsiQSubChannelPmaMode */
	uchar min;
	uchar sec;
	uchar frame;

	uchar zero;
	uchar p_min;
	uchar p_sec;
	uchar p_frame;
};

struct _RejillaScsiAtipDesc {
	uchar reference_speed		:3;	/* 1 */
	uchar reserved0			:1;
	uchar indicative_target_wrt_pwr	:4;

	uchar reserved1			:6;	/* 2 */
	uchar unrestricted_use		:1;
	uchar bit0			:1;

	uchar A3_valid			:1;	/* 3 */
	uchar A2_valid			:1;
	uchar A1_valid			:1;
	uchar disc_sub_type		:3;
	uchar erasable			:1;
	uchar bit1			:1;

	uchar reserved2;			/* 4 */

	uchar leadin_mn;
	uchar leadin_sec;
	uchar leadin_frame;
	uchar reserved3;			/* 8 */

	/* Additional capacity for high capacity CD-R,
	 * otherwise last possible leadout */
	uchar leadout_mn;
	uchar leadout_sec;
	uchar leadout_frame;
	uchar reserved4;			/* 12 */

	/* Write strategy recording parameters.
	 * See MMC1 and MMC2 for a description. */
	uchar A1_data			[3];
	uchar reserved5;
	
	uchar A2_data			[3];
	uchar reserved6;

	uchar A3_data			[3];
	uchar reserved7;

	/* Be careful here since the following is only true for MMC3. That means
	 * if we use this size with a MMC1/2 drives it returns an error (invalid
	 * field). The following value is not really useful anyway. */
	uchar S4_data			[3];
	uchar reserved8;
};

#else

struct _RejillaScsiTocPmaAtipHdr {
	uchar len			[2];

	uchar first_track_session;
	uchar last_track_session;
};

struct _RejillaScsiTocDesc {
	uchar reserved0;

	uchar adr			:4;
	uchar control			:4;

	uchar track_num;
	uchar reserved1;

	uchar track_start		[4];
};

struct _RejillaScsiRawTocDesc {
	uchar session_num;

	uchar adr			:4;
	uchar control			:4;

	uchar track_num;

	uchar point;
	uchar min;
	uchar sec;
	uchar frame;

	uchar zero;
	uchar p_min;
	uchar p_sec;
	uchar p_frame;
};

struct _RejillaScsiPmaDesc {
	uchar reserved0;

	uchar adr			:4;
	uchar control			:4;

	uchar track_num;

	uchar point;
	uchar min;
	uchar sec;
	uchar frame;

	uchar zero;
	uchar p_min;
	uchar p_sec;
	uchar p_frame;
};

struct _RejillaScsiAtipDesc {
	uchar indicative_target_wrt_pwr	:4;
	uchar reserved0			:1;
	uchar reference_speed		:3;

	uchar bit0			:1;
	uchar unrestricted_use		:1;
	uchar reserved1			:6;

	uchar bit1			:1;
	uchar erasable			:1;
	uchar disc_sub_type		:3;
	uchar A1_valid			:1;
	uchar A2_valid			:1;
	uchar A3_valid			:1;

	uchar reserved2;

	uchar leadin_start_time_mn;
	uchar leadin_start_time_sec;
	uchar leadin_start_time_frame;
	uchar reserved3;

	/* Additional capacity for high capacity CD-R,
	 * otherwise last possible leadout */
	uchar leadout_mn;
	uchar leadout_sec;
	uchar leadout_frame;
	uchar reserved4;

	/* write strategy recording parameters */
	uchar A1_data			[3];
	uchar reserved5;
	
	uchar A2_data			[3];
	uchar reserved6;

	uchar A3_data			[3];
	uchar reserved7;

	uchar S4_data			[3];
	uchar reserved8;
};

#endif

typedef struct _RejillaScsiTocDesc RejillaScsiTocDesc;
typedef struct _RejillaScsiRawTocDesc RejillaScsiRawTocDesc;
typedef struct _RejillaScsiPmaDesc RejillaScsiPmaDesc;
typedef struct _RejillaScsiAtipDesc RejillaScsiAtipDesc;

typedef struct _RejillaScsiTocPmaAtipHdr RejillaScsiTocPmaAtipHdr;

/* multiple toc descriptors may be returned */
struct _RejillaScsiFormattedTocData {
	RejillaScsiTocPmaAtipHdr hdr	[1];
	RejillaScsiTocDesc desc		[0];
};
typedef struct _RejillaScsiFormattedTocData RejillaScsiFormattedTocData;

/* multiple toc descriptors may be returned */
struct _RejillaScsiRawTocData {
	RejillaScsiTocPmaAtipHdr hdr	[1];
	RejillaScsiRawTocDesc desc	[0];
};
typedef struct _RejillaScsiRawTocData RejillaScsiRawTocData;

/* multiple pma descriptors may be returned */
struct _RejillaScsiPmaData {
	RejillaScsiTocPmaAtipHdr hdr	[1];
	RejillaScsiPmaDesc desc		[0];	
};
typedef struct _RejillaScsiPmaData RejillaScsiPmaData;

struct _RejillaScsiAtipData {
	RejillaScsiTocPmaAtipHdr hdr	[1];
	RejillaScsiAtipDesc desc	[1];
};
typedef struct _RejillaScsiAtipData RejillaScsiAtipData;

struct _RejillaScsiMultisessionData {
	RejillaScsiTocPmaAtipHdr hdr	[1];
	RejillaScsiTocDesc desc		[1];
};
typedef struct _RejillaScsiMultisessionData RejillaScsiMultisessionData;

/* Inside a language block, packs must be recorded in that order */
typedef enum {
REJILLA_SCSI_CD_TEXT_ALBUM_TITLE	= 0x80,
REJILLA_SCSI_CD_TEXT_PERFORMER_NAME	= 0x81,
REJILLA_SCSI_CD_TEXT_SONGWRITER_NAME	= 0x82,
REJILLA_SCSI_CD_TEXT_COMPOSER_NAME	= 0x83,
REJILLA_SCSI_CD_TEXT_ARRANGER_NAME	= 0x84,
REJILLA_SCSI_CD_TEXT_ARTIST_NAME	= 0x85,
REJILLA_SCSI_CD_TEXT_DISC_ID_INFO	= 0x86,
REJILLA_SCSI_CD_TEXT_GENRE_ID_INFO	= 0x87,
REJILLA_SCSI_CD_TEXT_TOC_1		= 0x88,
REJILLA_SCSI_CD_TEXT_TOC_2		= 0x89,
REJILLA_SCSI_CD_TEXT_RESERVED_1		= 0x8A,
REJILLA_SCSI_CD_TEXT_RESERVED_2		= 0x8B,
REJILLA_SCSI_CD_TEXT_RESERVED_3		= 0x8C,
REJILLA_SCSI_CD_TEXT_RESERVED_CONTENT	= 0x8D,
REJILLA_SCSI_CD_TEXT_UPC_EAN_ISRC	= 0x8E,
REJILLA_SCSI_CD_TEXT_BLOCK_SIZE		= 0x8F,
} RejillaScsiCDTextPackType;

typedef enum {
	REJILLA_CD_TEXT_8859_1		= 0x00,
	REJILLA_CD_TEXT_ASCII		= 0x01,	/* (7 bit)	*/

	/* Reserved */

	REJILLA_CD_TEXT_KANJI		= 0x80,
	REJILLA_CD_TEXT_KOREAN		= 0x81,
	REJILLA_CD_TEXT_CHINESE		= 0x82	/* Mandarin */
} RejillaScsiCDTextCharset;

struct _RejillaScsiCDTextPackData {
	uchar type;
	uchar track_num;
	uchar pack_num;

	uchar char_pos			:4;	/* byte not used for type 0x8F */
	uchar block_num			:3;
	uchar double_byte		:1;

	uchar text			[12];
	uchar crc			[2];
};
typedef struct _RejillaScsiCDTextPackData RejillaScsiCDTextPackData;

/* Takes two RejillaScsiCDTextPackData (18 bytes) 3 x 12 = 36 bytes */
struct _RejillaScsiCDTextPackCharset {
	char charset;
	char first_track;
	char last_track;
	char copyr_flags;
	char pack_count [16];
	char last_seqnum [8];
	char language_code [8];
};
typedef struct _RejillaScsiCDTextPackCharset RejillaScsiCDTextPackCharset;

struct _RejillaScsiCDTextData {
	RejillaScsiTocPmaAtipHdr hdr	[1];
	RejillaScsiCDTextPackData pack	[0];
};
typedef struct _RejillaScsiCDTextData RejillaScsiCDTextData;

#define REJILLA_SCSI_TRACK_LEADOUT_START	0xAA

G_END_DECLS

#endif /* _SCSI_READ_TOC_PMA_ATIP_H */
