/* arch/arm/mach-omap2/board-strasbourg_ps-psupply.c
 *
 * Copyright (C) 2008 Google, Inc.
 * Copyright (C) 2010 TomTom 
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */
#include <linux/module.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/types.h>
#include <linux/pci.h>
#include <linux/interrupt.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/gpio.h>
#include <linux/vgpio.h>
#include <linux/reboot.h>
#include <linux/fs.h>
#include <plat/offenburg.h>
#include <asm/io.h>

static int strasbourg_ps_bat_get_property(struct power_supply *psy,
				 enum power_supply_property psp,
				 union power_supply_propval *val);

static int strasbourg_ps_ac_get_property(struct power_supply *psy,
			enum power_supply_property psp,
			union power_supply_propval *val);

static struct platform_device *strasbourg_ps_pdev;
static struct wake_lock strasbourg_ps_wake_lock;
static struct workqueue_struct *poweroff_work_queue;

static enum power_supply_property strasbourg_ps_battery_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_TECHNOLOGY,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_CAPACITY,
};

static enum power_supply_property strasbourg_ps_ac_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static struct power_supply strasbourg_ps_bat = {
	.name = "battery",
	.type = POWER_SUPPLY_TYPE_BATTERY,
	.properties 	=  strasbourg_ps_battery_props,
	.num_properties = ARRAY_SIZE(strasbourg_ps_battery_props),
	.get_property	= strasbourg_ps_bat_get_property,
	.use_for_apm 	= 1,
};

static struct power_supply strasbourg_ps_ac = {
	.name = "ac",
	.type = POWER_SUPPLY_TYPE_MAINS,
	.properties 	=  strasbourg_ps_ac_props,
	.num_properties = ARRAY_SIZE(strasbourg_ps_ac_props),
	.get_property	= strasbourg_ps_ac_get_property,
};

static struct power_supply strasbourg_ps_usb = {
	.name = "usb",
	.type = POWER_SUPPLY_TYPE_MAINS,
	.properties 	=  strasbourg_ps_ac_props,
	.num_properties = ARRAY_SIZE(strasbourg_ps_ac_props),
	.get_property	= strasbourg_ps_ac_get_property,
};


