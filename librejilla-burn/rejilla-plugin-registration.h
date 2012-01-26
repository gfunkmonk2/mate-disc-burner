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

#ifndef _BURN_PLUGIN_H_REGISTRATION_
#define _BURN_PLUGIN_H_REGISTRATION_

#include <glib.h>

#include "rejilla-medium.h"

#include "rejilla-enums.h"
#include "rejilla-track.h"
#include "rejilla-track-stream.h"
#include "rejilla-track-data.h"
#include "rejilla-plugin.h"

G_BEGIN_DECLS

#define REJILLA_PLUGIN_BURN_FLAG_MASK	(REJILLA_BURN_FLAG_DUMMY|		\
					 REJILLA_BURN_FLAG_MULTI|		\
					 REJILLA_BURN_FLAG_DAO|			\
					 REJILLA_BURN_FLAG_RAW|			\
					 REJILLA_BURN_FLAG_BURNPROOF|		\
					 REJILLA_BURN_FLAG_OVERBURN|		\
					 REJILLA_BURN_FLAG_NOGRACE|		\
					 REJILLA_BURN_FLAG_APPEND|		\
					 REJILLA_BURN_FLAG_MERGE)

#define REJILLA_PLUGIN_IMAGE_FLAG_MASK	(REJILLA_BURN_FLAG_APPEND|		\
					 REJILLA_BURN_FLAG_MERGE)

#define REJILLA_PLUGIN_BLANK_FLAG_MASK	(REJILLA_BURN_FLAG_NOGRACE|		\
					 REJILLA_BURN_FLAG_FAST_BLANK)

/**
 * These are the functions a plugin must implement
 */

GType rejilla_plugin_register_caps (RejillaPlugin *plugin, gchar **error);

void
rejilla_plugin_define (RejillaPlugin *plugin,
		       const gchar *name,
                       const gchar *display_name,
		       const gchar *description,
		       const gchar *author,
		       guint priority);
void
rejilla_plugin_set_compulsory (RejillaPlugin *self,
			       gboolean compulsory);

void
rejilla_plugin_register_group (RejillaPlugin *plugin,
			       const gchar *name);

typedef enum {
	REJILLA_PLUGIN_IO_NONE			= 0,
	REJILLA_PLUGIN_IO_ACCEPT_PIPE		= 1,
	REJILLA_PLUGIN_IO_ACCEPT_FILE		= 1 << 1,
} RejillaPluginIOFlag;

GSList *
rejilla_caps_image_new (RejillaPluginIOFlag flags,
			RejillaImageFormat format);

GSList *
rejilla_caps_audio_new (RejillaPluginIOFlag flags,
			RejillaStreamFormat format);

GSList *
rejilla_caps_data_new (RejillaImageFS fs_type);

GSList *
rejilla_caps_disc_new (RejillaMedia media);

GSList *
rejilla_caps_checksum_new (RejillaChecksumType checksum);

void
rejilla_plugin_link_caps (RejillaPlugin *plugin,
			  GSList *outputs,
			  GSList *inputs);

void
rejilla_plugin_blank_caps (RejillaPlugin *plugin,
			   GSList *caps);

/**
 * This function is important since not only does it set the flags but it also 
 * tells rejilla which types of media are supported. So even if a plugin doesn't
 * support any flag, use it to tell rejilla which media are supported.
 * That's only needed if the plugin supports burn/blank operations.
 */
void
rejilla_plugin_set_flags (RejillaPlugin *plugin,
			  RejillaMedia media,
			  RejillaBurnFlag supported,
			  RejillaBurnFlag compulsory);
void
rejilla_plugin_set_blank_flags (RejillaPlugin *self,
				RejillaMedia media,
				RejillaBurnFlag supported,
				RejillaBurnFlag compulsory);

void
rejilla_plugin_process_caps (RejillaPlugin *plugin,
			     GSList *caps);

void
rejilla_plugin_set_process_flags (RejillaPlugin *plugin,
				  RejillaPluginProcessFlag flags);

void
rejilla_plugin_check_caps (RejillaPlugin *plugin,
			   RejillaChecksumType type,
			   GSList *caps);

/**
 * Plugin configure options
 */

RejillaPluginConfOption *
rejilla_plugin_conf_option_new (const gchar *key,
				const gchar *description,
				RejillaPluginConfOptionType type);

