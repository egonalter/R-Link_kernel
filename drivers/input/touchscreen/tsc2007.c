/*
 * drivers/input/touchscreen/tsc2007.c
 *
 * Copyright (c) 2008 MtekVision Co., Ltd.
 *	Kwangwoo Lee <kwlee@mtekvision.com>
 *
 * Using code from:
 *  - ads7846.c
 *	Copyright (c) 2005 David Brownell
 *	Copyright (c) 2006 Nokia Corporation
 *  - corgi_ts.c
 *	Copyright (C) 2004-2005 Richard Purdie
 *  - omap_ts.[hc], ads7846.h, ts_osk.c
 *	Copyright (C) 2002 MontaVista Software
 *	Copyright (C) 2004 Texas Instruments
 *	Copyright (C) 2005 Dirk Behme
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/i2c/tsc2007.h>
#include <linux/irq.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#if defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
#define TSC2007_SUSPEND_LEVEL 1
#endif

#define TS_PEN_DEBOUNCE			80 /* 80 ms pen-up debouce delay */
#define TS_POLL_DELAY			5  /* ms delay between samples */
#define TS_POLL_PERIOD			10 /* ms delay between samples */

#define TSC2007_MEASURE_TEMP0		(0x0 << 4)
#define TSC2007_MEASURE_AUX		(0x2 << 4)
#define TSC2007_MEASURE_TEMP1		(0x4 << 4)
#define TSC2007_ACTIVATE_XN		(0x8 << 4)
#define TSC2007_ACTIVATE_YN		(0x9 << 4)
#define TSC2007_ACTIVATE_YP_XN		(0xa << 4)
#define TSC2007_SETUP			(0xb << 4)
#define TSC2007_MEASURE_X		(0xc << 4)
#define TSC2007_MEASURE_Y		(0xd << 4)
#define TSC2007_MEASURE_Z1		(0xe << 4)
#define TSC2007_MEASURE_Z2		(0xf << 4)

#define TSC2007_POWER_OFF_IRQ_EN	(0x0 << 2)
#define TSC2007_ADC_ON_IRQ_DIS0		(0x1 << 2)
#define TSC2007_ADC_OFF_IRQ_EN		(0x2 << 2)
#define TSC2007_ADC_ON_IRQ_DIS1		(0x3 << 2)

#define TSC2007_12BIT			(0x0 << 1)
#define TSC2007_8BIT			(0x1 << 1)

#define	MAX_12BIT			((1 << 12) - 1)

#define ADC_ON_12BIT	(TSC2007_12BIT | TSC2007_ADC_ON_IRQ_DIS0)

#define READ_Y		(ADC_ON_12BIT | TSC2007_MEASURE_Y)
#define READ_Z1		(ADC_ON_12BIT | TSC2007_MEASURE_Z1)
#define READ_Z2		(ADC_ON_12BIT | TSC2007_MEASURE_Z2)
#define READ_X		(ADC_ON_12BIT | TSC2007_MEASURE_X)
#define READ_T0		(ADC_ON_12BIT | TSC2007_MEASURE_TEMP0)
#define READ_T1		(ADC_ON_12BIT | TSC2007_MEASURE_TEMP1)
#define PWRDOWN		(TSC2007_12BIT | TSC2007_POWER_OFF_IRQ_EN)

#define debugk(fmt, ...)   { if (debug) printk(KERN_DEBUG fmt, ##__VA_ARGS__); }
#define dbg(fmt, ...)      debugk("%s - " fmt, __FUNCTION__, ##__VA_ARGS__)

static unsigned int debug = 0;
module_param(debug, uint, S_IRUGO);
MODULE_PARM_DESC(debug, "Debug level");

struct ts_event {
	u16	x;
	u16	y;
	u16	z1, z2;
};

struct tsc2007 {
	struct input_dev	*input;
	char			phys[32];
	struct delayed_work	work;

	struct i2c_client	*client;

	u16			model;
	u16			x_plate_ohms;
	u16			y_plate_ohms;	
	u16			max_rt;
	bool			pendown;
	int			pen_debounce;
	int			irq;

