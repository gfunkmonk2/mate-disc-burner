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

#include <glib.h>

#include "scsi-base.h"

#ifndef _SCSI_GET_CONFIGURATION_H
#define _SCSI_GET_CONFIGURATION_H

G_BEGIN_DECLS

typedef enum {
REJILLA_SCSI_PROF_EMPTY				= 0x0000,
REJILLA_SCSI_PROF_NON_REMOVABLE		= 0x0001,
REJILLA_SCSI_PROF_REMOVABLE		= 0x0002,
REJILLA_SCSI_PROF_MO_ERASABLE		= 0x0003,
REJILLA_SCSI_PROF_MO_WRITE_ONCE		= 0x0004,
REJILLA_SCSI_PROF_MO_ADVANCED_STORAGE	= 0x0005,
	/* reserved */
REJILLA_SCSI_PROF_CDROM			= 0x0008,
REJILLA_SCSI_PROF_CDR			= 0x0009,
REJILLA_SCSI_PROF_CDRW			= 0x000A,
	/* reserved */
REJILLA_SCSI_PROF_DVD_ROM		= 0x0010,
REJILLA_SCSI_PROF_DVD_R			= 0x0011,
REJILLA_SCSI_PROF_DVD_RAM		= 0x0012,
REJILLA_SCSI_PROF_DVD_RW_RESTRICTED	= 0x0013,
REJILLA_SCSI_PROF_DVD_RW_SEQUENTIAL	= 0x0014,
REJILLA_SCSI_PROF_DVD_R_DL_SEQUENTIAL	= 0x0015,
REJILLA_SCSI_PROF_DVD_R_DL_JUMP		= 0x0016,
	/* reserved */
REJILLA_SCSI_PROF_DVD_RW_PLUS		= 0x001A,
REJILLA_SCSI_PROF_DVD_R_PLUS		= 0x001B,
	/* reserved */
REJILLA_SCSI_PROF_DDCD_ROM		= 0x0020,
REJILLA_SCSI_PROF_DDCD_R		= 0x0021,
REJILLA_SCSI_PROF_DDCD_RW		= 0x0022,
	/* reserved */
REJILLA_SCSI_PROF_DVD_RW_PLUS_DL	= 0x002A,
REJILLA_SCSI_PROF_DVD_R_PLUS_DL		= 0x002B,
	/* reserved */
REJILLA_SCSI_PROF_BD_ROM		= 0x0040,
REJILLA_SCSI_PROF_BR_R_SEQUENTIAL	= 0x0041,
REJILLA_SCSI_PROF_BR_R_RANDOM		= 0x0042,
REJILLA_SCSI_PROF_BD_RW			= 0x0043,
REJILLA_SCSI_PROF_HD_DVD_ROM		= 0x0050,
REJILLA_SCSI_PROF_HD_DVD_R		= 0x0051,
REJILLA_SCSI_PROF_HD_DVD_RAM		= 0x0052,
	/* reserved */
} RejillaScsiProfile;

typedef enum {
REJILLA_SCSI_INTERFACE_NONE		= 0x00000000,
REJILLA_SCSI_INTERFACE_SCSI		= 0x00000001,
REJILLA_SCSI_INTERFACE_ATAPI		= 0x00000002,
REJILLA_SCSI_INTERFACE_FIREWIRE_95	= 0x00000003,
REJILLA_SCSI_INTERFACE_FIREWIRE_A	= 0x00000004,
REJILLA_SCSI_INTERFACE_FCP		= 0x00000005,
REJILLA_SCSI_INTERFACE_FIREWIRE_B	= 0x00000006,
REJILLA_SCSI_INTERFACE_SERIAL_ATAPI	= 0x00000007,
REJILLA_SCSI_INTERFACE_USB		= 0x00000008
} RejillaScsiInterface;

typedef enum {
REJILLA_SCSI_LOADING_CADDY		= 0x000,
REJILLA_SCSI_LOADING_TRAY		= 0x001,
REJILLA_SCSI_LOADING_POPUP		= 0x002,
REJILLA_SCSI_LOADING_EMBED_CHANGER_IND	= 0X004,
REJILLA_SCSI_LOADING_EMBED_CHANGER_MAG	= 0x005
} RejillaScsiLoadingMech;

typedef enum {
REJILLA_SCSI_FEAT_PROFILES		= 0x0000,
REJILLA_SCSI_FEAT_CORE			= 0x0001,
REJILLA_SCSI_FEAT_MORPHING		= 0x0002,
REJILLA_SCSI_FEAT_REMOVABLE		= 0x0003,
REJILLA_SCSI_FEAT_WRT_PROTECT		= 0x0004,
	/* reserved */
REJILLA_SCSI_FEAT_RD_RANDOM		= 0x0010,
	/* reserved */
REJILLA_SCSI_FEAT_RD_MULTI		= 0x001D,
REJILLA_SCSI_FEAT_RD_CD			= 0x001E,
REJILLA_SCSI_FEAT_RD_DVD		= 0x001F,
REJILLA_SCSI_FEAT_WRT_RANDOM		= 0x0020,
REJILLA_SCSI_FEAT_WRT_INCREMENT		= 0x0021,
REJILLA_SCSI_FEAT_WRT_ERASE		= 0x0022,
REJILLA_SCSI_FEAT_WRT_FORMAT		= 0x0023,
REJILLA_SCSI_FEAT_DEFECT_MNGT		= 0x0024,
REJILLA_SCSI_FEAT_WRT_ONCE		= 0x0025,
REJILLA_SCSI_FEAT_RESTRICT_OVERWRT	= 0x0026,
REJILLA_SCSI_FEAT_WRT_CAV_CDRW		= 0x0027,
REJILLA_SCSI_FEAT_MRW			= 0x0028,
REJILLA_SCSI_FEAT_DEFECT_REPORT		= 0x0029,
REJILLA_SCSI_FEAT_WRT_DVDRW_PLUS	= 0x002A,
REJILLA_SCSI_FEAT_WRT_DVDR_PLUS		= 0x002B,
REJILLA_SCSI_FEAT_RIGID_OVERWRT		= 0x002C,
REJILLA_SCSI_FEAT_WRT_TAO		= 0x002D,
REJILLA_SCSI_FEAT_WRT_SAO_RAW		= 0x002E,
REJILLA_SCSI_FEAT_WRT_DVD_LESS		= 0x002F,
REJILLA_SCSI_FEAT_RD_DDCD		= 0x0030,
REJILLA_SCSI_FEAT_WRT_DDCD		= 0x0031,
REJILLA_SCSI_FEAT_RW_DDCD		= 0x0032,
REJILLA_SCSI_FEAT_LAYER_JUMP		= 0x0033,
REJILLA_SCSI_FEAT_WRT_CDRW		= 0x0037,
REJILLA_SCSI_FEAT_BDR_POW		= 0x0038,
	/* reserved */
REJILLA_SCSI_FEAT_WRT_DVDRW_PLUS_DL		= 0x003A,
REJILLA_SCSI_FEAT_WRT_DVDR_PLUS_DL		= 0x003B,
	/* reserved */
REJILLA_SCSI_FEAT_RD_BD			= 0x0040,
REJILLA_SCSI_FEAT_WRT_BD		= 0x0041,
REJILLA_SCSI_FEAT_TSR			= 0x0042,
	/* reserved */
REJILLA_SCSI_FEAT_RD_HDDVD		= 0x0050,
REJILLA_SCSI_FEAT_WRT_HDDVD		= 0x0051,
	/* reserved */
REJILLA_SCSI_FEAT_HYBRID_DISC		= 0x0080,
	/* reserved */
REJILLA_SCSI_FEAT_PWR_MNGT		= 0x0100,
REJILLA_SCSI_FEAT_SMART			= 0x0101,
REJILLA_SCSI_FEAT_EMBED_CHNGR		= 0x0102,
REJILLA_SCSI_FEAT_AUDIO_PLAY		= 0x0103,
REJILLA_SCSI_FEAT_FIRM_UPGRADE		= 0x0104,
REJILLA_SCSI_FEAT_TIMEOUT		= 0x0105,
REJILLA_SCSI_FEAT_DVD_CSS		= 0x0106,
REJILLA_SCSI_FEAT_REAL_TIME_STREAM	= 0x0107,
REJILLA_SCSI_FEAT_DRIVE_SERIAL_NB	= 0x0108,
REJILLA_SCSI_FEAT_MEDIA_SERIAL_NB	= 0x0109,
REJILLA_SCSI_FEAT_DCB			= 0x010A,
REJILLA_SCSI_FEAT_DVD_CPRM		= 0x010B,
REJILLA_SCSI_FEAT_FIRMWARE_INFO		= 0x010C,
REJILLA_SCSI_FEAT_AACS			= 0x010D,
	/* reserved */
REJILLA_SCSI_FEAT_VCPS			= 0x0110,
} RejillaScsiFeatureType;


