/* drivers/tomtom/vbusmon.c

Copyright (C) 2009 TomTom BV <http://www.tomtom.com/>
Author: Pepijn de Langen <pepijn.delangen@tomtom.com>

This driver implements power detection on the USB vbus pin

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.
*/

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/device.h>
#include <linux/notifier.h>

#include <plat/tt_vbusmon.h>

#define PFX "vbusmon: "
#define DRIVER_DESC_SHORT "TomTom Vbus monitor"
#define DRIVER_DESC_LONG  "TomTom Vbus monitor, (C) 2009, 2010 TomTom BV"

// vbus_on is only to transfer data from vbus_handler() to vbus_changed_work_handler()
// if devices are slow then those should perform caching internally and return that in poll_vbusdetect()
static atomic_t vbus_on = ATOMIC_INIT(0);
#ifdef CONFIG_TOMTOM_USBMODE_QUICKCHANGES
static int last_vbus = 0;
#endif
static RAW_NOTIFIER_HEAD(vbusmon_chain);
static struct work_struct vbus_changed;

static struct device *current_dev;

// this protects the vbusmon_chain, last_vbus and current_dev
static DEFINE_MUTEX(gpio_vbus_mutex);

static void vbus_notify(int value)
{
  dev_info(current_dev, PFX "notifying listeners about vbus state (%d)\n", value);
  raw_notifier_call_chain(&vbusmon_chain, value, NULL);
}

/* Define a worker method used to handle changes asynchronously */
static void vbus_changed_work_handler(struct work_struct *work)
{
  int v = atomic_read(&vbus_on);

  mutex_lock( &gpio_vbus_mutex );

#ifndef CONFIG_TOMTOM_USBMODE_QUICKCHANGES
	vbus_notify(v);
#else
  // Invert last recorded vbus - This is still kept because of suspend issues.
  last_vbus ^= 1;
	vbus_notify(last_vbus);
#endif

#ifdef CONFIG_TOMTOM_USBMODE_QUICKCHANGES
	if (last_vbus != v)
	{
		last_vbus^=1;
		// Vbus changed again
		if(current_dev != NULL){
			dev_notice(current_dev, PFX "vbus changed between interrupt and work_handler, \
						sending new state\n");
		}
		vbus_notify(last_vbus);
	}
#endif

  mutex_unlock( &gpio_vbus_mutex );
}

/**
 * This is basically the interrupt handler, but our arch callback supplies the
 * level of the irq in adition to the usual info
 * Return 0 for success
 **/
static int vbus_handler(struct device *dev, int pinstate)
{
	dev_info(dev, PFX "vbus is: %d\n", pinstate);
	if (dev == current_dev) {
	  atomic_set(&vbus_on, pinstate);
    schedule_work(&vbus_changed);
	}
	return 0;
}

static ssize_t vbusmon_sysfs_read_vbus(struct device *dev, struct device_attribute *attr,char *buf)
{
  struct vbus_pdata *pdata = (struct vbus_pdata *)dev->platform_data;
	return sprintf(buf, "%d\n", pdata->poll_vbusdetect());
}

static DEVICE_ATTR(vbus, S_IRUGO, vbusmon_sysfs_read_vbus,NULL);

static ssize_t vbusmon_sysfs_read_name(struct device *dev, struct device_attribute *attr, char *buf)
{
  struct vbus_pdata *pdata = (struct vbus_pdata *)dev->platform_data;
  return sprintf(buf, "%s\n", pdata->name);
}

static DEVICE_ATTR(name, S_IRUGO, vbusmon_sysfs_read_name,NULL);

int vbusmon_poll(void)
{
  int ret;

  mutex_lock( &gpio_vbus_mutex );
  if (current_dev) {
    struct vbus_pdata *pdata = (struct vbus_pdata *)current_dev->platform_data;
    ret = pdata->poll_vbusdetect();
  }
  else {
    ret = -ENODEV;
  }
  mutex_unlock( &gpio_vbus_mutex );
  return ret;
}

void vbusmon_set_device(struct device *dev)
{
  int ret;

  mutex_lock( &gpio_vbus_mutex );

  if (current_dev)
    sysfs_remove_link(&current_dev->parent->kobj, "tomtom-vbus");
  current_dev = dev;
  if (current_dev) {
    ret = sysfs_create_link(&current_dev->parent->kobj, &dev->kobj, "tomtom-vbus");
    if (ret)
      printk(KERN_ERR PFX "Failed to create tomtom-vbus link in sysfs (%d).\n", ret);
  }

  if (current_dev) {
    // do the notifications asynchronously to avoid locking issues
    struct vbus_pdata *pdata = (struct vbus_pdata *) current_dev->platform_data;
    vbus_handler(current_dev, pdata->poll_vbusdetect());
  }

  mutex_unlock( &gpio_vbus_mutex );
}

