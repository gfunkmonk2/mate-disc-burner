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

#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gi18n-lib.h>

#include <gtk/gtk.h>

#include "rejilla-jacket-edit.h"
#include "rejilla-jacket-view.h"

#include "rejilla-units.h"

#include "rejilla-tags.h"
#include "rejilla-track.h"
#include "rejilla-session.h"
#include "rejilla-track-stream.h"

#include "rejilla-cover.h"

/**
 *
 */

#define REJILLA_JACKET_EDIT_INSERT_TAGGED_TEXT(buffer_MACRO, text_MACRO, tag_MACRO, start_MACRO)	\
	gtk_text_buffer_insert_with_tags_by_name (buffer_MACRO, start_MACRO, text_MACRO, -1, tag_MACRO, NULL);

static void
rejilla_jacket_edit_set_audio_tracks_back (RejillaJacketView *back,
					   const gchar *label,
					   GSList *tracks)
{
	GtkTextBuffer *buffer;
	GtkTextIter start;
	GSList *iter;

	/* create the tags for back cover */
	buffer = rejilla_jacket_view_get_body_buffer (back);

	gtk_text_buffer_create_tag (buffer,
				    "Title",
				    "justification", GTK_JUSTIFY_CENTER,
				    "weight", PANGO_WEIGHT_BOLD,
				    "size", 12 * PANGO_SCALE,
				    NULL);
	gtk_text_buffer_create_tag (buffer,
				    "Subtitle",
				    "justification", GTK_JUSTIFY_LEFT,
				    "weight", PANGO_WEIGHT_NORMAL,
				    "size", 10 * PANGO_SCALE,
				    NULL);
	gtk_text_buffer_create_tag (buffer,
				    "Artist",
				    "weight", PANGO_WEIGHT_NORMAL,
				    "justification", GTK_JUSTIFY_LEFT,
				    "size", 8 * PANGO_SCALE,
				    "style", PANGO_STYLE_ITALIC,
				    NULL);

	gtk_text_buffer_get_start_iter (buffer, &start);

	if (label) {
		REJILLA_JACKET_EDIT_INSERT_TAGGED_TEXT (buffer, label, "Title", &start);
		REJILLA_JACKET_EDIT_INSERT_TAGGED_TEXT (buffer, "\n\n", "Title", &start);
	}

	for (iter = tracks; iter; iter = iter->next) {
		gchar *num;
		gchar *time;
		const gchar *info;
		RejillaTrack *track;

		track = iter->data;
		if (!REJILLA_IS_TRACK_STREAM (track))
			continue;

		num = g_strdup_printf ("%02d - ", g_slist_index (tracks, track) + 1);
		REJILLA_JACKET_EDIT_INSERT_TAGGED_TEXT (buffer, num, "Subtitle", &start);
		g_free (num);

		info = rejilla_track_tag_lookup_string (REJILLA_TRACK (track),
							REJILLA_TRACK_STREAM_TITLE_TAG);
		if (info) {
			REJILLA_JACKET_EDIT_INSERT_TAGGED_TEXT (buffer, info, "Subtitle", &start);
		}
		else {
			REJILLA_JACKET_EDIT_INSERT_TAGGED_TEXT (buffer, _("Unknown song"), "Subtitle", &start);
		}

		REJILLA_JACKET_EDIT_INSERT_TAGGED_TEXT (buffer, "\t\t", "Subtitle", &start);

		time = rejilla_units_get_time_string (rejilla_track_stream_get_end (REJILLA_TRACK_STREAM (track)) -
						      rejilla_track_stream_get_start (REJILLA_TRACK_STREAM (track)),
						      FALSE,
						      FALSE);
		REJILLA_JACKET_EDIT_INSERT_TAGGED_TEXT (buffer, time, "Subtitle", &start);
		g_free (time);

		REJILLA_JACKET_EDIT_INSERT_TAGGED_TEXT (buffer, "\n", "Subtitle", &start);

		info = rejilla_track_tag_lookup_string (REJILLA_TRACK (track),
							REJILLA_TRACK_STREAM_ARTIST_TAG);
		if (info) {
			gchar *string;

			/* Reminder: if this string happens to be used
			 * somewhere else in rejilla we'll need a
			 * context with C_() macro */
			/* Translators: %s is the name of the artist.
			 * This text is the one written on the cover of a disc.
			 * Before it there is the name of the song.
			 * I had to break it because it is in a GtkTextBuffer
			 * and every word has a different tag. */
			string = g_strdup_printf (_("by %s"), info);
			REJILLA_JACKET_EDIT_INSERT_TAGGED_TEXT (buffer, string, "Artist", &start);
			REJILLA_JACKET_EDIT_INSERT_TAGGED_TEXT (buffer, " ", "Artist", &start);
			g_free (string);
		}

		info = rejilla_track_tag_lookup_string (REJILLA_TRACK (track),
							REJILLA_TRACK_STREAM_COMPOSER_TAG);

		if (info)
			REJILLA_JACKET_EDIT_INSERT_TAGGED_TEXT (buffer, info, "Subtitle", &start);

		REJILLA_JACKET_EDIT_INSERT_TAGGED_TEXT (buffer, "\n\n", "Subtitle", &start);
	}

	/* side */
	buffer = rejilla_jacket_view_get_side_buffer (back);

	/* create the tags for sides */
	gtk_text_buffer_create_tag (buffer,
				    "Title",
				    "justification", GTK_JUSTIFY_CENTER,
				    "weight", PANGO_WEIGHT_BOLD,
				    "size", 10 * PANGO_SCALE,
				    NULL);

	gtk_text_buffer_get_start_iter (buffer, &start);

	if (label)
		REJILLA_JACKET_EDIT_INSERT_TAGGED_TEXT (buffer, label, "Title", &start);
}

