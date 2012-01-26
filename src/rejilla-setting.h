/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * rejilla
 * Copyright (C) Philippe Rouquier 2009 <bonfire-app@wanadoo.fr>
 * 
 * rejilla is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * rejilla is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _REJILLA_SETTING_H_
#define _REJILLA_SETTING_H_

#include <glib-object.h>

G_BEGIN_DECLS

typedef enum {
	REJILLA_SETTING_VALUE_NONE,

	/** gint value **/
	REJILLA_SETTING_WIN_WIDTH,
	REJILLA_SETTING_WIN_HEIGHT,
	REJILLA_SETTING_STOCK_FILE_CHOOSER_PERCENT,
	REJILLA_SETTING_REJILLA_FILE_CHOOSER_PERCENT,
	REJILLA_SETTING_PLAYER_VOLUME,
	REJILLA_SETTING_DISPLAY_PROPORTION,
	REJILLA_SETTING_DISPLAY_LAYOUT,
	REJILLA_SETTING_DATA_DISC_COLUMN,
	REJILLA_SETTING_DATA_DISC_COLUMN_ORDER,
	REJILLA_SETTING_IMAGE_SIZE_WIDTH,
	REJILLA_SETTING_IMAGE_SIZE_HEIGHT,
	REJILLA_SETTING_VIDEO_SIZE_HEIGHT,
	REJILLA_SETTING_VIDEO_SIZE_WIDTH,

	/** gboolean **/
	REJILLA_SETTING_WIN_MAXIMIZED,
	REJILLA_SETTING_SHOW_SIDEPANE,
	REJILLA_SETTING_SHOW_PREVIEW,

	/** gchar * **/
	REJILLA_SETTING_DISPLAY_LAYOUT_AUDIO,
	REJILLA_SETTING_DISPLAY_LAYOUT_DATA,
	REJILLA_SETTING_DISPLAY_LAYOUT_VIDEO,

	/** gchar ** **/
	REJILLA_SETTING_SEARCH_ENTRY_HISTORY,

} RejillaSettingValue;

#define REJILLA_TYPE_SETTING             (rejilla_setting_get_type ())
#define REJILLA_SETTING(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), REJILLA_TYPE_SETTING, RejillaSetting))
#define REJILLA_SETTING_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass), REJILLA_TYPE_SETTING, RejillaSettingClass))
#define REJILLA_IS_SETTING(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), REJILLA_TYPE_SETTING))
#define REJILLA_IS_SETTING_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass), REJILLA_TYPE_SETTING))
#define REJILLA_SETTING_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj), REJILLA_TYPE_SETTING, RejillaSettingClass))

typedef struct _RejillaSettingClass RejillaSettingClass;
typedef struct _RejillaSetting RejillaSetting;

struct _RejillaSettingClass
{
	GObjectClass parent_class;

	/* Signals */
	void(* value_changed) (RejillaSetting *self, gint value);
};

struct _RejillaSetting
{
	GObject parent_instance;
};

GType rejilla_setting_get_type (void) G_GNUC_CONST;

RejillaSetting *
rejilla_setting_get_default (void);

gboolean
rejilla_setting_get_value (RejillaSetting *setting,
                           RejillaSettingValue setting_value,
                           gpointer *value);

gboolean
rejilla_setting_set_value (RejillaSetting *setting,
                           RejillaSettingValue setting_value,
                           gconstpointer value);

gboolean
rejilla_setting_load (RejillaSetting *setting);

gboolean
rejilla_setting_save (RejillaSetting *setting);

G_END_DECLS

#endif /* _REJILLA_SETTING_H_ */
