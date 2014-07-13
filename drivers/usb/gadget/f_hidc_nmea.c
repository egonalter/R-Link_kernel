/* b/drivers/usb/gadget/f_hidc_nmea.c
 *
 * Copyright (c) 2009 TomTom BV <http://www.tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 */
#include "f_hidc.h"

#if defined(CONFIG_USB_AVH_NMEA) || defined(CONFIG_USB_ANDROID_NMEA)

#include <linux/usb/composite.h>

#undef INFO
#undef DEBUG
#define ERROR

#define PFX "usb-fhid-nmea: "

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

#define __atomic

#define FHIDC_NMEA_FIFO_SIZE	32
#define FHIDC_NMEA_STR_LEN   	82
#define FHIDC_NMEA_REPORT_SIZE ( FHIDC_NMEA_STR_LEN + 1)

static struct fhidc_nmea {
	char *nmea_fifo[FHIDC_NMEA_FIFO_SIZE];
	int nmea_read_index;
	int nmea_write_index;
} *fhidc_nmea;

static __atomic void fhidc_nmea_fifo_write(char nmeastr[])
{
	memcpy(fhidc_nmea->nmea_fifo[fhidc_nmea->nmea_write_index], nmeastr,
	       FHIDC_NMEA_STR_LEN * sizeof(char));

	if (++fhidc_nmea->nmea_write_index >= FHIDC_NMEA_FIFO_SIZE)
		fhidc_nmea->nmea_write_index = 0;

	if (fhidc_nmea->nmea_write_index == fhidc_nmea->nmea_read_index)
		if (++fhidc_nmea->nmea_read_index >= FHIDC_NMEA_FIFO_SIZE)
			fhidc_nmea->nmea_read_index = 0;
}

/* assumes device lock is being held */
static ssize_t fhidc_nmea_read(struct file *file, char __user * buffer,
			      size_t count, loff_t * offset)
{
	int status;

	if (fhidc_nmea->nmea_read_index == fhidc_nmea->nmea_write_index) {
		info("nmea fifo is empty\n");
		return -EAGAIN;
	}

	status =
	    __copy_to_user(buffer,
			   fhidc_nmea->nmea_fifo[fhidc_nmea->nmea_read_index],
			   FHIDC_NMEA_STR_LEN * sizeof(char));

	if (++fhidc_nmea->nmea_read_index >= FHIDC_NMEA_FIFO_SIZE)
		fhidc_nmea->nmea_read_index = 0;

	return FHIDC_NMEA_STR_LEN * sizeof(char);
}

static int fhidc_nmea_set_report(const unsigned char *buf, int len)
{
	static char nmeastr[FHIDC_NMEA_STR_LEN] = { 0 };

	if ((len < FHIDC_NMEA_REPORT_SIZE) || (*buf++ != fhidc_nmea_report.id)) {
		err("huh? malformed nmea report\n");
		return 0;
	}

	memcpy(nmeastr, buf, FHIDC_NMEA_STR_LEN);
	fhidc_nmea_fifo_write(nmeastr);
	nmeastr[FHIDC_NMEA_STR_LEN - 1] = 0;
	info("set-report: {%s}\n", nmeastr);
	return 0;
}

static unsigned char fhidc_nmea_desc[] = {
	/* NMEA */
	0x06, 0x00, 0xFF,	// USAGE_PAGE (Vendor defined)
	0x09, 0x04,		// USAGE (Vendor Usage - GPS NMEA)
	0xA1, 0x01,		// COLLECTION (Appllication)
	0x85, 0x00,		// REPORT_ID (?)
	0x75, 0x08,		// REPORT_SIZE (8)
	0x95, 0x52,		// REPORT_COUNT (82)
	0x15, 0x00,		// LOGICAL_MINIMUM (0)
	0x26, 0xFF, 0x00,	// LOGICAL_MAXIMUM (255)
	0x09, 0x08,		// USAGE (Vendor usage - GPS NMEA)
	0x91, 0x02,		// OUTPUT (Data, Var, Abs)
	0xC0,			// END_COLLECTION
};

#define FHIDC_NMEA_DESC_ID_OFFSET	8

static void fhidc_nmea_set_report_id(int id)
{
	fhidc_nmea_desc[FHIDC_NMEA_DESC_ID_OFFSET] = id;
}

static int fhidc_nmea_init(void)
{
	int i;
	if (!(fhidc_nmea = kzalloc(sizeof *fhidc_nmea, GFP_KERNEL)))
		return -ENOMEM;
	fhidc_nmea->nmea_read_index = 0;
	fhidc_nmea->nmea_write_index = 0;

	fhidc_nmea_set_report_id(fhidc_nmea_report.id);

	for (i = 0; i < FHIDC_NMEA_FIFO_SIZE; i++)
		fhidc_nmea->nmea_fifo[i] =
		    kmalloc(FHIDC_NMEA_STR_LEN * sizeof(char), GFP_KERNEL);

	return 0;
}

static int fhidc_nmea_exit(void)
{
	int i;

	fhidc_nmea_set_report_id(0);

	for (i = 0; i < FHIDC_NMEA_FIFO_SIZE; i++) {
		if (fhidc_nmea->nmea_fifo[i]) {
			kfree(fhidc_nmea->nmea_fifo[i]);
			fhidc_nmea->nmea_fifo[i] = 0;
		}
	}
	kfree(fhidc_nmea);

	return 0;
}

/* ------------------------------------------------------------------------- */
/*                         external interface                                */
/* ------------------------------------------------------------------------- */

struct fhidc_report fhidc_nmea_report = {
	.size = FHIDC_NMEA_REPORT_SIZE,
	.init = fhidc_nmea_init,
	.exit = fhidc_nmea_exit,
	.set_report = fhidc_nmea_set_report,
	.read = fhidc_nmea_read,
	.desc = fhidc_nmea_desc,
	.desc_size = sizeof(fhidc_nmea_desc),
};
#else
struct fhidc_report fhidc_nmea_report = { 0 };
#endif
