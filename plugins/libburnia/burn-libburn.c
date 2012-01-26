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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gi18n-lib.h>
#include <gmodule.h>

#include <libburn/libburn.h>

#include "rejilla-units.h"
#include "burn-job.h"
#include "burn-debug.h"
#include "rejilla-plugin-registration.h"
#include "burn-libburn-common.h"
#include "burn-libburnia.h"
#include "rejilla-track-image.h"
#include "rejilla-track-stream.h"


#define REJILLA_TYPE_LIBBURN         (rejilla_libburn_get_type ())
#define REJILLA_LIBBURN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), REJILLA_TYPE_LIBBURN, RejillaLibburn))
#define REJILLA_LIBBURN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), REJILLA_TYPE_LIBBURN, RejillaLibburnClass))
#define REJILLA_IS_LIBBURN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), REJILLA_TYPE_LIBBURN))
#define REJILLA_IS_LIBBURN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), REJILLA_TYPE_LIBBURN))
#define REJILLA_LIBBURN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), REJILLA_TYPE_LIBBURN, RejillaLibburnClass))

REJILLA_PLUGIN_BOILERPLATE (RejillaLibburn, rejilla_libburn, REJILLA_TYPE_JOB, RejillaJob);

#define REJILLA_PVD_SIZE	32ULL * 2048ULL

struct _RejillaLibburnPrivate {
	RejillaLibburnCtx *ctx;

	/* This buffer is used to capture Primary Volume Descriptor for
	 * for overwrite media so as to "grow" the latter. */
	unsigned char *pvd;

	guint sig_handler:1;
};
typedef struct _RejillaLibburnPrivate RejillaLibburnPrivate;

#define REJILLA_LIBBURN_PRIVATE(o)  (G_TYPE_INSTANCE_GET_PRIVATE ((o), REJILLA_TYPE_LIBBURN, RejillaLibburnPrivate))

/**
 * taken from scsi-get-configuration.h
 */

typedef enum {
REJILLA_SCSI_PROF_DVD_RW_RESTRICTED	= 0x0013,
REJILLA_SCSI_PROF_DVD_RW_PLUS		= 0x001A,
} RejillaScsiProfile;

static GObjectClass *parent_class = NULL;

struct _RejillaLibburnSrcData {
	int fd;
	off_t size;

	/* That's for the primary volume descriptor used for overwrite media */
	int pvd_size;						/* in blocks */
	unsigned char *pvd;

	int read_pvd:1;
};
typedef struct _RejillaLibburnSrcData RejillaLibburnSrcData;

static void
rejilla_libburn_src_free_data (struct burn_source *src)
{
	RejillaLibburnSrcData *data;

	data = src->data;
	close (data->fd);
	g_free (data);
}

static off_t
rejilla_libburn_src_get_size (struct burn_source *src)
{
	RejillaLibburnSrcData *data;

	data = src->data;
	return data->size;
}

static int
rejilla_libburn_src_set_size (struct burn_source *src,
			      off_t size)
{
	RejillaLibburnSrcData *data;

	data = src->data;
	data->size = size;
	return 1;
}

/**
 * This is a copy from burn-volume.c
 */

struct _RejillaVolDesc {
	guchar type;
	gchar id			[5];
	guchar version;
};
typedef struct _RejillaVolDesc RejillaVolDesc;

static int
rejilla_libburn_src_read_xt (struct burn_source *src,
			     unsigned char *buffer,
			     int size)
{
	int total;
	RejillaLibburnSrcData *data;

	data = src->data;

	total = 0;
	while (total < size) {
		int bytes;

		bytes = read (data->fd, buffer + total, size - total);
		if (bytes < 0)
			return -1;

		if (!bytes)
			break;

		total += bytes;
	}

	/* copy the primary volume descriptor if a buffer is provided */
	if (data->pvd
	&& !data->read_pvd
	&&  data->pvd_size < REJILLA_PVD_SIZE) {
		unsigned char *current_pvd;
		int i;

		current_pvd = data->pvd + data->pvd_size;

		/* read volume descriptors until we reach the end of the
		 * buffer or find a volume descriptor set end. */
		for (i = 0; (i << 11) < size && data->pvd_size + (i << 11) < REJILLA_PVD_SIZE; i ++) {
			RejillaVolDesc *desc;

			/* No need to check the first 16 blocks */
			if ((data->pvd_size >> 11) + i < 16)
				continue;

			desc = (RejillaVolDesc *) (buffer + sizeof (RejillaVolDesc) * i);
			if (desc->type == 255) {
				data->read_pvd = 1;
				REJILLA_BURN_LOG ("found volume descriptor set end");
				break;
			}
		}

		memcpy (current_pvd, buffer, i << 11);
		data->pvd_size += i << 11;
	}

	return total;
}

