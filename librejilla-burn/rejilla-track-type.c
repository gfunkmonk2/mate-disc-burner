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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib.h>

#include "rejilla-medium.h"
#include "rejilla-drive.h"
#include "rejilla-track-type.h"
#include "rejilla-track-type-private.h"

/**
 * rejilla_track_type_new:
 *
 * Creates a new #RejillaTrackType structure.
 * Free it with rejilla_track_type_free ().
 *
 * Return value: a #RejillaTrackType pointer.
 **/

RejillaTrackType *
rejilla_track_type_new (void)
{
	return g_new0 (RejillaTrackType, 1);
}

/**
 * rejilla_track_type_free:
 * @type: a #RejillaTrackType.
 *
 * Frees #RejillaTrackType structure.
 *
 **/

void
rejilla_track_type_free (RejillaTrackType *type)
{
	if (!type)
		return;

	g_free (type);
}

/**
 * rejilla_track_type_get_image_format:
 * @type: a #RejillaTrackType.
 *
 * Returns the format of an image when
 * rejilla_track_type_get_has_image () returned
 * TRUE.
 *
 * Return value: a #RejillaImageFormat
 **/

RejillaImageFormat
rejilla_track_type_get_image_format (const RejillaTrackType *type) 
{
	g_return_val_if_fail (type != NULL, REJILLA_IMAGE_FORMAT_NONE);

	if (type->type != REJILLA_TRACK_TYPE_IMAGE)
		return REJILLA_IMAGE_FORMAT_NONE;

	return type->subtype.img_format;
}

/**
 * rejilla_track_type_get_data_fs:
 * @type: a #RejillaTrackType.
 *
 * Returns the parameters for the image generation
 * when rejilla_track_type_get_has_data () returned
 * TRUE.
 *
 * Return value: a #RejillaImageFS
 **/

RejillaImageFS
rejilla_track_type_get_data_fs (const RejillaTrackType *type) 
{
	g_return_val_if_fail (type != NULL, REJILLA_IMAGE_FS_NONE);

	if (type->type != REJILLA_TRACK_TYPE_DATA)
		return REJILLA_IMAGE_FS_NONE;

	return type->subtype.fs_type;
}

/**
 * rejilla_track_type_get_stream_format:
 * @type: a #RejillaTrackType.
 *
 * Returns the format for a stream (song or video)
 * when rejilla_track_type_get_has_stream () returned
 * TRUE.
 *
 * Return value: a #RejillaStreamFormat
 **/

RejillaStreamFormat
rejilla_track_type_get_stream_format (const RejillaTrackType *type) 
{
	g_return_val_if_fail (type != NULL, REJILLA_AUDIO_FORMAT_NONE);

	if (type->type != REJILLA_TRACK_TYPE_STREAM)
		return REJILLA_AUDIO_FORMAT_NONE;

	return type->subtype.stream_format;
}

/**
 * rejilla_track_type_get_medium_type:
 * @type: a #RejillaTrackType.
 *
 * Returns the medium type
 * when rejilla_track_type_get_has_medium () returned
 * TRUE.
 *
 * Return value: a #RejillaMedia
 **/

RejillaMedia
rejilla_track_type_get_medium_type (const RejillaTrackType *type) 
{
	g_return_val_if_fail (type != NULL, REJILLA_MEDIUM_NONE);

	if (type->type != REJILLA_TRACK_TYPE_DISC)
		return REJILLA_MEDIUM_NONE;

	return type->subtype.media;
}

/**
 * rejilla_track_type_set_image_format:
 * @type: a #RejillaTrackType.
 * @format: a #RejillaImageFormat
 *
 * Sets the #RejillaImageFormat. Must be called
 * after rejilla_track_type_set_has_image ().
 *
 **/

void
rejilla_track_type_set_image_format (RejillaTrackType *type,
				     RejillaImageFormat format) 
{
	g_return_if_fail (type != NULL);

	if (type->type != REJILLA_TRACK_TYPE_IMAGE)
		return;

	type->subtype.img_format = format;
}

