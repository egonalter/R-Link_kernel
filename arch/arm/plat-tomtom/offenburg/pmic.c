/*
 * Copyright (C) 2010 TomTom BV <http://www.tomtom.com/>
 * Authors: Hante Meuleman <hante.meuleman@tomtom.com>
 *  
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/gpio.h>
#include <plat/offenburg.h>

static int pmic_probe(struct platform_device *pdev)
{
	gpio_request(TT_VGPIO_5V75_ON, "5V75 ON");
	gpio_direction_output(TT_VGPIO_5V75_ON, 0);
	msleep(500);

	return  gpio_request(TT_VGPIO_3V3_ON, "3V3 ON")    ||
		gpio_direction_output(TT_VGPIO_3V3_ON, 1)  ||
		gpio_request(TT_VGPIO_6V_ON, "6V ON")      ||
		gpio_direction_output(TT_VGPIO_6V_ON, 1)   ||
		gpio_direction_output(TT_VGPIO_5V75_ON, 1) ||
		gpio_request(TT_VGPIO_5V_ON, "5V ON")      ||
		gpio_direction_output(TT_VGPIO_5V_ON, 1);
}

static int pmic_remove(struct platform_device *pdev)
{
 	platform_device_unregister(pdev);
	return 0;
}

static int pmic_suspend(struct platform_device *pdev, pm_message_t state)
{
	gpio_set_value_cansleep(TT_VGPIO_3V3_ON, 0);
	gpio_set_value_cansleep(TT_VGPIO_6V_ON, 0);
	gpio_set_value_cansleep(TT_VGPIO_5V75_ON, 0);
	gpio_set_value_cansleep(TT_VGPIO_5V_ON, 0);

	printk(KERN_INFO "delay PMIC suspend to discharge VBUS...\n");
	mdelay(2000);

	return 0;
}

static int pmic_resume(struct platform_device *pdev)
{
	gpio_set_value_cansleep(TT_VGPIO_3V3_ON, 1);
	gpio_set_value_cansleep(TT_VGPIO_6V_ON, 1);
	gpio_set_value_cansleep(TT_VGPIO_5V75_ON, 1);
	gpio_set_value_cansleep(TT_VGPIO_5V_ON, 1);
	return 0;
}

static struct platform_driver pmic_driver = {
	.probe = pmic_probe,
	.remove	= pmic_remove,
	.suspend = pmic_suspend,
	.resume = pmic_resume,
	.driver	= {
		.name	= "pmic",
		.owner	= THIS_MODULE,
	}
};

static struct platform_device pmic_device = {
	.name	= "pmic",
};

static int __init pmic_init(void)
{
	platform_device_register(&pmic_device);
	return platform_driver_register(&pmic_driver);
}

static void __exit pmic_exit(void)
{
	platform_driver_unregister(&pmic_driver);
}

subsys_initcall_sync(pmic_init);
module_exit(pmic_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Offenburg pmic module");
