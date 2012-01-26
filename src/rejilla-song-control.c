/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * rejilla
 * Copyright (C) Philippe Rouquier 2010 <bonfire-app@wanadoo.fr>
 * 
 * rejilla is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * rejilla is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <glib.h>
#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>

#include <gst/gst.h>

#include "rejilla-song-control.h"
#include "rejilla-units.h"
#include "rejilla-misc.h"
#include "rejilla-setting.h"

typedef struct _RejillaSongControlPrivate RejillaSongControlPrivate;
struct _RejillaSongControlPrivate
{
	GstElement *pipe;
	GstState state;

	gchar *uri;

	gint update_scale_id;

	GtkWidget *progress;
	GtkWidget *button;
	GtkWidget *header;
	GtkWidget *size;

	gsize start;
	gsize end;
};

#define REJILLA_SONG_CONTROL_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REJILLA_TYPE_SONG_CONTROL, RejillaSongControlPrivate))

G_DEFINE_TYPE (RejillaSongControl, rejilla_song_control, GTK_TYPE_ALIGNMENT);

gint64
rejilla_song_control_get_length (RejillaSongControl *self)
{
	RejillaSongControlPrivate *priv;

	priv = REJILLA_SONG_CONTROL_PRIVATE (self);
	if (!priv->uri)
		return -1;

	return priv->end - priv->start;
}

gint64
rejilla_song_control_get_pos (RejillaSongControl *self)
{
	RejillaSongControlPrivate *priv;

	priv = REJILLA_SONG_CONTROL_PRIVATE (self);
	return gtk_range_get_value (GTK_RANGE (priv->progress));
}

static void
rejilla_song_control_update_position (RejillaSongControl *player)
{
	gdouble value;
	GtkAdjustment *adjustment;
	RejillaSongControlPrivate *priv;
	gchar *pos_string, *len_string, *result;

	priv = REJILLA_SONG_CONTROL_PRIVATE (player);

	adjustment = gtk_range_get_adjustment (GTK_RANGE (priv->progress));
	len_string = rejilla_units_get_time_string (priv->end - priv->start, FALSE, FALSE);

	value = gtk_range_get_value (GTK_RANGE (priv->progress));
	pos_string = rejilla_units_get_time_string (value, FALSE, FALSE);

	/**
	 * Translators: this is the position being played in a stream. The 
	 * first %s is the position and the second %s is the whole length of
	 * the stream. I chose to make that translatable in case some languages
	 * don't allow the "/" */
	result = g_strdup_printf (_("%s / %s"), pos_string, len_string);
	g_free (len_string);
	g_free (pos_string);

	gtk_label_set_text (GTK_LABEL (priv->size), result);
	g_free (result);
}

static gboolean
rejilla_song_control_set_pos (RejillaSongControl *player,
                              gint64 pos)
{
	RejillaSongControlPrivate *priv;

	priv = REJILLA_SONG_CONTROL_PRIVATE (player);
	return gst_element_seek (priv->pipe,
				 1.0,
				 GST_FORMAT_TIME,
				 GST_SEEK_FLAG_FLUSH,
				 GST_SEEK_TYPE_SET,
				 (gint64) pos,
				 priv->end ? GST_SEEK_TYPE_SET:GST_SEEK_TYPE_NONE,
				 priv->end);
}

static void
rejilla_song_control_range_value_changed (GtkRange *range,
                                          RejillaSongControl *player)
{
	RejillaSongControlPrivate *priv;

	priv = REJILLA_SONG_CONTROL_PRIVATE (player);

	if (priv->state >= GST_STATE_PAUSED && !priv->update_scale_id) {
		gdouble pos;

		/* user changed the value tell the player/pipeline */
		pos = gtk_range_get_value (GTK_RANGE (priv->progress));
		rejilla_song_control_set_pos (REJILLA_SONG_CONTROL (player), (gint64) pos + priv->start);
	}

	rejilla_song_control_update_position (player);
}

