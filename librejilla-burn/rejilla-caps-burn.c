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

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>

#include <glib.h>
#include <glib/gi18n.h>

#include "rejilla-caps-burn.h"
#include "burn-caps.h"
#include "burn-debug.h"
#include "rejilla-plugin.h"
#include "rejilla-plugin-private.h"
#include "rejilla-plugin-information.h"
#include "burn-task.h"
#include "rejilla-session-helper.h"

/**
 * This macro is used to determine whether or not blanking could change anything
 * for the medium so that we can write to it.
 */
#define REJILLA_BURN_CAPS_NOT_SUPPORTED_LOG(session)				\
{										\
	rejilla_burn_session_log (session, "Unsupported type of task operation"); \
	REJILLA_BURN_LOG ("Unsupported type of task operation");		\
	return NULL;								\
}

#define REJILLA_BURN_CAPS_NOT_SUPPORTED_LOG_ERROR(session, error)		\
{										\
	if (error)								\
		g_set_error (error,						\
			     REJILLA_BURN_ERROR,				\
			     REJILLA_BURN_ERROR_GENERAL,			\
			     _("An internal error occurred"));	 		\
	REJILLA_BURN_CAPS_NOT_SUPPORTED_LOG (session);				\
}

/* That function receives all errors returned by the object and 'learns' from 
 * these errors what are the safest defaults for a particular system. It should 
 * also offer fallbacks if an error occurs through a signal */
static RejillaBurnResult
rejilla_burn_caps_job_error_cb (RejillaJob *job,
				RejillaBurnError error,
				RejillaBurnCaps *caps)
{
	return REJILLA_BURN_ERR;
}

static RejillaPlugin *
rejilla_caps_link_find_plugin (RejillaCapsLink *link,
			       gint group_id,
			       RejillaBurnFlag session_flags,
			       RejillaTrackType *output,
			       RejillaMedia media)
{
	GSList *iter;
	RejillaPlugin *candidate;

	/* Go through all plugins for a link and find the best one. It must:
	 * - be active
	 * - be part of the group (as much as possible)
	 * - have the highest priority
	 * - support the flags */
	candidate = NULL;
	for (iter = link->plugins; iter; iter = iter->next) {
		RejillaPlugin *plugin;

		plugin = iter->data;

		if (!rejilla_plugin_get_active (plugin, 0))
			continue;

		if (output->type == REJILLA_TRACK_TYPE_DISC) {
			gboolean result;

			result = rejilla_plugin_check_record_flags (plugin,
								    media,
								    session_flags);
			if (!result)
				continue;
		}

		if (link->caps->type.type == REJILLA_TRACK_TYPE_DATA) {
			gboolean result;

			result = rejilla_plugin_check_image_flags (plugin,
								   media,
								   session_flags);
			if (!result)
				continue;
		}
		else if (!rejilla_plugin_check_media_restrictions (plugin, media))
			continue;

		if (group_id > 0 && candidate) {
			/* the candidate must be in the favourite group as much as possible */
			if (rejilla_plugin_get_group (candidate) != group_id) {
				if (rejilla_plugin_get_group (plugin) == group_id) {
					candidate = plugin;
					continue;
				}
			}
			else if (rejilla_plugin_get_group (plugin) != group_id)
				continue;
		}

		if (!candidate)
			candidate = plugin;
		else if (rejilla_plugin_get_priority (plugin) >
			 rejilla_plugin_get_priority (candidate))
			candidate = plugin;
	}

	return candidate;
}

typedef struct _RejillaCapsLinkList RejillaCapsLinkList;
struct _RejillaCapsLinkList {
	RejillaCapsLink *link;
	RejillaPlugin *plugin;
};

static gint
rejilla_caps_link_list_sort (gconstpointer a,
                             gconstpointer b)
{
	const RejillaCapsLinkList *node1 = a;
	const RejillaCapsLinkList *node2 = b;
	return rejilla_plugin_get_priority (node2->plugin) -
	       rejilla_plugin_get_priority (node1->plugin);
}