/**
 * rejilla_track_type_set_data_fs:
 * @type: a #RejillaTrackType.
 * @fs_type: a #RejillaImageFS
 *
 * Sets the #RejillaImageFS. Must be called
 * after rejilla_track_type_set_has_data ().
 *
 **/

void
rejilla_track_type_set_data_fs (RejillaTrackType *type,
				RejillaImageFS fs_type) 
{
	g_return_if_fail (type != NULL);

	if (type->type != REJILLA_TRACK_TYPE_DATA)
		return;

	type->subtype.fs_type = fs_type;
}

/**
 * rejilla_track_type_set_stream_format:
 * @type: a #RejillaTrackType.
 * @format: a #RejillaImageFormat
 *
 * Sets the #RejillaStreamFormat. Must be called
 * after rejilla_track_type_set_has_stream ().
 *
 **/

void
rejilla_track_type_set_stream_format (RejillaTrackType *type,
				      RejillaStreamFormat format) 
{
	g_return_if_fail (type != NULL);

	if (type->type != REJILLA_TRACK_TYPE_STREAM)
		return;

	type->subtype.stream_format = format;
}

/**
 * rejilla_track_type_set_medium_type:
 * @type: a #RejillaTrackType.
 * @media: a #RejillaMedia
 *
 * Sets the #RejillaMedia. Must be called
 * after rejilla_track_type_set_has_medium ().
 *
 **/

void
rejilla_track_type_set_medium_type (RejillaTrackType *type,
				    RejillaMedia media) 
{
	g_return_if_fail (type != NULL);

	if (type->type != REJILLA_TRACK_TYPE_DISC)
		return;

	type->subtype.media = media;
}

/**
 * rejilla_track_type_is_empty:
 * @type: a #RejillaTrackType.
 *
 * Returns TRUE if no type was set.
 *
 * Return value: a #gboolean
 **/

gboolean
rejilla_track_type_is_empty (const RejillaTrackType *type)
{
	g_return_val_if_fail (type != NULL, FALSE);

	return (type->type == REJILLA_TRACK_TYPE_NONE);
}

/**
 * rejilla_track_type_get_has_data:
 * @type: a #RejillaTrackType.
 *
 * Returns TRUE if DATA type (see rejilla_track_data_new ()) was set.
 *
 * Return value: a #gboolean
 **/

gboolean
rejilla_track_type_get_has_data (const RejillaTrackType *type)
{
	g_return_val_if_fail (type != NULL, FALSE);

	return type->type == REJILLA_TRACK_TYPE_DATA;
}

/**
 * rejilla_track_type_get_has_image:
 * @type: a #RejillaTrackType.
 *
 * Returns TRUE if IMAGE type (see rejilla_track_image_new ()) was set.
 *
 * Return value: a #gboolean
 **/

gboolean
rejilla_track_type_get_has_image (const RejillaTrackType *type)
{
	g_return_val_if_fail (type != NULL, FALSE);

	return type->type == REJILLA_TRACK_TYPE_IMAGE;
}

/**
 * rejilla_track_type_get_has_stream:
 * @type: a #RejillaTrackType.
 *
 * This function returns %TRUE if IMAGE type (see rejilla_track_stream_new ()) was set.
 *
 * Return value: a #gboolean
 **/

gboolean
rejilla_track_type_get_has_stream (const RejillaTrackType *type)
{
	g_return_val_if_fail (type != NULL, FALSE);

	return type->type == REJILLA_TRACK_TYPE_STREAM;
}

/**
 * rejilla_track_type_get_has_medium:
 * @type: a #RejillaTrackType.
 *
 * Returns TRUE if MEDIUM type (see rejilla_track_disc_new ()) was set.
 *
 * Return value: a #gboolean
 **/

gboolean
rejilla_track_type_get_has_medium (const RejillaTrackType *type)
{
	g_return_val_if_fail (type != NULL, FALSE);

	return type->type == REJILLA_TRACK_TYPE_DISC;
}

/**
 * rejilla_track_type_set_has_data:
 * @type: a #RejillaTrackType.
 *
 * Set DATA type for @type.
 *
 **/

