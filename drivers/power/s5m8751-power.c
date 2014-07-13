/*
 * s5m8751-battery.c  --  S5M8751 Power-Audio IC ALSA Soc Audio driver
 *
 * Copyright 2009 Samsung Electronics.
 *
 * Author: Jongbin Won <jongbin.won@samsung.com>
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/module.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/slab.h>
#include <asm/irq.h>
#include <plat/adc.h>

#include <mach/regs-irq.h>
#include <mach/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <mach/gpio.h>
#include <plat/gpio-bank-n.h>


#include <linux/mfd/s5m8751/core.h>
#include <linux/mfd/s5m8751/power.h>
#include <linux/mfd/s5m8751/pdata.h>
#include <mach/regs-clock.h>
#include <asm/io.h>
#include <linux/delay.h>

#if defined(CONFIG_MACH_S5P6450_TOMTOM)
#include "valdez_battery.h"
#endif

struct s3c_adc_client *client;
#if defined(CONFIG_MACH_S5P6450_TOMTOM)
#define 	BATT_MON_CHANNEL  	1
#define		USB_ID_CHANNEL		3
#else
#define 	BATT_MON_CHANNEL 	0
#endif

#define	S5M8751_CHARGE_CURRENT_LOW	400
/* limit fast charge current to 400mA until charger detection works reliably */
#define	S5M8751_CHARGE_CURRENT_HIGH	401 /* 800 */

struct power_report
{
	int status;
	int online;
	int voltage_level;
	int capacity;
};

struct s5m8751_power
{
	struct s5m8751 *s5m8751;
	struct power_supply wall;
	struct power_supply usb;
	struct power_supply battery;
	struct power_report prev_report;
	struct power_report report;
	unsigned int gpio_hadp_lusb;
	struct delayed_work s5m8751_power_monitor_work;
};

static struct s5m8751_power *s5m8751_power;
static struct s5m8751 *s5m8751_chg;

static void s5m8751_power_battery_update_status(struct s5m8751_power *);

static int s5m8751_wall_get_prop(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct s5m8751_power *power = dev_get_drvdata(psy->dev->parent);
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = (power->prev_report.online == POWER_SUPPLY_TYPE_MAINS ? 1 : 0); 
		break;
	default:
		ret = -EINVAL;
	break;
	}

	return ret;
}

