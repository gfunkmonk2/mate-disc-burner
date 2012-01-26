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

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>

#include "rejilla-burn.h"

#include "librejilla-marshal.h"
#include "burn-basics.h"
#include "burn-debug.h"
#include "burn-dbus.h"
#include "burn-task-ctx.h"
#include "burn-task.h"
#include "rejilla-caps-burn.h"

#include "rejilla-drive-priv.h"

#include "rejilla-volume.h"
#include "rejilla-drive.h"

#include "rejilla-tags.h"
#include "rejilla-track.h"
#include "rejilla-session.h"
#include "rejilla-track-image.h"
#include "rejilla-track-disc.h"
#include "rejilla-session-helper.h"

G_DEFINE_TYPE (RejillaBurn, rejilla_burn, G_TYPE_OBJECT);

typedef struct _RejillaBurnPrivate RejillaBurnPrivate;
struct _RejillaBurnPrivate {
	RejillaBurnCaps *caps;
	RejillaBurnSession *session;

	GMainLoop *sleep_loop;
	guint timeout_id;

	guint tasks_done;
	guint task_nb;
	RejillaTask *task;

	RejillaDrive *src;
	RejillaDrive *dest;

	gint appcookie;

	guint64 session_start;
	guint64 session_end;

	guint mounted_by_us:1;
};

#define REJILLA_BURN_NOT_SUPPORTED_LOG(burn)					\
	{									\
		rejilla_burn_log (burn,						\
				  "unsupported operation (in %s at %s)",	\
				  G_STRFUNC,					\
				  G_STRLOC);					\
		return REJILLA_BURN_NOT_SUPPORTED;				\
	}

#define REJILLA_BURN_NOT_READY_LOG(burn)					\
	{									\
		rejilla_burn_log (burn,						\
				  "not ready to operate (in %s at %s)",		\
				  G_STRFUNC,					\
				  G_STRLOC);					\
		return REJILLA_BURN_NOT_READY;					\
	}

