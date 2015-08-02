/* drivers/tomtom/gps/bcm4760.c
 *
 * GPS driver for Broadcom BCM4760
 *
 * Copyright (C) 2009 TomTom BV <http://www.tomtom.com/>
 * Author: Benoit Leffray <benoit.leffray@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/rfkill.h>
#include <linux/regulator/consumer.h>

#include <plat/gps.h>
#include <asm/irq.h>
#include <asm/mach/irq.h>

#define TOMTOM_GPS_NAME		"tomtom-gps"
#define PFX			TOMTOM_GPS_NAME " Driver: "

#define TOMTOM_GPS_NONE		0x00
#define TOMTOM_GPS_SUSPENDED	0x01
#define TOMTOM_GPS_RESUMED	0x02

#define TOMTOM_GPS_NO_IRQ	-1

#define DRIVER_DESC_LONG "TomTom generic GPS Driver, (C) 2009 TomTom BV "

typedef struct {
	const char *		name;
	unsigned long 		flags;
	gps_power_t		suspend_power;
	struct generic_gps_info *machinfo;
	int gps_irq;
	struct rfkill *rfk_data;
	struct regulator *regulator;
} tomtom_gps_data_t;

static inline struct gps_device *gps_get_device(struct device *dev)
{
	return platform_get_drvdata(container_of(dev, struct platform_device, dev));
}

static inline tomtom_gps_data_t *gps_get_data(struct gps_device *gps_dev)
{
	return dev_get_drvdata(&gps_dev->dev);
}

static ssize_t tomtom_gps_coldboot_show(struct device *dev,
                                        struct device_attribute *attr, char *buf)
{
	struct gps_device *gpsd = gps_get_device(dev);

	if (!mutex_trylock(&gpsd->update_sem))
		return sprintf(buf, "busy\n");

	mutex_unlock(&gpsd->update_sem);
	return sprintf(buf, "available\n");
}

static ssize_t tomtom_gps_coldboot_set(struct device *dev, struct device_attribute *attr,
                                       const char *buf, size_t count)
{
	struct gps_device *gpsd = gps_get_device(dev);
	tomtom_gps_data_t *gps_drv_data	= gps_get_data(gpsd);
	struct regulator *reg = gps_drv_data->regulator;
	unsigned long delay;

	if (strict_strtoul(buf, 10, &delay)) {
		dev_err(dev, "Invalid time for coldboot reset\n");
		return count;
	}

	mutex_lock(&gpsd->update_sem);

	if (gps_drv_data->machinfo->coldboot_start(dev, reg)) {
		dev_err(dev, "unable to start coldboot reset\n");
		goto error;
	}

	if (msleep_interruptible(delay * 1000))
		dev_err(dev, "coldboot reset sleep interrupted\n");

	if (gps_drv_data->machinfo->coldboot_finish(dev, reg))
		dev_err(dev, "unable to finish coldboot reset\n");

error:
	mutex_unlock(&gpsd->update_sem);
	return count;
}
static DEVICE_ATTR(coldboot, 0644, tomtom_gps_coldboot_show, tomtom_gps_coldboot_set);

static int tomtom_gps_rfkill_set_block(void *data, bool blocked)
{
	struct gps_device *gpsd		= (struct gps_device *) data;
	tomtom_gps_data_t *gps_drv_data	= NULL;
	BUG_ON(!gpsd);

	gps_drv_data = gps_get_data(gpsd);
	BUG_ON(!gps_drv_data);

	mutex_lock(&gpsd->update_sem);
	if (blocked)
		/* Power off the GPS chip */
		gpsd->props.power = GPS_OFF;
	else
		/* Power on the GPS chip */
		gpsd->props.power = GPS_ON;

	gps_drv_data->machinfo->gps_set_power(gpsd->props.power);

	mutex_unlock(&gpsd->update_sem);

	return 0;
}

static const struct rfkill_ops ttgps_rfkill_ops = {
	.set_block = tomtom_gps_rfkill_set_block,
};

static void tomtom_gps_set_power(struct gps_device *gpsd)
{
	tomtom_gps_data_t *gps_drv_data = NULL;

	BUG_ON(!gpsd);

	gps_drv_data = gps_get_data(gpsd);
	BUG_ON(!gps_drv_data);

	if (gps_drv_data->flags & TOMTOM_GPS_SUSPENDED) {
		gps_drv_data->suspend_power = gpsd->props.power;
		gpsd->props.power = GPS_OFF;
	}

	if (gps_drv_data->flags & TOMTOM_GPS_RESUMED)
		gpsd->props.power = gps_drv_data->suspend_power;

	gps_drv_data->machinfo->gps_set_power(gpsd->props.power);
}

static void tomtom_gps_get_timestamp(struct gps_device *gpsd, struct timeval *tv)
{
	unsigned long flags;

	BUG_ON(!gpsd);
	BUG_ON(!tv);

	read_lock_irqsave(&gpsd->tv_lock, flags);
	memcpy(tv, &gpsd->props.tv, sizeof(struct timeval));
	read_unlock_irqrestore(&gpsd->tv_lock, flags);
}

