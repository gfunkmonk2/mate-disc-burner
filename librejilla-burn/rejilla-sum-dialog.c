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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>

#include "rejilla-sum-dialog.h"
#include "rejilla-tool-dialog.h"
#include "rejilla-tool-dialog-private.h"


#include "rejilla-misc.h"
#include "rejilla-xfer.h"

#include "rejilla-drive.h"
#include "rejilla-medium.h"
#include "rejilla-volume.h"

#include "burn-basics.h"

#include "rejilla-tags.h"
#include "rejilla-track-disc.h"

#include "rejilla-session-helper.h"
#include "rejilla-burn.h"

G_DEFINE_TYPE (RejillaSumDialog, rejilla_sum_dialog, REJILLA_TYPE_TOOL_DIALOG);

typedef struct _RejillaSumDialogPrivate RejillaSumDialogPrivate;
struct _RejillaSumDialogPrivate {
	RejillaBurnSession *session;

	GtkWidget *md5_chooser;
	GtkWidget *md5_check;

	RejillaXferCtx *xfer_ctx;
	GCancellable *xfer_cancel;
};

#define REJILLA_SUM_DIALOG_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REJILLA_TYPE_SUM_DIALOG, RejillaSumDialogPrivate))

static void
rejilla_sum_dialog_md5_toggled (GtkToggleButton *button,
				RejillaSumDialog *self)
{
	RejillaSumDialogPrivate *priv;

	priv = REJILLA_SUM_DIALOG_PRIVATE (self);
	gtk_widget_set_sensitive (priv->md5_chooser,
				  gtk_toggle_button_get_active (button));  
}

static void
rejilla_sum_dialog_stop (RejillaSumDialog *self)
{
	RejillaSumDialogPrivate *priv;

	priv = REJILLA_SUM_DIALOG_PRIVATE (self);
	if (priv->xfer_cancel)
		g_cancellable_cancel (priv->xfer_cancel);
}

static gboolean
rejilla_sum_dialog_message (RejillaSumDialog *self,
			    const gchar *primary_message,
			    const gchar *secondary_message,
			    GtkMessageType type)
{
	GtkWidget *button;
	GtkWidget *message;
	GtkResponseType answer;

	rejilla_tool_dialog_set_progress (REJILLA_TOOL_DIALOG (self),
					  1.0,
					  1.0,
					  -1,
					  -1,
					  -1);

	message = gtk_message_dialog_new (GTK_WINDOW (self),
					  GTK_DIALOG_MODAL |
					  GTK_DIALOG_DESTROY_WITH_PARENT,
					  type,
					  GTK_BUTTONS_NONE,
					  "%s", primary_message);

	gtk_window_set_icon_name (GTK_WINDOW (message),
	                          gtk_window_get_icon_name (GTK_WINDOW (self)));

	gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (message),
						  "%s.",
						  secondary_message);

	button = rejilla_utils_make_button (_("Check _Again"),
					    GTK_STOCK_FIND,
					    NULL,
					    GTK_ICON_SIZE_BUTTON);
	gtk_widget_show (button);
	gtk_dialog_add_action_widget (GTK_DIALOG (message),
				      button,
				      GTK_RESPONSE_OK);

	gtk_dialog_add_button (GTK_DIALOG (message),
			       GTK_STOCK_CLOSE,
			       GTK_RESPONSE_CLOSE);

	answer = gtk_dialog_run (GTK_DIALOG (message));
	gtk_widget_destroy (message);

	if (answer == GTK_RESPONSE_OK)
		return FALSE;

	return TRUE;
}

static gboolean
rejilla_sum_dialog_message_error (RejillaSumDialog *self,
				  const GError *error)
{
	rejilla_tool_dialog_set_action (REJILLA_TOOL_DIALOG (self),
					REJILLA_BURN_ACTION_NONE,
					NULL);

	return rejilla_sum_dialog_message (self,
					   _("The file integrity check could not be performed."),
					   error ? error->message:_("An unknown error occurred"),
					   GTK_MESSAGE_ERROR);
}

static gboolean
rejilla_sum_dialog_success (RejillaSumDialog *self)
{
	rejilla_tool_dialog_set_action (REJILLA_TOOL_DIALOG (self),
					REJILLA_BURN_ACTION_FINISHED,
					NULL);

	return rejilla_sum_dialog_message (self,
					   _("The file integrity check was performed successfully."),
					   _("There seem to be no corrupted files on the disc"),
					   GTK_MESSAGE_INFO);
}