static GSList *
rejilla_caps_get_best_path (GSList *path1,
                            GSList *path2)
{
	GSList *iter1, *iter2;

	iter1 = path1;
	iter2 = path2;

	for (; iter1 && iter2; iter1 = iter1->next, iter2 = iter2->next) {
		gint priority1, priority2;
		RejillaCapsLinkList *node1, *node2;

		node1 = iter1->data;
		node2 = iter2->data;
		priority1 = rejilla_plugin_get_priority (node1->plugin);
		priority2 = rejilla_plugin_get_priority (node2->plugin);
		if (priority1 > priority2) {
			g_slist_foreach (path2, (GFunc) g_free, NULL);
			g_slist_free (path2);
			return path1;
		}

		if (priority1 < priority2) {
			g_slist_foreach (path1, (GFunc) g_free, NULL);
			g_slist_free (path1);
			return path2;
		}
	}

	/* equality all along or one of them is shorter. Keep the shorter or
	 * path1 in case of complete equality. */
	if (!iter2 && iter1) {
		/* This one seems shorter */
		g_slist_foreach (path1, (GFunc) g_free, NULL);
		g_slist_free (path1);
		return path2;
	}

	g_slist_foreach (path2, (GFunc) g_free, NULL);
	g_slist_free (path2);
	return path1;
}

static GSList *
rejilla_caps_find_best_link (RejillaCaps *caps,
			     gint group_id,
			     GSList *used_caps,
			     RejillaBurnFlag session_flags,
			     RejillaMedia media,
			     RejillaTrackType *input,
			     RejillaPluginIOFlag io_flags);

static GSList *
rejilla_caps_get_plugin_results (RejillaCapsLinkList *node,
                                 int group_id,
                                 GSList *used_caps,
                                 RejillaBurnFlag session_flags,
                                 RejillaMedia media,
                                 RejillaTrackType *input,
                                 RejillaPluginIOFlag io_flags)
{
	GSList *results;
	guint search_group_id;
	GSList *plugin_used_caps = g_slist_copy (used_caps);

	/* determine the group_id for the search */
	if (rejilla_plugin_get_group (node->plugin) > 0 && group_id <= 0)
		search_group_id = rejilla_plugin_get_group (node->plugin);
	else
		search_group_id = group_id;

	/* It's not a perfect fit. First see if a plugin with the same
	 * priority don't have the right input. Then see if we can reach
	 * the right input by going through all previous nodes */
	results = rejilla_caps_find_best_link (node->link->caps,
	                                       search_group_id,
	                                       plugin_used_caps,
	                                       session_flags,
	                                       media,
	                                       input,
	                                       io_flags);
	g_slist_free (plugin_used_caps);
	return results;
}

static gboolean
rejilla_caps_link_list_have_processing_plugin (GSList *list)
{
	GSList *iter;
	RejillaPluginProcessFlag position;

	position = REJILLA_PLUGIN_RUN_BEFORE_TARGET;

	for (iter = list; iter; iter = iter->next) {
		RejillaCapsLinkList *node;
		RejillaCaps *caps;
		GSList *modifiers;

		node = list->data;
		caps = node->link->caps;

		if (rejilla_track_type_get_has_medium (&caps->type))
			continue;

		if (!iter->next)
			position = REJILLA_PLUGIN_RUN_PREPROCESSING;

		for (modifiers = caps->modifiers; modifiers; modifiers = modifiers->next) {
			RejillaPluginProcessFlag flags;
			RejillaPlugin *plugin;

			plugin = modifiers->data;
			if (!rejilla_plugin_get_active (plugin, 0))
				continue;

			rejilla_plugin_get_process_flags (plugin, &flags);
			if ((flags & position) == position)
				return TRUE;
		}
	}

	return FALSE;
}

