/* b/drivers/usb/gadget/f_hidc.c
 *
 * Copyright (c) 2009 TomTom BV <http://www.tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 */
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/usb/composite.h>
#include <linux/uaccess.h>
#include <linux/semaphore.h>
#include <linux/poll.h>

#ifdef CONFIG_USB_ANDROID_HIDC
#include <linux/usb/android_composite.h>
#endif

#include "f_hidc.h"
#include <linux/version.h>
#include <linux/input.h>
#include <linux/list.h>

#define INFO
#undef DEBUG
#define ERROR

/* define BUGME to enable run-time invariant checks */
#undef BUGME

#define PFX "usb-fhidc: "

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

#define FHIDC_DEFAULT_MAJOR	(230)
#define FHIDC_DEFAULT_VENDOR	(0x0000)
#define FHIDC_DEFAULT_PRODUCT	(0x0000)
#define FHIDC_DEFAULT_VERSION	(0x0000)
#define FHIDC_DEFAULT_LIMITS_X  (0xFFFF)
#define FHIDC_DEFAULT_LIMITS_Y  (0xFFFF)
#define FHIDC_DEFAULT_LIMITS_P  (0xFFFF)

#define FHIDC_NAME            "fhid"
#define FHIDC_MAJOR           248
#define FHIDC_DEV_MINOR       0

#define FHIDC_STRING_IDX      0

static struct usb_string fhidc_strings_dev[] = {
	[FHIDC_STRING_IDX].s = "USB Events",
	{}
};

static struct usb_gadget_strings fhidc_stringtab_dev = {
	.language = 0x0409,
	.strings = fhidc_strings_dev,
};

static struct usb_gadget_strings *fhidc_strings[] = {
	&fhidc_stringtab_dev,
	NULL,
};

static struct usb_interface_descriptor hid_interface_desc = {
	.bLength = sizeof hid_interface_desc,  /* size of the descriptor in bytes */
	.bDescriptorType = USB_DT_INTERFACE,   /* interface descriptor. 0x04 */
	.bInterfaceNumber = 0x00,              /* interface number */
	.bAlternateSetting = 0x00,             /* index of this setting */
	.bNumEndpoints = 0x01,                 /* we don't have any extra endpoints (interrupt EP for example) */
	.bInterfaceClass = 0x03,               /* HID class: 0x03 */
	.bInterfaceSubClass = 0x00,            /* no subclass; only used for boot interface */
	.bInterfaceProtocol = 0x00,            /* not used. */
};

static struct hid_descriptor hid_desc = {
	.bLength = sizeof hid_desc,
	.bHIDDescriptorType	= USB_DT_HID,
	.bcdHID = __constant_cpu_to_le16(0x111),
	.bCountryCode		= 0,
	.bNumDescriptors	= 1,
	.bReportDescriptorType	= USB_DT_REPORT,
	.wDescriptorLength 	= __constant_cpu_to_le16(0),
};

/* this endpoint is not really used, but it is here because the specification 
   demands it so its static characteristics are set to extreme values (small 
   packets, long inter-packet interval */

static struct usb_endpoint_descriptor fs_intr_in_ep_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = USB_DIR_IN,
	.bmAttributes = USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize = __constant_cpu_to_le16(8), 	/* minimal size               */
	.bInterval = 0x10,				/* 16 = 2^15 frames (~30 sec) */
};

static struct usb_endpoint_descriptor hs_intr_in_ep_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = USB_DIR_IN,
	.bmAttributes = USB_ENDPOINT_XFER_INT,
	.wMaxPacketSize = __constant_cpu_to_le16(8), 	/* minimal size                   */
	.bInterval = 0x10,				/* 16 = 2^15 microframes (~ 4sec) */
};

static struct usb_descriptor_header *fhidc_full_speed_descriptors[] = {
	(struct usb_descriptor_header *)&hid_interface_desc,
	(struct usb_descriptor_header *)&hid_desc,
	(struct usb_descriptor_header *)&fs_intr_in_ep_desc,
	NULL,
};

#ifdef CONFIG_USB_GADGET_DUALSPEED
static struct usb_descriptor_header *fhidc_high_speed_descriptors[] = {
	(struct usb_descriptor_header *)&hid_interface_desc,
	(struct usb_descriptor_header *)&hid_desc,
	(struct usb_descriptor_header *)&hs_intr_in_ep_desc,
	NULL,
};
#endif /* CONFIG_USB_GADGET_DUALSPEED */

