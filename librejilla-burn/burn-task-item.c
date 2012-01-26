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

#include <glib-object.h>

#include "burn-basics.h"
#include "burn-task-ctx.h"
#include "burn-task-item.h"

GType
rejilla_task_item_get_type (void)
{
	static GType type = 0;
	
	if (type == 0) {
		static const GTypeInfo info = {
			sizeof (RejillaTaskItemIFace),
			NULL,   /* base_init */
			NULL,   /* base_finalize */
			NULL,   /* class_init */
			NULL,   /* class_finalize */
			NULL,   /* class_data */
			0,
			0,      /* n_preallocs */
			NULL    /* instance_init */
		};
		type = g_type_register_static (G_TYPE_INTERFACE,
					       "RejillaTaskItem",
					       &info,
					       0);
	}
	return type;
}

RejillaBurnResult
rejilla_task_item_link (RejillaTaskItem *input, RejillaTaskItem *output)
{
	RejillaTaskItemIFace *klass;

	g_return_val_if_fail (REJILLA_IS_TASK_ITEM (input), REJILLA_BURN_ERR);
	g_return_val_if_fail (REJILLA_IS_TASK_ITEM (output), REJILLA_BURN_ERR);

	klass = REJILLA_TASK_ITEM_GET_CLASS (input);
	if (klass->link)
		return klass->link (input, output);

	klass = REJILLA_TASK_ITEM_GET_CLASS (output);
	if (klass->link)
		return klass->link (input, output);

	return REJILLA_BURN_NOT_SUPPORTED;
}

RejillaTaskItem *
rejilla_task_item_previous (RejillaTaskItem *item)
{
	RejillaTaskItemIFace *klass;

	g_return_val_if_fail (REJILLA_IS_TASK_ITEM (item), NULL);

	klass = REJILLA_TASK_ITEM_GET_CLASS (item);
	if (klass->previous)
		return klass->previous (item);

	return NULL;
}

RejillaTaskItem *
rejilla_task_item_next (RejillaTaskItem *item)
{
	RejillaTaskItemIFace *klass;

	g_return_val_if_fail (REJILLA_IS_TASK_ITEM (item), NULL);

	klass = REJILLA_TASK_ITEM_GET_CLASS (item);
	if (klass->next)
		return klass->next (item);

	return NULL;
}

gboolean
rejilla_task_item_is_active (RejillaTaskItem *item)
{
	RejillaTaskItemIFace *klass;

	g_return_val_if_fail (REJILLA_IS_TASK_ITEM (item), FALSE);

	klass = REJILLA_TASK_ITEM_GET_CLASS (item);
	if (klass->is_active)
		return klass->is_active (item);

	return FALSE;
}

RejillaBurnResult
rejilla_task_item_activate (RejillaTaskItem *item,
			    RejillaTaskCtx *ctx,
			    GError **error)
{
	RejillaTaskItemIFace *klass;

	g_return_val_if_fail (REJILLA_IS_TASK_ITEM (item), REJILLA_BURN_ERR);

	klass = REJILLA_TASK_ITEM_GET_CLASS (item);
	if (klass->activate)
		return klass->activate (item, ctx, error);

	return REJILLA_BURN_NOT_SUPPORTED;
}

RejillaBurnResult
rejilla_task_item_start (RejillaTaskItem *item,
			 GError **error)
{
	RejillaTaskItemIFace *klass;

	g_return_val_if_fail (REJILLA_IS_TASK_ITEM (item), REJILLA_BURN_ERR);

	klass = REJILLA_TASK_ITEM_GET_CLASS (item);
	if (klass->start)
		return klass->start (item, error);

	return REJILLA_BURN_NOT_SUPPORTED;
}

RejillaBurnResult
rejilla_task_item_clock_tick (RejillaTaskItem *item,
			      RejillaTaskCtx *ctx,
			      GError **error)
{
	RejillaTaskItemIFace *klass;

	g_return_val_if_fail (REJILLA_IS_TASK_ITEM (item), REJILLA_BURN_ERR);

	klass = REJILLA_TASK_ITEM_GET_CLASS (item);
	if (klass->clock_tick)
		return klass->clock_tick (item, ctx, error);

	return REJILLA_BURN_NOT_SUPPORTED;
}

RejillaBurnResult
rejilla_task_item_stop (RejillaTaskItem *item,
			RejillaTaskCtx *ctx,
			GError **error)
{
	RejillaTaskItemIFace *klass;

	g_return_val_if_fail (REJILLA_IS_TASK_ITEM (item), REJILLA_BURN_ERR);

	klass = REJILLA_TASK_ITEM_GET_CLASS (item);
	if (klass->stop)
		return klass->stop (item, ctx, error);

	return REJILLA_BURN_NOT_SUPPORTED;
}
