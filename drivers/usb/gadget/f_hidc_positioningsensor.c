/* b/drivers/usb/gadget/f_hidc_positioningsensor.c
 *
 * Copyright (c) 2009 TomTom BV <http://www.tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 */
#include "f_hidc.h"

#if defined(CONFIG_USB_AVH_POSITIONINGSENSOR) || defined(CONFIG_USB_ANDROID_POSITIONINGSENSOR)
#include <linux/usb/composite.h>

#undef INFO
#undef DEBUG
#define ERROR

#define PFX "usb-fhidc-positioningsensor: "

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

#define FHIDC_POSITIONSENSOR_REPORT_SIZE  0
#define FHIDC_POSITIONSENSOR_FIFO_SIZE    32

struct positionsensor_data {
	char config;		/* true if config, false if data */
	short type;
	short id;
	short period_milliseconds;
	unsigned long long timestamp;
	char size;
	signed short values[4];
};

static struct fhidc_positionsensor {
	struct positionsensor_data *fifo[FHIDC_POSITIONSENSOR_FIFO_SIZE];
	int read_index;
	int write_index;
	wait_queue_head_t inq;
} *fhidc_ps;

static void fhidc_ps_fifo_write(char config, short type, short id,
			       short period_milliseconds,
			       unsigned long long timestamp, char size,
			       signed short *values)
{
	struct positionsensor_data *data_ptr = NULL;
	int i;

	data_ptr = kmalloc(sizeof(struct positionsensor_data), GFP_ATOMIC);
	if (data_ptr != NULL) {
		data_ptr->config = config;
		// config
		data_ptr->type = type;
		data_ptr->id = id;
		data_ptr->period_milliseconds = period_milliseconds;
		// data
		data_ptr->timestamp = timestamp;
		data_ptr->size = size;
		for (i = 0; i < size; i++)
			data_ptr->values[i] = values[i];
		fhidc_ps->fifo[fhidc_ps->write_index] = data_ptr;
		if (++fhidc_ps->write_index >= FHIDC_POSITIONSENSOR_FIFO_SIZE)
			fhidc_ps->write_index = 0;
		if (fhidc_ps->write_index == fhidc_ps->read_index)
			if (++fhidc_ps->read_index >=
			    FHIDC_POSITIONSENSOR_FIFO_SIZE)
				fhidc_ps->read_index = 0;
	} else {
		//fail
		err("unable to kmalloc memory for positionsensor_config, dropping data\n");
	}
	wake_up_interruptible(&fhidc_ps->inq);
}

static ssize_t fhidc_positionsensor_read(struct file *file, char __user * buffer,
					size_t count, loff_t * offset)
{
	int status;
	int result = 0;
	int buffer_offset = 0;
	unsigned int i = 0;

	if (fhidc_ps->read_index == fhidc_ps->write_index)
		return -EAGAIN;

	/* copy config byte to user space */
	status = __copy_to_user(buffer,
				&fhidc_ps->fifo[fhidc_ps->read_index]->config, 1);
	buffer_offset += 1;
	/* copy type to user space */
	status = __copy_to_user(buffer + buffer_offset,
				&(fhidc_ps->fifo[fhidc_ps->read_index]->type), 2);
	buffer_offset += 2;
	/* copy id to user space */
	status = __copy_to_user(buffer + buffer_offset,
				&(fhidc_ps->fifo[fhidc_ps->read_index]->id), 2);
	buffer_offset += 2;
	/* copy period_milliseconds to user space */
	status = __copy_to_user(buffer + buffer_offset,
				&(fhidc_ps->fifo[fhidc_ps->read_index]->
				  period_milliseconds), 2);
	buffer_offset += 2;

	/* copy timestamp to user space */
	status = __copy_to_user(buffer + buffer_offset,
				&(fhidc_ps->fifo[fhidc_ps->read_index]->
				  timestamp), 8);
	buffer_offset += 8;
	/* copy size to user space */
	status = __copy_to_user(buffer + buffer_offset,
				&(fhidc_ps->fifo[fhidc_ps->read_index]->size), 1);
	buffer_offset += 1;
	/* copy values to user space */
	for (i = 0; i < (fhidc_ps->fifo[fhidc_ps->read_index]->size); i++) {
		status = __copy_to_user(buffer + buffer_offset,
					&(fhidc_ps->fifo[fhidc_ps->read_index]->
					  values[i]), 2);
		buffer_offset += 2;
	}
	kfree(fhidc_ps->fifo[fhidc_ps->read_index]);
	result = buffer_offset;

	if (++fhidc_ps->read_index >= FHIDC_POSITIONSENSOR_FIFO_SIZE)
		fhidc_ps->read_index = 0;
	return result;
}

