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
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>

#include <gio/gio.h>

#include <gmodule.h>

#include "rejilla-units.h"
#include "burn-job.h"
#include "rejilla-plugin-registration.h"

#include "rejilla-track.h"
#include "rejilla-track-data.h"
#include "rejilla-track-image.h"


#define REJILLA_TYPE_BURN_URI         (rejilla_burn_uri_get_type ())
#define REJILLA_BURN_URI(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), REJILLA_TYPE_BURN_URI, RejillaBurnURI))
#define REJILLA_BURN_URI_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), REJILLA_TYPE_BURN_URI, RejillaBurnURIClass))
#define REJILLA_IS_BURN_URI(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), REJILLA_TYPE_BURN_URI))
#define REJILLA_IS_BURN_URI_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), REJILLA_TYPE_BURN_URI))
#define REJILLA_BURN_URI_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), REJILLA_TYPE_BURN_URI, RejillaBurnURIClass))

REJILLA_PLUGIN_BOILERPLATE (RejillaBurnURI, rejilla_burn_uri, REJILLA_TYPE_JOB, RejillaJob);

struct _RejillaBurnURIPrivate {
	GCancellable *cancel;

	RejillaTrack *track;

	guint thread_id;
	GThread *thread;
	GMutex *mutex;
	GCond *cond;

	GError *error;
};
typedef struct _RejillaBurnURIPrivate RejillaBurnURIPrivate;

#define REJILLA_BURN_URI_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REJILLA_TYPE_BURN_URI, RejillaBurnURIPrivate))

static GObjectClass *parent_class = NULL;


static gboolean
rejilla_burn_uri_thread_finished (RejillaBurnURI *self)
{
	RejillaBurnURIPrivate *priv;

	priv = REJILLA_BURN_URI_PRIVATE (self);

	priv->thread_id = 0;

	if (priv->cancel) {
		g_object_unref (priv->cancel);
		priv->cancel = NULL;
		if (g_cancellable_is_cancelled (priv->cancel))
			return FALSE;
	}

	if (priv->error) {
		GError *error;

		error = priv->error;
		priv->error = NULL;
		rejilla_job_error (REJILLA_JOB (self), error);
		return FALSE;
	}

	rejilla_job_add_track (REJILLA_JOB (self), priv->track);
	rejilla_job_finished_track (REJILLA_JOB (self));

	return FALSE;
}

static gint
rejilla_burn_uri_find_graft (gconstpointer A, gconstpointer B)
{
	const RejillaGraftPt *graft = A;

	if (graft && graft->path)
		return strcmp (graft->path, B);

	return 1;
}

