/*
 * Copyright (C) 2011 TomTom BV <http://www.tomtom.com/>
 * Author: Niels Langendorff <niels.langendorff@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/vgpio.h>
#include <plat/mendoza.h>
#include <plat/tt_setup_handler.h>
#include <plat/bcm2070.h>

/* BT PART: Also for BT part the naming of the BT gpios are a bit confusing   */
/* BT_RST is more like an on/off signal. So it can be used as reset.          */
/* The EN signal is used to switch on internal LDOs. Whenever WLAN and BT EN  */
/* are both low then internal LDOs will be switched off.                      */
static int bcm2070_request_gpio( void )
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

static void bcm2070_free_gpio(void)
{
	gpio_free(TT_VGPIO_BT_RST);
	gpio_free(TT_VGPIO_BT_EN);
}

static void bcm2070_config_gpio(void)
{
	gpio_direction_output(TT_VGPIO_BT_RST, 1);
	gpio_direction_output(TT_VGPIO_BT_EN, 0);
}

static void bcm2070_suspend(void)
{
	gpio_direction_output(TT_VGPIO_BT_RST, 1);
	gpio_direction_output(TT_VGPIO_BT_EN, 0);
}

static void bcm2070_resume(void)
{
	gpio_direction_output(TT_VGPIO_BT_EN, 1);
	gpio_direction_output(TT_VGPIO_BT_RST, 0);
}

static void bcm2070_reset(int state)
{
	gpio_set_value(TT_VGPIO_BT_RST, state);
}

static void bcm2070_power(int state)
{
	gpio_direction_output(TT_VGPIO_BT_EN, state);
}

static struct bcm2070_platform_data bcm2070_pdata = 
{
	.suspend		= bcm2070_suspend,
	.resume			= bcm2070_resume,

	.config_gpio	= bcm2070_config_gpio,
	.request_gpio	= bcm2070_request_gpio,
	.free_gpio		= bcm2070_free_gpio,

	.reset			= bcm2070_reset,
	.power			= bcm2070_power,
};

static struct platform_device bcm2070_pdev = 
{
	.name 	= BCM2070_DEVNAME,
	.id 	= -1,
	.dev = {
		.platform_data = ((void *) &bcm2070_pdata),
	},
};

static int  __init bcm2070_init (char *str)
{
	printk("Initializing BCM2070\n");
	
	return platform_device_register( &bcm2070_pdev );
}
TT_SETUP_CB(bcm2070_init, "tomtom-bcm-bluetooth-bcm2070");

