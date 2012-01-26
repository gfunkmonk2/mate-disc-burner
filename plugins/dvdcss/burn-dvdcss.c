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
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>
#include <gmodule.h>

#include "rejilla-units.h"

#include "burn-job.h"
#include "rejilla-plugin-registration.h"
#include "burn-dvdcss-private.h"
#include "burn-volume.h"
#include "rejilla-medium.h"
#include "rejilla-track-image.h"
#include "rejilla-track-disc.h"


#define REJILLA_TYPE_DVDCSS         (rejilla_dvdcss_get_type ())
#define REJILLA_DVDCSS(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), REJILLA_TYPE_DVDCSS, RejillaDvdcss))
#define REJILLA_DVDCSS_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), REJILLA_TYPE_DVDCSS, RejillaDvdcssClass))
#define REJILLA_IS_DVDCSS(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), REJILLA_TYPE_DVDCSS))
#define REJILLA_IS_DVDCSS_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), REJILLA_TYPE_DVDCSS))
#define REJILLA_DVDCSS_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), REJILLA_TYPE_DVDCSS, RejillaDvdcssClass))

REJILLA_PLUGIN_BOILERPLATE (RejillaDvdcss, rejilla_dvdcss, REJILLA_TYPE_JOB, RejillaJob);

struct _RejillaDvdcssPrivate {
	GError *error;
	GThread *thread;
	GMutex *mutex;
	GCond *cond;
	guint thread_id;

	guint cancel:1;
};
typedef struct _RejillaDvdcssPrivate RejillaDvdcssPrivate;

#define REJILLA_DVDCSS_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REJILLA_TYPE_DVDCSS, RejillaDvdcssPrivate))

#define REJILLA_DVDCSS_I_BLOCKS	16ULL

static GObjectClass *parent_class = NULL;

static gboolean
rejilla_dvdcss_library_init (RejillaPlugin *plugin)
{
	gpointer address;
	GModule *module;
	gchar *dvdcss_interface_2 = NULL;

	if (css_ready)
		return TRUE;

	/* load libdvdcss library and see the version (mine is 1.2.0) */
	module = g_module_open ("libdvdcss.so.2", G_MODULE_BIND_LOCAL);
	if (!module)
		goto error_doesnt_exist;

	if (!g_module_symbol (module, "dvdcss_interface_2", &address))
		goto error_version;
	dvdcss_interface_2 = address;

	if (!g_module_symbol (module, "dvdcss_open", &address))
		goto error_version;
	dvdcss_open = address;

	if (!g_module_symbol (module, "dvdcss_close", &address))
		goto error_version;
	dvdcss_close = address;

	if (!g_module_symbol (module, "dvdcss_read", &address))
		goto error_version;
	dvdcss_read = address;

	if (!g_module_symbol (module, "dvdcss_seek", &address))
		goto error_version;
	dvdcss_seek = address;

	if (!g_module_symbol (module, "dvdcss_error", &address))
		goto error_version;
	dvdcss_error = address;

	if (plugin) {
		g_module_close (module);
		return TRUE;
	}

	css_ready = TRUE;
	return TRUE;

error_doesnt_exist:
	rejilla_plugin_add_error (plugin,
	                          REJILLA_PLUGIN_ERROR_MISSING_LIBRARY,
	                          "libdvdcss.so.2");
	return FALSE;

error_version:
	rejilla_plugin_add_error (plugin,
	                          REJILLA_PLUGIN_ERROR_LIBRARY_VERSION,
	                          "libdvdcss.so.2");
	g_module_close (module);
	return FALSE;
}