enum {
	REJILLA_SUM_DIALOG_PATH,
	REJILLA_SUM_DIALOG_NB_COL
};

static gboolean
rejilla_sum_dialog_corruption_warning (RejillaSumDialog *self,
				       const gchar **wrong_sums)
{
	GtkWidget *tree;
	GtkWidget *scroll;
	GtkWidget *button;
	GtkWidget *message;
	GtkTreeModel *model;
	GtkResponseType answer;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	message = gtk_message_dialog_new_with_markup (GTK_WINDOW (self),
						      GTK_DIALOG_MODAL |
						      GTK_DIALOG_DESTROY_WITH_PARENT,
						      GTK_MESSAGE_ERROR,
						      GTK_BUTTONS_NONE,
						      "<b><big>%s</big></b>",
						      _("The following files appear to be corrupted:"));

	gtk_window_set_icon_name (GTK_WINDOW (message),
	                          gtk_window_get_icon_name (GTK_WINDOW (self)));

	gtk_window_set_resizable (GTK_WINDOW (message), TRUE);
	gtk_widget_set_size_request (GTK_WIDGET (message), 440, 300);

	button = rejilla_utils_make_button (_("Check _Again"),
					    GTK_STOCK_FIND,
					    NULL,
					    GTK_ICON_SIZE_BUTTON);
	gtk_widget_show (button);
	gtk_dialog_add_action_widget (GTK_DIALOG (message),
				      button,
				      GTK_RESPONSE_OK);

	gtk_dialog_add_button (GTK_DIALOG (message),
			       GTK_STOCK_CLOSE,
			       GTK_RESPONSE_CLOSE);

	/* build a list */
	model = GTK_TREE_MODEL (gtk_list_store_new (REJILLA_SUM_DIALOG_NB_COL, G_TYPE_STRING));
	for (; wrong_sums && (*wrong_sums); wrong_sums ++) {
		const gchar *path;
		GtkTreeIter tree_iter;

		path = (*wrong_sums);
		gtk_list_store_append (GTK_LIST_STORE (model), &tree_iter);
		gtk_list_store_set (GTK_LIST_STORE (model), &tree_iter,
				    REJILLA_SUM_DIALOG_PATH, path,
				    -1);
	}

	tree = gtk_tree_view_new_with_model (model);
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (tree), TRUE);

	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_add_attribute (column, renderer,
					    "text", REJILLA_SUM_DIALOG_PATH);
	gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
	gtk_tree_view_column_set_title (column, _("Corrupted Files"));

	scroll = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scroll),
					     GTK_SHADOW_ETCHED_IN);

	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scroll),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (scroll), tree);
	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (message))),
			    scroll, 
			    TRUE,
			    TRUE,
			    0);

	gtk_widget_show_all (scroll);

	answer = gtk_dialog_run (GTK_DIALOG (message));
	gtk_widget_destroy (message);

	if (answer == GTK_RESPONSE_OK)
		return FALSE;

	return TRUE;
}

static gboolean
rejilla_sum_dialog_progress_poll (gpointer user_data)
{
	RejillaSumDialogPrivate *priv;
	RejillaSumDialog *self;
	gdouble progress = 0.0;
	gint64 written, total;

	self = REJILLA_SUM_DIALOG (user_data);

	priv = REJILLA_SUM_DIALOG_PRIVATE (self);

	if (!priv->xfer_ctx)
		return TRUE;

	rejilla_xfer_get_progress (priv->xfer_ctx,
				   &written,
				   &total);

	progress = (gdouble) written / (gdouble) total;

	rejilla_tool_dialog_set_progress (REJILLA_TOOL_DIALOG (self),
					  progress,
					  -1.0,
					  -1,
					  -1,
					  -1);
	return TRUE;
}

