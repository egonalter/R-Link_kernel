/* 
 *  Copyright (C) 2011 TomTom BV <http://www.tomtom.com/>
 *
 *  Allows to set a time interval after which the device will suicide
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
#include <linux/sysfs.h>
#include <linux/rtc.h>
#include <linux/reboot.h>
#include <linux/android_alarm.h>
#include <linux/ktime.h>
#include <linux/earlysuspend.h>
#include <linux/jiffies.h>

/* The time interval after which we'll wake up. Is being set at suspend */
static unsigned long alm_interval = 0;

/* The time interval after which we'll restart the system on resume. Is 
   being (re)set on write (suicide_alm_store_alm_interval_reboot) */
static unsigned long alm_interval_reboot = 0;

/* The real time at which we'll wake up */
static ktime_t alm_time;

/* The time (in jiffies) after which we trigger a restart on resume */
static unsigned long reboot_time_in_jiffies;

/* The android alarm structure used to wake up */
static struct alarm suicide_alarm;

/* prototypes */
static void suicide_alm_early_suspend(struct early_suspend *h);
static void suicide_alm_late_resume(struct early_suspend *h);
static ssize_t suicide_alm_show_alm_interval(struct device *dev, 
					struct device_attribute *attr, 
					char *buf);
static ssize_t suicide_alm_show_alm_interval_reboot(struct device *dev, 
					struct device_attribute *attr, 
					char *buf);
static ssize_t suicide_alm_store_alm_interval(struct device *dev, 
					struct device_attribute *attr, 
					const char *buf, size_t n);
static ssize_t suicide_alm_store_alm_interval_reboot(struct device *dev, 
					struct device_attribute *attr, 
					const char *buf, size_t n);

/* define sysfs attributes */
static DEVICE_ATTR(alarm_interval, S_IRUGO | S_IWUSR, 
		suicide_alm_show_alm_interval, suicide_alm_store_alm_interval);

static DEVICE_ATTR(alarm_interval_reboot, S_IRUGO | S_IWUSR, 
		suicide_alm_show_alm_interval_reboot, suicide_alm_store_alm_interval_reboot);

/* Userland sysfs interface */
static ssize_t suicide_alm_show_alm_interval(struct device *dev, 
					struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%lu\n", alm_interval);
}

static ssize_t suicide_alm_show_alm_interval_reboot(struct device *dev, 
					struct device_attribute *attr, char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%lu\n", alm_interval_reboot);
}

/* Userland sysfs interface */
static ssize_t suicide_alm_store_alm_interval(struct device *dev, 
					struct device_attribute *attr,
					const char *buf, size_t n)
{
	alm_interval = simple_strtoul(buf, NULL, 10);
	return n;
}

/* Userland sysfs interface */
static ssize_t suicide_alm_store_alm_interval_reboot(struct device *dev, 
					struct device_attribute *attr,
					const char *buf, size_t n)
{
	alm_interval_reboot = simple_strtoul(buf, NULL, 10);
        printk(KERN_INFO "suicide_alarm driver - reboot set to %lu seconds\n", alm_interval_reboot);

        reboot_time_in_jiffies = jiffies + (HZ * alm_interval_reboot);

	return n;
}

/* Android alarm callback function - should never be called */
static void suicide_alm_triggered_func(struct alarm *alarm)
{
}

static struct early_suspend suicide_early_suspend = 
{
#ifdef CONFIG_HAS_EARLYSUSPEND
	.suspend = suicide_alm_early_suspend,
	.resume = suicide_alm_late_resume,
	.level = EARLY_SUSPEND_LEVEL_DISABLE_FB
#endif
};

static void suicide_alm_early_suspend(struct early_suspend *h)
{
	if (alm_interval > 0) {
		alm_time = ktime_add(ktime_get_real(), ktime_set(alm_interval, 0));
		alarm_start_range (&suicide_alarm, alm_time, alm_time);
	}
}

static void suicide_alm_late_resume(struct early_suspend *h)
{
	if (alm_interval > 0)
		alarm_try_to_cancel(&suicide_alarm);
}

#ifdef CONFIG_PM
static int suicide_alm_resume(struct platform_device *pdev)
{
	ktime_t real_ktime = ktime_get_real();
	if (alm_interval > 0 && 
	    (ktime_us_delta(alm_time, real_ktime)/USEC_PER_SEC <= 1)) {
		/* We went past our expiration date */
		printk(KERN_INFO "suicide_alarm driver - system powering off\n");
		printk(KERN_INFO "alm_interval(%lu_), alm_time(%lld), ktime_get_real()(%lld)\n", alm_interval, (long long)alm_time.tv64, (long long)real_ktime.tv64);
		machine_power_off();
	} else if ((alm_interval_reboot > 0) && 
	    (time_after(jiffies, reboot_time_in_jiffies) > 0)) {
		printk(KERN_INFO "suicide_alarm driver - reboot timer expired! rebooting!\n");
		machine_restart(0);
        }
	return 0;
}
#endif

static int suicide_alm_probe(struct platform_device *pdev)
{
	int rc = device_create_file(&pdev->dev, &dev_attr_alarm_interval);
	if (rc < 0) {
		dev_err(&pdev->dev, "Could not create alm_interval sysfs attribute\n");
		return rc;
	}

	rc = device_create_file(&pdev->dev, &dev_attr_alarm_interval_reboot);
	if (rc < 0) {
		dev_err(&pdev->dev, "Could not create alm_interval_reboot sysfs attribute\n");
		return rc;
	}

	alarm_init(&suicide_alarm, ANDROID_ALARM_RTC_WAKEUP, 
		suicide_alm_triggered_func);
	register_early_suspend(&suicide_early_suspend);

	return 0;
};

static int suicide_alm_remove(struct platform_device *pdev)
{
	unregister_early_suspend(&suicide_early_suspend);
	device_remove_file(&pdev->dev, &dev_attr_alarm_interval);
	device_remove_file(&pdev->dev, &dev_attr_alarm_interval_reboot);
	return 0;
}

static struct platform_driver suicide_alm_driver = {
	.probe		= suicide_alm_probe,
	.remove		= suicide_alm_remove,
#ifdef CONFIG_PM
	.resume		= suicide_alm_resume,
#endif
	.driver	= {
		.owner	= THIS_MODULE,
		.name	= "suicide_alarm",
	}
};

static int __init suicide_alm_init_module(void)
{
	printk(KERN_INFO "suicide_alarm driver, (c) 2011 TomTom BV\n");
	return platform_driver_register(&suicide_alm_driver);
}

static void __exit suicide_alm_exit_module(void)
{
	platform_driver_unregister(&suicide_alm_driver);
}

module_init(suicide_alm_init_module);
module_exit(suicide_alm_exit_module);

MODULE_DESCRIPTION("TomTom suicide_alarm driver");
MODULE_LICENSE("GPL");
