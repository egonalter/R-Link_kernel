/*
 *  I2C board info for s5m8751 wired to a mendoza board
 *
 *  Copyright (C) 2009 TomTom BV <http://www.tomtom.com/>
 *  Author: Marc Zyngier <marc.zyngier@tomtom.com>

 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 */

#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/mfd/s5m8751/s5m8751_core.h>

#include <plat/mendoza_s5m8751.h>

struct platform_device *mendoza_s5m8751_devs[] = {
	&mendoza_s5m8751_bl,
	&mendoza_s5m8751_codec,
	&mendoza_s5m8751_pmic,
};

static int mendoza_s5m8751_init(struct s5m8751 *s5m8751)
{
	int i, ret;

	for (i = 0 ; i < ARRAY_SIZE(mendoza_s5m8751_devs); i++) {
		if ((ret = s5m8751_client_register(s5m8751, mendoza_s5m8751_devs[i]))) {
			dev_err(s5m8751->dev, "Can't register %s (%d)\n", mendoza_s5m8751_devs[i]->name, ret);
			return ret;
		}
	}

	return 0;
}

static struct s5m8751_platform_data mendoza_s5m8751_pdata = {
	.init	= mendoza_s5m8751_init,
};

static struct i2c_board_info mendoza_s5m8751_i2c_info = {
	I2C_BOARD_INFO("s5m8751", 0x68),
	.platform_data = &mendoza_s5m8751_pdata,
};

int __init mendoza_s5m8751_i2c_init(int busnr)
{
	return i2c_register_board_info(busnr, &mendoza_s5m8751_i2c_info, 1);
}