void
rejilla_track_type_set_has_data (RejillaTrackType *type)
{
	g_return_if_fail (type != NULL);

	type->type = REJILLA_TRACK_TYPE_DATA;
}

/**
 * rejilla_track_type_set_has_image:
 * @type: a #RejillaTrackType.
 *
 * Set IMAGE type for @type.
 *
 **/

void
rejilla_track_type_set_has_image (RejillaTrackType *type)
{
	g_return_if_fail (type != NULL);

	type->type = REJILLA_TRACK_TYPE_IMAGE;
}

/**
 * rejilla_track_type_set_has_stream:
 * @type: a #RejillaTrackType.
 *
 * Set STREAM type for @type
 *
 **/

void
rejilla_track_type_set_has_stream (RejillaTrackType *type)
{
	g_return_if_fail (type != NULL);

	type->type = REJILLA_TRACK_TYPE_STREAM;
}

/**
 * rejilla_track_type_set_has_medium:
 * @type: a #RejillaTrackType.
 *
 * Set MEDIUM type for @type.
 *
 **/

void
rejilla_track_type_set_has_medium (RejillaTrackType *type)
{
	g_return_if_fail (type != NULL);

	type->type = REJILLA_TRACK_TYPE_DISC;
}

/**
 * rejilla_track_type_equal:
 * @type_A: a #RejillaTrackType.
 * @type_B: a #RejillaTrackType.
 *
 * Returns TRUE if @type_A and @type_B represents
 * the same type and subtype.
 *
 * Return value: a #gboolean
 **/

gboolean
rejilla_track_type_equal (const RejillaTrackType *type_A,
			  const RejillaTrackType *type_B)
{
	g_return_val_if_fail (type_A != NULL, FALSE);
	g_return_val_if_fail (type_B != NULL, FALSE);

	if (type_A->type != type_B->type)
		return FALSE;

	switch (type_A->type) {
	case REJILLA_TRACK_TYPE_DATA:
		if (type_A->subtype.fs_type != type_B->subtype.fs_type)
			return FALSE;
		break;
	
	case REJILLA_TRACK_TYPE_DISC:
		if (type_B->subtype.media != type_A->subtype.media)
			return FALSE;
		break;
	
	case REJILLA_TRACK_TYPE_IMAGE:
		if (type_A->subtype.img_format != type_B->subtype.img_format)
			return FALSE;
		break;

	case REJILLA_TRACK_TYPE_STREAM:
		if (type_A->subtype.stream_format != type_B->subtype.stream_format)
			return FALSE;
		break;

	default:
		break;
	}

	return TRUE;
}

#if 0
/**
 * rejilla_track_type_match:
 * @type_A: a #RejillaTrackType.
 * @type_B: a #RejillaTrackType.
 *
 * Returns TRUE if @type_A and @type_B match.
 *
 * (Used internally)
 *
 * Return value: a #gboolean
 **/

gboolean
rejilla_track_type_match (const RejillaTrackType *type_A,
			  const RejillaTrackType *type_B)
{
	g_return_val_if_fail (type_A != NULL, FALSE);
	g_return_val_if_fail (type_B != NULL, FALSE);

	if (type_A->type != type_B->type)
		return FALSE;

	switch (type_A->type) {
	case REJILLA_TRACK_TYPE_DATA:
		if (!(type_A->subtype.fs_type & type_B->subtype.fs_type))
			return FALSE;
		break;
	
	case REJILLA_TRACK_TYPE_DISC:
		if (!(type_A->subtype.media & type_B->subtype.media))
			return FALSE;
		break;
	
	case REJILLA_TRACK_TYPE_IMAGE:
		if (!(type_A->subtype.img_format & type_B->subtype.img_format))
			return FALSE;
		break;

	case REJILLA_TRACK_TYPE_STREAM:
		if (!(type_A->subtype.stream_format & type_B->subtype.stream_format))
			return FALSE;
		break;

	default:
		break;
	}

	return TRUE;
}

#endif
