/* drivers/tomtom/usbmode/usbmode.c
Copyright (C) 2007 TomTom BV <http://www.tomtom.com/>
Author: Kees Jongenburger <kees.jongenburger@tomtom.com>
        Balazs Gerofi <balazs.gerofi@tomtom.com>
	Martin Jackson <martin.jackson@tomtom.com>
	Benoit Leffray <benoit.leffray@tomtom.com>

Driver to perform probing usb of modes. This driver has two functionalities.
The first is to be able to detect the current usb mode. The second
is to be able to switch between "idle" , usb device and usb host mode.

sysfs interface
================================================================================
There are several attributes published under the sysfs directory corresponding
to the usbmode platform _device_ kobject. These are as follows:

Name            | Read behaviour                | Write behaviour
----------------+-------------------------------+-------------------------------
mode            | Return current usbmode state  | N/A
----------------+-------------------------------+-------------------------------
ack             | N/A                           | Write "ack": If usbmode in
                |                               | state USB_STATE_DEVICE_WAIT
                |                               | then proceed to
                |                               | USB_STATE_DEVICE, otherwise no
                |                               | change
----------------+-------------------------------+-------------------------------
ack_enable      | List of possible states, with | Write "enable": Enable waiting
                | current state enclosed in []  | on acknowledge
                |                               | Write "disable": disable
                |                               | waiting on acknowledge

If waiting on acknowledge is enabled, After being in USB_STATE_DEVICE_DETECT,
the state USB_STATE_DEVICE_WAIT will be entered on receiving a USB bus reset
event. Otherwise, the state USB_STATE_DEVICE will be entered on receiving a USB
bus reset event.
This feature is to let userland decide whether the usb subsystem is allowed to
go into device mode.

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.
*/

/* Normal kernel includes */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kfifo.h>
#include <linux/spinlock.h>
#include <linux/platform_device.h>

#include <plat/usbmode.h>

/* Defines */
#define PFX "usbmode:"
#define PK_DBG PK_DBG_FUNC

#define DRIVER_DESC_LONG "TomTom GO Legacy USB mode detect, (C) 2007,2008 TomTom BV "
#define ACK_TOKEN "ack"
#define ACK_ENABLE_TOKEN "enable"
#define ACK_DISABLE_TOKEN "disable"

const char *USB_STATE_STRINGS[] = {
	"USB_STATE_INITIAL",
	"USB_STATE_IDLE",
	"USB_STATE_DEVICE_DETECT",
	"USB_STATE_DEVICE_WAIT",
	"USB_STATE_DEVICE",
	"USB_STATE_HOST_DETECT",
	"USB_STATE_HOST",
	"USB_STATE_CLA"
};

const char *USB_EVENT_STRINGS[] = {
	"USB_EVENT_POWER_ON",
	"USB_EVENT_POWER_OFF",
	"USB_EVENT_DEVICE_RESET",
	"USB_EVENT_DEVICE_DETECT_TIMEOUT",
	"USB_EVENT_DEVICE_USERLAND_ACK",
	"USB_EVENT_HOST_DETECT_TIMEOUT",
	"USB_EVENT_HOST_DEVICE_ADDED",
	"USB_EVENT_HOST_DEVICE_REMOVED",
	"USB_EVENT_KERNEL_SUSPEND",
	"USB_EVENT_KERNEL_INIT"
};

static void usbmode_atomic_set_event(struct usbmode_data *a_data,
				     unsigned int a_mask)
{
	unsigned char mask;
	/* kfifo uses a char array
	   for now: cast a_mask to char and verify
	   we don't miss anything (FIXME)*/
	mask=(unsigned char)a_mask;
	if ((unsigned int) mask != a_mask) 
		printk("!!! WARNING: EVENT does not fit in char!!!\n");
	
	if (a_data->input_events == NULL){
		printk("ERROR a_data->input_events == NULL\n");
		return;
	}
	
	if (!kfifo_put(a_data->input_events,&mask,1))
		printk("WARNING: Not enough room in usbmode fifo buffer!\n");
}

/**
 * data member used to keep usbmode status data.
 **/
static struct usbmode_data *st_usbmode_data = NULL;

/**
 * sysfs entry attached to the driver to output the usbmode mode
 **/
static ssize_t usbmode_sysfs_read_mode(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct usbmode_data *data = (struct usbmode_data *)dev->driver_data;
	return sprintf(buf, "%s\n", USB_STATE_STRINGS[data->usb_state]);
}

/* define driver_attr_mode variable this will be used to define the "mode"
entry in the sysfs*/
static DEVICE_ATTR(mode, S_IRUGO, usbmode_sysfs_read_mode, NULL);

/* define an input attribute that we use for enabling device mode */
static ssize_t usbmode_sysfs_ack(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count);
static DEVICE_ATTR(ack, S_IWUGO, NULL, usbmode_sysfs_ack);

/*
 * input attribute that we use for enabling userland acknowledge of device mode
 */
static ssize_t usbmode_sysfs_read_acken(struct device *dev,
					struct device_attribute *attr,
					char *buf);
