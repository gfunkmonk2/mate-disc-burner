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
#include <glib/gi18n-lib.h>
#include <glib-object.h>

#include "rejilla-track-image-cfg.h"
#include "rejilla-track-image.h"
#include "rejilla-track.h"

#include "burn-image-format.h"


typedef struct _RejillaTrackImageInfo RejillaTrackImageInfo;
struct _RejillaTrackImageInfo {
	gchar *uri;
	guint64 blocks;
	GCancellable *cancel;
	RejillaImageFormat format;
};

typedef struct _RejillaTrackImageCfgPrivate RejillaTrackImageCfgPrivate;
struct _RejillaTrackImageCfgPrivate
{
	GCancellable *cancel;
	GError *error;

	RejillaImageFormat format;
};

#define REJILLA_TRACK_IMAGE_CFG_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REJILLA_TYPE_TRACK_IMAGE_CFG, RejillaTrackImageCfgPrivate))

G_DEFINE_TYPE (RejillaTrackImageCfg, rejilla_track_image_cfg, REJILLA_TYPE_TRACK_IMAGE);


static void
rejilla_track_image_cfg_set_uri (RejillaTrackImageCfg *track,
				 const gchar *uri,
				 RejillaImageFormat format)
{
	switch (format) {
	case REJILLA_IMAGE_FORMAT_NONE:
	case REJILLA_IMAGE_FORMAT_BIN:
		REJILLA_TRACK_IMAGE_CLASS (rejilla_track_image_cfg_parent_class)->set_source (REJILLA_TRACK_IMAGE (track),
											      uri,
											      NULL,
											      format);
		break;
	case REJILLA_IMAGE_FORMAT_CLONE:
	case REJILLA_IMAGE_FORMAT_CUE:
	case REJILLA_IMAGE_FORMAT_CDRDAO:
		REJILLA_TRACK_IMAGE_CLASS (rejilla_track_image_cfg_parent_class)->set_source (REJILLA_TRACK_IMAGE (track),
											      NULL,
											      uri,
											      format);
		break;

	default:
		break;
	}
}

static void
rejilla_track_image_cfg_get_info_cb (GObject *object,
				     GAsyncResult *result,
				     gpointer user_data)
{
	RejillaTrackImageInfo *info;
	RejillaTrackImageCfgPrivate *priv;

	priv = REJILLA_TRACK_IMAGE_CFG_PRIVATE (object);

	info = g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (result));
	if (priv->cancel == info->cancel) {
		g_object_unref (priv->cancel);
		priv->cancel = NULL;
	}

	if (g_cancellable_is_cancelled (info->cancel)) {
		rejilla_track_changed (REJILLA_TRACK (object));
		return;
	}

	if (info->format == REJILLA_IMAGE_FORMAT_NONE
	||  info->blocks == 0) {
		GError *error = NULL;

		g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (result), &error);
		priv->error = error;

		rejilla_track_changed (REJILLA_TRACK (object));
		return;
	}

	rejilla_track_image_cfg_set_uri (REJILLA_TRACK_IMAGE_CFG (object),
	                                 info->uri,
	                                 priv->format != REJILLA_IMAGE_FORMAT_NONE? priv->format:info->format);

	REJILLA_TRACK_IMAGE_CLASS (rejilla_track_image_cfg_parent_class)->set_block_num (REJILLA_TRACK_IMAGE (object), info->blocks);
	rejilla_track_changed (REJILLA_TRACK (object));
}