	int			(*get_pendown_state)(void);
	void			(*clear_penirq)(void);

	spinlock_t		irqlock;
	unsigned		irq_enabled:1;	/* P: irqlock */

	struct mutex		lock;
	unsigned		disabled:1;	/* P: lock */
	unsigned		suspended:1;	/* P: lock */

#ifdef CONFIG_SENSORS_TSC2007
	struct mutex		xfer_lock;
	unsigned long		last_updated;    /* In jiffies */
	struct device		*hwmon_dev;
	int			temp;
#endif
#if defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend    early_suspend;
#endif
};

static void tsc2007_send_up_event(struct tsc2007 *tsc);
static ssize_t tsc2007_disable_show(struct device *dev,
				     struct device_attribute *attr, char *buf);
static ssize_t tsc2007_disable_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count);

static DEVICE_ATTR(disable, S_IRUGO | S_IWUSR, tsc2007_disable_show, tsc2007_disable_store);

static inline void tsc2007_try_enable_irq(struct tsc2007 *tsc)
{
	unsigned long flags;
	
	spin_lock_irqsave(&tsc->irqlock, flags);
	if (!tsc->irq_enabled) {
		enable_irq(tsc->irq);
		tsc->irq_enabled = true;
	}
	spin_unlock_irqrestore(&tsc->irqlock, flags);
}

static inline void tsc2007_try_disable_irq(struct tsc2007 *tsc)
{
	unsigned long flags;
	
	spin_lock_irqsave(&tsc->irqlock, flags);
	if (tsc->irq_enabled) {
		disable_irq(tsc->irq);
		tsc->irq_enabled = false;
	}
	spin_unlock_irqrestore(&tsc->irqlock, flags);
}

/*
   enable the device (the call must hold the device lock)
 */
static void __tsc2007_enable(struct tsc2007 *ts)
{
	tsc2007_try_enable_irq(ts);
}

/*
   disable the device (the call must hold the device lock)
 */
static void __tsc2007_disable(struct tsc2007 *ts)
{
	cancel_delayed_work_sync(&ts->work);
	tsc2007_try_disable_irq(ts);

	/* Leave the input driver in a known state */
	if (ts->pendown) {
		tsc2007_send_up_event(ts);
		ts->pendown = false;
	}
}

static ssize_t tsc2007_disable_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	struct tsc2007 *ts = dev_get_drvdata(dev);

	return sprintf(buf, "%u\n", ts->disabled);
}

static ssize_t tsc2007_disable_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	struct tsc2007 *ts = dev_get_drvdata(dev);
	unsigned long value;
	int rc;

	rc = strict_strtoul(buf, 10, &value);
	if (rc)
		return rc;

	mutex_lock(&ts->lock);
	if (!ts->suspended) {
		if (value) {
			if (!ts->disabled)
				__tsc2007_disable(ts);
		} else {
			if (ts->disabled)
				__tsc2007_enable(ts);
		}
	}
	ts->disabled = !!value;
	mutex_unlock(&ts->lock);

	return count;
}

static inline int tsc2007_xfer(struct tsc2007 *tsc, u8 cmd)
{
	s32 data;
	u16 val;

	data = i2c_smbus_read_word_data(tsc->client, cmd);
	if (data < 0) {
		dev_err(&tsc->client->dev, "i2c io error: %d\n", data);
		return data;
	}

	/* The protocol and raw data format from i2c interface:
	 * S Addr Wr [A] Comm [A] S Addr Rd [A] [DataLow] A [DataHigh] NA P
	 * Where DataLow has [D11-D4], DataHigh has [D3-D0 << 4 | Dummy 4bit].
	 */
	val = swab16(data) >> 4;

	dev_dbg(&tsc->client->dev, "data: 0x%x, val: 0x%x\n", data, val);

	return val;
}

