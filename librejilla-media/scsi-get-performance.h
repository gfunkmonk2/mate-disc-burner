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

#ifndef _BURN_GET_PERFORMANCE_H
#define _BURN_GET_PERFORMANCE_H

G_BEGIN_DECLS

#if G_BYTE_ORDER == G_LITTLE_ENDIAN

struct _RejillaScsiGetPerfHdr {
	uchar len	[4];

	uchar except	:1;
	uchar wrt	:1;
	uchar reserved0	:6;

	uchar reserved1	[3];
};

struct _RejillaScsiWrtSpdDesc {
	uchar mrw	:1;
	uchar exact	:1;
	uchar rdd	:1;
	uchar wrc	:1;
	uchar reserved0	:3;

	uchar reserved1	[3];

	uchar capacity	[4];
	uchar rd_speed	[4];
	uchar wr_speed	[4];
};

#else

struct _RejillaScsiGetPerfHdr {
	uchar len	[4];

	uchar reserved0	:6;
	uchar wrt	:1;
	uchar except	:1;

	uchar reserved1	[3];
};

struct _RejillaScsiWrtSpdDesc {
	uchar reserved0	:3;
	uchar wrc	:1;
	uchar rdd	:1;
	uchar exact	:1;
	uchar mrw	:1;

	uchar reserved1	[3];

	uchar capacity	[4];
	uchar rd_speed	[4];
	uchar wr_speed	[4];
};

#endif

typedef struct _RejillaScsiGetPerfHdr RejillaScsiGetPerfHdr;
typedef struct _RejillaScsiWrtSpdDesc RejillaScsiWrtSpdDesc;

struct _RejillaScsiGetPerfData {
	RejillaScsiGetPerfHdr hdr;
	gpointer data;			/* depends on the request */
};
typedef struct _RejillaScsiGetPerfData RejillaScsiGetPerfData;

G_END_DECLS

#endif /* _BURN_GET_PERFORMANCE_H */

 
