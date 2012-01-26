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
#include <glib-object.h>
#include <glib/gi18n-lib.h>

#include "burn-basics.h"
#include "burn-debug.h"
#include "burn-plugin-manager.h"
#include "rejilla-session.h"
#include "burn-plugin-manager.h"
#include "burn-image-format.h"
#include "librejilla-marshal.h"

#include "rejilla-tags.h"
#include "rejilla-track-image.h"
#include "rejilla-track-data-cfg.h"
#include "rejilla-session-cfg.h"
#include "rejilla-burn-lib.h"
#include "rejilla-session-helper.h"


/**
 * SECTION:rejilla-session-cfg
 * @short_description: Configure automatically a #RejillaBurnSession object
 * @see_also: #RejillaBurn, #RejillaBurnSession
 * @include: rejilla-session-cfg.h
 *
 * This object configures automatically a session reacting to any change
 * made to the various parameters.
 **/

typedef struct _RejillaSessionCfgPrivate RejillaSessionCfgPrivate;
struct _RejillaSessionCfgPrivate
{
	RejillaBurnFlag supported;
	RejillaBurnFlag compulsory;

	gchar *output;

	RejillaSessionError is_valid;

	guint CD_TEXT_modified:1;
	guint configuring:1;
	guint disabled:1;
};

#define REJILLA_SESSION_CFG_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REJILLA_TYPE_SESSION_CFG, RejillaSessionCfgPrivate))

enum
{
	IS_VALID_SIGNAL,
	WRONG_EXTENSION_SIGNAL,
	LAST_SIGNAL
};