static GSList *
rejilla_caps_find_best_link (RejillaCaps *caps,
			     gint group_id,
			     GSList *used_caps,
			     RejillaBurnFlag session_flags,
			     RejillaMedia media,
			     RejillaTrackType *input,
			     RejillaPluginIOFlag io_flags)
{
	GSList *iter;
	GSList *list = NULL;
	GSList *results = NULL;
	gboolean perfect_fit = FALSE;
	RejillaCapsLinkList *node = NULL;
	gboolean have_processing_plugin = FALSE;

	REJILLA_BURN_LOG_WITH_TYPE (&caps->type, REJILLA_PLUGIN_IO_NONE, "find_best_link");

	/* First, build a list of possible links and sort them out according to
	 * the priority based on the highest priority among their plugins. In 
	 * this case, we can't sort links beforehand since according to the
	 * flags, input, output in the session the plugins will or will not 
	 * be used. Moreover given the group_id thing the choice of plugin may
	 * depends. */

	/* This is done to address possible issues namely:
	 * - growisofs can handle DATA right from the start but has a lower
	 * priority than libburn. In this case growisofs would be used every 
	 * time for DATA despite its having a lower priority than libburn if we
	 * were looking for the best fit first
	 * - We don't want to follow the long path and have a useless (in this
	 * case) image converter plugin get included.
	 * ex: we would have: CDRDAO (input) toc2cue => (CUE) cdrdao => (DISC)
	 * instead of simply: CDRDAO (input) cdrdao => (DISC) */

	for (iter = caps->links; iter; iter = iter->next) {
		RejillaPlugin *plugin;
		RejillaCapsLink *link;
		gboolean fits;

		link = iter->data;

		/* skip blanking links */
		if (!link->caps) {
			REJILLA_BURN_LOG ("Blanking caps");
			continue;
		}

		/* the link should not link to an already used caps */
		if (g_slist_find (used_caps, link->caps)) {
			REJILLA_BURN_LOG ("Already used caps");
			continue;
		}

		if (rejilla_track_type_get_has_medium (&caps->type)) {
			if (rejilla_caps_link_check_recorder_flags_for_input (link, session_flags))
				continue;		
		}

		/* see if that's a perfect fit;
		 * - it must have the same caps (type + subtype)
		 * - it must have the proper IO (file). */
		fits = (link->caps->flags & REJILLA_PLUGIN_IO_ACCEPT_FILE) &&
			rejilla_caps_is_compatible_type (link->caps, input);

		if (!fits) {
			/* if it doesn't fit it must be at least connectable */
			if ((link->caps->flags & io_flags) == REJILLA_PLUGIN_IO_NONE) {
				REJILLA_BURN_LOG ("Not connectable");
				continue;
			}

			/* we can't go further than a DISC type, no need to keep it */
			if (link->caps->type.type == REJILLA_TRACK_TYPE_DISC) {
				REJILLA_BURN_LOG ("Can't go further than DISC caps");
				continue;
			}
		}

		/* See if this link can be used. For a link to be followed it 
		 * must:
		 * - have at least an active plugin
		 * - have at least a plugin accepting the record flags if caps type (output) 
		 *   is a disc (that means that the link is the recording part) 
		 * - have at least a plugin accepting the data flags if caps type (input)
		 *   is DATA. */
		plugin = rejilla_caps_link_find_plugin (link,
							group_id,
							session_flags,
							&caps->type,
							media);
		if (!plugin) {
			REJILLA_BURN_LOG ("No plugin found");
			continue;
		}

		REJILLA_BURN_LOG_TYPE (&link->caps->type, "Found candidate link");

		/* A plugin could be found which means that link can be used.
		 * Insert it in the list at the right place.
		 * The list is sorted according to priorities (starting with the
		 * highest). If 2 links have the same priority put first the one
		 * that has the correct input. */
		node = g_new0 (RejillaCapsLinkList, 1);
		node->plugin = plugin;
		node->link = link;

		list = g_slist_insert_sorted (list, node, rejilla_caps_link_list_sort);
	}

	if (!list) {
		REJILLA_BURN_LOG ("No links found");
		return NULL;
	}

	used_caps = g_slist_prepend (used_caps, caps);

	/* Then, go through this list (starting with highest priority links)
	 * The rule is we prefer the links with the highest priority; if two
	 * links have the same priority and one of them leads to a caps with
	 * the correct type then choose this one. */
	for (iter = list; iter; iter = iter->next) {
		RejillaCapsLinkList *iter_node;

		iter_node = iter->data;

		REJILLA_BURN_LOG ("Trying %s with a priority of %i",
			          rejilla_plugin_get_name (iter_node->plugin),
			          rejilla_plugin_get_priority (iter_node->plugin));

		/* see if that's a perfect fit; if so, then we're good. 
		 * - it must have the same caps (type + subtype)
		 * - it must have the proper IO (file).
		 * The only case where we don't want a perfect fit is when the
		 * other possibility allows for the inclusion and expression
		 * of active track processing plugins. It allows for example to
		 * choose:
		 * mkisofs => checksum image => growisofs
		 * instead of simply:
		 * growisofs.  */
		if ((iter_node->link->caps->flags & REJILLA_PLUGIN_IO_ACCEPT_FILE)
		&&   rejilla_caps_is_compatible_type (iter_node->link->caps, input)) {
			perfect_fit = TRUE;
			break;
		}

		results = rejilla_caps_get_plugin_results (iter_node,
			                                   group_id,
			                                   used_caps,
			                                   session_flags,
			                                   media,
			                                   input,
			                                   io_flags);
		if (results) {
			have_processing_plugin = rejilla_caps_link_list_have_processing_plugin (results);
			break;
		}
	}

	/* Do not check for results that could be NULL in case of a perfect fit */
	if (!iter)
		goto end;

	node = iter->data;

	/* Stage 3: there may be other link with the same priority (most the
	 * time because it is the same plugin) so we try them as well and keep
	 * the one whose next plugin in the list has the highest priority. */
	for (iter = iter->next; iter; iter = iter->next) {
		GSList *other_results;
		RejillaCapsLinkList *iter_node;

		iter_node = iter->data;
		if (rejilla_plugin_get_priority (iter_node->plugin) !=
		    rejilla_plugin_get_priority (node->plugin))
			break;

		REJILLA_BURN_LOG ("Trying %s with a priority of %i",
			          rejilla_plugin_get_name (iter_node->plugin),
			          rejilla_plugin_get_priority (iter_node->plugin));

		/* see if that's a perfect fit */
		if ((iter_node->link->caps->flags & REJILLA_PLUGIN_IO_ACCEPT_FILE) == 0
		||  !rejilla_caps_is_compatible_type (iter_node->link->caps, input)) {
			other_results = rejilla_caps_get_plugin_results (iter_node,
				                                         group_id,
				                                         used_caps,
				                                         session_flags,
				                                         media,
				                                         input,
				                                         io_flags);
			if (!other_results)
				continue;

			if (perfect_fit) {
				have_processing_plugin = rejilla_caps_link_list_have_processing_plugin (other_results);
				if (have_processing_plugin) {
					/* Note: results == NULL for perfect fit */
					results = other_results;

					perfect_fit = FALSE;
					node = iter_node;
				}
				else {
					g_slist_foreach (other_results, (GFunc) g_free, NULL);
					g_slist_free (other_results);
				}
			}
			else {
				results = rejilla_caps_get_best_path (results, other_results);
				if (results == other_results) {
					have_processing_plugin = rejilla_caps_link_list_have_processing_plugin (other_results);
					node = iter_node;
				}
			}
		}
		else if (!perfect_fit && !have_processing_plugin) {
			g_slist_foreach (results, (GFunc) g_free, NULL);
			g_slist_free (results);
			results = NULL;

			perfect_fit = TRUE;
			node = iter_node;
		}
	}

	results = g_slist_prepend (results, node);
	list = g_slist_remove (list, node);

end:

	/* clear up */
	used_caps = g_slist_remove (used_caps, caps);
	g_slist_foreach (list, (GFunc) g_free, NULL);
	g_slist_free (list);

	return results;
}

