/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Librejilla-media
 * Copyright (C) Philippe Rouquier 2005-2009 <bonfire-app@wanadoo.fr>
 *
 * Librejilla-burn is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * The Librejilla-burn authors hereby grant permission for non-GPL compatible
 * GStreamer plugins to be used and distributed together with GStreamer
 * and Librejilla-burn. This permission is above and beyond the permissions granted
 * by the GPL license by which Librejilla-burn is covered. If you modify this code
 * you may extend this exception to your version of the code, but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version.
 * 
 * Librejilla-burn is distributed in the hope that it will be useful,
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

#ifndef _SCSI_MODE_PAGES_H
#define _SCSI_MODE_PAGES_H

G_BEGIN_DECLS

#if G_BYTE_ORDER == G_LITTLE_ENDIAN

struct _RejillaScsiModePage {
	uchar code			:6;
	uchar reserved			:1;
	uchar ps			:1;

	uchar len;
};

#else

struct _RejillaScsiModePage {
	uchar ps			:1;
	uchar reserved			:1;
	uchar code			:6;

	uchar len;
};

#endif

typedef struct _RejillaScsiModePage RejillaScsiModePage;

struct _RejillaScsiModeHdr {
	uchar len			[2];
	uchar medium_type		:8;
	uchar device_param		:8;
	uchar reserved			[2];
	uchar bdlen			[2];
};
typedef struct _RejillaScsiModeHdr RejillaScsiModeHdr;

struct _RejillaScsiModeData {
	RejillaScsiModeHdr hdr;
	RejillaScsiModePage page;
};
typedef struct _RejillaScsiModeData RejillaScsiModeData;

/**
 * Pages codes
 */

typedef enum {
	REJILLA_SPC_PAGE_NULL		= 0x00,
	REJILLA_SPC_PAGE_WRITE		= 0x05,
	REJILLA_SPC_PAGE_STATUS		= 0x2a,
} RejillaSPCPageType;

G_END_DECLS

#endif /* _SCSI_MODE-PAGES_H */

 
