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

#ifndef _REJILLA_ENUM_H_
#define _REJILLA_ENUM_H_

#include <glib.h>

G_BEGIN_DECLS

typedef enum {
	REJILLA_BURN_OK,
	REJILLA_BURN_ERR,
	REJILLA_BURN_RETRY,
	REJILLA_BURN_CANCEL,
	REJILLA_BURN_RUNNING,
	REJILLA_BURN_DANGEROUS,
	REJILLA_BURN_NOT_READY,
	REJILLA_BURN_NOT_RUNNING,
	REJILLA_BURN_NEED_RELOAD,
	REJILLA_BURN_NOT_SUPPORTED,
} RejillaBurnResult;

/* These flags are sorted by importance. That's done to solve the problem of
 * exclusive flags: that way MERGE will always win over any other flag if they
 * are exclusive. On the other hand DAO will always lose. */
typedef enum {
	REJILLA_BURN_FLAG_NONE			= 0,

	/* These flags should always be supported */
	REJILLA_BURN_FLAG_CHECK_SIZE		= 1,
	REJILLA_BURN_FLAG_NOGRACE		= 1 << 1,
	REJILLA_BURN_FLAG_EJECT			= 1 << 2,

	/* These are of great importance for the result */
	REJILLA_BURN_FLAG_MERGE			= 1 << 3,
	REJILLA_BURN_FLAG_MULTI			= 1 << 4,
	REJILLA_BURN_FLAG_APPEND		= 1 << 5,

	REJILLA_BURN_FLAG_BURNPROOF		= 1 << 6,
	REJILLA_BURN_FLAG_NO_TMP_FILES		= 1 << 7,
	REJILLA_BURN_FLAG_DUMMY			= 1 << 8,

	REJILLA_BURN_FLAG_OVERBURN		= 1 << 9,

	REJILLA_BURN_FLAG_BLANK_BEFORE_WRITE	= 1 << 10,
	REJILLA_BURN_FLAG_FAST_BLANK		= 1 << 11,

	/* NOTE: these two are contradictory? */
	REJILLA_BURN_FLAG_DAO			= 1 << 13,
	REJILLA_BURN_FLAG_RAW			= 1 << 14,

	REJILLA_BURN_FLAG_LAST
} RejillaBurnFlag;

typedef enum {
	REJILLA_BURN_ACTION_NONE		= 0,
	REJILLA_BURN_ACTION_GETTING_SIZE,
	REJILLA_BURN_ACTION_CREATING_IMAGE,
	REJILLA_BURN_ACTION_RECORDING,
	REJILLA_BURN_ACTION_BLANKING,
	REJILLA_BURN_ACTION_CHECKSUM,
	REJILLA_BURN_ACTION_DRIVE_COPY,
	REJILLA_BURN_ACTION_FILE_COPY,
	REJILLA_BURN_ACTION_ANALYSING,
	REJILLA_BURN_ACTION_TRANSCODING,
	REJILLA_BURN_ACTION_PREPARING,
	REJILLA_BURN_ACTION_LEADIN,
	REJILLA_BURN_ACTION_RECORDING_CD_TEXT,
	REJILLA_BURN_ACTION_FIXATING,
	REJILLA_BURN_ACTION_LEADOUT,
	REJILLA_BURN_ACTION_START_RECORDING,
	REJILLA_BURN_ACTION_FINISHED,
	REJILLA_BURN_ACTION_EJECTING,
	REJILLA_BURN_ACTION_LAST
} RejillaBurnAction;

typedef enum {
	REJILLA_IMAGE_FS_NONE			= 0,
	REJILLA_IMAGE_FS_ISO			= 1,
	REJILLA_IMAGE_FS_UDF			= 1 << 1,
	REJILLA_IMAGE_FS_JOLIET			= 1 << 2,
	REJILLA_IMAGE_FS_VIDEO			= 1 << 3,

	/* The following one conflict with UDF and JOLIET */
	REJILLA_IMAGE_FS_SYMLINK		= 1 << 4,

	REJILLA_IMAGE_ISO_FS_LEVEL_3		= 1 << 5,
	REJILLA_IMAGE_ISO_FS_DEEP_DIRECTORY	= 1 << 6,
	REJILLA_IMAGE_FS_ANY			= REJILLA_IMAGE_FS_ISO|
						  REJILLA_IMAGE_FS_UDF|
						  REJILLA_IMAGE_FS_JOLIET|
						  REJILLA_IMAGE_FS_SYMLINK|
						  REJILLA_IMAGE_ISO_FS_LEVEL_3|
						  REJILLA_IMAGE_FS_VIDEO|
						  REJILLA_IMAGE_ISO_FS_DEEP_DIRECTORY
} RejillaImageFS;

