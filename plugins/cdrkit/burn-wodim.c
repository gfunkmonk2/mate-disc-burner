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
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <glib/gstdio.h>
#include <gmodule.h>

#include "rejilla-units.h"

#include "burn-job.h"
#include "burn-process.h"
#include "rejilla-plugin-registration.h"
#include "burn-cdrkit.h"

#include "rejilla-tags.h"
#include "rejilla-track-image.h"
#include "rejilla-track-stream.h"


#define REJILLA_TYPE_WODIM         (rejilla_wodim_get_type ())
#define REJILLA_WODIM(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), REJILLA_TYPE_WODIM, RejillaWodim))
#define REJILLA_WODIM_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), REJILLA_TYPE_WODIM, RejillaWodimClass))
#define REJILLA_IS_WODIM(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), REJILLA_TYPE_WODIM))
#define REJILLA_IS_WODIM_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), REJILLA_TYPE_WODIM))
#define REJILLA_WODIM_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), REJILLA_TYPE_WODIM, RejillaWodimClass))

REJILLA_PLUGIN_BOILERPLATE (RejillaWodim, rejilla_wodim, REJILLA_TYPE_PROCESS, RejillaProcess);

struct _RejillaWodimPrivate {
	goffset current_track_end_pos;
	goffset current_track_written;

	gint current_track_num;
	gint track_count;

	gint minbuf;

	GSList *infs;

	guint immediate:1;
};
typedef struct _RejillaWodimPrivate RejillaWodimPrivate;
#define REJILLA_WODIM_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REJILLA_TYPE_WODIM, RejillaWodimPrivate))

static GObjectClass *parent_class = NULL;

#define REJILLA_SCHEMA_CONFIG		"org.mate.rejilla.config"
#define REJILLA_KEY_IMMEDIATE_FLAG      "immed-flag"
#define REJILLA_KEY_MINBUF_VALUE	"minbuf-value"

static RejillaBurnResult
rejilla_wodim_stderr_read (RejillaProcess *process, const gchar *line)
{
	RejillaBurnFlag flags;

	rejilla_job_get_flags (REJILLA_JOB (process), &flags);

	if (strstr (line, "Cannot open SCSI driver.")
	||  strstr (line, "Operation not permitted. Cannot send SCSI cmd via ioctl")
	||  strstr (line, "Cannot open or use SCSI driver")) {
		rejilla_job_error (REJILLA_JOB (process),
				   g_error_new (REJILLA_BURN_ERROR,
						REJILLA_BURN_ERROR_PERMISSION,
						_("You do not have the required permissions to use this drive")));
	}
	else if (!(flags & REJILLA_BURN_FLAG_OVERBURN)
	     &&  strstr (line, "Data may not fit on current disk")) {
		/* we don't error out if overburn was chosen */
		rejilla_job_error (REJILLA_JOB (process),
				   g_error_new (REJILLA_BURN_ERROR,
						REJILLA_BURN_ERROR_MEDIUM_SPACE,
						_("Not enough space available on the disc")));
	}
	else if (strstr (line ,"cdrecord: A write error occurred")
	     ||  strstr (line, "Could not write Lead-in")
	     ||  strstr (line, "Cannot fixate disk")) {
		rejilla_job_error (REJILLA_JOB (process),
				   g_error_new (REJILLA_BURN_ERROR,
						REJILLA_BURN_ERROR_WRITE_MEDIUM,
						_("An error occurred while writing to disc")));
	}
	else if (strstr (line, "DMA speed too slow") != NULL) {
		rejilla_job_error (REJILLA_JOB (process),
				   g_error_new (REJILLA_BURN_ERROR,
						REJILLA_BURN_ERROR_SLOW_DMA,
						_("The system is too slow to write the disc at this speed. Try a lower speed")));
	}
	else if (strstr (line, "Device or resource busy")) {
		if (!strstr (line, "retrying in")) {
			rejilla_job_error (REJILLA_JOB (process),
					   g_error_new (REJILLA_BURN_ERROR,
							REJILLA_BURN_ERROR_DRIVE_BUSY,
							_("The drive is busy")));
		}
	}
	else if (strstr (line, "Illegal write mode for this drive")) {
		/* NOTE : when it happened I had to unlock the
		 * drive with cdrdao and eject it. Should we ? */
		rejilla_job_error (REJILLA_JOB (process),
				   g_error_new (REJILLA_BURN_ERROR,
						REJILLA_BURN_ERROR_DRIVE_BUSY,
						_("The drive is busy")));
	}
	else if (strstr (line, "Probably trying to use ultra high speed+ medium on improper writer")) {
		/* Set a deferred error as this message tends to indicate a failure */
		rejilla_process_deferred_error (process,
						g_error_new (REJILLA_BURN_ERROR,
							     REJILLA_BURN_ERROR_MEDIUM_INVALID,
							     _("The disc is not supported")));
	}
	/* REMINDER: these should not be necessary as we checked that already */
	/**
	else if (strstr (line, "cannot write medium - incompatible format") != NULL) {
		rejilla_job_error (REJILLA_JOB (process),
				   g_error_new (REJILLA_BURN_ERROR,
						REJILLA_BURN_ERROR_INPUT_INVALID,
						_("The image does not seem to be a proper iso9660 file system")));
	}
	else if (strstr (line, "This means that we are checking recorded media.") != NULL) {
	**/	/* NOTE: defer the consequence of this error as it is not always
		 * fatal. So send a warning but don't stop the process. */
	/**	rejilla_process_deferred_error (process,
						g_error_new (REJILLA_BURN_ERROR,
							     REJILLA_BURN_ERROR_MEDIUM_INVALID,
							     _("The disc is already burnt")));
	}
	else if (strstr (line, "Cannot blank disk, aborting.") != NULL) {
		rejilla_job_error (REJILLA_JOB (process),
				   g_error_new (REJILLA_BURN_ERROR,
						REJILLA_BURN_ERROR_MEDIUM_INVALID,
						_("The disc could not be blanked")));
	}
	else if (strstr (line, "Bad audio track size")) {
		rejilla_job_error (REJILLA_JOB (process),
				   g_error_new (REJILLA_BURN_ERROR,
						REJILLA_BURN_ERROR_GENERAL,
						_("The audio tracks are too short or not a multiple of 2352")));
	}
	else if (strstr (line, "cdrecord: No such file or directory. Cannot open")
	     ||  strstr (line, "No tracks specified. Need at least one.")) {
		rejilla_job_error (REJILLA_JOB (process),
				   g_error_new (REJILLA_BURN_ERROR,
						REJILLA_BURN_ERROR_INPUT,
						_("The image file cannot be found")));
	}
	else if (strstr (line, "Inappropriate audio coding")) {
		rejilla_job_error (REJILLA_JOB (process),
				   g_error_new (REJILLA_BURN_ERROR,
						REJILLA_BURN_ERROR_INPUT_INVALID,
						_("All audio files must be stereo, 16-bit digital audio with 44100Hz samples")));
	}
	else if (strstr (line, "No disk / Wrong disk!") != NULL) {
		rejilla_job_error (REJILLA_JOB (process),
				   g_error_new (REJILLA_BURN_ERROR,
						REJILLA_BURN_ERROR_MEDIA_NONE,
						_("There seems to be no disc in the drive")));
	}

	**/

	/** For these we'd rather have a message saying "cdrecord failed"
	 *  as an internal error occurred says nothing/even less
	else if (strstr (line, "Bad file descriptor. read error on input file")
	     ||  strstr (line, "Input buffer error, aborting")) {
		rejilla_job_error (REJILLA_JOB (process),
				   g_error_new (REJILLA_BURN_ERROR,
						REJILLA_BURN_ERROR_GENERAL,
						_("An internal error occurred")));
	}

	**/

	return REJILLA_BURN_OK;
}

