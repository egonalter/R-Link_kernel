/* drivers/tomtom/gprs/gprs/gprs.c
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

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uio.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/jiffies.h>
#include <asm/uaccess.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/kdev_t.h>
#include <linux/kobject.h>
#include <linux/sysfs.h>
#include <linux/stat.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <plat/gprs.h>

/* Defines */
#define PFX "gprs:"
#define PK_DBG PK_DBG_FUNC
#define DRIVER_DESC_LONG "TomTom Generic GPRS Driver, (C) 2008 TomTom BV "

static struct gprs_data *st_gprs_data = NULL;

gprs_state_e gprs_modem_status(struct gprs_data *data)
{
	gprs_state_e state = GPRS_STATUS_OFF;

	BUG_ON(!data);
	BUG_ON(!data->u_ops);

	if (data->u_ops->gprs_state)
		state = data->u_ops->gprs_state(data);

	return state;
}

static ssize_t gprs_show_power(struct device *dev,
			       struct device_attribute *attr, char *buffer)
{
	struct gprs_data *data = dev_get_drvdata(dev);

	snprintf(buffer, PAGE_SIZE, "%i", gprs_modem_status(data));
	return strlen(buffer);
}

static ssize_t gprs_store_power(struct device *dev,
				struct device_attribute *attr,
				const char *buffer, size_t size)
{
	struct gprs_data *data = dev_get_drvdata(dev);

	/* Make sure a modem is registered */
	if (!data->u_ops)
		return -ECANCELED;

	/* Power. Should match the entry power in the gprs attributes structure. */
	switch (buffer[0]) {
	case '0':
		if (data->u_ops->gprs_off) {
			data->u_ops->gprs_off(data);
		}
		break;
	case '1':
		if (data->u_ops->gprs_on) {
			data->u_ops->gprs_on(data);
		}
		break;
	}

	return size;
}

static DEVICE_ATTR(power_on, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH,
		   gprs_show_power, gprs_store_power);

static ssize_t gprs_show_name(struct device *dev,
			      struct device_attribute *attr, char *buffer)
{
	struct gprs_data *data = dev_get_drvdata(dev);

	if (!data->name)
		return -ECANCELED;

	snprintf(buffer, PAGE_SIZE, "%s\n", data->name);
	return strlen(buffer);
}

static DEVICE_ATTR(name, S_IRUSR | S_IRGRP, gprs_show_name, NULL);

static ssize_t gprs_store_reset(struct device *dev,
				struct device_attribute *attr,
				const char *buffer, size_t size)
{
	struct gprs_data *data = dev_get_drvdata(dev);
	size_t ret = 1;

	/* Make sure a modem is registered */
	if (!data->u_ops)
		return -ECANCELED;

	/* Reset. Should match the entry reset in the gprs attributes structure. */
	switch (buffer[0]) {
	case '1':
		if (data->u_ops->gprs_reset)
			data->u_ops->gprs_reset(data);
		break;
	default:
		return -EINVAL;
	}

	return size;
}

static DEVICE_ATTR(reset, S_IWUSR | S_IWGRP | S_IWOTH, NULL, gprs_store_reset);

/* Register gprs driver */
int register_gprs_driver(struct gprs_operations *u_ops, char *name,
			 unsigned int gprs_id)
{
	struct platform_device *pdev;
	int length;

	/* Is the driver initialized */
	if (!st_gprs_data) {
		printk(KERN_ERR PFX " Could not register driver, I am not "
		       "initialized yet.\n");
		return -ENODEV;
	}

	/* Is the modprobe called */
	if (!st_gprs_data->dev) {
		printk(KERN_ERR PFX
		       " Could not register driver, I have not "
		       "been probed yet.\n");
		return -ENODEV;
	}

	/* Is there a platform_device struct */
	pdev = to_platform_device(st_gprs_data->dev);
	if (!pdev) {
		printk(KERN_ERR PFX " Could not register driver, I do not "
		       "have a platform_device.\n");
		return -EINVAL;
	}

	/* Is there a driver already using the state machine? */
	if (st_gprs_data->u_ops || st_gprs_data->name) {
		printk(KERN_INFO PFX
		       " GPRS driver already registered! \n");
		return -EBUSY;
	}

	/* Check if the correct gprs modem is registering */
	if (st_gprs_data->pd->gprs_id != gprs_id) {
		printk(KERN_ERR PFX " Could not register driver, I have a "
		       "wrong gprs id.\n");
		return -EINVAL;
	}

	/* Store the struct with callbacks */
	st_gprs_data->u_ops = u_ops;

	/* Get the name */
	if (name) {
		length = strlen(name) + 1;	/* Include '\0' */

		if (!(st_gprs_data->name = kmalloc(length, GFP_KERNEL))) {
			printk(KERN_ERR PFX
			       " failed to allocate memory for the gprs_data.name \n");
			return -ENOMEM;
		}
		snprintf(st_gprs_data->name, length, "%s", name);
	}

	if (st_gprs_data->u_ops->gprs_probe)
		return st_gprs_data->u_ops->gprs_probe(st_gprs_data);

	return 0;
}

