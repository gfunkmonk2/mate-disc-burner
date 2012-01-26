/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Librejilla-burn
 * Copyright (C) Luis Medinas 2008 <lmedinas@gmail.com>
 * Copyright (C) Philippe Rouquier 2008 <bonfire-app@wanadoo.fr>
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

#ifndef _REJILLA_PK_H_
#define _REJILLA_PK_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define REJILLA_TYPE_PK             (rejilla_pk_get_type ())
#define REJILLA_PK(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_PK, RejillaPK))
#define REJILLA_PK_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_PK, RejillaPKClass))
#define REJILLA_IS_PK(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_PK))
#define REJILLA_IS_PK_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_PK))
#define REJILLA_PK_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_PK, RejillaPKClass))

typedef struct _RejillaPKClass RejillaPKClass;
typedef struct _RejillaPK RejillaPK;

struct _RejillaPKClass
{
	GObjectClass parent_class;
};

struct _RejillaPK
{
	GObject parent_instance;
};

GType rejilla_pk_get_type (void) G_GNUC_CONST;

RejillaPK *rejilla_pk_new (void);

gboolean
rejilla_pk_install_gstreamer_plugin (RejillaPK *package,
                                     const gchar *element_name,
                                     int xid,
                                     GCancellable *cancel);
gboolean
rejilla_pk_install_missing_app (RejillaPK *package,
                                const gchar *file_name,
                                int xid,
                                GCancellable *cancel);
gboolean
rejilla_pk_install_missing_library (RejillaPK *package,
                                    const gchar *library_name,
                                    int xid,
                                    GCancellable *cancel);

G_END_DECLS

#endif /* _REJILLA_PK_H_ */
