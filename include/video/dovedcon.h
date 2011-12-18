/*
 * linux/include/video/dovedcon.h -- Marvell DCON for DOVE
 *
 *
 * Copyright (C) Marvell Semiconductor Company.  All rights reserved.
 *
 * Written by Green Wan <gwan@marvell.com>
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License. See the file COPYING in the main directory of this archive for
 * more details.
 */
#ifndef _DOVEDCON_H_
#define	_DOVEDCON_H_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/tty.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/dma-mapping.h>
#include <linux/init.h>
#include <linux/ioport.h>
#include <linux/types.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <linux/clk.h>
#include <linux/fb.h>
#include <linux/notifier.h>
#include <linux/device.h>

#define _PA_LCD0_BYPASS	(0x00)
#define _PA_OLPC_DCON	(0x01)		/* display lcd0. */
#define _PA_DUAL_VIEW	(0x10)
#define _PA_DCON_MODE	(0x11)		/* LCD0 data */
#define _PB_LCD1_BYPASS	(0x00)
#define _PB_LCD0_BYPASS	(0x01)
#define _PB_RESERVED	(0x10)
#define _PB_DCON_MODE	(0x11)		/* LCD0 data */

#ifdef __KERNEL__

#define DCON_PORT_NUM 2
struct dovedcon_mach_info {
	unsigned int port_a;
	unsigned int port_b;
	char *clk_name[DCON_PORT_NUM];
};

struct dovedcon_info {
	void *reg_base;
	struct clk *clk[DCON_PORT_NUM];
	struct notifier_block fb_notif;
	unsigned int port_a;
	unsigned int port_b;
	unsigned int bias_adj[3];	/* bias adjustment. */
	unsigned int vcal_ena[3];	/* voltage calibration. */
	unsigned int vcal_range[3];	/* vol cal range */
	unsigned int ovol_detect[3];	/* over voltage detect. */
	unsigned int impd_detect[3];	/* impedance detect. */
	unsigned int use_37_5ohm[3];	/* Use 37.5 ohm load. */
	unsigned int use_75ohm[3];	/* Use 75 ohm load. */
	unsigned int gain[3];		/* DAC gain. */
};

#define to_dcon_device(obj) container_of(obj, struct dovedcon_info, dev)

/* Register map */
#define DCON_CTRL0			0x000
#define DCON_CTRL1			0x008
#define DCON_DITHER_RED			0x050
#define	DCON_DITHER_GREEN		0x054
#define	DCON_DITHER_BLUE		0x058
#define	DCON_DITHER_PATTERN_R_LOW	0x060
#define	DCON_DITHER_PATTERN_R_HIGH	0x064
#define	DCON_DITHER_PATTERN_G_LOW	0x068
#define	DCON_DITHER_PATTERN_G_HIGH	0x06c
#define	DCON_DITHER_PATTERN_B_LOW	0x070
#define	DCON_DITHER_PATTERN_B_HIGH	0x074
#define DCON_VGA_DAC_CTRL		0x080
#define DCON_VGA_DAC_CHANNEL_A_CTRL	0x084
#define DCON_VGA_DAC_CHANNEL_B_CTRL	0x088
#define DCON_VGA_DAC_CHANNEL_C_CTRL	0x08c
#define DCON_VGA_DAC_CHANNEL_A_STATUS	0x090
#define DCON_VGA_DAC_CHANNEL_B_STATUS	0x094
#define DCON_VGA_DAC_CHANNEL_C_STATUS	0x098
#define DCON_DUAL_VIEW_CTRL		0x0A0
#define DCON_CT_LUT_INDEX		0x0A4
#define DCON_CT_LUT_DATA		0x0A8

/* bit field & operation macros.*/
/* offset 0x000, control register 0. */
#define _LCD1_CLK		(0x1<<27)
#define _LCD0_CLK		(0x1<<26)
#define _VGA_CLK		(0x1<<25)
#define _DCON_CLK		(0x1<<24)
#define _DCON_RST		(0x1<<23)
#define _OUT_DELAY		(0x1<<22)
#define _DV_MODE(mode)		(mode<<18)
	#define	_DV_DISABLE	0x000
	#define	_DV_COMPOSITE	0x001
	#define	_DV_COLOR_RDR	0x011
	#define	_DV_LUT_TALK	0x101
	#define	_DV_FRC_LUT	0x111