EXPORT_SYMBOL(register_gprs_driver);

/* Unregister gprs driver, note: the actual implementation has to do its cleanup by itself */
void unregister_gprs_driver(void)
{
	printk(KERN_INFO PFX " unregister gprs driver\n");

	BUG_ON(!st_gprs_data);
	BUG_ON(!st_gprs_data->u_ops);

	if (st_gprs_data->u_ops->gprs_remove) {
		st_gprs_data->u_ops->gprs_remove(st_gprs_data);
	}

	st_gprs_data->u_ops = NULL;

	if (st_gprs_data->name) {
		kfree(st_gprs_data->name);
		st_gprs_data->name = NULL;

	}
}

EXPORT_SYMBOL(unregister_gprs_driver);

static int gprs_probe(struct platform_device *pdev)
{
	struct gprs_platform_data *pd = pdev->dev.platform_data;

	/* Put platform data in global driver data - we assume there it is only
	 * possible to plug one dock at a time
	 */
	st_gprs_data->pd = pdev->dev.platform_data;
	BUG_ON(pd == NULL);

	/* make gprs_data the driver_data */
	dev_set_drvdata(&pdev->dev, st_gprs_data);

	/* and assign the dev back to the structure */
	st_gprs_data->dev = &pdev->dev;

	if (device_create_file(&pdev->dev, &dev_attr_power_on)) {
		printk(KERN_ERR PFX
		       " unable to create the 'power_on' attribute of the gprs driver.\n");
		return 1;
	}

	if (device_create_file(&pdev->dev, &dev_attr_name)) {
		printk(KERN_ERR PFX
		       " unable to create the 'name' attribute of the gprs driver.\n");
		return 1;
	}

	if (device_create_file(&pdev->dev, &dev_attr_reset)) {
		printk(KERN_ERR PFX
		       " unable to create the 'reset' attribute of the gprs driver.\n");
		return 1;
	}

	/* Done. */
	return 0;
}

static int gprs_remove(struct platform_device *pdev)
{
	if (st_gprs_data->u_ops) {
		printk(KERN_INFO PFX
		       " GPRS driver still has a driver registered!\n");
		return EBUSY;
	}

	/* Is there a platform_device struct */
	if (!pdev) {
		printk(KERN_ERR PFX
		       "Can't get device data for device removal!!!\n");
		return -ENODEV;
	}

	device_remove_file(&pdev->dev, &dev_attr_power_on);
	device_remove_file(&pdev->dev, &dev_attr_name);
	device_remove_file(&pdev->dev, &dev_attr_reset);

	return 0;
}

static void gprs_shutdown(struct platform_device *pdev)
{
	if (pdev)
		gprs_remove(pdev);
	return;
}

static int gprs_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct gprs_data *data = dev_get_drvdata(&pdev->dev);
	int rc = 0;

	if (state.event == PM_EVENT_SUSPEND && data->u_ops) {
		printk(KERN_INFO PFX " suspend\n");
		if (data->u_ops->gprs_suspend)
			rc = data->u_ops->gprs_suspend(data);
	}

	return rc;
}

/*This driver registers the highlevel gprs driver */
static struct platform_driver gprs_driver = {
	.probe = gprs_probe,
	.remove = __exit_p(gprs_remove),
	.suspend = gprs_suspend,
	.shutdown = gprs_shutdown,
	.driver = {
		   .owner = THIS_MODULE,
		   .name = GPRS_DEVNAME,
		   },
};

static int __init gprs_init(void)
{
	int ret;

	/* display driver information when the driver is started */
	printk(KERN_INFO DRIVER_DESC_LONG "(" PFX ")\n");

	if (!
	    (st_gprs_data =
	     kmalloc(sizeof(struct gprs_data), GFP_KERNEL))) {
		printk(KERN_ERR PFX
		       " failed to allocate memory for the gprs_data struct \n");
		return -ENOMEM;
	}

	/* clear the data and initialize */
	memset(st_gprs_data, 0, sizeof(struct gprs_data));

	printk(KERN_DEBUG PFX " Initialized\n");

	ret = platform_driver_register(&gprs_driver);
	if (ret) {
		printk(KERN_ERR PFX " unable to register driver (%d)\n",
		       ret);
		return ret;
	}

	return 0;
}

static void __exit gprs_exit(void)
{
	platform_driver_unregister(&gprs_driver);
	kfree(st_gprs_data);
	printk(KERN_INFO PFX " exit\n");
}

subsys_initcall(gprs_init);
module_exit(gprs_exit);

MODULE_DESCRIPTION(DRIVER_DESC_LONG);
MODULE_AUTHOR("Rogier Stam " "(rogier.stam@tomtom.com)" ","
	      "Niels Langendorff " "(niels.langendorff@tomtom.com)");
MODULE_LICENSE("GPL");
