/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Librejilla-misc
 * Copyright (C) Philippe Rouquier 2005-2009 <bonfire-app@wanadoo.fr>
 *
 * Librejilla-misc is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * The Librejilla-misc authors hereby grant permission for non-GPL compatible
 * GStreamer plugins to be used and distributed together with GStreamer
 * and Librejilla-misc. This permission is above and beyond the permissions granted
 * by the GPL license by which Librejilla-burn is covered. If you modify this code
 * you may extend this exception to your version of the code, but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version.
 * 
 * Librejilla-misc is distributed in the hope that it will be useful,
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

#ifndef ASYNC_TASK_MANAGER_H
#define ASYNC_TASK_MANAGER_H

#include <glib.h>
#include <glib-object.h>

#include <gio/gio.h>

G_BEGIN_DECLS

#define REJILLA_TYPE_ASYNC_TASK_MANAGER         (rejilla_async_task_manager_get_type ())
#define REJILLA_ASYNC_TASK_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), REJILLA_TYPE_ASYNC_TASK_MANAGER, RejillaAsyncTaskManager))
#define REJILLA_ASYNC_TASK_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), REJILLA_TYPE_ASYNC_TASK_MANAGER, RejillaAsyncTaskManagerClass))
#define REJILLA_IS_ASYNC_TASK_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), REJILLA_TYPE_ASYNC_TASK_MANAGER))
#define REJILLA_IS_ASYNC_TASK_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), REJILLA_TYPE_ASYNC_TASK_MANAGER))
#define REJILLA_ASYNC_TASK_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), REJILLA_TYPE_ASYNC_TASK_MANAGER, RejillaAsyncTaskManagerClass))

typedef struct RejillaAsyncTaskManagerPrivate RejillaAsyncTaskManagerPrivate;
typedef struct _RejillaAsyncTaskManagerClass RejillaAsyncTaskManagerClass;
typedef struct _RejillaAsyncTaskManager RejillaAsyncTaskManager;

struct _RejillaAsyncTaskManager {
	GObject parent;
	RejillaAsyncTaskManagerPrivate *priv;
};

struct _RejillaAsyncTaskManagerClass {
	GObjectClass parent_class;
};

GType rejilla_async_task_manager_get_type (void);

typedef enum {
	REJILLA_ASYNC_TASK_FINISHED		= 0,
	REJILLA_ASYNC_TASK_RESCHEDULE		= 1
} RejillaAsyncTaskResult;

typedef RejillaAsyncTaskResult	(*RejillaAsyncThread)		(RejillaAsyncTaskManager *manager,
								 GCancellable *cancel,
								 gpointer user_data);
typedef void			(*RejillaAsyncDestroy)		(RejillaAsyncTaskManager *manager,
								 gboolean cancelled,
								 gpointer user_data);
typedef gboolean		(*RejillaAsyncFindTask)		(RejillaAsyncTaskManager *manager,
								 gpointer task,
								 gpointer user_data);

struct _RejillaAsyncTaskType {
	RejillaAsyncThread thread;
	RejillaAsyncDestroy destroy;
};
typedef struct _RejillaAsyncTaskType RejillaAsyncTaskType;

typedef enum {
	/* used internally when reschedule */
	REJILLA_ASYNC_RESCHEDULE	= 1,

	REJILLA_ASYNC_IDLE		= 1 << 1,
	REJILLA_ASYNC_NORMAL		= 1 << 2,
	REJILLA_ASYNC_URGENT		= 1 << 3
} RejillaAsyncPriority;

gboolean
rejilla_async_task_manager_queue (RejillaAsyncTaskManager *manager,
				  RejillaAsyncPriority priority,
				  const RejillaAsyncTaskType *type,
				  gpointer data);

gboolean
rejilla_async_task_manager_foreach_active (RejillaAsyncTaskManager *manager,
					   RejillaAsyncFindTask func,
					   gpointer user_data);
gboolean
rejilla_async_task_manager_foreach_active_remove (RejillaAsyncTaskManager *manager,
						  RejillaAsyncFindTask func,
						  gpointer user_data);
gboolean
rejilla_async_task_manager_foreach_unprocessed_remove (RejillaAsyncTaskManager *self,
						       RejillaAsyncFindTask func,
						       gpointer user_data);

gboolean
rejilla_async_task_manager_find_urgent_task (RejillaAsyncTaskManager *manager,
					     RejillaAsyncFindTask func,
					     gpointer user_data);

G_END_DECLS

#endif /* ASYNC_JOB_MANAGER_H */
