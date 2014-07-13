/* b/drivers/usb/gadget/f_hidc_lightsensor.c
 *
 * Copyright (c) 2009 TomTom BV <http://www.tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 */
#include "f_hidc.h"
#include <linux/hwmon.h>
#include <linux/cdev.h>
#include <linux/device.h>

#if defined(CONFIG_USB_AVH_LIGHTSENSOR) || defined(CONFIG_USB_ANDROID_LIGHTSENSOR)

#include <linux/usb/composite.h>

#undef INFO
#undef DEBUG
#define ERROR

#define PFX "usb-fhidc-lightsensor: "

#ifdef INFO
#define info(fmt, args...) printk(KERN_INFO PFX fmt, ## args)
#else
#define info(fmt, args...) do {} while (0)
#endif

#ifdef DEBUG
#define debug(fmt, args...) printk(KERN_INFO PFX fmt, ## args)
#else
#define debug(fmt, args...) do {} while (0)
#endif

#ifdef ERROR
#define err(fmt, args...) printk(KERN_INFO PFX fmt, ## args)
#else
#define err(fmt, args...) do {} while (0)
#endif

#ifdef BUGME
#define BUGME_ON(cond) do { if (cond) BUG();} while (0)
#else
#define BUGME_ON(cond) do { } while (0)
#endif

#define FHIDC_LIGHTSENSOR_REPORT_SIZE	2
#define FHIDC_LIGHTSENSOR_FIFO_SIZE	32

static struct fhidc_lightsensor {
	int fifo[FHIDC_LIGHTSENSOR_FIFO_SIZE];
	int read_index;
	int write_index;
} *fhidc_lightsensor;

static void fhidc_lightsensor_fifo_write(int value)
{
	fhidc_lightsensor->fifo[fhidc_lightsensor->write_index] = value;

	if (++fhidc_lightsensor->write_index >= FHIDC_LIGHTSENSOR_FIFO_SIZE)
		fhidc_lightsensor->write_index = 0;

	if (fhidc_lightsensor->write_index == fhidc_lightsensor->read_index)
		if (++fhidc_lightsensor->read_index >=
		    FHIDC_LIGHTSENSOR_FIFO_SIZE)
			fhidc_lightsensor->read_index = 0;
}

static ssize_t fhidc_lightsensor_read(struct file *file, char __user * buffer,
				     size_t count, loff_t * offset)
{
	int status;

	if (fhidc_lightsensor->read_index == fhidc_lightsensor->write_index)
		return -EAGAIN;

	status = __copy_to_user(buffer,
				&fhidc_lightsensor->fifo[fhidc_lightsensor->
							read_index],
				sizeof(*fhidc_lightsensor->fifo));

	if (++fhidc_lightsensor->read_index >= FHIDC_LIGHTSENSOR_FIFO_SIZE)
		fhidc_lightsensor->read_index = 0;

	return sizeof(*fhidc_lightsensor->fifo);
}

static ssize_t fhidc_lightsensor_read_lux(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	long unsigned int value = 0;

	if (fhidc_lightsensor->read_index == fhidc_lightsensor->write_index)
		return -EBUSY;

	value = fhidc_lightsensor->fifo[fhidc_lightsensor->read_index];

	if (++fhidc_lightsensor->read_index >= FHIDC_LIGHTSENSOR_FIFO_SIZE)
		fhidc_lightsensor->read_index = 0;

	return sprintf(buf, "%lu\n", value);
}

static DEVICE_ATTR(lux, S_IRUGO, fhidc_lightsensor_read_lux, NULL);

static int fhidc_lightsensor_set_report(const unsigned char *buf, int len)
{
	int value = 0;

	if ((len < FHIDC_LIGHTSENSOR_REPORT_SIZE) ||
	    (*buf++ != fhidc_lightsensor_report.id)) {
		err("huh? malformed lightsensor report\n");
		return 0;
	}

	value = *buf++;
	info("set-report: {%d}\n", value);
	fhidc_lightsensor_fifo_write(value);
	return 0;
}

static unsigned char fhidc_lightsensor_desc[] = {
	/* LIGHT_SENSOR */
	0x06, 0x00, 0xff,	// USAGE_PAGE (Vendor defined)
	0x09, 0x05,		// USAGE (Vendor Usage - light sensor)
	0xa1, 0x01,		// COLLECTION (Application) light sensor
	0x85, 0x81,		//   REPORT_ID (129)
	0x75, 0x08,		//   REPORT_SIZE (8) size in bits
	0x95, 0x01,		//   REPORT_COUNT (1)
	0x15, 0x00,		//   LOGICAL_MINIMUM (0)
	0x26, 0xff, 0x00,	//   LOGICAL_MAXIMUM (255)
	0x09, 0x01,		//   USAGE (Vendor Usage - light sensor value)
	0x91, 0x02,		//   OUTPUT (Data,Var,Abs)
	0xc0,			// END_COLLECTION
};

#define FHIDC_LIGHTSENSOR_DESC_ID_OFFSET	8

static void fhidc_lightsensor_release(struct device *dev)
{
	info("%s - not implemented", __FUNCTION__);
}

static void fhidc_lightsensor_set_report_id(int id)
{
	fhidc_lightsensor_desc[FHIDC_LIGHTSENSOR_DESC_ID_OFFSET] = id;
}

struct device fhidc_lightsensor_dev = {
	.bus_id = "fhidc_lightsensor",
	.release = fhidc_lightsensor_release
};

static int fhidc_lightsensor_init(void)
{
	struct device *dev;

	if (!(fhidc_lightsensor = kzalloc(sizeof *fhidc_lightsensor, GFP_KERNEL)))
		return -ENOMEM;

	fhidc_lightsensor->read_index = 0;
	fhidc_lightsensor->write_index = 0;

	if (device_register(&fhidc_lightsensor_dev) < 0)
		return -ENODEV;

	/* Under the hwmon class. */
	dev = hwmon_device_register(&fhidc_lightsensor_dev);
	if (dev == NULL)
		return -ENODEV;

	dev_set_drvdata(&fhidc_lightsensor_dev, dev);

	/* Sysfs attributes. */
	if (device_create_file(&fhidc_lightsensor_dev, &dev_attr_lux)) {
		hwmon_device_unregister(&fhidc_lightsensor_dev);
		return -ENODEV;
	}

	fhidc_lightsensor_set_report_id(fhidc_lightsensor_report.id);
	return 0;
}

static int fhidc_lightsensor_exit(void)
{
	fhidc_lightsensor_set_report_id(0);
	kfree(fhidc_lightsensor);
	device_remove_file(&fhidc_lightsensor_dev, &dev_attr_lux);
	hwmon_device_unregister(&fhidc_lightsensor_dev);
	device_unregister(&fhidc_lightsensor_dev);
	return 0;
}

/* ------------------------------------------------------------------------- */
/*                         external interface                                */
/* ------------------------------------------------------------------------- */

struct fhidc_report fhidc_lightsensor_report = {
	.size = FHIDC_LIGHTSENSOR_REPORT_SIZE,
	.init = fhidc_lightsensor_init,
	.exit = fhidc_lightsensor_exit,
	.set_report = fhidc_lightsensor_set_report,
	.read = fhidc_lightsensor_read,
	.desc = fhidc_lightsensor_desc,
	.desc_size = sizeof(fhidc_lightsensor_desc),
};
#else
struct fhidc_report fhidc_lightsensor_report = { 0 };
#endif
