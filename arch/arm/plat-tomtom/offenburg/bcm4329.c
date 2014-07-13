/*
 * Copyright (C) 2011 TomTom BV <http://www.tomtom.com/>
 * Author: Niels Langendorff <niels.langendorff@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This file combines both the WiFi and BT part of the BCM4329
*/
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/vgpio.h>
#include <plat/offenburg.h>
#include <plat/bcm4329-bt.h>
#include <linux/wlan_plat.h>

/* WIFI PART */
static int bcm4329_wifi_cd;		/* WIFI virtual 'card detect' status */
static void (*wifi_status_cb)(int card_present, void *dev_id);
static void *wifi_status_cb_devid;

int omap_wifi_status_register(void (*callback)(int card_present,
						void *dev_id), void *dev_id)
{
	if (wifi_status_cb)
		return -EAGAIN;
	wifi_status_cb = callback;
	wifi_status_cb_devid = dev_id;
	return 0;
}

int omap_wifi_status(int irq)
{
	return bcm4329_wifi_cd;
}

static int bcm4329_wifi_request_gpio( void )
{
	int err = 0;

	if ((err = gpio_request(TT_VGPIO_WIFI_RST, "TT_VGPIO_WIFI_RST"))) {
		printk("%s: Can't request TT_VGPIO_WIFI_RST GPIO\n", __func__);
		return err;
	}

	if ((err = gpio_request(TT_VGPIO_WIFI_EN, "TT_VGPIO_WIFI_EN"))) {
		printk("%s: Can't request TT_VGPIO_WIFI_EN GPIO\n", __func__);
		return err;
	}

	return err;
}

static void bcm4329_wifi_config_gpio(void)
{
	gpio_direction_output(TT_VGPIO_WIFI_RST, 1);
	gpio_direction_output(TT_VGPIO_WIFI_EN, 0);
}

static int bcm4329_wifi_reset(int state)
{
	return 0;
}

static int bcm4329_wifi_power(int state)
{
	/* The BCM4329 (Murata module) seems to use RST as a power on/off     */
	/* The WIFI_EN gpio is used to possibly shutdown LDO (if BT is also   */
	/* off, but does not control the chip. So use the RST to shutdown     */
	switch(state) {
		case 0:
			gpio_direction_output(TT_VGPIO_WIFI_RST, 1);
			gpio_direction_output(TT_VGPIO_WIFI_EN, 0);
			break;
		case 1:
			gpio_direction_output(TT_VGPIO_WIFI_EN, 1);
			gpio_direction_output(TT_VGPIO_WIFI_RST, 0);
			break;
		default:
			return -EINVAL;
			break;
	}

	return 0;
}

static int bcm4329_wifi_carddetect(int state)
{
	bcm4329_wifi_cd = state;

	if (wifi_status_cb) {
		wifi_status_cb(state, wifi_status_cb_devid);
	}

	return 0;
}

static struct wifi_platform_data bcm4329_wifi_control = {
	.set_power      = bcm4329_wifi_power,
	.set_reset      = bcm4329_wifi_reset,
	.set_carddetect = bcm4329_wifi_carddetect,
};

static struct platform_device bcm4329_wifi_device = {
	.name           = "bcm4329_wlan",
	.id             = 1,
	.dev            = {
			.platform_data = &bcm4329_wifi_control,
	},
};

#ifndef CONFIG_BCM4329_MODULE
void config_bcm4329_power (void) {

	if(bcm4329_wifi_request_gpio()){
		printk(KERN_INFO"BCM4329: error requesting gpio\n");
		return;
	}
	bcm4329_wifi_config_gpio();
	bcm4329_wifi_power(1);
}
#endif

static int __init bcm4329_wifi_init(void)
{
	printk("Initializing BCM4329 WiFi\n");

#ifdef CONFIG_BCM4329_MODULE
	if(bcm4329_wifi_request_gpio()){
		printk(KERN_INFO"BCM4329: error requesting gpio\n");
		return 1;
	}
	bcm4329_wifi_config_gpio();
#endif

	return platform_device_register(&bcm4329_wifi_device);
}
// This module is to be initialized before the bcm4329 module. Otherwise it 
// simply wont work.
subsys_initcall(bcm4329_wifi_init);


/* BT PART: Also for BT part the naming of the BT gpios are a bit confusing   */
/* BT_RST is more like an on/off signal. So it can be used as reset.          */
/* The EN signal is used to switch on internal LDOs. Whenever WLAN and BT EN  */
/* are both low then internal LDOs will be switched off.                      */
static int bcm4329_bt_request_gpio( void )
{
	int err = 0;

	if ((err = gpio_request(TT_VGPIO_BT_RST, "TT_VGPIO_BT_RST"))) {
		printk("%s: Can't request TT_VGPIO_BT_RST GPIO\n", __func__);
		return err;
	}

	if ((err = gpio_request(TT_VGPIO_BT_EN, "TT_VGPIO_BT_EN"))) {
		printk("%s: Can't request TT_VGPIO_BT_EN GPIO\n", __func__);
		return err;
	}

	return err;
}

static void bcm4329_bt_free_gpio(void)
{
	gpio_free(TT_VGPIO_BT_RST);
	gpio_free(TT_VGPIO_BT_EN);
}

static void bcm4329_bt_config_gpio(void)
{
	gpio_direction_output(TT_VGPIO_BT_RST, 1);
	gpio_direction_output(TT_VGPIO_BT_EN, 0);
}

static void bcm4329_bt_suspend(void)
{
	gpio_direction_output(TT_VGPIO_BT_RST, 1);
	gpio_direction_output(TT_VGPIO_BT_EN, 0);
}

static void bcm4329_bt_resume(void)
{
	gpio_direction_output(TT_VGPIO_BT_EN, 1);
	gpio_direction_output(TT_VGPIO_BT_RST, 0);
}

static void bcm4329_bt_reset(int state)
{
}

static void bcm4329_bt_power(int state)
{
	if (state) {
		gpio_direction_output(TT_VGPIO_BT_EN, 1);
		gpio_direction_output(TT_VGPIO_BT_RST, 0);
	}
	else {
		gpio_direction_output(TT_VGPIO_BT_RST, 1);
		gpio_direction_output(TT_VGPIO_BT_EN, 0);
	}
}

static struct bcm4329_platform_data bcm4329_pdata = 
{
	.suspend		= bcm4329_bt_suspend,
	.resume			= bcm4329_bt_resume,

	.config_gpio		= bcm4329_bt_config_gpio,
	.request_gpio		= bcm4329_bt_request_gpio,
	.free_gpio		= bcm4329_bt_free_gpio,

	.reset			= bcm4329_bt_reset,
	.power			= bcm4329_bt_power,
};

static struct platform_device bcm4329_pdev = 
{
	.name 	= "bcm4329-bt",
	.id 	= -1,
	.dev = {
		.platform_data = ((void *) &bcm4329_pdata),
	},
};

static int  __init bcm4329_bt_init (void)
{
	printk("Initializing BCM4329 BT\n");
	
	return platform_device_register( &bcm4329_pdev );
}
arch_initcall( bcm4329_bt_init );


