/*
 * SDRC register values for the Samsung K4X2G323PC-8GD8
 *
 * Copyright (C) 2010 TomTom 
 *
 * Oreste Salerno
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef ARCH_ARM_MACH_OMAP2_SDRAM_SAMSUNG_K4X2G323PC8GD8
#define ARCH_ARM_MACH_OMAP2_SDRAM_SAMSUNG_K4X2G323PC8GD8

#include <plat/sdrc.h>

/* Samsung K4X2G323PC-8GD8 */
static struct omap_sdrc_params k4x2g323pc8gd8_sdrc_params[] = {
	[0] = {
		.rate        = 200000000,
		.actim_ctrla = 0xc2e1b4c6,
		.actim_ctrlb = 0x00022218,
		.rfr_ctrl    = 0x0002db01,
		.mr          = 0x00000032,
	},
	[1] = {
		.rate        = 166000000,
		.actim_ctrla = 0xa29db485,
		.actim_ctrlb = 0x00022214,
		.rfr_ctrl    = 0x00025901,
		.mr          = 0x00000032,
	},
	[2] = {
		.rate        = 100000000,
		.actim_ctrla = 0x61912283,
		.actim_ctrlb = 0x0002220c,
		.rfr_ctrl    = 0x00015401,
		.mr          = 0x00000022,
	},
	[3] = {
		.rate        = 83000000,
		.actim_ctrla = 0x51512243,
		.actim_ctrlb = 0x0002220a,
		.rfr_ctrl    = 0x00011301,
		.mr          = 0x00000022,
	},
	[4] = {
		.rate        = 0
	},
};

#endif
