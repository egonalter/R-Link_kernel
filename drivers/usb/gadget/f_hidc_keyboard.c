/* b/drivers/usb/gadget/f_hidc_keyboard.c
 *
 * Copyright (c) 2009 TomTom BV <http://www.tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 */
#include "f_hidc.h"

#if defined(CONFIG_USB_AVH_KEYBOARD) || defined(CONFIG_USB_ANDROID_KEYBOARD)

#include <linux/version.h>
#include <linux/input.h>
#include <linux/usb/composite.h>

#undef INFO
#undef DEBUG
#define ERROR

#define PFX "usb-fhidc-keyboard: "

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

#define FHIDC_KEYBOARD_REPORT_SIZE	3

#define UNASSIGNED -1

/* Define this to enable processing of all key events 
 * 
 * The report is interpreted as a 12 bit key code with
 * a 4 bit modifier. The 10 remote control and 4 steering
 * wheel buttons are still supported. If key code = 0 then
 * the 4 bit modifier is used as the index in fhid_keyboard_keycode.
 * Otherwise, the modifier is used as the state of the key.
 * Note that for the 'remote'/'steering wheel' controls, per
 * received report, two input framework events are generated (down/up).
 * For the other key events (key code != 0), only one event is
 * generated according to the modifier state.
 * This allows transmission of modified keys. To transmit ALT-G:
 *  - LEFT-ALT down
 *  - LEFT-SHIFT down
 *  - G down
 *  - G up
 *  - LEFT-SHIFT up
 *  - LEFT-ALT up
 *
 * If left undefined, only the keys supported in fhid_keyboard_keycode
 * are supported.
 */
#define SUPPORT_ALL_KEYS

#ifdef SUPPORT_ALL_KEYS
#define	STANDARD_KEY_STATE_LENGTH	(4)
#define STANDARD_KEY_MASK		(0xffff << STANDARD_KEY_STATE_LENGTH)
#define	STANDARD_KEY_STATE_MASK		(~STANDARD_KEY_MASK)
#endif

/* event specific definitions */
static int fhidc_keyboard_keycode[] = {
	KEY_MENU, KEY_C, KEY_X, KEY_UP,
	KEY_RIGHT, KEY_VOLUMEUP, KEY_Z, KEY_LEFT,
	KEY_DOWN, KEY_VOLUMEDOWN, KEY_ENTER, KEY_HOME,
	KEY_END, KEY_PAGEUP, KEY_PAGEDOWN, UNASSIGNED,
};

static struct input_dev *input_remote_keyboard;

static int fhidc_keyboard_set_report(const unsigned char *buf, int len)
{
	unsigned short scancode;

	info("keyboard set report");

	if (!buf)
		return 0;

	if ((len < FHIDC_KEYBOARD_REPORT_SIZE) ||
	    (*buf++ != fhidc_keyboard_report.id)) {
		err("huh? malformed keyboard report\n");
		return 0;
	}

	scancode = *buf++;
	scancode |= (*buf) << 8;

#ifdef SUPPORT_ALL_KEYS
	if (scancode & STANDARD_KEY_MASK) {
		int state = scancode & STANDARD_KEY_STATE_MASK;
		int code =
		    (scancode & STANDARD_KEY_MASK) >> STANDARD_KEY_STATE_LENGTH;

		info("set-report: {s=%d, std.key=%d state=%d}\n", scancode,
		     code, state);

		if ((code > KEY_RESERVED) && (code < KEY_UNKNOWN)) {
			input_report_key(input_remote_keyboard, code, state);
			input_sync(input_remote_keyboard);
		}

		return 0;
	}
#endif

	if (scancode < ARRAY_SIZE(fhidc_keyboard_keycode)) {
		unsigned int keycode = fhidc_keyboard_keycode[scancode];

		info("set-report: {s=%d, k=%d}\n", scancode, keycode);

		input_report_key(input_remote_keyboard, keycode, 1);
		input_report_key(input_remote_keyboard, keycode, 0);
		input_sync(input_remote_keyboard);
	} else {
		err("keyboard event out of range: scancode: %d", scancode);
	}

	return 0;
}

