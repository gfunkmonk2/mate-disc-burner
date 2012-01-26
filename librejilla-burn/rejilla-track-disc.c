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
#include <glib-object.h>

#include "rejilla-track-disc.h"

typedef struct _RejillaTrackDiscPrivate RejillaTrackDiscPrivate;
struct _RejillaTrackDiscPrivate
{
	RejillaDrive *drive;

	guint track_num;

	glong src_removed_sig;
	glong src_added_sig;
};

#define REJILLA_TRACK_DISC_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REJILLA_TYPE_TRACK_DISC, RejillaTrackDiscPrivate))

G_DEFINE_TYPE (RejillaTrackDisc, rejilla_track_disc, REJILLA_TYPE_TRACK);

/**
 * rejilla_track_disc_set_track_num:
 * @track: a #RejillaTrackDisc
 * @num: a #guint
 *
 * Sets a track number which can be used
 * to copy only one specific session on a multisession disc
 *
 * Return value: a #RejillaBurnResult.
 * REJILLA_BURN_OK if it was successful,
 * REJILLA_BURN_ERR otherwise.
 **/

RejillaBurnResult
rejilla_track_disc_set_track_num (RejillaTrackDisc *track,
				  guint num)
{
	RejillaTrackDiscPrivate *priv;

	g_return_val_if_fail (REJILLA_IS_TRACK_DISC (track), REJILLA_BURN_ERR);

	priv = REJILLA_TRACK_DISC_PRIVATE (track);
	priv->track_num = num;

	return REJILLA_BURN_OK;
}

/**
 * rejilla_track_disc_get_track_num:
 * @track: a #RejillaTrackDisc
 *
 * Gets the track number which will be used
 * to copy only one specific session on a multisession disc
 *
 * Return value: a #guint. 0 if none is set, any other number otherwise.
 **/

guint
rejilla_track_disc_get_track_num (RejillaTrackDisc *track)
{
	RejillaTrackDiscPrivate *priv;

	g_return_val_if_fail (REJILLA_IS_TRACK_DISC (track), REJILLA_BURN_ERR);

	priv = REJILLA_TRACK_DISC_PRIVATE (track);
	return priv->track_num;
}

static void
rejilla_track_disc_remove_drive (RejillaTrackDisc *track)
{
	RejillaTrackDiscPrivate *priv;

	priv = REJILLA_TRACK_DISC_PRIVATE (track);

	if (priv->src_added_sig) {
		g_signal_handler_disconnect (priv->drive, priv->src_added_sig);
		priv->src_added_sig = 0;
	}

	if (priv->src_removed_sig) {
		g_signal_handler_disconnect (priv->drive, priv->src_removed_sig);
		priv->src_removed_sig = 0;
	}

	if (priv->drive) {
		g_object_unref (priv->drive);
		priv->drive = NULL;
	}
}

static void
rejilla_track_disc_medium_changed (RejillaDrive *drive,
				   RejillaMedium *medium,
				   RejillaTrack *track)
{
	rejilla_track_changed (track);
}

/**
 * rejilla_track_disc_set_drive:
 * @track: a #RejillaTrackDisc
 * @drive: a #RejillaDrive
 *
 * Sets @drive to be the #RejillaDrive that will be used
 * as the source when copying
 *
 * Return value: a #RejillaBurnResult.
 * REJILLA_BURN_OK if it was successful,
 * REJILLA_BURN_ERR otherwise.
 **/

RejillaBurnResult
rejilla_track_disc_set_drive (RejillaTrackDisc *track,
			      RejillaDrive *drive)
{
	RejillaTrackDiscPrivate *priv;

	g_return_val_if_fail (REJILLA_IS_TRACK_DISC (track), REJILLA_BURN_ERR);

	priv = REJILLA_TRACK_DISC_PRIVATE (track);

	rejilla_track_disc_remove_drive (track);
	if (!drive) {
		rejilla_track_changed (REJILLA_TRACK (track));
		return REJILLA_BURN_OK;
	}

	priv->drive = drive;
	g_object_ref (drive);

	priv->src_added_sig = g_signal_connect (drive,
						"medium-added",
						G_CALLBACK (rejilla_track_disc_medium_changed),
						track);
	priv->src_removed_sig = g_signal_connect (drive,
						  "medium-removed",
						  G_CALLBACK (rejilla_track_disc_medium_changed),
						  track);

	rejilla_track_changed (REJILLA_TRACK (track));

	return REJILLA_BURN_OK;
}