static gboolean
rejilla_dvdcss_thread_finished (gpointer data)
{
	goffset blocks = 0;
	gchar *image = NULL;
	RejillaDvdcss *self = data;
	RejillaDvdcssPrivate *priv;
	RejillaTrackImage *track = NULL;

	priv = REJILLA_DVDCSS_PRIVATE (self);
	priv->thread_id = 0;

	if (priv->error) {
		GError *error;

		error = priv->error;
		priv->error = NULL;
		rejilla_job_error (REJILLA_JOB (self), error);
		return FALSE;
	}

	track = rejilla_track_image_new ();
	rejilla_job_get_image_output (REJILLA_JOB (self),
				      &image,
				      NULL);
	rejilla_track_image_set_source (track,
					image,
					NULL,
					REJILLA_IMAGE_FORMAT_BIN);

	rejilla_job_get_session_output_size (REJILLA_JOB (self), &blocks, NULL);
	rejilla_track_image_set_block_num (track, blocks);

	rejilla_job_add_track (REJILLA_JOB (self), REJILLA_TRACK (track));

	/* It's good practice to unref the track afterwards as we don't need it
	 * anymore. RejillaTaskCtx refs it. */
	g_object_unref (track);

	rejilla_job_finished_track (REJILLA_JOB (self));

	return FALSE;
}

static RejillaBurnResult
rejilla_dvdcss_write_sector_to_fd (RejillaDvdcss *self,
				   gpointer buffer,
				   gint bytes_remaining)
{
	int fd;
	gint bytes_written = 0;
	RejillaDvdcssPrivate *priv;

	priv = REJILLA_DVDCSS_PRIVATE (self);

	rejilla_job_get_fd_out (REJILLA_JOB (self), &fd);
	while (bytes_remaining) {
		gint written;

		written = write (fd,
				 ((gchar *) buffer)  + bytes_written,
				 bytes_remaining);

		if (priv->cancel)
			break;

		if (written != bytes_remaining) {
			if (errno != EINTR && errno != EAGAIN) {
                                int errsv = errno;

				/* unrecoverable error */
				priv->error = g_error_new (REJILLA_BURN_ERROR,
							   REJILLA_BURN_ERROR_GENERAL,
							   _("Data could not be written (%s)"),
							   g_strerror (errsv));
				return REJILLA_BURN_ERR;
			}

			g_thread_yield ();
		}

		if (written > 0) {
			bytes_remaining -= written;
			bytes_written += written;
		}
	}

	return REJILLA_BURN_OK;
}

struct _RejillaScrambledSectorRange {
	gint start;
	gint end;
};
typedef struct _RejillaScrambledSectorRange RejillaScrambledSectorRange;

static gboolean
rejilla_dvdcss_create_scrambled_sectors_map (RejillaDvdcss *self,
                                             GQueue *map,
					     dvdcss_handle *handle,
					     RejillaVolFile *parent,
					     GError **error)
{
	GList *iter;

	/* this allows to cache keys for encrypted files */
	for (iter = parent->specific.dir.children; iter; iter = iter->next) {
		RejillaVolFile *file;

		file = iter->data;
		if (!file->isdir) {
			if (!strncmp (file->name + strlen (file->name) - 6, ".VOB", 4)) {
				RejillaScrambledSectorRange *range;
				gsize current_extent;
				GSList *extents;

				REJILLA_JOB_LOG (self, "Retrieving keys for %s", file->name);

				/* take the first address for each extent of the file */
				if (!file->specific.file.extents) {
					REJILLA_JOB_LOG (self, "Problem: file has no extents");
					return FALSE;
				}

				range = g_new0 (RejillaScrambledSectorRange, 1);
				for (extents = file->specific.file.extents; extents; extents = extents->next) {
					RejillaVolFileExtent *extent;

					extent = extents->data;

					range->start = extent->block;
					range->end = extent->block + REJILLA_BYTES_TO_SECTORS (extent->size, DVDCSS_BLOCK_SIZE);

					REJILLA_JOB_LOG (self, "From 0x%llx to 0x%llx", range->start, range->end);
					g_queue_push_head (map, range);

					if (extent->size == 0) {
						REJILLA_JOB_LOG (self, "0 size extent");
						continue;
					}

					current_extent = dvdcss_seek (handle, range->start, DVDCSS_SEEK_KEY);
					if (current_extent != range->start) {
						REJILLA_JOB_LOG (self, "Problem: could not retrieve key");
						g_set_error (error,
							     REJILLA_BURN_ERROR,
							     REJILLA_BURN_ERROR_GENERAL,
							     _("Error while reading video DVD (%s)"),
							     dvdcss_error (handle));
						return FALSE;
					}
				}
			}
		}
		else if (!rejilla_dvdcss_create_scrambled_sectors_map (self, map, handle, file, error))
			return FALSE;
	}

	return TRUE;
}

