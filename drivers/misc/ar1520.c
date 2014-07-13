/*
* ar1520.c: driver for Atheros AR1520 GPS chipset
*
* Copyright (c) 2011 Atheros Communications Inc.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
* MA  02110-1301, USA.
*
* I2C code is derived from i2c-dev.c - i2c-bus driver, char device interface
* Copyright (C) 1995-97 Simon G. Vogl
* Copyright (C) 1998-99 Frodo Looijaard <frodol@dds.nl>
* Copyright (C) 2003 Greg Kroah-Hartman <greg@kroah.com>
*
* GPIO code is derived from tc35894xbg.c: Keypad driver for Toshiba
* TC35894XBG
*
* (C) Copyright 2010 Intel Corporation
* Author: Charlie Paul (z8cpaul@windriver.com)
*
* You need a userspace application to cooperate with this driver. It and
* more information about this driver can be obtained here:
*
* http://wireless.kernel.org/en/users/Drivers/ar5120
*
*/
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/wait.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/irq.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/cdev.h>
#include <linux/poll.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/major.h>
#include <linux/version.h>
#include <linux/io.h>
#include <linux/uaccess.h>
#include <asm/system.h>
#include <linux/mutex.h>
#include <linux/spinlock.h>
#include <linux/ar1520.h>

#define DRV_VERSION		"1.0.0.2"
#define AR1520A_NO_ERROR		0
#define AR1520A_SIG_TERM		1
#define POLL_RTS			0
#define TWO_EDGE_IRQ_RTS		1
#define IRQ_AND_POLL_RTS		2
#define AR1520_DBG_INFO			0
#define AR1520_DBG_WARNING		1
#define AR1520_DBG_ERROR		2
#define AR1520_DBG_ALWAYS		2

struct ar1520_data {
	unsigned gps_gpio_rts;
	/* RTS gpio line number */
	unsigned gps_gpio_wakeup;
	/* WAKEUP gpio line number */
	unsigned gps_gpio_reset;
	/* RESET gpio line number */
	unsigned gps_irq;
	/* RTS irq number */
	struct i2c_client *i2c_client;
	/* handler to access i2c driver */
	unsigned rts_flag;
	/* flag showing that RTS got up and requires processing */
	unsigned use_rts;
	/* flag showing is driver should use RTS
		line or just translate read requests to i2c driver */
	struct mutex wr_mutex;
	/* mutex for data write */
	struct mutex rts_mutex;
	/* mutex for rts */
	struct mutex gpio_mutex;
	/* mutex for gpio */
	spinlock_t rts_flag_lock;
	/* spin lock for rts flag */
	spinlock_t rts_state_lock;
	/* spin lock  for rts state */
	char *tmp_buf;
	/* buffer for read operation from i2c bus */
	unsigned rts_state;
	/* flag reflecting state of RTS pin
	in case of two edge interrupt registration */
	unsigned rts_irq_handling;
	/* method of interrupt processing */
	wait_queue_head_t wait_irq;
	/* IRQ handler notifies this wait queue on receipt of an IRQ */
	struct cdev cdev;
	/* struct of cdev */
};

static int ar1520_major;
static struct class *ar1520_class;
static int min_debug_level = AR1520_DBG_INFO;

static int ar1520_set_rts_state(struct ar1520_data *data, int value)
{
	dev_dbg(&data->i2c_client->adapter->dev,
		"ar1520_data->rts_state=%d\n", data->rts_state);

	spin_lock(&data->rts_state_lock);
	data->rts_state = value;
	spin_unlock(&data->rts_state_lock);
	return 0;
}

static int set_rts_flag(struct ar1520_data *data, int new_val)
{
	dev_dbg(&data->i2c_client->adapter->dev,
		"set_rts_flag(%d)\n", new_val);
	spin_lock(&data->rts_flag_lock);
	data->rts_flag = new_val;
	spin_unlock(&data->rts_flag_lock);
	return 0;
}

static irqreturn_t gpsrts_irq_handler(int irq, void *devid)
{
	struct ar1520_data *data = devid;
	dev_dbg(&data->i2c_client->adapter->dev,
		"%s start\n", __func__);
	if (data->rts_irq_handling == IRQ_AND_POLL_RTS) {
		ar1520_set_rts_state(data, 1);
		set_rts_flag(data, 1);
	} else {
		ar1520_set_rts_state(data, gpio_get_value(data->gps_gpio_rts));
		if (data->rts_state)
			set_rts_flag(data, 1);
	}
	wake_up_interruptible(&data->wait_irq);
	return IRQ_HANDLED;
}


