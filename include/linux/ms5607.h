/*
 * include/linux/ms5607.h
 *
 * Copyright (C) 2011 TomTom BV <http://www.tomtom.com/>
 * Authors: Matthias Kaehlcke <matthias.kaehlcke@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef INCLUDE_LINUX_MS5607_H
#define INCLUDE_LINUX_MS5607_H

typedef enum MS5607_OSR
{
	MS5607_OSR_256,
	MS5607_OSR_512,
	MS5607_OSR_1024,
	MS5607_OSR_2048,
	MS5607_OSR_4096,

	MS5607_NUM_OSR_CFG
} MS5607_OSR_t;

struct ms5607_platform_data
{
	int sample_rate;	/* in ms */
	MS5607_OSR_t osr;	/* OSR configuration */
};

#endif
