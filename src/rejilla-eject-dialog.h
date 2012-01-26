/***************************************************************************
 *            
 *
 *  Copyright  2008  Philippe Rouquier <rejilla-app@wanadoo.fr>
 *  Copyright  2008  Luis Medinas <lmedinas@gmail.com>
 *
 *
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
 *
 */

#ifndef REJILLA_EJECT_DIALOG_H
#define REJILLA_EJECT_DIALOG_H

#include <glib.h>
#include <glib-object.h>

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define REJILLA_TYPE_EJECT_DIALOG         (rejilla_eject_dialog_get_type ())
#define REJILLA_EJECT_DIALOG(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), REJILLA_TYPE_EJECT_DIALOG, RejillaEjectDialog))
#define REJILLA_EJECT_DIALOG_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), REJILLA_TYPE_EJECT_DIALOG, RejillaEjectDialogClass))
#define REJILLA_IS_EJECT_DIALOG(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), REJILLA_TYPE_EJECT_DIALOG))
#define REJILLA_IS_EJECT_DIALOG_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), REJILLA_TYPE_EJECT_DIALOG))
#define REJILLA_EJECT_DIALOG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), REJILLA_TYPE_EJECT_DIALOG, RejillaEjectDialogClass))

typedef struct _RejillaEjectDialog RejillaEjectDialog;
typedef struct _RejillaEjectDialogClass RejillaEjectDialogClass;

struct _RejillaEjectDialog {
	GtkDialog parent;
};

struct _RejillaEjectDialogClass {
	GtkDialogClass parent_class;
};

GType rejilla_eject_dialog_get_type (void);
GtkWidget *rejilla_eject_dialog_new (void);

gboolean
rejilla_eject_dialog_cancel (RejillaEjectDialog *dialog);

G_END_DECLS

#endif /* REJILLA_Eject_DIALOG_H */
