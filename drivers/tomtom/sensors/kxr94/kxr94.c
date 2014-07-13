/*
 * drivers/tomtom/kxr94/kxr94.c
 *
 * Kionix KXR94 Three-axis Accelerometer driver
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

#include <linux/earlysuspend.h>
#include <linux/hrtimer.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/kxr94.h>
#include <linux/platform_device.h>
#include <linux/regulator/consumer.h>
#include <linux/spi/spi.h>
#include <linux/types.h>
#include <linux/wait.h>

#define DRIVER_NAME		"kxr94"
#define DEVICE_NAME		"Kionix KXR94 Three-axis Accelerometer"
#define DRIVER_DESC		DEVICE_NAME " Driver"

#define GRAVITY_EARTH		9807 /* g * 1000 */

#define KXR94_CMD_READ_CTRL	0x03
#define KXR94_CMD_WRITE_CTRL	0x04
#define KXR94_CTRL_ENABLE	(1 << 2)

#define KXR94_MODE_STANDBY	0
#define KXR94_MODE_ON		1

#define KXR94_NUM_AXIS		3

/* #define KXR94_SAMPLE_AUX_IN */

#ifdef KXR94_SAMPLE_AUX_IN
#define KXR94_NUM_CHANNELS	4
#else
#define KXR94_NUM_CHANNELS	3
#endif

#define KXR94_CHAN_AUX_IN	3

#define KXR94_NUM_SAMPLES_AVG	10

#define KXR94_NUM_SUPPLIES	2

#define KXR94_FUZZ		0 /* experimental value, when set to 20 no events
				     are generated when the devices is resting,
				     but we are still very responsive to slight
				     movements */

#define KXR94_INVALID_VALUE	0xffff

static const char *kxr94_supply_names[KXR94_NUM_SUPPLIES] = {
	"VDD",
	"IO_VDD",
};

struct kxr94_channel {
	int cmd;
	int ev_code;
};

static struct kxr94_channel kxr94_channels[KXR94_NUM_CHANNELS] = {
	[0] = {
		.cmd		= 0x00,
		.ev_code	= ABS_X
	},

	[1] = {
		.cmd		= 0x01,
		.ev_code	= ABS_Y
	},

	[2] = {
		.cmd		= 0x02,
		.ev_code	= ABS_Z
	},

#ifdef KXR94_SAMPLE_AUX_IN
 	[3] = {
 		.cmd		= 0x07,
 		/* ev_code is set in probe() */
  	}
#endif
};

struct kxr94;

struct kxr94_async_spi_context
{
	struct kxr94 *kxr94;
	struct {
		struct spi_message msg;
		struct spi_transfer transfer[2];
		uint8_t rx_buf[2];
	} spi;
	uint16_t adc_value;
};

struct kxr94
{
	struct {
		int opened;
		int disabled;
		int suspended;
	} status;

	struct {
		uint16_t channels[KXR94_NUM_CHANNELS][KXR94_NUM_SAMPLES_AVG];
		uint8_t sample_count;
	} avg;

	struct {
		struct task_struct *thread;
		wait_queue_head_t wq;
		struct hrtimer timer;
		int wake_up;
	} worker;

	struct {
		struct kxr94_async_spi_context context[KXR94_NUM_CHANNELS];
		atomic_t transfers_completed;
	} async_spi;

	struct kxr94_platform_data *pdata;

	struct mutex lock;
	struct device *dev;
	struct spi_device *spi_dev;
	struct input_dev *input;
	char phys[32];
	struct regulator_bulk_data supplies[KXR94_NUM_SUPPLIES];

#ifdef CONFIG_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
};

static struct kxr94_platform_data kxr94_default_pdata = {
	.sample_rate		= 10,
	.zero_g_offset		= 2048,
	.sensitivity		= 819,
	.aux_in_ev_type		= EV_ABS,
	.aux_in_ev_code		= ABS_RX, /* ABS_MISC is used for the heartbeat */
	.averaging_enabled	= 1
};

/* prototypes */
static int kxr94_set_mode(struct kxr94 *kxr94, int mode);