static gint
rejilla_dvdcss_sort_ranges (gconstpointer a, gconstpointer b, gpointer user_data)
{
	const RejillaScrambledSectorRange *range_a = a;
	const RejillaScrambledSectorRange *range_b = b;

	return range_a->start - range_b->start;
}

static gpointer
rejilla_dvdcss_write_image_thread (gpointer data)
{
	guchar buf [DVDCSS_BLOCK_SIZE * REJILLA_DVDCSS_I_BLOCKS];
	RejillaScrambledSectorRange *range = NULL;
	RejillaMedium *medium = NULL;
	RejillaVolFile *files = NULL;
	dvdcss_handle *handle = NULL;
	RejillaDrive *drive = NULL;
	RejillaDvdcssPrivate *priv;
	gint64 written_sectors = 0;
	RejillaDvdcss *self = data;
	RejillaTrack *track = NULL;
	guint64 remaining_sectors;
	FILE *output_fd = NULL;
	RejillaVolSrc *vol;
	gint64 volume_size;
	GQueue *map = NULL;

	rejilla_job_set_use_average_rate (REJILLA_JOB (self), TRUE);
	rejilla_job_set_current_action (REJILLA_JOB (self),
					REJILLA_BURN_ACTION_ANALYSING,
					_("Retrieving DVD keys"),
					FALSE);
	rejilla_job_start_progress (REJILLA_JOB (self), FALSE);

	priv = REJILLA_DVDCSS_PRIVATE (self);

	/* get the contents of the DVD */
	rejilla_job_get_current_track (REJILLA_JOB (self), &track);
	drive = rejilla_track_disc_get_drive (REJILLA_TRACK_DISC (track));

	vol = rejilla_volume_source_open_file (rejilla_drive_get_device (drive), &priv->error);
	files = rejilla_volume_get_files (vol,
					  0,
					  NULL,
					  NULL,
					  NULL,
					  &priv->error);
	rejilla_volume_source_close (vol);
	if (!files)
		goto end;

	medium = rejilla_drive_get_medium (drive);
	rejilla_medium_get_data_size (medium, NULL, &volume_size);
	if (volume_size == -1) {
		priv->error = g_error_new (REJILLA_BURN_ERROR,
					   REJILLA_BURN_ERROR_GENERAL,
					   _("The size of the volume could not be retrieved"));
		goto end;
	}

	/* create a handle/open DVD */
	handle = dvdcss_open (rejilla_drive_get_device (drive));
	if (!handle) {
		priv->error = g_error_new (REJILLA_BURN_ERROR,
					   REJILLA_BURN_ERROR_GENERAL,
					   _("Video DVD could not be opened"));
		goto end;
	}

	/* look through the files to get the ranges of encrypted sectors
	 * and cache the CSS keys while at it. */
	map = g_queue_new ();
	if (!rejilla_dvdcss_create_scrambled_sectors_map (self, map, handle, files, &priv->error))
		goto end;

	REJILLA_JOB_LOG (self, "DVD map created (keys retrieved)");

	g_queue_sort (map, rejilla_dvdcss_sort_ranges, NULL);

	rejilla_volume_file_free (files);
	files = NULL;

	if (dvdcss_seek (handle, 0, DVDCSS_NOFLAGS) < 0) {
		REJILLA_JOB_LOG (self, "Error initial seeking");
		priv->error = g_error_new (REJILLA_BURN_ERROR,
					   REJILLA_BURN_ERROR_GENERAL,
					   _("Error while reading video DVD (%s)"),
					   dvdcss_error (handle));
		goto end;
	}

	rejilla_job_set_current_action (REJILLA_JOB (self),
					REJILLA_BURN_ACTION_DRIVE_COPY,
					_("Copying video DVD"),
					FALSE);

	rejilla_job_start_progress (REJILLA_JOB (self), TRUE);

	remaining_sectors = volume_size;
	range = g_queue_pop_head (map);

	if (rejilla_job_get_fd_out (REJILLA_JOB (self), NULL) != REJILLA_BURN_OK) {
		gchar *output = NULL;

		rejilla_job_get_image_output (REJILLA_JOB (self), &output, NULL);
		output_fd = fopen (output, "w");
		if (!output_fd) {
			priv->error = g_error_new_literal (REJILLA_BURN_ERROR,
							   REJILLA_BURN_ERROR_GENERAL,
							   g_strerror (errno));
			g_free (output);
			goto end;
		}
		g_free (output);
	}

	while (remaining_sectors) {
		gint flag;
		guint64 num_blocks, data_size;

		if (priv->cancel)
			break;

		num_blocks = REJILLA_DVDCSS_I_BLOCKS;

		/* see if we are approaching the end of the dvd */
		if (num_blocks > remaining_sectors)
			num_blocks = remaining_sectors;

		/* see if we need to update the key */
		if (!range || written_sectors < range->start) {
			/* this is in a non scrambled sectors range */
			flag = DVDCSS_NOFLAGS;
	
			/* we don't want to mix scrambled and non scrambled sectors */
			if (range && written_sectors + num_blocks > range->start)
				num_blocks = range->start - written_sectors;
		}
		else {
			/* this is in a scrambled sectors range */
			flag = DVDCSS_READ_DECRYPT;

			/* see if we need to update the key */
			if (written_sectors == range->start) {
				int pos;

				pos = dvdcss_seek (handle, written_sectors, DVDCSS_SEEK_KEY);
				if (pos < 0) {
					REJILLA_JOB_LOG (self, "Error seeking");
					priv->error = g_error_new (REJILLA_BURN_ERROR,
								   REJILLA_BURN_ERROR_GENERAL,
								   _("Error while reading video DVD (%s)"),
								   dvdcss_error (handle));
					break;
				}
			}

			/* we don't want to mix scrambled and non scrambled sectors
			 * NOTE: range->end address is the next non scrambled sector */
			if (written_sectors + num_blocks > range->end)
				num_blocks = range->end - written_sectors;

			if (written_sectors + num_blocks == range->end) {
				/* update to get the next range of scrambled sectors */
				g_free (range);
				range = g_queue_pop_head (map);
			}
		}

		num_blocks = dvdcss_read (handle, buf, num_blocks, flag);
		if (num_blocks < 0) {
			REJILLA_JOB_LOG (self, "Error reading");
			priv->error = g_error_new (REJILLA_BURN_ERROR,
						   REJILLA_BURN_ERROR_GENERAL,
						   _("Error while reading video DVD (%s)"),
						   dvdcss_error (handle));
			break;
		}

		data_size = num_blocks * DVDCSS_BLOCK_SIZE;
		if (output_fd) {
			if (fwrite (buf, 1, data_size, output_fd) != data_size) {
                                int errsv = errno;

				priv->error = g_error_new (REJILLA_BURN_ERROR,
							   REJILLA_BURN_ERROR_GENERAL,
							   _("Data could not be written (%s)"),
							   g_strerror (errsv));
				break;
			}
		}
		else {
			RejillaBurnResult result;

			result = rejilla_dvdcss_write_sector_to_fd (self,
								    buf,
								    data_size);
			if (result != REJILLA_BURN_OK)
				break;
		}

		written_sectors += num_blocks;
		remaining_sectors -= num_blocks;
		rejilla_job_set_written_track (REJILLA_JOB (self), written_sectors * DVDCSS_BLOCK_SIZE);
	}

end:

	if (range)
		g_free (range);

	if (handle)
		dvdcss_close (handle);

	if (files)
		rejilla_volume_file_free (files);

	if (output_fd)
		fclose (output_fd);

	if (map) {
		g_queue_foreach (map, (GFunc) g_free, NULL);
		g_queue_free (map);
	}

	if (!priv->cancel)
		priv->thread_id = g_idle_add (rejilla_dvdcss_thread_finished, self);

	/* End thread */
	g_mutex_lock (priv->mutex);
	priv->thread = NULL;
	g_cond_signal (priv->cond);
	g_mutex_unlock (priv->mutex);

	g_thread_exit (NULL);

	return NULL;
}

