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

#ifndef REJILLA_TOOL_DIALOG_PRIVATE_H
#define REJILLA_TOOL_DIALOG_PRIVATE_H

#include "rejilla-tool-dialog.h"


G_BEGIN_DECLS

void
rejilla_tool_dialog_pack_options (RejillaToolDialog *dialog, ...);

void
rejilla_tool_dialog_set_button (RejillaToolDialog *dialog,
				const gchar *text,
				const gchar *image,
				const gchar *theme);
void
rejilla_tool_dialog_set_valid (RejillaToolDialog *dialog,
			       gboolean valid);
void
rejilla_tool_dialog_set_medium_type_shown (RejillaToolDialog *dialog,
					   RejillaMediaType media_type);
void
rejilla_tool_dialog_set_progress (RejillaToolDialog *dialog,
				  gdouble overall_progress,
				  gdouble task_progress,
				  glong remaining,
				  gint size_mb,
				  gint written_mb);
void
rejilla_tool_dialog_set_action (RejillaToolDialog *dialog,
				RejillaBurnAction action,
				const gchar *string);

RejillaBurn *
rejilla_tool_dialog_get_burn (RejillaToolDialog *dialog);

RejillaMedium *
rejilla_tool_dialog_get_medium (RejillaToolDialog *dialog);

G_END_DECLS

#endif