#if G_BYTE_ORDER == G_LITTLE_ENDIAN

struct _RejillaScsiFeatureDesc {
	uchar code		[2];

	uchar current		:1;
	uchar persistent	:1;
	uchar version		:4;
	uchar reserved		:2;

	uchar add_len;
	uchar data		[0];
};

struct _RejillaScsiCoreDescMMC4 {
	/* this is for MMC4 & MMC5 */
	uchar dbe		:1;
	uchar inq2		:1;
	uchar reserved0		:6;

	uchar reserved1		[3];
};

struct _RejillaScsiCoreDescMMC3 {
	uchar interface		[4];
};

struct _RejillaScsiProfileDesc {
	uchar number		[2];

	uchar currentp		:1;
	uchar reserved0		:7;

	uchar reserved1;
};

struct _RejillaScsiMorphingDesc {
	uchar async		:1;
	uchar op_chge_event	:1;
	uchar reserved0		:6;

	uchar reserved1		[3];
};

struct _RejillaScsiMediumDesc {
	uchar lock		:1;
	uchar reserved		:1;
	uchar prevent_jmp	:1;
	uchar eject		:1;
	uchar reserved1		:1;
	uchar loading_mech	:3;

	uchar reserved2		[3];
};

struct _RejillaScsiWrtProtectDesc {
	uchar sswpp		:1;
	uchar spwp		:1;
	uchar wdcb		:1;
	uchar dwp		:1;
	uchar reserved0		:4;

	uchar reserved1		[3];
};

struct _RejillaScsiRandomReadDesc {
	uchar block_size	[4];
	uchar blocking		[2];

	uchar pp		:1;
	uchar reserved0		:7;

	uchar reserved1;
};

struct _RejillaScsiCDReadDesc {
	uchar cdtext		:1;
	uchar c2flags		:1;
	uchar reserved0		:5;
	uchar dap		:1;

	uchar reserved1		[3];
};

/* MMC5 only otherwise just the header */
struct _RejillaScsiDVDReadDesc {
	uchar multi110		:1;
	uchar reserved0		:7;

	uchar reserved1;

	uchar dual_R		:1;
	uchar reserved2		:7;

	uchar reserved3;
};

struct _RejillaScsiRandomWriteDesc {
	/* MMC4/MMC5 only */
	uchar last_lba		[4];

	uchar block_size	[4];
	uchar blocking		[2];

	uchar pp		:1;
	uchar reserved0		:7;

	uchar reserved1;
};

struct _RejillaScsiIncrementalWrtDesc {
	uchar block_type	[2];

	uchar buf		:1;
	uchar arsv		:1;		/* MMC5 */
	uchar trio		:1;		/* MMC5 */
	uchar reserved0		:5;

	uchar num_link_sizes;
	uchar links		[0];
};

/* MMC5 only */
struct _RejillaScsiFormatDesc {
	uchar cert		:1;
	uchar qcert		:1;
	uchar expand		:1;
	uchar renosa		:1;
	uchar reserved0		:4;

	uchar reserved1		[3];

	uchar rrm		:1;
	uchar reserved2		:7;

	uchar reserved3		[3];
};

struct _RejillaScsiDefectMngDesc {
	uchar reserved0		:7;
	uchar ssa		:1;

	uchar reserved1		[3];
};

struct _RejillaScsiWrtOnceDesc {
	uchar lba_size		[4];
	uchar blocking		[2];

	uchar pp		:1;
	uchar reserved0		:7;

	uchar reserved1;
};

struct _RejillaScsiMRWDesc {
	uchar wrt_CD		:1;
	uchar rd_DVDplus	:1;
	uchar wrt_DVDplus	:1;
	uchar reserved0		:5;

	uchar reserved1		[3];
};

struct _RejillaScsiDefectReportDesc {
	uchar drt_dm		:1;
	uchar reserved0		:7;

	uchar dbi_zones_num;
	uchar num_entries	[2];
};

struct _RejillaScsiDVDRWplusDesc {
	uchar write		:1;
	uchar reserved0		:7;

	uchar close		:1;
	uchar quick_start	:1;
	uchar reserved1		:6;

	uchar reserved2		[2];
};

struct _RejillaScsiDVDRplusDesc {
	uchar write		:1;
	uchar reserved0		:7;

	uchar reserved1		[3];
};

struct _RejillaScsiRigidOverwrtDesc {
	uchar blank		:1;
	uchar intermediate	:1;
	uchar dsdr		:1;
	uchar dsdg		:1;
	uchar reserved0		:4;

	uchar reserved1		[3];
};

