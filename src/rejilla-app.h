/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Rejilla
 * Copyright (C) Philippe Rouquier 2005-2009 <bonfire-app@wanadoo.fr>
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

#ifndef _REJILLA_APP_H_
#define _REJILLA_APP_H_

#include <glib-object.h>
#include <gtk/gtk.h>

#include <unique/unique.h>

#include "rejilla-session-cfg.h"

G_BEGIN_DECLS

#define REJILLA_TYPE_APP             (rejilla_app_get_type ())
#define REJILLA_APP(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_APP, RejillaApp))
#define REJILLA_APP_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_APP, RejillaAppClass))
#define REJILLA_IS_APP(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_APP))
#define REJILLA_IS_APP_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_APP))
#define REJILLA_APP_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_APP, RejillaAppClass))

typedef struct _RejillaAppClass RejillaAppClass;
typedef struct _RejillaApp RejillaApp;

struct _RejillaAppClass
{
	GtkWindowClass parent_class;
};

struct _RejillaApp
{
	GtkWindow parent_instance;
};

GType rejilla_app_get_type (void) G_GNUC_CONST;

RejillaApp *
rejilla_app_new (UniqueApp *gapp);

RejillaApp *
rejilla_app_get_default (void);

void
rejilla_app_set_parent (RejillaApp *app,
			guint xid);

void
rejilla_app_set_toplevel (RejillaApp *app, GtkWindow *window);

void
rejilla_app_create_mainwin (RejillaApp *app);

gboolean
rejilla_app_run_mainwin (RejillaApp *app);

gboolean
rejilla_app_is_running (RejillaApp *app);

GtkWidget *
rejilla_app_dialog (RejillaApp *app,
		    const gchar *primary_message,
		    GtkButtonsType button_type,
		    GtkMessageType msg_type);

void
rejilla_app_alert (RejillaApp *app,
		   const gchar *primary_message,
		   const gchar *secondary_message,
		   GtkMessageType type);

gboolean
rejilla_app_burn (RejillaApp *app,
		  RejillaBurnSession *session,
		  gboolean multi);

void
rejilla_app_burn_uri (RejillaApp *app,
		      RejillaDrive *burner,
		      gboolean burn);

void
rejilla_app_data (RejillaApp *app,
		  RejillaDrive *burner,
		  gchar * const *uris,
		  gboolean burn);

void
rejilla_app_stream (RejillaApp *app,
		    RejillaDrive *burner,
		    gchar * const *uris,
		    gboolean is_video,
		    gboolean burn);

void
rejilla_app_image (RejillaApp *app,
		   RejillaDrive *burner,
		   const gchar *uri,
		   gboolean burn);

void
rejilla_app_copy_disc (RejillaApp *app,
		       RejillaDrive *burner,
		       const gchar *device,
		       const gchar *cover,
		       gboolean burn);

void
rejilla_app_blank (RejillaApp *app,
		   RejillaDrive *burner,
		   gboolean burn);

void
rejilla_app_check (RejillaApp *app,
		   RejillaDrive *burner,
		   gboolean burn);

gboolean
rejilla_app_open_project (RejillaApp *app,
			  RejillaDrive *burner,
                          const gchar *uri,
                          gboolean is_playlist,
                          gboolean warn_user,
                          gboolean burn);

gboolean
rejilla_app_open_uri (RejillaApp *app,
                      const gchar *uri_arg,
                      gboolean warn_user);

gboolean
rejilla_app_open_uri_drive_detection (RejillaApp *app,
                                      RejillaDrive *burner,
                                      const gchar *uri,
                                      const gchar *cover_project,
                                      gboolean burn);
GtkWidget *
rejilla_app_get_statusbar1 (RejillaApp *app);

GtkWidget *
rejilla_app_get_statusbar2 (RejillaApp *app);

GtkWidget *
rejilla_app_get_project_manager (RejillaApp *app);

/**
 * Session management
 */

#define REJILLA_SESSION_TMP_PROJECT_PATH	"rejilla-tmp-project"

const gchar *
rejilla_app_get_saved_contents (RejillaApp *app);

gboolean
rejilla_app_save_contents (RejillaApp *app,
			   gboolean cancellable);

G_END_DECLS

#endif /* _REJILLA_APP_H_ */
