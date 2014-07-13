/*
 * drivers/tomtom/accelerometer/ms5607/ms5607.c
 *
 * MS5607 Micro Altimeter Module driver
 *
 * Copyright (C) 2011 TomTom International BV
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

#include <linux/delay.h>
#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/ms5607.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/spi/spi.h>
#include <linux/types.h>
#include <linux/byteorder/generic.h>

#define DRIVER_NAME		"ms5607"
#define DEVICE_NAME		"MS5607-B Pressure Sensor"
#define DRIVER_DESC		DEVICE_NAME " Driver"

#define MS5607_MIN_PRESSURE	10000
#define MS5607_MAX_PRESSURE	1200000
#define MS5607_PRESSURE_FUZZ	0

#define MS5607_MIN_TEMPERATURE	-4000
#define MS5607_MAX_TEMPERATURE	8500
#define MS5607_TEMPERATURE_FUZZ	0

#define MS5607_CMD_READ_ADC	0x00
#define MS5607_CMD_RESET	0x1E
#define MS5607_CMD_READ_C1	0xA2

#define MS5607_CONV_ERROR	UINT_MAX

enum {
	MS5607_C1 = 0,
	MS5607_C2,
	MS5607_C3,
	MS5607_C4,
	MS5607_C5,
	MS5607_C6,

	MS5607_CAL_NUM_VALUES
};

struct ms5607_values
{
	int32_t temperature;
	int32_t pressure;
};

struct ms5607_osr_cfg
{
	int	osr;
	int	cmd_conv_d1;
	int	cmd_conv_d2;
	int	conv_time;	/* ms */
};

static struct ms5607_osr_cfg ms5607_osr_cfg[] = {
	[MS5607_OSR_256] = {
		.osr		= 256,
		.cmd_conv_d1	= 0x40,
		.cmd_conv_d2	= 0x50,
		.conv_time	= 1	/* 600us */
	},

	[MS5607_OSR_512] = {
		.osr		= 512,
		.cmd_conv_d1	= 0x42,
		.cmd_conv_d2	= 0x52,
		.conv_time	= 2	/* 1117us */
	},

	[MS5607_OSR_1024] = {
		.osr		= 1024,
		.cmd_conv_d1	= 0x44,
		.cmd_conv_d2	= 0x54,
		.conv_time	= 3	/* 2280us*/
	},

	[MS5607_OSR_2048] = {
		.osr		= 2048,
		.cmd_conv_d1	= 0x46,
		.cmd_conv_d2	= 0x56,
		.conv_time	= 5	/* 4540us */
	},

	[MS5607_OSR_4096] = {
		.osr		= 4096,
		.cmd_conv_d1	= 0x48,
		.cmd_conv_d2	= 0x58,
		.conv_time	= 10	/* 9040us */
	},
};

struct ms5607
{
	struct {
		int opened;
		int disabled;
		int suspended;
		int comm_error;
	} status;

	struct {
		struct task_struct *thread;
		wait_queue_head_t wq;
		struct hrtimer timer;
		int wake_up;
	} worker;

	struct ms5607_platform_data *pdata;
	uint16_t calibration[MS5607_CAL_NUM_VALUES];

	struct spi_device *spi_dev;
	struct device* dev;
	struct input_dev *input;
	char phys[32];
	struct mutex lock;
	struct regulator *regulator;

#ifdef CONFIG_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
};

static struct ms5607_platform_data ms5607_default_pdata = {
	.sample_rate	= 1000,
	.osr		= MS5607_OSR_512
};

/* prototypes */
static int ms5607_spi_read(struct spi_device *spi_dev, unsigned char cmd,
			uint8_t *data, uint8_t len);
static int ms5607_send_command(struct ms5607 *ms5607, uint8_t cmd);
static int ms5607_reset(struct ms5607 *ms5607);

