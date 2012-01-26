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

#include <string.h>

#include "rejilla-track-image.h"
#include "rejilla-enums.h"
#include "rejilla-track.h"

#include "burn-debug.h"
#include "burn-image-format.h"

typedef struct _RejillaTrackImagePrivate RejillaTrackImagePrivate;
struct _RejillaTrackImagePrivate
{
	gchar *image;
	gchar *toc;

	guint64 blocks;

	RejillaImageFormat format;
};

#define REJILLA_TRACK_IMAGE_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REJILLA_TYPE_TRACK_IMAGE, RejillaTrackImagePrivate))


G_DEFINE_TYPE (RejillaTrackImage, rejilla_track_image, REJILLA_TYPE_TRACK);

static RejillaBurnResult
rejilla_track_image_set_source_real (RejillaTrackImage *track,
				     const gchar *image,
				     const gchar *toc,
				     RejillaImageFormat format)
{
	RejillaTrackImagePrivate *priv;

	priv = REJILLA_TRACK_IMAGE_PRIVATE (track);

	priv->format = format;

	if (priv->image)
		g_free (priv->image);

	if (priv->toc)
		g_free (priv->toc);

	priv->image = g_strdup (image);
	priv->toc = g_strdup (toc);

	return REJILLA_BURN_OK;
}

/**
 * rejilla_track_image_set_source:
 * @track: a #RejillaTrackImage
 * @image: a #gchar or NULL
 * @toc: a #gchar or NULL
 * @format: a #RejillaImageFormat
 *
 * Sets the image source path (and its toc if need be)
 * as well as its format.
 *
 * Return value: a #RejillaBurnResult. REJILLA_BURN_OK if it is successful.
 **/