static RejillaBurnResult
rejilla_sum_dialog_download (RejillaSumDialog *self,
			     const gchar *src,
			     gchar **retval,
			     GError **error)
{
	RejillaSumDialogPrivate *priv;
	gboolean result;
	gchar *tmppath;
	gint id;
	int fd;

	priv = REJILLA_SUM_DIALOG_PRIVATE (self);

	/* create the temp destination */
	tmppath = g_strdup_printf ("%s/"REJILLA_BURN_TMP_FILE_NAME,
				   g_get_tmp_dir ());
	fd = g_mkstemp (tmppath);
	if (fd < 0) {
		int errnum = errno;

		g_free (tmppath);
		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_GENERAL,
			     "%s (%s)",
			     _("A file could not be created at the location specified for temporary files"),
			     g_strerror (errnum));

		return REJILLA_BURN_ERR;
	}
	close (fd);

	rejilla_tool_dialog_set_action (REJILLA_TOOL_DIALOG (self),
					REJILLA_BURN_ACTION_FILE_COPY,
					_("Downloading MD5 file"));

	id = g_timeout_add (500,
			    rejilla_sum_dialog_progress_poll,
			    self);

	priv->xfer_ctx = rejilla_xfer_new ();
	priv->xfer_cancel = g_cancellable_new ();

	result = rejilla_xfer_wait (priv->xfer_ctx,
				    src,
				    tmppath,
				    priv->xfer_cancel,
				    error);

	g_source_remove (id);

	g_object_unref (priv->xfer_cancel);
	priv->xfer_cancel = NULL;

	rejilla_xfer_free (priv->xfer_ctx);
	priv->xfer_ctx = NULL;

	if (!result) {
		g_remove (tmppath);
		g_free (tmppath);
		return result;
	}

	*retval = tmppath;
	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_sum_dialog_get_file_checksum (RejillaSumDialog *self,
				      const gchar *file_path,
				      gchar **checksum,
				      GError **error)
{
	RejillaBurnResult result;
	gchar buffer [33];
	GFile *file_src;
	gchar *tmppath;
	gchar *scheme;
	gchar *uri;
	gchar *src;
	FILE *file;
	int read;

	/* see if this file needs downloading */
	file_src = g_file_new_for_commandline_arg (file_path);
	if (!file_src) {
		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_GENERAL,
			     _("\"%s\" is not a valid URI"),
			     file_path);
		return REJILLA_BURN_ERR;
	}

	tmppath = NULL;
	scheme = g_file_get_uri_scheme (file_src);
	if (strcmp (scheme, "file")) {
		uri = g_file_get_uri (file_src);
		g_object_unref (file_src);

		result = rejilla_sum_dialog_download (self,
						      uri,
						      &tmppath,
						      error);
		if (result != REJILLA_BURN_CANCEL) {
			g_free (scheme);
			return result;
		}

		src = tmppath;
	}
	else {
		src = g_file_get_path (file_src);
		g_object_unref (file_src);
	}
	g_free (scheme);

	/* now get the md5 sum from the file */
	file = fopen (src, "r");
	if (!file) {
                int errsv = errno;

		if (tmppath)
			g_remove (tmppath);

		g_free (src);

		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_GENERAL,
			     "%s",
			     g_strerror (errsv));
		return REJILLA_BURN_ERR;
	}

	read = fread (buffer, 1, sizeof (buffer) - 1, file);
	if (read)
		buffer [read] = '\0';

	if (tmppath)
		g_remove (tmppath);

	g_free (src);

	if (ferror (file)) {
		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_GENERAL,
			     "%s",
			     g_strerror (errno));

		fclose (file);
		return REJILLA_BURN_ERR;
	}

	fclose (file);

	*checksum = strdup (buffer);
	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_sum_dialog_get_disc_checksum (RejillaSumDialog *self,
				      RejillaDrive *drive,
				      gchar *checksum,
				      GError **error)
{
	RejillaTrackDisc *track = NULL;
	RejillaSumDialogPrivate *priv;
	RejillaBurnResult result;
	RejillaBurn *burn;

	priv = REJILLA_SUM_DIALOG_PRIVATE (self);

	track = rejilla_track_disc_new ();
	rejilla_track_disc_set_drive (track, drive);
	rejilla_track_set_checksum (REJILLA_TRACK (track), REJILLA_CHECKSUM_MD5, checksum);
	rejilla_burn_session_add_track (priv->session, REJILLA_TRACK (track), NULL);

	/* It's good practice to unref the track afterwards as we don't need it
	 * anymore. RejillaBurnSession refs it. */
	g_object_unref (track);

	burn = rejilla_tool_dialog_get_burn (REJILLA_TOOL_DIALOG (self));
	result = rejilla_burn_check (burn, priv->session, error);

	return result;
}

