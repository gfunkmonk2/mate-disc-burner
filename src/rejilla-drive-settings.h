/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Rejilla
 * Copyright (C) Philippe Rouquier 2005-2010 <bonfire-app@wanadoo.fr>
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

#ifndef _REJILLA_DRIVE_SETTINGS_H_
#define _REJILLA_DRIVE_SETTINGS_H_

#include <glib-object.h>

#include "rejilla-session.h"

G_BEGIN_DECLS

#define REJILLA_TYPE_DRIVE_SETTINGS             (rejilla_drive_settings_get_type ())
#define REJILLA_DRIVE_SETTINGS(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_DRIVE_SETTINGS, RejillaDriveSettings))
#define REJILLA_DRIVE_SETTINGS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_DRIVE_SETTINGS, RejillaDriveSettingsClass))
#define REJILLA_IS_DRIVE_SETTINGS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_DRIVE_SETTINGS))
#define REJILLA_IS_DRIVE_SETTINGS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_DRIVE_SETTINGS))
#define REJILLA_DRIVE_SETTINGS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_DRIVE_SETTINGS, RejillaDriveSettingsClass))

typedef struct _RejillaDriveSettingsClass RejillaDriveSettingsClass;
typedef struct _RejillaDriveSettings RejillaDriveSettings;

struct _RejillaDriveSettingsClass
{
	GObjectClass parent_class;
};

struct _RejillaDriveSettings
{
	GObject parent_instance;
};

GType rejilla_drive_settings_get_type (void) G_GNUC_CONST;

RejillaDriveSettings *
rejilla_drive_settings_new (void);

void
rejilla_drive_settings_set_session (RejillaDriveSettings *self,
                                    RejillaBurnSession *session);

G_END_DECLS

#endif /* _REJILLA_DRIVE_SETTINGS_H_ */
