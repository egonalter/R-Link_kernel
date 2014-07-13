/*
 * drivers/tomtom/ewts98/ewts98.c
 *
 * Panasonic EWTS98 Angular Rate Sensor driver
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

#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/ewts98.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/types.h>
#include <linux/wait.h>

#define DRIVER_NAME	"ewts98"
#define DEVICE_NAME	"Panasonic EWTS98 Angular Rate Sensor"
#define DRIVER_DESC	DEVICE_NAME " Driver"

#define PI_X_1M		3141592

typedef enum {
	EWTS98_AXIS_X,
	EWTS98_AXIS_Y,
} ewts98_axis_t;

struct etws98_model_struct
{
	int dynamic_range;	/* °/s */
	int sensitivity;	/* uV per °/s */
};

static struct etws98_model_struct ewts98_models[EWTS98_NUM_MODELS] = {
	[EWTS98L] = {
		.dynamic_range	= 30,
		.sensitivity	= 38000
	},

	[EWTS98C] = {
		.dynamic_range	= 15,
		.sensitivity	= 65000
	},

	[EWTS9CV] = {
		.dynamic_range	= 300,
		.sensitivity	= 2000
	},

	[EWTS98PA21] = {
		.dynamic_range	= 50,
		.sensitivity	= 10500
	}
};

struct ewts98
{
	struct {
		int opened;
		int disabled;
		int suspended;
	} status;

	struct {
		int *samples_x;
		int *samples_y;
		uint8_t sample_count;
	} avg;

	struct {
		struct task_struct *thread;
		wait_queue_head_t wq;
		struct hrtimer timer;
		int wake_up;
	} worker;

	/* (micro-)radians/s per ADC count */
	int adc_x_urad_s_per_count;
	int adc_y_urad_s_per_count;

	struct mutex lock;

	struct device *dev;
	struct ewts98_platform_data *pdata;

	struct input_dev *input;
	char phys[32];
	struct regulator *regulator;

#ifdef CONFIG_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
};

