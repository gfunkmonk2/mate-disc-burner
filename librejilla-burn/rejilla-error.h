/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Librejilla-burn
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

#ifndef _REJILLA_ERROR_H_
#define _REJILLA_ERROR_H_

#include <glib.h>

G_BEGIN_DECLS

typedef enum {
	REJILLA_BURN_ERROR_NONE,
	REJILLA_BURN_ERROR_GENERAL,

	REJILLA_BURN_ERROR_PLUGIN_MISBEHAVIOR,

	REJILLA_BURN_ERROR_SLOW_DMA,
	REJILLA_BURN_ERROR_PERMISSION,
	REJILLA_BURN_ERROR_DRIVE_BUSY,
	REJILLA_BURN_ERROR_DISK_SPACE,

	REJILLA_BURN_ERROR_EMPTY,
	REJILLA_BURN_ERROR_INPUT_INVALID,

	REJILLA_BURN_ERROR_OUTPUT_NONE,

	REJILLA_BURN_ERROR_FILE_INVALID,
	REJILLA_BURN_ERROR_FILE_FOLDER,
	REJILLA_BURN_ERROR_FILE_PLAYLIST,
	REJILLA_BURN_ERROR_FILE_NOT_FOUND,
	REJILLA_BURN_ERROR_FILE_NOT_LOCAL,

	REJILLA_BURN_ERROR_WRITE_MEDIUM,
	REJILLA_BURN_ERROR_WRITE_IMAGE,

	REJILLA_BURN_ERROR_IMAGE_INVALID,
	REJILLA_BURN_ERROR_IMAGE_JOLIET,
	REJILLA_BURN_ERROR_IMAGE_LAST_SESSION,

	REJILLA_BURN_ERROR_MEDIUM_NONE,
	REJILLA_BURN_ERROR_MEDIUM_INVALID,
	REJILLA_BURN_ERROR_MEDIUM_SPACE,
	REJILLA_BURN_ERROR_MEDIUM_NO_DATA,
	REJILLA_BURN_ERROR_MEDIUM_NOT_WRITABLE,
	REJILLA_BURN_ERROR_MEDIUM_NOT_REWRITABLE,
	REJILLA_BURN_ERROR_MEDIUM_NEED_RELOADING,

	REJILLA_BURN_ERROR_BAD_CHECKSUM,

	REJILLA_BURN_ERROR_MISSING_APP_AND_PLUGIN,

	/* these are not necessarily error */
	REJILLA_BURN_WARNING_CHECKSUM,
	REJILLA_BURN_WARNING_INSERT_AFTER_COPY,

	REJILLA_BURN_ERROR_TMP_DIRECTORY
} RejillaBurnError;

/**
 * Error handling and defined return values
 */

GQuark rejilla_burn_quark (void);

#define REJILLA_BURN_ERROR				\
	rejilla_burn_quark()

G_END_DECLS

#endif /* _REJILLA_ERROR_H_ */
