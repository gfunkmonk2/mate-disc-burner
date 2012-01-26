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

#include "rejilla-drive.h"
#include "rejilla-medium.h"

#include "burn-debug.h"
#include "rejilla-session-helper.h"
#include "rejilla-track.h"
#include "rejilla-track-data.h"
#include "rejilla-track-data-cfg.h"
#include "rejilla-session-span.h"

typedef struct _RejillaSessionSpanPrivate RejillaSessionSpanPrivate;
struct _RejillaSessionSpanPrivate
{
	GSList * track_list;
	RejillaTrack * last_track;
};

#define REJILLA_SESSION_SPAN_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REJILLA_TYPE_SESSION_SPAN, RejillaSessionSpanPrivate))

G_DEFINE_TYPE (RejillaSessionSpan, rejilla_session_span, REJILLA_TYPE_BURN_SESSION);

/**
 * rejilla_session_span_get_max_space:
 * @session: a #RejillaSessionSpan
 *
 * Returns the maximum required space (in sectors) 
 * among all the possible spanned batches.
 * This means that when burningto a media
 * it will also be the minimum required
 * space to burn all the contents in several
 * batches.
 *
 * Return value: a #goffset.
 **/

goffset
rejilla_session_span_get_max_space (RejillaSessionSpan *session)
{
	GSList *tracks;
	goffset max_sectors = 0;
	RejillaSessionSpanPrivate *priv;

	g_return_val_if_fail (REJILLA_IS_SESSION_SPAN (session), 0);

	priv = REJILLA_SESSION_SPAN_PRIVATE (session);

	if (priv->last_track) {
		tracks = g_slist_find (priv->track_list, priv->last_track);

		if (!tracks->next)
			return 0;

		tracks = tracks->next;
	}
	else if (priv->track_list)
		tracks = priv->track_list;
	else
		tracks = rejilla_burn_session_get_tracks (REJILLA_BURN_SESSION (session));

	for (; tracks; tracks = tracks->next) {
		RejillaTrack *track;
		goffset track_blocks = 0;

		track = tracks->data;

		if (REJILLA_IS_TRACK_DATA_CFG (track))
			return rejilla_track_data_cfg_span_max_space (REJILLA_TRACK_DATA_CFG (track));

		/* This is the common case */
		rejilla_track_get_size (REJILLA_TRACK (track),
					&track_blocks,
					NULL);

		max_sectors = MAX (max_sectors, track_blocks);
	}

	return max_sectors;
}

/**
 * rejilla_session_span_again:
 * @session: a #RejillaSessionSpan
 *
 * Checks whether some data were not included during calls to rejilla_session_span_next ().
 *
 * Return value: a #RejillaBurnResult. REJILLA_BURN_OK if there is not anymore data.
 * REJILLA_BURN_RETRY if the operation was successful and a new #RejillaTrackDataCfg was created.
 * REJILLA_BURN_ERR otherwise.
 **/

RejillaBurnResult
rejilla_session_span_again (RejillaSessionSpan *session)
{
	GSList *tracks;
	RejillaTrack *track;
	RejillaSessionSpanPrivate *priv;

	g_return_val_if_fail (REJILLA_IS_SESSION_SPAN (session), REJILLA_BURN_ERR);

	priv = REJILLA_SESSION_SPAN_PRIVATE (session);

	/* This is not an error */
	if (!priv->track_list)
		return REJILLA_BURN_OK;

	if (priv->last_track) {
		tracks = g_slist_find (priv->track_list, priv->last_track);
		if (!tracks->next) {
			priv->track_list = NULL;
			return REJILLA_BURN_OK;
		}

		return REJILLA_BURN_RETRY;
	}

	tracks = priv->track_list;
	track = tracks->data;

	if (REJILLA_IS_TRACK_DATA_CFG (track))
		return rejilla_track_data_cfg_span_again (REJILLA_TRACK_DATA_CFG (track));

	return (tracks != NULL)? REJILLA_BURN_RETRY:REJILLA_BURN_OK;
}

