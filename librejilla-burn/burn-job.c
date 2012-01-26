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
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>

#include <gio/gio.h>

#include "rejilla-drive.h"
#include "rejilla-medium.h"

#include "burn-basics.h"
#include "burn-debug.h"
#include "rejilla-session.h"
#include "rejilla-session-helper.h"
#include "rejilla-plugin-information.h"
#include "burn-job.h"
#include "burn-task-ctx.h"
#include "burn-task-item.h"
#include "librejilla-marshal.h"

#include "rejilla-track-type-private.h"

typedef struct _RejillaJobOutput {
	gchar *image;
	gchar *toc;
} RejillaJobOutput;

typedef struct _RejillaJobInput {
	int out;
	int in;
} RejillaJobInput;

static void rejilla_job_iface_init_task_item (RejillaTaskItemIFace *iface);
G_DEFINE_TYPE_WITH_CODE (RejillaJob, rejilla_job, G_TYPE_OBJECT,
			 G_IMPLEMENT_INTERFACE (REJILLA_TYPE_TASK_ITEM,
						rejilla_job_iface_init_task_item));

typedef struct RejillaJobPrivate RejillaJobPrivate;
struct RejillaJobPrivate {
	RejillaJob *next;
	RejillaJob *previous;

	RejillaTaskCtx *ctx;

	/* used if job reads data from a pipe */
	RejillaJobInput *input;

	/* output type (sets at construct time) */
	RejillaTrackType type;

	/* used if job writes data to a pipe (link is then NULL) */
	RejillaJobOutput *output;
	RejillaJob *linked;
};

#define REJILLA_JOB_DEBUG(job_MACRO)						\
{										\
	const gchar *class_name_MACRO = NULL;					\
	if (REJILLA_IS_JOB (job_MACRO))						\
		class_name_MACRO = G_OBJECT_TYPE_NAME (job_MACRO);		\
	rejilla_job_log_message (job_MACRO, G_STRLOC,				\
				 "%s called %s", 				\
				 class_name_MACRO,				\
				 G_STRFUNC);					\
}

#define REJILLA_JOB_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REJILLA_TYPE_JOB, RejillaJobPrivate))

enum {
	PROP_NONE,
	PROP_OUTPUT,
};

typedef enum {
	ERROR_SIGNAL,
	LAST_SIGNAL
} RejillaJobSignalType;

static guint rejilla_job_signals [LAST_SIGNAL] = { 0 };

static GObjectClass *parent_class = NULL;


/**
 * Task item virtual functions implementation
 */

static RejillaTaskItem *
rejilla_job_item_next (RejillaTaskItem *item)
{
	RejillaJob *self;
	RejillaJobPrivate *priv;

	self = REJILLA_JOB (item);
	priv = REJILLA_JOB_PRIVATE (self);

	if (!priv->next)
		return NULL;

	return REJILLA_TASK_ITEM (priv->next);
}

static RejillaTaskItem *
rejilla_job_item_previous (RejillaTaskItem *item)
{
	RejillaJob *self;
	RejillaJobPrivate *priv;

	self = REJILLA_JOB (item);
	priv = REJILLA_JOB_PRIVATE (self);

	if (!priv->previous)
		return NULL;

	return REJILLA_TASK_ITEM (priv->previous);
}

static RejillaBurnResult
rejilla_job_item_link (RejillaTaskItem *input,
		       RejillaTaskItem *output)
{
	RejillaJobPrivate *priv_input;
	RejillaJobPrivate *priv_output;

	priv_input = REJILLA_JOB_PRIVATE (input);
	priv_output = REJILLA_JOB_PRIVATE (output);

	priv_input->next = REJILLA_JOB (output);
	priv_output->previous = REJILLA_JOB (input);

	g_object_ref (input);
	return REJILLA_BURN_OK;
}

static gboolean
rejilla_job_is_last_active (RejillaJob *self)
{
	RejillaJobPrivate *priv;
	RejillaJob *next;

	priv = REJILLA_JOB_PRIVATE (self);
	if (!priv->ctx)
		return FALSE;

	next = priv->next;
	while (next) {
		priv = REJILLA_JOB_PRIVATE (next);
		if (priv->ctx)
			return FALSE;
		next = priv->next;
	}

	return TRUE;
}

static gboolean
rejilla_job_is_first_active (RejillaJob *self)
{
	RejillaJobPrivate *priv;
	RejillaJob *prev;

	priv = REJILLA_JOB_PRIVATE (self);
	if (!priv->ctx)
		return FALSE;

	prev = priv->previous;
	while (prev) {
		priv = REJILLA_JOB_PRIVATE (prev);
		if (priv->ctx)
			return FALSE;
		prev = priv->previous;
	}

	return TRUE;
}

static void
rejilla_job_input_free (RejillaJobInput *input)
{
	if (!input)
		return;

	if (input->in > 0)
		close (input->in);

	if (input->out > 0)
		close (input->out);

	g_free (input);
}

static void
rejilla_job_output_free (RejillaJobOutput *output)
{
	if (!output)
		return;

	if (output->image) {
		g_free (output->image);
		output->image = NULL;
	}

	if (output->toc) {
		g_free (output->toc);
		output->toc = NULL;
	}

	g_free (output);
}

static void
rejilla_job_deactivate (RejillaJob *self)
{
	RejillaJobPrivate *priv;

	priv = REJILLA_JOB_PRIVATE (self);

	REJILLA_JOB_LOG (self, "deactivating");

	/* ::start hasn't been called yet */
	if (priv->ctx) {
		g_object_unref (priv->ctx);
		priv->ctx = NULL;
	}

	if (priv->input) {
		rejilla_job_input_free (priv->input);
		priv->input = NULL;
	}

	if (priv->output) {
		rejilla_job_output_free (priv->output);
		priv->output = NULL;
	}

	if (priv->linked)
		priv->linked = NULL;
}

static RejillaBurnResult
rejilla_job_allow_deactivation (RejillaJob *self,
				RejillaBurnSession *session,
				GError **error)
{
	RejillaJobPrivate *priv;
	RejillaTrackType input;

	priv = REJILLA_JOB_PRIVATE (self);

	/* This job refused to work. This is allowed in three cases:
	 * - the job is the only one in the task (no other job linked) and the
	 *   track type as input is the same as the track type of the output
	 *   except if type is DISC as input or output
	 * - if the job hasn't got any job linked before the next linked job
	 *   accepts the track type of the session as input
	 * - the output of the previous job is the same as this job output type
	 */

	/* there can't be two recorders in a row so ... */
	if (priv->type.type == REJILLA_TRACK_TYPE_DISC)
		goto error;

	if (priv->previous) {
		RejillaJobPrivate *previous;
		previous = REJILLA_JOB_PRIVATE (priv->previous);
		memcpy (&input, &previous->type, sizeof (RejillaTrackType));
	}
	else
		rejilla_burn_session_get_input_type (session, &input);

	if (!rejilla_track_type_equal (&input, &priv->type))
		goto error;

	return REJILLA_BURN_NOT_RUNNING;

error:

	g_set_error (error,
		     REJILLA_BURN_ERR,
		     REJILLA_BURN_ERROR_PLUGIN_MISBEHAVIOR,
		     /* Translators: %s is the plugin name */
		     _("\"%s\" did not behave properly"),
		     G_OBJECT_TYPE_NAME (self));
	return REJILLA_BURN_ERR;
}