static ssize_t usbmode_sysfs_write_acken(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count);
static DEVICE_ATTR(ack_enable, S_IWUGO | S_IRUGO, usbmode_sysfs_read_acken,
		   usbmode_sysfs_write_acken);

static void handle_usbmode_state_event(struct usbmode_data *data, USB_EVENT event);

/*==== START change listener data code */
/* data members for the change listener callback */
struct state_change_listener_list_item {
	struct list_head node;
	usb_state_change_listener_func func;
	void *arg;
};
/* create a list for state_change_listeners */
static LIST_HEAD(state_change_listener_list);
/* define a lock to use when performing operations on the change listener list */
DECLARE_MUTEX(state_change_listener_list_lock);

/*==== START state machine code */

/**
 * Method that allows to push an usbevent to the event queue
 * this method can be called from any context since it uses a worker thread to 
 * push the changes
 *
 * This will check of the current status of the event
 **/
int usbmode_push_event(unsigned int event)
{
	if (!st_usbmode_data) {
		printk(KERN_ERR PFX " %s not initialized yet \n",
		       __FUNCTION__);
		return 0;
	}
	// It's not an error, but we just want to show this all the time
	printk(KERN_ERR PFX " EVENT %i\n", event);

	usbmode_atomic_set_event(st_usbmode_data, event);

	schedule_work(&st_usbmode_data->state_changed_work);

	return 1;
}

EXPORT_SYMBOL(usbmode_push_event);

/* Define a worker method used to handle state changes in an asynchronous manner */
static void state_changed_work_handler(struct work_struct *work)
{
	unsigned char event;
	while (kfifo_len(st_usbmode_data->input_events) > 0) {
		printk(KERN_NOTICE PFX " %d in buffer\n",kfifo_len(st_usbmode_data->input_events));
		if (!kfifo_get(st_usbmode_data->input_events,&event,1))
			printk("WARNING: Failed to read from usbmode fifo buffer!\n");
		switch (event) {
			case INPUT_DEVICE_RESET:
				handle_usbmode_state_event(st_usbmode_data,
					USB_EVENT_DEVICE_RESET);
				break;
			case INPUT_DEVICE_DETECT_TIMEOUT:
				handle_usbmode_state_event(st_usbmode_data,
					USB_EVENT_DEVICE_DETECT_TIMEOUT);
				break;
			case INPUT_HOST_DETECT_TIMEOUT:
				handle_usbmode_state_event(st_usbmode_data,
					USB_EVENT_HOST_DETECT_TIMEOUT);
				break;
			case INPUT_HOST_DEVICE_DISCONNECT:
				handle_usbmode_state_event(st_usbmode_data,
					USB_EVENT_HOST_DEVICE_REMOVED);
				break;
			case INPUT_HOST_DEVICE_CONNECT:
				handle_usbmode_state_event(st_usbmode_data,
					USB_EVENT_HOST_DEVICE_ADDED);
				break;
			case INPUT_BUS_POWER_ON_EVENT:
				handle_usbmode_state_event(st_usbmode_data, 
					USB_EVENT_POWER_ON);
				break;
			case INPUT_BUS_POWER_OFF_EVENT:
				handle_usbmode_state_event(st_usbmode_data,
					USB_EVENT_POWER_OFF);
				break;
			case INPUT_DEVICE_USERLAND_ACK:
				handle_usbmode_state_event(st_usbmode_data,
					USB_EVENT_DEVICE_USERLAND_ACK);
				break;
			case INPUT_KERNEL_SUSPEND:
				handle_usbmode_state_event(st_usbmode_data,
					USB_EVENT_KERNEL_SUSPEND);
				break;
			case INPUT_KERNEL_INIT:
				handle_usbmode_state_event(st_usbmode_data,
					USB_EVENT_KERNEL_INIT);
				break;
	/* we expect to at least handle one event every time we are called
	   if this is not the case I guess that something is not ok (given_data
	   corrupt? log a message */
			default:
				printk(KERN_INFO PFX
					" %s called but no event was emitted "
					"state %08x\n", __FUNCTION__, event);
		}
	}
}