static GSList *
rejilla_burn_uri_explore_directory (RejillaBurnURI *self,
				    GSList *grafts,
				    GFile *file,
				    const gchar *path,
				    GCancellable *cancel,
				    GError **error)
{
	RejillaTrack *current = NULL;
	GFileEnumerator *enumerator;
	GSList *current_grafts;
	GFileInfo *info;

	enumerator = g_file_enumerate_children (file,
						G_FILE_ATTRIBUTE_STANDARD_NAME ","
						G_FILE_ATTRIBUTE_STANDARD_TYPE ","
						"burn::backing-file",
						G_FILE_QUERY_INFO_NONE,
						cancel,
						error);

	if (!enumerator) {
		g_slist_foreach (grafts, (GFunc) rejilla_graft_point_free, NULL);
		g_slist_free (grafts);
		return NULL;
	}

	rejilla_job_get_current_track (REJILLA_JOB (self), &current);
	current_grafts = rejilla_track_data_get_grafts (REJILLA_TRACK_DATA (current));

	while ((info = g_file_enumerator_next_file (enumerator, cancel, error))) {
		if (g_file_info_get_file_type (info) == G_FILE_TYPE_DIRECTORY) {
			gchar *disc_path;
			GFile *directory;
			RejillaGraftPt *graft;

			/* Make sure it's not one of the original grafts */
			/* we need to know if that's a directory or not since if
			 * it is then mkisofs (but not genisoimage) requires the
			 * disc path to end with '/'; if there isn't '/' at the 
			 * end then only the directory contents are added. */
			disc_path = g_build_filename (path, g_file_info_get_name (info), G_DIR_SEPARATOR_S, NULL);
			if (g_slist_find_custom (current_grafts, disc_path, (GCompareFunc) rejilla_burn_uri_find_graft)) {
				REJILLA_JOB_LOG (self, "Graft already in list %s", disc_path);
				g_object_unref (info);
				g_free (disc_path);
				continue;
			}

			/* we need a dummy directory */
			graft = g_new0 (RejillaGraftPt, 1);
			graft->uri = NULL;
			graft->path = disc_path;
			grafts = g_slist_prepend (grafts, graft);

			REJILLA_JOB_LOG (self, "Adding directory %s at %s", graft->uri, graft->path);

			directory = g_file_get_child (file, g_file_info_get_name (info));
			grafts = rejilla_burn_uri_explore_directory (self,
								     grafts,
								     directory,
								     graft->path,
								     cancel,
								     error);
			g_object_unref (directory);

			if (!grafts) {
				g_object_unref (info);
				g_object_unref (enumerator);
				return NULL;
			}
		}
		else if (g_file_info_get_file_type (info) == G_FILE_TYPE_REGULAR
		     /* NOTE: burn:// URI allows symlink */
		     ||  g_file_info_get_file_type (info) == G_FILE_TYPE_SYMBOLIC_LINK) {
			const gchar *real_path;
			RejillaGraftPt *graft;
			gchar *disc_path;

			real_path = g_file_info_get_attribute_byte_string (info, "burn::backing-file");
			if (!real_path) {
				g_set_error (error,
					     REJILLA_BURN_ERROR,
					     REJILLA_BURN_ERROR_GENERAL,
					     _("Impossible to retrieve local file path"));

				g_slist_foreach (grafts, (GFunc) rejilla_graft_point_free, NULL);
				g_slist_free (grafts);
				g_object_unref (info);
				g_object_unref (file);
				return NULL;
			}

			/* Make sure it's not one of the original grafts */
			disc_path = g_build_filename (path, g_file_info_get_name (info), NULL);
			if (g_slist_find_custom (current_grafts, disc_path, (GCompareFunc) rejilla_burn_uri_find_graft)) {
				REJILLA_JOB_LOG (self, "Graft already in list %s", disc_path);
				g_object_unref (info);
				g_free (disc_path);
				continue;
			}

			graft = g_new0 (RejillaGraftPt, 1);
			graft->path = disc_path;
			graft->uri = g_strdup (real_path);
			/* FIXME: maybe one day, graft->uri will always be an URI */
			/* graft->uri = g_filename_to_uri (real_path, NULL, NULL); */

			/* Make sure it's not one of the original grafts */
			
			grafts = g_slist_prepend (grafts, graft);

			REJILLA_JOB_LOG (self, "Added file %s at %s", graft->uri, graft->path);
		}

		g_object_unref (info);
	}
	g_object_unref (enumerator);

	return grafts;
}