static RejillaBurnResult
rejilla_job_item_activate (RejillaTaskItem *item,
			   RejillaTaskCtx *ctx,
			   GError **error)
{
	RejillaJob *self;
	RejillaJobClass *klass;
	RejillaJobPrivate *priv;
	RejillaBurnSession *session;
	RejillaBurnResult result = REJILLA_BURN_OK;

	self = REJILLA_JOB (item);
	priv = REJILLA_JOB_PRIVATE (self);
	session = rejilla_task_ctx_get_session (ctx);

	g_object_ref (ctx);
	priv->ctx = ctx;

	klass = REJILLA_JOB_GET_CLASS (self);

	/* see if this job needs to be deactivated (if no function then OK) */
	if (klass->activate)
		result = klass->activate (self, error);
	else
		REJILLA_BURN_LOG ("no ::activate method %s",
				  G_OBJECT_TYPE_NAME (item));

	if (result != REJILLA_BURN_OK) {
		g_object_unref (ctx);
		priv->ctx = NULL;

		if (result == REJILLA_BURN_NOT_RUNNING)
			result = rejilla_job_allow_deactivation (self, session, error);

		return result;
	}

	return REJILLA_BURN_OK;
}

static gboolean
rejilla_job_item_is_active (RejillaTaskItem *item)
{
	RejillaJob *self;
	RejillaJobPrivate *priv;

	self = REJILLA_JOB (item);
	priv = REJILLA_JOB_PRIVATE (self);

	return (priv->ctx != NULL);
}

static RejillaBurnResult
rejilla_job_check_output_disc_space (RejillaJob *self,
				     GError **error)
{
	RejillaBurnSession *session;
	goffset output_blocks = 0;
	goffset media_blocks = 0;
	RejillaJobPrivate *priv;
	RejillaBurnFlag flags;
	RejillaMedium *medium;
	RejillaDrive *drive;

	priv = REJILLA_JOB_PRIVATE (self);

	rejilla_task_ctx_get_session_output_size (priv->ctx,
						  &output_blocks,
						  NULL);

	session = rejilla_task_ctx_get_session (priv->ctx);
	drive = rejilla_burn_session_get_burner (session);
	medium = rejilla_drive_get_medium (drive);
	flags = rejilla_burn_session_get_flags (session);

	/* FIXME: if we can't recover the size of the medium 
	 * what should we do ? do as if we could ? */

	/* see if we are appending or not */
	if (flags & (REJILLA_BURN_FLAG_APPEND|REJILLA_BURN_FLAG_MERGE))
		rejilla_medium_get_free_space (medium, NULL, &media_blocks);
	else
		rejilla_medium_get_capacity (medium, NULL, &media_blocks);

	/* This is not really an error, we'll probably ask the 
	 * user to load a new disc */
	if (output_blocks > media_blocks) {
		gchar *media_blocks_str;
		gchar *output_blocks_str;

		REJILLA_BURN_LOG ("Insufficient space on disc %"G_GINT64_FORMAT"/%"G_GINT64_FORMAT,
				  media_blocks,
				  output_blocks);

		media_blocks_str = g_strdup_printf ("%"G_GINT64_FORMAT, media_blocks);
		output_blocks_str = g_strdup_printf ("%"G_GINT64_FORMAT, output_blocks);

		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_MEDIUM_SPACE,
			     /* Translators: the first %s is the size of the free space on the medium
			      * and the second %s is the size of the space required by the data to be
			      * burnt. */
			     _("Not enough space available on the disc (%s available for %s)"),
			     media_blocks_str,
			     output_blocks_str);

		g_free (media_blocks_str);
		g_free (output_blocks_str);
		return REJILLA_BURN_NEED_RELOAD;
	}

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_job_check_output_volume_space (RejillaJob *self,
				       GError **error)
{
	GFileInfo *info;
	gchar *directory;
	GFile *file = NULL;
	struct rlimit limit;
	guint64 vol_size = 0;
	gint64 output_size = 0;
	RejillaJobPrivate *priv;
	const gchar *filesystem;

	/* now that the job has a known output we must check that the volume the
	 * job is writing to has enough space for all output */

	priv = REJILLA_JOB_PRIVATE (self);

	/* Get the size and filesystem type for the volume first.
	 * NOTE: since any plugin must output anything LOCALLY, we can then use
	 * all libc API. */
	if (!priv->output)
		return REJILLA_BURN_ERR;

	directory = g_path_get_dirname (priv->output->image);
	file = g_file_new_for_path (directory);
	g_free (directory);

	if (file == NULL)
		goto error;

	info = g_file_query_info (file,
				  G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE,
				  G_FILE_QUERY_INFO_NONE,
				  NULL,
				  error);
	if (!info)
		goto error;

	/* Check permissions first */
	if (!g_file_info_get_attribute_boolean (info, G_FILE_ATTRIBUTE_ACCESS_CAN_WRITE)) {
		REJILLA_JOB_LOG (self, "No permissions");

		g_object_unref (info);
		g_object_unref (file);
		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_PERMISSION,
			     _("You do not have the required permission to write at this location"));
		return REJILLA_BURN_ERR;
	}
	g_object_unref (info);

	/* Now size left */
	info = g_file_query_filesystem_info (file,
					     G_FILE_ATTRIBUTE_FILESYSTEM_FREE ","
					     G_FILE_ATTRIBUTE_FILESYSTEM_TYPE,
					     NULL,
					     error);
	if (!info)
		goto error;

	g_object_unref (file);
	file = NULL;

	/* Now check the filesystem type: the problem here is that some
	 * filesystems have a maximum file size limit of 4 GiB and more than
	 * often we need a temporary file size of 4 GiB or more. */
	filesystem = g_file_info_get_attribute_string (info, G_FILE_ATTRIBUTE_FILESYSTEM_TYPE);
	REJILLA_BURN_LOG ("%s filesystem detected", filesystem);

	rejilla_job_get_session_output_size (self, NULL, &output_size);

	if (output_size >= 2147483648ULL
	&&  filesystem
	&& !strcmp (filesystem, "msdos")) {
		g_object_unref (info);
		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_DISK_SPACE,
			     _("The filesystem you chose to store the temporary image on cannot hold files with a size over 2 GiB"));
		return REJILLA_BURN_ERR;
	}

	vol_size = g_file_info_get_attribute_uint64 (info, G_FILE_ATTRIBUTE_FILESYSTEM_FREE);
	g_object_unref (info);

	/* get the size of the output this job is supposed to create */
	REJILLA_BURN_LOG ("Volume size %lli, output size %lli", vol_size, output_size);

	/* it's fine here to check size in bytes */
	if (output_size > vol_size) {
		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_DISK_SPACE,
			     _("The location you chose to store the temporary image on does not have enough free space for the disc image (%ld MiB needed)"),
			     (unsigned long) output_size / 1048576);
		return REJILLA_BURN_ERR;
	}

	/* Last but not least, use getrlimit () to check that we are allowed to
	 * write a file of such length and that quotas won't get in our way */
	if (getrlimit (RLIMIT_FSIZE, &limit)) {
		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_DISK_SPACE,
			     "%s",
			     g_strerror (errno));
		return REJILLA_BURN_ERR;
	}

	if (limit.rlim_cur < output_size) {
		REJILLA_BURN_LOG ("User not allowed to write such a large file");

		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_DISK_SPACE,
			     _("The location you chose to store the temporary image on does not have enough free space for the disc image (%ld MiB needed)"),
			     (unsigned long) output_size / 1048576);
		return REJILLA_BURN_ERR;
	}

	return REJILLA_BURN_OK;

error:

	if (error && *error == NULL)
		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_GENERAL,
			     _("The size of the volume could not be retrieved"));

	if (file)
		g_object_unref (file);

	return REJILLA_BURN_ERR;
}