struct _RejillaScsiCDTAODesc {
	uchar RW_subcode	:1;
	uchar CDRW		:1;
	uchar dummy		:1;
	uchar RW_pack		:1;
	uchar RW_raw		:1;
	uchar reserved0		:1;
	uchar buf		:1;
	uchar reserved1		:1;

	uchar reserved2;

	uchar data_type		[2];
};

struct _RejillaScsiCDSAODesc {
	uchar rw_sub_chan	:1;
	uchar rw_CD		:1;
	uchar dummy		:1;
	uchar raw		:1;
	uchar raw_multi		:1;
	uchar sao		:1;
	uchar buf		:1;
	uchar reserved		:1;

	uchar max_cue_size	[3];
};

struct _RejillaScsiDVDRWlessWrtDesc {
	uchar reserved0		:1;
	uchar rw_DVD		:1;
	uchar dummy		:1;
	uchar dual_layer_r	:1;
	uchar reserved1		:2;
	uchar buf		:1;
	uchar reserved2		:1;

	uchar reserved3		[3];
};

struct _RejillaScsiCDRWWrtDesc {
	uchar reserved0;

	uchar sub0		:1;
	uchar sub1		:1;
	uchar sub2		:1;
	uchar sub3		:1;
	uchar sub4		:1;
	uchar sub5		:1;
	uchar sub6		:1;
	uchar sub7		:1;

	uchar reserved1		[2];
};

struct _RejillaScsiDVDRWDLDesc {
	uchar write		:1;
	uchar reserved0		:7;

	uchar close		:1;
	uchar quick_start	:1;
	uchar reserved1		:6;

	uchar reserved2		[2];
};

struct _RejillaScsiDVDRDLDesc {
	uchar write		:1;
	uchar reserved0		:7;

	uchar reserved1		[3];
};

struct _RejillaScsiBDReadDesc {
	uchar reserved		[4];

	uchar class0_RE_v8	:1;
	uchar class0_RE_v9	:1;
	uchar class0_RE_v10	:1;
	uchar class0_RE_v11	:1;
	uchar class0_RE_v12	:1;
	uchar class0_RE_v13	:1;
	uchar class0_RE_v14	:1;
	uchar class0_RE_v15	:1;

	uchar class0_RE_v0	:1;
	uchar class0_RE_v1	:1;
	uchar class0_RE_v2	:1;
	uchar class0_RE_v3	:1;
	uchar class0_RE_v4	:1;
	uchar class0_RE_v5	:1;
	uchar class0_RE_v6	:1;
	uchar class0_RE_v7	:1;
	
	uchar class1_RE_v8	:1;
	uchar class1_RE_v9	:1;
	uchar class1_RE_v10	:1;
	uchar class1_RE_v11	:1;
	uchar class1_RE_v12	:1;
	uchar class1_RE_v13	:1;
	uchar class1_RE_v14	:1;
	uchar class1_RE_v15	:1;
	
	uchar class1_RE_v0	:1;
	uchar class1_RE_v1	:1;
	uchar class1_RE_v2	:1;
	uchar class1_RE_v3	:1;
	uchar class1_RE_v4	:1;
	uchar class1_RE_v5	:1;
	uchar class1_RE_v6	:1;
	uchar class1_RE_v7	:1;
	
	uchar class2_RE_v8	:1;
	uchar class2_RE_v9	:1;
	uchar class2_RE_v10	:1;
	uchar class2_RE_v11	:1;
	uchar class2_RE_v12	:1;
	uchar class2_RE_v13	:1;
	uchar class2_RE_v14	:1;
	uchar class2_RE_v15	:1;
	
	uchar class2_RE_v0	:1;
	uchar class2_RE_v1	:1;
	uchar class2_RE_v2	:1;
	uchar class2_RE_v3	:1;
	uchar class2_RE_v4	:1;
	uchar class2_RE_v5	:1;
	uchar class2_RE_v6	:1;
	uchar class2_RE_v7	:1;
	
	uchar class3_RE_v8	:1;
	uchar class3_RE_v9	:1;
	uchar class3_RE_v10	:1;
	uchar class3_RE_v11	:1;
	uchar class3_RE_v12	:1;
	uchar class3_RE_v13	:1;
	uchar class3_RE_v14	:1;
	uchar class3_RE_v15	:1;
	
	uchar class3_RE_v0	:1;
	uchar class3_RE_v1	:1;
	uchar class3_RE_v2	:1;
	uchar class3_RE_v3	:1;
	uchar class3_RE_v4	:1;
	uchar class3_RE_v5	:1;
	uchar class3_RE_v6	:1;
	uchar class3_RE_v7	:1;

	uchar class0_R_v8	:1;
	uchar class0_R_v9	:1;
	uchar class0_R_v10	:1;
	uchar class0_R_v11	:1;
	uchar class0_R_v12	:1;
	uchar class0_R_v13	:1;
	uchar class0_R_v14	:1;
	uchar class0_R_v15	:1;
	
	uchar class0_R_v0	:1;
	uchar class0_R_v1	:1;
	uchar class0_R_v2	:1;
	uchar class0_R_v3	:1;
	uchar class0_R_v4	:1;
	uchar class0_R_v5	:1;
	uchar class0_R_v6	:1;
	uchar class0_R_v7	:1;
	
	uchar class1_R_v8	:1;
	uchar class1_R_v9	:1;
	uchar class1_R_v10	:1;
	uchar class1_R_v11	:1;
	uchar class1_R_v12	:1;
	uchar class1_R_v13	:1;
	uchar class1_R_v14	:1;
	uchar class1_R_v15	:1;
	
	uchar class1_R_v0	:1;
	uchar class1_R_v1	:1;
	uchar class1_R_v2	:1;
	uchar class1_R_v3	:1;
	uchar class1_R_v4	:1;
	uchar class1_R_v5	:1;
	uchar class1_R_v6	:1;
	uchar class1_R_v7	:1;
	
	uchar class2_R_v8	:1;
	uchar class2_R_v9	:1;
	uchar class2_R_v10	:1;
	uchar class2_R_v11	:1;
	uchar class2_R_v12	:1;
	uchar class2_R_v13	:1;
	uchar class2_R_v14	:1;
	uchar class2_R_v15	:1;
	
	uchar class2_R_v0	:1;
	uchar class2_R_v1	:1;
	uchar class2_R_v2	:1;
	uchar class2_R_v3	:1;
	uchar class2_R_v4	:1;
	uchar class2_R_v5	:1;
	uchar class2_R_v6	:1;
	uchar class2_R_v7	:1;
	
	uchar class3_R_v8	:1;
	uchar class3_R_v9	:1;
	uchar class3_R_v10	:1;
	uchar class3_R_v11	:1;
	uchar class3_R_v12	:1;
	uchar class3_R_v13	:1;
	uchar class3_R_v14	:1;
	uchar class3_R_v15	:1;
	
