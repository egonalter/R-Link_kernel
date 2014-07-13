/* linux/arch/arm/plat-s5p/setup-i2c0.c
 *
 * Copyright (c) 2009 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * I2C0 GPIO configuration.
 *
 * Based on plat-s3c64xx/setup-i2c0.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>

struct platform_device; /* don't need the contents */

#include <mach/gpio.h>
#include <plat/iic.h>
#include <plat/gpio-bank-b.h>
#include <plat/gpio-cfg.h>


void s3c_i2c0_cfg_gpio(struct platform_device *dev)
{
        s3c_gpio_cfgpin(S5P6450_GPB(5), S5P64XX_GPB5_I2C_SCL0);
        s3c_gpio_cfgpin(S5P6450_GPB(6), S5P64XX_GPB6_I2C_SDA0);
        s3c_gpio_setpull(S5P6450_GPB(5), S3C_GPIO_PULL_UP);
        s3c_gpio_setpull(S5P6450_GPB(6), S3C_GPIO_PULL_UP);
}