/* Handle events */
static void _handle_usbmode_state_event(struct usbmode_data *data, USB_EVENT event)
{
	struct state_change_listener_list_item *item;
	int previous_state;
	int recurse = 0; // do we want to call ourself again

	previous_state = data->usb_state;

	switch (data->usb_state) {
	case USB_STATE_INITIAL:
		switch (event) {
		case USB_EVENT_KERNEL_INIT:
			data->u_ops->initial_to_idle(st_usbmode_data);
			data->usb_state = USB_STATE_IDLE;
			break;
		default:
			break;
		};

		break;
	case USB_STATE_IDLE:
		switch (event) {
		case USB_EVENT_POWER_ON:
			/* if we are in idle mode and get power start detecting if we must behave
			   like a device */
			if (data->u_ops->idle_to_device_detect(data)) {
				break;
			}
			data->usb_state = USB_STATE_DEVICE_DETECT;
			break;
		case USB_EVENT_POWER_OFF:
			/* when power is on we move to at least USB_STATE_DEVICE_DETECT mode
			   so we don't expect to get a USB power off while in this mode.
			   While the above sound reasonable it it not completely true.
			   We have no hard guaranty about the order in with we receive events.
			   therefore I can happen that we first detect that the D signal have stopped
			   and therefore switch to IDLE mode. in that case we could receive a power
			   off event while in idle mode. Still I guess we do not need to do anything here */
			//printk(KERN_DEBUG PFX
			//       " USB power off while in idle mode. (not expected)\n");
			kobject_uevent(&data->dev->kobj, KOBJ_OFFLINE);
			break;
		case USB_EVENT_DEVICE_RESET:
		  	// this can happen during boot without a power on event
			printk(KERN_DEBUG PFX " Device reset in idle mode. (not really expected)\n");
		      	if (data->u_ops->idle_to_device_detect(data)) {
				break;
			}
			data->usb_state = USB_STATE_DEVICE_DETECT;

			// handle the device reset a bit more; never can be sure whether there'll another one
			recurse = 1;

			break;
		case USB_EVENT_DEVICE_DETECT_TIMEOUT:
			/* not expected either */
			printk(KERN_DEBUG PFX
			       " Device detect timeout  in idle mode. (not expected)\n");
			break;
		case USB_EVENT_DEVICE_USERLAND_ACK:
			/* not expected */
			printk(KERN_DEBUG PFX
			       " Device userland acknowledge while in idle mode (not expected)\n");
			break;
		case USB_EVENT_HOST_DETECT_TIMEOUT:
			/* not really expected but I guess it can happen if a different event (power down) puts the
			   state machine in idle mode but the host detect timeout is still running */
			printk(KERN_DEBUG PFX " host detect timeout  in idle mode. (not expected)\n");
			break;
		case USB_EVENT_HOST_DEVICE_ADDED:
			/* don't do anything */
			break;
		case USB_EVENT_HOST_DEVICE_REMOVED:
			/* don't do anything */
			break;
		case USB_EVENT_KERNEL_SUSPEND:
			/* Don't do anything we are already pretty much idle */
			break;
		case USB_EVENT_KERNEL_INIT:
			/* not expected */
			printk (KERN_DEBUG PFX " Init event while in idle mode (not expected)\n");
			break;
		}
		break;
	case USB_STATE_DEVICE_DETECT:
		switch (event) {
		case USB_EVENT_POWER_ON:
			printk(KERN_DEBUG PFX
			       " USB power on while in device detect detect mode. (not expected) \n");
			break;
		case USB_EVENT_POWER_OFF:
			if (data->u_ops->device_detect_to_idle(data)) {
				break;
			}
			data->usb_state = USB_STATE_IDLE;
			break;
		case USB_EVENT_DEVICE_RESET:
			if (atomic_read(&(data->ack_enabled))) {
				printk(KERN_DEBUG PFX
				       " Device reset while in device detect."
				       " Switching to usb device wait mode\n");
				if (data->u_ops->
				    device_detect_to_device_wait(data)) {
					break;
				}
				data->usb_state = USB_STATE_DEVICE_WAIT;
			} else {
				printk(KERN_DEBUG PFX
				       " Device reset while in device detect."
				       " Switching to usb device mode\n");
				if (data->u_ops->
				    device_detect_to_device_wait(data)
				    || data->u_ops->device_wait_to_device(data)) {
					break;
				}
				data->usb_state = USB_STATE_DEVICE;
			}

			break;
		case USB_EVENT_DEVICE_DETECT_TIMEOUT:
			printk(KERN_DEBUG PFX
			       " Device detect timeout  while in device detect. switching to host detect\n");
			if (data->u_ops->device_detect_to_host_detect(data)) {
				break;
			}
			data->usb_state = USB_STATE_HOST_DETECT;
			break;
		case USB_EVENT_DEVICE_USERLAND_ACK:
			/* not expected */
			printk(KERN_DEBUG PFX " Device userland acknowledge while in device detect (not expected)\n");
			break;
		case USB_EVENT_HOST_DETECT_TIMEOUT:
			printk(KERN_DEBUG PFX
			       " Host detect timeout  while in device detect .(not expected)\n");
			break;
		case USB_EVENT_HOST_DEVICE_ADDED:
			/* don't do anything */
			break;
		case USB_EVENT_HOST_DEVICE_REMOVED:
			/* don't do anything */
			break;
		case USB_EVENT_KERNEL_SUSPEND:
			/* upon suspend we want to go back in idle mode to make sure we
			   have no timers running etc */
			if (data->u_ops->device_detect_to_idle(data)) {
				break;
			}
			data->usb_state = USB_STATE_IDLE;
			break;
		case USB_EVENT_KERNEL_INIT:
			/* not expected */
			printk (KERN_DEBUG PFX " Initial event while in device detect (not expected)\n");
			break;
		}
		break;
	case USB_STATE_DEVICE_WAIT:
		switch (event) {
		case USB_EVENT_POWER_ON:
			printk(KERN_DEBUG PFX
			       " USB power on while in usb device wait mode\n");
			break;
		case USB_EVENT_POWER_OFF:
			if (data->u_ops->device_wait_to_idle(data)) {
				break;
			}
			data->usb_state = USB_STATE_IDLE;
			break;
		case USB_EVENT_DEVICE_RESET:
			printk(KERN_DEBUG PFX
			       " Device reset while in device wait mode .(not expected) \n");
			break;
		case USB_EVENT_DEVICE_DETECT_TIMEOUT:
			/* not expected */
			printk(KERN_DEBUG PFX
			       " Device detect timeout while in device wait mode .(not expected) \n");
			break;
		case USB_EVENT_DEVICE_USERLAND_ACK:
			if (data->u_ops->device_wait_to_device(data)) {
				break;
			}
			data->usb_state = USB_STATE_DEVICE;
			break;
		case USB_EVENT_HOST_DETECT_TIMEOUT:
			/* not expected */
			printk(KERN_DEBUG PFX
			       " host detect timeout while in device wait mode .(not expected) \n");
			break;
		case USB_EVENT_HOST_DEVICE_ADDED:
			/* don't to anything */
			break;
		case USB_EVENT_HOST_DEVICE_REMOVED:
			/* don't to anything */
			break;
		case USB_EVENT_KERNEL_SUSPEND:
			/* Don't do anything , we are perhaps rebooting to connect to the computer
			   perhaps we will need to disable interrupts at some point? */
			break;
		case USB_EVENT_KERNEL_INIT:
			/* not expected */
			printk (KERN_DEBUG PFX " Initial event while in device wait mode (not expected)\n");
			break;

		}
		break;

	case USB_STATE_DEVICE:
		switch (event) {
		case USB_EVENT_POWER_ON:
			printk(KERN_DEBUG PFX
			       " USB power on while in device mode\n");
			break;
		case USB_EVENT_POWER_OFF:
			if (data->u_ops->device_to_idle(data)) {
				break;
			}
			data->usb_state = USB_STATE_IDLE;
			break;
		case USB_EVENT_DEVICE_RESET:
			printk(KERN_DEBUG PFX
			       " Device reset while in device mode .(not expected) \n");
			break;
		case USB_EVENT_DEVICE_DETECT_TIMEOUT:
			/* not expected */
			printk(KERN_DEBUG PFX
			       " Device detect timeout while in device mode .(not expected) \n");
			break;
		case USB_EVENT_DEVICE_USERLAND_ACK:
			/* not expected */
			printk(KERN_DEBUG PFX
			       " Device userland acknowledge while in device mode (not expected)\n");
			break;
		case USB_EVENT_HOST_DETECT_TIMEOUT:
			/* not expected */
			printk(KERN_DEBUG PFX
			       " host detect timeout while in device mode .(not expected) \n");
			break;
		case USB_EVENT_HOST_DEVICE_ADDED:
			/* don't to anything */
			break;
		case USB_EVENT_HOST_DEVICE_REMOVED:
			/* don't to anything */
			break;
		case USB_EVENT_KERNEL_SUSPEND:
			/* Don't do anything , we are perhaps rebooting to connect to the computer
			   perhaps we will need to disable interrupts at some point? */
			break;
		case USB_EVENT_KERNEL_INIT:
			/* not expected */
			printk (KERN_DEBUG PFX " Initial event while in device mode (not expected)\n");
			break;
		}
		break;

	case USB_STATE_HOST_DETECT:
		switch (event) {
		case USB_EVENT_POWER_ON:
			printk(KERN_DEBUG PFX
			       " USB power on while in  usb host detect mode .(not expected) \n");
			break;
		case USB_EVENT_POWER_OFF:
			if (data->u_ops->host_detect_to_idle(data)) {
				break;
			}
			data->usb_state = USB_STATE_IDLE;
			break;
		case USB_EVENT_DEVICE_RESET:
			printk(KERN_DEBUG PFX
			       " Device reset  while in host detect mode. (not expected)\n");
			break;
		case USB_EVENT_DEVICE_DETECT_TIMEOUT:
			/* not expected */
			printk(KERN_DEBUG PFX
			       " Device detect timeout  while in host detect mode. (not expected) \n");
			break;
		case USB_EVENT_DEVICE_USERLAND_ACK:
			/* not expected */
			printk(KERN_DEBUG PFX
			       " Device userland acknowledge while in host detect mode (not expected)\n");
			break;
		case USB_EVENT_HOST_DETECT_TIMEOUT:
			printk(KERN_DEBUG PFX
			       " host detect timeout while in host detect mode, we can assume that we are in CLA mode\n");
			if (data->u_ops->host_detect_to_cla(data)) {
				break;
			}
			data->usb_state = USB_STATE_CLA;
			break;
		case USB_EVENT_HOST_DEVICE_ADDED:
			/* go in usb host mode */
			printk(KERN_DEBUG PFX
			       " hotplug while in host detect mode,switching to usb host\n");
			if (data->u_ops->host_detect_to_host(data)) {
				break;
			}
			data->usb_state = USB_STATE_HOST;
			break;
		case USB_EVENT_HOST_DEVICE_REMOVED:
			/* don't to anything */
			break;
		case USB_EVENT_KERNEL_SUSPEND:
			if (data->u_ops->host_detect_to_idle(data)) {
				break;
			}
			data->usb_state = USB_STATE_IDLE;
			break;
		case USB_EVENT_KERNEL_INIT:
			/* not expected */
			printk (KERN_DEBUG PFX " Initial event while in host detect mode (not expected)\n");
			break;
		}
		break;
	case USB_STATE_HOST:
		switch (event) {
		case USB_EVENT_POWER_ON:
			printk(KERN_DEBUG PFX " USB power on while in usb host mode.\n");
			break;
		case USB_EVENT_POWER_OFF:
			/* TODO: if we are in host mode and we get a power off we do not want
			   to switch to an other mode since we only want to exit host mode if there
			   are no devices any more. I guess we need to look if there are devices left
			   on the bus */
			if (data->u_ops->host_detect_to_idle(data)) {
				break;
			}
			data->usb_state = USB_STATE_IDLE;
			break;
		case USB_EVENT_DEVICE_RESET:
			printk(KERN_DEBUG PFX
			       " Device reset  while in host mode. (not expected)\n");
			break;
		case USB_EVENT_DEVICE_DETECT_TIMEOUT:
			printk(KERN_DEBUG PFX
			       " Device detect timeout while in host mode. (not expected) \n");
			break;
		case USB_EVENT_DEVICE_USERLAND_ACK:
			/* not expected */
			printk(KERN_DEBUG PFX
			       " Device userland acknowledge while in host mode (not expected)\n");
			break;
		case USB_EVENT_HOST_DETECT_TIMEOUT:
			printk(KERN_DEBUG PFX
			       " host detect timeout while in host mode. (not expected)\n");
			break;
		case USB_EVENT_HOST_DEVICE_ADDED:
			/* TODO: expand hotplug kind of stuff to detect when the last device gets removed */
			printk(KERN_DEBUG PFX
			       " hotplug while in host mode TODO\n");
			break;
		case USB_EVENT_HOST_DEVICE_REMOVED:
			if (data->u_ops->host_to_idle(data)) {
				break;
			}
			data->usb_state = USB_STATE_IDLE;
			break;
		case USB_EVENT_KERNEL_SUSPEND:
			if (data->u_ops->host_to_idle(data)) {
				break;
			}
			data->usb_state = USB_STATE_IDLE;
			break;
		case USB_EVENT_KERNEL_INIT:
			/* not expected */
			printk (KERN_DEBUG PFX " Initial event while in host mode (not expected)\n");
			break;
		}
		break;
	case USB_STATE_CLA:
		switch (event) {
		case USB_EVENT_POWER_ON:
			printk(KERN_DEBUG PFX
			       " USB power on while in cla mode .(not expected) \n");
			break;
		case USB_EVENT_POWER_OFF:
			printk(KERN_DEBUG PFX
			       " Power off  while in cla mode. switching to idle mode\n");
			if (data->u_ops->cla_to_idle(data)) {
				break;
			}
			data->usb_state = USB_STATE_IDLE;
			break;
		case USB_EVENT_DEVICE_RESET:
			printk(KERN_DEBUG PFX
			       " Device reset while in cla mode. switching to idle mode\n");
			if (data->u_ops->cla_to_idle(data)) {
				break;
			}
			data->usb_state = USB_STATE_IDLE;

			// immediately proceed to handle this; you never know whether there'll be more reset events
			recurse = 1;

			break;
		case USB_EVENT_DEVICE_DETECT_TIMEOUT:
			printk(KERN_DEBUG PFX
			       " Device detect timeout while in cla mode. (not expected) \n");
			break;
		case USB_EVENT_DEVICE_USERLAND_ACK:
			/* not expected */
			printk(KERN_DEBUG PFX
			       " Device userland acknowledge while in cla mode (not expected)\n");
			break;
		case USB_EVENT_HOST_DETECT_TIMEOUT:
			printk(KERN_DEBUG PFX
			       " host detect timeout while in cla mode. (not expected)\n");
			break;
		case USB_EVENT_HOST_DEVICE_ADDED:
			/*TODO: switch to host mode? */
			printk(KERN_DEBUG PFX
			       " hotplug while in cla mode. (not expected)\n");
			break;
		case USB_EVENT_HOST_DEVICE_REMOVED:
			/*TODO: switch to idle mode? */
			printk(KERN_DEBUG PFX
			       " hotplug while in cla mode. (not expected)\n");
			break;
		case USB_EVENT_KERNEL_SUSPEND:
			if (data->u_ops->cla_to_idle(data)) {
				break;
			}
			data->usb_state = USB_STATE_IDLE;
			break; 
		case USB_EVENT_KERNEL_INIT:
			/* not expected */
			printk (KERN_DEBUG PFX " Initial event while in cla mode (not expected)\n");
			break;
		}
		break;

	};

	if (previous_state != data->usb_state) {
		/* we have performed a state change */
		printk(KERN_ERR PFX "[%i,%i,%i] %s ==( %s )==> %s\n",
		       previous_state,
		       event,
		       data->usb_state,
		       USB_STATE_STRINGS[previous_state],
		       USB_EVENT_STRINGS[event],
		       USB_STATE_STRINGS[data->usb_state]);

		/* Then call all the registered poll functions. */
		down(&state_change_listener_list_lock);
		list_for_each_entry(item, &state_change_listener_list, node) {
			item->func(previous_state, data->usb_state, item->arg);
		}
		up(&state_change_listener_list_lock);

		/* notify that the usb mode has changed: */
		kobject_uevent(&data->dev->kobj, KOBJ_CHANGE);
		/* send ONLINE for STATE_DEVICE and OFFLINE for STATE_IDLE */
		/* FIXME: Would be cleaner to put these into the g_file_storage */
		if (data->usb_state == USB_STATE_DEVICE) kobject_uevent(&data->dev->kobj, KOBJ_ONLINE);
		else if (data->usb_state == USB_STATE_IDLE) kobject_uevent(&data->dev->kobj, KOBJ_OFFLINE);
		else if (data->usb_state == USB_STATE_HOST_DETECT) kobject_uevent(&data->dev->kobj, KOBJ_OFFLINE);
	} else {
		/* if we are here the event did not result is a state change so output this
		   information in debug mode so we at least know the event did not result
		   in a change */
		printk(KERN_DEBUG PFX "[%i,%i] %s==(%s) NOP\n",
		       previous_state,
		       event,
		       USB_STATE_STRINGS[previous_state],
		       USB_EVENT_STRINGS[event]);
	}

	if (recurse) {
	  // we don't want to recurse indefinitely; the limit is arbitrary
	  static int count = 0;
	  WARN_ON(count++ > 3);
	  printk(KERN_INFO PFX " Recursing... %d\n", count);

	  // it seems bad to call the SAME code again but there might be good reasons unknown to me at this moment
	  WARN_ON( previous_state == data->usb_state );

    _handle_usbmode_state_event(data, event);

    count--;
	}
};

