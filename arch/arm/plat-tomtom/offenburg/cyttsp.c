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
#include <linux/psoc-ctsic.h>
#include <plat/fdt.h>
#include <plat/offenburg.h>
#include <plat/tt_setup_handler.h>

#define CYTTSP_DEVICE_ADDR		0x0A
#define CYTTSP_I2C_BUS			1

static psoc_pdata_t cyttsp_pdata;

char bootloader_key_fw_v2[] = {0x4E, 0x69, 0x4C, 0x61, 0x56, 0x69, 0x44, 0x65};

static struct i2c_board_info cyttsp_dev_i2c_info = {
	.type = PSOC_DEVNAME,
	.addr = CYTTSP_DEVICE_ADDR,
	.platform_data = &cyttsp_pdata,
};

static void offenburg_cyttsp_reset(void)
{
	gpio_set_value(TT_VGPIO_TP_RESET, 1);
	msleep(100);
	gpio_set_value(TT_VGPIO_TP_RESET, 0);
}

static int offenburg_cyttsp_i2c_init(void)
{
	struct i2c_adapter *adapter;
	struct i2c_client  *i2c_client_dev;
	const char *screen = fdt_get_string ("/features", "tft", "");

	cyttsp_pdata.reset_ts = offenburg_cyttsp_reset;

	cyttsp_dev_i2c_info.irq = gpio_to_irq(TT_VGPIO_TP_IRQ);

	cyttsp_pdata.key = (char*) kmalloc(SIZE_BOOTLOADER_KEY * sizeof(char), GFP_KERNEL);
	memcpy(cyttsp_pdata.key, bootloader_key_fw_v2, SIZE_BOOTLOADER_KEY * sizeof(char));

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
		printk("Could not get i2c adapter: %ld\n", PTR_ERR(adapter));
		return PTR_ERR(adapter);
	}

	i2c_client_dev = i2c_new_device(adapter, &cyttsp_dev_i2c_info);
	if (IS_ERR(i2c_client_dev))
		return PTR_ERR(i2c_client_dev);

	return 0;
}

static int __init offenburg_cyttsp_init(void)
{
	printk(KERN_INFO "Initializing Cypress TrueTouch controller\n");

	// TODO: should this be in a generic file (like lcm_init.c)?
	if (gpio_request(TT_VGPIO_TP_IRQ, "TP IRQ")     ||
	    gpio_direction_input(TT_VGPIO_TP_IRQ)       ||
	    gpio_request(TT_VGPIO_TP_RESET, "TP RESET") ||
	    gpio_direction_output(TT_VGPIO_TP_RESET, 0)) {
		printk(KERN_INFO "ts-cyttsp: Failed to init GPIO\n");
		return -1;
	}

	offenburg_cyttsp_reset();

	offenburg_cyttsp_i2c_init();

	return 0;
}

// TODO: enable this and remove the late_initcall once we have factory data
// TT_SETUP_CB(offenburg_cyttsp_init, "tomtom-offenburg-touchscreen-cypress");
late_initcall(offenburg_cyttsp_init);
