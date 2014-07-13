/*
 *  Copyright (C) 2008 by TomTom International BV. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as 
 * published by the Free Software Foundation.
 *
 *
 *
 */

#include <linux/platform_device.h>
#include <linux/io.h>
#include <mach/gpio.h>
#include <plat/tilt_sensor.h>
#include <plat/tt_setup_handler.h>
#include <plat/offenburg.h>

static int tilt_power = 0;

struct resource tilt_resources[] = {
	{
	 .name = "tilt_sensor",
	 .flags = IORESOURCE_IRQ,
	 },
};

static int tilt_power_get(void)
{
	return tilt_power;
}

static int tilt_power_set(int power_on)
{
	if (power_on) {
		gpio_direction_output(TT_VGPIO_TILT_PWR, 1);
		tilt_power = 1;
	} else {
		gpio_direction_output(TT_VGPIO_TILT_PWR, 0);
		tilt_power = 0;
	}
	return 0;
}

static int tilt_value(void)
{
	return gpio_get_value(TT_VGPIO_TILT_OUT);
}

static int config_gpio(void)
{
	int err = 0;

	if ((err = gpio_request(TT_VGPIO_TILT_OUT, "TT_VGPIO_TILT_OUT"))) {
		printk("%s: Can't request TT_VGPIO_TILT_OUT\n",
		       __func__);
		goto err_free1;
	}
	if ((err = gpio_request(TT_VGPIO_TILT_PWR, "TT_VGPIO_TILT_PWR"))) {
		printk("%s: Can't request TT_VGPIO_TILT_PWR\n",
		       __func__);
		goto err_free2;
	}
	/* config TILT_OUT pin as input */
	gpio_direction_input(TT_VGPIO_TILT_OUT);

	tilt_resources[0].start = gpio_to_irq(TT_VGPIO_TILT_OUT);
	tilt_resources[0].end = tilt_resources[0].start;

	/* config TILT_PWR pin as output */
	/* power off tilt first */
	tilt_power_set(0);

	return 0;

      err_free2:
	gpio_free(TT_VGPIO_TILT_OUT);
      err_free1:

	return err;
}

static struct tilt_sensor_platform_data tilt_sensor_pdata = {
	.tilt_power_set = tilt_power_set,
	.tilt_power_get = tilt_power_get,
	.tilt_value = tilt_value,
	.config_gpio = config_gpio,
};

static struct platform_device tilt_sensor_pdev = {
	.name = "tilt_sensor",
	.id = -1,
	.num_resources = ARRAY_SIZE(tilt_resources),
	.resource = tilt_resources,
	.dev = {
		.platform_data = (void *)&tilt_sensor_pdata,
		},
};

static int __init tt_tilt_setup(void)
{
	printk(KERN_INFO "Initializing Tilt sensor\n");

	BUG_ON(TT_VGPIO_TILT_PWR == 0);
	BUG_ON(TT_VGPIO_TILT_OUT == 0);

	return platform_device_register(&tilt_sensor_pdev);
};

arch_initcall(tt_tilt_setup);

