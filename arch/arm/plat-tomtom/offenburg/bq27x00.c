/*
 * arch/arm/plat-tomtom/offenburg/
 *
 * Board glue for the BQ27x00 Fuel Gauge
 *
 * Copyright (C) 2010 TomTom International BV
 * Author: Niels Langendorff <niels.langendorff@tomtom.com>
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/vgpio.h>

#include <plat/offenburg.h>

#define BQ27X00_I2C_BUS		1

struct i2c_board_info bq27x00_i2c_board_info = {
	I2C_BOARD_INFO("bq27500", 0x55),
};

static int __init bq27x00_power(void)
{
	struct i2c_adapter	*adapter;
	struct i2c_client  *i2c_client_dev;

	adapter	= i2c_get_adapter(BQ27X00_I2C_BUS);
	if (IS_ERR(adapter)) {
		printk("Error getting the i2c adapter: %ld\n", PTR_ERR(adapter));
		return PTR_ERR(adapter);
	}

	i2c_client_dev = i2c_new_device(adapter, &bq27x00_i2c_board_info);
	if (IS_ERR(i2c_client_dev)) {
		printk("Error registering new device: %ld\n", PTR_ERR(i2c_client_dev));
		return PTR_ERR(i2c_client_dev);
	}

	return 0;
}

late_initcall(bq27x00_power);
