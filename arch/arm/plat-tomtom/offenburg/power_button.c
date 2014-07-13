/*
 * arch/arm/plat-tomtom/offenburg/power_button.c
 *
 * Power button platform driver for Offenburg
 *
 * Copyright (C) 2010 TomTom International BV
 * Author: Matthias Kaehlcke <matthias.kaehlcke@tomtom.com>
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */
#include <linux/gpio_event.h>
#include <linux/input.h>
#include <linux/platform_device.h>
#include <linux/power_button.h>
#include <plat/offenburg.h>

static struct gpio_event_direct_entry offenburg_keypad_switch_map[] = {
	{
		.gpio = TT_VGPIO_ON_OFF,
		.code = KEY_POWER,
	}
};

static struct gpio_event_input_info offenburg_keypad_switch_info = {
	.info.func = gpio_event_input_func,
	.info.no_suspend = false,
	.flags = GPIOEDF_PRINT_KEYS | GPIOEDF_ACTIVE_HIGH,
	.type = EV_KEY,
	.keymap = offenburg_keypad_switch_map,
	.keymap_size = ARRAY_SIZE(offenburg_keypad_switch_map)
};

static struct gpio_event_info *offenburg_keypad_info[] = {
	&offenburg_keypad_switch_info.info,
};

static struct gpio_event_platform_data offenburg_keypad_data = {
	.name = "power-button",
	.info = offenburg_keypad_info,
	.info_count = ARRAY_SIZE(offenburg_keypad_info)
};

static struct platform_device offenburg_keypad_device = {
	.name = GPIO_EVENT_DEV_NAME,
	.id = 0,
	.dev		= {
		.platform_data	= &offenburg_keypad_data,
	},
};

/* Set debounce interval time in mS */
static void set_debounce_time(int time)
{
	if (time > 1000) {
		/* Set Seconds */
		offenburg_keypad_switch_info.debounce_time.tv.sec = time / 1000;
		/* Calculate mS */
		time %= 1000;
	} else  {
		/* Set Seconds to 0 */
		offenburg_keypad_switch_info.debounce_time.tv.sec = 0;
	}
	
	/* Set mS */
	offenburg_keypad_switch_info.debounce_time.tv.nsec = time * NSEC_PER_MSEC;
}

/* Return debounce interval time in mS */
static int get_debounce_time(void)
{
	int time = offenburg_keypad_switch_info.debounce_time.tv.sec * 1000 + 
		(int)(offenburg_keypad_switch_info.debounce_time.tv.nsec / NSEC_PER_MSEC);

	return time;
}


/*****************************************************************************************/
/* power button                                                                          */
/*****************************************************************************************/

static struct powerbutton_pdata offenburg_pb_pdata =
{
	.keymap					= &offenburg_keypad_switch_map[0],
	.set_debounce_interval	= set_debounce_time,
	.get_debounce_interval	= get_debounce_time,
};

static struct platform_device offenburg_pb_pdev = {
	.name = "power-button",
	.id = -1,
	.num_resources	= 0,
	.dev = {
		.platform_data = &offenburg_pb_pdata,
	},
};

static int __init offenburg_pb_init(void)
{
	int rc;

	/* The debounce interval is set to 500 msec. If this needs to be changed then */
	/* it should be done in init.rc (matching device) and NOT here. The reason is */
	/* that the powerbutton might have different function before reaching android */
	/* and till then we dont want a to high debounce interval                     */
	set_debounce_time(500);

	rc = platform_device_register(&offenburg_keypad_device);
	if (!rc) {
		rc = platform_device_register(&offenburg_pb_pdev);
		if (rc)
			platform_device_unregister(&offenburg_keypad_device);
	}
	
	return rc;
}

arch_initcall(offenburg_pb_init);