static struct gps_ops tomtom_gps_ops = {
	.get_timestamp	= tomtom_gps_get_timestamp,
};

static irqreturn_t tomtom_gps_irq_handler(int irq, void *dev_id)
{
	unsigned long		flags;
	struct gps_device	*gpsd = NULL;

	BUG_ON(!dev_id);

	gpsd = platform_get_drvdata((struct platform_device *)dev_id);
	BUG_ON(!gpsd);

	write_lock_irqsave(&gpsd->tv_lock, flags);
	do_gettimeofday(&gpsd->props.tv);
	write_unlock_irqrestore(&gpsd->tv_lock, flags);

	return IRQ_HANDLED;
}

#ifdef CONFIG_PM
static int tomtom_gps_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct gps_device *gpsd		= NULL;
	tomtom_gps_data_t *gps_drv_data	= NULL;

	BUG_ON(!pdev);

	gpsd = platform_get_drvdata(pdev);
	BUG_ON(!gpsd);

	gps_drv_data = gps_get_data(gpsd);
	BUG_ON(!gps_drv_data);

	gps_drv_data->flags |= TOMTOM_GPS_SUSPENDED;

	if (gps_drv_data->machinfo->suspend)
		gps_drv_data->machinfo->suspend();

	return 0;
}

static int tomtom_gps_resume(struct platform_device *pdev)
{
	struct gps_device *gpsd		= NULL;
	tomtom_gps_data_t *gps_drv_data	= NULL;

	BUG_ON(!pdev);

	gpsd = platform_get_drvdata(pdev);
	BUG_ON(!gpsd);

	gps_drv_data = gps_get_data(gpsd);
	BUG_ON(!gps_drv_data);

	if (gps_drv_data->machinfo->resume)
		gps_drv_data->machinfo->resume();

	gps_drv_data->flags &= ~TOMTOM_GPS_SUSPENDED;
	gps_drv_data->flags |= TOMTOM_GPS_RESUMED;

	gps_drv_data->flags &= ~TOMTOM_GPS_RESUMED;

	return 0;
}
#else
#define tomtom_gps_suspend	NULL
#define tomtom_gps_resume	NULL
#endif

static int tomtom_gps_request_resource(struct platform_device *pdev)
{
	struct gps_device *gpsd		= NULL;
	tomtom_gps_data_t *gps_drv_data	= NULL;
	struct resource *res;
	int ret = 0;

	BUG_ON(!pdev);

	gpsd = platform_get_drvdata(pdev);
	BUG_ON(!gpsd);

	gps_drv_data = gps_get_data(gpsd);
	BUG_ON(!gps_drv_data);

	res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (NULL != res) {
		/* The platform device provides a IRQ resource. */
		printk(KERN_INFO PFX " %s device provides an IRQ resource (%s)\n",
				dev_name(&gpsd->dev), res->name);
		gps_drv_data->gps_irq = res->start;

		/* Get irqs */
		if (request_irq(gps_drv_data->gps_irq, &tomtom_gps_irq_handler,
						res->flags & IRQF_TRIGGER_MASK,
						res->name, pdev)) {
			gps_drv_data->gps_irq = TOMTOM_GPS_NO_IRQ;

			printk(KERN_ERR PFX " Could not allocate IRQ (%s)!\n",
					res->name);

			ret = -EIO;
		}
	}

	return ret;
}

static void tomtom_gps_release_resource(struct platform_device *pdev)
{
	struct gps_device *gpsd		= NULL;
	tomtom_gps_data_t *gps_drv_data	= NULL;

	BUG_ON(!pdev);

	gpsd = platform_get_drvdata(pdev);
	BUG_ON(!gpsd);

	gps_drv_data = gps_get_data(gpsd);
	BUG_ON(!gps_drv_data);

	if (TOMTOM_GPS_NO_IRQ != gps_drv_data->gps_irq)
		free_irq(gps_drv_data->gps_irq, pdev);
}

static tomtom_gps_data_t * tomtom_gps_init_drv_data(struct platform_device *pdev)
{
	tomtom_gps_data_t *gps_drv_data = NULL;
	struct generic_gps_info *machinfo;

	BUG_ON(!pdev);

	machinfo = pdev->dev.platform_data;
	BUG_ON(!machinfo);

	gps_drv_data = kzalloc(sizeof(tomtom_gps_data_t), GFP_KERNEL);
	if (NULL == gps_drv_data)
		return ERR_PTR(-ENOMEM);

	gps_drv_data->name	= TOMTOM_GPS_NAME;
	gps_drv_data->flags	= TOMTOM_GPS_NONE;
	gps_drv_data->gps_irq	= TOMTOM_GPS_NO_IRQ;
	gps_drv_data->machinfo	= machinfo;


	if (machinfo->name)
		gps_drv_data->name = machinfo->name;

	return gps_drv_data;
}

