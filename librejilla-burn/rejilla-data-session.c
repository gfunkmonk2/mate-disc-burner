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
#include <glib/gi18n-lib.h>

#include "rejilla-media-private.h"

#include "scsi-device.h"

#include "rejilla-drive.h"
#include "rejilla-medium.h"
#include "rejilla-medium-monitor.h"
#include "burn-volume.h"

#include "rejilla-burn-lib.h"

#include "rejilla-data-session.h"
#include "rejilla-data-project.h"
#include "rejilla-file-node.h"
#include "rejilla-io.h"

#include "librejilla-marshal.h"

typedef struct _RejillaDataSessionPrivate RejillaDataSessionPrivate;
struct _RejillaDataSessionPrivate
{
	RejillaIOJobBase *load_dir;

	/* Multisession drives that are inserted */
	GSList *media;

	/* Drive whose session is loaded */
	RejillaMedium *loaded;

	/* Nodes from the loaded session in the tree */
	GSList *nodes;
};

#define REJILLA_DATA_SESSION_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REJILLA_TYPE_DATA_SESSION, RejillaDataSessionPrivate))

G_DEFINE_TYPE (RejillaDataSession, rejilla_data_session, REJILLA_TYPE_DATA_PROJECT);

enum {
	AVAILABLE_SIGNAL,
	LOADED_SIGNAL,
	LAST_SIGNAL
};

static gulong rejilla_data_session_signals [LAST_SIGNAL] = { 0 };

/**
 * to evaluate the contents of a medium or image async
 */
struct _RejillaIOImageContentsData {
	RejillaIOJob job;
	gchar *dev_image;

	gint64 session_block;
	gint64 block;
};
typedef struct _RejillaIOImageContentsData RejillaIOImageContentsData;

static void
rejilla_io_image_directory_contents_destroy (RejillaAsyncTaskManager *manager,
					     gboolean cancelled,
					     gpointer callback_data)
{
	RejillaIOImageContentsData *data = callback_data;

	g_free (data->dev_image);
	rejilla_io_job_free (cancelled, REJILLA_IO_JOB (data));
}

static RejillaAsyncTaskResult
rejilla_io_image_directory_contents_thread (RejillaAsyncTaskManager *manager,
					    GCancellable *cancel,
					    gpointer callback_data)
{
	RejillaIOImageContentsData *data = callback_data;
	RejillaDeviceHandle *handle;
	GList *children, *iter;
	GError *error = NULL;
	RejillaVolSrc *vol;

	handle = rejilla_device_handle_open (data->job.uri, FALSE, NULL);
	if (!handle) {
		GError *error;

		error = g_error_new (REJILLA_BURN_ERROR,
		                     REJILLA_BURN_ERROR_GENERAL,
		                     _("The drive is busy"));

		rejilla_io_return_result (data->job.base,
					  data->job.uri,
					  NULL,
					  error,
					  data->job.callback_data);
		return REJILLA_ASYNC_TASK_FINISHED;
	}

	vol = rejilla_volume_source_open_device_handle (handle, &error);
	if (!vol) {
		rejilla_device_handle_close (handle);
		rejilla_io_return_result (data->job.base,
					  data->job.uri,
					  NULL,
					  error,
					  data->job.callback_data);
		return REJILLA_ASYNC_TASK_FINISHED;
	}

	children = rejilla_volume_load_directory_contents (vol,
							   data->session_block,
							   data->block,
							   &error);
	rejilla_volume_source_close (vol);
	rejilla_device_handle_close (handle);

	for (iter = children; iter; iter = iter->next) {
		RejillaVolFile *file;
		GFileInfo *info;

		file = iter->data;

		info = g_file_info_new ();
		g_file_info_set_file_type (info, file->isdir? G_FILE_TYPE_DIRECTORY:G_FILE_TYPE_REGULAR);
		g_file_info_set_name (info, REJILLA_VOLUME_FILE_NAME (file));

		if (file->isdir)
			g_file_info_set_attribute_int64 (info,
							 REJILLA_IO_DIR_CONTENTS_ADDR,
							 file->specific.dir.address);
		else
			g_file_info_set_size (info, REJILLA_VOLUME_FILE_SIZE (file));

		rejilla_io_return_result (data->job.base,
					  data->job.uri,
					  info,
					  NULL,
					  data->job.callback_data);
	}

	g_list_foreach (children, (GFunc) rejilla_volume_file_free, NULL);
	g_list_free (children);

	return REJILLA_ASYNC_TASK_FINISHED;
}

static const RejillaAsyncTaskType image_contents_type = {
	rejilla_io_image_directory_contents_thread,
	rejilla_io_image_directory_contents_destroy
};

