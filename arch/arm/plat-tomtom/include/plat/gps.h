/* include/asm-arm/plat-tomtom/gps.h
 *
 * GPS driver 
 *
 * Copyright (C) 2008 TomTom BV <http://www.tomtom.com/>
 * Author: Benoit Leffray <benoit.leffray@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __GPS_H__
#define __GPS_H__

#include <linux/device.h>
#include <linux/mutex.h>
#include <linux/notifier.h>
#include <linux/time.h>

typedef enum {
	GPS_OFF,
	GPS_ON
} gps_power_t;

struct gps_device;

struct gps_ops {
	void (*get_timestamp)(struct gps_device *, struct timeval *);
};

/* This structure defines all the properties of a GPS */
struct gps_properties {
	gps_power_t		power;
	struct timeval	tv;
};

struct gps_device {
	/* GPS properties */
	struct gps_properties props;

	/* Serialise access to the driver methods */
	struct mutex update_sem;

	/* This protects the 'ops' field. If 'ops' is NULL, the driver that
	 * registered this device has been unloaded
	 */
	struct mutex ops_sem;
	struct gps_ops *ops;

	rwlock_t tv_lock;

	struct device dev;
};

extern void gps_get_timestamp(struct gps_device *gpsd, struct timeval *tv);

extern struct gps_device *gps_device_register(const char *name,
										struct device *dev, 
										void *devdata, 
										struct gps_ops *ops);

extern void gps_device_unregister(struct gps_device *gpsd);

#define to_gps_device(obj) container_of(obj, struct gps_device, dev)

static inline void * gps_get_data(struct gps_device *gps_dev)
{
	return dev_get_drvdata(&gps_dev->dev);
}

struct generic_gps_info {
	const char *name;
	void (*gps_set_power)(int power);
};
#endif /* __GPS_H__ */
