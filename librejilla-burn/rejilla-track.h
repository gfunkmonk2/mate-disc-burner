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

#ifndef _BURN_TRACK_H
#define _BURN_TRACK_H

#include <glib.h>
#include <glib-object.h>

#include <rejilla-drive.h>
#include <rejilla-medium.h>

#include <rejilla-enums.h>
#include <rejilla-error.h>
#include <rejilla-status.h>

#include <rejilla-track-type.h>

G_BEGIN_DECLS

#define REJILLA_TYPE_TRACK             (rejilla_track_get_type ())
#define REJILLA_TRACK(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_TRACK, RejillaTrack))
#define REJILLA_TRACK_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_TRACK, RejillaTrackClass))
#define REJILLA_IS_TRACK(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_TRACK))
#define REJILLA_IS_TRACK_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_TRACK))
#define REJILLA_TRACK_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_TRACK, RejillaTrackClass))

typedef struct _RejillaTrackClass RejillaTrackClass;
typedef struct _RejillaTrack RejillaTrack;

struct _RejillaTrackClass
{
	GObjectClass parent_class;

	/* Virtual functions */
	RejillaBurnResult	(* get_status)		(RejillaTrack *track,
							 RejillaStatus *status);

	RejillaBurnResult	(* get_size)		(RejillaTrack *track,
							 goffset *blocks,
							 goffset *block_size);

	RejillaBurnResult	(* get_type)		(RejillaTrack *track,
							 RejillaTrackType *type);

	/* Signals */
	void			(* changed)		(RejillaTrack *track);
};

struct _RejillaTrack
{
	GObject parent_instance;
};

GType rejilla_track_get_type (void) G_GNUC_CONST;

void
rejilla_track_changed (RejillaTrack *track);



RejillaBurnResult
rejilla_track_get_size (RejillaTrack *track,
			goffset *blocks,
			goffset *bytes);

RejillaBurnResult
rejilla_track_get_track_type (RejillaTrack *track,
			      RejillaTrackType *type);

RejillaBurnResult
rejilla_track_get_status (RejillaTrack *track,
			  RejillaStatus *status);


/** 
 * Checksums
 */

typedef enum {
	REJILLA_CHECKSUM_NONE			= 0,
	REJILLA_CHECKSUM_DETECT			= 1,		/* means the plugin handles detection of checksum type */
	REJILLA_CHECKSUM_MD5			= 1 << 1,
	REJILLA_CHECKSUM_MD5_FILE		= 1 << 2,
	REJILLA_CHECKSUM_SHA1			= 1 << 3,
	REJILLA_CHECKSUM_SHA1_FILE		= 1 << 4,
	REJILLA_CHECKSUM_SHA256			= 1 << 5,
	REJILLA_CHECKSUM_SHA256_FILE		= 1 << 6,
} RejillaChecksumType;

RejillaBurnResult
rejilla_track_set_checksum (RejillaTrack *track,
			    RejillaChecksumType type,
			    const gchar *checksum);

const gchar *
rejilla_track_get_checksum (RejillaTrack *track);

RejillaChecksumType
rejilla_track_get_checksum_type (RejillaTrack *track);

RejillaBurnResult
rejilla_track_tag_add (RejillaTrack *track,
		       const gchar *tag,
		       GValue *value);

RejillaBurnResult
rejilla_track_tag_lookup (RejillaTrack *track,
			  const gchar *tag,
			  GValue **value);

void
rejilla_track_tag_copy_missing (RejillaTrack *dest,
				RejillaTrack *src);

/**
 * Convenience functions for tags
 */

RejillaBurnResult
rejilla_track_tag_add_string (RejillaTrack *track,
			      const gchar *tag,
			      const gchar *string);

const gchar *
rejilla_track_tag_lookup_string (RejillaTrack *track,
				 const gchar *tag);

RejillaBurnResult
rejilla_track_tag_add_int (RejillaTrack *track,
			   const gchar *tag,
			   int value);

int
rejilla_track_tag_lookup_int (RejillaTrack *track,
			      const gchar *tag);

G_END_DECLS

#endif /* _BURN_TRACK_H */

 
