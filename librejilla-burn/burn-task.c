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

#include <gdk/gdk.h>

#include "burn-basics.h"
#include "burn-debug.h"
#include "rejilla-session.h"
#include "burn-task.h"
#include "burn-task-item.h"
#include "burn-task-ctx.h"

#include "rejilla-track-image.h"
#include "rejilla-track-stream.h"

static void rejilla_task_class_init (RejillaTaskClass *klass);
static void rejilla_task_init (RejillaTask *sp);
static void rejilla_task_finalize (GObject *object);

typedef struct _RejillaTaskPrivate RejillaTaskPrivate;

struct _RejillaTaskPrivate {
	/* The loop for the task */
	GMainLoop *loop;

	/* used to poll for progress (every 0.5 sec) */
	gint clock_id;

	RejillaTaskItem *leader;
	RejillaTaskItem *first;

	/* result of the task */
	RejillaBurnResult retval;
	GError *error;
};

#define REJILLA_TASK_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REJILLA_TYPE_TASK, RejillaTaskPrivate))
G_DEFINE_TYPE (RejillaTask, rejilla_task, REJILLA_TYPE_TASK_CTX);

static GObjectClass *parent_class = NULL;

#define MAX_JOB_START_ATTEMPTS			5
#define JOB_ATTEMPTS_WAIT_TIME			1

void
rejilla_task_add_item (RejillaTask *task, RejillaTaskItem *item)
{
	RejillaTaskPrivate *priv;

	g_return_if_fail (REJILLA_IS_TASK (task));
	g_return_if_fail (REJILLA_IS_TASK_ITEM (item));

	priv = REJILLA_TASK_PRIVATE (task);

	if (priv->leader) {
		rejilla_task_item_link (priv->leader, item);
		g_object_unref (priv->leader);
	}

	if (!priv->first)
		priv->first = item;

	priv->leader = item;
	g_object_ref (priv->leader);
}

static void
rejilla_task_reset_real (RejillaTask *task)
{
	RejillaTaskPrivate *priv;

	priv = REJILLA_TASK_PRIVATE (task);

	if (priv->loop)
		g_main_loop_unref (priv->loop);

	priv->loop = NULL;
	priv->clock_id = 0;
	priv->retval = REJILLA_BURN_OK;

	if (priv->error) {
		g_error_free (priv->error);
		priv->error = NULL;
	}

	rejilla_task_ctx_reset (REJILLA_TASK_CTX (task));
}

void
rejilla_task_reset (RejillaTask *task)
{
	RejillaTaskPrivate *priv;

	priv = REJILLA_TASK_PRIVATE (task);

	if (rejilla_task_is_running (task))
		rejilla_task_cancel (task, TRUE);

	g_object_unref (priv->leader);
	rejilla_task_reset_real (task);
}

static gboolean
rejilla_task_clock_tick (gpointer data)
{
	RejillaTask *task = REJILLA_TASK (data);
	RejillaTaskPrivate *priv;
	RejillaTaskItem *item;

	priv = REJILLA_TASK_PRIVATE (task);

	/* some jobs need to be called periodically to update their status
	 * because the main process run in a thread. We do it before calling
	 * progress/action changed so they can update the task on time */
	for (item = priv->leader; item; item = rejilla_task_item_previous (item)) {
		RejillaTaskItemIFace *klass;

		klass = REJILLA_TASK_ITEM_GET_CLASS (item);
		if (klass->clock_tick)
			klass->clock_tick (item, REJILLA_TASK_CTX (task), NULL);
	}

	/* now call ctx to update progress */
	rejilla_task_ctx_report_progress (REJILLA_TASK_CTX (data));

	return TRUE;
}

/**
 * Used to run/stop task
 */

static RejillaBurnResult
rejilla_task_deactivate_item (RejillaTask *task,
			      RejillaTaskItem *item,
			      GError **error)
{
	RejillaBurnResult result = REJILLA_BURN_OK;
	RejillaTaskItemIFace *klass;

	if (!rejilla_task_item_is_active (item)) {
		REJILLA_BURN_LOG ("%s already stopped", G_OBJECT_TYPE_NAME (item));
		return REJILLA_BURN_OK;
	}

	/* stop task for real now */
	REJILLA_BURN_LOG ("stopping %s", G_OBJECT_TYPE_NAME (item));

	klass = REJILLA_TASK_ITEM_GET_CLASS (item);
	if (klass->stop)
		result = klass->stop (item,
				      REJILLA_TASK_CTX (task),
				      error);

	REJILLA_BURN_LOG ("stopped %s", G_OBJECT_TYPE_NAME (item));
	return result;
}

