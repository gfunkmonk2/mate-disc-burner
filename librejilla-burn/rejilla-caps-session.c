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
#include <glib/gi18n.h>

#include "burn-caps.h"
#include "burn-debug.h"
#include "rejilla-plugin.h"
#include "rejilla-plugin-private.h"
#include "rejilla-plugin-information.h"
#include "rejilla-session-helper.h"

#define REJILLA_BURN_CAPS_SHOULD_BLANK(media_MACRO, flags_MACRO)		\
	((media_MACRO & REJILLA_MEDIUM_UNFORMATTED) ||				\
	((media_MACRO & (REJILLA_MEDIUM_HAS_AUDIO|REJILLA_MEDIUM_HAS_DATA)) &&	\
	 (flags_MACRO & (REJILLA_BURN_FLAG_MERGE|REJILLA_BURN_FLAG_APPEND)) == FALSE))

#define REJILLA_BURN_CAPS_NOT_SUPPORTED_LOG_RES(session)			\
{										\
	rejilla_burn_session_log (session, "Unsupported type of task operation"); \
	REJILLA_BURN_LOG ("Unsupported type of task operation");		\
	return REJILLA_BURN_NOT_SUPPORTED;					\
}

static RejillaBurnResult
rejilla_burn_caps_get_blanking_flags_real (RejillaBurnCaps *caps,
                                           gboolean ignore_errors,
					   RejillaMedia media,
					   RejillaBurnFlag session_flags,
					   RejillaBurnFlag *supported,
					   RejillaBurnFlag *compulsory)
{
	GSList *iter;
	gboolean supported_media;
	RejillaBurnFlag supported_flags = REJILLA_BURN_FLAG_NONE;
	RejillaBurnFlag compulsory_flags = REJILLA_BURN_FLAG_ALL;

	REJILLA_BURN_LOG_DISC_TYPE (media, "Getting blanking flags for");
	if (media == REJILLA_MEDIUM_NONE) {
		REJILLA_BURN_LOG ("Blanking not possible: no media");
		if (supported)
			*supported = REJILLA_BURN_FLAG_NONE;
		if (compulsory)
			*compulsory = REJILLA_BURN_FLAG_NONE;
		return REJILLA_BURN_NOT_SUPPORTED;
	}

	supported_media = FALSE;
	for (iter = caps->priv->caps_list; iter; iter = iter->next) {
		RejillaMedia caps_media;
		RejillaCaps *caps;
		GSList *links;

		caps = iter->data;
		if (!rejilla_track_type_get_has_medium (&caps->type))
			continue;

		caps_media = rejilla_track_type_get_medium_type (&caps->type);
		if ((media & caps_media) != media)
			continue;

		for (links = caps->links; links; links = links->next) {
			GSList *plugins;
			RejillaCapsLink *link;

			link = links->data;

			if (link->caps != NULL)
				continue;

			supported_media = TRUE;
			/* don't need the plugins to be sorted since we go
			 * through all the plugin list to get all blanking flags
			 * available. */
			for (plugins = link->plugins; plugins; plugins = plugins->next) {
				RejillaPlugin *plugin;
				RejillaBurnFlag supported_plugin;
				RejillaBurnFlag compulsory_plugin;

				plugin = plugins->data;
				if (!rejilla_plugin_get_active (plugin, ignore_errors))
					continue;

				if (!rejilla_plugin_get_blank_flags (plugin,
								     media,
								     session_flags,
								     &supported_plugin,
								     &compulsory_plugin))
					continue;

				supported_flags |= supported_plugin;
				compulsory_flags &= compulsory_plugin;
			}
		}
	}

	if (!supported_media) {
		REJILLA_BURN_LOG ("media blanking not supported");
		return REJILLA_BURN_NOT_SUPPORTED;
	}

	/* This is a special case that is in MMC specs:
	 * DVD-RW sequential must be fully blanked
	 * if we really want multisession support. */
	if (REJILLA_MEDIUM_IS (media, REJILLA_MEDIUM_DVDRW)
	&& (session_flags & REJILLA_BURN_FLAG_MULTI)) {
		if (compulsory_flags & REJILLA_BURN_FLAG_FAST_BLANK) {
			REJILLA_BURN_LOG ("fast media blanking only supported but multisession required for DVDRW");
			return REJILLA_BURN_NOT_SUPPORTED;
		}

		supported_flags &= ~REJILLA_BURN_FLAG_FAST_BLANK;

		REJILLA_BURN_LOG ("removed fast blank for a DVDRW with multisession");
	}

	if (supported)
		*supported = supported_flags;
	if (compulsory)
		*compulsory = compulsory_flags;

	return REJILLA_BURN_OK;
}

/**
 * rejilla_burn_session_get_blank_flags:
 * @session: a #RejillaBurnSession
 * @supported: a #RejillaBurnFlag
 * @compulsory: a #RejillaBurnFlag
 *
 * Given the various parameters stored in @session,
 * stores in @supported and @compulsory, the flags
 * that can be used (@supported) and must be used
 * (@compulsory) when blanking the medium in the
 * #RejillaDrive (set with rejilla_burn_session_set_burner ()).
 *
 * Return value: a #RejillaBurnResult.
 * REJILLA_BURN_OK if the retrieval was successful.
 * REJILLA_BURN_ERR otherwise.
 **/

RejillaBurnResult
rejilla_burn_session_get_blank_flags (RejillaBurnSession *session,
				      RejillaBurnFlag *supported,
				      RejillaBurnFlag *compulsory)
{
	RejillaMedia media;
	RejillaBurnCaps *self;
	RejillaBurnResult result;
	RejillaBurnFlag session_flags;

	media = rejilla_burn_session_get_dest_media (session);
	REJILLA_BURN_LOG_DISC_TYPE (media, "Getting blanking flags for");

	if (media == REJILLA_MEDIUM_NONE) {
		REJILLA_BURN_LOG ("Blanking not possible: no media");
		if (supported)
			*supported = REJILLA_BURN_FLAG_NONE;
		if (compulsory)
			*compulsory = REJILLA_BURN_FLAG_NONE;
		return REJILLA_BURN_NOT_SUPPORTED;
	}

	session_flags = rejilla_burn_session_get_flags (session);

	self = rejilla_burn_caps_get_default ();
	result = rejilla_burn_caps_get_blanking_flags_real (self,
	                                                    rejilla_burn_session_get_strict_support (session) == FALSE,
							    media,
							    session_flags,
							    supported,
							    compulsory);
	g_object_unref (self);

	return result;
}

static RejillaBurnResult
rejilla_burn_caps_can_blank_real (RejillaBurnCaps *self,
                                  gboolean ignore_plugin_errors,
                                  RejillaMedia media,
				  RejillaBurnFlag flags)
{
	GSList *iter;

	REJILLA_BURN_LOG_DISC_TYPE (media, "Testing blanking caps for");
	if (media == REJILLA_MEDIUM_NONE) {
		REJILLA_BURN_LOG ("no media => no blanking possible");
		return REJILLA_BURN_NOT_SUPPORTED;
	}

	/* This is a special case from MMC: DVD-RW sequential
	 * can only be multisession is they were fully blanked
	 * so if there are the two flags, abort. */
	if (REJILLA_MEDIUM_IS (media, REJILLA_MEDIUM_DVDRW)
	&&  (flags & REJILLA_BURN_FLAG_MULTI)
	&&  (flags & REJILLA_BURN_FLAG_FAST_BLANK)) {
		REJILLA_BURN_LOG ("fast media blanking only supported but multisession required for DVD-RW");
		return REJILLA_BURN_NOT_SUPPORTED;
	}

	for (iter = self->priv->caps_list; iter; iter = iter->next) {
		RejillaCaps *caps;
		GSList *links;

		caps = iter->data;
		if (caps->type.type != REJILLA_TRACK_TYPE_DISC)
			continue;

		if ((media & rejilla_track_type_get_medium_type (&caps->type)) != media)
			continue;

		REJILLA_BURN_LOG_TYPE (&caps->type, "Searching links for caps");

		for (links = caps->links; links; links = links->next) {
			GSList *plugins;
			RejillaCapsLink *link;

			link = links->data;

			if (link->caps != NULL)
				continue;

			REJILLA_BURN_LOG ("Searching plugins");

			/* Go through all plugins for the link and stop if we 
			 * find at least one active plugin that accepts the
			 * flags. No need for plugins to be sorted */
			for (plugins = link->plugins; plugins; plugins = plugins->next) {
				RejillaPlugin *plugin;

				plugin = plugins->data;
				if (!rejilla_plugin_get_active (plugin, ignore_plugin_errors))
					continue;

				if (rejilla_plugin_check_blank_flags (plugin, media, flags)) {
					REJILLA_BURN_LOG_DISC_TYPE (media, "Can blank");
					return REJILLA_BURN_OK;
				}
			}
		}
	}

	REJILLA_BURN_LOG_DISC_TYPE (media, "No blanking capabilities for");

	return REJILLA_BURN_NOT_SUPPORTED;
}

/**
 * rejilla_burn_session_can_blank:
 * @session: a #RejillaBurnSession
 *
 * Given the various parameters stored in @session, this
 * function checks whether the medium in the
 * #RejillaDrive (set with rejilla_burn_session_set_burner ())
 * can be blanked.
 *
 * Return value: a #RejillaBurnResult.
 * REJILLA_BURN_OK if it is possible.
 * REJILLA_BURN_ERR otherwise.
 **/