static void
rejilla_track_image_cfg_get_info_thread (GSimpleAsyncResult *result,
					 GObject *object,
					 GCancellable *cancel)
{
	RejillaTrackImageInfo *info;
	GError *error = NULL;

	info = g_simple_async_result_get_op_res_gpointer (result);
	if (info->format == REJILLA_IMAGE_FORMAT_NONE) {
		GFile *file;
		const gchar *mime;
		GFileInfo *file_info;

		/* check if we do have a URI */
		file = g_file_new_for_commandline_arg (info->uri);
		file_info = g_file_query_info (file,
					       G_FILE_ATTRIBUTE_STANDARD_CONTENT_TYPE,
					       G_FILE_QUERY_INFO_NONE,
					       cancel,
					       &error);
		g_object_unref (file);

		if (!file_info) {
			if (error && !g_cancellable_is_cancelled (cancel))
				g_simple_async_result_set_from_error (result, error);

			if (error)
				g_error_free (error);

			return;
		}

		mime = g_file_info_get_content_type (file_info);
		if (mime
		&& (!strcmp (mime, "application/x-toc")
		||  !strcmp (mime, "application/x-cdrdao-toc")
		||  !strcmp (mime, "application/x-cue"))) {
			info->format = rejilla_image_format_identify_cuesheet (info->uri, cancel, &error);

			if (error) {
				if (!g_cancellable_is_cancelled (cancel))
					g_simple_async_result_set_from_error (result, error);

				g_error_free (error);
				g_object_unref (file_info);
				return;
			}

			if (info->format == REJILLA_IMAGE_FORMAT_NONE
			&&  g_str_has_suffix (info->uri, ".toc"))
				info->format = REJILLA_IMAGE_FORMAT_CLONE;
		}
		else if (mime && !strcmp (mime, "application/octet-stream")) {
			/* that could be an image, so here is the deal:
			 * if we can find the type through the extension, fine.
			 * if not default to BIN */
			if (g_str_has_suffix (info->uri, ".bin"))
				info->format = REJILLA_IMAGE_FORMAT_CDRDAO;
			else if (g_str_has_suffix (info->uri, ".raw"))
				info->format = REJILLA_IMAGE_FORMAT_CLONE;
			else
				info->format = REJILLA_IMAGE_FORMAT_BIN;
		}
		else if (mime && !strcmp (mime, "application/x-cd-image"))
			info->format = REJILLA_IMAGE_FORMAT_BIN;

		g_object_unref (file_info);
	}

	if (info->format == REJILLA_IMAGE_FORMAT_NONE)
		return;

	if (info->format == REJILLA_IMAGE_FORMAT_BIN)
		rejilla_image_format_get_iso_size (info->uri, &info->blocks, NULL, cancel, &error);
	else if (info->format == REJILLA_IMAGE_FORMAT_CLONE) {
		gchar *complement;

		complement = rejilla_image_format_get_complement (REJILLA_IMAGE_FORMAT_CLONE, info->uri);
		rejilla_image_format_get_clone_size (complement, &info->blocks, NULL, cancel, &error);
	}
	else if (info->format == REJILLA_IMAGE_FORMAT_CDRDAO)
		rejilla_image_format_get_cdrdao_size (info->uri, &info->blocks, NULL, cancel, &error);
	else if (info->format == REJILLA_IMAGE_FORMAT_CUE)
		rejilla_image_format_get_cue_size (info->uri, &info->blocks, NULL, cancel, &error);

	if (error && !g_cancellable_is_cancelled (cancel))
		g_simple_async_result_set_from_error (result, error);

	if (error)
		g_error_free (error);
}

static void
rejilla_track_image_info_free (gpointer data)
{
	RejillaTrackImageInfo *info = data;

	g_object_unref (info->cancel);
	g_free (info->uri);
	g_free (info);
}

static void
rejilla_track_image_cfg_get_info (RejillaTrackImageCfg *track,
				  const gchar *uri)
{
	RejillaTrackImageCfgPrivate *priv;
	RejillaTrackImageInfo *info;
	GSimpleAsyncResult *res;

	priv = REJILLA_TRACK_IMAGE_CFG_PRIVATE (track);

	/* Cancel a possible ongoing thread */
	if (priv->cancel) {
		g_cancellable_cancel (priv->cancel);
		g_object_unref (priv->cancel);
		priv->cancel = NULL;
	}

	if (priv->error) {
		g_error_free (priv->error);
		priv->error = NULL;
	}

	res = g_simple_async_result_new (G_OBJECT (track),
					 rejilla_track_image_cfg_get_info_cb,
					 NULL,
					 rejilla_track_image_cfg_get_info);

	priv->cancel = g_cancellable_new ();

	info = g_new0 (RejillaTrackImageInfo, 1);
	info->uri = g_strdup (uri);
	info->format = priv->format;
	info->cancel = g_object_ref (priv->cancel);

	g_simple_async_result_set_op_res_gpointer (res, info, rejilla_track_image_info_free);
	g_simple_async_result_run_in_thread (res,
					     rejilla_track_image_cfg_get_info_thread,
					     G_PRIORITY_LOW,
					     priv->cancel);
	g_object_unref (res);
}

