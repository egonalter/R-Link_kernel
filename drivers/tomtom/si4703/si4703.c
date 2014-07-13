/* si4703.c
 *
 * Control driver for Silicon Lab 4703 chip.
 *
 * Copyright (C) 2006 TomTom BV <http://www.tomtom.com/>
 * Authors: Xander Hover <Xander.Hover@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/fmtransmitter.h>	/* XXX guba - remove any reference to this in the final version */
#include <linux/i2c.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <asm/irq.h>
//#include <asm/arch/irqs.h>
#include <linux/wait.h>   /* needed to implement blocking read and wait queues */
#include <linux/poll.h>   /* needed for poll_wait() call */
#include <linux/kfifo.h>  /* needed for the kfifo ringbuffer */
//#include <asm/arch/gpio.h>
#include <linux/platform_device.h>

#include <plat/dock.h>

#include "si4703.h"			/* XXX guba */
#include "fmreceiver.h"			/* XXX guba */
#include "si470x_bit_fields.h"

#define PFX "SI4703: "
#define PK_DBG(fmt, arg...)    printk(KERN_DEBUG PFX "%s: " fmt, __FUNCTION__ ,##arg)
#define PK_ERR(fmt, arg...)    printk(KERN_ERR PFX "%s: " fmt, __FUNCTION__ ,##arg)

#define I2C_DRIVERID_SI4703	70

#ifdef LOGGING
#define PK_X(fmt, arg...)     printk(fmt, ##arg);
#else
#define PK_X(fmt, arg...)
#endif

static atomic_t si4703_available = ATOMIC_INIT(1); /* there shall only be one user of the device */

#ifdef RDSQ_LOGGING
static u32 prev_rds_good=0;
#endif

/* Spec says SI4703 sits on address 0x10 */
static unsigned short normal_i2c[] = { SI4703_I2C_SLAVE_ADDR, I2C_CLIENT_END };

/* Magic definition of all other variables and things */
I2C_CLIENT_INSMOD;

static void si4703_workqueue_handler(struct work_struct *work);
static int si470x_initialize(void);
static int si470x_disable(void);
static int si470x_powerup(void);
int si470x_reg_write(u8 number_of_reg);
int si470x_reg_read(u8 number_of_reg, u16 *plreg);
int si470x_seek(struct fmr_seek_param_type *seek_param);
static int si470x_seek_next(u8 seekup, u8 seekmode);

extern int	si470x_set_band(struct si4703dev *, __u16);
extern int	si470x_tune(struct si4703dev *, __u16 freq);
extern int	si470x_set_frequency(struct si4703dev *, __u16);
extern int	si470x_set_volume(struct si4703dev *, __u16);
extern int	si470x_set_rds_enabled(struct si4703dev *, __u16);
extern int	si470x_enable_rds(struct si4703dev *);
extern int	si470x_disable_rds(struct si4703dev *);
extern u16	si470x_get_current_frequency(struct si4703dev *);
extern u8	si470x_get_current_rssi(struct si4703dev *);

static int si4703_fifo_alloc(struct si4703dev *dev, int size);
static void si4703_fifo_free(struct si4703dev *dev);
void		si4703_fifo_purge(struct si4703dev *dev);
static int si4703_fifo_len(struct si4703dev *dev);
int 		si4703_fifo_put(struct si4703dev *dev, unsigned char *data, int len);
int si4703_fifo_get(struct si4703dev *dev, unsigned char *data, int len);

u16 freqToChan(u16 freq);
#ifdef LOGGING
void si470x_reg_dump(u16 *lreg, int startreg, int endreg);
#endif

static int si4703_probe(struct i2c_client *client, const struct i2c_device_id *id);
static int si4703_remove(struct i2c_client *client);
#if 0
static int si4703_core_probe(struct device *dev);
static int si4703_core_suspend(struct device *dev, pm_message_t state, u32 level);
static int si4703_core_resume(struct device *dev, u32 level);
#endif

static irqreturn_t si4703_interrupthandler(int irq, void *dev_id);

static int si4703_open(struct inode *nodep, struct file *filep);
static int si4703_release(struct inode *nodep, struct file *filep);
static int si4703_ioctl(struct inode *nodep, struct file *filep, unsigned int cmd, unsigned long arg);
static unsigned int si4703_poll(struct file *filep, struct poll_table_struct *pq);
static ssize_t si4703_read(struct file *filep, char __user * buf, size_t count, loff_t * pos);

extern void	si4703_sysfs_register(struct device	*dev);
extern void	si4703_sysfs_unregister(struct device	*dev);

static const struct i2c_device_id si470x_id[] = {
	{ "si4703", 0x4703 },
	{ "si4702", 0x4702 },
	{ }
};

static struct i2c_driver driver = {
	.id		= I2C_DRIVERID_SI4703,
	.id_table	= si470x_id,
	.probe		= si4703_probe,
	.remove		= si4703_remove,
	.driver 	= {
		.name 	= "si4703", // XXX - SI4703_DEVNAME,
		.owner = THIS_MODULE,
#if 0
	.suspend = si4703_core_suspend,
	.resume = si4703_core_resume,
#endif
	}
};

