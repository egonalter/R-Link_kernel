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
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <plat/offenburg.h>

static struct platform_device *micpwr_pdev;

static int micpwr_probe(struct platform_device *pdev)
{
	return  gpio_request(TT_VGPIO_MIC_ON, "MIC_ON") ||
		gpio_direction_output(TT_VGPIO_MIC_ON, 1);
}

static int micpwr_remove(struct platform_device *pdev)
{
	gpio_free(TT_VGPIO_MIC_ON);
 	return 0;
}

static int micpwr_suspend(struct platform_device *pdev, pm_message_t state)
{
	gpio_set_value(TT_VGPIO_MIC_ON, 0);
	return 0;
}

static int micpwr_resume(struct platform_device *pdev)
{
	gpio_set_value(TT_VGPIO_MIC_ON, 1);
	return 0;
}

static struct platform_driver micpwr_driver = {
	.probe = micpwr_probe,
	.remove	= micpwr_remove,
	.suspend = micpwr_suspend,
	.resume = micpwr_resume,
	.driver	= {
		.name	= "micpwr",
		.owner	= THIS_MODULE,
	}
};

static int __init micpwr_init(void)
{
	micpwr_pdev = platform_device_register_simple("micpwr", -1, NULL, 0);
	if (IS_ERR(micpwr_pdev))
		return PTR_ERR(micpwr_pdev);

	return platform_driver_probe(&micpwr_driver, micpwr_probe);
}

static void __exit micpwr_exit(void)
{
	platform_driver_unregister(&micpwr_driver);
	platform_device_unregister(micpwr_pdev);
}

late_initcall(micpwr_init);
module_exit(micpwr_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Offenburg microphone power module");
