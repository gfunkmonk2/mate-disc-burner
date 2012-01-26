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

#ifndef _REJILLA_IO_H_
#define _REJILLA_IO_H_

#include <glib-object.h>
#include <gtk/gtk.h>

#include "rejilla-async-task-manager.h"

G_BEGIN_DECLS

typedef enum {
	REJILLA_IO_INFO_NONE			= 0,
	REJILLA_IO_INFO_MIME			= 1,
	REJILLA_IO_INFO_ICON			= 1,
	REJILLA_IO_INFO_PERM			= 1 << 1,
	REJILLA_IO_INFO_METADATA		= 1 << 2,
	REJILLA_IO_INFO_METADATA_THUMBNAIL	= 1 << 3,
	REJILLA_IO_INFO_RECURSIVE		= 1 << 4,
	REJILLA_IO_INFO_CHECK_PARENT_SYMLINK	= 1 << 5,
	REJILLA_IO_INFO_METADATA_MISSING_CODEC	= 1 << 6,

	REJILLA_IO_INFO_FOLLOW_SYMLINK		= 1 << 7,

	REJILLA_IO_INFO_URGENT			= 1 << 9,
	REJILLA_IO_INFO_IDLE			= 1 << 10
} RejillaIOFlags;


typedef enum {
	REJILLA_IO_PHASE_START		= 0,
	REJILLA_IO_PHASE_DOWNLOAD,
	REJILLA_IO_PHASE_END
} RejillaIOPhase;

#define REJILLA_IO_XFER_DESTINATION	"xfer::destination"

#define REJILLA_IO_PLAYLIST_TITLE	"playlist::title"
#define REJILLA_IO_IS_PLAYLIST		"playlist::is_playlist"
#define REJILLA_IO_PLAYLIST_ENTRIES_NUM	"playlist::entries_num"

#define REJILLA_IO_COUNT_NUM		"count::num"
#define REJILLA_IO_COUNT_SIZE		"count::size"
#define REJILLA_IO_COUNT_INVALID	"count::invalid"

#define REJILLA_IO_THUMBNAIL		"metadata::thumbnail"

#define REJILLA_IO_LEN			"metadata::length"
#define REJILLA_IO_ISRC			"metadata::isrc"
#define REJILLA_IO_TITLE		"metadata::title"
#define REJILLA_IO_ARTIST		"metadata::artist"
#define REJILLA_IO_ALBUM		"metadata::album"
#define REJILLA_IO_GENRE		"metadata::genre"
#define REJILLA_IO_COMPOSER		"metadata::composer"
#define REJILLA_IO_HAS_AUDIO		"metadata::has_audio"
#define REJILLA_IO_HAS_VIDEO		"metadata::has_video"
#define REJILLA_IO_IS_SEEKABLE		"metadata::is_seekable"

#define REJILLA_IO_HAS_DTS			"metadata::audio::wav::has_dts"

#define REJILLA_IO_CHANNELS		"metadata::audio::channels"
#define REJILLA_IO_RATE				"metadata::audio::rate"

#define REJILLA_IO_DIR_CONTENTS_ADDR	"image::directory::address"

typedef struct _RejillaIOJobProgress RejillaIOJobProgress;

typedef void		(*RejillaIOResultCallback)	(GObject *object,
							 GError *error,
							 const gchar *uri,
							 GFileInfo *info,
							 gpointer callback_data);

typedef void		(*RejillaIOProgressCallback)	(GObject *object,
							 RejillaIOJobProgress *info,
							 gpointer callback_data);

typedef void		(*RejillaIODestroyCallback)	(GObject *object,
							 gboolean cancel,
							 gpointer callback_data);

typedef gboolean	(*RejillaIOCompareCallback)	(gpointer data,
							 gpointer user_data);


struct _RejillaIOJobCallbacks {
	RejillaIOResultCallback callback;
	RejillaIODestroyCallback destroy;
	RejillaIOProgressCallback progress;