RejillaBurnResult
rejilla_plugin_add_conf_option (RejillaPlugin *plugin,
				RejillaPluginConfOption *option);

RejillaBurnResult
rejilla_plugin_conf_option_bool_add_suboption (RejillaPluginConfOption *option,
					       RejillaPluginConfOption *suboption);

RejillaBurnResult
rejilla_plugin_conf_option_int_set_range (RejillaPluginConfOption *option,
					  gint min,
					  gint max);

RejillaBurnResult
rejilla_plugin_conf_option_choice_add (RejillaPluginConfOption *option,
				       const gchar *string,
				       gint value);

void
rejilla_plugin_add_error (RejillaPlugin *plugin,
                          RejillaPluginErrorType type,
                          const gchar *detail);

void
rejilla_plugin_test_gstreamer_plugin (RejillaPlugin *plugin,
                                      const gchar *name);

void
rejilla_plugin_test_app (RejillaPlugin *plugin,
                         const gchar *name,
                         const gchar *version_arg,
                         const gchar *version_format,
                         gint version [3]);

/**
 * Boiler plate for plugin definition to save the hassle of definition.
 * To be put at the beginning of the .c file.
 */
typedef GType	(* RejillaPluginRegisterType)	(RejillaPlugin *plugin);

G_MODULE_EXPORT void
rejilla_plugin_check_config (RejillaPlugin *plugin);

#define REJILLA_PLUGIN_BOILERPLATE(PluginName, plugin_name, PARENT_NAME, ParentName) \
typedef struct {								\
	ParentName parent;							\
} PluginName;									\
										\
typedef struct {								\
	ParentName##Class parent_class;						\
} PluginName##Class;								\
										\
static GType plugin_name##_type = 0;						\
										\
static GType									\
plugin_name##_get_type (void)							\
{										\
	return plugin_name##_type;						\
}										\
										\
static void plugin_name##_class_init (PluginName##Class *klass);		\
static void plugin_name##_init (PluginName *sp);				\
static void plugin_name##_finalize (GObject *object);			\
static void plugin_name##_export_caps (RejillaPlugin *plugin);	\
G_MODULE_EXPORT GType								\
rejilla_plugin_register (RejillaPlugin *plugin);				\
G_MODULE_EXPORT GType								\
rejilla_plugin_register (RejillaPlugin *plugin)				\
{														\
	if (rejilla_plugin_get_gtype (plugin) == G_TYPE_NONE)	\
		plugin_name##_export_caps (plugin);					\
	static const GTypeInfo our_info = {					\
		sizeof (PluginName##Class),					\
		NULL,										\
		NULL,										\
		(GClassInitFunc)plugin_name##_class_init,			\
		NULL,										\
		NULL,										\
		sizeof (PluginName),							\
		0,											\
		(GInstanceInitFunc)plugin_name##_init,			\
	};												\
	plugin_name##_type = g_type_module_register_type (G_TYPE_MODULE (plugin),		\
							  PARENT_NAME,			\
							  G_STRINGIFY (PluginName),		\
							  &our_info,				\
							  0);						\
	return plugin_name##_type;						\
}

#define REJILLA_PLUGIN_ADD_STANDARD_CDR_FLAGS(plugin_MACRO, unsupported_MACRO)	\
	/* Use DAO for first session since AUDIO need it to write CD-TEXT */	\
	rejilla_plugin_set_flags (plugin_MACRO,					\
				  REJILLA_MEDIUM_CD|				\
				  REJILLA_MEDIUM_WRITABLE|			\
				  REJILLA_MEDIUM_BLANK,				\
				  (REJILLA_BURN_FLAG_DAO|			\
				  REJILLA_BURN_FLAG_MULTI|			\
				  REJILLA_BURN_FLAG_BURNPROOF|			\
				  REJILLA_BURN_FLAG_OVERBURN|			\
				  REJILLA_BURN_FLAG_DUMMY|			\
				  REJILLA_BURN_FLAG_NOGRACE) &			\
				  (~(unsupported_MACRO)),				\
				  REJILLA_BURN_FLAG_NONE);			\
	/* This is a CDR with data data can be merged or at least appended */	\
	rejilla_plugin_set_flags (plugin_MACRO,					\
				  REJILLA_MEDIUM_CD|				\
				  REJILLA_MEDIUM_WRITABLE|			\
				  REJILLA_MEDIUM_APPENDABLE|			\
				  REJILLA_MEDIUM_HAS_AUDIO|			\
				  REJILLA_MEDIUM_HAS_DATA,			\
				  (REJILLA_BURN_FLAG_APPEND|			\
				  REJILLA_BURN_FLAG_MERGE|			\
				  REJILLA_BURN_FLAG_BURNPROOF|			\
				  REJILLA_BURN_FLAG_OVERBURN|			\
				  REJILLA_BURN_FLAG_MULTI|			\
				  REJILLA_BURN_FLAG_DUMMY|			\
				  REJILLA_BURN_FLAG_NOGRACE) &			\
				  (~(unsupported_MACRO)),				\
				  REJILLA_BURN_FLAG_APPEND);

