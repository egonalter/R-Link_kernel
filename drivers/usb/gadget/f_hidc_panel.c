/* b/drivers/usb/gadget/f_hidc_panel.c
 *
 * Copyright (c) 2009 TomTom BV <http://www.tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 */

#include "f_hidc.h"

#if defined(CONFIG_USB_AVH_PANEL) || defined(CONFIG_USB_ANDROID_PANEL)

#include <linux/version.h>
#include <linux/input.h>
#include <linux/usb/composite.h>

#undef INFO
#undef DEBUG
#define ERROR

#define PFX "usb-fhidc-panel: "

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

#define FHIDC_PANEL_REPORT_SIZE	7
#undef ANDROID_SONY_HACK

#ifdef ANDROID_SONY_HACK
#warning "f_hidc_panel: input coordinates are scaled with a factor (800/480, 480/272)!"
#endif

static struct input_dev *input_remote_panel;

static int fhidc_panel_set_report(const unsigned char *buf, int len)
{
	unsigned long int x;
	unsigned long int y;
	unsigned long int pressure;

	if (!buf)
		return 0;

	if ((len < FHIDC_PANEL_REPORT_SIZE) || (*buf++ != fhidc_panel_report.id)) {
		err("huh? malformed panel report\n");
		return 0;
	}

	x = *buf++;
	x |= (*buf++) << 8;

	y = *buf++;
	y |= (*buf++) << 8;

	pressure = *buf++;
	pressure |= (*buf++) << 8;

#ifdef ANDROID_SONY_HACK

	x = (x * 8000)/(4800);
	y = (y * 4800)/(2720);

	
#endif

	info("set-report: {%ld @ (%ld,%ld)}\n", pressure, x, y);
	/* ANDROID expects a BTN_TOUCH when pressing/releasing,
		otherwise the whole event (press/drag/release) is not
		treated as a valid event. */
	input_report_key(input_remote_panel, BTN_TOUCH, (pressure == 0)?0:1);
	input_report_abs(input_remote_panel, ABS_X, x);
	input_report_abs(input_remote_panel, ABS_Y, y);
	input_report_abs(input_remote_panel, ABS_PRESSURE, pressure);
	input_sync(input_remote_panel);

	return 0;
}

static unsigned char fhidc_panel_desc[] = {
	/* TOUCH_SCREEN */
	0x06, 0x00, 0xff,	// USAGE_PAGE (Vendor defined)
	0x09, 0x01,		// USAGE (Vendor Usage - touch screen)
	0xa1, 0x01,		// COLLECTION (Application) touch screen
	0x85, 0x00,		//   REPORT_ID (?)
	0x75, 0x10,		//   REPORT_SIZE (16) // size in bits
	0x95, 0x03,		//   REPORT_COUNT (3) // X, Y, pressure
	0x15, 0x00,		//   LOGICAL_MINIMUM (0)
	0x27, 0xff, 0xff, 0x00, 0x00,	//   LOGICAL_MAXIMUM (65535)
	0x09, 0x01,		//   USAGE (Vendor Usage - X)
	0x09, 0x02,		//   USAGE (Vendor Usage - Y)
	0x09, 0x03,		//   USAGE (Vendor Usage - Pressure)
	0x91, 0x02,		//   OUTPUT (Data,Var,Abs)
	0xc0,			// END_COLLECTION touch screen
};

#define FHIDC_PANEL_DESC_ID_OFFSET	8

static void fhidc_panel_set_report_id(int id)
{
	fhidc_panel_desc[FHIDC_PANEL_DESC_ID_OFFSET] = id;
}

static int fhidc_panel_init(void)
{
	int status;
	struct input_dev *input_panel;

	input_panel = input_allocate_device();
	if (!input_panel) {
		err("%s - allocate input device failed\n", __FUNCTION__);
		status = -ENODEV;
		goto fail;
	}

	input_remote_panel = input_panel;

	/* init input device */
	input_panel->name = "remote_ts";
	input_panel->phys = "remote_ts/input0";
	input_panel->id.bustype = BUS_USB;
	input_panel->id.vendor = fhidc_function_driver.param.vendor;
	input_panel->id.product = fhidc_function_driver.param.product;
	input_panel->id.version = fhidc_function_driver.param.version;

	/* enable event bits */
#if LINUX_VERSION_CODE > KERNEL_VERSION(2,6,24)
	input_panel->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	input_panel->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
#else
	input_panel->evbit[0] = BIT(EV_KEY) | BIT(EV_ABS);
	input_panel->keybit[LONG(BTN_TOUCH)] = BIT(BTN_TOUCH);
#endif
	input_set_abs_params(input_panel, ABS_X, 0, fhidc_function_driver.param.limits.x, 0, 0);
	input_set_abs_params(input_panel, ABS_Y, 0, fhidc_function_driver.param.limits.y, 0, 0);
	input_set_abs_params(input_panel, ABS_PRESSURE, 0, fhidc_function_driver.param.limits.p, 0, 0);

	status = input_register_device(input_remote_panel);
	if (status) {
		err("failed to register input device remote (touch) panel\n");
		goto fail;
	}

	fhidc_panel_set_report_id(fhidc_panel_report.id);

	return 0;

      fail:
	if (input_panel) {
		input_free_device(input_panel);
		input_panel = 0;
	}

	input_remote_panel = 0;

	return status;
}

static int fhidc_panel_exit(void)
{
	fhidc_panel_set_report_id(0);

	if (input_remote_panel) {
		input_unregister_device(input_remote_panel);
	}

	return 0;
}

/* ------------------------------------------------------------------------- */
/*                         external interface                                */
/* ------------------------------------------------------------------------- */

struct fhidc_report fhidc_panel_report = {
	.size = FHIDC_PANEL_REPORT_SIZE,
	.init = fhidc_panel_init,
	.exit = fhidc_panel_exit,
	.set_report = fhidc_panel_set_report,
	.desc = fhidc_panel_desc,
	.desc_size = sizeof(fhidc_panel_desc),
};
#else
struct fhidc_report fhidc_panel_report = { 0 };
#endif
