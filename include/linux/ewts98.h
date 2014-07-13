/*
 * include/linux/ewts98.h
 *
 * Copyright (C) 2011 TomTom BV <http://www.tomtom.com/>
 * Authors: Matthias Kaehlcke <matthias.kaehlcke@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef INCLUDE_LINUX_EWTS98_H
#define INCLUDE_LINUX_EWTS98_H

#include <linux/types.h>

#define EWTS98_ERROR_X	(1 << 0)
#define EWTS98_ERROR_Y	(1 << 1)

#define EWTS98_INVALID_VALUE	0xffff

typedef enum
{
	EWTS98L,
	EWTS98C,
	EWTS9CV,
	EWTS98PA21,

	EWTS98_NUM_MODELS
} EWTS98_MODEL;

struct ewts98_adc_data
{
	int resolution;		/* in bits */
	int zero_point_offset;	/* in counts */
	int min_input_mV;	/* minimum input voltage in mV */
	int max_input_mV;	/* maximum input voltage in mV */
};

struct ewts98_adc_values
{
	uint16_t x;
	uint16_t y;
};

struct ewts98_platform_data
{
	EWTS98_MODEL model;
	int sample_rate;	/* in ms */

	struct {
		int enabled;
		int num_samples;
	} averaging;

	struct ewts98_adc_data adc_x;
	struct ewts98_adc_data adc_y;

	int (*read_adcs)(struct ewts98_adc_values *);	/* callback for reading the ADCs */

	int fuzz_x;		/* noise filtering for x axis (in mrad/s) */
	int fuzz_y;		/* noise filtering for y axis (in mrad/s) */
};

#endif