/**
 * rejilla_track_image_cfg_set_source:
 * @track: a #RejillaTrackImageCfg
 * @uri: a #gchar
 *
 * Sets the image uri or path (absolute or relative). @track will then identify its format and retrieve its size.
 *
 * Return value: a #RejillaBurnResult. REJILLA_BURN_OK if it is successful.
 **/

RejillaBurnResult
rejilla_track_image_cfg_set_source (RejillaTrackImageCfg *track,
				    const gchar *uri)
{
	GFile *file;
	gchar *uri_arg;
	gchar *current_uri;
	RejillaTrackImageCfgPrivate *priv;

	g_return_val_if_fail (REJILLA_IS_TRACK_IMAGE_CFG (track), REJILLA_BURN_NOT_SUPPORTED);
	g_return_val_if_fail (uri != NULL, REJILLA_BURN_NOT_SUPPORTED);

	priv = REJILLA_TRACK_IMAGE_CFG_PRIVATE (track);

	/* See if it has changed */
	file = g_file_new_for_commandline_arg (uri);
	uri_arg = g_file_get_uri (file);
	g_object_unref (file);

	if (!uri_arg)
		return REJILLA_BURN_ERR;

	current_uri = rejilla_track_image_get_source (REJILLA_TRACK_IMAGE (track), TRUE);
	if (current_uri && !strcmp (current_uri, uri_arg)) {
		g_free (current_uri);
		g_free (uri_arg);
		return REJILLA_BURN_OK;
	}
	g_free (current_uri);

	/* Do it before to update our status first then update track info */
	rejilla_track_image_cfg_get_info (track, uri_arg);

	/* Update the image info container values. If it was invalid then */
	/* NOTE: this resets the size as well */
	REJILLA_TRACK_IMAGE_CLASS (rejilla_track_image_cfg_parent_class)->set_block_num (REJILLA_TRACK_IMAGE (track), 0);
	rejilla_track_image_cfg_set_uri (track, uri_arg, priv->format);
	rejilla_track_changed (REJILLA_TRACK (track));

	g_free (uri_arg);

	return REJILLA_BURN_OK;
}

/**
 * rejilla_track_image_cfg_get_forced_format:
 * @track: a #RejillaTrackImageCfg
 *
 * This function returns the #RejillaImageFormat that was set for the image.
 * See rejilla_track_image_cfg_force_format ().
 *
 * Return value: a #RejillaBurnResult. REJILLA_BURN_OK if it is successful.
 **/

RejillaImageFormat
rejilla_track_image_cfg_get_forced_format (RejillaTrackImageCfg *track)
{
	RejillaTrackImageCfgPrivate *priv;

	g_return_val_if_fail (REJILLA_IS_TRACK_IMAGE_CFG (track), REJILLA_IMAGE_FORMAT_NONE);

	priv = REJILLA_TRACK_IMAGE_CFG_PRIVATE (track);
	return priv->format;
}

/**
 * rejilla_track_image_cfg_force_format:
 * @track: a #RejillaTrackImageCfg
 * @format: a #RejillaImageFormat
 *
 * This function allows to prevents the identification of the format of the image.
 * It does not cancel size retrieval.
 * If @format is REJILLA_IMAGE_FORMAT_NONE then the format of the image
 * will be retrieved.
 *
 * Return value: a #RejillaBurnResult. REJILLA_BURN_OK if it is successful.
 **/

