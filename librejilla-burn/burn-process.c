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

#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>

#include <gio/gio.h>

#include "burn-basics.h"
#include "burn-process.h"
#include "burn-job.h"

#include "rejilla-track-stream.h"
#include "rejilla-track-image.h"

G_DEFINE_TYPE (RejillaProcess, rejilla_process, REJILLA_TYPE_JOB);

enum {
	REJILLA_CHANNEL_STDOUT	= 0,
	REJILLA_CHANNEL_STDERR
};

static const gchar *debug_prefixes [] = {	"stdout: %s",
						"stderr: %s",
						NULL };

typedef RejillaBurnResult	(*RejillaProcessReadFunc)	(RejillaProcess *process,
								 const gchar *line);

typedef struct _RejillaProcessPrivate RejillaProcessPrivate;
struct _RejillaProcessPrivate {
	GPtrArray *argv;

	/* deferred error that will be used if the process doesn't return 0 */
	GError *error;

	GIOChannel *std_out;
	GString *out_buffer;

	GIOChannel *std_error;
	GString *err_buffer;

	gchar *working_directory;

	GPid pid;
	gint io_out;
	gint io_err;
	gint io_in;

	guint watch;
	guint return_status;

	guint process_finished:1;
};

#define REJILLA_PROCESS_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REJILLA_TYPE_PROCESS, RejillaProcessPrivate))

void
rejilla_process_deferred_error (RejillaProcess *self,
				GError *error)
{
	RejillaProcessPrivate *priv;

	priv = REJILLA_PROCESS_PRIVATE (self);
	if (priv->error)
		g_error_free (priv->error);

	priv->error = error;
}

static RejillaBurnResult
rejilla_process_ask_argv (RejillaJob *job,
			  GError **error)
{
	RejillaProcessClass *klass = REJILLA_PROCESS_GET_CLASS (job);
	RejillaProcessPrivate *priv = REJILLA_PROCESS_PRIVATE (job);
	RejillaProcess *process = REJILLA_PROCESS (job);
	RejillaBurnResult result;
	int i;

	if (priv->pid)
		return REJILLA_BURN_RUNNING;

	if (!klass->set_argv)
		REJILLA_JOB_NOT_SUPPORTED (process);

	REJILLA_JOB_LOG (process, "getting varg");

	if (priv->argv) {
		g_strfreev ((gchar**) priv->argv->pdata);
		g_ptr_array_free (priv->argv, FALSE);
	}

	priv->argv = g_ptr_array_new ();
	result = klass->set_argv (process,
				  priv->argv,
				  error);
	g_ptr_array_add (priv->argv, NULL);

	REJILLA_JOB_LOG (process, "got varg:");

	for (i = 0; priv->argv->pdata [i]; i++)
		REJILLA_JOB_LOG_ARG (process,
				     priv->argv->pdata [i]);

	if (result != REJILLA_BURN_OK) {
		g_strfreev ((gchar**) priv->argv->pdata);
		g_ptr_array_free (priv->argv, FALSE);
		priv->argv = NULL;
		return result;
	}

	return REJILLA_BURN_OK;
}

