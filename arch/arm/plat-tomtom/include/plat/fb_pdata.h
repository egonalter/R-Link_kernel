/*
 * Copyright (C) 2006 TomTom BV <http://www.tomtom.com/>
 * Authors: Martin Jackson <martin.jackon@tomtom.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#ifndef __PLAT_LMS480WV_H
#define __PLAT_LMS480WV_H

struct s3cfb_platform_data {
	void (*enable_lcm)(void);	/* Callback to enable LCM */
	void (*reset_lcm)(void);	/* Callback to reset LCM */
	void (*suspend_lcm)(void);	/* Callback to suspend */
	void (*resume_lcm)(void);	/* Callback to resume */
	void *sdata;			/* LCM model specific data */
};

struct lms480wv_screen_data {
	int spiclk;	/* GPIO pin # for LCM bitbanged SPI clock */
	int spidata;	/* GPIO pin # for LCM bitbanged SPI MOSI */
	int spics;	/* GPIO pin # for LCM bitbanged SPI CS */
};

#endif /* __LMS480WV_PDATA_H */