#ifdef TO_REMOVE
static struct i2c_client client_template = {
	.name = "si4703_i2c_client",
	.usage_count = 0,
	.driver = &driver,
	.addr = SI4703_I2C_SLAVE_ADDR,
};
#endif

struct file_operations si4703_fops = {
	.owner = THIS_MODULE,
	.open = si4703_open,
	.release = si4703_release,
	.ioctl = si4703_ioctl,
	.poll = si4703_poll,
	.read = si4703_read,
};

typedef struct si4703_registers si4703_registers_t;

#define	preg_shadow	(&gdev->shadow_regs)
#define	si470x_shadow	((u16 *) preg_shadow)
volatile u8 WaitSTCInterrupt = 0;
static struct si4703dev *gdev = NULL;  /* global pointer used everywhere */
static int si4703_suspended = 0;

static int si4703_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int			result;

	ENTER();

	/* General bookkeeping initialization... */
	gdev->client		= client;
	gdev->irq		= client->irq;
	gdev->set_volume	= si470x_set_volume;
	gdev->set_freq		= si470x_set_frequency;
	gdev->set_band		= si470x_set_band;
	gdev->set_rds_enabled	= si470x_set_rds_enabled;
	gdev->device_id		= 0xFFFF;

#ifdef RDSQ_LOGGING
	gdev->prev_rds_good	= 0;
	gdev->rdsq		= 0;
	gdev->rds_timestamp_ms	= 0;
#endif

	si4703_fifo_purge(gdev);

	// XXX guba - reset the chip _again_. This has to be moved to the dock driver
	// once I know for sure that this is indeed why I can sometimes get 0x0a00 in
	// chipid.
#if 0
	gpio_direction_output(3,0);
	gpio_set_value(149, 0);
	gpio_set_value(24, 0);
	mdelay(100);
	gpio_set_value(24, 128);
	gpio_set_value(149,1);
	gpio_direction_input(3);
	//mdelay(100);
#endif

	/* Initial register read */
	si470x_reg_read(NR_OF_REGISTERS, si470x_shadow);
	si470x_reg_dump(si470x_shadow, 0x00, 0x0F);

	/* Powerup the device */
	si470x_powerup();

	/* set RDS verbose mode */
	preg_shadow->powercfg |= SI47_RDSM;

	/* set STCI enable, GPIO2 == RDS/STC Interrupt */
	preg_shadow->sysconfig1 |= SI47_STCIEN | SI47_GPIO2_INT /*(2 << 2)*/;

	/* set RDS high performance mode */
	preg_shadow->sysconfig3 |= SI47_RDSPRF;

	//write (POWERCFG...SYSCONFIG3)
	if (si470x_reg_write(5) != 0) {
		PK_ERR("ERROR @ %d, si470x_reg_write\n", __LINE__);
		return -1;
	}

	si470x_reg_read(NR_OF_REGISTERS, si470x_shadow);
#ifdef LOGGING
	si470x_reg_dump(si470x_shadow, 0x00, 0x0F);
	printk("deviceid=%#x, chipid=%#x rssi=%#x powercfg=%#x\n", 
		preg_shadow->deviceid, preg_shadow->chipid, preg_shadow->statusrssi, 
		preg_shadow->powercfg);
#endif

	/* Perform basic checks on the read registers */
	if (preg_shadow->deviceid != 0x1242 || 
		((preg_shadow->chipid >> 6) & 0xF) != 0x0009) {
		printk("Error during chip initialization, devid=%d, chipid=%d, rdsa=%#x\n", 
			preg_shadow->deviceid, preg_shadow->chipid, preg_shadow->rdsa);
		return -EINVAL;
	}

	/* Find the device's generation based on the chipid */
	switch ((preg_shadow->chipid>>6)&0x000F) {
		case 9U:
			gdev->device_id	= 4703U;
			break;
		case 1U:
			gdev->device_id	= 4702U;
			break;
		default:
			printk("Invalid device id!\n");
			/* XXX f.o. something better than just this! */
			EXIT(-1);
			break;
	}

	/* Create a device file descriptor */
	cdev_init(&gdev->cdev, &si4703_fops);
	gdev->cdev.owner = THIS_MODULE;
	gdev->cdev.ops = &si4703_fops;

	PK_X("si4703_init cdev_add\n");
	result = cdev_add(&gdev->cdev, MKDEV(FMRECEIVER_MAJOR, FMRECEIVER_MINOR), 1);
	if (result) {
		PK_ERR("Unable to add character device\n");
		EXIT(result);
	}

	client->dev.driver_data	= gdev;
	si4703_sysfs_register(&client->dev);

	/* Requesting an IRQ for RDS data retrieval. */
	printk("*************************** registering irq %d\n", gdev->irq);
	result		= request_irq(gdev->irq, si4703_interrupthandler, 
		IRQF_TRIGGER_FALLING, "si4703", (void *) gdev);
	if (result) {
		cdev_del(&gdev->cdev);
		printk("Could not register irq #%d (%d)\n", client->irq, result);
		EXIT(-EINVAL);
	}