int ar1520_setup_gpio(struct ar1520_data *data)
{
	int ret;
	ret = gpio_request(data->gps_gpio_reset, "reset");
	if (ret < 0) {
		dev_dbg(&data->i2c_client->adapter->dev,
			"GPIO pin %d request failed, error %d\n",
			data->gps_gpio_reset, ret);
//		return 1;
	}
	ret = gpio_direction_output(data->gps_gpio_reset, 1);
	if (ret < 0) {
		dev_dbg(&data->i2c_client->adapter->dev,
			"GPIO pin %d direction configuration failed, \
			error %d\n",
			data->gps_gpio_reset, ret);
//		goto err_release_gpio_reset;
	}

	ret = gpio_request(data->gps_gpio_wakeup, "wakeup");
	if (ret < 0) {
		dev_dbg(&data->i2c_client->adapter->dev,
			"GPIO pin %d request failed, error %d\n",
			data->gps_gpio_wakeup, ret);
//		goto err_release_gpio_reset;
	}
	ret = gpio_direction_output(data->gps_gpio_wakeup, 1);
	if (ret < 0) {
		dev_dbg(&data->i2c_client->adapter->dev,
			"GPIO pin %d direction configuration failed, \
			error %d\n",
			data->gps_gpio_wakeup, ret);
//		goto err_release_gpio_wakeup;
	}

	ret = gpio_request(data->gps_gpio_rts, "rts");
	if (ret < 0) {
		dev_dbg(&data->i2c_client->adapter->dev,
			"GPIO pin %d request failed, error %d\n",
			data->gps_gpio_rts, ret);
		goto err_release_gpio_wakeup;
	}

	ret = gpio_direction_input(data->gps_gpio_rts);
	if (ret < 0) {
		dev_dbg(&data->i2c_client->adapter->dev,
			"GPIO pin %d direction configuration failed, \
			error %d\n",
			data->gps_gpio_rts, ret);
		goto err_release_gpio_rts;
	}
	ret = gpio_to_irq(data->gps_gpio_rts);
	if (ret < 0) {
		dev_dbg(&data->i2c_client->adapter->dev,
			"GPIO pin %d to IRQ failed, error %d\n",
			data->gps_gpio_rts, ret);
		goto err_release_gpio_rts;
	}
	data->gps_irq = ret;

	ret = request_threaded_irq(data->gps_irq,
		NULL, gpsrts_irq_handler,
	IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "ar1520_rts", data);
	if (ret) {
		dev_dbg(&data->i2c_client->adapter->dev,
			"Could not get GPS_IRQ = %d\n", data->gps_irq);
		goto err_release_irq;
	}

	return 0;

err_release_irq:
	free_irq(data->gps_irq, data);
err_release_gpio_rts:
	gpio_free(data->gps_gpio_rts);
err_release_gpio_wakeup:
	gpio_free(data->gps_gpio_wakeup);
err_release_gpio_reset:
	gpio_free(data->gps_gpio_reset);

	return 1;
}

int ar1520_gpio_set_value(struct ar1520_data *data, unsigned gpio)
{
	int ret = 0;
	if (gpio == 0)
		return 0;

	ret = mutex_lock_interruptible(&data->gpio_mutex);
	if (ret) {
		ret = -ERESTARTSYS;
		goto exit;
	}

	gpio_set_value(gpio, 0);
	msleep(100);
	gpio_set_value(gpio, 1);
	mutex_unlock(&data->gpio_mutex);
exit:
	return ret;
}

static int ar1520_check_rts_flag_or_wait_rts_irq(struct ar1520_data *data)
{
	int err;
	/* This procedure checks if rts flag is
		already set and waits for it otherwise */
	unsigned long timeout;

	dev_dbg(&data->i2c_client->adapter->dev,
		"%s start\n", __func__);
	if (data->rts_irq_handling == POLL_RTS) {
		timeout = jiffies + (10 * HZ);
		while (1) {
			if (time_after(jiffies, timeout)) {
				dev_dbg(&data->i2c_client->adapter->dev,
					"%s:timeout!!\n", __func__);
				return -ETIMEDOUT;
			}
			if (gpio_get_value(data->gps_gpio_rts) == 1)
				break;
			 msleep(50);
		}
		return AR1520A_NO_ERROR;
	} else if (data->rts_irq_handling == TWO_EDGE_IRQ_RTS) {
		if (data->rts_state)
			return AR1520A_NO_ERROR;
	} else if (data->rts_irq_handling == IRQ_AND_POLL_RTS) {
		if (gpio_get_value(data->gps_gpio_rts))
			return AR1520A_NO_ERROR;
		else
			ar1520_set_rts_state(data, 0);
	}

keep_wait:
	err = wait_event_interruptible_timeout(data->wait_irq,
		data->rts_flag != 0, msecs_to_jiffies(1000000));
	set_rts_flag(data, 0);
	if (err == 0) {
		if (data->rts_irq_handling == TWO_EDGE_IRQ_RTS) {
			if (data->rts_state) {
				dev_dbg(&data->i2c_client->adapter->dev,
				"%s:irq was missed\n", __func__);
				return AR1520A_NO_ERROR;
			}
		}

		else if (data->rts_irq_handling == IRQ_AND_POLL_RTS) {
			if (gpio_get_value(data->gps_gpio_rts)) {
				dev_dbg(&data->i2c_client->adapter->dev,
				"%s:irq was missed\n", __func__);
				return AR1520A_NO_ERROR;
			}
		}
		goto keep_wait;
	} else if (err < 0) {
		return AR1520A_SIG_TERM;
	} else {
		return AR1520A_NO_ERROR;
	}
}

