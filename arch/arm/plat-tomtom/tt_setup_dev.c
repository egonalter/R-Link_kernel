/*
 * linux/arch/arm/mach-s3c6410/tt_setup_dev.c
 *
 * tt_setup platform device setup/initialization for TomTom platforms.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>

struct platform_device tomtom_device_ttsetup = {
	.name		= "ttsetup",
	.id		= -1,
};	
