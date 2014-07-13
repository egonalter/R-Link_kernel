/*
 * Include file for the power button device.
 *
 * Author: Matthias Kaehlcke <matthias.kaehlcke@tomtom.com>
 *  Copyright (C) 2010 TomTom International BV
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation version 2.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#ifndef __LINUX_POWER_BUTTON_H__
#define __LINUX_POWER_BUTTON_H__

#include <linux/gpio_event.h>

struct powerbutton_pdata {
	/* gpio keys button corresponding to the power button */
	const struct gpio_event_direct_entry *keymap;

	void (*set_debounce_interval)(int time);
	int (*get_debounce_interval)(void);
};

#endif /* __LINUX_POWER_BUTTON_H__ */