static int report_reading_error(struct ar1520_data *data,
		int code, int retval)
{
	/* This procedure must do all necessary clean-ups */
	/* release possibly allocated buffer */
	if (data->tmp_buf != NULL) {
		kfree(data->tmp_buf);
		data->tmp_buf = NULL;
	}

	if (code == AR1520A_NO_ERROR)
		return retval;
	else {
		dev_dbg(&data->i2c_client->adapter->dev,
			"get error [%d]!\n", code);
		return code;
	}
}

static void ar1520_debug_arr_print(char *name, char *arr, int len)
{
	int i, x;

	if (min_debug_level > AR1520_DBG_INFO)
		return;

	pr_err("%s", name);
	pr_err("=[");
	for (i = 0; i < len; i++) {
		x = arr[i]&0xff;
		pr_err("%02x", x);
	}
	pr_err("]\n");
}

static int remove_fillers(char *p, const char filler_ch, int len)
{
	char *dst = p;
	char *src = p;
	char *end = p + len;
	char value;
	int res;

	while (src < end) {
		value = *src;
		if (value != filler_ch) {
			*dst = value;
			 dst++;
		}
		src++;
	}
	res = src - dst;
	return res;
}

static ssize_t ar1520_read(struct file *file,
	char __user *buf, size_t count, loff_t *offset)
{
	int n;
	int rt = 0;
	int ret, cf;
	struct ar1520_data *data = file->private_data;

	dev_dbg(&data->i2c_client->adapter->dev,
		"%s start\n", __func__);
	/* limit max read oeration size to 8K */
	if (count > 8192)
		count = 8192;
	n = count;
	/* allocate buffer, return error if impossible*/
	data->tmp_buf = kmalloc(count, GFP_KERNEL);
	if (data->tmp_buf == NULL)
		return -ENOMEM;
	if (file->f_flags & O_NONBLOCK) {
		ret = mutex_lock_interruptible(&data->wr_mutex);
		if (ret) {
			ret = -ERESTARTSYS;
			return report_reading_error(data, ret, 0);
		}

		ret = i2c_master_recv(data->i2c_client,
			&data->tmp_buf[rt], n);
		mutex_unlock(&data->wr_mutex);
		if (ret < 0) {
			/* reading error happened, report proper code */
			return report_reading_error(data, ret, 0);
		}
	} else {
		while (1) {
			if (ar1520_check_rts_flag_or_wait_rts_irq(data) ==
				AR1520A_NO_ERROR) {
				ret = mutex_lock_interruptible(
					&data->wr_mutex);
				if (ret) {
					ret = -ERESTARTSYS;
					return report_reading_error(
						data, ret, 0);
				}

				/* we either received interrupt or
					rts flag was already set:
					anyway safe to read*/
				ret = i2c_master_recv(data->i2c_client,
					&data->tmp_buf[rt], n);
				mutex_unlock(&data->wr_mutex);
				if (ret < 0) {
					/* reading error happened,
						report proper code */
					return report_reading_error(
						data, ret, 0);
				} else {
					/* Take into account how
					many bytes were actually read */
					if (ret != n)
						n = ret;
					cf = remove_fillers(
						&data->tmp_buf[rt],
							0xff, n);
					rt = rt + n - cf;
					ar1520_debug_arr_print(
						"collected so far",
						data->tmp_buf, rt);
					n = cf;
					if (n == 0) {
						/*  All data received,
							exit reading loop */
						break;
					}
				}
			} else {
				/* error happened witing interrupt,
					report proper code */
				return report_reading_error(data,
					-ETIMEDOUT, 0);
			}
		}
	}

	if (file->f_flags & O_NONBLOCK) {
		ar1520_debug_arr_print("non-blocking", data->tmp_buf, n);
		ret = copy_to_user(buf,
			data->tmp_buf, n) ? -EFAULT : n;
	} else {
		ret = copy_to_user(buf,
			data->tmp_buf, rt) ? -EFAULT : rt;
	}

	if (ret >= 0)
		return report_reading_error(data, AR1520A_NO_ERROR, ret);
	else
		return report_reading_error(data, ret, 0);
}