#if 0
	/* In sysfs mode, opening the device file is not mandatory. As a result, do not disable the irq */
	disable_irq(gdev->irq);
#endif

	EXIT(0);
}

/* XXX more error checking needed */
static int si4703_remove(struct i2c_client *client)
{
	ENTER();

	disable_irq(gdev->irq);

	/* Set it back to 0 so that if people try to get a device */
	/* ID while the dock has been removed, it will fail.      */
	gdev->device_id	= 0;

	YELL("Freeing the IRQ");
	free_irq(gdev->irq, client->dev.driver_data);

	YELL("Removing the driver");
	cdev_del(&gdev->cdev);

	YELL("Removing sysfs entries");
	si4703_sysfs_unregister(&client->dev);

	YELL("Removing the device!");
	cdev_del(&gdev->cdev);

	EXIT(0);
}

DECLARE_WORK(rds_data_grabber, si4703_workqueue_handler);

static void si4703_workqueue_handler(struct work_struct *work)
{
	u16 lreg[NR_OF_REGISTERS];
	si4703_registers_t *plreg = (si4703_registers_t*) lreg;

#if 0
	/* Unnecessary with sysfs */
	if (atomic_read(&si4703_available) != 0) {
		PK_X("si4703_workqueue_handler: si4703_available.counter(%d)\n", 
			si4703_available.counter);
		return;
	}
#endif

	if (si4703_suspended) {
		PK_X("si4703_workqueue_handler: si4703_suspended(%d)\n", si4703_suspended);
		return;
	}

	// This interrupt can fire for RDS or STC. Determine which one it is by
	// checking the shadow status of TUNE and SEEK
	if (!((preg_shadow->powercfg & SI47_SEEK) || 
		(preg_shadow->channel & SI47_TUNE))) {
		struct fmr_rds_type rdsoutdata;
		u8 rdssync;

		if (si470x_reg_read(6, lreg) != 0) {
			PK_ERR("ERROR @ %d, si470x_reg_read\n", __LINE__);
			return;
		}
#ifdef LOGGING
#ifdef DUMP_RDS_DATA
		PK_X("si4703_workqueue_handler : ");
		si470x_reg_dump(lreg, 0x0A, 0x0F);
#endif
#endif

		rdsoutdata.sf_bandlimit = (unsigned char) ((plreg->statusrssi & SI47_SFBL) >> 13);
		rdsoutdata.rssi = (unsigned char) si470x_get_current_rssi(gdev);
		rdsoutdata.freq = (unsigned short int) si470x_get_current_frequency(gdev);

		rdssync = (plreg->statusrssi & SI47_RDSS) ? 1 : 0;

		if (rdssync) {      
			u8 bler[4];		/* XXX guba - move this upstairs */
			u8 errors = 0;

			/* Gather the latest block error (BLER) info */
			bler[0]	= (plreg->statusrssi & SI47_BLERA) >> 9;
			bler[1] = (plreg->readchan   & SI47_BLERB) >> 14;
			bler[2] = (plreg->readchan   & SI47_BLERC) >> 12;
			bler[3] = (plreg->readchan   & SI47_BLERD) >> 10;

			errors = bler[0] + bler[1] + bler[2] + bler[3];

			if (errors > 0) {
				gdev->rds_bad++;
				PK_X("-");
			} else {
				gdev->rds_good++;
				PK_X("+");

				if (si4703_fifo_len(gdev) < (54 * sizeof(struct fmr_rds_type))) { /* put groups in fifo if it still fits */
					rdsoutdata.blocka	= plreg->rdsa;
					rdsoutdata.blockb	= plreg->rdsb;
					rdsoutdata.blockc	= plreg->rdsc;
					rdsoutdata.blockd	= plreg->rdsd;

					si4703_fifo_put(gdev, (u8 *) &rdsoutdata, sizeof(struct fmr_rds_type));
					wake_up_interruptible(&gdev->wq);
				}
			}

#ifdef RDSQ_LOGGING
			if (((gdev->rds_bad+gdev->rds_good)%10) == 0) {
				unsigned long now_ms = jiffies_to_msecs(jiffies);	/* XXX guba - move upstairs */
				unsigned long time_interval_ms = now_ms - gdev->rds_timestamp_ms;
				unsigned int rds_count;
				unsigned int rdsq;
				int rds_bad_ratio = 0;

				rds_count		= gdev->rds_good - prev_rds_good;
				prev_rds_good		= gdev->rds_good;

				/* reset rds timestamp */
				gdev->rds_timestamp_ms	= now_ms;

				rdsq			= 100 * (rds_count*100000) / (time_interval_ms * 1131);
				rds_bad_ratio		= (1000*gdev->rds_bad)/(gdev->rds_good+gdev->rds_bad);

				printk("si4703: f(%d) rssi(%d) #bad(%d) #good(%d) bad(%d.%d) t(%d) rdsq(%d)\n",
					rdsoutdata.freq, rdsoutdata.rssi, gdev->rds_bad,
					gdev->rds_good, rds_bad_ratio/10, rds_bad_ratio%10,
					(int)time_interval_ms, rdsq);
			}
#endif
		} else {
			/* no rds sync */
			PK_X("?");
		}
	} else {
		/* TUNE or SEEK finished */
		PK_X("STC\n");

		WaitSTCInterrupt = 0;
		wake_up_interruptible(&gdev->wq);
	}
}

