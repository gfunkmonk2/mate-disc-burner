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

#ifndef _BURN_SESSION_HELPER_H_
#define _BURN_SESSION_HELPER_H_

#include <glib.h>

#include "rejilla-media.h"
#include "rejilla-drive.h"

#include "rejilla-session.h"

G_BEGIN_DECLS


/**
 * Some convenience functions used internally
 */

RejillaBurnResult
rejilla_caps_session_get_image_flags (RejillaTrackType *input,
                                     RejillaTrackType *output,
                                     RejillaBurnFlag *supported,
                                     RejillaBurnFlag *compulsory);

goffset
rejilla_burn_session_get_available_medium_space (RejillaBurnSession *session);

RejillaMedia
rejilla_burn_session_get_dest_media (RejillaBurnSession *session);

RejillaDrive *
rejilla_burn_session_get_src_drive (RejillaBurnSession *session);

RejillaMedium *
rejilla_burn_session_get_src_medium (RejillaBurnSession *session);

gboolean
rejilla_burn_session_is_dest_file (RejillaBurnSession *session);

gboolean
rejilla_burn_session_same_src_dest_drive (RejillaBurnSession *session);

#define REJILLA_BURN_SESSION_EJECT(session)					\
(rejilla_burn_session_get_flags ((session)) & REJILLA_BURN_FLAG_EJECT)

#define REJILLA_BURN_SESSION_CHECK_SIZE(session)				\
(rejilla_burn_session_get_flags ((session)) & REJILLA_BURN_FLAG_CHECK_SIZE)

#define REJILLA_BURN_SESSION_NO_TMP_FILE(session)				\
(rejilla_burn_session_get_flags ((session)) & REJILLA_BURN_FLAG_NO_TMP_FILES)

#define REJILLA_BURN_SESSION_OVERBURN(session)					\
(rejilla_burn_session_get_flags ((session)) & REJILLA_BURN_FLAG_OVERBURN)

#define REJILLA_BURN_SESSION_APPEND(session)					\
(rejilla_burn_session_get_flags ((session)) & (REJILLA_BURN_FLAG_APPEND|REJILLA_BURN_FLAG_MERGE))

RejillaBurnResult
rejilla_burn_session_get_tmp_image (RejillaBurnSession *session,
				    RejillaImageFormat format,
				    gchar **image,
				    gchar **toc,
				    GError **error);

RejillaBurnResult
rejilla_burn_session_get_tmp_file (RejillaBurnSession *session,
				   const gchar *suffix,
				   gchar **path,
				   GError **error);

RejillaBurnResult
rejilla_burn_session_get_tmp_dir (RejillaBurnSession *session,
				  gchar **path,
				  GError **error);

RejillaBurnResult
rejilla_burn_session_get_tmp_image_type_same_src_dest (RejillaBurnSession *session,
                                                       RejillaTrackType *image_type);

/**
 * This is to log a session
 * (used internally)
 */

const gchar *
rejilla_burn_session_get_log_path (RejillaBurnSession *session);

gboolean
rejilla_burn_session_start (RejillaBurnSession *session);

void
rejilla_burn_session_stop (RejillaBurnSession *session);

void
rejilla_burn_session_logv (RejillaBurnSession *session,
			   const gchar *format,
			   va_list arg_list);
void
rejilla_burn_session_log (RejillaBurnSession *session,
			  const gchar *format,
			  ...);

/**
 * Allow to save a whole session settings/source and restore it later.
 * (used internally)
 */

void
rejilla_burn_session_push_settings (RejillaBurnSession *session);
void
rejilla_burn_session_pop_settings (RejillaBurnSession *session);

void
rejilla_burn_session_push_tracks (RejillaBurnSession *session);
RejillaBurnResult
rejilla_burn_session_pop_tracks (RejillaBurnSession *session);


G_END_DECLS

#endif
