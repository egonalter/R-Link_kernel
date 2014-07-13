/*
 * s5m8752-power.c  --  S5M8752 Power-Audio IC ALSA Soc Audio driver
 *
 * Copyright 2010 Samsung Electronics.
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
#include <linux/interrupt.h>

#include <asm/irq.h>
#include <mach/gpio.h>
#include <plat/gpio-cfg.h>

#include <linux/mfd/s5m8752/core.h>
#include <linux/mfd/s5m8752/power.h>
#include <linux/mfd/s5m8752/pdata.h>

static struct wake_lock vbus_wake_lock;

struct s5m8752_power {
	struct s5m8752 *s5m8752;
	struct power_supply wall;
	struct power_supply usb;
	struct power_supply battery;
	unsigned int gpio_hadp_lusb;
};

static int s3c_cable_status_update(int );
struct power_supply *battery;

typedef enum {
        CHARGER_BATTERY = 0,
        CHARGER_USB,
        CHARGER_AC,
        CHARGER_DISCHARGE
} charger_type_t;


/******************************************************************************
 * This function checks whether external power(Wall adapter or USB) is
 * inserted or not.
 ******************************************************************************/
static int s5m8752_power_check_online(struct s5m8752 *s5m8752, int supply,
					union power_supply_propval *val)
{
	int ret;
	uint8_t value;

	ret = s5m8752_reg_read(s5m8752, S5M8752_CHG_TEST4, &value);
	if (ret < 0)
		return ret;

	value &= S5M8752_CHG_CHGDET_MASK;

	switch (value >> S5M8752_CHG_CHGDET_SHIFT) {
	case S5M8752_EXT_PWR_DETECTED:
		val->intval = 1;
		break;
	case S5M8752_EXT_PWR_NOT_DETECTED:
		val->intval = 0;
		break;
	}

	return 0;
}

/******************************************************************************
 * This function is S5M8752 WALL adapter property.
 ******************************************************************************/
