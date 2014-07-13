/*
 * Copyright (C) 2010 TomTom BV <http://www.tomtom.com/>
 * Author: Oreste Salerno <oreste.salerno@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/vgpio.h>
#include <linux/platform_device.h>
#include <plat/offenburg.h>

static struct resource pps_resources[] =
{
        {
                .name           = "pps-irq",
                .flags          = IORESOURCE_IRQ,
        },
};

static struct platform_device pps_device = {
        .name           = "tomtom-pps",
        .id             = -1,
        .num_resources  = ARRAY_SIZE(pps_resources),
        .resource       = pps_resources
};

static __init int register_pps_device(void)
{
	int ret;

 	ret = gpio_request(TT_VGPIO_GPS_1PPS, "GPS_1PPS");
	if (ret) {
		printk(KERN_ERR "PPS: Can't request GPS_1PPS");
		return ret;
	}		
	ret = gpio_direction_input(TT_VGPIO_GPS_1PPS);
	if (ret) {
		printk(KERN_ERR "PPS: Can't set GPS_1PPS direction");
		return ret;
	}
        pps_resources[0].start  = gpio_to_irq(TT_VGPIO_GPS_1PPS);
        pps_resources[0].end    = pps_resources[0].start;

	return platform_device_register(&pps_device);
}

arch_initcall(register_pps_device);