/**
 * rejilla_session_span_possible:
 * @session: a #RejillaSessionSpan
 *
 * Checks if a new #RejillaTrackData can be created from the files remaining in the tree 
 * after calls to rejilla_session_span_next (). The maximum size of the data will be the one
 * of the medium inserted in the #RejillaDrive set for @session (see rejilla_burn_session_set_burner ()).
 *
 * Return value: a #RejillaBurnResult. REJILLA_BURN_OK if there is not anymore data.
 * REJILLA_BURN_RETRY if the operation was successful and a new #RejillaTrackDataCfg was created.
 * REJILLA_BURN_ERR otherwise.
 **/

RejillaBurnResult
rejilla_session_span_possible (RejillaSessionSpan *session)
{
	GSList *tracks;
	RejillaTrack *track;
	goffset max_sectors = 0;
	goffset track_blocks = 0;
	RejillaSessionSpanPrivate *priv;

	g_return_val_if_fail (REJILLA_IS_SESSION_SPAN (session), REJILLA_BURN_ERR);

	priv = REJILLA_SESSION_SPAN_PRIVATE (session);

	max_sectors = rejilla_burn_session_get_available_medium_space (REJILLA_BURN_SESSION (session));
	if (max_sectors <= 0)
		return REJILLA_BURN_ERR;

	if (!priv->track_list)
		tracks = rejilla_burn_session_get_tracks (REJILLA_BURN_SESSION (session));
	else if (priv->last_track) {
		tracks = g_slist_find (priv->track_list, priv->last_track);
		if (!tracks->next) {
			priv->track_list = NULL;
			return REJILLA_BURN_OK;
		}
		tracks = tracks->next;
	}
	else
		tracks = priv->track_list;

	if (!tracks)
		return REJILLA_BURN_ERR;

	track = tracks->data;

	if (REJILLA_IS_TRACK_DATA_CFG (track))
		return rejilla_track_data_cfg_span_possible (REJILLA_TRACK_DATA_CFG (track), max_sectors);

	/* This is the common case */
	rejilla_track_get_size (REJILLA_TRACK (track),
				&track_blocks,
				NULL);

	if (track_blocks >= max_sectors)
		return REJILLA_BURN_ERR;

	return REJILLA_BURN_RETRY;
}

/**
 * rejilla_session_span_start:
 * @session: a #RejillaSessionSpan
 *
 * Get the object ready for spanning a #RejillaBurnSession object. This function
 * must be called before rejilla_session_span_next ().
 *
 * Return value: a #RejillaBurnResult. REJILLA_BURN_OK if successful.
 **/

RejillaBurnResult
rejilla_session_span_start (RejillaSessionSpan *session)
{
	RejillaSessionSpanPrivate *priv;

	g_return_val_if_fail (REJILLA_IS_SESSION_SPAN (session), REJILLA_BURN_ERR);

	priv = REJILLA_SESSION_SPAN_PRIVATE (session);

	priv->track_list = rejilla_burn_session_get_tracks (REJILLA_BURN_SESSION (session));
	if (priv->last_track) {
		g_object_unref (priv->last_track);
		priv->last_track = NULL;
	}

	return REJILLA_BURN_OK;
}

/**
 * rejilla_session_span_next:
 * @session: a #RejillaSessionSpan
 *
 * Sets the next batch of data to be burnt onto the medium inserted in the #RejillaDrive
 * set for @session (see rejilla_burn_session_set_burner ()). Its free space or it capacity
 * will be used as the maximum amount of data to be burnt.
 *
 * Return value: a #RejillaBurnResult. REJILLA_BURN_OK if successful.
 **/