static int s5m8752_wall_get_prop(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct s5m8752_power *s5m8752_power =
					dev_get_drvdata(psy->dev->parent);
	struct s5m8752 *s5m8752 = s5m8752_power->s5m8752;
	int ret;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		ret = s5m8752_power_check_online(s5m8752, S5M8752_PWR_WALL,
									val);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static enum power_supply_property s5m8752_wall_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

/******************************************************************************
 * This function is S5M8752 WALL adapter property.
 ******************************************************************************/
static int s5m8752_usb_get_prop(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct s5m8752_power *s5m8752_power =
					dev_get_drvdata(psy->dev->parent);
	struct s5m8752 *s5m8752 = s5m8752_power->s5m8752;
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		ret = s5m8752_power_check_online(s5m8752, S5M8752_PWR_USB,
									val);
		break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static enum power_supply_property s5m8752_usb_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

/******************************************************************************
 * This function is the gpio information of S5M8752 battery charger.
 * External power
 *	WALL adapter : gpio_adp = high
 *	USB : gpio_adp = low
 *
 * Current limit control
 *	5X mode		: gpio_ilim1 = 0, gpio_ilim0 = 0
 *	suspend mode	: gpio_ilim1 = 0, gpio_ilim0 = 1
 *	15X mode	: gpio_ilim1 = 1, gpio_ilim0 = 0
 *	1X mode		: gpio_ilim1 = 1, gpio_ilim1 = 1
 ******************************************************************************/
int s5m8752_config_battery(struct s5m8752 *s5m8752)
{
	struct s5m8752_pdata *s5m8752_pdata = s5m8752->dev->platform_data;
	struct s5m8752_power_pdata *pdata;

	if (!s5m8752_pdata || !s5m8752_pdata->power) {
		dev_warn(s5m8752->dev,
			"No charger platform data, charger not configured.\n");
		return -EINVAL;
	}
	pdata = s5m8752_pdata->power;

	s3c_gpio_cfgpin(pdata->gpio_adp, S3C_GPIO_OUTPUT);
	s3c_gpio_cfgpin(pdata->gpio_ilim0, S3C_GPIO_OUTPUT);
	s3c_gpio_cfgpin(pdata->gpio_ilim1, S3C_GPIO_OUTPUT);

	if (pdata->charger_type)
		gpio_set_value(pdata->gpio_adp, 1);
	else
		gpio_set_value(pdata->gpio_adp, 0);

	if (pdata->current_limit == 2) {
		gpio_set_value(pdata->gpio_ilim1, 1);
		gpio_set_value(pdata->gpio_ilim0, 0);
	} else if (pdata->current_limit == 1) {
		gpio_set_value(pdata->gpio_ilim1, 0);
		gpio_set_value(pdata->gpio_ilim0, 1);
	} else if (pdata->current_limit == 0) {
		gpio_set_value(pdata->gpio_ilim1, 0);
		gpio_set_value(pdata->gpio_ilim0, 0);
	} else {
		gpio_set_value(pdata->gpio_ilim1, 1);
		gpio_set_value(pdata->gpio_ilim0, 1);
	}
	return 0;
}

/******************************************************************************
 * This function checks the status whether S5M8752 battery charging or
 * discharging.
 ******************************************************************************/
static int s5m8752_bat_check_status(struct s5m8752 *s5m8752, int *status)
{
	int ret;
	uint8_t val;

	ret = s5m8752_reg_read(s5m8752, S5M8752_CHG_TEST5, &val);
	if (ret < 0)
		return ret;

	if (val & S5M8752_CHG_EOC_MASK) {
		*status = POWER_SUPPLY_STATUS_FULL;
		return 0;
	}

	ret = s5m8752_reg_read(s5m8752, S5M8752_CHG_TEST3, &val);
	if (ret < 0)
		return ret;

	switch (val & S5M8752_CHG_STATUS_MASK) {
	case S5M8752_CHG_STATE_OFF:
		*status = POWER_SUPPLY_STATUS_NOT_CHARGING;
		break;
	case S5M8752_CHG_STATE_ON:
		*status = POWER_SUPPLY_STATUS_CHARGING;
		break;
	default:
		*status = POWER_SUPPLY_STATUS_UNKNOWN;
	}

	return 0;
}

/******************************************************************************
 * This function checks the charging mode of S5M8752 battery charger.
 ******************************************************************************/
static int s5m8752_bat_check_type(struct s5m8752 *s5m8752, int *type)
{
	int ret;
	uint8_t val;

	ret = s5m8752_reg_read(s5m8752, S5M8752_CHG_TEST3, &val);
	if (ret < 0)
		return ret;

	if (!(val & S5M8752_CHG_STATUS_MASK)) {
		*type = POWER_SUPPLY_CHARGE_TYPE_NONE;
		return 0;
	}

	ret = s5m8752_reg_read(s5m8752, S5M8752_CHG_TEST4, &val);
	if (ret < 0)
		return ret;

	switch (val & S5M8752_CHG_TYPE_MASK) {
	case S5M8752_CHG_STATE_TRICKLE:
		*type = POWER_SUPPLY_CHARGE_TYPE_TRICKLE;
		break;
	case S5M8752_CHG_STATE_FAST:
		*type = POWER_SUPPLY_CHARGE_TYPE_FAST;
		break;
	}

	return 0;
}

/******************************************************************************
 * This function is S5M8752 battery property.
 ******************************************************************************/
static int s5m8752_bat_get_prop(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct s5m8752_power *s5m8752_power =
					dev_get_drvdata(psy->dev->parent);
	struct s5m8752 *s5m8752 = s5m8752_power->s5m8752;
	int ret = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		ret = s5m8752_bat_check_status(s5m8752, &val->intval);
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		ret = s5m8752_bat_check_type(s5m8752, &val->intval);
		break;
	case POWER_SUPPLY_PROP_HEALTH:
//                val->intval = s3c_get_bat_health();
		return 0;           
                break;
        case POWER_SUPPLY_PROP_PRESENT:
//                val->intval = s3c_bat_info.present;
		return 0;           
                break;
        case POWER_SUPPLY_PROP_TECHNOLOGY:
//                val->intval = POWER_SUPPLY_TECHNOLOGY_LION;
		return 0;           
                break;
        case POWER_SUPPLY_PROP_CAPACITY:
//                val->intval = s3c_bat_info.bat_info.level;
		return 0;           
                break;
        case POWER_SUPPLY_PROP_TEMP:
//                val->intval = s3c_bat_info.bat_info.batt_temp;
//                dev_dbg(bat_ps->dev, "%s : temp = %d\n", __func__,
  //                              val->intval);
		return 0;           
                break;
	default:
		ret = -EINVAL;
		break;
	}

	return ret;
}

static enum power_supply_property s5m8752_bat_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
        POWER_SUPPLY_PROP_HEALTH,
        POWER_SUPPLY_PROP_PRESENT,
        POWER_SUPPLY_PROP_TECHNOLOGY,
        POWER_SUPPLY_PROP_CAPACITY,


};

/******************************************************************************
 * This function is S5M8752 charger interrupt service routine.
 ******************************************************************************/
