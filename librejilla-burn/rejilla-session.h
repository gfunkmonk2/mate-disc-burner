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

#ifndef BURN_SESSION_H
#define BURN_SESSION_H

#include <glib.h>
#include <glib-object.h>

#include <rejilla-drive.h>

#include <rejilla-error.h>
#include <rejilla-status.h>
#include <rejilla-track.h>

G_BEGIN_DECLS

#define REJILLA_TYPE_BURN_SESSION         (rejilla_burn_session_get_type ())
#define REJILLA_BURN_SESSION(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), REJILLA_TYPE_BURN_SESSION, RejillaBurnSession))
#define REJILLA_BURN_SESSION_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), REJILLA_TYPE_BURN_SESSION, RejillaBurnSessionClass))
#define REJILLA_IS_BURN_SESSION(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), REJILLA_TYPE_BURN_SESSION))
#define REJILLA_IS_BURN_SESSION_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), REJILLA_TYPE_BURN_SESSION))
#define REJILLA_BURN_SESSION_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), REJILLA_TYPE_BURN_SESSION, RejillaBurnSessionClass))

typedef struct _RejillaBurnSession RejillaBurnSession;
typedef struct _RejillaBurnSessionClass RejillaBurnSessionClass;

struct _RejillaBurnSession {
	GObject parent;
};

struct _RejillaBurnSessionClass {
	GObjectClass parent_class;

	/** Virtual functions **/
	RejillaBurnResult	(*set_output_image)	(RejillaBurnSession *session,
							 RejillaImageFormat format,
							 const gchar *image,
							 const gchar *toc);
	RejillaBurnResult	(*get_output_path)	(RejillaBurnSession *session,
							 gchar **image,
							 gchar **toc);
	RejillaImageFormat	(*get_output_format)	(RejillaBurnSession *session);

	/** Signals **/
	void			(*tag_changed)		(RejillaBurnSession *session,
					                 const gchar *tag);
	void			(*track_added)		(RejillaBurnSession *session,
							 RejillaTrack *track);
	void			(*track_removed)	(RejillaBurnSession *session,
							 RejillaTrack *track,
							 guint former_position);
	void			(*track_changed)	(RejillaBurnSession *session,
							 RejillaTrack *track);
	void			(*output_changed)	(RejillaBurnSession *session,
							 RejillaMedium *former_medium);
};

GType rejilla_burn_session_get_type (void);

RejillaBurnSession *rejilla_burn_session_new (void);


/**
 * Used to manage tracks for input
 */

RejillaBurnResult
rejilla_burn_session_add_track (RejillaBurnSession *session,
				RejillaTrack *new_track,
				RejillaTrack *sibling);

RejillaBurnResult
rejilla_burn_session_move_track (RejillaBurnSession *session,
				 RejillaTrack *track,
				 RejillaTrack *sibling);

RejillaBurnResult
rejilla_burn_session_remove_track (RejillaBurnSession *session,
				   RejillaTrack *track);

GSList *
rejilla_burn_session_get_tracks (RejillaBurnSession *session);

/**
 * Get some information about the session
 */

RejillaBurnResult
rejilla_burn_session_get_status (RejillaBurnSession *session,
				 RejillaStatus *status);

RejillaBurnResult
rejilla_burn_session_get_size (RejillaBurnSession *session,
			       goffset *blocks,
			       goffset *bytes);

RejillaBurnResult
rejilla_burn_session_get_input_type (RejillaBurnSession *session,
				     RejillaTrackType *type);

/**
 * This is to set additional arbitrary information
 */

RejillaBurnResult
rejilla_burn_session_tag_lookup (RejillaBurnSession *session,
				 const gchar *tag,
				 GValue **value);

RejillaBurnResult
rejilla_burn_session_tag_add (RejillaBurnSession *session,
			      const gchar *tag,
			      GValue *value);

RejillaBurnResult
rejilla_burn_session_tag_remove (RejillaBurnSession *session,
				 const gchar *tag);

RejillaBurnResult
rejilla_burn_session_tag_add_int (RejillaBurnSession *self,
                                  const gchar *tag,
                                  gint value);
