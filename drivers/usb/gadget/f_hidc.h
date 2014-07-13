/* b/drivers/usb/gadget/f_hidc.h
 * 
 * Copyright (c) 2009 TomTom BV <http://www.tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 */
#ifndef __F_HIDC_H__
#define __F_HIDC_H__

#include <linux/usb.h>
#include <linux/usb/gadget.h>
#include <linux/usb/composite.h>
#include <linux/fs.h>
#include <linux/poll.h>

#undef INFO
#undef DEBUG
#undef ERROR
#undef info
#undef debug
#undef err

struct fhidc_device_parameters {
	unsigned short major;
	unsigned short vendor;
	unsigned short product;
	unsigned short version;
	struct {
		unsigned short x;
		unsigned short y;
		unsigned short p;
	} limits;
};

struct fhidc_function {
	struct usb_function func;
	struct fhidc_device_parameters param;
};

extern struct fhidc_function fhidc_function_driver;

struct fhidc_report {
	unsigned int		id;
	int 			minor;
	int			size;
	int			desc_size;
	const unsigned char* 	desc;

	int	(*init)(void);
	int	(*exit)(void);

	int 	(*set_report)(const unsigned char* buf, int len);

	int 	(*get_report)(unsigned char* buf, int len);

	int 	(*read)(struct file *file, char __user *buffer,
				size_t count, loff_t *offset);

	int 	(*write)(struct file *file, const char __user *buffer, 
				size_t count, loff_t *offset);

	unsigned int 	(*poll)(struct file *file, poll_table *wait);
};

extern struct fhidc_report fhidc_panel_report;
extern struct fhidc_report fhidc_keyboard_report;

extern struct fhidc_report fhidc_nmea_report;
extern struct fhidc_report fhidc_lightsensor_report;
extern struct fhidc_report fhidc_positionsensor_report;
extern struct fhidc_report fhidc_routeinformation_report;

/* the definitions below should be included in include/linux/usb/hid.h */

#ifndef __LINUX_USB_HID_EXTENSION_H
#define __LINUX_USB_HID_EXTENSION_H

#include <linux/types.h>

#define USB_REQ_GET_REPORT		0x01
#define USB_REQ_GET_IDLE		0x02
#define USB_REQ_GET_PROTOCOL		0x03
#define USB_REQ_SET_REPORT		0x09
#define USB_REQ_SET_IDLE		0x0A
#define USB_REQ_SET_PROTOCOL		0x0B

#define USB_DT_HID			0x21
#define USB_DT_REPORT			0x22
#define USB_DT_PHYSICAL			0x23

struct hid_descriptor {
	__u8 bLength;
	__u8 bHIDDescriptorType;
	__u16 bcdHID;
	__u8 bCountryCode;
	__u8 bNumDescriptors;
	__u8 bReportDescriptorType;
	__u16 wDescriptorLength;
} __attribute__ ((packed));

#endif //__LINUX_USB_HID_EXTENSION_H

#endif //__F_HID_H__
