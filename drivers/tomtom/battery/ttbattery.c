/* drivers/tomtom/battery/ttbattery.c
 *
 * Copyright (c) 2009 TomTom BV <http://www.tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 */

#include <linux/workqueue.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/power_supply.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/notifier.h>
#include <linux/sysfs.h>
#include <plat/fdt.h>
#include <asm/atomic.h>
#include <plat/battery.h>
#include <plat/tt_vbusmon.h>

#define PFX "battery: "

#define BAT_NUM_SAMPLES_PER_AVERAGE	8

struct atomic_adc_polled_average_value
{
	spinlock_t	lock;
	u64		running_total;
	u32		the_samples[BAT_NUM_SAMPLES_PER_AVERAGE];
	u32		num_sample;
	u32		current_average;
	u8    		buffer_wrapped;
};

struct ttpower_supply
{
	struct power_supply				psy;
	struct ttbattery_platform_info*			platform_info;

	struct atomic_adc_polled_average_value		avg_batt_volt;
	struct atomic_adc_polled_average_value		avg_charge_curr;

	atomic_t					is_charging;
	atomic_t					current_capacity;
	atomic_t					remaining_time;
	atomic_t					current_consumption;
	atomic_t					running;

	struct delayed_work				work;
	struct workqueue_struct*			workqueue;
};

static int battery_get_property(
	struct power_supply *psy,
	enum power_supply_property psp,
	union power_supply_propval *val);

static u32 battery_get_charge(
	struct ttpower_supply *psy,
	u32 result_as_percent);

static enum power_supply_property	tomtom_battery_properties[] =
{
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
	POWER_SUPPLY_PROP_CHARGE_EMPTY_DESIGN,
	POWER_SUPPLY_PROP_CHARGE_AVG,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_TIME_TO_EMPTY_AVG
};

static struct ttpower_supply	tomtom_battery_psy=
{
	.psy =
	{
		.name			= "battery",
		.type			= POWER_SUPPLY_TYPE_BATTERY,
		.properties		= tomtom_battery_properties,
		.num_properties		= ARRAY_SIZE(tomtom_battery_properties),
		.get_property		= battery_get_property,
		.use_for_apm		= 1,
	},

	.platform_info			= NULL,

	.avg_batt_volt =
	{
		.lock			= SPIN_LOCK_UNLOCKED,
		.running_total		= 0,
		.the_samples        	= {},
		.num_sample		= 0,
		.buffer_wrapped     	= 0,
	},

	.avg_charge_curr =
	{
		.lock			= SPIN_LOCK_UNLOCKED,
		.running_total		= 0,
		.the_samples		= {},
		.num_sample 		= 0,
		.buffer_wrapped 	= 0,
	},

	.is_charging			= ATOMIC_INIT(0),
	.current_capacity		= ATOMIC_INIT(0),
	.remaining_time			= ATOMIC_INIT(0),
	.current_consumption   		= ATOMIC_INIT(350),
	.running			= ATOMIC_INIT(0),
};

static ssize_t battery_show_current(struct device *dev,
				    struct device_attribute *attr, char *buffer)
{
	snprintf(buffer, PAGE_SIZE, "%d",
		 atomic_read(&tomtom_battery_psy.current_consumption));

	return strlen(buffer);
}

static ssize_t battery_store_current(struct device *dev,
				     struct device_attribute *attr,
				     const char *buffer, size_t size)
{
	long new_current_consumption = simple_strtol(buffer, NULL, 10);
	if (new_current_consumption) {
		atomic_set(&tomtom_battery_psy.current_consumption,
			   new_current_consumption);
		return size;
	}

	return -EINVAL;
}

static DEVICE_ATTR(current_consumption, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP,
		   battery_show_current, battery_store_current);

static struct notifier_block vbus_change_nb;

static inline void update_average_atomic_adc_value(
	struct atomic_adc_polled_average_value *arg)
{
	u32 i;
	u32 result = 0;
	u32 sample_count;

	sample_count = (arg->buffer_wrapped) ?
		BAT_NUM_SAMPLES_PER_AVERAGE : arg->num_sample;
	for (i = 0; i < sample_count; i++) {
		result += arg->the_samples[i];
	}

