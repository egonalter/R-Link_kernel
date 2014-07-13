/*
 * include/linux/kxr94.h
 *
 * Copyright (C) 2011 TomTom BV <http://www.tomtom.com/>
 * Authors: Matthias Kaehlcke <matthias.kaehlcke@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef INCLUDE_LINUX_KXR94_H
#define INCLUDE_LINUX_KXR94_H

struct kxr94_platform_data
{
	int sample_rate;	/* in ms */
	int zero_g_offset;	/* in ADC counts */
	int sensitivity;	/* ADC counts / g */
	int aux_in_ev_type;	/* event type for aux in channel (EV_ABS or EV_REL) */
	int aux_in_ev_code;	/* event code for aux in channel */
	int averaging_enabled;	/* enables averaging */
};

#endif
