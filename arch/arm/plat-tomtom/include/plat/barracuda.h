/* include/asm-arm/plat-tomtom/barracuda.h
 *
 * barracuda driver 
 *
 * Copyright (C) 2008 TomTom BV <http://www.tomtom.com/>
 * Author: Benoit Leffray <benoit.leffray@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __BARRACUDA_H
#define __BARRACUDA_H __FILE__

#include <plat/gps.h>

struct barracuda_platform_data
{
	struct device *dev; /* Barracuda device */
	gps_status_t barracuda_status;
	void (* barracuda_power_en_activate)(int activation);
	void (* barracuda_rst_activate)(int activation);
};

#define barracuda_set_status(pdata, status) pdata->barracuda_status = status
#define barracuda_get_status(pdata) pdata->barracuda_status

#define BARRACUDA_DEVNAME			"tomtom-barracuda"

#endif /* __BARRACUDA_H */