static void handle_usbmode_state_event(struct usbmode_data *data, USB_EVENT event)
{
  if (down_interruptible(&data->usb_state_mutex))
    return;
  _handle_usbmode_state_event(data, event);
  up(&data->usb_state_mutex);
};

/*==== START state change listener  implementation */
int add_usb_state_change_listener(usb_state_change_listener_func function,
				  void *arg, USB_STATE * current_state)
{
	struct state_change_listener_list_item *item;

	item =
	    kmalloc(sizeof(struct state_change_listener_list_item), GFP_KERNEL);
	if (item == NULL) {
		printk(KERN_INFO PFX
		       " %s Unable to allocate state_change_listener_list_item node\n",
		       __FUNCTION__);
		return -ENOMEM;
	}

	/* Quite a bit of overhead for a simple pointer... */
	item->func = function;
	item->arg = arg;

	down(&state_change_listener_list_lock);
	list_add_tail(&item->node, &state_change_listener_list);
	if (current_state != NULL)
		*current_state = st_usbmode_data->usb_state;
	up(&state_change_listener_list_lock);

	return 0;
}

EXPORT_SYMBOL(add_usb_state_change_listener);

int remove_usb_state_change_listener(usb_state_change_listener_func function,
				     void *arg)
{
	struct state_change_listener_list_item *item;

	down(&state_change_listener_list_lock);
	list_for_each_entry(item, &state_change_listener_list, node) {
		if (item->func == function) {
			list_del(&item->node);
			kfree(item);
			up(&state_change_listener_list_lock);
			return 0;
		}
	}
	up(&state_change_listener_list_lock);

	printk(KERN_INFO PFX
	       " %s Unable to find the state change listener for function %p to remove \n",
	       __FUNCTION__, function);
	return -EINVAL;
}

