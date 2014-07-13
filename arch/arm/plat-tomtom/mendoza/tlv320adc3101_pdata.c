/*
 * Copyright (C) 2009 TomTom BV <http://www.tomtom.com/>
 * Authors: Niels Langendorff <niels.langendorff@tomtom.com>
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
#include <linux/i2c.h>
#include <plat/tlv320adc3101_pdata.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <mach/gpio.h>
#include <linux/vgpio.h>
#include <plat/mendoza.h>
#include <plat/gpio-cfg.h>

static int tlv320adc3101_gpio_request(void)
{
	int err = 0;

	if ((err = gpio_request(TT_VGPIO_MIC_RESET, "TT_VGPIO_MIC_RESET"))) {
		printk("%s: Can't request TT_VGPIO_MIC_RESET GPIO\n", __func__);
	}

	return err;
}

static void tlv320adc3101_gpio_free(void)
{
	gpio_free(TT_VGPIO_MIC_RESET);
}

static void tlv320adc3101_config_gpio(void)
{
	s3c_gpio_setpull(vgpio_to_gpio(TT_VGPIO_MIC_RESET), S3C_GPIO_PULL_NONE);
	gpio_direction_output(TT_VGPIO_MIC_RESET, 0);
}

static void mic_reset(int activate)
{
	gpio_set_value(TT_VGPIO_MIC_RESET, activate);
}

static void tlv320adc3101_suspend (void)
{
	gpio_direction_output(TT_VGPIO_MIC_RESET, 1);
}

static void tlv320adc3101_resume (void)
{
	gpio_direction_output(TT_VGPIO_MIC_RESET, 0);
}

static tlv320adc3101_platform_t tlv320adc3101_pdata = {
	.mic_reset	= mic_reset,

	.suspend		= tlv320adc3101_suspend,
	.resume			= tlv320adc3101_resume,

	.config_gpio	= tlv320adc3101_config_gpio,
	.request_gpio	= tlv320adc3101_gpio_request,
	.free_gpio		= tlv320adc3101_gpio_free,
};

static struct i2c_board_info	tlv320adc3101_i2c_info = {
	/* ADDR: 00110 A1 A0 --> 0x18, 0x19, 0x1A, 0x1B */
	I2C_BOARD_INFO(TLV320ADC3101_DEVNAME, 0x18),
	.platform_data = &tlv320adc3101_pdata,
};

static int __init tlv320adc3101_register(void)
{
	int err;
	struct i2c_adapter * adapter;
	struct i2c_client * client;

	adapter = i2c_get_adapter(1);
	if (adapter) {
		client = i2c_new_device(adapter, &tlv320adc3101_i2c_info);
		if (!client) {
			printk( "Can't register tlv320adc3101 I2C Board\n" );
			tlv320adc3101_gpio_free();
			return -ENODEV;
		}
	} else {
		if((err = i2c_register_board_info(1, &tlv320adc3101_i2c_info, 1)) < 0) {
			printk( "Can't register tlv320adc3101 I2C Board\n" );
			tlv320adc3101_gpio_free();
			return err;
		}
	}

	return 0;
}
arch_initcall(tlv320adc3101_register);