static gboolean
rejilla_burn_caps_sort_modifiers (gconstpointer a,
				  gconstpointer b)
{
	RejillaPlugin *plug_a = REJILLA_PLUGIN (a);
	RejillaPlugin *plug_b = REJILLA_PLUGIN (b);

	return rejilla_plugin_get_priority (plug_a) -
	       rejilla_plugin_get_priority (plug_b);
}

static GSList *
rejilla_caps_add_processing_plugins_to_task (RejillaBurnSession *session,
					     RejillaTask *task,
					     RejillaCaps *caps,
					     RejillaTrackType *io_type,
					     RejillaPluginProcessFlag position)
{
	GSList *retval = NULL;
	GSList *modifiers;
	GSList *iter;

	if (rejilla_track_type_get_has_medium (&caps->type))
		return NULL;

	REJILLA_BURN_LOG_WITH_TYPE (&caps->type,
				    caps->flags,
				    "Adding modifiers (position %i) (%i modifiers available) for",
				    position,
				    g_slist_length (caps->modifiers));

	/* Go through all plugins and add all possible modifiers. They must:
	 * - be active
	 * - accept the position flags */
	modifiers = g_slist_copy (caps->modifiers);
	modifiers = g_slist_sort (modifiers, rejilla_burn_caps_sort_modifiers);

	for (iter = modifiers; iter; iter = iter->next) {
		RejillaPluginProcessFlag flags;
		RejillaPlugin *plugin;
		RejillaJob *job;
		GType type;

		plugin = iter->data;
		if (!rejilla_plugin_get_active (plugin, 0))
			continue;

		rejilla_plugin_get_process_flags (plugin, &flags);
		if ((flags & position) != position)
			continue;

		type = rejilla_plugin_get_gtype (plugin);
		job = REJILLA_JOB (g_object_new (type,
						 "output", io_type,
						 NULL));
		g_signal_connect (job,
				  "error",
				  G_CALLBACK (rejilla_burn_caps_job_error_cb),
				  caps);

		if (!task
		||  !(caps->flags & REJILLA_PLUGIN_IO_ACCEPT_PIPE)
		||  !REJILLA_BURN_SESSION_NO_TMP_FILE (session)) {
			/* here the action taken is always to create an image */
			task = REJILLA_TASK (g_object_new (REJILLA_TYPE_TASK,
							   "session", session,
							   "action", REJILLA_BURN_ACTION_CREATING_IMAGE,
							   NULL));
			retval = g_slist_prepend (retval, task);
		}

		REJILLA_BURN_LOG ("%s (modifier) added to task",
				  rejilla_plugin_get_name (plugin));

		REJILLA_BURN_LOG_TYPE (io_type, "IO type");

		rejilla_task_add_item (task, REJILLA_TASK_ITEM (job));
	}
	g_slist_free (modifiers);

	return retval;
}