/*
 * I2C transactions with error messages.
 */
static ssize_t ar1520_write(struct file *file, const char __user *buf,
		size_t count, loff_t *offset)
{
	int ret;
	char *tmp;

	struct ar1520_data *data = file->private_data;
	dev_dbg(&data->i2c_client->adapter->dev,
		"%s start\n", __func__);
	if (count > 8192)
		count = 8192;
	tmp = memdup_user(buf, count);
	if (IS_ERR(tmp))
		return PTR_ERR(tmp);

	ret = mutex_lock_interruptible(&data->wr_mutex);
	if (ret) {
		ret = -ERESTARTSYS;
		goto exit;
	}

	ret = i2c_master_send(data->i2c_client, tmp, count);
	mutex_unlock(&data->wr_mutex);
	if (ret < 0)
		dev_err(&data->i2c_client->dev, "send error: %d\n", ret);
exit:
	kfree(tmp);
	return ret;
}

static int ar1520_open(struct inode *inode, struct file *file)
{
	struct ar1520_data *data = container_of(inode->i_cdev,
		struct ar1520_data, cdev);

	file->f_flags &= O_NONBLOCK;
	file->private_data = data;
	return 0;
}

static long ar1520_ioctl(struct file *file,	unsigned int cmd,
			unsigned long arg)
{
	int err = 0;
	struct ar1520_data *data = file->private_data;
	struct ar1520_ioctl ar1520_ioctl_pl;

	if (_IOC_TYPE(cmd) != AR1520_IOCTL_MAGIC)
		return -ENOTTY;
	if (_IOC_NR(cmd) > AR1520_IOCTL_MAXNR)
		return -ENOTTY;
	if (_IOC_DIR(cmd) & _IOC_READ)
		err = !access_ok(VERIFY_WRITE,
			(void __user *)arg, _IOC_SIZE(cmd));
	else if (_IOC_DIR(cmd) & _IOC_WRITE)
		err = !access_ok(VERIFY_READ,
			(void __user *)arg, _IOC_SIZE(cmd));

	if (err)
		return -EFAULT;

	memset(&ar1520_ioctl_pl, 0, sizeof(struct ar1520_ioctl));

	switch (cmd) {
	case AR1520_IOCTL_RESET:
		ar1520_gpio_set_value(data, data->gps_gpio_reset);
		break;
	case AR1520_IOCTL_WAKEUP:
		ar1520_gpio_set_value(data, data->gps_gpio_wakeup);
		break;
	case AR1520_IOCTL_GET_RTS:
		ar1520_ioctl_pl.value =
			gpio_get_value(data->gps_gpio_rts);
		if (copy_to_user((void *)arg, &ar1520_ioctl_pl,
			sizeof(ar1520_ioctl_pl)))
			return -EFAULT;
		break;
	case AR1520_IOCTL_ENABLE_IRQ:
		if (copy_from_user(&ar1520_ioctl_pl, (void __user *)arg,
			sizeof(ar1520_ioctl_pl)))
			return -EFAULT;
		dev_dbg(&data->i2c_client->adapter->dev,
			"ar1520_ioctl_pl.rts_irq_handling=%d\n",
				ar1520_ioctl_pl.value);
		data->rts_irq_handling =
			ar1520_ioctl_pl.value;
		err = mutex_lock_interruptible(&data->rts_mutex);
		if (err)
			return -ERESTARTSYS;

		free_irq(data->gps_irq, data);
		if (data->rts_irq_handling == TWO_EDGE_IRQ_RTS)
			err = request_threaded_irq(data->gps_irq,
				NULL, gpsrts_irq_handler,
				IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
				"ar1520_rts", data);
		else if (data->rts_irq_handling == IRQ_AND_POLL_RTS)
			err = request_threaded_irq(data->gps_irq,
				NULL, gpsrts_irq_handler,
				IRQF_TRIGGER_RISING , "ar1520_rts",
				data);
		if (err)
			dev_dbg(&data->i2c_client->adapter->dev,
				"Could not get GPS_IRQ = %d\n",
				data->gps_irq);
		mutex_unlock(&data->rts_mutex);
		break;
	case AR1520_IOCTL_DEBUG_LEVEL:
		if (copy_from_user(&ar1520_ioctl_pl, (void __user *)arg,
			sizeof(ar1520_ioctl_pl)))
			return -EFAULT;
		min_debug_level = ar1520_ioctl_pl.value;
		break;
	default:
		break;
	}
	return 0;
}