EXPORT_SYMBOL(remove_usb_state_change_listener);

/* Register usbmode implementation and call init to idle transition */
int register_usbmode_driver(struct usbmode_operations *u_ops,
			    struct impl_data_usbmode *u_impl_data)
{
	struct platform_device *pdev;
	printk(KERN_DEBUG PFX " %s\n", __FUNCTION__);

	/* Is the driver initialized */
	if (!st_usbmode_data) {
		printk(KERN_ERR PFX " Could not register impl, I am not "
		       "initialized yet.\n");
		return -ENODEV;
	}

	/* Is the modprobe called */
	if (!st_usbmode_data->dev) {
		printk(KERN_ERR PFX " Could not register impl, I have not "
		       "been probed yet.\n");
		return -ENODEV;
	}

	/* Is there a  platform_device struct */
	pdev = to_platform_device(st_usbmode_data->dev);
	if (!pdev) {
		printk(KERN_ERR PFX " Could not register impl, I do not "
		       "have a platform_device.\n");
		return -EINVAL;
	}

	/* Is the id of the controller correct */
	if (pdev->id != u_impl_data->id) {
		printk(KERN_INFO PFX " Could not register impl, impl has "
		       "incorrect id\n");
		return -EINVAL;
	}

	/* Is there a driver already using the state machine? */
	if (st_usbmode_data->u_ops || st_usbmode_data->u_impl_data) {
		printk(KERN_INFO PFX " USBmode driver already registered! \n");
		return -EBUSY;
	}

