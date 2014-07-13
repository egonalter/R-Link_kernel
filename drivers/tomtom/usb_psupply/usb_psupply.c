/*
 * drivers/tomtom/usb_psupply/usb_psupply.c
 *
 * Driver for USB power supplies
 *
 * Copyright (C) 2010 TomTom International BV
 * Author: Matthias Kaehlcke <matthias.kaehlcke@tomtom.com>
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

/*
  NOTE: This driver is intended to be generic, but it depends on a certain
  hardware implementation, which currently only exists on Santiago
*/

/*
  The main purpose of the USB power supply driver is to control fast charging
  (> 500mA). Fast charging is allowed in two cases: a high-power charger or
  and accessory charger adapter (ACA) is connected. It also notifies the system
  when USB power is plugged or unplugged

  On Santiago fast charging is controlled by the lines USB_CHRG_DET and
  WALL_SENSE_CONNECT. If either of these signals is asserted, fast charging
  is enabled

  High-power chargers are characterized by shorted D+/- lines. On Santiago this
  is detected by the PMIC, which asserts the signal USB_CHRG_DET when a
  high-power charger is connected to the OTG port

  Accessory charger adapters are detected by sensing the resistance on the
  OTG ID pin. If the value of the resistance is between 119kΩ and 132kΩ the
  device connected to the OTG port is an ACA

  The driver requests an IRQ to be generated upon changes on VBUS. In the
  corresponding ISR the driver checks if an ACA is connected. If that's the
  case WALL_SENSE_CONNECT is asserted, otherwise the signal is cleared
*/

#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/usb_psupply.h>

#define DRIVER_DESC_LONG "TomTom USB power supply driver, (C) 2010 TomTom BV"

#define USBPS_STATE_UNKNOWN 		0 /* unknown state */
#define USBPS_UNPLUGGED 		1 /* no VBUS supplied */
#define USBPS_PLUGGED_TO_HOST_PORT	2 /* powered by a USB host */
#define USBPS_PLUGGED_TO_ACA		3 /* accessory charger adapter */
#define USBPS_PLUGGED_TO_HPC		4 /* high-power charger */

static int usbps_power_supply_get_property(struct power_supply *psy,
					enum power_supply_property psp,
					union power_supply_propval *val);

static enum power_supply_property usbps_power_supply_props[] = {
        POWER_SUPPLY_PROP_ONLINE,
};

struct usb_psupply_struct {
	struct usb_psupply_pdata *pdata;
	struct platform_device *pdev;
	int state;
	struct power_supply ps;
	struct workqueue_struct *workqueue;
	struct delayed_work vbus_work;
	spinlock_t lock;
};

static struct usb_psupply_struct usb_psupply = {
	.state = USBPS_STATE_UNKNOWN,
	.ps = {
		.name		= "usb",
		.type		= POWER_SUPPLY_TYPE_USB,
		.properties	= usbps_power_supply_props,
		.num_properties	= ARRAY_SIZE(usbps_power_supply_props),
		.get_property	= usbps_power_supply_get_property,
	}
};

static ssize_t usbps_sysfs_show_state(struct device *dev, struct device_attribute *attr, char *buf);

static DEVICE_ATTR(state, S_IRUGO, usbps_sysfs_show_state, NULL);

static inline void __usbps_enable_fast_charging(int enable)
{
	gpio_set_value(usb_psupply.pdata->gpio_fast_charge_en, enable);
}

static inline void usbps_enable_fast_charging(void)
{
	__usbps_enable_fast_charging(1);
}

static inline void usbps_disable_fast_charging(void)
{
	__usbps_enable_fast_charging(0);
}

static int usbps_get_state(void)
{
	struct usb_psupply_pdata *pdata = (struct usb_psupply_pdata *)usb_psupply.pdata;
	int state = USBPS_UNPLUGGED;

	if (gpio_get_value(pdata->gpio_vbus)) {
		/* VBUS supplied */

		if (gpio_get_value(pdata->gpio_chrg_det)) {
			/* high-power charger connected */
			state = USBPS_PLUGGED_TO_HPC;
		} else {
			if (pdata->is_accessory_charger_plugged())
				state = USBPS_PLUGGED_TO_ACA;
			else
				state = USBPS_PLUGGED_TO_HOST_PORT;
		}
	}

	return state;
}

