/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * rejilla
 * Copyright (C) Philippe Rouquier 2007 <bonfire-app@wanadoo.fr>
 * 
 *  Rejilla is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 * rejilla is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with rejilla.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifndef _REJILLA_VIDEO_TREE_MODEL_H_
#define _REJILLA_VIDEO_TREE_MODEL_H_

#include <glib-object.h>

#include "rejilla-track.h"
#include "rejilla-session-cfg.h"

G_BEGIN_DECLS

/* This DND target when moving nodes inside ourselves */
#define REJILLA_DND_TARGET_SELF_FILE_NODES	"GTK_TREE_MODEL_ROW"

struct _RejillaDNDVideoContext {
	GtkTreeModel *model;
	GList *references;
};
typedef struct _RejillaDNDVideoContext RejillaDNDVideoContext;

typedef enum {
	REJILLA_VIDEO_TREE_MODEL_NAME		= 0,		/* Markup */
	REJILLA_VIDEO_TREE_MODEL_ARTIST		= 1,
	REJILLA_VIDEO_TREE_MODEL_THUMBNAIL,
	REJILLA_VIDEO_TREE_MODEL_ICON_NAME,
	REJILLA_VIDEO_TREE_MODEL_SIZE,
	REJILLA_VIDEO_TREE_MODEL_EDITABLE,
	REJILLA_VIDEO_TREE_MODEL_SELECTABLE,
	REJILLA_VIDEO_TREE_MODEL_INDEX,
	REJILLA_VIDEO_TREE_MODEL_INDEX_NUM,
	REJILLA_VIDEO_TREE_MODEL_IS_GAP,
	REJILLA_VIDEO_TREE_MODEL_WEIGHT,
	REJILLA_VIDEO_TREE_MODEL_STYLE,
	REJILLA_VIDEO_TREE_MODEL_COL_NUM
} RejillaVideoProjectColumn;

#define REJILLA_TYPE_VIDEO_TREE_MODEL             (rejilla_video_tree_model_get_type ())
#define REJILLA_VIDEO_TREE_MODEL(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_VIDEO_TREE_MODEL, RejillaVideoTreeModel))
#define REJILLA_VIDEO_TREE_MODEL_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_VIDEO_TREE_MODEL, RejillaVideoTreeModelClass))
#define REJILLA_IS_VIDEO_TREE_MODEL(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_VIDEO_TREE_MODEL))
#define REJILLA_IS_VIDEO_TREE_MODEL_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_VIDEO_TREE_MODEL))
#define REJILLA_VIDEO_TREE_MODEL_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_VIDEO_TREE_MODEL, RejillaVideoTreeModelClass))

typedef struct _RejillaVideoTreeModelClass RejillaVideoTreeModelClass;
typedef struct _RejillaVideoTreeModel RejillaVideoTreeModel;

struct _RejillaVideoTreeModelClass
{
	GObjectClass parent_class;
};

struct _RejillaVideoTreeModel
{
	GObject parent_instance;
};

GType rejilla_video_tree_model_get_type (void) G_GNUC_CONST;

RejillaVideoTreeModel *
rejilla_video_tree_model_new (void);

void
rejilla_video_tree_model_set_session (RejillaVideoTreeModel *model,
				      RejillaSessionCfg *session);
RejillaSessionCfg *
rejilla_video_tree_model_get_session (RejillaVideoTreeModel *model);

RejillaTrack *
rejilla_video_tree_model_path_to_track (RejillaVideoTreeModel *self,
					GtkTreePath *path);

GtkTreePath *
rejilla_video_tree_model_track_to_path (RejillaVideoTreeModel *self,
				        RejillaTrack *track);

void
rejilla_video_tree_model_move_before (RejillaVideoTreeModel *self,
				      GtkTreeIter *iter,
				      GtkTreePath *dest_before);

G_END_DECLS

#endif /* _REJILLA_VIDEO_TREE_MODEL_H_ */
