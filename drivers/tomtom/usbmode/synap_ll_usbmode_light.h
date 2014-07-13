/* drivers/tomtom/usbmode/synap_ll_usbmode_light.h

Copyright (C) 2008 TomTom BV <http://www.tomtom.com/>
Shamelessly Ripped from: drivers/tomtom/usbmode/s3c64xx_usbmode.h
Authors: Rogier Stam <rogier.stam@tomtom.com>
    Andrzej.Zukowski <andrzej.zukowski@tomtom.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

*/

#ifndef __DRIVERS_TOMTOM_USBMODE_SYNAP_LL_USBMODE_LIGHT_H__
#define __DRIVERS_TOMTOM_USBMODE_SYNAP_LL_USBMODE_LIGHT_H__
#define DRIVER_DESC_LONG "TomTom Synaptics Low-Level USB mode driver, (C) 2009 TomTom BV "

struct synap_ll_usbmode_priv
{
	struct timer_list		timer;
	struct impl_data_usbmode	impl_data;
	struct notifier_block		vbus_change_nb;
};

#endif /* __DRIVERS_TOMTOM_USBMODE_SYNAP_LL_USBMODE_LIGHT_H__ */