/* Notifier chain */
int vbusmon_register_notifier(struct notifier_block *nb)
{
	int ret;

  mutex_lock( &gpio_vbus_mutex );
  VBUSMON_TRACE();
	ret = raw_notifier_chain_register(&vbusmon_chain,nb);

  // notify of current value; if there's no device yet then notification will be when device is set
	if (current_dev)
	  nb->notifier_call(nb, atomic_read(&vbus_on), NULL);

	mutex_unlock( &gpio_vbus_mutex );
  return ret;
}

int vbusmon_unregister_notifier(struct notifier_block *nb)
{
  int ret;

  mutex_lock( &gpio_vbus_mutex );
	VBUSMON_TRACE();
	ret = raw_notifier_chain_unregister(&vbusmon_chain,nb);
  mutex_unlock( &gpio_vbus_mutex );

  return ret;
}

static int vbusmon_probe(struct platform_device *pdev)
{
	struct vbus_pdata *pdata;
	int ret;

	VBUSMON_TRACE();
	pdata = (struct vbus_pdata *) pdev->dev.platform_data;
  BUG_ON(pdata->name == NULL);
	if ((ret=pdata->config_vbusdetect(vbus_handler))) {
		printk(KERN_ERR PFX "unable to configure vbus_handler (%d)\n", ret);
		return ret;
	}

	ret=device_create_file(&pdev->dev, &dev_attr_vbus);
	if (ret) {
		pdata->cleanup_vbusdetect();
		printk(KERN_ERR PFX "unable to create vbus sysfs file (%d)\n", ret);
		return ret;
	}
  ret=device_create_file(&pdev->dev, &dev_attr_name);
  if (ret) {
    pdata->cleanup_vbusdetect();
    printk(KERN_ERR PFX "unable to create name sysfs file (%d)\n", ret);
    return ret;
  }

  /* set initial state */
  mutex_lock( &gpio_vbus_mutex );
  if (current_dev == &pdev->dev) {
    // handle asynchronous so caller won't get into trouble
    vbus_handler(&pdev->dev, pdata->poll_vbusdetect());
  }
  mutex_unlock( &gpio_vbus_mutex );
	
	return 0;
}

static int vbusmon_remove(struct platform_device *pdev)
{
	struct vbus_pdata *pdata = (struct vbus_pdata *) pdev->dev.platform_data;
  mutex_lock( &gpio_vbus_mutex );
	if (current_dev == &pdev->dev)
	  current_dev = NULL;
  mutex_unlock( &gpio_vbus_mutex );
	pdata->cleanup_vbusdetect();
	device_remove_file(&pdev->dev, &dev_attr_vbus);
  device_remove_file(&pdev->dev, &dev_attr_name);
	return 0;
}

#ifdef CONFIG_PM
static int vbusmon_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct vbus_pdata *pdata;
	pdata = (struct vbus_pdata *) pdev->dev.platform_data;
	pdata->do_suspend();
        return 0;
}

static int vbusmon_resume(struct platform_device *pdev)
{
	struct vbus_pdata *pdata;
	pdata = (struct vbus_pdata *) pdev->dev.platform_data;
	pdata->do_resume();

  mutex_lock( &gpio_vbus_mutex );
	if (&pdev->dev == current_dev) {
	  atomic_set(&vbus_on, pdata->poll_vbusdetect());
#ifdef CONFIG_TOMTOM_USBMODE_QUICKCHANGES
    if (last_vbus != atomic_read(&vbus_on))
#endif
    {
      schedule_work(&vbus_changed);
    }
	}
  mutex_unlock( &gpio_vbus_mutex );

  return 0;
}
#else
#define vbusmon_suspend NULL
#define vbusmon_resume  NULL
#endif /* CONFIG_PM */

static struct platform_driver vbusmon_driver = {
	.probe = vbusmon_probe,
	.remove = vbusmon_remove,
	.suspend = vbusmon_suspend,
	.resume = vbusmon_resume,
	.driver = {
		.owner = THIS_MODULE,
		.name = "tomtom-vbus",
	},
};

static int __init vbusmon_driver_init(void)
{
	int ret;
	ret = platform_driver_register(&vbusmon_driver);
	if (ret) {
		printk(KERN_ERR PFX "unable to register driver (%d)\n", ret);
		return ret;
	}
  INIT_WORK(&vbus_changed, vbus_changed_work_handler);

  printk(KERN_INFO DRIVER_DESC_LONG "(" PFX ")\n");
	return 0;
}

static void __exit vbusmon_driver_exit(void)
{
	platform_driver_unregister(&vbusmon_driver);
	return;
}

EXPORT_SYMBOL(vbusmon_set_device);
EXPORT_SYMBOL(vbusmon_poll);
EXPORT_SYMBOL(vbusmon_register_notifier);
EXPORT_SYMBOL(vbusmon_unregister_notifier);

fs_initcall(vbusmon_driver_init);
module_exit(vbusmon_driver_exit);

MODULE_DESCRIPTION(DRIVER_DESC_LONG);
MODULE_AUTHOR("Pepijn de Langen (pepijn.delangen@tomtom.com)");
MODULE_LICENSE("GPL");