static ssize_t ewts98_sysfs_show_disabled(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t ewts98_sysfs_store_disabled(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);

static DEVICE_ATTR(disabled, S_IRUGO | S_IWUSR, ewts98_sysfs_show_disabled, ewts98_sysfs_store_disabled);

/*
  enable the device (the call must hold the device lock)
*/
static int __ewts98_enable(struct ewts98 *ewts98)
{
	int rc;

	/* discard old samples */
	ewts98->avg.sample_count = 0;

	rc = regulator_enable(ewts98->regulator);
	if (rc) {
		dev_err(ewts98->dev, "Failed to enable regulator: %d\n", rc);
		goto err_reg_enable;
	}

	/* start the timer that triggers sampling */
	hrtimer_start(&ewts98->worker.timer,
		ns_to_ktime(ewts98->pdata->sample_rate * 1000 * 1000),
		HRTIMER_MODE_REL);

	rc = 0;

err_reg_enable:
	return rc;
}

/*
   disable the device (the call must hold the device lock)
*/
static int __ewts98_disable(struct ewts98 *ewts98)
{
	int rc;

	hrtimer_cancel(&ewts98->worker.timer);
	ewts98->worker.wake_up = 0;

	rc = regulator_disable(ewts98->regulator);
	if (rc) {
		dev_err(ewts98->dev, "Failed to disable regulator: %d\n", rc);
		goto err_reg_disable;
	}

	rc = 0;

err_reg_disable:
	return rc;
}

static int ewts98_input_open(struct input_dev *input)
{
	struct ewts98 *ewts98 = input_get_drvdata(input);

	mutex_lock(&ewts98->lock);

	if (!ewts98->status.suspended && !ewts98->status.disabled)
		__ewts98_enable(ewts98);

	ewts98->status.opened = 1;

	mutex_unlock(&ewts98->lock);

	return 0;
}

static void ewts98_input_close(struct input_dev *input)
{
	struct ewts98 *ewts98 = input_get_drvdata(input);

	mutex_lock(&ewts98->lock);

	if (!ewts98->status.suspended && !ewts98->status.disabled)
		__ewts98_disable(ewts98);

	ewts98->status.opened = 0;

	mutex_unlock(&ewts98->lock);
}

static inline int ewts98_calc_average(struct ewts98 *ewts98, ewts98_axis_t axis)
{
	int i;
	int sum = 0;
	int *samples = ((axis == EWTS98_AXIS_X)?
			ewts98->avg.samples_x : ewts98->avg.samples_y);

	for (i = 0; i < ewts98->pdata->averaging.num_samples; i++)
		sum += samples[i];

	return sum / ewts98->pdata->averaging.num_samples;
}

static void ewts98_report_value(struct ewts98 *ewts98, ewts98_axis_t axis, int adc_value)
{
	int ev_code;
	int zero_point_offset;
	int urad_s_per_count;
	int mrad_per_sec;

	if (axis == EWTS98_AXIS_X) {
		ev_code = ABS_X;
		zero_point_offset = ewts98->pdata->adc_x.zero_point_offset;
		urad_s_per_count = ewts98->adc_x_urad_s_per_count;
	} else {
		ev_code = ABS_Y;
		zero_point_offset = ewts98->pdata->adc_y.zero_point_offset;
		urad_s_per_count = ewts98->adc_y_urad_s_per_count;
	}

	mrad_per_sec = ((adc_value - zero_point_offset) * urad_s_per_count) / 1000;

	input_event(ewts98->input, EV_ABS, ev_code, mrad_per_sec);
}

static void ewts98_log_adc_error(struct ewts98 *ewts98, int adc_error)
{
	if (printk_ratelimit()) {
		if ((adc_error & EWTS98_ERROR_X) &&
			(adc_error & EWTS98_ERROR_Y))
			dev_warn(ewts98->dev, "failed to read ADCs of X and Y axis\n");
		else if (adc_error & EWTS98_ERROR_X)
			dev_warn(ewts98->dev, "failed to read ADC of X axis\n");
		else if (adc_error & EWTS98_ERROR_Y)
			dev_warn(ewts98->dev, "failed to read ADC of Y axis\n");
		else
			dev_err(ewts98->dev, "unsupported ADC error\n");
	}
}

static int ewts98_worker(void *data)
{
	struct ewts98 *ewts98 = (struct ewts98 *)data;

	do {
		wait_event_interruptible(ewts98->worker.wq,
					kthread_should_stop() || ewts98->worker.wake_up);

		if (ewts98->worker.wake_up) {
			/* normal work */

			int adc_status;
			struct ewts98_adc_values adc_values = {
				.x = EWTS98_INVALID_VALUE,
				.y = EWTS98_INVALID_VALUE
			};

			/* race condition alert: the timer could have expired
			   again and set wake_up to 1. in this unlikely case we
			   would loose one sample, we are aware of this and can
			   live with it */
			ewts98->worker.wake_up = 0;

			mutex_lock(&ewts98->lock);

			adc_status = ewts98->pdata->read_adcs(&adc_values);
			if (adc_status)
				ewts98_log_adc_error(ewts98, adc_status);

			/* we continue in case of errors, but take care that only values
			   read from the ADCs are reported */

			if (ewts98->pdata->averaging.enabled) {
				ewts98->avg.samples_x[ewts98->avg.sample_count] = adc_values.x;
				ewts98->avg.samples_y[ewts98->avg.sample_count] = adc_values.y;

				ewts98->avg.sample_count++;

				if (ewts98->avg.sample_count == ewts98->pdata->averaging.num_samples) {
					const int avg_x = ewts98_calc_average(ewts98, EWTS98_AXIS_X);
					const int avg_y = ewts98_calc_average(ewts98, EWTS98_AXIS_Y);

					if (avg_x != EWTS98_INVALID_VALUE)
						ewts98_report_value(ewts98, EWTS98_AXIS_X, avg_x);

					if (avg_y != EWTS98_INVALID_VALUE)
						ewts98_report_value(ewts98, EWTS98_AXIS_Y, avg_y);

					input_event(ewts98->input, EV_ABS, ABS_MISC, jiffies);
					input_sync(ewts98->input);

					ewts98->avg.sample_count = 0;
				}
			} else {
				if (adc_values.x != EWTS98_INVALID_VALUE)
					ewts98_report_value(ewts98, EWTS98_AXIS_X, adc_values.x);

				if (adc_values.y != EWTS98_INVALID_VALUE)
					ewts98_report_value(ewts98, EWTS98_AXIS_Y, adc_values.y);

				input_event(ewts98->input, EV_ABS, ABS_MISC, jiffies);
				input_sync(ewts98->input);
			}

			mutex_unlock(&ewts98->lock);
		}
	} while (!kthread_should_stop());

	return 0;
}

static enum hrtimer_restart ewts98_trigger_sampling(struct hrtimer *hrtimer)
{
	struct ewts98 *ewts98 = container_of(hrtimer, struct ewts98, worker.timer);

	hrtimer_forward_now(hrtimer, ns_to_ktime(ewts98->pdata->sample_rate * 1000 * 1000));

	ewts98->worker.wake_up = 1;
	wake_up(&ewts98->worker.wq);

	return HRTIMER_RESTART;
}

#ifdef CONFIG_PM
void evdev_suspend(struct input_dev *dev);

static int __ewts98_suspend(struct ewts98 *ewts98)
{
	dev_info(ewts98->dev, "suspend\n");

	mutex_lock(&ewts98->lock);

	if (!ewts98->status.suspended && !ewts98->status.disabled && ewts98->status.opened)
		__ewts98_disable(ewts98);

	evdev_suspend(ewts98->input);

	ewts98->status.suspended = 1;

	mutex_unlock(&ewts98->lock);

	return 0;
}

static int __ewts98_resume(struct ewts98 *ewts98)
{
	dev_info(ewts98->dev, "resume\n");

	mutex_lock(&ewts98->lock);

	if (ewts98->status.suspended && !ewts98->status.disabled && ewts98->status.opened)
		__ewts98_enable(ewts98);

	ewts98->status.suspended = 0;

	mutex_unlock(&ewts98->lock);

	return 0;
}

#ifdef CONFIG_EARLYSUSPEND
static void ewts98_early_suspend(struct early_suspend *handler) {
	struct ewts98 *ewts98 = container_of(handler, struct ewts98, early_suspend);

	__ewts98_suspend(ewts98);
}

static void ewts98_early_resume(struct early_suspend *handler) {
	struct ewts98 *ewts98 = container_of(handler, struct ewts98, early_suspend);

	__ewts98_resume(ewts98);
}

#define ewts98_suspend NULL
#define ewts98_resume  NULL
#else
static int ewts98_suspend(struct spi_device *spi, pm_message_t state)
{
	struct ewts98 *ewts98 = dev_get_drvdata(&spi->dev);

	return __ewts98_suspend(ewts98);
}

static int ewts98_resume(struct spi_device *spi)
{
	struct ewts98 *ewts98 = dev_get_drvdata(&spi->dev);

	return __ewts98_resume(ewts98);
}
#endif

#else
#define ewts98_suspend NULL
#define ewts98_resume  NULL
#endif /* CONFIG_PM */

/* calculate the number of (micro-)radians per adc count */
static inline int ewts98_urad_s_per_adc_count(struct ewts98 *ewts98, struct ewts98_adc_data *adc)
{
	/* voltage range of the ADC */
	const int adc_range = adc->max_input_mV - adc->min_input_mV;

	/* uV per ADC count */
	const int adc_uV_per_count = (adc_range * 1000) / (1 << adc->resolution);

	/* (milli-)degrees/s per ADC count */
	const int mdeg_s_per_count = ((adc_uV_per_count * 1000) / ewts98_models[ewts98->pdata->model].sensitivity);

	/* convert from (milli-)degrees to (micro-)radians */
	return ((mdeg_s_per_count * PI_X_1M) / (180 * 1000));
}

static int etws98_create_input_dev(struct ewts98 *ewts98)
{
	int rc;
	struct input_dev *input_dev;
	const int ewts98_range = (ewts98_models[ewts98->pdata->model].dynamic_range * PI_X_1M) / (180 * 1000);

	input_dev = input_allocate_device();
	if (NULL == input_dev) {
		dev_err(ewts98->dev, "Error allocating input device\n");
		rc = -ENOMEM;
		goto err_alloc_input;
	}

	ewts98->input = input_dev;

	input_dev->name = DEVICE_NAME;
	snprintf(ewts98->phys, sizeof(ewts98->phys), "%s/input0", dev_name(ewts98->dev));
	input_dev->phys = ewts98->phys;
	input_dev->dev.parent = ewts98->dev;

	input_dev->open = ewts98_input_open;
	input_dev->close = ewts98_input_close;

	input_set_drvdata(input_dev, ewts98);

	__set_bit(EV_ABS, input_dev->evbit);

	__set_bit(ABS_X, input_dev->absbit);
	__set_bit(ABS_Y, input_dev->absbit);
	/* the TT positioning engine expects data at a fixed rate. input devices don't
	   deliver redundant data. therefore we use send an ABS_MISC event with the
	   sensor data to indicate the Androids libsensors that we are alive and
	   force it to feed the sensor manager with the values received previously in
	   case the sensor data hasn't changed */
	__set_bit(ABS_MISC, input_dev->absbit);

	input_set_abs_params(input_dev, ABS_X, -ewts98_range, ewts98_range, ewts98->pdata->fuzz_x, 0);
	input_set_abs_params(input_dev, ABS_Y, -ewts98_range, ewts98_range, ewts98->pdata->fuzz_y, 0);

	rc = input_register_device(ewts98->input);
	if (rc) {
		dev_err(ewts98->dev, "Failed to register input device");
		input_free_device(input_dev);
	}

err_alloc_input:
	return rc;
}

static int ewts98_probe(struct platform_device *pdev)
{
	int rc = -1;
	struct ewts98 *ewts98;

	dev_info(&pdev->dev, DRIVER_DESC "\n");

	if (pdev->dev.platform_data == NULL) {
		dev_err(&pdev->dev, "No platform data provided\n");
		goto err_no_pdata;
	}

	ewts98 = kzalloc(sizeof(*ewts98), GFP_KERNEL);
	if (NULL == ewts98) {
		dev_err(&pdev->dev, "Error allocating memory for private data\n");
		rc = -ENOMEM;
		goto err_alloc_ewts98;
	}

	ewts98->dev = &pdev->dev;
	ewts98->pdata = ewts98->dev->platform_data;

	if (ewts98->pdata->averaging.enabled) {
		ewts98->avg.samples_x = kmalloc(ewts98->pdata->averaging.num_samples * sizeof(int), GFP_KERNEL);
		ewts98->avg.samples_y = kmalloc(ewts98->pdata->averaging.num_samples * sizeof(int), GFP_KERNEL);
	}

	platform_set_drvdata(pdev, ewts98);

	ewts98->regulator = regulator_get(ewts98->dev, "VDD");
	if (IS_ERR(ewts98->regulator)) {
		dev_err(ewts98->dev, "Failed to get regulator for VDD\n");
		rc = PTR_ERR(ewts98->regulator);
		goto err_regulator_get;
	}

	ewts98->status.opened = 0;
	ewts98->status.disabled = 0;
	ewts98->status.suspended = 0;

	ewts98->adc_x_urad_s_per_count = ewts98_urad_s_per_adc_count(ewts98, &ewts98->pdata->adc_x);
	ewts98->adc_y_urad_s_per_count = ewts98_urad_s_per_adc_count(ewts98, &ewts98->pdata->adc_y);

	mutex_init(&ewts98->lock);

	/* prepare and start worker thread */
	ewts98->worker.wake_up = 0;
	init_waitqueue_head(&ewts98->worker.wq);

	ewts98->worker.thread = kthread_run(ewts98_worker, ewts98, "ewts98");
	if (IS_ERR(ewts98->worker.thread)) {
		dev_err(ewts98->dev, "Failed to create worker thread\n");
		goto err_kthread;
	}

	/* set up sampling timer */
	hrtimer_init(&ewts98->worker.timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL_PINNED);
	ewts98->worker.timer.function = ewts98_trigger_sampling;

#ifdef CONFIG_EARLYSUSPEND
	ewts98->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB;
	ewts98->early_suspend.suspend = ewts98_early_suspend;
	ewts98->early_suspend.resume = ewts98_early_resume;

	register_early_suspend(&ewts98->early_suspend);
#endif

	/* create sysfs attributes */
	rc = device_create_file(ewts98->dev, &dev_attr_disabled);
	if (rc) {
		dev_err(ewts98->dev, "Failed to create sysfs file for disabled attribute");
		goto err_sysfs_disabled;
	}

	/* create input device */
	rc = etws98_create_input_dev(ewts98);
	if (rc)
		goto err_create_input_dev;

	return 0;

err_create_input_dev:
	device_remove_file(ewts98->dev, &dev_attr_disabled);

#ifdef CONFIG_EARLYSUSPEND
	unregister_early_suspend(&ewts98->early_suspend);
#endif

err_sysfs_disabled:
	kthread_stop(ewts98->worker.thread);

err_kthread:
	regulator_put(ewts98->regulator);

err_regulator_get:
	if (ewts98->pdata->averaging.enabled) {
		kfree(ewts98->avg.samples_x);
		kfree(ewts98->avg.samples_y);
	}

	kfree(ewts98);

err_alloc_ewts98:
err_no_pdata:
	return rc;
}

static int ewts98_remove(struct platform_device *pdev)
{
	struct ewts98 *ewts98 = dev_get_drvdata(&pdev->dev);

	/* no need to disable the device explicitly, as close() will
	   be called anyway if it is open */

	/* wait for the worker thread to exit */
	kthread_stop(ewts98->worker.thread);

	input_unregister_device(ewts98->input);
	input_free_device(ewts98->input);

#ifdef CONFIG_EARLYSUSPEND
	unregister_early_suspend(&ewts98->early_suspend);
#endif

	device_remove_file(ewts98->dev, &dev_attr_disabled);

	regulator_put(ewts98->regulator);

	if (ewts98->pdata->averaging.enabled) {
		kfree(ewts98->avg.samples_x);
		kfree(ewts98->avg.samples_y);
	}

	kfree(ewts98);

	return 0;
}

static ssize_t ewts98_sysfs_show_disabled(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct ewts98 *ewts98 = dev_get_drvdata(dev);

	snprintf(buf, PAGE_SIZE, "%d", ewts98->status.disabled);

	return strlen(buf) + 1;
}

static ssize_t ewts98_sysfs_store_disabled(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct ewts98 *ewts98 = dev_get_drvdata(dev);
	unsigned long value;
	int rc;

	rc = strict_strtoul(buf, 10, &value);
	if (rc)
		return rc;

	mutex_lock(&ewts98->lock);

	if (!ewts98->status.suspended && ewts98->status.opened) {
		if (value) {
			if (!ewts98->status.disabled)
				__ewts98_disable(ewts98);
		} else {
			if (ewts98->status.disabled)
				__ewts98_enable(ewts98);
		}
	}

	ewts98->status.disabled = !!value;

	mutex_unlock(&ewts98->lock);

	return count;
}

static struct platform_driver ewts98_driver = {
	.driver		= {
		.name	= DRIVER_NAME,
	},
	.probe		= ewts98_probe,
	.remove		= ewts98_remove,
	.suspend	= ewts98_suspend,
        .resume		= ewts98_resume,
};


static int __init ewts98_init(void)
{
	const int rc = platform_driver_register(&ewts98_driver);
	if (rc)
		printk(KERN_ERR "Could not register ewts98 platform driver! Error=%d", rc);

	return rc;
}

static void __exit ewts98_exit(void)
{
	platform_driver_unregister(&ewts98_driver);
}

module_init(ewts98_init);
module_exit(ewts98_exit);

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_AUTHOR("Matthias Kaehlcke <matthias.kaehlcke@tomtom.com>");
MODULE_LICENSE("GPL");
