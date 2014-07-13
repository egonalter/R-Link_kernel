/* ltc3577-pmic.c
 *
 * Control driver for LTC3577 Regulator.
 *
 * Copyright (C) 2009 TomTom BV <http://www.tomtom.com/>
 * Authors: Benoit Leffray <benoit.leffray@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/ltc3577-regulator.h>

#define LTC3577_REG_PFX		LTC3577_REG_DEVNAME " Driver: " 

ltc3577_reg_pdata_t *ltc3577_reg_pdata;


int ltc3577_regulator_set_power(int power)
{
	int ret = 0;
	if (NULL != ltc3577_reg_pdata)
		ret = ltc3577_reg_pdata->set_power(power);
	else {
		printk(KERN_INFO LTC3577_REG_PFX 
			"set_power: no set_power callback provided!!!\n");
		ret = -EINVAL;
	}

	return ret;
}
EXPORT_SYMBOL(ltc3577_regulator_set_power);

static int ltc3577_reg_probe(struct platform_device *pdev)
{
	int ret = 0;

	BUG_ON(!pdev);
	ltc3577_reg_pdata = pdev->dev.platform_data;
	if (NULL == ltc3577_reg_pdata) {
		printk(KERN_INFO LTC3577_REG_PFX 
				"Probe: No Platform Data!!!\n");
		return -EINVAL;
	}

	printk(KERN_INFO LTC3577_REG_PFX "Probed.\n");

	if (NULL != ltc3577_reg_pdata->init) {
		ret = ltc3577_reg_pdata->init();
		if (0 == ret)
			printk(KERN_INFO LTC3577_REG_PFX "Initialized.\n");
		else {
			printk(KERN_INFO LTC3577_REG_PFX 
					"Initialization Failure!!!\n");
		}
	} else {
		printk(KERN_INFO LTC3577_REG_PFX 
				"No init callback provided!!!\n");
		ret = -EINVAL;
	}

	return ret;
}


struct platform_driver ltc3577_reg_driver = {
	.probe		= ltc3577_reg_probe,
	.driver		= {
		.name	= LTC3577_REG_DEVNAME,
	},
};
EXPORT_SYMBOL(ltc3577_reg_driver);


MODULE_AUTHOR("Benoit Leffray <benoit.leffray@tomtom.com>");
MODULE_DESCRIPTION("Driver for LTC3577 Regulator.");
MODULE_LICENSE("GPL");