static void tsc2007_read_values(struct tsc2007 *tsc, struct ts_event *tc)
{
#ifdef CONFIG_SENSORS_TSC2007
	mutex_lock(&tsc->xfer_lock);
#endif

	/* y- still on; turn on only y+ (and ADC) */
	tc->y = tsc2007_xfer(tsc, READ_Y);

	/* turn y- off, x+ on, then leave in lowpower */
	tc->x = tsc2007_xfer(tsc, READ_X);

	/* turn y+ off, x- on; we'll use formula #1 */
	tc->z1 = tsc2007_xfer(tsc, READ_Z1);
	tc->z2 = tsc2007_xfer(tsc, READ_Z2);

	/* Prepare for next touch reading - power down ADC, enable PENIRQ */
	tsc2007_xfer(tsc, PWRDOWN);

#ifdef CONFIG_SENSORS_TSC2007
	mutex_unlock(&tsc->xfer_lock);
#endif
}

static u32 tsc2007_calculate_pressure(struct tsc2007 *tsc, struct ts_event *tc)
{
	u32 rt = 0;
	u32 rr = 0;

	/* range filtering */
	if (tc->x == MAX_12BIT)
		tc->x = 0;

	if (likely(tc->x && tc->z1)) {
		/* compute touch pressure resistance using equation #2 */
		rt = tsc->x_plate_ohms * tc->x;
		rt = (rt + 2047) >> 12;
		rt *= (4096 - tc->z1);
		rt /= tc->z1;
		rr = tsc->y_plate_ohms * (4096 - tc->y);
		rr = (rr + 2047) >> 12;
		rt -= rr;
	}

	return rt;
}

static void tsc2007_send_up_event(struct tsc2007 *tsc)
{
	struct input_dev *input = tsc->input;

	dev_dbg(&tsc->client->dev, "UP\n");

	input_report_key(input, BTN_TOUCH, 0);
	input_report_abs(input, ABS_PRESSURE, 0);
	input_sync(input);
}

static void tsc2007_work(struct work_struct *work)
{
	struct tsc2007 *ts =
		container_of(to_delayed_work(work), struct tsc2007, work);
	bool debounced = false;
	struct ts_event tc;
	u32 rt;

	/*
	 * NOTE: We can't rely on the pressure to determine the pen down
	 * state, even though this controller has a pressure sensor.
	 * The pressure value can fluctuate for quite a while after
	 * lifting the pen and in some cases may not even settle at the
	 * expected value.
	 *
	 * The only safe way to check for the pen up condition is in the
	 * work function by reading the pen signal state (it's a GPIO
	 * and IRQ). Unfortunately such callback is not always available,
	 * in that case we have rely on the pressure anyway.
	 */
	if (ts->get_pendown_state) {
		if (unlikely(!ts->get_pendown_state())) {
			if (!ts->pen_debounce) {
				tsc2007_send_up_event(ts);
				ts->pendown = false;
			}
			else
				ts->pen_debounce--;
			goto out;
		}

		dev_dbg(&ts->client->dev, "pen is still down\n");
	}

	tsc2007_read_values(ts, &tc);

	rt = tsc2007_calculate_pressure(ts, &tc);
	if (rt > ts->max_rt) {
		/*
		 * Sample found inconsistent by debouncing or pressure is
		 * beyond the maximum. Don't report it to user space,
		 * repeat at least once more the measurement.
		 */
		dev_dbg(&ts->client->dev, "ignored pressure %d\n", rt);
		debounced = true;
		goto out;

	}

	if (rt) {
		struct input_dev *input = ts->input;

		if (!ts->pendown) {
			dev_dbg(&ts->client->dev, "DOWN\n");

			input_report_key(input, BTN_TOUCH, 1);
			ts->pendown = true;
		}
		ts->pen_debounce = TS_PEN_DEBOUNCE/TS_POLL_PERIOD;

		input_report_abs(input, ABS_X, tc.x);
		input_report_abs(input, ABS_Y, tc.y);
		input_report_abs(input, ABS_PRESSURE, rt);

		input_sync(input);

		dev_dbg(&ts->client->dev, "point(%4d,%4d), pressure (%4u)\n",
			tc.x, tc.y, rt);

	} else if (!ts->get_pendown_state && ts->pendown) {
		/*
		 * We don't have callback to check pendown state, so we
		 * have to assume that since pressure reported is 0 the
		 * pen was lifted up.
		 */
		tsc2007_send_up_event(ts);
		ts->pendown = false;
	}

 out:
	if (ts->pendown || debounced)
		schedule_delayed_work(&ts->work,
				      msecs_to_jiffies(TS_POLL_PERIOD));
	else {
		tsc2007_try_enable_irq(ts);
		ts->pen_debounce = TS_PEN_DEBOUNCE/TS_POLL_PERIOD;
	}
}

