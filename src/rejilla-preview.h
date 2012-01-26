/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * rejilla
 * Copyright (C) Philippe Rouquier 2007-2008 <bonfire-app@wanadoo.fr>
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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#ifdef BUILD_PREVIEW

#ifndef _REJILLA_PREVIEW_H_
#define _REJILLA_PREVIEW_H_

#include <glib-object.h>
#include <gtk/gtk.h>

#include "rejilla-uri-container.h"

G_BEGIN_DECLS

#define REJILLA_TYPE_PREVIEW             (rejilla_preview_get_type ())
#define REJILLA_PREVIEW(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_PREVIEW, RejillaPreview))
#define REJILLA_PREVIEW_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_PREVIEW, RejillaPreviewClass))
#define REJILLA_IS_PREVIEW(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_PREVIEW))
#define REJILLA_IS_PREVIEW_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_PREVIEW))
#define REJILLA_PREVIEW_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_PREVIEW, RejillaPreviewClass))

typedef struct _RejillaPreviewClass RejillaPreviewClass;
typedef struct _RejillaPreview RejillaPreview;

struct _RejillaPreviewClass
{
	GtkAlignmentClass parent_class;
};

struct _RejillaPreview
{
	GtkAlignment parent_instance;
};

GType rejilla_preview_get_type (void) G_GNUC_CONST;
GtkWidget *rejilla_preview_new (void);

void
rejilla_preview_add_source (RejillaPreview *preview,
			    RejillaURIContainer *source);

void
rejilla_preview_hide (RejillaPreview *preview);

void
rejilla_preview_set_enabled (RejillaPreview *self,
			     gboolean preview);

G_END_DECLS

#endif				/* PLAYER_H */

#endif /* _REJILLA_PREVIEW_H_ */
