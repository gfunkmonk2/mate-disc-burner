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

#ifndef _BURN_TASK_CTX_H_
#define _BURN_TASK_CTX_H_

#include <glib-object.h>

#include "burn-basics.h"
#include "rejilla-session.h"

G_BEGIN_DECLS

#define REJILLA_TYPE_TASK_CTX             (rejilla_task_ctx_get_type ())
#define REJILLA_TASK_CTX(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_TASK_CTX, RejillaTaskCtx))
#define REJILLA_TASK_CTX_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_TASK_CTX, RejillaTaskCtxClass))
#define REJILLA_IS_TASK_CTX(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_TASK_CTX))
#define REJILLA_IS_TASK_CTX_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_TASK_CTX))
#define REJILLA_TASK_CTX_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_TASK_CTX, RejillaTaskCtxClass))

typedef enum {
	REJILLA_TASK_ACTION_NONE		= 0,
	REJILLA_TASK_ACTION_ERASE,
	REJILLA_TASK_ACTION_NORMAL,
	REJILLA_TASK_ACTION_CHECKSUM,
} RejillaTaskAction;

typedef struct _RejillaTaskCtxClass RejillaTaskCtxClass;
typedef struct _RejillaTaskCtx RejillaTaskCtx;

struct _RejillaTaskCtxClass
{
	GObjectClass parent_class;

	void			(* finished)		(RejillaTaskCtx *ctx,
							 RejillaBurnResult retval,
							 GError *error);

	/* signals */
	void			(*progress_changed)	(RejillaTaskCtx *task,
							 gdouble fraction,
							 glong remaining_time);
	void			(*action_changed)	(RejillaTaskCtx *task,
							 RejillaBurnAction action);
};

struct _RejillaTaskCtx
{
	GObject parent_instance;
};

GType rejilla_task_ctx_get_type (void) G_GNUC_CONST;

void
rejilla_task_ctx_reset (RejillaTaskCtx *ctx);

void
rejilla_task_ctx_set_fake (RejillaTaskCtx *ctx,
			   gboolean fake);

void
rejilla_task_ctx_set_dangerous (RejillaTaskCtx *ctx,
				gboolean value);

guint
rejilla_task_ctx_get_dangerous (RejillaTaskCtx *ctx);

/**
 * Used to get the session it is associated with
 */

RejillaBurnSession *
rejilla_task_ctx_get_session (RejillaTaskCtx *ctx);

RejillaTaskAction
rejilla_task_ctx_get_action (RejillaTaskCtx *ctx);

RejillaBurnResult
rejilla_task_ctx_get_stored_tracks (RejillaTaskCtx *ctx,
				    GSList **tracks);

RejillaBurnResult
rejilla_task_ctx_get_current_track (RejillaTaskCtx *ctx,
				    RejillaTrack **track);

/**
 * Used to give job results and tell when a job has finished
 */

RejillaBurnResult
rejilla_task_ctx_add_track (RejillaTaskCtx *ctx,
			    RejillaTrack *track);

RejillaBurnResult
rejilla_task_ctx_next_track (RejillaTaskCtx *ctx);

RejillaBurnResult
rejilla_task_ctx_finished (RejillaTaskCtx *ctx);

RejillaBurnResult
rejilla_task_ctx_error (RejillaTaskCtx *ctx,
			RejillaBurnResult retval,
			GError *error);

/**
 * Used to start progress reporting and starts an internal timer to keep track
 * of remaining time among other things
 */

RejillaBurnResult
rejilla_task_ctx_start_progress (RejillaTaskCtx *ctx,
				 gboolean force);

void
rejilla_task_ctx_report_progress (RejillaTaskCtx *ctx);

void
rejilla_task_ctx_stop_progress (RejillaTaskCtx *ctx);

/**
 * task progress report for jobs
 */

RejillaBurnResult
rejilla_task_ctx_set_rate (RejillaTaskCtx *ctx,
			   gint64 rate);

RejillaBurnResult
rejilla_task_ctx_set_written_session (RejillaTaskCtx *ctx,
				      gint64 written);
RejillaBurnResult
rejilla_task_ctx_set_written_track (RejillaTaskCtx *ctx,
				    gint64 written);
RejillaBurnResult
rejilla_task_ctx_reset_progress (RejillaTaskCtx *ctx);
RejillaBurnResult
rejilla_task_ctx_set_progress (RejillaTaskCtx *ctx,
			       gdouble progress);
RejillaBurnResult
rejilla_task_ctx_set_current_action (RejillaTaskCtx *ctx,
				     RejillaBurnAction action,
				     const gchar *string,
				     gboolean force);
RejillaBurnResult
rejilla_task_ctx_set_use_average (RejillaTaskCtx *ctx,
				  gboolean use_average);
RejillaBurnResult
rejilla_task_ctx_set_output_size_for_current_track (RejillaTaskCtx *ctx,
						    goffset sectors,
						    goffset bytes);

/**
 * task progress for library
 */

RejillaBurnResult
rejilla_task_ctx_get_rate (RejillaTaskCtx *ctx,
			   guint64 *rate);
RejillaBurnResult
rejilla_task_ctx_get_remaining_time (RejillaTaskCtx *ctx,
				     long *remaining);
RejillaBurnResult
rejilla_task_ctx_get_session_output_size (RejillaTaskCtx *ctx,
					  goffset *blocks,
					  goffset *bytes);
RejillaBurnResult
rejilla_task_ctx_get_written (RejillaTaskCtx *ctx,
			      goffset *written);
RejillaBurnResult
rejilla_task_ctx_get_current_action_string (RejillaTaskCtx *ctx,
					    RejillaBurnAction action,
					    gchar **string);
RejillaBurnResult
rejilla_task_ctx_get_progress (RejillaTaskCtx *ctx, 
			       gdouble *progress);
RejillaBurnResult
rejilla_task_ctx_get_current_action (RejillaTaskCtx *ctx,
				     RejillaBurnAction *action);

G_END_DECLS

#endif /* _BURN_TASK_CTX_H_ */