GSList *
rejilla_burn_caps_new_task (RejillaBurnCaps *self,
			    RejillaBurnSession *session,
                            RejillaTrackType *temp_output,
			    GError **error)
{
	RejillaPluginProcessFlag position;
	RejillaBurnFlag session_flags;
	RejillaTrackType plugin_input;
	RejillaTask *blanking = NULL;
	RejillaPluginIOFlag flags;
	RejillaTask *task = NULL;
	RejillaTrackType output;
	RejillaTrackType input;
	RejillaCaps *last_caps;
	GSList *retval = NULL;
	GSList *iter, *list;
	RejillaMedia media;
	gboolean res;

	/* determine the output and the flags for this task */
	if (temp_output) {
		output.type = temp_output->type;
		output.subtype.img_format = temp_output->subtype.img_format;
	}
	else
		rejilla_burn_session_get_output_type (session, &output);

	if (rejilla_track_type_get_has_medium (&output))
		media = rejilla_track_type_get_medium_type (&output);
	else
		media = REJILLA_MEDIUM_FILE;

	if (REJILLA_BURN_SESSION_NO_TMP_FILE (session))
		flags = REJILLA_PLUGIN_IO_ACCEPT_PIPE;
	else
		flags = REJILLA_PLUGIN_IO_ACCEPT_FILE;

	REJILLA_BURN_LOG_WITH_TYPE (&output,
				    flags,
				    "Creating recording/imaging task");

	/* search the start caps and try to get a list of links */
	last_caps = rejilla_burn_caps_find_start_caps (self, &output);
	if (!last_caps)
		REJILLA_BURN_CAPS_NOT_SUPPORTED_LOG_ERROR (session, error);

	rejilla_burn_session_get_input_type (session, &input);
	REJILLA_BURN_LOG_WITH_TYPE (&input,
				    REJILLA_PLUGIN_IO_NONE,
				    "Input set =");

	session_flags = rejilla_burn_session_get_flags (session);
	res = rejilla_check_flags_for_drive (rejilla_burn_session_get_burner (session), session_flags);
	if (!res)
		REJILLA_BURN_CAPS_NOT_SUPPORTED_LOG (session);

	list = rejilla_caps_find_best_link (last_caps,
					    self->priv->group_id,
					    NULL,
					    session_flags,
					    media,
					    &input,
					    flags);
	if (!list) {
		/* we reached this point in two cases:
		 * - if the disc cannot be handled
		 * - if some flags are not handled
		 * It is helpful only if:
		 * - the disc was closed and no plugin can handle this type of 
		 * disc once closed (CD-R(W))
		 * - there was the flag BLANK_BEFORE_WRITE set and no plugin can
		 * handle this flag (means that the plugin should erase and
		 * then write on its own. Basically that works only with
		 * overwrite formatted discs, DVD+RW, ...) */
		if (!rejilla_track_type_get_has_medium (&output))
			REJILLA_BURN_CAPS_NOT_SUPPORTED_LOG_ERROR (session, error);

		/* output is a disc try with initial blanking */
		REJILLA_BURN_LOG ("failed to create proper task. Trying with initial blanking");

		/* apparently nothing can be done to reach our goal. Maybe that
		 * is because we first have to blank the disc. If so add a blank 
		 * task to the others as a first step */
		if (!(session_flags & REJILLA_BURN_FLAG_BLANK_BEFORE_WRITE)
		||    rejilla_burn_session_can_blank (session) != REJILLA_BURN_OK)
			REJILLA_BURN_CAPS_NOT_SUPPORTED_LOG_ERROR (session, error);

		/* retry with the same disc type but blank this time */
		media &= ~(REJILLA_MEDIUM_CLOSED|
			   REJILLA_MEDIUM_APPENDABLE|
	   		   REJILLA_MEDIUM_UNFORMATTED|
			   REJILLA_MEDIUM_HAS_DATA|
			   REJILLA_MEDIUM_HAS_AUDIO);
		media |= REJILLA_MEDIUM_BLANK;

		rejilla_track_type_set_medium_type (&output, media);

		last_caps = rejilla_burn_caps_find_start_caps (self, &output);
		if (!last_caps)
			REJILLA_BURN_CAPS_NOT_SUPPORTED_LOG_ERROR (session, error);

		/* if the flag BLANK_BEFORE_WRITE was set then remove it since
		 * we are actually blanking. Simply the record plugin won't have
		 * to do it. */
		session_flags &= ~REJILLA_BURN_FLAG_BLANK_BEFORE_WRITE;
		list = rejilla_caps_find_best_link (last_caps,
						    self->priv->group_id,
						    NULL,
						    session_flags,
						    media,
						    &input,
						    flags);
		if (!list)
			REJILLA_BURN_CAPS_NOT_SUPPORTED_LOG_ERROR (session, error);

		REJILLA_BURN_LOG ("initial blank/erase task required")

		blanking = rejilla_burn_caps_new_blanking_task (self, session, error);
		/* The problem here is that we shouldn't always prepend such a 
		 * task. For example when we copy a disc to another using the 
		 * same drive. In this case we should insert it before the last.
		 * Now, that always work so that's what we do in all cases. Once
		 * the whole list of tasks is created we insert this blanking
		 * task just before the last one. Another advantage is that the
		 * blanking of the disc is delayed as late as we can which means
		 * in case of error we keep it intact as late as we can. */
	}

	/* reverse the list of links to have them in the right order */
	list = g_slist_reverse (list);
	position = REJILLA_PLUGIN_RUN_PREPROCESSING;

	rejilla_burn_session_get_input_type (session, &plugin_input);
	for (iter = list; iter; iter = iter->next) {
		RejillaTrackType plugin_output;
		RejillaCapsLinkList *node;
		RejillaJob *job;
		GSList *result;
		GType type;

		node = iter->data;

		/* determine the plugin output:
		 * if it's not the last one it takes the input of the next
		 * plugin as its output.
		 * Otherwise it uses the final output type */
		if (iter->next) {
			RejillaCapsLinkList *next_node;

			next_node = iter->next->data;
			memcpy (&plugin_output,
				&next_node->link->caps->type,
				sizeof (RejillaTrackType));
		}
		else
			memcpy (&plugin_output,
				&output,
				sizeof (RejillaTrackType));

		/* first see if there are track processing plugins */
		result = rejilla_caps_add_processing_plugins_to_task (session,
								      task,
								      node->link->caps,
								      &plugin_input,
								      position);
		retval = g_slist_concat (retval, result);

		/* Create an object from the plugin */
		type = rejilla_plugin_get_gtype (node->plugin);
		job = REJILLA_JOB (g_object_new (type,
						 "output", &plugin_output,
						 NULL));
		g_signal_connect (job,
				  "error",
				  G_CALLBACK (rejilla_burn_caps_job_error_cb),
				  node->link);

		if (!task
		||  !(node->link->caps->flags & REJILLA_PLUGIN_IO_ACCEPT_PIPE)
		||  !REJILLA_BURN_SESSION_NO_TMP_FILE (session)) {
			/* only the last task will be doing the proper action
			 * all other are only steps to take to reach the final
			 * action */
			REJILLA_BURN_LOG ("New task");
			task = REJILLA_TASK (g_object_new (REJILLA_TYPE_TASK,
							   "session", session,
							   "action", REJILLA_TASK_ACTION_NORMAL,
							   NULL));
			retval = g_slist_append (retval, task);
		}

		rejilla_task_add_item (task, REJILLA_TASK_ITEM (job));

		REJILLA_BURN_LOG ("%s added to task", rejilla_plugin_get_name (node->plugin));
		REJILLA_BURN_LOG_TYPE (&plugin_input, "input");
		REJILLA_BURN_LOG_TYPE (&plugin_output, "output");

		position = REJILLA_PLUGIN_RUN_BEFORE_TARGET;

		/* the output of the plugin will become the input of the next */
		memcpy (&plugin_input, &plugin_output, sizeof (RejillaTrackType));
	}
	g_slist_foreach (list, (GFunc) g_free, NULL);
	g_slist_free (list);

	/* add the post processing plugins */
	list = rejilla_caps_add_processing_plugins_to_task (session,
							    NULL,
							    last_caps,
							    &output,
							    REJILLA_PLUGIN_RUN_AFTER_TARGET);
	retval = g_slist_concat (retval, list);

	if (rejilla_track_type_get_has_medium (&last_caps->type) && blanking) {
		retval = g_slist_insert_before (retval,
						g_slist_last (retval),
						blanking);
	}

	return retval;
}

