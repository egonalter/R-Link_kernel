/* include/asm-arm/plat-tomtom/tt_vbusmon.h

Copyright (C) 2008 TomTom BV <http://www.tomtom.com/>
Author: Pepijn de Langen <pepijn.delangen@tomtom.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

*/

#ifndef __DRIVERS_TOMTOM_VBUS_H
#define __DRIVERS_TOMTOM_VBUS_H
#include <linux/notifier.h>
#include <linux/device.h>

#undef VBUSMON_DEBUG

#ifdef VBUSMON_DEBUG
#define VBUSMON_DBGPRINT(args...) printk(KERN_DEBUG PFX args)
#define VBUSMON_TRACE() printk(KERN_DEBUG PFX "%s (%s:%d)\n", __FUNCTION__, __FILE__, __LINE__)
#else
#define VBUSMON_DBGPRINT(args...)
#define VBUSMON_TRACE()
#endif


struct vbus_pdata {
  const char *name;
	int (*config_vbusdetect) (int (*)(struct device *, int));
	void (*cleanup_vbusdetect) (void);
	int (*poll_vbusdetect) (void);
	void (*do_suspend) (void);
	void (*do_resume) (void);
};

extern int vbusmon_register_notifier(struct notifier_block*);
extern int vbusmon_unregister_notifier(struct notifier_block*);
extern int vbusmon_poll(void);
extern void vbusmon_set_device(struct device *dev);

#endif