#define REJILLA_BURN_DEBUG(burn, message, ...)					\
	{									\
		gchar *format;							\
		REJILLA_BURN_LOG (message, ##__VA_ARGS__);			\
		format = g_strdup_printf ("%s (%s %s)",				\
					  message,				\
					  G_STRFUNC,				\
					  G_STRLOC);				\
		rejilla_burn_log (burn,						\
				  format,					\
				  ##__VA_ARGS__);				\
		g_free (format);						\
	}

typedef enum {
	ASK_DISABLE_JOLIET_SIGNAL,
	WARN_DATA_LOSS_SIGNAL,
	WARN_PREVIOUS_SESSION_LOSS_SIGNAL,
	WARN_AUDIO_TO_APPENDABLE_SIGNAL,
	WARN_REWRITABLE_SIGNAL,
	INSERT_MEDIA_REQUEST_SIGNAL,
	LOCATION_REQUEST_SIGNAL,
	PROGRESS_CHANGED_SIGNAL,
	ACTION_CHANGED_SIGNAL,
	DUMMY_SUCCESS_SIGNAL,
	EJECT_FAILURE_SIGNAL,
	BLANK_FAILURE_SIGNAL,
	INSTALL_MISSING_SIGNAL,
	LAST_SIGNAL
} RejillaBurnSignalType;

static guint rejilla_burn_signals [LAST_SIGNAL] = { 0 };

#define REJILLA_BURN_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REJILLA_TYPE_BURN, RejillaBurnPrivate))

#define MAX_EJECT_ATTEMPTS	5
#define MAX_MOUNT_ATTEMPTS	40

#define MOUNT_TIMEOUT		500

static GObjectClass *parent_class = NULL;

static void
rejilla_burn_powermanagement (RejillaBurn *self,
			      gboolean wake)
{
	RejillaBurnPrivate *priv = REJILLA_BURN_PRIVATE (self);

	if (wake)
	  	priv->appcookie = rejilla_inhibit_suspend (_("Burning CD/DVD"));
	else
		rejilla_uninhibit_suspend (priv->appcookie); 
}

/**
 * rejilla_burn_new:
 *
 *  Creates a new #RejillaBurn object.
 *
 * Return value: a #RejillaBurn object.
 **/

RejillaBurn *
rejilla_burn_new ()
{
	return g_object_new (REJILLA_TYPE_BURN, NULL);
}

static void
rejilla_burn_log (RejillaBurn *burn,
		  const gchar *format,
		  ...)
{
	RejillaBurnPrivate *priv = REJILLA_BURN_PRIVATE (burn);
	va_list arg_list;

	va_start (arg_list, format);
	rejilla_burn_session_logv (priv->session, format, arg_list);
	va_end (arg_list);
}

static RejillaBurnResult
rejilla_burn_emit_signal (RejillaBurn *burn,
                          guint signal,
                          RejillaBurnResult default_answer)
{
	GValue instance_and_params;
	GValue return_value;

	instance_and_params.g_type = 0;
	g_value_init (&instance_and_params, G_TYPE_FROM_INSTANCE (burn));
	g_value_set_instance (&instance_and_params, burn);

	return_value.g_type = 0;
	g_value_init (&return_value, G_TYPE_INT);
	g_value_set_int (&return_value, default_answer);

	g_signal_emitv (&instance_and_params,
			rejilla_burn_signals [signal],
			0,
			&return_value);

	g_value_unset (&instance_and_params);

	return g_value_get_int (&return_value);
}

static void
rejilla_burn_action_changed_real (RejillaBurn *burn,
				  RejillaBurnAction action)
{
	g_signal_emit (burn,
		       rejilla_burn_signals [ACTION_CHANGED_SIGNAL],
		       0,
		       action);

	if (action == REJILLA_BURN_ACTION_FINISHED)
		g_signal_emit (burn,
		               rejilla_burn_signals [PROGRESS_CHANGED_SIGNAL],
		               0,
		               1.0,
		               1.0,
		               -1L);
	else if (action == REJILLA_BURN_ACTION_EJECTING)
		g_signal_emit (burn,
			       rejilla_burn_signals [PROGRESS_CHANGED_SIGNAL],
			       0,
			       -1.0,
			       -1.0,
			       -1L);
}

static gboolean
rejilla_burn_wakeup (RejillaBurn *burn)
{
	RejillaBurnPrivate *priv = REJILLA_BURN_PRIVATE (burn);

	if (priv->sleep_loop)
		g_main_loop_quit (priv->sleep_loop);

	priv->timeout_id = 0;
	return FALSE;
}

static RejillaBurnResult
rejilla_burn_sleep (RejillaBurn *burn, gint msec)
{
	RejillaBurnPrivate *priv = REJILLA_BURN_PRIVATE (burn);
	GMainLoop *loop;

	priv->sleep_loop = g_main_loop_new (NULL, FALSE);
	priv->timeout_id = g_timeout_add (msec,
					  (GSourceFunc) rejilla_burn_wakeup,
					  burn);

	/* Keep a reference to the loop in case we are cancelled to destroy it */
	loop = priv->sleep_loop;
	g_main_loop_run (loop);

	if (priv->timeout_id) {
		g_source_remove (priv->timeout_id);
		priv->timeout_id = 0;
	}

	g_main_loop_unref (loop);
	if (priv->sleep_loop) {
		priv->sleep_loop = NULL;
		return REJILLA_BURN_OK;
	}

	/* if sleep_loop = NULL => We've been cancelled */
	return REJILLA_BURN_CANCEL;
}

static RejillaBurnResult
rejilla_burn_reprobe (RejillaBurn *burn)
{
	RejillaBurnPrivate *priv;
	RejillaBurnResult result = REJILLA_BURN_OK;

	priv = REJILLA_BURN_PRIVATE (burn);

	REJILLA_BURN_LOG ("Reprobing for medium");

	/* reprobe the medium and wait for it to be probed */
	rejilla_drive_reprobe (priv->dest);
	while (rejilla_drive_probing (priv->dest)) {
		result = rejilla_burn_sleep (burn, 250);
		if (result != REJILLA_BURN_OK)
			return result;
	}

	return result;
}

static RejillaBurnResult
rejilla_burn_unmount (RejillaBurn *self,
                      RejillaMedium *medium,
                      GError **error)
{
	guint counter = 0;

	if (!medium)
		return REJILLA_BURN_OK;

	/* Retry several times, since sometimes the drives are really busy */
	while (rejilla_volume_is_mounted (REJILLA_VOLUME (medium))) {
		GError *ret_error;
		RejillaBurnResult result;

		counter ++;
		if (counter > MAX_EJECT_ATTEMPTS) {
			REJILLA_BURN_LOG ("Max attempts reached at unmounting");
			if (error && !(*error))
				g_set_error (error,
					     REJILLA_BURN_ERROR,
					     REJILLA_BURN_ERROR_DRIVE_BUSY,
					     "%s. %s",
					     _("The drive is busy"),
					     _("Make sure another application is not using it"));
			return REJILLA_BURN_ERR;
		}

		REJILLA_BURN_LOG ("Retrying unmounting");

		ret_error = NULL;
		rejilla_volume_umount (REJILLA_VOLUME (medium), TRUE, NULL);

		if (ret_error) {
			REJILLA_BURN_LOG ("Ejection error: %s", ret_error->message);
			g_error_free (ret_error);
		}

		result = rejilla_burn_sleep (self, 500);
		if (result != REJILLA_BURN_OK)
			return result;
	}

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_burn_emit_eject_failure_signal (RejillaBurn *burn,
                                        RejillaDrive *drive) 
{
	GValue instance_and_params [4];
	GValue return_value;

	instance_and_params [0].g_type = 0;
	g_value_init (instance_and_params, G_TYPE_FROM_INSTANCE (burn));
	g_value_set_instance (instance_and_params, burn);
	
	instance_and_params [1].g_type = 0;
	g_value_init (instance_and_params + 1, G_TYPE_FROM_INSTANCE (drive));
	g_value_set_instance (instance_and_params + 1, drive);

	return_value.g_type = 0;
	g_value_init (&return_value, G_TYPE_INT);
	g_value_set_int (&return_value, REJILLA_BURN_CANCEL);

	g_signal_emitv (instance_and_params,
			rejilla_burn_signals [EJECT_FAILURE_SIGNAL],
			0,
			&return_value);

	g_value_unset (instance_and_params);

	return g_value_get_int (&return_value);
}

static RejillaBurnResult
rejilla_burn_eject (RejillaBurn *self,
		    RejillaDrive *drive,
		    GError **error)
{
	guint counter = 0;
	RejillaMedium *medium;
	RejillaBurnResult result;

	REJILLA_BURN_LOG ("Ejecting drive/medium");

	/* Unmount, ... */
	medium = rejilla_drive_get_medium (drive);
	result = rejilla_burn_unmount (self, medium, error);
	if (result != REJILLA_BURN_OK)
		return result;

	/* Release lock, ... */
	if (rejilla_drive_is_locked (drive, NULL)) {
		if (!rejilla_drive_unlock (drive)) {
			gchar *name;

			name = rejilla_drive_get_display_name (drive);
			g_set_error (error,
				     REJILLA_BURN_ERROR,
				     REJILLA_BURN_ERROR_GENERAL,
				     _("\"%s\" cannot be unlocked"),
				     name);
			g_free (name);
			return REJILLA_BURN_ERR;
		}
	}

	/* Retry several times, since sometimes the drives are really busy */
	while (rejilla_drive_get_medium (drive) || rejilla_drive_probing (drive)) {
		GError *ret_error;

		/* Don't interrupt a probe */
		if (rejilla_drive_probing (drive)) {
			result = rejilla_burn_sleep (self, 500);
			if (result != REJILLA_BURN_OK)
				return result;

			continue;
		}

		counter ++;
		if (counter == 1)
			rejilla_burn_action_changed_real (self, REJILLA_BURN_ACTION_EJECTING);

		if (counter > MAX_EJECT_ATTEMPTS) {
			REJILLA_BURN_LOG ("Max attempts reached at ejecting");

			result = rejilla_burn_emit_eject_failure_signal (self, drive);
			if (result != REJILLA_BURN_OK)
				return result;

			continue;
		}

		REJILLA_BURN_LOG ("Retrying ejection");
		ret_error = NULL;
		rejilla_drive_eject (drive, TRUE, &ret_error);

		if (ret_error) {
			REJILLA_BURN_LOG ("Ejection error: %s", ret_error->message);
			g_error_free (ret_error);
		}

		result = rejilla_burn_sleep (self, 500);
		if (result != REJILLA_BURN_OK)
			return result;
	}

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_burn_ask_for_media (RejillaBurn *burn,
			    RejillaDrive *drive,
			    RejillaBurnError error_type,
			    RejillaMedia required_media,
			    GError **error)
{
	GValue instance_and_params [4];
	GValue return_value;

	instance_and_params [0].g_type = 0;
	g_value_init (instance_and_params, G_TYPE_FROM_INSTANCE (burn));
	g_value_set_instance (instance_and_params, burn);
	
	instance_and_params [1].g_type = 0;
	g_value_init (instance_and_params + 1, G_TYPE_FROM_INSTANCE (drive));
	g_value_set_instance (instance_and_params + 1, drive);
	
	instance_and_params [2].g_type = 0;
	g_value_init (instance_and_params + 2, G_TYPE_INT);
	g_value_set_int (instance_and_params + 2, error_type);
	
	instance_and_params [3].g_type = 0;
	g_value_init (instance_and_params + 3, G_TYPE_INT);
	g_value_set_int (instance_and_params + 3, required_media);
	
	return_value.g_type = 0;
	g_value_init (&return_value, G_TYPE_INT);
	g_value_set_int (&return_value, REJILLA_BURN_CANCEL);

	g_signal_emitv (instance_and_params,
			rejilla_burn_signals [INSERT_MEDIA_REQUEST_SIGNAL],
			0,
			&return_value);

	g_value_unset (instance_and_params);
	g_value_unset (instance_and_params + 1);

	return g_value_get_int (&return_value);
}

static RejillaBurnResult
rejilla_burn_ask_for_src_media (RejillaBurn *burn,
				RejillaBurnError error_type,
				RejillaMedia required_media,
				GError **error)
{
	RejillaMedium *medium;
	RejillaBurnPrivate *priv = REJILLA_BURN_PRIVATE (burn);

	medium = rejilla_drive_get_medium (priv->src);
	if (rejilla_medium_get_status (medium) != REJILLA_MEDIUM_NONE
	||  rejilla_drive_probing (priv->src)) {
		RejillaBurnResult result;
		result = rejilla_burn_eject (burn, priv->src, error);
		if (result != REJILLA_BURN_OK)
			return result;
	}

	return rejilla_burn_ask_for_media (burn,
					   priv->src,
					   error_type,
					   required_media,
					   error);
}

static RejillaBurnResult
rejilla_burn_ask_for_dest_media (RejillaBurn *burn,
				 RejillaBurnError error_type,
				 RejillaMedia required_media,
				 GError **error)
{
	RejillaDrive *drive;
	RejillaMedium *medium;
	RejillaBurnPrivate *priv = REJILLA_BURN_PRIVATE (burn);

	/* Since in some cases (like when we reload
	 * a medium after a copy), the destination
	 * medium may not be locked yet we use 
	 * separate variable drive. */
	if (!priv->dest) {
		drive = rejilla_burn_session_get_burner (priv->session);
		if (!drive) {
			g_set_error (error,
				     REJILLA_BURN_ERROR,
				     REJILLA_BURN_ERROR_OUTPUT_NONE,
				     "%s", _("No burner specified"));
			return REJILLA_BURN_ERR;
		}
	}
	else
		drive = priv->dest;

	medium = rejilla_drive_get_medium (drive);
	if (rejilla_medium_get_status (medium) != REJILLA_MEDIUM_NONE
	||  rejilla_drive_probing (drive)) {
		RejillaBurnResult result;

		result = rejilla_burn_eject (burn, drive, error);
		if (result != REJILLA_BURN_OK)
			return result;
	}

	return rejilla_burn_ask_for_media (burn,
					   drive,
					   error_type,
					   required_media,
					   error);
}

static RejillaBurnResult
rejilla_burn_lock_src_media (RejillaBurn *burn,
			     GError **error)
{
	gchar *failure = NULL;
	RejillaMedia media;
	RejillaMedium *medium;
	RejillaBurnResult result;
	RejillaBurnError error_type;
	RejillaBurnPrivate *priv = REJILLA_BURN_PRIVATE (burn);

	priv->src = rejilla_burn_session_get_src_drive (priv->session);
	if (!priv->src) {
		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_GENERAL,
			     "%s", _("No source drive specified"));
		return REJILLA_BURN_ERR;
	}


again:

	while (rejilla_drive_probing (priv->src)) {
		result = rejilla_burn_sleep (burn, 500);
		if (result != REJILLA_BURN_OK)
			return result;
	}

	medium = rejilla_drive_get_medium (priv->src);
	if (rejilla_volume_is_mounted (REJILLA_VOLUME (medium))) {
		if (!rejilla_volume_umount (REJILLA_VOLUME (medium), TRUE, NULL))
			g_warning ("Couldn't unmount volume in drive: %s",
				   rejilla_drive_get_device (priv->src));
	}

	/* NOTE: we used to unmount the media before now we shouldn't need that
	 * get any information from the drive */
	media = rejilla_medium_get_status (medium);
	if (media == REJILLA_MEDIUM_NONE)
		error_type = REJILLA_BURN_ERROR_MEDIUM_NONE;
	else if (media == REJILLA_MEDIUM_BUSY)
		error_type = REJILLA_BURN_ERROR_DRIVE_BUSY;
	else if (media == REJILLA_MEDIUM_UNSUPPORTED)
		error_type = REJILLA_BURN_ERROR_MEDIUM_INVALID;
	else if (media & REJILLA_MEDIUM_BLANK)
		error_type = REJILLA_BURN_ERROR_MEDIUM_NO_DATA;
	else
		error_type = REJILLA_BURN_ERROR_NONE;

	if (media & REJILLA_MEDIUM_BLANK) {
		result = rejilla_burn_ask_for_src_media (burn,
							 REJILLA_BURN_ERROR_MEDIUM_NO_DATA,
							 REJILLA_MEDIUM_HAS_DATA,
							 error);
		if (result != REJILLA_BURN_OK)
			return result;

		goto again;
	}

	if (!rejilla_drive_is_locked (priv->src, NULL)
	&&  !rejilla_drive_lock (priv->src, _("Ongoing copying process"), &failure)) {
		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_GENERAL,
			     _("The drive cannot be locked (%s)"),
			     failure);
		return REJILLA_BURN_ERR;
	}

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_burn_reload_src_media (RejillaBurn *burn,
			       RejillaBurnError error_code,
			       GError **error)
{
	RejillaBurnResult result;

	result = rejilla_burn_ask_for_src_media (burn,
						 error_code,
						 REJILLA_MEDIUM_HAS_DATA,
						 error);
	if (result != REJILLA_BURN_OK)
		return result;

	result = rejilla_burn_lock_src_media (burn, error);
	return result;
}

static RejillaBurnResult
rejilla_burn_lock_rewritable_media (RejillaBurn *burn,
				    GError **error)
{
	gchar *failure;
	RejillaMedia media;
	RejillaMedium *medium;
	RejillaBurnResult result;
	RejillaBurnError error_type;
	RejillaBurnPrivate *priv = REJILLA_BURN_PRIVATE (burn);

	priv->dest = rejilla_burn_session_get_burner (priv->session);
	if (!priv->dest) {
		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_OUTPUT_NONE,
			     "%s", _("No burner specified"));
		return REJILLA_BURN_NOT_SUPPORTED;
	}

 again:

	while (rejilla_drive_probing (priv->dest)) {
		result = rejilla_burn_sleep (burn, 500);
		if (result != REJILLA_BURN_OK)
			return result;
	}

	medium = rejilla_drive_get_medium (priv->dest);
	if (!rejilla_medium_can_be_rewritten (medium)) {
		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_MEDIUM_NOT_REWRITABLE,
			     "%s", _("The drive has no rewriting capabilities"));
		return REJILLA_BURN_NOT_SUPPORTED;
	}

	if (rejilla_volume_is_mounted (REJILLA_VOLUME (medium))) {
		if (!rejilla_volume_umount (REJILLA_VOLUME (medium), TRUE, NULL))
			g_warning ("Couldn't unmount volume in drive: %s",
				   rejilla_drive_get_device (priv->dest));
	}

	media = rejilla_medium_get_status (medium);
	if (media == REJILLA_MEDIUM_NONE)
		error_type = REJILLA_BURN_ERROR_MEDIUM_NONE;
	else if (media == REJILLA_MEDIUM_BUSY)
		error_type = REJILLA_BURN_ERROR_DRIVE_BUSY;
	else if (media == REJILLA_MEDIUM_UNSUPPORTED)
		error_type = REJILLA_BURN_ERROR_MEDIUM_INVALID;
	else if (!(media & REJILLA_MEDIUM_REWRITABLE))
		error_type = REJILLA_BURN_ERROR_MEDIUM_NOT_REWRITABLE;
	else
		error_type = REJILLA_BURN_ERROR_NONE;

	if (error_type != REJILLA_BURN_ERROR_NONE) {
		result = rejilla_burn_ask_for_dest_media (burn,
							  error_type,
							  REJILLA_MEDIUM_REWRITABLE|
							  REJILLA_MEDIUM_HAS_DATA,
							  error);

		if (result != REJILLA_BURN_OK)
			return result;

		goto again;
	}

	if (!rejilla_drive_is_locked (priv->dest, NULL)
	&&  !rejilla_drive_lock (priv->dest, _("Ongoing blanking process"), &failure)) {
		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_GENERAL,
			     _("The drive cannot be locked (%s)"),
			     failure);
		return REJILLA_BURN_ERR;
	}

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_burn_lock_dest_media (RejillaBurn *burn,
			      RejillaBurnError *ret_error,
			      GError **error)
{
	gchar *failure;
	RejillaMedia media;
	RejillaBurnError berror;
	RejillaMedium *medium;
	RejillaBurnResult result;
	RejillaTrackType *input = NULL;
	RejillaBurnPrivate *priv = REJILLA_BURN_PRIVATE (burn);

	priv->dest = rejilla_burn_session_get_burner (priv->session);
	if (!priv->dest) {
		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_OUTPUT_NONE,
			     "%s", _("No burner specified"));
		return REJILLA_BURN_ERR;
	}

	/* Check the capabilities of the drive */
	if (!rejilla_drive_can_write (priv->dest)) {
		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_GENERAL,
			     "%s", _("The drive cannot burn"));
		REJILLA_BURN_NOT_SUPPORTED_LOG (burn);
	}

	/* NOTE: don't lock the drive here yet as
	 * otherwise we'd be probing forever. */
	while (rejilla_drive_probing (priv->dest)) {
		result = rejilla_burn_sleep (burn, 500);
		if (result != REJILLA_BURN_OK)
			return result;
	}

	medium = rejilla_drive_get_medium (priv->dest);
	if (!medium) {
		result = REJILLA_BURN_NEED_RELOAD;
		berror = REJILLA_BURN_ERROR_MEDIUM_NONE;
		goto end;
	}

	/* unmount the medium */
	result = rejilla_burn_unmount (burn, medium, error);
	if (result != REJILLA_BURN_OK)
		return result;

	result = REJILLA_BURN_OK;
	berror = REJILLA_BURN_ERROR_NONE;

	media = rejilla_medium_get_status (medium);
	REJILLA_BURN_LOG_WITH_FULL_TYPE (REJILLA_TRACK_TYPE_DISC,
					 media,
					 REJILLA_PLUGIN_IO_NONE,
					 "Media inserted is");

	if (media == REJILLA_MEDIUM_NONE) {
		result = REJILLA_BURN_NEED_RELOAD;
		berror = REJILLA_BURN_ERROR_MEDIUM_NONE;
		goto end;
	}

	if (media == REJILLA_MEDIUM_UNSUPPORTED) {
		result = REJILLA_BURN_NEED_RELOAD;
		berror = REJILLA_BURN_ERROR_MEDIUM_INVALID;
		goto end;
	}

	if (media == REJILLA_MEDIUM_BUSY) {
		result = REJILLA_BURN_NEED_RELOAD;
		berror = REJILLA_BURN_ERROR_DRIVE_BUSY;
		goto end;
	}

	/* Make sure that media is supported and
	 * can be written to */

	/* NOTE: Since we did not check the flags
	 * and since they might change, check if the
	 * session is supported without the flags.
	 * We use quite a strict checking though as
	 * from now on we require plugins to be
	 * ready. */
	result = rejilla_burn_session_can_burn (priv->session, FALSE);
	if (result != REJILLA_BURN_OK) {
		REJILLA_BURN_LOG ("Inserted media is not supported");
		result = REJILLA_BURN_NEED_RELOAD;
		berror = REJILLA_BURN_ERROR_MEDIUM_INVALID;
		goto end;
	}

	input = rejilla_track_type_new ();
	rejilla_burn_session_get_input_type (priv->session, input);

	if (rejilla_track_type_get_has_image (input)) {
		goffset medium_sec = 0;
		goffset session_sec = 0;

		/* This test is only valid when it's an image
		 * as input which comes handy when copying
		 * from the same drive as we're sure we do 
		 * have an image. */
		rejilla_medium_get_capacity (medium,
		                             NULL,
		                             &medium_sec);
		rejilla_burn_session_get_size (priv->session,
		                               &session_sec,
		                               NULL);

		if (session_sec > medium_sec) {
			REJILLA_BURN_LOG ("Not enough space for image %"G_GOFFSET_FORMAT"/%"G_GOFFSET_FORMAT, session_sec, medium_sec);
			berror = REJILLA_BURN_ERROR_MEDIUM_SPACE;
			result = REJILLA_BURN_NEED_RELOAD;
			goto end;
		}
	}

	/* Only lock the drive after all checks succeeded */
	if (!rejilla_drive_is_locked (priv->dest, NULL)
	&&  !rejilla_drive_lock (priv->dest, _("Ongoing burning process"), &failure)) {
		rejilla_track_type_free (input);

		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_GENERAL,
			     _("The drive cannot be locked (%s)"),
			     failure);
		return REJILLA_BURN_ERR;
	}