static void
rejilla_process_add_automatic_track (RejillaProcess *self)
{
	RejillaBurnResult result;
	RejillaTrack *track = NULL;
	RejillaTrackType *type = NULL;
	RejillaJobAction action = REJILLA_BURN_ACTION_NONE;
	RejillaProcessPrivate *priv = REJILLA_PROCESS_PRIVATE (self);

	/* On error, don't automatically add a track */
	if (priv->return_status)
		return;

	/* See if the plugin already added some new
	 * tracks while it was running; if so, don't add it
	 * automatically */
	if (rejilla_job_get_done_tracks (REJILLA_JOB (self), NULL) == REJILLA_BURN_OK)
		return;

	/* Only the last running job when it images to a
	 * file should add a track.
	 * NOTE: the last job in a task is the one that
	 * does not pipe anything. */
	if (rejilla_job_get_fd_out (REJILLA_JOB (self), NULL) == REJILLA_BURN_OK)
		return;

	rejilla_job_get_action (REJILLA_JOB (self), &action);
	if (action != REJILLA_JOB_ACTION_IMAGE)
		return;

	/* Now add a new track */
	type = rejilla_track_type_new ();
	result = rejilla_job_get_output_type (REJILLA_JOB (self), type);
	if (result != REJILLA_BURN_OK) {
		rejilla_track_type_free (type);
		return;
	}

	REJILLA_JOB_LOG (self, "Automatically adding track");

	/* NOTE: we are only able to handle the two
	 * following track types. For other ones, the
	 * plugin is supposed to handle that itself */
	if (rejilla_track_type_get_has_image (type)) {
		gchar *toc = NULL;
		gchar *image = NULL;
		goffset blocks = 0;

		track = REJILLA_TRACK (rejilla_track_image_new ());
		rejilla_job_get_image_output (REJILLA_JOB (self),
					      &image,
					      &toc);

		rejilla_track_image_set_source (REJILLA_TRACK_IMAGE (track),
						image,
						toc,
						rejilla_track_type_get_image_format (type));

		g_free (image);
		g_free (toc);

		rejilla_job_get_session_output_size (REJILLA_JOB (self), &blocks, NULL);
		rejilla_track_image_set_block_num (REJILLA_TRACK_IMAGE (track), blocks);
	}
	else if (rejilla_track_type_get_has_stream (type)) {
		gchar *uri = NULL;

		track = REJILLA_TRACK (rejilla_track_stream_new ());
		rejilla_job_get_audio_output (REJILLA_JOB (self), &uri);
		rejilla_track_stream_set_source (REJILLA_TRACK_STREAM (track), uri);
		rejilla_track_stream_set_format (REJILLA_TRACK_STREAM (track),
						 rejilla_track_type_get_stream_format (type));

		g_free (uri);
	}
	rejilla_track_type_free (type);

	if (track) {
		rejilla_job_add_track (REJILLA_JOB (self), track);

		/* It's good practice to unref the track afterwards as we don't
		 * need it anymore. RejillaTaskCtx refs it. */
		g_object_unref (track);
	}
}

static RejillaBurnResult
rejilla_process_finished (RejillaProcess *self)
{
	RejillaProcessPrivate *priv = REJILLA_PROCESS_PRIVATE (self);
	RejillaProcessClass *klass = REJILLA_PROCESS_GET_CLASS (self);

	priv->process_finished = TRUE;

	/* check if an error went undetected */
	if (priv->return_status) {
		if (priv->error) {
			rejilla_job_error (REJILLA_JOB (self),
					   g_error_new (REJILLA_BURN_ERROR,
							REJILLA_BURN_ERROR_GENERAL,
						        /* Translators: %s is the name of the rejilla element */
							_("Process \"%s\" ended with an error code (%i)"),
							G_OBJECT_TYPE_NAME (self),
							priv->return_status));
		}
		else {
			rejilla_job_error (REJILLA_JOB (self), priv->error);
			priv->error = NULL;
		}

		return REJILLA_BURN_OK;
	}

	/* This is a deferred error that is an error that
	 * was by a plugin and that should be only used
	 * if the process finishes with a bad return value */
	if (priv->error) {
		g_error_free (priv->error);
		priv->error = NULL;
	}

	/* Tell the world we're done */
	return klass->post (REJILLA_JOB (self));
}

static gboolean
rejilla_process_watch_child (gpointer data)
{
	int status;
	RejillaBurnResult result;
	RejillaProcessPrivate *priv = REJILLA_PROCESS_PRIVATE (data);

	if (waitpid (priv->pid, &status, WNOHANG) <= 0)
		return TRUE;

	/* store the return value it will be checked only if no 
	 * rejilla_job_finished/_error is called before the pipes are closed so
	 * as to let plugins read stderr / stdout till the end and set a better
	 * error message or simply decide all went well, in one word override */
	priv->return_status = WEXITSTATUS (status);
	priv->watch = 0;
	priv->pid = 0;

	REJILLA_JOB_LOG (data, "process finished with status %i", WEXITSTATUS (status));

	result = rejilla_process_finished (data);
	if (result == REJILLA_BURN_RETRY) {
		GError *error = NULL;
		RejillaJobClass *job_class;

		priv->process_finished = FALSE;

		job_class = REJILLA_JOB_GET_CLASS (data);
		if (job_class->stop) {
			result = job_class->stop (data, &error);
			if (result != REJILLA_BURN_OK) {
				rejilla_job_error (data, error);
				return FALSE;
			}
		}

		if (job_class->start) {
			/* we were asked by the plugin to restart it */
			result = job_class->start (data, &error);
			if (result != REJILLA_BURN_OK)
				rejilla_job_error (data, error);
		}
	}

	return FALSE;
}