#define _SMAB_DATAI		(0x1<<16)
#define _SMAB_DATAOE		(0x1<<15)
#define _SMAB_DATAO		(0x1<<14)
#define _SMAB_CLKI		(0x1<<13)
#define _SMAB_CLKOE		(0x1<<12)
#define _SMAB_CLKO		(0x1<<11)
#define _PORTB_SEL(mode)	(mode<<8)
#define _PORTA_SEL(mode)	(mode<<6)
#define _DCON_STATUS1		(0x1<<3)
#define _DCON_STATUS0		(0x1<<2)
#define _MONO_LUMI		(0x1<<2)
#define _COLOR_AA		(0x1<<1)
#define _DCON_BLANK		(0x1<<1)
#define _COLOR_SWIZZING		(0x1<<0)
#define _DCON_LOAD		(0x1<<0)


/* offset 0x008, control register 1. */
#define _BACK_LIGHT_CTRL(v)	(v<<16)		/* in OLPC Panel Mode. */

/* offset 0x050/0x054/0x058, dither for R/G/B pixel */
#define _DITHER_BITS(len)	(len<<8)
#define _DITHER_SHIFT(offset)	(offset<<1)
#define _DITHER_EN		(0x1<<0)

/* offset 0x060~0x074, dither pattern for RGB. Max length 64bits. */

/* offset 0x080, VGA DAC CTRL*/
#define _TST_BITS(ctrl)		(ctrl<<4)
#define _GDAC_LATCH_EN		(0x1<<3)
#define _PD_IREFP		(0x1<<2)		/* poly-P */
#define _PD_IREFC		(0x1<<1)		/* Reference external */
#define _PD_bg			(0x1<<0)		/* bandgap */

/* offset, 0x084~0x08c, DAC channel for A, B and C */
#define _POWER_DOWN		(0x1<<22)
#define _SEL_1P4		(0x1<<21)	/* DAC internal test bit. */
#define _OHM_65_GPULSE		(0x1<<20)	/* OHM adjust */
#define _BIAS_65_GPULSE		(0x1<<19)	/* Bias adjustment */
#define _VOL_CALIBRATION	(0x1<<18)	/* Voltage Calibration. */
#define _DIS_OVLT		(0x1<<16)	/* Disable OV detect */
#define _CALIBRATION_RANGE(r)	(r<<12)		/* full scale voltage */
#define _DIS_IMPEDANCE		(0x1<<11)	/* Disable impedance detect */
#define _ONLY_37P5		(0x1<<10)	/* Allow 37.5 Ohm load. */
#define _ONLY_75		(0x1<<10)	/* Allow 75 Ohm load. */
#define _GDAC(v)		(v<<1)		/* gain value */

/* offset, 0x090~0x098, DAC Channel A, B and C status. */
#define _POR_N			(0x1<<15)	/* power on reset state. */
#define _IMP			(0x3<<13)
	#define _SHORT		(0x0)		/*       Rload<27.5 ohm */
	#define _375OHM		(0x1)		/* 27.5<=Rload<55 ohm   */
	#define _750OHM		(0x2)		/* 55  <=Rload<110 ohm. */
	#define _OPEN		(0x3)		/* 110 <=Rload          */
#define _GDAC_INPUT		(0xFF<<5)	/* gain input */
#define _CAL_DONE		(0x1<<3)	/* calibration done. */
#define _CAL_OVR_FLOW		(0x1<<2)	/* overflow of calibration. */
#define _OVLT_EVENT		(0x1<<1)	/* over voltage */
#define _OVLT_FLAG		(0x1<<0)	/* over voltage flag. */

/* offset, 0x0A0 */
#define _CT_LEVEL(lv)		(lv<<24)	/* cross talk level. */
	#define _CT_LV0		(0x0)		/* 0. */
	#define _CT_LV1		(0x1)		/* 0.25 */
	#define _CT_LV2		(0x2)		/* 0.50 */
	#define _CT_LV3		(0x3)		/* 0.75 */
	#define _CT_LV4		(0x4)		/* 1.00 */
	#define _CT_LV5		(0x5)		/* 1.25 */
	#define _CT_LV6		(0x6)		/* 1.50 */
	#define _CT_LV7		(0x7)		/* 1.75 */
	#define _CT_LV8		(0x8)		/* 2.00 */
#define _RED_ALPHA(v)		(v<<16)		/* red. */
#define _GREEN_ALPHA(v)		(v<<8)		/* green. */
#define _BLUE_ALPHA(v)		(v<<0)		/* blue. */

/* offset, 0x0A4, Crosstalk Look up table index. Table 1 @ 0x0000_0xxx. */
/* offset, 0x0A8, Crosstalk Look up table data. */
#endif /* __KERNEL__ */
#endif /*_DOVEDCON_H_ */
