/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * rejilla
 * Copyright (C) Philippe Rouquier 2005-2008 <bonfire-app@wanadoo.fr>
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

#ifndef _REJILLA_FILE_FILTERED_H_
#define _REJILLA_FILE_FILTERED_H_

#include <glib-object.h>
#include <gtk/gtk.h>

#include "rejilla-track-data-cfg.h"

G_BEGIN_DECLS

#define REJILLA_TYPE_FILE_FILTERED             (rejilla_file_filtered_get_type ())
#define REJILLA_FILE_FILTERED(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_FILE_FILTERED, RejillaFileFiltered))
#define REJILLA_FILE_FILTERED_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_FILE_FILTERED, RejillaFileFilteredClass))
#define REJILLA_IS_FILE_FILTERED(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_FILE_FILTERED))
#define REJILLA_IS_FILE_FILTERED_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_FILE_FILTERED))
#define REJILLA_FILE_FILTERED_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_FILE_FILTERED, RejillaFileFilteredClass))

typedef struct _RejillaFileFilteredClass RejillaFileFilteredClass;
typedef struct _RejillaFileFiltered RejillaFileFiltered;

struct _RejillaFileFilteredClass
{
	GtkExpanderClass parent_class;
};

struct _RejillaFileFiltered
{
	GtkExpander parent_instance;
};

GType rejilla_file_filtered_get_type (void) G_GNUC_CONST;

GtkWidget*
rejilla_file_filtered_new (RejillaTrackDataCfg *track);

void
rejilla_file_filtered_set_right_button_group (RejillaFileFiltered *self,
					      GtkSizeGroup *group);

G_END_DECLS

#endif /* _REJILLA_FILE_FILTERED_H_ */
