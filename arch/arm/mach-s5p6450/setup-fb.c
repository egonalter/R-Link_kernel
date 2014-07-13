/* linux/arch/arm/plat-s5p64xx/setup-fb.c
 *
 * Copyright 2009 Samsung Electronics
 *	Jinsung Yang <jsgood.yang@samsung.com>
 *	http://samsungsemi.com/
 *
 * Base S5P64XX FIMD gpio configuration
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <asm/io.h>
#include <plat/gpio-cfg.h>
#include <mach/regs-gpio.h>
#include <mach/map.h>
#include <plat/gpio-bank-f.h>
#include <plat/gpio-bank-n.h>
struct platform_device; /* don't need the contents */

void s3cfb_cfg_gpio(struct platform_device *pdev)
{
	unsigned int cfg;
	int i;

	/* select TFT LCD type (RGB I/F) */
	cfg = readl(S5P64XX_SPC_BASE);
	cfg &= ~S5P6450_SPCON_LCD_SEL_MASK;
	cfg |= S5P6450_SPCON_LCD_SEL_RGB;
	writel(cfg, S5P64XX_SPC_BASE);

	for (i = 0; i < 16; i++)
		s3c_gpio_cfgpin(S5P6450_GPI(i), S3C_GPIO_SFN(2));

	for (i = 0; i < 12; i++)
		s3c_gpio_cfgpin(S5P6450_GPJ(i), S3C_GPIO_SFN(2));
}

int s3cfb_backlight_onoff(struct platform_device *pdev,int onoff)
{
	unsigned int gpfcon;
	int err;

	err = gpio_request(S5P6450_BACKLIGHT_PIN, "GPIO");
	if (err) {
		printk(KERN_ERR "failed to request GPIO for "
			"lcd backlight control\n");
		return err;
	}

	if (onoff) {
		gpio_direction_output(S5P6450_BACKLIGHT_PIN, S5P6450_BACKLIGHT_PUD);
#ifdef CONFIG_BACKLIGHT_PWM
		s3c_gpio_cfgpin(S5P6450_BACKLIGHT_PIN,S5P64XX_GPF15_PWM_TOUT1);
#endif
	}
	else {
		gpio_direction_output(S5P6450_BACKLIGHT_PIN, ((~S5P6450_BACKLIGHT_PUD) & 0x1));
	}
	
	gpio_free(S5P6450_BACKLIGHT_PIN);
	gpfcon = __raw_readl(S5P64XX_GPFCON);

	return 0;
}
int s3cfb_reset_lcd_onoff(struct platform_device *pdev,int onoff)
{
	unsigned int gpncon;
	int err;

	err = gpio_request(S5P6450_LCD_RESET_PIN, "GPN");
	if (err) {
		printk(KERN_ERR "failed to request GPN for "
			"lcd reset control\n");
		return err;
	}

	gpio_direction_output(S5P6450_LCD_RESET_PIN, S5P6450_LCD_RESET_PUD);

	
	if (onoff) {
		gpio_set_value(S5P6450_LCD_RESET_PIN, (S5P6450_LCD_RESET_PUD));
		mdelay(10);
	} else {
#ifdef CONFIG_MACH_VALDEZ
	 	gpio_set_value(S5P6450_LCD_RESET_PIN, ((~S5P6450_LCD_RESET_PUD) & 0x1));
		mdelay(10);
#endif
		mdelay(10);
	}
	
	gpio_free(S5P6450_LCD_RESET_PIN);

	gpncon = __raw_readl(S5P64XX_GPNCON);

	return 0;
}

