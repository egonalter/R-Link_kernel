/* drivers/tomtom/usbmode/dummy_usbmode.c

Copyright (C) 2008 TomTom BV <http://www.tomtom.com/>
Author: Martin Jackson <martin.jackson@tomtom.com>

This is a quick hack which replaces the low-level usbmode driver.
We use this driver to force the usbmode high-level driver into a particular
state. This allows testability of the usbmode subsystem.

Usage:
This driver exports an attribute in sysfs ("force_event"). To force an event, we
write the text value of one of the INPUT_* defines in the header
	arch/arm/plat-tomtom/include/plat/usbmode.h
to the force_event attribute.

e.g. To force the vbus_on event, we do:

$ echo INPUT_BUS_POWER_ON_EVENT > \
	/sys/devices/platform/usbmode-impl-dummy/force_event

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.
*/

#include <linux/module.h>

#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <plat/usbmode.h>

#include <asm/io.h>
#include <mach/map.h>

#define PFX "dummy_usbmode:"
#define ERR " ERROR"
#define PK_DBG PK_DBG_FUNC

#define DRIVER_DESC_LONG "TomTom dummy USB mode driver, (C) 2008 TomTom BV "

/* macros for reading and writing USBOTG registers */
#define usbotg_write_reg32(v, x) __raw_writel((v), DUMMY_VA_USB_OTG + (x))
#define usbotg_read_reg32(x) __raw_readl(DUMMY_VA_USB_OTG + (x))

/* Time in seconds to assume there is no host present when performing
in device detect mode(Device detect == detect is we need to become device)*/
#define DEVICE_DETECT_TIMEOUT_SECONDS 2

/* Time in seconds to assume there there is no connected (non hub) device
on the bus */
#define HOST_DETECT_TIMEOUT_SECONDS 2

struct impl_data_dummy {
	/* timer to trigger a time based timeout when in device dectect mode
	 * to detect no activity */
	struct timer_list device_detect_timeout;

	/* work struct to trigger a time based timeout to detect if a device is 
	   attached to the PND */
	struct timer_list host_detect_timeout;

	int reset_counter;
};

static struct impl_data_usbmode *usbmode_impl_data = NULL;
static struct impl_data_dummy   *dummy_impl_data   = NULL;

/**
 * Allow events to be forced with writes to sysfs
 **/

#define force_event(buf, ev) __force_event(buf, #ev, ev)
static inline int __force_event(const char *buf, char *ev_text, int ev)
{
	if (!strncmp(buf, ev_text, strlen(ev_text))) {
		printk("DUMMY: forced event %s\n", ev_text);
		usbmode_push_event(ev);
		return 0;
	}
	else {
		return -1;
	}
	
}
static ssize_t dummy_sysfs_force_write(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
#ifdef INPUT_BUS_POWER_ON_EVENT
	if (!force_event(buf, INPUT_BUS_POWER_ON_EVENT)) {
		return strnlen(buf, PAGE_SIZE);
	}
#endif /* INPUT_BUS_POWER_ON_EVENT */

#ifdef INPUT_BUS_POWER_OFF_EVENT
	if (!force_event(buf, INPUT_BUS_POWER_OFF_EVENT)) {
		return strnlen(buf, PAGE_SIZE);
	}
#endif /* INPUT_BUS_POWER_OFF_EVENT */

#ifdef INPUT_DEVICE_RESET
	if (!force_event(buf, INPUT_DEVICE_RESET)) {
		return strnlen(buf, PAGE_SIZE);
	}
#endif /* INPUT_DEVICE_RESET */

#ifdef INPUT_DEVICE_DETECT_TIMEOUT
	if (!force_event(buf, INPUT_DEVICE_DETECT_TIMEOUT)) {
		return strnlen(buf, PAGE_SIZE);
	}
#endif /* INPUT_DEVICE_DETECT_TIMEOUT */

#ifdef INPUT_HOST_DETECT_TIMEOUT
	if (!force_event(buf, INPUT_HOST_DETECT_TIMEOUT)) {
		return strnlen(buf, PAGE_SIZE);
	}
#endif /* INPUT_HOST_DETECT_TIMEOUT */

#ifdef INPUT_HOST_DEVICE_CONNECT
	if (!force_event(buf, INPUT_HOST_DEVICE_CONNECT)) {
		return strnlen(buf, PAGE_SIZE);
	}
#endif /* INPUT_HOST_DEVICE_CONNECT */

#ifdef INPUT_HOST_DEVICE_DISCONNECT
	if (!force_event(buf, INPUT_HOST_DEVICE_DISCONNECT)) {
		return strnlen(buf, PAGE_SIZE);
	}
#endif /* INPUT_HOST_DEVICE_DISCONNECT */

	printk(KERN_DEBUG PFX " Invalid forced usbmode event \"%s\"\n", buf);
	return strnlen(buf, PAGE_SIZE);
}