static gboolean
rejilla_burn_uri_retrieve_path (RejillaBurnURI *self,
				const gchar *uri,
				gchar **path)
{
	GFile *file;
	GFileInfo *info;
	RejillaBurnURIPrivate *priv;

	priv = REJILLA_BURN_URI_PRIVATE (self);

	if (!uri)
		return FALSE;

	file = g_file_new_for_uri (uri);
	info = g_file_query_info (file,
				  G_FILE_ATTRIBUTE_STANDARD_NAME ","
				  G_FILE_ATTRIBUTE_STANDARD_TYPE ","
				  "burn::backing-file",
				  G_FILE_QUERY_INFO_NONE,
				  priv->cancel,
				  &priv->error);

	if (priv->error) {
		g_object_unref (file);
		return FALSE;
	}

	if (g_cancellable_is_cancelled (priv->cancel)) {
		g_object_unref (file);
		return FALSE;
	}

	if (!info) {
		/* Error */
		g_object_unref (file);
		g_object_unref (info);
		return FALSE;
	}
		
	if (g_file_info_get_file_type (info) == G_FILE_TYPE_DIRECTORY) {
		*path = NULL;
	}
	else if (g_file_info_get_file_type (info) == G_FILE_TYPE_REGULAR
	     /* NOTE: burn:// URI allows symlink */
	     ||  g_file_info_get_file_type (info) == G_FILE_TYPE_SYMBOLIC_LINK) {
		const gchar *real_path;

		real_path = g_file_info_get_attribute_byte_string (info, "burn::backing-file");
		if (!real_path) {
			priv->error = g_error_new (REJILLA_BURN_ERROR,
						   REJILLA_BURN_ERROR_GENERAL,
						   _("Impossible to retrieve local file path"));
			g_object_unref (info);
			g_object_unref (file);
			return FALSE;
		}

		*path = g_strdup (real_path);
	}

	g_object_unref (file);
	g_object_unref (info);
	return TRUE;
}