	/* Otherwise register and call init */
	st_usbmode_data->u_ops = u_ops;
	st_usbmode_data->u_impl_data = u_impl_data;

	/* Switch from initial mode to idle */
	usbmode_push_event(INPUT_KERNEL_INIT);

	/* Regenerate any events we might have missed earlier e.g. power on */
	if (st_usbmode_data->u_ops->poll_controller(st_usbmode_data)) {
		printk(KERN_INFO PFX " Poll of usb controller failed.\n");
		return -1;
	}

	printk(KERN_DEBUG PFX " %s done\n", __FUNCTION__);

	return 0;
}

EXPORT_SYMBOL(register_usbmode_driver);

/* Unregister usbmode driver, note: the actual implementation has to do its cleanup by itself */
void unregister_usbmode_driver(void)
{
	struct state_change_listener_list_item *item;
	USB_STATE previous_state;

	printk(KERN_DEBUG PFX " %s\n", __FUNCTION__);

	if (down_interruptible(&st_usbmode_data->usb_state_mutex)) {
		printk (KERN_ERR "Unable to take mutex in usbmode ...\n");
		return;
	}

	previous_state = st_usbmode_data->usb_state;

	st_usbmode_data->u_ops = NULL;
	st_usbmode_data->u_impl_data = NULL;
	st_usbmode_data->usb_state = USB_STATE_INITIAL;

	/* Then call all the registered poll functions. */
	down(&state_change_listener_list_lock);
	list_for_each_entry(item, &state_change_listener_list, node) {
		item->func(previous_state, st_usbmode_data->usb_state,
			   item->arg);
	}
	up(&state_change_listener_list_lock);

	up(&st_usbmode_data->usb_state_mutex);

	/* Notify that the usb mode has changed */
	kobject_uevent(&st_usbmode_data->dev->kobj, KOBJ_CHANGE);
	kobject_uevent(&st_usbmode_data->dev->kobj, KOBJ_OFFLINE);
}