/* event specific definitions */
enum fhidc_dev_state {
	FHIDC_STATE_VOID,          /* pseudo state when driver is not initialized
				    (fhidc_dev == 0) */
	FHIDC_STATE_IDLE,          /* state after config_bind */
	FHIDC_STATE_CONFIGURED,    /* configuration binding done, necessary
				     resources claimed, user space access ok */
	FHIDC_STATE_SELECTED,      /* interface activated */
};

/* this array determines the report id associated with a certain report
   handler; note that some of these reports may - according to the kernel
   configuration - not be supported (these will have desc and all func pointers
   equal to zero. */

static struct fhidc_report *fhidc_reports[] = {
	0,
	&fhidc_panel_report,
	&fhidc_keyboard_report,
/*	&fhidc_nmea_report,
	&fhidc_lightsensor_report,
	&fhidc_positionsensor_report,
	&fhidc_routeinformation_report, */
};

/* this array determines the assignment of minor numbers to report handlers;
   those that need fops (minor 0 is reserved for fops associated with this
   generic fhid driver) */
static struct fhidc_report *fhidc_minors[] = {
	0,
/*	&fhidc_nmea_report,
	&fhidc_lightsensor_report,
	&fhidc_positionsensor_report,
	&fhidc_routeinformation_report, */
};

static struct fhidc_dev {
	struct semaphore 		mutex;
	enum fhidc_dev_state 		state;
	int 				use;
	int 				alt;
	struct usb_ep                   *intr_in;
	struct usb_endpoint_descriptor  *intr_in_ep_desc;
	dev_t 				devnum;
} *fhidc_dev = 0;

static inline void fhidc_set_dev_state(struct fhidc_dev *dev, 
						enum fhidc_dev_state state)
{
	if (!dev)
		return;

	if (state != dev->state) {
		info("switching from %d to %d\n", dev->state, state);
		dev->state = state;
	}
}

static inline int fhidc_access_claim(void)
{
	BUGME_ON(!fhidc_dev);
	BUGME_ON(fhidc_dev->state < FHIDC_STATE_CONFIGURED);

	return down_interruptible(&fhidc_dev->mutex);
}

static inline void fhidc_access_release(void)
{
	BUGME_ON(!fhidc_dev);

	up(&fhidc_dev->mutex);
}

static void fhidc_disable(struct usb_function *f)
{
	BUGME_ON(!fhidc_dev);

	if (fhidc_dev->state <= FHIDC_STATE_CONFIGURED)
		return;

	fhidc_set_dev_state(fhidc_dev, FHIDC_STATE_CONFIGURED);

	usb_ep_disable(fhidc_dev->intr_in);
}

static int fhidc_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	struct usb_composite_dev *cdev = f->config->cdev;

	BUGME_ON(!fhidc_dev);
	BUGME_ON(fhidc_dev->state < FHIDC_STATE_CONFIGURED);

	info("setting alternate setting (intf=%d,alt=%d,current=%d)",
						intf, alt, fhidc_dev->alt);

	if ((intf == hid_interface_desc.bInterfaceNumber) && 
	    (alt == hid_interface_desc.bAlternateSetting)) {
		fhidc_dev->intr_in_ep_desc = ep_choose(cdev->gadget, 
							&fs_intr_in_ep_desc,
							&hs_intr_in_ep_desc);

		usb_ep_enable(fhidc_dev->intr_in, fhidc_dev->intr_in_ep_desc);

		fhidc_set_dev_state(fhidc_dev, FHIDC_STATE_SELECTED);
		fhidc_dev->alt = alt;
	}

	return 0;
}

static int fhidc_get_alt(struct usb_function *f, unsigned intf)
{
	BUGME_ON(!fhidc_dev);
	info("getting alternate setting (=%d)", fhidc_dev->alt);
	return fhidc_dev->alt;
}

static void fhidc_suspend(struct usb_function *f)
{
	info("suspend (nop)\n");
}

static void fhidc_resume(struct usb_function *f)
{
	info("resume (nop)\n");
}

/* -------------------------------------------------------------------------
       functions that are safe to call from atomic (non-blocking) context
   ------------------------------------------------------------------------- */

static void fhidc_set_report_complete(struct usb_ep *ep, 
						struct usb_request *req)
{
	struct fhidc_report *r;
	if (!ep)
		return;
	if (!req)
		return;

	if (req->status) {
		err("set report request failure (%d)\n", req->status);
		return;
	}

	r = (struct fhidc_report*) req->context;