static void usbps_vbus_event_handler(struct work_struct *work)
{
	const int prev_state = usb_psupply.state;

	usb_psupply.state = usbps_get_state();

	if (usb_psupply.state == USBPS_PLUGGED_TO_ACA) {
		usbps_enable_fast_charging();

		USBPS_DBG("accessory charger adapter connected (fast charging enabled)");
	} else {
		/* disable fast charging (in case of a high power charger
		   the CHRG_DET signal will still allow fast charging) */
		usbps_disable_fast_charging();

		if (usb_psupply.state == USBPS_PLUGGED_TO_HPC)
			USBPS_DBG("high-power charger connected (fast charging enabled)");
		else if (usb_psupply.state == USBPS_PLUGGED_TO_HOST_PORT)
			USBPS_DBG("connected to USB host (fast charging disabled)");
		else
			USBPS_DBG("no VBUS supplied (not charging)");
	}

	if (work != NULL) {
		/* it's not the initial call from the probe() function */

		if ((prev_state == USBPS_UNPLUGGED) ||
			(usb_psupply.state == USBPS_UNPLUGGED)) {
			/* we switched from unplugged to plugged, or the otherway around,
			   inform the system about it */
			power_supply_changed(&usb_psupply.ps);

			kobject_uevent(&usb_psupply.pdev->dev.kobj, KOBJ_CHANGE);
		}
	}
}

static irqreturn_t usbps_vbus_irq(int irq, void *pdev)
{
	if (delayed_work_pending(&usb_psupply.vbus_work))
		/* cancel the work, we will schedule it again with the full delay */
		cancel_delayed_work(&usb_psupply.vbus_work);

	/* to abolutely prevent frying a USB host port we disable fast charging
	   on receipt of VBUS events. usbps_vbus_event_handler() will take care to
	   enable it again if required */
	usbps_disable_fast_charging();

	/* timing isn't critical, we delay the execution of the event handler a
	   bit to give the USB driver time to activate the PHY and allow the
	   charger driver to set CHRG_DET */
	queue_delayed_work(usb_psupply.workqueue,
			   &usb_psupply.vbus_work,
			   msecs_to_jiffies(500));

	return IRQ_HANDLED;
}

#ifdef CONFIG_PM
static int usbps_suspend(struct platform_device *pdev, pm_message_t state)
{
	unsigned long flags;

	spin_lock_irqsave(&usb_psupply.lock, flags);

	cancel_delayed_work(&usb_psupply.vbus_work);

	spin_unlock_irqrestore(&usb_psupply.lock, flags);

	if (usb_psupply.state == USBPS_PLUGGED_TO_ACA) {
		/* on suspend we disable ACA fast charging to prevent frying
		   innocent USB ports while asleep */
		USBPS_DBG("going to suspend => switching to slow charge");

		usbps_disable_fast_charging();
	}

        return 0;
}

static int usbps_resume(struct platform_device *pdev)
{
	unsigned long flags;

	spin_lock_irqsave(&usb_psupply.lock, flags);

	/* USB port status could have changed, behave as if we had received
	   a VBUS interrupt */

	usb_psupply.state = USBPS_STATE_UNKNOWN;

	if (!delayed_work_pending(&usb_psupply.vbus_work))
		schedule_delayed_work(&usb_psupply.vbus_work, 0); /* no delay */

	spin_unlock_irqrestore(&usb_psupply.lock, flags);

	return 0;
}
#else
#define usbps_suspend NULL
#define usbps_resume  NULL
#endif /* CONFIG_PM */

