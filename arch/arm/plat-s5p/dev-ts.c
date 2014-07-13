/* linux/arch/arm/plat-s3c/dev-ts.c
 *
 * Copyright (c) 2008 Simtec Electronics
 *	Ben Dooks <ben@simtec.co.uk>
 *	http://armlinux.simtec.co.uk/
 *
 * S3C series device definition for hsmmc devices
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/platform_device.h>

#include <mach/map.h>
#include <asm/irq.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <plat/ts.h>
#include <plat/tt_setup_handler.h>

/* Touch srcreen */
static struct resource s3c_ts_resource[] = {
	[0] = {
		.start = IRQ_PENDN,
		.end   = IRQ_PENDN,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device s3c_device_ts = {
	.name		  = "s3c-ts",
	.id		  = -1,
	.num_resources	  = ARRAY_SIZE(s3c_ts_resource),
	.resource	  = s3c_ts_resource,
};

static struct s3c_ts_mach_info s3c_ts_platform __initdata = {
	.oversampling_shift	= 1,
	.resol_bit		= 12,
};

static int __init s3c_ts_set_platdata(char *str)
{
	struct s3c_ts_mach_info *npd;

	npd = kmalloc(sizeof(*npd), GFP_KERNEL);
	if (npd) {
		memcpy(npd, &s3c_ts_platform, sizeof(*npd));
		s3c_device_ts.dev.platform_data = npd;
		platform_device_register(&s3c_device_ts);
		return 0;
	} else {
		printk(KERN_ERR "no memory for Touchscreen platform data\n");
	}
	return -ENOMEM;
}
TT_SETUP_CB(s3c_ts_set_platdata, "tomtom-mendoza-resistive-touchscreen");