	if (sample_count != 0)
		result /= sample_count;
	else
		result = 0;

	arg->current_average = result;
}

static inline void reset_atomic_average(
	struct atomic_adc_polled_average_value *arg)
{
	unsigned long	flags;

	spin_lock_irqsave(&(arg->lock), flags);

	arg->num_sample = 0;
	arg->buffer_wrapped = 0;
	arg->running_total  = 0;
	arg->current_average = 0;
	memset(arg->the_samples, 0, BAT_NUM_SAMPLES_PER_AVERAGE);

	spin_unlock_irqrestore(&(arg->lock), flags);
}

static inline void add_atomic_adc_value(
	struct atomic_adc_polled_average_value *arg, u32 value)
{
	unsigned long	flags;

	spin_lock_irqsave(&(arg->lock), flags);

	arg->the_samples[arg->num_sample++] = value;
	if (arg->num_sample == BAT_NUM_SAMPLES_PER_AVERAGE) {
		arg->num_sample = 0;
		arg->buffer_wrapped = 1;
	}

	update_average_atomic_adc_value(arg);

	spin_unlock_irqrestore(&(arg->lock), flags);
}

static inline u32 get_average_atomic_adc_value(
	struct atomic_adc_polled_average_value *arg)
{
	unsigned long	flags;
	u32	retval;

	spin_lock_irqsave(&(arg->lock), flags);

	retval = arg->current_average;

	spin_unlock_irqrestore(&(arg->lock), flags);

	return retval;
}

/* Return: y0 */
static inline long linear_interpolate(long x0,  long x1, long y1,
				      long x2, long y2)
{
	int64_t y0;
	long div, sign = 0;
	if (x1 == x2)
		return y1;

	y0 = (x0 - x1);
	y0 *= (y2 - y1);
	if (y0 < 0) {
		y0 = -y0;
		sign = !sign;
	}

	div = x2 - x1;
	if (div < 0) {
		div = -div;
		sign = !sign;
	}

	do_div(y0, div); // This works only for positive numbers, tweak needed

	if (sign)
		y0 = -y0;

	y0 += y1;

	return (long)y0;
}

/*
 * Get remaining capacity from a given discharge curve, using linear
 * interpolation.
 * Parameters: curve - discharge curve to use
 *             result_as_percent - 1 : return result as % of capacity
 *                                 0 : return result as remaining time in sec
 */
#define GET_CAPACITY(curve)						\
	(result_as_percent ? curve.remaining_capacity : curve.remaining_time)
static u32 get_capacity_from_curve(batt_discharge_curve_t *curve, u32 voltage,
				   int result_as_percent)
{
	batt_discharge_lut_t	*discharge_lut;
	u32			i, cap1, cap2, voltage1, voltage2;

	cap1 = cap2 = voltage1 = voltage2 = (u32)-1;
	discharge_lut = curve->lut;
	for (i = 0; i < curve->dclut; i++) {
		if (voltage > discharge_lut[i].vbatt) {
			voltage1 = discharge_lut[i].vbatt;
			cap1 = GET_CAPACITY(discharge_lut[i]);

			if (i > 0) {
				voltage2 = discharge_lut[i - 1].vbatt;
				cap2 = GET_CAPACITY(discharge_lut[i - 1]);
			}

			break;
		}
	}

	if (cap1 == (u32)-1) {
		voltage1 = discharge_lut[curve->dclut - 1].vbatt;
		cap1 = GET_CAPACITY(discharge_lut[curve->dclut - 1]);
	}

	if (cap2 == (u32)-1) {
		cap2 = cap1;
		voltage2 = voltage1;
	}

	return linear_interpolate(voltage, voltage1, cap1, voltage2, cap2);
}
#undef GET_CAPACITY

/*
 * Converts the battery voltage into a capacity measurement using
 * precalculated look up tables: we first select the correct 2 tables
 * based on current consumption, then evaluate capacity/remaining time
 * based on battery voltage.
 *
 * battery_volts:     Current battery voltage
 * result_as_percent: 1 - the capacity is returned as a percentage,
 *                    0 -  "   in minutes.
 */
