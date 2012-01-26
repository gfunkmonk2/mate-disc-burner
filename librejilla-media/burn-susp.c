/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8; tab-width: 8 -*- */
/*
 * Librejilla-media
 * Copyright (C) Philippe Rouquier 2005-2009 <bonfire-app@wanadoo.fr>
 *
 * Librejilla-media is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * The Librejilla-media authors hereby grant permission for non-GPL compatible
 * GStreamer plugins to be used and distributed together with GStreamer
 * and Librejilla-media. This permission is above and beyond the permissions granted
 * by the GPL license by which Librejilla-media is covered. If you modify this code
 * you may extend this exception to your version of the code, but you are not
 * obligated to do so. If you do not wish to do so, delete this exception
 * statement from your version.
 * 
 * Librejilla-media is distributed in the hope that it will be useful,
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

#include <string.h>

#include <glib.h>

#include "burn-iso-field.h"
#include "burn-susp.h"

struct _RejillaSusp {
	gchar signature		[2];
	guchar len;
	gchar version;
	gchar data		[0];
};
typedef struct _RejillaSusp RejillaSusp;

struct _RejillaSuspSP {
	RejillaSusp susp	[1];
	gchar magic		[2];
	gchar skip;
};
typedef struct _RejillaSuspSP RejillaSuspSP;

struct _RejillaSuspER {
	RejillaSusp susp	[1];
	gchar id_len;
	gchar desc_len;
	gchar src_len;
	gchar extension_version;
	gchar id [0];
};
typedef struct _RejillaSuspER RejillaSuspER;

struct _RejillaSuspCE {
	RejillaSusp susp	[1];
	guchar block		[8];
	guchar offset		[8];
	guchar len		[8];
};
typedef struct _RejillaSuspCE RejillaSuspCE;

struct _RejillaRockNM {
	RejillaSusp susp	[1];
	gchar flags;
	gchar name		[0];
};
typedef struct _RejillaRockNM RejillaRockNM;

struct _RejillaRockCL {
	RejillaSusp susp	[1];
	guchar location		[8];
};
typedef struct _RejillaRockCL RejillaRockCL;

struct _RejillaRockPL {
	RejillaSusp susp	[1];
	guchar location		[8];
};
typedef struct _RejillaRockPL RejillaRockPL;

typedef enum {
	REJILLA_NM_CONTINUE	= 1,
	REJILLA_NM_CURRENT	= 1 << 1,
	REJILLA_NM_PARENT	= 1 << 2,
	REJILLA_NM_NETWORK	= 1 << 5
} RejillaNMFlag;

void
rejilla_susp_ctx_clean (RejillaSuspCtx *ctx)
{
	if (ctx->rr_name)
		g_free (ctx->rr_name);
}

static gboolean
rejilla_susp_SP (RejillaSusp *susp,
		 RejillaSuspCtx *ctx)
{
	gchar magic [2] = { 0xBE, 0xEF };
	RejillaSuspSP *sp;

	sp = (RejillaSuspSP *) susp;

	if (memcmp (sp->magic, magic, 2))
		return FALSE;

	ctx->skip = sp->skip;
	ctx->has_SP = TRUE;
	return TRUE;
}

static gboolean
rejilla_susp_CE (RejillaSusp *susp,
		 RejillaSuspCtx *ctx)
{
	RejillaSuspCE *ce;

	ce = (RejillaSuspCE *) susp;

	ctx->CE_address = rejilla_iso9660_get_733_val (ce->block);
	ctx->CE_offset = rejilla_iso9660_get_733_val (ce->offset);
	ctx->CE_len = rejilla_iso9660_get_733_val (ce->len);

	return TRUE;
}

static gboolean
rejilla_susp_ER (RejillaSusp *susp,
		 RejillaSuspCtx *ctx)
{
	RejillaSuspER *er;

	er = (RejillaSuspER *) susp;

	/* Make sure the extention is Rock Ridge */
	if (susp->version != 1)
		return FALSE;

	if (er->id_len == 9 && !strncmp (er->id, "IEEE_1282", 9)) {
		ctx->has_RockRidge = TRUE;
		return TRUE;
	}

	if (er->id_len != 10)
		return TRUE;

	if (!strncmp (er->id, "IEEE_P1282", 10))
		ctx->has_RockRidge = TRUE;
	else if (!strncmp (er->id, "RRIP_1991A", 10))
		ctx->has_RockRidge = TRUE;

	return TRUE;
}

