/*
 * arch/arm/mach-omap2/board-strasbourg-camera.c
 *
 * Driver for Strasbourg TVP5151 video decoder
 *
 * arch/arm/mach-omap2/board-strasbourg-camera.h
 *
 * Copyright (C) 2010 Texas Instruments Inc
 * Author: Gilles Talis <g-talis@ti.com>
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

#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/mm.h>
#include <linux/delay.h>

#include <plat/mux.h>
#include <plat/board.h>

/* Include V4L2 ISP-Camera driver related header file */
#include <plat/vrfb.h>

#include <media/videobuf-dma-sg.h>
#include <media/v4l2-device.h>
#include <../drivers/media/video/omap/omap_voutdef.h>
#include <../drivers/media/video/isp/ispreg.h>

#define MODULE_NAME "omap_vout"

static struct resource offenburg_vout_resource[1] = { };

#if CONFIG_PM
struct vout_platform_data offenburg_vout_data = {
	.set_min_bus_tput	= omap_pm_set_min_bus_tput,
	.set_max_mpu_wakeup_lat	= omap_pm_set_max_mpu_wakeup_lat,
	.set_min_mpu_freq	= omap_pm_set_min_mpu_freq,
};
#endif

static struct platform_device offenburg_vout_device = {
	.name 		= "omap_vout",
	.num_resources 	= ARRAY_SIZE(offenburg_vout_resource),
	.resource 	= &offenburg_vout_resource[0],
	.id		= -1,
#ifdef CONFIG_PM
	.dev = {
	 	.platform_data = &offenburg_vout_data,
	}
#else
	.dev = {
		.platform_data = NULL,
	}
#endif
};

/**
 * @brief offenburg_vout_init - module init function. Should be called before any
 *                          client driver init call
 *
 * @return result of operation - 0 is success
 */
int __init offenburg_vout_init(void)
{
	int err;

	printk(KERN_INFO MODULE_NAME ": Driver registration starting \n");
	err = platform_device_register(&offenburg_vout_device);

	return 0;
}

arch_initcall(offenburg_vout_init);