static u32 convert_volts_to_capacity(u32 battery_volts, int result_as_percent)
{
	batt_discharge_curve_t	*curve1, *curve2, *curves;
	struct ttbatt_info	*batt_info;
	u32			i, current_consumption, cap1, cap2, battery_volts_int;

        batt_info = tomtom_battery_psy.platform_info->battery_info;
        curves = batt_info->battery.discharge_curves;
	current_consumption = atomic_read(
		&tomtom_battery_psy.current_consumption);

	battery_volts_int = linear_interpolate(	battery_volts,
						batt_info->battery.Vminf,
						batt_info->battery.Vmin,
						batt_info->battery.Vmaxf,
						batt_info->battery.Vmax);

	/* Find the two nearest curves */
	curve1 = curve2 = NULL;
	for (i = 0; i < batt_info->battery.curves; i++)	{
		if (current_consumption > curves[i].current_current) {
			curve1 = &(curves[i]);

			if (i > 0)
				curve2 = &(curves[i - 1]);

			break;
		}
	}

	if (curve1 == NULL)
		curve1 = &(curves[batt_info->battery.curves - 1]);

	cap1 = get_capacity_from_curve(curve1, battery_volts_int,
				       result_as_percent);
	if (curve2) {
		cap2 = get_capacity_from_curve(curve2, battery_volts_int,
					       result_as_percent);
	} else {
		cap2 = cap1;
		curve2 = curve1;
	}

	/* Do a linear interpolation between the two values */
	return linear_interpolate(current_consumption,
				  curve1->current_current, cap1,
				  curve2->current_current, cap2);
}

/* R1 = TOP resistor, R2 = BOTTOM resistor. */
/* Return in uV */
static u32 batteryadc2voltage(struct ttbattery_platform_info *ttplat_info)
{
	u32	hpval;
	u64	raw_sample = (u64)ttplat_info->get_battery_voltage_adc();

	raw_sample *= (ttplat_info->battery_info->adc.battery_voltage_resistor_1
		       + ttplat_info->battery_info->adc.battery_voltage_resistor_2);
	do_div(raw_sample,
	       ttplat_info->battery_info->adc.battery_voltage_resistor_2);

	/* Correct raw ADC value */
	hpval = linear_interpolate ((u32)raw_sample,ttplat_info->battery_info->battery.VminADC,
				    ttplat_info->battery_info->battery.VminEXT,
				    ttplat_info->battery_info->battery.VmaxADC,
				    ttplat_info->battery_info->battery.VmaxEXT);

	return hpval;
}

/* Returns in uA */
static u32 adc2current(struct ttbattery_platform_info *ttplat_info)
{
	u32	hpval;
	u64	raw_sample = (u64)ttplat_info->get_charge_current_adc();

	do_div(raw_sample, ttplat_info->battery_info->adc.charge_current_resistor);

	/* Correct raw ADC value */
	hpval = linear_interpolate ((u32)raw_sample,ttplat_info->battery_info->battery.VminADC,
				    ttplat_info->battery_info->battery.VminEXT,
				    ttplat_info->battery_info->battery.VmaxADC,
				    ttplat_info->battery_info->battery.VmaxEXT);
	return hpval;
}

static int battery_has_samples(struct ttpower_supply *ttpsy)
{
	unsigned long     flags;
	int	has_samples;
	struct atomic_adc_polled_average_value *arg = &ttpsy->avg_batt_volt;

	spin_lock_irqsave(&(arg->lock), flags);

	has_samples = arg->buffer_wrapped;

	spin_unlock_irqrestore(&(arg->lock), flags);

	return has_samples;
}
static atomic_t mg_adc_batt_voltage=ATOMIC_INIT(0);;
static atomic_t mg_current=ATOMIC_INIT(0);;

static ssize_t vbat_show(struct device *dev, struct device_attribute *attr, char *buffer)
{
	snprintf(buffer, PAGE_SIZE, "%d",atomic_read(&mg_adc_batt_voltage));

        return strlen(buffer);
}

