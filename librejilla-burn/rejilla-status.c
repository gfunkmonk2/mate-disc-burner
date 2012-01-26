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


#include "rejilla-status.h"


typedef struct _RejillaStatusPrivate RejillaStatusPrivate;
struct _RejillaStatusPrivate
{
	RejillaBurnResult res;
	GError * error;
	gdouble progress;
	gchar * current_action;
};

#define REJILLA_STATUS_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REJILLA_TYPE_STATUS, RejillaStatusPrivate))

G_DEFINE_TYPE (RejillaStatus, rejilla_status, G_TYPE_OBJECT);


/**
 * rejilla_status_new:
 *
 * Creates a new #RejillaStatus object.
 *
 * Return value: a #RejillaStatus.
 **/

RejillaStatus *
rejilla_status_new (void)
{
	return g_object_new (REJILLA_TYPE_STATUS, NULL);
}

/**
 * rejilla_status_free:
 * @status: a #RejillaStatus.
 *
 * Frees #RejillaStatus structure.
 *
 * Deprecated since 2.29.2.
 *
 **/

G_GNUC_DEPRECATED void
rejilla_status_free (RejillaStatus *status)
{
	g_object_unref (status);
}

/**
 * rejilla_status_get_result:
 * @status: a #RejillaStatus.
 *
 * After an object (see rejilla_burn_track_get_status ()) has
 * been requested its status, this function returns that status.
 *
 * Return value: a #RejillaBurnResult.
 * REJILLA_BURN_OK if the object is ready.
 * REJILLA_BURN_NOT_READY if some time should be given to the object before it is ready.
 * REJILLA_BURN_ERR if there is an error.
 **/

RejillaBurnResult
rejilla_status_get_result (RejillaStatus *status)
{
	RejillaStatusPrivate *priv;

	g_return_val_if_fail (status != NULL, REJILLA_BURN_ERR);
	g_return_val_if_fail (REJILLA_IS_STATUS (status), REJILLA_BURN_ERR);

	priv = REJILLA_STATUS_PRIVATE (status);
	return priv->res;
}

/**
 * rejilla_status_get_progress:
 * @status: a #RejillaStatus.
 *
 * If rejilla_status_get_result () returned REJILLA_BURN_NOT_READY,
 * this function returns the progress regarding the operation completion.
 *
 * Return value: a #gdouble
 **/

gdouble
rejilla_status_get_progress (RejillaStatus *status)
{
	RejillaStatusPrivate *priv;

	g_return_val_if_fail (status != NULL, -1.0);
	g_return_val_if_fail (REJILLA_IS_STATUS (status), -1.0);

	priv = REJILLA_STATUS_PRIVATE (status);
	if (priv->res == REJILLA_BURN_OK)
		return 1.0;

	if (priv->res != REJILLA_BURN_NOT_READY)
		return -1.0;

	return priv->progress;
}

/**
 * rejilla_status_get_error:
 * @status: a #RejillaStatus.
 *
 * If rejilla_status_get_result () returned REJILLA_BURN_ERR,
 * this function returns the error.
 *
 * Return value: a #GError
 **/

GError *
rejilla_status_get_error (RejillaStatus *status)
{
	RejillaStatusPrivate *priv;

	g_return_val_if_fail (status != NULL, NULL);
	g_return_val_if_fail (REJILLA_IS_STATUS (status), NULL);

	priv = REJILLA_STATUS_PRIVATE (status);
	if (priv->res != REJILLA_BURN_ERR)
		return NULL;

	return g_error_copy (priv->error);
}

/**
 * rejilla_status_get_current_action:
 * @status: a #RejillaStatus.
 *
 * If rejilla_status_get_result () returned REJILLA_BURN_NOT_READY,
 * this function returns a string describing the operation currently performed.
 * Free the string when it is not needed anymore.
 *
 * Return value: a #gchar.
 **/

