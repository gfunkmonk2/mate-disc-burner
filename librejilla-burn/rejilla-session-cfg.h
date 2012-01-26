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

#ifndef _REJILLA_SESSION_CFG_H_
#define _REJILLA_SESSION_CFG_H_

#include <glib-object.h>

#include <rejilla-session.h>
#include <rejilla-session-span.h>

G_BEGIN_DECLS

#define REJILLA_TYPE_SESSION_CFG             (rejilla_session_cfg_get_type ())
#define REJILLA_SESSION_CFG(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_SESSION_CFG, RejillaSessionCfg))
#define REJILLA_SESSION_CFG_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_SESSION_CFG, RejillaSessionCfgClass))
#define REJILLA_IS_SESSION_CFG(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_SESSION_CFG))
#define REJILLA_IS_SESSION_CFG_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_SESSION_CFG))
#define REJILLA_SESSION_CFG_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_SESSION_CFG, RejillaSessionCfgClass))

typedef struct _RejillaSessionCfgClass RejillaSessionCfgClass;
typedef struct _RejillaSessionCfg RejillaSessionCfg;

struct _RejillaSessionCfgClass
{
	RejillaSessionSpanClass parent_class;
};

struct _RejillaSessionCfg
{
	RejillaSessionSpan parent_instance;
};

GType rejilla_session_cfg_get_type (void) G_GNUC_CONST;

/**
 * This is for the signal sent to tell whether or not session is valid
 */

typedef enum {
	REJILLA_SESSION_VALID				= 0,
	REJILLA_SESSION_NO_CD_TEXT			= 1,
	REJILLA_SESSION_NOT_READY,
	REJILLA_SESSION_EMPTY,
	REJILLA_SESSION_NO_INPUT_IMAGE,
	REJILLA_SESSION_UNKNOWN_IMAGE,
	REJILLA_SESSION_NO_INPUT_MEDIUM,
	REJILLA_SESSION_NO_OUTPUT,
	REJILLA_SESSION_INSUFFICIENT_SPACE,
	REJILLA_SESSION_OVERBURN_NECESSARY,
	REJILLA_SESSION_NOT_SUPPORTED,
	REJILLA_SESSION_DISC_PROTECTED
} RejillaSessionError;

#define REJILLA_SESSION_IS_VALID(result_MACRO)					\
	((result_MACRO) == REJILLA_SESSION_VALID ||				\
	 (result_MACRO) == REJILLA_SESSION_NO_CD_TEXT)

RejillaSessionCfg *
rejilla_session_cfg_new (void);

RejillaSessionError
rejilla_session_cfg_get_error (RejillaSessionCfg *cfg);

void
rejilla_session_cfg_add_flags (RejillaSessionCfg *cfg,
			       RejillaBurnFlag flags);
void
rejilla_session_cfg_remove_flags (RejillaSessionCfg *cfg,
				  RejillaBurnFlag flags);
gboolean
rejilla_session_cfg_is_supported (RejillaSessionCfg *cfg,
				  RejillaBurnFlag flag);
gboolean
rejilla_session_cfg_is_compulsory (RejillaSessionCfg *cfg,
				   RejillaBurnFlag flag);

gboolean
rejilla_session_cfg_has_default_output_path (RejillaSessionCfg *cfg);

void
rejilla_session_cfg_enable (RejillaSessionCfg *cfg);

void
rejilla_session_cfg_disable (RejillaSessionCfg *cfg);

G_END_DECLS

#endif /* _REJILLA_SESSION_CFG_H_ */