static gboolean
rejilla_process_read (RejillaProcess *process,
		      GIOChannel *channel,
		      GIOCondition condition,
		      gint channel_type,
		      RejillaProcessReadFunc readfunc)
{
	GString *buffer;
	GIOStatus status;
	RejillaBurnResult result = REJILLA_BURN_OK;
	RejillaProcessPrivate *priv = REJILLA_PROCESS_PRIVATE (process);

	if (!channel)
		return FALSE;

	if (channel_type == REJILLA_CHANNEL_STDERR)
		buffer = priv->err_buffer;
	else
		buffer = priv->out_buffer;

	if (condition & G_IO_IN) {
		gsize term;
		gchar *line = NULL;

		status = g_io_channel_read_line (channel,
						 &line,
						 &term,
						 NULL,
						 NULL);

		if (status == G_IO_STATUS_AGAIN) {
			gchar character;
			/* there wasn't any character ending the line.
			 * some processes (like cdrecord/cdrdao) produce an
			 * extra character sometimes */
			status = g_io_channel_read_chars (channel,
							  &character,
							  1,
							  NULL,
							  NULL);

			if (status == G_IO_STATUS_NORMAL) {
				g_string_append_c (buffer, character);

				switch (character) {
				case '\b':
				case '\n':
				case '\r':
				case '\xe2': /* Unicode paragraph separator */
				case '\0':
					REJILLA_JOB_LOG (process,
							 debug_prefixes [channel_type],
							 buffer->str);

					if (readfunc && buffer->str [0] != '\0')
						result = readfunc (process, buffer->str);

					/* a subclass could have stopped or errored out.
					 * in this case rejilla_process_stop will have 
					 * been called and the buffer deallocated. So we
					 * check that it still exists */
					if (channel_type == REJILLA_CHANNEL_STDERR)
						buffer = priv->err_buffer;
					else
						buffer = priv->out_buffer;

					if (buffer)
						g_string_set_size (buffer, 0);

					if (result != REJILLA_BURN_OK)
						return FALSE;

					break;
				default:
					break;
				}
			}
		}
		else if (status == G_IO_STATUS_NORMAL) {
			if (term)
				line [term - 1] = '\0';

			g_string_append (buffer, line);
			g_free (line);

			REJILLA_JOB_LOG (process,
					 debug_prefixes [channel_type],
					 buffer->str);

			if (readfunc && buffer->str [0] != '\0')
				result = readfunc (process, buffer->str);

			/* a subclass could have stopped or errored out.
			 * in this case rejilla_process_stop will have 
			 * been called and the buffer deallocated. So we
			 * check that it still exists */
			if (channel_type == REJILLA_CHANNEL_STDERR)
				buffer = priv->err_buffer;
			else
				buffer = priv->out_buffer;

			if (buffer)
				g_string_set_size (buffer, 0);

			if (result != REJILLA_BURN_OK)
				return FALSE;
		}
		else if (status == G_IO_STATUS_EOF) {
			REJILLA_JOB_LOG (process, 
					 debug_prefixes [channel_type],
					 "EOF");
			return FALSE;
		}
		else
			return FALSE;
	}
	else if (condition & G_IO_HUP) {
		/* only handle the HUP when we have read all available lines of output */
		REJILLA_JOB_LOG (process,
				 debug_prefixes [channel_type],
				 "HUP");
		return FALSE;
	}

	return TRUE;
}