static void
rejilla_jacket_edit_set_audio_tracks_front (RejillaJacketView *front,
					    const gchar *cover,
					    const gchar *label)
{
	/* set background for front cover */
	if (cover) {
		gchar *path;

		path = g_filename_from_uri (cover, NULL, NULL);
		if (!path)
			path = g_strdup (cover);

		rejilla_jacket_view_set_image_style (front, REJILLA_JACKET_IMAGE_STRETCH);
		rejilla_jacket_view_set_image (front, path);
		g_free (path);
	}

	/* create the tags for front cover */
	if (label) {
		GtkTextBuffer *buffer;
		GtkTextIter start;

		buffer = rejilla_jacket_view_get_body_buffer (front);
		gtk_text_buffer_create_tag (buffer,
					    "Title",
					    "justification", GTK_JUSTIFY_CENTER,
					    "weight", PANGO_WEIGHT_BOLD,
					    "size", 14 * PANGO_SCALE,
					    NULL);

		gtk_text_buffer_get_start_iter (buffer, &start);

		REJILLA_JACKET_EDIT_INSERT_TAGGED_TEXT (buffer, "\n\n\n\n", "Title", &start);
		REJILLA_JACKET_EDIT_INSERT_TAGGED_TEXT (buffer, label, "Title", &start);
	}
}

GtkWidget *
rejilla_session_edit_cover (RejillaBurnSession *session,
			    GtkWidget *toplevel)
{
	RejillaJacketEdit *contents;
	RejillaTrackType *type;
	GValue *cover_value;
	const gchar *title;
	GtkWidget *edit;
	GSList *tracks;

	edit = rejilla_jacket_edit_dialog_new (GTK_WIDGET (toplevel), &contents);

	/* Don't go any further if it's not video */
	type = rejilla_track_type_new ();
	rejilla_burn_session_get_input_type (session, type);
	if (!rejilla_track_type_get_has_stream (type)) {
		rejilla_track_type_free (type);
		return edit;
	}

	rejilla_track_type_free (type);

	title = rejilla_burn_session_get_label (session);
	tracks = rejilla_burn_session_get_tracks (session);

	cover_value = NULL;
	rejilla_burn_session_tag_lookup (session,
					 REJILLA_COVER_URI,
					 &cover_value);

	rejilla_jacket_edit_freeze (contents);
	rejilla_jacket_edit_set_audio_tracks_front (rejilla_jacket_edit_get_front (contents),
						    cover_value? g_value_get_string (cover_value):NULL,
						    title);
	rejilla_jacket_edit_set_audio_tracks_back (rejilla_jacket_edit_get_back (contents), title, tracks);
	rejilla_jacket_edit_thaw (contents);

	return edit;
}
