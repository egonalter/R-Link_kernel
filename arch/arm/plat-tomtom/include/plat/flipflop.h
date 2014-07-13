/*
 * Copyright (C) 2008 TomTom BV <http://www.tomtom.com/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _LINUX_FLIPFLOP_H
#define _LINUX_FLIPFLOP_H

#include <linux/device.h>

struct flipflop_info {
	int (*init)(struct platform_device *pdev);
	void (*set_value)(int v);
	int (*get_value)(void);
};

#endif