RejillaTask *
rejilla_burn_caps_new_checksuming_task (RejillaBurnCaps *self,
					RejillaBurnSession *session,
					GError **error)
{
	RejillaTrackType track_type;
	RejillaPlugin *candidate;
	RejillaCaps *last_caps;
	RejillaTrackType input;
	guint checksum_type;
	RejillaTrack *track;
	RejillaTask *task;
	RejillaJob *job;
	GSList *tracks;
	GSList *links;
	GSList *list;
	GSList *iter;

	rejilla_burn_session_get_input_type (session, &input);
	REJILLA_BURN_LOG_WITH_TYPE (&input,
				    REJILLA_PLUGIN_IO_NONE,
				    "Creating checksuming task with input");

	/* first find a checksuming job that can output the type of required
	 * checksum. Then go through the caps to see if the input type can be
	 * found. */

	/* some checks */
	tracks = rejilla_burn_session_get_tracks (session);
	if (g_slist_length (tracks) != 1) {
		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_GENERAL,
			     _("Only one track at a time can be checked"));
		return NULL;
	}

	/* get the required checksum type */
	track = tracks->data;
	checksum_type = rejilla_track_get_checksum_type (track);

	links = NULL;
	for (iter = self->priv->tests; iter; iter = iter->next) {
		RejillaCapsTest *test;

		test = iter->data;
		if (!test->links)
			continue;

		/* check this caps test supports the right checksum type */
		if (test->type & checksum_type) {
			links = test->links;
			break;
		}
	}

	if (!links) {
		/* we failed to find and create a proper task */
		REJILLA_BURN_CAPS_NOT_SUPPORTED_LOG_ERROR (session, error);
	}

	list = NULL;
	last_caps = NULL;
	rejilla_track_get_track_type (track, &track_type);
	for (iter = links; iter; iter = iter->next) {
		RejillaCapsLink *link;
		GSList *plugins;

		link = iter->data;

		/* NOTE: that shouldn't happen */
		if (!link->caps)
			continue;

		REJILLA_BURN_LOG_TYPE (&link->caps->type, "Trying link to");

		/* Make sure we have a candidate */
		candidate = NULL;
		for (plugins = link->plugins; plugins; plugins = plugins->next) {
			RejillaPlugin *plugin;

			plugin = plugins->data;
			if (!rejilla_plugin_get_active (plugin, 0))
				continue;

			/* note for checksuming task there is no group possible */
			if (!candidate)
				candidate = plugin;
			else if (rejilla_plugin_get_priority (plugin) >
				 rejilla_plugin_get_priority (candidate))
				candidate = plugin;
		}

		if (!candidate)
			continue;

		/* see if it can handle the input or if it can be linked to 
		 * another plugin that can */
		if (rejilla_caps_is_compatible_type (link->caps, &input)) {
			/* this is the right caps */
			last_caps = link->caps;
			break;
		}

		/* don't go any further if that's a DISC type */
		if (link->caps->type.type == REJILLA_TRACK_TYPE_DISC)
			continue;

		/* the caps itself is not the right one so we try to 
		 * go through its links to find the right caps. */
		list = rejilla_caps_find_best_link (link->caps,
						    self->priv->group_id,
						    NULL,
						    REJILLA_BURN_FLAG_NONE,
						    REJILLA_MEDIUM_NONE,
						    &input,
						    REJILLA_PLUGIN_IO_ACCEPT_PIPE);
		if (list) {
			last_caps = link->caps;
			break;
		}
	}

	if (!last_caps) {
		/* no link worked failure */
		REJILLA_BURN_CAPS_NOT_SUPPORTED_LOG_ERROR (session, error);
	}

	/* we made it. Create task */
	task = REJILLA_TASK (g_object_new (REJILLA_TYPE_TASK,
					   "session", session,
					   "action", REJILLA_TASK_ACTION_CHECKSUM,
					   NULL));

	list = g_slist_reverse (list);
	for (iter = list; iter; iter = iter->next) {
		GType type;
		RejillaCapsLinkList *node;
		RejillaTrackType *plugin_output;

		node = iter->data;

		/* determine the plugin output */
		if (iter->next) {
			RejillaCapsLink *next_link;

			next_link = iter->next->data;
			plugin_output = &next_link->caps->type;
		}
		else
			plugin_output = &last_caps->type;

		/* create the object */
		type = rejilla_plugin_get_gtype (node->plugin);
		job = REJILLA_JOB (g_object_new (type,
						 "output", plugin_output,
						 NULL));
		g_signal_connect (job,
				  "error",
				  G_CALLBACK (rejilla_burn_caps_job_error_cb),
				  node->link);

		rejilla_task_add_item (task, REJILLA_TASK_ITEM (job));

		REJILLA_BURN_LOG ("%s added to task", rejilla_plugin_get_name (node->plugin));
	}
	g_slist_foreach (list, (GFunc) g_free, NULL);
	g_slist_free (list);

	/* Create the candidate */
	job = REJILLA_JOB (g_object_new (rejilla_plugin_get_gtype (candidate),
					 "output", NULL,
					 NULL));
	g_signal_connect (job,
			  "error",
			  G_CALLBACK (rejilla_burn_caps_job_error_cb),
			  self);
	rejilla_task_add_item (task, REJILLA_TASK_ITEM (job));

	return task;
}