static RejillaBurnResult
rejilla_job_set_output_file (RejillaJob *self,
			     GError **error)
{
	RejillaTrackType *session_output;
	RejillaBurnSession *session;
	RejillaBurnResult result;
	RejillaJobPrivate *priv;
	goffset output_size = 0;
	gchar *image = NULL;
	gchar *toc = NULL;
	gboolean is_last;

	priv = REJILLA_JOB_PRIVATE (self);

	/* no next job so we need a file pad */
	session = rejilla_task_ctx_get_session (priv->ctx);

	/* If the plugin is not supposed to output anything, then don't test */
	rejilla_job_get_session_output_size (REJILLA_JOB (self), NULL, &output_size);

	/* This should be re-enabled when we make sure all plugins (like vcd)
	 * don't advertize an output of 0 whereas it's not true. Maybe we could
	 * use -1 for plugins that don't output. */
	/* if (!output_size)
		return REJILLA_BURN_OK; */

	/* check if that's the last task */
	session_output = rejilla_track_type_new ();
	rejilla_burn_session_get_output_type (session, session_output);
	is_last = rejilla_track_type_equal (session_output, &priv->type);
	rejilla_track_type_free (session_output);

	if (priv->type.type == REJILLA_TRACK_TYPE_IMAGE) {
		if (is_last) {
			RejillaTrackType input = { 0, };

			result = rejilla_burn_session_get_output (session,
								  &image,
								  &toc);

			/* check paths are set */
			if (!image
			|| (priv->type.subtype.img_format != REJILLA_IMAGE_FORMAT_BIN && !toc)) {
				g_set_error (error,
					     REJILLA_BURN_ERROR,
					     REJILLA_BURN_ERROR_OUTPUT_NONE,
					     _("No path was specified for the image output"));
				return REJILLA_BURN_ERR;
			}

			rejilla_burn_session_get_input_type (session,
							     &input);

			/* if input is the same as output, then that's a
			 * processing task and there's no need to check if the
			 * output already exists since it will (but that's OK) */
			if (input.type == REJILLA_TRACK_TYPE_IMAGE
			&&  input.subtype.img_format == priv->type.subtype.img_format) {
				REJILLA_BURN_LOG ("Processing task, skipping check size");
				priv->output = g_new0 (RejillaJobOutput, 1);
				priv->output->image = image;
				priv->output->toc = toc;
				return REJILLA_BURN_OK;
			}
		}
		else {
			/* NOTE: no need to check for the existence here */
			result = rejilla_burn_session_get_tmp_image (session,
								     priv->type.subtype.img_format,
								     &image,
								     &toc,
								     error);
			if (result != REJILLA_BURN_OK)
				return result;
		}

		REJILLA_JOB_LOG (self, "output set (IMAGE) image = %s toc = %s",
				 image,
				 toc ? toc : "none");
	}
	else if (priv->type.type == REJILLA_TRACK_TYPE_STREAM) {
		/* NOTE: this one can only a temporary file */
		result = rejilla_burn_session_get_tmp_file (session,
							    ".cdr",
							    &image,
							    error);
		REJILLA_JOB_LOG (self, "Output set (AUDIO) image = %s", image);
	}
	else /* other types don't need an output */
		return REJILLA_BURN_OK;

	if (result != REJILLA_BURN_OK)
		return result;

	priv->output = g_new0 (RejillaJobOutput, 1);
	priv->output->image = image;
	priv->output->toc = toc;

	if (rejilla_burn_session_get_flags (session) & REJILLA_BURN_FLAG_CHECK_SIZE)
		return rejilla_job_check_output_volume_space (self, error);

	return result;
}

static RejillaJob *
rejilla_job_get_next_active (RejillaJob *self)
{
	RejillaJobPrivate *priv;
	RejillaJob *next;

	/* since some jobs can return NOT_RUNNING after ::init, skip them */
	priv = REJILLA_JOB_PRIVATE (self);
	if (!priv->next)
		return NULL;

	next = priv->next;
	while (next) {
		priv = REJILLA_JOB_PRIVATE (next);

		if (priv->ctx)
			return next;

		next = priv->next;
	}

	return NULL;
}

static RejillaBurnResult
rejilla_job_item_start (RejillaTaskItem *item,
		        GError **error)
{
	RejillaJob *self;
	RejillaJobClass *klass;
	RejillaJobAction action;
	RejillaJobPrivate *priv;
	RejillaBurnResult result;

	/* This function is compulsory */
	self = REJILLA_JOB (item);
	priv = REJILLA_JOB_PRIVATE (self);

	/* skip jobs that are not active */
	if (!priv->ctx)
		return REJILLA_BURN_OK;

	/* set the output if need be */
	rejilla_job_get_action (self, &action);
	priv->linked = rejilla_job_get_next_active (self);

	if (!priv->linked) {
		/* that's the last job so is action is image it needs a file */
		if (action == REJILLA_JOB_ACTION_IMAGE) {
			result = rejilla_job_set_output_file (self, error);
			if (result != REJILLA_BURN_OK)
				return result;
		}
		else if (action == REJILLA_JOB_ACTION_RECORD) {
			RejillaBurnFlag flags;
			RejillaBurnSession *session;

			session = rejilla_task_ctx_get_session (priv->ctx);
			flags = rejilla_burn_session_get_flags (session);
			if (flags & REJILLA_BURN_FLAG_CHECK_SIZE
			&& !(flags & REJILLA_BURN_FLAG_OVERBURN)) {
				result = rejilla_job_check_output_disc_space (self, error);
				if (result != REJILLA_BURN_OK)
					return result;
			}
		}
	}
	else
		REJILLA_JOB_LOG (self, "linked to %s", G_OBJECT_TYPE_NAME (priv->linked));

	if (!rejilla_job_is_first_active (self)) {
		int fd [2];

		REJILLA_JOB_LOG (self, "creating input");

		/* setup a pipe */
		if (pipe (fd)) {
                        int errsv = errno;

			REJILLA_BURN_LOG ("A pipe couldn't be created");
			g_set_error (error,
				     REJILLA_BURN_ERROR,
				     REJILLA_BURN_ERROR_GENERAL,
				     _("An internal error occurred (%s)"),
				     g_strerror (errsv));

			return REJILLA_BURN_ERR;
		}

		/* NOTE: don't set O_NONBLOCK automatically as some plugins 
		 * don't like that (genisoimage, mkisofs) */
		priv->input = g_new0 (RejillaJobInput, 1);
		priv->input->in = fd [0];
		priv->input->out = fd [1];
	}

	klass = REJILLA_JOB_GET_CLASS (self);
	if (!klass->start) {
		REJILLA_JOB_LOG (self, "no ::start method");
		REJILLA_JOB_NOT_SUPPORTED (self);
	}

	result = klass->start (self, error);
	if (result == REJILLA_BURN_NOT_RUNNING) {
		/* this means that the task is already completed. This 
		 * must be returned by the last active job of the task
		 * (and usually the only one?) */

		if (priv->linked) {
			g_set_error (error,
				     REJILLA_BURN_ERROR,
				     REJILLA_BURN_ERROR_PLUGIN_MISBEHAVIOR,
				     _("\"%s\" did not behave properly"),
				     G_OBJECT_TYPE_NAME (self));
			return REJILLA_BURN_ERR;
		}
	}

	if (result == REJILLA_BURN_NOT_SUPPORTED) {
		/* only forgive this error when that's the last job and we're
		 * searching for a job to set the current track size */
		if (action != REJILLA_JOB_ACTION_SIZE) {
			g_set_error (error,
				     REJILLA_BURN_ERROR,
				     REJILLA_BURN_ERROR_PLUGIN_MISBEHAVIOR,
				     _("\"%s\" did not behave properly"),
				     G_OBJECT_TYPE_NAME (self));
			return REJILLA_BURN_ERR;
		}

		/* deactivate it */
		rejilla_job_deactivate (self);
	}

	return result;
}

