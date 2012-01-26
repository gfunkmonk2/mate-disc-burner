/*
 * Rejilla is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * Rejilla is distributed in the hope that it will be useful,
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
/***************************************************************************
 *            utils.c
 *
 *  Wed May 18 16:58:16 2005
 *  Copyright  2005  Philippe Rouquier
 *  <rejilla-app@wanadoo.fr>
 ****************************************************************************/


#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <glib.h>
#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>

#include "rejilla-utils.h"
#include "rejilla-app.h"

#define REJILLA_ERROR rejilla_error_quark()

GQuark
rejilla_error_quark (void)
{
	static GQuark quark = 0;

	if (!quark)
		quark = g_quark_from_static_string ("BraSero_error");

	return quark;
}

void
rejilla_utils_launch_app (GtkWidget *widget,
			  GSList *list)
{
	GSList *item;

	for (item = list; item; item = item->next) {
		GError *error;
		gchar *uri;

		error = NULL;
		uri = item->data;

		if (!g_app_info_launch_default_for_uri (uri, NULL, &error)) {
			gchar *string;

			string = g_strdup_printf ("\"%s\" could not be opened", uri);
			rejilla_app_alert (rejilla_app_get_default (),
					   string,
					   error->message,
					   GTK_MESSAGE_ERROR);
			g_free (string);
			g_error_free (error);
			continue;
		}
	}
}

gboolean
rejilla_clipboard_selection_may_have_uri (GdkAtom *atoms,
					  gint n_atoms)
{
	GdkAtom *iter;

	/* Check for a text target */
	if (gtk_targets_include_text (atoms, n_atoms))
		return TRUE;

	/* Check for special targets like caja' and its file copied */
	iter = atoms;
	while (n_atoms > 0) {
		gchar *target;

		target = gdk_atom_name (*iter);
		if (!strcmp (target, "x-special/mate-copied-files")) {
			g_free (target);
			return TRUE;
		}
		g_free (target);

		iter++;
		n_atoms--;
	}

	return FALSE;
}
