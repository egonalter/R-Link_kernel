/*
 * Copyright (C) 2013 TomTom BV <http://www.tomtom.com/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef INCLUDE_LINUX_LM26_H
#define INCLUDE_LINUX_LM26_H

struct lm26_platform_data
{
	unsigned int sample_rate; /* in second */
	int (*read_raw_adc)(int *raw_adc_value);
};

#endif
