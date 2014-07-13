/*
 * Copyright (C) 2011 TomTom BV <http://www.tomtom.com/>
 * Authors: Oreste Salerno <oreste.salerno@tomtom.com>
 *  
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/spi/spi.h>
#include <plat/mcspi.h>
#include <plat/offenburg.h>
#include "../../mach-omap2/mux.h"
#include <plat/apple_auth.h>

#include <asm/mach-types.h>
#include <mach/strasbourg_ver.h>

static struct omap2_mcspi_device_config apple_auth_mcspi_config = {
        .turbo_mode     = 0,
        .single_channel = 1,
};

static struct spi_board_info apple_auth_board_info[] = {
        {
                .modalias               = "spidev",
                .bus_num                = 3,
                .chip_select            = 0,
                .max_speed_hz           = 75000, 
                .controller_data        = &apple_auth_mcspi_config,
                .mode                   = SPI_MODE_1
        }
};

static void apple_auth_set_spi_mode (void)
{
	unsigned long msecs;

	/*
	 * The IPOD chip needs to be configured to use SPI instead of I2C
	 * To do this the MODE0 and MODE1 need to be set to 0, then the nRESET has
	 * to be set high. Next 30 ms needs to be waited for the reset cycle to
	 * complete
	 */

	/* Configure MODE0 and MODE1 as outputs and pull them down */
	/* NOTE: We do not use omap_mux_init_gpio() because of multiple gpio paths */
	omap_mux_init_signal ("sdmmc2_clk", OMAP_PIN_OUTPUT | OMAP_MUX_MODE4);
	omap_mux_init_signal ("sdmmc2_dat3", OMAP_PIN_OUTPUT | OMAP_MUX_MODE4);
	gpio_direction_output(TT_VGPIO_APPLE_AUTH_MODE0, 0);
	gpio_direction_output(TT_VGPIO_APPLE_AUTH_MODE1, 0);

	/* Reset Apple chip */
	gpio_direction_output (TT_VGPIO_APPLE_AUTH_RST, 1);

	/* Wait for 1 ms and release the reset */
	msecs = 1;
	while (msecs)
		msecs = msleep_interruptible(msecs);

	gpio_direction_output (TT_VGPIO_APPLE_AUTH_RST, 0);

	/* Wait for 30ms */
	msecs = 30;
	while (msecs)
		msecs = msleep_interruptible(msecs);

	/* Reconfigure the MODE0 and MODE1 signals as SPI3_CLK and SPI3_CS0 */
	omap_mux_init_signal ("sdmmc2_clk", OMAP_PIN_INPUT | OMAP_MUX_MODE1);
	omap_mux_init_signal ("sdmmc2_dat3", OMAP_MUX_MODE1);
}

static void apple_auth_set_i2c_mode (void)
{
	unsigned long msecs;

	/* Configure MODE0 and MODE1 as outputs and pull them up */
	/* NOTE: We do not use omap_mux_init_gpio() because of multiple gpio paths */
	omap_mux_init_signal ("i2c3_scl", OMAP_PIN_OUTPUT | OMAP_MUX_MODE4);
	omap_mux_init_signal ("i2c3_sda", OMAP_PIN_OUTPUT | OMAP_MUX_MODE4);
	gpio_direction_output(TT_VGPIO_APPLE_AUTH_MODE0, 1);
	gpio_direction_output(TT_VGPIO_APPLE_AUTH_MODE1, 1);

	/* Reset Apple chip */
	gpio_direction_output (TT_VGPIO_APPLE_AUTH_RST, 1);

	/* Wait for 15 ms and release the reset */
	msecs = 15;
	while (msecs)
		msecs = msleep_interruptible(msecs);

	/* Reconfigure the MODE0 and MODE1 signals as I2C3_SCL and I2C3_SDA */
	omap_mux_init_signal ("i2c3_scl", OMAP_PIN_INPUT | OMAP_MUX_MODE0);
	omap_mux_init_signal ("i2c3_sda", OMAP_PIN_INPUT | OMAP_MUX_MODE0);

	/* Wait for 1 ms and release the reset */
	msecs = 1;
	while (msecs)
		msecs = msleep_interruptible(msecs);

	gpio_direction_output (TT_VGPIO_APPLE_AUTH_RST, 0);

}