static irqreturn_t si4703_interrupthandler(int irq, void *dev_id)
{
	schedule_work(&rds_data_grabber);
	return IRQ_HANDLED;
}

static int si4703_fifo_alloc(struct si4703dev *dev, int size)
{
	int ret = 0;

	dev->fifo = kfifo_alloc(size, GFP_KERNEL, &dev->sp_lock);

	if (dev->fifo == NULL) {
	ret = -1;
	}    

	PK_X("si4703_fifo_alloc return %d\n", ret);
	return ret;
}

static void si4703_fifo_free(struct si4703dev *dev)
{
	kfifo_free(dev->fifo);
}

void si4703_fifo_purge(struct si4703dev *dev)
{
	kfifo_reset(dev->fifo);
}

static int si4703_fifo_len(struct si4703dev *dev)
{
	return kfifo_len(dev->fifo);
}

int si4703_fifo_put(struct si4703dev *dev, unsigned char *data, int len)
{
	return kfifo_put(dev->fifo, data, len);
}

int si4703_fifo_get(struct si4703dev *dev, unsigned char *data, int len)
{
	return kfifo_get(dev->fifo, data, len);
}

static int si4703_open(struct inode *nodep, struct file *filep)
{
	ENTER();

	/* Only one client may open the device */
	// XXX guba - why? In my model we are just doing ioctls on this device. Of course, it means
	// that if I authorize this I have to make sure that count is < max(available) before freeing
	// the IRQ.
	if (!atomic_dec_and_test(&si4703_available)) {
		atomic_inc(&si4703_available);
		PK_X("si4703_open return -EBUSY\n");
		return -EBUSY;
	}

	filep->private_data = gdev; /* for easy reference */
	si4703_fifo_purge(gdev);
  
	EXIT(0);
}

static int si4703_release(struct inode *nodep, struct file *filep)
{
	ENTER();  
  
	if (filep && filep->private_data) {
		filep->private_data = NULL;
	}

	/* make device available for others to open */
	atomic_inc(&si4703_available);

	EXIT(0);
}

static int si4703_ioctl(struct inode *nodep, struct file *filep, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	__u16 value;

	ENTER();

	if (_IOC_TYPE(cmd) != FMRECEIVER_DRIVER_MAGIC) {
		PK_ERR("Wrong IOC type! Failing command\n");
		ret = -ENOTTY;
	}

	switch (cmd) {
		case TMCIOC_RDS_START:
			PK_X("si4703_ioctl TMCIOC_RDS_START\n");
			ret = si470x_enable_rds(gdev);
			break;
		case TMCIOC_RDS_STOP:
			PK_X("si4703_ioctl TMCIOC_RDS_STOP\n");
			ret = si470x_disable_rds(gdev);;
			break;
		case TMCIOC_G_FREQUENCY:
			PK_X("si4703_ioctl TMCIOC_G_FREQUENCY\n");
			value = si470x_get_current_frequency(gdev);
			if (put_user(value, (__u16 __user *) arg)) {
				ret = -EFAULT;
			}
			break;
		case TMCIOC_S_FREQUENCY:
			YELL("si4703_ioctl TMCIOC_S_FREQUENCY");
			if (get_user(value, (__u16 __user *) arg)) {
				ret = -EFAULT;
			} else {
				ret = si470x_set_frequency(gdev, value);
			}
			break;
		case TMCIOC_S_BAND:
			YELL("si4703_ioctl TMCIOC_S_BAND");
			if (get_user(value, (__u16 __user *) arg)) {
				ret = -EFAULT;
			} else {
				ret = gdev->set_band(gdev, value);
			}
			break;
		case TMCIOC_S_VOLUME:
			YELL("si4703_ioctl TMCIOC_S_VOLUME");
			if (get_user(value, (__u16 __user *) arg)) {
				ret = -EFAULT;
			} else {
				ret = gdev->set_volume(gdev, value);
			}
			break;
		case TMCIOC_SEEK:
			{
				struct fmr_seek_param_type param;

				YELL("si4703_ioctl TMCIOC_SEEK");
				if (copy_from_user(&param, (void __user *) arg, 
					sizeof(struct fmr_seek_param_type))) {
					ret = -EFAULT;
				} else {
					ret = si470x_seek(&param);
				}
			}
			break;
		case TMCIOC_QUERY_DEVICE_ID:
			PK_X("si4703_ioctl TMCIOC_QUERY_DEVICE_ID\n");

			if (put_user(gdev->device_id, (__u16 __user *) arg)) {
			ret = -EFAULT;
			}
			break;
		case TMCIOC_Q_RSSI:
			{
				__u16 rssi = 0;

				PK_X("si4703_ioctl TMCIOC_Q_RSSI\n");
				//update shadow, to get up to date rssi
				if (si470x_reg_read(1, si470x_shadow) != 0) {
					PK_ERR("ERROR @ %d, si470x_reg_read\n", __LINE__);
					ret = -1;
				} else {
					rssi = si470x_get_current_rssi(gdev);
					if (put_user(rssi, (__u16 __user *) arg)) {
						ret = -EFAULT;
					}
				}
			}
			break;
		default:
			/* other commands not supported by this device */
			ret = -ENOTTY;
			break;
	}

	EXIT(ret);
}