RejillaBurnResult
rejilla_session_span_next (RejillaSessionSpan *session)
{
	GSList *tracks;
	gboolean pushed = FALSE;
	goffset max_sectors = 0;
	goffset total_sectors = 0;
	RejillaSessionSpanPrivate *priv;

	g_return_val_if_fail (REJILLA_IS_SESSION_SPAN (session), REJILLA_BURN_ERR);

	priv = REJILLA_SESSION_SPAN_PRIVATE (session);

	g_return_val_if_fail (priv->track_list != NULL, REJILLA_BURN_ERR);

	max_sectors = rejilla_burn_session_get_available_medium_space (REJILLA_BURN_SESSION (session));
	if (max_sectors <= 0)
		return REJILLA_BURN_ERR;

	/* NOTE: should we pop here? */
	if (priv->last_track) {
		tracks = g_slist_find (priv->track_list, priv->last_track);
		g_object_unref (priv->last_track);
		priv->last_track = NULL;

		if (!tracks->next) {
			priv->track_list = NULL;
			return REJILLA_BURN_OK;
		}
		tracks = tracks->next;
	}
	else
		tracks = priv->track_list;

	for (; tracks; tracks = tracks->next) {
		RejillaTrack *track;
		goffset track_blocks = 0;

		track = tracks->data;

		if (REJILLA_IS_TRACK_DATA_CFG (track)) {
			RejillaTrackData *new_track;
			RejillaBurnResult result;

			/* NOTE: the case where track_blocks < max_blocks will
			 * be handled by rejilla_track_data_cfg_span () */

			/* This track type is the only one to be able to span itself */
			new_track = rejilla_track_data_new ();
			result = rejilla_track_data_cfg_span (REJILLA_TRACK_DATA_CFG (track),
							      max_sectors,
							      new_track);
			if (result != REJILLA_BURN_RETRY) {
				g_object_unref (new_track);
				return result;
			}

			pushed = TRUE;
			rejilla_burn_session_push_tracks (REJILLA_BURN_SESSION (session));
			rejilla_burn_session_add_track (REJILLA_BURN_SESSION (session),
							REJILLA_TRACK (new_track),
							NULL);
			break;
		}

		/* This is the common case */
		rejilla_track_get_size (REJILLA_TRACK (track),
					&track_blocks,
					NULL);

		/* NOTE: keep the order of tracks */
		if (track_blocks + total_sectors >= max_sectors) {
			REJILLA_BURN_LOG ("Reached end of spanned size");
			break;
		}

		total_sectors += track_blocks;

		if (!pushed) {
			REJILLA_BURN_LOG ("Pushing tracks for media spanning");
			rejilla_burn_session_push_tracks (REJILLA_BURN_SESSION (session));
			pushed = TRUE;
		}

		REJILLA_BURN_LOG ("Adding tracks");
		rejilla_burn_session_add_track (REJILLA_BURN_SESSION (session), track, NULL);

		if (priv->last_track)
			g_object_unref (priv->last_track);

		priv->last_track = g_object_ref (track);
	}

	/* If we pushed anything it means we succeeded */
	return (pushed? REJILLA_BURN_RETRY:REJILLA_BURN_ERR);
}

/**
 * rejilla_session_span_stop:
 * @session: a #RejillaSessionSpan
 *
 * Ends and cleans a spanning operation started with rejilla_session_span_start ().
 *
 **/

void
rejilla_session_span_stop (RejillaSessionSpan *session)
{
	RejillaSessionSpanPrivate *priv;

	g_return_if_fail (REJILLA_IS_SESSION_SPAN (session));

	priv = REJILLA_SESSION_SPAN_PRIVATE (session);

	if (priv->last_track) {
		g_object_unref (priv->last_track);
		priv->last_track = NULL;
	}
	else if (priv->track_list) {
		RejillaTrack *track;

		track = priv->track_list->data;
		if (REJILLA_IS_TRACK_DATA_CFG (track))
			rejilla_track_data_cfg_span_stop (REJILLA_TRACK_DATA_CFG (track));
	}

	priv->track_list = NULL;
}

static void
rejilla_session_span_init (RejillaSessionSpan *object)
{ }

static void
rejilla_session_span_finalize (GObject *object)
{
	rejilla_session_span_stop (REJILLA_SESSION_SPAN (object));
	G_OBJECT_CLASS (rejilla_session_span_parent_class)->finalize (object);
}

static void
rejilla_session_span_class_init (RejillaSessionSpanClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (RejillaSessionSpanPrivate));

	object_class->finalize = rejilla_session_span_finalize;
}

/**
 * rejilla_session_span_new:
 *
 * Creates a new #RejillaSessionSpan object.
 *
 * Return value: a #RejillaSessionSpan object
 **/

RejillaSessionSpan *
rejilla_session_span_new (void)
{
	return g_object_new (REJILLA_TYPE_SESSION_SPAN, NULL);
}
