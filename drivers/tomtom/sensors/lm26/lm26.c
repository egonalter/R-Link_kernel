/*
 * TI LM26 temperature sensor driver
 *
 * Copyright (C) 2013 TomTom International BV
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation version 2.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/platform_device.h>
#include <linux/earlysuspend.h>
#include <linux/lm26.h>

#define DRIVER_NAME		"lm26"
#define DEVICE_NAME		"LM26 Temperature Sensor"
#define DRIVER_DESC		DEVICE_NAME " Driver"

struct lm26_sensor {
	struct device			*hwmon_dev;
	struct input_dev		*report_dev;
	struct lm26_platform_data	*pdata;
	struct delayed_work		adc_update;
	unsigned int			enabled;
	unsigned int			enabled_before_suspend;
	int				current_adc;
#ifdef CONFIG_EARLYSUSPEND
	struct early_suspend		early_suspend;
#endif
};

static int lm26_update_enable(struct lm26_sensor *lm26)
{
	unsigned int timer = lm26->pdata->sample_rate * 1000;

	if (!lm26->enabled) {
		schedule_delayed_work(&lm26->adc_update, msecs_to_jiffies(timer));
		lm26->enabled = 1;
		dev_dbg(lm26->hwmon_dev->parent, "update start\n");
	}

	return 0;
}

static int lm26_update_disable(struct lm26_sensor *lm26)
{
	int cancel;
	if (lm26->enabled) {
		cancel = cancel_delayed_work_sync(&lm26->adc_update);
		lm26->enabled = 0;
		dev_dbg(lm26->hwmon_dev->parent, "update cancel=%d\n", cancel);
	}

	return 0;
}

static ssize_t show_lm26_adc(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct lm26_sensor *lm26 = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", lm26->current_adc);
}

static ssize_t show_lm26_enable(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct lm26_sensor *lm26 = dev_get_drvdata(dev);

	return sprintf(buf, "%d\n", lm26->enabled);
}

static ssize_t store_lm26_enable(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	long val;
	struct lm26_sensor *lm26 = dev_get_drvdata(dev);

	if (strict_strtol(buf, 10, &val))
		return -EINVAL;

	if (((0 == val) || (1 == val)) && (val != lm26->enabled)) {
		if (val)
			lm26_update_enable(lm26);
		else
			lm26_update_disable(lm26);
	}

	return count;
}

static SENSOR_DEVICE_ATTR(lm26_adc, S_IRUGO,
				show_lm26_adc, NULL, 0);
static SENSOR_DEVICE_ATTR(lm26_enable, S_IWUGO | S_IRUGO,
				show_lm26_enable, store_lm26_enable, 1);
static struct attribute *lm26_attrs[] = {
	&sensor_dev_attr_lm26_adc.dev_attr.attr,
	&sensor_dev_attr_lm26_enable.dev_attr.attr,
	NULL
};
static struct attribute_group lm26_attr_group = {
	.attrs = lm26_attrs,
};

static void adc_update_func(struct work_struct *work)
{
	struct lm26_sensor *lm26;
	int raw_adc;
	unsigned int timer;

	lm26 = container_of((struct delayed_work *)work,
			struct lm26_sensor, adc_update);
	if (lm26->pdata->read_raw_adc(&raw_adc)) {
		dev_err(lm26->hwmon_dev->parent, "read raw adc failed\n");
	} else {
		/* Report to input system */
		input_report_abs(lm26->report_dev, ABS_X, raw_adc);
		input_report_abs(lm26->report_dev, ABS_MISC, jiffies);
		input_sync(lm26->report_dev);
		dev_dbg(lm26->hwmon_dev->parent, "adc=%d\n", raw_adc);
	}
	timer = lm26->pdata->sample_rate * 1000;
	schedule_delayed_work(&lm26->adc_update, msecs_to_jiffies(timer));
}

static int _lm26_suspend(struct lm26_sensor *lm26)
{
	lm26->enabled_before_suspend = lm26->enabled;
	lm26_update_disable(lm26);
	return 0;
}

static int _lm26_resume(struct lm26_sensor *lm26)
{
	if (lm26->enabled_before_suspend)
		lm26_update_enable(lm26);
	return 0;
} 

#ifdef CONFIG_EARLYSUSPEND
static void lm26_early_suspend(struct early_suspend *handler)
{
	struct lm26_sensor *lm26;
	lm26 =  container_of(handler, struct lm26_sensor, early_suspend);

	_lm26_suspend(lm26);
}

static void lm26_early_resume(struct early_suspend *handler)
{
	struct lm26_sensor *lm26;
	lm26 =  container_of(handler, struct lm26_sensor, early_suspend);

	_lm26_resume(lm26);
}

#define lm26_suspend NULL
#define lm26_resume NULL
#else
static int lm26_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct lm26_sensor *lm26 = platform_get_drvdata(pdev);

	return _lm26_suspend(lm26);
}

