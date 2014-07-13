/*
 * drivers/tomtom/power_button/power_button.c
 *
 * Driver for the power button device
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

#include <linux/gpio.h>
#include <linux/input.h>
#include <linux/keyboard.h>
#include <linux/notifier.h>
#include <linux/platform_device.h>
#include <linux/power_button.h>
#include <linux/sysfs.h>

#define PFX "power-button: "
#define DRIVER_DESC_LONG "TomTom Power Button driver, (C) 2010 TomTom BV "

#define PB_ERR(fmt, args...) \
	printk(KERN_ERR PFX "##ERROR## :" fmt "\n", ##args)

struct powerbutton_struct {
	/* current state of the button */
	int button_pressed;

	/* indicates if the button is continously pressed since system boot */
	int button_pressed_since_boot;

	/* used to register to keyboard events */
	struct notifier_block nb;

	/* platform data */
	struct powerbutton_pdata *pdata;
};

struct powerbutton_struct power_button;

static ssize_t pb_sysfs_show_pressed(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t pb_sysfs_show_pressed_since_boot(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t pb_sysfs_show_debounce_interval(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t pb_sysfs_store_debounce_interval(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);

/* define sysfs attributes */
static DEVICE_ATTR(pb_pressed, S_IRUGO, pb_sysfs_show_pressed, NULL);
static DEVICE_ATTR(pb_pressed_since_boot, S_IRUGO, pb_sysfs_show_pressed_since_boot, NULL);
static DEVICE_ATTR(pb_debounce_interval, 0644, pb_sysfs_show_debounce_interval, pb_sysfs_store_debounce_interval);

/* called by the keyboard subsystem when a key is pressed */
int pb_key_event(struct notifier_block *nb, unsigned long l, void *p)
{
	struct keyboard_notifier_param *ev_button = p;

	if (ev_button->value == power_button.pdata->keymap->code) {
		if (ev_button->down) {
			/* button pressed */

			power_button.button_pressed = 1;
		} else {
			/* button released */

			power_button.button_pressed = 0;
			power_button.button_pressed_since_boot = 0;
		}
	}

	return NOTIFY_DONE;
}

static int pb_probe(struct platform_device *pdev)
{
	int rc;
	int button_pressed;

	printk(DRIVER_DESC_LONG "\n");

	power_button.pdata = (struct powerbutton_pdata *)pdev->dev.platform_data;

	/* we read the value directly from the GPIO, as we might have missed a
	   keyboard event (gpio_keys starts earlier than we) */
	button_pressed = gpio_get_value(power_button.pdata->keymap->gpio);

	power_button.button_pressed = button_pressed;
	power_button.button_pressed_since_boot = button_pressed;

	/* register with keyboard events */
	power_button.nb.notifier_call = pb_key_event;
	rc = register_keyboard_notifier(&power_button.nb);
	if (rc) {
		PB_ERR("Failed to register for keyboard events\n");
		goto err_register_keyboard_notifier;
	}

	/* create sysfs attributes */
	rc = device_create_file(&pdev->dev, &dev_attr_pb_pressed);
	if (rc) {
		PB_ERR("Failed to create sysfs file for pressed attribute\n");
		goto err_sysfs_pressed;
	}

	rc = device_create_file(&pdev->dev, &dev_attr_pb_pressed_since_boot);
	if (rc) {
		PB_ERR("Failed to create sysfs file for pressed_since_boot attribute\n");
		goto err_sysfs_pressed_since_boot;
	}

	rc = device_create_file(&pdev->dev, &dev_attr_pb_debounce_interval);
	if (rc) {
		PB_ERR("Failed to create sysfs file for debounce_interval attribute\n");
		goto err_sysfs_debounce_interval;
	}

	return 0;

err_sysfs_debounce_interval:
	device_remove_file(&pdev->dev, &dev_attr_pb_pressed_since_boot);

err_sysfs_pressed_since_boot:
	device_remove_file(&pdev->dev, &dev_attr_pb_pressed);

err_sysfs_pressed:
	unregister_keyboard_notifier(&power_button.nb);

err_register_keyboard_notifier:

	return rc;
}

static int pb_remove(struct platform_device *pdev)
{
	unregister_keyboard_notifier(&power_button.nb);

	device_remove_file(&pdev->dev, &dev_attr_pb_pressed);
	device_remove_file(&pdev->dev, &dev_attr_pb_pressed_since_boot);
	device_remove_file(&pdev->dev, &dev_attr_pb_debounce_interval);

	return 0;
}

static struct platform_driver pb_driver = {
	.driver		= {
		.name	= "power-button", /* WARNING, the platform device is shared with drivers/tomtom/pb/pb.c! */
	},
	.probe		= pb_probe,
	.remove		= pb_remove,
};

static int __init pb_init(void)
{
	int rc = platform_driver_register(&pb_driver);
	if (rc)
		PB_ERR("Could not register pb platform driver! Error=%d", rc);

	return rc;
}

static void __exit pb_exit(void)
{
	platform_driver_unregister(&pb_driver);
}

static ssize_t pb_sysfs_show_pressed(struct device *dev, struct device_attribute *attr, char *buf)
{
	snprintf(buf, PAGE_SIZE, "%d", power_button.button_pressed);

	return strlen(buf) + 1;
}

static ssize_t pb_sysfs_show_pressed_since_boot(struct device *dev, struct device_attribute *attr, char *buf)
{
	snprintf(buf, PAGE_SIZE, "%d", power_button.button_pressed_since_boot);

	return strlen(buf) + 1;
}

static ssize_t pb_sysfs_show_debounce_interval(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct powerbutton_pdata *pdata = (struct powerbutton_pdata *)dev->platform_data;

	if (!pdata->get_debounce_interval)
		return 0;

	snprintf(buf, PAGE_SIZE, "%d", pdata->get_debounce_interval());

	return strlen(buf) + 1;
}

static ssize_t pb_sysfs_store_debounce_interval(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int value;
	struct powerbutton_pdata *pdata = (struct powerbutton_pdata *)dev->platform_data;

	if (sscanf(buf, "%u", &value) != 1) {
		dev_err(dev, "pb_sysfs_store_debounce_interval: Invalid value\n");
		return -EINVAL;
	}

	if (pdata->set_debounce_interval)
		pdata->set_debounce_interval(value);

	return count;
}

module_init(pb_init);
module_exit(pb_exit);

MODULE_DESCRIPTION(DRIVER_DESC_LONG);
MODULE_AUTHOR("Matthias Kaehlcke");
MODULE_LICENSE("GPL");