static struct burn_source *
rejilla_libburn_create_fd_source (int fd,
				  gint64 size,
				  unsigned char *pvd)
{
	struct burn_source *src;
	RejillaLibburnSrcData *data;

	data = g_new0 (RejillaLibburnSrcData, 1);
	data->fd = fd;
	data->size = size;
	data->pvd = pvd;

	/* FIXME: this could be wrapped into a fifo source to get a smoother
	 * data delivery. But that means another thread ... */
	src = g_new0 (struct burn_source, 1);
	src->version = 1;
	src->refcount = 1;
	src->read_xt = rejilla_libburn_src_read_xt;
	src->get_size = rejilla_libburn_src_get_size;
	src->set_size = rejilla_libburn_src_set_size;
	src->free_data = rejilla_libburn_src_free_data;
	src->data = data;

	return src;
}

static RejillaBurnResult
rejilla_libburn_add_track (struct burn_session *session,
			   struct burn_track *track,
			   struct burn_source *src,
			   gint mode,
			   GError **error)
{
	if (burn_track_set_source (track, src) != BURN_SOURCE_OK) {
		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_GENERAL,
			     _("libburn track could not be created"));
		return REJILLA_BURN_ERR;
	}

	if (!burn_session_add_track (session, track, BURN_POS_END)) {
		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_GENERAL,
			     _("libburn track could not be created"));
		return REJILLA_BURN_ERR;
	}

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_libburn_add_fd_track (struct burn_session *session,
			      int fd,
			      gint mode,
			      gint64 size,
			      unsigned char *pvd,
			      GError **error)
{
	struct burn_source *src;
	struct burn_track *track;
	RejillaBurnResult result;

	track = burn_track_create ();
	burn_track_define_data (track, 0, 0, 0, mode);

	src = rejilla_libburn_create_fd_source (fd, size, pvd);
	result = rejilla_libburn_add_track (session, track, src, mode, error);

	burn_source_free (src);
	burn_track_free (track);

	return result;
}

static RejillaBurnResult
rejilla_libburn_add_file_track (struct burn_session *session,
				const gchar *path,
				gint mode,
				off_t size,
				unsigned char *pvd,
				GError **error)
{
	int fd;

	fd = open (path, O_RDONLY);
	if (fd == -1) {
		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_GENERAL,
			     "%s",
			     g_strerror (errno));
		return REJILLA_BURN_ERR;
	}

	return rejilla_libburn_add_fd_track (session, fd, mode, size, pvd, error);
}

static RejillaBurnResult
rejilla_libburn_setup_session_fd (RejillaLibburn *self,
			          struct burn_session *session,
			          GError **error)
{
	int fd;
	goffset bytes = 0;
	RejillaLibburnPrivate *priv;
	RejillaTrackType *type = NULL;
	RejillaBurnResult result = REJILLA_BURN_OK;

	priv = REJILLA_LIBBURN_PRIVATE (self);

	rejilla_job_get_fd_in (REJILLA_JOB (self), &fd);

	/* need to find out what type of track the imager will output */
	type = rejilla_track_type_new ();
	rejilla_job_get_input_type (REJILLA_JOB (self), type);

	if (rejilla_track_type_get_has_image (type)) {
		gint mode;

		/* FIXME: implement other IMAGE types */
		if (rejilla_track_type_get_image_format (type) == REJILLA_IMAGE_FORMAT_BIN)
			mode = BURN_MODE1;
		else
			mode = BURN_MODE1|BURN_MODE_RAW|BURN_SUBCODE_R96;

		rejilla_track_type_free (type);

		rejilla_job_get_session_output_size (REJILLA_JOB (self),
						     NULL,
						     &bytes);

		result = rejilla_libburn_add_fd_track (session,
						       fd,
						       mode,
						       bytes,
						       priv->pvd,
						       error);
	}
	else if (rejilla_track_type_get_has_stream (type)) {
		GSList *tracks;
		guint64 length = 0;

		rejilla_track_type_free (type);

		rejilla_job_get_tracks (REJILLA_JOB (self), &tracks);
		for (; tracks; tracks = tracks->next) {
			RejillaTrack *track;

			track = tracks->data;
			rejilla_track_stream_get_length (REJILLA_TRACK_STREAM (track), &length);
			bytes = REJILLA_DURATION_TO_BYTES (length);

			/* we dup the descriptor so the same 
			 * will be shared by all tracks */
			result = rejilla_libburn_add_fd_track (session,
							       dup (fd),
							       BURN_AUDIO,
							       bytes,
							       NULL,
							       error);
			if (result != REJILLA_BURN_OK)
				return result;
		}
	}
	else
		REJILLA_JOB_NOT_SUPPORTED (self);

	return result;
}

