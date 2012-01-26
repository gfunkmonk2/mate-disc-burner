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

#include <glib.h>

#include "rejilla-track.h"


typedef struct _RejillaTrackPrivate RejillaTrackPrivate;
struct _RejillaTrackPrivate
{
	GHashTable *tags;

	gchar *checksum;
	RejillaChecksumType checksum_type;
};

#define REJILLA_TRACK_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REJILLA_TYPE_TRACK, RejillaTrackPrivate))

enum
{
	CHANGED,

	LAST_SIGNAL
};


static guint track_signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (RejillaTrack, rejilla_track, G_TYPE_OBJECT);

/**
 * rejilla_track_get_track_type:
 * @track: a #RejillaTrack
 * @type: a #RejillaTrackType or NULL
 *
 * Sets @type to reflect the type of data contained in @track
 *
 * Return value: the #RejillaBurnResult of the track
 **/

RejillaBurnResult
rejilla_track_get_track_type (RejillaTrack *track,
			      RejillaTrackType *type)
{
	RejillaTrackClass *klass;

	g_return_val_if_fail (REJILLA_IS_TRACK (track), REJILLA_BURN_ERR);
	g_return_val_if_fail (type != NULL, REJILLA_BURN_ERR);

	klass = REJILLA_TRACK_GET_CLASS (track);
	if (!klass->get_type)
		return REJILLA_BURN_ERR;

	return klass->get_type (track, type);
}

/**
 * rejilla_track_get_size:
 * @track: a #RejillaTrack
 * @blocks: a #goffset or NULL
 * @bytes: a #goffset or NULL
 *
 * Returns the size of the data contained by @track in bytes or in sectors
 *
 * Return value: a #RejillaBurnResult.
 * REJILLA_BURN_OK if it was successful
 * REJILLA_BURN_NOT_READY if @track needs more time for processing the size
 * REJILLA_BURN_ERR if something is wrong or if it is empty
 **/

RejillaBurnResult
rejilla_track_get_size (RejillaTrack *track,
			goffset *blocks,
			goffset *bytes)
{
	RejillaBurnResult res;
	RejillaTrackClass *klass;
	goffset blocks_local = 0;
	goffset block_size_local = 0;

	g_return_val_if_fail (REJILLA_IS_TRACK (track), REJILLA_BURN_ERR);

	klass = REJILLA_TRACK_GET_CLASS (track);
	if (!klass->get_size)
		return REJILLA_BURN_OK;

	res = klass->get_size (track, &blocks_local, &block_size_local);
	if (res != REJILLA_BURN_OK)
		return res;

	if (blocks)
		*blocks = blocks_local;

	if (bytes)
		*bytes = blocks_local * block_size_local;

	return REJILLA_BURN_OK;
}

/**
 * rejilla_track_get_status:
 * @track: a #RejillaTrack
 * @status: a #RejillaTrackStatus
 *
 * Sets @status to reflect whether @track is ready to be used
 *
 * Return value: a #RejillaBurnResult.
 * REJILLA_BURN_OK if it was successful
 * REJILLA_BURN_NOT_READY if @track needs more time for processing
 * REJILLA_BURN_ERR if something is wrong or if it is empty
 **/

RejillaBurnResult
rejilla_track_get_status (RejillaTrack *track,
			  RejillaStatus *status)
{
	RejillaTrackClass *klass;

	g_return_val_if_fail (REJILLA_IS_TRACK (track), REJILLA_BURN_ERR);

	klass = REJILLA_TRACK_GET_CLASS (track);

	/* If this is not implement we consider that it means it is not needed
	 * and that the track doesn't perform on the side (threaded?) checks or
	 * information retrieval and that it's therefore always OK:
	 * - for example RejillaTrackDisc. */
	if (!klass->get_status) {
		if (status)
			rejilla_status_set_completed (status);

		return REJILLA_BURN_OK;
	}

	return klass->get_status (track, status);
}

/**
 * rejilla_track_set_checksum:
 * @track: a #RejillaTrack
 * @type: a #RejillaChecksumType
 * @checksum: a #gchar * holding the checksum
 *
 * Sets a checksum for the track
 *
 * Return value: a #RejillaBurnResult.
 * REJILLA_BURN_OK if the checksum was previously empty or matches the new one
 * REJILLA_BURN_ERR otherwise
 **/

RejillaBurnResult
rejilla_track_set_checksum (RejillaTrack *track,
			    RejillaChecksumType type,
			    const gchar *checksum)
{
	RejillaBurnResult result = REJILLA_BURN_OK;
	RejillaTrackPrivate *priv;

	g_return_val_if_fail (REJILLA_IS_TRACK (track), REJILLA_CHECKSUM_NONE);
	priv = REJILLA_TRACK_PRIVATE (track);

	if (type == priv->checksum_type
	&& (type == REJILLA_CHECKSUM_MD5 || type == REJILLA_CHECKSUM_SHA1 || type == REJILLA_CHECKSUM_SHA256)
	&&  checksum && strcmp (checksum, priv->checksum))
		result = REJILLA_BURN_ERR;

	if (priv->checksum)
		g_free (priv->checksum);

	priv->checksum_type = type;
	if (checksum)
		priv->checksum = g_strdup (checksum);
	else
		priv->checksum = NULL;

	return result;
}

