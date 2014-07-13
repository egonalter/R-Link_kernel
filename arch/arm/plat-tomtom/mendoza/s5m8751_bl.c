/*
 *  Backlight PLATFORM DEVICE using the s5m8751 chip interface
 *
 *  Copyright (C) 2009 TomTom BV <http://www.tomtom.com/>
 *  Author: Andrzej Zukowski <andrzej.zukowski@tomtom.com>

 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 */

#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/mfd/s5m8751/s5m8751_backlight.h>

#include <plat/mendoza_s5m8751.h>

#define S5M8751_BL_MAX_INTENSITY	100
#define S5M8751_BL_DEFAULT_INTENSITY	35
#define S5M8751_1200_KHZ		1
#define S5M8751_600_KHZ			0

static int s5m8751_curve_lut[] = {
	 0,		/*   0 */
	 1,		/*   5 */
	 2,		/*  10 */
	 3,		/*  15 */
	 4,		/*  20 */
	 5,		/*  25 */
	 6,		/*  30 */
	 7,		/*  35 */
	 8,		/*  40 */
	 9,		/*  45 */
	 10,		/*  50 */
	 11,		/*  55 */
	 12,		/*  60 */
	 13,		/*  65 */
	 14,		/*  70 */
	 15,		/*  75 */
	 16,		/*  80 */
	 17,		/*  85 */
	 18,		/*  90 */
	 19,		/*  95 */
	 20,		/* 100 */
};

static int s5m8751_backlight_convert(int brightness)
{
	if (brightness > S5M8751_BL_MAX_INTENSITY)
		brightness = S5M8751_BL_MAX_INTENSITY;

	if (brightness < 0)
		brightness = 0;

	return s5m8751_curve_lut[brightness/5];
}

static struct platform_s5m8751_backlight_data s5m8751_backlight_data = {
	.max_brightness	= S5M8751_BL_MAX_INTENSITY,
	.dft_brightness	= S5M8751_BL_DEFAULT_INTENSITY,
	.pwm_freq	= S5M8751_1200_KHZ,
	.convert	= s5m8751_backlight_convert
};

struct platform_device mendoza_s5m8751_bl = {
	.name		= "s5m8751-backlight",
	.id		= -1,
	.dev		= {
		.platform_data	= &s5m8751_backlight_data,
	},
};