typedef enum {
	REJILLA_AUDIO_FORMAT_NONE		= 0,
	REJILLA_AUDIO_FORMAT_UNDEFINED		= 1,
	REJILLA_AUDIO_FORMAT_DTS		= 1 << 1,
	REJILLA_AUDIO_FORMAT_RAW		= 1 << 2,
	REJILLA_AUDIO_FORMAT_AC3		= 1 << 3,
	REJILLA_AUDIO_FORMAT_MP2		= 1 << 4,

	REJILLA_AUDIO_FORMAT_44100		= 1 << 5,
	REJILLA_AUDIO_FORMAT_48000		= 1 << 6,


	REJILLA_VIDEO_FORMAT_UNDEFINED		= 1 << 7,
	REJILLA_VIDEO_FORMAT_VCD		= 1 << 8,
	REJILLA_VIDEO_FORMAT_VIDEO_DVD		= 1 << 9,

	REJILLA_METADATA_INFO			= 1 << 10,

	REJILLA_AUDIO_FORMAT_RAW_LITTLE_ENDIAN  = 1 << 11,

	/* Deprecated */
	REJILLA_AUDIO_FORMAT_4_CHANNEL		= 0
} RejillaStreamFormat;

#define REJILLA_STREAM_FORMAT_AUDIO(stream_FORMAT)	((stream_FORMAT) & 0x087F)
#define REJILLA_STREAM_FORMAT_VIDEO(stream_FORMAT)	((stream_FORMAT) & 0x0380)

#define	REJILLA_MIN_STREAM_LENGTH			((gint64) 6 * 1000000000LL)
#define REJILLA_STREAM_LENGTH(start_MACRO, end_MACRO)					\
	((end_MACRO) - (start_MACRO) > REJILLA_MIN_STREAM_LENGTH) ?			\
	((end_MACRO) - (start_MACRO)) : REJILLA_MIN_STREAM_LENGTH

#define REJILLA_STREAM_FORMAT_HAS_VIDEO(format_MACRO)				\
	 ((format_MACRO) & (REJILLA_VIDEO_FORMAT_UNDEFINED|			\
	 		    REJILLA_VIDEO_FORMAT_VCD|				\
	 		    REJILLA_VIDEO_FORMAT_VIDEO_DVD))

typedef enum {
	REJILLA_IMAGE_FORMAT_NONE = 0,
	REJILLA_IMAGE_FORMAT_BIN = 1,
	REJILLA_IMAGE_FORMAT_CUE = 1 << 1,
	REJILLA_IMAGE_FORMAT_CLONE = 1 << 2,
	REJILLA_IMAGE_FORMAT_CDRDAO = 1 << 3,
	REJILLA_IMAGE_FORMAT_ANY = REJILLA_IMAGE_FORMAT_BIN|
	REJILLA_IMAGE_FORMAT_CUE|
	REJILLA_IMAGE_FORMAT_CDRDAO|
	REJILLA_IMAGE_FORMAT_CLONE,
} RejillaImageFormat; 

typedef enum {
	REJILLA_PLUGIN_ERROR_NONE					= 0,
	REJILLA_PLUGIN_ERROR_MODULE,
	REJILLA_PLUGIN_ERROR_MISSING_APP,
	REJILLA_PLUGIN_ERROR_WRONG_APP_VERSION,
	REJILLA_PLUGIN_ERROR_SYMBOLIC_LINK_APP,
	REJILLA_PLUGIN_ERROR_MISSING_LIBRARY,
	REJILLA_PLUGIN_ERROR_LIBRARY_VERSION,
	REJILLA_PLUGIN_ERROR_MISSING_GSTREAMER_PLUGIN,
} RejillaPluginErrorType;

G_END_DECLS

#endif