/* we are implementing poll to support the select/poll/epoll system calls
 * This is done to allow the userspace app to check whether it is possible
 * to read non-blocking */
static unsigned int si4703_poll(struct file *filep, struct poll_table_struct *pq)
{
	unsigned int mask = 0;

	if (si4703_fifo_len(gdev) > 0)
		mask |= POLLIN | POLLRDNORM;  /* readable */
	else {
		poll_wait(filep, &gdev->wq, pq);
		if (si4703_fifo_len(gdev) > 0)
			mask |= POLLIN | POLLRDNORM;  /* readable */
	}

	return mask;
}

/* read blocks when there is no data, as it should */
static ssize_t si4703_read(struct file *filep, char __user * buf, size_t count, loff_t * pos)
{
	struct fmr_rds_type rdsdata;

	count = sizeof(struct fmr_rds_type);

	while (!(si4703_fifo_len(gdev) > 0)) {
		if (filep->f_flags & O_NONBLOCK)
			goto read_again_error;

		if (wait_event_interruptible(gdev->wq, (si4703_fifo_len(gdev) > 0)))
			goto read_restartsys_error;

	}
	/* ok RDS data is available */
	/* unless, in the mean time, a successful seek purged the fifo. */
	/* then we need to tell the caller to restart the read */
	if (sizeof(struct fmr_rds_type) != si4703_fifo_get(gdev, (u8 *) &rdsdata, 
		sizeof(struct fmr_rds_type))) {
		goto read_again_error;
	}

	if (copy_to_user(buf, &rdsdata, count)) {
		goto read_fault_error;
	}

	return count;

read_fault_error:
	PK_X("si4703_read return result = -EFAULT\n");
	return -EFAULT;
read_again_error:
	PK_X("si4703_read return result = -EAGAIN\n");
	return -EAGAIN;
read_restartsys_error:
	PK_X("si4703_read return result = -ERESTARTSYS\n");
	return -ERESTARTSYS;
}

static int __init si4703_init(void)
{
	int result = 0 ;

	ENTER();

	gdev = kmalloc(sizeof *gdev, GFP_KERNEL);
	if (!gdev) {
		PK_ERR("si4703_init kmalloc ERROR\n");
		EXIT(-ENOMEM);
	}
	memset(gdev, 0, sizeof *gdev);

	init_waitqueue_head(&gdev->wq); /* initialize waitqueue for readers */  
	spin_lock_init(&gdev->sp_lock);
	result = si4703_fifo_alloc(gdev, 55 * sizeof(struct fmr_rds_type));
	if (result) {
		EXIT(result);
	}

	// Register the i2c driver, si4703_probe() should be called as a result
	// with the device that was declared in the dock driver as an argument.
	result = i2c_add_driver(&driver);
	if (result) {
		printk("Driver registration failed!\n");
		EXIT(result);
	}

	EXIT(result);
}

static void __exit si4703_exit(void)
{
	ENTER();

	YELL("i2c_del_driver");
	i2c_del_driver(&driver);

	if (gdev->fifo != NULL) {
    	YELL("si4703_exit si4703_fifo_purge(gdev)");
		si4703_fifo_purge(gdev);
    	YELL("si4703_exit si4703_fifo_free(gdev)");
		si4703_fifo_free(gdev);
		gdev->fifo = NULL;
	}
  
	if ( gdev ) {
		YELL("si4703_exit kfree(gdev)");
		kfree(gdev);
		gdev = NULL;
	}
}