	uchar class3_R_v0	:1;
	uchar class3_R_v1	:1;
	uchar class3_R_v2	:1;
	uchar class3_R_v3	:1;
	uchar class3_R_v4	:1;
	uchar class3_R_v5	:1;
	uchar class3_R_v6	:1;
	uchar class3_R_v7	:1;
};

struct _RejillaScsiBDWriteDesc {
	uchar reserved		[4];

	uchar class0_RE_v8	:1;
	uchar class0_RE_v9	:1;
	uchar class0_RE_v10	:1;
	uchar class0_RE_v11	:1;
	uchar class0_RE_v12	:1;
	uchar class0_RE_v13	:1;
	uchar class0_RE_v14	:1;
	uchar class0_RE_v15	:1;
	
	uchar class0_RE_v0	:1;
	uchar class0_RE_v1	:1;
	uchar class0_RE_v2	:1;
	uchar class0_RE_v3	:1;
	uchar class0_RE_v4	:1;
	uchar class0_RE_v5	:1;
	uchar class0_RE_v6	:1;
	uchar class0_RE_v7	:1;
	
	uchar class1_RE_v8	:1;
	uchar class1_RE_v9	:1;
	uchar class1_RE_v10	:1;
	uchar class1_RE_v11	:1;
	uchar class1_RE_v12	:1;
	uchar class1_RE_v13	:1;
	uchar class1_RE_v14	:1;
	uchar class1_RE_v15	:1;
	
	uchar class1_RE_v0	:1;
	uchar class1_RE_v1	:1;
	uchar class1_RE_v2	:1;
	uchar class1_RE_v3	:1;
	uchar class1_RE_v4	:1;
	uchar class1_RE_v5	:1;
	uchar class1_RE_v6	:1;
	uchar class1_RE_v7	:1;
	
	uchar class2_RE_v8	:1;
	uchar class2_RE_v9	:1;
	uchar class2_RE_v10	:1;
	uchar class2_RE_v11	:1;
	uchar class2_RE_v12	:1;
	uchar class2_RE_v13	:1;
	uchar class2_RE_v14	:1;
	uchar class2_RE_v15	:1;
	
	uchar class2_RE_v0	:1;
	uchar class2_RE_v1	:1;
	uchar class2_RE_v2	:1;
	uchar class2_RE_v3	:1;
	uchar class2_RE_v4	:1;
	uchar class2_RE_v5	:1;
	uchar class2_RE_v6	:1;
	uchar class2_RE_v7	:1;
	
	uchar class3_RE_v8	:1;
	uchar class3_RE_v9	:1;
	uchar class3_RE_v10	:1;
	uchar class3_RE_v11	:1;
	uchar class3_RE_v12	:1;
	uchar class3_RE_v13	:1;
	uchar class3_RE_v14	:1;
	uchar class3_RE_v15	:1;
	
	uchar class3_RE_v0	:1;
	uchar class3_RE_v1	:1;
	uchar class3_RE_v2	:1;
	uchar class3_RE_v3	:1;
	uchar class3_RE_v4	:1;
	uchar class3_RE_v5	:1;
	uchar class3_RE_v6	:1;
	uchar class3_RE_v7	:1;

	uchar class0_R_v8	:1;
	uchar class0_R_v9	:1;
	uchar class0_R_v10	:1;
	uchar class0_R_v11	:1;
	uchar class0_R_v12	:1;
	uchar class0_R_v13	:1;
	uchar class0_R_v14	:1;
	uchar class0_R_v15	:1;
	
	uchar class0_R_v0	:1;
	uchar class0_R_v1	:1;
	uchar class0_R_v2	:1;
	uchar class0_R_v3	:1;
	uchar class0_R_v4	:1;
	uchar class0_R_v5	:1;
	uchar class0_R_v6	:1;
	uchar class0_R_v7	:1;
	
	uchar class1_R_v8	:1;
	uchar class1_R_v9	:1;
	uchar class1_R_v10	:1;
	uchar class1_R_v11	:1;
	uchar class1_R_v12	:1;
	uchar class1_R_v13	:1;
	uchar class1_R_v14	:1;
	uchar class1_R_v15	:1;
	
	uchar class1_R_v0	:1;
	uchar class1_R_v1	:1;
	uchar class1_R_v2	:1;
	uchar class1_R_v3	:1;
	uchar class1_R_v4	:1;
	uchar class1_R_v5	:1;
	uchar class1_R_v6	:1;
	uchar class1_R_v7	:1;
	
	uchar class2_R_v8	:1;
	uchar class2_R_v9	:1;
	uchar class2_R_v10	:1;
	uchar class2_R_v11	:1;
	uchar class2_R_v12	:1;
	uchar class2_R_v13	:1;
	uchar class2_R_v14	:1;
	uchar class2_R_v15	:1;
	
	uchar class2_R_v0	:1;
	uchar class2_R_v1	:1;
	uchar class2_R_v2	:1;
	uchar class2_R_v3	:1;
	uchar class2_R_v4	:1;
	uchar class2_R_v5	:1;
	uchar class2_R_v6	:1;
	uchar class2_R_v7	:1;
	
	uchar class3_R_v8	:1;
	uchar class3_R_v9	:1;
	uchar class3_R_v10	:1;
	uchar class3_R_v11	:1;
	uchar class3_R_v12	:1;
	uchar class3_R_v13	:1;
	uchar class3_R_v14	:1;
	uchar class3_R_v15	:1;
	
	uchar class3_R_v0	:1;
	uchar class3_R_v1	:1;
	uchar class3_R_v2	:1;
	uchar class3_R_v3	:1;
	uchar class3_R_v4	:1;
	uchar class3_R_v5	:1;
	uchar class3_R_v6	:1;
	uchar class3_R_v7	:1;
};

struct _RejillaScsiHDDVDReadDesc {
	uchar hd_dvd_r		:1;
	uchar reserved0		:7;

	uchar reserved1;

	uchar hd_dvd_ram	:1;
	uchar reserved2		:7;

	uchar reserved3;
};

struct _RejillaScsiHDDVDWriteDesc {
	uchar hd_dvd_r		:1;
	uchar reserved0		:7;

	uchar reserved1;

	uchar hd_dvd_ram	:1;
	uchar reserved2		:7;

	uchar reserved3;
};

struct _RejillaScsiHybridDiscDesc {
	uchar ri		:1;
	uchar reserved0		:7;

	uchar reserved1		[3];
};

struct _RejillaScsiSmartDesc {
	uchar pp		:1;
	uchar reserved0		:7;

	uchar reserved1		[3];
};

struct _RejillaScsiEmbedChngDesc {
	uchar reserved0		:1;
	uchar sdp		:1;
	uchar reserved1		:1;
	uchar scc		:1;
	uchar reserved2		:3;