static gpointer
rejilla_burn_uri_thread (gpointer data)
{
	RejillaBurnURI *self = REJILLA_BURN_URI (data);
	RejillaTrack *current = NULL;
	RejillaBurnURIPrivate *priv;
	RejillaTrackData *track;
	GSList *excluded = NULL;
	GSList *grafts = NULL;
	guint64 num = 0;
	GSList *src;

	priv = REJILLA_BURN_URI_PRIVATE (self);
	rejilla_job_set_current_action (REJILLA_JOB (self),
					REJILLA_BURN_ACTION_FILE_COPY,
					_("Copying files locally"),
					TRUE);

	rejilla_job_get_current_track (REJILLA_JOB (self), &current);

	/* This is for IMAGE tracks */
	if (REJILLA_IS_TRACK_IMAGE (current)) {
		gchar *uri;
		gchar *path_toc;
		gchar *path_image;
		goffset blocks = 0;
		RejillaTrackImage *image;

		path_image = NULL;
		uri = rejilla_track_image_get_source (REJILLA_TRACK_IMAGE (current), TRUE);
		if (!rejilla_burn_uri_retrieve_path (self, uri, &path_image)) {
			g_free (uri);
			goto end;
		}
		g_free (uri);

		path_toc = NULL;
		uri = rejilla_track_image_get_toc_source (REJILLA_TRACK_IMAGE (current), TRUE);
		if (uri) {
			/* NOTE: if it's a .bin image there is not .toc file */
			if (!rejilla_burn_uri_retrieve_path (self, uri, &path_toc)) {
				g_free (path_image);
				g_free (uri);
				goto end;
			}
			g_free (uri);
		}

		rejilla_track_get_size (current, &blocks, NULL);

		image = rejilla_track_image_new ();
		rejilla_track_tag_copy_missing (REJILLA_TRACK (image), current);
		rejilla_track_image_set_source (image,
						path_image,
						path_toc,
						rejilla_track_image_get_format (REJILLA_TRACK_IMAGE (current)));
		rejilla_track_image_set_block_num (image, blocks);

		priv->track = REJILLA_TRACK (image);

		g_free (path_toc);
		g_free (path_image);
		goto end;
	}

	/* This is for DATA tracks */
	for (src = rejilla_track_data_get_grafts (REJILLA_TRACK_DATA (current)); src; src = src->next) {
		GFile *file;
		GFileInfo *info;
		RejillaGraftPt *graft;

		graft = src->data;

		if (!graft->uri) {
			grafts = g_slist_prepend (grafts, rejilla_graft_point_copy (graft));
			continue;
		}

		if (!g_str_has_prefix (graft->uri, "burn://")) {
			grafts = g_slist_prepend (grafts, rejilla_graft_point_copy (graft));
			continue;
		}

		REJILLA_JOB_LOG (self, "Information retrieval for %s", graft->uri);

		file = g_file_new_for_uri (graft->uri);
		info = g_file_query_info (file,
					  G_FILE_ATTRIBUTE_STANDARD_NAME ","
					  G_FILE_ATTRIBUTE_STANDARD_TYPE ","
					  "burn::backing-file",
					  G_FILE_QUERY_INFO_NONE,
					  priv->cancel,
					  &priv->error);

		if (priv->error) {
			g_object_unref (file);
			goto end;
		}

		if (g_cancellable_is_cancelled (priv->cancel)) {
			g_object_unref (file);
			goto end;
		}

		if (!info) {
			/* Error */
			g_object_unref (file);
			g_object_unref (info);
			goto end;
		}

		/* See if we were passed the burn:/// uri itself (the root).
		 * Then skip graft point addition */
		if (g_file_info_get_file_type (info) == G_FILE_TYPE_DIRECTORY) {
			if (g_file_info_get_name (info)
			&&  strcmp (g_file_info_get_name (info), "/")) {
				RejillaGraftPt *newgraft;

				/* we need a dummy directory */
				newgraft = g_new0 (RejillaGraftPt, 1);
				newgraft->uri = NULL;
				newgraft->path = g_strdup (graft->path);
				grafts = g_slist_prepend (grafts, newgraft);

				REJILLA_JOB_LOG (self,
						 "Adding directory %s at %s",
						 newgraft->uri,
						 newgraft->path);
				grafts = rejilla_burn_uri_explore_directory (self,
									     grafts,
									     file,
									     newgraft->path,
									     priv->cancel,
									     &priv->error);
			}
			else {
				REJILLA_JOB_LOG (self, "Directory is root");
				grafts = rejilla_burn_uri_explore_directory (self,
									     grafts,
									     file,
									     "/",
									     priv->cancel,
									     &priv->error);
			}

			if (!grafts) {
				g_object_unref (info);
				g_object_unref (file);
				goto end;
			}
		}
		else if (g_file_info_get_file_type (info) == G_FILE_TYPE_REGULAR
		     /* NOTE: burn:// URI allows symlink */
		     ||  g_file_info_get_file_type (info) == G_FILE_TYPE_SYMBOLIC_LINK) {
			const gchar *real_path;
			RejillaGraftPt *newgraft;

			real_path = g_file_info_get_attribute_byte_string (info, "burn::backing-file");
			if (!real_path) {
				priv->error = g_error_new (REJILLA_BURN_ERROR,
							   REJILLA_BURN_ERROR_GENERAL,
							   _("Impossible to retrieve local file path"));

				g_slist_foreach (grafts, (GFunc) rejilla_graft_point_free, NULL);
				g_slist_free (grafts);
				g_object_unref (info);
				g_object_unref (file);
				goto end;
			}

			newgraft = rejilla_graft_point_copy (graft);
			g_free (newgraft->uri);

			newgraft->uri = g_strdup (real_path);
			/* FIXME: maybe one day, graft->uri will always be an URI */
			/* newgraft->uri = g_filename_to_uri (real_path, NULL, NULL); */

			REJILLA_JOB_LOG (self,
					 "Added file %s at %s",
					 newgraft->uri,
					 newgraft->path);
			grafts = g_slist_prepend (grafts, newgraft);
		}

		g_object_unref (info);
		g_object_unref (file);
	}
	grafts = g_slist_reverse (grafts);

	/* remove all excluded starting by burn:// from the list */
	for (src = rejilla_track_data_get_excluded_list (REJILLA_TRACK_DATA (current)); src; src = src->next) {
		gchar *uri;

		uri = src->data;

		if (uri && g_str_has_prefix (uri, "burn://"))
			continue;

		uri = g_strdup (uri);
		excluded = g_slist_prepend (excluded, uri);

		REJILLA_JOB_LOG (self, "Added excluded file %s", uri);
	}
	excluded = g_slist_reverse (excluded);

	track = rejilla_track_data_new ();
	rejilla_track_tag_copy_missing (REJILLA_TRACK (track), current);
	
	rejilla_track_data_add_fs (track, rejilla_track_data_get_fs (REJILLA_TRACK_DATA (current)));

	rejilla_track_data_get_file_num (REJILLA_TRACK_DATA (current), &num);
	rejilla_track_data_set_file_num (track, num);

	rejilla_track_data_set_source (track,
				       grafts,
				       excluded);
	priv->track = REJILLA_TRACK (track);

end:

	if (!g_cancellable_is_cancelled (priv->cancel))
		priv->thread_id = g_idle_add ((GSourceFunc) rejilla_burn_uri_thread_finished, self);

	/* End thread */
	g_mutex_lock (priv->mutex);
	g_atomic_pointer_set (&priv->thread, NULL);
	g_cond_signal (priv->cond);
	g_mutex_unlock (priv->mutex);

	g_thread_exit (NULL);

	return NULL;
}

