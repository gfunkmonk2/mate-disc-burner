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

#ifndef _REJILLA_DRIVE_SELECTION_H_
#define _REJILLA_DRIVE_SELECTION_H_

#include <glib-object.h>

#include <gtk/gtk.h>

#include <rejilla-medium-monitor.h>
#include <rejilla-drive.h>

G_BEGIN_DECLS

#define REJILLA_TYPE_DRIVE_SELECTION             (rejilla_drive_selection_get_type ())
#define REJILLA_DRIVE_SELECTION(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_DRIVE_SELECTION, RejillaDriveSelection))
#define REJILLA_DRIVE_SELECTION_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_DRIVE_SELECTION, RejillaDriveSelectionClass))
#define REJILLA_IS_DRIVE_SELECTION(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_DRIVE_SELECTION))
#define REJILLA_IS_DRIVE_SELECTION_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_DRIVE_SELECTION))
#define REJILLA_DRIVE_SELECTION_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_DRIVE_SELECTION, RejillaDriveSelectionClass))

typedef struct _RejillaDriveSelectionClass RejillaDriveSelectionClass;
typedef struct _RejillaDriveSelection RejillaDriveSelection;

struct _RejillaDriveSelectionClass
{
	GtkComboBoxClass parent_class;

	/* Signals */
	void		(* drive_changed)		(RejillaDriveSelection *selector,
							 RejillaDrive *drive);
};

struct _RejillaDriveSelection
{
	GtkComboBox parent_instance;
};

G_MODULE_EXPORT GType rejilla_drive_selection_get_type (void) G_GNUC_CONST;

GtkWidget* rejilla_drive_selection_new (void);

RejillaDrive *
rejilla_drive_selection_get_active (RejillaDriveSelection *selector);

gboolean
rejilla_drive_selection_set_active (RejillaDriveSelection *selector,
				    RejillaDrive *drive);

void
rejilla_drive_selection_show_type (RejillaDriveSelection *selector,
				   RejillaDriveType type);

G_END_DECLS

#endif /* _REJILLA_DRIVE_SELECTION_H_ */