	guint ref;

	/* Whether we are returning something for this base */
	guint in_use:1;
};
typedef struct _RejillaIOJobCallbacks RejillaIOJobCallbacks;

struct _RejillaIOJobBase {
	GObject *object;
	RejillaIOJobCallbacks *methods;
};
typedef struct _RejillaIOJobBase RejillaIOJobBase;

struct _RejillaIOResultCallbackData {
	gpointer callback_data;
	gint ref;
};
typedef struct _RejillaIOResultCallbackData RejillaIOResultCallbackData;

struct _RejillaIOJob {
	gchar *uri;
	RejillaIOFlags options;

	const RejillaIOJobBase *base;
	RejillaIOResultCallbackData *callback_data;
};
typedef struct _RejillaIOJob RejillaIOJob;

#define REJILLA_IO_JOB(data)	((RejillaIOJob *) (data))

void
rejilla_io_job_free (gboolean cancelled,
		     RejillaIOJob *job);

void
rejilla_io_set_job (RejillaIOJob *self,
		    const RejillaIOJobBase *base,
		    const gchar *uri,
		    RejillaIOFlags options,
		    RejillaIOResultCallbackData *callback_data);

void
rejilla_io_push_job (RejillaIOJob *job,
		     const RejillaAsyncTaskType *type);

void
rejilla_io_return_result (const RejillaIOJobBase *base,
			  const gchar *uri,
			  GFileInfo *info,
			  GError *error,
			  RejillaIOResultCallbackData *callback_data);


typedef GtkWindow *	(* RejillaIOGetParentWinCb)	(gpointer user_data);

void
rejilla_io_set_parent_window_callback (RejillaIOGetParentWinCb callback,
                                       gpointer user_data);

void
rejilla_io_shutdown (void);

/* NOTE: The split in methods and objects was
 * done to prevent jobs sharing the same methods
 * to return their results concurently. In other
 * words only one job among those sharing the
 * same methods can return its results. */
 
RejillaIOJobBase *
rejilla_io_register (GObject *object,
		     RejillaIOResultCallback callback,
		     RejillaIODestroyCallback destroy,
		     RejillaIOProgressCallback progress);

RejillaIOJobBase *
rejilla_io_register_with_methods (GObject *object,
                                  RejillaIOJobCallbacks *methods);

RejillaIOJobCallbacks *
rejilla_io_register_job_methods (RejillaIOResultCallback callback,
                                 RejillaIODestroyCallback destroy,
                                 RejillaIOProgressCallback progress);

void
rejilla_io_job_base_free (RejillaIOJobBase *base);

void
rejilla_io_cancel_by_base (RejillaIOJobBase *base);

void
rejilla_io_find_urgent (const RejillaIOJobBase *base,
			RejillaIOCompareCallback callback,
			gpointer callback_data);			

void
rejilla_io_load_directory (const gchar *uri,
			   const RejillaIOJobBase *base,
			   RejillaIOFlags options,
			   gpointer callback_data);
void
rejilla_io_get_file_info (const gchar *uri,
			  const RejillaIOJobBase *base,
			  RejillaIOFlags options,
			  gpointer callback_data);
void
rejilla_io_get_file_count (GSList *uris,
			   const RejillaIOJobBase *base,
			   RejillaIOFlags options,
			   gpointer callback_data);
void
rejilla_io_parse_playlist (const gchar *uri,
			   const RejillaIOJobBase *base,
			   RejillaIOFlags options,
			   gpointer callback_data);

guint64
rejilla_io_job_progress_get_read (RejillaIOJobProgress *progress);

guint64
rejilla_io_job_progress_get_total (RejillaIOJobProgress *progress);

RejillaIOPhase
rejilla_io_job_progress_get_phase (RejillaIOJobProgress *progress);

guint
rejilla_io_job_progress_get_file_processed (RejillaIOJobProgress *progress);

G_END_DECLS

#endif /* _REJILLA_IO_H_ */
