/* 
 *  Copyright (C) 2009 TomTom BV <http://www.tomtom.com/>
 *
 *  Export flip-flop value R/W via sysfs
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/device.h>

#include <plat/flipflop.h>

/* Defines */
#define PFX "flipflop:"
#define PK_DBG PK_DBG_FUNC

static ssize_t flipflop_show(struct device *dev,
			struct device_attribute *attr,
			char *buf)
{
	struct flipflop_info *ffinfo = (struct flipflop_info*)
		dev->platform_data;
	int flag = ffinfo->get_value();
	return snprintf(buf, PAGE_SIZE, "%x\n", flag);
}

static ssize_t flipflop_store(struct device *dev,
			struct device_attribute *attr,
			const char *buf,
			size_t count)
{
	ssize_t ret = count;
	struct flipflop_info *ffinfo = (struct flipflop_info*)
		dev->platform_data;

	if (count > 2)
		return -EINVAL;

	if ((count == 2) && (buf[1] != '\n'))
		return -EINVAL;

	if (buf[0] == '0')
		ffinfo->set_value(0);
	else if (buf[0] == '1')
		ffinfo->set_value(1);
	else
		ret = -EINVAL;

	return ret;
}

DEVICE_ATTR(value, S_IRUGO| S_IWUGO, flipflop_show, flipflop_store);


static int flipflop_probe(struct platform_device *pdev)
{
	int rc = 0;

	struct flipflop_info *ffinfo =
		(struct flipflop_info*)pdev->dev.platform_data;

	rc = ffinfo->init(pdev);
	if (rc < 0) {
		printk(KERN_ERR "flipflop probe init err: %d\n", rc);
		goto ffprobe_out;
	}

	rc = device_create_file(&pdev->dev, &dev_attr_value);
	if (rc < 0) {
		printk(KERN_ERR "flipflop probe failure: %d\n", rc);
		goto ffprobe_out;
	}

ffprobe_out:
	return rc;
};

#ifdef CONFIG_PM
static int flipflop_resume(struct platform_device *pdev)
{
	struct flipflop_info *ffinfo =
		(struct flipflop_info*)pdev->dev.platform_data;

	printk(KERN_INFO "TomTom flipflop resuming.\n");
	ffinfo->set_value(0);
	return 0;
}
#else
#define flipflop_resume NULL
#endif

static int flipflop_remove(struct platform_device *pdev)
{
	device_remove_file(&pdev->dev, &dev_attr_value);
	return 0;
}

static struct platform_driver flipflop_driver = {
	.probe		= flipflop_probe,
	.remove		= flipflop_remove,
	.suspend	= NULL,
	.resume		= flipflop_resume,
	.driver	= {
		.owner	= THIS_MODULE,
		.name	= "flipflop",
	}
};

static char banner[] = KERN_INFO "flipflop driver, (c) 2009 TomTom BV\n";

int __init flipflop_init_module(void)
{
	printk(banner);
	return platform_driver_register(&flipflop_driver);
}

void __exit flipflop_exit_module(void)
{
	platform_driver_unregister(&flipflop_driver);
}

module_init(flipflop_init_module);
module_exit(flipflop_exit_module);

MODULE_DESCRIPTION("TomTom flipflop");
MODULE_LICENSE("GPL");