static gboolean
rejilla_process_read_stderr (GIOChannel *source,
			     GIOCondition condition,
			     RejillaProcess *process)
{
	gboolean result;
	RejillaProcessClass *klass;
	RejillaProcessPrivate *priv = REJILLA_PROCESS_PRIVATE (process);

	if (!priv->err_buffer)
		priv->err_buffer = g_string_new (NULL);

	klass = REJILLA_PROCESS_GET_CLASS (process);
	result = rejilla_process_read (process,
				       source,
				       condition,
				       REJILLA_CHANNEL_STDERR,
				       klass->stderr_func);

	if (result)
		return result;

	priv->io_err = 0;

	g_io_channel_unref (priv->std_error);
	priv->std_error = NULL;

	g_string_free (priv->err_buffer, TRUE);
	priv->err_buffer = NULL;

	/* What if the above function (rejilla_process_read called
	 * rejilla_job_finished */
	if (priv->pid
	&& !priv->io_err
	&& !priv->io_out) {
		/* setup a child watch callback to be warned when it finishes so
		 * as to check the return value for errors.
		 * need to reap our children by ourselves g_child_watch_add
		 * doesn't work well with multiple processes. regularly poll
		 * with waitpid ()*/
		priv->watch = g_timeout_add (500, rejilla_process_watch_child, process);
	}
	return FALSE;
}

static gboolean
rejilla_process_read_stdout (GIOChannel *source,
			     GIOCondition condition,
			     RejillaProcess *process)
{
	gboolean result;
	RejillaProcessClass *klass;
	RejillaProcessPrivate *priv = REJILLA_PROCESS_PRIVATE (process);

	if (!priv->out_buffer)
		priv->out_buffer = g_string_new (NULL);

	klass = REJILLA_PROCESS_GET_CLASS (process);
	result = rejilla_process_read (process,
				       source,
				       condition,
				       REJILLA_CHANNEL_STDOUT,
				       klass->stdout_func);

	if (result)
		return result;

	priv->io_out = 0;

	if (priv->std_out) {
		g_io_channel_unref (priv->std_out);
		priv->std_out = NULL;
	}

	g_string_free (priv->out_buffer, TRUE);
	priv->out_buffer = NULL;

	if (priv->pid
	&& !priv->io_err
	&& !priv->io_out) {
		/* setup a child watch callback to be warned when it finishes so
		 * as to check the return value for errors.
		 * need to reap our children by ourselves g_child_watch_add
		 * doesn't work well with multiple processes. regularly poll
		 * with waitpid ()*/
		priv->watch = g_timeout_add (500, rejilla_process_watch_child, process);
	}

	return FALSE;
}

static GIOChannel *
rejilla_process_setup_channel (RejillaProcess *process,
			       int pipe,
			       gint *watch,
			       GIOFunc function)
{
	GIOChannel *channel;

	fcntl (pipe, F_SETFL, O_NONBLOCK);
	channel = g_io_channel_unix_new (pipe);

	/* It'd be good if we were allowed to add some to the default line
	 * separator */
//	g_io_channel_set_line_term (channel, "\b", -1);

	g_io_channel_set_flags (channel,
				g_io_channel_get_flags (channel) | G_IO_FLAG_NONBLOCK,
				NULL);
	g_io_channel_set_encoding (channel, NULL, NULL);
	*watch = g_io_add_watch (channel,
				(G_IO_IN | G_IO_HUP | G_IO_ERR | G_IO_NVAL),
				 function,
				 process);

	g_io_channel_set_close_on_unref (channel, TRUE);
	return channel;
}

static void
rejilla_process_setup (gpointer data)
{
	RejillaProcessPrivate *priv;
	RejillaProcess *process;
	int fd;

	process = REJILLA_PROCESS (data);
	priv = REJILLA_PROCESS_PRIVATE (process);

	fd = -1;
	if (rejilla_job_get_fd_in (REJILLA_JOB (process), &fd) == REJILLA_BURN_OK) {
		if (dup2 (fd, 0) == -1)
			REJILLA_JOB_LOG (process, "Dup2 failed: %s", g_strerror (errno));
	}

	fd = -1;
	if (rejilla_job_get_fd_out (REJILLA_JOB (process), &fd) == REJILLA_BURN_OK) {
		if (dup2 (fd, 1) == -1)
			REJILLA_JOB_LOG (process, "Dup2 failed: %s", g_strerror (errno));
	}
}

