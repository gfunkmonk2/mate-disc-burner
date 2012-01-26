/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Librejilla-media
 * Copyright (C) Philippe Rouquier 2005-2009 <bonfire-app@wanadoo.fr>
 *
 * Librejilla-media is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * The Librejilla-media authors hereby grant permission for non-GPL compatible
 * GStreamer plugins to be used and distributed together with GStreamer
 * and Librejilla-media. This permission is above and beyond the permissions granted
 * by the GPL license by which Librejilla-media is covered. If you modify this code
 * you may extend this exception to your version of the code, but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version.
 * 
 * Librejilla-media is distributed in the hope that it will be useful,
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdio.h>

#include <glib.h>

#include "burn-volume.h"
#include "burn-volume-source.h"

#ifndef _BURN_ISO9660_H
#define _BURN_ISO9660_H

G_BEGIN_DECLS

#define ISO9660_BLOCK_SIZE 2048

typedef enum {
	REJILLA_ISO_FLAG_NONE	= 0,
	REJILLA_ISO_FLAG_RR	= 1
} RejillaIsoFlag;

gboolean
rejilla_iso9660_is_primary_descriptor (const gchar *buffer,
				       GError **error);

gboolean
rejilla_iso9660_get_size (const gchar *block,
			  gint64 *nb_blocks,
			  GError **error);

gboolean
rejilla_iso9660_get_label (const gchar *block,
			   gchar **label,
			   GError **error);

RejillaVolFile *
rejilla_iso9660_get_contents (RejillaVolSrc *src,
			      const gchar *block,
			      gint64 *nb_blocks,
			      GError **error);

/**
 * Address to -1 for root
 */
GList *
rejilla_iso9660_get_directory_contents (RejillaVolSrc *vol,
					const gchar *vol_desc,
					gint address,
					GError **error);

RejillaVolFile *
rejilla_iso9660_get_file (RejillaVolSrc *src,
			  const gchar *path,
			  const gchar *block,
			  GError **error);

G_END_DECLS

#endif /* _BURN_ISO9660_H */

 
