/*
 * Include file for the power button device.
 *
 * Author: Mark Vels <mark.vels@tomtom.com>
 *  * (C) Copyright 2008 TomTom International BV.
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

#include <linux/platform_device.h>
#include <plat/tt_mem.h>
#include <linux/device.h>

/* Defines */
#define PFX "pb: "
#define DRIVER_DESC_LONG "TomTom Memory driver, (C) 2010 TomTom BV "

#define PB_ERR(fmt, args...) \
	printk(KERN_ERR PFX "##ERROR## :" fmt "\n", ##args )

#ifdef PB_DEBUG
#define PB_DBG(fmt, args...) printk( KERN_DEBUG PFX fmt "\n", ##args )
#else
#define PB_DBG(fmt, args...)
#endif

static int mem_probe(struct platform_device *pdev)
{
	struct ttmem_info *mem_info;

	printk (DRIVER_DESC_LONG "\n");
	mem_info = (struct ttmem_info *)(pdev->dev.platform_data);

	mem_info->allow();

	return 0;
}
 
static int mem_remove(struct platform_device *pdev)
{
	struct ttmem_info *mem_info;

	mem_info = (struct ttmem_info *)(pdev->dev.platform_data);
	mem_info->disallow();

	return 0;
}

static struct platform_driver mem_driver = {
	.driver		= {
		.name	= "ttmem",
	},
	.probe		= mem_probe,
	.remove		= mem_remove,
};
 
static int __init mem_init (void)
{
	int ret;

	ret = platform_driver_register(&mem_driver);
	if(ret) {
		PB_ERR("Could not register mem platform driver! Error=%d", ret);
	} 

	return ret;
}

static void __exit mem_exit (void)
{
	
	platform_driver_unregister(&mem_driver);
}

module_init (mem_init);
module_exit (mem_exit);

MODULE_DESCRIPTION(DRIVER_DESC_LONG);
MODULE_LICENSE("GPL");