static irqreturn_t tsc2007_irq(int irq, void *handle)
{
	struct tsc2007 *ts = handle;

	if (!ts->get_pendown_state || likely(ts->get_pendown_state())) {
		disable_irq_nosync(ts->irq);
		ts->irq_enabled = false;
		schedule_delayed_work(&ts->work,
				      msecs_to_jiffies(TS_POLL_DELAY));
	}

	if (ts->clear_penirq)
		ts->clear_penirq();

	return IRQ_HANDLED;
}

static void tsc2007_free_irq(struct tsc2007 *ts)
{
	free_irq(ts->irq, ts);
	/*
	 * Try to balance the disable_irq() done in the
	 * interrupt handler.
	 */
	tsc2007_try_enable_irq(ts);
}

#ifdef CONFIG_SENSORS_TSC2007

/* tension is in mV */
static int reg_to_tension(int reg)
{
	/* rescale from 12bit to Vref (1800 mV) */
	return reg * 1800 / MAX_12BIT;
}

/* temperature is in Celsius, v0 and v1 are in mV */
static int tension_to_temperature(int v0, int v1)
{
	int delta = v1 - v0;
	int temp = delta * 2573 - 273000;

	dbg("v0: %d mV, v1: %d mV, delta: %d mV, temp: %d C\n", v0, v1, delta, temp);
	return temp;
}

static int tsc2007_hwmon_read_temp(struct tsc2007 *tsc)
{
	int data, v0, v1, temp;

	data = tsc2007_xfer(tsc, READ_T0);
	v0 = reg_to_tension(data);
	if (data < 0) {
		dbg("unable to read v0 (err: %d), returning previous value...\n", data);
		temp = tsc->temp;
		goto exit;
	}

	data = tsc2007_xfer(tsc, READ_T1);
	v1 = reg_to_tension(data);
	if (data < 0) {
		dbg("unable to read v1 (err: %d), returning previous value...\n", data);
		temp = tsc->temp;
		goto exit;
	}

	temp = tension_to_temperature(v0, v1);

exit:
	/* Prepare for next touch reading - power down ADC, enable PENIRQ */
	tsc2007_xfer(tsc, PWRDOWN);
	return temp;
}

static ssize_t tsc2007_hwmon_show_temp(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = container_of(dev, struct i2c_client, dev);
	struct tsc2007 *tsc = i2c_get_clientdata(client);

	mutex_lock(&tsc->xfer_lock);

	if (time_after(jiffies, tsc->last_updated + HZ + HZ/2) || tsc->temp == -1) {
		tsc->temp = tsc2007_hwmon_read_temp(tsc);
		tsc->last_updated = jiffies;
	}

	mutex_unlock(&tsc->xfer_lock);
	return sprintf(buf, "%d\n", tsc->temp);
}

static struct sensor_device_attribute tsc2007_hwmon_sensors[] = {
	SENSOR_ATTR(temp1_input, S_IRUGO, tsc2007_hwmon_show_temp, NULL, 0),
};

static void tsc2007_hwmon_device_remove_files(struct device *dev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(tsc2007_hwmon_sensors); i++)
		device_remove_file(dev, &tsc2007_hwmon_sensors[i].dev_attr);
}

