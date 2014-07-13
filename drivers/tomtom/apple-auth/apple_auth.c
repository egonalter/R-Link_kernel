/*
 * Copyright (C) 2011 TomTom BV <http://www.tomtom.com/>
 * Authors: Oreste Salerno <oreste.salerno@tomtom.com>
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
#include <plat/apple_auth.h>

static int apple_auth_probe(struct platform_device *pdev)
{
	struct apple_auth_platform_data *pdata;

	pdata = pdev->dev.platform_data;
	if (pdata == NULL)
		return -1;

	if (pdata->set_mode)
		pdata->set_mode();
	else
		printk(KERN_INFO "Registering default interface to Apple Auth chip\n");

	return 0;
}

static int apple_auth_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct apple_auth_platform_data *pdata;

	pdata = pdev->dev.platform_data;
	if (pdata == NULL)
		return -1;

	return pdata->suspend ? pdata->suspend(pdev, state) : 0;
}

static int apple_auth_resume(struct platform_device *pdev)
{
	struct apple_auth_platform_data *pdata;

	pdata = pdev->dev.platform_data;
	if (pdata == NULL)
		return -1;

	return pdata->resume ? pdata->resume(pdev) : 0;
}

static struct platform_driver apple_auth_driver = {
	.probe 	 = apple_auth_probe,
	.suspend = apple_auth_suspend,
	.resume  = apple_auth_resume,
	.driver	 = {
		.name	= "apple_auth",
		.owner	= THIS_MODULE,
	}
};

static int __init apple_auth_init(void)
{
	return platform_driver_register(&apple_auth_driver);
}

static void __exit apple_auth_exit(void)
{
	platform_driver_unregister(&apple_auth_driver);
}
module_init(apple_auth_init);
module_exit(apple_auth_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Offenburg Apple authentication module");