static RejillaBurnResult
rejilla_task_send_stop_signal (RejillaTask *task,
			       RejillaBurnResult retval,
			       GError **error)
{
	RejillaTaskItem *item;
	RejillaTaskPrivate *priv;
	GError *local_error = NULL;
	RejillaBurnResult result = retval;

	priv = REJILLA_TASK_PRIVATE (task);

	item = priv->leader;
	while (rejilla_task_item_previous (item))
		item = rejilla_task_item_previous (item);

	/* we stop all the slaves first and then go up the list */
	for (; item; item = rejilla_task_item_next (item)) {
		GError *item_error;

		item_error = NULL;

		/* stop task for real now */
		result = rejilla_task_deactivate_item (task, item, &item_error);
		if (item_error) {
			g_error_free (local_error);
			local_error = item_error;
		}
	}

	if (local_error) {
		if (error && *error == NULL)
			g_propagate_error (error, local_error);
		else
			g_error_free (local_error);
	}

	/* we don't want to lose the original result if it was not OK */
	return (result == REJILLA_BURN_OK? retval:result);
}

static gboolean
rejilla_task_wakeup (gpointer user_data)
{
	RejillaTaskPrivate *priv;

	priv = REJILLA_TASK_PRIVATE (user_data);

	if (priv->loop)
		g_main_loop_quit (priv->loop);

	priv->clock_id = 0;
	priv->retval = REJILLA_BURN_OK;
	return FALSE;
}

static RejillaBurnResult
rejilla_task_sleep (RejillaTask *self, guint sec)
{
	RejillaTaskPrivate *priv;

	priv = REJILLA_TASK_PRIVATE (self);

	REJILLA_BURN_LOG ("wait loop");

	priv->loop = g_main_loop_new (NULL, FALSE);
	priv->clock_id = g_timeout_add_seconds (sec,
	                                        rejilla_task_wakeup,
	                                        self);

	GDK_THREADS_LEAVE ();  
	g_main_loop_run (priv->loop);
	GDK_THREADS_ENTER ();

	g_main_loop_unref (priv->loop);
	priv->loop = NULL;

	if (priv->clock_id) {
		g_source_remove (priv->clock_id);
		priv->clock_id = 0;
	}

	return priv->retval;
}

static RejillaBurnResult
rejilla_task_start_item (RejillaTask *task,
			 RejillaTaskItem *item,
			 GError **error)
{
	guint attempts = 0;
	RejillaBurnResult result;
	GError *ret_error = NULL;
	RejillaTaskItemIFace *klass;

	klass = REJILLA_TASK_ITEM_GET_CLASS (item);
	if (!klass->start)
		return REJILLA_BURN_ERR;

	REJILLA_BURN_LOG ("::start method %s", G_OBJECT_TYPE_NAME (item));

	result = klass->start (item, &ret_error);
	while (result == REJILLA_BURN_RETRY) {
		/* FIXME: a GError?? */
		if (attempts >= MAX_JOB_START_ATTEMPTS) {
			if (ret_error)
				g_propagate_error (error, ret_error);

			return REJILLA_BURN_ERR;
		}

		if (ret_error) {
			g_error_free (ret_error);
			ret_error = NULL;
		}

		result = rejilla_task_sleep (task, 1);
		if (result != REJILLA_BURN_OK)
			return result;

		attempts ++;
		result = klass->start (item, &ret_error);
	}

	if (ret_error)
		g_propagate_error (error, ret_error);

	return result;
}

static RejillaBurnResult
rejilla_task_activate_item (RejillaTask *task,
			    RejillaTaskItem *item,
			    GError **error)
{
	RejillaTaskItemIFace *klass;
	RejillaBurnResult result = REJILLA_BURN_OK;

	klass = REJILLA_TASK_ITEM_GET_CLASS (item);
	if (!klass->activate)
		return REJILLA_BURN_ERR;

	REJILLA_BURN_LOG ("::activate method %s", G_OBJECT_TYPE_NAME (item));

	result = klass->activate (item, REJILLA_TASK_CTX (task), error);
	return result;
}