static RejillaBurnResult
rejilla_job_item_clock_tick (RejillaTaskItem *item,
			     RejillaTaskCtx *ctx,
			     GError **error)
{
	RejillaJob *self;
	RejillaJobClass *klass;
	RejillaJobPrivate *priv;
	RejillaBurnResult result = REJILLA_BURN_OK;

	self = REJILLA_JOB (item);
	priv = REJILLA_JOB_PRIVATE (self);
	if (!priv->ctx)
		return REJILLA_BURN_OK;

	klass = REJILLA_JOB_GET_CLASS (self);
	if (klass->clock_tick)
		result = klass->clock_tick (self);

	return result;
}

static RejillaBurnResult
rejilla_job_disconnect (RejillaJob *self,
			GError **error)
{
	RejillaJobPrivate *priv;
	RejillaBurnResult result = REJILLA_BURN_OK;

	priv = REJILLA_JOB_PRIVATE (self);

	/* NOTE: this function is only called when there are no more track to 
	 * process */

	if (priv->linked) {
		RejillaJobPrivate *priv_link;

		REJILLA_JOB_LOG (self,
				 "disconnecting %s from %s",
				 G_OBJECT_TYPE_NAME (self),
				 G_OBJECT_TYPE_NAME (priv->linked));

		priv_link = REJILLA_JOB_PRIVATE (priv->linked);

		/* only close the input to tell the other end that we're
		 * finished with writing to the pipe */
		if (priv_link->input->out > 0) {
			close (priv_link->input->out);
			priv_link->input->out = 0;
		}
	}
	else if (priv->output) {
		rejilla_job_output_free (priv->output);
		priv->output = NULL;
	}

	if (priv->input) {
		REJILLA_JOB_LOG (self,
				 "closing connection for %s",
				 G_OBJECT_TYPE_NAME (self));

		rejilla_job_input_free (priv->input);
		priv->input = NULL;
	}

	return result;
}

static RejillaBurnResult
rejilla_job_item_stop (RejillaTaskItem *item,
		       RejillaTaskCtx *ctx,
		       GError **error)
{
	RejillaJob *self;
	RejillaJobClass *klass;
	RejillaJobPrivate *priv;
	RejillaBurnResult result = REJILLA_BURN_OK;

	self = REJILLA_JOB (item);
	priv = REJILLA_JOB_PRIVATE (self);

	if (!priv->ctx)
		return REJILLA_BURN_OK;

	REJILLA_JOB_LOG (self, "stopping");

	/* the order is important here */
	klass = REJILLA_JOB_GET_CLASS (self);
	if (klass->stop)
		result = klass->stop (self, error);

	rejilla_job_disconnect (self, error);

	if (priv->ctx) {
		g_object_unref (priv->ctx);
		priv->ctx = NULL;
	}

	return REJILLA_BURN_OK;
}

static void
rejilla_job_iface_init_task_item (RejillaTaskItemIFace *iface)
{
	iface->next = rejilla_job_item_next;
	iface->previous = rejilla_job_item_previous;
	iface->link = rejilla_job_item_link;
	iface->activate = rejilla_job_item_activate;
	iface->is_active = rejilla_job_item_is_active;
	iface->start = rejilla_job_item_start;
	iface->stop = rejilla_job_item_stop;
	iface->clock_tick = rejilla_job_item_clock_tick;
}

RejillaBurnResult
rejilla_job_tag_lookup (RejillaJob *self,
			const gchar *tag,
			GValue **value)
{
	RejillaJobPrivate *priv;
	RejillaBurnSession *session;

	REJILLA_JOB_DEBUG (self);

	priv = REJILLA_JOB_PRIVATE (self);

	session = rejilla_task_ctx_get_session (priv->ctx);
	return rejilla_burn_session_tag_lookup (session,
						tag,
						value);
}

RejillaBurnResult
rejilla_job_tag_add (RejillaJob *self,
		     const gchar *tag,
		     GValue *value)
{
	RejillaJobPrivate *priv;
	RejillaBurnSession *session;

	REJILLA_JOB_DEBUG (self);

	priv = REJILLA_JOB_PRIVATE (self);

	if (!rejilla_job_is_last_active (self))
		return REJILLA_BURN_ERR;

	session = rejilla_task_ctx_get_session (priv->ctx);
	rejilla_burn_session_tag_add (session,
				      tag,
				      value);

	return REJILLA_BURN_OK;
}

/**
 * Means a job successfully completed its task.
 * track can be NULL, depending on whether or not the job created a track.
 */

RejillaBurnResult
rejilla_job_add_track (RejillaJob *self,
		       RejillaTrack *track)
{
	RejillaJobPrivate *priv;
	RejillaJobAction action;

	REJILLA_JOB_DEBUG (self);

	priv = REJILLA_JOB_PRIVATE (self);

	/* to add a track to the session, a job :
	 * - must be the last running in the chain
	 * - the action for the job must be IMAGE */

	action = REJILLA_JOB_ACTION_NONE;
	rejilla_job_get_action (self, &action);
	if (action != REJILLA_JOB_ACTION_IMAGE)
		return REJILLA_BURN_ERR;

	if (!rejilla_job_is_last_active (self))
		return REJILLA_BURN_ERR;

	return rejilla_task_ctx_add_track (priv->ctx, track);
}

RejillaBurnResult
rejilla_job_finished_session (RejillaJob *self)
{
	GError *error = NULL;
	RejillaJobClass *klass;
	RejillaJobPrivate *priv;
	RejillaBurnResult result;

	priv = REJILLA_JOB_PRIVATE (self);

	REJILLA_JOB_LOG (self, "Finished successfully session");

	if (rejilla_job_is_last_active (self))
		return rejilla_task_ctx_finished (priv->ctx);

	if (!rejilla_job_is_first_active (self)) {
		/* This job is apparently a go between job.
		 * It should only call for a stop on an error. */
		REJILLA_JOB_LOG (self, "is not a leader");
		error = g_error_new (REJILLA_BURN_ERROR,
				     REJILLA_BURN_ERROR_PLUGIN_MISBEHAVIOR,
				     _("\"%s\" did not behave properly"),
				     G_OBJECT_TYPE_NAME (self));
		return rejilla_task_ctx_error (priv->ctx,
					       REJILLA_BURN_ERR,
					       error);
	}

	/* call the stop method of the job since it's finished */ 
	klass = REJILLA_JOB_GET_CLASS (self);
	if (klass->stop) {
		result = klass->stop (self, &error);
		if (result != REJILLA_BURN_OK)
			return rejilla_task_ctx_error (priv->ctx,
						       result,
						       error);
	}

	/* this job is finished but it's not the leader so
	 * the task is not finished. Close the pipe on
	 * one side to let the next job know that there
	 * isn't any more data to be expected */
	result = rejilla_job_disconnect (self, &error);
	g_object_unref (priv->ctx);
	priv->ctx = NULL;

	if (result != REJILLA_BURN_OK)
		return rejilla_task_ctx_error (priv->ctx,
					       result,
					       error);

	return REJILLA_BURN_OK;
}

