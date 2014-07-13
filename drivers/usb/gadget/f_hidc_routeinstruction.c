/* b/drivers/usb/gadget/f_hidc_routeinstruction.c
 *
 * Copyright (c) 2009 TomTom BV <http://www.tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 */
#include "f_hidc.h"

#if defined(CONFIG_USB_AVH_ROUTEINFORMATION) || defined(CONFIG_USB_ANDROID_ROUTEINFORMATION)
#include <linux/usb/composite.h>

#undef INFO
#undef DEBUG
#define ERROR

#define PFX "usb-fhidc-routeinformation: "

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

#define FHIDC_ROUTEINFORMATION_REPORT_SIZE	336
#define FHIDC_ROUTEINFORMATION_FIFO_SIZE		32

static struct fhidc_ri {
	char *fifo[FHIDC_ROUTEINFORMATION_FIFO_SIZE];
	int read_index;
	int write_index;
} *fhidc_ri;

/* need to acquire device structure lock and disable interrupts */
static int fhidc_routeinformation_write(struct file *file,
				       const char __user * buffer,
				       size_t count, loff_t * offset)
{
	int room = 0;
	char *in_buf;
	char *storage_buf;

	if (fhidc_ri->write_index < fhidc_ri->read_index) {
		room = fhidc_ri->read_index - fhidc_ri->write_index;
		info("%s: fifo has %d room\n", __FUNCTION__, room);
	} else if (fhidc_ri->write_index > fhidc_ri->read_index) {
		room = FHIDC_ROUTEINFORMATION_FIFO_SIZE -
		    fhidc_ri->write_index + fhidc_ri->read_index;
		info("%s: fifo has %d room\n", __FUNCTION__, room);
	} else
		room = FHIDC_ROUTEINFORMATION_FIFO_SIZE;

	if (room <= 1) {
		info("%s - buffer is full, dropping data\n", __FUNCTION__);
		return -1;
	}

	in_buf = (char *)buffer;
	storage_buf = fhidc_ri->fifo[fhidc_ri->write_index];

	storage_buf[0] = fhidc_routeinformation_report.id;
	storage_buf[1] = 1;	//new_data
	memcpy(storage_buf + 2, in_buf, FHIDC_ROUTEINFORMATION_REPORT_SIZE - 2);
	if (++fhidc_ri->write_index >= FHIDC_ROUTEINFORMATION_FIFO_SIZE)
		fhidc_ri->write_index = 0;

	return count;
}

static int fhidc_routeinformation_get_report(unsigned char *buf, int len)
{
	int value = -EOPNOTSUPP;
	int data;

	if (!buf)
		return -EINVAL;

	if (fhidc_ri->read_index == fhidc_ri->write_index) {
		info("fifo is empty\n");
		data = 0;
	} else if (fhidc_ri->read_index < fhidc_ri->write_index) {
		data = fhidc_ri->write_index - fhidc_ri->read_index;
		info("fifo has %d elements\n", data);
	} else {
		data =
		    FHIDC_ROUTEINFORMATION_FIFO_SIZE - fhidc_ri->read_index +
		    fhidc_ri->write_index;
		info("fifo has %d elements\n", data);
	}

	if (data == 0) {
		*buf++ = fhidc_routeinformation_report.id;
		*buf++ = 0;	/* no new data */
		/* TODO: check with jwvb why this is 4 */
		value = 4;
	} else {
		if (len > FHIDC_ROUTEINFORMATION_REPORT_SIZE) {
			len = FHIDC_ROUTEINFORMATION_REPORT_SIZE;
		}

		memcpy(buf, fhidc_ri->fifo[fhidc_ri->read_index], len);
		value = len;

		if (++fhidc_ri->read_index >= FHIDC_ROUTEINFORMATION_FIFO_SIZE)
			fhidc_ri->read_index = 0;
	}
	return value;
}