static void
rejilla_io_load_image_directory (const gchar *dev_image,
				 gint64 session_block,
				 gint64 block,
				 const RejillaIOJobBase *base,
				 RejillaIOFlags options,
				 gpointer user_data)
{
	RejillaIOImageContentsData *data;
	RejillaIOResultCallbackData *callback_data = NULL;

	if (user_data) {
		callback_data = g_new0 (RejillaIOResultCallbackData, 1);
		callback_data->callback_data = user_data;
	}

	data = g_new0 (RejillaIOImageContentsData, 1);
	data->block = block;
	data->session_block = session_block;

	rejilla_io_set_job (REJILLA_IO_JOB (data),
			    base,
			    dev_image,
			    options,
			    callback_data);

	rejilla_io_push_job (REJILLA_IO_JOB (data),
			     &image_contents_type);

}

void
rejilla_data_session_remove_last (RejillaDataSession *self)
{
	RejillaDataSessionPrivate *priv;
	GSList *iter;

	priv = REJILLA_DATA_SESSION_PRIVATE (self);

	if (!priv->nodes)
		return;

	/* go through the top nodes and remove all the imported nodes */
	for (iter = priv->nodes; iter; iter = iter->next) {
		RejillaFileNode *node;

		node = iter->data;
		rejilla_data_project_destroy_node (REJILLA_DATA_PROJECT (self), node);
	}

	g_slist_free (priv->nodes);
	priv->nodes = NULL;

	g_signal_emit (self,
		       rejilla_data_session_signals [LOADED_SIGNAL],
		       0,
		       priv->loaded,
		       FALSE);

	if (priv->loaded) {
		g_object_unref (priv->loaded);
		priv->loaded = NULL;
	}
}

static void
rejilla_data_session_load_dir_destroy (GObject *object,
				       gboolean cancelled,
				       gpointer data)
{
	gint reference;
	RejillaFileNode *parent;

	/* reference */
	reference = GPOINTER_TO_INT (data);
	if (reference <= 0)
		return;

	parent = rejilla_data_project_reference_get (REJILLA_DATA_PROJECT (object), reference);
	if (parent)
		rejilla_data_project_directory_node_loaded (REJILLA_DATA_PROJECT (object), parent);

	rejilla_data_project_reference_free (REJILLA_DATA_PROJECT (object), reference);
}

static void
rejilla_data_session_load_dir_result (GObject *owner,
				      GError *error,
				      const gchar *dev_image,
				      GFileInfo *info,
				      gpointer data)
{
	RejillaDataSessionPrivate *priv;
	RejillaFileNode *parent;
	RejillaFileNode *node;
	gint reference;

	priv = REJILLA_DATA_SESSION_PRIVATE (owner);

	if (!info) {
		g_signal_emit (owner,
			       rejilla_data_session_signals [LOADED_SIGNAL],
			       0,
			       priv->loaded,
			       FALSE);

		/* FIXME: tell the user the error message */
		return;
	}

	reference = GPOINTER_TO_INT (data);
	if (reference > 0)
		parent = rejilla_data_project_reference_get (REJILLA_DATA_PROJECT (owner),
							     reference);
	else
		parent = NULL;

	/* add all the files/folders at the root of the session */
	node = rejilla_data_project_add_imported_session_file (REJILLA_DATA_PROJECT (owner),
							       info,
							       parent);
	if (!node) {
		/* This is not a problem, it could be simply that the user did 
		 * not want to overwrite, so do not do the following (reminder):
		g_signal_emit (owner,
			       rejilla_data_session_signals [LOADED_SIGNAL],
			       0,
			       priv->loaded,
			       (priv->nodes != NULL));
		*/
		return;
 	}

	/* Only if we're exploring root directory */
	if (!parent) {
		priv->nodes = g_slist_prepend (priv->nodes, node);

		if (g_slist_length (priv->nodes) == 1) {
			/* Only tell when the first top node is successfully loaded */
			g_signal_emit (owner,
				       rejilla_data_session_signals [LOADED_SIGNAL],
				       0,
				       priv->loaded,
				       TRUE);
		}
	}
}