RejillaBurnResult
rejilla_job_finished_track (RejillaJob *self)
{
	GError *error = NULL;
	RejillaJobPrivate *priv;
	RejillaBurnResult result;

	priv = REJILLA_JOB_PRIVATE (self);

	REJILLA_JOB_LOG (self, "Finished track successfully");

	/* we first check if it's the first job */
	if (rejilla_job_is_first_active (self)) {
		RejillaJobClass *klass;

		/* call ::stop for the job since it's finished */ 
		klass = REJILLA_JOB_GET_CLASS (self);
		if (klass->stop) {
			result = klass->stop (self, &error);

			if (result != REJILLA_BURN_OK)
				return rejilla_task_ctx_error (priv->ctx,
							       result,
							       error);
		}

		/* see if there is another track to process */
		result = rejilla_task_ctx_next_track (priv->ctx);
		if (result == REJILLA_BURN_RETRY) {
			/* there is another track to process: don't close the
			 * input of the next connected job. Leave it active */
			return REJILLA_BURN_OK;
		}

		if (!rejilla_job_is_last_active (self)) {
			/* this job is finished but it's not the leader so the
			 * task is not finished. Close the pipe on one side to
			 * let the next job know that there isn't any more data
			 * to be expected */
			result = rejilla_job_disconnect (self, &error);

			rejilla_job_deactivate (self);

			if (result != REJILLA_BURN_OK)
				return rejilla_task_ctx_error (priv->ctx,
							       result,
							       error);

			return REJILLA_BURN_OK;
		}
	}
	else if (!rejilla_job_is_last_active (self)) {
		/* This job is apparently a go between job. It should only call
		 * for a stop on an error. */
		REJILLA_JOB_LOG (self, "is not a leader");
		error = g_error_new (REJILLA_BURN_ERROR,
				     REJILLA_BURN_ERROR_PLUGIN_MISBEHAVIOR,
				     _("\"%s\" did not behave properly"),
				     G_OBJECT_TYPE_NAME (self));
		return rejilla_task_ctx_error (priv->ctx, REJILLA_BURN_ERR, error);
	}

	return rejilla_task_ctx_finished (priv->ctx);
}

/**
 * means a job didn't successfully completed its task
 */

RejillaBurnResult
rejilla_job_error (RejillaJob *self, GError *error)
{
	GValue instance_and_params [2];
	RejillaJobPrivate *priv;
	GValue return_value;

	REJILLA_JOB_DEBUG (self);

	REJILLA_JOB_LOG (self, "finished with an error");

	priv = REJILLA_JOB_PRIVATE (self);

	instance_and_params [0].g_type = 0;
	g_value_init (instance_and_params, G_TYPE_FROM_INSTANCE (self));
	g_value_set_instance (instance_and_params, self);

	instance_and_params [1].g_type = 0;
	g_value_init (instance_and_params + 1, G_TYPE_INT);

	if (error)
		g_value_set_int (instance_and_params + 1, error->code);
	else
		g_value_set_int (instance_and_params + 1, REJILLA_BURN_ERROR_GENERAL);

	return_value.g_type = 0;
	g_value_init (&return_value, G_TYPE_INT);
	g_value_set_int (&return_value, REJILLA_BURN_ERR);

	/* There was an error: signal it. That's mainly done
	 * for RejillaBurnCaps to override the result value */
	g_signal_emitv (instance_and_params,
			rejilla_job_signals [ERROR_SIGNAL],
			0,
			&return_value);

	g_value_unset (instance_and_params);

	REJILLA_JOB_LOG (self,
			 "asked to stop because of an error\n"
			 "\terror\t\t= %i\n"
			 "\tmessage\t= \"%s\"",
			 error ? error->code:0,
			 error ? error->message:"no message");

	return rejilla_task_ctx_error (priv->ctx, g_value_get_int (&return_value), error);
}

/**
 * Used to retrieve config for a job
 * If the parameter is missing for the next 4 functions
 * it allows to test if they could be used
 */

RejillaBurnResult
rejilla_job_get_fd_in (RejillaJob *self, int *fd_in)
{
	RejillaJobPrivate *priv;

	REJILLA_JOB_DEBUG (self);

	priv = REJILLA_JOB_PRIVATE (self);

	if (!priv->input)
		return REJILLA_BURN_ERR;

	if (!fd_in)
		return REJILLA_BURN_OK;

	*fd_in = priv->input->in;
	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_job_set_nonblocking_fd (int fd, GError **error)
{
	long flags = 0;

	if (fcntl (fd, F_GETFL, &flags) != -1) {
		/* Unfortunately some plugin (mkisofs/genisofs don't like 
		 * O_NONBLOCK (which is a shame) so we don't set them
		 * automatically but still offer that possibility. */
		flags |= O_NONBLOCK;
		if (fcntl (fd, F_SETFL, flags) == -1) {
			REJILLA_BURN_LOG ("couldn't set non blocking mode");
			g_set_error (error,
				     REJILLA_BURN_ERROR,
				     REJILLA_BURN_ERROR_GENERAL,
				     _("An internal error occurred"));
			return REJILLA_BURN_ERR;
		}
	}
	else {
		REJILLA_BURN_LOG ("couldn't get pipe flags");
		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_GENERAL,
			     _("An internal error occurred"));
		return REJILLA_BURN_ERR;
	}

	return REJILLA_BURN_OK;
}

RejillaBurnResult
rejilla_job_set_nonblocking (RejillaJob *self,
			     GError **error)
{
	RejillaBurnResult result;
	RejillaJobPrivate *priv;
	int fd;

	REJILLA_JOB_DEBUG (self);

	priv = REJILLA_JOB_PRIVATE (self);

	fd = -1;
	if (rejilla_job_get_fd_in (self, &fd) == REJILLA_BURN_OK) {
		result = rejilla_job_set_nonblocking_fd (fd, error);
		if (result != REJILLA_BURN_OK)
			return result;
	}

	fd = -1;
	if (rejilla_job_get_fd_out (self, &fd) == REJILLA_BURN_OK) {
		result = rejilla_job_set_nonblocking_fd (fd, error);
		if (result != REJILLA_BURN_OK)
			return result;
	}

	return REJILLA_BURN_OK;
}

RejillaBurnResult
rejilla_job_get_current_track (RejillaJob *self,
			       RejillaTrack **track)
{
	RejillaJobPrivate *priv;

	REJILLA_JOB_DEBUG (self);

	priv = REJILLA_JOB_PRIVATE (self);
	if (!track)
		return REJILLA_BURN_OK;

	return rejilla_task_ctx_get_current_track (priv->ctx, track);
}

RejillaBurnResult
rejilla_job_get_done_tracks (RejillaJob *self, GSList **tracks)
{
	RejillaJobPrivate *priv;

	REJILLA_JOB_DEBUG (self);

	/* tracks already done are those that are in session */
	priv = REJILLA_JOB_PRIVATE (self);
	return rejilla_task_ctx_get_stored_tracks (priv->ctx, tracks);
}

RejillaBurnResult
rejilla_job_get_tracks (RejillaJob *self, GSList **tracks)
{
	RejillaJobPrivate *priv;
	RejillaBurnSession *session;

	REJILLA_JOB_DEBUG (self);

	g_return_val_if_fail (tracks != NULL, REJILLA_BURN_ERR);

	priv = REJILLA_JOB_PRIVATE (self);
	session = rejilla_task_ctx_get_session (priv->ctx);
	*tracks = rejilla_burn_session_get_tracks (session);
	return REJILLA_BURN_OK;
}

RejillaBurnResult
rejilla_job_get_fd_out (RejillaJob *self, int *fd_out)
{
	RejillaJobPrivate *priv;

	REJILLA_JOB_DEBUG (self);

	priv = REJILLA_JOB_PRIVATE (self);

	if (!priv->linked)
		return REJILLA_BURN_ERR;

	if (!fd_out)
		return REJILLA_BURN_OK;

	priv = REJILLA_JOB_PRIVATE (priv->linked);
	if (!priv->input)
		return REJILLA_BURN_ERR;

	*fd_out = priv->input->out;
	return REJILLA_BURN_OK;
}

RejillaBurnResult
rejilla_job_get_image_output (RejillaJob *self,
			      gchar **image,
			      gchar **toc)
{
	RejillaJobPrivate *priv;

	REJILLA_JOB_DEBUG (self);

	priv = REJILLA_JOB_PRIVATE (self);

	if (!priv->output)
		return REJILLA_BURN_ERR;

	if (image)
		*image = g_strdup (priv->output->image);

	if (toc)
		*toc = g_strdup (priv->output->toc);

	return REJILLA_BURN_OK;
}

