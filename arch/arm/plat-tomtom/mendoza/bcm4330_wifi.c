/*
 * Copyright (C) 2011 TomTom BV <http://www.tomtom.com/>
 * Author: Niels Langendorff <niels.langendorff@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This file combines both the WiFi and BT part of the BCM4330
*/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/vgpio.h>
#include <plat/mendoza.h>
#include <plat/sdhci.h>
#include <linux/wlan_plat.h>
#include <linux/io.h>
#include <mach/map.h>
#include <mach/regs-clock.h>

#define HSMMC_CONTROL2N_OFFSET	0x80
#define CONTROL2N_SDCDSEL	(1 << 13)
#define XRTCCLKOUT_OE		(1 << 21)

/* WIFI PART */
static unsigned int bcm4330_wifi_cd;		/* WIFI virtual 'card detect' status */

static void __iomem		*hsmmc_regs;

static int bcm4330_wifi_reset(int state)
{
	return 0;
}

static int bcm4330_wifi_power(int state)
{
	printk(KERN_DEBUG "\nbcm4330_wifi_power %d\n", state);

	switch (state) {
		case 0:
			gpio_set_value(TT_VGPIO_WIFI_RST, 1);
			msleep(100);
			break;

		case 1:
			gpio_set_value(TT_VGPIO_WIFI_RST, 0);
			break;

		default:
			return -EINVAL;
			break;
	}

	return 0;
}

static int bcm4330_wifi_carddetect(int state)
{
	unsigned int ctrl2;

	bcm4330_wifi_cd = state;

	printk(KERN_DEBUG "bcm4330_wifi_carddetect %d\n", state);

	ctrl2 = readl(hsmmc_regs + HSMMC_CONTROL2N_OFFSET);
	ctrl2 |= CONTROL2N_SDCDSEL;
	writel(ctrl2, hsmmc_regs + HSMMC_CONTROL2N_OFFSET);

	return 0;
}

static struct wifi_platform_data bcm4330_wifi_control = {
	.set_power      = bcm4330_wifi_power,
	.set_reset      = bcm4330_wifi_reset,
	.set_carddetect = bcm4330_wifi_carddetect,
};

static struct platform_device bcm4330_wifi_device = {
	.name           = "bcm4330_wlan",
	.id             = 1,
	.dev            = {
		.platform_data = &bcm4330_wifi_control,
	},
};

static int __init bcm4330_wifi_init(void)
{
	unsigned int val;
	int err;;

	printk(KERN_DEBUG "Initializing BCM4330 WiFi\n");

	if ((err = gpio_request(TT_VGPIO_WIFI_RST, "TT_VGPIO_WIFI_RST"))) {
		printk(KERN_ERR "%s: Can't request TT_VGPIO_WIFI_RST GPIO\n", __func__);
		return err;
	}

	gpio_direction_output(TT_VGPIO_WIFI_RST, 1);

	hsmmc_regs = ioremap(S3C_PA_HSMMC0, S3C_SZ_HSMMC);
	if (!hsmmc_regs) {
		printk(KERN_ERR "failed to map hsmmc registers\n");
		gpio_free(TT_VGPIO_WIFI_RST);
		return -1;
	}

	printk(KERN_DEBUG "remapped hsmmc port 0 to %p\n", hsmmc_regs);

	/* enable XRTCCLKOUT */
	val = readl(S5P_OTHERS);
	val |= XRTCCLKOUT_OE;
	writel(val, S5P_OTHERS);

	return platform_device_register(&bcm4330_wifi_device);
}

// This module is to be initialized before the bcm4330 module
subsys_initcall(bcm4330_wifi_init);