static int tomtom_gps_probe(struct platform_device *pdev)
{
	struct gps_device *gpsd;
	tomtom_gps_data_t *gps_drv_data = NULL;

	int ret = 0;

	BUG_ON(!pdev);
	BUG_ON(!(&pdev->dev));

	printk(KERN_INFO PFX "Probed.\n");

	gps_drv_data = tomtom_gps_init_drv_data(pdev);
	if (IS_ERR (gps_drv_data)) {
		ret = PTR_ERR(gps_drv_data);
		goto gps_init_drv_data_failed;
	}

	gpsd = gps_device_register(gps_drv_data->name, &pdev->dev,
				   NULL, &tomtom_gps_ops);
	if (IS_ERR (gpsd)) {
		ret = PTR_ERR(gpsd);
		goto gps_device_register_failed;
	}

	dev_set_drvdata(&gpsd->dev, gps_drv_data);

	platform_set_drvdata(pdev, gpsd);

	gpsd->props.power = GPS_OFF;
	memset(&gpsd->props.tv, 0x00, sizeof(struct timeval));

	if (0 != (ret = tomtom_gps_request_resource(pdev))) {
		goto gps_request_resource_failed;
	}

	/* Register as rfkill device */
	gps_drv_data->rfk_data = rfkill_alloc(gps_drv_data->name, &pdev->dev,
					      RFKILL_TYPE_GPS, &ttgps_rfkill_ops,
					      (void *)gpsd);

	if (unlikely(!gps_drv_data->rfk_data)) {
		ret = -ENOMEM;
		goto rfkill_alloc_failed;
	}

	/* mark device as persistent */
	rfkill_init_sw_state(gps_drv_data->rfk_data, 0);

	printk(KERN_INFO PFX "Registering rfkill for GPS.\n");
	ret = rfkill_register(gps_drv_data->rfk_data);
	if (unlikely(ret)) {
		printk (KERN_INFO PFX "Rfkill registration failed\n");
		goto rfkill_register_failed;
	}

	gps_drv_data->regulator = regulator_get(&pdev->dev, "vdd");
	if (gps_drv_data->regulator)
		regulator_enable(gps_drv_data->regulator);

	if (gps_drv_data->machinfo->coldboot_start &&
	    gps_drv_data->machinfo->coldboot_finish &&
	    gps_drv_data->regulator &&
	    device_create_file(&pdev->dev, &dev_attr_coldboot))
		goto sysfs_register_failed;

	printk(KERN_INFO PFX "Initialized.\n");
	return 0;

sysfs_register_failed:
	rfkill_unregister(gps_drv_data->rfk_data);
rfkill_register_failed:
	rfkill_destroy(gps_drv_data->rfk_data);
rfkill_alloc_failed:
gps_request_resource_failed:
	gps_device_unregister(gpsd);
gps_device_register_failed:
	kfree(gps_drv_data);
gps_init_drv_data_failed:
	return ret;
}

static int tomtom_gps_remove(struct platform_device *pdev)
{
	struct gps_device *gpsd		= NULL;
	tomtom_gps_data_t *gps_drv_data	= NULL;

	BUG_ON(!pdev);

	gpsd = platform_get_drvdata(pdev);
	BUG_ON(!gpsd);

	gps_drv_data = gps_get_data(gpsd);
	BUG_ON(!gps_drv_data);

	device_remove_file(&pdev->dev, &dev_attr_coldboot);

	if (gps_drv_data->regulator) {
		regulator_disable(gps_drv_data->regulator);
		regulator_put(gps_drv_data->regulator);
	}

	rfkill_unregister(gps_drv_data->rfk_data);
	rfkill_destroy(gps_drv_data->rfk_data);

	tomtom_gps_release_resource(pdev);

	gpsd->props.power = GPS_OFF;
	tomtom_gps_set_power(gpsd);

	gps_device_unregister(gpsd);

	kfree(gps_drv_data);

	printk(KERN_INFO PFX "Removed.\n");
	return 0;
}

static struct platform_driver tomtom_gps_driver = {
	.probe		= tomtom_gps_probe,
	.remove		= tomtom_gps_remove,
	.suspend	= tomtom_gps_suspend,
	.resume		= tomtom_gps_resume,
	.driver		= {
		.name	= TOMTOM_GPS_NAME,
	},
};

static int __init tomtom_gps_init(void)
{
	return platform_driver_register(&tomtom_gps_driver);
}

static void __exit tomtom_gps_exit(void)
{
	platform_driver_unregister(&tomtom_gps_driver);
}

module_init(tomtom_gps_init);
module_exit(tomtom_gps_exit);

MODULE_DESCRIPTION(DRIVER_DESC_LONG);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Benoit Leffray <benoit.leffray@tomtom.com>");
