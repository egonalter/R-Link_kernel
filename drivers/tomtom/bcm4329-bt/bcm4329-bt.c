/* drivers/tomtom/bcm4329-bt/bcm4329-bt.c
 *
 * BCM4329 BT driver
 *
 * Copyright (C) 2011 TomTom BV <http://www.tomtom.com/>
 * Author: Niels Langendorff <niels.langendorff@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/init.h>
#include <linux/rfkill.h>

#include <plat/bcm4329-bt.h>

/* Defines */
#define PFX "bcm4329: "
#define PK_DBG PK_DBG_FUNC

#define DRIVER_DESC_LONG "TomTom BCM4329 BT Driver, (C) 2011 TomTom BV "

static int bt_state = 0;

static int bcm4329_rfkill_set_power(void *data, bool blocked)
{
	struct bcm4329_platform_data *pdata = (struct bcm4329_platform_data*) data;
	
 	/*
	 * transmitter on  --> blocked == false
	 * transmitter off --> blocked == true
	 */
	if (blocked) {
		/* Inverted: Power off */
		pdata->power( 0 );
		pdata->reset( 0 );
	} else {
		/* Inverted: Power on */
		pdata->power( 1 );
		pdata->reset( 0 );
	}

	bt_state = !blocked;
	return 0;
}

static const struct rfkill_ops bcm4329_rfkill_ops = {
	.set_block = bcm4329_rfkill_set_power,
};

static int bcm4329_reset(struct bcm4329_platform_data *data, int state)
{
	BUG_ON(!data);

	data->reset( 0 );
	msleep(1);
	data->reset( 1 );
	msleep(1);
	data->reset( 0 );

	printk(KERN_INFO PFX " BT reset\n");
	return 0;
}

static int bcm4329_power(struct bcm4329_platform_data *data, int state)
{
	BUG_ON(!data);

	if (bt_state == state)
		return 0;

	if (state) {
		data->power( 1 );
		data->reset( 0 );
		msleep(1);
	} else {
		data->power( 0 );
		data->reset( 0 );
		msleep(1);
	}

	bt_state = state;
	printk(KERN_INFO PFX " BT power\n");
	return 0;
}

static ssize_t bt_show_power(struct device *dev,
			       struct device_attribute *attr, char *buffer)
{
	snprintf(buffer, PAGE_SIZE, "%i", bt_state);
	return strlen(buffer);
}

static ssize_t bt_store_power(struct device *dev,
				struct device_attribute *attr,
				const char *buffer, size_t size)
{
	size_t ret = 1;
	struct bcm4329_platform_data *pdata =
		(struct bcm4329_platform_data*)dev->platform_data;

	BUG_ON(!pdata);

	if (!strncmp(buffer, "0\n", PAGE_SIZE)) {
		ret = (bcm4329_power(pdata, 0) != 0 ? -EINVAL : 1);
	} else if (!strncmp(buffer, "1\n", PAGE_SIZE)) {
		ret = (bcm4329_power(pdata, 1) != 0 ? -EINVAL : 1);
	} else {
		ret = -EINVAL;
	}

	return ret;
}
static DEVICE_ATTR(bt_power, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
		   bt_show_power, bt_store_power);


static ssize_t bt_store_reset(struct device *dev,
				struct device_attribute *attr,
				const char *buffer, size_t size)
{
	size_t ret = 1;
	struct bcm4329_platform_data *pdata =
		(struct bcm4329_platform_data*)dev->platform_data;

	BUG_ON(!pdata);

	if (!strncmp(buffer, "0\n", PAGE_SIZE)) {
		ret = (bcm4329_reset(pdata, 0) != 0 ? -EINVAL : 1);
	} else if (!strncmp(buffer, "1\n", PAGE_SIZE)) {
		ret = (bcm4329_reset(pdata, 1) != 0 ? -EINVAL : 1);
	} else {
		ret = -EINVAL;
	}

	return ret;
}
static DEVICE_ATTR(bt_reset, S_IWUSR | S_IRGRP, NULL, bt_store_reset);

static void power(int state)
{
}

static void reset(int state)
{
}

