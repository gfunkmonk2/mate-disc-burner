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

#include "scsi-device.h"
#include "scsi-error.h"
#include "scsi-utils.h"
#include "scsi-base.h"

#ifndef _BURN_SCSI_COMMAND_H
#define _BURN_SCSI_COMMAND_H

G_BEGIN_DECLS

/* Most scsi commands are <= 16 (apparently some of the new mmc can be longer) */
#define REJILLA_SCSI_CMD_MAX_LEN	16
	
typedef enum {
	REJILLA_SCSI_WRITE	= 1,
	REJILLA_SCSI_READ	= 1 << 1
} RejillaScsiDirection;

struct _RejillaScsiCmdInfo {
	int size;
	uchar opcode;
	RejillaScsiDirection direction;
};
typedef struct _RejillaScsiCmdInfo RejillaScsiCmdInfo;

#define REJILLA_SCSI_COMMAND_DEFINE(cdb, name, direction)			\
static const RejillaScsiCmdInfo info =						\
{	/* SCSI commands always end by 1 byte of ctl */				\
	G_STRUCT_OFFSET (cdb, ctl) + 1, 					\
	REJILLA_##name##_OPCODE,						\
	direction								\
}

gpointer
rejilla_scsi_command_new (const RejillaScsiCmdInfo *info,
			  RejillaDeviceHandle *handle);

RejillaScsiResult
rejilla_scsi_command_free (gpointer command);

RejillaScsiResult
rejilla_scsi_command_issue_sync (gpointer command,
				 gpointer buffer,
				 int size,
				 RejillaScsiErrCode *error);
G_END_DECLS

#endif /* _BURN_SCSI_COMMAND_H */

 