RejillaBurnResult
rejilla_track_image_cfg_force_format (RejillaTrackImageCfg *track,
				      RejillaImageFormat format)
{
	RejillaTrackImageCfgPrivate *priv;
	RejillaImageFormat current_format;
	gchar *uri = NULL;

	g_return_val_if_fail (REJILLA_IS_TRACK_IMAGE_CFG (track), REJILLA_BURN_NOT_SUPPORTED);

	priv = REJILLA_TRACK_IMAGE_CFG_PRIVATE (track);

	current_format = rejilla_track_image_get_format (REJILLA_TRACK_IMAGE (track));
	if (format != REJILLA_IMAGE_FORMAT_NONE) {
		if (current_format == format)
			return REJILLA_BURN_OK;
	}
	else if (format == priv->format)
		return REJILLA_BURN_OK;

	priv->format = format;

	switch (current_format) {
	case REJILLA_IMAGE_FORMAT_NONE:
	case REJILLA_IMAGE_FORMAT_BIN:
		uri = rejilla_track_image_get_source (REJILLA_TRACK_IMAGE (track), TRUE);
		break;
	case REJILLA_IMAGE_FORMAT_CLONE:
	case REJILLA_IMAGE_FORMAT_CUE:
	case REJILLA_IMAGE_FORMAT_CDRDAO:
		uri = rejilla_track_image_get_toc_source (REJILLA_TRACK_IMAGE (track), TRUE);
		break;

	default:
		break;
	}

	if (!uri)
		return REJILLA_BURN_NOT_READY;

	/* Do it before to update our status first then update track info */
	rejilla_track_image_cfg_get_info (track, uri);

	uri = rejilla_track_image_get_source (REJILLA_TRACK_IMAGE (track), TRUE);
	rejilla_track_image_cfg_set_uri (track, uri, priv->format);
	g_free (uri);

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_track_image_cfg_get_status (RejillaTrack *track,
				    RejillaStatus *status)
{
	RejillaTrackImageCfgPrivate *priv;

	priv = REJILLA_TRACK_IMAGE_CFG_PRIVATE (track);

	if (priv->cancel) {
		if (status)
			rejilla_status_set_not_ready (status, -1.0, _("Retrieving image format and size"));

		return REJILLA_BURN_NOT_READY;
	}

	if (priv->error) {
		if (status)
			rejilla_status_set_error (status, g_error_copy (priv->error));

		return REJILLA_BURN_ERR;
	}

	/* See if we managed to set a format (all went well then) */
	if (rejilla_track_image_get_format (REJILLA_TRACK_IMAGE (track)) == REJILLA_IMAGE_FORMAT_NONE) {
		if (status)
			rejilla_status_set_error (status,
						  g_error_new (REJILLA_BURN_ERROR,
							       REJILLA_BURN_ERROR_GENERAL,
							       "%s.\n%s",
							       /* Translators: This is a disc image */
							       _("The format of the disc image could not be identified"),
							       _("Please set it manually")));

		return REJILLA_BURN_ERR;
	}

	if (status)
		rejilla_status_set_completed (status);

	return REJILLA_BURN_OK;
}

static void
rejilla_track_image_cfg_init (RejillaTrackImageCfg *object)
{ }

static void
rejilla_track_image_cfg_finalize (GObject *object)
{
	RejillaTrackImageCfgPrivate *priv;

	priv = REJILLA_TRACK_IMAGE_CFG_PRIVATE (object);

	if (priv->cancel) {
		g_cancellable_cancel (priv->cancel);
		g_object_unref (priv->cancel);
		priv->cancel = NULL;
	}

	if (priv->error) {
		g_error_free (priv->error);
		priv->error = NULL;
	}

	G_OBJECT_CLASS (rejilla_track_image_cfg_parent_class)->finalize (object);
}

static void
rejilla_track_image_cfg_class_init (RejillaTrackImageCfgClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	RejillaTrackClass *track_class = REJILLA_TRACK_CLASS (klass);

	g_type_class_add_private (klass, sizeof (RejillaTrackImageCfgPrivate));

	object_class->finalize = rejilla_track_image_cfg_finalize;

	track_class->get_status = rejilla_track_image_cfg_get_status;
}

/**
 * rejilla_track_image_cfg_new:
 *
 *  Creates a new #RejillaTrackImageCfg object.
 *
 * Return value: a #RejillaTrackImageCfg object.
 **/

RejillaTrackImageCfg *
rejilla_track_image_cfg_new (void)
{
	return g_object_new (REJILLA_TYPE_TRACK_IMAGE_CFG, NULL);
}
