/* include/barcelona/usbmode.h

Copyright (C) 2007,2008 TomTom BV <http://www.tomtom.com/>
Author: Kees Jongenburger <kees.jongenburger@tomtom.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.
*/
#ifndef __INCLUDE_TOMTOM_USBMODE_H
#define __INCLUDE_TOMTOM_USBMODE_H

#include <linux/timer.h>	/* timer_list */
#include <linux/interrupt.h>
#include <linux/notifier.h>
#include <linux/workqueue.h>


/**
 * device usb mode states
 * 
 * USB_STATE_IDLE: This is the initial mode, the device is waiting for events  to
 * start detecting.
 * 
 * USB_STATE_DEVICE_DETECT: In this mode the module is trying to determine if the
 * device is connected to a host and must start behaving like a usb device.
 * (currently device mode is only supported in the bootloader).
 * 
 * USB_STATE_DEVICE: In this mode the device is acting like an usb mass storage
 * device. This mode is not implemented by the kernel.
 * 
 * USB_STATE_HOSTDETECT: In this mode the module is trying to determine if the
 * device is supposed to behave like a usb host.
 * 
 * USBMODE_HOST: In this mode the device is acting like an usb host.
 * 
 * USBMODE_CLA: In this mode usually no communication is performed on the data level.
 **/
typedef enum {
	USB_STATE_INITIAL,
	USB_STATE_IDLE,
	USB_STATE_DEVICE_DETECT,
	USB_STATE_DEVICE_WAIT,
	USB_STATE_DEVICE,
	USB_STATE_HOST_DETECT,
	USB_STATE_HOST,
	USB_STATE_CLA
} USB_STATE;

/* Strings representing the use mode state (defined in usbmode.c) */
extern const char *USB_STATE_STRINGS[];

typedef enum {
	USB_EVENT_POWER_ON,
	USB_EVENT_POWER_OFF,
	USB_EVENT_DEVICE_RESET,
	USB_EVENT_DEVICE_DETECT_TIMEOUT,
	USB_EVENT_DEVICE_USERLAND_ACK,
	USB_EVENT_HOST_DETECT_TIMEOUT,
	USB_EVENT_HOST_DEVICE_ADDED,
	USB_EVENT_HOST_DEVICE_REMOVED,
	USB_EVENT_KERNEL_SUSPEND,
	USB_EVENT_KERNEL_INIT,
} USB_EVENT;

/* Types Definition used to secure the access to atomic data. 
 * This type is meant to be accessed through the corresponding API */
typedef struct { volatile unsigned int event; } input_event_t;

/* Strings representing the use mode state */
extern const char *USB_EVENT_STRINGS[];

/**
 * To determine the right state the usbmode subsystem needs to know a little
 * about the world around. based on preliminary research it looks like the
 * required inputs are
 * 
 * INPUT_BUS_POWER: This is a gpio that shows if there is 5 volt on the bus, this
 * is called VBUS in the rest of the code.
 * 
 * INPUT_DEVICE_RESET: When we are in device mode we expect to receive a reset
 * request from the host we are attached to. This is a signal picked up by the
 * hardware usb device and that we can receive via an interrupt.
 * 
 * INPUT_HOST_DEVICE_DETECT: When we are in host mode we want to know if we are
 * attached to a device. In some situations (like with an usb car lighter adapter)
 * we want to override the power restrictions imposed by the usb specs in order
 * to allow a  device to charge via the usb(max 2A?)
 * While not strictly related to usb mode we must make sure that we do not
 * overload the usb system of a computer :p
 * 
 * INPUT_HOST_DEVICE_DISCONNECT: For prague it can happen that we lose INPUT_BUS_POWER
 * but still keep connected , the INPUT_HOST_DISCONNECT
 **/

