/***************************************************************************
 *            audio-disc.h
 *
 *  dim nov 27 15:34:32 2005
 *  Copyright  2005  Rouquier Philippe
 *  rejilla-app@wanadoo.fr
 ***************************************************************************/

/*
 *  Rejilla is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Rejilla is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#ifndef AUDIO_DISC_H
#define AUDIO_DISC_H

#include <glib.h>
#include <glib-object.h>

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define REJILLA_TYPE_AUDIO_DISC         (rejilla_audio_disc_get_type ())
#define REJILLA_AUDIO_DISC(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), REJILLA_TYPE_AUDIO_DISC, RejillaAudioDisc))
#define REJILLA_AUDIO_DISC_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), REJILLA_TYPE_AUDIO_DISC, RejillaAudioDiscClass))
#define REJILLA_IS_AUDIO_DISC(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), REJILLA_TYPE_AUDIO_DISC))
#define REJILLA_IS_AUDIO_DISC_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), REJILLA_TYPE_AUDIO_DISC))
#define REJILLA_AUDIO_DISC_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), REJILLA_TYPE_AUDIO_DISC, RejillaAudioDiscClass))

typedef struct _RejillaAudioDiscPrivate RejillaAudioDiscPrivate;

typedef struct {
	GtkVBox parent;
	RejillaAudioDiscPrivate *priv;
} RejillaAudioDisc;

typedef struct {
	GtkVBoxClass parent_class;
} RejillaAudioDiscClass;

GType rejilla_audio_disc_get_type (void);
GtkWidget *rejilla_audio_disc_new (void);

G_END_DECLS

G_END_DECLS

#endif /* AUDIO_DISC_H */