static RejillaBurnResult
rejilla_libburn_setup_session_file (RejillaLibburn *self, 
				    struct burn_session *session,
				    GError **error)
{
	RejillaLibburnPrivate *priv;
	RejillaBurnResult result;
	GSList *tracks = NULL;

	priv = REJILLA_LIBBURN_PRIVATE (self);

	/* create the track(s) */
	result = REJILLA_BURN_OK;
	rejilla_job_get_tracks (REJILLA_JOB (self), &tracks);
	for (; tracks; tracks = tracks->next) {
		RejillaTrack *track;

		track = tracks->data;
		if (REJILLA_IS_TRACK_STREAM (track)) {
			gchar *audiopath;
			guint64 size;

			audiopath = rejilla_track_stream_get_source (REJILLA_TRACK_STREAM (track), FALSE);
			rejilla_track_stream_get_length (REJILLA_TRACK_STREAM (track), &size);
			size = REJILLA_DURATION_TO_BYTES (size);

			result = rejilla_libburn_add_file_track (session,
								 audiopath,
								 BURN_AUDIO,
								 size,
								 NULL,
								 error);
			if (result != REJILLA_BURN_OK)
				break;
		}
		else if (REJILLA_IS_TRACK_IMAGE (track)) {
			RejillaImageFormat format;
			gchar *imagepath;
			goffset bytes;
			gint mode;

			format = rejilla_track_image_get_format (REJILLA_TRACK_IMAGE (track));
			if (format == REJILLA_IMAGE_FORMAT_BIN) {
				mode = BURN_MODE1;
				imagepath = rejilla_track_image_get_source (REJILLA_TRACK_IMAGE (track), FALSE);
			}
			else if (format == REJILLA_IMAGE_FORMAT_NONE) {
				mode = BURN_MODE1;
				imagepath = rejilla_track_image_get_source (REJILLA_TRACK_IMAGE (track), FALSE);
			}
			else
				REJILLA_JOB_NOT_SUPPORTED (self);

			if (!imagepath)
				return REJILLA_BURN_ERR;

			result = rejilla_track_get_size (track,
							 NULL,
							 &bytes);
			if (result != REJILLA_BURN_OK)
				return REJILLA_BURN_ERR;

			result = rejilla_libburn_add_file_track (session,
								 imagepath,
								 mode,
								 bytes,
								 priv->pvd,
								 error);
			g_free (imagepath);
		}
		else
			REJILLA_JOB_NOT_SUPPORTED (self);
	}

	return result;
}

static RejillaBurnResult
rejilla_libburn_create_disc (RejillaLibburn *self,
			     struct burn_disc **retval,
			     GError **error)
{
	struct burn_disc *disc;
	RejillaBurnResult result;
	struct burn_session *session;

	/* set the source image */
	disc = burn_disc_create ();

	/* create the session */
	session = burn_session_create ();
	burn_disc_add_session (disc, session, BURN_POS_END);
	burn_session_free (session);

	if (rejilla_job_get_fd_in (REJILLA_JOB (self), NULL) == REJILLA_BURN_OK)
		result = rejilla_libburn_setup_session_fd (self, session, error);
	else
		result = rejilla_libburn_setup_session_file (self, session, error);

	if (result != REJILLA_BURN_OK) {
		burn_disc_free (disc);
		return result;
	}

	*retval = disc;
	return result;
}