RejillaBurnResult
rejilla_burn_session_can_blank (RejillaBurnSession *session)
{
	RejillaMedia media;
	RejillaBurnFlag flags;
	RejillaBurnCaps *self;
	RejillaBurnResult result;

	self = rejilla_burn_caps_get_default ();

	media = rejilla_burn_session_get_dest_media (session);
	REJILLA_BURN_LOG_DISC_TYPE (media, "Testing blanking caps for");

	if (media == REJILLA_MEDIUM_NONE) {
		REJILLA_BURN_LOG ("no media => no blanking possible");
		g_object_unref (self);
		return REJILLA_BURN_NOT_SUPPORTED;
	}

	flags = rejilla_burn_session_get_flags (session);
	result = rejilla_burn_caps_can_blank_real (self,
	                                           rejilla_burn_session_get_strict_support (session) == FALSE,
	                                           media,
	                                           flags);
	g_object_unref (self);

	return result;
}

static void
rejilla_caps_link_get_record_flags (RejillaCapsLink *link,
                                    gboolean ignore_plugin_errors,
				    RejillaMedia media,
				    RejillaBurnFlag session_flags,
				    RejillaBurnFlag *supported,
				    RejillaBurnFlag *compulsory_retval)
{
	GSList *iter;
	RejillaBurnFlag compulsory;

	compulsory = REJILLA_BURN_FLAG_ALL;

	/* Go through all plugins to get the supported/... record flags for link */
	for (iter = link->plugins; iter; iter = iter->next) {
		gboolean result;
		RejillaPlugin *plugin;
		RejillaBurnFlag plugin_supported;
		RejillaBurnFlag plugin_compulsory;

		plugin = iter->data;
		if (!rejilla_plugin_get_active (plugin, ignore_plugin_errors))
			continue;

		result = rejilla_plugin_get_record_flags (plugin,
							  media,
							  session_flags,
							  &plugin_supported,
							  &plugin_compulsory);
		if (!result)
			continue;

		*supported |= plugin_supported;
		compulsory &= plugin_compulsory;
	}

	*compulsory_retval = compulsory;
}

static void
rejilla_caps_link_get_data_flags (RejillaCapsLink *link,
                                  gboolean ignore_plugin_errors,
				  RejillaMedia media,
				  RejillaBurnFlag session_flags,
				  RejillaBurnFlag *supported)
{
	GSList *iter;

	/* Go through all plugins the get the supported/... data flags for link */
	for (iter = link->plugins; iter; iter = iter->next) {
		gboolean result;
		RejillaPlugin *plugin;
		RejillaBurnFlag plugin_supported;
		RejillaBurnFlag plugin_compulsory;

		plugin = iter->data;
		if (!rejilla_plugin_get_active (plugin, ignore_plugin_errors))
			continue;

		result = rejilla_plugin_get_image_flags (plugin,
							 media,
							 session_flags,
							 &plugin_supported,
							 &plugin_compulsory);
		*supported |= plugin_supported;
	}
}

static gboolean
rejilla_caps_link_check_data_flags (RejillaCapsLink *link,
                                    gboolean ignore_plugin_errors,
				    RejillaBurnFlag session_flags,
				    RejillaMedia media)
{
	GSList *iter;
	RejillaBurnFlag flags;

	/* here we just make sure that at least one of the plugins in the link
	 * can comply with the flags (APPEND/MERGE) */
	flags = session_flags & (REJILLA_BURN_FLAG_APPEND|REJILLA_BURN_FLAG_MERGE);

	/* If there are no image flags forget it */
	if (flags == REJILLA_BURN_FLAG_NONE)
		return TRUE;

	/* Go through all plugins; at least one must support image flags */
	for (iter = link->plugins; iter; iter = iter->next) {
		gboolean result;
		RejillaPlugin *plugin;

		plugin = iter->data;
		if (!rejilla_plugin_get_active (plugin, ignore_plugin_errors))
			continue;

		result = rejilla_plugin_check_image_flags (plugin,
							   media,
							   session_flags);
		if (result)
			return TRUE;
	}

	return FALSE;
}

static gboolean
rejilla_caps_link_check_record_flags (RejillaCapsLink *link,
                                      gboolean ignore_plugin_errors,
				      RejillaBurnFlag session_flags,
				      RejillaMedia media)
{
	GSList *iter;
	RejillaBurnFlag flags;

	flags = session_flags & REJILLA_PLUGIN_BURN_FLAG_MASK;

	/* If there are no record flags forget it */
	if (flags == REJILLA_BURN_FLAG_NONE)
		return TRUE;
	
	/* Go through all plugins: at least one must support record flags */
	for (iter = link->plugins; iter; iter = iter->next) {
		gboolean result;
		RejillaPlugin *plugin;

		plugin = iter->data;
		if (!rejilla_plugin_get_active (plugin, ignore_plugin_errors))
			continue;

		result = rejilla_plugin_check_record_flags (plugin,
							    media,
							    session_flags);
		if (result)
			return TRUE;
	}

	return FALSE;
}

static gboolean
rejilla_caps_link_check_media_restrictions (RejillaCapsLink *link,
                                            gboolean ignore_plugin_errors,
					    RejillaMedia media)
{
	GSList *iter;

	/* Go through all plugins: at least one must support media */
	for (iter = link->plugins; iter; iter = iter->next) {
		gboolean result;
		RejillaPlugin *plugin;

		plugin = iter->data;
		if (!rejilla_plugin_get_active (plugin, ignore_plugin_errors))
			continue;

		result = rejilla_plugin_check_media_restrictions (plugin, media);
		if (result)
			return TRUE;
	}

	return FALSE;
}

static RejillaBurnResult
rejilla_caps_report_plugin_error (RejillaPlugin *plugin,
                                  RejillaForeachPluginErrorCb callback,
                                  gpointer user_data)
{
	GSList *errors;

	errors = rejilla_plugin_get_errors (plugin);
	if (!errors)
		return REJILLA_BURN_ERR;

	do {
		RejillaPluginError *error;
		RejillaBurnResult result;

		error = errors->data;
		result = callback (error->type, error->detail, user_data);
		if (result == REJILLA_BURN_RETRY) {
			/* Something has been done
			 * to fix the error like an install
			 * so reload the errors */
			rejilla_plugin_check_plugin_ready (plugin);
			errors = rejilla_plugin_get_errors (plugin);
			continue;
		}

		if (result != REJILLA_BURN_OK)
			return result;

		errors = errors->next;
	} while (errors);

	return REJILLA_BURN_OK;
}

struct _RejillaFindLinkCtx {
	RejillaMedia media;
	RejillaTrackType *input;
	RejillaPluginIOFlag io_flags;
	RejillaBurnFlag session_flags;

	RejillaForeachPluginErrorCb callback;
	gpointer user_data;

	guint ignore_plugin_errors:1;
	guint check_session_flags:1;
};
typedef struct _RejillaFindLinkCtx RejillaFindLinkCtx;

static void
rejilla_caps_find_link_set_ctx (RejillaBurnSession *session,
                                RejillaFindLinkCtx *ctx,
                                RejillaTrackType *input)
{
	ctx->input = input;

	if (ctx->check_session_flags) {
		ctx->session_flags = rejilla_burn_session_get_flags (session);

		if (REJILLA_BURN_SESSION_NO_TMP_FILE (session))
			ctx->io_flags = REJILLA_PLUGIN_IO_ACCEPT_PIPE;
		else
			ctx->io_flags = REJILLA_PLUGIN_IO_ACCEPT_FILE;
	}
	else
		ctx->io_flags = REJILLA_PLUGIN_IO_ACCEPT_FILE|
					REJILLA_PLUGIN_IO_ACCEPT_PIPE;

	if (!ctx->callback)
		ctx->ignore_plugin_errors = (rejilla_burn_session_get_strict_support (session) == FALSE);
	else
		ctx->ignore_plugin_errors = TRUE;
}

static RejillaBurnResult
rejilla_caps_find_link (RejillaCaps *caps,
                        RejillaFindLinkCtx *ctx)
{
	GSList *iter;

	REJILLA_BURN_LOG_WITH_TYPE (&caps->type, REJILLA_PLUGIN_IO_NONE, "Found link (with %i links):", g_slist_length (caps->links));

	/* Here we only make sure we have at least one link working. For a link
	 * to be followed it must first:
	 * - link to a caps with correct io flags
	 * - have at least a plugin accepting the record flags if caps type is
	 *   a disc (that means that the link is the recording part)
	 *
	 * and either:
	 * - link to a caps equal to the input
	 * - link to a caps (linking itself to another caps, ...) accepting the
	 *   input
	 */

	for (iter = caps->links; iter; iter = iter->next) {
		RejillaCapsLink *link;
		RejillaBurnResult result;

		link = iter->data;

		if (!link->caps)
			continue;
		
		/* check that the link has some active plugin */
		if (!rejilla_caps_link_active (link, ctx->ignore_plugin_errors))
			continue;

		/* since this link contains recorders, check that at least one
		 * of them can handle the record flags */
		if (ctx->check_session_flags && rejilla_track_type_get_has_medium (&caps->type)) {
			if (!rejilla_caps_link_check_record_flags (link, ctx->ignore_plugin_errors, ctx->session_flags, ctx->media))
				continue;

			if (rejilla_caps_link_check_recorder_flags_for_input (link, ctx->session_flags) != REJILLA_BURN_OK)
				continue;
		}

		/* first see if that's the perfect fit:
		 * - it must have the same caps (type + subtype)
		 * - it must have the proper IO */
		if (rejilla_track_type_get_has_data (&link->caps->type)) {
			if (ctx->check_session_flags
			&& !rejilla_caps_link_check_data_flags (link, ctx->ignore_plugin_errors, ctx->session_flags, ctx->media))
				continue;
		}
		else if (!rejilla_caps_link_check_media_restrictions (link, ctx->ignore_plugin_errors, ctx->media))
			continue;

		if ((link->caps->flags & REJILLA_PLUGIN_IO_ACCEPT_FILE)
		&&   rejilla_caps_is_compatible_type (link->caps, ctx->input)) {
			if (ctx->callback) {
				RejillaPlugin *plugin;

				/* If we are supposed to report/signal that the plugin
				 * could be used but only if some more elements are 
				 * installed */
				plugin = rejilla_caps_link_need_download (link);
				if (plugin)
					return rejilla_caps_report_plugin_error (plugin, ctx->callback, ctx->user_data);
			}
			return REJILLA_BURN_OK;
		}

		/* we can't go further than a DISC type */
		if (rejilla_track_type_get_has_medium (&link->caps->type))
			continue;

		if ((link->caps->flags & ctx->io_flags) == REJILLA_PLUGIN_IO_NONE)
			continue;

		/* try to see where the inputs of this caps leads to */
		result = rejilla_caps_find_link (link->caps, ctx);
		if (result == REJILLA_BURN_CANCEL)
			return result;

		if (result == REJILLA_BURN_OK) {
			if (ctx->callback) {
				RejillaPlugin *plugin;

				/* If we are supposed to report/signal that the plugin
				 * could be used but only if some more elements are 
				 * installed */
				plugin = rejilla_caps_link_need_download (link);
				if (plugin)
					return rejilla_caps_report_plugin_error (plugin, ctx->callback, ctx->user_data);
			}
			return REJILLA_BURN_OK;
		}
	}

	return REJILLA_BURN_NOT_SUPPORTED;
}