	info("forwarding set_report_complete...\n");

	if ((r) && (r->set_report)) 
		r->set_report(req->buf, req->length);
		}

static int class_setup_req(struct usb_composite_dev *cdev, 
				    const struct usb_ctrlrequest *ctrl)
{
	int value = -EOPNOTSUPP;
	u16 w_value = le16_to_cpu(ctrl->wValue);
	u16 w_length = le16_to_cpu(ctrl->wLength);
	struct usb_request *req = cdev->req;

	info("class setup (%d)\n", ctrl->bRequest);

	switch (ctrl->bRequest) {
	case USB_REQ_GET_REPORT:	//Mandatory
	{
		int id = w_value & 0xff;
		unsigned char *buf = req->buf;

		info("USB_REQ_GET_REPORT\n");

		if (id > ARRAY_SIZE(fhidc_reports)) {
			err("unsupported report id\n");
			value = -EINVAL;
		} else {
			struct fhidc_report *r = fhidc_reports[id];

			if((r) && (r->get_report)) {
				value = r->get_report(buf, w_length);
			} else {
				err("unregistered report id!?\n");
				value = -EINVAL;
			}
		}
		break;
	}

	case USB_REQ_SET_REPORT:
	{
		int id = w_value & 0xff;
		
		info("USB_REQ_SET_REPORT\n");
		
		if (id > ARRAY_SIZE(fhidc_reports)) {
			err("unsupported report id\n");
			value = -EINVAL;
		} else {
			struct fhidc_report *r = fhidc_reports[id];

			if((r) && (r->set_report) && (r->size > 0)) {
				cdev->req->complete =
				    fhidc_set_report_complete;
				cdev->req->context = r;
				value = r->size;
			} else {
				err("unregistered report id!?\n");
				value = -EINVAL;
			}
		}
		break;
	}

	case USB_REQ_SET_IDLE:
		info("USB_REQ_SET_IDLE (ignored)\n");
		value = 0;
		break;

	case USB_REQ_GET_PROTOCOL:
		err("USB_REQ_GET_PROTOCOL (ignored)\n");
		value = -EOPNOTSUPP;
		break;

	case USB_REQ_GET_IDLE:
		err("USB_REQ_GET_IDLE (ignored)\n");
		value = -EOPNOTSUPP;
		break;

	case USB_REQ_SET_PROTOCOL:              //Only required for boot devices
		info("USB_REQ_SET_PROTOCOL (ignored)\n");
		value = 0;
		break;
	}

	return value;
}

static int fhidc_copy_hid_descriptor(unsigned char* buf, int len)
{
	BUGME_ON(!fhidc_dev);
	memcpy(buf, &hid_desc, hid_desc.bLength);
	return (hid_desc.bLength);
}

static int fhidc_copy_report_descriptors(unsigned char* buf, int len)
{
	int i;
	unsigned char* org = buf;

	BUGME_ON(!fhidc_dev);

	for(i = 0; i < ARRAY_SIZE(fhidc_reports); i++) {
		struct fhidc_report *r = fhidc_reports[i];
		if ((r) && (r->desc) && (r->desc_size > 0)) {
			len -= r->desc_size;
			if (len >= 0) {
				memcpy(buf, r->desc, r->desc_size);
			} else
				break;

			buf += r->desc_size;
		}
	}
	
	return (buf - org);
}

/* Handle non class specific setup messages */
static int standard_setup_req(struct usb_composite_dev *cdev, 
				       const struct usb_ctrlrequest *ctrl)
{
	struct usb_request *req = cdev->req;
	int value = -EOPNOTSUPP;
	u16 w_value = le16_to_cpu(ctrl->wValue);
	u16 w_length = le16_to_cpu(ctrl->wLength);

#ifdef INFO
	u16 w_index = le16_to_cpu(ctrl->wIndex);
#endif
	info("standard setup (req=%d val=%d idx=%d len=%d)\n", 
				ctrl->bRequest, w_value, w_index, w_length);

	switch (ctrl->bRequest) {
	case USB_REQ_GET_DESCRIPTOR:
		switch (w_value >> 8) {
		/* Class specific */
		case USB_DT_HID:
			/* HID descriptor */
			value = fhidc_copy_hid_descriptor(req->buf, w_length);
			break;
		case USB_DT_REPORT:
			value =
			    fhidc_copy_report_descriptors(req->buf, w_length);
			break;
		case USB_DT_PHYSICAL:
			value = 0;
			break;
		}
		break;

	default:
		err("unknown control\n");
		break;
	}
	return value;
}