#define REJILLA_PLUGIN_ADD_STANDARD_CDRW_FLAGS(plugin_MACRO, unsupported_MACRO)			\
	/* Use DAO for first session since AUDIO needs it to write CD-TEXT */	\
	rejilla_plugin_set_flags (plugin_MACRO,					\
				  REJILLA_MEDIUM_CD|				\
				  REJILLA_MEDIUM_REWRITABLE|			\
				  REJILLA_MEDIUM_BLANK,				\
				  (REJILLA_BURN_FLAG_DAO|			\
				  REJILLA_BURN_FLAG_MULTI|			\
				  REJILLA_BURN_FLAG_BURNPROOF|			\
				  REJILLA_BURN_FLAG_OVERBURN|			\
				  REJILLA_BURN_FLAG_DUMMY|			\
				  REJILLA_BURN_FLAG_NOGRACE) &			\
				  (~(unsupported_MACRO)),				\
				  REJILLA_BURN_FLAG_NONE);			\
	/* It is a CDRW we want the CD to be either blanked before or appended	\
	 * that's why we set MERGE as compulsory. That way if the CD is not 	\
	 * MERGED we force the blank before writing to avoid appending sessions	\
	 * endlessly until there is no free space. */				\
	rejilla_plugin_set_flags (plugin_MACRO,					\
				  REJILLA_MEDIUM_CD|				\
				  REJILLA_MEDIUM_REWRITABLE|			\
				  REJILLA_MEDIUM_APPENDABLE|			\
				  REJILLA_MEDIUM_HAS_AUDIO|			\
				  REJILLA_MEDIUM_HAS_DATA,			\
				  (REJILLA_BURN_FLAG_APPEND|			\
				  REJILLA_BURN_FLAG_MERGE|			\
				  REJILLA_BURN_FLAG_BURNPROOF|			\
				  REJILLA_BURN_FLAG_OVERBURN|			\
				  REJILLA_BURN_FLAG_MULTI|			\
				  REJILLA_BURN_FLAG_DUMMY|			\
				  REJILLA_BURN_FLAG_NOGRACE) &			\
				  (~(unsupported_MACRO)),				\
				  REJILLA_BURN_FLAG_MERGE);

#define REJILLA_PLUGIN_ADD_STANDARD_DVDR_FLAGS(plugin_MACRO, unsupported_MACRO)			\
	/* DAO and MULTI are exclusive */					\
	rejilla_plugin_set_flags (plugin_MACRO,					\
				  REJILLA_MEDIUM_DVDR|				\
				  REJILLA_MEDIUM_DUAL_L|			\
				  REJILLA_MEDIUM_JUMP|				\
				  REJILLA_MEDIUM_BLANK,				\
				  (REJILLA_BURN_FLAG_DAO|			\
				  REJILLA_BURN_FLAG_BURNPROOF|			\
				  REJILLA_BURN_FLAG_DUMMY|			\
				  REJILLA_BURN_FLAG_NOGRACE) &			\
				  (~(unsupported_MACRO)),				\
				  REJILLA_BURN_FLAG_NONE);			\
	rejilla_plugin_set_flags (plugin_MACRO,					\
				  REJILLA_MEDIUM_DVDR|				\
				  REJILLA_MEDIUM_DUAL_L|			\
				  REJILLA_MEDIUM_JUMP|				\
				  REJILLA_MEDIUM_BLANK,				\
				  (REJILLA_BURN_FLAG_BURNPROOF|			\
				  REJILLA_BURN_FLAG_MULTI|			\
				  REJILLA_BURN_FLAG_DUMMY|			\
				  REJILLA_BURN_FLAG_NOGRACE) &			\
				  (~(unsupported_MACRO)),				\
				  REJILLA_BURN_FLAG_NONE);			\
	/* This is a DVDR with data; data can be merged or at least appended */	\
	rejilla_plugin_set_flags (plugin_MACRO,					\
				  REJILLA_MEDIUM_DVDR|				\
				  REJILLA_MEDIUM_DUAL_L|			\
				  REJILLA_MEDIUM_JUMP|				\
				  REJILLA_MEDIUM_APPENDABLE|			\
				  REJILLA_MEDIUM_HAS_DATA,			\
				  (REJILLA_BURN_FLAG_APPEND|			\
				  REJILLA_BURN_FLAG_MERGE|			\
				  REJILLA_BURN_FLAG_BURNPROOF|			\
				  REJILLA_BURN_FLAG_MULTI|			\
				  REJILLA_BURN_FLAG_DUMMY|			\
				  REJILLA_BURN_FLAG_NOGRACE) &			\
				  (~(unsupported_MACRO)),				\
				  REJILLA_BURN_FLAG_APPEND);