static void
rejilla_task_stop (RejillaTask *task,
		   RejillaBurnResult retval,
		   GError *error)
{
	RejillaBurnResult result;
	RejillaTaskPrivate *priv;

	priv = REJILLA_TASK_PRIVATE (task);

	/* rejilla_job_error/rejilla_job_finished ()
	 * should not be called during ::init and ::start.
	 * Instead a job should return errors directly */
	result = rejilla_task_send_stop_signal (task, retval, &error);

	priv->retval = retval;
	priv->error = error;

	if (priv->loop && g_main_loop_is_running (priv->loop))
		g_main_loop_quit (priv->loop);
	else
		REJILLA_BURN_LOG ("task was asked to stop (%i/%i) during ::init or ::start",
				  result, retval);
}

RejillaBurnResult
rejilla_task_cancel (RejillaTask *task,
		     gboolean protect)
{
	RejillaTaskPrivate *priv;

	priv = REJILLA_TASK_PRIVATE (task);
	if (protect && rejilla_task_ctx_get_dangerous (REJILLA_TASK_CTX (task)))
		return REJILLA_BURN_DANGEROUS;

	rejilla_task_stop (task, REJILLA_BURN_CANCEL, NULL);
	return REJILLA_BURN_OK;
}

gboolean
rejilla_task_is_running (RejillaTask *task)
{
	RejillaTaskPrivate *priv;

	priv = REJILLA_TASK_PRIVATE (task);
	return (priv->loop && g_main_loop_is_running (priv->loop));
}

static void
rejilla_task_finished (RejillaTaskCtx *ctx,
		       RejillaBurnResult retval,
		       GError *error)
{
	RejillaTask *self;
	RejillaTaskPrivate *priv;

	self = REJILLA_TASK (ctx);
	priv = REJILLA_TASK_PRIVATE (self);

	/* see if we have really started a loop */
	/* FIXME: shouldn't it be an error if it is called
	 * while the loop is not running ? */
	if (!rejilla_task_is_running (self))
		return;
		
	if (retval == REJILLA_BURN_RETRY) {
		RejillaTaskItem *item;
		GError *error_item = NULL;

		/* There are some tracks left, get the first
		 * task item and restart it. */
		item = priv->leader;
		while (rejilla_task_item_previous (item))
			item = rejilla_task_item_previous (item);

		if (rejilla_task_item_start (item, &error_item) != REJILLA_BURN_OK)
			rejilla_task_stop (self, REJILLA_BURN_ERR, error_item);

		return;
	}

	rejilla_task_stop (self, retval, error);
}

static RejillaBurnResult
rejilla_task_run_loop (RejillaTask *self,
		       GError **error)
{
	RejillaTaskPrivate *priv;

	priv = REJILLA_TASK_PRIVATE (self);

	rejilla_task_ctx_report_progress (REJILLA_TASK_CTX (self));

	priv->clock_id = g_timeout_add (500,
					rejilla_task_clock_tick,
					self);

	priv->loop = g_main_loop_new (NULL, FALSE);

	REJILLA_BURN_LOG ("entering loop");

	GDK_THREADS_LEAVE ();  
	g_main_loop_run (priv->loop);
	GDK_THREADS_ENTER ();

	REJILLA_BURN_LOG ("got out of loop");
	g_main_loop_unref (priv->loop);
	priv->loop = NULL;

	if (priv->error) {
		g_propagate_error (error, priv->error);
		priv->error = NULL;
	}

	/* stop all progress reporting thing */
	if (priv->clock_id) {
		g_source_remove (priv->clock_id);
		priv->clock_id = 0;
	}

	if (priv->retval == REJILLA_BURN_OK
	&&  rejilla_task_ctx_get_progress (REJILLA_TASK_CTX (self), NULL) == REJILLA_BURN_OK) {
		rejilla_task_ctx_set_progress (REJILLA_TASK_CTX (self), 1.0);
		rejilla_task_ctx_report_progress (REJILLA_TASK_CTX (self));
	}

	rejilla_task_ctx_stop_progress (REJILLA_TASK_CTX (self));
	return priv->retval;	
}

