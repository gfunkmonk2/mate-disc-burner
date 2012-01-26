/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Librejilla-media
 * Copyright (C) Lin Ma 2008 <lin.ma@sun.com>
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

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include <sys/scsi/scsi.h>
#include <sys/scsi/impl/uscsi.h>

#include "rejilla-media-private.h"
#include "scsi-command.h"
#include "scsi-utils.h"
#include "scsi-error.h"
#include "scsi-sense-data.h"

#define DEBUG REJILLA_MEDIA_LOG

struct _RejillaDeviceHandle {
	int fd;
};

struct _RejillaScsiCmd {
	uchar cmd [REJILLA_SCSI_CMD_MAX_LEN];
	RejillaDeviceHandle *handle;

	const RejillaScsiCmdInfo *info;
};
typedef struct _RejillaScsiCmd RejillaScsiCmd;

#define REJILLA_SCSI_CMD_OPCODE_OFF			0
#define REJILLA_SCSI_CMD_SET_OPCODE(command)		(command->cmd [REJILLA_SCSI_CMD_OPCODE_OFF] = command->info->opcode)

#define OPEN_FLAGS			O_RDONLY | O_NONBLOCK

gchar *
dump_bytes(guchar *buf, gint len)
{
	GString *out;
	out = g_string_new("");
	for(;len > 0; len--) {
		g_string_append_printf(out, "%02X ", *buf++);
	}
	return g_string_free(out, FALSE);
}

void
dump_cdb(guchar *cdb, gint cdblen)
{
	gchar *out = dump_bytes(cdb, cdblen);
	DEBUG("CDB:\t%s", out);
	g_free(out);
}

/**
 * This is to send a command
 */
RejillaScsiResult
rejilla_scsi_command_issue_sync (gpointer command,
				 gpointer buffer,
				 int size,
				 RejillaScsiErrCode *error)
{
	uchar sense_buffer [REJILLA_SENSE_DATA_SIZE];
	struct uscsi_cmd transport;
	int res;
	RejillaScsiCmd *cmd;
	short timeout = 4 * 60;

	memset (&sense_buffer, 0, REJILLA_SENSE_DATA_SIZE);
	memset (&transport, 0, sizeof (struct uscsi_cmd));

	cmd = command;

	if (cmd->info->direction & REJILLA_SCSI_READ)
		transport.uscsi_flags = USCSI_READ;
	else if (cmd->info->direction & REJILLA_SCSI_WRITE)
		transport.uscsi_flags = USCSI_WRITE;

	transport.uscsi_cdb = (caddr_t) cmd->cmd;
	transport.uscsi_cdblen = (uchar_t) cmd->info->size;
	dump_cdb(transport.uscsi_cdb, transport.uscsi_cdblen);
	transport.uscsi_bufaddr = (caddr_t) buffer;
	transport.uscsi_buflen = (size_t) size;
	transport.uscsi_timeout = timeout;

	/* where to output the scsi sense buffer */
	transport.uscsi_flags |= USCSI_RQENABLE | USCSI_SILENT | USCSI_DIAGNOSE;
	transport.uscsi_rqbuf = sense_buffer;
	transport.uscsi_rqlen = REJILLA_SENSE_DATA_SIZE;

	/* NOTE only for TEST UNIT READY, REQUEST/MODE SENSE, INQUIRY, READ
	 * CAPACITY, READ BUFFER, READ and LOG SENSE are allowed with it */
	res = ioctl (cmd->handle->fd, USCSICMD, &transport);

	DEBUG("ret: %d errno: %d (%s)", res,
	    res != 0 ? errno : 0,
	    res != 0 ? g_strerror(errno) : "Error 0");
	DEBUG("uscsi_flags:     0x%x", transport.uscsi_flags);
	DEBUG("uscsi_status:    0x%x", transport.uscsi_status);
	DEBUG("uscsi_timeout:   %d", transport.uscsi_timeout);
	DEBUG("uscsi_bufaddr:   0x%lx", (long)transport.uscsi_bufaddr);
	DEBUG("uscsi_buflen:    %d", (int)transport.uscsi_buflen);
	DEBUG("uscsi_resid:     %d", (int)transport.uscsi_resid);
	DEBUG("uscsi_rqlen:     %d", transport.uscsi_rqlen);
	DEBUG("uscsi_rqstatus:  0x%x", transport.uscsi_rqstatus);
	DEBUG("uscsi_rqresid:   %d", transport.uscsi_rqresid);
	DEBUG("uscsi_rqbuf ptr: 0x%lx", (long)transport.uscsi_rqbuf);
	if (transport.uscsi_rqbuf != NULL && transport.uscsi_rqlen > transport.uscsi_rqresid) {
		int	len = transport.uscsi_rqlen - transport.uscsi_rqresid;
		gchar	*out;

		out = dump_bytes((char *)transport.uscsi_rqbuf, len);
		DEBUG("uscsi_rqbuf:     %s\n", out);
		g_free(out);
	} else {
		DEBUG("uscsi_rqbuf:     <data not available>\n");
	}

	if (res == -1) {
		REJILLA_SCSI_SET_ERRCODE (error, REJILLA_SCSI_ERRNO);
		return REJILLA_SCSI_FAILURE;
	}

	if ((transport.uscsi_status & STATUS_MASK) == STATUS_GOOD)
		return REJILLA_SCSI_OK;

	if ((transport.uscsi_rqstatus & STATUS_MASK == STATUS_CHECK)
	    && transport.uscsi_rqlen)
		return rejilla_sense_data_process (sense_buffer, error);

	return REJILLA_SCSI_FAILURE;
}

gpointer
rejilla_scsi_command_new (const RejillaScsiCmdInfo *info,
			  RejillaDeviceHandle *handle) 
{
	RejillaScsiCmd *cmd;

	/* make sure we can set the flags of the descriptor */

	/* allocate the command */
	cmd = g_new0 (RejillaScsiCmd, 1);
	cmd->info = info;
	cmd->handle = handle;

	REJILLA_SCSI_CMD_SET_OPCODE (cmd);
	return cmd;
}

RejillaScsiResult
rejilla_scsi_command_free (gpointer cmd)
{
	g_free (cmd);
	return REJILLA_SCSI_OK;
}

/**
 * This is to open a device
 */

RejillaDeviceHandle *
rejilla_device_handle_open (const gchar *path,
			    gboolean exclusive,
			    RejillaScsiErrCode *code)
{
	int fd;
	int flags = OPEN_FLAGS;
	RejillaDeviceHandle *handle;

/* 	if (exclusive) */
/* 		flags |= O_EXCL; */

	if (g_str_has_prefix(path, "/dev/dsk/")) {
		gchar *rawdisk;
		rawdisk = g_strdup_printf ("/dev/rdsk/%s", path + 9);
		fd = open (rawdisk, flags);
		g_free(rawdisk);
	} else {
		fd = open (path, flags);
	}

	if (fd < 0) {
		if (code) {
			if (errno == EAGAIN
			||  errno == EWOULDBLOCK
			||  errno == EBUSY)
				*code = REJILLA_SCSI_NOT_READY;
			else
				*code = REJILLA_SCSI_ERRNO;
		}

		DEBUG("open ERR: %s\n", g_strerror(errno));
		return NULL;
	}

	handle = g_new (RejillaDeviceHandle, 1);
	handle->fd = fd;

	return handle;
}

void
rejilla_device_handle_close (RejillaDeviceHandle *handle)
{
	close (handle->fd);
	g_free (handle);
}

char *
rejilla_device_get_bus_target_lun (const gchar *device)
{
	return strdup (device);
}
