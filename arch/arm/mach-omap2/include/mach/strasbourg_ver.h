/*
 * Copyright (C) 2009 TomTom BV <http://www.tomtom.com/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#ifndef STRASBOURG_VER_H
#define STRASBOURG_VER_H

#include <mach/mfd_feat.h>

#define STRASBOURG_B1_SAMPLE 	0x0011
#define STRASBOURG_B2_SAMPLE 	0x0012
#define RENNES_A1_SAMPLE 	0x0013
#define RENNES_B1_SAMPLE 	0x0014
#define STUTTGART_B1_SAMPLE 	0x0015

#define MFD_1_0_B1(_feat)	MFD_FEAT_INIT(MACH_TYPE_STRASBOURG_A2, STRASBOURG_B1_SAMPLE, _feat)
#define MFD_1_0_B2(_feat)	MFD_FEAT_INIT(MACH_TYPE_STRASBOURG_A2, STRASBOURG_B2_SAMPLE, _feat)
#define MFD_1_0(_feat)		MFD_1_0_B1(_feat), MFD_1_0_B2(_feat)
#define MFD_1_0_5(_feat)	MFD_FEAT_INIT(MACH_TYPE_STRASBOURG_A2, RENNES_A1_SAMPLE, _feat)
#define MFD_1_1(_feat)		MFD_FEAT_INIT(MACH_TYPE_STRASBOURG_A2, RENNES_B1_SAMPLE, _feat)
#define MFD_2(_feat)		MFD_FEAT_INIT(MACH_TYPE_STRASBOURG_A2, STUTTGART_B1_SAMPLE, _feat)

static inline unsigned int get_strasbourg_ver(void)
{
	return get_mfd_rev();
}

#endif // STRASBOURG_VER_H
