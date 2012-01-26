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

#ifndef JOB_H
#define JOB_H

#include <glib.h>
#include <glib-object.h>

#include "rejilla-track.h"

G_BEGIN_DECLS

#define REJILLA_TYPE_JOB         (rejilla_job_get_type ())
#define REJILLA_JOB(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), REJILLA_TYPE_JOB, RejillaJob))
#define REJILLA_JOB_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), REJILLA_TYPE_JOB, RejillaJobClass))
#define REJILLA_IS_JOB(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), REJILLA_TYPE_JOB))
#define REJILLA_IS_JOB_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), REJILLA_TYPE_JOB))
#define REJILLA_JOB_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), REJILLA_TYPE_JOB, RejillaJobClass))

typedef enum {
	REJILLA_JOB_ACTION_NONE		= 0,
	REJILLA_JOB_ACTION_SIZE,
	REJILLA_JOB_ACTION_IMAGE,
	REJILLA_JOB_ACTION_RECORD,
	REJILLA_JOB_ACTION_ERASE,
	REJILLA_JOB_ACTION_CHECKSUM
} RejillaJobAction;

typedef struct {
	GObject parent;
} RejillaJob;

typedef struct {
	GObjectClass parent_class;

	/**
	 * Virtual functions to implement in each object deriving from
	 * RejillaJob.
	 */

	/**
	 * This function is not compulsory. If not implemented then the library
	 * will act as if OK had been returned.
	 * returns 	REJILLA_BURN_OK on successful initialization
	 *		The job can return REJILLA_BURN_NOT_RUNNING if it should
	 *		not be started.
	 * 		REJILLA_BURN_ERR otherwise
	 */
	RejillaBurnResult	(*activate)		(RejillaJob *job,
							 GError **error);

	/**
	 * This function is compulsory.
	 * returns 	REJILLA_BURN_OK if a loop should be run afterward
	 *		The job can return REJILLA_BURN_NOT_RUNNING if it already
	 *		completed successfully the task or don't need to be run. In this
	 *		case, it's the whole task that will be skipped.
	 *		NOT_SUPPORTED if it can't do the action required. When running
	 *		in fake mode (to get size mostly), the job will be "forgiven" and
	 *		deactivated.
	 *		RETRY if the job is not able to start at the moment but should
	 *		be given another chance later.
	 * 		ERR otherwise
	 */
	RejillaBurnResult	(*start)		(RejillaJob *job,
							 GError **error);

	RejillaBurnResult	(*clock_tick)		(RejillaJob *job);

	RejillaBurnResult	(*stop)			(RejillaJob *job,
							 GError **error);

	/**
	 * you should not connect to this signal. It's used internally to 
	 * "autoconfigure" the backend when an error occurs
	 */
	RejillaBurnResult	(*error)		(RejillaJob *job,
							 RejillaBurnError error);
} RejillaJobClass;

GType rejilla_job_get_type (void);

/**
 * These functions are to be used to get information for running jobs.
 * They are only available when a job is running.
 */

RejillaBurnResult
rejilla_job_set_nonblocking (RejillaJob *self,
			     GError **error);

RejillaBurnResult
rejilla_job_get_action (RejillaJob *job, RejillaJobAction *action);

RejillaBurnResult
rejilla_job_get_flags (RejillaJob *job, RejillaBurnFlag *flags);

RejillaBurnResult
rejilla_job_get_fd_in (RejillaJob *job, int *fd_in);

RejillaBurnResult
rejilla_job_get_tracks (RejillaJob *job, GSList **tracks);

RejillaBurnResult
rejilla_job_get_done_tracks (RejillaJob *job, GSList **tracks);

RejillaBurnResult
rejilla_job_get_current_track (RejillaJob *job, RejillaTrack **track);

RejillaBurnResult
rejilla_job_get_input_type (RejillaJob *job, RejillaTrackType *type);

RejillaBurnResult
rejilla_job_get_audio_title (RejillaJob *job, gchar **album);

RejillaBurnResult
rejilla_job_get_data_label (RejillaJob *job, gchar **label);

RejillaBurnResult
rejilla_job_get_session_output_size (RejillaJob *job,
				     goffset *blocks,
				     goffset *bytes);

/**
 * Used to get information of the destination media
 */

RejillaBurnResult
rejilla_job_get_medium (RejillaJob *job, RejillaMedium **medium);

RejillaBurnResult
rejilla_job_get_bus_target_lun (RejillaJob *job, gchar **BTL);

RejillaBurnResult
rejilla_job_get_device (RejillaJob *job, gchar **device);

RejillaBurnResult
rejilla_job_get_media (RejillaJob *job, RejillaMedia *media);

RejillaBurnResult
rejilla_job_get_last_session_address (RejillaJob *job, goffset *address);

RejillaBurnResult
rejilla_job_get_next_writable_address (RejillaJob *job, goffset *address);

RejillaBurnResult
rejilla_job_get_rate (RejillaJob *job, guint64 *rate);