static void
rejilla_wodim_compute (RejillaWodim *wodim,
		       goffset mb_written,
		       goffset mb_total,
		       goffset track_num)
{
	gboolean track_num_changed = FALSE;
	RejillaWodimPrivate *priv;
	RejillaJobAction action;
	goffset this_remain;
	goffset bytes;
	goffset total;

	priv = REJILLA_WODIM_PRIVATE (wodim);
	if (mb_total <= 0)
		return;

	total = mb_total * (goffset) 1048576LL;

	if (track_num > priv->current_track_num) {
		track_num_changed = TRUE;
		priv->current_track_num = track_num;
		priv->current_track_end_pos += mb_total * (goffset) 1048576LL;
	}

	this_remain = (mb_total - mb_written) * (goffset) 1048576LL;
	bytes = (total - priv->current_track_end_pos) + this_remain;
	rejilla_job_set_written_session (REJILLA_JOB (wodim), total - bytes);

	rejilla_job_get_action (REJILLA_JOB (wodim), &action);
	if (action == REJILLA_JOB_ACTION_RECORD) {
		gchar *action_string;
		gchar *track_num_str;

		track_num_str = g_strdup_printf ("%02" G_GOFFSET_FORMAT, track_num);

		/* Translators: %s is the number of the track */
		action_string = g_strdup_printf (_("Writing track %s"), track_num_str);
		g_free (track_num_str);

		rejilla_job_set_current_action (REJILLA_JOB (wodim),
						REJILLA_BURN_ACTION_RECORDING,
						action_string,
						track_num_changed);
		g_free (action_string);
	}
	else if (action == REJILLA_JOB_ACTION_ERASE) {
		rejilla_job_set_progress (REJILLA_JOB (wodim), (gfloat) mb_written / (gfloat) mb_total);
		rejilla_job_set_current_action (REJILLA_JOB (wodim),
						REJILLA_BURN_ACTION_BLANKING,
						NULL,
						FALSE);
	}
}

static void
rejilla_wodim_set_rate (RejillaProcess *process,
                        int speed_1,
                        int speed_2)
{
	gdouble current_rate = -1.0;
	RejillaMedia media;

	if (rejilla_job_get_media (REJILLA_JOB (process), &media) != REJILLA_BURN_OK)
		return;

	if (REJILLA_MEDIUM_IS (media, REJILLA_MEDIUM_CD))
		current_rate = (gdouble) ((gdouble) speed_1 +
			       (gdouble) speed_2 / 10.0) *
			       (gdouble) CD_RATE;
	else if (REJILLA_MEDIUM_IS (media, REJILLA_MEDIUM_DVD))
		current_rate = (gdouble) ((gdouble) speed_1 +
			       (gdouble) speed_2 / 10.0) *
			       (gdouble) DVD_RATE;
	else if (REJILLA_MEDIUM_IS (media, REJILLA_MEDIUM_BD))
		current_rate = (gdouble) ((gdouble) speed_1 +
			       (gdouble) speed_2 / 10.0) *
			       (gdouble) BD_RATE;

	rejilla_job_set_rate (REJILLA_JOB (process), current_rate);
}