	uchar reserved3		[2];

	uchar slot_num		:5;
	uchar reserved4		:3;
};

struct _RejillaScsiExtAudioPlayDesc {
	uchar separate_vol	:1;
	uchar separate_chnl_mute:1;
	uchar scan_command	:1;
	uchar reserved0		:5;

	uchar reserved1;

	uchar number_vol	[2];
};

struct _RejillaScsiFirmwareUpgrDesc {
	uchar m5		:1;
	uchar reserved0		:7;

	uchar reserved1		[3];
};

struct _RejillaScsiTimeoutDesc {
	uchar group3		:1;
	uchar reserved0		:7;

	uchar reserved1;
	uchar unit_len		[2];
};

struct _RejillaScsiRTStreamDesc {
	uchar stream_wrt	:1;
	uchar wrt_spd		:1;
	uchar mp2a		:1;
	uchar set_cd_spd	:1;
	uchar rd_buf_caps_block	:1;
	uchar reserved0		:3;

	uchar reserved1		[3];
};

struct _RejillaScsiAACSDesc {
	uchar bng		:1;
	uchar reserved0		:7;

	uchar block_count;

	uchar agids_num		:4;
	uchar reserved1		:4;

	uchar version;
};

#else

struct _RejillaScsiFeatureDesc {
	uchar code		[2];

	uchar current		:1;
	uchar persistent	:1;
	uchar version		:4;
	uchar reserved		:2;

	uchar add_len;
	uchar data		[0];
};

struct _RejillaScsiProfileDesc {
	uchar number		[2];

	uchar reserved0		:7;
	uchar currentp		:1;

	uchar reserved1;
};

struct _RejillaScsiCoreDescMMC4 {
	uchar reserved0		:6;
	uchar inq2		:1;
	uchar dbe		:1;

  	uchar mmc4		[0];
	uchar reserved1		[3];
};

struct _RejillaScsiCoreDescMMC3 {
	uchar interface		[4];
};

struct _RejillaScsiMorphingDesc {
	uchar reserved0		:6;
	uchar op_chge_event	:1;
	uchar async		:1;

	uchar reserved1		[3];
};

struct _RejillaScsiMediumDesc {
	uchar loading_mech	:3;
	uchar reserved1		:1;
	uchar eject		:1;
	uchar prevent_jmp	:1;
	uchar reserved		:1;
	uchar lock		:1;

	uchar reserved2		[3];
};

struct _RejillaScsiWrtProtectDesc {
	uchar reserved0		:4;
	uchar dwp		:1;
	uchar wdcb		:1;
	uchar spwp		:1;
	uchar sswpp		:1;

	uchar reserved1		[3];
};

struct _RejillaScsiRandomReadDesc {
	uchar block_size	[4];
	uchar blocking		[2];

	uchar reserved0		:7;
	uchar pp		:1;

	uchar reserved1;
};

struct _RejillaScsiCDReadDesc {
	uchar dap		:1;
	uchar reserved0		:5;
	uchar c2flags		:1;
	uchar cdtext		:1;

	uchar reserved1		[3];
};

struct _RejillaScsiDVDReadDesc {
	uchar reserved0		:7;
	uchar multi110		:1;

	uchar reserved1;

	uchar reserved2		:7;
	uchar dual_R		:1;

	uchar reserved3;
};

struct _RejillaScsiRandomWriteDesc {
	uchar last_lba		[4];
	uchar block_size	[4];
	uchar blocking		[2];

	uchar reserved0		:7;
	uchar pp		:1;

	uchar reserved1;
};

struct _RejillaScsiIncrementalWrtDesc {
	uchar block_type	[2];

	uchar reserved0		:5;
	uchar trio		:1;
	uchar arsv		:1;
	uchar buf		:1;

	uchar num_link_sizes;
	uchar links;
};

struct _RejillaScsiFormatDesc {
	uchar reserved0		:4;
	uchar renosa		:1;
	uchar expand		:1;
	uchar qcert		:1;
	uchar cert		:1;

	uchar reserved1		[3];

	uchar reserved2		:7;
	uchar rrm		:1;

	uchar reserved3		[3];
};

struct _RejillaScsiDefectMngDesc {
	uchar ssa		:1;
	uchar reserved0		:7;

	uchar reserved1		[3];
};

struct _RejillaScsiWrtOnceDesc {
	uchar lba_size		[4];
	uchar blocking		[2];

	uchar reserved0		:7;
	uchar pp		:1;

	uchar reserved1;
};

struct _RejillaScsiMRWDesc {
	uchar reserved0		:5;
	uchar wrt_DVDplus	:1;
	uchar rd_DVDplus	:1;
	uchar wrt_CD		:1;

	uchar reserved1		[3];
};

struct _RejillaScsiDefectReportDesc {
	uchar reserved0		:7;
	uchar drt_dm		:1;

	uchar dbi_zones_num;
	uchar num_entries	[2];
};

struct _RejillaScsiDVDRWplusDesc {
	uchar reserved0		:7;
	uchar write		:1;

	uchar reserved1		:6;
	uchar quick_start	:1;
	uchar close		:1;

	uchar reserved2		[2];
};

struct _RejillaScsiDVDRplusDesc {
	uchar reserved0		:7;
	uchar write		:1;

	uchar reserved1		[3];
};

struct _RejillaScsiRigidOverwrtDesc {
	uchar reserved0		:4;
	uchar dsdg		:1;
	uchar dsdr		:1;
	uchar intermediate	:1;
	uchar blank		:1;

	uchar reserved1		[3];
};

struct _RejillaScsiCDTAODesc {
	uchar reserved1		:1;
	uchar buf		:1;
	uchar reserved0		:1;
	uchar RW_raw		:1;
	uchar RW_pack		:1;
	uchar dummy		:1;
	uchar CDRW		:1;
	uchar RW_subcode	:1;

	uchar reserved2;

	uchar data_type		[2];
};

struct _RejillaScsiCDSAODesc {
	uchar reserved		:1;
	uchar buf		:1;
	uchar sao		:1;
	uchar raw_multi		:1;
	uchar raw		:1;
	uchar dummy		:1;
	uchar rw_CD		:1;
	uchar rw_sub_chan	:1;

	uchar max_cue_size	[3];
};

struct _RejillaScsiDVDRWlessWrtDesc {
	uchar reserved2		:1;
	uchar buf		:1;
	uchar reserved1		:2;
	uchar dual_layer_r	:1;
	uchar dummy		:1;
	uchar rw_DVD		:1;
	uchar reserved0		:1;

	uchar reserved3		[3];
};

struct _RejillaScsiCDRWWrtDesc {
	uchar reserved0;