/**
 * rejilla_track_get_checksum:
 * @track: a #RejillaTrack
 *
 * Get the current checksum (as a string) for the track
 *
 * Return value: a #gchar * (not to be freed) or NULL
 **/

const gchar *
rejilla_track_get_checksum (RejillaTrack *track)
{
	RejillaTrackPrivate *priv;

	g_return_val_if_fail (REJILLA_IS_TRACK (track), NULL);
	priv = REJILLA_TRACK_PRIVATE (track);

	return priv->checksum ? priv->checksum : NULL;
}

/**
 * rejilla_track_get_checksum_type:
 * @track: a #RejillaTrack
 *
 * Get the current checksum type for the track if any.
 *
 * Return value: a #RejillaChecksumType
 **/

RejillaChecksumType
rejilla_track_get_checksum_type (RejillaTrack *track)
{
	RejillaTrackPrivate *priv;

	g_return_val_if_fail (REJILLA_IS_TRACK (track), REJILLA_CHECKSUM_NONE);
	priv = REJILLA_TRACK_PRIVATE (track);

	return priv->checksum_type;
}

/**
 * Can be used to set arbitrary data
 */

static void
rejilla_track_tag_value_free (gpointer user_data)
{
	GValue *value = user_data;

	g_value_reset (value);
	g_free (value);
}

/**
 * rejilla_track_tag_add:
 * @track: a #RejillaTrack
 * @tag: a #gchar *
 * @value: a #GValue
 *
 * Associates a new @tag with a track. This can be used
 * to pass arbitrary information for plugins, like parameters
 * for video discs, ...
 * See rejilla-tags.h for a list of knowns tags.
 *
 * Return value: a #RejillaBurnResult.
 * REJILLA_BURN_OK if it was successful,
 * REJILLA_BURN_ERR otherwise.
 **/

RejillaBurnResult
rejilla_track_tag_add (RejillaTrack *track,
		       const gchar *tag,
		       GValue *value)
{
	RejillaTrackPrivate *priv;

	g_return_val_if_fail (REJILLA_IS_TRACK (track), REJILLA_BURN_ERR);

	priv = REJILLA_TRACK_PRIVATE (track);

	if (!priv->tags)
		priv->tags = g_hash_table_new_full (g_str_hash,
						    g_str_equal,
						    g_free,
						    rejilla_track_tag_value_free);
	g_hash_table_insert (priv->tags,
			     g_strdup (tag),
			     value);

	return REJILLA_BURN_OK;
}

/**
 * rejilla_track_tag_add_int:
 * @track: a #RejillaTrack
 * @tag: a #gchar *
 * @value: a #int
 *
 * A wrapper around rejilla_track_tag_add () to associate
 * a int value with @track
 * See also rejilla_track_tag_add ()
 *
 * Return value: a #RejillaBurnResult.
 * REJILLA_BURN_OK if it was successful,
 * REJILLA_BURN_ERR otherwise.
 **/

RejillaBurnResult
rejilla_track_tag_add_int (RejillaTrack *track,
			   const gchar *tag,
			   int value_int)
{
	GValue *value;

	value = g_new0 (GValue, 1);
	g_value_init (value, G_TYPE_INT);
	g_value_set_int (value, value_int);

	return rejilla_track_tag_add (track, tag, value);
}

/**
 * rejilla_track_tag_add_string:
 * @track: a #RejillaTrack
 * @tag: a #gchar *
 * @string: a #gchar *
 *
 * A wrapper around rejilla_track_tag_add () to associate
 * a string with @track
 * See also rejilla_track_tag_add ()
 *
 * Return value: a #RejillaBurnResult.
 * REJILLA_BURN_OK if it was successful,
 * REJILLA_BURN_ERR otherwise.
 **/

RejillaBurnResult
rejilla_track_tag_add_string (RejillaTrack *track,
			      const gchar *tag,
			      const gchar *string)
{
	GValue *value;

	value = g_new0 (GValue, 1);
	g_value_init (value, G_TYPE_STRING);
	g_value_set_string (value, string);

	return rejilla_track_tag_add (track, tag, value);
}

/**
 * rejilla_track_tag_lookup:
 * @track: a #RejillaTrack
 * @tag: a #gchar *
 * @value: a #GValue **
 *
 * Retrieves a value associated with @track through
 * rejilla_track_tag_add () and stores it in @value. Do
 * not destroy @value afterwards as it is not a copy
 *
 * Return value: a #RejillaBurnResult.
 * REJILLA_BURN_OK if the retrieval was successful
 * REJILLA_BURN_ERR otherwise
 **/