static int fhidc_setup(struct usb_function *f, 
				const struct usb_ctrlrequest *ctrl)
{
	u16 w_length = le16_to_cpu(ctrl->wLength);
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_request *req = cdev->req;
	int value = -EOPNOTSUPP;

	info("setup\n");

	BUGME_ON(!fhidc_dev);
	BUGME_ON(fhidc_dev->state < FHIDC_STATE_CONFIGURED);

	if ((ctrl->bRequestType & USB_TYPE_MASK) == USB_TYPE_CLASS)
		value = class_setup_req(cdev, ctrl);
	else
		value = standard_setup_req(cdev, ctrl);

	if (value >= 0) {
		value = min((u16)value, w_length);
		req->length = value;
		req->zero = value < w_length;
		value = usb_ep_queue(cdev->gadget->ep0, req, GFP_ATOMIC);
	}
	return value;
}

static unsigned int fhidc_poll(struct file *file, poll_table * wait)
{
	int result;
	unsigned long int flags;
	struct inode *inode = file->f_path.dentry->d_inode;
	int idx = iminor(inode);

	BUGME_ON(!fhidc_dev);

	info("polling minor hid device %d\n", idx);

	result = fhidc_access_claim();

	if (result) {
		err("could not get driver access from user space?\n");
		return result;
	}

	local_irq_save(flags);

	result = -EOPNOTSUPP;

	if (idx != FHIDC_DEV_MINOR) {
		struct fhidc_report *r =
		    (struct fhidc_report *)file->private_data;
		if ((r) && (r->poll))
			result = r->poll(file, wait);
	}

	local_irq_restore(flags);

	fhidc_access_release();

	return result;
}

/* need to acquire device structure lock and disable interrupts */
static int fhidc_write(struct file *file, const char __user * buffer, 
			size_t count, loff_t* offset)
{
	int result;
	unsigned long int flags;
	struct inode *inode = file->f_path.dentry->d_inode;
	int idx = iminor(inode);

	BUGME_ON(!fhidc_dev);

	info("writing to minor hid device %d\n", idx);

	result = fhidc_access_claim();

	if (result) {
		err("could not get driver access from user space?\n");
		return result;
	}

	local_irq_save(flags);

	result = -EOPNOTSUPP;

	if (idx != FHIDC_DEV_MINOR) {
		struct fhidc_report *r =
		    (struct fhidc_report *)file->private_data;
		if ((r) && (r->write))
			result = r->write(file, buffer, count, offset);
	}

	local_irq_restore(flags);

	fhidc_access_release();

	return result;
}

/* needs to acquire device structure lock */
static int fhidc_open(struct inode *inode, struct file *file)
{
	int result;
	int idx = iminor(inode);
	int i;

	/* TODO: check open mode - READ/WRITE with availability of operations */

	BUGME_ON(!fhidc_dev);

	result = fhidc_access_claim();

	if (result) {
		err("could not get driver access from user space?\n");
		return result;
	}

	file->private_data = 0;
	/* mutex locked, do not 'return' */
	
	result = -EOPNOTSUPP;

	for (i = 0; i < ARRAY_SIZE(fhidc_reports); i++) {
		struct fhidc_report *r = fhidc_reports[i];
		if ((r) && (r->minor == idx)) {
			file->private_data = r;
			result = 0;
			break;
		}
	}

	if (result == -ENODEV) {
		goto unlock;
	}

	if (fhidc_dev->use & (1 << idx)) {
		result = -EBUSY;
		goto unlock;
	}

	fhidc_dev->use |= (1 << idx);

 unlock:
	fhidc_access_release();

	return result;
}

static int fhidc_release(struct inode *inode, struct file *file)
{
	int result;
	int idx = iminor(inode);

	BUGME_ON(!fhidc_dev);

	result = fhidc_access_claim();

	if (result) {
		err("could not get driver access from user space?\n");
		return result;
	}

	if (file->private_data == 0) {
		result = -ENODEV;
		goto unlock;
	}

	if (!(fhidc_dev->use & (1 << idx))) {
		result = -ENOENT;
		goto unlock;
	}

	fhidc_dev->use &= ~(1 << idx);

 unlock:
	fhidc_access_release();

	return result;
}

/* need to acquire device structure lock and disable interrupts */
static ssize_t fhidc_dev_read(struct file *file, char __user * buffer, 
				size_t count, loff_t* offset)
{
	/* no locking required, only access to read-only data */

	return fhidc_copy_report_descriptors(buffer, count);
}

