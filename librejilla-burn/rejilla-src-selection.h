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

#ifndef _REJILLA_SRC_SELECTION_H_
#define _REJILLA_SRC_SELECTION_H_

#include <glib-object.h>

#include "rejilla-medium-selection.h"
#include "rejilla-session.h"

G_BEGIN_DECLS

#define REJILLA_TYPE_SRC_SELECTION             (rejilla_src_selection_get_type ())
#define REJILLA_SRC_SELECTION(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_SRC_SELECTION, RejillaSrcSelection))
#define REJILLA_SRC_SELECTION_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_SRC_SELECTION, RejillaSrcSelectionClass))
#define REJILLA_IS_SRC_SELECTION(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_SRC_SELECTION))
#define REJILLA_IS_SRC_SELECTION_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_SRC_SELECTION))
#define REJILLA_SRC_SELECTION_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_SRC_SELECTION, RejillaSrcSelectionClass))

typedef struct _RejillaSrcSelectionClass RejillaSrcSelectionClass;
typedef struct _RejillaSrcSelection RejillaSrcSelection;

struct _RejillaSrcSelectionClass
{
	RejillaMediumSelectionClass parent_class;
};

struct _RejillaSrcSelection
{
	RejillaMediumSelection parent_instance;
};

GType rejilla_src_selection_get_type (void) G_GNUC_CONST;

GtkWidget *
rejilla_src_selection_new (RejillaBurnSession *session);

G_END_DECLS

#endif /* _REJILLA_SRC_SELECTION_H_ */