static unsigned char fhidc_routeinformation_desc[] = {
	/* ROUTE_INFORMATION */
	0x06, 0x00, 0xff,	// USAGE_PAGE (Vendor defined)
	0x09, 0x07,		// USAGE (Vendor Usage - remote information)
	0xa1, 0x01,		// COLLECTION (Application)
	0x85, 0x00,		//   REPORT_ID (?) remote information
	0x75, 0x08,		//   REPORT_SIZE (8) 8 bits
	0x95, 0x01,		//   REPORT_COUNT (1) new_data
	0x15, 0x00,		//   LOGICAL_MINIMUM (0)
	0x25, 0x01,		//   LOGICAL_MAXIMUM (1)
	0x09, 0x00,		//   USAGE (Vendor Usage - new_data)
	0x81, 0x02,		//   INPUT (Data,Var,Abs)
	0x75, 0x20,		//   REPORT_SIZE (32) 32 bits
	0x95, 0x01,		//   REPORT_COUNT (1) offset
	0x15, 0x00,		//   LOGICAL_MINIMUM (0)
	0x25, 0xff,		//   LOGICAL_MAXIMUM (2^32-1)
	0x09, 0x01,		//   USAGE (Vendor Usage - offset)
	0x81, 0x02,		//   INPUT (Data,Var,Abs)
	0x75, 0x20,		//   REPORT_SIZE (32) 32 bits
	0x95, 0x01,		//   REPORT_COUNT (1) remaining distance
	0x15, 0x00,		//   LOGICAL_MINIMUM (0)
	0x25, 0xff,		//   LOGICAL_MAXIMUM (2^32-1)
	0x09, 0x02,		//   USAGE (Vendor Usage - distance)
	0x81, 0x02,		//   INPUT (Data,Var,Abs)
	0x75, 0x08,		//   REPORT_SIZE (8) 8 bits
	0x95, 0x01,		//   REPORT_COUNT (1) side
	0x15, 0x00,		//   LOGICAL_MINIMUM (0)
	0x25, 0x01,		//   LOGICAL_MAXIMUM (1)
	0x09, 0x03,		//   USAGE (Vendor Usage - side)
	0x81, 0x02,		//   INPUT (Data,Var,Abs)
	0x75, 0x08,		//   REPORT_SIZE (8) 8 bits
	0x95, 0x01,		//   REPORT_COUNT (1) size
	0x15, 0x00,		//   LOGICAL_MINIMUM (0)
	0x25, 0xff,		//   LOGICAL_MAXIMUM (1)
	0x09, 0x04,		//   USAGE (Vendor Usage - size)
	0x81, 0x02,		//   INPUT (Data,Var,Abs)
	0x75, 0x08,		//   REPORT_SIZE (8) 8 bits
	0x95, 0x14,		//   REPORT_COUNT (20) descriptions
	0x15, 0x00,		//   LOGICAL_MINIMUM (0)
	0x25, 0xff,		//   LOGICAL_MAXIMUM (1)
	0x09, 0x05,		//   USAGE (Vendor Usage - descriptions)
	0x81, 0x02,		//   INPUT (Data,Var,Abs)
	0x75, 0x08,		//   REPORT_SIZE (8) 8 bits
	0x95, 0x64,		//   REPORT_COUNT (100) streetnumber
	0x15, 0x00,		//   LOGICAL_MINIMUM (0)
	0x25, 0xff,		//   LOGICAL_MAXIMUM (1)
	0x09, 0x06,		//   USAGE (Vendor Usage - streetnumber)
	0x81, 0x02,		//   INPUT (Data,Var,Abs)
	0x75, 0x08,		//   REPORT_SIZE (8) 8 bits
	0x95, 0x64,		//   REPORT_COUNT (100) streetname
	0x15, 0x00,		//   LOGICAL_MINIMUM (0)
	0x25, 0xff,		//   LOGICAL_MAXIMUM (1)
	0x09, 0x07,		//   USAGE (Vendor Usage - streetname)
	0x81, 0x02,		//   INPUT (Data,Var,Abs)
	0x75, 0x08,		//   REPORT_SIZE (8) 8 bits
	0x95, 0x64,		//   REPORT_COUNT (100) posttext
	0x15, 0x00,		//   LOGICAL_MINIMUM (0)
	0x25, 0xff,		//   LOGICAL_MAXIMUM (1)
	0x09, 0x08,		//   USAGE (Vendor Usage - posttext)
	0x81, 0x02,		//   INPUT (Data,Var,Abs)

	/* PROGRESS_ON_ROUTE */
	0x75, 0x20,		//   REPORT_SIZE (32) 32 bits
	0x95, 0x01,		//   REPORT_COUNT (100) current offset
	0x15, 0x00,		//   LOGICAL_MINIMUM (0)
	0x25, 0xff,		//   LOGICAL_MAXIMUM (2^32-1)
	0x09, 0x09,		//   USAGE (Vendor Usage - current offset)
	0x81, 0x02,		//   INPUT (Data,Var,Abs)
	0xc0,			// END_COLLECTION
};

#define FHIDC_ROUTEINFORMATION_DESC_ID_OFFSET	8

static void fhidc_routeinformation_set_report_id(int id)
{
	fhidc_routeinformation_desc[FHIDC_ROUTEINFORMATION_DESC_ID_OFFSET] = id;
}

static int fhidc_routeinformation_init(void)
{
	int i;
	if (!(fhidc_ri = kzalloc(sizeof *fhidc_ri, GFP_KERNEL)))
		return -ENOMEM;

	fhidc_ri->read_index = 0;
	fhidc_ri->write_index = 0;
	for (i = 0; i < FHIDC_ROUTEINFORMATION_FIFO_SIZE; i++) {
		fhidc_ri->fifo[i] = kmalloc(336, GFP_KERNEL);
	}
	fhidc_routeinformation_set_report_id(fhidc_routeinformation_report.id);
	return 0;
}

static int fhidc_routeinformation_exit(void)
{
	int i;

	fhidc_routeinformation_set_report_id(0);

	for (i = 0; i < FHIDC_ROUTEINFORMATION_FIFO_SIZE; i++) {
		if (fhidc_ri->fifo[i]) {
			kfree(fhidc_ri->fifo[i]);
			fhidc_ri->fifo[i] = 0;
		}
	}

	kfree(fhidc_ri);

	return 0;
}

/* ------------------------------------------------------------------------- */
/*                         external interface                                */
/* ------------------------------------------------------------------------- */

struct fhidc_report fhidc_routeinformation_report = {
	.size = FHIDC_ROUTEINFORMATION_REPORT_SIZE,
	.init = fhidc_routeinformation_init,
	.exit = fhidc_routeinformation_exit,
	.get_report = fhidc_routeinformation_get_report,
	.desc = fhidc_routeinformation_desc,
	.write = fhidc_routeinformation_write,
	.desc_size = sizeof(fhidc_routeinformation_desc),
};
#else
struct fhidc_report fhidc_routeinformation_report = { 0 };
#endif