static enum power_supply_property s5m8751_wall_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static int s5m8751_usb_get_prop(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct s5m8751_power *power = dev_get_drvdata(psy->dev->parent);
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = (power->prev_report.online == POWER_SUPPLY_TYPE_USB ? 1 : 0); 
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static enum power_supply_property s5m8751_usb_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

int s5m8751_config_battery(struct s5m8751 *s5m8751)
{
	struct s5m8751_pdata *s5m8751_pdata = s5m8751->dev->platform_data;
	struct s5m8751_power_pdata *pdata;
	uint8_t reg_val, constant_voltage;

	if (!s5m8751_pdata || !s5m8751_pdata->power) {
		dev_warn(s5m8751->dev,
			"No charger platform data, charger not configured.\n");
		return -EINVAL;
	}
	pdata = s5m8751_pdata->power;

	if ((pdata->constant_voltage < 4100) ||
	    (pdata->constant_voltage > 4275)) {
		dev_err(s5m8751->dev, "Constant voltage is out of range!\n");
		return -EINVAL;
	}
	constant_voltage = S5M8751_CHG_CONT_VOLTAGE(pdata->constant_voltage);
	s5m8751_reg_read(s5m8751, S5M8751_CHG_IV_SET, &reg_val);
	reg_val &= ~S5M8751_CONSTANT_VOLTAGE_MASK;
	s5m8751_reg_write(s5m8751, S5M8751_CHG_IV_SET, reg_val | constant_voltage);

	return 0;
}

static void s5m8751_set_charge_current_limit(int current_limit)
{
	struct s5m8751_pdata *s5m8751_pdata = s5m8751_chg->dev->platform_data;
	struct s5m8751_power_pdata *pdata = s5m8751_pdata->power;
	uint8_t reg_val;

	s5m8751_reg_read(s5m8751_chg, S5M8751_CHG_IV_SET, &reg_val);
	reg_val &= ~S5M8751_FAST_CHG_CURRENT_MASK;
	s5m8751_reg_write(s5m8751_chg, S5M8751_CHG_IV_SET,
			  reg_val | S5M8751_FAST_CHG_CURRENT(current_limit));

	switch (current_limit) {
		case S5M8751_CHARGE_CURRENT_LOW:
			dev_dbg(s5m8751_chg->dev, "Setting current current limit to %dmA\n", current_limit);
			gpio_set_value(pdata->gpio_hadp_lusb, 0);
			break;

		case S5M8751_CHARGE_CURRENT_HIGH:
			dev_dbg(s5m8751_chg->dev, "Setting current current limit to %dmA\n", current_limit);
			gpio_set_value(pdata->gpio_hadp_lusb, 1);
			break;

		default:
			dev_err(s5m8751_chg->dev, "Unsupported current limit %dmA\n", current_limit);
	}
}

/*
 * s5m8751_set_charging_mode function puts the battery driver into required
 * charging mode.
 */
void s5m8751_set_charger_type(int charger_type)
{
	if ((s5m8751_chg->dev->power.status == DPM_ON) ||
	    (s5m8751_chg->dev->power.status == DPM_RESUMING)) {
		/* really set charger type when fully alive or resuming */

		switch (charger_type)
		{
			case POWER_SUPPLY_TYPE_USB:
				dev_info(s5m8751_chg->dev, "Power supply: USB\n");
				s5m8751_power->report.status = POWER_SUPPLY_STATUS_CHARGING;
				s5m8751_power->report.online = POWER_SUPPLY_TYPE_USB;
				break;

			case POWER_SUPPLY_TYPE_MAINS:
				dev_info(s5m8751_chg->dev, "Power supply: AC\n");
				s5m8751_power->report.status = POWER_SUPPLY_STATUS_CHARGING;
				s5m8751_power->report.online = POWER_SUPPLY_TYPE_MAINS;
				break;

			case POWER_SUPPLY_TYPE_BATTERY:
				dev_info(s5m8751_chg->dev, "Power supply: none\n");
				s5m8751_power->report.status = POWER_SUPPLY_STATUS_DISCHARGING;
				s5m8751_power->report.online = POWER_SUPPLY_TYPE_BATTERY;
				break;

			default:
				dev_err(s5m8751_chg->dev, "Unsupported charger type: %d\n", charger_type);
		}

		if (charger_type == POWER_SUPPLY_TYPE_MAINS)
			s5m8751_set_charge_current_limit(S5M8751_CHARGE_CURRENT_HIGH);
		else
			s5m8751_set_charge_current_limit(S5M8751_CHARGE_CURRENT_LOW);

		/* update status of power supplies */
		s5m8751_power_battery_update_status(s5m8751_power);

		/* (re-)start monitoring of battery level */
		schedule_delayed_work(&s5m8751_power->s5m8751_power_monitor_work, 0);
	} else {
		/* we are in (going to) some low power state, just take note of
		   the change of the charger type and handle it upon resume */
		s5m8751_power->report.online = charger_type;
	}
}

#ifdef CONFIG_TOMTOM_USB_MODE_LITE
EXPORT_SYMBOL(s5m8751_set_charger_type);
#endif

static int s5m8751_bat_get_prop(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct s5m8751_power *power = dev_get_drvdata(psy->dev->parent);
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = power->prev_report.status;
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
		val->intval = power->prev_report.capacity;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = power->prev_report.voltage_level;
                break;
        case POWER_SUPPLY_PROP_TEMP:
		val->intval = 0;
                break;

	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static enum power_supply_property s5m8751_bat_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_HEALTH,
        POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
        POWER_SUPPLY_PROP_TECHNOLOGY,
        POWER_SUPPLY_PROP_CAPACITY,
};

static irqreturn_t s5m8751_charger_handler(int irq, void *data)
{
	struct s5m8751_power *s5m8751_power = data;
	struct s5m8751 *s5m8751 = s5m8751_power->s5m8751;

	switch (irq - s5m8751->irq_base) {
		case S5M8751_IRQ_PWRKEY1B:
			break;
		case S5M8751_IRQ_VCHG_DETECTION:
			dev_dbg(s5m8751->dev, "Charger source inserted\n");
			break;
		case S5M8751_IRQ_VCHG_REMOVAL:
			dev_dbg(s5m8751->dev, "Charger source removal\n");
			break;
		case S5M8751_IRQ_VCHG_TIMEOUT:
			dev_info(s5m8751->dev, "Charger timeout\n");
			break;
		default:
			dev_info(s5m8751->dev, "Unknown IRQ source\n");
			break;
	}

	s5m8751_reg_write(s5m8751, S5M8751_IRQB_EVENT1, 0x00);
	s5m8751_reg_write(s5m8751, S5M8751_IRQB_EVENT2, 0x00);

	return IRQ_HANDLED;
}

#define REQUEST_IRQ(_irq, _name)					\
do {									\
	ret = request_threaded_irq(s5m8751->irq_base + _irq, NULL,	\
				    s5m8751_charger_handler,		\
				    IRQF_ONESHOT, _name, power);	\
	if (ret)							\
		dev_err(s5m8751->dev, "Fail to request IRQ #%d: %d\n",	\
			_irq, ret);					\
} while (0)

static __devinit int s5m8751_init_charger(struct s5m8751 *s5m8751,
						struct s5m8751_power *power)
{
	int ret;
	uint8_t value = 0;

	REQUEST_IRQ(S5M8751_IRQ_PWRKEY1B, "power key 1");
	REQUEST_IRQ(S5M8751_IRQ_VCHG_DETECTION, "chg detection");
	REQUEST_IRQ(S5M8751_IRQ_VCHG_REMOVAL, "chg removal");
	REQUEST_IRQ(S5M8751_IRQ_VCHG_TIMEOUT, "chg timeout");

       	s5m8751_reg_read(s5m8751, S5M8751_IRQB_EVENT2, &value);

	s5m8751_reg_write(s5m8751, S5M8751_IRQB_EVENT1, 0x00);
        s5m8751_reg_write(s5m8751, S5M8751_IRQB_EVENT2, 0x00);

        if (value == S5M8751_IRQB_VCHG_DET) {
		/* some charger source attached */

#ifndef CONFIG_TOMTOM_USB_MODE_LITE
		s5m8751_set_charger_type(POWER_SUPPLY_TYPE_USB);
#endif
	} else if (value == S5M8751_IRQB_VCHG_REM) {
		s5m8751_set_charger_type(POWER_SUPPLY_TYPE_BATTERY);
	}

	return 0;
};

static __devexit int s5m8751_exit_charger(struct s5m8751_power *power)
{
	struct s5m8751 *s5m8751 = power->s5m8751;
	int irq;

	irq = s5m8751->irq_base + S5M8751_IRQ_VCHG_DETECTION;

	for (; irq <= s5m8751->irq_base + S5M8751_IRQ_VCHG_TIMEOUT; irq++)
		free_irq(irq, power);

	return 0;
}

/* This function checks if changes occured in the battery status. If so     */
/* then the power_supply driver is kicked so the new data will be retreived */
static void s5m8751_power_battery_update_status(struct s5m8751_power *power)
{
#if defined(CONFIG_MACH_S5P6450_TOMTOM)
	battery_calc_add_adc(s3c_adc_read(client, BATT_MON_CHANNEL),
		(power->report.status == POWER_SUPPLY_STATUS_DISCHARGING ? 0 : 1),
		&power->report.voltage_level, &power->report.capacity);
	if ((power->report.status == POWER_SUPPLY_STATUS_CHARGING) &&
		(power->report.capacity == 100)) {
		power->report.status = POWER_SUPPLY_STATUS_FULL;
	}
#else
	power->report.voltage_level = s3c_adc_read(client, BATT_MON_CHANNEL) * 1129;
	power->report.capacity = (power->report.voltage_level - 3400000) / 8000;
	printk("Brute calculations of voltage/level = %d/%d\n",
		(power->report.voltage_level / 1000), power->report.capacity);
#endif
	if ((power->report.voltage_level != power->prev_report.voltage_level) ||
	    (power->report.status != power->prev_report.status)) {
		power->prev_report = power->report;
		power_supply_changed(&power->battery);
	}
	if (power->report.online != power->prev_report.online) {
		power->prev_report = power->report;
		power_supply_changed(&power->wall);
		power_supply_changed(&power->usb);
	}
}

/* Work routine which reschedules itself for 100 jiffies. This is worker    */
/* thread which can be scheduled any desired time (like cable_status_update */
static void s5m8751_power_battery_work(struct work_struct *work)
{
	struct s5m8751_power *power = container_of(work,
		struct s5m8751_power, s5m8751_power_monitor_work.work);

	s5m8751_power_battery_update_status(power);
	schedule_delayed_work(&power->s5m8751_power_monitor_work, 100);
}

static __devinit int s5m8751_power_probe(struct platform_device *pdev)
{
	struct s5m8751 *s5m8751 = dev_get_drvdata(pdev->dev.parent);
	struct s5m8751_pdata *s5m8751_pdata = s5m8751->dev->platform_data;
	struct s5m8751_power_pdata *pdata = s5m8751_pdata->power;
	struct s5m8751_power *power;
	struct power_supply *usb;
	struct power_supply *battery;
	struct power_supply *wall;
	int ret;

	// Register to ADC driver:
        client = s3c_adc_register(pdev,NULL,NULL,0);

        if (!client)
		dev_err(s5m8751->dev, "Fail to Register ADC driver\n");

	power = kzalloc(sizeof(struct s5m8751_power), GFP_KERNEL);
	if (power == NULL)
		return -ENOMEM;

	power->s5m8751 = s5m8751;
	power->gpio_hadp_lusb = pdata->gpio_hadp_lusb;
	platform_set_drvdata(pdev, power);
	s5m8751_power = power;

	usb = &power->usb;
	battery = &power->battery;
	wall = &power->wall;

	s5m8751_chg = s5m8751;

	s5m8751_config_battery(s5m8751);

	ret = gpio_request(pdata->gpio_hadp_lusb, "lusb_hadp");
	if (ret) {
		printk("Failed to request gpio for limiting charge current %d\n", pdata->gpio_hadp_lusb);
		goto err_kmalloc;
	}

	/* disable high power charging initially */
	gpio_direction_output(pdata->gpio_hadp_lusb, 0);

	wall->name = "ac";
	wall->type = POWER_SUPPLY_TYPE_MAINS;
	wall->properties = s5m8751_wall_props;
	wall->num_properties = ARRAY_SIZE(s5m8751_wall_props);
	wall->get_property = s5m8751_wall_get_prop;
	ret = power_supply_register(&pdev->dev, wall);
	if (ret)
		goto err_gpio;

	battery->name = "battery";
	battery->properties = s5m8751_bat_props;
	battery->num_properties = ARRAY_SIZE(s5m8751_bat_props);
	battery->get_property = s5m8751_bat_get_prop;
	ret = power_supply_register(&pdev->dev, battery);
	if (ret)
		goto err_wall;

	usb->name = "usb",
	usb->type = POWER_SUPPLY_TYPE_USB;
	usb->properties = s5m8751_usb_props;
	usb->num_properties = ARRAY_SIZE(s5m8751_usb_props);
	usb->get_property = s5m8751_usb_get_prop;
	ret = power_supply_register(&pdev->dev, usb);
	if (ret)
		goto err_battery;

#if defined(CONFIG_MACH_S5P6450_TOMTOM)
	battery_calc_init(pdev);
#endif

	INIT_DELAYED_WORK_DEFERRABLE(&power->s5m8751_power_monitor_work,
			s5m8751_power_battery_work);
	schedule_delayed_work(&power->s5m8751_power_monitor_work, 500);

	s5m8751_init_charger(s5m8751, power);

	return ret;

err_battery:
	power_supply_unregister(battery);
err_wall:
	power_supply_unregister(wall);
err_gpio:
	gpio_free(pdata->gpio_hadp_lusb);
err_kmalloc:
	kfree(power);
	return ret;
}

static int s5m8751_power_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct s5m8751_power *s5m8751_power = platform_get_drvdata(pdev);
	struct s5m8751 *s5m8751 = dev_get_drvdata(pdev->dev.parent);

	cancel_delayed_work(&s5m8751_power->s5m8751_power_monitor_work);

	/* limit charge current to prevent damaging USB host ports */
	s5m8751_set_charge_current_limit(S5M8751_CHARGE_CURRENT_LOW);

        s5m8751_reg_write(s5m8751, S5M8751_IRQB_EVENT1, 0x00);
        s5m8751_reg_write(s5m8751, S5M8751_IRQB_EVENT2, 0x00);
        /* mask powerbutton events coming from PMIC */
        s5m8751_reg_write(s5m8751, S5M8751_IRQB_MASK1, 0x0E);
        /* don't wake up on cable removal */
        s5m8751_reg_write(s5m8751, S5M8751_IRQB_MASK2, 0x08);

        return 0;
}