static ssize_t icharge_show(struct device *dev, struct device_attribute *attr, char *buffer)
{
	snprintf(buffer, PAGE_SIZE, "%d",atomic_read(&mg_current));

        return strlen(buffer);
}

static struct device_attribute batt_sysfs_attributes[]=
{
        {{"vbat", THIS_MODULE, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP}, vbat_show, NULL},
        {{"icharge", THIS_MODULE, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP}, icharge_show, NULL},
};


static void battery_poll(struct work_struct *work)
{
	struct ttpower_supply	*ttpsy = &tomtom_battery_psy;
	u32			 previous_ave, ave, rem;
	u32			 battery_voltage, averaged_batt_voltage;
	u32			 charge_current = -1;

	/* Get the voltage. */
	battery_voltage = batteryadc2voltage(ttpsy->platform_info);
	atomic_set(&mg_adc_batt_voltage,battery_voltage);

	add_atomic_adc_value(&ttpsy->avg_batt_volt, battery_voltage);
	averaged_batt_voltage = get_average_atomic_adc_value(&ttpsy->avg_batt_volt);

	/* We compute capacity at every ~1 sec poll and cache the value
	 * for client to read for performance reasons. */
	previous_ave = atomic_read(&ttpsy->current_capacity);
	ave = convert_volts_to_capacity(averaged_batt_voltage, 1);

	/* We only test for % charge change, remaining time will change
	 * almost always and we don't want to update every time. Also
	 * we re-compute remaining time only if capacity changed. */
	if (ave != previous_ave) {
		atomic_set(&ttpsy->current_capacity, ave);
		rem = convert_volts_to_capacity(averaged_batt_voltage, 0);
		atomic_set(&ttpsy->remaining_time, rem);
		power_supply_changed(&tomtom_battery_psy.psy);
	}

	/* Get the charge current value only if charging. */
	if (atomic_read(&(ttpsy->is_charging))) {
		charge_current = adc2current(ttpsy->platform_info);
		add_atomic_adc_value(&ttpsy->avg_charge_curr, charge_current);
		atomic_set(&mg_current,charge_current);
	} else {
		atomic_set(&mg_current, 0);
	}

	if (atomic_read(&ttpsy->running)) {
		/* Poll faster until we have at least
		 * BAT_NUM_SAMPLES_PER_AVERAGE samples */
		queue_delayed_work(ttpsy->workqueue, &ttpsy->work,
				   battery_has_samples(ttpsy) ?
				   BATT_POLL_INTERVAL : 10);
	}
}

static int battery_state_change(struct notifier_block *self,
				unsigned long vbus_on, void *arg)
{
	static u32		change_count = 0;
	u32			is_charging_prev;
	struct ttpower_supply	*ttpsy = &tomtom_battery_psy;

	is_charging_prev = atomic_read(&(tomtom_battery_psy.is_charging));


	atomic_set(&(tomtom_battery_psy.is_charging), vbus_on);
	printk("TTBATTERY: Vbus state change %d.. Vbus %s, ", change_count++, vbus_on ? "on" : "off");
	if (is_charging_prev != vbus_on)
		power_supply_changed(&tomtom_battery_psy.psy);

#if 0
	printk(" Batt Voltage level: %d\n",(unsigned int)batteryadc2voltage(ttpsy->platform_info));
#else
	printk("\n");
#endif

	if (vbus_on)
		reset_atomic_average(&tomtom_battery_psy.avg_charge_curr);
	else
		reset_atomic_average(&tomtom_battery_psy.avg_batt_volt);

	/* Reschedule unless it's already running */
	if (!cancel_delayed_work(&tomtom_battery_psy.work))
		return 0;

	queue_delayed_work(tomtom_battery_psy.workqueue,
			   &tomtom_battery_psy.work, 0);
	return 0;
}