static unsigned int fhidc_positionsensor_poll(struct file *filp,
					     poll_table * wait)
{
	unsigned int mask = 0;

	poll_wait(filp, &fhidc_ps->inq, wait);
	if (fhidc_ps->read_index != fhidc_ps->write_index)
		mask |= POLLIN | POLLRDNORM;
	return mask;
}

static int fhidc_positionsensor_set_report(const unsigned char *buf, int len)
{
	char config;
	short id = 0;
	short type = 0;
	short period_milliseconds = 0;
	unsigned long long timestamp = 0;
	char *timestamp_byte = (char *)&timestamp;
	signed short data[4];
	signed short temp_data;
	char data_size = 0;
	int i;

	if ((len < FHIDC_POSITIONSENSOR_REPORT_SIZE) ||
	    (*buf++ != fhidc_positionsensor_report.id)) {
		err("huh? malformed positionsensor report\n");
		return 0;
	}

	config = *buf++;

	type = *buf++;
	type |= ((*buf++) << 8);

	id = *buf++;
	id |= ((*buf++) << 8);

	period_milliseconds = *buf++;
	period_milliseconds |= ((*buf++) << 8);

	info("set-report: {id=%d, type=%d, period=%d}\n", id, type,
	     period_milliseconds);

	for (i = 0; i < sizeof(timestamp); i++)
		timestamp_byte[i] = *buf++;

	data_size = *buf++;

	for (i = 0; i < data_size; i++) {
		temp_data = *buf++;
		temp_data |= (*buf++) << 8;
		data[i] = temp_data;
		info("set-report: {time=%llu, size=%d, data[%d]=%d}\n",
		     timestamp, data_size, i, data[i]);
	}

	fhidc_ps_fifo_write(config, type, id, period_milliseconds, timestamp,
			   data_size, data);

	return 0;
}