RejillaTask *
rejilla_burn_caps_new_blanking_task (RejillaBurnCaps *self,
				     RejillaBurnSession *session,
				     GError **error)
{
	GSList *iter;
	RejillaMedia media;
	RejillaBurnFlag flags;
	RejillaTask *task = NULL;

	media = rejilla_burn_session_get_dest_media (session);
	flags = rejilla_burn_session_get_flags (session);

	for (iter = self->priv->caps_list; iter; iter = iter->next) {
		RejillaCaps *caps;
		GSList *links;

		caps = iter->data;
		if (caps->type.type != REJILLA_TRACK_TYPE_DISC)
			continue;

		if ((media & caps->type.subtype.media) != media)
			continue;

		for (links = caps->links; links; links = links->next) {
			GSList *plugins;
			RejillaCapsLink *link;
			RejillaPlugin *candidate;

			link = links->data;

			if (link->caps != NULL)
				continue;

			/* Go through all the plugins and find the best plugin
			 * for the task. It must :
			 * - be active
			 * - have the highest priority
			 * - accept the flags */
			candidate = NULL;
			for (plugins = link->plugins; plugins; plugins = plugins->next) {
				RejillaPlugin *plugin;

				plugin = plugins->data;

				if (!rejilla_plugin_get_active (plugin, 0))
					continue;

				if (!rejilla_plugin_check_blank_flags (plugin, media, flags))
					continue;

				if (self->priv->group_id > 0 && candidate) {
					/* the candidate must be in the favourite group as much as possible */
					if (rejilla_plugin_get_group (candidate) != self->priv->group_id) {
						if (rejilla_plugin_get_group (plugin) == self->priv->group_id) {
							candidate = plugin;
							continue;
						}
					}
					else if (rejilla_plugin_get_group (plugin) != self->priv->group_id)
						continue;
				}

				if (!candidate)
					candidate = plugin;
				else if (rejilla_plugin_get_priority (plugin) >
					 rejilla_plugin_get_priority (candidate))
					candidate = plugin;
			}

			if (candidate) {
				RejillaJob *job;
				GType type;

				type = rejilla_plugin_get_gtype (candidate);
				job = REJILLA_JOB (g_object_new (type,
								 "output", NULL,
								 NULL));
				g_signal_connect (job,
						  "error",
						  G_CALLBACK (rejilla_burn_caps_job_error_cb),
						  caps);

				task = REJILLA_TASK (g_object_new (REJILLA_TYPE_TASK,
								   "session", session,
								   "action", REJILLA_TASK_ACTION_ERASE,
								   NULL));
				rejilla_task_add_item (task, REJILLA_TASK_ITEM (job));
				return task;
			}
		}
	}

	REJILLA_BURN_CAPS_NOT_SUPPORTED_LOG_ERROR (session, error);
}
