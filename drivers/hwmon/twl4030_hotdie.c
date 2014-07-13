/*
 *
 * drivers/hwmon/twl4030_hotdie.c
 * 
 * TWL4030 Hotdie
 * supports configuration of temperature threshold
 * handles interrupt when temperature crosses the threshold
 *
 * Copyright (C) 2011 TomTom International BV
 * Author: Oreste Salerno <oreste.salerno@tomtom.com>
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
#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/i2c/twl.h>
#include <linux/irq.h>

#define REG_MISC_CFG	(0xD)
#define TEMP_SEL_OFFSET	(6)
#define TEMP_SEL_MASK	(3 << TEMP_SEL_OFFSET)

struct twl4030_hotdie {
	struct device		*dev;
	atomic_t		state;
};

static irqreturn_t twl4030_hotdie_interrupt(int irq, void *_hd)
{
	struct twl4030_hotdie *hd = _hd;
	
#ifdef CONFIG_LOCKDEP
	/* WORKAROUND for lockdep forcing IRQF_DISABLED on us, which
	 * we don't want and can't tolerate since this is a threaded
	 * IRQ and can sleep due to the i2c reads it has to issue.
	 * Although it might be friendlier not to borrow this thread
	 * context...
	 */
	local_irq_enable();
#endif
	if (atomic_read(&hd->state)) {
		/* Temperature has fallen below the threshold */
		atomic_set(&hd->state, 0);
		set_irq_type(irq, IRQ_TYPE_EDGE_RISING);
	}
	else {
		/* Temperature has risen above the threshold */
		atomic_set(&hd->state, 1);
		set_irq_type(irq, IRQ_TYPE_EDGE_FALLING);
	}
	dev_warn(hd->dev, "TWL4030 Hotdie status %d\n", atomic_read(&hd->state));
	sysfs_notify(&hd->dev->kobj, NULL, "hotdie");
	kobject_uevent(&hd->dev->kobj, KOBJ_CHANGE);

	return IRQ_HANDLED;
}

static ssize_t set_hotdie_threshold(struct device *dev,
		struct device_attribute *attr, const char *buf, size_t count)
{
	long val;
	u8 misc_cfg, threshold_sel = 0;

	if (strict_strtol(buf, 10, &val))
		return -EINVAL;

	if (val < 130)
		threshold_sel = 0;
	else if (val >= 130 && val < 140)
		threshold_sel = 1;
	else
		threshold_sel = 2;

	twl_i2c_read_u8(TWL4030_MODULE_PM_RECEIVER, &misc_cfg, REG_MISC_CFG);
	misc_cfg &= ~TEMP_SEL_MASK;
	misc_cfg |= (threshold_sel << TEMP_SEL_OFFSET);
	twl_i2c_write_u8(TWL4030_MODULE_PM_RECEIVER, misc_cfg, REG_MISC_CFG);

	return count;
}

static ssize_t show_hotdie_threshold(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int val = 120;
	u8 misc_cfg, threshold_sel = 0;

	twl_i2c_read_u8(TWL4030_MODULE_PM_RECEIVER, &misc_cfg, REG_MISC_CFG);
	threshold_sel = (misc_cfg & TEMP_SEL_MASK) >> TEMP_SEL_OFFSET;

	if (threshold_sel == 0)
		val = 120;
	else if (threshold_sel == 1)
		val = 130;
	else if (threshold_sel == 2)
		val = 140;

	return sprintf(buf, "%d\n", val);
}

static ssize_t show_hotdie_state(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int val;
	struct twl4030_hotdie *hd = dev_get_drvdata(dev);

	val = atomic_read(&hd->state);
	return sprintf(buf, "%d\n", val);
}

static DEVICE_ATTR(hotdie_state, S_IRUGO, show_hotdie_state, NULL);
static DEVICE_ATTR(hotdie_threshold, S_IWUSR | S_IRUGO, show_hotdie_threshold,
				set_hotdie_threshold);

static struct attribute *twl4030_hotdie_attributes[] = {
	&dev_attr_hotdie_state.attr,
	&dev_attr_hotdie_threshold.attr,
	NULL,
};

static const struct attribute_group twl4030_hotdie_attr_group = {
	.attrs = twl4030_hotdie_attributes,
};

static int __devinit twl4030_hotdie_probe(struct platform_device *pdev)
{
	struct twl4030_hotdie *hd;
	int irq, ret;

	hd = kzalloc(sizeof(*hd), GFP_KERNEL);
	if (!hd)
		return -ENOMEM;

	atomic_set(&hd->state, 0);
	hd->dev = hwmon_device_register(&pdev->dev);
	if (IS_ERR(hd->dev)) {
		dev_err(&pdev->dev, "failed to register twl4030 hotdie\n");
		ret = PTR_ERR(hd->dev);
		goto exit_register;
	}
	platform_set_drvdata(pdev, hd);
	dev_set_drvdata(hd->dev, hd);

	ret = sysfs_create_group(&hd->dev->kobj, &twl4030_hotdie_attr_group);
	kobject_uevent(&hd->dev->kobj, KOBJ_ADD);
	if (ret) {
		dev_err(&pdev->dev, "could not create sysfs files\n");
		goto exit_sysfs;
	}

	irq = platform_get_irq(pdev, 0);
	ret = request_irq(irq, twl4030_hotdie_interrupt, IRQF_TRIGGER_RISING,
			"twl4030_hotdie", hd);
	if (ret) {
		dev_err(&pdev->dev, "could not request irq %d, status %d\n",
			irq, ret);
		goto exit_irq;
	}

	return 0;
	
exit_irq:
	sysfs_remove_group(&hd->dev->kobj, &twl4030_hotdie_attr_group);
exit_sysfs:
	dev_set_drvdata(hd->dev, NULL);
	platform_set_drvdata(pdev, NULL);
	hwmon_device_unregister(hd->dev);
exit_register:
	kfree(hd);
	return ret;
}

static int __devexit twl4030_hotdie_remove(struct platform_device *pdev)
{
	struct twl4030_hotdie *hd = platform_get_drvdata(pdev);
	int irq = platform_get_irq(pdev, 0);
	
	free_irq(irq, hd);
	sysfs_remove_group(&hd->dev->kobj, &twl4030_hotdie_attr_group);
	dev_set_drvdata(hd->dev, NULL);
	platform_set_drvdata(pdev, NULL);
	hwmon_device_unregister(hd->dev);
	kfree(hd);

	return 0;
}

static struct platform_driver twl4030_hotdie_driver = {
	.probe		= twl4030_hotdie_probe,
	.remove		= __devexit_p(twl4030_hotdie_remove),
	.driver		= {
		.name	= "twl4030_hotdie",
	},
};

static int __init twl4030_hotdie_init(void)
{
	return platform_driver_register(&twl4030_hotdie_driver);
}
module_init(twl4030_hotdie_init);

static void __exit twl4030_hotdie_exit(void)
{
	platform_driver_unregister(&twl4030_hotdie_driver);
}
module_exit(twl4030_hotdie_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("TWL4030 Hotdie");
MODULE_ALIAS("platform:twl4030_hotdie");
MODULE_AUTHOR("Oreste Salerno <oreste.salerno@tomtom.com>");