static RejillaBurnResult
rejilla_burn_uri_start_thread (RejillaBurnURI *self,
			       GError **error)
{
	RejillaBurnURIPrivate *priv;
	GError *thread_error = NULL;

	priv = REJILLA_BURN_URI_PRIVATE (self);

	if (priv->thread)
		return REJILLA_BURN_RUNNING;

	priv->cancel = g_cancellable_new ();

	g_mutex_lock (priv->mutex);
	priv->thread = g_thread_create (rejilla_burn_uri_thread,
					self,
					FALSE,
					&thread_error);
	g_mutex_unlock (priv->mutex);

	/* Reminder: this is not necessarily an error as the thread may have finished */
	//if (!priv->thread)
	//	return REJILLA_BURN_ERR;
	if (thread_error) {
		g_propagate_error (error, thread_error);
		return REJILLA_BURN_ERR;
	}

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_burn_uri_start_if_found (RejillaBurnURI *self,
				 const gchar *uri,
				 GError **error)
{
	if (!uri)
		return REJILLA_BURN_NOT_RUNNING;

	/* Find any graft point with burn:// URI */
	if (!g_str_has_prefix (uri, "burn://"))
		return REJILLA_BURN_NOT_RUNNING;

	REJILLA_JOB_LOG (self, "burn:// URI found %s", uri);
	rejilla_burn_uri_start_thread (self, error);
	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_burn_uri_start (RejillaJob *job,
			GError **error)
{
	RejillaBurnURIPrivate *priv;
	RejillaBurnResult result;
	RejillaJobAction action;
	RejillaBurnURI *self;
	RejillaTrack *track;
	GSList *grafts;
	gchar *uri;

	self = REJILLA_BURN_URI (job);
	priv = REJILLA_BURN_URI_PRIVATE (self);

	/* skip that part */
	rejilla_job_get_action (job, &action);
	if (action == REJILLA_JOB_ACTION_SIZE) {
		/* say we won't write to disc */
		rejilla_job_set_output_size_for_current_track (job, 0, 0);
		return REJILLA_BURN_NOT_RUNNING;
	}

	if (action != REJILLA_JOB_ACTION_IMAGE)
		return REJILLA_BURN_NOT_SUPPORTED;

	/* can't be piped so rejilla_job_get_current_track will work */
	rejilla_job_get_current_track (job, &track);

	result = REJILLA_BURN_NOT_RUNNING;

	/* make a list of all non local uris to be downloaded and put them in a
	 * list to avoid to download the same file twice. */
	if (REJILLA_IS_TRACK_DATA (track)) {
		/* we put all the non local graft point uris in the hash */
		grafts = rejilla_track_data_get_grafts (REJILLA_TRACK_DATA (track));
		for (; grafts; grafts = grafts->next) {
			RejillaGraftPt *graft;

			graft = grafts->data;
			result = rejilla_burn_uri_start_if_found (self, graft->uri, error);
			if (result != REJILLA_BURN_NOT_RUNNING)
				break;
		}
	}
	else if (REJILLA_IS_TRACK_IMAGE (track)) {
		/* NOTE: don't delete URI as they will be inserted in hash */
		uri = rejilla_track_image_get_source (REJILLA_TRACK_IMAGE (track), TRUE);
		result = rejilla_burn_uri_start_if_found (self, uri, error);
		g_free (uri);

		if (result == REJILLA_BURN_NOT_RUNNING) {
			uri = rejilla_track_image_get_toc_source (REJILLA_TRACK_IMAGE (track), TRUE);
			result = rejilla_burn_uri_start_if_found (self, uri, error);
			g_free (uri);
		}
	}
	else
		REJILLA_JOB_NOT_SUPPORTED (self);

	if (!priv->thread)
		REJILLA_JOB_LOG (self, "no burn:// URI found");

	return result;
}

static RejillaBurnResult
rejilla_burn_uri_stop (RejillaJob *job,
		       GError **error)
{
	RejillaBurnURIPrivate *priv = REJILLA_BURN_URI_PRIVATE (job);

	if (priv->cancel) {
		/* signal that we've been cancelled */
		g_cancellable_cancel (priv->cancel);
	}

	g_mutex_lock (priv->mutex);
	if (priv->thread)
		g_cond_wait (priv->cond, priv->mutex);
	g_mutex_unlock (priv->mutex);

	if (priv->cancel) {
		/* unref it after the thread has stopped */
		g_object_unref (priv->cancel);
		priv->cancel = NULL;
	}

	if (priv->thread_id) {
		g_source_remove (priv->thread_id);
		priv->thread_id = 0;
	}

	if (priv->error) {
		g_error_free (priv->error);
		priv->error = NULL;
	}

	return REJILLA_BURN_OK;
}

static void
rejilla_burn_uri_finalize (GObject *object)
{
	RejillaBurnURIPrivate *priv = REJILLA_BURN_URI_PRIVATE (object);

	if (priv->mutex) {
		g_mutex_free (priv->mutex);
		priv->mutex = NULL;
	}

	if (priv->cond) {
		g_cond_free (priv->cond);
		priv->cond = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
rejilla_burn_uri_class_init (RejillaBurnURIClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	RejillaJobClass *job_class = REJILLA_JOB_CLASS (klass);

	g_type_class_add_private (klass, sizeof (RejillaBurnURIPrivate));

	parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = rejilla_burn_uri_finalize;

	job_class->start = rejilla_burn_uri_start;
	job_class->stop = rejilla_burn_uri_stop;
}

static void
rejilla_burn_uri_init (RejillaBurnURI *obj)
{
	RejillaBurnURIPrivate *priv = REJILLA_BURN_URI_PRIVATE (obj);

	priv->mutex = g_mutex_new ();
	priv->cond = g_cond_new ();
}

static void
rejilla_burn_uri_export_caps (RejillaPlugin *plugin)
{
	GSList *caps;

	rejilla_plugin_define (plugin,
	                       "burn-uri",
			       /* Translators: this is the name of the plugin
				* which will be translated only when it needs
				* displaying. */
			       N_("CD/DVD Creator Folder"),
			       _("Allows files added to the \"CD/DVD Creator Folder\" in Caja to be burned"),
			       "Philippe Rouquier",
			       11);

	caps = rejilla_caps_image_new (REJILLA_PLUGIN_IO_ACCEPT_FILE,
				       REJILLA_IMAGE_FORMAT_ANY);
	rejilla_plugin_process_caps (plugin, caps);
	g_slist_free (caps);

	caps = rejilla_caps_data_new (REJILLA_IMAGE_FS_ANY);
	rejilla_plugin_process_caps (plugin, caps);
	g_slist_free (caps);

	rejilla_plugin_set_process_flags (plugin, REJILLA_PLUGIN_RUN_PREPROCESSING);
}