static RejillaBurnResult
rejilla_caps_try_output (RejillaBurnCaps *self,
                         RejillaFindLinkCtx *ctx,
                         RejillaTrackType *output)
{
	RejillaCaps *caps;

	/* here we search the start caps */
	caps = rejilla_burn_caps_find_start_caps (self, output);
	if (!caps) {
		REJILLA_BURN_LOG ("No caps available");
		return REJILLA_BURN_NOT_SUPPORTED;
	}

	/* Here flags don't matter as we don't record anything.
	 * Even the IOFlags since that can be checked later with
	 * rejilla_burn_caps_get_flags. */
	if (rejilla_track_type_get_has_medium (output))
		ctx->media = rejilla_track_type_get_medium_type (output);
	else
		ctx->media = REJILLA_MEDIUM_FILE;

	return rejilla_caps_find_link (caps, ctx);
}

static RejillaBurnResult
rejilla_caps_try_output_with_blanking (RejillaBurnCaps *self,
                                       RejillaBurnSession *session,
                                       RejillaFindLinkCtx *ctx,
                                       RejillaTrackType *output)
{
	gboolean result;
	RejillaCaps *last_caps;

	result = rejilla_caps_try_output (self, ctx, output);
	if (result == REJILLA_BURN_OK
	||  result == REJILLA_BURN_CANCEL)
		return result;

	/* we reached this point in two cases:
	 * - if the disc cannot be handled
	 * - if some flags are not handled
	 * It is helpful only if:
	 * - the disc was closed and no plugin can handle this type of 
	 * disc once closed (CD-R(W))
	 * - there was the flag BLANK_BEFORE_WRITE set and no plugin can
	 * handle this flag (means that the plugin should erase and
	 * then write on its own. Basically that works only with
	 * overwrite formatted discs, DVD+RW, ...) */
	if (!rejilla_track_type_get_has_medium (output))
		return REJILLA_BURN_NOT_SUPPORTED;

	/* output is a disc try with initial blanking */
	REJILLA_BURN_LOG ("Support for input/output failed.");

	/* apparently nothing can be done to reach our goal. Maybe that
	 * is because we first have to blank the disc. If so add a blank 
	 * task to the others as a first step */
	if ((ctx->check_session_flags
	&& !(ctx->session_flags & REJILLA_BURN_FLAG_BLANK_BEFORE_WRITE))
	||   rejilla_burn_session_can_blank (session) != REJILLA_BURN_OK)
		return REJILLA_BURN_NOT_SUPPORTED;

	REJILLA_BURN_LOG ("Trying with initial blanking");

	/* retry with the same disc type but blank this time */
	ctx->media = rejilla_track_type_get_medium_type (output);
	ctx->media &= ~(REJILLA_MEDIUM_CLOSED|
	                REJILLA_MEDIUM_APPENDABLE|
	                REJILLA_MEDIUM_UNFORMATTED|
	                REJILLA_MEDIUM_HAS_DATA|
	                REJILLA_MEDIUM_HAS_AUDIO);
	ctx->media |= REJILLA_MEDIUM_BLANK;
	rejilla_track_type_set_medium_type (output, ctx->media);

	last_caps = rejilla_burn_caps_find_start_caps (self, output);
	if (!last_caps)
		return REJILLA_BURN_NOT_SUPPORTED;

	return rejilla_caps_find_link (last_caps, ctx);
}

/**
 * rejilla_burn_session_input_supported:
 * @session: a #RejillaBurnSession
 * @input: a #RejillaTrackType
 * @check_flags: a #gboolean
 *
 * Given the various parameters stored in @session, this
 * function checks whether a session with the data type
 * @type could be burnt to the medium in the #RejillaDrive (set 
 * through rejilla_burn_session_set_burner ()).
 * If @check_flags is %TRUE, then flags are taken into account
 * and are not if it is %FALSE.
 *
 * Return value: a #RejillaBurnResult.
 * REJILLA_BURN_OK if it is possible.
 * REJILLA_BURN_ERR otherwise.
 **/

RejillaBurnResult
rejilla_burn_session_input_supported (RejillaBurnSession *session,
				      RejillaTrackType *input,
                                      gboolean check_flags)
{
	RejillaBurnCaps *self;
	RejillaBurnResult result;
	RejillaTrackType output;
	RejillaFindLinkCtx ctx = { 0, NULL, 0, };

	result = rejilla_burn_session_get_output_type (session, &output);
	if (result != REJILLA_BURN_OK)
		return result;

	REJILLA_BURN_LOG_TYPE (input, "Checking support for input");
	REJILLA_BURN_LOG_TYPE (&output, "and output");

	ctx.check_session_flags = check_flags;
	rejilla_caps_find_link_set_ctx (session, &ctx, input);

	if (check_flags) {
		result = rejilla_check_flags_for_drive (rejilla_burn_session_get_burner (session),
							ctx.session_flags);

		if (!result)
			REJILLA_BURN_CAPS_NOT_SUPPORTED_LOG_RES (session);

		REJILLA_BURN_LOG_FLAGS (ctx.session_flags, "with flags");
	}

	self = rejilla_burn_caps_get_default ();
	result = rejilla_caps_try_output_with_blanking (self,
							session,
	                                                &ctx,
							&output);
	g_object_unref (self);

	if (result != REJILLA_BURN_OK) {
		REJILLA_BURN_LOG_TYPE (input, "Input not supported");
		return result;
	}

	return REJILLA_BURN_OK;
}

/**
 * rejilla_burn_session_output_supported:
 * @session: a #RejillaBurnSession *
 * @output: a #RejillaTrackType *
 *
 * Make sure that the image type or medium type defined in @output can be
 * created/burnt given the parameters and the current data set in @session.
 *
 * Return value: REJILLA_BURN_OK if the medium type or the image type can be used as an output.
 **/
RejillaBurnResult
rejilla_burn_session_output_supported (RejillaBurnSession *session,
				       RejillaTrackType *output)
{
	RejillaBurnCaps *self;
	RejillaTrackType input;
	RejillaBurnResult result;
	RejillaFindLinkCtx ctx = { 0, NULL, 0, };

	/* Here, we can't check if the drive supports the flags since the output
	 * is hypothetical. There is no real medium. So forget the following :
	 * if (!rejilla_burn_caps_flags_check_for_drive (session))
	 *	REJILLA_BURN_CAPS_NOT_SUPPORTED_LOG_RES (session);
	 * The only thing we could do would be to check some known forbidden 
	 * flags for some media provided the output type is DISC. */

	rejilla_burn_session_get_input_type (session, &input);
	rejilla_caps_find_link_set_ctx (session, &ctx, &input);

	REJILLA_BURN_LOG_TYPE (output, "Checking support for output");
	REJILLA_BURN_LOG_TYPE (&input, "and input");
	REJILLA_BURN_LOG_FLAGS (rejilla_burn_session_get_flags (session), "with flags");
	
	self = rejilla_burn_caps_get_default ();
	result = rejilla_caps_try_output_with_blanking (self,
							session,
	                                                &ctx,
							output);
	g_object_unref (self);

	if (result != REJILLA_BURN_OK) {
		REJILLA_BURN_LOG_TYPE (output, "Output not supported");
		return result;
	}

	return REJILLA_BURN_OK;
}

/**
 * This is only to be used in case one wants to copy using the same drive.
 * It determines the possible middle image type.
 */