static irqreturn_t s5m8752_charger_handler(int irq, void *data)
{
	struct s5m8752_power *s5m8752_power = data;
	struct s5m8752 *s5m8752 = s5m8752_power->s5m8752;
	

	switch (irq - s5m8752->irq_base) {
	case S5M8752_IRQ_VCHG_DET:
		s3c_cable_status_update(1);
		dev_info(s5m8752->dev, "Charger source inserted\n");
		break;
	case S5M8752_IRQ_VCHG_REM:
		s3c_cable_status_update(0);
		dev_info(s5m8752->dev, "Charger source removal\n");
		break;
	case S5M8752_IRQ_CHG_TOUT:
		dev_info(s5m8752->dev, "Charger timeout\n");
		break;
	case S5M8752_IRQ_CHG_EOC:
		dev_info(s5m8752->dev, "The end of Charging\n");
		break;
	default:
		dev_info(s5m8752->dev, "Unknown IRQ source\n");
		break;
	}

	s5m8752_reg_write(s5m8752, S5M8752_IRQB_EVENT1, 0x00);
	s5m8752_reg_write(s5m8752, S5M8752_IRQB_EVENT2, 0x00);

	return IRQ_HANDLED;
}

#define REQUEST_IRQ(_irq, _name)					\
do {									\
	ret = request_threaded_irq(s5m8752->irq_base + _irq, NULL,	\
				    s5m8752_charger_handler,		\
				    IRQF_ONESHOT, _name, power);	\
	if (ret)							\
		dev_err(s5m8752->dev, "Fail to request IRQ #%d: %d\n",	\
			_irq, ret);					\
} while (0)

static __devinit int s5m8752_init_charger(struct s5m8752 *s5m8752,
						struct s5m8752_power *power)
{
	int ret;
	REQUEST_IRQ(S5M8752_IRQ_VCHG_DET, "chg detection");
	REQUEST_IRQ(S5M8752_IRQ_VCHG_REM, "chg removal");
	REQUEST_IRQ(S5M8752_IRQ_CHG_TOUT, "chg timeout");
	REQUEST_IRQ(S5M8752_IRQ_CHG_EOC, "end of charging");

	return 0;
};

static __devexit int s5m8752_exit_charger(struct s5m8752_power *power)
{
	struct s5m8752 *s5m8752 = power->s5m8752;
	int irq;

	irq = s5m8752->irq_base + S5M8752_IRQ_VCHG_DET;

	for (; irq <= s5m8752->irq_base + S5M8752_IRQ_CHG_EOC; irq++)
		free_irq(irq, power);

	return 0;
}

static __devinit int s5m8752_power_probe(struct platform_device *pdev)
{
	struct s5m8752 *s5m8752 = dev_get_drvdata(pdev->dev.parent);
	struct s5m8752_power *power;
	struct power_supply *usb;
//	struct power_supply *battery;
	struct power_supply *wall;
	int ret;
	int8_t value;       

	power = kzalloc(sizeof(struct s5m8752_power), GFP_KERNEL);
	if (power == NULL)
		return -ENOMEM;

	power->s5m8752 = s5m8752;
	platform_set_drvdata(pdev, power);

	usb = &power->usb;
	battery = &power->battery;
	wall = &power->wall;

	s5m8752_config_battery(s5m8752);

//	wall->name = "s5m8752-wall";
	wall->name = "ac";
	wall->type = POWER_SUPPLY_TYPE_MAINS;
	wall->properties = s5m8752_wall_props;
	wall->num_properties = ARRAY_SIZE(s5m8752_wall_props);
	wall->get_property = s5m8752_wall_get_prop;
	ret = power_supply_register(&pdev->dev, wall);
	if (ret)
		goto err_kmalloc;

//	battery->name = "s5m8752-battery";
	battery->name = "battery";
	battery->properties = s5m8752_bat_props;
	battery->num_properties = ARRAY_SIZE(s5m8752_bat_props);
	battery->get_property = s5m8752_bat_get_prop;
	ret = power_supply_register(&pdev->dev, battery);
	if (ret)
		goto err_wall;

	//usb->name = "s5m8752-usb",
	usb->name = "usb",
	usb->type = POWER_SUPPLY_TYPE_USB;
	usb->properties = s5m8752_usb_props;
	usb->num_properties = ARRAY_SIZE(s5m8752_usb_props);
	usb->get_property = s5m8752_usb_get_prop;
	ret = power_supply_register(&pdev->dev, usb);
	if (ret)
		goto err_battery;

	s5m8752_init_charger(s5m8752, power);

	s5m8752_reg_read(s5m8752, S5M8752_CHG_TEST3, &value);

        if(value == 13){
                s3c_cable_status_update(1);
        }
	
return ret;

err_battery:
	power_supply_unregister(battery);
err_wall:
	power_supply_unregister(wall);
err_kmalloc:
	kfree(power);
	return ret;
}

