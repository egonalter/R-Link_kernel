/*
 * Copyright (C) 2010 TomTom BV <http://www.tomtom.com/>
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
#include <linux/io.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <linux/vgpio.h>
#include <plat/mendoza.h>
#include <plat/csr_bc_mode.h>

static int bluetooth_gpio_request(void)
{
	unsigned int val;

	if (machine_is_taranto()) {
		s3c_gpio_cfgpin(S5P6450_GPP(6), S3C_GPIO_SFN(1));
		s3c_gpio_cfgpin(S5P6450_GPD(4), S3C_GPIO_SFN(2));
		s3c_gpio_cfgpin(S5P6450_GPD(5), S3C_GPIO_SFN(2));

		val = readl(S5P_OTHERS)
		val |= (1 << 21);
		writel(val, S5P_OTHERS);
	}

	return gpio_request(TT_VGPIO_BT_RST, "BT RST") ||
	       gpio_direction_output(TT_VGPIO_BT_RST, 1);
}

static void bluetooth_gpio_free(void)
{
}

static void bluetooth_config_gpio(void)
{
	gpio_set_value(TT_VGPIO_BT_RST, 0);
}

static void bluetooth_reset(int activate)
{
	gpio_set_value(TT_VGPIO_BT_RST, !!activate);
}

/* 
 * The pio lines can be connected to the SoCs GPIO or hardwired to GND or VCC
 * In our case all pins are hardwired to GND
 *
 * PIO [0][1][4]
 * 0 0 0 UART,BCSP
 * 0 0 1 BCSP(2 stop bits,no parity)
 * 0 1 0 USB,16MHz
 * 0 1 1 USB,26MHz
 * 1 0 0 Three-wire UART
 * 1 0 1 H4DS
 * 1 1 0 UART(H4)
 * 1 1 1 Undefine internal weak pull-down
 */
static int bluetooth_pio0(void) {
	return 0;
}

static int bluetooth_pio1(void) {
	return 0;
}

static int bluetooth_pio4(void) {
	return 0;
}

static void bluetooth_suspend (void)
{
	gpio_set_value(TT_VGPIO_BT_RST, 0);
}

static void bluetooth_resume (void)
{
	gpio_set_value(TT_VGPIO_BT_RST, 0);
}

static struct csr_bc_info bluetooth_pdata = {
	.reset			= bluetooth_reset,
	.set_pio0		= NULL,
	.set_pio1		= NULL,
	.set_pio4		= NULL,

	.get_pio0		= bluetooth_pio0,
	.get_pio1		= bluetooth_pio1,
	.get_pio4		= bluetooth_pio4,

	.suspend		= bluetooth_suspend,
	.resume			= bluetooth_resume,

	.config_gpio		= bluetooth_config_gpio,
	.request_gpio		= bluetooth_gpio_request,
	.free_gpio		= bluetooth_gpio_free,

	.do_reset		= NULL,
};

static struct platform_device bt_pdev = {
	.name = "csr-bc",
	.id = -1,
	.dev = {
		.platform_data = ((void *) &bluetooth_pdata),
	},
};

static int __init bluetooth_init( void )
{
	if (platform_device_register(&bt_pdev))
		printk(KERN_ERR "Could not register csr-bc platform device\n");

	return 0;
}
arch_initcall( bluetooth_init );

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Mendoza bluetooth module");