static RejillaBurnResult
rejilla_burn_caps_is_session_supported_same_src_dest (RejillaBurnCaps *self,
						      RejillaBurnSession *session,
                                                      RejillaFindLinkCtx *ctx,
                                                      RejillaTrackType *tmp_type)
{
	GSList *iter;
	RejillaDrive *burner;
	RejillaTrackType input;
	RejillaBurnResult result;
	RejillaTrackType output;
	RejillaImageFormat format;

	REJILLA_BURN_LOG ("Checking disc copy support with same source and destination");

	/* To determine if a CD/DVD can be copied using the same source/dest,
	 * we first determine if can be imaged and then if this image can be 
	 * burnt to whatever medium type. */
	rejilla_caps_find_link_set_ctx (session, ctx, &input);
	ctx->io_flags = REJILLA_PLUGIN_IO_ACCEPT_FILE;

	memset (&input, 0, sizeof (RejillaTrackType));
	rejilla_burn_session_get_input_type (session, &input);
	REJILLA_BURN_LOG_TYPE (&input, "input");

	if (ctx->check_session_flags) {
		/* NOTE: DAO can be a problem. So just in case remove it. It is
		 * not really useful in this context. What we want here is to
		 * know whether a medium can be used given the input; only 1
		 * flag is important here (MERGE) and can have consequences. */
		ctx->session_flags &= ~REJILLA_BURN_FLAG_DAO;
		REJILLA_BURN_LOG_FLAGS (ctx->session_flags, "flags");
	}

	burner = rejilla_burn_session_get_burner (session);

	/* First see if it works with a stream type */
	rejilla_track_type_set_has_stream (&output);

	/* FIXME! */
	rejilla_track_type_set_stream_format (&output,
	                                      REJILLA_AUDIO_FORMAT_RAW|
	                                      REJILLA_METADATA_INFO);

	REJILLA_BURN_LOG_TYPE (&output, "Testing stream type");
	result = rejilla_caps_try_output (self, ctx, &output);
	if (result == REJILLA_BURN_CANCEL)
		return result;

	if (result == REJILLA_BURN_OK) {
		REJILLA_BURN_LOG ("Stream type seems to be supported as output");

		/* This format can be used to create an image. Check if can be
		 * burnt now. Just find at least one medium. */
		for (iter = self->priv->caps_list; iter; iter = iter->next) {
			RejillaBurnResult result;
			RejillaMedia media;
			RejillaCaps *caps;

			caps = iter->data;

			if (!rejilla_track_type_get_has_medium (&caps->type))
				continue;

			media = rejilla_track_type_get_medium_type (&caps->type);
			/* Audio is only supported by CDs */
			if ((media & REJILLA_MEDIUM_CD) == 0)
				continue;

			/* This type of disc cannot be burnt; skip them */
			if (media & REJILLA_MEDIUM_ROM)
				continue;

			/* Make sure this is supported by the drive */
			if (!rejilla_drive_can_write_media (burner, media))
				continue;

			ctx->media = media;
			result = rejilla_caps_find_link (caps, ctx);
			REJILLA_BURN_LOG_DISC_TYPE (media,
						    "Tested medium (%s)",
						    result == REJILLA_BURN_OK ? "working":"not working");

			if (result == REJILLA_BURN_OK) {
				if (tmp_type) {
					rejilla_track_type_set_has_stream (tmp_type);
					rejilla_track_type_set_stream_format (tmp_type, rejilla_track_type_get_stream_format (&output));
				}
					
				return REJILLA_BURN_OK;
			}

			if (result == REJILLA_BURN_CANCEL)
				return result;
		}
	}
	else
		REJILLA_BURN_LOG ("Stream format not supported as output");

	/* Find one available output format */
	format = REJILLA_IMAGE_FORMAT_CDRDAO;
	rejilla_track_type_set_has_image (&output);

	for (; format > REJILLA_IMAGE_FORMAT_NONE; format >>= 1) {
		rejilla_track_type_set_image_format (&output, format);

		REJILLA_BURN_LOG_TYPE (&output, "Testing temporary image format");

		/* Don't need to try blanking here (saves
		 * a few lines of debug) since that is an 
		 * image */
		result = rejilla_caps_try_output (self, ctx, &output);
		if (result == REJILLA_BURN_CANCEL)
			return result;

		if (result != REJILLA_BURN_OK)
			continue;

		/* This format can be used to create an image. Check if can be
		 * burnt now. Just find at least one medium. */
		for (iter = self->priv->caps_list; iter; iter = iter->next) {
			RejillaBurnResult result;
			RejillaMedia media;
			RejillaCaps *caps;

			caps = iter->data;

			if (!rejilla_track_type_get_has_medium (&caps->type))
				continue;

			media = rejilla_track_type_get_medium_type (&caps->type);

			/* This type of disc cannot be burnt; skip them */
			if (media & REJILLA_MEDIUM_ROM)
				continue;

			/* These three types only work with CDs. Skip the rest. */
			if ((format == REJILLA_IMAGE_FORMAT_CDRDAO
			||   format == REJILLA_IMAGE_FORMAT_CLONE
			||   format == REJILLA_IMAGE_FORMAT_CUE)
			&& (media & REJILLA_MEDIUM_CD) == 0)
				continue;

			/* Make sure this is supported by the drive */
			if (!rejilla_drive_can_write_media (burner, media))
				continue;

			ctx->media = media;
			result = rejilla_caps_find_link (caps, ctx);
			REJILLA_BURN_LOG_DISC_TYPE (media,
						    "Tested medium (%s)",
						    result == REJILLA_BURN_OK ? "working":"not working");

			if (result == REJILLA_BURN_OK) {
				if (tmp_type) {
					rejilla_track_type_set_has_image (tmp_type);
					rejilla_track_type_set_image_format (tmp_type, rejilla_track_type_get_image_format (&output));
				}
					
				return REJILLA_BURN_OK;
			}

			if (result == REJILLA_BURN_CANCEL)
				return result;
		}
	}

	return REJILLA_BURN_NOT_SUPPORTED;
}

RejillaBurnResult
rejilla_burn_session_get_tmp_image_type_same_src_dest (RejillaBurnSession *session,
                                                       RejillaTrackType *image_type)
{
	RejillaBurnCaps *self;
	RejillaBurnResult result;
	RejillaFindLinkCtx ctx = { 0, NULL, 0, };

	g_return_val_if_fail (REJILLA_IS_BURN_SESSION (session), REJILLA_BURN_ERR);

	self = rejilla_burn_caps_get_default ();
	result = rejilla_burn_caps_is_session_supported_same_src_dest (self,
	                                                               session,
	                                                               &ctx,
	                                                               image_type);
	g_object_unref (self);
	return result;
}

static RejillaBurnResult
rejilla_burn_session_supported (RejillaBurnSession *session,
                                RejillaFindLinkCtx *ctx)
{
	gboolean result;
	RejillaBurnCaps *self;
	RejillaTrackType input;
	RejillaTrackType output;

	/* Special case */
	if (rejilla_burn_session_same_src_dest_drive (session)) {
		RejillaBurnResult res;

		self = rejilla_burn_caps_get_default ();
		res = rejilla_burn_caps_is_session_supported_same_src_dest (self, session, ctx, NULL);
		g_object_unref (self);

		return res;
	}

	result = rejilla_burn_session_get_output_type (session, &output);
	if (result != REJILLA_BURN_OK)
		REJILLA_BURN_CAPS_NOT_SUPPORTED_LOG_RES (session);

	rejilla_burn_session_get_input_type (session, &input);
	rejilla_caps_find_link_set_ctx (session, ctx, &input);

	REJILLA_BURN_LOG_TYPE (&output, "Checking support for session. Ouput is ");
	REJILLA_BURN_LOG_TYPE (&input, "and input is");

	if (ctx->check_session_flags) {
		result = rejilla_check_flags_for_drive (rejilla_burn_session_get_burner (session), ctx->session_flags);
		if (!result)
			REJILLA_BURN_CAPS_NOT_SUPPORTED_LOG_RES (session);

		REJILLA_BURN_LOG_FLAGS (ctx->session_flags, "with flags");
	}

	self = rejilla_burn_caps_get_default ();
	result = rejilla_caps_try_output_with_blanking (self,
							session,
	                                                ctx,
							&output);
	g_object_unref (self);

	if (result != REJILLA_BURN_OK) {
		REJILLA_BURN_LOG_TYPE (&output, "Session not supported");
		return result;
	}

	REJILLA_BURN_LOG_TYPE (&output, "Session supported");
	return REJILLA_BURN_OK;
}

/**
 * rejilla_burn_session_can_burn:
 * @session: a #RejillaBurnSession
 * @check_flags: a #gboolean
 *
 * Given the various parameters stored in @session, this
 * function checks whether the data contained in @session
 * can be burnt to the medium in the #RejillaDrive (set 
 * through rejilla_burn_session_set_burner ()).
 * If @check_flags determine the behavior of this function.
 *
 * Return value: a #RejillaBurnResult.
 * REJILLA_BURN_OK if it is possible.
 * REJILLA_BURN_ERR otherwise.
 **/

RejillaBurnResult
rejilla_burn_session_can_burn (RejillaBurnSession *session,
			       gboolean check_flags)
{
	RejillaFindLinkCtx ctx = { 0, NULL, 0, };

	g_return_val_if_fail (REJILLA_IS_BURN_SESSION (session), REJILLA_BURN_ERR);

	ctx.check_session_flags = check_flags;

	return rejilla_burn_session_supported (session, &ctx);
}

/**
 * rejilla_session_foreach_plugin_error:
 * @session: a #RejillaBurnSession.
 * @callback: a #RejillaSessionPluginErrorCb.
 * @user_data: a #gpointer. The data passed to @callback.
 *
 * Call @callback for each error in plugins.
 *
 * Return value: a #RejillaBurnResult.
 * REJILLA_BURN_OK if it is possible.
 * REJILLA_BURN_ERR otherwise.
 **/

RejillaBurnResult
rejilla_session_foreach_plugin_error (RejillaBurnSession *session,
                                      RejillaForeachPluginErrorCb callback,
                                      gpointer user_data)
{
	RejillaFindLinkCtx ctx = { 0, NULL, 0, };

	g_return_val_if_fail (REJILLA_IS_BURN_SESSION (session), REJILLA_BURN_ERR);

	ctx.callback = callback;
	ctx.user_data = user_data;
	
	return rejilla_burn_session_supported (session, &ctx);
}