int
si470x_seek(struct fmr_seek_param_type	*seek_param)
{
	struct fmr_rds_type rdsoutdata;

	PK_X("si470x_seek enter freq(%d) threshold(%d) up(%d) wrap(%d)\n",
	seek_param->freq, seek_param->threshold, seek_param->up, seek_param->wrap);

	si4703_fifo_purge(gdev);

	if (si470x_tune(gdev, seek_param->freq) != 0) {
		PK_ERR("ERROR tune failed\n");
		return -1;
	} else {
		//seek config:
		preg_shadow->sysconfig2 &= ~SI47_SEEKTH; // Clear the field first
		preg_shadow->sysconfig2 |= ((u16) (seek_param->threshold << 8) & 0xFF00); // Set the SEEKTH
		preg_shadow->sysconfig3 &= ~SI47_SKSNR;  // Clear the field first
		preg_shadow->sysconfig3 &= ~SI47_SKCNT;  // Clear the field first
		preg_shadow->sysconfig3 |= 0xA << 4;     // Set the SKSNR to 0xA
		preg_shadow->sysconfig3 |= 4;            // Set the SKCNT to 4

		if (si470x_seek_next(seek_param->up, (seek_param->wrap+1)%2) < 0) {
			PK_ERR("ERROR seek_next failed\n");
			return -1;
		} else {
			//preg_shadow (STATUSRSSI and READCHAN) up to date
			rdsoutdata.sf_bandlimit = (unsigned char) ((preg_shadow->statusrssi & SI47_SFBL) >> 13);
			rdsoutdata.rssi = (unsigned char) si470x_get_current_rssi(gdev);
			rdsoutdata.freq = (unsigned short int) si470x_get_current_frequency(gdev);
			rdsoutdata.blocka = (unsigned short int) 0x00FF;
			rdsoutdata.blockb = (unsigned short int) 0x0000;
			rdsoutdata.blockc = (unsigned short int) 0x0000;
			rdsoutdata.blockd = (unsigned short int) 0x0000;

			PK_X("si470x_seek: fifo_put f(%d) rssi(%d) bandlimit(%d)\n", rdsoutdata.freq, rdsoutdata.rssi, rdsoutdata.sf_bandlimit);
			si4703_fifo_put(gdev, (u8 *) &rdsoutdata, sizeof(struct fmr_rds_type));
			wake_up_interruptible(&gdev->wq);
		}
	}

	EXIT(0);
}

//-----------------------------------------------------------------------------
// Inputs:
//      seekup:  0 = seek down
//               1 = seek up
//      seekmode: 0 = wrap at band limits
//                1 = stop at band limits
// Outputs:
//      zero = seek found a station
//      nonzero = seek did not find a station
//-----------------------------------------------------------------------------
static int si470x_seek_next(u8 seekup, u8 seekmode)
{
	int ret=0;
	long timeout=0;
	int retry=0;

	PK_X("si470x_seek_next enter up(%d) mode(%d)\n", seekup, seekmode);

	// Set or clear seekmode bit in address 0x02
	if (seekmode) {
		preg_shadow->powercfg |= SI47_SKMODE;
	} else {
		preg_shadow->powercfg &= ~SI47_SKMODE;
	}

	// Set or clear seekup bit in address 0x02
	if(seekup) {
		preg_shadow->powercfg |= SI47_SEEKUP;
	} else {
		preg_shadow->powercfg &= ~SI47_SEEKUP;
	}

	// Set seek bit in address 0x02
	preg_shadow->powercfg |= SI47_SEEK;

#ifdef LOGGING
	PK_X("si470x_seek_next: ");
	si470x_reg_dump(si470x_shadow, 0x00, 0x0F);
#endif

	WaitSTCInterrupt = 1;
	PK_X("si470x_seek_next: set SEEK bit\n");
	// write seek config upto sysconfig3
	if (si470x_reg_write(5) != 0) {
		PK_ERR("ERROR @ %d, si470x_reg_write\n", __LINE__);
		return -1;
	}

	// Wait for stc bit to be set
	timeout = wait_event_interruptible_timeout(gdev->wq, (WaitSTCInterrupt == 0), SEEK_TIMEOUT * HZ);

	if (timeout == 0) {
		PK_ERR("ERROR STC interrupt missed\n");
#ifdef LOGGING
		//timeout, read back all registers
		if (si470x_reg_read(NR_OF_REGISTERS, si470x_shadow) != 0) {
			PK_ERR("ERROR @ %d, si470x_reg_read\n", __LINE__);
			return -1;
		}
		PK_X("si470x_seek_next: ");
		si470x_reg_dump(si470x_shadow, 0x00, 0x0F);
#endif    
		ret = -1;
	} else {
		PK_X("si470x_seek_next: STC interrupt after %d s\n", (int)(SEEK_TIMEOUT - (timeout/HZ)));
	}

	// Clear seek bit in address 0x02
	preg_shadow->powercfg &= ~SI47_SEEK;
	if (si470x_reg_write(1) != 0) {
		PK_ERR("ERROR @ %d, si470x_reg_write\n", __LINE__);
		return -1;
	}

	// Wait for stc bit to be cleared.  This step is very important. If it
	// is ignored and another seek or tune occurs too quickly, the tuner
	// will not set the STC bit properly.
	retry=0;
	if (si470x_reg_read(2, si470x_shadow) != 0) {
		PK_ERR("ERROR @ %d, si470x_reg_read\n", __LINE__);
		return -1;
	}
	while (((preg_shadow->statusrssi & SI47_STC) != 0)  && (retry < 100)) {
		if (si470x_reg_read(2, si470x_shadow) != 0) {
			PK_ERR("ERROR @ %d, si470x_reg_read\n", __LINE__);
			return -1;
		}
		msleep(5);
		retry++;
	}

	/* Update cached value */
	gdev->last_found_freq	= si470x_get_current_frequency(gdev);

	// The tuner is now set to the newly found channel if one was available
	// as indicated by the seek-fail bit.
	PK_X("si470x_seek_next return retry(%d) f(%d) rssi(%d) afcrl(%d)\n", 
		retry, 
		si470x_get_current_frequency(gdev), 
		(u8)(preg_shadow->statusrssi & SI47_RSSI),
		(u8)(preg_shadow->statusrssi & SI47_AFCRL));

	EXIT(ret);
}

