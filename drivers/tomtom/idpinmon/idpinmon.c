/* drivers/tomtom/idpinmon/idpinmon.c

Copyright (C) 2010 TomTom BV <http://www.tomtom.com/>
Author: Andrzej Zukowski <andrzej.zukowski@tomtom.com>

This driver implements Host/Device mode detection based on the type of
USB receptacle

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>

#include <plat/tt_idpinmon.h>

#define PFX "idpinmon: "
#define DRIVER_DESC_SHORT "TomTom ID Pin monitor"
#define DRIVER_DESC_LONG  "TomTom ID Pin monitor, (C) 2010 TomTom BV"

#define idpinmon_to_dev(idpm)	((struct device *) &(idpm->pdev->dev))

struct idpinmon_struct {
	struct delayed_work     work;
	struct platform_device *pdev;
};

static inline void idpinmon_switch(struct device *dev, struct idpinmon_pdata *pdata, unsigned int n)
{
	switch (n) {
	case IDPINMON_DEVICE:
		printk(KERN_NOTICE PFX "OTG -> DEVICE mode\n");
		pdata->idpinmon_state = IDPINMON_DEVICE;
		kobject_uevent(&dev->kobj, KOBJ_OFFLINE);
		pdata->otg_device();
		break;
	case IDPINMON_HOST:
		printk(KERN_NOTICE PFX "OTG -> HOST mode\n");
		pdata->idpinmon_state = IDPINMON_HOST;
		kobject_uevent(&dev->kobj, KOBJ_ONLINE);
		pdata->otg_host();
		break;
	default:
		break;
	}
}

static void idpinmon_worker_func(struct work_struct *work)
{
	struct idpinmon_struct *idpm  = container_of(work, struct idpinmon_struct, work.work);
	struct device          *dev   = idpinmon_to_dev(idpm);
	struct idpinmon_pdata  *pdata = dev->platform_data;

	static unsigned int otgmode_prev = IDPINMON_DEVICE;
	unsigned int otgmode_curr;

	otgmode_curr = pdata->otg_getmode();

	if (otgmode_curr != otgmode_prev)
	{
		idpinmon_switch(dev, pdata, otgmode_curr);
		otgmode_prev = otgmode_curr;
	}

	schedule_delayed_work(&idpm->work, pdata->delay);
}

static ssize_t idpinmon_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct idpinmon_pdata *pdata = dev->platform_data;

	return sprintf(buf, "%d\n", pdata->idpinmon_state);
}

static ssize_t idpinmon_mode_store(struct device *dev, struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct idpinmon_pdata *pdata = dev->platform_data;
	unsigned int n = simple_strtoul(buf, NULL, 0);

	idpinmon_switch(dev, pdata, n);
	return count;
}

static DEVICE_ATTR(mode, S_IWUGO | S_IRUGO, idpinmon_mode_show, idpinmon_mode_store);

static int idpinmon_probe(struct platform_device *pdev)
{
	struct idpinmon_pdata  *pdata = pdev->dev.platform_data;
	struct idpinmon_struct *idpm;
	int ret;

	idpm = kzalloc(sizeof(struct idpinmon_struct), GFP_KERNEL);
	if (!idpm)
	{
		printk(KERN_ERR PFX " insufficient memory\n");
		return -ENOMEM;
	}

	INIT_DELAYED_WORK(&idpm->work, idpinmon_worker_func);
	idpm->pdev = pdev;
	platform_set_drvdata(pdev, idpm);

	if ((ret = device_create_file(&pdev->dev, &dev_attr_mode)))
	{
		printk(KERN_ERR PFX " unable to create sysfs file (%d)\n", ret);
		goto err_sysfs;
	}

	schedule_delayed_work(&idpm->work, pdata->delay);
	return 0;

err_sysfs:
	kfree(idpm);
	return ret;
}

static int idpinmon_remove(struct platform_device *pdev)
{
	struct idpinmon_struct *idpm = platform_get_drvdata(pdev);

	cancel_delayed_work(&idpm->work);
	flush_scheduled_work();

	device_remove_file(&pdev->dev, &dev_attr_mode);
	kfree(idpm);
	return 0;
}

#ifdef CONFIG_PM
static int idpinmon_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct idpinmon_struct *idpm = platform_get_drvdata(pdev);

	cancel_delayed_work(&idpm->work);
	return 0;
}

static int idpinmon_resume(struct platform_device *pdev)
{
	struct idpinmon_struct *idpm = platform_get_drvdata(pdev);
	struct idpinmon_pdata  *pdata = pdev->dev.platform_data;

	schedule_delayed_work(&idpm->work, pdata->delay);
	return 0;
}
#else
#define idpinmon_suspend NULL
#define idpinmon_resume  NULL
#endif /* CONFIG_PM */

static struct platform_driver idpinmon_driver = {
	.probe   = idpinmon_probe,
	.remove  = idpinmon_remove,
	.suspend = idpinmon_suspend,
	.resume  = idpinmon_resume,
	.driver  = {
		.owner = THIS_MODULE,
		.name = "tomtom-idpinmon",
	},
};

static int __init idpinmon_init(void)
{
	int ret;

	ret = platform_driver_register(&idpinmon_driver);
	if (ret) {
		printk(KERN_ERR PFX " unable to register driver (%d)\n", ret);
		return ret;
	}
	printk(KERN_INFO DRIVER_DESC_LONG "\n");
	return 0;
}

static void __exit idpinmon_exit(void)
{
	platform_driver_unregister(&idpinmon_driver);
	return;
}

module_init(idpinmon_init);
module_exit(idpinmon_exit);

MODULE_DESCRIPTION(DRIVER_DESC_LONG);
MODULE_AUTHOR("Andrzej Zukowski <andrzej.zukowski@tomtom.com>");
MODULE_LICENSE("GPL");