static ssize_t fhidc_read(struct file *file, char __user * buffer, 
			size_t count, loff_t* offset)
{
	int result;
	unsigned long int flags;
	struct inode *inode = file->f_path.dentry->d_inode;
	int idx = iminor(inode);

	BUGME_ON(!fhidc_dev);

	info("reading from minor hid device %d\n", idx);

	result = fhidc_access_claim();

	if (result) {
		err("could not get driver access from user space?\n");
		return result;
	}

	local_irq_save(flags);

	result = -EOPNOTSUPP;

	if (idx == FHIDC_DEV_MINOR) {
		result = fhidc_dev_read(file, buffer, count, offset);
	} else {
		struct fhidc_report *r =
		    (struct fhidc_report *)file->private_data;
		if ((r) && (r->read))
			result = r->read(file, buffer, count, offset);
	}
	local_irq_restore(flags);

	fhidc_access_release();

	return result;
}

struct file_operations fhidc_fops = {
	.owner = THIS_MODULE,
	.open = fhidc_open,
	.release = fhidc_release,
	.write = fhidc_write,
	.read = fhidc_read,
	.poll = fhidc_poll,
};

static void fhidc_init_reports(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(fhidc_minors); i++) {
		struct fhidc_report *r = fhidc_minors[i];

		if (r) {
			if ((r->desc) && (r->desc_size > 0)) {
				r->minor = i;
			} else {
				r->id = 0;
			}
		}
	}

	for (i = 0; i < ARRAY_SIZE(fhidc_reports); i++) {
		struct fhidc_report *r = fhidc_reports[i];

		if (r) {
			r->id = i;

			if (r->init) 
				r->init();
		}
	}
}

static void fhidc_exit_reports(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(fhidc_reports); i++) {
		struct fhidc_report *r = fhidc_reports[i];

		if (r) {
			if (r->exit) 
				r->exit();
		}
	}
}

static void __init fhidc_prepare_descriptors(void)
{
	int i;
	int size = 0;
	BUGME_ON(!fhidc_dev);

	for (i = 0; i < ARRAY_SIZE(fhidc_reports); i++) {
		struct fhidc_report *r = fhidc_reports[i];

		if (r) {
			size += r->desc_size;
		}
	}

	hid_desc.wDescriptorLength = __cpu_to_le16(size);
}
/* init/exit - module insert/removal */

static int __init fhidc_bind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct usb_ep *ep = 0;
	int status;

	info("bind\n");

	BUGME_ON(fhidc_dev);

	if (!(fhidc_dev = kzalloc(sizeof *fhidc_dev, GFP_KERNEL)))
		return -ENOMEM;

	/* reset all report types, support for each report
	   depends on the kernel configuration */

	fhidc_init_reports();

	sema_init(&fhidc_dev->mutex, 0);

	fhidc_prepare_descriptors();

	fhidc_set_dev_state(fhidc_dev, FHIDC_STATE_IDLE);

	if (register_chrdev(fhidc_function_driver.param.major, FHIDC_NAME, &fhidc_fops) < 0) {
		err("could not register usb hid character device "
			"(using major number %d)\n", fhidc_function_driver.param.major);
		return -ENODEV;
	} else {
		info("usb hid character device registered\n");
		debug("major number %d\n", fhidc_function_driver.param.major);
	}

	fhidc_dev->devnum = MKDEV(fhidc_function_driver.param.major, 0);

	/* user space access allowed from now */
	up(&fhidc_dev->mutex);

	status = usb_interface_id(c, f);
	if (status < 0) {
		err("allocate interface failed\n");
		goto fail;
	}

	hid_interface_desc.bInterfaceNumber = status;
	info("allocated interface number %d\n", status);

	if (fhidc_strings_dev[0].id == 0) {
		status = usb_string_id(c->cdev);
		if (status < 0)
			goto fail;

		fhidc_strings_dev[0].id = status;
		hid_interface_desc.iInterface = status;
	}

	ep = usb_ep_autoconfig(cdev->gadget, &fs_intr_in_ep_desc);
	if (!ep) {
		err("allocation of interrupt-in endpoint failed\n");
		goto fail;
	}

	info("interrupt-in endpoint ep %s allocated\n", ep->name);

	f->descriptors = fhidc_full_speed_descriptors;
	fhidc_dev->intr_in = ep;

	if (gadget_is_dualspeed(c->cdev->gadget)) {
		hs_intr_in_ep_desc.bEndpointAddress = 
					fs_intr_in_ep_desc.bEndpointAddress;

		f->hs_descriptors = fhidc_high_speed_descriptors;
	}

	ep->driver_data = fhidc_dev;
	fhidc_set_dev_state(fhidc_dev, FHIDC_STATE_CONFIGURED);

	return 0;

 fail:
	/* cleanup of allocated resources (memory, descriptors, ...) is done in
	   fhidc_unbind that is called by the composite framework because this
	   bind call fails */
	err("can't bind\n");
	return -ENODEV;
}