#define REJILLA_PLUGIN_ADD_STANDARD_DVDR_PLUS_FLAGS(plugin_MACRO, unsupported_MACRO)		\
	/* DVD+R don't have a DUMMY mode */					\
	rejilla_plugin_set_flags (plugin_MACRO,					\
				  REJILLA_MEDIUM_DVDR_PLUS|			\
				  REJILLA_MEDIUM_DUAL_L|			\
				  REJILLA_MEDIUM_BLANK,				\
				  (REJILLA_BURN_FLAG_DAO|			\
				  REJILLA_BURN_FLAG_BURNPROOF|			\
				  REJILLA_BURN_FLAG_NOGRACE) &			\
				  (~(unsupported_MACRO)),				\
				  REJILLA_BURN_FLAG_NONE);			\
	rejilla_plugin_set_flags (plugin_MACRO,					\
				  REJILLA_MEDIUM_DVDR_PLUS|			\
				  REJILLA_MEDIUM_DUAL_L|			\
				  REJILLA_MEDIUM_BLANK,				\
				  (REJILLA_BURN_FLAG_BURNPROOF|			\
				  REJILLA_BURN_FLAG_MULTI|			\
				  REJILLA_BURN_FLAG_NOGRACE) &			\
				  (~(unsupported_MACRO)),				\
				  REJILLA_BURN_FLAG_NONE);			\
	/* DVD+R with data: data can be merged or at least appended */		\
	rejilla_plugin_set_flags (plugin_MACRO,					\
				  REJILLA_MEDIUM_DVDR_PLUS|			\
				  REJILLA_MEDIUM_DUAL_L|			\
				  REJILLA_MEDIUM_APPENDABLE|			\
				  REJILLA_MEDIUM_HAS_DATA,			\
				  (REJILLA_BURN_FLAG_MERGE|			\
				  REJILLA_BURN_FLAG_APPEND|			\
				  REJILLA_BURN_FLAG_BURNPROOF|			\
				  REJILLA_BURN_FLAG_MULTI|			\
				  REJILLA_BURN_FLAG_NOGRACE) &			\
				  (~(unsupported_MACRO)),				\
				  REJILLA_BURN_FLAG_APPEND);