static guint session_cfg_signals [LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (RejillaSessionCfg, rejilla_session_cfg, REJILLA_TYPE_SESSION_SPAN);

/**
 * This is to handle tags (and more particularly video ones)
 */

static void
rejilla_session_cfg_tag_changed (RejillaBurnSession *session,
                                 const gchar *tag)
{
	if (!strcmp (tag, REJILLA_VCD_TYPE)) {
		int svcd_type;

		svcd_type = rejilla_burn_session_tag_lookup_int (session, REJILLA_VCD_TYPE);
		if (svcd_type != REJILLA_SVCD)
			rejilla_burn_session_tag_add_int (session,
			                                  REJILLA_VIDEO_OUTPUT_ASPECT,
			                                  REJILLA_VIDEO_ASPECT_4_3);
	}
}

/**
 * rejilla_session_cfg_has_default_output_path:
 * @cfg: a #RejillaSessionCfg
 *
 * This function returns whether the path returned
 * by rejilla_burn_session_get_output () is an 
 * automatically created one.
 *
 * Return value: a #gboolean. TRUE if the path(s)
 * creation is handled by @session, FALSE if it was
 * set.
 **/

gboolean
rejilla_session_cfg_has_default_output_path (RejillaSessionCfg *session)
{
	RejillaBurnSessionClass *klass;
	RejillaBurnResult result;

	klass = REJILLA_BURN_SESSION_CLASS (rejilla_session_cfg_parent_class);
	result = klass->get_output_path (REJILLA_BURN_SESSION (session), NULL, NULL);
	return (result == REJILLA_BURN_OK);
}

static gboolean
rejilla_session_cfg_wrong_extension_signal (RejillaSessionCfg *session)
{
	GValue instance_and_params [1];
	GValue return_value;

	instance_and_params [0].g_type = 0;
	g_value_init (instance_and_params, G_TYPE_FROM_INSTANCE (session));
	g_value_set_instance (instance_and_params, session);
	
	return_value.g_type = 0;
	g_value_init (&return_value, G_TYPE_BOOLEAN);
	g_value_set_boolean (&return_value, FALSE);

	g_signal_emitv (instance_and_params,
			session_cfg_signals [WRONG_EXTENSION_SIGNAL],
			0,
			&return_value);

	g_value_unset (instance_and_params);

	return g_value_get_boolean (&return_value);
}

static RejillaBurnResult
rejilla_session_cfg_set_output_image (RejillaBurnSession *session,
				      RejillaImageFormat format,
				      const gchar *image,
				      const gchar *toc)
{
	gchar *dot;
	gchar *set_toc = NULL;
	gchar * set_image = NULL;
	RejillaBurnResult result;
	RejillaBurnSessionClass *klass;
	const gchar *suffixes [] = {".iso",
				    ".toc",
				    ".cue",
				    ".toc",
				    NULL };

	/* Make sure something actually changed */
	klass = REJILLA_BURN_SESSION_CLASS (rejilla_session_cfg_parent_class);
	klass->get_output_path (REJILLA_BURN_SESSION (session),
	                        &set_image,
	                        &set_toc);

	if (!set_image && !set_toc) {
		/* see if image and toc set paths differ */
		rejilla_burn_session_get_output (REJILLA_BURN_SESSION (session),
		                                 &set_image,
		                                 &set_toc);
		if (set_image && image && !strcmp (set_image, image)) {
			/* It's the same default path so no 
			 * need to carry on and actually set
			 * the path of image. */
			image = NULL;
		}

		if (set_toc && toc && !strcmp (set_toc, toc)) {
			/* It's the same default path so no 
			 * need to carry on and actually set
			 * the path of image. */
			toc = NULL;
		}
	}

	if (set_image)
		g_free (set_image);

	if (set_toc)
		g_free (set_toc);

	/* First set all information */
	result = klass->set_output_image (session,
					  format,
					  image,
					  toc);

	if (!image && !toc)
		return result;

	if (format == REJILLA_IMAGE_FORMAT_NONE)
		format = rejilla_burn_session_get_output_format (session);

	if (format == REJILLA_IMAGE_FORMAT_NONE)
		return result;

	if (format & REJILLA_IMAGE_FORMAT_BIN) {
		dot = g_utf8_strrchr (image, -1, '.');
		if (!dot || strcmp (suffixes [0], dot)) {
			gboolean res;

			res = rejilla_session_cfg_wrong_extension_signal (REJILLA_SESSION_CFG (session));
			if (res) {
				gchar *fixed_path;

				fixed_path = rejilla_image_format_fix_path_extension (format, FALSE, image);
				/* NOTE: call ourselves with the fixed path as this way,
				 * in case the path is the same as the default one after
				 * fixing the extension we'll keep on using default path */
				result = rejilla_burn_session_set_image_output_full (session,
				                                                     format,
				                                                     fixed_path,
				                                                     toc);
				g_free (fixed_path);
			}
		}
	}
	else {
		dot = g_utf8_strrchr (toc, -1, '.');

		if (format & REJILLA_IMAGE_FORMAT_CLONE
		&& (!dot || strcmp (suffixes [1], dot))) {
			gboolean res;

			res = rejilla_session_cfg_wrong_extension_signal (REJILLA_SESSION_CFG (session));
			if (res) {
				gchar *fixed_path;

				fixed_path = rejilla_image_format_fix_path_extension (format, FALSE, toc);
				result = rejilla_burn_session_set_image_output_full (session,
				                                                     format,
				                                                     image,
				                                                     fixed_path);
				g_free (fixed_path);
			}
		}
		else if (format & REJILLA_IMAGE_FORMAT_CUE
		     && (!dot || strcmp (suffixes [2], dot))) {
			gboolean res;

			res = rejilla_session_cfg_wrong_extension_signal (REJILLA_SESSION_CFG (session));
			if (res) {
				gchar *fixed_path;

				fixed_path = rejilla_image_format_fix_path_extension (format, FALSE, toc);
				result = rejilla_burn_session_set_image_output_full (session,
				                                                     format,
				                                                     image,
				                                                     fixed_path);
				g_free (fixed_path);
			}
		}
		else if (format & REJILLA_IMAGE_FORMAT_CDRDAO
		     && (!dot || strcmp (suffixes [3], dot))) {
			gboolean res;

			res = rejilla_session_cfg_wrong_extension_signal (REJILLA_SESSION_CFG (session));
			if (res) {
				gchar *fixed_path;

				fixed_path = rejilla_image_format_fix_path_extension (format, FALSE, toc);
				result = rejilla_burn_session_set_image_output_full (session,
				                                                     format,
				                                                     image,
				                                                     fixed_path);
				g_free (fixed_path);
			}
		}
	}

	return result;
}

static RejillaBurnResult
rejilla_session_cfg_get_output_path (RejillaBurnSession *session,
				     gchar **image,
				     gchar **toc)
{
	gchar *path = NULL;
	RejillaBurnResult result;
	RejillaImageFormat format;
	RejillaBurnSessionClass *klass;

	klass = REJILLA_BURN_SESSION_CLASS (rejilla_session_cfg_parent_class);

	result = klass->get_output_path (session,
					 image,
					 toc);
	if (result == REJILLA_BURN_OK)
		return result;

	format = rejilla_burn_session_get_output_format (session);
	path = rejilla_image_format_get_default_path (format);

	switch (format) {
	case REJILLA_IMAGE_FORMAT_BIN:
		if (image)
			*image = path;
		break;

	case REJILLA_IMAGE_FORMAT_CDRDAO:
	case REJILLA_IMAGE_FORMAT_CLONE:
	case REJILLA_IMAGE_FORMAT_CUE:
		if (toc)
			*toc = path;

		if (image)
			*image = rejilla_image_format_get_complement (format, path);
		break;

	default:
		g_free (path);
		return REJILLA_BURN_ERR;
	}

	return REJILLA_BURN_OK;
}

static RejillaImageFormat
rejilla_session_cfg_get_output_format (RejillaBurnSession *session)
{
	RejillaBurnSessionClass *klass;
	RejillaImageFormat format;

	klass = REJILLA_BURN_SESSION_CLASS (rejilla_session_cfg_parent_class);
	format = klass->get_output_format (session);

	if (format == REJILLA_IMAGE_FORMAT_NONE)
		format = rejilla_burn_session_get_default_output_format (session);

	return format;
}

/**
 * rejilla_session_cfg_get_error:
 * @cfg: a #RejillaSessionCfg
 *
 * This function returns the current status and if
 * autoconfiguration is/was successful.
 *
 * Return value: a #RejillaSessionError.
 **/

RejillaSessionError
rejilla_session_cfg_get_error (RejillaSessionCfg *self)
{
	RejillaSessionCfgPrivate *priv;

	priv = REJILLA_SESSION_CFG_PRIVATE (self);

	if (priv->is_valid == REJILLA_SESSION_VALID
	&&  priv->CD_TEXT_modified)
		return REJILLA_SESSION_NO_CD_TEXT;

	return priv->is_valid;
}

/**
 * rejilla_session_cfg_disable:
 * @cfg: a #RejillaSessionCfg
 *
 * This function disables autoconfiguration
 *
 **/

void
rejilla_session_cfg_disable (RejillaSessionCfg *self)
{
	RejillaSessionCfgPrivate *priv;

	priv = REJILLA_SESSION_CFG_PRIVATE (self);
	priv->disabled = TRUE;
}

/**
 * rejilla_session_cfg_enable:
 * @cfg: a #RejillaSessionCfg
 *
 * This function (re)-enables autoconfiguration
 *
 **/

void
rejilla_session_cfg_enable (RejillaSessionCfg *self)
{
	RejillaSessionCfgPrivate *priv;

	priv = REJILLA_SESSION_CFG_PRIVATE (self);
	priv->disabled = FALSE;
}

static void
rejilla_session_cfg_set_drive_properties_default_flags (RejillaSessionCfg *self)
{
	RejillaMedia media;
	RejillaSessionCfgPrivate *priv;

	priv = REJILLA_SESSION_CFG_PRIVATE (self);

	media = rejilla_burn_session_get_dest_media (REJILLA_BURN_SESSION (self));

	if (REJILLA_MEDIUM_IS (media, REJILLA_MEDIUM_DVDRW_PLUS)
	||  REJILLA_MEDIUM_IS (media, REJILLA_MEDIUM_DVDRW_RESTRICTED)
	||  REJILLA_MEDIUM_IS (media, REJILLA_MEDIUM_DVDRW_PLUS_DL)) {
		/* This is a special case to favour libburn/growisofs
		 * wodim/cdrecord for these types of media. */
		if (priv->supported & REJILLA_BURN_FLAG_MULTI) {
			rejilla_burn_session_add_flag (REJILLA_BURN_SESSION (self),
						       REJILLA_BURN_FLAG_MULTI);

			priv->supported = REJILLA_BURN_FLAG_NONE;
			priv->compulsory = REJILLA_BURN_FLAG_NONE;
			rejilla_burn_session_get_burn_flags (REJILLA_BURN_SESSION (self),
							     &priv->supported,
							     &priv->compulsory);
		}
	}

	/* Always set this flag whenever possible */
	if (priv->supported & REJILLA_BURN_FLAG_BLANK_BEFORE_WRITE) {
		rejilla_burn_session_add_flag (REJILLA_BURN_SESSION (self),
					       REJILLA_BURN_FLAG_BLANK_BEFORE_WRITE);

		if (priv->supported & REJILLA_BURN_FLAG_FAST_BLANK
		&& (media & REJILLA_MEDIUM_UNFORMATTED) == 0)
			rejilla_burn_session_add_flag (REJILLA_BURN_SESSION (self),
						       REJILLA_BURN_FLAG_FAST_BLANK);

		priv->supported = REJILLA_BURN_FLAG_NONE;
		priv->compulsory = REJILLA_BURN_FLAG_NONE;
		rejilla_burn_session_get_burn_flags (REJILLA_BURN_SESSION (self),
						     &priv->supported,
						     &priv->compulsory);
	}

#if 0
	
	/* NOTE: Stop setting DAO here except if it
	 * is declared compulsory (but that's handled
	 * somewhere else) or if it was explicity set.
	 * If we set it just  by  default when it's
	 * supported but not compulsory, MULTI
	 * becomes not supported anymore.
	 * For data the only way it'd be really useful
	 * is if we wanted to fit a selection on the disc.
	 * The problem here is that we don't know
	 * what the size of the final image is going
	 * to be.
	 * For images there are cases when after 
	 * writing an image the user may want to
	 * add some more data to it later. As in the
	 * case of data the only way this flag would
	 * be useful would be to help fit the image
	 * on the disc. But I doubt it would really
	 * help a lot.
	 * For audio we need it to write things like
	 * CD-TEXT but in this case the backend
	 * will return it as compulsory. */

	/* Another case when this flag is needed is
	 * for DVD-RW quick blanked as they only
	 * support DAO. But there again this should
	 * be covered by the backend that should
	 * return DAO as compulsory. */

	/* Leave the code as a reminder. */
	/* When copying with same drive don't set write mode, it'll be set later */
	if (!rejilla_burn_session_same_src_dest_drive (REJILLA_BURN_SESSION (self))
	&&  !(media & REJILLA_MEDIUM_DVD)) {
		/* use DAO whenever it's possible except for DVDs otherwise
		 * wodime which claims to support it will be used by default
		 * instead of say growisofs. */
		if (priv->supported & REJILLA_BURN_FLAG_DAO) {
			rejilla_burn_session_add_flag (REJILLA_BURN_SESSION (self), REJILLA_BURN_FLAG_DAO);

			priv->supported = REJILLA_BURN_FLAG_NONE;
			priv->compulsory = REJILLA_BURN_FLAG_NONE;
			rejilla_burn_session_get_burn_flags (REJILLA_BURN_SESSION (self),
							     &priv->supported,
							     &priv->compulsory);

			/* NOTE: after setting DAO, some flags may become
			 * compulsory like MULTI. */
		}
	}
#endif
}

static void
rejilla_session_cfg_set_drive_properties_flags (RejillaSessionCfg *self,
						RejillaBurnFlag flags)
{
	RejillaDrive *drive;
	RejillaMedia media;
	RejillaBurnFlag flag;
	RejillaMedium *medium;
	RejillaBurnResult result;
	RejillaBurnFlag original_flags;
	RejillaSessionCfgPrivate *priv;

	priv = REJILLA_SESSION_CFG_PRIVATE (self);

	original_flags = rejilla_burn_session_get_flags (REJILLA_BURN_SESSION (self));
	REJILLA_BURN_LOG ("Resetting all flags");
	REJILLA_BURN_LOG_FLAGS (original_flags, "Current are");
	REJILLA_BURN_LOG_FLAGS (flags, "New should be");

	drive = rejilla_burn_session_get_burner (REJILLA_BURN_SESSION (self));
	if (!drive) {
		REJILLA_BURN_LOG ("No drive");
		return;
	}

	medium = rejilla_drive_get_medium (drive);
	if (!medium) {
		REJILLA_BURN_LOG ("No medium");
		return;
	}

	media = rejilla_medium_get_status (medium);

	/* This prevents signals to be emitted while (re-) adding them one by one */
	g_object_freeze_notify (G_OBJECT (self));

	rejilla_burn_session_set_flags (REJILLA_BURN_SESSION (self), REJILLA_BURN_FLAG_NONE);

	priv->supported = REJILLA_BURN_FLAG_NONE;
	priv->compulsory = REJILLA_BURN_FLAG_NONE;
	result = rejilla_burn_session_get_burn_flags (REJILLA_BURN_SESSION (self),
						      &priv->supported,
						      &priv->compulsory);

	if (result != REJILLA_BURN_OK) {
		rejilla_burn_session_set_flags (REJILLA_BURN_SESSION (self), original_flags | flags);
		g_object_thaw_notify (G_OBJECT (self));
		return;
	}

	for (flag = REJILLA_BURN_FLAG_EJECT; flag < REJILLA_BURN_FLAG_LAST; flag <<= 1) {
		/* see if this flag was originally set */
		if ((flags & flag) == 0)
			continue;

		/* Don't set write modes now in this case */
		if (rejilla_burn_session_same_src_dest_drive (REJILLA_BURN_SESSION (self))
		&& (flag & (REJILLA_BURN_FLAG_DAO|REJILLA_BURN_FLAG_RAW)))
			continue;

		if (priv->compulsory
		&& (priv->compulsory & rejilla_burn_session_get_flags (REJILLA_BURN_SESSION (self))) != priv->compulsory) {
			rejilla_burn_session_add_flag (REJILLA_BURN_SESSION (self), priv->compulsory);

			priv->supported = REJILLA_BURN_FLAG_NONE;
			priv->compulsory = REJILLA_BURN_FLAG_NONE;
			rejilla_burn_session_get_burn_flags (REJILLA_BURN_SESSION (self),
							     &priv->supported,
							     &priv->compulsory);
		}

		if (priv->supported & flag) {
			rejilla_burn_session_add_flag (REJILLA_BURN_SESSION (self), flag);

			priv->supported = REJILLA_BURN_FLAG_NONE;
			priv->compulsory = REJILLA_BURN_FLAG_NONE;
			rejilla_burn_session_get_burn_flags (REJILLA_BURN_SESSION (self),
							     &priv->supported,
							     &priv->compulsory);
		}
	}

	if (original_flags & REJILLA_BURN_FLAG_DAO
	&&  priv->supported & REJILLA_BURN_FLAG_DAO) {
		/* Only set DAO if it was explicitely requested */
		rejilla_burn_session_add_flag (REJILLA_BURN_SESSION (self), REJILLA_BURN_FLAG_DAO);

		priv->supported = REJILLA_BURN_FLAG_NONE;
		priv->compulsory = REJILLA_BURN_FLAG_NONE;
		rejilla_burn_session_get_burn_flags (REJILLA_BURN_SESSION (self),
						     &priv->supported,
						     &priv->compulsory);

		/* NOTE: after setting DAO, some flags may become
		 * compulsory like MULTI. */
	}

	rejilla_session_cfg_set_drive_properties_default_flags (self);

	/* These are always supported and better be set. */
	rejilla_burn_session_add_flag (REJILLA_BURN_SESSION (self),
	                               REJILLA_BURN_FLAG_CHECK_SIZE|
	                               REJILLA_BURN_FLAG_NOGRACE);

	/* This one is only supported when we are
	 * burning to a disc or copying a disc but it
	 * would better be set. */
	if (priv->supported & REJILLA_BURN_FLAG_EJECT)
		rejilla_burn_session_add_flag (REJILLA_BURN_SESSION (self),
		                               REJILLA_BURN_FLAG_EJECT);

	/* allow notify::flags signal again */
	g_object_thaw_notify (G_OBJECT (self));
}

static void
rejilla_session_cfg_add_drive_properties_flags (RejillaSessionCfg *self,
						RejillaBurnFlag flags)
{
	/* add flags then wipe out flags from session to check them one by one */
	flags |= rejilla_burn_session_get_flags (REJILLA_BURN_SESSION (self));
	rejilla_session_cfg_set_drive_properties_flags (self, flags);
}

static void
rejilla_session_cfg_rm_drive_properties_flags (RejillaSessionCfg *self,
					       RejillaBurnFlag flags)
{
	/* add flags then wipe out flags from session to check them one by one */
	flags = rejilla_burn_session_get_flags (REJILLA_BURN_SESSION (self)) & (~flags);
	rejilla_session_cfg_set_drive_properties_flags (self, flags);
}

static void
rejilla_session_cfg_check_drive_settings (RejillaSessionCfg *self)
{
	RejillaSessionCfgPrivate *priv;
	RejillaBurnFlag flags;

	priv = REJILLA_SESSION_CFG_PRIVATE (self);

	/* Try to properly update the flags for the current drive */
	flags = rejilla_burn_session_get_flags (REJILLA_BURN_SESSION (self));
	if (rejilla_burn_session_same_src_dest_drive (REJILLA_BURN_SESSION (self))) {
		flags |= REJILLA_BURN_FLAG_BLANK_BEFORE_WRITE|
			 REJILLA_BURN_FLAG_FAST_BLANK;
	}

	/* check each flag before re-adding it */
	rejilla_session_cfg_add_drive_properties_flags (self, flags);
}

static RejillaSessionError
rejilla_session_cfg_check_size (RejillaSessionCfg *self)
{
	RejillaSessionCfgPrivate *priv;
	RejillaBurnFlag flags;
	RejillaMedium *medium;
	RejillaDrive *burner;
	GValue *value = NULL;
	/* in sectors */
	goffset session_size;
	goffset max_sectors;
	goffset disc_size;

	priv = REJILLA_SESSION_CFG_PRIVATE (self);

	burner = rejilla_burn_session_get_burner (REJILLA_BURN_SESSION (self));
	if (!burner) {
		priv->is_valid = REJILLA_SESSION_NO_OUTPUT;
		return REJILLA_SESSION_NO_OUTPUT;
	}

	/* FIXME: here we could check the hard drive space */
	if (rejilla_drive_is_fake (burner)) {
		priv->is_valid = REJILLA_SESSION_VALID;
		return REJILLA_SESSION_VALID;
	}

	medium = rejilla_drive_get_medium (burner);
	if (!medium) {
		priv->is_valid = REJILLA_SESSION_NO_OUTPUT;
		return REJILLA_SESSION_NO_OUTPUT;
	}

	disc_size = rejilla_burn_session_get_available_medium_space (REJILLA_BURN_SESSION (self));
	if (disc_size < 0)
		disc_size = 0;

	/* get input track size */
	session_size = 0;

	if (rejilla_burn_session_tag_lookup (REJILLA_BURN_SESSION (self),
					     REJILLA_DATA_TRACK_SIZE_TAG,
					     &value) == REJILLA_BURN_OK) {
		session_size = g_value_get_int64 (value);
	}
	else if (rejilla_burn_session_tag_lookup (REJILLA_BURN_SESSION (self),
						  REJILLA_STREAM_TRACK_SIZE_TAG,
						  &value) == REJILLA_BURN_OK) {
		session_size = g_value_get_int64 (value);
	}
	else
		rejilla_burn_session_get_size (REJILLA_BURN_SESSION (self),
					       &session_size,
					       NULL);

	REJILLA_BURN_LOG ("Session size %lli/Disc size %lli",
			  session_size,
			  disc_size);

	if (session_size < disc_size) {
		priv->is_valid = REJILLA_SESSION_VALID;
		return REJILLA_SESSION_VALID;
	}

	/* Overburn is only for CDs */
	if ((rejilla_medium_get_status (medium) & REJILLA_MEDIUM_CD) == 0) {
		priv->is_valid = REJILLA_SESSION_INSUFFICIENT_SPACE;
		return REJILLA_SESSION_INSUFFICIENT_SPACE;
	}

	/* The idea would be to test write the disc with cdrecord from /dev/null
	 * until there is an error and see how much we were able to write. So,
	 * when we propose overburning to the user, we could ask if he wants
	 * us to determine how much data can be written to a particular disc
	 * provided he has chosen a real disc. */
	max_sectors = disc_size * 103 / 100;
	if (max_sectors < session_size) {
		priv->is_valid = REJILLA_SESSION_INSUFFICIENT_SPACE;
		return REJILLA_SESSION_INSUFFICIENT_SPACE;
	}

	flags = rejilla_burn_session_get_flags (REJILLA_BURN_SESSION (self));
	if (!(flags & REJILLA_BURN_FLAG_OVERBURN)) {
		RejillaSessionCfgPrivate *priv;

		priv = REJILLA_SESSION_CFG_PRIVATE (self);

		if (!(priv->supported & REJILLA_BURN_FLAG_OVERBURN)) {
			priv->is_valid = REJILLA_SESSION_INSUFFICIENT_SPACE;
			return REJILLA_SESSION_INSUFFICIENT_SPACE;
		}

		priv->is_valid = REJILLA_SESSION_OVERBURN_NECESSARY;
		return REJILLA_SESSION_OVERBURN_NECESSARY;
	}

	priv->is_valid = REJILLA_SESSION_VALID;
	return REJILLA_SESSION_VALID;
}

static void
rejilla_session_cfg_set_tracks_audio_format (RejillaBurnSession *session,
					     RejillaStreamFormat format)
{
	GSList *tracks;
	GSList *iter;

	tracks = rejilla_burn_session_get_tracks (session);
	for (iter = tracks; iter; iter = iter->next) {
		RejillaTrack *track;

		track = iter->data;
		if (!REJILLA_IS_TRACK_STREAM (track))
			continue;

		rejilla_track_stream_set_format (REJILLA_TRACK_STREAM (track), format);
	}
}

static void
rejilla_session_cfg_update (RejillaSessionCfg *self)
{
	RejillaTrackType *source = NULL;
	RejillaSessionCfgPrivate *priv;
	RejillaBurnResult result;
	RejillaStatus *status;
	RejillaDrive *burner;

	priv = REJILLA_SESSION_CFG_PRIVATE (self);

	if (priv->configuring)
		return;

	/* Make sure the session is ready */
	status = rejilla_status_new ();
	result = rejilla_burn_session_get_status (REJILLA_BURN_SESSION (self), status);
	if (result == REJILLA_BURN_NOT_READY || result == REJILLA_BURN_RUNNING) {
		g_object_unref (status);

		priv->is_valid = REJILLA_SESSION_NOT_READY;
		g_signal_emit (self,
			       session_cfg_signals [IS_VALID_SIGNAL],
			       0);
		return;
	}

	if (result == REJILLA_BURN_ERR) {
		GError *error;

		error = rejilla_status_get_error (status);
		if (error) {
			if (error->code == REJILLA_BURN_ERROR_EMPTY) {
				g_object_unref (status);
				g_error_free (error);

				priv->is_valid = REJILLA_SESSION_EMPTY;
				g_signal_emit (self,
					       session_cfg_signals [IS_VALID_SIGNAL],
					       0);
				return;
			}

			g_error_free (error);
		}
	}
	g_object_unref (status);

	/* Make sure there is a source */
	source = rejilla_track_type_new ();
	rejilla_burn_session_get_input_type (REJILLA_BURN_SESSION (self), source);

	if (rejilla_track_type_is_empty (source)) {
		rejilla_track_type_free (source);

		priv->is_valid = REJILLA_SESSION_EMPTY;
		g_signal_emit (self,
			       session_cfg_signals [IS_VALID_SIGNAL],
			       0);
		return;
	}

	/* it can be empty with just an empty track */
	if (rejilla_track_type_get_has_medium (source)
	&&  rejilla_track_type_get_medium_type (source) == REJILLA_MEDIUM_NONE) {
		rejilla_track_type_free (source);

		priv->is_valid = REJILLA_SESSION_NO_INPUT_MEDIUM;
		g_signal_emit (self,
			       session_cfg_signals [IS_VALID_SIGNAL],
			       0);
		return;
	}

	if (rejilla_track_type_get_has_image (source)
	&&  rejilla_track_type_get_image_format (source) == REJILLA_IMAGE_FORMAT_NONE) {
		gchar *uri;
		GSList *tracks;

		rejilla_track_type_free (source);

		tracks = rejilla_burn_session_get_tracks (REJILLA_BURN_SESSION (self));

		/* It can be two cases:
		 * - no image set
		 * - image format cannot be detected */
		if (tracks) {
			RejillaTrack *track;

			track = tracks->data;
			uri = rejilla_track_image_get_source (REJILLA_TRACK_IMAGE (track), TRUE);
			if (uri) {
				priv->is_valid = REJILLA_SESSION_UNKNOWN_IMAGE;
				g_free (uri);
			}
			else
				priv->is_valid = REJILLA_SESSION_NO_INPUT_IMAGE;
		}
		else
			priv->is_valid = REJILLA_SESSION_NO_INPUT_IMAGE;

		g_signal_emit (self,
			       session_cfg_signals [IS_VALID_SIGNAL],
			       0);
		return;
	}

	/* make sure there is an output set */
	burner = rejilla_burn_session_get_burner (REJILLA_BURN_SESSION (self));
	if (!burner) {
		rejilla_track_type_free (source);

		priv->is_valid = REJILLA_SESSION_NO_OUTPUT;
		g_signal_emit (self,
			       session_cfg_signals [IS_VALID_SIGNAL],
			       0);
		return;
	}

	/* Check that current input and output work */
	if (rejilla_track_type_get_has_stream (source)) {
		if (priv->CD_TEXT_modified) {
			/* Try to redo what we undid (after all a new plugin
			 * could have been activated in the mean time ...) and
			 * see what happens */
			rejilla_track_type_set_stream_format (source,
							      REJILLA_METADATA_INFO|
							      rejilla_track_type_get_stream_format (source));
			result = rejilla_burn_session_input_supported (REJILLA_BURN_SESSION (self), source, FALSE);
			if (result == REJILLA_BURN_OK) {
				priv->CD_TEXT_modified = FALSE;

				priv->configuring = TRUE;
				rejilla_session_cfg_set_tracks_audio_format (REJILLA_BURN_SESSION (self),
									     rejilla_track_type_get_stream_format (source));
				priv->configuring = FALSE;
			}
			else {
				/* No, nothing's changed */
				rejilla_track_type_set_stream_format (source,
								      (~REJILLA_METADATA_INFO) &
								      rejilla_track_type_get_stream_format (source));
				result = rejilla_burn_session_input_supported (REJILLA_BURN_SESSION (self), source, FALSE);
			}
		}
		else {
			result = rejilla_burn_session_can_burn (REJILLA_BURN_SESSION (self), FALSE);

			if (result != REJILLA_BURN_OK
			&& (rejilla_track_type_get_stream_format (source) & REJILLA_METADATA_INFO)) {
				/* Another special case in case some burning backends 
				 * don't support CD-TEXT for audio (libburn). If no
				 * other backend is available remove CD-TEXT option but
				 * tell user... */
				rejilla_track_type_set_stream_format (source,
								      (~REJILLA_METADATA_INFO) &
								      rejilla_track_type_get_stream_format (source));

				result = rejilla_burn_session_input_supported (REJILLA_BURN_SESSION (self), source, FALSE);

				REJILLA_BURN_LOG ("Tested support without Metadata information (result %d)", result);
				if (result == REJILLA_BURN_OK) {
					priv->CD_TEXT_modified = TRUE;

					priv->configuring = TRUE;
					rejilla_session_cfg_set_tracks_audio_format (REJILLA_BURN_SESSION (self),
										     rejilla_track_type_get_has_stream (source));
					priv->configuring = FALSE;
				}
			}
		}
	}
	else if (rejilla_track_type_get_has_medium (source)
	&&  (rejilla_track_type_get_medium_type (source) & REJILLA_MEDIUM_HAS_AUDIO)) {
		RejillaImageFormat format = REJILLA_IMAGE_FORMAT_NONE;

		/* If we copy an audio disc check the image
		 * type we're writing to as it may mean that
		 * CD-TEXT cannot be copied.
		 * Make sure that the writing backend
		 * supports writing CD-TEXT?
		 * no, if a backend reports it supports an
		 * image type it should be able to burn all
		 * its data. */
		if (!rejilla_burn_session_is_dest_file (REJILLA_BURN_SESSION (self))) {
			RejillaTrackType *tmp_type;

			tmp_type = rejilla_track_type_new ();

			/* NOTE: this is the same as rejilla_burn_session_supported () */
			result = rejilla_burn_session_get_tmp_image_type_same_src_dest (REJILLA_BURN_SESSION (self), tmp_type);
			if (result == REJILLA_BURN_OK) {
				if (rejilla_track_type_get_has_image (tmp_type)) {
					format = rejilla_track_type_get_image_format (tmp_type);
					priv->CD_TEXT_modified = (format & (REJILLA_IMAGE_FORMAT_CDRDAO|REJILLA_IMAGE_FORMAT_CUE)) == 0;
				}
				else if (rejilla_track_type_get_has_stream (tmp_type)) {
					/* FIXME: for the moment
					 * we consider that this
					 * type will always allow
					 * to copy CD-TEXT */
					priv->CD_TEXT_modified = FALSE;
				}
				else
					priv->CD_TEXT_modified = TRUE;
			}
			else
				priv->CD_TEXT_modified = TRUE;

			rejilla_track_type_free (tmp_type);

			REJILLA_BURN_LOG ("Temporary image type %i", format);
		}
		else {
			result = rejilla_burn_session_can_burn (REJILLA_BURN_SESSION (self), FALSE);
			format = rejilla_burn_session_get_output_format (REJILLA_BURN_SESSION (self));
			priv->CD_TEXT_modified = (format & (REJILLA_IMAGE_FORMAT_CDRDAO|REJILLA_IMAGE_FORMAT_CUE)) == 0;
		}
	}
	else {
		/* Don't use flags as they'll be adapted later. */
		priv->CD_TEXT_modified = FALSE;
		result = rejilla_burn_session_can_burn (REJILLA_BURN_SESSION (self), FALSE);
	}

	if (result != REJILLA_BURN_OK) {
		if (rejilla_track_type_get_has_medium (source)
		&& (rejilla_track_type_get_medium_type (source) & REJILLA_MEDIUM_PROTECTED)
		&&  rejilla_burn_library_input_supported (source) != REJILLA_BURN_OK) {
			/* This is a special case to display a helpful message */
			priv->is_valid = REJILLA_SESSION_DISC_PROTECTED;
			g_signal_emit (self,
				       session_cfg_signals [IS_VALID_SIGNAL],
				       0);
		}
		else {
			priv->is_valid = REJILLA_SESSION_NOT_SUPPORTED;
			g_signal_emit (self,
				       session_cfg_signals [IS_VALID_SIGNAL],
				       0);
		}

		rejilla_track_type_free (source);
		return;
	}

	/* Special case for video projects */
	if (rejilla_track_type_get_has_stream (source)
	&&  REJILLA_STREAM_FORMAT_HAS_VIDEO (rejilla_track_type_get_stream_format (source))) {
		/* Only set if it was not already set */
		if (rejilla_burn_session_tag_lookup (REJILLA_BURN_SESSION (self), REJILLA_VCD_TYPE, NULL) != REJILLA_BURN_OK)
			rejilla_burn_session_tag_add_int (REJILLA_BURN_SESSION (self),
							  REJILLA_VCD_TYPE,
							  REJILLA_SVCD);
	}

	rejilla_track_type_free (source);

	/* Configure flags */
	priv->configuring = TRUE;

	if (rejilla_drive_is_fake (burner))
		/* Remove some impossible flags */
		rejilla_burn_session_remove_flag (REJILLA_BURN_SESSION (self),
						  REJILLA_BURN_FLAG_DUMMY|
						  REJILLA_BURN_FLAG_NO_TMP_FILES);

	priv->configuring = FALSE;

	/* Finally check size */
	if (rejilla_burn_session_same_src_dest_drive (REJILLA_BURN_SESSION (self))) {
		priv->is_valid = REJILLA_SESSION_VALID;
		g_signal_emit (self,
			       session_cfg_signals [IS_VALID_SIGNAL],
			       0);
	}
	else {
		rejilla_session_cfg_check_size (self);
		g_signal_emit (self,
			       session_cfg_signals [IS_VALID_SIGNAL],
			       0);
	}
}

static void
rejilla_session_cfg_session_loaded (RejillaTrackDataCfg *track,
				    RejillaMedium *medium,
				    gboolean is_loaded,
				    RejillaSessionCfg *session)
{
	RejillaBurnFlag session_flags;
	RejillaSessionCfgPrivate *priv;

	priv = REJILLA_SESSION_CFG_PRIVATE (session);
	if (priv->disabled)
		return;

	session_flags = rejilla_burn_session_get_flags (REJILLA_BURN_SESSION (session));
	if (is_loaded) {
		/* Set the correct medium and add the flag */
		rejilla_burn_session_set_burner (REJILLA_BURN_SESSION (session),
						 rejilla_medium_get_drive (medium));

		if ((session_flags & REJILLA_BURN_FLAG_MERGE) == 0)
			rejilla_session_cfg_add_drive_properties_flags (session, REJILLA_BURN_FLAG_MERGE);
	}
	else if ((session_flags & REJILLA_BURN_FLAG_MERGE) != 0)
		rejilla_session_cfg_rm_drive_properties_flags (session, REJILLA_BURN_FLAG_MERGE);
}

static void
rejilla_session_cfg_track_added (RejillaBurnSession *session,
				 RejillaTrack *track)
{
	RejillaSessionCfgPrivate *priv;

	priv = REJILLA_SESSION_CFG_PRIVATE (session);
	if (priv->disabled)
		return;

	if (REJILLA_IS_TRACK_DATA_CFG (track))
		g_signal_connect (track,
				  "session-loaded",
				  G_CALLBACK (rejilla_session_cfg_session_loaded),
				  session);

	/* A track was added: 
	 * - check if all flags are supported
	 * - check available formats for path
	 * - set one path */
	rejilla_session_cfg_check_drive_settings (REJILLA_SESSION_CFG (session));
	rejilla_session_cfg_update (REJILLA_SESSION_CFG (session));
}

static void
rejilla_session_cfg_track_removed (RejillaBurnSession *session,
				   RejillaTrack *track,
				   guint former_position)
{
	RejillaSessionCfgPrivate *priv;

	priv = REJILLA_SESSION_CFG_PRIVATE (session);
	if (priv->disabled)
		return;

	/* Just in case */
	g_signal_handlers_disconnect_by_func (track,
					      rejilla_session_cfg_session_loaded,
					      session);

	/* If there were several tracks and at least one remained there is no
	 * use checking flags since the source type has not changed anyway.
	 * If there is no more track, there is no use checking flags anyway. */
	rejilla_session_cfg_update (REJILLA_SESSION_CFG (session));
}

static void
rejilla_session_cfg_track_changed (RejillaBurnSession *session,
				   RejillaTrack *track)
{
	RejillaSessionCfgPrivate *priv;

	priv = REJILLA_SESSION_CFG_PRIVATE (session);
	if (priv->disabled)
		return;

	/* when that happens it's mostly because a medium source changed, or
	 * a new image was set. 
	 * - check if all flags are supported
	 * - check available formats for path
	 * - set one path if need be */
	rejilla_session_cfg_check_drive_settings (REJILLA_SESSION_CFG (session));
	rejilla_session_cfg_update (REJILLA_SESSION_CFG (session));
}

static void
rejilla_session_cfg_output_changed (RejillaBurnSession *session,
				    RejillaMedium *former)
{
	RejillaSessionCfgPrivate *priv;
	RejillaTrackType *type;

	priv = REJILLA_SESSION_CFG_PRIVATE (session);
	if (priv->disabled)
		return;

	/* Case for video project */
	type = rejilla_track_type_new ();
	rejilla_burn_session_get_input_type (session, type);

	if (rejilla_track_type_get_has_stream (type)
	&&  REJILLA_STREAM_FORMAT_HAS_VIDEO (rejilla_track_type_get_stream_format (type))) {
		RejillaMedia media;

		media = rejilla_burn_session_get_dest_media (session);
		if (media & REJILLA_MEDIUM_DVD)
			rejilla_burn_session_tag_add_int (session,
			                                  REJILLA_DVD_STREAM_FORMAT,
			                                  REJILLA_AUDIO_FORMAT_AC3);
		else if (media & REJILLA_MEDIUM_CD)
			rejilla_burn_session_tag_add_int (session,
							  REJILLA_DVD_STREAM_FORMAT,
							  REJILLA_AUDIO_FORMAT_MP2);
		else {
			RejillaImageFormat format;

			format = rejilla_burn_session_get_output_format (session);
			if (format == REJILLA_IMAGE_FORMAT_CUE)
				rejilla_burn_session_tag_add_int (session,
								  REJILLA_DVD_STREAM_FORMAT,
								  REJILLA_AUDIO_FORMAT_MP2);
			else
				rejilla_burn_session_tag_add_int (session,
								  REJILLA_DVD_STREAM_FORMAT,
								  REJILLA_AUDIO_FORMAT_AC3);
		}
	}
	rejilla_track_type_free (type);

	/* In this case need to :
	 * - check if all flags are supported
	 * - for images, set a path if it wasn't already set */
	rejilla_session_cfg_check_drive_settings (REJILLA_SESSION_CFG (session));
	rejilla_session_cfg_update (REJILLA_SESSION_CFG (session));
}

static void
rejilla_session_cfg_caps_changed (RejillaPluginManager *manager,
				  RejillaSessionCfg *self)
{
	RejillaSessionCfgPrivate *priv;

	priv = REJILLA_SESSION_CFG_PRIVATE (self);
	if (priv->disabled)
		return;

	/* In this case we need to check if:
	 * - flags are supported or not supported anymore
	 * - image types as input/output are supported
	 * - if the current set of input/output still works */
	rejilla_session_cfg_check_drive_settings (self);
	rejilla_session_cfg_update (self);
}

/**
 * rejilla_session_cfg_add_flags:
 * @cfg: a #RejillaSessionCfg
 * @flags: a #RejillaBurnFlag
 *
 * Adds all flags from @flags that are supported.
 *
 **/

void
rejilla_session_cfg_add_flags (RejillaSessionCfg *self,
			       RejillaBurnFlag flags)
{
	RejillaSessionCfgPrivate *priv;

	priv = REJILLA_SESSION_CFG_PRIVATE (self);

	if ((priv->supported & flags) != flags)
		return;

	if ((rejilla_burn_session_get_flags (REJILLA_BURN_SESSION (self)) & flags) == flags)
		return;

	rejilla_session_cfg_add_drive_properties_flags (self, flags);
	rejilla_session_cfg_update (self);
}

/**
 * rejilla_session_cfg_remove_flags:
 * @cfg: a #RejillaSessionCfg
 * @flags: a #RejillaBurnFlag
 *
 * Removes all flags that are not compulsory.
 *
 **/

void
rejilla_session_cfg_remove_flags (RejillaSessionCfg *self,
				  RejillaBurnFlag flags)
{
	RejillaSessionCfgPrivate *priv;

	priv = REJILLA_SESSION_CFG_PRIVATE (self);

	rejilla_burn_session_remove_flag (REJILLA_BURN_SESSION (self), flags);

	/* For this case reset all flags as some flags could
	 * be made available after the removal of one flag
	 * Example: After the removal of MULTI, FAST_BLANK
	 * becomes available again for DVDRW sequential */
	rejilla_session_cfg_set_drive_properties_default_flags (self);
	rejilla_session_cfg_update (self);
}

/**
 * rejilla_session_cfg_is_supported:
 * @cfg: a #RejillaSessionCfg
 * @flag: a #RejillaBurnFlag
 *
 * Checks whether a particular flag is supported.
 *
 * Return value: a #gboolean. TRUE if it is supported;
 * FALSE otherwise.
 **/

gboolean
rejilla_session_cfg_is_supported (RejillaSessionCfg *self,
				  RejillaBurnFlag flag)
{
	RejillaSessionCfgPrivate *priv;

	priv = REJILLA_SESSION_CFG_PRIVATE (self);
	return (priv->supported & flag) == flag;
}

/**
 * rejilla_session_cfg_is_compulsory:
 * @cfg: a #RejillaSessionCfg
 * @flag: a #RejillaBurnFlag
 *
 * Checks whether a particular flag is compulsory.
 *
 * Return value: a #gboolean. TRUE if it is compulsory;
 * FALSE otherwise.
 **/

gboolean
rejilla_session_cfg_is_compulsory (RejillaSessionCfg *self,
				   RejillaBurnFlag flag)
{
	RejillaSessionCfgPrivate *priv;

	priv = REJILLA_SESSION_CFG_PRIVATE (self);
	return (priv->compulsory & flag) == flag;
}

static void
rejilla_session_cfg_init (RejillaSessionCfg *object)
{
	RejillaSessionCfgPrivate *priv;
	RejillaPluginManager *manager;

	priv = REJILLA_SESSION_CFG_PRIVATE (object);

	manager = rejilla_plugin_manager_get_default ();
	g_signal_connect (manager,
	                  "caps-changed",
	                  G_CALLBACK (rejilla_session_cfg_caps_changed),
	                  object);
}

static void
rejilla_session_cfg_finalize (GObject *object)
{
	RejillaPluginManager *manager;
	RejillaSessionCfgPrivate *priv;
	GSList *tracks;

	priv = REJILLA_SESSION_CFG_PRIVATE (object);

	tracks = rejilla_burn_session_get_tracks (REJILLA_BURN_SESSION (object));
	for (; tracks; tracks = tracks->next) {
		RejillaTrack *track;

		track = tracks->data;
		g_signal_handlers_disconnect_by_func (track,
						      rejilla_session_cfg_session_loaded,
						      object);
	}

	manager = rejilla_plugin_manager_get_default ();
	g_signal_handlers_disconnect_by_func (manager,
	                                      rejilla_session_cfg_caps_changed,
	                                      object);

	G_OBJECT_CLASS (rejilla_session_cfg_parent_class)->finalize (object);
}

static void
rejilla_session_cfg_class_init (RejillaSessionCfgClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	RejillaBurnSessionClass *session_class = REJILLA_BURN_SESSION_CLASS (klass);

	g_type_class_add_private (klass, sizeof (RejillaSessionCfgPrivate));

	object_class->finalize = rejilla_session_cfg_finalize;

	session_class->get_output_path = rejilla_session_cfg_get_output_path;
	session_class->get_output_format = rejilla_session_cfg_get_output_format;
	session_class->set_output_image = rejilla_session_cfg_set_output_image;

	session_class->track_added = rejilla_session_cfg_track_added;
	session_class->track_removed = rejilla_session_cfg_track_removed;
	session_class->track_changed = rejilla_session_cfg_track_changed;
	session_class->output_changed = rejilla_session_cfg_output_changed;
	session_class->tag_changed = rejilla_session_cfg_tag_changed;

	session_cfg_signals [WRONG_EXTENSION_SIGNAL] =
		g_signal_new ("wrong_extension",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_LAST | G_SIGNAL_RUN_CLEANUP | G_SIGNAL_ACTION,
		              0,
		              NULL, NULL,
		              rejilla_marshal_BOOLEAN__VOID,
		              G_TYPE_BOOLEAN,
			      0,
		              G_TYPE_NONE);
	session_cfg_signals [IS_VALID_SIGNAL] =
		g_signal_new ("is_valid",
		              G_OBJECT_CLASS_TYPE (klass),
		              G_SIGNAL_RUN_LAST | G_SIGNAL_RUN_CLEANUP | G_SIGNAL_ACTION,
		              0,
		              NULL, NULL,
		              g_cclosure_marshal_VOID__VOID,
		              G_TYPE_NONE,
			      0,
		              G_TYPE_NONE);
}

/**
 * rejilla_session_cfg_new:
 *
 *  Creates a new #RejillaSessionCfg object.
 *
 * Return value: a #RejillaSessionCfg object.
 **/

RejillaSessionCfg *
rejilla_session_cfg_new (void)
{
	return g_object_new (REJILLA_TYPE_SESSION_CFG, NULL);
}
