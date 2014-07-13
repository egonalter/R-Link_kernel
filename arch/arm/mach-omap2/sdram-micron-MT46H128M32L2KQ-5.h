/*
 * SDRC register values for the Micron MT46H128M32L2KQ-5
 *
 * Copyright (C) 2008 Texas Instruments, Inc.
 * Copyright (C) 2008-2009 Nokia Corporation
 *
 * Paul Walmsley
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef ARCH_ARM_MACH_OMAP2_SDRAM_MICRON_MT46H128M32L2KQ
#define ARCH_ARM_MACH_OMAP2_SDRAM_MICRON_MT46H128M32L2KQ

#include <plat/sdrc.h>

/* Micron MT46H128M32L2KQ-5 */
static struct omap_sdrc_params mt46h128m32l2kq5_sdrc_params[] = {
	[0] = {
		.rate        = 200000000,
		.actim_ctrla = 0x7ae1b4c6,
		.actim_ctrlb = 0x00021217,
		.rfr_ctrl    = 0x0005e601,
		.mr          = 0x00000032,
	},
	[1] = {
		.rate        = 166000000,
		.actim_ctrla = 0x629db4c6,
		.actim_ctrlb = 0x00011113,
		.rfr_ctrl    = 0x0004e201,
		.mr          = 0x00000032,
	},
	[2] = {
		.rate        = 100000000,
		.actim_ctrla = 0x41912284,
		.actim_ctrlb = 0x0001110c,
		.rfr_ctrl    = 0x0002da01,
		.mr          = 0x00000022,
	},
	[3] = {
		.rate        = 83000000,
		.actim_ctrla = 0x31512284,
		.actim_ctrlb = 0x0001110a,
		.rfr_ctrl    = 0x00025801,
		.mr          = 0x00000022,
	},
	[4] = {
		.rate        = 0
	},
};

#endif