static void
rejilla_song_control_set_length (RejillaSongControl *player)
{
	RejillaSongControlPrivate *priv;

	priv = REJILLA_SONG_CONTROL_PRIVATE (player);
	if (priv->end - priv->start != 0)
		gtk_range_set_range (GTK_RANGE (priv->progress),
				     0.0,
				     (gdouble) priv->end - priv->start);

	rejilla_song_control_update_position (player);
}

static gboolean
rejilla_song_control_update_progress_cb (RejillaSongControl *player)
{
	gint64 pos;
	gboolean result;
	RejillaSongControlPrivate *priv;
	GstFormat format = GST_FORMAT_TIME;

	priv = REJILLA_SONG_CONTROL_PRIVATE (player);
	result = gst_element_query_position (priv->pipe,
					     &format,
					     &pos);

	if (pos >= 0) {
		gtk_range_set_value (GTK_RANGE (priv->progress), (gsize) (pos - priv->start));
		gtk_widget_queue_draw (GTK_WIDGET (priv->progress));
	}

	return TRUE;
}

static gboolean
rejilla_song_control_range_button_pressed_cb (GtkWidget *widget,
                                              GdkEvent *event,
                                              RejillaSongControl *player)
{
	RejillaSongControlPrivate *priv;

	priv = REJILLA_SONG_CONTROL_PRIVATE (player);

	/* stop the automatic update of progress bar position */
	if (priv->update_scale_id) {
		g_source_remove (priv->update_scale_id);
		priv->update_scale_id = 0;
	}

	return FALSE;
}

static gboolean
rejilla_song_control_range_button_released_cb (GtkWidget *widget,
                                               GdkEvent *event,
                                               RejillaSongControl *player)
{
	RejillaSongControlPrivate *priv;

	priv = REJILLA_SONG_CONTROL_PRIVATE (player);

	/* restart the automatic update of progress bar */
	if (priv->state == GST_STATE_PLAYING && !priv->update_scale_id)
		priv->update_scale_id = g_timeout_add (500,
		                                       (GSourceFunc) rejilla_song_control_update_progress_cb,
		                                       player);

	return FALSE;
}

static void
rejilla_song_control_button_clicked_cb (GtkButton *button,
                                        RejillaSongControl *player)
{
	RejillaSongControlPrivate *priv;

	priv = REJILLA_SONG_CONTROL_PRIVATE (player);

	if (priv->state == GST_STATE_READY)
		gst_element_set_state (priv->pipe, GST_STATE_PLAYING);
	else if (priv->state == GST_STATE_PAUSED)
		gst_element_set_state (priv->pipe, GST_STATE_PLAYING);
	else if (priv->state == GST_STATE_PLAYING)
		gst_element_set_state (priv->pipe, GST_STATE_PAUSED);
}

static void
rejilla_song_control_volume_changed_cb (GtkScaleButton *button,
                                        gdouble volume,
                                        RejillaSongControl *player)
{
	RejillaSongControlPrivate *priv;

	priv = REJILLA_SONG_CONTROL_PRIVATE (player);
	g_object_set (priv->pipe,
		      "volume", volume,
		      NULL);
}

const gchar *
rejilla_song_control_get_uri (RejillaSongControl *player)
{
	RejillaSongControlPrivate *priv;

	priv = REJILLA_SONG_CONTROL_PRIVATE (player);
	return priv->uri;
}