int si470x_reg_write(u8 number_of_reg)
{
	u16 lreg[NR_OF_WRITE_REGS];
	int retval=0, ret=0, i=0;

	//Note: write starts at register 0x02h (POWERCFG), register 0x00, 0x01, 0x0A .. 0x0F are readonly
	//Note: we never write to reg 0x08 (TEST2) and 0x09 (BOOTCONFIG)
	if ((gdev->client != NULL) && (number_of_reg <= NR_OF_WRITE_REGS)) {
		for (i = 0; i < number_of_reg; i++) {
			lreg[i] = cpu_to_be16(si470x_shadow[i+2]);
		}

		ret = i2c_master_send(gdev->client, (u8 *) lreg, (2 * number_of_reg));

		if (ret != (2 * number_of_reg)) {
			retval = -EBUSY;
		}
	}

	return retval;
}

int si470x_reg_read(u8 number_of_reg, u16 *plreg)
{
	u16 lreg[NR_OF_REGISTERS];
	int retval=0, ret=0, i=0;  

	// Note: read starts at register 0Ah (STATUSRSSI) and wraps back around till it reaches 09h
	if ((gdev->client != NULL) && (number_of_reg <= NR_OF_REGISTERS)) {    
		ret = i2c_master_recv(gdev->client, (u8 *) lreg, (2 * number_of_reg));   

		if (ret == (2 * number_of_reg)) {
			if ((0x0A + number_of_reg) < NR_OF_REGISTERS) {
				for (i = 0x0A; i < (0x0A + number_of_reg); i++) {
					plreg[i] = be16_to_cpu(lreg[i - 0x0A]);
				}
			} else {
				for (i = 0x0A; i <= 0x0F; i++) {
					plreg[i] = be16_to_cpu(lreg[i - 0x0A]);
				}
				for (i=0; i < (number_of_reg-6); i++) {
					plreg[i] = be16_to_cpu(lreg[i+6]);
				}
			}
		} else {
			retval = -EBUSY;
		}
	}

	return retval;
}


//-----------------------------------------------------------------------------
// Resets the part and initializes registers to the point of being ready for
// the first tune or seek.
//-----------------------------------------------------------------------------
/* XXX remove or use for suspend/resume */
static int si470x_initialize(void)
{
	int rc=0;

	PK_X("si470x_initialize enter\n");

	si470x_powerup();

	// Initialize shadow registers
	if (si470x_reg_read(NR_OF_READABLE_REGISTERS, si470x_shadow) != 0) {
		PK_ERR("ERROR @ %d, si470x_reg_read\n", __LINE__);
		return -1;
	}

#ifdef LOGGING
	si470x_reg_dump(si470x_shadow, 0x00, 0x0F);
#endif

	/* set RDS verbose mode */
	preg_shadow->powercfg |= SI47_RDSM;

	/* set RDS enable, RDSI enable, STCI enable, GPIO2 == RDS/STC Interrupt */
	preg_shadow->sysconfig1 |= /*SI47_RDS | SI47_RDSIEN |*/ SI47_STCIEN | SI47_GPIO2_INT;

	/* set RDS high performance mode */
	preg_shadow->sysconfig3 |= SI47_RDSPRF;

	//write (POWERCFG...SYSCONFIG3)
	if (si470x_reg_write(5) != 0) {
		PK_ERR("ERROR @ %d, si470x_reg_write\n", __LINE__);
		return -1;
	}

#ifdef LOGGING
	PK_X("si470x_initialize: ");
	si470x_reg_dump(si470x_shadow, 0x00, 0x0F);
#endif

	PK_X("si470x_initialize: deviceid(0x%04x) chipid(0x%04x) test1(0x%04x)\n",
			preg_shadow->deviceid, preg_shadow->chipid, preg_shadow->test1);

	if (preg_shadow->deviceid != 0x1242) {
		PK_X("si470x_initialize: invalid device id 0x%x\n", preg_shadow->deviceid);
		gdev->device_id = 0xFFFF;
		rc = -1;
	} else {
		u16 dev = 0U;

		dev = (preg_shadow->chipid >> 6) & 0x000F;

		switch (dev) {
		case 1U:
			gdev->device_id = 4702U;
			break;
		case 8U:
			PK_X("si470x_initialize ERROR\n");
			rc = -1;
		case 9U:
			gdev->device_id = 4703U;
			break;
		default:
			PK_X("si470x_initialize ERROR dev(%d)\n", dev);
			rc = -1;
			break;
		}
	}

	PK_X("si470x_initialize return\n");
	return rc;
}