static gboolean
rejilla_sum_dialog_check_md5_file (RejillaSumDialog *self,
				   RejillaMedium *medium)
{
	RejillaSumDialogPrivate *priv;
	RejillaBurnResult result;
	gchar *file_sum = NULL;
	GError *error = NULL;
	gboolean retval;
	gchar *uri;

	priv = REJILLA_SUM_DIALOG_PRIVATE (self);

	/* get the sum from the file */
    	uri = gtk_file_chooser_get_uri (GTK_FILE_CHOOSER (priv->md5_chooser));
	if (!uri) {
		retval = rejilla_sum_dialog_message (self,
						     _("The file integrity check could not be performed."),
						     error ? error->message:_("No MD5 file was given."),
						     GTK_MESSAGE_ERROR);
		return retval;
	}

	result = rejilla_sum_dialog_get_file_checksum (self, uri, &file_sum, &error);
	g_free (uri);

	if (result == REJILLA_BURN_CANCEL)
		return FALSE;

	if (result != REJILLA_BURN_OK) {
		retval = rejilla_sum_dialog_message_error (self, error);

		if (error)
			g_error_free (error);

		return retval;
	}

	result = rejilla_sum_dialog_get_disc_checksum (self,
						       rejilla_medium_get_drive (medium),
						       file_sum,
						       &error);
	if (result == REJILLA_BURN_CANCEL) {
		g_free (file_sum);
		return FALSE;
	}

	if (result != REJILLA_BURN_OK) {
		g_free (file_sum);

		retval = rejilla_sum_dialog_message_error (self, error);

		if (error)
			g_error_free (error);

		return retval;
	}

	return rejilla_sum_dialog_success (self);
}

static gboolean
rejilla_sum_dialog_check_disc_sum (RejillaSumDialog *self,
				   RejillaDrive *drive)
{
	RejillaSumDialogPrivate *priv;
	RejillaBurnResult result;
	RejillaTrackDisc *track;
	GError *error = NULL;
	GValue *value = NULL;
	RejillaBurn *burn;
	gboolean retval;

	priv = REJILLA_SUM_DIALOG_PRIVATE (self);

	/* make track */
	track = rejilla_track_disc_new ();
	rejilla_track_disc_set_drive (track, drive);
	rejilla_track_set_checksum (REJILLA_TRACK (track), REJILLA_CHECKSUM_DETECT, NULL);
	rejilla_burn_session_add_track (priv->session, REJILLA_TRACK (track), NULL);

	/* no eject at the end (it should be default) */
	rejilla_burn_session_remove_flag (priv->session, REJILLA_BURN_FLAG_EJECT);

	/* It's good practice to unref the track afterwards as we don't need it
	 * anymore. RejillaBurnSession refs it. */
	g_object_unref (track);

	burn = rejilla_tool_dialog_get_burn (REJILLA_TOOL_DIALOG (self));
	result = rejilla_burn_check (burn, priv->session, &error);

	if (result == REJILLA_BURN_CANCEL) {
		if (error)
			g_error_free (error);

		return FALSE;
	}

	if (result == REJILLA_BURN_OK)
		return rejilla_sum_dialog_success (self);

	if (!error || error->code != REJILLA_BURN_ERROR_BAD_CHECKSUM) {
		retval = rejilla_sum_dialog_message_error (self, error);

		if (error)
			g_error_free (error);

		return retval;
	}

	g_error_free (error);

	rejilla_track_tag_lookup (REJILLA_TRACK (track),
				  REJILLA_TRACK_MEDIUM_WRONG_CHECKSUM_TAG,
				  &value);

	return rejilla_sum_dialog_corruption_warning (self, g_value_get_boxed (value));
}

static gboolean
rejilla_sum_dialog_activate (RejillaToolDialog *dialog,
			     RejillaMedium *medium)
{
	gboolean result;
	RejillaSumDialog *self;
	RejillaSumDialogPrivate *priv;

	self = REJILLA_SUM_DIALOG (dialog);
	priv = REJILLA_SUM_DIALOG_PRIVATE (dialog);

	rejilla_burn_session_start (priv->session);
	if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->md5_check)))
		result = rejilla_sum_dialog_check_disc_sum (self, rejilla_medium_get_drive (medium));
	else
		result = rejilla_sum_dialog_check_md5_file (self, medium);

	rejilla_tool_dialog_set_valid (dialog, TRUE);
	return result;
}

