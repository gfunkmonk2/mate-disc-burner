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
 *            rejilla-session.c
 *
 *  Thu May 18 18:32:37 2006
 *  Copyright  2006  Philippe Rouquier
 *  <rejilla-app@wanadoo.fr>
 ****************************************************************************/

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>

#include <glib.h>
#include <glib/gstdio.h>

#include <gtk/gtk.h>

#include "rejilla-xsession.h"

#include "eggsmclient.h"

#include "rejilla-app.h"
#include "rejilla-session.h"

/**
 * This code is for session management
 */

static void
rejilla_session_quit_cb (EggSMClient *client,
			 RejillaApp *app)
{
	/* With this always exit whatever we're doing. So exit fast. */
	gtk_widget_destroy (GTK_WIDGET (app));
}

static void
rejilla_session_quit_requested_cb (EggSMClient *client,
				   RejillaApp *app)
{
	/* See if we can quit */
	egg_sm_client_will_quit (client, (rejilla_app_save_contents (app, TRUE) == FALSE));
}

static void
rejilla_session_save_state_cb (EggSMClient *client,
			       GKeyFile *key_file,
			       RejillaApp *app)
{
    	const gint argc = 3;
    	const gchar *argv [] = { "rejilla", "-p", NULL, NULL };

	/* Try to save its contents */
	argv [2] = rejilla_app_get_saved_contents (app);

	/* How to restart the application */
	egg_sm_client_set_restart_command (client,
					   argc,
					   argv);

	/* How to discard the save */
/*	argv [0] = "rm";
	argv [1] = "-f";
	argv [2] = project_path;
	egg_sm_client_set_discard_command (client,
					   argc,
					   (const char **) argv);
*/
    	gtk_widget_destroy (GTK_WIDGET (app));
}

gboolean
rejilla_session_connect (RejillaApp *app)
{
	EggSMClient *client;

	client = egg_sm_client_get ();
	if (client) {
		g_signal_connect (client,
				  "quit",
				  G_CALLBACK (rejilla_session_quit_cb),
				  app);
		g_signal_connect (client,
				  "quit-requested",
				  G_CALLBACK (rejilla_session_quit_requested_cb),
				  app);
		g_signal_connect (client,
				  "save-state",
				  G_CALLBACK (rejilla_session_save_state_cb),
				  app);

		return TRUE;
	}

	return FALSE;
}

void
rejilla_session_disconnect (RejillaApp *app)
{
	EggSMClient *client;

	client = egg_sm_client_get ();

	g_signal_handlers_disconnect_by_func (client,
					      rejilla_session_quit_cb,
					      app);
	g_signal_handlers_disconnect_by_func (client,
					      rejilla_session_quit_requested_cb,
					      app);
	g_signal_handlers_disconnect_by_func (client,
					      rejilla_session_save_state_cb,
					      app);
}