DEVICE_ATTR(force_event, S_IWUGO, NULL, dummy_sysfs_force_write);

/* Start usb device detect mode . in this mode we try to detect if we must behave like a usb device */
void device_detect_timeout(unsigned long data_pointer)
{
	usbmode_push_event(INPUT_DEVICE_DETECT_TIMEOUT);
}

/* dummy function */
void dummy_dummy(struct usbmode_data *data)
{
};

int dummy_initial_to_idle(struct usbmode_data *data)
{
	return 0;
};

int dummy_idle_to_device_detect(struct usbmode_data *data)
{
	struct impl_data_dummy *impl =
	    (struct impl_data_dummy *)data->u_impl_data->data;
	/* At this point we assume that initial_to_idle
	 * was the last called function. We will not setup
	 * the required interrupts and system registers in
	 * order to be able to detect if we are to act like a 
	 * device
	 */
	impl->reset_counter = 0;

	printk(KERN_INFO PFX " %s device detect \n", __FUNCTION__);

	return 0;		/* success */
};

int dummy_device_detect_to_idle(struct usbmode_data *data)
{
	return 0;
};

int dummy_device_detect_to_device_wait(struct usbmode_data *data)
{
	return 0;
};

int dummy_device_wait_to_device_idle(struct usbmode_data *data)
{
	return 0;
}

int dummy_device_wait_to_device(struct usbmode_data *data)
{
	return 0;
}

int dummy_device_to_idle(struct usbmode_data *data)
{
	return 0;		/* success */
};

void host_detect_timeout(unsigned long data_pointer)
{
	usbmode_push_event(INPUT_HOST_DETECT_TIMEOUT);
}

int helper_enable_host(void)
{
	return 0;
}

int helper_disable_host(void)
{
	return 0;
}
int dummy_device_detect_to_host_detect(struct usbmode_data *data)
{
	return 0;
};

int dummy_host_detect_to_idle(struct usbmode_data *data)
{
	/* At this point we want to disable the host function
	 * this is done by disabling the interrupts, the pull down
	 * and the host controller
	 */
	return 0;		/* success */
}

int dummy_host_detect_to_cla(struct usbmode_data *data)
{
	/* at this point the best thing would 
	 * be to enable the function controller
	 * and listen to usb-reset signals. This
	 * is because usb mode will end up in this
	 * situation if it is connected to a powered hub
	 * that is not attached to a device
	 */
	return 0;		/* success */
};

int dummy_host_detect_to_host(struct usbmode_data *data)
{
	return 0;		/* success */
};

int dummy_cla_to_idle(struct usbmode_data *data)
{
	return helper_disable_host();
};

int dummy_host_to_idle(struct usbmode_data *data)
{
	return helper_disable_host();
};

int dummy_poll_controller(struct usbmode_data * data)
{
	return 0;
};

