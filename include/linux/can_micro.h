/*
 * include/linux/can_micro.h
 *
 * Public interface for the Can processor Micro driver.
 *
 * Copyright (C) 2007, 2010 TomTom BV <http://www.tomtom.com/>
 * Author: Peter Wouters <peter.wouters@tomtom.com>
 *         Martin Jackson <martin.jackson@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __INCLUDE_PLAT_CAN_MICRO_H
#define __INCLUDE_PLAT_CAN_MICRO_H

#include <linux/ioctl.h>

#ifdef __KERNEL__
# include <linux/spinlock.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define MICRO_DEVNAME		"micro"
#define MICRO_DRIVER_MAGIC	    'M'	/* micro driver magic number */
#define MICRO_MINOR_OFFSET	     0
#define MICRO_MAX_DEVS		     2	/* Arbitrary maximum number of micro devices */

/* Output pins */
#define IO_nCAN_RST		_IOW(MICRO_DRIVER_MAGIC, 1, int)	/* LOW: Resets the CAN processor */
#define IO_CAN_SYNC		_IOW(MICRO_DRIVER_MAGIC, 2, int)	/* High-Low: Sync pulse (XXX remove argument) */
#define IO_CAN_BT_MD		_IOW(MICRO_DRIVER_MAGIC, 3, int)	/* CAN processor Boot Mode (Low=normal mode; High=SW loading mode) */
#define IO_PWR_5V_ON_MCU	_IOW(MICRO_DRIVER_MAGIC, 4, int)	/* High: Enable 5V power to CAN processor */
#define IO_MCU_nPWR_ON		_IOW(MICRO_DRIVER_MAGIC, 5, int)	/* Low: Enable 3V3 power to CAN processor */

/* Input pins */
#define IO_CAN_nRST		_IOR(MICRO_DRIVER_MAGIC, 10, int)	/* LOW: CAN processor has been reset */
#define IO_CAN_RES		_IOR(MICRO_DRIVER_MAGIC, 11, int)	/* Spare pin */


#ifdef __KERNEL__
struct can_micro_dev {
	/* Mandatory pins */
	int gpio_pin_can_resetin;
	int gpio_pin_can_resetout;
	int gpio_pin_can_bt_mode;
	int gpio_pin_can_sync;

	/* Optional pins (set to -1 if not present) */
	int gpio_pin_can_reserv;
	int gpio_pin_mcu_pwr_on;
	int gpio_pin_pwr_5v_on_mcu;

	/* Data to be filled in on device probe */
	struct cdev *cdev;
	spinlock_t lock;
	unsigned count;
	int irq;
	struct fasync_struct *fasync_listeners;
};
#endif

#ifdef __cplusplus
}
#endif

#endif /* __INCLUDE_PLAT_CAN_MICRO_H */

/* EOF */ 