static int s5m8751_power_resume(struct platform_device *pdev)
{
	struct s5m8751_power *s5m8751_power = platform_get_drvdata(pdev);
	struct s5m8751 *s5m8751 = dev_get_drvdata(pdev->dev.parent);
	int8_t value;

	schedule_delayed_work(&s5m8751_power->s5m8751_power_monitor_work, 100);

        s5m8751_reg_read(s5m8751, S5M8751_IRQB_EVENT2, &value);
        if (value == S5M8751_IRQB_VCHG_DET) {
#ifdef CONFIG_TOMTOM_USB_MODE_LITE
		if (s5m8751_power->report.online != s5m8751_power->prev_report.online)
			/* charger type changed during resume => make the update effective */
			s5m8751_set_charger_type(s5m8751_power->report.online);
#else
		s5m8751_set_charger_type(POWER_SUPPLY_TYPE_USB);
#endif
	} else if (value == S5M8751_IRQB_VCHG_DET) {
		s5m8751_set_charger_type(POWER_SUPPLY_TYPE_BATTERY);
	}

	s5m8751_reg_write(s5m8751, S5M8751_IRQB_EVENT1, 0x00);
        s5m8751_reg_write(s5m8751, S5M8751_IRQB_EVENT2, 0x00);
        /* enable cable removal irq */
        s5m8751_reg_write(s5m8751, S5M8751_IRQB_MASK2, 0x0);

        return 0;
}