static int s3c_cable_status_update(int status)
{
        int ret = 0;
        charger_type_t source = CHARGER_BATTERY;
	source = status;
        //dev_dbg(dev, "%s\n", __func__);

        //if(!s3c_battery_initial)
          //      return -EPERM;

        switch(status) {
        case CHARGER_BATTERY:
                //dev_dbg(dev, "%s : cable NOT PRESENT\n", __func__);
      //          s3c_bat_info.bat_info.charging_source = CHARGER_BATTERY;
                break;
        case CHARGER_USB:
          //      dev_dbg(dev, "%s : cable USB\n", __func__);
    //            s3c_bat_info.bat_info.charging_source = CHARGER_USB;
                break;
        case CHARGER_AC:
            //    dev_dbg(dev, "%s : cable AC\n", __func__);
  //              s3c_bat_info.bat_info.charging_source = CHARGER_AC;
                break;
        case CHARGER_DISCHARGE:
              //  dev_dbg(dev, "%s : Discharge\n", __func__);
//                s3c_bat_info.bat_info.charging_source = CHARGER_DISCHARGE;
                break;
        default:
                //dev_err(dev, "%s : Nat supported status\n", __func__);
                ret = -EINVAL;
        }
       // source = s3c_bat_info.bat_info.charging_source;

        if (source == CHARGER_USB || source == CHARGER_AC) {
                wake_lock(&vbus_wake_lock);
        } else {
                /* give userspace some time to see the uevent and update
 *                  * LED state or whatnot...
 *                                   */
                wake_lock_timeout(&vbus_wake_lock, HZ / 2);
        }
        /* if the power source changes, all power supplies may change state */
        power_supply_changed(battery);
        /*
 *         power_supply_changed(&s3c_power_supplies[CHARGER_USB]);
 *                 power_supply_changed(&s3c_power_supplies[CHARGER_AC]);
 *                         */
//        dev_dbg(dev, "%s : call power_supply_changed\n", __func__);
        return ret;
}


static int s5m8752_power_suspend(struct platform_device *dev, pm_message_t state)
{
	struct s5m8752 *s5m8752 = dev_get_drvdata(dev->dev.parent);
	
	s5m8752_reg_write(s5m8752, S5M8752_IRQB_EVENT1, 0x00);
	s5m8752_reg_write(s5m8752, S5M8752_IRQB_EVENT2, 0x00);
	return 0;
}

static int s5m8752_power_resume(struct platform_device *dev)
{
	struct s5m8752 *s5m8752 = dev_get_drvdata(dev->dev.parent);
	int8_t value;
#if 1
        s5m8752_reg_read(s5m8752, S5M8752_IRQB_EVENT2, &value);
	if(value == 0x8){
		s3c_cable_status_update(1);
	}
	if(value == 0x4){
		s3c_cable_status_update(0);
	}	
#endif
	s5m8752_reg_write(s5m8752, S5M8752_IRQB_EVENT1, 0x00);
	s5m8752_reg_write(s5m8752, S5M8752_IRQB_EVENT2, 0x00);
	return 0;
}

static __devexit int s5m8752_power_remove(struct platform_device *pdev)
{
	struct s5m8752_power *s5m8752_power = platform_get_drvdata(pdev);

	power_supply_unregister(&s5m8752_power->battery);
	power_supply_unregister(&s5m8752_power->wall);
	power_supply_unregister(&s5m8752_power->usb);
	s5m8752_exit_charger(s5m8752_power);
	return 0;
}

static struct platform_driver s5m8752_power_driver = {
	.driver	= {
		.name	= "s5m8752-power",
		.owner	= THIS_MODULE,
	},
	.probe = s5m8752_power_probe,
	.suspend = s5m8752_power_suspend,
	.resume =  s5m8752_power_resume,
	.remove = __devexit_p(s5m8752_power_remove),
};

static int __init s5m8752_power_init(void)
{
	wake_lock_init(&vbus_wake_lock, WAKE_LOCK_SUSPEND, "vbus_present");
	return platform_driver_register(&s5m8752_power_driver);
}
module_init(s5m8752_power_init);

static void __exit s5m8752_power_exit(void)
{
	platform_driver_unregister(&s5m8752_power_driver);
}
module_exit(s5m8752_power_exit);

MODULE_DESCRIPTION("Battery power supply driver for S5M8752");
MODULE_AUTHOR("Jongbin,Won <jognbin.won@samsung.com>");
MODULE_LICENSE("GPL");