void
rejilla_song_control_set_info (RejillaSongControl *player,
                               const gchar *title,
                               const gchar *artist)
{
	gchar *header = NULL;
	RejillaSongControlPrivate *priv;

	priv = REJILLA_SONG_CONTROL_PRIVATE (player);

	rejilla_song_control_set_length (player);

	if (artist) {
		gchar *artist_string;
		gchar *artist_markup;
		gchar *title_string;

		artist_markup = g_markup_printf_escaped ("<span size=\"smaller\"><i>%s</i></span>", artist);

		/* Translators: %s is the name of an artist. */
		artist_string = g_strdup_printf (_("by %s"), artist_markup);
		g_free (artist_markup);

		title_string = g_markup_printf_escaped ("<span weight=\"bold\">%s</span>", title);
		header = g_strdup_printf ("%s\n%s", title_string, artist_string);
		g_free (artist_string);
		g_free (title_string);

		gtk_label_set_ellipsize (GTK_LABEL (priv->header),
					 PANGO_ELLIPSIZE_END);

	}
	else if (title) {
		header = g_markup_printf_escaped ("<span weight=\"bold\">%s</span>", title);
		gtk_label_set_ellipsize (GTK_LABEL (priv->header),
					 PANGO_ELLIPSIZE_END);
	}
	else {
		gchar *name;

	    	REJILLA_GET_BASENAME_FOR_DISPLAY (priv->uri, name);
		header = g_markup_printf_escaped ("<span weight=\"bold\">%s</span>\n", name);
		g_free (name);

		gtk_label_set_ellipsize (GTK_LABEL (priv->header),
					 PANGO_ELLIPSIZE_END);
	}

	gtk_label_set_markup (GTK_LABEL (priv->header), header);
	g_free (header);
}

void
rejilla_song_control_set_boundaries (RejillaSongControl *player, 
                                     gsize start,
                                     gsize end)
{
	RejillaSongControlPrivate *priv;

	priv = REJILLA_SONG_CONTROL_PRIVATE (player);

	priv->start = start;
	priv->end = end;

	rejilla_song_control_set_length (player);
	gtk_range_set_value (GTK_RANGE (priv->progress), 0);

	rejilla_song_control_set_pos (player, start);
}

void
rejilla_song_control_set_uri (RejillaSongControl *player,
                              const gchar *uri)
{
	RejillaSongControlPrivate *priv;

	g_return_if_fail (uri != NULL);

	priv = REJILLA_SONG_CONTROL_PRIVATE (player);

	/* avoid reloading everything if it's the same uri */
	if (!g_strcmp0 (uri, priv->uri))
		return;

	if (priv->uri)
		g_free (priv->uri);

	priv->uri = g_strdup (uri);

	priv->start = 0;
	priv->end = 0;

	gst_element_set_state (priv->pipe, GST_STATE_NULL);
	priv->state = GST_STATE_NULL;

	g_object_set (G_OBJECT (priv->pipe),
		      "uri", uri,
		      NULL);

	gst_element_set_state (priv->pipe, GST_STATE_PAUSED);
}

static gboolean
rejilla_song_control_bus_messages (GstBus *bus,
				   GstMessage *msg,
				   RejillaSongControl *player)
{
	RejillaSongControlPrivate *priv;
	GstStateChangeReturn result;
	GError *error = NULL;
	GstState state;

	priv = REJILLA_SONG_CONTROL_PRIVATE (player);

	switch (GST_MESSAGE_TYPE (msg)) {
	case GST_MESSAGE_EOS:
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->button), FALSE);

		if (priv->update_scale_id) {
			g_source_remove (priv->update_scale_id);
			priv->update_scale_id = 0;
		}

		gtk_range_set_value (GTK_RANGE (priv->progress), 0.0);
		gst_element_set_state (priv->pipe, GST_STATE_PAUSED);
		rejilla_song_control_set_pos (player, priv->start);
		break;

	case GST_MESSAGE_ERROR:
		gst_message_parse_error (msg, &error, NULL);
		g_warning ("%s", error->message);
		g_error_free (error);

		gtk_widget_set_sensitive (priv->button, FALSE);
		gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->button), FALSE);

		gtk_widget_set_sensitive (priv->progress, FALSE);
		gtk_range_set_value (GTK_RANGE (priv->progress), 0.0);
		break;

	case GST_MESSAGE_STATE_CHANGED:
		result = gst_element_get_state (GST_ELEMENT (priv->pipe),
						&state,
						NULL,
						500);

		if (result != GST_STATE_CHANGE_SUCCESS)
			break;

		if (priv->state == state || state < GST_STATE_READY)
			break;

		if (state == GST_STATE_PLAYING) {
			if (priv->state == GST_STATE_READY) {
				gdouble pos;

				pos = gtk_range_get_value (GTK_RANGE (priv->progress));
				rejilla_song_control_set_pos (player, priv->start + pos);

				gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->button), TRUE);
			}

			if (!priv->update_scale_id)
				priv->update_scale_id = g_timeout_add (500,
								       (GSourceFunc) rejilla_song_control_update_progress_cb,
								       player);
		}
		else if (state == GST_STATE_PAUSED) {
			if (priv->state != GST_STATE_PLAYING) {
				gdouble pos;

				pos = gtk_range_get_value (GTK_RANGE (priv->progress));
				rejilla_song_control_set_pos (player, priv->start + pos);
			}

			gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->button), FALSE);	
			if (priv->update_scale_id) {
				g_source_remove (priv->update_scale_id);
				priv->update_scale_id = 0;
			}
		}

		priv->state = state;
		break;

	default:
		break;
	}

	return TRUE;
}