gchar *
rejilla_status_get_current_action (RejillaStatus *status)
{
	gchar *string;
	RejillaStatusPrivate *priv;

	g_return_val_if_fail (status != NULL, NULL);
	g_return_val_if_fail (REJILLA_IS_STATUS (status), NULL);

	priv = REJILLA_STATUS_PRIVATE (status);

	if (priv->res != REJILLA_BURN_NOT_READY)
		return NULL;

	string = g_strdup (priv->current_action);
	return string;

}

/**
 * rejilla_status_set_completed:
 * @status: a #RejillaStatus.
 *
 * Sets the status for a request to REJILLA_BURN_OK.
 *
 **/

void
rejilla_status_set_completed (RejillaStatus *status)
{
	RejillaStatusPrivate *priv;

	g_return_if_fail (status != NULL);
	g_return_if_fail (REJILLA_IS_STATUS (status));

	priv = REJILLA_STATUS_PRIVATE (status);

	priv->res = REJILLA_BURN_OK;
	priv->progress = 1.0;
}

/**
 * rejilla_status_set_not_ready:
 * @status: a #RejillaStatus.
 * @progress: a #gdouble or -1.0.
 * @current_action: a #gchar or NULL.
 *
 * Sets the status for a request to REJILLA_BURN_NOT_READY.
 * Allows to set a string describing the operation currently performed
 * as well as the progress regarding the operation completion.
 *
 **/

void
rejilla_status_set_not_ready (RejillaStatus *status,
			      gdouble progress,
			      const gchar *current_action)
{
	RejillaStatusPrivate *priv;

	g_return_if_fail (status != NULL);
	g_return_if_fail (REJILLA_IS_STATUS (status));

	priv = REJILLA_STATUS_PRIVATE (status);

	priv->res = REJILLA_BURN_NOT_READY;
	priv->progress = progress;

	if (priv->current_action)
		g_free (priv->current_action);
	priv->current_action = g_strdup (current_action);
}

/**
 * rejilla_status_set_running:
 * @status: a #RejillaStatus.
 * @progress: a #gdouble or -1.0.
 * @current_action: a #gchar or NULL.
 *
 * Sets the status for a request to REJILLA_BURN_RUNNING.
 * Allows to set a string describing the operation currently performed
 * as well as the progress regarding the operation completion.
 *
 **/

void
rejilla_status_set_running (RejillaStatus *status,
			    gdouble progress,
			    const gchar *current_action)
{
	RejillaStatusPrivate *priv;

	g_return_if_fail (status != NULL);
	g_return_if_fail (REJILLA_IS_STATUS (status));

	priv = REJILLA_STATUS_PRIVATE (status);

	priv->res = REJILLA_BURN_RUNNING;
	priv->progress = progress;

	if (priv->current_action)
		g_free (priv->current_action);
	priv->current_action = g_strdup (current_action);
}

/**
 * rejilla_status_set_error:
 * @status: a #RejillaStatus.
 * @error: a #GError or NULL.
 *
 * Sets the status for a request to REJILLA_BURN_ERR.
 *
 **/

void
rejilla_status_set_error (RejillaStatus *status,
			  GError *error)
{
	RejillaStatusPrivate *priv;

	g_return_if_fail (status != NULL);
	g_return_if_fail (REJILLA_IS_STATUS (status));

	priv = REJILLA_STATUS_PRIVATE (status);

	priv->res = REJILLA_BURN_ERR;
	priv->progress = -1.0;

	if (priv->error)
		g_error_free (priv->error);
	priv->error = error;
}

static void
rejilla_status_init (RejillaStatus *object)
{}

static void
rejilla_status_finalize (GObject *object)
{
	RejillaStatusPrivate *priv;

	priv = REJILLA_STATUS_PRIVATE (object);
	if (priv->error)
		g_error_free (priv->error);

	if (priv->current_action)
		g_free (priv->current_action);

	G_OBJECT_CLASS (rejilla_status_parent_class)->finalize (object);
}

static void
rejilla_status_class_init (RejillaStatusClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (RejillaStatusPrivate));

	object_class->finalize = rejilla_status_finalize;
}