	uchar sub7		:1;
	uchar sub6		:1;
	uchar sub5		:1;
	uchar sub4		:1;
	uchar sub3		:1;
	uchar sub2		:1;
	uchar sub1		:1;
	uchar sub0		:1;

	uchar reserved1		[2];
};

struct _RejillaScsiDVDRWDLDesc {
	uchar reserved0		:7;
	uchar write		:1;

	uchar reserved1		:6;
	uchar quick_start	:1;
	uchar close		:1;

	uchar reserved2		[2];
};

struct _RejillaScsiDVDRDLDesc {
	uchar reserved0		:7;
	uchar write		:1;

	uchar reserved1		[3];
};

struct _RejillaScsiBDReadDesc {
	uchar reserved		[4];

	uchar class0_RE_v15	:1;
	uchar class0_RE_v14	:1;
	uchar class0_RE_v13	:1;
	uchar class0_RE_v12	:1;
	uchar class0_RE_v11	:1;
	uchar class0_RE_v10	:1;
	uchar class0_RE_v9	:1;
	uchar class0_RE_v8	:1;

	uchar class0_RE_v7	:1;
	uchar class0_RE_v6	:1;
	uchar class0_RE_v5	:1;
	uchar class0_RE_v4	:1;
	uchar class0_RE_v3	:1;
	uchar class0_RE_v2	:1;
	uchar class0_RE_v1	:1;
	uchar class0_RE_v0	:1;

	uchar class1_RE_v15	:1;
	uchar class1_RE_v14	:1;
	uchar class1_RE_v13	:1;
	uchar class1_RE_v12	:1;
	uchar class1_RE_v11	:1;
	uchar class1_RE_v10	:1;
	uchar class1_RE_v9	:1;
	uchar class1_RE_v8	:1;
	
	uchar class1_RE_v7	:1;
	uchar class1_RE_v6	:1;
	uchar class1_RE_v5	:1;
	uchar class1_RE_v4	:1;
	uchar class1_RE_v3	:1;
	uchar class1_RE_v2	:1;
	uchar class1_RE_v1	:1;
	uchar class1_RE_v0	:1;
	
	uchar class2_RE_v15	:1;
	uchar class2_RE_v14	:1;
	uchar class2_RE_v13	:1;
	uchar class2_RE_v12	:1;
	uchar class2_RE_v11	:1;
	uchar class2_RE_v10	:1;
	uchar class2_RE_v9	:1;
	uchar class2_RE_v8	:1;
	
	uchar class2_RE_v7	:1;
	uchar class2_RE_v6	:1;
	uchar class2_RE_v5	:1;
	uchar class2_RE_v4	:1;
	uchar class2_RE_v3	:1;
	uchar class2_RE_v2	:1;
	uchar class2_RE_v1	:1;
	uchar class2_RE_v0	:1;
	
	uchar class3_RE_v15	:1;
	uchar class3_RE_v14	:1;
	uchar class3_RE_v13	:1;
	uchar class3_RE_v12	:1;
	uchar class3_RE_v11	:1;
	uchar class3_RE_v10	:1;
	uchar class3_RE_v9	:1;
	uchar class3_RE_v8	:1;
	
	uchar class3_RE_v7	:1;
	uchar class3_RE_v6	:1;
	uchar class3_RE_v5	:1;
	uchar class3_RE_v4	:1;
	uchar class3_RE_v3	:1;
	uchar class3_RE_v2	:1;
	uchar class3_RE_v1	:1;
	uchar class3_RE_v0	:1;

	uchar class0_R_v15	:1;
	uchar class0_R_v14	:1;
	uchar class0_R_v13	:1;
	uchar class0_R_v12	:1;
	uchar class0_R_v11	:1;
	uchar class0_R_v10	:1;
	uchar class0_R_v9	:1;
	uchar class0_R_v8	:1;

	uchar class0_R_v7	:1;
	uchar class0_R_v6	:1;
	uchar class0_R_v5	:1;
	uchar class0_R_v4	:1;
	uchar class0_R_v3	:1;
	uchar class0_R_v2	:1;
	uchar class0_R_v1	:1;
	uchar class0_R_v0	:1;

	uchar class1_R_v15	:1;
	uchar class1_R_v14	:1;
	uchar class1_R_v13	:1;
	uchar class1_R_v12	:1;
	uchar class1_R_v11	:1;
	uchar class1_R_v10	:1;
	uchar class1_R_v9	:1;
	uchar class1_R_v8	:1;
	
	uchar class1_R_v7	:1;
	uchar class1_R_v6	:1;
	uchar class1_R_v5	:1;
	uchar class1_R_v4	:1;
	uchar class1_R_v3	:1;
	uchar class1_R_v2	:1;
	uchar class1_R_v1	:1;
	uchar class1_R_v0	:1;
	
	uchar class2_R_v15	:1;
	uchar class2_R_v14	:1;
	uchar class2_R_v13	:1;
	uchar class2_R_v12	:1;
	uchar class2_R_v11	:1;
	uchar class2_R_v10	:1;
	uchar class2_R_v9	:1;
	uchar class2_R_v8	:1;
	
	uchar class2_R_v7	:1;
	uchar class2_R_v6	:1;
	uchar class2_R_v5	:1;
	uchar class2_R_v4	:1;
	uchar class2_R_v3	:1;
	uchar class2_R_v2	:1;
	uchar class2_R_v1	:1;
	uchar class2_R_v0	:1;
	
	uchar class3_R_v15	:1;
	uchar class3_R_v14	:1;
	uchar class3_R_v13	:1;
	uchar class3_R_v12	:1;
	uchar class3_R_v11	:1;
	uchar class3_R_v10	:1;
	uchar class3_R_v9	:1;
	uchar class3_R_v8	:1;
	
	uchar class3_R_v7	:1;
	uchar class3_R_v6	:1;
	uchar class3_R_v5	:1;
	uchar class3_R_v4	:1;
	uchar class3_R_v3	:1;
	uchar class3_R_v2	:1;
	uchar class3_R_v1	:1;
	uchar class3_R_v0	:1;
};

struct _RejillaScsiBDWriteDesc {
	uchar reserved		[4];

	uchar class0_RE_v15	:1;
	uchar class0_RE_v14	:1;
	uchar class0_RE_v13	:1;
	uchar class0_RE_v12	:1;
	uchar class0_RE_v11	:1;
	uchar class0_RE_v10	:1;
	uchar class0_RE_v9	:1;
	uchar class0_RE_v8	:1;

	uchar class0_RE_v7	:1;
	uchar class0_RE_v6	:1;
	uchar class0_RE_v5	:1;
	uchar class0_RE_v4	:1;
	uchar class0_RE_v3	:1;
	uchar class0_RE_v2	:1;
	uchar class0_RE_v1	:1;
	uchar class0_RE_v0	:1;

