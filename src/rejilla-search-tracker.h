/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * rejilla
 * Copyright (C) Rouquier Philippe 2009 <bonfire-app@wanadoo.fr>
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

#ifndef _REJILLA_SEARCH_TRACKER_H_
#define _REJILLA_SEARCH_TRACKER_H_

#include <glib-object.h>

G_BEGIN_DECLS

#define REJILLA_TYPE_SEARCH_TRACKER             (rejilla_search_tracker_get_type ())
#define REJILLA_SEARCH_TRACKER(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_SEARCH_TRACKER, RejillaSearchTracker))
#define REJILLA_SEARCH_TRACKER_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_SEARCH_TRACKER, RejillaSearchTrackerClass))
#define REJILLA_IS_SEARCH_TRACKER(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_SEARCH_TRACKER))
#define REJILLA_IS_SEARCH_TRACKER_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_SEARCH_TRACKER))
#define REJILLA_SEARCH_TRACKER_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_SEARCH_TRACKER, RejillaSearchTrackerClass))

typedef struct _RejillaSearchTrackerClass RejillaSearchTrackerClass;
typedef struct _RejillaSearchTracker RejillaSearchTracker;

struct _RejillaSearchTrackerClass
{
	GObjectClass parent_class;
};

struct _RejillaSearchTracker
{
	GObject parent_instance;
};

GType rejilla_search_tracker_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* _REJILLA_SEARCH_TRACKER_H_ */