static RejillaBurnResult
rejilla_task_set_track_output_size_default (RejillaTask *self,
					    GError **error)
{
	RejillaTrack *track = NULL;

	REJILLA_BURN_LOG ("Trying to set a default output size");

	rejilla_task_ctx_get_current_track (REJILLA_TASK_CTX (self), &track);
//	REJILLA_BURN_LOG_TYPE (&input, "Track type");

	if (REJILLA_IS_TRACK_IMAGE (track)
	||  REJILLA_IS_TRACK_STREAM (track)) {
		RejillaBurnResult result;
		goffset sectors = 0;
		goffset bytes = 0;

		result = rejilla_track_get_size (track,
						 &sectors,
						 &bytes);
		if (result != REJILLA_BURN_OK)
			return result;

		REJILLA_BURN_LOG ("Got a default image or stream track length %lli", sectors);
		rejilla_task_ctx_set_output_size_for_current_track (REJILLA_TASK_CTX (self),
								    sectors,
								    bytes);
	}

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_task_activate_items (RejillaTask *self,
			     GError **error)
{
	RejillaTaskPrivate *priv;
	RejillaBurnResult retval;
	RejillaTaskItem *item;

	priv = REJILLA_TASK_PRIVATE (self);

	retval = REJILLA_BURN_NOT_RUNNING;
	for (item = priv->first; item; item = rejilla_task_item_next (item)) {
		RejillaBurnResult result;

		result = rejilla_task_activate_item (self, item, error);
		if (result == REJILLA_BURN_NOT_RUNNING) {
			REJILLA_BURN_LOG ("::start skipped for %s",
					  G_OBJECT_TYPE_NAME (item));
			continue;
		}

		if (result != REJILLA_BURN_OK)
			return result;

		retval = REJILLA_BURN_OK;
	}

	return retval;
}

static RejillaBurnResult
rejilla_task_start_items (RejillaTask *self,
			  GError **error)
{
	RejillaBurnResult retval;
	RejillaTaskPrivate *priv;
	RejillaTaskItem *item;

	priv = REJILLA_TASK_PRIVATE (self);

	/* start from the master down to the slave */
	retval = REJILLA_BURN_NOT_SUPPORTED;
	for (item = priv->leader; item; item = rejilla_task_item_previous (item)) {
		RejillaBurnResult result;

		if (!rejilla_task_item_is_active (item))
			continue;

		result = rejilla_task_start_item (self, item, error);
		if (result == REJILLA_BURN_NOT_SUPPORTED) {
			REJILLA_BURN_LOG ("%s doesn't support action",
					  G_OBJECT_TYPE_NAME (item));

			/* "fake mode" to get size. Forgive the jobs that cannot
			 * retrieve the size for one track. Just deactivate and
			 * go on with the next.
			 * NOTE: after this result the job is no longer active */
			continue;
		}

		/* if the following is true don't stop everything */
		if (result == REJILLA_BURN_NOT_RUNNING)
			return result;

		if (result != REJILLA_BURN_OK)
			return result;

		retval = REJILLA_BURN_OK;
	}

	if (retval == REJILLA_BURN_NOT_SUPPORTED) {
		/* if all jobs did not want/could not run then resort to a
		 * default function and return REJILLA_BURN_NOT_RUNNING */
		retval = rejilla_task_set_track_output_size_default (self, error);
		if (retval != REJILLA_BURN_OK)
			return retval;

		return REJILLA_BURN_NOT_RUNNING;
	}

	return rejilla_task_run_loop (self, error);
}

static RejillaBurnResult
rejilla_task_start (RejillaTask *self,
		    gboolean fake,
		    GError **error)
{
	RejillaBurnResult result = REJILLA_BURN_OK;
	RejillaTaskPrivate *priv;

	priv = REJILLA_TASK_PRIVATE (self);

	REJILLA_BURN_LOG ("Starting %s task (%i)",
			  fake ? "fake":"normal",
			  rejilla_task_ctx_get_action (REJILLA_TASK_CTX (self)));

	/* check the task is not running */
	if (rejilla_task_is_running (self)) {
		REJILLA_BURN_LOG ("task is already running");
		return REJILLA_BURN_RUNNING;
	}

	if (!priv->leader) {
		REJILLA_BURN_LOG ("no jobs");
		return REJILLA_BURN_RUNNING;
	}

	rejilla_task_ctx_set_fake (REJILLA_TASK_CTX (self), fake);
	rejilla_task_ctx_reset (REJILLA_TASK_CTX (self));

	/* Activate all items that can be. If no item can be then skip */
	result = rejilla_task_activate_items (self, error);
	if (result == REJILLA_BURN_NOT_RUNNING) {
		REJILLA_BURN_LOG ("Task skipped");
		return REJILLA_BURN_OK;
	}

	if (result != REJILLA_BURN_OK)
		return result;

	result = rejilla_task_start_items (self, error);
	while (result == REJILLA_BURN_NOT_RUNNING) {
		REJILLA_BURN_LOG ("current track skipped");

		/* this track was skipped without actual loop therefore see if
		 * there is another track and, if there is, start again */
		result = rejilla_task_ctx_next_track (REJILLA_TASK_CTX (self));
		if (result != REJILLA_BURN_RETRY) {
			rejilla_task_send_stop_signal (self, result, NULL);
			return result;
		}

		result = rejilla_task_start_items (self, error);
	}

	if (result != REJILLA_BURN_OK)
		rejilla_task_send_stop_signal (self, result, NULL);

	return result;
}

RejillaBurnResult
rejilla_task_check (RejillaTask *self,
		    GError **error)
{
	RejillaTaskAction action;

	g_return_val_if_fail (REJILLA_IS_TASK (self), REJILLA_BURN_ERR);

	/* the task MUST be of a REJILLA_TASK_ACTION_NORMAL type */
	action = rejilla_task_ctx_get_action (REJILLA_TASK_CTX (self));
	if (action != REJILLA_TASK_ACTION_NORMAL)
		return REJILLA_BURN_OK;

	/* The main purpose of this function is to get the final size of the
	 * task output whether it be recorded to disc or stored as a file later.
	 * That size will be stored by task-ctx.
	 * To do this we run all the task in fake mode that means we don't write
	 * anything to disc or hard drive. Only the last running job in the
	 * chain will be aware that we're running in fake mode / get-size mode.
	 * All others will be told to image. To determine what should be the
	 * last job and therefore the one telling the final size of the output,
	 * we start to call ::init for each job starting from the leader. we
	 * don't skip recording jobs in case they modify the contents (and
	 * therefore the output size). When a job returns NOT_RUNNING after
	 * ::init then we skip it; this return value will mean that it won't
	 * change the output size or that it has already determined the output
	 * size. Only the last running job is allowed to set the final size (see
	 * burn-jobs.c), all values from other jobs will be ignored. */

	return rejilla_task_start (self, TRUE, error);
}

RejillaBurnResult
rejilla_task_run (RejillaTask *self,
		  GError **error)
{
	g_return_val_if_fail (REJILLA_IS_TASK (self), REJILLA_BURN_ERR);

	return rejilla_task_start (self, FALSE, error);
}

static void
rejilla_task_class_init (RejillaTaskClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	RejillaTaskCtxClass *ctx_class = REJILLA_TASK_CTX_CLASS (klass);

	g_type_class_add_private (klass, sizeof (RejillaTaskPrivate));

	parent_class = g_type_class_peek_parent(klass);
	object_class->finalize = rejilla_task_finalize;

	ctx_class->finished = rejilla_task_finished;
}

static void
rejilla_task_init (RejillaTask *obj)
{ }

static void
rejilla_task_finalize (GObject *object)
{
	RejillaTask *cobj;
	RejillaTaskPrivate *priv;

	cobj = REJILLA_TASK (object);
	priv = REJILLA_TASK_PRIVATE (cobj);

	if (priv->leader) {
		g_object_unref (priv->leader);
		priv->leader = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

RejillaTask *
rejilla_task_new ()
{
	RejillaTask *obj;

	obj = REJILLA_TASK (g_object_new (REJILLA_TYPE_TASK, NULL));
	return obj;
}
