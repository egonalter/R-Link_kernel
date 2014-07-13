/*
 * Copyright (C) 2008 TomTom BV <http://www.tomtom.com/>
 * Author: Benoit Leffray <benoit.leffray@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/vgpio.h>
#include <mach/gpio.h>
#include <asm/delay.h>
#include <plat/mendoza.h>
#include <plat/gpio-cfg.h>
#include <plat/flipflop.h>

#define PFX		"flipflop: "

#ifndef CONFIG_TOMTOM_FLIPFLOP_HW
#define FLIPFLOP_REG_ADDR	0xE0100A10 /* INFORM4 */
#define FLIPFLOP_BIT		(1 << 31)

static void __iomem		*ff_reg = NULL;
#endif

static int flipflop_init(struct platform_device *pdev)
{
#ifdef CONFIG_TOMTOM_FLIPFLOP_HW
	int err=0;

	if (gpio_is_valid(TT_VGPIO_RFS_BOOT_CLK)) {
		err = gpio_request(TT_VGPIO_RFS_BOOT_CLK, "RFS_BOOT_CLK");

		if (err) {
			printk(KERN_ERR "error requesting RFS_BOOT_CLK %d\n", TT_VGPIO_RFS_BOOT_CLK);
			return err;
		}
	}

	if (gpio_is_valid(TT_VGPIO_RFS_BOOT_Q)) {
		err = gpio_request(TT_VGPIO_RFS_BOOT_Q, "RFS_BOOT_Q");

		if (err) {
			printk(KERN_ERR "error requesting RFS_BOOT_Q %d\n", TT_VGPIO_RFS_BOOT_Q);
			gpio_free(TT_VGPIO_RFS_BOOT_CLK);
			return err;
		}
	}

	gpio_direction_input(TT_VGPIO_RFS_BOOT_Q);

	gpio_direction_output(TT_VGPIO_RFS_BOOT_CLK, 0);
	s3c_gpio_setpull(vgpio_to_gpio(TT_VGPIO_RFS_BOOT_Q), S3C_GPIO_PULL_DOWN);

	return 0;
#else
	ff_reg = ioremap(FLIPFLOP_REG_ADDR, 4);
	if (ff_reg == NULL) {
		printk(KERN_ERR PFX "Unabled to map flipflop register\n");
		return -EINVAL;
	}

	return 0;
#endif
}

static void flipflop_release(struct device *dev)
{
#ifdef CONFIG_TOMTOM_FLIPFLOP_HW
	gpio_free(TT_VGPIO_RFS_BOOT_CLK);
	gpio_free(TT_VGPIO_RFS_BOOT_Q);
#else
	if (ff_reg != NULL) {
		iounmap(ff_reg);
		ff_reg = NULL;
	}
#endif
}

static void flipflop_set(int state)
{
#ifdef CONFIG_TOMTOM_FLIPFLOP_HW
        const int cur_state = gpio_get_value(TT_VGPIO_RFS_BOOT_Q);
        if (cur_state != state) {
                gpio_set_value(TT_VGPIO_RFS_BOOT_CLK, 1);
                udelay(10);
                gpio_set_value(TT_VGPIO_RFS_BOOT_CLK, 0);
        }
#else
	if (state)
		*((unsigned long *)ff_reg) |= FLIPFLOP_BIT;
	else
		*((unsigned long *)ff_reg) &= ~FLIPFLOP_BIT;
#endif
}

static int flipflop_get(void)
{
#ifdef CONFIG_TOMTOM_FLIPFLOP_HW
	const int state = gpio_get_value(TT_VGPIO_RFS_BOOT_Q);

	return state;
#else
	int state = 0;

	if (*((unsigned long *)ff_reg) & FLIPFLOP_BIT)
		state = 1;

	return state;
#endif
}

static struct flipflop_info flipflop_info = {
	.init = flipflop_init,
	.set_value = flipflop_set,
	.get_value = flipflop_get
};

static struct platform_device flipflop_pdev =
{
	.name   = "flipflop",
	.id		= -1,
	.dev = {
		.release = flipflop_release,
		.platform_data = &flipflop_info,
	},
};

static int __init flipflop_register(void)
{
	platform_device_register(&flipflop_pdev);

	printk(KERN_INFO PFX " Registered\n");
	return 0;
};

arch_initcall(flipflop_register);