RejillaBurnResult
rejilla_job_get_audio_output (RejillaJob *self,
			      gchar **path)
{
	RejillaJobPrivate *priv;

	REJILLA_JOB_DEBUG (self);

	priv = REJILLA_JOB_PRIVATE (self);
	if (!priv->output)
		return REJILLA_BURN_ERR;

	if (path)
		*path = g_strdup (priv->output->image);

	return REJILLA_BURN_OK;
}

RejillaBurnResult
rejilla_job_get_flags (RejillaJob *self, RejillaBurnFlag *flags)
{
	RejillaBurnSession *session;
	RejillaJobPrivate *priv;

	REJILLA_JOB_DEBUG (self);

	g_return_val_if_fail (flags != NULL, REJILLA_BURN_ERR);

	priv = REJILLA_JOB_PRIVATE (self);
	session = rejilla_task_ctx_get_session (priv->ctx);
	*flags = rejilla_burn_session_get_flags (session);

	return REJILLA_BURN_OK;
}

RejillaBurnResult
rejilla_job_get_input_type (RejillaJob *self, RejillaTrackType *type)
{
	RejillaBurnResult result = REJILLA_BURN_OK;
	RejillaJobPrivate *priv;

	REJILLA_JOB_DEBUG (self);

	priv = REJILLA_JOB_PRIVATE (self);
	if (!priv->previous) {
		RejillaBurnSession *session;

		session = rejilla_task_ctx_get_session (priv->ctx);
		result = rejilla_burn_session_get_input_type (session, type);
	}
	else {
		RejillaJobPrivate *prev_priv;

		prev_priv = REJILLA_JOB_PRIVATE (priv->previous);
		memcpy (type, &prev_priv->type, sizeof (RejillaTrackType));
		result = REJILLA_BURN_OK;
	}

	return result;
}

RejillaBurnResult
rejilla_job_get_output_type (RejillaJob *self, RejillaTrackType *type)
{
	RejillaJobPrivate *priv;

	REJILLA_JOB_DEBUG (self);

	priv = REJILLA_JOB_PRIVATE (self);

	memcpy (type, &priv->type, sizeof (RejillaTrackType));
	return REJILLA_BURN_OK;
}

RejillaBurnResult
rejilla_job_get_action (RejillaJob *self, RejillaJobAction *action)
{
	RejillaJobPrivate *priv;
	RejillaTaskAction task_action;

	REJILLA_JOB_DEBUG (self);

	g_return_val_if_fail (action != NULL, REJILLA_BURN_ERR);

	priv = REJILLA_JOB_PRIVATE (self);

	if (!rejilla_job_is_last_active (self)) {
		*action = REJILLA_JOB_ACTION_IMAGE;
		return REJILLA_BURN_OK;
	}

	task_action = rejilla_task_ctx_get_action (priv->ctx);
	switch (task_action) {
	case REJILLA_TASK_ACTION_NONE:
		*action = REJILLA_JOB_ACTION_SIZE;
		break;

	case REJILLA_TASK_ACTION_ERASE:
		*action = REJILLA_JOB_ACTION_ERASE;
		break;

	case REJILLA_TASK_ACTION_NORMAL:
		if (priv->type.type == REJILLA_TRACK_TYPE_DISC)
			*action = REJILLA_JOB_ACTION_RECORD;
		else
			*action = REJILLA_JOB_ACTION_IMAGE;
		break;

	case REJILLA_TASK_ACTION_CHECKSUM:
		*action = REJILLA_JOB_ACTION_CHECKSUM;
		break;

	default:
		*action = REJILLA_JOB_ACTION_NONE;
		break;
	}

	return REJILLA_BURN_OK;
}

RejillaBurnResult
rejilla_job_get_bus_target_lun (RejillaJob *self, gchar **BTL)
{
	RejillaBurnSession *session;
	RejillaJobPrivate *priv;
	RejillaDrive *drive;

	REJILLA_JOB_DEBUG (self);

	g_return_val_if_fail (BTL != NULL, REJILLA_BURN_ERR);

	priv = REJILLA_JOB_PRIVATE (self);
	session = rejilla_task_ctx_get_session (priv->ctx);

	drive = rejilla_burn_session_get_burner (session);
	*BTL = rejilla_drive_get_bus_target_lun_string (drive);

	return REJILLA_BURN_OK;
}

RejillaBurnResult
rejilla_job_get_device (RejillaJob *self, gchar **device)
{
	RejillaBurnSession *session;
	RejillaJobPrivate *priv;
	RejillaDrive *drive;
	const gchar *path;

	REJILLA_JOB_DEBUG (self);

	g_return_val_if_fail (device != NULL, REJILLA_BURN_ERR);

	priv = REJILLA_JOB_PRIVATE (self);
	session = rejilla_task_ctx_get_session (priv->ctx);

	drive = rejilla_burn_session_get_burner (session);
	path = rejilla_drive_get_device (drive);
	*device = g_strdup (path);

	return REJILLA_BURN_OK;
}

RejillaBurnResult
rejilla_job_get_media (RejillaJob *self, RejillaMedia *media)
{
	RejillaBurnSession *session;
	RejillaJobPrivate *priv;

	REJILLA_JOB_DEBUG (self);

	g_return_val_if_fail (media != NULL, REJILLA_BURN_ERR);

	priv = REJILLA_JOB_PRIVATE (self);
	session = rejilla_task_ctx_get_session (priv->ctx);
	*media = rejilla_burn_session_get_dest_media (session);

	return REJILLA_BURN_OK;
}

RejillaBurnResult
rejilla_job_get_medium (RejillaJob *job, RejillaMedium **medium)
{
	RejillaBurnSession *session;
	RejillaJobPrivate *priv;
	RejillaDrive *drive;

	REJILLA_JOB_DEBUG (job);

	g_return_val_if_fail (medium != NULL, REJILLA_BURN_ERR);

	priv = REJILLA_JOB_PRIVATE (job);
	session = rejilla_task_ctx_get_session (priv->ctx);
	drive = rejilla_burn_session_get_burner (session);
	*medium = rejilla_drive_get_medium (drive);

	if (!(*medium))
		return REJILLA_BURN_ERR;

	g_object_ref (*medium);
	return REJILLA_BURN_OK;
}

RejillaBurnResult
rejilla_job_get_last_session_address (RejillaJob *self, goffset *address)
{
	RejillaBurnSession *session;
	RejillaJobPrivate *priv;
	RejillaMedium *medium;
	RejillaDrive *drive;

	REJILLA_JOB_DEBUG (self);

	g_return_val_if_fail (address != NULL, REJILLA_BURN_ERR);

	priv = REJILLA_JOB_PRIVATE (self);
	session = rejilla_task_ctx_get_session (priv->ctx);
	drive = rejilla_burn_session_get_burner (session);
	medium = rejilla_drive_get_medium (drive);
	if (rejilla_medium_get_last_data_track_address (medium, NULL, address))
		return REJILLA_BURN_OK;

	return REJILLA_BURN_ERR;
}

RejillaBurnResult
rejilla_job_get_next_writable_address (RejillaJob *self, goffset *address)
{
	RejillaBurnSession *session;
	RejillaJobPrivate *priv;
	RejillaMedium *medium;
	RejillaDrive *drive;

	REJILLA_JOB_DEBUG (self);

	g_return_val_if_fail (address != NULL, REJILLA_BURN_ERR);

	priv = REJILLA_JOB_PRIVATE (self);
	session = rejilla_task_ctx_get_session (priv->ctx);
	drive = rejilla_burn_session_get_burner (session);
	medium = rejilla_drive_get_medium (drive);
	*address = rejilla_medium_get_next_writable_address (medium);

	return REJILLA_BURN_OK;
}