static int battery_probe(struct platform_device *pdev)
{
	int index;
	struct ttbattery_platform_info	*ttplatinfo =
		(struct ttbattery_platform_info *)pdev->dev.platform_data;


	/* Initialize platform dependant stuff. */
	if (ttplatinfo->platform_init(pdev))
		return -EINVAL;

	/* Register the battery. */
	tomtom_battery_psy.platform_info = ttplatinfo;
	if (power_supply_register(&pdev->dev, &tomtom_battery_psy.psy))
		return -EINVAL;

	/* Setup the workqueue. Needs to be done before calling
	   battery_state_changed */
	tomtom_battery_psy.workqueue = create_workqueue("ttbattery");
	INIT_DELAYED_WORK(&tomtom_battery_psy.work, battery_poll);

	/* Register vbus notifier to get the right charging state. */
	vbus_change_nb.notifier_call = battery_state_change;
	vbusmon_register_notifier(&vbus_change_nb);

	/* Get ADC calibration data from fdt*/
	ttplatinfo->battery_info->battery.VminADC = fdt_get_ulong("/options/battery", "VminADC", 4000000);
	ttplatinfo->battery_info->battery.VminEXT = fdt_get_ulong("/options/battery", "VminEXT", 4000000);
	ttplatinfo->battery_info->battery.VmaxADC = fdt_get_ulong("/options/battery", "VmaxADC", 4000000);
	ttplatinfo->battery_info->battery.VmaxEXT = fdt_get_ulong("/options/battery", "VmaxEXT", 4000000);
	ttplatinfo->battery_info->battery.IminADC = fdt_get_ulong("/options/battery", "IminADC", 4000000);
	ttplatinfo->battery_info->battery.IminEXT = fdt_get_ulong("/options/battery", "IminEXT", 4000000);
	ttplatinfo->battery_info->battery.ImaxADC = fdt_get_ulong("/options/battery", "ImaxADC", 4000000);
	ttplatinfo->battery_info->battery.ImaxEXT = fdt_get_ulong("/options/battery", "ImaxEXT", 4000000);

	/* 4000 is default when no entry in fdt. */
	ttplatinfo->battery_info->battery.Vmaxf = fdt_get_ulong("/options/battery", "Vmaxf", 4000000);
	/* 3500 is default when no entry in fdt. */
	ttplatinfo->battery_info->battery.Vminf = fdt_get_ulong("/options/battery", "Vminf", 3500000);


	/* Sanity check: if Vmaxf > Vmax and/or Vminf > Vmin then the unit would
	 * suspent too urly because the interpolated
	 * Voltage would be lower then the actual Vbatt
	 */
	if (ttplatinfo->battery_info->battery.Vmaxf > ttplatinfo->battery_info->battery.Vmax)
		ttplatinfo->battery_info->battery.Vmaxf = ttplatinfo->battery_info->battery.Vmax;

	if (ttplatinfo->battery_info->battery.Vminf > ttplatinfo->battery_info->battery.Vmin)
		ttplatinfo->battery_info->battery.Vminf = ttplatinfo->battery_info->battery.Vmin;

	if (device_create_file(&pdev->dev, &dev_attr_current_consumption)) {
		printk(KERN_ERR PFX
		       "unable to create the 'current_consumption' "
		       "attribute of the battery driver.\n");
		power_supply_unregister(&tomtom_battery_psy.psy);
		vbusmon_unregister_notifier(&vbus_change_nb);
		return -EINVAL;
	}

	/* Reset the VBatt averaging counters */
	reset_atomic_average(&tomtom_battery_psy.avg_batt_volt);

	/* Start up the battery polling work Q asap. */
	atomic_set(&tomtom_battery_psy.running, 1);
	queue_delayed_work(tomtom_battery_psy.workqueue,
			   &tomtom_battery_psy.work, 0);

	for (index = 0; index < (sizeof(batt_sysfs_attributes) / sizeof(batt_sysfs_attributes[0])); index++)
                device_create_file(&pdev->dev, &batt_sysfs_attributes[index]);

	printk ("TomTom Battery driver, (C) 2008 TomTom BV\n");

	return 0;
}

