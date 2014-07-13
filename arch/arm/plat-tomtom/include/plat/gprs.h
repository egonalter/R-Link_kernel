/* include/asm-arm/plat-tomtom/gprs.h
 *
 * GPRS driver 
 *
 * Copyright (C) 2008 TomTom BV <http://www.tomtom.com/>
 * Author: Rogier Stam <rogier.stam@tomtom.com>
 *         Niels Langendorff <niels.langendorff@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __GPRS_H
#define __GPRS_H __FILE__

#include <linux/platform_device.h>
#include <linux/kobject.h>
#include <linux/string.h>

struct gprs_platform_data
{
	/* Pin definition. */
	int (*gprs_power)( int ena_dis );
	int (*gprs_reset)( int ena_dis );
	void (*gprs_suspend)( void );
	void (*gprs_setup_port)(void);
	unsigned int	gprs_id;
};

#define GPRS_DEVNAME			"gprs"

typedef enum
{
	GPRS_STATUS_OFF=0,
	GPRS_STATUS_ON,
	GPRS_STATUS_SUSPEND,
} gprs_state_e;

struct gprs_data;

/* GPRS operations */
struct gprs_operations {
	int (*gprs_reset) (struct gprs_data * data);
	int (*gprs_on) (struct gprs_data * data);
	int (*gprs_off) (struct gprs_data * data);
	int (*gprs_probe) (struct gprs_data *data);
	int (*gprs_remove) (struct gprs_data *data);
	gprs_state_e (*gprs_state) (struct gprs_data * data);
#ifdef CONFIG_PM
	int (*gprs_suspend) (struct gprs_data * data);
#endif
};

int register_gprs_driver(struct gprs_operations *u_ops, char * name, unsigned int gprs_id);
void unregister_gprs_driver(void);

/* Structure for the gprs data */
struct gprs_data {

	/* Pointer to the device we are representing */
	struct device *dev;

	/* gprs platform datas */
	struct gprs_platform_data	*pd;

	/* gprs operations (hardware specific implementations) */
	struct gprs_operations *u_ops;

	/* gprs modem name */
	char*	name;

	/* gprs private data */
	void*	private_data; 
};

#define gprs_get_drvdata(gprs_data) ((gprs_data)->private_data)
#define gprs_set_drvdata(gprs_data, drvdata) ((gprs_data)->private_data = (drvdata))
#endif /* __GPRS_H */
