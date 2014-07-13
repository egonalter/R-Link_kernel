/* include/asm-arm/plat-tomtom/tt_idpinmon.h

Copyright (C) 2008 TomTom BV <http://www.tomtom.com/>
Author: Andrzej Zukowski <andrzej.zukowski@tomtom.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.
*/

#ifndef __DRIVERS_TOMTOM_IDPINMON_H
#define __DRIVERS_TOMTOM_IDPINMON_H

#define IDPINMON_DEVICE		0
#define IDPINMON_HOST		1

struct idpinmon_adc {
	struct s3c_adc_client	*client;
	struct mutex		mutex;
	struct completion	complete;

	unsigned int		channel;
	unsigned int		samples;
	unsigned int		otgmode;
};

struct idpinmon_pdata {
	int  idpinmon_state;
	int  (*request_gpio)(void);
	void (*free_gpio)   (void);
	void (*otg_host)    (void);
	void (*otg_device)  (void);
	unsigned int (*otg_getmode) (void);

	unsigned long delay;
};

#endif