static RejillaBurnResult
rejilla_libburn_start_record (RejillaLibburn *self,
			      GError **error)
{
	guint64 rate;
	goffset blocks = 0;
	RejillaMedia media;
	RejillaBurnFlag flags;
	RejillaBurnResult result;
	RejillaLibburnPrivate *priv;
	struct burn_write_opts *opts;
	gchar reason [BURN_REASONS_LEN];

	priv = REJILLA_LIBBURN_PRIVATE (self);

	/* if appending a DVD+-RW get PVD */
	rejilla_job_get_flags (REJILLA_JOB (self), &flags);
	rejilla_job_get_media (REJILLA_JOB (self), &media);

	if (flags & (REJILLA_BURN_FLAG_MERGE|REJILLA_BURN_FLAG_APPEND)
	&&  REJILLA_MEDIUM_RANDOM_WRITABLE (media)
	&& (media & REJILLA_MEDIUM_HAS_DATA))
		priv->pvd = g_new0 (unsigned char, REJILLA_PVD_SIZE);

	result = rejilla_libburn_create_disc (self, &priv->ctx->disc, error);
	if (result != REJILLA_BURN_OK)
		return result;

	/* Note: we don't need to call burn_drive_get_status nor
	 * burn_disc_get_status since we took care of the disc
	 * checking thing earlier ourselves. Now there is a proper
	 * disc and tray is locked. */
	opts = burn_write_opts_new (priv->ctx->drive);

	/* only turn this on for CDs */
	if ((media & REJILLA_MEDIUM_CD) != 0)
		burn_write_opts_set_perform_opc (opts, 0);
	else
		burn_write_opts_set_perform_opc (opts, 0);

	if (flags & REJILLA_BURN_FLAG_DAO)
		burn_write_opts_set_write_type (opts,
						BURN_WRITE_SAO,
						BURN_BLOCK_SAO);
	else {
		burn_write_opts_set_write_type (opts,
						BURN_WRITE_TAO,
						BURN_BLOCK_MODE1);

		/* we also set the start block to write from if MERGE is set.
		 * That only for random writable media; for other media libburn
		 * handles all by himself where to start writing. */
		if (REJILLA_MEDIUM_RANDOM_WRITABLE (media)
		&& (flags & REJILLA_BURN_FLAG_MERGE)) {
			goffset address = 0;

			rejilla_job_get_next_writable_address (REJILLA_JOB (self), &address);

			REJILLA_JOB_LOG (self, "Starting to write at block = %lli and byte %lli", address, address * 2048);
			burn_write_opts_set_start_byte (opts, address * 2048);
		}
	}

	if (!REJILLA_MEDIUM_RANDOM_WRITABLE (media)) {
		REJILLA_JOB_LOG (REJILLA_JOB (self), "Setting multi %i", (flags & REJILLA_BURN_FLAG_MULTI) != 0);
		burn_write_opts_set_multi (opts, (flags & REJILLA_BURN_FLAG_MULTI) != 0);
	}

	burn_write_opts_set_underrun_proof (opts, (flags & REJILLA_BURN_FLAG_BURNPROOF) != 0);
	REJILLA_JOB_LOG (REJILLA_JOB (self), "Setting burnproof %i", (flags & REJILLA_BURN_FLAG_BURNPROOF) != 0);

	burn_write_opts_set_simulate (opts, (flags & REJILLA_BURN_FLAG_DUMMY) != 0);
	REJILLA_JOB_LOG (REJILLA_JOB (self), "Setting dummy %i", (flags & REJILLA_BURN_FLAG_DUMMY) != 0);

	rejilla_job_get_rate (REJILLA_JOB (self), &rate);
	burn_drive_set_speed (priv->ctx->drive, rate, 0);

	if (burn_precheck_write (opts, priv->ctx->disc, reason, 0) < 1) {
		REJILLA_JOB_LOG (REJILLA_JOB (self), "Precheck failed %s", reason);
		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_GENERAL,
			     reason);
		return REJILLA_BURN_ERR;
	}

	/* If we're writing to a disc remember that the session can't be under
	 * 300 sectors (= 614400 bytes) */
	rejilla_job_get_session_output_size (REJILLA_JOB (self), &blocks, NULL);
	if (blocks < 300)
		rejilla_job_set_output_size_for_current_track (REJILLA_JOB (self),
							       300L - blocks,
							       614400L - blocks * 2048);

	if (!priv->sig_handler) {
		burn_set_signal_handling ("rejilla", NULL, 0);
		priv->sig_handler = 1;
	}

	burn_disc_write (opts, priv->ctx->disc);
	burn_write_opts_free (opts);

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_libburn_start_erase (RejillaLibburn *self,
			     GError **error)
{
	char reasons [BURN_REASONS_LEN];
	struct burn_session *session;
	struct burn_write_opts *opts;
	RejillaLibburnPrivate *priv;
	RejillaBurnResult result;
	RejillaBurnFlag flags;
	char prof_name [80];
	int profile;
	int fd;

	priv = REJILLA_LIBBURN_PRIVATE (self);
	if (burn_disc_get_profile (priv->ctx->drive, &profile, prof_name) <= 0) {
		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_MEDIUM_INVALID,
			     _("The disc is not supported"));
		return REJILLA_BURN_ERR;
	}

	/* here we try to respect the current formatting of DVD-RW. For 
	 * overwritable media fast option means erase the first 64 Kib
	 * and long a forced reformatting */
	rejilla_job_get_flags (REJILLA_JOB (self), &flags);
	if (profile == REJILLA_SCSI_PROF_DVD_RW_RESTRICTED) {
		if (!(flags & REJILLA_BURN_FLAG_FAST_BLANK)) {
			/* leave libburn choose the best format */
			if (!priv->sig_handler) {
				burn_set_signal_handling ("rejilla", NULL, 0);
				priv->sig_handler = 1;
			}

			burn_disc_format (priv->ctx->drive,
					  (off_t) 0,
					  (1 << 4));
			return REJILLA_BURN_OK;
		}
	}
	else if (profile == REJILLA_SCSI_PROF_DVD_RW_PLUS) {
		if (!(flags & REJILLA_BURN_FLAG_FAST_BLANK)) {
			/* Bit 2 is for format max available size
			 * Bit 4 is enforce (re)-format if needed
			 * 0x26 is DVD+RW format is to be set from bit 8
			 * in the latter case bit 7 needs to be set as 
			 * well. */
			if (!priv->sig_handler) {
				burn_set_signal_handling ("rejilla", NULL, 0);
				priv->sig_handler = 1;
			}

			burn_disc_format (priv->ctx->drive,
					  (off_t) 0,
					  (1 << 2)|(1 << 4));
			return REJILLA_BURN_OK;
		}
	}
	else if (burn_disc_erasable (priv->ctx->drive)) {
		/* This is mainly for CDRW and sequential DVD-RW */
		if (!priv->sig_handler) {
			burn_set_signal_handling ("rejilla", NULL, 0);
			priv->sig_handler = 1;
		}

		/* NOTE: for an unknown reason (to me)
		 * libburn when minimally blanking a DVD-RW
		 * will only allow to write to it with DAO 
		 * afterwards... */
		burn_disc_erase (priv->ctx->drive, (flags & REJILLA_BURN_FLAG_FAST_BLANK) != 0);
		return REJILLA_BURN_OK;
	}
	else
		REJILLA_JOB_NOT_SUPPORTED (self);

	/* This is the "fast option": basically we only write 64 Kib of 0 from
	 * /dev/null. If we reached that part it means we're dealing with
	 * overwrite media. */
	fd = open ("/dev/null", O_RDONLY);
	if (fd == -1) {
		int errnum = errno;

		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_GENERAL,
			     /* Translators: first %s is the filename, second %s is the error
			      * generated from errno */
			     _("\"%s\" could not be opened (%s)"),
			     "/dev/null",
			     g_strerror (errnum));
		return REJILLA_BURN_ERR;
	}

	priv->ctx->disc = burn_disc_create ();

	/* create the session */
	session = burn_session_create ();
	burn_disc_add_session (priv->ctx->disc, session, BURN_POS_END);
	burn_session_free (session);

	result = rejilla_libburn_add_fd_track (session,
					       fd,
					       BURN_MODE1,
					       65536,		/* 32 blocks */
					       priv->pvd,
					       error);
	close (fd);

	opts = burn_write_opts_new (priv->ctx->drive);
	burn_write_opts_set_perform_opc (opts, 0);
	burn_write_opts_set_underrun_proof (opts, 1);
	burn_write_opts_set_simulate (opts, (flags & REJILLA_BURN_FLAG_DUMMY));

	burn_drive_set_speed (priv->ctx->drive, burn_drive_get_write_speed (priv->ctx->drive), 0);
	burn_write_opts_set_write_type (opts,
					BURN_WRITE_TAO,
					BURN_BLOCK_MODE1);

	if (burn_precheck_write (opts, priv->ctx->disc, reasons, 0) <= 0) {
		burn_write_opts_free (opts);

		g_set_error (error,
			     REJILLA_BURN_ERROR,
			     REJILLA_BURN_ERROR_GENERAL,
			     /* Translators: %s is the error returned by libburn */
			     _("An internal error occurred (%s)"),
			     reasons);
		return REJILLA_BURN_ERR;
	}

	if (!priv->sig_handler) {
		burn_set_signal_handling ("rejilla", NULL, 0);
		priv->sig_handler = 1;
	}

	burn_disc_write (opts, priv->ctx->disc);
	burn_write_opts_free (opts);

	return result;
}