static const struct file_operations ar1520_fops = {
	.owner		= THIS_MODULE,
	.read		= ar1520_read,
	.write		= ar1520_write,
	.open		= ar1520_open,
	.unlocked_ioctl = ar1520_ioctl,
};

static int __init ar1520_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	struct ar1520_data *data;
	struct ar1520_platform_data *pdata;
	int ret;
	data = kzalloc(sizeof(struct ar1520_data), GFP_KERNEL);
	if (!data) {
		dev_err(&client->dev,	"Could not allocate AR1520 device\n");
		return -ENOMEM;
	}
	/* get platform data */
	pdata = client->dev.platform_data;
	if (!pdata) {
		dev_err(&client->dev, "No platform data\n");
		ret = -EINVAL;
		goto failed;
	}
	cdev_init(&data->cdev, &ar1520_fops);
	data->cdev.owner = THIS_MODULE;
	ret = cdev_add(&data->cdev, MKDEV(ar1520_major, 0), 1);
	if (ret) {
		dev_err(&client->dev, "Could not add cdev\n");
		goto failed;
	}
	if (IS_ERR(device_create(ar1520_class, NULL,
		MKDEV(ar1520_major, 0), NULL, AR1520_DEV)))
		dev_err(&client->dev, "can't create device\n");
	dev_info(&client->dev, "ar1520 v.%s", DRV_VERSION);

	data->i2c_client = client;
	data->gps_gpio_reset = pdata->gps_gpio_reset;
	data->gps_gpio_wakeup = pdata->gps_gpio_wakeup;
	data->gps_gpio_rts = pdata->gps_gpio_rts;
	data->rts_flag = 0;
	data->use_rts = 0;
	data->rts_state = 1;
	data->rts_irq_handling = IRQ_AND_POLL_RTS;
	i2c_set_clientdata(client, data);
	spin_lock_init(&data->rts_flag_lock);
	spin_lock_init(&data->rts_state_lock);
	mutex_init(&data->wr_mutex);
	mutex_init(&data->rts_mutex);
	mutex_init(&data->gpio_mutex);
	init_waitqueue_head(&data->wait_irq);
	ar1520_setup_gpio(data);

	return 0;
failed:
	kfree(data);
	return ret;
}

static int ar1520_remove(struct i2c_client *client)
{
	struct ar1520_data *data = i2c_get_clientdata(client);

	device_destroy(ar1520_class, MKDEV(ar1520_major, 0));
	cdev_del(&data->cdev);
	wake_up_interruptible(&data->wait_irq);
	free_irq(data->gps_irq, data);
	gpio_free(data->gps_gpio_rts);
	gpio_free(data->gps_gpio_wakeup);
	gpio_free(data->gps_gpio_reset);

	return 0;
}

static const struct i2c_device_id ar1520_id[] = {
	{ "ath1520a", 0 },
	{ }
};

static struct i2c_driver ar1520_driver = {
	.driver = {
		.name	= "ath1520a",
		.owner  = THIS_MODULE,
	},
	.probe		= ar1520_probe,
	.remove		= ar1520_remove,
	.id_table	= ar1520_id,
};

static int __init ar1520_init(void)
{
	int ret;
	dev_t dev;

	ar1520_class = class_create(THIS_MODULE, AR1520_DEV);
	if (IS_ERR(ar1520_class)) {
		ret = PTR_ERR(ar1520_class);
		goto error;
	}

	ret = alloc_chrdev_region(&dev, 0, 1, AR1520_DEV);
	if (ret)
		goto class_destroy;

	ar1520_major = MAJOR(dev);
	i2c_add_driver(&ar1520_driver);

	return 0;

class_destroy:
	class_destroy(ar1520_class);
error:
	return ret;
}

static void __exit ar1520_exit(void)
{
	i2c_del_driver(&ar1520_driver);
	unregister_chrdev(ar1520_major, AR1520_DEV);
	class_destroy(ar1520_class);
}

module_init(ar1520_init);
module_exit(ar1520_exit);

MODULE_AUTHOR("Allen Kao <allen.kao@atheros.com>, \
	Jekyll <jekyll.lai@gmail.com>");
MODULE_DESCRIPTION("Atheros AR1520a driver");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_VERSION(DRV_VERSION);