#define REJILLA_PLUGIN_ADD_STANDARD_DVDRW_FLAGS(plugin_MACRO, unsupported_MACRO)			\
	rejilla_plugin_set_flags (plugin_MACRO,					\
				  REJILLA_MEDIUM_DVDRW|				\
				  REJILLA_MEDIUM_UNFORMATTED|			\
				  REJILLA_MEDIUM_BLANK,				\
				  (REJILLA_BURN_FLAG_DAO|			\
				  REJILLA_BURN_FLAG_BURNPROOF|			\
				  REJILLA_BURN_FLAG_DUMMY|			\
				  REJILLA_BURN_FLAG_NOGRACE) &			\
				  (~(unsupported_MACRO)),				\
				  REJILLA_BURN_FLAG_NONE);			\
	rejilla_plugin_set_flags (plugin_MACRO,					\
				  REJILLA_MEDIUM_DVDRW|				\
				  REJILLA_MEDIUM_BLANK,				\
				  (REJILLA_BURN_FLAG_BURNPROOF|			\
				  REJILLA_BURN_FLAG_MULTI|			\
				  REJILLA_BURN_FLAG_DUMMY|			\
				  REJILLA_BURN_FLAG_NOGRACE) &			\
				  (~(unsupported_MACRO)),				\
				  REJILLA_BURN_FLAG_NONE);			\
	/* This is a DVDRW we want the DVD to be either blanked before or	\
	 * appended that's why we set MERGE as compulsory. That way if the DVD	\
	 * is not MERGED we force the blank before writing to avoid appending	\
	 * sessions endlessly until there is no free space. */			\
	rejilla_plugin_set_flags (plugin_MACRO,					\
				  REJILLA_MEDIUM_DVDRW|				\
				  REJILLA_MEDIUM_APPENDABLE|			\
				  REJILLA_MEDIUM_HAS_DATA,			\
				  (REJILLA_BURN_FLAG_MERGE|			\
				  REJILLA_BURN_FLAG_APPEND|			\
				  REJILLA_BURN_FLAG_BURNPROOF|			\
				  REJILLA_BURN_FLAG_MULTI|			\
				  REJILLA_BURN_FLAG_DUMMY|			\
				  REJILLA_BURN_FLAG_NOGRACE) &			\
				  (~(unsupported_MACRO)),				\
				  REJILLA_BURN_FLAG_MERGE);

/**
 * These kind of media don't support:
 * - BURNPROOF
 * - DAO
 * - APPEND
 * since they don't behave and are not written in the same way.
 * They also can't be closed so MULTI is compulsory.
 */
#define REJILLA_PLUGIN_ADD_STANDARD_DVDRW_PLUS_FLAGS(plugin_MACRO, unsupported_MACRO)		\
	rejilla_plugin_set_flags (plugin_MACRO,					\
				  REJILLA_MEDIUM_DVDRW_PLUS|			\
				  REJILLA_MEDIUM_DUAL_L|			\
				  REJILLA_MEDIUM_UNFORMATTED|			\
				  REJILLA_MEDIUM_BLANK,				\
				  (REJILLA_BURN_FLAG_MULTI|			\
				  REJILLA_BURN_FLAG_NOGRACE) &			\
				  (~(unsupported_MACRO)),				\
				  REJILLA_BURN_FLAG_MULTI);			\
	rejilla_plugin_set_flags (plugin_MACRO,					\
				  REJILLA_MEDIUM_DVDRW_PLUS|			\
				  REJILLA_MEDIUM_DUAL_L|			\
				  REJILLA_MEDIUM_APPENDABLE|			\
				  REJILLA_MEDIUM_CLOSED|			\
				  REJILLA_MEDIUM_HAS_DATA,			\
				  (REJILLA_BURN_FLAG_MULTI|			\
				  REJILLA_BURN_FLAG_NOGRACE|			\
				  REJILLA_BURN_FLAG_MERGE) &			\
				  (~(unsupported_MACRO)),				\
				  REJILLA_BURN_FLAG_MULTI);

/**
 * The above statement apply to these as well. There is no longer dummy mode
 * NOTE: there is no such thing as a DVD-RW DL.
 */
#define REJILLA_PLUGIN_ADD_STANDARD_DVDRW_RESTRICTED_FLAGS(plugin_MACRO, unsupported_MACRO)	\
	rejilla_plugin_set_flags (plugin_MACRO,					\
				  REJILLA_MEDIUM_DVD|				\
				  REJILLA_MEDIUM_RESTRICTED|			\
				  REJILLA_MEDIUM_REWRITABLE|			\
				  REJILLA_MEDIUM_UNFORMATTED|			\
				  REJILLA_MEDIUM_BLANK,				\
				  (REJILLA_BURN_FLAG_MULTI|			\
				  REJILLA_BURN_FLAG_NOGRACE) &			\
				  (~(unsupported_MACRO)),				\
				  REJILLA_BURN_FLAG_MULTI);			\
	rejilla_plugin_set_flags (plugin_MACRO,					\
				  REJILLA_MEDIUM_DVD|				\
				  REJILLA_MEDIUM_RESTRICTED|			\
				  REJILLA_MEDIUM_REWRITABLE|			\
				  REJILLA_MEDIUM_APPENDABLE|			\
				  REJILLA_MEDIUM_CLOSED|			\
				  REJILLA_MEDIUM_HAS_DATA,			\
				  (REJILLA_BURN_FLAG_MULTI|			\
				  REJILLA_BURN_FLAG_NOGRACE|			\
				  REJILLA_BURN_FLAG_MERGE) &			\
				  (~(unsupported_MACRO)),				\
				  REJILLA_BURN_FLAG_MULTI);