EXPORT_SYMBOL(unregister_usbmode_driver);

/*==== START usbmode module code */

/* probe method for the usbmode device driver */
static int usbmode_probe(struct platform_device *pdev)
{

	/* make usbmode_data the driver_data */
	dev_set_drvdata(&pdev->dev, st_usbmode_data);

	INIT_WORK(&st_usbmode_data->state_changed_work,
		  state_changed_work_handler);

	/* and assign the dev back to the structure */
	st_usbmode_data->dev = &pdev->dev;

	if (device_create_file(&pdev->dev, &dev_attr_mode)) {
		printk(KERN_ERR PFX
		       " unable to create the 'mode' attribute of the tomtom-usbmode driver.\n");
		return 1;
	}

	if (device_create_file(&pdev->dev, &dev_attr_ack)) {
		printk(KERN_ERR PFX
		       " unable to create the 'ack' attribute of the tomtom-usbmode driver.\n");
		return 1;
	}

	if (device_create_file(&pdev->dev, &dev_attr_ack_enable)) {
		printk(KERN_ERR PFX
		       " unable to create the 'ack_enable' attribute of the tomtom-usbmode driver.\n");
		return 1;
	}

	return 0;		/* success */
};

/**
 * usbmode driver remove method
 **/
static int usbmode_remove(struct platform_device *pdev)
{
	if (st_usbmode_data->u_ops) {
		printk(KERN_INFO PFX
		       " USBmode still has a driver registered!\n");
		return EBUSY;
	}

	device_remove_file(&pdev->dev, &dev_attr_mode);
	device_remove_file(&pdev->dev, &dev_attr_ack);
	device_remove_file(&pdev->dev, &dev_attr_ack_enable);

	return 0;
};

static int usbmode_suspend(struct platform_device *dev, pm_message_t state)
{
	printk(KERN_DEBUG PFX " %s power down \n", __FUNCTION__);
	usbmode_push_event(INPUT_KERNEL_SUSPEND);

	return 0;
}