static int usbps_probe(struct platform_device *pdev)
{
	int rc;
	struct usb_psupply_pdata *pdata = (struct usb_psupply_pdata *)pdev->dev.platform_data;

	usb_psupply.pdata = pdata;
	usb_psupply.pdev = pdev;

	USBPS_INFO(DRIVER_DESC_LONG);

	rc = gpio_request(pdata->gpio_fast_charge_en, "fast-charge-enable");
	if (rc) {
		USBPS_ERR("failed to request fast charge enable GPIO: %d", rc);
		goto err_gpio_fce;
	}

	/* disable fast charging initially */
	rc = gpio_direction_output(pdata->gpio_fast_charge_en, 0);
	if (rc) {
		USBPS_ERR("failed to configure fast charge enable GPIO: %d", rc);
		goto err_gpio_fce_cfg;
	}

	rc = gpio_request(pdata->gpio_vbus, "vbus-detect");
	if (rc) {
		USBPS_ERR("failed to request GPIO for VBUS detection: %d", rc);
		goto err_gpio_vbus_detect;
	}

	rc = gpio_request(pdata->gpio_chrg_det, "usb-chrg-det");
	if (rc) {
		USBPS_ERR("failed to request GPIO for USB charger detection: %d", rc);
		goto err_gpio_chrg_det;
	}

	usb_psupply.workqueue = create_singlethread_workqueue("usb_psupply");
	if (usb_psupply.workqueue == NULL) {
		USBPS_ERR("failed to create workqueue");
		goto err_workqueue;
	}

	INIT_DELAYED_WORK(&usb_psupply.vbus_work, usbps_vbus_event_handler);
	spin_lock_init(&usb_psupply.lock);

	/* determine the initial status of the USB port */
	usbps_vbus_event_handler(NULL);

	rc = request_irq(gpio_to_irq(pdata->gpio_vbus), usbps_vbus_irq,
	                IRQF_SHARED | IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING,
                        DRIVER_NAME, pdev);
	if (rc) {
		USBPS_ERR("failed to register IRQ %d for VBUS detection", gpio_to_irq(pdata->gpio_vbus));
		goto err_irq_vbus;
	}

	/* create sysfs attributes */
	rc = device_create_file(&pdev->dev, &dev_attr_state);
	if (rc) {
		USBPS_ERR("failed to create sysfs file for state attribute");
		goto err_sysfs_state;
	}

	rc = power_supply_register(&pdev->dev, &usb_psupply.ps);
	if (rc) {
		goto err_power_supply;
	}

	return 0;

err_power_supply:
	device_remove_file(&pdev->dev, &dev_attr_state);

err_sysfs_state:
	free_irq(gpio_to_irq(pdata->gpio_vbus), pdev);

err_irq_vbus:
	destroy_workqueue(usb_psupply.workqueue);

err_workqueue:
	gpio_free(pdata->gpio_chrg_det);

err_gpio_chrg_det:
	gpio_free(pdata->gpio_vbus);

err_gpio_vbus_detect:
err_gpio_fce_cfg:
	gpio_free(pdata->gpio_fast_charge_en);

err_gpio_fce:
	return -1;
}

static int usbps_remove(struct platform_device *pdev)
{
	struct usb_psupply_pdata *pdata = (struct usb_psupply_pdata *)pdev->dev.platform_data;

	power_supply_unregister(&usb_psupply.ps);

	device_remove_file(&pdev->dev, &dev_attr_state);

	free_irq(gpio_to_irq(pdata->gpio_vbus), pdev);

	usbps_disable_fast_charging();

	destroy_workqueue(usb_psupply.workqueue);

	gpio_free(pdata->gpio_chrg_det);

	gpio_free(pdata->gpio_vbus);

	gpio_free(pdata->gpio_fast_charge_en);

	return 0;
}

static int usbps_power_supply_get_property(struct power_supply *psy,
					enum power_supply_property psp,
					union power_supply_propval *val)
{
        int rc = 0;

        switch (psp) {
        case POWER_SUPPLY_PROP_ONLINE:
		if (usb_psupply.state != USBPS_UNPLUGGED)
			val->intval = 1;
		else
			val->intval = 0;

                break;

        default:
                rc = -EINVAL;
                break;
        }

        return rc;
}

static ssize_t usbps_sysfs_show_state(struct device *dev, struct device_attribute *attr, char *buf)
{
	snprintf(buf, PAGE_SIZE, "%d", usb_psupply.state);

	return strlen(buf) + 1;
}

static struct platform_driver usbps_driver = {
	.driver		= {
		.name	= DRIVER_NAME,
	},
	.probe		= usbps_probe,
	.remove		= usbps_remove,
	.suspend	= usbps_suspend,
        .resume		= usbps_resume,
};

static int __init usbps_init(void)
{
	int rc;

	USBPS_DBG("usbps_init");

	rc = platform_driver_register(&usbps_driver);
	if (rc)
		USBPS_ERR("failed to register platform driver (rc = %d)", rc);

	return rc;
}

static void __exit usbps_exit(void)
{
	platform_driver_unregister(&usbps_driver);
}

module_init(usbps_init);
module_exit(usbps_exit);

MODULE_DESCRIPTION(DRIVER_DESC_LONG);
MODULE_AUTHOR("Matthias Kaehlcke");
MODULE_LICENSE("GPL");

