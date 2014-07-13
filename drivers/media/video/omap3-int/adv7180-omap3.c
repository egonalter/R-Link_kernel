/*
 * arch/arm/mach-omap2/board-strasbourg-camera.c
 *
 * Driver for Strasbourg TVP5151 video decoder
 *
 * arch/arm/mach-omap2/board-strasbourg-camera.h
 *
 * Copyright (C) 2011 TomTom International BV
 * Author: Martin Jackson <martin.jackson@tomtom.com>
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/string.h>
#include <linux/delay.h>

#include "../../../../arch/arm/plat-tomtom/offenburg/include/plat/offenburg.h"

/* Deps: i2c, ISP */

#define DESCR "Analog Devices ADV7180 video decoder internal driver for OMAP3 ISP"

static int use_bt656 = 0;
module_param(use_bt656, bool, S_IRUGO);
MODULE_PARM_DESC(use_bt656, "Use ITU-R BT.656 synchronization");

static unsigned const short __initdata adv7180_omap3_addrs[] = { 0x20, 0x21, I2C_CLIENT_END };

enum {
	GPIO_IN = 0,
	GPIO_OUT = 1,
};

static struct {
	const struct {
		int num;
		char *name;
		int dir;
		int init_out;	/* Initial value if GPIO_OUT */
	}
	gpios[4];
}
adv7180_omap3_pdata = {
	.gpios = { { TT_VGPIO_CAM_IRQ, "cam_irq", GPIO_IN },
		   { TT_VGPIO_CAM_PWR_ON, "cam_pwr_on", GPIO_OUT, 1 },
		   { TT_VGPIO_CAM_ON, "cam_on", GPIO_OUT, 1 },
		   { TT_VGPIO_CAM_RST, "cam_rst", GPIO_OUT, 1 }, },
};

static struct i2c_board_info adv7180_omap3_info = {
	.type = "adv7180_omap3",
	.flags = 0,
	.platform_data = &adv7180_omap3_pdata,
};

struct adv7180_omap3_i2c_client_data {
	struct i2c_adapter *adap;
	struct i2c_client *client;
};

static struct adv7180_omap3_i2c_client_data cdata;

int __init adv7180_omap3_init(void)
{
	int registered_gpios;
	int r;

	pr_info(DESCR);

#define request_gpios(g) ({							\
	int i, r = 0;								\
	for(i=0; i<ARRAY_SIZE(g); i++) {					\
		if (r |= !!gpio_request(g[i].num, g[i].name)) {			\
			i--;							\
			break;							\
		}								\
		if (g[i].dir == GPIO_OUT)					\
			r |= !!gpio_direction_output(g[i].num, g[i].init_out);	\
	}									\
	registered_gpios = r ? i : i-1;						\
	r;									\
})

	if (request_gpios(adv7180_omap3_pdata.gpios)) {
		pr_err("Failed to request all Camera GPIOs\n");
		r = -EBUSY;
		goto err;
	}
	msleep(20); /* Power on sequence isn't terribly well specified */

	gpio_set_value(TT_VGPIO_CAM_RST, 0); /* Deassert reset after 20ms */
	msleep(20);

	cdata.adap = i2c_get_adapter(2);
	if (cdata.adap == NULL) {
		pr_err("Couldn't get i2c adapter #2\n");
		r = -ENXIO;
		goto err1;
	}

	adv7180_omap3_info.irq = gpio_to_irq(TT_VGPIO_CAM_IRQ); /* Checked by driver for validity */
	cdata.client = i2c_new_probed_device(cdata.adap, &adv7180_omap3_info, adv7180_omap3_addrs);
	if (cdata.client == NULL) {
		pr_err("Couldn't detect adv7180 video decoder\n");
		r = -ENXIO;
		goto err1;
	}

	return 0;
err1:
	i2c_put_adapter(cdata.adap);
	memset(&cdata, 0, sizeof(cdata));
err:
	while (registered_gpios >= 0)
		gpio_free(adv7180_omap3_pdata.gpios[registered_gpios--].num);

	return r;
}

void __exit adv7180_omap3_exit(void)
{
	i2c_release_client(cdata.client);
	i2c_put_adapter(cdata.adap);
}

module_init(adv7180_omap3_init);
module_exit(adv7180_omap3_exit);

MODULE_DESCRIPTION(DESCR);
MODULE_AUTHOR("TomTom International BV");
MODULE_LICENSE("GPL");