static RejillaBurnResult
rejilla_wodim_stdout_read (RejillaProcess *process, const gchar *line)
{
	guint track;
	guint speed_1, speed_2;
	RejillaWodim *wodim;
	RejillaWodimPrivate *priv;
	int mb_written = 0, mb_total = 0, fifo = 0, buf = 0;

	wodim = REJILLA_WODIM (process);
	priv = REJILLA_WODIM_PRIVATE (wodim);

	if (sscanf (line, "Track %2u: %d of %d MB written (fifo %d%%) [buf %d%%] %d.%dx.",
		    &track, &mb_written, &mb_total, &fifo, &buf, &speed_1, &speed_2) == 7 ||
	    /* This is for DVD+R */
	    sscanf (line, "Track %2u:    %d of %d MB written (fifo  %d%%) [buf  %d%%] |%*s  %*s|   %d.%dx.",
	            &track, &mb_written, &mb_total, &fifo, &buf, &speed_1, &speed_2) == 7) {
		rejilla_wodim_set_rate (process, speed_1, speed_2);
		priv->current_track_written = (goffset) mb_written * (goffset) 1048576LL;
		rejilla_wodim_compute (wodim,
				       mb_written,
				       mb_total,
				       track);

		rejilla_job_start_progress (REJILLA_JOB (wodim), FALSE);
	} 
	else if (sscanf (line, "Track %2u:    %d MB written (fifo %d%%) [buf  %d%%]  %d.%dx.",
			 &track, &mb_written, &fifo, &buf, &speed_1, &speed_2) == 6 ||
	         sscanf (line, "Track %2u:    %d MB written (fifo %d%%) [buf  %d%%] |%*s  %*s|   %d.%dx.",
			 &track, &mb_written, &fifo, &buf, &speed_1, &speed_2) == 6) {
		/* this line is printed when wodim writes on the fly */
		rejilla_wodim_set_rate (process, speed_1, speed_2);
		priv->current_track_written = (goffset) mb_written * (goffset) 1048576LL;
		if (rejilla_job_get_fd_in (REJILLA_JOB (wodim), NULL) == REJILLA_BURN_OK) {
			goffset bytes = 0;

			/* we must ask the imager what is the total size */
			rejilla_job_get_session_output_size (REJILLA_JOB (wodim),
							     NULL,
							     &bytes);
			mb_total = bytes / (goffset) 1048576LL;
			rejilla_wodim_compute (wodim,
					       mb_written,
					       mb_total,
					       track);
		}

		rejilla_job_start_progress (REJILLA_JOB (wodim), FALSE);
	}
	else if (sscanf (line, "Formating in progress: %d.%d %% done", &mb_written, &mb_total) == 2) {
		rejilla_job_set_current_action (REJILLA_JOB (process),
						REJILLA_BURN_ACTION_BLANKING,
						_("Formatting disc"),
						FALSE);

		rejilla_job_start_progress (REJILLA_JOB (wodim), FALSE);
		rejilla_job_set_progress (REJILLA_JOB (wodim),
					  (gdouble) ((gdouble) mb_written + ((gdouble) mb_total) / 10.0) / 100.0);
	}
	else if (sscanf (line, "Track %*d: %*s %d MB ", &mb_total) == 1) {
/*		if (mb_total > 0)
			priv->tracks_total_bytes += mb_total * 1048576;
*/	}
	else if (strstr (line, "Sending CUE sheet")) {
		RejillaTrackType *type = NULL;

		/* See if we are in an audio case which would mean we're writing
		 * CD-TEXT */
		type = rejilla_track_type_new ();
		rejilla_job_get_input_type (REJILLA_JOB (wodim), type);
		rejilla_job_set_current_action (REJILLA_JOB (process),
						REJILLA_BURN_ACTION_RECORDING_CD_TEXT,
						rejilla_track_type_get_has_stream (type) ? NULL:_("Writing cue sheet"),
						FALSE);
		rejilla_track_type_free (type);
	}
	else if (g_str_has_prefix (line, "Re-load disk and hit <CR>")
	     ||  g_str_has_prefix (line, "send SIGUSR1 to continue")) {
		RejillaBurnAction action = REJILLA_BURN_ACTION_NONE;

		rejilla_job_get_current_action (REJILLA_JOB (process), &action);

		/* NOTE: There seems to be a BUG somewhere when writing raw images
		 * with clone mode. After disc has been written and fixated wodim
		 * asks the media to be reloaded. So we simply ignore this message
		 * and returns that everything went well. Which is indeed the case */
		if (action == REJILLA_BURN_ACTION_FIXATING) {
			rejilla_job_finished_session (REJILLA_JOB (process));
			return REJILLA_BURN_OK;
		}

		rejilla_job_error (REJILLA_JOB (process),
				   g_error_new (REJILLA_BURN_ERROR,
						REJILLA_BURN_ERROR_MEDIUM_NEED_RELOADING,
						_("The disc needs to be reloaded before being recorded")));
	}
	else if (g_str_has_prefix (line, "Fixating...")
	     ||  g_str_has_prefix (line, "Writing Leadout...")) {
		RejillaJobAction action;

		/* Do this to avoid strange things to appear when erasing */
		rejilla_job_get_action (REJILLA_JOB (wodim), &action);
		if (action == REJILLA_JOB_ACTION_RECORD)
			rejilla_job_set_current_action (REJILLA_JOB (process),
							REJILLA_BURN_ACTION_FIXATING,
							NULL,
							FALSE);
	}
	else if (g_str_has_prefix (line, "Last chance to quit, ")) {
		rejilla_job_set_dangerous (REJILLA_JOB (process), TRUE);
	}
/*	else if (g_str_has_prefix (line, "Blanking PMA, TOC, pregap")
	     ||  strstr (line, "Blanking entire disk")) {

	}
 */
	else if (strstr (line, "Disk sub type: Ultra High speed+")) {
		/* Set a deferred error as this message tends to indicate a failure */
		rejilla_process_deferred_error (process,
						g_error_new (REJILLA_BURN_ERROR,
							     REJILLA_BURN_ERROR_MEDIUM_INVALID,
							     _("The disc is not supported")));
	}
	/* This should not happen */
	/* else if (strstr (line, "Use tsize= option in SAO mode to specify track size")) */

	return REJILLA_BURN_OK;
}