/**
 * rejilla_burn_session_get_required_media_type:
 * @session: a #RejillaBurnSession
 *
 * Return the medium types that could be used to burn
 * @session.
 *
 * Return value: a #RejillaMedia
 **/

RejillaMedia
rejilla_burn_session_get_required_media_type (RejillaBurnSession *session)
{
	RejillaMedia required_media = REJILLA_MEDIUM_NONE;
	RejillaFindLinkCtx ctx = { 0, NULL, 0, };
	RejillaTrackType input;
	RejillaBurnCaps *self;
	GSList *iter;

	if (rejilla_burn_session_is_dest_file (session))
		return REJILLA_MEDIUM_FILE;

	/* we try to determine here what type of medium is allowed to be burnt
	 * to whether a CD or a DVD. Appendable, blank are not properties being
	 * determined here. We just want it to be writable in a broad sense. */
	ctx.check_session_flags = TRUE;
	rejilla_burn_session_get_input_type (session, &input);
	rejilla_caps_find_link_set_ctx (session, &ctx, &input);
	REJILLA_BURN_LOG_TYPE (&input, "Determining required media type for input");

	/* NOTE: REJILLA_BURN_FLAG_BLANK_BEFORE_WRITE is a problem here since it
	 * is only used if needed. Likewise DAO can be a problem. So just in
	 * case remove them. They are not really useful in this context. What we
	 * want here is to know which media can be used given the input; only 1
	 * flag is important here (MERGE) and can have consequences. */
	ctx.session_flags &= ~REJILLA_BURN_FLAG_DAO;
	REJILLA_BURN_LOG_FLAGS (ctx.session_flags, "and flags");

	self = rejilla_burn_caps_get_default ();
	for (iter = self->priv->caps_list; iter; iter = iter->next) {
		RejillaCaps *caps;
		gboolean result;

		caps = iter->data;

		if (!rejilla_track_type_get_has_medium (&caps->type))
			continue;

		/* Put REJILLA_MEDIUM_NONE so we can always succeed */
		result = rejilla_caps_find_link (caps, &ctx);
		REJILLA_BURN_LOG_DISC_TYPE (caps->type.subtype.media,
					    "Tested (%s)",
					    result == REJILLA_BURN_OK ? "working":"not working");

		if (result == REJILLA_BURN_CANCEL) {
			g_object_unref (self);
			return result;
		}

		if (result != REJILLA_BURN_OK)
			continue;

		/* This caps work, add its subtype */
		required_media |= rejilla_track_type_get_medium_type (&caps->type);
	}

	/* filter as we are only interested in these */
	required_media &= REJILLA_MEDIUM_WRITABLE|
			  REJILLA_MEDIUM_CD|
			  REJILLA_MEDIUM_DVD;

	g_object_unref (self);
	return required_media;
}

/**
 * rejilla_burn_session_get_possible_output_formats:
 * @session: a #RejillaBurnSession
 * @formats: a #RejillaImageFormat
 *
 * Returns the disc image types that could be set to create
 * an image given the current state of @session.
 *
 * Return value: a #guint. The number of formats available.
 **/

guint
rejilla_burn_session_get_possible_output_formats (RejillaBurnSession *session,
						  RejillaImageFormat *formats)
{
	guint num = 0;
	RejillaImageFormat format;
	RejillaTrackType *output = NULL;

	g_return_val_if_fail (REJILLA_IS_BURN_SESSION (session), 0);

	/* see how many output format are available */
	format = REJILLA_IMAGE_FORMAT_CDRDAO;
	(*formats) = REJILLA_IMAGE_FORMAT_NONE;

	output = rejilla_track_type_new ();
	rejilla_track_type_set_has_image (output);

	for (; format > REJILLA_IMAGE_FORMAT_NONE; format >>= 1) {
		RejillaBurnResult result;

		rejilla_track_type_set_image_format (output, format);
		result = rejilla_burn_session_output_supported (session, output);
		if (result == REJILLA_BURN_OK) {
			(*formats) |= format;
			num ++;
		}
	}

	rejilla_track_type_free (output);

	return num;
}

/**
 * rejilla_burn_session_get_default_output_format:
 * @session: a #RejillaBurnSession
 *
 * Returns the default disc image type that should be set to create
 * an image given the current state of @session.
 *
 * Return value: a #RejillaImageFormat
 **/

RejillaImageFormat
rejilla_burn_session_get_default_output_format (RejillaBurnSession *session)
{
	RejillaBurnCaps *self;
	RejillaBurnResult result;
	RejillaTrackType source = { REJILLA_TRACK_TYPE_NONE, { 0, }};
	RejillaTrackType output = { REJILLA_TRACK_TYPE_NONE, { 0, }};

	self = rejilla_burn_caps_get_default ();

	if (!rejilla_burn_session_is_dest_file (session)) {
		g_object_unref (self);
		return REJILLA_IMAGE_FORMAT_NONE;
	}

	rejilla_burn_session_get_input_type (session, &source);
	if (rejilla_track_type_is_empty (&source)) {
		g_object_unref (self);
		return REJILLA_IMAGE_FORMAT_NONE;
	}

	if (rejilla_track_type_get_has_image (&source)) {
		g_object_unref (self);
		return rejilla_track_type_get_image_format (&source);
	}

	rejilla_track_type_set_has_image (&output);
	rejilla_track_type_set_image_format (&output, REJILLA_IMAGE_FORMAT_NONE);

	if (rejilla_track_type_get_has_stream (&source)) {
		/* Otherwise try all possible image types */
		output.subtype.img_format = REJILLA_IMAGE_FORMAT_CDRDAO;
		for (; output.subtype.img_format != REJILLA_IMAGE_FORMAT_NONE;
		       output.subtype.img_format >>= 1) {
		
			result = rejilla_burn_session_output_supported (session,
									&output);
			if (result == REJILLA_BURN_OK) {
				g_object_unref (self);
				return rejilla_track_type_get_image_format (&output);
			}
		}

		g_object_unref (self);
		return REJILLA_IMAGE_FORMAT_NONE;
	}

	if (rejilla_track_type_get_has_data (&source)
	|| (rejilla_track_type_get_has_medium (&source)
	&& (rejilla_track_type_get_medium_type (&source) & REJILLA_MEDIUM_DVD))) {
		rejilla_track_type_set_image_format (&output, REJILLA_IMAGE_FORMAT_BIN);
		result = rejilla_burn_session_output_supported (session, &output);
		g_object_unref (self);

		if (result != REJILLA_BURN_OK)
			return REJILLA_IMAGE_FORMAT_NONE;

		return REJILLA_IMAGE_FORMAT_BIN;
	}

	/* for the input which are CDs there are lots of possible formats */
	output.subtype.img_format = REJILLA_IMAGE_FORMAT_CDRDAO;
	for (; output.subtype.img_format != REJILLA_IMAGE_FORMAT_NONE;
	       output.subtype.img_format >>= 1) {
	
		result = rejilla_burn_session_output_supported (session, &output);
		if (result == REJILLA_BURN_OK) {
			g_object_unref (self);
			return rejilla_track_type_get_image_format (&output);
		}
	}

	g_object_unref (self);
	return REJILLA_IMAGE_FORMAT_NONE;
}

static RejillaBurnResult
rejilla_caps_set_flags_from_recorder_input (RejillaTrackType *input,
                                            RejillaBurnFlag *supported,
                                            RejillaBurnFlag *compulsory)
{
	if (rejilla_track_type_get_has_image (input)) {
		RejillaImageFormat format;

		format = rejilla_track_type_get_image_format (input);
		if (format == REJILLA_IMAGE_FORMAT_CUE
		||  format == REJILLA_IMAGE_FORMAT_CDRDAO) {
			if ((*supported) & REJILLA_BURN_FLAG_DAO)
				(*compulsory) |= REJILLA_BURN_FLAG_DAO;
			else
				return REJILLA_BURN_NOT_SUPPORTED;
		}
		else if (format == REJILLA_IMAGE_FORMAT_CLONE) {
			/* RAW write mode should (must) only be used in this case */
			if ((*supported) & REJILLA_BURN_FLAG_RAW) {
				(*supported) &= ~REJILLA_BURN_FLAG_DAO;
				(*compulsory) &= ~REJILLA_BURN_FLAG_DAO;
				(*compulsory) |= REJILLA_BURN_FLAG_RAW;
			}
			else
				return REJILLA_BURN_NOT_SUPPORTED;
		}
		else
			(*supported) &= ~REJILLA_BURN_FLAG_RAW;
	}
	
	return REJILLA_BURN_OK;
}

