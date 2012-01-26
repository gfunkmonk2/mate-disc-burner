/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2005 William Jon McCann <mccann@jhu.edu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * Authors: William Jon McCann <mccann@jhu.edu>
 *
 */

#ifndef __CAJA_BURN_BAR_H
#define __CAJA_BURN_BAR_H

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define CAJA_TYPE_DISC_BURN_BAR         (caja_disc_burn_bar_get_type ())
#define CAJA_DISC_BURN_BAR(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), CAJA_TYPE_DISC_BURN_BAR, CajaDiscBurnBar))
#define CAJA_DISC_BURN_BAR_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), CAJA_TYPE_DISC_BURN_BAR, CajaDiscBurnBarClass))
#define CAJA_IS_DISC_BURN_BAR(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), CAJA_TYPE_DISC_BURN_BAR))
#define CAJA_IS_DISC_BURN_BAR_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), CAJA_TYPE_DISC_BURN_BAR))
#define CAJA_DISC_BURN_BAR_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), CAJA_TYPE_DISC_BURN_BAR, CajaDiscBurnBarClass))

typedef struct CajaDiscBurnBarPrivate CajaDiscBurnBarPrivate;

typedef struct
{
        GtkHBox                 box;

        CajaDiscBurnBarPrivate *priv;
} CajaDiscBurnBar;

typedef struct
{
        GtkHBoxClass            parent_class;

	void (* activate) (CajaDiscBurnBar *bar);

} CajaDiscBurnBarClass;

GType       caja_disc_burn_bar_get_type          (void);
GtkWidget  *caja_disc_burn_bar_new               (void);

GtkWidget  *caja_disc_burn_bar_get_button        (CajaDiscBurnBar *bar);

G_END_DECLS

#endif /* __GS_BURN_BAR_H */