//-----------------------------------------------------------------------------
// Take the Si470x out of powerdown mode.
//-----------------------------------------------------------------------------
static int si470x_powerup(void)
{
	ENTER();

#if USE_CRYSTAL
	// Check if the device is already powered up. Only initialize shadow
	// registers if it is.  This isn't necessary if not using the crystal
	// or if it can be guaranteed that this function isn't called while
	// the device is already poweredup. This check prevents register 7
	// from being written with the wrong value. If the device is already
	// powered up, the XOSCEN bit should be or'd with 0x3C04 instead of 0x0100
	if(preg_shadow->powercfg & SI47_ENABLE)
		preg_shadow->test1 = SI47_XOSCEN | 0x3C04;
	else
		preg_shadow->test1 = SI47_XOSCEN | 0x0100;

#ifdef LOGGING
	YELL("si470x_powerup write(6): ");
	si470x_reg_dump(si470x_shadow, 0x02, 0x07);
#endif

	if (si470x_reg_write(6) != 0)
		EXIT(-1);

	/* wait for crystal frequency to stabilize */
	msleep(CRYSTAL_TIME);
#endif

	/* Power the device up */
	preg_shadow->powercfg |= SI47_ENABLE;
	preg_shadow->powercfg &= ~SI47_DISABLE;

	if (si470x_reg_write(1) != 0)
		EXIT(-1);

	/* wait for si470x to powerup */
	msleep(POWERUP_TIME);

	EXIT(0);
}

#ifdef LOGGING
void si470x_reg_dump(u16 *lreg, int startreg, int endreg)
{
	int i;

	for (i=startreg; i<=endreg; i++) {
		printk("%04x ", lreg[i]);
	}
	printk("\n");
}
#endif

static int si470x_disable()
{
	int retry=0;

	if (preg_shadow->powercfg & SI47_ENABLE) {
		// First, disable RDS      
		PK_X("si470x_disable: disable RDS\n");
		preg_shadow->sysconfig1 &= ~SI47_RDSIEN;  /* unset RDSI enable */
		preg_shadow->sysconfig1 &= ~SI47_RDS;     /* unset RDS enable  */
		if (si470x_reg_write(3) != 0) {
			PK_ERR("ERROR @ %d, si470x_reg_write\n", __LINE__);
		}

		// Set DISABLE=1
		preg_shadow->powercfg |= SI47_DISABLE;
		PK_X("si470x_disable: Set DISABLE=1 powercfg(0x%x)\n", preg_shadow->powercfg);
		si470x_reg_write(1);
	}

	// Wait for indication that powerdown was successful: ENABLE=0
	retry=0;
	if (si470x_reg_read(9, si470x_shadow) != 0) { // Note: read starts from register 0xA
		PK_ERR("ERROR @ %d, si470x_reg_read\n", __LINE__);
	}
	while (((preg_shadow->powercfg & SI47_ENABLE) != 0)  && (retry < 10)) {
		if (si470x_reg_read(9, si470x_shadow) != 0) {
			PK_ERR("ERROR @ %d, si470x_reg_read\n", __LINE__);
		}      
		msleep(10);
		retry++;      
	}
	PK_X("si470x_disable: Low Power, Bus Accessible, powercfg(0x%x) retry(%d)\n", preg_shadow->powercfg, retry);

	return 0;
}

static int si4703_core_suspend(struct device *dev, pm_message_t state, u32 level)
{  
	PK_X("si4703_core_suspend enter\n");

	if (si4703_suspended == 0) { 
		si4703_suspended = 1;          

		if (atomic_read(&si4703_available) == 0) {
			PK_X("si4703_core_suspend si470x_disable()\n");
			si470x_disable();      
		}
	}

	PK_X("si4703_core_suspend return\n"); 
	return 0;
}

static int si4703_core_resume(struct device *dev, u32 level)
{
	int rc = 0;
	int retries = 0;

	PK_X("si4703_core_resume enter\n");

	if (si4703_suspended == 1) {
		if (atomic_read(&si4703_available) == 0) {
			do {
				rc = si470x_initialize();
				PK_X("si4703_core_resume: retry(%d) si470x_initialize return %d\n", retries, rc);
				retries++;
				msleep(100);
			} while ((rc < 0) && (retries < 3));
		}

		si4703_suspended = 0;
	}

	PK_X("si4703_core_resume return\n");
	return rc;
}

MODULE_AUTHOR("RDS-TMC dev team ehv, Guillaume Ballet (Guillaume.Ballet@tomtom.com)");
MODULE_DESCRIPTION("Driver for I2C connected Silicon Labs 4703 FM/RDS/TMC Receiver");
MODULE_LICENSE("GPL");  /* needed for access to some linux api functions */

module_init(si4703_init);
module_exit(si4703_exit);