static int lm26_resume(struct platform_device *pdev)
{
	struct lm26_sensor *lm26 = platform_get_drvdata(pdev);

	return _lm26_resume(lm26);
}
#endif

static int lm26_probe(struct platform_device *pdev)
{
	struct lm26_sensor *lm26;
	int ret;

	lm26 = kzalloc(sizeof(struct lm26_sensor), GFP_KERNEL);
	if (!lm26) {
		dev_err(&pdev->dev, "Error allocating memory for private data");
		return -ENOMEM;
	}
	lm26->pdata = pdev->dev.platform_data;

	/* Under the hwmon class */
	lm26->hwmon_dev = hwmon_device_register(&pdev->dev);
	if (IS_ERR(lm26->hwmon_dev)) {
		ret = PTR_ERR(lm26->hwmon_dev);
		dev_err(&pdev->dev, "hwmon device register: err = %d\n", ret);
		goto err_hwmon_reg;
	}
	platform_set_drvdata(pdev, lm26);
	dev_set_drvdata(lm26->hwmon_dev, lm26);

	/* Sysfs attributes */
	ret = sysfs_create_group(&lm26->hwmon_dev->kobj, &lm26_attr_group);
	if (ret) {
		dev_err(&pdev->dev, "sysfs create group: err = %d\n", ret);
		goto err_sysfs;
	}

	INIT_DELAYED_WORK(&lm26->adc_update, adc_update_func);

	/* setup input device */
	lm26->report_dev = input_allocate_device();
	if (NULL == lm26->report_dev) {
		ret = -ENOMEM;
		dev_err(&pdev->dev, "Error allocate input device\n");
		goto err_alloc_input_dev;
	}
	lm26->report_dev->name = DEVICE_NAME;
	lm26->report_dev->id.bustype = BUS_HOST;
	lm26->report_dev->dev.parent = &pdev->dev;
	input_set_drvdata(lm26->report_dev, lm26);

	/*
	 * set reporting ADC to ABS_X and ADC resolution (range) is 10 bits
	 * jiffies to ABS_MISC: this can keep updating ABS_X data even sensor
	 * data hasn't changed
	 */
	__set_bit(EV_ABS, lm26->report_dev->evbit);
	__set_bit(ABS_X, lm26->report_dev->evbit);
	__set_bit(ABS_MISC, lm26->report_dev->evbit);
	input_set_abs_params(lm26->report_dev, ABS_X, 0, BIT(10)-1, 0 , 0);
	input_set_abs_params(lm26->report_dev, ABS_MISC, 0, ULONG_MAX, 0 , 0);

	ret = input_register_device(lm26->report_dev);
	if (ret) {
		dev_err(&pdev->dev, "input register device: err = %d\n", ret);
		goto err_input_register;
	}

#ifdef CONFIG_EARLYSUSPEND
	lm26->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN;
	lm26->early_suspend.suspend = lm26_early_suspend;
	lm26->early_suspend.resume = lm26_early_resume;
	register_early_suspend(&lm26->early_suspend);
#endif
	return 0;

err_input_register:
	input_set_drvdata(lm26->report_dev, NULL);
	input_free_device(lm26->report_dev);
err_alloc_input_dev:
	sysfs_remove_group(&lm26->hwmon_dev->kobj, &lm26_attr_group);
err_sysfs:
	dev_set_drvdata(lm26->hwmon_dev, NULL);
	platform_set_drvdata(pdev, NULL);
	hwmon_device_register(lm26->hwmon_dev);
err_hwmon_reg:
	kfree(lm26);

	return ret;
}

static int lm26_remove(struct platform_device *pdev)
{
	struct lm26_sensor *lm26 = platform_get_drvdata(pdev);

	/* disable sensor update timer */
	lm26_update_disable(lm26);

	/* clean up */
#ifdef CONFIG_EARLYSUSPEND
	unregister_early_suspend(&lm26->early_suspend);
#endif
	input_unregister_device(lm26->report_dev);
	input_set_drvdata(lm26->report_dev, NULL);
	input_free_device(lm26->report_dev);
	sysfs_remove_group(&lm26->hwmon_dev->kobj, &lm26_attr_group);
	dev_set_drvdata(lm26->hwmon_dev, NULL);
	platform_set_drvdata(pdev, NULL);
	hwmon_device_unregister(lm26->hwmon_dev);
	kfree(lm26);

	return 0;
}

static struct platform_driver lm26_driver = {
	.driver		= {
		.name	= DRIVER_NAME,
		.owner	= THIS_MODULE,
	},
	.probe		= lm26_probe,
	.remove		= lm26_remove,
#ifdef CONFIG_PM
	.suspend	= lm26_suspend,
        .resume		= lm26_resume,
#else
	.suspend	= NULL,
        .resume		= NULL,
#endif
};

static int __init lm26_init(void)
{
	return platform_driver_register(&lm26_driver);
}

static void __exit lm26_exit(void)
{
	platform_driver_unregister(&lm26_driver);
}

module_init(lm26_init);
module_exit(lm26_exit);

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL");