static void
rejilla_song_control_destroy (GtkObject *obj)
{
	RejillaSongControlPrivate *priv;

	priv = REJILLA_SONG_CONTROL_PRIVATE (obj);

	if (priv->update_scale_id) {
		g_source_remove (priv->update_scale_id);
		priv->update_scale_id = 0;
	}

	if (priv->pipe) {
		gdouble volume;

		g_object_get (priv->pipe,
			      "volume", &volume,
			      NULL);
		rejilla_setting_set_value (rejilla_setting_get_default (),
		                           REJILLA_SETTING_PLAYER_VOLUME,
		                           GINT_TO_POINTER ((gint) (volume * 100.0)));

		gst_element_set_state (priv->pipe, GST_STATE_NULL);
		gst_object_unref (GST_OBJECT (priv->pipe));
		priv->pipe = NULL;
	}

	if (priv->uri) {
		g_free (priv->uri);
		priv->uri = NULL;
	}

	GTK_OBJECT_CLASS (rejilla_song_control_parent_class)->destroy (obj);
}

static void
rejilla_song_control_init (RejillaSongControl *object)
{
	RejillaSongControlPrivate *priv;
	GtkWidget *alignment;
	GtkWidget *volume;
	gint volume_value;
	GtkWidget *image;
	GtkWidget *vbox;
	GtkWidget *hbox;
	gpointer value;
	GstBus *bus;

	priv = REJILLA_SONG_CONTROL_PRIVATE (object);

	/* Pipeline */
	priv->pipe = gst_element_factory_make ("playbin2", NULL);
	if (priv->pipe) {
		GstElement *audio_sink;

		audio_sink = gst_element_factory_make ("mateconfaudiosink", NULL);
		if (audio_sink)
			g_object_set (G_OBJECT (priv->pipe),
				      "audio-sink", audio_sink,
				      NULL);
	}
	else
		g_warning ("Pipe creation error : can't create pipe.\n");

	bus = gst_pipeline_get_bus (GST_PIPELINE (priv->pipe));
	gst_bus_add_watch (bus,
			   (GstBusFunc) rejilla_song_control_bus_messages,
			   object);
	gst_object_unref (bus);

	/* Widget itself */
	vbox = gtk_vbox_new (FALSE, 0);
	gtk_widget_show (vbox);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 0);
	gtk_container_add (GTK_CONTAINER (object), vbox);

	/* first line title */
	hbox = gtk_hbox_new (FALSE, 12);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox),
			    hbox,
			    FALSE,
			    FALSE,
			    0);

	priv->header = gtk_label_new (_("No file"));
	gtk_widget_show (priv->header);
	gtk_label_set_use_markup (GTK_LABEL (priv->header), TRUE);
	gtk_label_set_justify (GTK_LABEL (priv->header), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment (GTK_MISC (priv->header), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (hbox),
			    priv->header,
			    TRUE,
			    TRUE,
			    0);

	priv->size = gtk_label_new (NULL);
	gtk_widget_show (priv->size);
	gtk_label_set_justify (GTK_LABEL (priv->size), GTK_JUSTIFY_LEFT);
	gtk_misc_set_alignment (GTK_MISC (priv->size), 0.0, 0.0);
	gtk_box_pack_end (GTK_BOX (hbox),
	                  priv->size,
	                  FALSE,
	                  FALSE,
	                  0);
	
	/* second line : play, progress, volume button */
	hbox = gtk_hbox_new (FALSE, 12);
	gtk_widget_show (hbox);
	gtk_box_pack_start (GTK_BOX (vbox),
			    hbox,
			    FALSE,
			    FALSE,
			    0);

	alignment = gtk_alignment_new (0.0, 0.0, 0.0, 0.0);
	gtk_widget_show (alignment);

	priv->button = gtk_toggle_button_new ();
	gtk_widget_show (priv->button);
	gtk_widget_set_tooltip_text (priv->button, _("Start and stop playing"));
	gtk_container_add (GTK_CONTAINER (alignment), priv->button);
	gtk_box_pack_start (GTK_BOX (hbox),
			    alignment,
			    FALSE,
			    FALSE,
			    0);

	image = gtk_image_new_from_stock (GTK_STOCK_MEDIA_PLAY, GTK_ICON_SIZE_BUTTON);
	gtk_widget_show (image);
	gtk_container_add (GTK_CONTAINER (priv->button), image);
	g_signal_connect (G_OBJECT (priv->button), "clicked",
			  G_CALLBACK (rejilla_song_control_button_clicked_cb),
			  object);

	priv->progress = gtk_hscale_new_with_range (0, 1, 500000000);
	gtk_widget_show (priv->progress);
	gtk_scale_set_digits (GTK_SCALE (priv->progress), 0);
	gtk_scale_set_draw_value (GTK_SCALE (priv->progress), FALSE);
	gtk_widget_set_size_request (priv->progress, 80, -1);
	gtk_range_set_update_policy (GTK_RANGE (priv->progress), GTK_UPDATE_CONTINUOUS);
	gtk_box_pack_start (GTK_BOX (hbox),
	                    priv->progress,
	                    TRUE,
	                    TRUE,
	                    0);

	g_signal_connect (G_OBJECT (priv->progress),
			  "button-press-event",
			  G_CALLBACK (rejilla_song_control_range_button_pressed_cb), object);
	g_signal_connect (G_OBJECT (priv->progress),
			  "button-release-event",
			  G_CALLBACK (rejilla_song_control_range_button_released_cb), object);
	g_signal_connect (G_OBJECT (priv->progress),
			  "value-changed",
			  G_CALLBACK (rejilla_song_control_range_value_changed),
			  object);

	/* Set saved volume */
	rejilla_setting_get_value (rejilla_setting_get_default (),
	                           REJILLA_SETTING_PLAYER_VOLUME,
	                           &value);
	volume_value = GPOINTER_TO_INT (value);
	volume_value = CLAMP (volume_value, 0, 100);
	g_object_set (priv->pipe,
		      "volume", (gdouble) volume_value / 100.0,
		      NULL);

	volume = gtk_volume_button_new ();
	gtk_widget_show (volume);
	gtk_box_pack_start (GTK_BOX (hbox),
			    volume,
			    FALSE,
			    FALSE,
			    0);

	gtk_scale_button_set_value (GTK_SCALE_BUTTON (volume), (gdouble) volume_value / 100.0);
	g_signal_connect (volume,
			  "value-changed",
			  G_CALLBACK (rejilla_song_control_volume_changed_cb),
			  object);

	gtk_alignment_set_padding (GTK_ALIGNMENT (object), 12, 0, 0, 0);
}

static void
rejilla_song_control_finalize (GObject *object)
{
	G_OBJECT_CLASS (rejilla_song_control_parent_class)->finalize (object);
}

static void
rejilla_song_control_class_init (RejillaSongControlClass *klass)
{
	GObjectClass* object_class = G_OBJECT_CLASS (klass);
	GtkObjectClass* gtk_object_class = GTK_OBJECT_CLASS (klass);

	g_type_class_add_private (klass, sizeof (RejillaSongControlPrivate));

	object_class->finalize = rejilla_song_control_finalize;
	gtk_object_class->destroy = rejilla_song_control_destroy;
}

GtkWidget *
rejilla_song_control_new (void)
{
	return g_object_new (REJILLA_TYPE_SONG_CONTROL, NULL);
}
