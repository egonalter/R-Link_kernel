/*
 * Windscreen dock with RDSTMC receiver
 *
 *
 *  Copyright (C) 2008 by TomTom International BV. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as 
 * published by the Free Software Foundation.
 *
 *
 * ** See include/asm-arm/plat-tomtom/dock.h for documentation **
 *
 */

#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <plat/dock.h>

#define PFX "RDSDOCK windscr_rds"
#ifdef DOCK_DEBUG
#define DBG(fmt, args...) printk(KERN_DEBUG PFX fmt, ## args)
#else
#define DBG(fmt, args...)
#endif
#define DBG_PRINT_FUNC() DBG(" Function %s\n", __func__);

static int dock_windscr_nords_probe_id(void *fn_data)
{
	/* Priority 1, if another dock type with the same detid is more appropriate, its lookup function will return a higher priority */
	return 1;
}

static struct dock_type dock_type_windscr_nords = {
	.dock_name	= "DOCK_WINDSCR_NO_RDS",
	.detection_id	= DETID_NOPWR(0,1) | DETID_PWR(0,1),
	.connect	= NULL,
	.disconnect	= NULL,
	.reset_devices	= NULL,
	.probe_id	= dock_windscr_nords_probe_id,
};

int __init dock_windscr_nords_init(void)
{
	dock_type_register(&dock_type_windscr_nords);
	return 0;
}
