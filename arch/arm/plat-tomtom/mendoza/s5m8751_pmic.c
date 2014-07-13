/*
 *  S5M8751 PMIC PLATFORM DEVICE
 *
 *  Copyright (C) 2009 TomTom BV <http://www.tomtom.com/>
 *  Author: Andrzej Zukowski <andrzej.zukowski@tomtom.com>
 *
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
#include <mach/gpio.h>

#include <plat/mendoza.h>
#include <plat/mendoza_s5m8751.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-gpio.h>

static void s5m8751_pmic_poweroff(void)
{
	gpio_direction_output(TT_VGPIO_PWR_KILL, 0);
}

static int s5m8751_pmic_request_gpio(void)
{
    int err = 0;

    if ((err = gpio_request(TT_VGPIO_PWR_KILL, "TT_VGPIO_PWR_KILL")))
    {
        printk("%s: Can't request TT_VGPIO_PWR_KILL GPIO\n", __func__);
        goto err_pwr;
    }
    if ((err = gpio_request(TT_VGPIO_CHARGING, "TT_VGPIO_CHARGING")))
    {
        printk("%s: Can't request TT_VGPIO_PWR_KILL GPIO\n", __func__);
        goto err_pwr;
    }
err_pwr:
    return err;
}

static void s5m8751_pmic_config_gpio(void)
{
    uint32_t reg;

    // this GPIO is P port--semi alive. we need to do so to avoid wakeup-suicide.
    s3c_gpio_setpull(vgpio_to_gpio(TT_VGPIO_PWR_KILL),S3C_GPIO_PULL_UP);
    gpio_direction_input(TT_VGPIO_PWR_KILL);
    s3c_gpio_cfgpin_slp(vgpio_to_gpio(TT_VGPIO_PWR_KILL), S3C_GPIO_SLEEP_INPUT);
    s3c_gpio_setpull_slp(vgpio_to_gpio(TT_VGPIO_PWR_KILL),S3C_GPIO_PULL_UP);

    s3c_gpio_setpull(vgpio_to_gpio(TT_VGPIO_CHARGING),S3C_GPIO_PULL_NONE);
    gpio_direction_input(TT_VGPIO_CHARGING);
    s3c_gpio_cfgpin_slp(vgpio_to_gpio(TT_VGPIO_CHARGING), S3C_GPIO_SLEEP_INPUT);
    s3c_gpio_setpull_slp(vgpio_to_gpio(TT_VGPIO_CHARGING),S3C_GPIO_PULL_NONE);

    pm_power_off = s5m8751_pmic_poweroff;
}

static void s5m8751_pmic_free_gpio(void)
{
	pm_power_off = NULL;
	gpio_free(TT_VGPIO_PWR_KILL);
	gpio_free(TT_VGPIO_CHARGING);
}

static uint32_t s5m8751_pmic_get_charging(void)
{
    //USB_HOST_DETECT is low detected
    return (gpio_get_value(TT_VGPIO_CHARGING)>0&&gpio_get_value(TT_VGPIO_USB_HOST_DETECT)==0);
}

#ifdef CONFIG_MACH_CATANIA_S
static struct platform_s5m8751_pmic_data s5m8751_pmic_data = {
	.sleep_mode_state =	S5M8751_LDO1_EN |
				S5M8751_LDO3_EN |
				S5M8751_LDO5_EN |
				S5M8751_BCK1_EN,
	.wall_pwr_pin     = TT_VGPIO_WALL_ON,
	.wall_pwr_current = S5M8751_WALLADAPTER_550,

	.request_gpio     = s5m8751_pmic_request_gpio,
	.config_gpio      = s5m8751_pmic_config_gpio,
	.free_gpio        = s5m8751_pmic_free_gpio,
	.get_charging       = s5m8751_pmic_get_charging
};

#else
static struct platform_s5m8751_pmic_data s5m8751_pmic_data = {
	/* Those settings are the default on Havana.
	 * Please use mendoza_s5m8751_pmic_setup()
	 * on your favorite platform */
	.sleep_mode_state =	S5M8751_LDO1_EN |
				S5M8751_LDO3_EN |
				S5M8751_BCK1_EN,
	.wall_pwr_pin     = TT_VGPIO_WALL_ON,
	.wall_pwr_current = S5M8751_WALLADAPTER_550,

	.request_gpio     = s5m8751_pmic_request_gpio,
	.config_gpio      = s5m8751_pmic_config_gpio,
	.free_gpio        = s5m8751_pmic_free_gpio,
	.get_charging       = s5m8751_pmic_get_charging
};
#endif

struct platform_device mendoza_s5m8751_pmic = {
	.name		= "s5m8751-pmic",
	.id		= -1,
	.dev		= {
		.platform_data	= &s5m8751_pmic_data,
	}
};

void mendoza_s5m8751_pmic_setup(int sleep_mode_state)
{
	s5m8751_pmic_data.sleep_mode_state = sleep_mode_state;
}