static void
rejilla_sum_dialog_drive_changed (RejillaToolDialog *dialog,
				  RejillaMedium *medium)
{
	if (medium)
		rejilla_tool_dialog_set_valid (dialog, REJILLA_MEDIUM_VALID (rejilla_medium_get_status (medium)));
	else
		rejilla_tool_dialog_set_valid (dialog, FALSE);
}

static void
rejilla_sum_dialog_finalize (GObject *object)
{
	RejillaSumDialogPrivate *priv;

	priv = REJILLA_SUM_DIALOG_PRIVATE (object);

	rejilla_sum_dialog_stop (REJILLA_SUM_DIALOG (object));

	if (priv->session) {
		g_object_unref (priv->session);
		priv->session = NULL;
	}

	G_OBJECT_CLASS (rejilla_sum_dialog_parent_class)->finalize (object);
}

static void
rejilla_sum_dialog_class_init (RejillaSumDialogClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	RejillaToolDialogClass *tool_dialog_class = REJILLA_TOOL_DIALOG_CLASS (klass);

	g_type_class_add_private (klass, sizeof (RejillaSumDialogPrivate));

	rejilla_sum_dialog_parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = rejilla_sum_dialog_finalize;

	tool_dialog_class->activate = rejilla_sum_dialog_activate;
	tool_dialog_class->medium_changed = rejilla_sum_dialog_drive_changed;
}

static void
rejilla_sum_dialog_init (RejillaSumDialog *obj)
{
	GtkWidget *box;
	RejillaMedium *medium;
	RejillaSumDialogPrivate *priv;

	priv = REJILLA_SUM_DIALOG_PRIVATE (obj);

	priv->session = rejilla_burn_session_new ();

	box = gtk_vbox_new (FALSE, 6);

	priv->md5_check = gtk_check_button_new_with_mnemonic (_("Use an _MD5 file to check the disc"));
	gtk_widget_set_tooltip_text (priv->md5_check, _("Use an external .md5 file that stores the checksum of a disc"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->md5_check), FALSE);
	g_signal_connect (priv->md5_check,
			  "toggled",
			  G_CALLBACK (rejilla_sum_dialog_md5_toggled),
			  obj);

	gtk_box_pack_start (GTK_BOX (box),
			    priv->md5_check,
			    TRUE,
			    TRUE,
			    0);

	priv->md5_chooser = gtk_file_chooser_button_new (_("Open an MD5 file"), GTK_FILE_CHOOSER_ACTION_OPEN);
	gtk_file_chooser_set_local_only (GTK_FILE_CHOOSER (priv->md5_chooser), FALSE);
	gtk_widget_set_sensitive (priv->md5_chooser, FALSE);
	gtk_box_pack_start (GTK_BOX (box),
			    priv->md5_chooser,
			    TRUE,
			    TRUE,
			    0);

	gtk_widget_show_all (box);
	rejilla_tool_dialog_pack_options (REJILLA_TOOL_DIALOG (obj),
					  box,
					  NULL);

	rejilla_tool_dialog_set_button (REJILLA_TOOL_DIALOG (obj),
					_("_Check"),
					GTK_STOCK_FIND,
					NULL);

	/* only media with data, no blank medium */
	rejilla_tool_dialog_set_medium_type_shown (REJILLA_TOOL_DIALOG (obj),
						   REJILLA_MEDIA_TYPE_AUDIO|
						   REJILLA_MEDIA_TYPE_DATA);

	medium = rejilla_tool_dialog_get_medium (REJILLA_TOOL_DIALOG (obj));
	if (medium) {
		rejilla_tool_dialog_set_valid (REJILLA_TOOL_DIALOG (obj), REJILLA_MEDIUM_VALID (rejilla_medium_get_status (medium)));
		g_object_unref (medium);
	}
	else
		rejilla_tool_dialog_set_valid (REJILLA_TOOL_DIALOG (obj), FALSE);
}

/**
 * rejilla_sum_dialog_new:
 *
 * Creates a new #RejillaSumDialog object
 *
 * Return value: a #RejillaSumDialog. Unref when it is not needed anymore.
 **/
RejillaSumDialog *
rejilla_sum_dialog_new ()
{
	RejillaSumDialog *obj;
	
	obj = REJILLA_SUM_DIALOG (g_object_new (REJILLA_TYPE_SUM_DIALOG, NULL));
	gtk_window_set_title (GTK_WINDOW (obj), _("Disc Checking"));
	
	return obj;
}
