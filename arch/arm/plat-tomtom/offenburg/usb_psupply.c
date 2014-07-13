/*
 * arch/arm/plat-tomtom/offenburg/usb_psupply.c
 *
 * Platform part of USB power supply driver
 *
 * Copyright (C) 2010 TomTom International BV
 * Author: Matthias Kaehlcke <matthias.kaehlcke@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation version 2.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <linux/delay.h>
#include <linux/i2c/twl.h>
#include <linux/usb_psupply.h>
#include <linux/gpio.h>
#include <plat/offenburg.h>

#define OTG_CTRL_SET	0x0B
#define OTG_IDPULLUP	(1 << 0)

#define ID_STATUS	0x96
#define ID_RES_200K	(1 << 2) /* type 1 charger */

static int offenburg_is_accessory_charger_plugged(void)
{
	int rc = 0;
	u8 value;

	/* we have to enable id pin sensing everytime, as it can be
	   disabled by other drivers */
	if (twl_i2c_write_u8(TWL4030_MODULE_USB, OTG_IDPULLUP, OTG_CTRL_SET)) {
		USBPS_ERR("failed to enable ID pin sensing");
		goto err;
	}

	msleep(50);

	if (twl_i2c_read_u8(TWL4030_MODULE_USB, &value, ID_STATUS)) {
		USBPS_ERR("failed to read ID pin status");
		goto err;
	}

	USBPS_DBG("ID_STATUS: 0x%02x\n", value);

	if (value & ID_RES_200K)
		rc = 1;

err:
	return rc;
}


/******************************************************************************
 * platform device registration                                               *
 ******************************************************************************/

static struct usb_psupply_pdata usb_psupply_pdata =
{
	.gpio_fast_charge_en = TT_VGPIO_WALL_SENSE_CONNECT,
	.gpio_vbus = TT_VGPIO_USB_DETECT,
	.gpio_chrg_det = TT_VGPIO_USB_CHRG_DET,

	.is_accessory_charger_plugged = offenburg_is_accessory_charger_plugged
};

static struct platform_device usb_psupply_pdev = {
	.name = DRIVER_NAME,
	.id = -1,
	.num_resources	= 0,
	.dev = {
		.platform_data = &usb_psupply_pdata,
	},
};

static int __init offenburg_usb_psupply_init(void)
{
	return platform_device_register(&usb_psupply_pdev);
}

late_initcall(offenburg_usb_psupply_init);