static unsigned char fhidc_keyboard_desc[] = {
	/* KEYBOARD */
	0x06, 0x00, 0xFF,	// USAGE_PAGE (Vendor defined)
	0x09, 0x02,		// USAGE (Vendor Usage - Remote keyboard)
	0xA1, 0x01,		// COLLECTION (Application)
	0x85, 0x00,		// REPORT_ID (?)
	0x75, 0x10,		// REPORT_SIZE (16)
	0x95, 0x01,		// REPORT_COUNT (1)
	0x15, 0x00,		// LOGICAL_MINIMUM (0)
	0x27, 0xff, 0xff, 0x00, 0x00,	// LOGICAL_MAXIMUM (65535)
	0x09, 0x01,		// USAGE (Vendor usage - scancode)
	0x91, 0x02,		// OUTPUT (Data, Var, Abs)
	0xC0,			//END_COLLECTION
};

#define FHIDC_KEYBOARD_DESC_ID_OFFSET	8

static void fhidc_keyboard_set_report_id(int id)
{
	fhidc_keyboard_desc[FHIDC_KEYBOARD_DESC_ID_OFFSET] = id;
}

static int fhidc_keyboard_init(void)
{
	int i;
	int error;
	struct input_dev *input_keyboard;

	input_keyboard = input_allocate_device();
	if (!input_keyboard) {
		err("%s - allocate input device failed\n", __FUNCTION__);
		return -ENODEV;
	}

	input_remote_keyboard = input_keyboard;

	/* init input device */
	input_keyboard->name = "remote_keyboard";
	input_keyboard->phys = "remote_keyboard/input0";
	input_keyboard->id.bustype = BUS_USB;
	input_keyboard->id.vendor = fhidc_function_driver.param.vendor;
	input_keyboard->id.product = fhidc_function_driver.param.product;
	input_keyboard->id.version = fhidc_function_driver.param.version;

	/* enable event bits */
	input_keyboard->evbit[0] = BIT_MASK(EV_KEY);

#ifdef SUPPORT_ALL_KEYS
	/* enable all keys except KEY_RESERVED and KEY_UNKNOWN */
	for (i = 1; i < KEY_UNKNOWN; i++) {
		set_bit(i, input_keyboard->keybit);
	}		
#else
	for (i = 0; i < ARRAY_SIZE(fhid_keyboard_keycode); i++) {
		if (fhid_keyboard_keycode[i] < 0) continue;
	/* enable all keys except KEY_RESERVED and KEY_UNKNOWN */
		set_bit(fhid_keyboard_keycode[i], input_keyboard->keybit);
	}
#endif

	error = input_register_device(input_remote_keyboard);
	if (error) {
		err("failed to register input device remote keyboard\n");
		goto fail;
	}

	fhidc_keyboard_set_report_id(fhidc_keyboard_report.id);

	return 0;

fail:
	if (input_keyboard) {
		input_free_device(input_keyboard);
		input_keyboard = 0;
	}

	input_remote_keyboard = 0;

	return error;
}

static int fhidc_keyboard_exit(void)
{
	fhidc_keyboard_set_report_id(0);

	if (input_remote_keyboard) {
		input_unregister_device(input_remote_keyboard);
	}

	return 0;
}

/* ------------------------------------------------------------------------- */
/*                         external interface                                */
/* ------------------------------------------------------------------------- */

struct fhidc_report fhidc_keyboard_report = {
	.size = FHIDC_KEYBOARD_REPORT_SIZE,
	.init = fhidc_keyboard_init,
	.exit = fhidc_keyboard_exit,
	.set_report = fhidc_keyboard_set_report,
	.desc = fhidc_keyboard_desc,
	.desc_size = sizeof(fhidc_keyboard_desc),
};
#else
struct fhidc_report fhidc_keyboard_report = { 0 };
#endif
