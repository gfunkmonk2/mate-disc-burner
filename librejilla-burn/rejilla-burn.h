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

#ifndef BURN_H
#define BURN_H

#include <glib.h>
#include <glib-object.h>

#include <rejilla-error.h>
#include <rejilla-track.h>
#include <rejilla-session.h>

#include <rejilla-medium.h>

G_BEGIN_DECLS

#define REJILLA_TYPE_BURN         (rejilla_burn_get_type ())
#define REJILLA_BURN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), REJILLA_TYPE_BURN, RejillaBurn))
#define REJILLA_BURN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), REJILLA_TYPE_BURN, RejillaBurnClass))
#define REJILLA_IS_BURN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), REJILLA_TYPE_BURN))
#define REJILLA_IS_BURN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), REJILLA_TYPE_BURN))
#define REJILLA_BURN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), REJILLA_TYPE_BURN, RejillaBurnClass))

typedef struct {
	GObject parent;
} RejillaBurn;

typedef struct {
	GObjectClass parent_class;

	/* signals */
	RejillaBurnResult		(*insert_media_request)		(RejillaBurn *obj,
									 RejillaDrive *drive,
									 RejillaBurnError error,
									 RejillaMedia required_media);

	RejillaBurnResult		(*eject_failure)		(RejillaBurn *obj,
							                 RejillaDrive *drive);

	RejillaBurnResult		(*blank_failure)		(RejillaBurn *obj);

	RejillaBurnResult		(*location_request)		(RejillaBurn *obj,
									 GError *error,
									 gboolean is_temporary);

	RejillaBurnResult		(*ask_disable_joliet)		(RejillaBurn *obj);

	RejillaBurnResult		(*warn_data_loss)		(RejillaBurn *obj);
	RejillaBurnResult		(*warn_previous_session_loss)	(RejillaBurn *obj);
	RejillaBurnResult		(*warn_audio_to_appendable)	(RejillaBurn *obj);
	RejillaBurnResult		(*warn_rewritable)		(RejillaBurn *obj);

	RejillaBurnResult		(*dummy_success)		(RejillaBurn *obj);

	void				(*progress_changed)		(RejillaBurn *obj,
									 gdouble overall_progress,
									 gdouble action_progress,
									 glong time_remaining);
	void				(*action_changed)		(RejillaBurn *obj,
									 RejillaBurnAction action);

	RejillaBurnResult		(*install_missing)		(RejillaBurn *obj,
									 RejillaPluginErrorType error,
									 const gchar *detail);
} RejillaBurnClass;

GType rejilla_burn_get_type (void);
RejillaBurn *rejilla_burn_new (void);

RejillaBurnResult 
rejilla_burn_record (RejillaBurn *burn,
		     RejillaBurnSession *session,
		     GError **error);

RejillaBurnResult
rejilla_burn_check (RejillaBurn *burn,
		    RejillaBurnSession *session,
		    GError **error);

RejillaBurnResult
rejilla_burn_blank (RejillaBurn *burn,
		    RejillaBurnSession *session,
		    GError **error);

RejillaBurnResult
rejilla_burn_cancel (RejillaBurn *burn,
		     gboolean protect);

RejillaBurnResult
rejilla_burn_status (RejillaBurn *burn,
		     RejillaMedia *media,
		     goffset *isosize,
		     goffset *written,
		     guint64 *rate);

void
rejilla_burn_get_action_string (RejillaBurn *burn,
				RejillaBurnAction action,
				gchar **string);

G_END_DECLS

#endif /* BURN_H */