static gboolean
rejilla_susp_NM (RejillaSusp *susp,
		 RejillaSuspCtx *ctx)
{
	gint len;
	gchar *rr_name;
	RejillaRockNM *nm;

	nm = (RejillaRockNM *) susp;
	if (nm->flags & (REJILLA_NM_CURRENT|REJILLA_NM_PARENT|REJILLA_NM_NETWORK))
		return TRUE;

	len = susp->len - sizeof (RejillaRockNM);

	if (!len)
		return TRUE;

	/* should we concatenate ? */
	if (ctx->rr_name
	&&  ctx->rr_name_continue)
		rr_name = g_strdup_printf ("%s%.*s",
					   ctx->rr_name,
					   len,
					   nm->name);
	else
		rr_name = g_strndup (nm->name, len);

	if (ctx->rr_name)
		g_free (ctx->rr_name);

	ctx->rr_name = rr_name;
	ctx->rr_name_continue = (nm->flags & REJILLA_NM_CONTINUE);

	return TRUE;
}

/**
 * All these entries are defined in rrip standards.
 * They are mainly used for directory/file relocation.
 */
static gboolean
rejilla_susp_CL (RejillaSusp *susp,
		 RejillaSuspCtx *ctx)
{
	RejillaRockCL *cl;

	/* Child Link */
	cl = (RejillaRockCL *) susp;
	ctx->CL_address = rejilla_iso9660_get_733_val (cl->location);

	return TRUE;
}

static gboolean
rejilla_susp_RE (RejillaSusp *susp,
		 RejillaSuspCtx *ctx)
{
	/* Nothing to see here really this is just an indication.
	 * Check consistency though BP3 = "4" and BP4 = "1" */
	if (susp->len != 4 || susp->version != 1)
		return FALSE;

	ctx->has_RE = TRUE;
	return TRUE;
}

static gboolean
rejilla_susp_PL (RejillaSusp *susp,
		 RejillaSuspCtx *ctx)
{
	RejillaRockPL *pl;

	if (ctx->rr_parent)
		return FALSE;

	/* That's to store original parent address of this node before it got
	 * relocated. */
	pl = (RejillaRockPL *) susp;
	ctx->rr_parent = rejilla_iso9660_get_733_val (pl->location);
	return TRUE;
}

gboolean
rejilla_susp_read (RejillaSuspCtx *ctx, gchar *buffer, guint max)
{
	RejillaSusp *susp;
	gint offset;

	if (max <= 0)
		return TRUE;

	if (!buffer)
		return FALSE;

	susp = (RejillaSusp *) buffer;
	if (susp->len > max)
		goto error;

	offset = 0;

	/* we are only interested in some of the Rock Ridge entries:
	 * name, relocation for directories */
	while (susp->len) {
		gboolean result;

		result = TRUE;
		if (!memcmp (susp->signature, "SP", 2))
			result = rejilla_susp_SP (susp, ctx);
		/* Continuation area */
		else if (!memcmp (susp->signature, "CE", 2))
			result = rejilla_susp_CE (susp, ctx);
		/* This is to indicate that we're using Rock Ridge */
		else if (!memcmp (susp->signature, "ER", 2))
			result = rejilla_susp_ER (susp, ctx);
		else if (!memcmp (susp->signature, "NM", 2))
			result = rejilla_susp_NM (susp, ctx);
		else if (!memcmp (susp->signature, "CL", 2))
			result = rejilla_susp_CL (susp, ctx);
		else if (!memcmp (susp->signature, "PL", 2))
			result = rejilla_susp_PL (susp, ctx);

		/* This is to indicate that the entry has been relocated */
		else if (!memcmp (susp->signature, "RE", 2))
			result = rejilla_susp_RE (susp, ctx);

		if (!result)
			goto error;

		offset += susp->len;
		/* there may be one byte for padding to an even number */
		if (offset == max || offset + 1 == max)
			break;

		if (offset > max)
			goto error;

		susp = (RejillaSusp *) (buffer + offset);

		/* some coherency checks */
		if (offset + susp->len > max)
			goto error;
	}

	return TRUE;

error:

	/* clean up context */
	rejilla_susp_ctx_clean (ctx);
	return FALSE;
}