RejillaBurnResult
rejilla_job_get_rate (RejillaJob *self, guint64 *rate)
{
	RejillaBurnSession *session;
	RejillaJobPrivate *priv;

	g_return_val_if_fail (rate != NULL, REJILLA_BURN_ERR);

	priv = REJILLA_JOB_PRIVATE (self);
	session = rejilla_task_ctx_get_session (priv->ctx);
	*rate = rejilla_burn_session_get_rate (session);

	return REJILLA_BURN_OK;
}

static int
_round_speed (float number)
{
	int retval = (gint) number;

	/* NOTE: number must always be positive */
	number -= (float) retval;
	if (number >= 0.5)
		return retval + 1;

	return retval;
}

RejillaBurnResult
rejilla_job_get_speed (RejillaJob *self, guint *speed)
{
	RejillaBurnSession *session;
	RejillaJobPrivate *priv;
	RejillaMedia media;
	guint64 rate;

	REJILLA_JOB_DEBUG (self);

	g_return_val_if_fail (speed != NULL, REJILLA_BURN_ERR);

	priv = REJILLA_JOB_PRIVATE (self);
	session = rejilla_task_ctx_get_session (priv->ctx);
	rate = rejilla_burn_session_get_rate (session);

	media = rejilla_burn_session_get_dest_media (session);
	if (media & REJILLA_MEDIUM_DVD)
		*speed = _round_speed (REJILLA_RATE_TO_SPEED_DVD (rate));
	else if (media & REJILLA_MEDIUM_BD)
		*speed = _round_speed (REJILLA_RATE_TO_SPEED_BD (rate));
	else
		*speed = _round_speed (REJILLA_RATE_TO_SPEED_CD (rate));

	return REJILLA_BURN_OK;
}

RejillaBurnResult
rejilla_job_get_max_rate (RejillaJob *self, guint64 *rate)
{
	RejillaBurnSession *session;
	RejillaJobPrivate *priv;
	RejillaMedium *medium;
	RejillaDrive *drive;

	REJILLA_JOB_DEBUG (self);

	g_return_val_if_fail (rate != NULL, REJILLA_BURN_ERR);

	priv = REJILLA_JOB_PRIVATE (self);
	session = rejilla_task_ctx_get_session (priv->ctx);

	drive = rejilla_burn_session_get_burner (session);
	medium = rejilla_drive_get_medium (drive);

	if (!medium)
		return REJILLA_BURN_NOT_READY;

	*rate = rejilla_medium_get_max_write_speed (medium);

	return REJILLA_BURN_OK;
}

RejillaBurnResult
rejilla_job_get_max_speed (RejillaJob *self, guint *speed)
{
	RejillaBurnSession *session;
	RejillaJobPrivate *priv;
	RejillaMedium *medium;
	RejillaDrive *drive;
	RejillaMedia media;
	guint64 rate;

	REJILLA_JOB_DEBUG (self);

	g_return_val_if_fail (speed != NULL, REJILLA_BURN_ERR);

	priv = REJILLA_JOB_PRIVATE (self);
	session = rejilla_task_ctx_get_session (priv->ctx);

	drive = rejilla_burn_session_get_burner (session);
	medium = rejilla_drive_get_medium (drive);
	if (!medium)
		return REJILLA_BURN_NOT_READY;

	rate = rejilla_medium_get_max_write_speed (medium);
	media = rejilla_medium_get_status (medium);
	if (media & REJILLA_MEDIUM_DVD)
		*speed = _round_speed (REJILLA_RATE_TO_SPEED_DVD (rate));
	else if (media & REJILLA_MEDIUM_BD)
		*speed = _round_speed (REJILLA_RATE_TO_SPEED_BD (rate));
	else
		*speed = _round_speed (REJILLA_RATE_TO_SPEED_CD (rate));

	return REJILLA_BURN_OK;
}

RejillaBurnResult
rejilla_job_get_tmp_file (RejillaJob *self,
			  const gchar *suffix,
			  gchar **output,
			  GError **error)
{
	RejillaBurnSession *session;
	RejillaJobPrivate *priv;

	priv = REJILLA_JOB_PRIVATE (self);
	session = rejilla_task_ctx_get_session (priv->ctx);
	return rejilla_burn_session_get_tmp_file (session,
						  suffix,
						  output,
						  error);
}

RejillaBurnResult
rejilla_job_get_tmp_dir (RejillaJob *self,
			 gchar **output,
			 GError **error)
{
	RejillaBurnSession *session;
	RejillaJobPrivate *priv;

	REJILLA_JOB_DEBUG (self);

	priv = REJILLA_JOB_PRIVATE (self);
	session = rejilla_task_ctx_get_session (priv->ctx);
	rejilla_burn_session_get_tmp_dir (session,
					  output,
					  error);

	return REJILLA_BURN_OK;
}

RejillaBurnResult
rejilla_job_get_audio_title (RejillaJob *self, gchar **album)
{
	RejillaBurnSession *session;
	RejillaJobPrivate *priv;

	REJILLA_JOB_DEBUG (self);

	g_return_val_if_fail (album != NULL, REJILLA_BURN_ERR);

	priv = REJILLA_JOB_PRIVATE (self);
	session = rejilla_task_ctx_get_session (priv->ctx);

	*album = g_strdup (rejilla_burn_session_get_label (session));
	return REJILLA_BURN_OK;
}

RejillaBurnResult
rejilla_job_get_data_label (RejillaJob *self, gchar **label)
{
	RejillaBurnSession *session;
	RejillaJobPrivate *priv;

	REJILLA_JOB_DEBUG (self);

	g_return_val_if_fail (label != NULL, REJILLA_BURN_ERR);

	priv = REJILLA_JOB_PRIVATE (self);
	session = rejilla_task_ctx_get_session (priv->ctx);

	*label = g_strdup (rejilla_burn_session_get_label (session));
	return REJILLA_BURN_OK;
}

RejillaBurnResult
rejilla_job_get_session_output_size (RejillaJob *self,
				     goffset *blocks,
				     goffset *bytes)
{
	RejillaJobPrivate *priv;

	REJILLA_JOB_DEBUG (self);

	priv = REJILLA_JOB_PRIVATE (self);
	return rejilla_task_ctx_get_session_output_size (priv->ctx, blocks, bytes);
}

/**
 * Starts task internal timer 
 */

RejillaBurnResult
rejilla_job_start_progress (RejillaJob *self,
			    gboolean force)
{
	RejillaJobPrivate *priv;

	/* Turn this off as otherwise it floods bug reports */
	// REJILLA_JOB_DEBUG (self);

	priv = REJILLA_JOB_PRIVATE (self);
	if (priv->next)
		return REJILLA_BURN_NOT_RUNNING;

	return rejilla_task_ctx_start_progress (priv->ctx, force);
}

RejillaBurnResult
rejilla_job_reset_progress (RejillaJob *self)
{
	RejillaJobPrivate *priv;

	priv = REJILLA_JOB_PRIVATE (self);
	if (priv->next)
		return REJILLA_BURN_ERR;

	return rejilla_task_ctx_reset_progress (priv->ctx);
}

/**
 * these should be used to set the different values of the task by the jobs
 */

RejillaBurnResult
rejilla_job_set_progress (RejillaJob *self,
			  gdouble progress)
{
	RejillaJobPrivate *priv;

	/* Turn this off as it floods bug reports */
	//REJILLA_JOB_LOG (self, "Called rejilla_job_set_progress (%lf)", progress);

	priv = REJILLA_JOB_PRIVATE (self);
	if (priv->next)
		return REJILLA_BURN_ERR;

	if (progress < 0.0 || progress > 1.0) {
		REJILLA_JOB_LOG (self, "Tried to set an insane progress value (%lf)", progress);
		return REJILLA_BURN_ERR;
	}

	return rejilla_task_ctx_set_progress (priv->ctx, progress);
}