RejillaBurnResult
rejilla_track_image_set_source (RejillaTrackImage *track,
				const gchar *image,
				const gchar *toc,
				RejillaImageFormat format)
{
	RejillaTrackImagePrivate *priv;
	RejillaTrackImageClass *klass;
	RejillaBurnResult res;

	g_return_val_if_fail (REJILLA_IS_TRACK_IMAGE (track), REJILLA_BURN_ERR);

	/* See if it has changed */
	priv = REJILLA_TRACK_IMAGE_PRIVATE (track);

	klass = REJILLA_TRACK_IMAGE_GET_CLASS (track);
	if (!klass->set_source)
		return REJILLA_BURN_ERR;

	res = klass->set_source (track, image, toc, format);
	if (res != REJILLA_BURN_OK)
		return res;

	rejilla_track_changed (REJILLA_TRACK (track));
	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_track_image_set_block_num_real (RejillaTrackImage *track,
					goffset blocks)
{
	RejillaTrackImagePrivate *priv;

	priv = REJILLA_TRACK_IMAGE_PRIVATE (track);
	priv->blocks = blocks;
	return REJILLA_BURN_OK;
}

/**
 * rejilla_track_image_set_block_num:
 * @track: a #RejillaTrackImage
 * @blocks: a #goffset
 *
 * Sets the image size (in sectors).
 *
 * Return value: a #RejillaBurnResult. REJILLA_BURN_OK if it is successful.
 **/

RejillaBurnResult
rejilla_track_image_set_block_num (RejillaTrackImage *track,
				   goffset blocks)
{
	RejillaTrackImagePrivate *priv;
	RejillaTrackImageClass *klass;
	RejillaBurnResult res;

	g_return_val_if_fail (REJILLA_IS_TRACK_IMAGE (track), REJILLA_BURN_ERR);

	priv = REJILLA_TRACK_IMAGE_PRIVATE (track);
	if (priv->blocks == blocks)
		return REJILLA_BURN_OK;

	klass = REJILLA_TRACK_IMAGE_GET_CLASS (track);
	if (!klass->set_block_num)
		return REJILLA_BURN_ERR;

	res = klass->set_block_num (track, blocks);
	if (res != REJILLA_BURN_OK)
		return res;

	rejilla_track_changed (REJILLA_TRACK (track));
	return REJILLA_BURN_OK;
}

/**
 * rejilla_track_image_get_source:
 * @track: a #RejillaTrackImage
 * @uri: a #gboolean
 *
 * This function returns the path or the URI (if @uri is TRUE) of the
 * source image file.
 *
 * Return value: a #gchar
 **/

gchar *
rejilla_track_image_get_source (RejillaTrackImage *track,
				gboolean uri)
{
	RejillaTrackImagePrivate *priv;

	g_return_val_if_fail (REJILLA_IS_TRACK_IMAGE (track), NULL);

	priv = REJILLA_TRACK_IMAGE_PRIVATE (track);

	if (!priv->image) {
		gchar *complement;
		gchar *retval;
		gchar *toc;

		if (!priv->toc) {
			REJILLA_BURN_LOG ("Image nor toc were set");
			return NULL;
		}

		toc = rejilla_string_get_localpath (priv->toc);
		complement = rejilla_image_format_get_complement (priv->format, toc);
		g_free (toc);

		if (!complement) {
			REJILLA_BURN_LOG ("No complement could be retrieved");
			return NULL;
		}

		REJILLA_BURN_LOG ("Complement file retrieved %s", complement);
		if (uri)
			retval = rejilla_string_get_uri (complement);
		else
			retval = rejilla_string_get_localpath (complement);

		g_free (complement);
		return retval;
	}

	if (uri)
		return rejilla_string_get_uri (priv->image);
	else
		return rejilla_string_get_localpath (priv->image);
}

/**
 * rejilla_track_image_get_toc_source:
 * @track: a #RejillaTrackImage
 * @uri: a #gboolean
 *
 * This function returns the path or the URI (if @uri is TRUE) of the
 * source toc file.
 *
 * Return value: a #gchar
 **/

gchar *
rejilla_track_image_get_toc_source (RejillaTrackImage *track,
				    gboolean uri)
{
	RejillaTrackImagePrivate *priv;

	g_return_val_if_fail (REJILLA_IS_TRACK_IMAGE (track), NULL);

	priv = REJILLA_TRACK_IMAGE_PRIVATE (track);

	/* Don't use file complement retrieval here as it's not possible */
	if (uri)
		return rejilla_string_get_uri (priv->toc);
	else
		return rejilla_string_get_localpath (priv->toc);
}

/**
 * rejilla_track_image_get_format:
 * @track: a #RejillaTrackImage
 *
 * This function returns the format of the
 * source image.
 *
 * Return value: a #RejillaImageFormat
 **/

RejillaImageFormat
rejilla_track_image_get_format (RejillaTrackImage *track)
{
	RejillaTrackImagePrivate *priv;

	g_return_val_if_fail (REJILLA_IS_TRACK_IMAGE (track), REJILLA_IMAGE_FORMAT_NONE);

	priv = REJILLA_TRACK_IMAGE_PRIVATE (track);
	return priv->format;
}

/**
 * rejilla_track_image_need_byte_swap:
 * @track: a #RejillaTrackImage
 *
 * This function returns whether the data bytes need swapping. Some .bin files
 * associated with .cue files are little endian for audio whereas they should
 * be big endian.
 *
 * Return value: a #gboolean
 **/

gboolean
rejilla_track_image_need_byte_swap (RejillaTrackImage *track)
{
	RejillaTrackImagePrivate *priv;
	gchar *cueuri;
	gboolean res;

	g_return_val_if_fail (REJILLA_IS_TRACK_IMAGE (track), REJILLA_IMAGE_FORMAT_NONE);

	priv = REJILLA_TRACK_IMAGE_PRIVATE (track);
	if (priv->format != REJILLA_IMAGE_FORMAT_CUE)
		return FALSE;

	cueuri = rejilla_string_get_uri (priv->toc);
	res = rejilla_image_format_cue_bin_byte_swap (cueuri, NULL, NULL);
	g_free (cueuri);

	return res;
}

static RejillaBurnResult
rejilla_track_image_get_track_type (RejillaTrack *track,
				    RejillaTrackType *type)
{
	RejillaTrackImagePrivate *priv;

	priv = REJILLA_TRACK_IMAGE_PRIVATE (track);

	rejilla_track_type_set_has_image (type);
	rejilla_track_type_set_image_format (type, priv->format);

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_track_image_get_size (RejillaTrack *track,
			      goffset *blocks,
			      goffset *block_size)
{
	RejillaTrackImagePrivate *priv;

	priv = REJILLA_TRACK_IMAGE_PRIVATE (track);

	if (priv->format == REJILLA_IMAGE_FORMAT_BIN) {
		if (block_size)
			*block_size = 2048;
	}
	else if (priv->format == REJILLA_IMAGE_FORMAT_CLONE) {
		if (block_size)
			*block_size = 2448;
	}
	else if (priv->format == REJILLA_IMAGE_FORMAT_CDRDAO) {
		if (block_size)
			*block_size = 2352;
	}
	else if (priv->format == REJILLA_IMAGE_FORMAT_CUE) {
		if (block_size)
			*block_size = 2352;
	}
	else if (block_size)
		*block_size = 0;

	if (blocks)
		*blocks = priv->blocks;

	return REJILLA_BURN_OK;
}

static void
rejilla_track_image_init (RejillaTrackImage *object)
{ }

static void
rejilla_track_image_finalize (GObject *object)
{
	RejillaTrackImagePrivate *priv;

	priv = REJILLA_TRACK_IMAGE_PRIVATE (object);
	if (priv->image) {
		g_free (priv->image);
		priv->image = NULL;
	}

	if (priv->toc) {
		g_free (priv->toc);
		priv->toc = NULL;
	}

	G_OBJECT_CLASS (rejilla_track_image_parent_class)->finalize (object);
}

static void
rejilla_track_image_class_init (RejillaTrackImageClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	RejillaTrackClass *track_class = REJILLA_TRACK_CLASS (klass);

	g_type_class_add_private (klass, sizeof (RejillaTrackImagePrivate));

	object_class->finalize = rejilla_track_image_finalize;

	track_class->get_size = rejilla_track_image_get_size;
	track_class->get_type = rejilla_track_image_get_track_type;

	klass->set_source = rejilla_track_image_set_source_real;
	klass->set_block_num = rejilla_track_image_set_block_num_real;
}

/**
 * rejilla_track_image_new:
 *
 * Creates a new #RejillaTrackImage object.
 *
 * This type of tracks is used to burn disc images.
 *
 * Return value: a #RejillaTrackImage object.
 **/

RejillaTrackImage *
rejilla_track_image_new (void)
{
	return g_object_new (REJILLA_TYPE_TRACK_IMAGE, NULL);
}
