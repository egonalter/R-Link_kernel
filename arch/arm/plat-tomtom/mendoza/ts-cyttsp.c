/*
 * Copyright (C) 2010, 2006 TomTom BV <http://www.tomtom.com/>
 * Authors: Vincent Dejouy <vincent.dejouy@tomtom.com>
 *          Matthias Kaehlcke <matthias.kaehlcke@tomtom.com>
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


#include <linux/delay.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/vgpio.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/cyttsp.h>
#include <plat/fdt.h>
#include <plat/mendoza.h>
#include <plat/tt_setup_handler.h>
#include <plat/gpio-cfg.h>
#include <plat/gpio-bank-n.h>
#include <asm/mach-types.h>

#define CYTTSP_DEVICE_ADDR		0x0A
#define CYTTSP_I2C_BUS			1

static char bootloader_key_fw_v2[] = {0x4E, 0x69, 0x4C, 0x61, 0x56, 0x69, 0x44, 0x65};

static void mendoza_cyttsp_reset(void)
{
	gpio_set_value(TT_VGPIO_TP_RESET, 1);
	msleep(100);
	gpio_set_value(TT_VGPIO_TP_RESET, 0);
}

static struct cyttsp_pdata cyttsp_pdata = 
{
	.reset_ts	= mendoza_cyttsp_reset,
};

static struct i2c_board_info cyttsp_dev_i2c_info = {
	.type = CYTTSP_DEVNAME,
	.addr = CYTTSP_DEVICE_ADDR,
	.platform_data = &cyttsp_pdata,
};

static int mendoza_cyttsp_i2c_init(void)
{
	struct i2c_adapter *adapter;
	struct i2c_client  *i2c_client_dev;
	const char *screen = fdt_get_string ("/features", "tft", "");

	cyttsp_dev_i2c_info.irq = gpio_to_irq(TT_VGPIO_TP_IRQ);
	memcpy(cyttsp_pdata.key, bootloader_key_fw_v2, SIZE_BOOTLOADER_KEY);

	/* 
	 * HACK: instead of hiding it in a header file
	 * Santiago LCM might be smaller than CTS, 
	 * check and correct xmax if needed
	 */
	if (!strcmp("ld050wv1sp01", screen)) 
		cyttsp_pdata.xmax = 762;
	else
		cyttsp_pdata.xmax = XMAX;

	adapter = i2c_get_adapter(CYTTSP_I2C_BUS);
	if (IS_ERR(adapter)) {
		printk(KERN_ERR "Could not get i2c adapter: %ld\n", PTR_ERR(adapter));
		return PTR_ERR(adapter);
	}

	i2c_client_dev = i2c_new_device(adapter, &cyttsp_dev_i2c_info);
	if (IS_ERR(i2c_client_dev)) {
		printk(KERN_ERR "Could not get i2c device: %ld\n", PTR_ERR(i2c_client_dev));
		return PTR_ERR(i2c_client_dev);
	}

	return 0;
}

static int __init mendoza_cyttsp_init(char *str)
{
	printk(KERN_INFO "Initializing Cypress TrueTouch controller\n");

	// TODO: should this be in a generic file (like lcm_init.c)?
	if (gpio_request(TT_VGPIO_TP_IRQ, "TP IRQ")     ||
		gpio_direction_input(TT_VGPIO_TP_IRQ)       ||
		s3c_gpio_setpull(vgpio_to_gpio(TT_VGPIO_TP_IRQ), S3C_GPIO_PULL_UP) ||
		gpio_request(TT_VGPIO_TP_RESET, "TP RESET") ||
		gpio_direction_output(TT_VGPIO_TP_RESET, 0)) {
		printk(KERN_ERR "ts-cyttsp: Failed to init GPIO\n");
		return -1;
	}

	if (machine_arch_type == MACH_TYPE_VALDEZ){
		s3c_gpio_cfgpin(vgpio_to_gpio(TT_VGPIO_TP_IRQ), S5P64XX_GPN7_EINT7);
	}

	mendoza_cyttsp_reset();

	mendoza_cyttsp_i2c_init();

	return 0;
}
TT_SETUP_CB(mendoza_cyttsp_init, "tomtom-mendoza-touchscreen-cypress");