static int strasbourg_ps_ac_get_property(struct power_supply *psy,
			enum power_supply_property psp,
			union power_supply_propval *val)
{
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = gpio_get_value (TT_VGPIO_ON_OFF) ? 1 : 0;
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static int strasbourg_ps_bat_get_property(struct power_supply *psy,
				 enum power_supply_property psp,
				 union power_supply_propval *val)
{
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = POWER_SUPPLY_STATUS_FULL;
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = POWER_SUPPLY_HEALTH_GOOD;
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = 1;
		break;
	case POWER_SUPPLY_PROP_TECHNOLOGY:
		val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = 100;
		break;
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = 20;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = 5;
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static void poweroff(struct work_struct *work)
{
	emergency_sync();
	kernel_power_off();
}
static DECLARE_WORK(poweroff_work, poweroff);

#define SYSTEM_OFF_TIMER_INTERVAL		msecs_to_jiffies(45000)
static struct timer_list system_off_timer;

extern void dump_all_active_wakelocks(void);

static void timer_callback(unsigned long arg)
{
	printk(KERN_INFO "timer_callback: Dumping active wakelocks:\n");

	dump_all_active_wakelocks();

	printk(KERN_INFO "SYSTEM_OFF Timer expired: Forcing power off\n");

	queue_work(poweroff_work_queue, &poweroff_work);
}

static void start_timer(void)
{
	init_timer(&system_off_timer);
	system_off_timer.expires = jiffies + SYSTEM_OFF_TIMER_INTERVAL;
	system_off_timer.function = timer_callback;
	system_off_timer.data =(unsigned long) &system_off_timer;
	add_timer(&system_off_timer);
}

static void stop_timer(void)
{
	del_timer(&system_off_timer);
}

static irqreturn_t strasbourg_ps_isr(int irq, void *dev_id)
{
	if (gpio_get_value (TT_VGPIO_ON_OFF)) {
		wake_lock (&strasbourg_ps_wake_lock);
		stop_timer();
	}
	else {
		start_timer();
		wake_unlock (&strasbourg_ps_wake_lock);
	}
	power_supply_changed(&strasbourg_ps_ac);

	return IRQ_HANDLED;
}

static int __init strasbourg_ps_init(void)
{
	int irq;	
	int ret = 0;	

	strasbourg_ps_pdev = platform_device_register_simple("strasbourg-power-supply", 0, NULL, 0);
	if (IS_ERR(strasbourg_ps_pdev)) {
		return PTR_ERR(strasbourg_ps_pdev);
	}
	ret = power_supply_register(&strasbourg_ps_pdev->dev, &strasbourg_ps_bat);
	if (ret) {
		goto bat_failed;
	}
	ret = power_supply_register(&strasbourg_ps_pdev->dev, &strasbourg_ps_ac);
	if (ret) {
		goto ac_failed;
	}
	ret = power_supply_register(&strasbourg_ps_pdev->dev, &strasbourg_ps_usb);
	if (ret) {
		goto usb_failed;
	}
	wake_lock_init(&strasbourg_ps_wake_lock, WAKE_LOCK_SUSPEND, "strasbourg-power-supply");

	ret = gpio_request(TT_VGPIO_ON_OFF, "ON_OFF") || gpio_direction_input (TT_VGPIO_ON_OFF);
	if (ret)
		goto gpio_alloc_failed;

	/* Take a wake lock if SYS_ON is high */
	if (gpio_get_value (TT_VGPIO_ON_OFF)) {
		wake_lock (&strasbourg_ps_wake_lock);
	}
	else {
		start_timer();
	}
	irq = gpio_to_irq(TT_VGPIO_ON_OFF);
	ret = request_irq(irq, strasbourg_ps_isr,
			  IRQF_SHARED |IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
			  "strasbourg-power-supply",
			   strasbourg_ps_pdev);
	if (ret) {
		goto request_irq_failed;
	}

	enable_irq_wake(irq);
	poweroff_work_queue = create_singlethread_workqueue("poweroff");
        if (poweroff_work_queue == NULL) {
                ret = -ENOMEM;
                goto poweroff_work_queue_create_failed;
        }

	printk(KERN_INFO "Strasbourg: power supply driver loaded\n");
	return 0;

poweroff_work_queue_create_failed:
	free_irq (gpio_to_irq(TT_VGPIO_ON_OFF), strasbourg_ps_pdev);
request_irq_failed:
	gpio_free (TT_VGPIO_ON_OFF);
gpio_alloc_failed:
	wake_lock_destroy(&strasbourg_ps_wake_lock);
usb_failed:
	power_supply_unregister(&strasbourg_ps_usb);
ac_failed:
	power_supply_unregister(&strasbourg_ps_ac);
bat_failed:
	power_supply_unregister(&strasbourg_ps_bat);
	platform_device_unregister(strasbourg_ps_pdev);
	return ret;
}

static void __exit strasbourg_ps_exit(void)
{
	free_irq (gpio_to_irq(TT_VGPIO_ON_OFF), strasbourg_ps_pdev);
	gpio_free (TT_VGPIO_ON_OFF);
	destroy_workqueue(poweroff_work_queue);
	wake_lock_destroy(&strasbourg_ps_wake_lock);
	power_supply_unregister(&strasbourg_ps_bat);
	power_supply_unregister(&strasbourg_ps_ac);
	power_supply_unregister(&strasbourg_ps_usb);
	platform_device_unregister(strasbourg_ps_pdev);
	printk(KERN_INFO "Strasbourg: power supply driver unloaded\n");
}

module_init(strasbourg_ps_init);
module_exit(strasbourg_ps_exit);
MODULE_AUTHOR("Oreste Salerno oreste.salerno@tomtom.com");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Strasbourg Power Supply Driver");