	uchar class1_RE_v15	:1;
	uchar class1_RE_v14	:1;
	uchar class1_RE_v13	:1;
	uchar class1_RE_v12	:1;
	uchar class1_RE_v11	:1;
	uchar class1_RE_v10	:1;
	uchar class1_RE_v9	:1;
	uchar class1_RE_v8	:1;
	
	uchar class1_RE_v7	:1;
	uchar class1_RE_v6	:1;
	uchar class1_RE_v5	:1;
	uchar class1_RE_v4	:1;
	uchar class1_RE_v3	:1;
	uchar class1_RE_v2	:1;
	uchar class1_RE_v1	:1;
	uchar class1_RE_v0	:1;
	
	uchar class2_RE_v15	:1;
	uchar class2_RE_v14	:1;
	uchar class2_RE_v13	:1;
	uchar class2_RE_v12	:1;
	uchar class2_RE_v11	:1;
	uchar class2_RE_v10	:1;
	uchar class2_RE_v9	:1;
	uchar class2_RE_v8	:1;
	
	uchar class2_RE_v7	:1;
	uchar class2_RE_v6	:1;
	uchar class2_RE_v5	:1;
	uchar class2_RE_v4	:1;
	uchar class2_RE_v3	:1;
	uchar class2_RE_v2	:1;
	uchar class2_RE_v1	:1;
	uchar class2_RE_v0	:1;
	
	uchar class3_RE_v15	:1;
	uchar class3_RE_v14	:1;
	uchar class3_RE_v13	:1;
	uchar class3_RE_v12	:1;
	uchar class3_RE_v11	:1;
	uchar class3_RE_v10	:1;
	uchar class3_RE_v9	:1;
	uchar class3_RE_v8	:1;
	
	uchar class3_RE_v7	:1;
	uchar class3_RE_v6	:1;
	uchar class3_RE_v5	:1;
	uchar class3_RE_v4	:1;
	uchar class3_RE_v3	:1;
	uchar class3_RE_v2	:1;
	uchar class3_RE_v1	:1;
	uchar class3_RE_v0	:1;

	uchar class0_R_v15	:1;
	uchar class0_R_v14	:1;
	uchar class0_R_v13	:1;
	uchar class0_R_v12	:1;
	uchar class0_R_v11	:1;
	uchar class0_R_v10	:1;
	uchar class0_R_v9	:1;
	uchar class0_R_v8	:1;

	uchar class0_R_v7	:1;
	uchar class0_R_v6	:1;
	uchar class0_R_v5	:1;
	uchar class0_R_v4	:1;
	uchar class0_R_v3	:1;
	uchar class0_R_v2	:1;
	uchar class0_R_v1	:1;
	uchar class0_R_v0	:1;

	uchar class1_R_v15	:1;
	uchar class1_R_v14	:1;
	uchar class1_R_v13	:1;
	uchar class1_R_v12	:1;
	uchar class1_R_v11	:1;
	uchar class1_R_v10	:1;
	uchar class1_R_v9	:1;
	uchar class1_R_v8	:1;
	
	uchar class1_R_v7	:1;
	uchar class1_R_v6	:1;
	uchar class1_R_v5	:1;
	uchar class1_R_v4	:1;
	uchar class1_R_v3	:1;
	uchar class1_R_v2	:1;
	uchar class1_R_v1	:1;
	uchar class1_R_v0	:1;
	
	uchar class2_R_v15	:1;
	uchar class2_R_v14	:1;
	uchar class2_R_v13	:1;
	uchar class2_R_v12	:1;
	uchar class2_R_v11	:1;
	uchar class2_R_v10	:1;
	uchar class2_R_v9	:1;
	uchar class2_R_v8	:1;
	
	uchar class2_R_v7	:1;
	uchar class2_R_v6	:1;
	uchar class2_R_v5	:1;
	uchar class2_R_v4	:1;
	uchar class2_R_v3	:1;
	uchar class2_R_v2	:1;
	uchar class2_R_v1	:1;
	uchar class2_R_v0	:1;
	
	uchar class3_R_v15	:1;
	uchar class3_R_v14	:1;
	uchar class3_R_v13	:1;
	uchar class3_R_v12	:1;
	uchar class3_R_v11	:1;
	uchar class3_R_v10	:1;
	uchar class3_R_v9	:1;
	uchar class3_R_v8	:1;
	
	uchar class3_R_v7	:1;
	uchar class3_R_v6	:1;
	uchar class3_R_v5	:1;
	uchar class3_R_v4	:1;
	uchar class3_R_v3	:1;
	uchar class3_R_v2	:1;
	uchar class3_R_v1	:1;
	uchar class3_R_v0	:1;
};

struct _RejillaScsiHDDVDReadDesc {
	uchar reserved0		:7;
	uchar hd_dvd_r		:1;

	uchar reserved1;

	uchar reserved2		:7;
	uchar hd_dvd_ram	:1;

	uchar reserved3;
};

struct _RejillaScsiHDDVDWriteDesc {
	uchar reserved0		:7;
	uchar hd_dvd_r		:1;

	uchar reserved1;

	uchar reserved2		:7;
	uchar hd_dvd_ram	:1;

	uchar reserved3;
};

struct _RejillaScsiHybridDiscDesc {
	uchar reserved0		:7;
	uchar ri		:1;

	uchar reserved1		[3];
};

struct _RejillaScsiSmartDesc {
	uchar reserved0		:7;
	uchar pp		:1;

	uchar reserved1		[3];
};

struct _RejillaScsiEmbedChngDesc {
	uchar reserved2		:3;
	uchar scc		:1;
	uchar reserved1		:1;
	uchar sdp		:1;
	uchar reserved0		:1;

	uchar reserved3		[2];

	uchar reserved4		:3;
	uchar slot_num		:5;
};

struct _RejillaScsiExtAudioPlayDesc {
	uchar reserved0		:5;
	uchar scan_command	:1;
	uchar separate_chnl_mute:1;
	uchar separate_vol	:1;

	uchar reserved1;

	uchar number_vol	[2];
};

struct _RejillaScsiFirmwareUpgrDesc {
	uchar reserved0		:7;
	uchar m5		:1;

	uchar reserved1		[3];
};

struct _RejillaScsiTimeoutDesc {
	uchar reserved0		:7;
	uchar group3		:1;

	uchar reserved1;
	uchar unit_len		[2];
};

struct _RejillaScsiRTStreamDesc {
	uchar reserved0		:3;
	uchar rd_buf_caps_block	:1;
	uchar set_cd_spd	:1;
	uchar mp2a		:1;
	uchar wrt_spd		:1;
	uchar stream_wrt	:1;

	uchar reserved1		[3];
};

struct _RejillaScsiAACSDesc {
	uchar reserved0		:7;
	uchar bng		:1;