gint
rejilla_burn_session_tag_lookup_int (RejillaBurnSession *self,
                                     const gchar *tag);

/**
 * Destination 
 */
RejillaBurnResult
rejilla_burn_session_get_output_type (RejillaBurnSession *self,
                                      RejillaTrackType *output);

RejillaDrive *
rejilla_burn_session_get_burner (RejillaBurnSession *session);

void
rejilla_burn_session_set_burner (RejillaBurnSession *session,
				 RejillaDrive *drive);

RejillaBurnResult
rejilla_burn_session_set_image_output_full (RejillaBurnSession *session,
					    RejillaImageFormat format,
					    const gchar *image,
					    const gchar *toc);

RejillaBurnResult
rejilla_burn_session_get_output (RejillaBurnSession *session,
				 gchar **image,
				 gchar **toc);

RejillaBurnResult
rejilla_burn_session_set_image_output_format (RejillaBurnSession *self,
					    RejillaImageFormat format);

RejillaImageFormat
rejilla_burn_session_get_output_format (RejillaBurnSession *session);

const gchar *
rejilla_burn_session_get_label (RejillaBurnSession *session);

void
rejilla_burn_session_set_label (RejillaBurnSession *session,
				const gchar *label);

RejillaBurnResult
rejilla_burn_session_set_rate (RejillaBurnSession *session,
			       guint64 rate);

guint64
rejilla_burn_session_get_rate (RejillaBurnSession *session);

/**
 * Session flags
 */

void
rejilla_burn_session_set_flags (RejillaBurnSession *session,
			        RejillaBurnFlag flags);

void
rejilla_burn_session_add_flag (RejillaBurnSession *session,
			       RejillaBurnFlag flags);

void
rejilla_burn_session_remove_flag (RejillaBurnSession *session,
				  RejillaBurnFlag flags);

RejillaBurnFlag
rejilla_burn_session_get_flags (RejillaBurnSession *session);


/**
 * Used to deal with the temporary files (mostly used by plugins)
 */

RejillaBurnResult
rejilla_burn_session_set_tmpdir (RejillaBurnSession *session,
				 const gchar *path);
const gchar *
rejilla_burn_session_get_tmpdir (RejillaBurnSession *session);

/**
 * Test the supported or compulsory flags for a given session
 */

RejillaBurnResult
rejilla_burn_session_get_burn_flags (RejillaBurnSession *session,
				     RejillaBurnFlag *supported,
				     RejillaBurnFlag *compulsory);

RejillaBurnResult
rejilla_burn_session_get_blank_flags (RejillaBurnSession *session,
				      RejillaBurnFlag *supported,
				      RejillaBurnFlag *compulsory);

/**
 * Used to test the possibilities offered for a given session
 */

void
rejilla_burn_session_set_strict_support (RejillaBurnSession *session,
                                         gboolean strict_check);

gboolean
rejilla_burn_session_get_strict_support (RejillaBurnSession *session);

RejillaBurnResult
rejilla_burn_session_can_blank (RejillaBurnSession *session);

RejillaBurnResult
rejilla_burn_session_can_burn (RejillaBurnSession *session,
                               gboolean check_flags);

typedef RejillaBurnResult	(* RejillaForeachPluginErrorCb)	(RejillaPluginErrorType type,
		                                                 const gchar *detail,
		                                                 gpointer user_data);

RejillaBurnResult
rejilla_session_foreach_plugin_error (RejillaBurnSession *session,
                                      RejillaForeachPluginErrorCb callback,
                                      gpointer user_data);

RejillaBurnResult
rejilla_burn_session_input_supported (RejillaBurnSession *session,
				      RejillaTrackType *input,
                                      gboolean check_flags);

RejillaBurnResult
rejilla_burn_session_output_supported (RejillaBurnSession *session,
				       RejillaTrackType *output);

RejillaMedia
rejilla_burn_session_get_required_media_type (RejillaBurnSession *session);

guint
rejilla_burn_session_get_possible_output_formats (RejillaBurnSession *session,
						  RejillaImageFormat *formats);

RejillaImageFormat
rejilla_burn_session_get_default_output_format (RejillaBurnSession *session);


G_END_DECLS

#endif /* BURN_SESSION_H */