static RejillaBurnResult
rejilla_process_start (RejillaJob *job, GError **error)
{
	RejillaProcessPrivate *priv = REJILLA_PROCESS_PRIVATE (job);
	RejillaProcess *process = REJILLA_PROCESS (job);
	int stdout_pipe, stderr_pipe;
	RejillaProcessClass *klass;
	RejillaBurnResult result;
	gboolean read_stdout;
	/* that's to make sure programs are not translated */
	gchar *envp [] = {	"LANG=C",
				"LANGUAGE=C",
				"LC_ALL=C",
				NULL};

	if (priv->pid)
		return REJILLA_BURN_RUNNING;

	/* ask the arguments for the program */
	result = rejilla_process_ask_argv (job, error);
	if (result != REJILLA_BURN_OK)
		return result;

	if (priv->working_directory) {
		REJILLA_JOB_LOG (process, "Launching command in %s", priv->working_directory);
	}
	else {
		REJILLA_JOB_LOG (process, "Launching command");
	}

	klass = REJILLA_PROCESS_GET_CLASS (process);

	/* only watch stdout coming from the last object in the queue */
	read_stdout = (klass->stdout_func &&
		       rejilla_job_get_fd_out (REJILLA_JOB (process), NULL) != REJILLA_BURN_OK);

	priv->process_finished = FALSE;
	priv->return_status = 0;

	if (!g_spawn_async_with_pipes (priv->working_directory,
				       (gchar **) priv->argv->pdata,
				       (gchar **) envp,
				       G_SPAWN_SEARCH_PATH|
				       G_SPAWN_DO_NOT_REAP_CHILD,
				       rejilla_process_setup,
				       process,
				       &priv->pid,
				       NULL,
				       read_stdout ? &stdout_pipe : NULL,
				       &stderr_pipe,
				       error)) {
		return REJILLA_BURN_ERR;
	}

	/* error channel */
	priv->std_error = rejilla_process_setup_channel (process,
							 stderr_pipe,
							&priv->io_err,
							(GIOFunc) rejilla_process_read_stderr);

	if (read_stdout)
		priv->std_out = rejilla_process_setup_channel (process,
							       stdout_pipe,
							       &priv->io_out,
							       (GIOFunc) rejilla_process_read_stdout);

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_process_stop (RejillaJob *job,
		      GError **error)
{
	RejillaBurnResult result = REJILLA_BURN_OK;
	RejillaProcessPrivate *priv;
	RejillaProcess *process;

	process = REJILLA_PROCESS (job);
	priv = REJILLA_PROCESS_PRIVATE (process);

	if (priv->watch) {
		/* if the child is still running at this stage 
		 * that means that we were cancelled or
		 * that we decided to stop ourselves so
		 * don't check the returned value */
		g_source_remove (priv->watch);
		priv->watch = 0;
	}

	/* it might happen that the slave detected an
	 * error triggered by the master BEFORE the
	 * master so we finish reading whatever is in
	 * the pipes to see: fdsink will notice cdrecord
	 * closed the pipe before cdrecord reports it */
	if (priv->pid) {
		GPid pid;

		pid = priv->pid;
		priv->pid = 0;

		/* Reminder: -1 is here to send the signal
		 * to all children of the process with pid as
		 * well */
		if (pid > 0 && kill ((-1) * pid, SIGTERM) == -1 && errno != ESRCH) {
			REJILLA_JOB_LOG (process, 
					 "process (%s) couldn't be killed: terminating",
					 g_strerror (errno));
			kill ((-1) * pid, SIGKILL);
		}
		else
			REJILLA_JOB_LOG (process, "got killed");

		g_spawn_close_pid (pid);
	}

	/* read every pending data and close the pipes */
	if (priv->io_out) {
		g_source_remove (priv->io_out);
		priv->io_out = 0;
	}

	if (priv->std_out) {
		if (error && !(*error)) {
			RejillaProcessClass *klass;

			/* we need to nullify the buffer since we've just read a line
			 * that got the job to stop so if we don't do that following 
			 * data will be appended to this line but each will provoke the
			 * same stop every time */
			if (priv->out_buffer)
				g_string_set_size (priv->out_buffer, 0);

			klass = REJILLA_PROCESS_GET_CLASS (process);
			while (priv->std_out && g_io_channel_get_buffer_condition (priv->std_out) == G_IO_IN)
				rejilla_process_read (process,
						      priv->std_out,
						      G_IO_IN,
						      REJILLA_CHANNEL_STDOUT,
						      klass->stdout_func);
		}

	    	/* NOTE: we already checked if priv->std_out wasn't 
		 * NULL but rejilla_process_read could have closed it */
	    	if (priv->std_out) {
			g_io_channel_unref (priv->std_out);
			priv->std_out = NULL;
		}
	}

	if (priv->out_buffer) {
		g_string_free (priv->out_buffer, TRUE);
		priv->out_buffer = NULL;
	}

	if (priv->io_err) {
		g_source_remove (priv->io_err);
		priv->io_err = 0;
	}

	if (priv->std_error) {
		if (error && !(*error)) {
			RejillaProcessClass *klass;
		
			/* we need to nullify the buffer since we've just read a line
			 * that got the job to stop so if we don't do that following 
			 * data will be appended to this line but each will provoke the
			 * same stop every time */
			if (priv->err_buffer)
				g_string_set_size (priv->err_buffer, 0);

			klass = REJILLA_PROCESS_GET_CLASS (process);
			while (priv->std_error && g_io_channel_get_buffer_condition (priv->std_error) == G_IO_IN)
				rejilla_process_read (process,
						     priv->std_error,
						     G_IO_IN,
						     REJILLA_CHANNEL_STDERR,
						     klass->stderr_func);
		}

	    	/* NOTE: we already checked if priv->std_out wasn't 
		 * NULL but rejilla_process_read could have closed it */
	    	if (priv->std_error) {
			g_io_channel_unref (priv->std_error);
			priv->std_error = NULL;
		}
	}

	if (priv->err_buffer) {
		g_string_free (priv->err_buffer, TRUE);
		priv->err_buffer = NULL;
	}

	if (priv->argv) {
		g_strfreev ((gchar**) priv->argv->pdata);
		g_ptr_array_free (priv->argv, FALSE);
		priv->argv = NULL;
	}

	if (priv->error) {
		g_error_free (priv->error);
		priv->error = NULL;
	}

	/* See if we need to automatically add a track */
	if (priv->process_finished)
		rejilla_process_add_automatic_track (REJILLA_PROCESS (job));

	return result;
}

void
rejilla_process_set_working_directory (RejillaProcess *process,
				       const gchar *directory)
{
	RejillaProcessPrivate *priv = REJILLA_PROCESS_PRIVATE (process);

	if (priv->working_directory) {
		g_free (priv->working_directory);
		priv->working_directory = NULL;
	}

	priv->working_directory = g_strdup (directory);
}

static void
rejilla_process_finalize (GObject *object)
{
	RejillaProcessPrivate *priv = REJILLA_PROCESS_PRIVATE (object);

	if (priv->watch) {
		g_source_remove (priv->watch);
		priv->watch = 0;
	}

	if (priv->io_out) {
		g_source_remove (priv->io_out);
		priv->io_out = 0;
	}

	if (priv->std_out) {
		g_io_channel_unref (priv->std_out);
		priv->std_out = NULL;
	}

	if (priv->out_buffer) {
		g_string_free (priv->out_buffer, TRUE);
		priv->out_buffer = NULL;
	}

	if (priv->io_err) {
		g_source_remove (priv->io_err);
		priv->io_err = 0;
	}

	if (priv->std_error) {
		g_io_channel_unref (priv->std_error);
		priv->std_error = NULL;
	}

	if (priv->err_buffer) {
		g_string_free (priv->err_buffer, TRUE);
		priv->err_buffer = NULL;
	}

	if (priv->pid) {
		kill (priv->pid, SIGKILL);
		priv->pid = 0;
	}

	if (priv->argv) {
		g_strfreev ((gchar**) priv->argv->pdata);
		g_ptr_array_free (priv->argv, FALSE);
		priv->argv = NULL;
	}

	if (priv->error) {
		g_error_free (priv->error);
		priv->error = NULL;
	}

	if (priv->working_directory) {
		g_free (priv->working_directory);
		priv->working_directory = NULL;
	}

	G_OBJECT_CLASS (rejilla_process_parent_class)->finalize (object);
}

static void
rejilla_process_class_init (RejillaProcessClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	RejillaJobClass *job_class = REJILLA_JOB_CLASS (klass);

	g_type_class_add_private (klass, sizeof (RejillaProcessPrivate));

	object_class->finalize = rejilla_process_finalize;

	job_class->start = rejilla_process_start;
	job_class->stop = rejilla_process_stop;

	klass->post = rejilla_job_finished_track;
}

static void
rejilla_process_init (RejillaProcess *obj)
{ }
