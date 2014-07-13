/*
 * TomTom GPS Lowlevel Control Abstraction
 *
 * drivers/tomtom/gps/gps.c
 *
 * TomTom GPS driver
 *
 * Copyright (C) 2009 TomTom BV <http://www.tomtom.com/>
 * Author: Benoit Leffray <benoit.leffray@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/ctype.h>
#include <linux/err.h>
#include <plat/gps.h>

#define GPS_NAME	"gps" 
#define GPS_PFX		GPS_NAME ": " 

void gps_get_timestamp(struct gps_device *gpsd, struct timeval *tv)
{
	mutex_lock(&gpsd->update_sem);
	if (gpsd->ops && gpsd->ops->get_timestamp)
		gpsd->ops->get_timestamp(gpsd, tv);
	mutex_unlock(&gpsd->update_sem);
}
EXPORT_SYMBOL(gps_get_timestamp);

static ssize_t gps_show_timestamp(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct timeval		tv;
	struct gps_device	*gpsd = to_gps_device(dev);

	mutex_lock(&gpsd->ops_sem);
	if (gpsd->ops) {
		gps_get_timestamp(gpsd, &tv);
	}
	mutex_unlock(&gpsd->ops_sem);

	return sprintf(buf, "%8ld.%06ld\n", tv.tv_sec, tv.tv_usec);
}

static ssize_t gps_show_name(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%s\n", dev_name(dev));
}

static struct class *gps_class;

static void gps_device_release(struct device *dev)
{
	struct gps_device *gpsd = to_gps_device(dev);
	kfree(gpsd);
}

static struct device_attribute gps_device_attributes[] = {
	__ATTR(name, 0444, gps_show_name, NULL),
	__ATTR(timestamp, 0444, gps_show_timestamp, NULL),
	__ATTR_NULL,
};

/**
 * gps_device_register	- create and register a new object of
 * 							gps_device class.
 * @name: 		- the name of the new object.
 * @parent:		- a pointer to the parent device
 * @devdata: 	- an optional pointer to be stored for private driver use. The
 *				  methods may retrieve it by using gps_get_data(gpsd).
 * @ops:		- the GPS operations structure.
 *
 * Creates and registers new GPS device. Returns either an
 * ERR_PTR() or a pointer to the newly allocated device.
 */
struct gps_device *gps_device_register(const char *name, struct device *parent,
										 void *devdata, struct gps_ops *ops)
{
	struct gps_device *new_gpsd;
	int rc;

	printk(KERN_INFO GPS_PFX "device registration: name=\"%s\"\n", name);

	BUG_ON(ops == NULL);

	new_gpsd = kzalloc(sizeof(struct gps_device), GFP_KERNEL);
	if (!new_gpsd)
		return ERR_PTR(-ENOMEM);

	mutex_init(&new_gpsd->update_sem);
	mutex_init(&new_gpsd->ops_sem);

	new_gpsd->tv_lock = RW_LOCK_UNLOCKED;

	new_gpsd->dev.class		= gps_class;
	new_gpsd->dev.parent	= parent;
	new_gpsd->dev.release	= gps_device_release;
	dev_set_name(&new_gpsd->dev, "%s", name);
	dev_set_drvdata(&new_gpsd->dev, devdata);

	rc = device_register(&new_gpsd->dev);
	if (rc) {
		kfree(new_gpsd);
		return ERR_PTR(rc);
	}

	new_gpsd->ops = ops;

	return new_gpsd;
}
EXPORT_SYMBOL(gps_device_register);

/**
 * gps_device_unregister	- unregisters a GPS device object.
 * @gpsd:	- the GPS device object to be unregistered and freed.
 *
 * Unregisters a previously registered via gps_device_register object.
 */
void gps_device_unregister(struct gps_device *gpsd)
{
	if (!gpsd)
		return;

	mutex_lock(&gpsd->ops_sem);
	gpsd->ops = NULL;
	mutex_unlock(&gpsd->ops_sem);

	device_unregister(&gpsd->dev);
}
EXPORT_SYMBOL(gps_device_unregister);

static void __exit gps_class_exit(void)
{
	class_destroy(gps_class);
}

static int __init gps_class_init(void)
{
	gps_class = class_create(THIS_MODULE, GPS_NAME);
	if (IS_ERR(gps_class)) {
		printk(KERN_ERR GPS_PFX "Unable to create GPS class; errno = %ld\n",
				PTR_ERR(gps_class));
		return PTR_ERR(gps_class);
	}

	gps_class->dev_attrs = gps_device_attributes;
	return 0;
}

/*
 * this is statically linked in the kernel and we need to ensure that the
 * class is registered before users of the class try to register GPS
 * devices.
 */
postcore_initcall(gps_class_init);
module_exit(gps_class_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Benoit Leffray <benoit.leffray@tomtom.com>");
MODULE_DESCRIPTION("GPS Lowlevel Control Abstraction");
