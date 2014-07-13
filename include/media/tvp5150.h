/*
    tvp5150.h - definition for tvp5150 inputs

    Copyright (C) 2006 Hans Verkuil (hverkuil@xs4all.nl)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef _TVP5150_H_
#define _TVP5150_H_

#define TVP5150_XCLK_BT656		(27000000)

/* TVP5150 HW inputs */
#define TVP5150_COMPOSITE0 0
#define TVP5150_COMPOSITE1 1
#define TVP5150_SVIDEO     2

/* TVP5150 HW outputs */
#define TVP5150_NORMAL       0
#define TVP5150_BLACK_SCREEN 1

/* TVP5150 HW mode */
#define TVP5150_BT656       		 0
#define TVP5150_DISCRETE_SYNCS 1


/* Number of pixels and number of lines per frame for different standards */
#define NTSC_NUM_ACTIVE_PIXELS	(720)
#define NTSC_NUM_ACTIVE_LINES		(480)
#define PAL_NUM_ACTIVE_PIXELS		(720)
#define PAL_NUM_ACTIVE_LINES		(576)

/**
 * struct tvp5150_platform_data - Platform data values and access functions.
 * @power_set: Power state access function, zero is off, non-zero is on.
 * @ifparm: Interface parameters access function.
 * @priv_data_set: Device private data (pointer) access function.
 * @clk_polarity: Clock polarity of the current interface.
 * @ hs_polarity: HSYNC Polarity configuration for current interface.
 * @ vs_polarity: VSYNC Polarity configuration for current interface.
 */
struct tvp5150_platform_data {
	char *master;
	int (*power_set) (void *, int );
	int (*priv_data_set) (void *, void *);
	int (*suspend)(void *);
	int (*resume)(void *, int);
	/* Interface control params */
	int analog_input_channel;
	int digital_input_mode;
};

#endif