	uchar block_count;

	uchar reserved1		:4;
	uchar agids_num		:4;

	uchar version;
};

#endif

struct _RejillaScsiInterfaceDesc {
	uchar code		[4];
};

struct _RejillaScsiCDrwCavDesc {
	uchar reserved		[4];
};

/* NOTE: this structure is extendable with padding to have a multiple of 4 */
struct _RejillaScsiLayerJmpDesc {
	uchar reserved0		[3];
	uchar num_link_sizes;
	uchar links		[0];
};

struct _RejillaScsiPOWDesc {
	uchar reserved		[4];
};

struct _RejillaScsiDVDCssDesc {
	uchar reserved		[3];
	uchar version;
};

/* NOTE: this structure is extendable with padding to have a multiple of 4 */
struct _RejillaScsiDriveSerialNumDesc {
	uchar serial		[4];
};

struct _RejillaScsiMediaSerialNumDesc {
	uchar serial		[4];
};

/* NOTE: this structure is extendable with padding to have a multiple of 4 */
struct _RejillaScsiDiscCtlBlocksDesc {
	uchar entry		[1][4];
};

struct _RejillaScsiDVDCprmDesc {
	uchar reserved0 	[3];
	uchar version;
};

struct _RejillaScsiFirmwareDesc {
	uchar century		[2];
	uchar year		[2];
	uchar month		[2];
	uchar day		[2];
	uchar hour		[2];
	uchar minute		[2];
	uchar second		[2];
	uchar reserved		[2];
};

struct _RejillaScsiVPSDesc {
	uchar reserved		[4];
};

typedef struct _RejillaScsiFeatureDesc RejillaScsiFeatureDesc;
typedef struct _RejillaScsiProfileDesc RejillaScsiProfileDesc;
typedef struct _RejillaScsiCoreDescMMC3 RejillaScsiCoreDescMMC3;
typedef struct _RejillaScsiCoreDescMMC4 RejillaScsiCoreDescMMC4;
typedef struct _RejillaScsiInterfaceDesc RejillaScsiInterfaceDesc;
typedef struct _RejillaScsiMorphingDesc RejillaScsiMorphingDesc;
typedef struct _RejillaScsiMediumDesc RejillaScsiMediumDesc;
typedef struct _RejillaScsiWrtProtectDesc RejillaScsiWrtProtectDesc;
typedef struct _RejillaScsiRandomReadDesc RejillaScsiRandomReadDesc;
typedef struct _RejillaScsiCDReadDesc RejillaScsiCDReadDesc;
typedef struct _RejillaScsiDVDReadDesc RejillaScsiDVDReadDesc;
typedef struct _RejillaScsiRandomWriteDesc RejillaScsiRandomWriteDesc;
typedef struct _RejillaScsiIncrementalWrtDesc RejillaScsiIncrementalWrtDesc;
typedef struct _RejillaScsiFormatDesc RejillaScsiFormatDesc;
typedef struct _RejillaScsiDefectMngDesc RejillaScsiDefectMngDesc;
typedef struct _RejillaScsiWrtOnceDesc RejillaScsiWrtOnceDesc;
typedef struct _RejillaScsiCDrwCavDesc RejillaScsiCDrwCavDesc;
typedef struct _RejillaScsiMRWDesc RejillaScsiMRWDesc;
typedef struct _RejillaScsiDefectReportDesc RejillaScsiDefectReportDesc;
typedef struct _RejillaScsiDVDRWplusDesc RejillaScsiDVDRWplusDesc;
typedef struct _RejillaScsiDVDRplusDesc RejillaScsiDVDRplusDesc;
typedef struct _RejillaScsiRigidOverwrtDesc RejillaScsiRigidOverwrtDesc;
typedef struct _RejillaScsiCDTAODesc RejillaScsiCDTAODesc;
typedef struct _RejillaScsiCDSAODesc RejillaScsiCDSAODesc;
typedef struct _RejillaScsiDVDRWlessWrtDesc RejillaScsiDVDRWlessWrtDesc;
typedef struct _RejillaScsiLayerJmpDesc RejillaScsiLayerJmpDesc;
typedef struct _RejillaScsiCDRWWrtDesc RejillaScsiCDRWWrtDesc;
typedef struct _RejillaScsiDVDRWDLDesc RejillaScsiDVDRWDLDesc;
typedef struct _RejillaScsiDVDRDLDesc RejillaScsiDVDRDLDesc;
typedef struct _RejillaScsiBDReadDesc RejillaScsiBDReadDesc;
typedef struct _RejillaScsiBDWriteDesc RejillaScsiBDWriteDesc;
typedef struct _RejillaScsiHDDVDReadDesc RejillaScsiHDDVDReadDesc;
typedef struct _RejillaScsiHDDVDWriteDesc RejillaScsiHDDVDWriteDesc;
typedef struct _RejillaScsiHybridDiscDesc RejillaScsiHybridDiscDesc;
typedef struct _RejillaScsiSmartDesc RejillaScsiSmartDesc;
typedef struct _RejillaScsiEmbedChngDesc RejillaScsiEmbedChngDesc;
typedef struct _RejillaScsiExtAudioPlayDesc RejillaScsiExtAudioPlayDesc;
typedef struct _RejillaScsiFirmwareUpgrDesc RejillaScsiFirmwareUpgrDesc;
typedef struct _RejillaScsiTimeoutDesc RejillaScsiTimeoutDesc;
typedef struct _RejillaScsiRTStreamDesc RejillaScsiRTStreamDesc;
typedef struct _RejillaScsiAACSDesc RejillaScsiAACSDesc;
typedef struct _RejillaScsiPOWDesc RejillaScsiPOWDesc;
typedef struct _RejillaScsiDVDCssDesc RejillaScsiDVDCssDesc;
typedef struct _RejillaScsiDriveSerialNumDesc RejillaScsiDriveSerialNumDesc;
typedef struct _RejillaScsiMediaSerialNumDesc RejillaScsiMediaSerialNumDesc;
typedef struct _RejillaScsiDiscCtlBlocksDesc RejillaScsiDiscCtlBlocksDesc;
typedef struct _RejillaScsiDVDCprmDesc RejillaScsiDVDCprmDesc;
typedef struct _RejillaScsiFirmwareDesc RejillaScsiFirmwareDesc;
typedef struct _RejillaScsiVPSDesc RejillaScsiVPSDesc;

struct _RejillaScsiGetConfigHdr {
	uchar len			[4];
	uchar reserved			[2];
	uchar current_profile		[2];

	RejillaScsiFeatureDesc desc 	[0];
};
typedef struct _RejillaScsiGetConfigHdr RejillaScsiGetConfigHdr;

G_END_DECLS

#endif /* _SCSI_GET_CONFIGURATION_H */

 