static int __devinit tsc2007_init_hwmon(struct tsc2007 *tsc)
{
	struct device *dev = &tsc->client->dev;
	int i, err = 0;

	for (i = 0; i < ARRAY_SIZE(tsc2007_hwmon_sensors); i++) {
		err = device_create_file(dev, &tsc2007_hwmon_sensors[i].dev_attr);
		if (err < 0) {
			dbg("unable to create sysfs file (err: %d)\n", err);
			goto error;
		}
	}

	tsc->hwmon_dev = hwmon_device_register(dev);
	if (IS_ERR(tsc->hwmon_dev)) {
		err = PTR_ERR(tsc->hwmon_dev);
		dbg("unable to register hwmon driver (err: %d)\n", err);
		goto error;
	}

	mutex_init(&tsc->xfer_lock);
	tsc->temp = -1;
	return 0;

error:
	tsc2007_hwmon_device_remove_files(dev);
	return err;
}

static void __devexit tsc2007_exit_hwmon(struct tsc2007 *tsc)
{
	hwmon_device_unregister(tsc->hwmon_dev);
	tsc2007_hwmon_device_remove_files(&tsc->client->dev);
}

#else /* CONFIG_SENSORS_TSC2007 */

static int __devinit tsc2007_init_hwmon(struct tsc2007 *tsc)
{
	dbg("hwmon driver is disabled (CONFIG_SENSORS_TSC2007=n)\n");
	return 0;
}

static int __devexit tsc2007_exit_hwmon(struct tsc2007 *tsc)
{
	return 0;
}

#endif /* CONFIG_SENSORS_TSC2007 */

#ifdef CONFIG_PM
static int tsc2007_suspend(struct device *dev)
{
	struct tsc2007 *ts = dev_get_drvdata(dev);

	mutex_lock(&ts->lock);
	if (!ts->suspended && !ts->disabled)
		__tsc2007_disable(ts);

	ts->suspended = 1;
	mutex_unlock(&ts->lock);

	return 0;
}

static int tsc2007_resume(struct device *dev)
{
	struct tsc2007	*ts = dev_get_drvdata(dev);

	mutex_lock(&ts->lock);
	if (ts->suspended && !ts->disabled)
		__tsc2007_enable(ts);

	ts->suspended = 0;
	mutex_unlock(&ts->lock);

	return 0;
}

#ifdef CONFIG_HAS_EARLYSUSPEND
static void tsc2007_early_suspend(struct early_suspend *h)
{
	struct tsc2007 *ts = container_of(h, struct tsc2007, early_suspend);

	tsc2007_suspend(&ts->client->dev);
}

static void tsc2007_late_resume(struct early_suspend *h)
{
	struct tsc2007 *ts = container_of(h, struct tsc2007, early_suspend);

	tsc2007_resume(&ts->client->dev);
}
#endif

static const struct dev_pm_ops tsc2007_pm_ops = {
#ifndef CONFIG_HAS_EARLYSUSPEND
	.suspend	= tsc2007_suspend,
	.resume		= tsc2007_resume,
#endif
};
#endif