static void fhidc_unbind(struct usb_configuration *c, 
					     struct usb_function *f)
{
	info("unbind\n");

	BUGME_ON(in_interrupt());
	BUGME_ON(!fhidc_dev);

	fhidc_set_dev_state(fhidc_dev, FHIDC_STATE_IDLE);

	fhidc_exit_reports();

	if (fhidc_dev->intr_in) {
		usb_ep_disable(fhidc_dev->intr_in);
	}

	f->descriptors = 0;
	f->hs_descriptors = 0;

	if (fhidc_dev->intr_in) {
		fhidc_dev->intr_in->driver_data = 0;
		fhidc_dev->intr_in = 0;
	}

	if (fhidc_dev->devnum == MKDEV(fhidc_function_driver.param.major, 0)) {
		unregister_chrdev(MAJOR(fhidc_dev->devnum), FHIDC_NAME);
	} else {
		err("skipping unregistering character device driver ?!\n");
	}

	fhidc_set_dev_state(fhidc_dev, FHIDC_STATE_VOID);

	kfree(fhidc_dev);
	fhidc_dev = 0;
}

/* ------------------------------------------------------------------------- */
/*                         external interface                                */
/* ------------------------------------------------------------------------- */

struct fhidc_function fhidc_function_driver = {
	.func = {
		.name = FHIDC_NAME,
		.strings = fhidc_strings,
		.bind = fhidc_bind,
		.unbind = fhidc_unbind,
		.setup = fhidc_setup,
		.disable = fhidc_disable,
		.set_alt = fhidc_set_alt,
		.get_alt = fhidc_get_alt,
		.suspend = fhidc_suspend,
		.resume = fhidc_resume,
	},
	.param = {
		.major = FHIDC_DEFAULT_MAJOR,
		.vendor = FHIDC_DEFAULT_VENDOR,
		.product = FHIDC_DEFAULT_PRODUCT,
		.version = FHIDC_DEFAULT_VERSION,
		.limits.x = FHIDC_DEFAULT_LIMITS_X,
		.limits.y = FHIDC_DEFAULT_LIMITS_Y,
		.limits.p = FHIDC_DEFAULT_LIMITS_P,
	},
};

#ifdef CONFIG_USB_ANDROID_HIDC
module_param_named(hid_major, fhidc_function_driver.param.major, short, S_IRUGO);
MODULE_PARM_DESC(hid_major, "(events) major device number");

module_param_named(hid_max_x, fhidc_function_driver.param.limits.x, short, S_IRUGO);
MODULE_PARM_DESC(hid_major, "(events) major device number");

module_param_named(hid_max_y, fhidc_function_driver.param.limits.y, short, S_IRUGO);
MODULE_PARM_DESC(hid_major, "(events) major device number");

module_param_named(hid_max_p, fhidc_function_driver.param.limits.p, short, S_IRUGO);
MODULE_PARM_DESC(hid_major, "(events) major device number");

static int fhidc_bind_config(struct usb_configuration *c)
{
	int ret = 0;
	printk(KERN_ERR "%s\n", __FUNCTION__);

	fhidc_function_driver.param.vendor = c->cdev->desc.idVendor;
	fhidc_function_driver.param.product = c->cdev->desc.idProduct;
	fhidc_function_driver.param.version = c->cdev->desc.bcdDevice;

	ret = usb_add_function(c, &fhidc_function_driver.func);
	if(ret) {
		info("adding hid function failed\n");
	}
	printk(KERN_ERR "%s: returning %d\n", __FUNCTION__, ret);
	return ret;
}

static struct android_usb_function fhidc_function = {
	.name = FHIDC_NAME,
	.bind_config = fhidc_bind_config,
};

static int __init fhidc_init(void)
{
	printk(KERN_INFO "fhidc init\n");
	android_register_function(&fhidc_function);
	return 0;
}
late_initcall(fhidc_init);
#endif