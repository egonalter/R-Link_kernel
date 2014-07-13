/*
 *  Audio PLATFORM DEVICE using the s5m8751 chip interface
 *
 *  Copyright (C) 2009 TomTom BV <http://www.tomtom.com/>
 *  Author: Niels Langendorff <niels.langendorff@tomtom.com>

 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 */

#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/kernel.h>
#include <linux/mfd/s5m8751/s5m8751_audio.h>

#include <mach/gpio.h>
#include <plat/mendoza.h>
#include <plat/gpio-cfg.h>

#include <plat/mendoza_s5m8751.h>

static int s5m8751_gpio_request(void)
{
	int err = 0;

	if (gpio_is_valid(TT_VGPIO_AMP_PWR_EN)) {
		if ((err = gpio_request(TT_VGPIO_AMP_PWR_EN, "TT_VGPIO_AMP_PWR_EN"))) {
			printk("%s: Can't request TT_VGPIO_AMP_PWR_EN GPIO\n", __func__);
		}
	}
	
	return err;
}

static void s5m8751_gpio_free(void)
{
	gpio_free(TT_VGPIO_AMP_PWR_EN);
}

static void s5m8751_config_gpio(void)
{

	s3c_gpio_setpull(vgpio_to_gpio(TT_VGPIO_AMP_PWR_EN), S3C_GPIO_PULL_DOWN);
	gpio_direction_output(TT_VGPIO_AMP_PWR_EN, 0);	
}

static void s5m8751_amp_pwr_en(int activate)
{
	gpio_set_value(TT_VGPIO_AMP_PWR_EN, activate);

}

static void s5m8751_suspend (void)
{
	gpio_direction_output(TT_VGPIO_AMP_PWR_EN, 0);
}

static void s5m8751_resume (void)
{
	gpio_direction_output(TT_VGPIO_AMP_PWR_EN, 1);
}

static s5m8751_platform_t s5m8751_pdata = {
	.amp_pwr_en		= s5m8751_amp_pwr_en,

	.suspend		= s5m8751_suspend,
	.resume			= s5m8751_resume,

	.config_gpio		= s5m8751_config_gpio,
	.request_gpio		= s5m8751_gpio_request,
	.free_gpio		= s5m8751_gpio_free,
};

struct platform_device mendoza_s5m8751_codec = {
	.name		= "s5m8751-audio",
	.id		= -1,
	.dev 		= {
			.platform_data = (void *) &s5m8751_pdata,
	},
};