static int __devinit tsc2007_probe(struct i2c_client *client,
				   const struct i2c_device_id *id)
{
	struct tsc2007 *ts;
	struct tsc2007_platform_data *pdata = pdata = client->dev.platform_data;
	struct input_dev *input_dev;
	int err;

	if (!pdata) {
		dev_err(&client->dev, "platform data is required!\n");
		return -EINVAL;
	}

	if (!i2c_check_functionality(client->adapter,
				     I2C_FUNC_SMBUS_READ_WORD_DATA))
		return -EIO;

	ts = kzalloc(sizeof(struct tsc2007), GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!ts || !input_dev) {
		err = -ENOMEM;
		goto err_free_mem;
	}

	ts->client = client;
	ts->irq = client->irq;
	ts->input = input_dev;
	INIT_DELAYED_WORK(&ts->work, tsc2007_work);
	spin_lock_init(&ts->irqlock);
	mutex_init(&ts->lock);

	ts->model             = pdata->model;
	ts->x_plate_ohms      = pdata->x_plate_ohms;
	ts->y_plate_ohms      = pdata->y_plate_ohms;	
	ts->max_rt            = pdata->max_rt ? : MAX_12BIT;
	ts->get_pendown_state = pdata->get_pendown_state;
	ts->clear_penirq      = pdata->clear_penirq;
	ts->pen_debounce      = TS_PEN_DEBOUNCE/TS_POLL_PERIOD;

	snprintf(ts->phys, sizeof(ts->phys),
		 "%s/input0", dev_name(&client->dev));

	input_dev->name = "TSC2007 Touchscreen";
	input_dev->phys = ts->phys;
	input_dev->id.bustype = BUS_I2C;

	input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);

	input_set_abs_params(input_dev, ABS_X, 0, MAX_12BIT, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, 0, MAX_12BIT, 0, 0);
	input_set_abs_params(input_dev, ABS_PRESSURE, 0, MAX_12BIT, 0, 0);

	err = tsc2007_init_hwmon(ts);
	if (err < 0)
		goto err_free_mem;

	if (pdata->init_platform_hw)
		pdata->init_platform_hw();

	err = request_irq(ts->irq, tsc2007_irq, IRQ_TYPE_EDGE_FALLING,
			client->dev.driver->name, ts);
	if (err < 0) {
		dev_err(&client->dev, "irq %d busy?\n", ts->irq);
		goto err_free_hwmon;
	}
	ts->irq_enabled = true;

	/* Prepare for touch readings - power down ADC and enable PENIRQ */
	err = tsc2007_xfer(ts, PWRDOWN);
	if (err < 0)
		goto err_free_irq;

	/* create sysfs attributes */
	err = device_create_file(&client->dev, &dev_attr_disable);
	if (err < 0)
		goto err_free_irq;

	/* register input device */
	err = input_register_device(input_dev);
	if (err)
		goto err_remove_sysfs_file;

#ifdef CONFIG_HAS_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN +
					TSC2007_SUSPEND_LEVEL;
	ts->early_suspend.suspend = tsc2007_early_suspend;
	ts->early_suspend.resume = tsc2007_late_resume;
	register_early_suspend(&ts->early_suspend);
#endif

	i2c_set_clientdata(client, ts);

	return 0;

 err_remove_sysfs_file:
	device_remove_file(&client->dev, &dev_attr_disable);
 err_free_irq:
	tsc2007_free_irq(ts);
	if (pdata->exit_platform_hw)
		pdata->exit_platform_hw();
 err_free_hwmon:
	tsc2007_exit_hwmon(ts);
 err_free_mem:
	input_free_device(input_dev);
	kfree(ts);
	return err;
}

static int __devexit tsc2007_remove(struct i2c_client *client)
{
	struct tsc2007	*ts = i2c_get_clientdata(client);
	struct tsc2007_platform_data *pdata = client->dev.platform_data;

	tsc2007_free_irq(ts);

	if (pdata->exit_platform_hw)
		pdata->exit_platform_hw();

	tsc2007_exit_hwmon(ts);
#ifdef CONFIG_HAS_EARLYSUSPEND
	unregister_early_suspend(&ts->early_suspend);
#endif
	input_unregister_device(ts->input);
	device_remove_file(&client->dev, &dev_attr_disable);
	kfree(ts);

	return 0;
}

static struct i2c_device_id tsc2007_idtable[] = {
	{ "tsc2007", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, tsc2007_idtable);

static struct i2c_driver tsc2007_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "tsc2007",
#ifdef CONFIG_PM
		.pm = &tsc2007_pm_ops,
#endif
	},
	.id_table	= tsc2007_idtable,
	.probe		= tsc2007_probe,
	.remove		= __devexit_p(tsc2007_remove),
};

static int __init tsc2007_init(void)
{
	return i2c_add_driver(&tsc2007_driver);
}

static void __exit tsc2007_exit(void)
{
	i2c_del_driver(&tsc2007_driver);
}

module_init(tsc2007_init);
module_exit(tsc2007_exit);

MODULE_AUTHOR("Kwangwoo Lee <kwlee@mtekvision.com>");
MODULE_DESCRIPTION("TSC2007 TouchScreen Driver");
MODULE_LICENSE("GPL");