static ssize_t kxr94_sysfs_show_disabled(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t kxr94_sysfs_store_disabled(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);

static DEVICE_ATTR(disabled, S_IRUGO | S_IWUSR, kxr94_sysfs_show_disabled, kxr94_sysfs_store_disabled);

static int __kxr94_enable(struct kxr94 *kxr94)
{
	int rc;

	/* discard old samples */
	kxr94->avg.sample_count = 0;

	rc = regulator_bulk_enable(ARRAY_SIZE(kxr94->supplies),
				kxr94->supplies);
	if (rc) {
		dev_err(kxr94->dev, "Failed to enable regulators: %d\n", rc);
		goto err_reg_enable;
	}

	kxr94_set_mode(kxr94, KXR94_MODE_ON);

	/* start the timer that triggers sampling */
	hrtimer_start(&kxr94->worker.timer, ns_to_ktime(kxr94->pdata->sample_rate * 1000 * 1000),
		HRTIMER_MODE_REL);

err_reg_enable:
	return rc;
}

static int __kxr94_disable(struct kxr94 *kxr94)
{
	int rc;

	hrtimer_cancel(&kxr94->worker.timer);
	kxr94->worker.wake_up = 0;

	kxr94_set_mode(kxr94, KXR94_MODE_STANDBY);

	rc = regulator_bulk_disable(ARRAY_SIZE(kxr94->supplies),
				kxr94->supplies);
	if (rc)
		dev_err(kxr94->dev, "Failed to disable regulators: %d\n", rc);

	return rc;
}

static int kxr94_input_open(struct input_dev *input)
{
	struct kxr94 *kxr94 = input_get_drvdata(input);

	mutex_lock(&kxr94->lock);

	if (!kxr94->status.suspended && !kxr94->status.disabled)
		__kxr94_enable(kxr94);

	kxr94->status.opened = 1;

	mutex_unlock(&kxr94->lock);

	return 0;
}

static void kxr94_input_close(struct input_dev *input)
{
	struct kxr94 *kxr94 = input_get_drvdata(input);

	mutex_lock(&kxr94->lock);

	if (!kxr94->status.suspended && !kxr94->status.disabled)
		__kxr94_disable(kxr94);

	kxr94->status.opened = 0;

	mutex_unlock(&kxr94->lock);
}

static void kxr94_async_spi_complete(void *context)
{
	struct kxr94_async_spi_context *async_spi_ctx = (struct kxr94_async_spi_context *)context;
	struct kxr94 *kxr94 = async_spi_ctx->kxr94;

	if (!async_spi_ctx->spi.msg.status) {
		/* convert to platform byte-order */
		async_spi_ctx->adc_value = be16_to_cpu(*((uint16_t *)async_spi_ctx->spi.rx_buf));

		/* 12-bit ADC => 16-bit value */
		async_spi_ctx->adc_value >>= 4;
	} else {
		if (printk_ratelimit())
			dev_warn(kxr94->dev,
				"Failed to read ADC value from SPI (error = %d)\n",
				async_spi_ctx->spi.msg.status);

		async_spi_ctx->adc_value = KXR94_INVALID_VALUE;
	}

	atomic_inc(&kxr94->async_spi.transfers_completed);
	wake_up(&kxr94->worker.wq);
}

static int kxr94_start_async_spi(struct kxr94 *kxr94, int channel)
{
	struct kxr94_async_spi_context *async_spi_ctx = &kxr94->async_spi.context[channel];
	struct spi_message *msg = &async_spi_ctx->spi.msg;
	struct spi_transfer *transfer = async_spi_ctx->spi.transfer;

	spi_message_init(msg);
	memset(transfer, 0, 2 * sizeof(struct spi_transfer));

	transfer->tx_buf = &kxr94_channels[channel].cmd;
	transfer->len = 1;
	transfer->delay_usecs = 40;
	spi_message_add_tail(transfer, msg);

	transfer = &async_spi_ctx->spi.transfer[1];
	transfer->rx_buf = async_spi_ctx->spi.rx_buf;
	transfer->len = 2;
	spi_message_add_tail(transfer, msg);

	msg->spi = kxr94->spi_dev;
	msg->is_dma_mapped = 0;
	msg->context = async_spi_ctx;
	msg->complete = kxr94_async_spi_complete;

	async_spi_ctx->kxr94 = kxr94;

	return spi_async(kxr94->spi_dev, msg);
}

/* convert ADC value to m/s2 * 1000 */
static inline int kxr94_adc_to_si(struct kxr94 *kxr94, uint16_t adc_value)
{
	return ((adc_value - kxr94->pdata->zero_g_offset) * GRAVITY_EARTH) / kxr94->pdata->sensitivity;
}

static inline void kxr94_report_value(struct kxr94 *kxr94, int channel, uint16_t adc_value)
{
	if (channel != KXR94_CHAN_AUX_IN)
		/* acceleration */
		input_event(kxr94->input,
			EV_ABS,
			kxr94_channels[channel].ev_code,
			kxr94_adc_to_si(kxr94, adc_value));
	else
		/* aux in */
		input_event(kxr94->input,
			EV_ABS,
			kxr94_channels[channel].ev_code,
			adc_value);
}

static inline uint16_t kxr94_calc_average(struct kxr94 *kxr94, int channel)
{
	int i;
	uint32_t sum = 0;
	uint16_t avg;
	int valid_samples = 0;

	for (i = 0; i < KXR94_NUM_SAMPLES_AVG; i++) {
		const uint16_t value = kxr94->avg.channels[channel][i];

		if (value != KXR94_INVALID_VALUE) {
			sum += value;
			valid_samples++;
		}
	}

	if (valid_samples != 0)
		avg = (uint16_t)(sum / valid_samples);
	else
		avg = KXR94_INVALID_VALUE;

	return avg;
}

static int kxr94_worker(void *data)
{
	struct kxr94 *kxr94 = (struct kxr94 *)data;

	do {
		wait_event_interruptible(kxr94->worker.wq,
					kxr94->worker.wake_up || kthread_should_stop());

		if (kxr94->worker.wake_up) {
			/* normal work */

			int i;

			/* race condition alert: the timer could have expired
			   again and set wake_up to 1. in this unlikely case we
			   would loose one sample, we are aware of this and can
			   live with it */
			kxr94->worker.wake_up = 0;

			mutex_lock(&kxr94->lock);

			atomic_set(&kxr94->async_spi.transfers_completed, 0);

			/* request read of all channels */
			for (i = 0; i < KXR94_NUM_CHANNELS; i++) {
				/* start asynchronous SPI read of the ADC. asynchronous SPI is
				   used to reduce overhead from scheduling between this thread
				   and the SPI workqueue for each channel. a significant overhead
				   has been observed when sampling at high frequencies (100Hz) */
				if (kxr94_start_async_spi(kxr94, i)) {
					dev_err(kxr94->dev, "Failed to read from device\n");
					continue;
				}
			}

			/* release the mutex before going to sleep */
			mutex_unlock(&kxr94->lock);

			/* wait for all async SPI transfers to complete */
			wait_event_interruptible(kxr94->worker.wq,
						(atomic_read(&kxr94->async_spi.transfers_completed) == KXR94_NUM_CHANNELS) ||
						kthread_should_stop());

			if (atomic_read(&kxr94->async_spi.transfers_completed) == KXR94_NUM_CHANNELS) {
				mutex_lock(&kxr94->lock);

				for (i = 0; i < KXR94_NUM_CHANNELS; i++) {
					const uint16_t adc_value = kxr94->async_spi.context[i].adc_value;

					if (kxr94->pdata->averaging_enabled) {
						kxr94->avg.channels[i][kxr94->avg.sample_count] =
							adc_value;
					} else {
						if (adc_value != KXR94_INVALID_VALUE)
							kxr94_report_value(kxr94, i, adc_value);
					}
				}

				if (kxr94->pdata->averaging_enabled) {
					kxr94->avg.sample_count++;

					if (kxr94->avg.sample_count == KXR94_NUM_SAMPLES_AVG) {
						/* calculate the average value for all channels and report it */

						for (i = 0; i < KXR94_NUM_CHANNELS; i++) {
							const uint16_t avg = kxr94_calc_average(kxr94, i);

							if (avg != KXR94_INVALID_VALUE)
								kxr94_report_value(kxr94, i, avg);
						}

						input_event(kxr94->input, EV_ABS, ABS_MISC, jiffies);
						input_sync(kxr94->input);
						kxr94->avg.sample_count = 0;
					}
				} else {
					/* no averaging */

					input_event(kxr94->input, EV_ABS, ABS_MISC, jiffies);
					input_sync(kxr94->input);
				}

				mutex_unlock(&kxr94->lock);
			}
		}
	} while (!kthread_should_stop());

	return 0;
}

static enum hrtimer_restart kxr94_trigger_sampling(struct hrtimer *hrtimer)
{
	struct kxr94 *kxr94 = container_of(hrtimer, struct kxr94, worker.timer);

	hrtimer_forward_now(hrtimer, ns_to_ktime(kxr94->pdata->sample_rate * 1000 * 1000));

	kxr94->worker.wake_up = 1;
	wake_up(&kxr94->worker.wq);

	return HRTIMER_RESTART;
}

static int kxr94_set_mode(struct kxr94 *kxr94, int mode)
{
	unsigned char tx_buf[2];
	struct spi_message msg;
	struct spi_transfer transfer;

	tx_buf[0] = KXR94_CMD_WRITE_CTRL;
	if (mode == KXR94_MODE_ON)
		tx_buf[1] = KXR94_CTRL_ENABLE;
	else
		tx_buf[1] = 0;

	memset(&msg, 0, sizeof(msg));
	memset(&transfer, 0, sizeof(transfer));
	spi_message_init(&msg);

	transfer.tx_buf = tx_buf;
	transfer.len = sizeof(tx_buf);
	spi_message_add_tail(&transfer, &msg);

	msg.spi = kxr94->spi_dev;
	msg.is_dma_mapped = 0;

	return spi_sync(kxr94->spi_dev, &msg);
}

#ifdef CONFIG_PM
void evdev_suspend(struct input_dev *dev);

static int __kxr94_suspend(struct kxr94 *kxr94) {

	dev_info(kxr94->dev, "suspend\n");

	mutex_lock(&kxr94->lock);

	if (!kxr94->status.suspended && !kxr94->status.disabled && kxr94->status.opened)
		__kxr94_disable(kxr94);

	evdev_suspend(kxr94->input);

	kxr94->status.suspended = 1;

	mutex_unlock(&kxr94->lock);

	return 0;
}

static int __kxr94_resume(struct kxr94 *kxr94)
{
	dev_info(kxr94->dev, "resume\n");

	mutex_lock(&kxr94->lock);

	if (kxr94->status.suspended && !kxr94->status.disabled && kxr94->status.opened)
		__kxr94_enable(kxr94);

	kxr94->status.suspended = 0;

	mutex_unlock(&kxr94->lock);

	return 0;
}

#ifdef CONFIG_EARLYSUSPEND
static void kxr94_early_suspend(struct early_suspend *handler) {
	struct kxr94 *kxr94 = container_of(handler, struct kxr94, early_suspend);

	__kxr94_suspend(kxr94);
}

static void kxr94_early_resume(struct early_suspend *handler) {
	struct kxr94 *kxr94 = container_of(handler, struct kxr94, early_suspend);

	__kxr94_resume(kxr94);
}

#define kxr94_suspend NULL
#define kxr94_resume  NULL
#else
static int kxr94_suspend(struct spi_device *spi, pm_message_t state)
{
	struct kxr94 *kxr94 = dev_get_drvdata(&spi->dev);

	return __kxr94_suspend(kxr94);
}

static int kxr94_resume(struct spi_device *spi)
{
	struct kxr94 *kxr94 = dev_get_drvdata(&spi->dev);

	return __kxr94_resume(kxr94);
}
#endif

#else
#define kxr94_suspend NULL
#define kxr94_resume  NULL
#endif /* CONFIG_PM */

static int kxr94_create_input_dev(struct kxr94 *kxr94)
{
	int rc;
	struct input_dev *input_dev;
	int i;

	input_dev = input_allocate_device();
	if (NULL == input_dev) {
		dev_err(kxr94->dev, "Error allocating input device\n");
		rc = -ENOMEM;
		goto err_alloc_input;
	}

	kxr94->input = input_dev;

	input_dev->name = DEVICE_NAME;
	snprintf(kxr94->phys, sizeof(kxr94->phys), "%s/input0", dev_name(kxr94->dev));
	input_dev->phys = kxr94->phys;
	input_dev->dev.parent = kxr94->dev;

	input_dev->id.bustype = BUS_SPI;
	input_dev->open = kxr94_input_open;
	input_dev->close = kxr94_input_close;

	input_set_drvdata(input_dev, kxr94);

	__set_bit(EV_ABS, input_dev->evbit);

	for (i = 0; i < KXR94_NUM_AXIS; i++) {
		__set_bit(kxr94_channels[i].ev_code, input_dev->absbit);
		input_set_abs_params(input_dev, kxr94_channels[i].ev_code, -(GRAVITY_EARTH * 2), (GRAVITY_EARTH * 2), KXR94_FUZZ, 0);
	}

	__set_bit(kxr94->pdata->aux_in_ev_type, input_dev->evbit);

	/* the TT positioning engine expects data at a fixed rate. input devices don't
	   deliver redundant data. therefore we use send an ABS_MISC event with the
	   sensor data to indicate the Androids libsensors that we are alive and
	   force it to feed the sensor manager with the values received previously in
	   case the sensor data hasn't changed */
	__set_bit(ABS_MISC, input_dev->absbit);

#ifdef KXR94_SAMPLE_AUX_IN
	if (kxr94->pdata->aux_in_ev_type == EV_ABS) {
		__set_bit(kxr94_channels[KXR94_CHAN_AUX_IN].ev_code, input_dev->absbit);
		input_set_abs_params(input_dev, kxr94_channels[KXR94_CHAN_AUX_IN].ev_code, 0, 4096, 0, 0);
	} else if (kxr94->pdata->aux_in_ev_type == EV_REL) {
		__set_bit(kxr94_channels[KXR94_CHAN_AUX_IN].ev_code, input_dev->relbit);
	} else {
		dev_warn(kxr94->dev, "Unsupported event type in for channel AuxIn in platform data: %d\n",
			kxr94->pdata->aux_in_ev_type);
	}
#endif

	rc = input_register_device(input_dev);
	if (rc) {
		dev_err(kxr94->dev, "Failed to register input device");
		input_free_device(input_dev);
	}

err_alloc_input:
	return rc;
}

static int kxr94_probe(struct spi_device *spi)
{
	int rc;
	int i;
	struct kxr94 *kxr94;

	dev_info(&spi->dev, DRIVER_DESC "\n");

	kxr94 = kzalloc(sizeof(*kxr94), GFP_KERNEL);
	if (NULL == kxr94) {
		dev_err(&spi->dev, "Error allocating memory for private data\n");
		rc = -ENOMEM;
		goto err_alloc_kxr94;
	}

	kxr94->dev = &spi->dev;
	kxr94->spi_dev = spi;

	spi_set_drvdata(spi, kxr94);

	if (kxr94->dev->platform_data) {
		kxr94->pdata = kxr94->dev->platform_data;
	} else {
		dev_dbg(kxr94->dev, "No platform data: Using default values\n");
		kxr94->pdata = &kxr94_default_pdata;
	}

#ifdef KXR94_SAMPLE_AUX_IN
	kxr94_channels[KXR94_CHAN_AUX_IN].ev_code = kxr94->pdata->aux_in_ev_code;
#endif

	for (i = 0; i < ARRAY_SIZE(kxr94->supplies); i++)
		kxr94->supplies[i].supply = kxr94_supply_names[i];

	rc = regulator_bulk_get(kxr94->dev, ARRAY_SIZE(kxr94->supplies),
				kxr94->supplies);
	if (rc != 0) {
		dev_err(kxr94->dev, "Failed to request supplies: %d\n", rc);
		goto err_regulator_get;
	}

	kxr94->status.opened = 0;
	kxr94->status.disabled = 0;
	kxr94->status.suspended = 0;

	mutex_init(&kxr94->lock);

	/* prepare and start worker thread */
	kxr94->worker.wake_up = 0;
	init_waitqueue_head(&kxr94->worker.wq);

	kxr94->worker.thread = kthread_run(kxr94_worker, kxr94, "kxr94-%s", dev_name(kxr94->dev));
	if (IS_ERR(kxr94->worker.thread)) {
		dev_err(kxr94->dev, "Failed to create worker thread\n");
		goto err_kthread;
	}

	hrtimer_init(&kxr94->worker.timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL_PINNED);
	kxr94->worker.timer.function = kxr94_trigger_sampling;

#ifdef CONFIG_EARLYSUSPEND
	kxr94->early_suspend.level = EARLY_SUSPEND_LEVEL_DISABLE_FB;
	kxr94->early_suspend.suspend = kxr94_early_suspend;
	kxr94->early_suspend.resume = kxr94_early_resume;

	register_early_suspend(&kxr94->early_suspend);
#endif

	/* create sysfs attributes */
	rc = device_create_file(kxr94->dev, &dev_attr_disabled);
	if (rc) {
		dev_err(kxr94->dev, "Failed to create sysfs file for disabled attribute");
		goto err_sysfs_disabled;
	}

	/* create input device */
	rc = kxr94_create_input_dev(kxr94);
	if (rc)
		goto err_create_input_dev;

	return 0;

err_create_input_dev:
	device_remove_file(kxr94->dev, &dev_attr_disabled);

#ifdef CONFIG_EARLYSUSPEND
	unregister_early_suspend(&kxr94->early_suspend);
#endif

err_sysfs_disabled:
	kthread_stop(kxr94->worker.thread);

err_kthread:
	regulator_bulk_free(ARRAY_SIZE(kxr94->supplies), kxr94->supplies);

err_regulator_get:
	kfree(kxr94);

err_alloc_kxr94:
	return rc;
}

static int kxr94_remove(struct spi_device *spi)
{
	struct kxr94 *kxr94 = dev_get_drvdata(&spi->dev);

	/* no need to disable the device explicitly, as close() will
	   be called anyway if it is open */

	/* wait for the worker thread to exit */
	kthread_stop(kxr94->worker.thread);

	input_unregister_device(kxr94->input);
	input_free_device(kxr94->input);

#ifdef CONFIG_EARLYSUSPEND
	unregister_early_suspend(&kxr94->early_suspend);
#endif

	device_remove_file(kxr94->dev, &dev_attr_disabled);

	regulator_bulk_free(ARRAY_SIZE(kxr94->supplies), kxr94->supplies);

	kfree(kxr94);

	return 0;
}

static ssize_t kxr94_sysfs_show_disabled(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct kxr94 *kxr94 = dev_get_drvdata(dev);

	snprintf(buf, PAGE_SIZE, "%d", kxr94->status.disabled);

	return strlen(buf) + 1;
}

static ssize_t kxr94_sysfs_store_disabled(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	struct kxr94 *kxr94 = dev_get_drvdata(dev);
	unsigned long value;
	int rc;

	rc = strict_strtoul(buf, 10, &value);
	if (rc)
		return rc;

	mutex_lock(&kxr94->lock);

	if (!kxr94->status.suspended && kxr94->status.opened) {
		if (value) {
			if (!kxr94->status.disabled)
				__kxr94_disable(kxr94);
		} else {
			if (kxr94->status.disabled)
				__kxr94_enable(kxr94);
		}
	}

	kxr94->status.disabled = !!value;

	mutex_unlock(&kxr94->lock);

	return count;
}

static struct spi_driver kxr94_driver = {
	.driver		= {
		.name	= DRIVER_NAME,
		.bus	= &spi_bus_type,
		.owner	= THIS_MODULE,
	},
	.probe		= kxr94_probe,
	.remove		= kxr94_remove,
	.suspend	= kxr94_suspend,
	.resume		= kxr94_resume,
};

static int __init kxr94_init(void)
{
	const int rc = spi_register_driver(&kxr94_driver);
	if (rc)
		printk(KERN_ERR "kxr94: Could not register SPI driver! Error=%d", rc);

	return rc;
}

static void __exit kxr94_exit(void)
{
	spi_unregister_driver(&kxr94_driver);
}

module_init(kxr94_init);
module_exit(kxr94_exit);

MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_AUTHOR("Matthias Kaehlcke <matthias.kaehlcke@tomtom.com>");
MODULE_LICENSE("GPL");