end:

	if (result != REJILLA_BURN_OK
	&& rejilla_drive_is_locked (priv->dest, NULL))
		rejilla_drive_unlock (priv->dest);

	if (result == REJILLA_BURN_NEED_RELOAD && ret_error)
		*ret_error = berror;

	rejilla_track_type_free (input);

	return result;
}

static RejillaBurnResult
rejilla_burn_reload_dest_media (RejillaBurn *burn,
				RejillaBurnError error_code,
				GError **error)
{
	RejillaBurnPrivate *priv = REJILLA_BURN_PRIVATE (burn);
	RejillaMedia required_media;
	RejillaBurnResult result;

again:

	/* eject and ask the user to reload a disc */
	required_media = rejilla_burn_session_get_required_media_type (priv->session);
	required_media &= (REJILLA_MEDIUM_WRITABLE|
			   REJILLA_MEDIUM_CD|
			   REJILLA_MEDIUM_DVD);

	if (required_media == REJILLA_MEDIUM_NONE)
		required_media = REJILLA_MEDIUM_WRITABLE;

	result = rejilla_burn_ask_for_dest_media (burn,
						  error_code,
						  required_media,
						  error);
	if (result != REJILLA_BURN_OK)
		return result;

	result = rejilla_burn_lock_dest_media (burn,
					       &error_code,
					       error);
	if (result == REJILLA_BURN_NEED_RELOAD)
		goto again;

	return result;
}

static RejillaBurnResult
rejilla_burn_lock_checksum_media (RejillaBurn *burn,
				  GError **error)
{
	gchar *failure;
	RejillaMedia media;
	RejillaMedium *medium;
	RejillaBurnResult result;
	RejillaBurnError error_type;
	RejillaBurnPrivate *priv = REJILLA_BURN_PRIVATE (burn);

	priv->dest = rejilla_burn_session_get_src_drive (priv->session);

again:

	while (rejilla_drive_probing (priv->dest)) {
		result = rejilla_burn_sleep (burn, 500);
		if (result != REJILLA_BURN_OK)
			return result;
	}

	medium = rejilla_drive_get_medium (priv->dest);
	media = rejilla_medium_get_status (medium);

	error_type = REJILLA_BURN_ERROR_NONE;
	REJILLA_BURN_LOG_DISC_TYPE (media, "Waiting for media to checksum");

	if (media == REJILLA_MEDIUM_NONE) {
		/* NOTE: that's done on purpose since here if the drive is empty
		 * that's because we ejected it */
		result = rejilla_burn_ask_for_dest_media (burn,
							  REJILLA_BURN_WARNING_CHECKSUM,
							  REJILLA_MEDIUM_NONE,
							  error);
		if (result != REJILLA_BURN_OK)
			return result;
	}
	else if (media == REJILLA_MEDIUM_BUSY)
		error_type = REJILLA_BURN_ERROR_DRIVE_BUSY;
	else if (media == REJILLA_MEDIUM_UNSUPPORTED)
		error_type = REJILLA_BURN_ERROR_MEDIUM_INVALID;
	else if (media & REJILLA_MEDIUM_BLANK)
		error_type = REJILLA_BURN_ERROR_MEDIUM_NO_DATA;

	if (error_type != REJILLA_BURN_ERROR_NONE) {
		result = rejilla_burn_ask_for_dest_media (burn,
							  REJILLA_BURN_WARNING_CHECKSUM,
							  REJILLA_MEDIUM_NONE,
							  error);
		if (result != REJILLA_BURN_OK)
			return result;

		goto again;
	}

	if (!rejilla_drive_is_locked (priv->dest, NULL)
	&&  !rejilla_drive_lock (priv->dest, _("Ongoing checksumming operation"), &failure)) {
		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_GENERAL,
			     _("The drive cannot be locked (%s)"),
			     failure);
		return REJILLA_BURN_ERR;
	}

	/* if drive is mounted then unmount before checking anything */
