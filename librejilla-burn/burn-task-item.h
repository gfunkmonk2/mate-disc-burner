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

#ifndef _REJILLA_TASK_ITEM_H
#define _REJILLA_TASK_ITEM_H

#include <glib-object.h>

#include "burn-basics.h"
#include "burn-task-ctx.h"

G_BEGIN_DECLS

#define REJILLA_TYPE_TASK_ITEM			(rejilla_task_item_get_type ())
#define REJILLA_TASK_ITEM(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_TASK_ITEM, RejillaTaskItem))
#define REJILLA_TASK_ITEM_CLASS(vtable)		(G_TYPE_CHECK_CLASS_CAST ((vtable), REJILLA_TYPE_TASK_ITEM, RejillaTaskItemIFace))
#define REJILLA_IS_TASK_ITEM(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_TASK_ITEM))
#define REJILLA_IS_TASK_ITEM_CLASS(vtable)	(G_TYPE_CHECK_CLASS_TYPE ((vtable), REJILLA_TYPE_TASK_ITEM))
#define REJILLA_TASK_ITEM_GET_CLASS(inst)	(G_TYPE_INSTANCE_GET_INTERFACE ((inst), REJILLA_TYPE_TASK_ITEM, RejillaTaskItemIFace))

typedef struct _RejillaTaskItem RejillaTaskItem;
typedef struct _RejillaTaskItemIFace RejillaTaskItemIFace;

struct _RejillaTaskItemIFace {
	GTypeInterface parent;

	RejillaBurnResult	(*link)		(RejillaTaskItem *input,
						 RejillaTaskItem *output);
	RejillaTaskItem *	(*previous)	(RejillaTaskItem *item);
	RejillaTaskItem *	(*next)		(RejillaTaskItem *item);

	gboolean		(*is_active)	(RejillaTaskItem *item);
	RejillaBurnResult	(*activate)	(RejillaTaskItem *item,
						 RejillaTaskCtx *ctx,
						 GError **error);
	RejillaBurnResult	(*start)	(RejillaTaskItem *item,
						 GError **error);
	RejillaBurnResult	(*clock_tick)	(RejillaTaskItem *item,
						 RejillaTaskCtx *ctx,
						 GError **error);
	RejillaBurnResult	(*stop)		(RejillaTaskItem *item,
						 RejillaTaskCtx *ctx,
						 GError **error);
};

GType
rejilla_task_item_get_type (void);

RejillaBurnResult
rejilla_task_item_link (RejillaTaskItem *input,
			RejillaTaskItem *output);

RejillaTaskItem *
rejilla_task_item_previous (RejillaTaskItem *item);

RejillaTaskItem *
rejilla_task_item_next (RejillaTaskItem *item);

gboolean
rejilla_task_item_is_active (RejillaTaskItem *item);

RejillaBurnResult
rejilla_task_item_activate (RejillaTaskItem *item,
			    RejillaTaskCtx *ctx,
			    GError **error);

RejillaBurnResult
rejilla_task_item_start (RejillaTaskItem *item,
			 GError **error);

RejillaBurnResult
rejilla_task_item_clock_tick (RejillaTaskItem *item,
			      RejillaTaskCtx *ctx,
			      GError **error);

RejillaBurnResult
rejilla_task_item_stop (RejillaTaskItem *item,
			RejillaTaskCtx *ctx,
			GError **error);

G_END_DECLS

#endif