struct usbmode_operations dummy_usbmode_ops = {
	.initial_to_idle = dummy_initial_to_idle,
	.idle_to_device_detect = dummy_idle_to_device_detect,
	.device_detect_to_idle = dummy_device_detect_to_idle,
	.device_detect_to_device_wait = dummy_device_detect_to_device_wait,
	.device_wait_to_idle = dummy_device_wait_to_device_idle,
	.device_wait_to_device = dummy_device_wait_to_device,
	.device_to_idle = dummy_device_to_idle,
	.device_detect_to_host_detect = dummy_device_detect_to_host_detect,
	.host_detect_to_idle = dummy_host_detect_to_idle,
	.host_detect_to_cla = dummy_host_detect_to_cla,
	.host_detect_to_host = dummy_host_detect_to_host,
	.cla_to_idle = dummy_cla_to_idle,
	.host_to_idle = dummy_host_to_idle,
	.poll_controller = dummy_poll_controller,
};

static int dummy_usbmode_platform_device_probe(struct platform_device *dev)
{
	int ret;

	printk("***INFO: dummy_usbmode_platform_device_probe\n");

	usbmode_impl_data = kmalloc(sizeof(struct impl_data_usbmode), GFP_KERNEL);
	if (!usbmode_impl_data) 
	{
		printk(KERN_ERR PFX "%s unable to allocated memory for "
			"usbmode_impl_data\n", __func__);
		return -ENOMEM;
	}

	dummy_impl_data = kmalloc(sizeof(struct impl_data_dummy), GFP_KERNEL);

	/* Allocate implementation spec data */
	if (!dummy_impl_data) {
		printk(KERN_INFO PFX
		       " %s unable to allocate memory for impl_data_dummy\n",
		       __FUNCTION__);
		kfree(usbmode_impl_data);
		usbmode_impl_data = NULL;
		return -ENOMEM;
	}

	/* Store the pointer to the data */
	usbmode_impl_data->data = dummy_impl_data;
	usbmode_impl_data->id = 1; /* Must be matched to factory data! */

	/* Register driver, usbmode late init will call init_to_idle transition */
	if ((ret =
	     register_usbmode_driver(&dummy_usbmode_ops,
				     usbmode_impl_data))) {
		printk(KERN_INFO PFX
		       " %s error registering dummy usbmode driver\n",
		       __FUNCTION__);
		kfree(usbmode_impl_data);
		usbmode_impl_data = NULL;
		kfree(dummy_impl_data);
		dummy_impl_data = NULL;
		return ret;
	}

	if (device_create_file(&dev->dev, &dev_attr_force_event)) {
		printk(KERN_ERR PFX
			" unable to create the 'force_event' attribute of the usbmode-impl-dummy driver.\n");
		return 1;
	}

	return 0;		/* success */
}

static int dummy_usbmode_platform_device_remove(struct platform_device *dev)
{
	/* Unregister driver */
	unregister_usbmode_driver();
	kfree(usbmode_impl_data);
	usbmode_impl_data = NULL;
	kfree(dummy_impl_data);
	dummy_impl_data = NULL;
	return 0;		/* success */
}

/*This driver registers a tomtomgo-usbmode */
static struct platform_driver usbmode_impl_dummy = {
	.probe = dummy_usbmode_platform_device_probe,
	.remove = dummy_usbmode_platform_device_remove,
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "usbmode-impl-dummy",
		   },
};

static int __init dummy_usbmode_init_module(void)
{
	int ret;

	ret = platform_driver_register(&usbmode_impl_dummy);
	if (ret) {
		printk(KERN_ERR PFX " unable to register driver (%d)\n", ret);
		return ret;
	}

	printk(KERN_INFO DRIVER_DESC_LONG "(" PFX ")\n");
	return 0;
}

static void __exit dummy_usbmode_exit_module(void)
{
	platform_driver_unregister(&usbmode_impl_dummy);
}

module_init(dummy_usbmode_init_module);
module_exit(dummy_usbmode_exit_module);

MODULE_DESCRIPTION(DRIVER_DESC_LONG);
MODULE_AUTHOR("Martin Jackson (martin.jackson@tomtom.com)");
MODULE_LICENSE("GPL");