static int bcm4329_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct bcm4329_platform_data * private;
	struct bcm4329_platform_data * pdata;

	BUG_ON(pdev == NULL);
	BUG_ON(pdev->dev.platform_data == NULL);

	pdata = (struct bcm4329_platform_data *) pdev->dev.platform_data;
	private = (struct bcm4329_platform_data *) 
		kmalloc( sizeof(struct bcm4329_platform_data), GFP_KERNEL);
	if (!private) {
		printk(KERN_ERR PFX
			" Unable to obtain memory for internal info structures!");
		return -ENOMEM;
	}

	/* Initialize structure */
	memcpy(private, pdata, sizeof(struct bcm4329_platform_data));

	/* Fill in default reset and power function if not provided */
	if (!private->reset)
		private->reset = reset;
	if (!private->power)
		private->power = power;

	if(private->request_gpio()) {
		ret = -EBUSY;
		goto gpio_request_failed;
	}

	private->config_gpio();

	if ((ret = device_create_file(&pdev->dev, &dev_attr_bt_power))) {
		printk(KERN_ERR PFX
		       " unable to create the 'power' attribute of the bt driver.\n");
		goto sysfs_power_failed;
	}

	if ((ret = device_create_file(&pdev->dev, &dev_attr_bt_reset))) {
		printk(KERN_ERR PFX
		       " unable to create the 'reset' attribute of the bt driver.\n");
		goto sysfs_reset_failed;
	}

	/* Register as rfkill device */
	private->rfk_data = rfkill_alloc("BCM4329 BT", &pdev->dev,
					RFKILL_TYPE_BLUETOOTH, &bcm4329_rfkill_ops, (void *)private);

	if (unlikely(!private->rfk_data)) {
		ret = -ENOMEM;
		goto rfkill_register_failed;
	}

	ret = rfkill_register(private->rfk_data);
	if (unlikely(ret)) {
		goto rfkill_register_failed;
	}
	dev_set_drvdata(&pdev->dev, private);

	return 0;

rfkill_register_failed:
		rfkill_destroy(private->rfk_data);
		device_remove_file(&pdev->dev, &dev_attr_bt_reset);
sysfs_reset_failed:
	device_remove_file(&pdev->dev, &dev_attr_bt_power);
sysfs_power_failed:
	if (private->free_gpio)
		private->free_gpio();
gpio_request_failed:
		kfree(private);
		private = NULL;
		return ret;
};

#ifdef CONFIG_PM
static int bcm4329_resume(struct platform_device *pdev)
{
	struct bcm4329_platform_data *pdata =
		(struct bcm4329_platform_data*)pdev->dev.platform_data;
	BUG_ON(pdata == NULL);

	printk(KERN_INFO "TomTom bcm4329 resuming.\n");
	if(pdata->resume)
		pdata->resume();
	return 0;
}

static int bcm4329_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct bcm4329_platform_data *pdata =
		(struct bcm4329_platform_data*)pdev->dev.platform_data;
	BUG_ON(pdata == NULL);

	printk(KERN_INFO "TomTom bcm4329 suspend.\n");
	if(pdata->suspend)
		pdata->suspend();
	return 0;
}
#else
#define bcm4329_resume NULL
#define bcm4329_suspend NULL
#endif

static int bcm4329_remove(struct platform_device *pdev)
{
	struct bcm4329_platform_data *pdata = (struct bcm4329_platform_data*) dev_get_drvdata(&pdev->dev);
	BUG_ON(pdata == NULL);

	/* De-register rfkill driver */
	rfkill_unregister(pdata->rfk_data);
	rfkill_destroy(pdata->rfk_data);

	device_remove_file(&pdev->dev, &dev_attr_bt_power);
	device_remove_file(&pdev->dev, &dev_attr_bt_reset);

	if (pdata->free_gpio)
		pdata->free_gpio();

	kfree(pdata);
	dev_set_drvdata(&pdev->dev, NULL);

	return 0;
}

static struct platform_driver bcm4329_driver = {
	.probe		= bcm4329_probe,
	.remove		= bcm4329_remove,
	.suspend	= bcm4329_suspend,
	.resume		= bcm4329_resume,
	.driver	= {
		.owner	= THIS_MODULE,
		.name	= "bcm4329-bt",
	}
};

int __init bcm4329_init_module(void)
{
	/* display driver information when the driver is started */
	printk(KERN_INFO DRIVER_DESC_LONG "(" PFX ")\n");

	return platform_driver_register(&bcm4329_driver);
}

void __exit bcm4329_exit_module(void)
{
	platform_driver_unregister(&bcm4329_driver);
}

module_init(bcm4329_init_module);
module_exit(bcm4329_exit_module);

MODULE_DESCRIPTION(DRIVER_DESC_LONG);
MODULE_AUTHOR("Niels Langendorff <niels.langendorff@tomtom.com>");
MODULE_LICENSE("GPL");