/**
 * rejilla_track_disc_get_drive:
 * @track: a #RejillaTrackDisc
 *
 * Gets the #RejillaDrive object that will be used as
 * the source when copying.
 *
 * Return value: a #RejillaDrive or NULL. Don't unref or free it.
 **/

RejillaDrive *
rejilla_track_disc_get_drive (RejillaTrackDisc *track)
{
	RejillaTrackDiscPrivate *priv;

	g_return_val_if_fail (REJILLA_IS_TRACK_DISC (track), NULL);

	priv = REJILLA_TRACK_DISC_PRIVATE (track);
	return priv->drive;
}

/**
 * rejilla_track_disc_get_medium_type:
 * @track: a #RejillaTrackDisc
 *
 * Gets the #RejillaMedia for the medium that is
 * currently inserted into the drive assigned for @track
 * with rejilla_track_disc_set_drive ().
 *
 * Return value: a #RejillaMedia.
 **/

RejillaMedia
rejilla_track_disc_get_medium_type (RejillaTrackDisc *track)
{
	RejillaTrackDiscPrivate *priv;
	RejillaMedium *medium;

	g_return_val_if_fail (REJILLA_IS_TRACK_DISC (track), REJILLA_MEDIUM_NONE);

	priv = REJILLA_TRACK_DISC_PRIVATE (track);
	medium = rejilla_drive_get_medium (priv->drive);
	if (!medium)
		return REJILLA_MEDIUM_NONE;

	return rejilla_medium_get_status (medium);
}

static RejillaBurnResult
rejilla_track_disc_get_size (RejillaTrack *track,
			     goffset *blocks,
			     goffset *block_size)
{
	RejillaMedium *medium;
	goffset medium_size = 0;
	goffset medium_blocks = 0;
	RejillaTrackDiscPrivate *priv;

	priv = REJILLA_TRACK_DISC_PRIVATE (track);
	medium = rejilla_drive_get_medium (priv->drive);
	if (!medium)
		return REJILLA_BURN_NOT_READY;

	rejilla_medium_get_data_size (medium, &medium_size, &medium_blocks);

	if (blocks)
		*blocks = medium_blocks;

	if (block_size)
		*block_size = medium_blocks? (medium_size / medium_blocks):0;

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_track_disc_get_track_type (RejillaTrack *track,
				   RejillaTrackType *type)
{
	RejillaTrackDiscPrivate *priv;
	RejillaMedium *medium;

	priv = REJILLA_TRACK_DISC_PRIVATE (track);

	medium = rejilla_drive_get_medium (priv->drive);

	rejilla_track_type_set_has_medium (type);
	rejilla_track_type_set_medium_type (type, rejilla_medium_get_status (medium));

	return REJILLA_BURN_OK;
}

static void
rejilla_track_disc_init (RejillaTrackDisc *object)
{ }

static void
rejilla_track_disc_finalize (GObject *object)
{
	rejilla_track_disc_remove_drive (REJILLA_TRACK_DISC (object));

	G_OBJECT_CLASS (rejilla_track_disc_parent_class)->finalize (object);
}

static void
rejilla_track_disc_class_init (RejillaTrackDiscClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	RejillaTrackClass* track_class = REJILLA_TRACK_CLASS (klass);

	g_type_class_add_private (klass, sizeof (RejillaTrackDiscPrivate));

	object_class->finalize = rejilla_track_disc_finalize;

	track_class->get_size = rejilla_track_disc_get_size;
	track_class->get_type = rejilla_track_disc_get_track_type;
}

/**
 * rejilla_track_disc_new:
 *
 * Creates a new #RejillaTrackDisc object.
 *
 * This type of tracks is used to copy media either
 * to a disc image file or to another medium.
 *
 * Return value: a #RejillaTrackDisc.
 **/

RejillaTrackDisc *
rejilla_track_disc_new (void)
{
	return g_object_new (REJILLA_TYPE_TRACK_DISC, NULL);
}