RejillaBurnResult
rejilla_job_get_speed (RejillaJob *self, guint *speed);

RejillaBurnResult
rejilla_job_get_max_rate (RejillaJob *job, guint64 *rate);

RejillaBurnResult
rejilla_job_get_max_speed (RejillaJob *job, guint *speed);

/**
 * necessary for objects imaging either to another or to a file
 */

RejillaBurnResult
rejilla_job_get_output_type (RejillaJob *job, RejillaTrackType *type);

RejillaBurnResult
rejilla_job_get_fd_out (RejillaJob *job, int *fd_out);

RejillaBurnResult
rejilla_job_get_image_output (RejillaJob *job,
			      gchar **image,
			      gchar **toc);
RejillaBurnResult
rejilla_job_get_audio_output (RejillaJob *job,
			      gchar **output);

/**
 * get a temporary file that will be deleted once the session is finished
 */
 
RejillaBurnResult
rejilla_job_get_tmp_file (RejillaJob *job,
			  const gchar *suffix,
			  gchar **output,
			  GError **error);

RejillaBurnResult
rejilla_job_get_tmp_dir (RejillaJob *job,
			 gchar **path,
			 GError **error);

/**
 * Each tag can be retrieved by any job
 */

RejillaBurnResult
rejilla_job_tag_lookup (RejillaJob *job,
			const gchar *tag,
			GValue **value);

RejillaBurnResult
rejilla_job_tag_add (RejillaJob *job,
		     const gchar *tag,
		     GValue *value);

/**
 * Used to give job results and tell when a job has finished
 */

RejillaBurnResult
rejilla_job_add_track (RejillaJob *job,
		       RejillaTrack *track);

RejillaBurnResult
rejilla_job_finished_track (RejillaJob *job);

RejillaBurnResult
rejilla_job_finished_session (RejillaJob *job);

RejillaBurnResult
rejilla_job_error (RejillaJob *job,
		   GError *error);

/**
 * Used to start progress reporting and starts an internal timer to keep track
 * of remaining time among other things
 */

RejillaBurnResult
rejilla_job_start_progress (RejillaJob *job,
			    gboolean force);

/**
 * task progress report: you can use only some of these functions
 */

RejillaBurnResult
rejilla_job_set_rate (RejillaJob *job,
		      gint64 rate);
RejillaBurnResult
rejilla_job_set_written_track (RejillaJob *job,
			       goffset written);
RejillaBurnResult
rejilla_job_set_written_session (RejillaJob *job,
				 goffset written);
RejillaBurnResult
rejilla_job_set_progress (RejillaJob *job,
			  gdouble progress);
RejillaBurnResult
rejilla_job_reset_progress (RejillaJob *job);
RejillaBurnResult
rejilla_job_set_current_action (RejillaJob *job,
				RejillaBurnAction action,
				const gchar *string,
				gboolean force);
RejillaBurnResult
rejilla_job_get_current_action (RejillaJob *job,
				RejillaBurnAction *action);
RejillaBurnResult
rejilla_job_set_output_size_for_current_track (RejillaJob *job,
					       goffset sectors,
					       goffset bytes);

/**
 * Used to tell it's (or not) dangerous to interrupt this job
 */

void
rejilla_job_set_dangerous (RejillaJob *job, gboolean value);

/**
 * This is for apps with a jerky current rate (like cdrdao)
 */

RejillaBurnResult
rejilla_job_set_use_average_rate (RejillaJob *job,
				  gboolean value);

/**
 * logging facilities
 */

void
rejilla_job_log_message (RejillaJob *job,
			 const gchar *location,
			 const gchar *format,
			 ...);

#define REJILLA_JOB_LOG(job, message, ...) 			\
{								\
	gchar *format;						\
	format = g_strdup_printf ("%s %s",			\
				  G_OBJECT_TYPE_NAME (job),	\
				  message);			\
	rejilla_job_log_message (REJILLA_JOB (job),		\
				 G_STRLOC,			\
				 format,		 	\
				 ##__VA_ARGS__);		\
	g_free (format);					\
}
#define REJILLA_JOB_LOG_ARG(job, message, ...)			\
{								\
	gchar *format;						\
	format = g_strdup_printf ("\t%s",			\
				  (gchar*) message);		\
	rejilla_job_log_message (REJILLA_JOB (job),		\
				 G_STRLOC,			\
				 format,			\
				 ##__VA_ARGS__);		\
	g_free (format);					\
}

#define REJILLA_JOB_NOT_SUPPORTED(job) 					\
	{								\
		REJILLA_JOB_LOG (job, "unsupported operation");		\
		return REJILLA_BURN_NOT_SUPPORTED;			\
	}

#define REJILLA_JOB_NOT_READY(job)						\
	{									\
		REJILLA_JOB_LOG (job, "not ready to operate");	\
		return REJILLA_BURN_NOT_READY;					\
	}


G_END_DECLS

#endif /* JOB_H */