/*	if (rejilla_volume_is_mounted (REJILLA_VOLUME (medium))
	&& !rejilla_volume_umount (REJILLA_VOLUME (medium), TRUE, NULL))
		g_warning ("Couldn't unmount volume in drive: %s",
			   rejilla_drive_get_device (priv->dest));
*/

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_burn_unlock_src_media (RejillaBurn *burn,
			       GError **error)
{
	RejillaBurnPrivate *priv = REJILLA_BURN_PRIVATE (burn);
	RejillaMedium *medium;

	if (!priv->src)
		return REJILLA_BURN_OK;

	/* If we mounted it ourselves, unmount it */
	medium = rejilla_drive_get_medium (priv->src);
	if (priv->mounted_by_us) {
		rejilla_burn_unmount (burn, medium, error);
		priv->mounted_by_us = FALSE;
	}

	if (rejilla_drive_is_locked (priv->src, NULL))
		rejilla_drive_unlock (priv->src);

	/* Never eject the source if we don't need to. Let the user do that. For
	 * one thing it avoids breaking other applications that are using it
	 * like for example idol. */
	/* if (REJILLA_BURN_SESSION_EJECT (priv->session))
		rejilla_drive_eject (REJILLA_VOLUME (medium), FALSE, error); */

	priv->src = NULL;
	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_burn_unlock_dest_media (RejillaBurn *burn,
				GError **error)
{
	RejillaBurnPrivate *priv = REJILLA_BURN_PRIVATE (burn);

	if (!priv->dest)
		return REJILLA_BURN_OK;

	if (rejilla_drive_is_locked (priv->dest, NULL))
		rejilla_drive_unlock (priv->dest);

	if (!REJILLA_BURN_SESSION_EJECT (priv->session))
		rejilla_drive_reprobe (priv->dest);
	else
		rejilla_burn_eject (burn, priv->dest, error);

	priv->dest = NULL;
	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_burn_unlock_medias (RejillaBurn *burn,
			    GError **error)
{
	rejilla_burn_unlock_dest_media (burn, error);
	rejilla_burn_unlock_src_media (burn, error);

	return REJILLA_BURN_OK;
}

static void
rejilla_burn_progress_changed (RejillaTaskCtx *task,
			       RejillaBurn *burn)
{
	RejillaBurnPrivate *priv = REJILLA_BURN_PRIVATE (burn);
	gdouble overall_progress = -1.0;
	gdouble task_progress = -1.0;
	glong time_remaining = -1;

	/* get the task current progress */
	if (rejilla_task_ctx_get_progress (task, &task_progress) == REJILLA_BURN_OK) {
		rejilla_task_ctx_get_remaining_time (task, &time_remaining);
		overall_progress = (task_progress + (gdouble) priv->tasks_done) /
				   (gdouble) priv->task_nb;
	}
	else
		overall_progress =  (gdouble) priv->tasks_done /
				    (gdouble) priv->task_nb;

	g_signal_emit (burn,
		       rejilla_burn_signals [PROGRESS_CHANGED_SIGNAL],
		       0,
		       overall_progress,
		       task_progress,
		       time_remaining);
}

static void
rejilla_burn_action_changed (RejillaTask *task,
			     RejillaBurnAction action,
			     RejillaBurn *burn)
{
	rejilla_burn_action_changed_real (burn, action);
}

/**
 * rejilla_burn_get_action_string:
 * @burn: a #RejillaBurn
 * @action: a #RejillaBurnAction
 * @string: a #gchar **
 *
 * This function returns the current action (in @string)  of
 * an ongoing operation performed by @burn.
 * @action is used to set a default string in case there was
 * no string set by the backend to describe the current
 * operation.
 *
 **/

void
rejilla_burn_get_action_string (RejillaBurn *burn,
				RejillaBurnAction action,
				gchar **string)
{
	RejillaBurnPrivate *priv;

	g_return_if_fail (REJILLA_BURN (burn));
	g_return_if_fail (string != NULL);

	priv = REJILLA_BURN_PRIVATE (burn);
	if (action == REJILLA_BURN_ACTION_FINISHED || !priv->task)
		(*string) = g_strdup (rejilla_burn_action_to_string (action));
	else
		rejilla_task_ctx_get_current_action_string (REJILLA_TASK_CTX (priv->task),
							    action,
							    string);
}

/**
 * rejilla_burn_status:
 * @burn: a #RejillaBurn
 * @media: a #RejillaMedia or NULL
 * @isosize: a #goffset or NULL
 * @written: a #goffset or NULL
 * @rate: a #guint64 or NULL
 *
 * Returns various information about the current operation 
 * in @media (the current media type being burnt),
 * @isosize (the size of the data being burnt), @written (the
 * number of bytes having been written so far) and @rate
 * (the speed at which data are written).
 *
 * Return value: a #RejillaBurnResult. REJILLA_BURN_OK if there is
 * an ongoing operation; REJILLA_BURN_NOT_READY otherwise.
 **/

RejillaBurnResult
rejilla_burn_status (RejillaBurn *burn,
		     RejillaMedia *media,
		     goffset *isosize,
		     goffset *written,
		     guint64 *rate)
{
	RejillaBurnPrivate *priv;
	RejillaBurnResult result;

	g_return_val_if_fail (REJILLA_BURN (burn), REJILLA_BURN_ERR);
	
	priv = REJILLA_BURN_PRIVATE (burn);

	if (!priv->task)
		return REJILLA_BURN_NOT_READY;

	if (isosize) {
		goffset size_local = 0;

		result = rejilla_task_ctx_get_session_output_size (REJILLA_TASK_CTX (priv->task),
								   NULL,
								   &size_local);
		if (result != REJILLA_BURN_OK)
			*isosize = -1;
		else
			*isosize = size_local;
	}

	if (!rejilla_task_is_running (priv->task))
		return REJILLA_BURN_NOT_READY;

	if (rate)
		rejilla_task_ctx_get_rate (REJILLA_TASK_CTX (priv->task), rate);

	if (written) {
		gint64 written_local = 0;

		result = rejilla_task_ctx_get_written (REJILLA_TASK_CTX (priv->task), &written_local);

		if (result != REJILLA_BURN_OK)
			*written = -1;
		else
			*written = written_local;
	}

	if (!media)
		return REJILLA_BURN_OK;

	/* return the disc we burn to if:
	 * - that's the last task to perform
	 * - rejilla_burn_session_is_dest_file returns FALSE
	 */
	if (priv->tasks_done < priv->task_nb - 1) {
		RejillaTrackType *input = NULL;

		input = rejilla_track_type_new ();
		rejilla_burn_session_get_input_type (priv->session, input);
		if (rejilla_track_type_get_has_medium (input))
			*media = rejilla_track_type_get_medium_type (input);
		else
			*media = REJILLA_MEDIUM_NONE;

		rejilla_track_type_free (input);
	}
	else if (rejilla_burn_session_is_dest_file (priv->session))
		*media = REJILLA_MEDIUM_FILE;
	else
		*media = rejilla_burn_session_get_dest_media (priv->session);

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_burn_ask_for_joliet (RejillaBurn *burn)
{
	RejillaBurnPrivate *priv = REJILLA_BURN_PRIVATE (burn);
	RejillaBurnResult result;
	GSList *tracks;
	GSList *iter;

	result = rejilla_burn_emit_signal (burn, ASK_DISABLE_JOLIET_SIGNAL, REJILLA_BURN_CANCEL);
	if (result != REJILLA_BURN_OK)
		return result;

	tracks = rejilla_burn_session_get_tracks (priv->session);
	for (iter = tracks; iter; iter = iter->next) {
		RejillaTrack *track;

		track = iter->data;
		rejilla_track_data_rm_fs (REJILLA_TRACK_DATA (track), REJILLA_IMAGE_FS_JOLIET);
	}

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_burn_ask_for_location (RejillaBurn *burn,
			       GError *received_error,
			       gboolean is_temporary,
			       GError **error)
{
	GValue instance_and_params [3];
	GValue return_value;

	instance_and_params [0].g_type = 0;
	g_value_init (instance_and_params, G_TYPE_FROM_INSTANCE (burn));
	g_value_set_instance (instance_and_params, burn);
	
	instance_and_params [1].g_type = 0;
	g_value_init (instance_and_params + 1, G_TYPE_POINTER);
	g_value_set_pointer (instance_and_params + 1, received_error);
	
	instance_and_params [2].g_type = 0;
	g_value_init (instance_and_params + 2, G_TYPE_BOOLEAN);
	g_value_set_boolean (instance_and_params + 2, is_temporary);
	
	return_value.g_type = 0;
	g_value_init (&return_value, G_TYPE_INT);
	g_value_set_int (&return_value, REJILLA_BURN_CANCEL);

	g_signal_emitv (instance_and_params,
			rejilla_burn_signals [LOCATION_REQUEST_SIGNAL],
			0,
			&return_value);

	g_value_unset (instance_and_params);
	g_value_unset (instance_and_params + 1);

	return g_value_get_int (&return_value);
}

static RejillaBurnResult
rejilla_burn_run_eraser (RejillaBurn *burn, GError **error)
{
	RejillaDrive *drive;
	RejillaMedium *medium;
	RejillaBurnPrivate *priv;
	RejillaBurnResult result;

	priv = REJILLA_BURN_PRIVATE (burn);

	drive = rejilla_burn_session_get_burner (priv->session);
	medium = rejilla_drive_get_medium (drive);

	result = rejilla_burn_unmount (burn, medium, error);
	if (result != REJILLA_BURN_OK)
		return result;

	result = rejilla_task_run (priv->task, error);
	if (result != REJILLA_BURN_OK)
		return result;

	/* Reprobe. It can happen (like with dvd+rw-format) that
	 * for the whole OS, the disc doesn't exist during the 
	 * formatting. Wait for the disc to reappear */
	/*  Likewise, this is necessary when we do a
	 * simulation before blanking since it blanked the disc
	 * and then to create all tasks necessary for the real
	 * burning operation, we'll need the real medium status 
	 * not to include a blanking job again. */
	return rejilla_burn_reprobe (burn);
}

static RejillaBurnResult
rejilla_burn_run_imager (RejillaBurn *burn,
			 gboolean fake,
			 GError **error)
{
	RejillaBurnPrivate *priv = REJILLA_BURN_PRIVATE (burn);
	RejillaBurnError error_code;
	RejillaBurnResult result;
	GError *ret_error = NULL;
	RejillaMedium *medium;
	RejillaDrive *src;

	src = rejilla_burn_session_get_src_drive (priv->session);

start:

	medium = rejilla_drive_get_medium (src);
	if (medium) {
		/* This is just in case */
		result = rejilla_burn_unmount (burn, medium, error);
		if (result != REJILLA_BURN_OK)
			return result;
	}

	/* If it succeeds then the new track(s) will be at the top of
	 * session tracks stack and therefore usable by the recorder.
	 * NOTE: it's up to the job to push the current tracks. */
	if (fake)
		result = rejilla_task_check (priv->task, &ret_error);
	else
		result = rejilla_task_run (priv->task, &ret_error);

	if (result == REJILLA_BURN_OK) {
		if (!fake) {
			g_signal_emit (burn,
				       rejilla_burn_signals [PROGRESS_CHANGED_SIGNAL],
				       0,
				       1.0,
				       1.0,
				       -1L);
		}
		return REJILLA_BURN_OK;
	}

	if (result != REJILLA_BURN_ERR) {
		if (error && ret_error)
			g_propagate_error (error, ret_error);

		return result;
	}

	if (!ret_error)
		return result;

	if (rejilla_burn_session_is_dest_file (priv->session)) {
		gchar *image = NULL;
		gchar *toc = NULL;

		/* If it was an image that was output, remove it. If that was
		 * a temporary image, it will be removed by RejillaBurnSession 
		 * object. But if it was a final image, it would be left and
		 * would clutter the disk, wasting space. */
		rejilla_burn_session_get_output (priv->session,
						 &image,
						 &toc);
		if (image)
			g_remove (image);
		if (toc)
			g_remove (toc);
	}

	/* See if we can recover from the error */
	error_code = ret_error->code;
	if (error_code == REJILLA_BURN_ERROR_IMAGE_JOLIET) {
		/* clean the error anyway since at worst the user will cancel */
		g_error_free (ret_error);
		ret_error = NULL;

		/* some files are not conforming to Joliet standard see
		 * if the user wants to carry on with a non joliet disc */
		result = rejilla_burn_ask_for_joliet (burn);
		if (result != REJILLA_BURN_OK)
			return result;

		goto start;
	}
	else if (error_code == REJILLA_BURN_ERROR_MEDIUM_NO_DATA) {
		/* clean the error anyway since at worst the user will cancel */
		g_error_free (ret_error);
		ret_error = NULL;

		/* The media hasn't data on it: ask for a new one. */
		result = rejilla_burn_reload_src_media (burn,
							error_code,
							error);
		if (result != REJILLA_BURN_OK)
			return result;

		goto start;
	}
	else if (error_code == REJILLA_BURN_ERROR_DISK_SPACE
	     ||  error_code == REJILLA_BURN_ERROR_PERMISSION
	     ||  error_code == REJILLA_BURN_ERROR_TMP_DIRECTORY) {
		gboolean is_temp;

		/* That's an imager (outputs an image to the disc) so that means
		 * that here the problem comes from the hard drive being too
		 * small or we don't have the right permission. */

		/* NOTE: Image file creation is always the last to take place 
		 * when it's not temporary. Another job should not take place
		 * afterwards */
		if (!rejilla_burn_session_is_dest_file (priv->session))
			is_temp = TRUE;
		else
			is_temp = FALSE;

		result = rejilla_burn_ask_for_location (burn,
							ret_error,
							is_temp,
							error);

		/* clean the error anyway since at worst the user will cancel */
		g_error_free (ret_error);
		ret_error = NULL;

		if (result != REJILLA_BURN_OK)
			return result;

		goto start;
	}

	/* If we reached this point that means the error was not recoverable.
	 * Propagate the error. */
	if (error && ret_error)
		g_propagate_error (error, ret_error);

	return REJILLA_BURN_ERR;
}

static RejillaBurnResult
rejilla_burn_can_use_drive_exclusively (RejillaBurn *burn,
					RejillaDrive *drive)
{
	RejillaBurnResult result;

	if (!drive)
		return REJILLA_BURN_OK;

	while (!rejilla_drive_can_use_exclusively (drive)) {
		REJILLA_BURN_LOG ("Device busy, retrying in 250 ms");
		result = rejilla_burn_sleep (burn, 250);
		if (result != REJILLA_BURN_OK)
			return result;
	}

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_burn_run_recorder (RejillaBurn *burn, GError **error)
{
	gint error_code;
	RejillaDrive *src;
	gboolean has_slept;
	RejillaDrive *burner;
	GError *ret_error = NULL;
	RejillaBurnResult result;
	RejillaMedium *src_medium;
	RejillaMedium *burnt_medium;
	RejillaBurnPrivate *priv = REJILLA_BURN_PRIVATE (burn);

	has_slept = FALSE;

	src = rejilla_burn_session_get_src_drive (priv->session);
	src_medium = rejilla_drive_get_medium (src);

	burner = rejilla_burn_session_get_burner (priv->session);
	burnt_medium = rejilla_drive_get_medium (burner);

start:

	/* this is just in case */
	if (REJILLA_BURN_SESSION_NO_TMP_FILE (priv->session)) {
		result = rejilla_burn_unmount (burn, src_medium, error);
		if (result != REJILLA_BURN_OK)
			return result;
	}

	result = rejilla_burn_unmount (burn, burnt_medium, error);
	if (result != REJILLA_BURN_OK)
		return result;

	/* before we start let's see if that drive can be used exclusively.
	 * Of course, it's not really safe since a process could take a lock
	 * just after us but at least it'll give some time to HAL and friends
	 * to finish what they're doing. 
	 * This was done because more than often backends wouldn't be able to 
	 * get a lock on a medium after a simulation. */
	result = rejilla_burn_can_use_drive_exclusively (burn, burner);
	if (result != REJILLA_BURN_OK)
		return result;

	/* actual running of task */
	result = rejilla_task_run (priv->task, &ret_error);

	/* let's see the results */
	if (result == REJILLA_BURN_OK) {
		g_signal_emit (burn,
			       rejilla_burn_signals [PROGRESS_CHANGED_SIGNAL],
			       0,
			       1.0,
			       1.0,
			       -1L);
		return REJILLA_BURN_OK;
	}

	if (result != REJILLA_BURN_ERR
	|| !ret_error
	||  ret_error->domain != REJILLA_BURN_ERROR) {
		if (ret_error)
			g_propagate_error (error, ret_error);

		return result;
	}

	/* see if error is recoverable */
	error_code = ret_error->code;
	if (error_code == REJILLA_BURN_ERROR_IMAGE_JOLIET) {
		/* NOTE: this error can only come from the source when 
		 * burning on the fly => no need to recreate an imager */

		/* some files are not conforming to Joliet standard see
		 * if the user wants to carry on with a non joliet disc */
		result = rejilla_burn_ask_for_joliet (burn);
		if (result != REJILLA_BURN_OK) {
			if (ret_error)
				g_propagate_error (error, ret_error);

			return result;
		}

		g_error_free (ret_error);
		ret_error = NULL;
		goto start;
	}
	else if (error_code == REJILLA_BURN_ERROR_MEDIUM_NEED_RELOADING) {
		/* NOTE: this error can only come from the source when 
		 * burning on the fly => no need to recreate an imager */

		/* The source media (when copying on the fly) is empty 
		 * so ask the user to reload another media with data */
		g_error_free (ret_error);
		ret_error = NULL;

		result = rejilla_burn_reload_src_media (burn,
							error_code,
							error);
		if (result != REJILLA_BURN_OK)
			return result;

		goto start;
	}
	else if (error_code == REJILLA_BURN_ERROR_SLOW_DMA) {
		guint64 rate;

		/* The whole system has just made a great effort. Sometimes it 
		 * helps to let it rest for a sec or two => that's what we do
		 * before retrying. (That's why usually cdrecord waits a little
		 * bit but sometimes it doesn't). Another solution would be to
		 * lower the speed a little (we could do both) */
		g_error_free (ret_error);
		ret_error = NULL;

		result = rejilla_burn_sleep (burn, 2000);
		if (result != REJILLA_BURN_OK)
			return result;

		has_slept = TRUE;

		/* set speed at 8x max and even less if speed  */
		rate = rejilla_burn_session_get_rate (priv->session);
		if (rate <= REJILLA_SPEED_TO_RATE_CD (8)) {
			rate = rate * 3 / 4;
			if (rate < CD_RATE)
				rate = CD_RATE;
		}
		else
			rate = REJILLA_SPEED_TO_RATE_CD (8);

		rejilla_burn_session_set_rate (priv->session, rate);
		goto start;
	}
	else if (error_code == REJILLA_BURN_ERROR_MEDIUM_SPACE) {
		/* NOTE: this error can only come from the dest drive */

		/* clean error and indicates this is a recoverable error */
		g_error_free (ret_error);
		ret_error = NULL;

		/* the space left on the media is insufficient (that's strange
		 * since we checked):
		 * the disc is either not rewritable or is too small anyway then
		 * we ask for a new media.
		 * It raises the problem of session merging. Indeed at this
		 * point an image can have been generated that was specifically
		 * generated for the inserted media. So if we have MERGE/APPEND
		 * that should fail.
		 */
		if (rejilla_burn_session_get_flags (priv->session) &
		   (REJILLA_BURN_FLAG_APPEND|REJILLA_BURN_FLAG_MERGE)) {
			g_set_error (error,
				     REJILLA_BURN_ERROR,
				     REJILLA_BURN_ERROR_MEDIUM_SPACE,
				     "%s. %s",
				     _("Merging data is impossible with this disc"),
				     _("Not enough space available on the disc"));
			return REJILLA_BURN_ERR;
		}

		/* ask for the destination media reload */
		result = rejilla_burn_reload_dest_media (burn,
							 error_code,
							 error);
		if (result != REJILLA_BURN_OK)
			return result;

		goto start;
	}
	else if (error_code >= REJILLA_BURN_ERROR_MEDIUM_NONE
	     &&  error_code <=  REJILLA_BURN_ERROR_MEDIUM_NEED_RELOADING) {
		/* NOTE: these errors can only come from the dest drive */

		/* clean error and indicates this is a recoverable error */
		g_error_free (ret_error);
		ret_error = NULL;

		/* ask for the destination media reload */
		result = rejilla_burn_reload_dest_media (burn,
							 error_code,
							 error);

		if (result != REJILLA_BURN_OK)
			return result;

		goto start;
	}

	if (ret_error)
		g_propagate_error (error, ret_error);

	return REJILLA_BURN_ERR;
}

static RejillaBurnResult
rejilla_burn_install_missing (RejillaPluginErrorType error,
			      const gchar *details,
			      gpointer user_data)
{
	GValue instance_and_params [3];
	GValue return_value;

	instance_and_params [0].g_type = 0;
	g_value_init (instance_and_params, G_TYPE_FROM_INSTANCE (user_data));
	g_value_set_instance (instance_and_params, user_data);
	
	instance_and_params [1].g_type = 0;
	g_value_init (instance_and_params + 1, G_TYPE_INT);
	g_value_set_int (instance_and_params + 1, error);

	instance_and_params [2].g_type = 0;
	g_value_init (instance_and_params + 2, G_TYPE_STRING);
	g_value_set_string (instance_and_params + 2, details);

	return_value.g_type = 0;
	g_value_init (&return_value, G_TYPE_INT);
	g_value_set_int (&return_value, REJILLA_BURN_ERROR);

	g_signal_emitv (instance_and_params,
			rejilla_burn_signals [INSTALL_MISSING_SIGNAL],
			0,
			&return_value);

	g_value_unset (instance_and_params);

	return g_value_get_int (&return_value);		       
}

static RejillaBurnResult
rejilla_burn_list_missing (RejillaPluginErrorType type,
			   const gchar *detail,
			   gpointer user_data)
{
	GString *string = user_data;

	if (type == REJILLA_PLUGIN_ERROR_MISSING_APP ||
	    type == REJILLA_PLUGIN_ERROR_SYMBOLIC_LINK_APP ||
	    type == REJILLA_PLUGIN_ERROR_WRONG_APP_VERSION) {
		g_string_append_c (string, '\n');
		/* Translators: %s is the name of a missing application */
		g_string_append_printf (string, _("%s (application)"), detail);
	}
	else if (type == REJILLA_PLUGIN_ERROR_MISSING_LIBRARY ||
	         type == REJILLA_PLUGIN_ERROR_LIBRARY_VERSION) {
		g_string_append_c (string, '\n');
		/* Translators: %s is the name of a missing library */
		g_string_append_printf (string, _("%s (library)"), detail);
	}
	else if (type == REJILLA_PLUGIN_ERROR_MISSING_GSTREAMER_PLUGIN) {
		g_string_append_c (string, '\n');
		/* Translators: %s is the name of a missing GStreamer plugin */
		g_string_append_printf (string, _("%s (GStreamer plugin)"), detail);
	}

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_burn_check_session_consistency (RejillaBurn *burn,
                                        RejillaTrackType *output,
					GError **error)
{
	RejillaBurnFlag flag;
	RejillaBurnFlag flags;
	RejillaBurnFlag retval;
	RejillaBurnResult result;
	RejillaTrackType *input = NULL;
	RejillaBurnFlag supported = REJILLA_BURN_FLAG_NONE;
	RejillaBurnFlag compulsory = REJILLA_BURN_FLAG_NONE;
	RejillaBurnPrivate *priv = REJILLA_BURN_PRIVATE (burn);

	REJILLA_BURN_DEBUG (burn, "Checking session consistency");

	/* make sure there is a track in the session. */
	input = rejilla_track_type_new ();
	rejilla_burn_session_get_input_type (priv->session, input);

	if (rejilla_track_type_is_empty (input)) {
		rejilla_track_type_free (input);

		REJILLA_BURN_DEBUG (burn, "No track set");
		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_GENERAL,
			     "%s", _("There is no track to burn"));
		return REJILLA_BURN_ERR;
	}

	/* No need to check if a burner was set as this
	 * is done when locking. */

	/* save then wipe out flags from session to check them one by one */
	flags = rejilla_burn_session_get_flags (priv->session);
	rejilla_burn_session_set_flags (REJILLA_BURN_SESSION (priv->session), REJILLA_BURN_FLAG_NONE);

	if (!output || rejilla_track_type_get_has_medium (output))
		result = rejilla_burn_session_get_burn_flags (priv->session,
							      &supported,
							      &compulsory);
	else
		result = rejilla_caps_session_get_image_flags (input,
		                                              output,
		                                              &supported,
		                                              &compulsory);

	if (result != REJILLA_BURN_OK) {
		rejilla_track_type_free (input);
		return result;
	}

	for (flag = 1; flag < REJILLA_BURN_FLAG_LAST; flag <<= 1) {
		/* see if this flag was originally set */
		if (!(flags & flag))
			continue;

		/* Check each flag before re-adding it. Emit warnings to user
		 * to know if he wants to carry on for some flags when they are
		 * not supported; namely DUMMY. Other flags trigger an error.
		 * No need for BURNPROOF since that usually means it is just the
		 * media type that doesn't need it. */
		if (supported & flag) {
			rejilla_burn_session_add_flag (priv->session, flag);

			if (!output || rejilla_track_type_get_has_medium (output))
				result = rejilla_burn_session_get_burn_flags (priv->session,
									      &supported,
									      &compulsory);
			else
				result = rejilla_caps_session_get_image_flags (input,
									      output,
									      &supported,
									      &compulsory);
		}
		else {
			REJILLA_BURN_LOG_FLAGS (flag, "Flag set but not supported:");

			if (flag & REJILLA_BURN_FLAG_DUMMY) {
				/* This is simply a warning that it's not possible */

			}
			else if (flag & REJILLA_BURN_FLAG_MERGE) {
				rejilla_track_type_free (input);

				/* we pay attention to one flag in particular
				 * (MERGE) if it was set then it must be
				 * supported. Otherwise error out. */
				g_set_error (error,
					     REJILLA_BURN_ERROR,
					     REJILLA_BURN_ERROR_GENERAL,
					     "%s", _("Merging data is impossible with this disc"));
				return REJILLA_BURN_ERR;
			}
			/* No need to tell the user burnproof is not supported
			 * as these drives handle errors differently which makes
			 * burnproof useless for them. */
		}
	}
	rejilla_track_type_free (input);

	retval = rejilla_burn_session_get_flags (priv->session);
	if (retval != flags)
		REJILLA_BURN_LOG_FLAGS (retval, "Some flags were not supported. Corrected to");

	if (retval != (retval | compulsory)) {
		retval |= compulsory;
		REJILLA_BURN_LOG_FLAGS (retval, "Some compulsory flags were forgotten. Corrected to");
	}

	rejilla_burn_session_set_flags (priv->session, retval);
	REJILLA_BURN_LOG_FLAGS (retval, "Flags after checking =");

	/* Check missing applications/GStreamer plugins.
	 * This is the best place. */
	rejilla_burn_session_set_strict_support (REJILLA_BURN_SESSION (priv->session), TRUE);
	result = rejilla_burn_session_can_burn (priv->session, FALSE);
	rejilla_burn_session_set_strict_support (REJILLA_BURN_SESSION (priv->session), FALSE);

	if (result == REJILLA_BURN_OK)
		return result;

	result = rejilla_burn_session_can_burn (priv->session, FALSE);
	if (result != REJILLA_BURN_OK)
		return result;

	result = rejilla_session_foreach_plugin_error (priv->session,
	                                               rejilla_burn_install_missing,
	                                               burn);
	if (result != REJILLA_BURN_OK) {
		if (result != REJILLA_BURN_CANCEL) {
			GString *string;

			string = g_string_new (_("Please install the following required applications and libraries manually and try again:"));
			rejilla_session_foreach_plugin_error (priv->session,
			                                      rejilla_burn_list_missing,
	        			                      string);
			g_set_error (error,
				     REJILLA_BURN_ERROR,
				     REJILLA_BURN_ERROR_MISSING_APP_AND_PLUGIN,
				     "%s", string->str);

			g_string_free (string, TRUE);
		}

		return REJILLA_BURN_ERR;
	}

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_burn_check_data_loss (RejillaBurn *burn,
                              GError **error)
{
	RejillaMedia media;
	RejillaBurnFlag flags;
	RejillaTrackType *input;
	RejillaBurnResult result;
	RejillaTrackType *output;
	RejillaBurnPrivate *priv = REJILLA_BURN_PRIVATE (burn);

	output = rejilla_track_type_new ();
	rejilla_burn_session_get_output_type (priv->session, output);
	if (!rejilla_track_type_get_has_medium (output)) {
		rejilla_track_type_free (output);
		return REJILLA_BURN_OK;
	}

	flags = rejilla_burn_session_get_flags (priv->session);
	media = rejilla_track_type_get_medium_type (output);
	rejilla_track_type_free (output);

	input = rejilla_track_type_new ();
	rejilla_burn_session_get_input_type (priv->session, input);

	if (media & (REJILLA_MEDIUM_HAS_DATA|REJILLA_MEDIUM_HAS_AUDIO)) {
		if (flags & REJILLA_BURN_FLAG_BLANK_BEFORE_WRITE) {
			/* There is an error if APPEND was set since this disc is not
			 * supported without a prior blanking. */

			/* we warn the user is going to lose data even if in the case of
			 * DVD+/-RW we don't really blank the disc we rather overwrite */
			result = rejilla_burn_emit_signal (burn,
							   WARN_DATA_LOSS_SIGNAL,
							   REJILLA_BURN_CANCEL);
			if (result == REJILLA_BURN_NEED_RELOAD)
				goto reload;

			if (result != REJILLA_BURN_OK) {
				rejilla_track_type_free (input);
				return result;
			}
		}
		else {
			/* A few special warnings for the discs with data/audio on them
			 * that don't need prior blanking or can't be blanked */
			if ((media & REJILLA_MEDIUM_CD)
			&&  rejilla_track_type_get_has_stream (input)
			&& !REJILLA_STREAM_FORMAT_HAS_VIDEO (rejilla_track_type_get_stream_format (input))) {
				/* We'd rather blank and rewrite a disc rather than
				 * append audio to appendable disc. That's because audio
				 * tracks have little chance to be readable by common CD
				 * player as last tracks */
				result = rejilla_burn_emit_signal (burn,
								   WARN_AUDIO_TO_APPENDABLE_SIGNAL,
								   REJILLA_BURN_CANCEL);
				if (result == REJILLA_BURN_NEED_RELOAD)
					goto reload;

				if (result != REJILLA_BURN_OK) {
					rejilla_track_type_free (input);
					return result;
				}
			}

			/* NOTE: if input is AUDIO we don't care since the OS
			 * will load the last session of DATA anyway */
			if ((media & REJILLA_MEDIUM_HAS_DATA)
			&&   rejilla_track_type_get_has_data (input)
			&& !(flags & REJILLA_BURN_FLAG_MERGE)) {
				/* warn the users that their previous data
				 * session (s) will not be mounted by default by
				 * the OS and that it'll be invisible */
				result = rejilla_burn_emit_signal (burn,
								   WARN_PREVIOUS_SESSION_LOSS_SIGNAL,
								   REJILLA_BURN_CANCEL);

				if (result == REJILLA_BURN_RETRY) {
					/* Wipe out the current flags,
					 * Add a new one 
					 * Recheck the result */
					rejilla_burn_session_pop_settings (priv->session);
					rejilla_burn_session_push_settings (priv->session);
					rejilla_burn_session_add_flag (priv->session, REJILLA_BURN_FLAG_MERGE);
					result = rejilla_burn_check_session_consistency (burn, NULL, error);
					if (result != REJILLA_BURN_OK)
						return result;
				}

				if (result == REJILLA_BURN_NEED_RELOAD)
					goto reload;

				if (result != REJILLA_BURN_OK) {
					rejilla_track_type_free (input);
					return result;
				}
			}
		}
	}

	if (media & REJILLA_MEDIUM_REWRITABLE) {
		/* emits a warning for the user if it's a rewritable
		 * disc and he wants to write only audio tracks on it */

		/* NOTE: no need to error out here since the only thing
		 * we are interested in is if it is AUDIO or not or if
		 * the disc we are copying has audio tracks only or not */
		if (rejilla_track_type_get_has_stream (input)
		&& !REJILLA_STREAM_FORMAT_HAS_VIDEO (rejilla_track_type_get_stream_format (input))) {
			result = rejilla_burn_emit_signal (burn, 
			                                   WARN_REWRITABLE_SIGNAL,
			                                   REJILLA_BURN_CANCEL);

			if (result == REJILLA_BURN_NEED_RELOAD)
				goto reload;

			if (result != REJILLA_BURN_OK) {
				rejilla_track_type_free (input);
				return result;
			}
		}

		if (rejilla_track_type_get_has_medium (input)
		&& (rejilla_track_type_get_medium_type (input) & REJILLA_MEDIUM_HAS_AUDIO)) {
			result = rejilla_burn_emit_signal (burn,
			                                   WARN_REWRITABLE_SIGNAL,
			                                   REJILLA_BURN_CANCEL);

			if (result == REJILLA_BURN_NEED_RELOAD)
				goto reload;

			if (result != REJILLA_BURN_OK) {
				rejilla_track_type_free (input);
				return result;
			}
		}
	}

	rejilla_track_type_free (input);

	return REJILLA_BURN_OK;

reload:

	rejilla_track_type_free (input);

	result = rejilla_burn_reload_dest_media (burn, REJILLA_BURN_ERROR_NONE, error);
	if (result != REJILLA_BURN_OK)
		return result;

	return REJILLA_BURN_RETRY;
}

/* FIXME: at the moment we don't allow for mixed CD type */
static RejillaBurnResult
rejilla_burn_run_tasks (RejillaBurn *burn,
			gboolean erase_allowed,
                        RejillaTrackType *temp_output,
                        gboolean *dummy_session,
			GError **error)
{
	RejillaBurnResult result;
	GSList *tasks, *next, *iter;
	RejillaBurnPrivate *priv = REJILLA_BURN_PRIVATE (burn);

	/* push the session settings to keep the original session untainted */
	rejilla_burn_session_push_settings (priv->session);

	/* check flags consistency */
	result = rejilla_burn_check_session_consistency (burn, temp_output, error);
	if (result != REJILLA_BURN_OK) {
		rejilla_burn_session_pop_settings (priv->session);
		return result;
	}

	/* performed some additional tests that can only be performed at this
	 * point. They are mere warnings. */
	result = rejilla_burn_check_data_loss (burn, error);
	if (result != REJILLA_BURN_OK) {
		rejilla_burn_session_pop_settings (priv->session);
		return result;
	}

	tasks = rejilla_burn_caps_new_task (priv->caps,
					    priv->session,
	                                    temp_output,
					    error);
	if (!tasks) {
		rejilla_burn_session_pop_settings (priv->session);
		return REJILLA_BURN_NOT_SUPPORTED;
	}

	priv->tasks_done = 0;
	priv->task_nb = g_slist_length (tasks);
	REJILLA_BURN_LOG ("%i tasks to perform", priv->task_nb);

	/* run all imaging tasks first */
	for (iter = tasks; iter; iter = next) {
		goffset len = 0;
		RejillaDrive *drive;
		RejillaMedium *medium;
		RejillaTaskAction action;

		next = iter->next;
		priv->task = iter->data;
		tasks = g_slist_remove (tasks, priv->task);

		g_signal_connect (priv->task,
				  "progress-changed",
				  G_CALLBACK (rejilla_burn_progress_changed),
				  burn);
		g_signal_connect (priv->task,
				  "action-changed",
				  G_CALLBACK (rejilla_burn_action_changed),
				  burn);

		/* see what type of task it is. It could be a blank/erase one. */
		/* FIXME!
		 * If so then that's time to test the size of the image against
		 * the size of the disc since erasing/formatting is always left
		 * for the end, just before burning. We would not like to 
		 * blank a disc and tell the user right after that the size of
		 * the disc is not enough. */
		action = rejilla_task_ctx_get_action (REJILLA_TASK_CTX (priv->task));
		if (action == REJILLA_TASK_ACTION_ERASE) {
			RejillaTrackType *type;

			/* FIXME: how could it be possible for a drive to test
			 * with a CLOSED CDRW for example. Maybe we should
			 * format/blank anyway. */

			/* This is to avoid blanking a medium without knowing
			 * if the data will fit on it. At this point we do know 
			 * what the size of the data is going to be. */
			type = rejilla_track_type_new ();
			rejilla_burn_session_get_input_type (priv->session, type);
			if (rejilla_track_type_get_has_image (type)
			||  rejilla_track_type_get_has_medium (type)) {
				RejillaMedium *medium;
				goffset session_sec = 0;
				goffset medium_sec = 0;

				medium = rejilla_drive_get_medium (priv->dest);
				rejilla_medium_get_capacity (medium,
							     NULL,
							     &medium_sec);

				rejilla_burn_session_get_size (priv->session,
							       &session_sec,
							       NULL);

				if (session_sec > medium_sec) {
					REJILLA_BURN_LOG ("Not enough space on medium %"G_GOFFSET_FORMAT"/%"G_GOFFSET_FORMAT, session_sec, medium_sec);
					result = rejilla_burn_reload_dest_media (burn,
					                                         REJILLA_BURN_ERROR_MEDIUM_SPACE,
					                                         error);
					if (result == REJILLA_BURN_OK)
						result = REJILLA_BURN_RETRY;

					break;
				}
			}
			rejilla_track_type_free (type);

			/* This is to avoid a potential problem when running a 
			 * dummy session first. When running dummy session the 
			 * media gets erased if need be. Since it is not
			 * reloaded afterwards, for rejilla it has still got 
			 * data on it when we get to the real recording. */
			if (erase_allowed) {
				result = rejilla_burn_run_eraser (burn, error);
				if (result == REJILLA_BURN_CANCEL)
					return result;

				/* If the erasing process did not work then do
				 * not fail and cancel the entire session but
				 * ask the user if he wants to insert another
				 * disc instead. */
				if (result != REJILLA_BURN_OK) {
					RejillaBurnResult local_result;

					local_result = rejilla_burn_emit_signal (burn,
					                                         BLANK_FAILURE_SIGNAL,
					                                         REJILLA_BURN_ERR);
					if (local_result == REJILLA_BURN_OK) {
						local_result = rejilla_burn_reload_dest_media (burn,
						                                               REJILLA_BURN_ERROR_NONE,
						                                               NULL);
						if (local_result == REJILLA_BURN_OK)
							result = REJILLA_BURN_RETRY;
					}

					break;
				}

				/* Since we blanked/formatted we need to recheck the burn 
				 * flags with the new medium type as some flags could have
				 * been given the benefit of the double (MULTI with a CLOSED
				 * CD for example). Recheck the original flags as they were
				 * passed. */
				/* FIXME: for some important flags we should warn the user
				 * that it won't be possible */
				rejilla_burn_session_pop_settings (priv->session);
				rejilla_burn_session_push_settings (priv->session);
				result = rejilla_burn_check_session_consistency (burn, temp_output,error);
				if (result != REJILLA_BURN_OK)
					break;
			}
			else
				result = REJILLA_BURN_OK;

			g_object_unref (priv->task);
			priv->task = NULL;
			priv->tasks_done ++;

			continue;
		}

		/* Init the task and set the task output size. The task should
		 * then check that the disc has enough space. If the output is
		 * to the hard drive it will be done afterwards when not in fake
		 * mode. */
		result = rejilla_burn_run_imager (burn, TRUE, error);
		if (result != REJILLA_BURN_OK)
			break;

		/* try to get the output size */
		rejilla_task_ctx_get_session_output_size (REJILLA_TASK_CTX (priv->task),
							  &len,
							  NULL);

		drive = rejilla_burn_session_get_burner (priv->session);
		medium = rejilla_drive_get_medium (drive);

		if (rejilla_burn_session_get_flags (priv->session) & (REJILLA_BURN_FLAG_MERGE|REJILLA_BURN_FLAG_APPEND))
			priv->session_start = rejilla_medium_get_next_writable_address (medium);
		else
			priv->session_start = 0;

		priv->session_end = priv->session_start + len;

		REJILLA_BURN_LOG ("Burning from %lld to %lld",
				  priv->session_start,
				  priv->session_end);

		/* see if we reached a recording task: it's the last task */
		if (!next) {
			if (!rejilla_burn_session_is_dest_file (priv->session)) {
				*dummy_session = (rejilla_burn_session_get_flags (priv->session) & REJILLA_BURN_FLAG_DUMMY);
				result = rejilla_burn_run_recorder (burn, error);
			}
			else
				result = rejilla_burn_run_imager (burn, FALSE, error);

			if (result == REJILLA_BURN_OK)
				priv->tasks_done ++;

			break;
		}

		/* run the imager */
		result = rejilla_burn_run_imager (burn, FALSE, error);
		if (result != REJILLA_BURN_OK)
			break;

		g_object_unref (priv->task);
		priv->task = NULL;
		priv->tasks_done ++;
	}

	/* restore the session settings. Keep the used flags
	 * nevertheless to make sure we actually use the flags that were
	 * set after checking for session consistency. */
	rejilla_burn_session_pop_settings (priv->session);

	if (priv->task) {
		g_object_unref (priv->task);
		priv->task = NULL;
	}

	g_slist_foreach (tasks, (GFunc) g_object_unref, NULL);
	g_slist_free (tasks);

	return result;
}

static RejillaBurnResult
rejilla_burn_check_real (RejillaBurn *self,
			 RejillaTrack *track,
			 GError **error)
{
	RejillaMedium *medium;
	RejillaBurnResult result;
	RejillaBurnPrivate *priv;
	RejillaChecksumType checksum_type;

	priv = REJILLA_BURN_PRIVATE (self);

	REJILLA_BURN_LOG ("Starting to check track integrity");

	checksum_type = rejilla_track_get_checksum_type (track);

	/* if the input is a DISC and there isn't any checksum specified that 
	 * means the checksum file is on the disc. */
	medium = rejilla_drive_get_medium (priv->dest);

	/* get the task and run it skip it otherwise */
	priv->task = rejilla_burn_caps_new_checksuming_task (priv->caps,
							     priv->session,
							     NULL);
	if (priv->task) {
		priv->task_nb = 1;
		priv->tasks_done = 0;
		g_signal_connect (priv->task,
				  "progress-changed",
				  G_CALLBACK (rejilla_burn_progress_changed),
				  self);
		g_signal_connect (priv->task,
				  "action-changed",
				  G_CALLBACK (rejilla_burn_action_changed),
				  self);


		/* make sure one last time it is not mounted IF and only IF the
		 * checksum type is NOT FILE_MD5 */
		/* it seems to work without unmounting ... */
		/* if (medium
		 * &&  rejilla_volume_is_mounted (REJILLA_VOLUME (medium))
		 * && !rejilla_volume_umount (REJILLA_VOLUME (medium), TRUE, NULL)) {
		 *	g_set_error (error,
		 *		     REJILLA_BURN_ERROR,
		 *		     REJILLA_BURN_ERROR_DRIVE_BUSY,
		 *		     "%s. %s",
		 *		     _("The drive is busy"),
		 *		     _("Make sure another application is not using it"));
		 *	return REJILLA_BURN_ERR;
		 * }
		 */

		result = rejilla_task_run (priv->task, error);
		g_signal_emit (self,
			       rejilla_burn_signals [PROGRESS_CHANGED_SIGNAL],
			       0,
			       1.0,
			       1.0,
			       -1L);

		g_object_unref (priv->task);
		priv->task = NULL;
	}
	else {
		REJILLA_BURN_LOG ("The track cannot be checked");
		return REJILLA_BURN_OK;
	}

	return result;
}

static void
rejilla_burn_unset_checksums (RejillaBurn *self)
{
	GSList *tracks;
	RejillaTrackType *type;
	RejillaBurnPrivate *priv;

	priv = REJILLA_BURN_PRIVATE (self);

	tracks = rejilla_burn_session_get_tracks (priv->session);
	type = rejilla_track_type_new ();
	for (; tracks; tracks = tracks->next) {
		RejillaTrack *track;

		track = tracks->data;
		rejilla_track_get_track_type (track, type);
		if (!rejilla_track_type_get_has_image (type)
		&& !rejilla_track_type_get_has_medium (type))
			rejilla_track_set_checksum (track,
						    REJILLA_CHECKSUM_NONE,
						    NULL);
	}

	rejilla_track_type_free (type);
}

static RejillaBurnResult
rejilla_burn_record_session (RejillaBurn *burn,
			     gboolean erase_allowed,
                             RejillaTrackType *temp_output,
			     GError **error)
{
	gboolean dummy_session = FALSE;
	const gchar *checksum = NULL;
	RejillaTrack *track = NULL;
	RejillaChecksumType type;
	RejillaBurnPrivate *priv;
	RejillaBurnResult result;
	GError *ret_error = NULL;
	RejillaMedium *medium;
	GSList *tracks;

	priv = REJILLA_BURN_PRIVATE (burn);

	/* unset checksum since no image has the exact
	 * same even if it is created from the same files */
	rejilla_burn_unset_checksums (burn);

	do {
		if (ret_error) {
			g_error_free (ret_error);
			ret_error = NULL;
		}

		result = rejilla_burn_run_tasks (burn,
						 erase_allowed,
		                                 temp_output,
		                                 &dummy_session,
						 &ret_error);
	} while (result == REJILLA_BURN_RETRY);

	if (result != REJILLA_BURN_OK) {
		/* handle errors */
		if (ret_error) {
			g_propagate_error (error, ret_error);
			ret_error = NULL;
		}

		return result;
	}

	if (rejilla_burn_session_is_dest_file (priv->session))
		return REJILLA_BURN_OK;

	if (dummy_session) {
		/* if we are in dummy mode and successfully completed then:
		 * - no need to checksum the media afterward (done later)
		 * - no eject to have automatic real burning */

		REJILLA_BURN_DEBUG (burn, "Dummy session successfully finished");

		/* recording was successful, so tell it */
		rejilla_burn_action_changed_real (burn, REJILLA_BURN_ACTION_FINISHED);

		/* need to try again but this time for real */
		result = rejilla_burn_emit_signal (burn,
						   DUMMY_SUCCESS_SIGNAL,
						   REJILLA_BURN_OK);
		if (result != REJILLA_BURN_OK)
			return result;

		/* unset checksum since no image has the exact same even if it
		 * is created from the same files */
		rejilla_burn_unset_checksums (burn);

		/* remove dummy flag and restart real burning calling ourselves
		 * NOTE: don't bother to push the session. We know the changes 
		 * that were made. */
		rejilla_burn_session_remove_flag (priv->session, REJILLA_BURN_FLAG_DUMMY);
		result = rejilla_burn_record_session (burn, FALSE, temp_output, error);
		rejilla_burn_session_add_flag (priv->session, REJILLA_BURN_FLAG_DUMMY);

		return result;
	}

	/* see if we have a checksum generated for the session if so use
	 * it to check if the recording went well remaining on the top of
	 * the session should be the last track burnt/imaged */
	tracks = rejilla_burn_session_get_tracks (priv->session);
	if (g_slist_length (tracks) != 1)
		return REJILLA_BURN_OK;

	track = tracks->data;
	type = rejilla_track_get_checksum_type (track);
	if (type == REJILLA_CHECKSUM_MD5
	||  type == REJILLA_CHECKSUM_SHA1
	||  type == REJILLA_CHECKSUM_SHA256)
		checksum = rejilla_track_get_checksum (track);
	else if (type == REJILLA_CHECKSUM_MD5_FILE)
		checksum = REJILLA_MD5_FILE;
	else if (type == REJILLA_CHECKSUM_SHA1_FILE)
		checksum = REJILLA_SHA1_FILE;
	else if (type == REJILLA_CHECKSUM_SHA256_FILE)
		checksum = REJILLA_SHA256_FILE;
	else
		return REJILLA_BURN_OK;

	/* recording was successful, so tell it */
	rejilla_burn_action_changed_real (burn, REJILLA_BURN_ACTION_FINISHED);

	/* the idea is to push a new track on the stack with
	 * the current disc burnt and the checksum generated
	 * during the session recording */
	rejilla_burn_session_push_tracks (priv->session);

	track = REJILLA_TRACK (rejilla_track_disc_new ());
	rejilla_track_set_checksum (REJILLA_TRACK (track),
	                            type,
	                            checksum);

	rejilla_track_disc_set_drive (REJILLA_TRACK_DISC (track), rejilla_burn_session_get_burner (priv->session));
	rejilla_burn_session_add_track (priv->session, track, NULL);

	/* It's good practice to unref the track afterwards as we don't need it
	 * anymore. RejillaBurnSession refs it. */
	g_object_unref (track);

	REJILLA_BURN_DEBUG (burn, "Preparing to checksum (type %i %s)", type, checksum);

	/* reprobe the medium and wait for it to be probed */
	result = rejilla_burn_reprobe (burn);
	if (result != REJILLA_BURN_OK) {
		rejilla_burn_session_pop_tracks (priv->session);
		return result;
	}

	medium = rejilla_drive_get_medium (priv->dest);

	/* Why do we do this?
	 * Because for a lot of medium types the size
	 * of the track return is not the real size of the
	 * data that was written; examples
	 * - CD that was written in SAO mode 
	 * - a DVD-R which usually aligns its track size
	 *   to a 16 block boundary
	 */
	if (type == REJILLA_CHECKSUM_MD5
	||  type == REJILLA_CHECKSUM_SHA1
	||  type == REJILLA_CHECKSUM_SHA256) {
		GValue *value;

		/* get the last written track address */
		value = g_new0 (GValue, 1);
		g_value_init (value, G_TYPE_UINT64);

		REJILLA_BURN_LOG ("Start of last written track address == %lli", priv->session_start);
		g_value_set_uint64 (value, priv->session_start);
		rejilla_track_tag_add (track,
				       REJILLA_TRACK_MEDIUM_ADDRESS_START_TAG,
				       value);

		value = g_new0 (GValue, 1);
		g_value_init (value, G_TYPE_UINT64);

		REJILLA_BURN_LOG ("End of last written track address == %lli", priv->session_end);
		g_value_set_uint64 (value, priv->session_end);
		rejilla_track_tag_add (track,
				       REJILLA_TRACK_MEDIUM_ADDRESS_END_TAG,
				       value);
	}

	result = rejilla_burn_check_real (burn, track, error);
	rejilla_burn_session_pop_tracks (priv->session);

	if (result == REJILLA_BURN_CANCEL) {
		/* change the result value so we won't stop here if there are 
		 * other copies to be made */
		result = REJILLA_BURN_OK;
	}

	return result;
}

/**
 * rejilla_burn_check:
 * @burn: a #RejillaBurn
 * @session: a #RejillaBurnSession
 * @error: a #GError
 *
 * Checks the integrity of a medium according to the parameters
 * set in @session. The medium must be inserted in the #RejillaDrive
 * set as the source of a #RejillaTrackDisc track inserted in @session.
 *
 * Return value: a #RejillaBurnResult. The result of the operation. 
 * REJILLA_BURN_OK if it was successful.
 **/

RejillaBurnResult
rejilla_burn_check (RejillaBurn *self,
		    RejillaBurnSession *session,
		    GError **error)
{
	GSList *tracks;
	RejillaTrack *track;
	RejillaBurnResult result;
	RejillaBurnPrivate *priv;

	g_return_val_if_fail (REJILLA_IS_BURN (self), REJILLA_BURN_ERR);
	g_return_val_if_fail (REJILLA_IS_BURN_SESSION (session), REJILLA_BURN_ERR);

	priv = REJILLA_BURN_PRIVATE (self);

	g_object_ref (session);
	priv->session = session;

	/* NOTE: no need to check for parameters here;
	 * that'll be done when asking for a task */
	tracks = rejilla_burn_session_get_tracks (priv->session);
	if (g_slist_length (tracks) != 1) {
		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_GENERAL,
			     "%s", _("Only one track at a time can be checked"));
		return REJILLA_BURN_ERR;
	}

	track = tracks->data;

	/* if the input is a DISC, ask/check there is one and lock it (as dest) */
	if (REJILLA_IS_TRACK_IMAGE (track)) {
		/* make sure there is a disc. If not, ask one and lock it */
		result = rejilla_burn_lock_checksum_media (self, error);
		if (result != REJILLA_BURN_OK)
			return result;
	}

	rejilla_burn_powermanagement (self, TRUE);

	result = rejilla_burn_check_real (self, track, error);

	rejilla_burn_powermanagement (self, FALSE);

	if (result == REJILLA_BURN_OK)
		result = rejilla_burn_unlock_medias (self, error);
	else
		rejilla_burn_unlock_medias (self, NULL);

	/* no need to check the result of the comparison, it's set in session */

	if (result == REJILLA_BURN_OK)
		rejilla_burn_action_changed_real (self,
		                                  REJILLA_BURN_ACTION_FINISHED);

	/* NOTE: unref session only AFTER drives are unlocked */
	priv->session = NULL;
	g_object_unref (session);

	return result;
}

static RejillaBurnResult
rejilla_burn_same_src_dest_image (RejillaBurn *self,
				  GError **error)
{
	RejillaBurnResult result;
	RejillaBurnPrivate *priv;
	RejillaTrackType *output = NULL;

	/* we can't create a proper list of tasks here since we don't know the
	 * dest media type yet. So we try to find an intermediate image type and
	 * add it to the session as output */
	priv = REJILLA_BURN_PRIVATE (self);

	/* get the first possible format */
	output = rejilla_track_type_new ();
	result = rejilla_burn_session_get_tmp_image_type_same_src_dest (priv->session, output);
	if (result != REJILLA_BURN_OK) {
		rejilla_track_type_free (output);
		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_GENERAL,
			     "%s", _("No format for the temporary image could be found"));
		return result;
	}

	/* lock drive */
	result = rejilla_burn_lock_src_media (self, error);
	if (result != REJILLA_BURN_OK)
		goto end;

	/* run */
	result = rejilla_burn_record_session (self, TRUE, output, error);

	/* Check the results right now. If there was
	 * an error the source medium will be dealt
	 * with in rejilla_burn_record () anyway 
	 * with rejilla_burn_unlock_medias () */
	if (result != REJILLA_BURN_OK)
		goto end;

	/* reset everything back to normal */
	result = rejilla_burn_eject (self, priv->src, error);
	if (result != REJILLA_BURN_OK)
		goto end;

	rejilla_burn_unlock_src_media (self, NULL);

	/* There should be (a) track(s) at the top of
	 * the session stack so no need to create a
	 * new one */

	rejilla_burn_action_changed_real (self, REJILLA_BURN_ACTION_FINISHED);

end:

	if (output)
		rejilla_track_type_free (output);

	return result;
}

static RejillaBurnResult
rejilla_burn_same_src_dest_reload_medium (RejillaBurn *burn,
					  GError **error)
{
	RejillaBurnError berror;
	RejillaBurnPrivate *priv;
	RejillaBurnResult result;
	RejillaMedia required_media;

	priv = REJILLA_BURN_PRIVATE (burn);

	REJILLA_BURN_LOG ("Reloading medium after copy");

	/* Now there is the problem of flags... This really is a special
	 * case. we need to try to adjust the flags to the media type
	 * just after a new one is detected. For example there could be
	 * BURNPROOF/DUMMY whereas they are not supported for DVD+RW. So
	 * we need to be lenient. */

	/* Eject and ask the user to reload a disc */
	required_media = rejilla_burn_session_get_required_media_type (priv->session);
	required_media &= (REJILLA_MEDIUM_WRITABLE|
			   REJILLA_MEDIUM_CD|
			   REJILLA_MEDIUM_DVD|
			   REJILLA_MEDIUM_BD);

	/* There is sometimes no way to determine which type of media is
	 * required since some flags (that will be adjusted afterwards)
	 * prevent to reach some media type like BURNPROOF and DVD+RW. */
	if (required_media == REJILLA_MEDIUM_NONE)
		required_media = REJILLA_MEDIUM_WRITABLE;

	berror = REJILLA_BURN_WARNING_INSERT_AFTER_COPY;

again:

	result = rejilla_burn_ask_for_dest_media (burn,
						  berror,
						  required_media,
						  error);
	if (result != REJILLA_BURN_OK)
		return result;

	/* Note: we don't need to update the flags anymore
	 * as they are updated in rejilla_burn_run_tasks ()
	 * anyway. */

	if (result != REJILLA_BURN_OK) {
		/* Tell the user his/her disc is not supported and reload */
		berror = REJILLA_BURN_ERROR_MEDIUM_INVALID;
		goto again;
	}

	result = rejilla_burn_lock_dest_media (burn, &berror, error);
	if (result == REJILLA_BURN_CANCEL)
		return result;

	if (result != REJILLA_BURN_OK) {
		/* Tell the user his/her disc is not supported and reload */
		goto again;
	}

	return REJILLA_BURN_OK;
}

/**
 * rejilla_burn_record:
 * @burn: a #RejillaBurn
 * @session: a #RejillaBurnSession
 * @error: a #GError
 *
 * Burns or creates a disc image according to the parameters
 * set in @session.
 *
 * Return value: a #RejillaBurnResult. The result of the operation. 
 * REJILLA_BURN_OK if it was successful.
 **/

RejillaBurnResult 
rejilla_burn_record (RejillaBurn *burn,
		     RejillaBurnSession *session,
		     GError **error)
{
	RejillaTrackType *type = NULL;
	RejillaBurnResult result;
	RejillaBurnPrivate *priv;

	g_return_val_if_fail (REJILLA_IS_BURN (burn), REJILLA_BURN_ERR);
	g_return_val_if_fail (REJILLA_IS_BURN_SESSION (session), REJILLA_BURN_ERR);

	priv = REJILLA_BURN_PRIVATE (burn);

	/* make sure we're ready */
	if (rejilla_burn_session_get_status (session, NULL) != REJILLA_BURN_OK)
		return REJILLA_BURN_ERR;

	g_object_ref (session);
	priv->session = session;

	rejilla_burn_powermanagement (burn, TRUE);

	/* say to the whole world we started */
	rejilla_burn_action_changed_real (burn, REJILLA_BURN_ACTION_PREPARING);

	if (rejilla_burn_session_same_src_dest_drive (session)) {
		/* This is a special case */
		result = rejilla_burn_same_src_dest_image (burn, error);
		if (result != REJILLA_BURN_OK)
			goto end;

		result = rejilla_burn_same_src_dest_reload_medium (burn, error);
		if (result != REJILLA_BURN_OK)
			goto end;
	}
	else if (!rejilla_burn_session_is_dest_file (session)) {
		RejillaBurnError berror = REJILLA_BURN_ERROR_NONE;

		/* do some drive locking quite early to make sure we have a
		 * media in the drive so that we'll have all the necessary
		 * information */
		result = rejilla_burn_lock_dest_media (burn, &berror, error);
		while (result == REJILLA_BURN_NEED_RELOAD) {
			RejillaMedia required_media;

			required_media = rejilla_burn_session_get_required_media_type (priv->session);
			if (required_media == REJILLA_MEDIUM_NONE)
				required_media = REJILLA_MEDIUM_WRITABLE;

			result = rejilla_burn_ask_for_dest_media (burn,
								  berror,
								  required_media,
								  error);
			if (result != REJILLA_BURN_OK)
				goto end;

			result = rejilla_burn_lock_dest_media (burn, &berror, error);
		}

		if (result != REJILLA_BURN_OK)
			goto end;
	}

	type = rejilla_track_type_new ();
	rejilla_burn_session_get_input_type (session, type);
	if (rejilla_track_type_get_has_medium (type)) {
		result = rejilla_burn_lock_src_media (burn, error);
		if (result != REJILLA_BURN_OK)
			goto end;
	}

	/* burn the session except if dummy session */
	result = rejilla_burn_record_session (burn, TRUE, NULL, error);

end:

	rejilla_track_type_free (type);

	if (result == REJILLA_BURN_OK)
		result = rejilla_burn_unlock_medias (burn, error);
	else
		rejilla_burn_unlock_medias (burn, NULL);

	if (error && (*error) == NULL
	&& (result == REJILLA_BURN_NOT_READY
	||  result == REJILLA_BURN_NOT_SUPPORTED
	||  result == REJILLA_BURN_RUNNING
	||  result == REJILLA_BURN_NOT_RUNNING)) {
		REJILLA_BURN_LOG ("Internal error with result %i", result);
		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_GENERAL,
			     "%s", _("An internal error occurred"));
	}

	if (result == REJILLA_BURN_CANCEL) {
		REJILLA_BURN_DEBUG (burn, "Session cancelled by user");
	}
	else if (result != REJILLA_BURN_OK) {
		if (error && (*error)) {
			REJILLA_BURN_DEBUG (burn,
					    "Session error : %s",
					    (*error)->message);
		}
		else
			REJILLA_BURN_DEBUG (burn, "Session error : unknown");
	}
	else {
		REJILLA_BURN_DEBUG (burn, "Session successfully finished");
		rejilla_burn_action_changed_real (burn,
		                                  REJILLA_BURN_ACTION_FINISHED);
	}

	rejilla_burn_powermanagement (burn, FALSE);

	/* release session */
	g_object_unref (priv->session);
	priv->session = NULL;

	return result;
}

static RejillaBurnResult
rejilla_burn_blank_real (RejillaBurn *burn, GError **error)
{
	RejillaBurnResult result;
	RejillaBurnPrivate *priv;

	priv = REJILLA_BURN_PRIVATE (burn);

	priv->task = rejilla_burn_caps_new_blanking_task (priv->caps,
							  priv->session,
							  error);
	if (!priv->task)
		return REJILLA_BURN_NOT_SUPPORTED;

	g_signal_connect (priv->task,
			  "progress-changed",
			  G_CALLBACK (rejilla_burn_progress_changed),
			  burn);
	g_signal_connect (priv->task,
			  "action-changed",
			  G_CALLBACK (rejilla_burn_action_changed),
			  burn);

	result = rejilla_burn_run_eraser (burn, error);
	g_object_unref (priv->task);
	priv->task = NULL;

	return result;
}

/**
 * rejilla_burn_blank:
 * @burn: a #RejillaBurn
 * @session: a #RejillaBurnSession
 * @error: a #GError
 *
 * Blanks a medium according to the parameters
 * set in @session. The medium must be inserted in the #RejillaDrive
 * set with rejilla_burn_session_set_burner ().
 *
 * Return value: a #RejillaBurnResult. The result of the operation. 
 * REJILLA_BURN_OK if it was successful.
 **/

RejillaBurnResult
rejilla_burn_blank (RejillaBurn *burn,
		    RejillaBurnSession *session,
		    GError **error)
{
	RejillaBurnPrivate *priv;
	RejillaBurnResult result;
	GError *ret_error = NULL;

	g_return_val_if_fail (burn != NULL, REJILLA_BURN_ERR);
	g_return_val_if_fail (session != NULL, REJILLA_BURN_ERR);

	priv = REJILLA_BURN_PRIVATE (burn);

	g_object_ref (session);
	priv->session = session;

	rejilla_burn_powermanagement (burn, TRUE);

	/* we wait for the insertion of a media and lock it */
	result = rejilla_burn_lock_rewritable_media (burn, error);
	if (result != REJILLA_BURN_OK)
		goto end;

	result = rejilla_burn_blank_real (burn, &ret_error);
	while (result == REJILLA_BURN_ERR &&
	       ret_error &&
	       ret_error->code == REJILLA_BURN_ERROR_MEDIUM_NOT_REWRITABLE) {
		g_error_free (ret_error);
		ret_error = NULL;

		result = rejilla_burn_ask_for_dest_media (burn,
							  REJILLA_BURN_ERROR_MEDIUM_NOT_REWRITABLE,
							  REJILLA_MEDIUM_REWRITABLE|
							  REJILLA_MEDIUM_HAS_DATA,
							  error);
		if (result != REJILLA_BURN_OK)
			break;

		result = rejilla_burn_lock_rewritable_media (burn, error);
		if (result != REJILLA_BURN_OK)
			break;

		result = rejilla_burn_blank_real (burn, &ret_error);
	}

end:
	if (ret_error)
		g_propagate_error (error, ret_error);

	if (result == REJILLA_BURN_OK && !ret_error)
		result = rejilla_burn_unlock_medias (burn, error);
	else
		rejilla_burn_unlock_medias (burn, NULL);

	if (result == REJILLA_BURN_OK)
		rejilla_burn_action_changed_real (burn, REJILLA_BURN_ACTION_FINISHED);

	rejilla_burn_powermanagement (burn, FALSE);

	/* release session */
	g_object_unref (priv->session);
	priv->session = NULL;

	return result;
}

/**
 * rejilla_burn_cancel:
 * @burn: a #RejillaBurn
 * @protect: a #gboolean
 *
 * Cancels any ongoing operation. If @protect is TRUE then
 * cancellation will not take place for a "critical" task, a task whose interruption
 * could damage the medium or the drive.
 *
 * Return value: a #RejillaBurnResult. The result of the operation. 
 * REJILLA_BURN_OK if it was successful.
 **/

RejillaBurnResult
rejilla_burn_cancel (RejillaBurn *burn, gboolean protect)
{
	RejillaBurnResult result = REJILLA_BURN_OK;
	RejillaBurnPrivate *priv;

	g_return_val_if_fail (REJILLA_BURN (burn), REJILLA_BURN_ERR);

	priv = REJILLA_BURN_PRIVATE (burn);

	if (priv->timeout_id) {
		g_source_remove (priv->timeout_id);
		priv->timeout_id = 0;
	}

	if (priv->sleep_loop) {
		g_main_loop_quit (priv->sleep_loop);
		priv->sleep_loop = NULL;
	}

	if (priv->dest)
		rejilla_drive_cancel_current_operation (priv->dest);

	if (priv->src)
		rejilla_drive_cancel_current_operation (priv->src);

	if (priv->task && rejilla_task_is_running (priv->task))
		result = rejilla_task_cancel (priv->task, protect);

	return result;
}

static void
rejilla_burn_finalize (GObject *object)
{
	RejillaBurnPrivate *priv = REJILLA_BURN_PRIVATE (object);

	if (priv->timeout_id) {
		g_source_remove (priv->timeout_id);
		priv->timeout_id = 0;
	}

	if (priv->sleep_loop) {
		g_main_loop_quit (priv->sleep_loop);
		priv->sleep_loop = NULL;
	}

	if (priv->task) {
		g_object_unref (priv->task);
		priv->task = NULL;
	}

	if (priv->session) {
		g_object_unref (priv->session);
		priv->session = NULL;
	}

	if (priv->caps)
		g_object_unref (priv->caps);

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
rejilla_burn_class_init (RejillaBurnClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (RejillaBurnPrivate));

	parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = rejilla_burn_finalize;

	rejilla_burn_signals [ASK_DISABLE_JOLIET_SIGNAL] =
		g_signal_new ("disable_joliet",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (RejillaBurnClass,
					       ask_disable_joliet),
			      NULL, NULL,
			      rejilla_marshal_INT__VOID,
			      G_TYPE_INT, 0);
        rejilla_burn_signals [WARN_DATA_LOSS_SIGNAL] =
		g_signal_new ("warn_data_loss",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (RejillaBurnClass,
					       warn_data_loss),
			      NULL, NULL,
			      rejilla_marshal_INT__VOID,
			      G_TYPE_INT, 0);
        rejilla_burn_signals [WARN_PREVIOUS_SESSION_LOSS_SIGNAL] =
		g_signal_new ("warn_previous_session_loss",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (RejillaBurnClass,
					       warn_previous_session_loss),
			      NULL, NULL,
			      rejilla_marshal_INT__VOID,
			      G_TYPE_INT, 0);
        rejilla_burn_signals [WARN_AUDIO_TO_APPENDABLE_SIGNAL] =
		g_signal_new ("warn_audio_to_appendable",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (RejillaBurnClass,
					       warn_audio_to_appendable),
			      NULL, NULL,
			      rejilla_marshal_INT__VOID,
			      G_TYPE_INT, 0);
	rejilla_burn_signals [WARN_REWRITABLE_SIGNAL] =
		g_signal_new ("warn_rewritable",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (RejillaBurnClass,
					       warn_rewritable),
			      NULL, NULL,
			      rejilla_marshal_INT__VOID,
			      G_TYPE_INT, 0);
	rejilla_burn_signals [INSERT_MEDIA_REQUEST_SIGNAL] =
		g_signal_new ("insert_media",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (RejillaBurnClass,
					       insert_media_request),
			      NULL, NULL,
			      rejilla_marshal_INT__OBJECT_INT_INT,
			      G_TYPE_INT, 
			      3,
			      REJILLA_TYPE_DRIVE,
			      G_TYPE_INT,
			      G_TYPE_INT);
	rejilla_burn_signals [LOCATION_REQUEST_SIGNAL] =
		g_signal_new ("location-request",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (RejillaBurnClass,
					       location_request),
			      NULL, NULL,
			      rejilla_marshal_INT__POINTER_BOOLEAN,
			      G_TYPE_INT, 
			      2,
			      G_TYPE_POINTER,
			      G_TYPE_INT);
	rejilla_burn_signals [PROGRESS_CHANGED_SIGNAL] =
		g_signal_new ("progress_changed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (RejillaBurnClass,
					       progress_changed),
			      NULL, NULL,
			      rejilla_marshal_VOID__DOUBLE_DOUBLE_LONG,
			      G_TYPE_NONE, 
			      3,
			      G_TYPE_DOUBLE,
			      G_TYPE_DOUBLE,
			      G_TYPE_LONG);
        rejilla_burn_signals [ACTION_CHANGED_SIGNAL] =
		g_signal_new ("action_changed",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (RejillaBurnClass,
					       action_changed),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__INT,
			      G_TYPE_NONE, 
			      1,
			      G_TYPE_INT);
        rejilla_burn_signals [DUMMY_SUCCESS_SIGNAL] =
		g_signal_new ("dummy_success",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (RejillaBurnClass,
					       dummy_success),
			      NULL, NULL,
			      rejilla_marshal_INT__VOID,
			      G_TYPE_INT, 0);
        rejilla_burn_signals [EJECT_FAILURE_SIGNAL] =
		g_signal_new ("eject_failure",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (RejillaBurnClass,
					       eject_failure),
			      NULL, NULL,
			      rejilla_marshal_INT__OBJECT,
			      G_TYPE_INT, 1,
		              REJILLA_TYPE_DRIVE);
        rejilla_burn_signals [BLANK_FAILURE_SIGNAL] =
		g_signal_new ("blank_failure",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (RejillaBurnClass,
					       blank_failure),
			      NULL, NULL,
			      rejilla_marshal_INT__VOID,
			      G_TYPE_INT, 0,
		              G_TYPE_NONE);
	rejilla_burn_signals [INSTALL_MISSING_SIGNAL] =
		g_signal_new ("install_missing",
			      G_TYPE_FROM_CLASS (klass),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (RejillaBurnClass,
					       install_missing),
			      NULL, NULL,
			      rejilla_marshal_INT__INT_STRING,
			      G_TYPE_INT, 2,
		              G_TYPE_INT,
			      G_TYPE_STRING);
}

static void
rejilla_burn_init (RejillaBurn *obj)
{
	RejillaBurnPrivate *priv = REJILLA_BURN_PRIVATE (obj);

	priv->caps = rejilla_burn_caps_get_default ();
}