RejillaBurnResult
rejilla_track_tag_lookup (RejillaTrack *track,
			  const gchar *tag,
			  GValue **value)
{
	gpointer data;
	RejillaTrackPrivate *priv;

	g_return_val_if_fail (REJILLA_IS_TRACK (track), REJILLA_BURN_ERR);

	priv = REJILLA_TRACK_PRIVATE (track);

	if (!priv->tags)
		return REJILLA_BURN_ERR;

	data = g_hash_table_lookup (priv->tags, tag);
	if (!data)
		return REJILLA_BURN_ERR;

	if (value)
		*value = data;

	return REJILLA_BURN_OK;
}

/**
 * rejilla_track_tag_lookup_int:
 * @track: a #RejillaTrack
 * @tag: a #gchar *
 *
 * Retrieves a int value associated with @track. This
 * is a wrapper around rejilla_track_tag_lookup ().
 *
 * Return value: a #int; the value or 0 otherwise
 **/

int
rejilla_track_tag_lookup_int (RejillaTrack *track,
			      const gchar *tag)
{
	GValue *value = NULL;
	RejillaBurnResult res;

	res = rejilla_track_tag_lookup (track, tag, &value);
	if (res != REJILLA_BURN_OK)
		return 0;

	if (!value)
		return 0;

	if (!G_VALUE_HOLDS_INT (value))
		return 0;

	return g_value_get_int (value);
}

/**
 * rejilla_track_tag_lookup_string:
 * @track: a #RejillaTrack
 * @tag: a #gchar *
 *
 * Retrieves a string value associated with @track. This
 * is a wrapper around rejilla_track_tag_lookup ().
 *
 * Return value: a #gchar *. The value or NULL otherwise.
 * Do not free the string as it is not a copy.
 **/

const gchar *
rejilla_track_tag_lookup_string (RejillaTrack *track,
				 const gchar *tag)
{
	GValue *value = NULL;
	RejillaBurnResult res;

	res = rejilla_track_tag_lookup (track, tag, &value);
	if (res != REJILLA_BURN_OK)
		return NULL;

	if (!value)
		return NULL;

	if (!G_VALUE_HOLDS_STRING (value))
		return NULL;

	return g_value_get_string (value);
}

/**
 * rejilla_track_tag_copy_missing:
 * @dest: a #RejillaTrack
 * @src: a #RejillaTrack
 *
 * Adds all tags of @dest to @src provided they do not
 * already exists.
 *
 **/

void
rejilla_track_tag_copy_missing (RejillaTrack *dest,
				RejillaTrack *src)
{
	RejillaTrackPrivate *priv;
	GHashTableIter iter;
	gpointer new_value;
	gpointer new_key;
	gpointer value;
	gpointer key;

	g_return_if_fail (REJILLA_IS_TRACK (dest));
	g_return_if_fail (REJILLA_IS_TRACK (src));

	priv = REJILLA_TRACK_PRIVATE (src);

	if (!priv->tags)
		return;

	g_hash_table_iter_init (&iter, priv->tags);

	priv = REJILLA_TRACK_PRIVATE (dest);
	if (!priv->tags)
		priv->tags = g_hash_table_new_full (g_str_hash,
						    g_str_equal,
						    g_free,
						    rejilla_track_tag_value_free);

	while (g_hash_table_iter_next (&iter, &key, &value)) {
		if (g_hash_table_lookup (priv->tags, key))
			continue;

		new_value = g_new0 (GValue, 1);

		g_value_init (new_value, G_VALUE_TYPE (value));
		g_value_copy (value, new_value);

		new_key = g_strdup (key);

		g_hash_table_insert (priv->tags, new_key, new_value);
	}
}

/**
 * rejilla_track_changed:
 * @track: a #RejillaTrack
 * 
 * Used internally in #RejillaTrack implementations to 
 * signal a #RejillaTrack object has changed.
 *
 **/

void
rejilla_track_changed (RejillaTrack *track)
{
	g_signal_emit (track,
		       track_signals [CHANGED],
		       0);
}


/**
 * GObject part
 */

static void
rejilla_track_init (RejillaTrack *object)
{ }

static void
rejilla_track_finalize (GObject *object)
{
	RejillaTrackPrivate *priv;

	priv = REJILLA_TRACK_PRIVATE (object);

	if (priv->tags) {
		g_hash_table_destroy (priv->tags);
		priv->tags = NULL;
	}

	if (priv->checksum) {
		g_free (priv->checksum);
		priv->checksum = NULL;
	}

	G_OBJECT_CLASS (rejilla_track_parent_class)->finalize (object);
}

static void
rejilla_track_class_init (RejillaTrackClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (RejillaTrackPrivate));

	object_class->finalize = rejilla_track_finalize;

	track_signals[CHANGED] =
		g_signal_new ("changed",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE,
		              G_STRUCT_OFFSET (RejillaTrackClass, changed),
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE, 0,
		              G_TYPE_NONE);
}