#define REJILLA_PLUGIN_ADD_STANDARD_BD_R_FLAGS(plugin_MACRO, unsupported_MACRO)			\
	/* DAO and MULTI are exclusive */					\
	rejilla_plugin_set_flags (plugin_MACRO,					\
				  REJILLA_MEDIUM_BDR_RANDOM|			\
				  REJILLA_MEDIUM_BDR_SRM|			\
				  REJILLA_MEDIUM_BDR_SRM_POW|			\
				  REJILLA_MEDIUM_DUAL_L|			\
				  REJILLA_MEDIUM_BLANK,				\
				  (REJILLA_BURN_FLAG_DAO|			\
				  REJILLA_BURN_FLAG_DUMMY|			\
				  REJILLA_BURN_FLAG_NOGRACE) &			\
				  (~(unsupported_MACRO)),				\
				  REJILLA_BURN_FLAG_NONE);			\
	rejilla_plugin_set_flags (plugin_MACRO,					\
				  REJILLA_MEDIUM_BDR_RANDOM|			\
				  REJILLA_MEDIUM_BDR_SRM|			\
				  REJILLA_MEDIUM_BDR_SRM_POW|			\
				  REJILLA_MEDIUM_DUAL_L|			\
				  REJILLA_MEDIUM_BLANK,				\
				  REJILLA_BURN_FLAG_MULTI|			\
				  REJILLA_BURN_FLAG_DUMMY|			\
				  REJILLA_BURN_FLAG_NOGRACE) &			\
				  (~(unsupported_MACRO)),				\
				  REJILLA_BURN_FLAG_NONE);			\
	/* This is a DVDR with data data can be merged or at least appended */	\
	rejilla_plugin_set_flags (plugin_MACRO,					\
				  REJILLA_MEDIUM_BDR_RANDOM|			\
				  REJILLA_MEDIUM_BDR_SRM|			\
				  REJILLA_MEDIUM_BDR_SRM_POW|			\
				  REJILLA_MEDIUM_DUAL_L|			\
				  REJILLA_MEDIUM_APPENDABLE|			\
				  REJILLA_MEDIUM_HAS_DATA,			\
				  (REJILLA_BURN_FLAG_APPEND|			\
				  REJILLA_BURN_FLAG_MERGE|			\
				  REJILLA_BURN_FLAG_MULTI|			\
				  REJILLA_BURN_FLAG_DUMMY|			\
				  REJILLA_BURN_FLAG_NOGRACE) &			\
				  (~(unsupported_MACRO)),				\
				  REJILLA_BURN_FLAG_APPEND);

/**
 * These kind of media don't support:
 * - BURNPROOF
 * - DAO
 * - APPEND
 * since they don't behave and are not written in the same way.
 * They also can't be closed so MULTI is compulsory.
 */
#define REJILLA_PLUGIN_ADD_STANDARD_BD_RE_FLAGS(plugin_MACRO, unsupported_MACRO)			\
	rejilla_plugin_set_flags (plugin_MACRO,					\
				  REJILLA_MEDIUM_BDRE|				\
				  REJILLA_MEDIUM_DUAL_L|			\
				  REJILLA_MEDIUM_UNFORMATTED|			\
				  REJILLA_MEDIUM_BLANK,				\
				  (REJILLA_BURN_FLAG_MULTI|			\
				  REJILLA_BURN_FLAG_NOGRACE) &			\
				  (~(unsupported_MACRO)),				\
				  REJILLA_BURN_FLAG_MULTI);			\
	rejilla_plugin_set_flags (plugin_MACRO,					\
				  REJILLA_MEDIUM_BDRE|				\
				  REJILLA_MEDIUM_DUAL_L|			\
				  REJILLA_MEDIUM_APPENDABLE|			\
				  REJILLA_MEDIUM_CLOSED|			\
				  REJILLA_MEDIUM_HAS_DATA,			\
				  (REJILLA_BURN_FLAG_MULTI|			\
				  REJILLA_BURN_FLAG_NOGRACE|			\
				  REJILLA_BURN_FLAG_MERGE) &			\
				  (~(unsupported_MACRO)),				\
				  REJILLA_BURN_FLAG_MULTI);

G_END_DECLS

#endif
