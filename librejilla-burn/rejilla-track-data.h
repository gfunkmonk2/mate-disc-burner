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

#ifndef _REJILLA_TRACK_DATA_H_
#define _REJILLA_TRACK_DATA_H_

#include <glib-object.h>

#include <rejilla-track.h>

G_BEGIN_DECLS

/**
 * RejillaGraftPt:
 * @uri: a URI
 * @path: a file path
 *
 * A pair of strings describing:
 * @uri the actual current location of the file
 * @path the path of the file on the future ISO9660/UDF/... filesystem
 **/
typedef struct _RejillaGraftPt {
	gchar *uri;
	gchar *path;
} RejillaGraftPt;

void
rejilla_graft_point_free (RejillaGraftPt *graft);

RejillaGraftPt *
rejilla_graft_point_copy (RejillaGraftPt *graft);


#define REJILLA_TYPE_TRACK_DATA             (rejilla_track_data_get_type ())
#define REJILLA_TRACK_DATA(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_TRACK_DATA, RejillaTrackData))
#define REJILLA_TRACK_DATA_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_TRACK_DATA, RejillaTrackDataClass))
#define REJILLA_IS_TRACK_DATA(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_TRACK_DATA))
#define REJILLA_IS_TRACK_DATA_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_TRACK_DATA))
#define REJILLA_TRACK_DATA_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_TRACK_DATA, RejillaTrackDataClass))

typedef struct _RejillaTrackDataClass RejillaTrackDataClass;
typedef struct _RejillaTrackData RejillaTrackData;

struct _RejillaTrackDataClass
{
	RejillaTrackClass parent_class;

	/* virtual functions */

	/**
	 * set_source:
	 * @track: a #RejillaTrackData.
	 * @grafts: (element-type RejillaBurn.GraftPt) (transfer full): a #GSList of #RejillaGraftPt.
	 * @unreadable: (element-type utf8) (transfer full) (allow-none): a #GSList of URIs (as strings) or %NULL.
	 *
	 * Return value: a #RejillaBurnResult
	 **/
	RejillaBurnResult	(*set_source)		(RejillaTrackData *track,
							 GSList *grafts,
							 GSList *unreadable);
	RejillaBurnResult       (*add_fs)		(RejillaTrackData *track,
							 RejillaImageFS fstype);

	RejillaBurnResult       (*rm_fs)		(RejillaTrackData *track,
							 RejillaImageFS fstype);

	RejillaImageFS		(*get_fs)		(RejillaTrackData *track);
	GSList*			(*get_grafts)		(RejillaTrackData *track);
	GSList*			(*get_excluded)		(RejillaTrackData *track);
	guint64			(*get_file_num)		(RejillaTrackData *track);
};

struct _RejillaTrackData
{
	RejillaTrack parent_instance;
};

GType rejilla_track_data_get_type (void) G_GNUC_CONST;

RejillaTrackData *
rejilla_track_data_new (void);

RejillaBurnResult
rejilla_track_data_set_source (RejillaTrackData *track,
			       GSList *grafts,
			       GSList *unreadable);

RejillaBurnResult
rejilla_track_data_add_fs (RejillaTrackData *track,
			   RejillaImageFS fstype);

RejillaBurnResult
rejilla_track_data_rm_fs (RejillaTrackData *track,
			  RejillaImageFS fstype);

RejillaBurnResult
rejilla_track_data_set_data_blocks (RejillaTrackData *track,
				    goffset blocks);

RejillaBurnResult
rejilla_track_data_set_file_num (RejillaTrackData *track,
				 guint64 number);

GSList *
rejilla_track_data_get_grafts (RejillaTrackData *track);

GSList *
rejilla_track_data_get_excluded_list (RejillaTrackData *track);

G_GNUC_DEPRECATED GSList *
rejilla_track_data_get_excluded (RejillaTrackData *track,
				 gboolean copy);

G_GNUC_DEPRECATED RejillaBurnResult
rejilla_track_data_get_paths (RejillaTrackData *track,
			      gboolean use_joliet,
			      const gchar *grafts_path,
			      const gchar *excluded_path,
			      const gchar *emptydir,
			      const gchar *videodir,
			      GError **error);

RejillaBurnResult
rejilla_track_data_write_to_paths (RejillaTrackData *track,
                                   const gchar *grafts_path,
                                   const gchar *excluded_path,
                                   const gchar *emptydir,
                                   const gchar *videodir,
                                   GError **error);

RejillaBurnResult
rejilla_track_data_get_file_num (RejillaTrackData *track,
				 guint64 *file_num);

RejillaImageFS
rejilla_track_data_get_fs (RejillaTrackData *track);

G_END_DECLS

#endif /* _REJILLA_TRACK_DATA_H_ */