static __devexit int s5m8751_power_remove(struct platform_device *pdev)
{
	struct s5m8751_power *s5m8751_power = platform_get_drvdata(pdev);
	struct s5m8751 *s5m8751 = dev_get_drvdata(pdev->dev.parent);
	struct s5m8751_pdata *s5m8751_pdata = s5m8751->dev->platform_data;
	struct s5m8751_power_pdata *pdata = s5m8751_pdata->power;

	power_supply_unregister(&s5m8751_power->battery);
	power_supply_unregister(&s5m8751_power->wall);
	power_supply_unregister(&s5m8751_power->usb);

	gpio_free(pdata->gpio_hadp_lusb);

	s5m8751_exit_charger(s5m8751_power);

	return 0;
}

static struct platform_driver s5m8751_power_driver =
{
	.driver	= {
		.name	= "s5m8751-power",
		.owner	= THIS_MODULE,
	},
	.probe = s5m8751_power_probe,
	.suspend = s5m8751_power_suspend,
        .resume =  s5m8751_power_resume,
	.remove = __devexit_p(s5m8751_power_remove),
};

static int __init s5m8751_power_init(void)
{
	return platform_driver_register(&s5m8751_power_driver);
}
module_init(s5m8751_power_init);

static void __exit s5m8751_power_exit(void)
{
	platform_driver_unregister(&s5m8751_power_driver);
}
module_exit(s5m8751_power_exit);

MODULE_DESCRIPTION("Power supply driver for S5M8751");
MODULE_AUTHOR("Jongbin,Won <jognbin.won@samsung.com>");
MODULE_LICENSE("GPL");