static RejillaBurnResult
rejilla_libburn_start (RejillaJob *job,
		       GError **error)
{
	RejillaLibburn *self;
	RejillaJobAction action;
	RejillaBurnResult result;
	RejillaLibburnPrivate *priv;

	self = REJILLA_LIBBURN (job);
	priv = REJILLA_LIBBURN_PRIVATE (self);

	rejilla_job_get_action (job, &action);
	if (action == REJILLA_JOB_ACTION_RECORD) {
		GError *ret_error = NULL;

		/* TRUE is a context that helps to adapt action
		 * messages like for DVD+RW which need a
		 * pre-formatting before actually writing
		 * and without this we would not know if
		 * we are actually formatting or just pre-
		 * formatting == starting to record */
		priv->ctx = rejilla_libburn_common_ctx_new (job, TRUE, &ret_error);
		if (!priv->ctx) {
			if (ret_error && ret_error->code == REJILLA_BURN_ERROR_DRIVE_BUSY) {
				g_propagate_error (error, ret_error);
				return REJILLA_BURN_RETRY;
			}

			if (error)
				g_propagate_error (error, ret_error);
			return REJILLA_BURN_ERR;
		}

		result = rejilla_libburn_start_record (self, error);
		if (result != REJILLA_BURN_OK)
			return result;

		rejilla_job_set_current_action (job,
						REJILLA_BURN_ACTION_START_RECORDING,
						NULL,
						FALSE);
	}
	else if (action == REJILLA_JOB_ACTION_ERASE) {
		GError *ret_error = NULL;

		priv->ctx = rejilla_libburn_common_ctx_new (job, FALSE, &ret_error);
		if (!priv->ctx) {
			if (ret_error && ret_error->code == REJILLA_BURN_ERROR_DRIVE_BUSY) {
				g_propagate_error (error, ret_error);
				return REJILLA_BURN_RETRY;
			}

			if (error)
				g_propagate_error (error, ret_error);
			return REJILLA_BURN_ERR;
		}

		result = rejilla_libburn_start_erase (self, error);
		if (result != REJILLA_BURN_OK)
			return result;

		rejilla_job_set_current_action (job,
						REJILLA_BURN_ACTION_BLANKING,
						NULL,
						FALSE);
	}
	else
		REJILLA_JOB_NOT_SUPPORTED (self);

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_libburn_stop (RejillaJob *job,
		      GError **error)
{
	RejillaLibburn *self;
	RejillaLibburnPrivate *priv;

	self = REJILLA_LIBBURN (job);
	priv = REJILLA_LIBBURN_PRIVATE (self);

	if (priv->sig_handler) {
		priv->sig_handler = 0;
		burn_set_signal_handling (NULL, NULL, 1);
	}

	if (priv->ctx) {
		rejilla_libburn_common_ctx_free (priv->ctx);
		priv->ctx = NULL;
	}

	if (priv->pvd) {
		g_free (priv->pvd);
		priv->pvd = NULL;
	}

	if (REJILLA_JOB_CLASS (parent_class)->stop)
		REJILLA_JOB_CLASS (parent_class)->stop (job, error);

	return REJILLA_BURN_OK;
}

static RejillaBurnResult
rejilla_libburn_clock_tick (RejillaJob *job)
{
	RejillaLibburnPrivate *priv;
	RejillaBurnResult result;
	int ret;

	priv = REJILLA_LIBBURN_PRIVATE (job);
	result = rejilla_libburn_common_status (job, priv->ctx);

	if (result != REJILLA_BURN_OK)
		return REJILLA_BURN_OK;

	/* Double check that everything went well */
	if (!burn_drive_wrote_well (priv->ctx->drive)) {
		REJILLA_JOB_LOG (job, "Something went wrong");
		rejilla_job_error (job,
				   g_error_new (REJILLA_BURN_ERROR,
						REJILLA_BURN_ERROR_WRITE_MEDIUM,
						_("An error occurred while writing to disc")));
		return REJILLA_BURN_OK;
	}

	/* That's finished */
	if (!priv->pvd) {
		rejilla_job_set_dangerous (job, FALSE);
		rejilla_job_finished_session (job);
		return REJILLA_BURN_OK;
	}

	/* In case we append data to a DVD+RW or DVD-RW
	 * (restricted overwrite) medium , we're not
	 * done since we need to overwrite the primary
	 * volume descriptor at sector 0.
	 * NOTE: This is a synchronous call but given the size of the buffer
	 * that shouldn't block.
	 * NOTE 2: in source we read the volume descriptors until we reached
	 * either the end of the buffer or the volume descriptor set end. That's
	 * kind of useless since for a DVD 16 blocks are written at a time. */
	REJILLA_JOB_LOG (job, "Starting to overwrite primary volume descriptor");
	ret = burn_random_access_write (priv->ctx->drive,
					0,
					(char*)priv->pvd,
					REJILLA_PVD_SIZE,
					0);
	if (ret != 1) {
		REJILLA_JOB_LOG (job, "Random write failed");
		rejilla_job_error (job,
				   g_error_new (REJILLA_BURN_ERROR,
						REJILLA_BURN_ERROR_WRITE_MEDIUM,
						_("An error occurred while writing to disc")));
		return REJILLA_BURN_OK;
	}

	rejilla_job_set_dangerous (job, FALSE);
	rejilla_job_finished_session (job);

	return REJILLA_BURN_OK;
}

static void
rejilla_libburn_class_init (RejillaLibburnClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);
	RejillaJobClass *job_class = REJILLA_JOB_CLASS (klass);

	g_type_class_add_private (klass, sizeof (RejillaLibburnPrivate));

	parent_class = g_type_class_peek_parent(klass);
	object_class->finalize = rejilla_libburn_finalize;

	job_class->start = rejilla_libburn_start;
	job_class->stop = rejilla_libburn_stop;
	job_class->clock_tick = rejilla_libburn_clock_tick;
}

