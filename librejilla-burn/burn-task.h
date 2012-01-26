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

#ifndef BURN_TASK_H
#define BURN_TASK_H

#include <glib.h>
#include <glib-object.h>

#include "burn-basics.h"
#include "burn-job.h"
#include "burn-task-ctx.h"
#include "burn-task-item.h"

G_BEGIN_DECLS

#define REJILLA_TYPE_TASK         (rejilla_task_get_type ())
#define REJILLA_TASK(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), REJILLA_TYPE_TASK, RejillaTask))
#define REJILLA_TASK_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), REJILLA_TYPE_TASK, RejillaTaskClass))
#define REJILLA_IS_TASK(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), REJILLA_TYPE_TASK))
#define REJILLA_IS_TASK_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), REJILLA_TYPE_TASK))
#define REJILLA_TASK_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), REJILLA_TYPE_TASK, RejillaTaskClass))

typedef struct _RejillaTask RejillaTask;
typedef struct _RejillaTaskClass RejillaTaskClass;

struct _RejillaTask {
	RejillaTaskCtx parent;
};

struct _RejillaTaskClass {
	RejillaTaskCtxClass parent_class;
};

GType rejilla_task_get_type (void);

RejillaTask *rejilla_task_new (void);

void
rejilla_task_add_item (RejillaTask *task, RejillaTaskItem *item);

void
rejilla_task_reset (RejillaTask *task);

RejillaBurnResult
rejilla_task_run (RejillaTask *task,
		  GError **error);

RejillaBurnResult
rejilla_task_check (RejillaTask *task,
		    GError **error);

RejillaBurnResult
rejilla_task_cancel (RejillaTask *task,
		     gboolean protect);

gboolean
rejilla_task_is_running (RejillaTask *task);

RejillaBurnResult
rejilla_task_get_output_type (RejillaTask *task, RejillaTrackType *output);

G_END_DECLS

#endif /* BURN_TASK_H */
