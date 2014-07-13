/* linux/arch/arm/plat-s5pc11x/setup-fimc0.c
 *
 * Copyright 2009 Samsung Electronics
 *	Jinsung Yang <jsgood.yang@samsung.com>
 *	http://samsungsemi.com/
 *
 * Base S5PC11X FIMC controller 0 gpio configuration
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/gpio.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <plat/clock.h>
#include <plat/gpio-cfg.h>
#include <plat/gpio-bank-q.h>
///#include <plat/gpio-bank-e1.h>
//#include <plat/gpio-bank-j0.h>
//#include <plat/gpio-bank-j1.h>
#include <asm/io.h>
#include <mach/map.h>

struct platform_device; /* don't need the contents */
#if 1
void s3c_fimc0_cfg_gpio(struct platform_device *pdev)
{
	int i;

	s3c_gpio_cfgpin(S5P6450_GPQ(0),S5P64XX_GPQ0_CAMIF_CLK_OUT);
	s3c_gpio_cfgpin(S5P6450_GPQ(1),S5P64XX_GPQ1_CAMIF_RSTOUTn);
	s3c_gpio_cfgpin(S5P6450_GPQ(2),S5P64XX_GPQ2_CAMIF_Pixel_CLK_IN);
	s3c_gpio_cfgpin(S5P6450_GPQ(3),S5P64XX_GPQ3_CAMIF_VSYNC);
	s3c_gpio_cfgpin(S5P6450_GPQ(4),S5P64XX_GPQ4_CAMIF_HREF);
	s3c_gpio_cfgpin(S5P6450_GPQ(5),S5P64XX_GPQ5_CAMIF_FIELD);
	s3c_gpio_cfgpin(S5P6450_GPQ(6),S5P64XX_GPQ6_CAMIF_DATA0);
	s3c_gpio_cfgpin(S5P6450_GPQ(7),S5P64XX_GPQ7_CAMIF_DATA1);
	s3c_gpio_cfgpin(S5P6450_GPQ(8),S5P64XX_GPQ8_CAMIF_DATA2);
	s3c_gpio_cfgpin(S5P6450_GPQ(9),S5P64XX_GPQ9_CAMIF_DATA3);
	s3c_gpio_cfgpin(S5P6450_GPQ(10),S5P64XX_GPQ10_CAMIF_DATA4);
	s3c_gpio_cfgpin(S5P6450_GPQ(11),S5P64XX_GPQ11_CAMIF_DATA5);
	s3c_gpio_cfgpin(S5P6450_GPQ(12),S5P64XX_GPQ12_CAMIF_DATA6);
	s3c_gpio_cfgpin(S5P6450_GPQ(13),S5P64XX_GPQ13_CAMIF_DATA7);

	for (i = 0; i < 14; i++)
		s3c_gpio_setpull(S5P6450_GPQ(i), S3C_GPIO_PULL_NONE);
}
#endif
int s3c_fimc_clk_on(struct platform_device *pdev, struct clk *clk)
{

	struct clk *lclk = NULL, *lclk_parent = NULL;
	int err;

	lclk = clk_get(&pdev->dev, "sclk_fimc");
	if (IS_ERR(lclk)) {
		dev_err(&pdev->dev, "failed to get local clock\n");
		goto err_clk1;
	}

	if (lclk->ops->set_parent) {
		lclk_parent = clk_get(&pdev->dev, "dout_mpll");
		if (IS_ERR(lclk_parent)) {
			dev_err(&pdev->dev, "failed to get parent of local clock\n");
			goto err_clk2;
		}

		lclk->parent = lclk_parent;
             if(lclk->ops->set_parent)
		err = lclk->ops->set_parent(lclk, lclk_parent);
		if (err) {
			dev_err(&pdev->dev, "failed to set parent of local clock\n");
			goto err_clk3;
		}

		if (lclk->ops->set_rate) {
			lclk->ops->set_rate(lclk, 166000000);
			dev_dbg(&pdev->dev, "set local clock rate to 166000000\n");
		}

		clk_put(lclk_parent);
	}

	clk_put(lclk);
	clk_enable(lclk);
#if 0
	/* be able to handle clock on/off only with this clock */
	clk = clk_get(&pdev->dev, "fimc");
	if (IS_ERR(clk)) {
		dev_err(&pdev->dev, "failed to get interface clock\n");
		goto err_clk3;
	}

	clk_enable(clk);
#endif
	return 0;

err_clk3:
	clk_put(lclk_parent);

err_clk2:
	clk_put(lclk);

err_clk1:
	return -EINVAL;
}

int s3c_fimc0_clk_off(struct platform_device *pdev, struct clk *clk)
{
	clk_disable(clk);
	clk_put(clk);

	clk = NULL;

	return 0;
}

