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

#ifndef _REJILLA_TAGS_H_
#define _REJILLA_TAGS_H_

#include <glib.h>

G_BEGIN_DECLS

/**
 * Some defined and usable tags for a track
 */

#define REJILLA_TRACK_MEDIUM_ADDRESS_START_TAG		"track::medium::address::start"
#define REJILLA_TRACK_MEDIUM_ADDRESS_END_TAG		"track::medium::address::end"

/**
 * Array of filenames (on medium) which have a wrong checksum value (G_TYPE_STRV)
 */

#define REJILLA_TRACK_MEDIUM_WRONG_CHECKSUM_TAG		"track::medium::error::checksum::list"

/**
 * Strings
 */

#define REJILLA_TRACK_STREAM_TITLE_TAG			"track::stream::info::title"
#define REJILLA_TRACK_STREAM_COMPOSER_TAG		"track::stream::info::composer"
#define REJILLA_TRACK_STREAM_ARTIST_TAG			"track::stream::info::artist"
#define REJILLA_TRACK_STREAM_ALBUM_TAG			"track::stream::info::album"
#define REJILLA_TRACK_STREAM_THUMBNAIL_TAG		"track::stream::snapshot"
#define REJILLA_TRACK_STREAM_MIME_TAG			"track::stream::mime"

/**
 * Int
 */

#define REJILLA_TRACK_STREAM_ISRC_TAG			"track::stream::info::isrc"

/**
 * This tag (for sessions) is used to set an estimated size, used to determine
 * in the burn option dialog if the selected medium is big enough.
 */

#define REJILLA_DATA_TRACK_SIZE_TAG			"track::data::estimated_size"
#define REJILLA_STREAM_TRACK_SIZE_TAG			"track::stream::estimated_size"

/**
 * Some defined and usable tags for a session
 */

/**
 * Gives the uri (gchar *) of the cover
 */
#define REJILLA_COVER_URI			"session::art::cover"

/**
 * Define the audio streams for a DVD
 */
#define REJILLA_DVD_STREAM_FORMAT		"session::DVD::stream::format"			/* Int */
#define REJILLA_SESSION_STREAM_AUDIO_FORMAT	"session::stream::audio::format"	/* Int */

/**
 * Define the format: whether VCD or SVCD
 */
enum {
	REJILLA_VCD_NONE,
	REJILLA_VCD_V1,
	REJILLA_VCD_V2,
	REJILLA_SVCD
};
#define REJILLA_VCD_TYPE			"session::VCD::format"

/**
 * This is the video format that should be used.
 */
enum {
	REJILLA_VIDEO_FRAMERATE_NATIVE,
	REJILLA_VIDEO_FRAMERATE_NTSC,
	REJILLA_VIDEO_FRAMERATE_PAL_SECAM
};
#define REJILLA_VIDEO_OUTPUT_FRAMERATE		"session::video::framerate"

/**
 * Aspect ratio
 */
enum {
	REJILLA_VIDEO_ASPECT_NATIVE,
	REJILLA_VIDEO_ASPECT_4_3,
	REJILLA_VIDEO_ASPECT_16_9
};
#define REJILLA_VIDEO_OUTPUT_ASPECT		"session::video::aspect"

G_END_DECLS

#endif