static gboolean
rejilla_data_session_load_directory_contents_real (RejillaDataSession *self,
						   RejillaFileNode *node,
						   GError **error)
{
	RejillaDataSessionPrivate *priv;
	goffset session_block;
	const gchar *device;
	gint reference = -1;

	if (node && !node->is_fake)
		return TRUE;

	priv = REJILLA_DATA_SESSION_PRIVATE (self);
	device = rejilla_drive_get_device (rejilla_medium_get_drive (priv->loaded));
	rejilla_medium_get_last_data_track_address (priv->loaded,
						    NULL,
						    &session_block);

	if (!priv->load_dir)
		priv->load_dir = rejilla_io_register (G_OBJECT (self),
						      rejilla_data_session_load_dir_result,
						      rejilla_data_session_load_dir_destroy,
						      NULL);

	/* If there aren't any node then that's root */
	if (node) {
		reference = rejilla_data_project_reference_new (REJILLA_DATA_PROJECT (self), node);
		node->is_exploring = TRUE;
	}

	rejilla_io_load_image_directory (device,
					 session_block,
					 REJILLA_FILE_NODE_IMPORTED_ADDRESS (node),
					 priv->load_dir,
					 REJILLA_IO_INFO_URGENT,
					 GINT_TO_POINTER (reference));

	if (node)
		node->is_fake = FALSE;

	return TRUE;
}

gboolean
rejilla_data_session_load_directory_contents (RejillaDataSession *self,
					      RejillaFileNode *node,
					      GError **error)
{
	if (node == NULL)
		return FALSE;

	return rejilla_data_session_load_directory_contents_real (self, node, error);
}

gboolean
rejilla_data_session_add_last (RejillaDataSession *self,
			       RejillaMedium *medium,
			       GError **error)
{
	RejillaDataSessionPrivate *priv;

	priv = REJILLA_DATA_SESSION_PRIVATE (self);

	if (priv->nodes)
		return FALSE;

	priv->loaded = medium;
	g_object_ref (medium);

	return rejilla_data_session_load_directory_contents_real (self, NULL, error);
}

gboolean
rejilla_data_session_has_available_media (RejillaDataSession *self)
{
	RejillaDataSessionPrivate *priv;

	priv = REJILLA_DATA_SESSION_PRIVATE (self);

	return priv->media != NULL;
}

GSList *
rejilla_data_session_get_available_media (RejillaDataSession *self)
{
	GSList *retval;
	RejillaDataSessionPrivate *priv;

	priv = REJILLA_DATA_SESSION_PRIVATE (self);

	retval = g_slist_copy (priv->media);
	g_slist_foreach (retval, (GFunc) g_object_ref, NULL);

	return retval;
}

RejillaMedium *
rejilla_data_session_get_loaded_medium (RejillaDataSession *self)
{
	RejillaDataSessionPrivate *priv;

	priv = REJILLA_DATA_SESSION_PRIVATE (self);
	if (!priv->media || !priv->nodes)
		return NULL;

	return priv->loaded;
}

static gboolean
rejilla_data_session_is_valid_multi (RejillaMedium *medium)
{
	RejillaMedia media;
	RejillaMedia media_status;

	media = rejilla_medium_get_status (medium);
	media_status = rejilla_burn_library_get_media_capabilities (media);

	return (media_status & REJILLA_MEDIUM_WRITABLE) &&
	       (media & REJILLA_MEDIUM_HAS_DATA) &&
	       (rejilla_medium_get_last_data_track_address (medium, NULL, NULL) != -1);
}

static void
rejilla_data_session_disc_added_cb (RejillaMediumMonitor *monitor,
				    RejillaMedium *medium,
				    RejillaDataSession *self)
{
	RejillaDataSessionPrivate *priv;

	priv = REJILLA_DATA_SESSION_PRIVATE (self);

	if (!rejilla_data_session_is_valid_multi (medium))
		return;

	g_object_ref (medium);
	priv->media = g_slist_prepend (priv->media, medium);

	g_signal_emit (self,
		       rejilla_data_session_signals [AVAILABLE_SIGNAL],
		       0,
		       medium,
		       TRUE);
}

static void
rejilla_data_session_disc_removed_cb (RejillaMediumMonitor *monitor,
				      RejillaMedium *medium,
				      RejillaDataSession *self)
{
	GSList *iter;
	GSList *next;
	RejillaDataSessionPrivate *priv;

	priv = REJILLA_DATA_SESSION_PRIVATE (self);

	/* see if that's the current loaded one */
	if (priv->loaded && priv->loaded == medium)
		rejilla_data_session_remove_last (self);

	/* remove it from our list */
	for (iter = priv->media; iter; iter = next) {
		RejillaMedium *iter_medium;

		iter_medium = iter->data;
		next = iter->next;

		if (medium == iter_medium) {
			g_signal_emit (self,
				       rejilla_data_session_signals [AVAILABLE_SIGNAL],
				       0,
				       medium,
				       FALSE);

			priv->media = g_slist_remove (priv->media, iter_medium);
			g_object_unref (iter_medium);
		}
	}
}

