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

#ifndef _REJILLA_SESSION_SPAN_H_
#define _REJILLA_SESSION_SPAN_H_

#include <glib-object.h>

#include <rejilla-session.h>

G_BEGIN_DECLS

#define REJILLA_TYPE_SESSION_SPAN             (rejilla_session_span_get_type ())
#define REJILLA_SESSION_SPAN(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_SESSION_SPAN, RejillaSessionSpan))
#define REJILLA_SESSION_SPAN_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_SESSION_SPAN, RejillaSessionSpanClass))
#define REJILLA_IS_SESSION_SPAN(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_SESSION_SPAN))
#define REJILLA_IS_SESSION_SPAN_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_SESSION_SPAN))
#define REJILLA_SESSION_SPAN_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_SESSION_SPAN, RejillaSessionSpanClass))

typedef struct _RejillaSessionSpanClass RejillaSessionSpanClass;
typedef struct _RejillaSessionSpan RejillaSessionSpan;

struct _RejillaSessionSpanClass
{
	RejillaBurnSessionClass parent_class;
};

struct _RejillaSessionSpan
{
	RejillaBurnSession parent_instance;
};

GType rejilla_session_span_get_type (void) G_GNUC_CONST;

RejillaSessionSpan *
rejilla_session_span_new (void);

RejillaBurnResult
rejilla_session_span_again (RejillaSessionSpan *session);

RejillaBurnResult
rejilla_session_span_possible (RejillaSessionSpan *session);

RejillaBurnResult
rejilla_session_span_start (RejillaSessionSpan *session);

RejillaBurnResult
rejilla_session_span_next (RejillaSessionSpan *session);

goffset
rejilla_session_span_get_max_space (RejillaSessionSpan *session);

void
rejilla_session_span_stop (RejillaSessionSpan *session);

G_END_DECLS

#endif /* _REJILLA_SESSION_SPAN_H_ */
