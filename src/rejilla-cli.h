/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Rejilla
 * Copyright (C) Philippe Rouquier 2005-2010 <bonfire-app@wanadoo.fr>
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

#ifndef _REJILLA_CLI_H_
#define _REJILLA_CLI_H_

#include <glib.h>

#include "rejilla-app.h"
#include "rejilla-drive.h"

G_BEGIN_DECLS

struct _RejillaCLI {
	gchar *burn_project_uri;
	gchar *project_uri;
	gchar *cover_project;
	gchar *playlist_uri;
	gchar *copy_project_path;
	gchar *image_project_uri;

	gint audio_project;
	gint data_project;
	gint video_project;
	gint empty_project;
	gint open_ncb;

	gint disc_blank;
	gint disc_check;

	gint parent_window;
	gint burn_immediately;

	gboolean image_project;
	gboolean copy_project;
	gboolean not_unique;

	RejillaDrive *burner;

	gchar **files;
};
typedef struct _RejillaCLI RejillaCLI;

extern RejillaCLI cmd_line_options;
extern const GOptionEntry prog_options [];

void
rejilla_cli_apply_options (RejillaApp *app);

G_END_DECLS

#endif