static unsigned char fhidc_positionsensor_desc[] = {
	/* POSITIONING_SENSOR */
	0x06, 0x00, 0xff,	// USAGE_PAGE (Vendor defined)
	0x09, 0x06,		// USAGE (Vendor Usage - positioning sensor)
	0xa1, 0x01,		// COLLECTION (Application)
	0x85, 0x00,		//   REPORT_ID (?) positioningsensor data
	0x75, 0x08,		//   REPORT_SIZE (8) 8 bits
	0x95, 0x01,		//   REPORT_COUNT (1) data(0)/config(1)
	0x15, 0x00,		//   LOGICAL_MINIMUM (0)
	0x26, 0xff, 0x00,	//   LOGICAL_MAXIMUM (255)
	0x09, 0x00,		//   USAGE (Vendor Usage - config 8 bits)
	0x91, 0x02,		//   OUTPUT (Data,Var,Abs)
	0x75, 0x10,		//   REPORT_SIZE (16) 16 bits
	0x95, 0x03,		//   REPORT_COUNT (3) type, id, period
	0x15, 0x00,		//   LOGICAL_MINIMUM (0)
	0x27, 0xff, 0xff, 0x00, 0x00,	//   LOGICAL_MAXIMUM (65535)
	0x09, 0x00,		//   USAGE (Vendor Usage - type 16 bits)
	0x09, 0x01,		//   USAGE (Vendor Usage - id 16 bits)
	0x09, 0x02,		//   USAGE (Vendor Usage - period 16 bits)
	0x91, 0x02,		//   OUTPUT (Data,Var,Abs)
	0x75, 0x08,		//   REPORT_SIZE (8) 8 bits
	0x95, 0x08,		//   REPORT_COUNT (8) timestamp 8 bytes
	0x15, 0x00,		//   LOGICAL_MINIMUM (0)
	0x26, 0xff, 0x00,	//   LOGICAL_MAXIMUM (255)
	0x09, 0x00,		//   USAGE (Vendor Usage - timestamp[i] 8 bits)
	0x09, 0x01,		//   USAGE (Vendor Usage - timestamp[i] 8 bits)
	0x09, 0x02,		//   USAGE (Vendor Usage - timestamp[i] 8 bits)
	0x09, 0x03,		//   USAGE (Vendor Usage - timestamp[i] 8 bits)
	0x09, 0x04,		//   USAGE (Vendor Usage - timestamp[i] 8 bits)
	0x09, 0x05,		//   USAGE (Vendor Usage - timestamp[i] 8 bits)
	0x09, 0x06,		//   USAGE (Vendor Usage - timestamp[i] 8 bits)
	0x09, 0x07,		//   USAGE (Vendor Usage - timestamp[i] 8 bits)
	0x91, 0x02,		//   OUTPUT (Data,Var,Abs)
	0x75, 0x08,		//   REPORT_SIZE (8) 8 bits
	0x95, 0x01,		//   REPORT_COUNT (1) data_size
	0x15, 0x00,		//   LOGICAL_MINIMUM (0)
	0x26, 0xff, 0x00,	//   LOGICAL_MAXIMUM (255)
	0x09, 0x00,		//   USAGE (Vendor Usage - data_size 8 bits)
	0x91, 0x02,		//   OUTPUT (Data,Var,Abs)
	0x75, 0x10,		//   REPORT_SIZE (8) 8 bits
	0x95, 0x04,		//   REPORT_COUNT (8) values 8 bytes
	0x15, 0x00,		//   LOGICAL_MINIMUM (0)
	0x26, 0xff, 0x00,	//   LOGICAL_MAXIMUM (255)
	0x09, 0x00,		//   USAGE (Vendor Usage - data_value[0] 16 bits)
	0x09, 0x01,		//   USAGE (Vendor Usage - data_value[1] 16 bits)
	0x09, 0x02,		//   USAGE (Vendor Usage - timestamp[2] 16 bits)
	0x09, 0x03,		//   USAGE (Vendor Usage - timestamp[3] 16 bits)
	0x91, 0x02,		//   OUTPUT (Data,Var,Abs)
	0xc0,			// END_COLLECTION
};

#define FHIDC_POSITIONSENSOR_DESC_ID_OFFSET	8

static void fhidc_positionsensor_set_report_id(int id)
{
	fhidc_positionsensor_desc[FHIDC_POSITIONSENSOR_DESC_ID_OFFSET] = id;
}

static int fhidc_positionsensor_init(void)
{
	if (!(fhidc_ps = kzalloc(sizeof *fhidc_ps, GFP_KERNEL)))
		return -ENOMEM;
	fhidc_ps->read_index = 0;
	fhidc_ps->write_index = 0;
	init_waitqueue_head(&fhidc_ps->inq);

	fhidc_positionsensor_set_report_id(fhidc_positionsensor_report.id);
	return 0;
}

static int fhidc_positionsensor_exit(void)
{
	fhidc_positionsensor_set_report_id(fhidc_positionsensor_report.id);
	kfree(fhidc_ps);
	return 0;
}

/* ------------------------------------------------------------------------- */
/*                         external interface                                */
/* ------------------------------------------------------------------------- */

struct fhidc_report fhidc_positionsensor_report = {
	.size = FHIDC_POSITIONSENSOR_REPORT_SIZE,
	.init = fhidc_positionsensor_init,
	.exit = fhidc_positionsensor_exit,
	.set_report = fhidc_positionsensor_set_report,
	.read = fhidc_positionsensor_read,
	.poll = fhidc_positionsensor_poll,
	.desc = fhidc_positionsensor_desc,
	.desc_size = sizeof(fhidc_positionsensor_desc),
};
#else
struct fhidc_report fhidc_positionsensor_report = { 0 };
#endif