static int usbmode_resume(struct platform_device *dev)
{
	printk(KERN_DEBUG PFX " %s power on \n", __FUNCTION__);

	if (down_interruptible(&st_usbmode_data->usb_state_mutex))
		return -ERESTARTSYS;

	st_usbmode_data->usb_state = USB_STATE_INITIAL;
	/*TODO: check whether the following is necessary once
	  suspend/resume is working */
	kfifo_reset(st_usbmode_data->input_events);

	
	usbmode_push_event(INPUT_KERNEL_INIT);

	up(&st_usbmode_data->usb_state_mutex);

//	These codes will be done during low-level usbmode resume
//	/* Regenerate any events we might have missed earlier e.g. power on */
//	if (st_usbmode_data->u_ops->poll_controller(st_usbmode_data)) {
//		printk(KERN_INFO PFX " Poll of usb controller failed.\n");
//		return -1;
//	}

	printk(KERN_DEBUG PFX " %s done\n", __FUNCTION__);

	return 0;
}

/*
 * We need to receive an acknowledge signal from userland before we know that it
 * is OK to enter device mode. This is realized by writing the string defined by
 * ACK_TOKEN to the sysfs node named "ack" in the directory of our usbmode
 * driver kobject.
 */
static ssize_t usbmode_sysfs_ack(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t count)
{
	if (!strncmp(buf, ACK_TOKEN, strlen(ACK_TOKEN))) {
		printk(PFX " ack received\n");
		usbmode_push_event(INPUT_DEVICE_USERLAND_ACK);
	}
	return strnlen(buf, PAGE_SIZE);
}

static ssize_t usbmode_sysfs_read_acken(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	atomic_t *p_ack_enabled =
	    &(((struct usbmode_data *)dev->driver_data)->ack_enabled);

	if (atomic_read(p_ack_enabled)) {
		strcpy(buf, "[" ACK_ENABLE_TOKEN "] " ACK_DISABLE_TOKEN "\n");
	} else {
		strcpy(buf, ACK_ENABLE_TOKEN " [" ACK_DISABLE_TOKEN "]\n");
	}
	return strnlen(buf, PAGE_SIZE);
}

static ssize_t usbmode_sysfs_write_acken(struct device *dev,
					 struct device_attribute *attr,
					 const char *buf, size_t count)
{
	atomic_t *p_ack_enabled =
	    &(((struct usbmode_data *)dev->driver_data)->ack_enabled);

	if (!strncmp(buf, ACK_ENABLE_TOKEN, strlen(ACK_ENABLE_TOKEN))) {
		printk(PFX " ack enabled\n");
		atomic_set(p_ack_enabled, 1);
	}
	if (!strncmp(buf, ACK_DISABLE_TOKEN, strlen(ACK_DISABLE_TOKEN))) {
		printk(PFX " ack disabled\n");
		atomic_set(p_ack_enabled, 0);
	}
	return strnlen(buf, PAGE_SIZE);
}

/*This driver registers a tomtomgo-usbmode */
static struct platform_driver usbmode_driver = {
	.probe = usbmode_probe,
	.remove = __exit_p(usbmode_remove),
	//.suspend = usbmode_suspend,
	//.resume = usbmode_resume,
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "tomtom-usbmode",
		   },
};

/**
 * usbmode module initialization routine for the linux kernel.
 * This method registers a bus driver called tomtomgo-usbmode.
 **/
static int __init usbmode_init_module(void)
{
	int ret;

	/* display driver information when the driver is started */
	printk(KERN_INFO DRIVER_DESC_LONG "(" PFX ")\n");

	/* this driver registers itself as driver to get the different changes in
	   software states */
	if (!
	    (st_usbmode_data =
	     kmalloc(sizeof(struct usbmode_data), GFP_KERNEL))) {
		/*make sure we don't have dangling pointers */
		printk(KERN_ERR PFX
		       " failed to allocate memory for the usbmode_data struct \n");
		return -ENOMEM;
	}

	/* clear the data and initialize */
	memset(st_usbmode_data, 0, sizeof(struct usbmode_data));

	init_MUTEX(&st_usbmode_data->usb_state_mutex);
	st_usbmode_data->usb_state = USB_STATE_INITIAL;
	spin_lock_init(&st_usbmode_data->input_events_lock);
	st_usbmode_data->input_events = kfifo_alloc(16, GFP_KERNEL,
		&st_usbmode_data->input_events_lock);

	printk(KERN_DEBUG PFX " Initialized\n");

	ret = platform_driver_register(&usbmode_driver);
	if (ret) {
		printk(KERN_ERR PFX " unable to register driver (%d)\n", ret);
		return ret;
	}

	return 0;
};

static void __exit usbmode_exit_module(void)
{
	kfifo_free(st_usbmode_data->input_events);
	platform_driver_unregister(&usbmode_driver);
	kfree(st_usbmode_data);
	printk(KERN_INFO PFX " exit\n");
};

subsys_initcall(usbmode_init_module);

module_exit(usbmode_exit_module);

MODULE_DESCRIPTION(DRIVER_DESC_LONG);
MODULE_AUTHOR("Kees Jongenburger " "(kees.jongenburger@tomtom.com)" ","
	      "Balazs Gerofi " "(balazs.gerofi@tomtom.com)" ","
	      "Martin Jackson " "(martin.jackson@tomtom.com)" ","
	      "Benoit Leffray " "(benoit.leffray@tomtom.com)");
MODULE_LICENSE("GPL");
