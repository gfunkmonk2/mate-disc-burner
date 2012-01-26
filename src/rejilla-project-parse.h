/***************************************************************************
 *            disc.h
 *
 *  dim nov 27 14:58:13 2005
 *  Copyright  2005  Rouquier Philippe
 *  rejilla-app@wanadoo.fr
 ***************************************************************************/

/*
 *  Rejilla is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Rejilla is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifndef _REJILLA_PROJECT_PARSE_H_
#define _REJILLA_PROJECT_PARSE_H_

#include <glib.h>

#include "rejilla-track.h"
#include "rejilla-session.h"

G_BEGIN_DECLS

typedef enum {
	REJILLA_PROJECT_SAVE_XML			= 0,
	REJILLA_PROJECT_SAVE_PLAIN			= 1,
	REJILLA_PROJECT_SAVE_PLAYLIST_PLS		= 2,
	REJILLA_PROJECT_SAVE_PLAYLIST_M3U		= 3,
	REJILLA_PROJECT_SAVE_PLAYLIST_XSPF		= 4,
	REJILLA_PROJECT_SAVE_PLAYLIST_IRIVER_PLA	= 5
} RejillaProjectSave;

typedef enum {
	REJILLA_PROJECT_TYPE_INVALID,
	REJILLA_PROJECT_TYPE_COPY,
	REJILLA_PROJECT_TYPE_ISO,
	REJILLA_PROJECT_TYPE_AUDIO,
	REJILLA_PROJECT_TYPE_DATA,
	REJILLA_PROJECT_TYPE_VIDEO
} RejillaProjectType;

gboolean
rejilla_project_open_project_xml (const gchar *uri,
				  RejillaBurnSession *session,
				  gboolean warn_user);

gboolean
rejilla_project_open_audio_playlist_project (const gchar *uri,
					     RejillaBurnSession *session,
					     gboolean warn_user);

gboolean 
rejilla_project_save_project_xml (RejillaBurnSession *session,
				  const gchar *uri);

gboolean
rejilla_project_save_audio_project_plain_text (RejillaBurnSession *session,
					       const gchar *uri);

gboolean
rejilla_project_save_audio_project_playlist (RejillaBurnSession *session,
					     const gchar *uri,
					     RejillaProjectSave type);

G_END_DECLS

#endif