static void
rejilla_libburn_init (RejillaLibburn *obj)
{

}

static void
rejilla_libburn_finalize (GObject *object)
{
	RejillaLibburn *cobj;
	RejillaLibburnPrivate *priv;

	cobj = REJILLA_LIBBURN (object);
	priv = REJILLA_LIBBURN_PRIVATE (cobj);

	if (priv->ctx) {
		rejilla_libburn_common_ctx_free (priv->ctx);
		priv->ctx = NULL;
	}

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
rejilla_libburn_export_caps (RejillaPlugin *plugin)
{
	const RejillaMedia media_cd = REJILLA_MEDIUM_CD|
				      REJILLA_MEDIUM_REWRITABLE|
				      REJILLA_MEDIUM_WRITABLE|
				      REJILLA_MEDIUM_BLANK|
				      REJILLA_MEDIUM_APPENDABLE|
				      REJILLA_MEDIUM_HAS_AUDIO|
				      REJILLA_MEDIUM_HAS_DATA;
	const RejillaMedia media_dvd_w = REJILLA_MEDIUM_DVD|
					 REJILLA_MEDIUM_PLUS|
					 REJILLA_MEDIUM_SEQUENTIAL|
					 REJILLA_MEDIUM_WRITABLE|
					 REJILLA_MEDIUM_APPENDABLE|
					 REJILLA_MEDIUM_HAS_DATA|
					 REJILLA_MEDIUM_BLANK;
	const RejillaMedia media_dvd_rw = REJILLA_MEDIUM_DVD|
					  REJILLA_MEDIUM_SEQUENTIAL|
					  REJILLA_MEDIUM_REWRITABLE|
					  REJILLA_MEDIUM_APPENDABLE|
					  REJILLA_MEDIUM_HAS_DATA|
					  REJILLA_MEDIUM_BLANK;
	const RejillaMedia media_dvd_rw_plus = REJILLA_MEDIUM_DVD|
					       REJILLA_MEDIUM_DUAL_L|
					       REJILLA_MEDIUM_PLUS|
					       REJILLA_MEDIUM_RESTRICTED|
					       REJILLA_MEDIUM_REWRITABLE|
					       REJILLA_MEDIUM_UNFORMATTED|
					       REJILLA_MEDIUM_BLANK|
					       REJILLA_MEDIUM_APPENDABLE|
					       REJILLA_MEDIUM_CLOSED|
					       REJILLA_MEDIUM_HAS_DATA;
	GSList *output;
	GSList *input;

	rejilla_plugin_define (plugin,
			       "libburn",
	                       NULL,
			       _("Burns, blanks and formats CDs, DVDs and BDs"),
			       "Philippe Rouquier",
			       15);

	/* libburn has no OVERBURN capabilities */

	/* CD(R)W */
	/* Use DAO for first session since AUDIO need it to write CD-TEXT
	 * Though libburn is unable to write CD-TEXT.... */
	/* Note: when burning multiple tracks to a CD (like audio for example)
	 * in dummy mode with TAO libburn will fail probably because it does
	 * not use a correct next writable address for the second track (it uses
	 * the same as for track #1). So remove dummy mode.
	 * This is probably the same reason why it fails at merging another
	 * session to a data CD in dummy mode. */
	REJILLA_PLUGIN_ADD_STANDARD_CDR_FLAGS (plugin,
	                                       REJILLA_BURN_FLAG_OVERBURN|
	                                       REJILLA_BURN_FLAG_DUMMY);
	REJILLA_PLUGIN_ADD_STANDARD_CDRW_FLAGS (plugin,
	                                        REJILLA_BURN_FLAG_OVERBURN|
	                                        REJILLA_BURN_FLAG_DUMMY);

	/* audio support for CDs only */
	input = rejilla_caps_audio_new (REJILLA_PLUGIN_IO_ACCEPT_PIPE|
					REJILLA_PLUGIN_IO_ACCEPT_FILE,
					REJILLA_AUDIO_FORMAT_RAW_LITTLE_ENDIAN);
	
	output = rejilla_caps_disc_new (media_cd);
	rejilla_plugin_link_caps (plugin, output, input);
	g_slist_free (input);

	/* Image support for CDs ... */
	input = rejilla_caps_image_new (REJILLA_PLUGIN_IO_ACCEPT_PIPE|
					REJILLA_PLUGIN_IO_ACCEPT_FILE,
					REJILLA_IMAGE_FORMAT_BIN);

	rejilla_plugin_link_caps (plugin, output, input);
	g_slist_free (output);

	/* ... and DVD-R and DVD+R ... */
	output = rejilla_caps_disc_new (media_dvd_w);
	rejilla_plugin_link_caps (plugin, output, input);
	g_slist_free (output);

	REJILLA_PLUGIN_ADD_STANDARD_DVDR_PLUS_FLAGS (plugin, REJILLA_BURN_FLAG_NONE);
	REJILLA_PLUGIN_ADD_STANDARD_DVDR_FLAGS (plugin, REJILLA_BURN_FLAG_NONE);

	/* ... and DVD-RW (sequential) */
	output = rejilla_caps_disc_new (media_dvd_rw);
	rejilla_plugin_link_caps (plugin, output, input);
	g_slist_free (output);

	REJILLA_PLUGIN_ADD_STANDARD_DVDRW_FLAGS (plugin, REJILLA_BURN_FLAG_NONE);

	/* for DVD+/-RW restricted */
	output = rejilla_caps_disc_new (media_dvd_rw_plus);
	rejilla_plugin_link_caps (plugin, output, input);
	g_slist_free (output);
	g_slist_free (input);

	REJILLA_PLUGIN_ADD_STANDARD_DVDRW_RESTRICTED_FLAGS (plugin, REJILLA_BURN_FLAG_NONE);
	REJILLA_PLUGIN_ADD_STANDARD_DVDRW_PLUS_FLAGS (plugin, REJILLA_BURN_FLAG_NONE);

	/* add blank caps */
	output = rejilla_caps_disc_new (REJILLA_MEDIUM_CD|
					REJILLA_MEDIUM_REWRITABLE|
					REJILLA_MEDIUM_APPENDABLE|
					REJILLA_MEDIUM_CLOSED|
					REJILLA_MEDIUM_HAS_DATA|
					REJILLA_MEDIUM_HAS_AUDIO|
					REJILLA_MEDIUM_BLANK);
	rejilla_plugin_blank_caps (plugin, output);
	g_slist_free (output);

	output = rejilla_caps_disc_new (REJILLA_MEDIUM_DVD|
					REJILLA_MEDIUM_PLUS|
					REJILLA_MEDIUM_SEQUENTIAL|
					REJILLA_MEDIUM_RESTRICTED|
					REJILLA_MEDIUM_REWRITABLE|
					REJILLA_MEDIUM_APPENDABLE|
					REJILLA_MEDIUM_CLOSED|
					REJILLA_MEDIUM_HAS_DATA|
					REJILLA_MEDIUM_UNFORMATTED|
				        REJILLA_MEDIUM_BLANK);
	rejilla_plugin_blank_caps (plugin, output);
	g_slist_free (output);

	rejilla_plugin_set_blank_flags (plugin,
					REJILLA_MEDIUM_CD|
					REJILLA_MEDIUM_DVD|
					REJILLA_MEDIUM_SEQUENTIAL|
					REJILLA_MEDIUM_RESTRICTED|
					REJILLA_MEDIUM_REWRITABLE|
					REJILLA_MEDIUM_APPENDABLE|
					REJILLA_MEDIUM_CLOSED|
					REJILLA_MEDIUM_HAS_DATA|
					REJILLA_MEDIUM_HAS_AUDIO|
					REJILLA_MEDIUM_UNFORMATTED|
				        REJILLA_MEDIUM_BLANK,
					REJILLA_BURN_FLAG_NOGRACE|
					REJILLA_BURN_FLAG_FAST_BLANK,
					REJILLA_BURN_FLAG_NONE);

	/* no dummy mode for DVD+RW */
	rejilla_plugin_set_blank_flags (plugin,
					REJILLA_MEDIUM_DVDRW_PLUS|
					REJILLA_MEDIUM_APPENDABLE|
					REJILLA_MEDIUM_CLOSED|
					REJILLA_MEDIUM_HAS_DATA|
					REJILLA_MEDIUM_UNFORMATTED|
				        REJILLA_MEDIUM_BLANK,
					REJILLA_BURN_FLAG_NOGRACE|
					REJILLA_BURN_FLAG_FAST_BLANK,
					REJILLA_BURN_FLAG_NONE);

	rejilla_plugin_register_group (plugin, _(LIBBURNIA_DESCRIPTION));
}