#define INPUT_BUS_POWER_ON_EVENT     1
#define INPUT_BUS_POWER_OFF_EVENT    2
#define INPUT_DEVICE_RESET           3
#define INPUT_DEVICE_DETECT_TIMEOUT  4
#define	INPUT_DEVICE_USERLAND_ACK    5
/* HOST == when we are host */
#define INPUT_HOST_DETECT_TIMEOUT    6
#define INPUT_HOST_DEVICE_CONNECT    7
#define INPUT_HOST_DEVICE_DISCONNECT 8
/* Kernel-related events */
#define INPUT_KERNEL_SUSPEND         9
#define INPUT_KERNEL_INIT	    10

struct usbmode_data;

/* USB mode operations that can vary through different devices */
struct usbmode_operations {
	int (*initial_to_idle) (struct usbmode_data * data);

	/* device detect related modes changes */
	int (*idle_to_device_detect) (struct usbmode_data * data);
	int (*device_detect_to_idle) (struct usbmode_data * data);

	/* device wait related mode changes */
	int (*device_detect_to_device_wait) (struct usbmode_data * data);
	int (*device_wait_to_idle) (struct usbmode_data * data);

	/* device related mode changes */
	int (*device_wait_to_device) (struct usbmode_data * data);
	int (*device_to_idle) (struct usbmode_data * data);

	/* host detect related mode changes */
	int (*device_detect_to_host_detect) (struct usbmode_data * data);
	int (*host_detect_to_idle) (struct usbmode_data * data);

	/* cla related mode changes */
	int (*host_detect_to_cla) (struct usbmode_data * data);
	int (*host_detect_to_host) (struct usbmode_data * data);
	int (*cla_to_idle) (struct usbmode_data * data);
	int (*host_to_idle) (struct usbmode_data * data);

	/* request that controller re-examines its state and regenerate events
	(e.g. if bus power is on, generate bus power on event) */
	int (*poll_controller) (struct usbmode_data * data);
};

struct impl_data_usbmode {
    u32     id;
    void *  data;
};

int register_usbmode_driver(struct usbmode_operations *u_ops,
			    struct impl_data_usbmode  *u_impl_data);
void unregister_usbmode_driver(void);

/* Structure for the usb mode data */
struct usbmode_data {

	/*Pointer to the device we are representing */
	struct device *dev;

	/* data member showing the usb mode state */
	USB_STATE usb_state;
	struct semaphore usb_state_mutex;

	/* data for the usbmode implementation */
	struct impl_data_usbmode *u_impl_data;

	/* usbmode operations (hardver specific implementations) */
	struct usbmode_operations *u_ops;

	/* the current input event (this is a combination of INPUT_BUS_POWER ,
	   INPUT_DEVICE_RESET etc..)  */
	struct kfifo *input_events;
	spinlock_t    input_events_lock;

	struct notifier_block buspower_change_listener;

	/**
	 * The kernel work queue used in order to minimize the impact of
	 * usb state changes to responsiveness of the kernel
	 **/
	struct work_struct state_changed_work;

	/* Shows whether an acknowledgement from userland is required to enter
	device mode */
	atomic_t ack_enabled;
};


/*define the function prototype a usb state change listener must implement */
typedef void (*usb_state_change_listener_func) (USB_STATE previous_usb_state,
						USB_STATE current_usb_state,
						void *arg);

/**
 * Register the change listener to receive state changes, arg is given back as parameter to the
 * change listener. the callback will be called withing a kernel thread context (sleeping should be ok)
 * Param current_state will be filled with the current state.
 * return 0 on success errno on error
 **/
extern int add_usb_state_change_listener(usb_state_change_listener_func
					 function, void *arg, USB_STATE *current_state );

extern int remove_usb_state_change_listener(usb_state_change_listener_func
					    function, void *arg);

/**
 * Method tha allows to push an usbevent to the event queue
 * this method can be called from any context since it uses a worker thread to 
 * push the changes
 **/
extern int usbmode_push_event(unsigned int event);

#endif				/* __INCLUDE_TOMTOM_USBMODE_H */