static gboolean
rejilla_wodim_write_inf (RejillaWodim *wodim,
			 GPtrArray *argv,
			 RejillaTrack *track,
			 const gchar *tmpdir,
			 const gchar *album,
			 gint index,
			 gint start,
			 gboolean last_track,
			 GError **error)
{
	gint fd;
	int isrc;
	gint size;
        int errsv;
	gchar *path;
	guint64 length;
	gchar *string;
	gint b_written;
	gint64 sectors;
	gchar buffer [128];
	const gchar *info;
	RejillaWodimPrivate *priv;

	priv = REJILLA_WODIM_PRIVATE (wodim);

	/* NOTE: about the .inf files: they should have the exact same path
	 * but the ending suffix file is replaced by inf:
	 * example : /path/to/file.mp3 => /path/to/file.inf */
	if (rejilla_job_get_fd_in (REJILLA_JOB (wodim), NULL) != REJILLA_BURN_OK) {
		gchar *dot, *separator;

		path = rejilla_track_stream_get_source (REJILLA_TRACK_STREAM (track), FALSE);

		dot = strrchr (path, '.');
		separator = strrchr (path, G_DIR_SEPARATOR);

		if (dot && dot > separator)
			path = g_strdup_printf ("%.*s.inf", (int) (dot - path), path);
		else
			path = g_strdup_printf ("%s.inf", path);

		/* since this file was not returned by rejilla_job_get_tmp_file
		 * it won't be erased when session is unrefed so we have to do 
		 * it ourselves */
		priv->infs = g_slist_prepend (priv->infs, g_strdup (path));
	}
	else {
		RejillaBurnResult result;

		/* in this case don't care about the name since stdin is used */
		result = rejilla_job_get_tmp_file (REJILLA_JOB (wodim),
						   ".inf",
						   &path,
						   error);
		if (result != REJILLA_BURN_OK)
			return result;
	}

	fd = open (path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
	if (fd < 0)
		goto error;

	REJILLA_JOB_LOG (wodim, "writing inf (%s)", path);

	/* The problem here is that when writing CD-TEXT from .inf files, wodim
	 * uses only one charset (and don't let us specify which one) which is
	 * ISO-8859-1. (NOTE: don't believe the doc claiming it requires ASCII
	 * and see cdrecord/cdtext.c line 309).
	 * So we have to convert our UTF-8 input into such a charset.
	 * NOTE: according to docs ASCII should be used for text packs other
	 * than disc/track title.
	 * It might be good in the end to write and pack CD-TEXT pack data 
	 * ourselves so we can set a different charset from English like 
	 * Chinese for example. */
	strcpy (buffer, "# created by rejilla\n");
	size = strlen (buffer);
	b_written = write (fd, buffer, size);
	if (b_written != size)
		goto error;

	strcpy (buffer, "MCN=\t\n");
	size = strlen (buffer);
	b_written = write (fd, buffer, size);
	if (b_written != size)
		goto error;

	/* ISRC */
	isrc = rejilla_track_tag_lookup_int (REJILLA_TRACK (track), REJILLA_TRACK_STREAM_ISRC_TAG);
	if (isrc > 0)
		string = g_strdup_printf ("ISRC=\t%i\n", isrc);
	else
		string = g_strdup ("ISRC=\t\n");
	size = strlen (string);
	b_written = write (fd, string, size);
	g_free (string);
	if (b_written != size)
		goto error;

	strcpy (buffer, "Albumperformer=\t\n");
	size = strlen (buffer);
	b_written = write (fd, buffer, size);
	if (b_written != size)
		goto error;

	if (album) {
		gchar *encoded;

		encoded = g_convert_with_fallback (album,
						   -1,
						   "ISO-8859-1",
						   "UTF-8",
						   "_",	/* Fallback for non convertible characters */
						   NULL,
						   NULL,
						   NULL);
		string = g_strdup_printf ("Albumtitle=\t%s\n", encoded);
		g_free (encoded);
	}
	else
		string = strdup ("Albumtitle=\t\n");
	size = strlen (string);
	b_written = write (fd, string, size);
	g_free (string);
	if (b_written != size)
		goto error;

	/* ARTIST */
	info = rejilla_track_tag_lookup_string (REJILLA_TRACK (track),
						REJILLA_TRACK_STREAM_ARTIST_TAG);
	if (info) {
		gchar *encoded;

		encoded = g_convert_with_fallback (info,
						   -1,
						   "ISO-8859-1",
						   "UTF-8",
						   "_",	/* Fallback for non convertible characters */
						   NULL,
						   NULL,
						   NULL);
		string = g_strdup_printf ("Performer=\t%s\n", encoded);
		g_free (encoded);
	}
	else
		string = strdup ("Performer=\t\n");
	size = strlen (string);
	b_written = write (fd, string, size);
	g_free (string);
	if (b_written != size)
		goto error;

	/* COMPOSER */
	info = rejilla_track_tag_lookup_string (REJILLA_TRACK (track),
						REJILLA_TRACK_STREAM_COMPOSER_TAG);
	if (info) {
		gchar *encoded;

		encoded = g_convert_with_fallback (info,
						   -1,
						   "ISO-8859-1",
						   "UTF-8",
						   "_",	/* Fallback for non convertible characters */
						   NULL,
						   NULL,
						   NULL);
		string = g_strdup_printf ("Composer=\t%s\n", encoded);
		g_free (encoded);
	}
	else
		string = strdup ("Composer=\t\n");
	size = strlen (string);
	b_written = write (fd, string, size);
	g_free (string);
	if (b_written != size)
		goto error;

	/* TITLE */
	info = rejilla_track_tag_lookup_string (REJILLA_TRACK (track),
						REJILLA_TRACK_STREAM_TITLE_TAG);
	if (info) {
		gchar *encoded;

		encoded = g_convert_with_fallback (info,
						   -1,
						   "ISO-8859-1",
						   "UTF-8",
						   "_",	/* Fallback for non convertible characters */
						   NULL,
						   NULL,
						   NULL);
		string = g_strdup_printf ("Tracktitle=\t%s\n", encoded);
		g_free (encoded);
	}
	else
		string = strdup ("Tracktitle=\t\n");
	size = strlen (string);
	b_written = write (fd, string, size);
	g_free (string);
	if (b_written != size)
		goto error;

	string = g_strdup_printf ("Tracknumber=\t%i\n", index);
	size = strlen (string);
	b_written = write (fd, string, size);
	g_free (string);
	if (b_written != size)
		goto error;

	string = g_strdup_printf ("Trackstart=\t%i\n", start);
	size = strlen (string);
	b_written = write (fd, string, size);
	g_free (string);
	if (b_written != size)
		goto error;

	length = 0;
	rejilla_track_stream_get_length (REJILLA_TRACK_STREAM (track), &length);
	sectors = REJILLA_DURATION_TO_SECTORS (length);

	REJILLA_JOB_LOG (wodim, "got track length %lli %lli", length, sectors);
	string = g_strdup_printf ("Tracklength=\t%"G_GINT64_FORMAT", 0\n", sectors);
	size = strlen (string);
	b_written = write (fd, string, size);
	g_free (string);
	if (b_written != size)
		goto error;

	strcpy (buffer, "Pre-emphasis=\tno\n");
	size = strlen (buffer);
	b_written = write (fd, buffer, size);
	if (b_written != size)
		goto error;

	strcpy (buffer, "Channels=\t2\n");
	size = strlen (buffer);
	b_written = write (fd, buffer, size);
	if (b_written != size)
		goto error;

	strcpy (buffer, "Copy_permitted=\tyes\n");
	size = strlen (buffer);
	b_written = write (fd, buffer, size);
	if (b_written != size)
		goto error;

	strcpy (buffer, "Endianess=\tlittle\n");
	size = strlen (buffer);
	b_written = write (fd, buffer, size);
	if (b_written != size)
		goto error;

	strcpy (buffer, "Index=\t\t0\n");
	size = strlen (buffer);
	b_written = write (fd, buffer, size);
	if (b_written != size)
		goto error;

	/* NOTE: -1 here means no pregap */
	if (!last_track) {
		/* K3b does this (possibly to remove silence) */
		string = g_strdup_printf ("Index0=\t\t%"G_GINT64_FORMAT"\n",
					  sectors - 150);
	}
	else
		string = g_strdup_printf ("Index0=\t\t-1\n");

	size = strlen (string);
	b_written = write (fd, string, size);
	g_free (string);

	if (b_written != size)
		goto error;

	close (fd);

	if (argv)
		g_ptr_array_add (argv, path);
	else
		g_free (path);

	return REJILLA_BURN_OK;


error:
        errsv = errno;

	g_remove (path);
	g_free (path);

	REJILLA_JOB_LOG (wodim,
			 "The inf file can't be written : %s",
			 g_strerror (errsv));

	g_set_error (error,
		     REJILLA_BURN_ERROR,
		     REJILLA_BURN_ERROR_GENERAL,
		     _("An internal error occurred"));

	return REJILLA_BURN_ERR;
}

static RejillaBurnResult
rejilla_wodim_write_infs (RejillaWodim *wodim,
			   GPtrArray *argv,
			   GError **error)
{
	RejillaWodimPrivate *priv;
	RejillaBurnResult result;
	gchar *tmpdir = NULL;
	GSList *tracks;
	GSList *iter;
	gchar *album;
	gint index;
	gint start;

	priv = REJILLA_WODIM_PRIVATE (wodim);

	rejilla_job_get_audio_title (REJILLA_JOB (wodim), &album);
	rejilla_job_get_tracks (REJILLA_JOB (wodim), &tracks);
	index = 1;
	start = 0;

	for (iter = tracks; iter; iter = iter->next) {
		goffset sectors;
		RejillaTrack *track;

		track = iter->data;
		result = rejilla_wodim_write_inf (wodim,
						  argv,
						  track,
						  tmpdir,
						  album,
						  index,
						  start,
						  (iter->next == NULL),
						  error);
		if (result != REJILLA_BURN_OK)
			return result;

		index ++;
		sectors = 0;

		rejilla_track_get_size (track, &sectors, NULL);
		start += sectors;
	}

	g_free (album);
	g_free (tmpdir);

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_wodim_set_argv_record (RejillaWodim *wodim,
				GPtrArray *argv, 
				GError **error)
{
	guint speed;
	RejillaBurnFlag flags;
	RejillaWodimPrivate *priv;
	RejillaTrackType *type = NULL;

	priv = REJILLA_WODIM_PRIVATE (wodim);

	if (priv->immediate) {
		g_ptr_array_add (argv, g_strdup ("-immed"));
		g_ptr_array_add (argv, g_strdup_printf ("minbuf=%i", priv->minbuf));
	}

	if (rejilla_job_get_speed (REJILLA_JOB (wodim), &speed) == REJILLA_BURN_OK) {
		gchar *speed_str;

		speed_str = g_strdup_printf ("speed=%d", speed);
		g_ptr_array_add (argv, speed_str);
	}

	rejilla_job_get_flags (REJILLA_JOB (wodim), &flags);
	if (flags & REJILLA_BURN_FLAG_OVERBURN)
		g_ptr_array_add (argv, g_strdup ("-overburn"));
	if (flags & REJILLA_BURN_FLAG_BURNPROOF)
		g_ptr_array_add (argv, g_strdup ("driveropts=burnfree"));
	if (flags & REJILLA_BURN_FLAG_MULTI)
		g_ptr_array_add (argv, g_strdup ("-multi"));

	/* NOTE: This write mode is necessary for all CLONE images burning */
	if (flags & REJILLA_BURN_FLAG_RAW)
		g_ptr_array_add (argv, g_strdup ("-raw96r"));

	/* NOTE1: DAO can't be used if we're appending to a disc */
	/* NOTE2: CD-text cannot be written in tao mode (which is the default)
	 * NOTE3: when we don't want wodim to use stdin then we give the audio
	 * file on the command line. Otherwise we use the .inf */
	if (flags & REJILLA_BURN_FLAG_DAO)
		g_ptr_array_add (argv, g_strdup ("-dao"));

	type = rejilla_track_type_new ();
	rejilla_job_get_input_type (REJILLA_JOB (wodim), type);
	if (rejilla_job_get_fd_in (REJILLA_JOB (wodim), NULL) == REJILLA_BURN_OK) {
		RejillaBurnResult result;
		int buffer_size;
		goffset sectors;
		
		/* we need to know what is the type of the track (audio / data) */
		result = rejilla_job_get_input_type (REJILLA_JOB (wodim), type);
		if (result != REJILLA_BURN_OK) {
			rejilla_track_type_free (type);

			REJILLA_JOB_LOG (wodim, "Imager doesn't seem to be ready");
			g_set_error (error,
				     REJILLA_BURN_ERROR,
				     REJILLA_BURN_ERROR_GENERAL,
				     _("An internal error occurred"));
			return REJILLA_BURN_ERR;
		}
		
		/* ask the size */
		result = rejilla_job_get_session_output_size (REJILLA_JOB (wodim),
							      &sectors,
							      NULL);
		if (result != REJILLA_BURN_OK) {
			rejilla_track_type_free (type);

			REJILLA_JOB_LOG (wodim, "The size of the session cannot be retrieved");
			g_set_error (error,
				     REJILLA_BURN_ERROR,
				     REJILLA_BURN_ERROR_GENERAL,
				     _("An internal error occurred"));
			return REJILLA_BURN_ERR;
		}

		/* we create a buffer depending on the size 
		 * buffer 4m> < 64m and is 1/25th of size otherwise */
		buffer_size = sectors * 2352 / 1024 / 1024 / 25;
		if (buffer_size > 32)
			buffer_size = 32;
		else if (buffer_size < 4)
			buffer_size = 4;

		g_ptr_array_add (argv, g_strdup_printf ("fs=%im", buffer_size));
		if (rejilla_track_type_get_has_image (type)) {
			if (rejilla_track_type_get_image_format (type) == REJILLA_IMAGE_FORMAT_BIN) {
				g_ptr_array_add (argv, g_strdup_printf ("tsize=%"G_GINT64_FORMAT"s", sectors));

				g_ptr_array_add (argv, g_strdup ("-data"));
				g_ptr_array_add (argv, g_strdup ("-nopad"));
				g_ptr_array_add (argv, g_strdup ("-"));
			}
			else {
				rejilla_track_type_free (type);
				REJILLA_JOB_NOT_SUPPORTED (wodim);
			}
		}
		else if (rejilla_track_type_get_has_stream (type)) {
			/* NOTE: when we don't want wodim to use stdin then we
			 * give the audio file on the command line. Otherwise we
			 * use the .inf */
			g_ptr_array_add (argv, g_strdup ("-audio"));
			g_ptr_array_add (argv, g_strdup ("-useinfo"));
			g_ptr_array_add (argv, g_strdup ("-text"));

			result = rejilla_wodim_write_infs (wodim,
							   argv,
							   error);
			if (result != REJILLA_BURN_OK) {
				rejilla_track_type_free (type);
				return result;
			}
		}
		else {
			rejilla_track_type_free (type);
			REJILLA_JOB_NOT_SUPPORTED (wodim);
		}
	}
	else if (rejilla_track_type_get_has_stream (type)) {
		RejillaBurnResult result;
		GSList *tracks;

		g_ptr_array_add (argv, g_strdup ("fs=16m"));
		g_ptr_array_add (argv, g_strdup ("-audio"));
		g_ptr_array_add (argv, g_strdup ("-pad"));
	
		g_ptr_array_add (argv, g_strdup ("-useinfo"));
		g_ptr_array_add (argv, g_strdup ("-text"));

		result = rejilla_wodim_write_infs (wodim,
						   NULL,
						   error);
		if (result != REJILLA_BURN_OK) {
			rejilla_track_type_free (type);
			return result;
		}

		tracks = NULL;
		rejilla_job_get_tracks (REJILLA_JOB (wodim), &tracks);
		for (; tracks; tracks = tracks->next) {
			RejillaTrack *track;
			gchar *path;

			track = tracks->data;
			path = rejilla_track_stream_get_source (REJILLA_TRACK_STREAM (track), FALSE);
			g_ptr_array_add (argv, path);
		}
	}
	else if (rejilla_track_type_get_has_image (type)) {
		RejillaTrack *track = NULL;
		RejillaImageFormat format;

		rejilla_job_get_current_track (REJILLA_JOB (wodim), &track);
		if (!track) {
			rejilla_track_type_free (type);
			REJILLA_JOB_NOT_READY (wodim);
		}

		format = rejilla_track_type_get_image_format (type);
		if (format == REJILLA_IMAGE_FORMAT_NONE) {
			gchar *image_path;

			image_path = rejilla_track_image_get_source (REJILLA_TRACK_IMAGE (track), FALSE);
			if (!image_path) {
				rejilla_track_type_free (type);
				REJILLA_JOB_NOT_READY (wodim);
			}

			g_ptr_array_add (argv, g_strdup ("fs=16m"));
			g_ptr_array_add (argv, g_strdup ("-data"));
			g_ptr_array_add (argv, g_strdup ("-nopad"));
			g_ptr_array_add (argv, image_path);
		}
		else if (format == REJILLA_IMAGE_FORMAT_BIN) {
			gchar *isopath;

			isopath = rejilla_track_image_get_source (REJILLA_TRACK_IMAGE (track), FALSE);
			if (!isopath) {
				rejilla_track_type_free (type);
				REJILLA_JOB_NOT_READY (wodim);
			}

			g_ptr_array_add (argv, g_strdup ("fs=16m"));
			g_ptr_array_add (argv, g_strdup ("-data"));
			g_ptr_array_add (argv, g_strdup ("-nopad"));
			g_ptr_array_add (argv, isopath);
		}
		else if (format == REJILLA_IMAGE_FORMAT_CLONE) {
			gchar *rawpath;

			rawpath = rejilla_track_image_get_source (REJILLA_TRACK_IMAGE (track), FALSE);
			if (!rawpath) {
				rejilla_track_type_free (type);
				REJILLA_JOB_NOT_READY (wodim);
			}

			g_ptr_array_add (argv, g_strdup ("fs=16m"));
			g_ptr_array_add (argv, g_strdup ("-clone"));
			g_ptr_array_add (argv, rawpath);
		}
		else if (format == REJILLA_IMAGE_FORMAT_CUE) {
			gchar *cue_str;
			gchar *cuepath;
			gchar *parent;

			cuepath = rejilla_track_image_get_toc_source (REJILLA_TRACK_IMAGE (track), FALSE);
			if (!cuepath) {
				rejilla_track_type_free (type);
				REJILLA_JOB_NOT_READY (wodim);
			}

			parent = g_path_get_dirname (cuepath);
			rejilla_process_set_working_directory (REJILLA_PROCESS (wodim), parent);
			g_free (parent);

			/* we need to check endianness */
			if (rejilla_track_image_need_byte_swap (REJILLA_TRACK_IMAGE (track)))
				g_ptr_array_add (argv, g_strdup ("-swab"));

			g_ptr_array_add (argv, g_strdup ("fs=16m"));

			/* This is to make sure the CD-TEXT stuff gets written */
			g_ptr_array_add (argv, g_strdup ("-text"));

			cue_str = g_strdup_printf ("cuefile=%s", cuepath);
			g_ptr_array_add (argv, cue_str);
			g_free (cuepath);
		}
		else {
			rejilla_track_type_free (type);
			REJILLA_JOB_NOT_SUPPORTED (wodim);
		}
	}
	else {
		rejilla_track_type_free (type);
		REJILLA_JOB_NOT_SUPPORTED (wodim);
	}

	rejilla_track_type_free (type);
	rejilla_job_set_current_action (REJILLA_JOB (wodim),
					REJILLA_BURN_ACTION_START_RECORDING,
					NULL,
					FALSE);
	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_wodim_set_argv_blank (RejillaWodim *wodim, GPtrArray *argv)
{
	RejillaBurnResult result;
	RejillaBurnFlag flags;
	RejillaMedia media;

	rejilla_job_get_flags (REJILLA_JOB (wodim), &flags);

	result = rejilla_job_get_media (REJILLA_JOB (wodim), &media);
	if (result != REJILLA_BURN_OK)
		return result;

	if (!REJILLA_MEDIUM_IS (media, REJILLA_MEDIUM_DVDRW_PLUS)) {
		gchar *blank_str;

		blank_str = g_strdup_printf ("blank=%s",
					    (flags & REJILLA_BURN_FLAG_FAST_BLANK) ? "fast" : "all");
		g_ptr_array_add (argv, blank_str);
	}
	else if (media & REJILLA_MEDIUM_UNFORMATTED) {
		g_ptr_array_add (argv, g_strdup ("-format"));

		/* There are many options for this given by an option that is
		 * not documented: formattype=#type where type can be:
		 * - force: to force reformatting => problem cannot/doesn't want to reformat a disc already formated
		 * - full: to do a full formatting => problem only reformats what is not done yet
		 * - background: formatting in the background => problem cannot/doesn't want to reformat a disc already formated
		 * conclusion: we can only accept unformated media.
		 * The following option allows to do it more quickly:
		 * g_ptr_array_add (argv, g_strdup ("formattype=background");
		 */
				
	}
	else {
		guint speed;
		RejillaWodimPrivate *priv;

		priv = REJILLA_WODIM_PRIVATE (wodim);

		/* Since we can't reformat any already formatted DVD+RW, we 
		 * write 0s to it to blank. */
		if (priv->immediate) {
			g_ptr_array_add (argv, g_strdup ("-immed"));
			g_ptr_array_add (argv, g_strdup_printf ("minbuf=%i", priv->minbuf));
		}

		if (rejilla_job_get_speed (REJILLA_JOB (wodim), &speed) == REJILLA_BURN_OK) {
			gchar *speed_str;

			speed_str = g_strdup_printf ("speed=%d", speed);
			g_ptr_array_add (argv, speed_str);
		}

		if (!(flags & REJILLA_BURN_FLAG_FAST_BLANK)) {
			gint64 sectors = 0;
			RejillaMedium *medium;

			rejilla_job_get_medium (REJILLA_JOB (wodim), &medium);
			rejilla_medium_get_data_size (medium, NULL, &sectors);
			if (!sectors)
				rejilla_medium_get_capacity (medium, NULL, &sectors);

			g_ptr_array_add (argv, g_strdup_printf ("tsize=%"G_GINT64_FORMAT"s", sectors));
		}
		else	/* we set 512s because wodim complains otherwise */
			g_ptr_array_add (argv, g_strdup_printf ("tsize=512s"));

		g_ptr_array_add (argv, g_strdup ("fs=16m"));
		g_ptr_array_add (argv, g_strdup ("-data"));
		g_ptr_array_add (argv, g_strdup ("-nopad"));
		g_ptr_array_add (argv, g_strdup ("/dev/zero"));
	}

	rejilla_job_set_current_action (REJILLA_JOB (wodim),
					REJILLA_BURN_ACTION_BLANKING,
					NULL,
					FALSE);
	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_wodim_set_argv (RejillaProcess *process,
			 GPtrArray *argv,
			 GError **error)
{
	RejillaWodimPrivate *priv;
	RejillaBurnResult result;
	RejillaJobAction action;
	RejillaWodim *wodim;
	RejillaBurnFlag flags;
	gchar *dev_str;
	gchar *device;

	wodim = REJILLA_WODIM (process);
	priv = REJILLA_WODIM_PRIVATE (wodim);

	rejilla_job_get_action (REJILLA_JOB (wodim), &action);
	if (action == REJILLA_JOB_ACTION_SIZE)
		return REJILLA_BURN_NOT_SUPPORTED;

	g_ptr_array_add (argv, g_strdup ("wodim"));
	g_ptr_array_add (argv, g_strdup ("-v"));

	rejilla_job_get_device (REJILLA_JOB (wodim), &device);
	dev_str = g_strdup_printf ("dev=%s", device);
	g_ptr_array_add (argv, dev_str);
	g_free (device);

	rejilla_job_get_flags (REJILLA_JOB (wodim), &flags);
        if (flags & REJILLA_BURN_FLAG_DUMMY)
		g_ptr_array_add (argv, g_strdup ("-dummy"));

	/* There is a bug in wodim where if we set that it takes 15 more so 
	 * disable it as long as the bug remains. */

	/* if (flags & REJILLA_BURN_FLAG_NOGRACE)
	 *	g_ptr_array_add (argv, g_strdup ("gracetime=0"));
	 */

	if (action == REJILLA_JOB_ACTION_RECORD)
		result = rejilla_wodim_set_argv_record (wodim, argv, error);
	else if (action == REJILLA_JOB_ACTION_ERASE)
		result = rejilla_wodim_set_argv_blank (wodim, argv);
	else
		REJILLA_JOB_NOT_SUPPORTED (wodim);

	return result;	
}

static RejillaBurnResult
rejilla_wodim_post (RejillaJob *job)
{
	RejillaWodimPrivate *priv;
	GSList *iter;

	priv = REJILLA_WODIM_PRIVATE (job);
	for (iter = priv->infs; iter; iter = iter->next) {
		gchar *path;

		path = iter->data;
		g_remove (path);
		g_free (path);
	}

	g_slist_free (priv->infs);
	priv->infs = NULL;

	return rejilla_job_finished_session (job);
}

static void
rejilla_wodim_class_init (RejillaWodimClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	RejillaProcessClass *process_class = REJILLA_PROCESS_CLASS (klass);

	g_type_class_add_private (klass, sizeof (RejillaWodimPrivate));

	parent_class = g_type_class_peek_parent (klass);
	object_class->finalize = rejilla_wodim_finalize;

	process_class->stderr_func = rejilla_wodim_stderr_read;
	process_class->stdout_func = rejilla_wodim_stdout_read;
	process_class->set_argv = rejilla_wodim_set_argv;
	process_class->post = rejilla_wodim_post;
}

static void
rejilla_wodim_init (RejillaWodim *obj)
{
	GSettings *settings;
	RejillaWodimPrivate *priv;

	/* load our "configuration" */
	priv = REJILLA_WODIM_PRIVATE (obj);

	settings = g_settings_new (REJILLA_SCHEMA_CONFIG);

	priv->immediate = g_settings_get_boolean (settings, REJILLA_KEY_IMMEDIATE_FLAG);
	priv->minbuf = g_settings_get_int (settings, REJILLA_KEY_MINBUF_VALUE);
	if (priv->minbuf > 95 || priv->minbuf < 25)
		priv->minbuf = 30;

	g_object_unref (settings);
}

static void
rejilla_wodim_finalize (GObject *object)
{
	RejillaWodimPrivate *priv;
	GSList *iter;

	priv = REJILLA_WODIM_PRIVATE (object);

	for (iter = priv->infs; iter; iter = iter->next) {
		gchar *path;

		path = iter->data;
		g_remove (path);
		g_free (path);
	}

	g_slist_free (priv->infs);
	priv->infs = NULL;

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
rejilla_wodim_export_caps (RejillaPlugin *plugin)
{
	RejillaPluginConfOption *immed, *minbuf;
	const RejillaMedia media = REJILLA_MEDIUM_CD|
				   REJILLA_MEDIUM_WRITABLE|
				   REJILLA_MEDIUM_REWRITABLE|
				   REJILLA_MEDIUM_BLANK|
				   REJILLA_MEDIUM_APPENDABLE|
				   REJILLA_MEDIUM_HAS_AUDIO|
				   REJILLA_MEDIUM_HAS_DATA;

	/* tests failed with DVD-RW in restricted overwrite mode */
	const RejillaMedia dvd_media = REJILLA_MEDIUM_DVD|
				       REJILLA_MEDIUM_PLUS|
				       REJILLA_MEDIUM_SEQUENTIAL|
				       REJILLA_MEDIUM_WRITABLE|
				       REJILLA_MEDIUM_REWRITABLE|
				       REJILLA_MEDIUM_BLANK|
				       REJILLA_MEDIUM_APPENDABLE|
				       REJILLA_MEDIUM_HAS_DATA;
	const RejillaMedia media_rw = REJILLA_MEDIUM_CD|
				      REJILLA_MEDIUM_REWRITABLE|
				      REJILLA_MEDIUM_APPENDABLE|
				      REJILLA_MEDIUM_CLOSED|
				      REJILLA_MEDIUM_HAS_AUDIO|
				      REJILLA_MEDIUM_HAS_DATA|
				      REJILLA_MEDIUM_BLANK;
	GSList *output;
	GSList *input;

	/* NOTE: it seems that cdrecord can burn cue files on the fly */
	rejilla_plugin_define (plugin,
			       "wodim",
	                       NULL,
			       _("Burns, blanks and formats CDs and DVDs"),
			       "Philippe Rouquier",
			       0);

	/* for recording */
	input = rejilla_caps_image_new (REJILLA_PLUGIN_IO_ACCEPT_PIPE|
					REJILLA_PLUGIN_IO_ACCEPT_FILE,
					REJILLA_IMAGE_FORMAT_BIN);

	/* wodim can burn all DVDs (except DVD-RW restricted) when it's ISOs */
	output = rejilla_caps_disc_new (dvd_media);
	rejilla_plugin_link_caps (plugin, output, input);
	g_slist_free (output);

	/* All CD-R(W) */
	output = rejilla_caps_disc_new (media);
	rejilla_plugin_link_caps (plugin, output, input);
	g_slist_free (input);

	input = rejilla_caps_audio_new (REJILLA_PLUGIN_IO_ACCEPT_PIPE|
					REJILLA_PLUGIN_IO_ACCEPT_FILE,
					REJILLA_AUDIO_FORMAT_RAW|
					REJILLA_METADATA_INFO);

	rejilla_plugin_link_caps (plugin, output, input);
	g_slist_free (input);

	input = rejilla_caps_audio_new (REJILLA_PLUGIN_IO_ACCEPT_PIPE|
					REJILLA_PLUGIN_IO_ACCEPT_FILE,
					REJILLA_AUDIO_FORMAT_RAW);

	rejilla_plugin_link_caps (plugin, output, input);
	g_slist_free (output);
	g_slist_free (input);

	/* for CLONE and CUE type images, we only want blank CD-R(W) */
	output = rejilla_caps_disc_new (REJILLA_MEDIUM_CD|
					REJILLA_MEDIUM_WRITABLE|
					REJILLA_MEDIUM_REWRITABLE|
					REJILLA_MEDIUM_BLANK);

	input = rejilla_caps_image_new (REJILLA_PLUGIN_IO_ACCEPT_FILE,
					REJILLA_IMAGE_FORMAT_CUE|
					REJILLA_IMAGE_FORMAT_CLONE);

	rejilla_plugin_link_caps (plugin, output, input);
	g_slist_free (output);
	g_slist_free (input);

	/* Flags for CD (RW)s */
	REJILLA_PLUGIN_ADD_STANDARD_CDR_FLAGS (plugin, REJILLA_BURN_FLAG_NONE);
	REJILLA_PLUGIN_ADD_STANDARD_CDRW_FLAGS (plugin, REJILLA_BURN_FLAG_NONE);

	/* Apart from DAO it also supports RAW mode to burn CLONE images. This
	 * is a special mode for which there isn't any DUMMY burn possible */
	rejilla_plugin_set_flags (plugin,
				  REJILLA_MEDIUM_CD|
				  REJILLA_MEDIUM_WRITABLE|
				  REJILLA_MEDIUM_REWRITABLE|
				  REJILLA_MEDIUM_BLANK,
				  REJILLA_BURN_FLAG_RAW|
				  REJILLA_BURN_FLAG_BURNPROOF|
				  REJILLA_BURN_FLAG_OVERBURN|
				  REJILLA_BURN_FLAG_NOGRACE,
				  REJILLA_BURN_FLAG_NONE);

	/* For DVD-W and DVD-RW sequential
	 * NOTE: given the performed tests it seems that wodim should not be 
	 * used to start a multisession DVD-RW or even continue one. */
	rejilla_plugin_set_flags (plugin,
				  REJILLA_MEDIUM_DVD|
				  REJILLA_MEDIUM_SEQUENTIAL|
				  REJILLA_MEDIUM_WRITABLE|
				  REJILLA_MEDIUM_REWRITABLE|
				  REJILLA_MEDIUM_BLANK,
				  REJILLA_BURN_FLAG_DAO|
				  REJILLA_BURN_FLAG_BURNPROOF|
				  REJILLA_BURN_FLAG_DUMMY|
				  REJILLA_BURN_FLAG_NOGRACE,
				  REJILLA_BURN_FLAG_NONE);

	/* For DVD+R limited capabilites to blank media */
	rejilla_plugin_set_flags (plugin,
				  REJILLA_MEDIUM_DVDR_PLUS|
				  REJILLA_MEDIUM_BLANK,
				  REJILLA_BURN_FLAG_BURNPROOF|
				  REJILLA_BURN_FLAG_MULTI|
				  REJILLA_BURN_FLAG_NOGRACE,
				  REJILLA_BURN_FLAG_NONE);

	/* For DVD+RW: limited capabilities there are no MULTI possible
	 * NOTE: no UNFORMATTED here since wodim doesn't format them before*/
	rejilla_plugin_set_flags (plugin,
				  REJILLA_MEDIUM_DVDRW_PLUS|
				  REJILLA_MEDIUM_BLANK,
				  REJILLA_BURN_FLAG_NOGRACE,
				  REJILLA_BURN_FLAG_NONE);

	rejilla_plugin_set_flags (plugin,
				  REJILLA_MEDIUM_DVDRW_PLUS|
				  REJILLA_MEDIUM_APPENDABLE|
				  REJILLA_MEDIUM_CLOSED|
				  REJILLA_MEDIUM_HAS_DATA,
				  REJILLA_BURN_FLAG_NOGRACE,
				  REJILLA_BURN_FLAG_NONE);

	/* blanking/formatting caps and flags for +/sequential RW
	 * NOTE: restricted overwrite DVD-RW can't be formatted.
	 * moreover DVD+RW are formatted while DVD-RW sequential are blanked.
	 * NOTE: blanking DVD-RW doesn't work */
	output = rejilla_caps_disc_new (REJILLA_MEDIUM_DVDRW_PLUS|
					REJILLA_MEDIUM_CLOSED|
					REJILLA_MEDIUM_HAS_DATA|
					REJILLA_MEDIUM_UNFORMATTED|
					REJILLA_MEDIUM_BLANK);
	rejilla_plugin_blank_caps (plugin, output);
	g_slist_free (output);

	/* again DVD+RW don't support dummy */
	/* NOTE: wodim doesn't support formating already formated DVD+RWs. That
	 * is an error for it (stupid!). So sets only unformated DVDs. */
	rejilla_plugin_set_blank_flags (plugin,
					REJILLA_MEDIUM_DVDRW_PLUS|
					REJILLA_MEDIUM_CLOSED|
					REJILLA_MEDIUM_HAS_DATA|
					REJILLA_MEDIUM_UNFORMATTED|
					REJILLA_MEDIUM_BLANK,
					REJILLA_BURN_FLAG_NOGRACE|
					REJILLA_BURN_FLAG_FAST_BLANK,
					REJILLA_BURN_FLAG_NONE);

	/* for blanking (CDRWs) */
	output = rejilla_caps_disc_new (media_rw);
	rejilla_plugin_blank_caps (plugin, output);
	g_slist_free (output);

	rejilla_plugin_set_blank_flags (plugin,
					media_rw,
					REJILLA_BURN_FLAG_NOGRACE|
					REJILLA_BURN_FLAG_FAST_BLANK,
					REJILLA_BURN_FLAG_NONE);

	/* add some configure options */
	immed = rejilla_plugin_conf_option_new (REJILLA_KEY_IMMEDIATE_FLAG,
						_("Enable the \"-immed\" flag (see wodim manual)"),
						REJILLA_PLUGIN_OPTION_BOOL);
	minbuf = rejilla_plugin_conf_option_new (REJILLA_KEY_MINBUF_VALUE,
						 _("Minimum drive buffer fill ratio (in %) (see wodim manual):"),
						 REJILLA_PLUGIN_OPTION_INT);
	rejilla_plugin_conf_option_int_set_range (minbuf, 25, 95);

	rejilla_plugin_conf_option_bool_add_suboption (immed, minbuf);
	rejilla_plugin_add_conf_option (plugin, immed);

	rejilla_plugin_register_group (plugin, _(CDRKIT_DESCRIPTION));
}

G_MODULE_EXPORT void
rejilla_plugin_check_config (RejillaPlugin *plugin)
{
	gint version [3] = { 1, 1, 0};
	rejilla_plugin_test_app (plugin,
	                         "wodim",
	                         "--version",
	                         "Cdrecord-yelling-line-to-tell-frontends-to-use-it-like-version %*s \nWodim %d.%d.%d",
	                         version);
}
