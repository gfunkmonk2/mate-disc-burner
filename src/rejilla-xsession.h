/*
 * Rejilla is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * Rejilla is distributed in the hope that it will be useful,
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
 
/***************************************************************************
 *            rejilla-session.h
 *
 *  Thu May 18 22:25:47 2006
 *  Copyright  2006  Philippe Rouquier
 *  <rejilla-app@wanadoo.fr>
 ****************************************************************************/

 
#ifndef _REJILLA_SESSION_H
#define _REJILLA_SESSION_H

#include <glib.h>

#include "rejilla-app.h"

G_BEGIN_DECLS

gboolean
rejilla_session_connect (RejillaApp *app);
void
rejilla_session_disconnect (RejillaApp *app);

G_END_DECLS

#endif /* _REJILLA-SESSION_H */