static RejillaBurnResult
rejilla_dvdcss_start (RejillaJob *job,
		      GError **error)
{
	RejillaDvdcss *self;
	RejillaJobAction action;
	RejillaDvdcssPrivate *priv;
	GError *thread_error = NULL;

	self = REJILLA_DVDCSS (job);
	priv = REJILLA_DVDCSS_PRIVATE (self);

	rejilla_job_get_action (job, &action);
	if (action == REJILLA_JOB_ACTION_SIZE) {
		goffset blocks = 0;
		RejillaTrack *track;

		rejilla_job_get_current_track (job, &track);
		rejilla_track_get_size (track, &blocks, NULL);
		rejilla_job_set_output_size_for_current_track (job,
							       blocks,
							       blocks * DVDCSS_BLOCK_SIZE);
		return REJILLA_BURN_NOT_RUNNING;
	}

	if (action != REJILLA_JOB_ACTION_IMAGE)
		return REJILLA_BURN_NOT_SUPPORTED;

	if (priv->thread)
		return REJILLA_BURN_RUNNING;

	if (!rejilla_dvdcss_library_init (NULL))
		return REJILLA_BURN_ERR;

	g_mutex_lock (priv->mutex);
	priv->thread = g_thread_create (rejilla_dvdcss_write_image_thread,
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

static void
rejilla_dvdcss_stop_real (RejillaDvdcss *self)
{
	RejillaDvdcssPrivate *priv;

	priv = REJILLA_DVDCSS_PRIVATE (self);

	g_mutex_lock (priv->mutex);
	if (priv->thread) {
		priv->cancel = 1;
		g_cond_wait (priv->cond, priv->mutex);
		priv->cancel = 0;
	}
	g_mutex_unlock (priv->mutex);

	if (priv->thread_id) {
		g_source_remove (priv->thread_id);
		priv->thread_id = 0;
	}

	if (priv->error) {
		g_error_free (priv->error);
		priv->error = NULL;
	}
}

static RejillaBurnResult
rejilla_dvdcss_stop (RejillaJob *job,
		     GError **error)
{
	RejillaDvdcss *self;

	self = REJILLA_DVDCSS (job);

	rejilla_dvdcss_stop_real (self);
	return REJILLA_BURN_OK;
}

static void
rejilla_dvdcss_class_init (RejillaDvdcssClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	RejillaJobClass *job_class = REJILLA_JOB_CLASS (klass);

	g_type_class_add_private (klass, sizeof (RejillaDvdcssPrivate));

	parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = rejilla_dvdcss_finalize;

	job_class->start = rejilla_dvdcss_start;
	job_class->stop = rejilla_dvdcss_stop;
}

static void
rejilla_dvdcss_init (RejillaDvdcss *obj)
{
	RejillaDvdcssPrivate *priv;

	priv = REJILLA_DVDCSS_PRIVATE (obj);

	priv->mutex = g_mutex_new ();
	priv->cond = g_cond_new ();
}

static void
rejilla_dvdcss_finalize (GObject *object)
{
	RejillaDvdcssPrivate *priv;

	priv = REJILLA_DVDCSS_PRIVATE (object);

	rejilla_dvdcss_stop_real (REJILLA_DVDCSS (object));

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
rejilla_dvdcss_export_caps (RejillaPlugin *plugin)
{
	GSList *output;
	GSList *input;

	rejilla_plugin_define (plugin,
			       "dvdcss",
	                       NULL,
			       _("Copies CSS encrypted video DVDs to a disc image"),
			       "Philippe Rouquier",
			       0);

	/* to my knowledge, css can only be applied to pressed discs so no need
	 * to specify anything else but ROM */
	output = rejilla_caps_image_new (REJILLA_PLUGIN_IO_ACCEPT_FILE|
					 REJILLA_PLUGIN_IO_ACCEPT_PIPE,
					 REJILLA_IMAGE_FORMAT_BIN);
	input = rejilla_caps_disc_new (REJILLA_MEDIUM_DVD|
				       REJILLA_MEDIUM_DUAL_L|
				       REJILLA_MEDIUM_ROM|
				       REJILLA_MEDIUM_CLOSED|
				       REJILLA_MEDIUM_HAS_DATA|
				       REJILLA_MEDIUM_PROTECTED);

	rejilla_plugin_link_caps (plugin, output, input);

	g_slist_free (input);
	g_slist_free (output);
}

G_MODULE_EXPORT void
rejilla_plugin_check_config (RejillaPlugin *plugin)
{
	rejilla_dvdcss_library_init (plugin);
}