static ssize_t ms5607_sysfs_show_disabled(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t ms5607_sysfs_store_disabled(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);

static DEVICE_ATTR(disabled, S_IRUGO | S_IWUSR, ms5607_sysfs_show_disabled, ms5607_sysfs_store_disabled);

static int __ms5607_enable(struct ms5607 *ms5607)
{
	int rc = -1;

	rc = regulator_enable(ms5607->regulator);
	if (rc) {
		dev_err(ms5607->dev, "Failed to enable regulator: %d\n", rc);
		goto err_reg_enable;
	}

	/* give the chip some time to initialize */
	msleep(10);

	ms5607_reset(ms5607);

	/* start the timer that triggers sampling */
	hrtimer_start(&ms5607->worker.timer,
		      ns_to_ktime(ms5607->pdata->sample_rate * 1000 * 1000),
		      HRTIMER_MODE_REL);

	rc = 0;

err_reg_enable:
	return rc;
}

static int __ms5607_disable(struct ms5607 *ms5607)
{
	int rc = -1;

	hrtimer_cancel(&ms5607->worker.timer);
	ms5607->worker.wake_up = 0;

	rc = regulator_disable(ms5607->regulator);
	if (rc) {
		dev_err(ms5607->dev, "Failed to disable regulator: %d\n", rc);
		goto err_reg_disable;
	}

	rc = 0;

err_reg_disable:
	return rc;
}

static int ms5607_input_open(struct input_dev *input)
{
	struct ms5607 *ms5607 = input_get_drvdata(input);

	mutex_lock(&ms5607->lock);

	if (!ms5607->status.suspended && !ms5607->status.disabled)
		__ms5607_enable(ms5607);

	ms5607->status.opened = 1;

	mutex_unlock(&ms5607->lock);

	return 0;
}

static void ms5607_input_close(struct input_dev *input)
{
	struct ms5607 *ms5607 = input_get_drvdata(input);

	mutex_lock(&ms5607->lock);

	if (!ms5607->status.suspended && !ms5607->status.disabled)
		__ms5607_disable(ms5607);

	ms5607->status.opened = 0;

	mutex_unlock(&ms5607->lock);
}

#define TO_INT32(x) ((int32_t)x)
#define TO_INT64(x) ((int64_t)x)
#define POW_2(x) (x * x)

/* calculate temperature compensated pressure */
static struct ms5607_values ms5607_calculate_values(struct ms5607 *ms5607, uint32_t D1, uint32_t D2)
{
	/* NOTE: some toolchains don't support division of 64-bit values, as we
	   divide by a power of two we can use shifting as a workaround */

	struct ms5607_values result;
	const int32_t dT = D2 - (TO_INT32(ms5607->calibration[MS5607_C5]) << 8);

	int32_t TEMP = 2000 + ((TO_INT64(dT) * ms5607->calibration[MS5607_C6]) >> 23);
	int64_t OFF = (TO_INT64(ms5607->calibration[MS5607_C2]) << 17) + ((TO_INT64(ms5607->calibration[MS5607_C4]) * dT) >> 6);
	int64_t	SENS = (TO_INT64(ms5607->calibration[MS5607_C1]) << 16) + ((TO_INT64(ms5607->calibration[MS5607_C3]) * dT) >> 7);

	if (TEMP < 2000) {
		/* temperature below 20°C => compensate non-linearity over the temperature */
		const int32_t T2 = (TO_INT64(dT)^2) >> 31;
		int64_t OFF2 = (61 * POW_2(TO_INT64(TEMP - 2000))) >> 4;
		int64_t SENS2 = 2 * POW_2(TO_INT64(TEMP - 2000));

		if (TEMP < 1500) {
			/* temperature below 15°C => further compensation needed */
			OFF2 += 15 * POW_2(TO_INT64(TEMP + 1500));
			SENS2 += 8 * POW_2(TO_INT64(TEMP + 1500));
		}

		TEMP -= T2;
		OFF -= OFF2;
		SENS -= SENS2;
	}

	result.temperature = TEMP;
	result.pressure = (((D1 * SENS) >> 21) - OFF) >> 15;

	return result;
}

static uint32_t ms5607_read_adc(struct ms5607 *ms5607, uint8_t conv_cmd, int conv_time)
{
	uint32_t adc_value = MS5607_CONV_ERROR;
	char err_msg[64];
	int rc;

	rc = ms5607_send_command(ms5607, conv_cmd);
	if (rc) {
		sprintf(err_msg, "Failed to send 0x%02x command: %d", conv_cmd, rc);

		goto err_send_cmd;
	}

	msleep(conv_time);

	rc = ms5607_spi_read(ms5607->spi_dev, MS5607_CMD_READ_ADC, (uint8_t *)&adc_value, 3);
	if (rc) {
		sprintf(err_msg, "Failed to read ADC: %d", rc);

		goto err_read_adc;
	}

	if (adc_value == MS5607_CONV_ERROR) {
		/* communication with the SPI controller ok, but no real data received (SDO = high) */
		sprintf(err_msg, "Failed to read ADC: device not responding");

		goto err_no_response;
	}

	if (ms5607->status.comm_error) {
		/* recovered from error */
		dev_info(ms5607->dev, "Successfully read ADC, recovered from previous error\n");
		ms5607->status.comm_error = 0;
	}

	adc_value = (be32_to_cpu(adc_value) >> 8);

	return adc_value;

err_no_response:
err_read_adc:
err_send_cmd:
	if (!ms5607->status.comm_error) {
		/* new error */
		dev_warn(ms5607->dev, "%s\n", err_msg);
		ms5607->status.comm_error = 1;
	}

	return MS5607_CONV_ERROR;
}

static int ms5607_worker(void *data)
{
	struct ms5607 *ms5607 = (struct ms5607 *)data;
	struct ms5607_osr_cfg *osr_cfg = &ms5607_osr_cfg[ms5607->pdata->osr];

	do {
		wait_event_interruptible(ms5607->worker.wq,
					 ms5607->worker.wake_up || kthread_should_stop());

		if (ms5607->worker.wake_up) {
			/* normal work */

			uint32_t D1, D2;
			struct ms5607_values values;

			/* race condition alert: the timer could have expired
			   again and set wake_up to 1. in this unlikely case we
			   would loose one sample, we are aware of this and can
			   live with it */
			ms5607->worker.wake_up = 0;

			mutex_lock(&ms5607->lock);

			D1 = ms5607_read_adc(ms5607,
					     osr_cfg->cmd_conv_d1,
					     osr_cfg->conv_time);
			if (D1 == MS5607_CONV_ERROR)
				goto err_read_adc;

			D2 = ms5607_read_adc(ms5607,
					     osr_cfg->cmd_conv_d2,
					     osr_cfg->conv_time);
			if (D2 == MS5607_CONV_ERROR)
				goto err_read_adc;

			values = ms5607_calculate_values(ms5607, D1, D2);

			input_event(ms5607->input, EV_ABS, ABS_PRESSURE, values.pressure);
			input_event(ms5607->input, EV_ABS, ABS_X, values.temperature);
			input_event(ms5607->input, EV_ABS, ABS_MISC, jiffies);

			input_sync(ms5607->input);

		  err_read_adc: /* try again later */

			mutex_unlock(&ms5607->lock);
		}
	} while (!kthread_should_stop());

	return 0;
}

static enum hrtimer_restart ms5607_trigger_sampling(struct hrtimer *hrtimer)
{
	struct ms5607 *ms5607 = container_of(hrtimer, struct ms5607, worker.timer);

	hrtimer_forward_now(hrtimer, ns_to_ktime(ms5607->pdata->sample_rate * 1000 * 1000));

	ms5607->worker.wake_up = 1;
	wake_up(&ms5607->worker.wq);

	return HRTIMER_RESTART;
}

static int ms5607_spi_read(struct spi_device *spi_dev, unsigned char cmd, uint8_t *data, uint8_t len)
{
	struct spi_message msg;
	struct spi_transfer transfer[2];

	spi_message_init(&msg);
	memset(transfer, 0, sizeof(transfer));

	transfer[0].tx_buf = &cmd;
	transfer[0].len = sizeof(cmd);
	spi_message_add_tail(&transfer[0], &msg);

	transfer[1].rx_buf = data;
	transfer[1].len = len;
	spi_message_add_tail(&transfer[1], &msg);

	msg.spi = spi_dev;
	msg.is_dma_mapped = 0;

	return spi_sync(spi_dev, &msg);
}

static int ms5607_send_command(struct ms5607 *ms5607, uint8_t cmd)
{
	return spi_write(ms5607->spi_dev, &cmd, sizeof(cmd));
}

static int ms5607_reset(struct ms5607 *ms5607)
{
	int rc;

	rc = ms5607_send_command(ms5607, MS5607_CMD_RESET);
	if (!rc) {
		msleep(3);
		dev_dbg(ms5607->dev, "Reset\n");
	} else {
		dev_err(ms5607->dev, "Failed to reset\n");
	}

	return rc;
}

static int ms5607_read_calibration_data(struct ms5607 *ms5607)
{
	int rc;
	int i;
	uint8_t cmd = MS5607_CMD_READ_C1;

	rc = regulator_enable(ms5607->regulator);
	if (rc) {
		printk("Failed to enable regulator: %d\n", rc);
		goto err_regulator;
	}

	/* give the chip some time to initialize */
	msleep(10);

	for (i = 0; i < MS5607_CAL_NUM_VALUES; i++) {
		uint16_t value;
		rc = ms5607_spi_read(ms5607->spi_dev, cmd, (uint8_t *)&value, 2);
		if (rc) {
			dev_err(&ms5607->spi_dev->dev, "Failed to read calibration data: %d\n", rc);
			break;
		}

		ms5607->calibration[i] = be16_to_cpu(value);

		cmd += 2;
	}

	regulator_disable(ms5607->regulator);

err_regulator:

	return rc;
}

#ifdef CONFIG_PM
void evdev_suspend(struct input_dev *dev);

static int __ms5607_suspend(struct ms5607 *ms5607)
{
	dev_info(ms5607->dev, "suspend\n");

	mutex_lock(&ms5607->lock);

	if (!ms5607->status.suspended && !ms5607->status.disabled && ms5607->status.opened)
		__ms5607_disable(ms5607);

	evdev_suspend(ms5607->input);

	ms5607->status.suspended = 1;

	mutex_unlock(&ms5607->lock);

	return 0;
}

static int __ms5607_resume(struct ms5607 *ms5607)
{
	dev_info(ms5607->dev, "resume\n");

	mutex_lock(&ms5607->lock);

	if (ms5607->status.suspended && !ms5607->status.disabled && ms5607->status.opened)
	 	__ms5607_enable(ms5607);

	ms5607->status.suspended = 0;

	mutex_unlock(&ms5607->lock);

	return 0;
}

#ifdef CONFIG_EARLYSUSPEND
static void ms5607_early_suspend(struct early_suspend *handler) {
	struct ms5607 *ms5607 = container_of(handler, struct ms5607, early_suspend);

	__ms5607_suspend(ms5607);
}

static void ms5607_early_resume(struct early_suspend *handler) {
	struct ms5607 *ms5607 = container_of(handler, struct ms5607, early_suspend);

	__ms5607_resume(ms5607);
}

#define ms5607_suspend NULL
#define ms5607_resume  NULL
#else
static int ms5607_suspend(struct spi_device *spi, pm_message_t state)
{
	struct ms5607 *ms5607 = dev_get_drvdata(&spi->dev);

	return __ms5607_suspend(ms5607);
}

static int ms5607_resume(struct spi_device *spi)
{
	struct ms5607 *ms5607 = dev_get_drvdata(&spi->dev);

	return __ms5607_resume(ms5607);
}
#endif

#else
#define ms5607_suspend NULL
#define ms5607_resume  NULL
#endif /* CONFIG_PM */

static int ms5607_create_input_dev(struct ms5607 *ms5607)
{
	int rc;
	struct input_dev *input_dev;

	input_dev = input_allocate_device();
	if (NULL == input_dev) {
		dev_err(ms5607->dev, "Error allocating input device\n");
		rc = -ENOMEM;
		goto err_alloc_input;
	}

	ms5607->input = input_dev;
	input_set_drvdata(input_dev, ms5607);

	input_dev->name = DEVICE_NAME;
	snprintf(ms5607->phys, sizeof(ms5607->phys), "%s/input0", dev_name(ms5607->dev));
	input_dev->phys = ms5607->phys;
	input_dev->dev.parent = ms5607->dev;

	input_dev->id.bustype = BUS_SPI;
	input_dev->open = ms5607_input_open;
	input_dev->close = ms5607_input_close;

	__set_bit(EV_ABS, input_dev->evbit);

	__set_bit(ABS_PRESSURE, input_dev->absbit);
	__set_bit(ABS_X, input_dev->absbit); /* temperature */
	/* the TT positioning engine expects data at a fixed rate. input devices don't
	   deliver redundant data. therefore we use send an ABS_MISC event with the
	   sensor data to indicate the Androids libsensors that we are alive and
	   force it to feed the sensor manager with the values received previously in
	   case the sensor data hasn't changed */
	__set_bit(ABS_MISC, input_dev->absbit);

	input_set_abs_params(input_dev, ABS_PRESSURE,
			MS5607_MIN_PRESSURE, MS5607_MAX_PRESSURE,
			MS5607_PRESSURE_FUZZ, 0);

	input_set_abs_params(input_dev, ABS_X,
			MS5607_MIN_TEMPERATURE, MS5607_MAX_TEMPERATURE,
			MS5607_TEMPERATURE_FUZZ, 0);

	rc = input_register_device(input_dev);
	if (rc) {
		dev_err(ms5607->dev, "Failed to register input device\n");
		input_free_device(input_dev);
	}

err_alloc_input:
	return rc;
}

static int ms5607_probe(struct spi_device *spi)
{
	int rc = -1;
	struct ms5607 *ms5607;

	dev_info(&spi->dev, DRIVER_DESC "\n");

	ms5607 = kzalloc(sizeof(*ms5607), GFP_KERNEL);
	if (NULL == ms5607) {
		dev_err(&spi->dev, "Error allocating memory for private data\n");
		rc = -ENOMEM;
		goto err_alloc_ms5607;
	}

	ms5607->dev = &spi->dev;
	ms5607->spi_dev = spi;

	spi_set_drvdata(spi, ms5607);

	if (ms5607->dev->platform_data) {
		ms5607->pdata = ms5607->dev->platform_data;
	} else {
		dev_dbg(ms5607->dev, "No platform data: Using default values\n");
		ms5607->pdata = &ms5607_default_pdata;
	}

	/* validate OSR configuration */
	if (ms5607->pdata->osr >= MS5607_NUM_OSR_CFG) {
		/* invalid OSR configuration */

		dev_err(ms5607->dev, "Invalid OSR configuration in platform data, using defaults\n");
		ms5607->pdata->osr = ms5607_default_pdata.osr;
	}

	ms5607->regulator = regulator_get(ms5607->dev, "VDD");
	if (IS_ERR(ms5607->regulator)) {
		dev_err(ms5607->dev, "Failed to get regulator for VDD\n");
		rc = PTR_ERR(ms5607->regulator);
		goto err_get_regulator;
	}

	if (ms5607_read_calibration_data(ms5607)) {
		goto err_read_cal_data;
	}

	ms5607->status.opened = 0;
	ms5607->status.disabled = 0;
	ms5607->status.suspended = 0;

	mutex_init(&ms5607->lock);

	/* prepare and start worker thread */
	ms5607->worker.wake_up = 0;
	init_waitqueue_head(&ms5607->worker.wq);

	ms5607->worker.thread = kthread_run(ms5607_worker, ms5607, "ms5607");
	if (IS_ERR(ms5607->worker.thread)) {
		dev_err(ms5607->dev, "Failed to create worker thread\n");
		goto err_kthread;
	}

	/* set up sampling timer */
	hrtimer_init(&ms5607->worker.timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL_PINNED);
	ms5607->worker.timer.function = ms5607_trigger_sampling;

#ifdef CONFIG_EARLYSUSPEND
	ms5607->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB;
	ms5607->early_suspend.suspend = ms5607_early_suspend;
	ms5607->early_suspend.resume = ms5607_early_resume;

	register_early_suspend(&ms5607->early_suspend);
#endif

	/* create sysfs attribute */
	rc = device_create_file(ms5607->dev, &dev_attr_disabled);
	if (rc) {
		dev_err(ms5607->dev, "Failed to create sysfs file for disabled attribute\n");
		goto err_sysfs_disabled;
	}

	rc = ms5607_create_input_dev(ms5607);
	if (rc)
		goto err_create_input_dev;

	return 0;

err_create_input_dev:
	device_remove_file(&ms5607->spi_dev->dev, &dev_attr_disabled);

#ifdef CONFIG_EARLYSUSPEND
	unregister_early_suspend(&ms5607->early_suspend);
#endif

err_sysfs_disabled:
	kthread_stop(ms5607->worker.thread);

err_kthread:
	regulator_put(ms5607->regulator);

err_read_cal_data:
err_get_regulator:
	kfree(ms5607);

err_alloc_ms5607:
	return rc;
}

static int ms5607_remove(struct spi_device *spi)
{
	struct ms5607 *ms5607 = dev_get_drvdata(&spi->dev);

	/* no need to disable the device explicitly, as close() will
	   be called anyway if it is open */

	/* wait for the worker thread to exit */
	kthread_stop(ms5607->worker.thread);

	input_unregister_device(ms5607->input);
	input_free_device(ms5607->input);

#ifdef CONFIG_EARLYSUSPEND
	unregister_early_suspend(&ms5607->early_suspend);
#endif

	device_remove_file(ms5607->dev, &dev_attr_disabled);

	regulator_put(ms5607->regulator);

	kfree(ms5607);

	return 0;
}

static ssize_t ms5607_sysfs_show_disabled(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct ms5607 *ms5607 = dev_get_drvdata(dev);

	snprintf(buf, PAGE_SIZE, "%d", ms5607->status.disabled);

	return strlen(buf) + 1;
}

static ssize_t ms5607_sysfs_store_disabled(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct ms5607 *ms5607 = dev_get_drvdata(dev);
	unsigned long value;
	int rc;

	rc = strict_strtoul(buf, 10, &value);
	if (rc)
		return rc;

	mutex_lock(&ms5607->lock);

	if (!ms5607->status.suspended && ms5607->status.opened) {
		if (value) {
			if (!ms5607->status.disabled)
				__ms5607_disable(ms5607);
		} else {
			if (ms5607->status.disabled)
				__ms5607_enable(ms5607);
		}
	}

	ms5607->status.disabled = !!value;

	mutex_unlock(&ms5607->lock);

	return count;
}

static struct spi_driver ms5607_driver = {
	.driver		= {
		.name	= DRIVER_NAME,
		.bus	= &spi_bus_type,
		.owner	= THIS_MODULE,
	},
	.probe		= ms5607_probe,
	.remove		= ms5607_remove,
	.suspend	= ms5607_suspend,
        .resume		= ms5607_resume,
};

static int __init ms5607_init(void)
{
	const int rc = spi_register_driver(&ms5607_driver);
	if (rc)
		printk(KERN_ERR "ms5607: Could not register SPI driver! Error=%d\n", rc);

	return rc;
}

static void __exit ms5607_exit(void)
{
	spi_unregister_driver(&ms5607_driver);
}

module_init(ms5607_init);
module_exit(ms5607_exit);

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_AUTHOR("Matthias Kaehlcke <matthias.kaehlcke@tomtom.com>");
MODULE_LICENSE("GPL");
