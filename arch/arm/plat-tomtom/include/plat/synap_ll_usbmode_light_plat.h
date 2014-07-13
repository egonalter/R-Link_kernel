/* include/asm-arm/plat-tomtom/synap_ll_usbmode_light_plat.h

Copyright (C) 2008 TomTom BV <http://www.tomtom.com/>
Shamelessly Ripped from: drivers/tomtom/usbmode/s3c64xx_usbmode.h
Authors: Rogier Stam <rogier.stam@tomtom.com>
    Andrzej.Zukowski <andrzej.zukowski@tomtom.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

*/

#ifndef __INCLUDE_ASM_ARM_PLAT_TOMTOM_SYNAP_LL_USBMODE_LIGHT_PLAT_H__
#define __INCLUDE_ASM_ARM_PLAT_TOMTOM_SYNAP_LL_USBMODE_LIGHT_PLAT_H__

struct synap_ll_usbmode_platform_data
{
	irqreturn_t	(*irqhandler)( struct platform_device *pdev );
	int 		(*setup)( struct platform_device *pdev );
	int		(*is_usbhost)( struct platform_device *pdev );
	uint32_t	device_detect_timeout;
};

/* Time in seconds to assume there is no host present when performing
in device detect mode(Device detect == detect is we need to become device)*/
#define DEVICE_DETECT_TIMEOUT_SECONDS 3


#endif /* __INCLUDE_ASM_ARM_PLAT_TOMTOM_SYNAP_LL_USBMODE_LIGHT_PLAT_H__ */