RejillaBurnResult
rejilla_job_set_current_action (RejillaJob *self,
				RejillaBurnAction action,
				const gchar *string,
				gboolean force)
{
	RejillaJobPrivate *priv;

	REJILLA_JOB_DEBUG (self);

	priv = REJILLA_JOB_PRIVATE (self);
	if (!rejilla_job_is_last_active (self))
		return REJILLA_BURN_NOT_RUNNING;

	return rejilla_task_ctx_set_current_action (priv->ctx,
						    action,
						    string,
						    force);
}

RejillaBurnResult
rejilla_job_get_current_action (RejillaJob *self,
				RejillaBurnAction *action)
{
	RejillaJobPrivate *priv;

	REJILLA_JOB_DEBUG (self);

	g_return_val_if_fail (action != NULL, REJILLA_BURN_ERR);

	priv = REJILLA_JOB_PRIVATE (self);

	if (!priv->ctx) {
		REJILLA_JOB_LOG (self,
				 "called %s whereas it wasn't running",
				 G_STRFUNC);
		return REJILLA_BURN_NOT_RUNNING;
	}

	return rejilla_task_ctx_get_current_action (priv->ctx, action);
}

RejillaBurnResult
rejilla_job_set_rate (RejillaJob *self,
		      gint64 rate)
{
	RejillaJobPrivate *priv;

	/* Turn this off as otherwise it floods bug reports */
	// REJILLA_JOB_DEBUG (self);

	priv = REJILLA_JOB_PRIVATE (self);
	if (priv->next)
		return REJILLA_BURN_NOT_RUNNING;

	return rejilla_task_ctx_set_rate (priv->ctx, rate);
}

RejillaBurnResult
rejilla_job_set_output_size_for_current_track (RejillaJob *self,
					       goffset sectors,
					       goffset bytes)
{
	RejillaJobPrivate *priv;

	/* this function can only be used by the last job which is not recording
	 * all other jobs trying to set this value will be ignored.
	 * It should be used mostly during a fake running. This value is stored
	 * by the task context as the amount of bytes/blocks produced by a task.
	 * That's why it's not possible to set the DATA type number of files.
	 * NOTE: the values passed on by this function to context may be added 
	 * to other when there are multiple tracks. */
	REJILLA_JOB_DEBUG (self);

	priv = REJILLA_JOB_PRIVATE (self);

	if (!rejilla_job_is_last_active (self))
		return REJILLA_BURN_ERR;

	return rejilla_task_ctx_set_output_size_for_current_track (priv->ctx,
								   sectors,
								   bytes);
}

RejillaBurnResult
rejilla_job_set_written_track (RejillaJob *self,
			       goffset written)
{
	RejillaJobPrivate *priv;

	/* Turn this off as otherwise it floods bug reports */
	// REJILLA_JOB_DEBUG (self);

	priv = REJILLA_JOB_PRIVATE (self);
	if (priv->next)
		return REJILLA_BURN_NOT_RUNNING;

	return rejilla_task_ctx_set_written_track (priv->ctx, written);
}

RejillaBurnResult
rejilla_job_set_written_session (RejillaJob *self,
				 goffset written)
{
	RejillaJobPrivate *priv;

	/* Turn this off as otherwise it floods bug reports */
	// REJILLA_JOB_DEBUG (self);

	priv = REJILLA_JOB_PRIVATE (self);
	if (priv->next)
		return REJILLA_BURN_NOT_RUNNING;

	return rejilla_task_ctx_set_written_session (priv->ctx, written);
}

RejillaBurnResult
rejilla_job_set_use_average_rate (RejillaJob *self, gboolean value)
{
	RejillaJobPrivate *priv;

	REJILLA_JOB_DEBUG (self);

	priv = REJILLA_JOB_PRIVATE (self);
	if (priv->next)
		return REJILLA_BURN_NOT_RUNNING;

	return rejilla_task_ctx_set_use_average (priv->ctx, value);
}

void
rejilla_job_set_dangerous (RejillaJob *self, gboolean value)
{
	RejillaJobPrivate *priv;

	REJILLA_JOB_DEBUG (self);

	priv = REJILLA_JOB_PRIVATE (self);
	if (priv->ctx)
		rejilla_task_ctx_set_dangerous (priv->ctx, value);
}

/**
 * used for debugging
 */

void
rejilla_job_log_message (RejillaJob *self,
			 const gchar *location,
			 const gchar *format,
			 ...)
{
	va_list arg_list;
	RejillaJobPrivate *priv;
	RejillaBurnSession *session;

	g_return_if_fail (REJILLA_IS_JOB (self));
	g_return_if_fail (format != NULL);

	priv = REJILLA_JOB_PRIVATE (self);
	session = rejilla_task_ctx_get_session (priv->ctx);

	va_start (arg_list, format);
	rejilla_burn_session_logv (session, format, arg_list);
	va_end (arg_list);

	va_start (arg_list, format);
	rejilla_burn_debug_messagev (location, format, arg_list);
	va_end (arg_list);
}

/**
 * Object creation stuff
 */

static void
rejilla_job_get_property (GObject *object,
			  guint prop_id,
			  GValue *value,
			  GParamSpec *pspec)
{
	RejillaJobPrivate *priv;
	RejillaTrackType *ptr;

	priv = REJILLA_JOB_PRIVATE (object);

	switch (prop_id) {
	case PROP_OUTPUT:
		ptr = g_value_get_pointer (value);
		memcpy (ptr, &priv->type, sizeof (RejillaTrackType));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
rejilla_job_set_property (GObject *object,
			  guint prop_id,
			  const GValue *value,
			  GParamSpec *pspec)
{
	RejillaJobPrivate *priv;
	RejillaTrackType *ptr;

	priv = REJILLA_JOB_PRIVATE (object);

	switch (prop_id) {
	case PROP_OUTPUT:
		ptr = g_value_get_pointer (value);
		if (!ptr) {
			priv->type.type = REJILLA_TRACK_TYPE_NONE;
			priv->type.subtype.media = REJILLA_MEDIUM_NONE;
		}
		else
			memcpy (&priv->type, ptr, sizeof (RejillaTrackType));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
		break;
	}
}

static void
rejilla_job_finalize (GObject *object)
{
	RejillaJobPrivate *priv;

	priv = REJILLA_JOB_PRIVATE (object);

	if (priv->ctx) {
		g_object_unref (priv->ctx);
		priv->ctx = NULL;
	}

	if (priv->previous) {
		g_object_unref (priv->previous);
		priv->previous = NULL;
	}

	if (priv->input) {
		rejilla_job_input_free (priv->input);
		priv->input = NULL;
	}

	if (priv->linked)
		priv->linked = NULL;

	if (priv->output) {
		rejilla_job_output_free (priv->output);
		priv->output = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
rejilla_job_class_init (RejillaJobClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (RejillaJobPrivate));

	parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = rejilla_job_finalize;
	object_class->set_property = rejilla_job_set_property;
	object_class->get_property = rejilla_job_get_property;

	rejilla_job_signals [ERROR_SIGNAL] =
	    g_signal_new ("error",
			  G_TYPE_FROM_CLASS (klass),
			  G_SIGNAL_RUN_LAST|G_SIGNAL_NO_RECURSE,
			  G_STRUCT_OFFSET (RejillaJobClass, error),
			  NULL, NULL,
			  rejilla_marshal_INT__INT,
			  G_TYPE_INT, 1, G_TYPE_INT);

	g_object_class_install_property (object_class,
					 PROP_OUTPUT,
					 g_param_spec_pointer ("output",
							       "The type the job must output",
							       "The type the job must output",
							       G_PARAM_READWRITE|G_PARAM_CONSTRUCT_ONLY));
}

static void
rejilla_job_init (RejillaJob *obj)
{ }