static RejillaPluginIOFlag
rejilla_caps_get_flags (RejillaCaps *caps,
                        gboolean ignore_plugin_errors,
			RejillaBurnFlag session_flags,
			RejillaMedia media,
			RejillaTrackType *input,
			RejillaPluginIOFlag flags,
			RejillaBurnFlag *supported,
			RejillaBurnFlag *compulsory)
{
	GSList *iter;
	RejillaPluginIOFlag retval = REJILLA_PLUGIN_IO_NONE;

	/* First we must know if this link leads somewhere. It must 
	 * accept the already existing flags. If it does, see if it 
	 * accepts the input and if not, if one of its ancestors does */
	for (iter = caps->links; iter; iter = iter->next) {
		RejillaBurnFlag data_supported = REJILLA_BURN_FLAG_NONE;
		RejillaBurnFlag rec_compulsory = REJILLA_BURN_FLAG_ALL;
		RejillaBurnFlag rec_supported = REJILLA_BURN_FLAG_NONE;
		RejillaPluginIOFlag io_flags;
		RejillaCapsLink *link;

		link = iter->data;

		if (!link->caps)
			continue;

		/* check that the link has some active plugin */
		if (!rejilla_caps_link_active (link, ignore_plugin_errors))
			continue;

		if (rejilla_track_type_get_has_medium (&caps->type)) {
			RejillaBurnFlag tmp;
			RejillaBurnResult result;

			rejilla_caps_link_get_record_flags (link,
			                                    ignore_plugin_errors,
							    media,
							    session_flags,
							    &rec_supported,
							    &rec_compulsory);

			/* see if that link can handle the record flags.
			 * NOTE: compulsory are not a failure in this case. */
			tmp = session_flags & REJILLA_PLUGIN_BURN_FLAG_MASK;
			if ((tmp & rec_supported) != tmp)
				continue;

			/* This is the recording plugin, check its input as
			 * some flags depend on it. */
			result = rejilla_caps_set_flags_from_recorder_input (&link->caps->type,
			                                                     &rec_supported,
			                                                     &rec_compulsory);
			if (result != REJILLA_BURN_OK)
				continue;
		}

		if (rejilla_track_type_get_has_data (&link->caps->type)) {
			RejillaBurnFlag tmp;

			rejilla_caps_link_get_data_flags (link,
			                                  ignore_plugin_errors,
							  media,
							  session_flags,
						    	  &data_supported);

			/* see if that link can handle the data flags. 
			 * NOTE: compulsory are not a failure in this case. */
			tmp = session_flags & (REJILLA_BURN_FLAG_APPEND|
					       REJILLA_BURN_FLAG_MERGE);

			if ((tmp & data_supported) != tmp)
				continue;
		}
		else if (!rejilla_caps_link_check_media_restrictions (link, ignore_plugin_errors, media))
			continue;

		/* see if that's the perfect fit */
		if ((link->caps->flags & REJILLA_PLUGIN_IO_ACCEPT_FILE)
		&&   rejilla_caps_is_compatible_type (link->caps, input)) {
			/* special case for input that handle output/input */
			if (caps->type.type == REJILLA_TRACK_TYPE_DISC)
				retval |= REJILLA_PLUGIN_IO_ACCEPT_PIPE;
			else
				retval |= caps->flags;

			(*compulsory) &= rec_compulsory;
			(*supported) |= data_supported|rec_supported;
			continue;
		}

		if ((link->caps->flags & flags) == REJILLA_PLUGIN_IO_NONE)
			continue;

		/* we can't go further than a DISC type */
		if (link->caps->type.type == REJILLA_TRACK_TYPE_DISC)
			continue;

		/* try to see where the inputs of this caps leads to */
		io_flags = rejilla_caps_get_flags (link->caps,
		                                   ignore_plugin_errors,
						   session_flags,
						   media,
						   input,
						   flags,
						   supported,
						   compulsory);
		if (io_flags == REJILLA_PLUGIN_IO_NONE)
			continue;

		(*compulsory) &= rec_compulsory;
		retval |= (io_flags & flags);
		(*supported) |= data_supported|rec_supported;
	}

	return retval;
}

static void
rejilla_medium_supported_flags (RejillaMedium *medium,
				RejillaBurnFlag *supported_flags,
                                RejillaBurnFlag *compulsory_flags)
{
	RejillaMedia media;

	media = rejilla_medium_get_status (medium);

	/* This is always FALSE */
	if (media & REJILLA_MEDIUM_PLUS)
		(*supported_flags) &= ~REJILLA_BURN_FLAG_DUMMY;

	/* Simulation is only possible according to write modes. This mode is
	 * mostly used by cdrecord/wodim for CLONE images. */
	else if (media & REJILLA_MEDIUM_DVD) {
		if (!rejilla_medium_can_use_dummy_for_sao (medium))
			(*supported_flags) &= ~REJILLA_BURN_FLAG_DUMMY;
	}
	else if ((*supported_flags) & REJILLA_BURN_FLAG_DAO) {
		if (!rejilla_medium_can_use_dummy_for_sao (medium))
			(*supported_flags) &= ~REJILLA_BURN_FLAG_DUMMY;
	}
	else if (!rejilla_medium_can_use_dummy_for_tao (medium))
		(*supported_flags) &= ~REJILLA_BURN_FLAG_DUMMY;

	/* The following is only true if we won't _have_ to blank
	 * the disc since a CLOSED disc is not able for tao/sao.
	 * so if BLANK_BEFORE_RIGHT is TRUE then we leave 
	 * the benefit of the doubt, but flags should be rechecked
	 * after the drive was blanked. */
	if (((*compulsory_flags) & REJILLA_BURN_FLAG_BLANK_BEFORE_WRITE) == 0
	&&  !REJILLA_MEDIUM_RANDOM_WRITABLE (media)
	&&  !rejilla_medium_can_use_tao (medium)) {
		(*supported_flags) &= ~REJILLA_BURN_FLAG_MULTI;

		if (rejilla_medium_can_use_sao (medium))
			(*compulsory_flags) |= REJILLA_BURN_FLAG_DAO;
		else
			(*supported_flags) &= ~REJILLA_BURN_FLAG_DAO;
	}

	if (!rejilla_medium_can_use_burnfree (medium))
		(*supported_flags) &= ~REJILLA_BURN_FLAG_BURNPROOF;
}

static void
rejilla_burn_caps_flags_update_for_drive (RejillaBurnSession *session,
                                          RejillaBurnFlag *supported_flags,
                                          RejillaBurnFlag *compulsory_flags)
{
	RejillaDrive *drive;
	RejillaMedium *medium;

	drive = rejilla_burn_session_get_burner (session);
	if (!drive)
		return;

	medium = rejilla_drive_get_medium (drive);
	if (!medium)
		return;

	rejilla_medium_supported_flags (medium,
	                                supported_flags,
	                                compulsory_flags);
}

