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
 
#ifndef _REJILLA_MEDIUM_HANDLE_H
#define REJILLA_MEDIUM_HANDLE_H

#include <glib.h>

#include "burn-basics.h"
#include "burn-volume-source.h"

G_BEGIN_DECLS

typedef struct _RejillaVolFileHandle RejillaVolFileHandle;


RejillaVolFileHandle *
rejilla_volume_file_open (RejillaVolSrc *src,
			  RejillaVolFile *file);

void
rejilla_volume_file_close (RejillaVolFileHandle *handle);

gboolean
rejilla_volume_file_rewind (RejillaVolFileHandle *handle);

gint
rejilla_volume_file_read (RejillaVolFileHandle *handle,
			  gchar *buffer,
			  guint len);

RejillaBurnResult
rejilla_volume_file_read_line (RejillaVolFileHandle *handle,
			       gchar *buffer,
			       guint len);

RejillaVolFileHandle *
rejilla_volume_file_open_direct (RejillaVolSrc *src,
				 RejillaVolFile *file);

gint64
rejilla_volume_file_read_direct (RejillaVolFileHandle *handle,
				 guchar *buffer,
				 guint blocks);

G_END_DECLS

#endif /* REJILLA_MEDIUM_HANDLE_H */

 