static int battery_remove(struct platform_device *pdev)
{
	int index;
	struct ttbattery_platform_info	*ttplatinfo =
		(struct ttbattery_platform_info *)pdev->dev.platform_data;

	for (index = 0; index < (sizeof(batt_sysfs_attributes) / sizeof(batt_sysfs_attributes[0])); index++)
		device_remove_file(&pdev->dev, &batt_sysfs_attributes[index]);

	vbusmon_unregister_notifier(&vbus_change_nb);

	/* Cancel polling work Q. */
	atomic_set(&tomtom_battery_psy.running, 0);
	cancel_delayed_work_sync(&tomtom_battery_psy.work);
	destroy_workqueue(tomtom_battery_psy.workqueue);

	power_supply_unregister(&tomtom_battery_psy.psy);

	ttplatinfo->platform_remove(pdev);

	return 0;
}

static u32 battery_get_charge(struct ttpower_supply *ttpsy,
			      u32 result_as_percent)
{
	if (!battery_has_samples(ttpsy)) {
		printk(KERN_DEBUG PFX "%s: waiting for samples\n", __FUNCTION__);

		do {
			msleep(10);
		} while (!battery_has_samples(ttpsy));

		printk(KERN_DEBUG PFX "%s: samples found\n", __FUNCTION__);
	}

	if (result_as_percent)
		return atomic_read(&ttpsy->current_capacity);
	else
		return atomic_read(&ttpsy->remaining_time);
}


static int battery_get_property(struct power_supply *psy,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	u32 chg_current;
	struct ttpower_supply *ttpsy = (struct ttpower_supply *)container_of(
		psy, struct ttpower_supply, psy);

	switch(psp)
	{
		case POWER_SUPPLY_PROP_TIME_TO_EMPTY_AVG:
			if (atomic_read(&ttpsy->is_charging))
				/* time is invalid while charging */
				val->intval = 1;
			else
				val->intval = battery_get_charge(ttpsy, 0);

			break;

		case POWER_SUPPLY_PROP_CHARGE_AVG:
		case POWER_SUPPLY_PROP_CAPACITY:/* added to support APM code */
			if (atomic_read(&ttpsy->is_charging))
				/* charge level is invalid while charging */
				val->intval = 100;
			else
				val->intval = battery_get_charge(ttpsy, 1);

			break;

		case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
			val->intval = 100;
			break;

		case POWER_SUPPLY_PROP_CHARGE_EMPTY_DESIGN:
			val->intval = 0;
			break;

		case POWER_SUPPLY_PROP_PRESENT:
		case POWER_SUPPLY_PROP_ONLINE:
			val->intval = 1;
			break;

		case POWER_SUPPLY_PROP_STATUS:
			if (atomic_read(&ttpsy->is_charging)) {
				/* Real test is if(chg current < n) where n
				 * is some small value, this will prevent
				 * charging dragging on for hours */
				chg_current = get_average_atomic_adc_value(&ttpsy->avg_charge_curr);

				if (chg_current < 10)
					val->intval = POWER_SUPPLY_STATUS_FULL;
				else
					val->intval = POWER_SUPPLY_STATUS_CHARGING;
			} else {
				val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
			}

			break;

		default:
			return -EINVAL;
	}

	return 0;
}

static int battery_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct ttbattery_platform_info *pinfo = pdev->dev.platform_data;

	if (pinfo->platform_suspend)
		return pinfo->platform_suspend(pdev);

	return 0;
}

static int battery_resume(struct platform_device *pdev)
{
	struct ttbattery_platform_info *pinfo = pdev->dev.platform_data;

	if (pinfo->platform_resume)
		return pinfo->platform_resume(pdev);

	return 0;
}

static struct platform_driver tomtom_battery_driver = {
	.driver	= {
		.name	= "battery",
	},
	.probe	= battery_probe,
	.remove	= battery_remove,
	.resume	= battery_resume,
	.suspend= battery_suspend,
};

static int __init tomtom_battery_init(void)
{
	return platform_driver_register(&tomtom_battery_driver);
}

static void __exit tomtom_battery_exit(void)
{
	platform_driver_unregister(&tomtom_battery_driver);
}

module_init(tomtom_battery_init);
module_exit(tomtom_battery_exit);
MODULE_LICENSE("GPL");