static int apple_auth_suspend(struct platform_device *pdev, pm_message_t state)
{
	/* Reset Apple chip */
	gpio_direction_output (TT_VGPIO_APPLE_AUTH_RST, 1);
	return 0;
}

static int apple_auth_resume_spi(struct platform_device *pdev)
{
	/* Set Apple chip into SPI mode */
	apple_auth_set_spi_mode ();
	return 0;
}

static int apple_auth_resume_i2c(struct platform_device *pdev)
{
	/* Set Apple chip into I2C mode */
	apple_auth_set_i2c_mode ();
	return 0;
}

static int __init apple_auth_spi_init(struct platform_device *pdev)
{
	int ret;

	ret = spi_register_board_info(apple_auth_board_info, ARRAY_SIZE(apple_auth_board_info));
	if (ret)
		return ret;

	return platform_device_register(pdev);
}

static int __init apple_auth_i2c_init(struct platform_device *pdev)
{
	return platform_device_register(pdev);
}

static struct apple_auth_platform_data platdata = {
	.init		= apple_auth_spi_init,
	.set_mode 	= apple_auth_set_spi_mode,
	.suspend	= apple_auth_suspend,
	.resume		= apple_auth_resume_spi,
};

static struct apple_auth_platform_data rennes_b1_platdata = {
	.init		= apple_auth_i2c_init,
	.set_mode	= apple_auth_set_i2c_mode,
	.suspend	= apple_auth_suspend,
	.resume		= apple_auth_resume_i2c,
};

static struct platform_device apple_auth_device = {
	.name			= "apple_auth",
	.dev.platform_data	= ((void *) &platdata),
};

static struct platform_device rennes_b1_apple_auth_device = {
	.name			= "apple_auth",
	.dev.platform_data	= ((void *) &rennes_b1_platdata),
};

struct mfd_feat apple_feats[] = {
	MFD_1_1(&rennes_b1_apple_auth_device),
	MFD_DEFAULT(&apple_auth_device),
};

static int __init apple_auth_init(void)
{
	int ret;
	struct platform_device *pdev;
	struct apple_auth_platform_data *pdata = NULL;

	ret = gpio_request(TT_VGPIO_APPLE_AUTH_RST, "APPLE_AUTH_RST");
	if (ret < 0) {
		printk(KERN_ERR "Failed to get GPIO %d for APPLE_AUTH_RST\n", TT_VGPIO_APPLE_AUTH_RST);
		return ret;
	}

	ret = gpio_request(TT_VGPIO_APPLE_AUTH_MODE0, "APPLE_AUTH_MODE0");
	if (ret < 0) {
		printk(KERN_ERR "Failed to get GPIO %d for APPLE_AUTH_MODE0\n", TT_VGPIO_APPLE_AUTH_MODE0);
		goto err_gpio_req_auth_mode0;
	}

	ret = gpio_request(TT_VGPIO_APPLE_AUTH_MODE1, "APPLE_AUTH_MODE1");
	if (ret < 0) {
		printk(KERN_ERR "Failed to get GPIO %d for APPLE_AUTH_MODE1\n", TT_VGPIO_APPLE_AUTH_MODE1);
		goto err_gpio_req_auth_mode1;
	}

	pdev = MFD_FEATURE(apple_feats);
	if (pdev)
		pdata = pdev->dev.platform_data;
	if (pdata && pdata->init)
		ret = pdata->init(pdev);
	if (ret)
		goto err_plat_init;

	return 0;

err_plat_init:
	gpio_free(TT_VGPIO_APPLE_AUTH_MODE1);
err_gpio_req_auth_mode1:
	gpio_free(TT_VGPIO_APPLE_AUTH_MODE0);
err_gpio_req_auth_mode0:
	gpio_free(TT_VGPIO_APPLE_AUTH_RST);

	return ret;
}

arch_initcall(apple_auth_init);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Offenburg Apple authentication module");
