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

#ifndef _REJILLA_TRACK_DATA_CFG_H_
#define _REJILLA_TRACK_DATA_CFG_H_

#include <glib-object.h>
#include <gtk/gtk.h>

#include <rejilla-track-data.h>

G_BEGIN_DECLS

/**
 * GtkTreeModel Part
 */

/* This DND target when moving nodes inside ourselves */
#define REJILLA_DND_TARGET_DATA_TRACK_REFERENCE_LIST	"GTK_TREE_MODEL_ROW"

typedef enum {
	REJILLA_DATA_TREE_MODEL_NAME		= 0,
	REJILLA_DATA_TREE_MODEL_URI,
	REJILLA_DATA_TREE_MODEL_MIME_DESC,
	REJILLA_DATA_TREE_MODEL_MIME_ICON,
	REJILLA_DATA_TREE_MODEL_SIZE,
	REJILLA_DATA_TREE_MODEL_SHOW_PERCENT,
	REJILLA_DATA_TREE_MODEL_PERCENT,
	REJILLA_DATA_TREE_MODEL_STYLE,
	REJILLA_DATA_TREE_MODEL_COLOR,
	REJILLA_DATA_TREE_MODEL_EDITABLE,
	REJILLA_DATA_TREE_MODEL_IS_FILE,
	REJILLA_DATA_TREE_MODEL_IS_LOADING,
	REJILLA_DATA_TREE_MODEL_IS_IMPORTED,
	REJILLA_DATA_TREE_MODEL_COL_NUM
} RejillaTrackDataCfgColumn;


#define REJILLA_TYPE_TRACK_DATA_CFG             (rejilla_track_data_cfg_get_type ())
#define REJILLA_TRACK_DATA_CFG(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_TRACK_DATA_CFG, RejillaTrackDataCfg))
#define REJILLA_TRACK_DATA_CFG_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_TRACK_DATA_CFG, RejillaTrackDataCfgClass))
#define REJILLA_IS_TRACK_DATA_CFG(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_TRACK_DATA_CFG))
#define REJILLA_IS_TRACK_DATA_CFG_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_TRACK_DATA_CFG))
#define REJILLA_TRACK_DATA_CFG_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_TRACK_DATA_CFG, RejillaTrackDataCfgClass))

typedef struct _RejillaTrackDataCfgClass RejillaTrackDataCfgClass;
typedef struct _RejillaTrackDataCfg RejillaTrackDataCfg;

struct _RejillaTrackDataCfgClass
{
	RejillaTrackDataClass parent_class;
};

struct _RejillaTrackDataCfg
{
	RejillaTrackData parent_instance;
};

GType rejilla_track_data_cfg_get_type (void) G_GNUC_CONST;

RejillaTrackDataCfg *
rejilla_track_data_cfg_new (void);

gboolean
rejilla_track_data_cfg_add (RejillaTrackDataCfg *track,
			    const gchar *uri,
			    GtkTreePath *parent);
GtkTreePath *
rejilla_track_data_cfg_add_empty_directory (RejillaTrackDataCfg *track,
					    const gchar *name,
					    GtkTreePath *parent);

gboolean
rejilla_track_data_cfg_remove (RejillaTrackDataCfg *track,
			       GtkTreePath *treepath);
gboolean
rejilla_track_data_cfg_rename (RejillaTrackDataCfg *track,
			       const gchar *newname,
			       GtkTreePath *treepath);

gboolean
rejilla_track_data_cfg_reset (RejillaTrackDataCfg *track);

gboolean
rejilla_track_data_cfg_load_medium (RejillaTrackDataCfg *track,
				    RejillaMedium *medium,
				    GError **error);
void
rejilla_track_data_cfg_unload_current_medium (RejillaTrackDataCfg *track);

RejillaMedium *
rejilla_track_data_cfg_get_current_medium (RejillaTrackDataCfg *track);

GSList *
rejilla_track_data_cfg_get_available_media (RejillaTrackDataCfg *track);

/**
 * For filtered URIs tree model
 */

void
rejilla_track_data_cfg_dont_filter_uri (RejillaTrackDataCfg *track,
					const gchar *uri);

GSList *
rejilla_track_data_cfg_get_restored_list (RejillaTrackDataCfg *track);

enum  {
	REJILLA_FILTERED_STOCK_ID_COL,
	REJILLA_FILTERED_URI_COL,
	REJILLA_FILTERED_STATUS_COL,
	REJILLA_FILTERED_FATAL_ERROR_COL,
	REJILLA_FILTERED_NB_COL,
};


void
rejilla_track_data_cfg_restore (RejillaTrackDataCfg *track,
				GtkTreePath *treepath);

GtkTreeModel *
rejilla_track_data_cfg_get_filtered_model (RejillaTrackDataCfg *track);


/**
 * Track Spanning
 */

RejillaBurnResult
rejilla_track_data_cfg_span (RejillaTrackDataCfg *track,
			     goffset sectors,
			     RejillaTrackData *new_track);
RejillaBurnResult
rejilla_track_data_cfg_span_again (RejillaTrackDataCfg *track);

RejillaBurnResult
rejilla_track_data_cfg_span_possible (RejillaTrackDataCfg *track,
				      goffset sectors);

goffset
rejilla_track_data_cfg_span_max_space (RejillaTrackDataCfg *track);

void
rejilla_track_data_cfg_span_stop (RejillaTrackDataCfg *track);

/**
 * Icon
 */

GIcon *
rejilla_track_data_cfg_get_icon (RejillaTrackDataCfg *track);

gchar *
rejilla_track_data_cfg_get_icon_path (RejillaTrackDataCfg *track);

gboolean
rejilla_track_data_cfg_set_icon (RejillaTrackDataCfg *track,
				 const gchar *icon_path,
				 GError **error);

G_END_DECLS

#endif /* _REJILLA_TRACK_DATA_CFG_H_ */