static void
rejilla_data_session_init (RejillaDataSession *object)
{
	GSList *iter, *list;
	RejillaMediumMonitor *monitor;
	RejillaDataSessionPrivate *priv;

	priv = REJILLA_DATA_SESSION_PRIVATE (object);

	monitor = rejilla_medium_monitor_get_default ();
	g_signal_connect (monitor,
			  "medium-added",
			  G_CALLBACK (rejilla_data_session_disc_added_cb),
			  object);
	g_signal_connect (monitor,
			  "medium-removed",
			  G_CALLBACK (rejilla_data_session_disc_removed_cb),
			  object);

	list = rejilla_medium_monitor_get_media (monitor,
						 REJILLA_MEDIA_TYPE_WRITABLE|
						 REJILLA_MEDIA_TYPE_REWRITABLE);
	g_object_unref (monitor);

	/* check for a multisession medium already in */
	for (iter = list; iter; iter = iter->next) {
		RejillaMedium *medium;

		medium = iter->data;
		if (rejilla_data_session_is_valid_multi (medium)) {
			g_object_ref (medium);
			priv->media = g_slist_prepend (priv->media, medium);
		}
	}
	g_slist_foreach (list, (GFunc) g_object_unref, NULL);
	g_slist_free (list);
}

static void
rejilla_data_session_stop_io (RejillaDataSession *self)
{
	RejillaDataSessionPrivate *priv;

	priv = REJILLA_DATA_SESSION_PRIVATE (self);

	if (priv->load_dir) {
		rejilla_io_cancel_by_base (priv->load_dir);
		rejilla_io_job_base_free (priv->load_dir);
		priv->load_dir = NULL;
	}
}

static void
rejilla_data_session_reset (RejillaDataProject *project,
			    guint num_nodes)
{
	rejilla_data_session_stop_io (REJILLA_DATA_SESSION (project));

	/* chain up this function except if we invalidated the node */
	if (REJILLA_DATA_PROJECT_CLASS (rejilla_data_session_parent_class)->reset)
		REJILLA_DATA_PROJECT_CLASS (rejilla_data_session_parent_class)->reset (project, num_nodes);
}

static void
rejilla_data_session_finalize (GObject *object)
{
	RejillaDataSessionPrivate *priv;
	RejillaMediumMonitor *monitor;

	priv = REJILLA_DATA_SESSION_PRIVATE (object);

	monitor = rejilla_medium_monitor_get_default ();
	g_signal_handlers_disconnect_by_func (monitor,
	                                      rejilla_data_session_disc_added_cb,
	                                      object);
	g_signal_handlers_disconnect_by_func (monitor,
	                                      rejilla_data_session_disc_removed_cb,
	                                      object);
	g_object_unref (monitor);

	if (priv->loaded) {
		g_object_unref (priv->loaded);
		priv->loaded = NULL;
	}

	if (priv->media) {
		g_slist_foreach (priv->media, (GFunc) g_object_unref, NULL);
		g_slist_free (priv->media);
		priv->media = NULL;
	}

	if (priv->nodes) {
		g_slist_free (priv->nodes);
		priv->nodes = NULL;
	}

	/* NOTE no need to clean up size_changed_sig since it's connected to 
	 * ourselves. It disappears with use. */

	rejilla_data_session_stop_io (REJILLA_DATA_SESSION (object));

	/* don't care about the nodes since they will be automatically
	 * destroyed */

	G_OBJECT_CLASS (rejilla_data_session_parent_class)->finalize (object);
}


static void
rejilla_data_session_class_init (RejillaDataSessionClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	RejillaDataProjectClass *project_class = REJILLA_DATA_PROJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (RejillaDataSessionPrivate));

	object_class->finalize = rejilla_data_session_finalize;

	project_class->reset = rejilla_data_session_reset;

	rejilla_data_session_signals [AVAILABLE_SIGNAL] = 
	    g_signal_new ("session_available",
			  G_TYPE_FROM_CLASS (klass),
			  G_SIGNAL_RUN_LAST,
			  0,
			  NULL, NULL,
			  rejilla_marshal_VOID__OBJECT_BOOLEAN,
			  G_TYPE_NONE,
			  2,
			  G_TYPE_OBJECT,
			  G_TYPE_BOOLEAN);
	rejilla_data_session_signals [LOADED_SIGNAL] = 
	    g_signal_new ("session_loaded",
			  G_TYPE_FROM_CLASS (klass),
			  G_SIGNAL_RUN_LAST,
			  0,
			  NULL, NULL,
			  rejilla_marshal_VOID__OBJECT_BOOLEAN,
			  G_TYPE_NONE,
			  2,
			  G_TYPE_OBJECT,
			  G_TYPE_BOOLEAN);
}