static RejillaBurnResult
rejilla_caps_get_flags_for_disc (RejillaBurnCaps *self,
                                 gboolean ignore_plugin_errors,
				 RejillaBurnFlag session_flags,
				 RejillaMedia media,
				 RejillaTrackType *input,
				 RejillaBurnFlag *supported,
				 RejillaBurnFlag *compulsory)
{
	RejillaBurnFlag supported_flags = REJILLA_BURN_FLAG_NONE;
	RejillaBurnFlag compulsory_flags = REJILLA_BURN_FLAG_ALL;
	RejillaPluginIOFlag io_flags;
	RejillaTrackType output;
	RejillaCaps *caps;

	/* create the output to find first caps */
	rejilla_track_type_set_has_medium (&output);
	rejilla_track_type_set_medium_type (&output, media);

	caps = rejilla_burn_caps_find_start_caps (self, &output);
	if (!caps) {
		REJILLA_BURN_LOG_DISC_TYPE (media, "FLAGS: no caps could be found for");
		return REJILLA_BURN_NOT_SUPPORTED;
	}

	REJILLA_BURN_LOG_WITH_TYPE (&caps->type,
				    caps->flags,
				    "FLAGS: trying caps");

	io_flags = rejilla_caps_get_flags (caps,
	                                   ignore_plugin_errors,
					   session_flags,
					   media,
					   input,
					   REJILLA_PLUGIN_IO_ACCEPT_FILE|
					   REJILLA_PLUGIN_IO_ACCEPT_PIPE,
					   &supported_flags,
					   &compulsory_flags);

	if (io_flags == REJILLA_PLUGIN_IO_NONE) {
		REJILLA_BURN_LOG ("FLAGS: not supported");
		return REJILLA_BURN_NOT_SUPPORTED;
	}

	/* NOTE: DO NOT TEST the input image here. What should be tested is the
	 * type of the input right before the burner plugin. See:
	 * rejilla_burn_caps_set_flags_from_recorder_input())
	 * For example in the following situation: AUDIO => CUE => BURNER the
	 * DAO flag would not be set otherwise. */

	if (rejilla_track_type_get_has_stream (input)) {
		RejillaStreamFormat format;

		format = rejilla_track_type_get_stream_format (input);
		if (format & REJILLA_METADATA_INFO) {
			/* In this case, DAO is compulsory if we want to write CD-TEXT */
			if (supported_flags & REJILLA_BURN_FLAG_DAO)
				compulsory_flags |= REJILLA_BURN_FLAG_DAO;
			else
				return REJILLA_BURN_NOT_SUPPORTED;
		}
	}

	if (compulsory_flags & REJILLA_BURN_FLAG_DAO) {
		/* unlikely */
		compulsory_flags &= ~REJILLA_BURN_FLAG_RAW;
		supported_flags &= ~REJILLA_BURN_FLAG_RAW;
	}
	
	if (io_flags & REJILLA_PLUGIN_IO_ACCEPT_PIPE) {
		supported_flags |= REJILLA_BURN_FLAG_NO_TMP_FILES;

		if ((io_flags & REJILLA_PLUGIN_IO_ACCEPT_FILE) == 0)
			compulsory_flags |= REJILLA_BURN_FLAG_NO_TMP_FILES;
	}

	*supported |= supported_flags;
	*compulsory |= compulsory_flags;

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_burn_caps_get_flags_for_medium (RejillaBurnCaps *self,
                                        RejillaBurnSession *session,
					RejillaMedia media,
					RejillaBurnFlag session_flags,
					RejillaTrackType *input,
					RejillaBurnFlag *supported_flags,
					RejillaBurnFlag *compulsory_flags)
{
	RejillaBurnResult result;
	gboolean can_blank = FALSE;

	/* See if medium is supported out of the box */
	result = rejilla_caps_get_flags_for_disc (self,
	                                          rejilla_burn_session_get_strict_support (session) == FALSE,
						  session_flags,
						  media,
						  input,
						  supported_flags,
						  compulsory_flags);

	/* see if we can add REJILLA_BURN_FLAG_BLANK_BEFORE_WRITE. Add it when:
	 * - media can be blanked, it has audio or data and we're not merging
	 * - media is not formatted and it can be blanked/formatted */
	if (rejilla_burn_caps_can_blank_real (self, rejilla_burn_session_get_strict_support (session) == FALSE, media, session_flags) == REJILLA_BURN_OK)
		can_blank = TRUE;
	else if (session_flags & REJILLA_BURN_FLAG_BLANK_BEFORE_WRITE)
		return REJILLA_BURN_NOT_SUPPORTED;

	if (can_blank) {
		gboolean first_success;
		RejillaBurnFlag blank_compulsory = REJILLA_BURN_FLAG_NONE;
		RejillaBurnFlag blank_supported = REJILLA_BURN_FLAG_NONE;

		/* we reached this point in two cases:
		 * - if the disc cannot be handled
		 * - if some flags are not handled
		 * It is helpful only if:
		 * - the disc was closed and no plugin can handle this type of 
		 * disc once closed (CD-R(W))
		 * - there was the flag BLANK_BEFORE_WRITE set and no plugin can
		 * handle this flag (means that the plugin should erase and
		 * then write on its own. Basically that works only with
		 * overwrite formatted discs, DVD+RW, ...) */

		/* What's above is not entirely true. In fact we always need to
		 * check even if we first succeeded. There are some cases like
		 * CDRW where it's useful.
		 * Ex: a CDRW with data appendable can be either appended (then
		 * no DAO possible) or blanked and written (DAO possible). */

		/* result here is the result of the first operation, so if it
		 * failed, BLANK before becomes compulsory. */
		first_success = (result == REJILLA_BURN_OK);

		/* pretends it is blank and formatted to see if it would work.
		 * If it works then that means that the BLANK_BEFORE_WRITE flag
		 * is compulsory. */
		media &= ~(REJILLA_MEDIUM_CLOSED|
			   REJILLA_MEDIUM_APPENDABLE|
			   REJILLA_MEDIUM_UNFORMATTED|
			   REJILLA_MEDIUM_HAS_DATA|
			   REJILLA_MEDIUM_HAS_AUDIO);
		media |= REJILLA_MEDIUM_BLANK;
		result = rejilla_caps_get_flags_for_disc (self,
		                                          rejilla_burn_session_get_strict_support (session) == FALSE,
							  session_flags,
							  media,
							  input,
							  supported_flags,
							  compulsory_flags);

		/* if both attempts failed, drop it */
		if (result != REJILLA_BURN_OK) {
			/* See if we entirely failed */
			if (!first_success)
				return result;

			/* we tried with a blank medium but did not 
			 * succeed. So that means the flags BLANK.
			 * is not supported */
		}
		else {
			(*supported_flags) |= REJILLA_BURN_FLAG_BLANK_BEFORE_WRITE;

			if (!first_success)
				(*compulsory_flags) |= REJILLA_BURN_FLAG_BLANK_BEFORE_WRITE;

			/* If BLANK flag is supported then MERGE/APPEND can't be compulsory */
			(*compulsory_flags) &= ~(REJILLA_BURN_FLAG_MERGE|REJILLA_BURN_FLAG_APPEND);

			/* need to add blanking flags */
			rejilla_burn_caps_get_blanking_flags_real (self,
			                                           rejilla_burn_session_get_strict_support (session) == FALSE,
								   media,
								   session_flags,
								   &blank_supported,
								   &blank_compulsory);
			(*supported_flags) |= blank_supported;
			(*compulsory_flags) |= blank_compulsory;
		}
		
	}
	else if (result != REJILLA_BURN_OK)
		return result;

	/* These are a special case for DVDRW sequential */
	if (REJILLA_MEDIUM_IS (media, REJILLA_MEDIUM_DVDRW)) {
		/* That's a way to give priority to MULTI over FAST
		 * and leave the possibility to always use MULTI. */
		if (session_flags & REJILLA_BURN_FLAG_MULTI)
			(*supported_flags) &= ~REJILLA_BURN_FLAG_FAST_BLANK;
		else if ((session_flags & REJILLA_BURN_FLAG_FAST_BLANK)
		         &&  (session_flags & REJILLA_BURN_FLAG_BLANK_BEFORE_WRITE)) {
			/* We should be able to handle this case differently but unfortunately
			 * there are buggy firmwares that won't report properly the supported
			 * mode writes */
			if (!((*supported_flags) & REJILLA_BURN_FLAG_DAO))
					 return REJILLA_BURN_NOT_SUPPORTED;

			(*compulsory_flags) |= REJILLA_BURN_FLAG_DAO;
		}
	}

	if (session_flags & REJILLA_BURN_FLAG_BLANK_BEFORE_WRITE) {
		/* make sure we remove MERGE/APPEND from supported and
		 * compulsory since that's not possible anymore */
		(*supported_flags) &= ~(REJILLA_BURN_FLAG_MERGE|REJILLA_BURN_FLAG_APPEND);
		(*compulsory_flags) &= ~(REJILLA_BURN_FLAG_MERGE|REJILLA_BURN_FLAG_APPEND);
	}

	/* FIXME! we should restart the whole process if
	 * ((session_flags & compulsory_flags) != compulsory_flags) since that
	 * means that some supported files could be excluded but were not */

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_burn_caps_get_flags_same_src_dest_for_types (RejillaBurnCaps *self,
                                                     RejillaBurnSession *session,
                                                     RejillaTrackType *input,
                                                     RejillaTrackType *output,
                                                     RejillaBurnFlag *supported_ret,
                                                     RejillaBurnFlag *compulsory_ret)
{
	GSList *iter;
	gboolean type_supported;
	RejillaBurnResult result;
	RejillaBurnFlag session_flags;
	RejillaFindLinkCtx ctx = { 0, NULL, 0, };
	RejillaBurnFlag supported_final = REJILLA_BURN_FLAG_NONE;
	RejillaBurnFlag compulsory_final = REJILLA_BURN_FLAG_ALL;

	/* NOTE: there is no need to get the flags here since there are
	 * no specific DISC => IMAGE flags. We just want to know if that
	 * is possible. */
	REJILLA_BURN_LOG_TYPE (output, "Testing temporary image format");

	rejilla_caps_find_link_set_ctx (session, &ctx, input);
	ctx.io_flags = REJILLA_PLUGIN_IO_ACCEPT_FILE;

	/* Here there is no need to try blanking as there
	 * is no disc (saves a few debug lines) */
	result = rejilla_caps_try_output (self, &ctx, output);
	if (result != REJILLA_BURN_OK) {
		REJILLA_BURN_LOG_TYPE (output, "Format not supported");
		return result;
	}

	session_flags = rejilla_burn_session_get_flags (session);

	/* This format can be used to create an image. Check if can be
	 * burnt now. Just find at least one medium. */
	type_supported = FALSE;
	for (iter = self->priv->caps_list; iter; iter = iter->next) {
		RejillaBurnFlag compulsory;
		RejillaBurnFlag supported;
		RejillaBurnResult result;
		RejillaMedia media;
		RejillaCaps *caps;

		caps = iter->data;
		if (!rejilla_track_type_get_has_medium (&caps->type))
			continue;

		media = rejilla_track_type_get_medium_type (&caps->type);

		/* This type of disc cannot be burnt; skip them */
		if (media & REJILLA_MEDIUM_ROM)
			continue;

		if ((media & REJILLA_MEDIUM_CD) == 0) {
			if (rejilla_track_type_get_has_image (output)) {
				RejillaImageFormat format;

				format = rejilla_track_type_get_image_format (output);
				/* These three types only work with CDs. */
				if (format == REJILLA_IMAGE_FORMAT_CDRDAO
				||   format == REJILLA_IMAGE_FORMAT_CLONE
				||   format == REJILLA_IMAGE_FORMAT_CUE)
					continue;
			}
			else if (rejilla_track_type_get_has_stream (output))
				continue;
		}

		/* Merge all available flags for each possible medium type */
		supported = REJILLA_BURN_FLAG_NONE;
		compulsory = REJILLA_BURN_FLAG_NONE;

		result = rejilla_caps_get_flags_for_disc (self,
		                                          rejilla_burn_session_get_strict_support (session) == FALSE,
		                                          session_flags,
		                                          media,
							  output,
							  &supported,
							  &compulsory);

		if (result != REJILLA_BURN_OK)
			continue;

		type_supported = TRUE;
		supported_final |= supported;
		compulsory_final &= compulsory;
	}

	REJILLA_BURN_LOG_TYPE (output, "Format supported %i", type_supported);
	if (!type_supported)
		return REJILLA_BURN_NOT_SUPPORTED;

	*supported_ret = supported_final;
	*compulsory_ret = compulsory_final;
	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_burn_caps_get_flags_same_src_dest (RejillaBurnCaps *self,
					   RejillaBurnSession *session,
					   RejillaBurnFlag *supported_ret,
					   RejillaBurnFlag *compulsory_ret)
{
	RejillaTrackType input;
	RejillaBurnResult result;
	gboolean copy_supported;
	RejillaTrackType output;
	RejillaImageFormat format;
	RejillaBurnFlag session_flags;
	RejillaBurnFlag supported_final = REJILLA_BURN_FLAG_NONE;
	RejillaBurnFlag compulsory_final = REJILLA_BURN_FLAG_ALL;

	REJILLA_BURN_LOG ("Retrieving disc copy flags with same source and destination");

	/* To determine if a CD/DVD can be copied using the same source/dest,
	 * we first determine if can be imaged and then what are the flags when
	 * we can burn it to a particular medium type. */
	memset (&input, 0, sizeof (RejillaTrackType));
	rejilla_burn_session_get_input_type (session, &input);
	REJILLA_BURN_LOG_TYPE (&input, "input");

	session_flags = rejilla_burn_session_get_flags (session);
	REJILLA_BURN_LOG_FLAGS (session_flags, "(FLAGS) Session flags");

	/* Check the current flags are possible */
	if (session_flags & (REJILLA_BURN_FLAG_MERGE|REJILLA_BURN_FLAG_NO_TMP_FILES))
		return REJILLA_BURN_NOT_SUPPORTED;

	/* Check for stream type */
	rejilla_track_type_set_has_stream (&output);
	/* FIXME! */
	rejilla_track_type_set_stream_format (&output,
	                                      REJILLA_AUDIO_FORMAT_RAW|
	                                      REJILLA_METADATA_INFO);

	result = rejilla_burn_caps_get_flags_same_src_dest_for_types (self,
	                                                              session,
	                                                              &input,
	                                                              &output,
	                                                              &supported_final,
	                                                              &compulsory_final);
	if (result == REJILLA_BURN_CANCEL)
		return result;

	copy_supported = (result == REJILLA_BURN_OK);

	/* Check flags for all available format */
	format = REJILLA_IMAGE_FORMAT_CDRDAO;
	rejilla_track_type_set_has_image (&output);
	for (; format > REJILLA_IMAGE_FORMAT_NONE; format >>= 1) {
		RejillaBurnFlag supported;
		RejillaBurnFlag compulsory;

		/* check if this image type is possible given the current flags */
		if (format != REJILLA_IMAGE_FORMAT_CLONE
		&& (session_flags & REJILLA_BURN_FLAG_RAW))
			continue;

		rejilla_track_type_set_image_format (&output, format);

		supported = REJILLA_BURN_FLAG_NONE;
		compulsory = REJILLA_BURN_FLAG_NONE;
		result = rejilla_burn_caps_get_flags_same_src_dest_for_types (self,
		                                                              session,
		                                                              &input,
		                                                              &output,
		                                                              &supported,
		                                                              &compulsory);
		if (result == REJILLA_BURN_CANCEL)
			return result;

		if (result != REJILLA_BURN_OK)
			continue;

		copy_supported = TRUE;
		supported_final |= supported;
		compulsory_final &= compulsory;
	}

	if (!copy_supported)
		return REJILLA_BURN_NOT_SUPPORTED;

	*supported_ret |= supported_final;
	*compulsory_ret |= compulsory_final;

	/* we also add these two flags as being supported
	 * since they could be useful and can't be tested
	 * until the disc is inserted which it is not at the
	 * moment */
	(*supported_ret) |= (REJILLA_BURN_FLAG_BLANK_BEFORE_WRITE|
			     REJILLA_BURN_FLAG_FAST_BLANK);

	if (rejilla_track_type_get_medium_type (&input) & REJILLA_MEDIUM_HAS_AUDIO) {
		/* This is a special case for audio discs.
		 * Since they may contain CD-TEXT and
		 * since CD-TEXT can only be written with
		 * DAO then we must make sure the user
		 * can't use MULTI since then DAO is
		 * impossible. */
		(*compulsory_ret) |= REJILLA_BURN_FLAG_DAO;

		/* This is just to make sure */
		(*supported_ret) &= (~REJILLA_BURN_FLAG_MULTI);
		(*compulsory_ret) &= (~REJILLA_BURN_FLAG_MULTI);
	}

	return REJILLA_BURN_OK;
}

/**
 * This is meant to use as internal API
 */
RejillaBurnResult
rejilla_caps_session_get_image_flags (RejillaTrackType *input,
                                     RejillaTrackType *output,
                                     RejillaBurnFlag *supported,
                                     RejillaBurnFlag *compulsory)
{
	RejillaBurnFlag compulsory_flags = REJILLA_BURN_FLAG_NONE;
	RejillaBurnFlag supported_flags = REJILLA_BURN_FLAG_CHECK_SIZE|REJILLA_BURN_FLAG_NOGRACE;

	REJILLA_BURN_LOG ("FLAGS: image required");

	/* In this case no APPEND/MERGE is possible */
	if (rejilla_track_type_get_has_medium (input))
		supported_flags |= REJILLA_BURN_FLAG_EJECT;

	*supported = supported_flags;
	*compulsory = compulsory_flags;

	REJILLA_BURN_LOG_FLAGS (supported_flags, "FLAGS: supported");
	REJILLA_BURN_LOG_FLAGS (compulsory_flags, "FLAGS: compulsory");

	return REJILLA_BURN_OK;
}

/**
 * rejilla_burn_session_get_burn_flags:
 * @session: a #RejillaBurnSession
 * @supported: a #RejillaBurnFlag or NULL
 * @compulsory: a #RejillaBurnFlag or NULL
 *
 * Given the various parameters stored in @session, this function
 * stores:
 * - the flags that can be used (@supported)
 * - the flags that must be used (@compulsory)
 * when writing @session to a disc.
 *
 * Return value: a #RejillaBurnResult.
 * REJILLA_BURN_OK if the retrieval was successful.
 * REJILLA_BURN_ERR otherwise.
 **/

RejillaBurnResult
rejilla_burn_session_get_burn_flags (RejillaBurnSession *session,
				     RejillaBurnFlag *supported,
				     RejillaBurnFlag *compulsory)
{
	gboolean res;
	RejillaMedia media;
	RejillaBurnCaps *self;
	RejillaTrackType *input;
	RejillaBurnResult result;

	RejillaBurnFlag session_flags;
	/* FIXME: what's the meaning of NOGRACE when outputting ? */
	RejillaBurnFlag compulsory_flags = REJILLA_BURN_FLAG_NONE;
	RejillaBurnFlag supported_flags = REJILLA_BURN_FLAG_CHECK_SIZE|
					  REJILLA_BURN_FLAG_NOGRACE;

	self = rejilla_burn_caps_get_default ();

	input = rejilla_track_type_new ();
	rejilla_burn_session_get_input_type (session, input);
	REJILLA_BURN_LOG_WITH_TYPE (input,
				    REJILLA_PLUGIN_IO_NONE,
				    "FLAGS: searching available flags for input");

	if (rejilla_burn_session_is_dest_file (session)) {
		RejillaTrackType *output;

		REJILLA_BURN_LOG ("FLAGS: image required");

		output = rejilla_track_type_new ();
		rejilla_burn_session_get_output_type (session, output);
		rejilla_caps_session_get_image_flags (input, output, supported, compulsory);
		rejilla_track_type_free (output);

		rejilla_track_type_free (input);
		g_object_unref (self);
		return REJILLA_BURN_OK;
	}

	supported_flags |= REJILLA_BURN_FLAG_EJECT;

	/* special case */
	if (rejilla_burn_session_same_src_dest_drive (session)) {
		REJILLA_BURN_LOG ("Same source and destination");
		result = rejilla_burn_caps_get_flags_same_src_dest (self,
								    session,
								    &supported_flags,
								    &compulsory_flags);

		/* These flags are of course never possible */
		supported_flags &= ~(REJILLA_BURN_FLAG_NO_TMP_FILES|
				     REJILLA_BURN_FLAG_MERGE);
		compulsory_flags &= ~(REJILLA_BURN_FLAG_NO_TMP_FILES|
				      REJILLA_BURN_FLAG_MERGE);

		if (result == REJILLA_BURN_OK) {
			REJILLA_BURN_LOG_FLAGS (supported_flags, "FLAGS: supported");
			REJILLA_BURN_LOG_FLAGS (compulsory_flags, "FLAGS: compulsory");

			*supported = supported_flags;
			*compulsory = compulsory_flags;
		}
		else
			REJILLA_BURN_LOG ("No available flags for copy");

		rejilla_track_type_free (input);
		g_object_unref (self);
		return result;
	}

	session_flags = rejilla_burn_session_get_flags (session);
	REJILLA_BURN_LOG_FLAGS (session_flags, "FLAGS (session):");

	/* sanity check:
	 * - drive must support flags
	 * - MERGE and BLANK are not possible together.
	 * - APPEND and MERGE are compatible. MERGE wins
	 * - APPEND and BLANK are incompatible */
	res = rejilla_check_flags_for_drive (rejilla_burn_session_get_burner (session), session_flags);
	if (!res) {
		REJILLA_BURN_LOG ("Session flags not supported by drive");
		rejilla_track_type_free (input);
		g_object_unref (self);
		return REJILLA_BURN_ERR;
	}

	if ((session_flags & (REJILLA_BURN_FLAG_MERGE|REJILLA_BURN_FLAG_APPEND))
	&&  (session_flags & REJILLA_BURN_FLAG_BLANK_BEFORE_WRITE)) {
		rejilla_track_type_free (input);
		g_object_unref (self);
		return REJILLA_BURN_NOT_SUPPORTED;
	}
	
	/* Let's get flags for recording */
	media = rejilla_burn_session_get_dest_media (session);
	result = rejilla_burn_caps_get_flags_for_medium (self,
	                                                 session,
							 media,
							 session_flags,
							 input,
							 &supported_flags,
							 &compulsory_flags);

	rejilla_track_type_free (input);
	g_object_unref (self);

	if (result != REJILLA_BURN_OK)
		return result;

	rejilla_burn_caps_flags_update_for_drive (session,
	                                          &supported_flags,
	                                          &compulsory_flags);

	if (supported)
		*supported = supported_flags;

	if (compulsory)
		*compulsory = compulsory_flags;

	REJILLA_BURN_LOG_FLAGS (supported_flags, "FLAGS: supported");
	REJILLA_BURN_LOG_FLAGS (compulsory_flags, "FLAGS: compulsory");
	return REJILLA_BURN_OK;
}
