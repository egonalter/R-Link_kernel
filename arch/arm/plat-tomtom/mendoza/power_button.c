/*
 * arch/arm/plat-tomtom/mendoza/power_button.c
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
#include <linux/gpio.h>
#include <linux/vgpio.h>
#include <linux/platform_device.h>
#include <linux/power_button.h>
#include <plat/mendoza.h>
#include <plat/gpio-cfg.h>
#include <plat/gpio-bank-n.h>
#include <asm/mach-types.h>

static struct gpio_event_direct_entry mendoza_keypad_switch_map[] = {
	{
		.gpio = TT_VGPIO_ON_OFF,
		.code = KEY_POWER,
	}
};

static struct gpio_event_input_info mendoza_keypad_switch_info = {
	.info.func = gpio_event_input_func,
	.info.no_suspend = false,
	.flags = GPIOEDF_PRINT_KEYS | GPIOEDF_ACTIVE_HIGH,
	.type = EV_KEY,
	.keymap = mendoza_keypad_switch_map,
	.keymap_size = ARRAY_SIZE(mendoza_keypad_switch_map)
};

static struct gpio_event_info *mendoza_keypad_info[] = {
	&mendoza_keypad_switch_info.info,
};

static struct gpio_event_platform_data mendoza_keypad_data = {
	.name = "power-button",
	.info = mendoza_keypad_info,
	.info_count = ARRAY_SIZE(mendoza_keypad_info)
};

static struct platform_device mendoza_keypad_device = {
	.name = GPIO_EVENT_DEV_NAME,
	.id = 0,
	.dev		= {
		.platform_data	= &mendoza_keypad_data,
	},
};

/* Set debounce interval time in mS */
static void set_debounce_time(int time)
{
	if (time > 1000) {
		/* Set Seconds */
		mendoza_keypad_switch_info.debounce_time.tv.sec = time / 1000;
		/* Calculate mS */
		time %= 1000;
	} else  {
		/* Set Seconds to 0 */
		mendoza_keypad_switch_info.debounce_time.tv.sec = 0;
	}
	
	/* Set mS */
	mendoza_keypad_switch_info.debounce_time.tv.nsec = time * NSEC_PER_MSEC;
}

/* Return debounce interval time in mS */
static int get_debounce_time(void)
{
	int time = mendoza_keypad_switch_info.debounce_time.tv.sec * 1000 + 
		(int)(mendoza_keypad_switch_info.debounce_time.tv.nsec / NSEC_PER_MSEC);

	return time;
}


/*****************************************************************************************/
/* power button                                                                          */
/*****************************************************************************************/

static struct powerbutton_pdata mendoza_pb_pdata =
{
	.keymap					= &mendoza_keypad_switch_map[0],
	.set_debounce_interval	= set_debounce_time,
	.get_debounce_interval	= get_debounce_time,
};

static struct platform_device mendoza_pb_pdev = {
	.name = "power-button",
	.id = -1,
	.num_resources	= 0,
	.dev = {
		.platform_data = &mendoza_pb_pdata,
	},
};

static int __init mendoza_pb_init(void)
{
	int rc;

	/* needed as Samsung still needs to add a generic gpio function selection method */
	if (machine_arch_type == MACH_TYPE_TARANTO || machine_arch_type == MACH_TYPE_VALDEZ) {
		s3c_gpio_cfgpin(vgpio_to_gpio(TT_VGPIO_ON_OFF), S5P64XX_GPN4_EINT4);
		s3c_gpio_setpull(S5P6450_GPN(4), S3C_GPIO_PULL_NONE);
	}

	/* The debounce interval is set to 500 msec. If this needs to be changed then */
	/* it should be done in init.rc (matching device) and NOT here. The reason is */
	/* that the powerbutton might have different function before reaching android */
	/* and till then we dont want a to high debounce interval                     */
	set_debounce_time(500);

	rc = platform_device_register(&mendoza_keypad_device);
	if (!rc) {
		rc = platform_device_register(&mendoza_pb_pdev);
		if (rc)
			platform_device_unregister(&mendoza_keypad_device);
	}
	
	return rc;
}

arch_initcall(mendoza_pb_init);
