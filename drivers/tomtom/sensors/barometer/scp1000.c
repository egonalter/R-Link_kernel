/* drivers/barcelona/baro/scp1000_baro.c
 *
 * Implementation of the SCP1000 Barometer driver. 
 *
 * Copyright (C) 2007,2008 TomTom BV <http://www.tomtom.com/>
 * Authors: Rogier Stam <Rogier.Stam@tomtom.com>
 *          Jeroen Taverne <Jeroen.Taverne@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uio.h>
#include <linux/errno.h>
#include <linux/device.h>
#include <linux/poll.h>
#include <linux/cdev.h>
#include <linux/jiffies.h>
#include <linux/spi/spi.h>
#include <linux/delay.h>
#include <asm/uaccess.h>
#include <linux/wait.h>
#include <linux/interrupt.h>
#include <linux/gadc.h>
#include <linux/scp1000.h>
#include <linux/drpower.h>

#define SCP1000_MAX_SAMPLERATE		7900
#define SCP1000_MAX_HP_SAMPLERATE	1600

#define SCP1000_BARO_FLGS_EXIT		(1<<0)
#define SCP1000_BARO_FLGS_FI_CONGESTION	(1<<1)
#define SCP1000_BARO_FLGS_ST_CONGESTION	(1<<2)
#define SCP1000_BARO_FLGS_SPI_ERROR	(1<<3)
#define SCP1000_BARO_FLGS_TMP_OPEN	(1<<4)
#define SCP1000_BARO_FLGS_PRS_OPEN	(1<<5)

#define SCP1000_BARO_REVID		0x03
#define SCP1000_BARO_DRIVER_NAME	"scp1000"

enum scp1000_spi_commands {
	REVID = 0x00,
	DATAWR = 0x01,
	ADDPTR = 0x02,
	OPERATION = 0x03,
	OPSTATUS = 0x04,
	RSTR = 0x06,
	STATUS = 0x07,
	DATARD8 = 0x1F,
	DATARD16 = 0x20,
	TEMPOUT = 0x21
};

enum scp1000_spi_icommands {
	CFG = 0x00,
	TWIADD = 0x05,
	USERDATA1 = 0x29,
	USERDATA2 = 0x2A,
	USERDATA3 = 0x2B,
	USERDATA4 = 0x2C
};

static struct scp1000_baro_private *private;

struct scp1000_baro_private {
	struct gadc_buffer temp_cbuf;
	struct gadc_buffer pres_cbuf;
	struct timer_list timer;
	struct workqueue_struct *work_queue;
	struct work_struct work_start;
	struct work_struct work_fini;
	struct spi_device *spi_dev;
	int flags;
};

static int scp1000_baro_hwinit(struct spi_device *spi_dev);
#ifdef CONFIG_PM
static int scp1000_baro_suspend(struct spi_device *spi, pm_message_t mesg);
static int scp1000_baro_resume(struct spi_device *spi);
#else
#define scp1000_baro_suspend	NULL
#define scp1000_baro_resume	NULL
#endif
static int scp1000_baro_remove(struct spi_device *spi_dev);
static int scp1000_baro_open(struct inode *inode, struct file *file);
static int scp1000_baro_release(struct inode *inode, struct file *file);
static unsigned scp1000_baro_poll(struct file *file, poll_table * wait);
static int scp1000_baro_ioctl(struct inode *inode, struct file *file,
			      unsigned int cmd, unsigned long arg);
static ssize_t scp1000_baro_read(struct file *file, char __user * buf,
				 size_t count, loff_t * loff);
static int scp1000_spi_niread(struct spi_device *spi_dev,
			      enum scp1000_spi_commands regidx, void *data);
static int scp1000_spi_niwrite(struct spi_device *spi_dev,
			       enum scp1000_spi_commands regidx,
			       unsigned char data);
static int scp1000_spi_iwrite(struct spi_device *spi_dev,
			      enum scp1000_spi_icommands regidx,
			      unsigned char data);
static void scp1000_baro_wq_trans_start(struct work_struct *work);
static void scp1000_baro_wq_trans_fini(struct work_struct *work);
static void scp1000_baro_timer_handler(unsigned long arg);
static irqreturn_t scp1000_baro_drdy_irq_handler(int irq, void *pdata);
static int scp1000_baro_set_samplerate(struct scp1000_baro_private *private,
				       unsigned long int samplerate);
static struct file_operations scp1000_baro_fops;
static int scp1000_baro_probe(struct spi_device *spi_dev);
static struct spi_driver scp1000_baro_driver;

static int scp1000_baro_hwinit(struct spi_device *spi_dev)
{
	int retval;
	int count;
	unsigned char data = 0;

	drpower_enable(SCP1000_POWER);

	/* First reset the chip before init. */
	retval = scp1000_spi_niwrite(spi_dev, RSTR, 0x01);
	if (retval)
		return retval;

	/* Wait for chip to initialize. */
	msleep(100);

	/* Check the REVID of the chip. */
	retval = scp1000_spi_niread(spi_dev, REVID, &data);
	if (retval)
		return retval;
	if (data != SCP1000_BARO_REVID) {
		printk(SCP1000_BARO_DRIVER_NAME
		       ": Error! Chip revision does not match!\n");
		return -1;
	}

	/* Start init procedure. */
	for (count = 0; count < 7; count++) {
		retval = scp1000_spi_niread(spi_dev, STATUS, &data);
		if (retval)
			return retval;

		/* Check if STARTUP is completed. */
		if ((data & 0x01) == 0x00)
			break;
		else
			msleep(10);
	}

	/* Check if INIT is completed. */
	if (count == 7) {
		printk(SCP1000_BARO_DRIVER_NAME ": Error! Init failed!\n");
		return -2;
	}

	/* Check the status of the EEPROM. Should not have an error. */
	retval = scp1000_spi_niread(spi_dev, DATARD8, &data);
	if (retval)
		return retval;
	if (data != 0x01)
		printk(SCP1000_BARO_DRIVER_NAME
		       ": EEPROM Checksum error! Init might have failed!\n");

	return 0;
}

static int scp1000_baro_hwexit(struct spi_device *spi_dev)
{
	scp1000_spi_niwrite(spi_dev, OPERATION, 0x00);
	drpower_disable(SCP1000_POWER);
	return 0;
}

#ifdef CONFIG_PM
static int scp1000_baro_suspend(struct spi_device *spi, pm_message_t mesg)
{
	if ((private->
	     flags & (SCP1000_BARO_FLGS_TMP_OPEN | SCP1000_BARO_FLGS_PRS_OPEN))
	    != 0) {
		private->flags |= SCP1000_BARO_FLGS_EXIT;
		flush_workqueue(private->work_queue);
		del_timer(&private->timer);
	}
	scp1000_baro_hwexit(spi);
	return 0;
}

static int scp1000_baro_resume(struct spi_device *spi)
{
	scp1000_baro_hwinit(spi);
	if ((private->
	     flags & (SCP1000_BARO_FLGS_TMP_OPEN | SCP1000_BARO_FLGS_PRS_OPEN))
	    != 0) {
		gadc_clear_buffer(&private->temp_cbuf);
		gadc_clear_buffer(&private->pres_cbuf);
		private->flags &= ~SCP1000_BARO_FLGS_EXIT;

		scp1000_baro_timer_handler((unsigned long int)private);
	}
	return 0;
}
#else
#define scp1000_baro_suspend	NULL
#define scp1000_baro_resume	NULL
#endif

static int scp1000_baro_remove(struct spi_device *spi_dev)
{
	struct gadc_cdev *chardev =
	    (struct gadc_cdev *)spi_dev->dev.driver_data;

	unregister_gadc_cdev(&(chardev[0]));
	unregister_gadc_cdev(&(chardev[1]));
	free_irq(spi_dev->irq, &spi_dev->dev);
	kfree(chardev);
	kfree(private);

	return 0;
}

static int scp1000_baro_open(struct inode *inode, struct file *file)
{
	struct gadc_cdev *chardev =
	    (struct gadc_cdev *)get_gadc_cdev(inode->i_cdev);
	struct gadc_buffer *buffer;
	unsigned long int sample_rate;
	unsigned long int flags;

	/* Check which buffer it is (temperature or pressure). */
	if (private->spi_dev->dev.driver_data == chardev) {
		buffer = &private->pres_cbuf;
		flags = SCP1000_BARO_FLGS_PRS_OPEN;
	} else {
		buffer = &private->temp_cbuf;
		flags = SCP1000_BARO_FLGS_TMP_OPEN;
	}

	if (private->flags & flags)
		return -EBUSY;

	/* Initialize data. */
	gadc_clear_buffer(buffer);

	/* Only init if the device wasn't opened yet. */
	if ((private->
	     flags & (SCP1000_BARO_FLGS_TMP_OPEN | SCP1000_BARO_FLGS_PRS_OPEN))
	    == 0) {
		init_timer(&private->timer);
		sample_rate = gadc_get_samplerate(buffer);
		private->timer.function = scp1000_baro_timer_handler;
		private->timer.data = ((unsigned long int)private);
		private->timer.expires =
		    jiffies + (HZ * 1000 + (sample_rate / 2)) / sample_rate;

		/* Start timer. */
		add_timer(&private->timer);
	}

	/* Flag as opened. */
	private->flags |= flags;

	file->private_data = chardev;
	return 0;
}

static int scp1000_baro_release(struct inode *inode, struct file *file)
{
	struct gadc_cdev *chardev =
	    (struct gadc_cdev *)get_gadc_cdev(inode->i_cdev);
	struct gadc_buffer *buffer;

	/* Check which buffer it is (temperature or pressure). */
	if (private->spi_dev->dev.driver_data == chardev) {
		buffer = &private->pres_cbuf;
		/* Set flags to be the flag that checks if the other cdev is open or not. */
		private->flags &= ~SCP1000_BARO_FLGS_PRS_OPEN;
	} else {
		buffer = &private->temp_cbuf;
		/* Set flags to be the flag that checks if the other cdev is open or not. */
		private->flags &= ~SCP1000_BARO_FLGS_TMP_OPEN;
	}

	if ((private->
	     flags & (SCP1000_BARO_FLGS_PRS_OPEN | SCP1000_BARO_FLGS_TMP_OPEN))
	    == 0) {
		private->flags |= SCP1000_BARO_FLGS_EXIT;
		flush_workqueue(private->work_queue);
		del_timer(&private->timer);
		private->flags = 0;
	}
	gadc_clear_buffer(buffer);
	file->private_data = NULL;
	return 0;
}

static unsigned scp1000_baro_poll(struct file *file, poll_table * wait)
{
	struct gadc_cdev *chardev = (struct gadc_cdev *)file->private_data;
	struct gadc_buffer *buffer;
	int tmpval;
	unsigned int mask = 0;

	/* Check which buffer it is (temperature or pressure). */
	if (private->spi_dev->dev.driver_data == chardev)
		buffer = &private->pres_cbuf;
	else
		buffer = &private->temp_cbuf;

	gadc_poll_wait(file, buffer, wait);
	if (gadc_get_samplecount(buffer, &tmpval) > 0)
		mask |= POLLIN | POLLRDNORM;

	return mask;
}

static int scp1000_baro_ioctl(struct inode *inode, struct file *file,
			      unsigned int cmd, unsigned long arg)
{
	struct gadc_cdev *chardev = (struct gadc_cdev *)file->private_data;
	struct gadc_buffer *buffer;
	struct gadc_format format;
	int ret = 0;

	/* Check which buffer it is (temperature or pressure). */
	if (private->spi_dev->dev.driver_data == chardev)
		buffer = &private->pres_cbuf;
	else
		buffer = &private->temp_cbuf;

	switch (cmd) {
	case ADC_SET_SAMPLERATE:
		if (scp1000_baro_set_samplerate
		    (private, (unsigned long int)arg))
			return -EINVAL;
		break;

	case ADC_GET_SAMPLERATE:
		*((unsigned long int *)arg) = gadc_get_samplerate(buffer);
		break;

	case ADC_GET_BUFFERSTATUS:
		*((unsigned long int *)arg) = gadc_get_status(buffer);
		break;

	case ADC_GET_FORMAT:
		if (gadc_get_samplerate(buffer) <= SCP1000_MAX_HP_SAMPLERATE)
			format.accuracy = 17;
		else
			format.accuracy = 15;
		format.bitformat = GADC_BF_ULLA;
		ret = copy_to_user((void *)arg, &format, sizeof(format));
		break;

	default:
		ret = -ENOTTY;
		break;
	}

	return ret;
}

static ssize_t scp1000_baro_read(struct file *file, char __user * buf,
				 size_t count, loff_t * loff)
{
	struct gadc_cdev *chardev = (struct gadc_cdev *)file->private_data;
	struct gadc_sample sample;
	struct gadc_buffer *buffer;
	int samplecount;
	int maxbufsamples;
	int loop = 0;

	/* Check which buffer it is (temperature or pressure). */
	if (private->spi_dev->dev.driver_data == chardev)
		buffer = &private->pres_cbuf;
	else
		buffer = &private->temp_cbuf;

	/* Determine the amount of samples that'll fit in the buffer. */
	gadc_get_samplecount(buffer, &samplecount);

	/* If no samples are present block & wait. */
	if (samplecount == 0) {
		if (!(file->f_flags & O_NONBLOCK)) {
			if (gadc_wait_buffer_fill(buffer, &samplecount))
				return -ERESTARTSYS;
		} else
			return -EAGAIN;
	}

	/* Determine how many samples to read. */
	maxbufsamples = count / sizeof(sample);
	if (samplecount > maxbufsamples)
		samplecount = maxbufsamples;

	/* Copy sample by sample. Since we need to copy to user space, do this in two steps. */
	for (loop = 0; loop < samplecount; loop++) {
		if (!gadc_get_sample(buffer, &sample))
			break;
		if (copy_to_user
		    (&(((struct gadc_sample *)buf)[loop]), &sample,
		     sizeof(sample))) {
			printk
			    ("SCP1000: Help! Can't copy data to userspace!\n");
			return -EFAULT;
		}
	}

	/* Return the read bytes. */
	return loop * sizeof(struct gadc_sample);
}

static int scp1000_spi_niread(struct spi_device *spi_dev,
			      enum scp1000_spi_commands regidx, void *data)
{
	struct spi_message msg;
	struct spi_transfer transfer[2];
	unsigned char cmd = (unsigned char)(regidx << 2);
	int retval = 0;

	/* Prepare the data. */
	memset(&msg, 0, sizeof(msg));
	memset(transfer, 0, sizeof(transfer));
	spi_message_init(&msg);

	/* Prepare the address cycle. */
	transfer[0].tx_buf = &cmd;
	transfer[0].len = sizeof(cmd);
	transfer[0].delay_usecs = 0;
	spi_message_add_tail(&(transfer[0]), &msg);

	/* Prepare the data cycle. */
	if ((regidx == DATARD16) || (regidx == TEMPOUT))
		transfer[1].len = 2;
	else
		transfer[1].len = 1;

	transfer[1].delay_usecs = 0;
	transfer[1].rx_buf = (u8 *) data;
	spi_message_add_tail(&(transfer[1]), &msg);

	/* Finalize and transmit. */
	msg.spi = spi_dev;
	msg.is_dma_mapped = 0;
	retval = spi_sync(spi_dev, &msg);
	if (retval) {
		return retval;
	}
	if ((regidx == DATARD16) || (regidx == TEMPOUT)) {
		/* High byte is read first, low byte second. */
		*((u16 *) data) =
		    (((u8 *) data)[0] << 8) | (((u8 *) data)[1] << 0);
	}
	return retval;
}

static int scp1000_spi_niwrite(struct spi_device *spi_dev,
			       enum scp1000_spi_commands regidx,
			       unsigned char data)
{
	unsigned char tx_buf[2] =
	    { (unsigned char)((regidx << 2) | 0x02), data };
	struct spi_message msg;
	struct spi_transfer transfer;
	int retval;

	/* Prepare the data. */
	memset(&msg, 0, sizeof(msg));
	memset(&transfer, 0, sizeof(transfer));
	spi_message_init(&msg);

	/* Prepare the address cycle. */
	transfer.tx_buf = tx_buf;
	transfer.len = sizeof(tx_buf);
	transfer.delay_usecs = 0;
	spi_message_add_tail(&transfer, &msg);

	/* Finalize and transmit. */
	msg.spi = spi_dev;
	msg.is_dma_mapped = 0;
	retval = spi_sync(spi_dev, &msg);
	return retval;
}

static int scp1000_spi_iwrite(struct spi_device *spi_dev,
			      enum scp1000_spi_icommands regidx,
			      unsigned char data)
{
	int retval;

	retval = scp1000_spi_niwrite(spi_dev, ADDPTR, (unsigned char)regidx);
	if (retval)
		return retval;
	retval = scp1000_spi_niwrite(spi_dev, DATAWR, data);
	if (retval)
		return retval;
	retval =
	    scp1000_spi_niwrite(spi_dev, OPERATION,
				(regidx >= USERDATA1 ? 0x06 : 0x02));
	msleep(60);
	return retval;
}

static void scp1000_baro_wq_trans_start(struct work_struct *work)
{
	unsigned char data;

	if (scp1000_spi_niread(private->spi_dev, OPSTATUS, &data)) {
		private->flags |= SCP1000_BARO_FLGS_SPI_ERROR;
		return;
	}

	/* Check if a transaction is still in progress. If so abort. */
	if (data & 0x01)
		return;

	/* Start new acquisition. */
	if (scp1000_spi_niwrite(private->spi_dev, OPERATION, 0x0C)) {
		private->flags |= SCP1000_BARO_FLGS_SPI_ERROR;
		return;
	}

	/* Done. */
	return;
}

static void scp1000_baro_wq_trans_fini(struct work_struct *work)
{
	unsigned char data;
	signed short int temperature;
	unsigned long int pressure;

	if (scp1000_spi_niread(private->spi_dev, OPSTATUS, &data)) {
		private->flags |= SCP1000_BARO_FLGS_SPI_ERROR;
		return;
	}

	/* Read back the results from the acquisition. */
	if (scp1000_spi_niread(private->spi_dev, TEMPOUT, &temperature)) {
		private->flags |= SCP1000_BARO_FLGS_SPI_ERROR;
		return;
	}

	/* First read the high 3 bits. */
	if (scp1000_spi_niread
	    (private->spi_dev, DATARD8, &(((unsigned char *)&pressure)[2]))) {
		private->flags |= SCP1000_BARO_FLGS_SPI_ERROR;
		return;
	}

	/* Now read the low 16 bits. */
	if (scp1000_spi_niread
	    (private->spi_dev, DATARD16, &(((unsigned char *)&pressure)[0]))) {
		private->flags |= SCP1000_BARO_FLGS_SPI_ERROR;
		return;
	}

	/* Mask out reserved bits. */
	pressure &= 0x0007FFFF;

	/* We have all data. Convert it to correct representation, and queue it. */
	/* We want to have the samplevalue multiplied by 1000, in order not to   */
	/* lose precision. To get from the read value to the actual pressure, it */
	/* needs to be divided by 4. To get from the read value to the actual    */
	/* temperature it needs to be divided by 20. */
	if (private->flags & SCP1000_BARO_FLGS_TMP_OPEN) {
		gadc_add_sample(&(private->temp_cbuf),
				(unsigned long int)(50 * ((signed long int)
							  temperature)));
		wake_up_interruptible(&private->temp_cbuf.poll_queue);
	}
	if (private->flags & SCP1000_BARO_FLGS_PRS_OPEN) {
		gadc_add_sample(&(private->pres_cbuf), 250 * pressure);
		wake_up_interruptible(&private->pres_cbuf.poll_queue);
	}

	return;
}

static void scp1000_baro_timer_handler(unsigned long arg)
{
	unsigned short sample_rate;

	if (!(private->flags & SCP1000_BARO_FLGS_EXIT)) {
		/* If we must exit then return and do not reschedule */
		sample_rate = gadc_get_samplerate(&(private->pres_cbuf));
		if (queue_work(private->work_queue, &private->work_start)) {
			/* There obviously is congestion if we try to schedule work
			 * while the previous scheduled one is not finished yet */
			private->flags |= SCP1000_BARO_FLGS_ST_CONGESTION;
		}

		/* Now reschedule the timer, this to keep things as much in sync as possible. */
		if (!(private->flags & SCP1000_BARO_FLGS_EXIT)) {
			sample_rate =
			    gadc_get_samplerate(&(private->pres_cbuf));
			mod_timer(&private->timer,
				  jiffies + (HZ * 1000 +
					     (sample_rate / 2)) /
				  (sample_rate));
		}
	}
	return;
}

static irqreturn_t scp1000_baro_drdy_irq_handler(int irq, void *pdata)
{
	struct scp1000_baro_private *private =
	    (struct scp1000_baro_private *)pdata;

	/* It seems that on driver initialization the IRQ is fired once without being needed. */
	/* This will ignore the IRQ unless the device is opened. */
	if ((private->
	     flags & (SCP1000_BARO_FLGS_PRS_OPEN | SCP1000_BARO_FLGS_TMP_OPEN))
	    != 0) {
		if (queue_work(private->work_queue, &private->work_fini)) {
			/* There obviously is congestion if we try to schedule work
			 * while the previous scheduled one is not finished yet */
			private->flags |= SCP1000_BARO_FLGS_FI_CONGESTION;
		}
	}

	return IRQ_HANDLED;
}

/* Note that in this routine we only use the low power mode. Samplerate is in mHz (mili, not Mega) */
static int scp1000_baro_set_samplerate(struct scp1000_baro_private *private,
				       unsigned long int samplerate)
{
	int retval;
	/* Don't exceed maximum frequency. */
	if (samplerate == 0)
		return -1;
	if (samplerate > SCP1000_MAX_SAMPLERATE)
		samplerate = SCP1000_MAX_SAMPLERATE;
	if (samplerate > 1000 * HZ)
		samplerate = 1000 * HZ;
	samplerate = 1000 * HZ / ((1000 * HZ + (samplerate / 2)) / samplerate);

	/* If the samplerate is higher than 1.8 Hz, we go to a lower precision to compensate. */
	if (samplerate > SCP1000_MAX_HP_SAMPLERATE)
		retval = scp1000_spi_iwrite(private->spi_dev, CFG, 0x0D);
	else
		retval = scp1000_spi_iwrite(private->spi_dev, CFG, 0x05);
	if (retval)
		return retval;

	gadc_set_samplerate(&private->temp_cbuf, samplerate);
	gadc_set_samplerate(&private->pres_cbuf, samplerate);

	/* Now reschedule the timer. Otherwise we will for one tick still run at the old rate. */
	del_timer(&private->timer);
	flush_workqueue(private->work_queue);
	scp1000_baro_timer_handler((unsigned long)private);
	return 0;
}

static struct file_operations scp1000_baro_fops = {
	.owner = THIS_MODULE,
	.read = scp1000_baro_read,
	.poll = scp1000_baro_poll,

	.ioctl = scp1000_baro_ioctl,
	.open = scp1000_baro_open,
	.release = scp1000_baro_release,
};

static int scp1000_baro_probe(struct spi_device *spi_dev)
{
	struct scp1000_priv *pdata =
	    (struct scp1000_priv *)spi_dev->dev.platform_data;
	struct gadc_cdev *chardev;

	printk(KERN_INFO
	       "TomTom GO SCP1000 Barometer Driver, (C) 2007, 2008 TomTom BV\n");

	if (pdata == NULL) {
		printk(SCP1000_BARO_DRIVER_NAME ": Missing platform data!\n");
		return -ENODEV;
	}

	/* Allocate memory for the structures. */
	private = (struct scp1000_baro_private *)
	    kmalloc(sizeof(struct scp1000_baro_private), GFP_KERNEL);
	if (private == NULL) {
		printk(SCP1000_BARO_DRIVER_NAME
		       ": Error allocating memory for private data!\n");
		return -ENOMEM;
	}

	chardev =
	    (struct gadc_cdev *)kmalloc(2 * sizeof(struct gadc_cdev),
					GFP_KERNEL);
	if (chardev == NULL) {
		printk(SCP1000_BARO_DRIVER_NAME
		       ": Error allocating memory for cdevs!\n");
		kfree(private);
		return -ENOMEM;
	}

	/* Initialize the private structure. */
	memset(private, 0, sizeof(*private));
	gadc_init_buffer(&private->temp_cbuf, 1000);
	gadc_init_buffer(&private->pres_cbuf, 1000);
	init_timer(&private->timer);
	private->work_queue =
	    create_singlethread_workqueue(SCP1000_BARO_DRIVER_NAME);
	INIT_WORK(&(private->work_start), scp1000_baro_wq_trans_start);
	INIT_WORK(&(private->work_fini), scp1000_baro_wq_trans_fini);
	private->spi_dev = spi_dev;

	/* Initialize the CDEV structures. */
	memset(chardev, 0, 2 * sizeof(*chardev));
	if (register_gadc_cdev
	    (&(chardev[0]), pdata->pressure.major, pdata->pressure.minor, 1,
	     SCP1000_BARO_DRIVER_NAME, &scp1000_baro_fops)) {
		printk(SCP1000_BARO_DRIVER_NAME
		       ": Could not register baro pressure cdev!\n");
		kfree(chardev);
		kfree(private);
		return -ENOMEM;
	}

	if (register_gadc_cdev
	    (&(chardev[1]), pdata->temperature.major, pdata->temperature.minor,
	     1, SCP1000_BARO_DRIVER_NAME, &scp1000_baro_fops)) {
		printk(SCP1000_BARO_DRIVER_NAME
		       ": Could not register baro temperature cdev!\n");
		unregister_gadc_cdev(&(chardev[0]));
		kfree(chardev);
		kfree(private);
		return -ENOMEM;
	}
	chardev[0].private = private;
	chardev[1].private = private;

	/* Set private device data. */
	spi_dev->dev.driver_data = chardev;

	/* Init the hardware */
	scp1000_baro_hwinit(private->spi_dev);

	/* Lastly, register the IRQ. We're in business. */
	if (request_irq
	    (spi_dev->irq, scp1000_baro_drdy_irq_handler,
	     IRQF_DISABLED | IRQF_SAMPLE_RANDOM | IRQF_TRIGGER_RISING,
	     SCP1000_BARO_DRIVER_NAME, private)) {
		printk(SCP1000_BARO_DRIVER_NAME
		       ": Could not register the DRDY interrupt!\n");
		unregister_gadc_cdev(&(chardev[0]));
		unregister_gadc_cdev(&(chardev[1]));
		kfree(chardev);
		kfree(private);
		return -ENOMEM;
	}

	/* Done. */
	return 0;
}

static struct spi_driver scp1000_baro_driver = {
	.driver = {
		   .name = SCP1000_BARO_DRIVER_NAME,
		   .bus = &spi_bus_type,
		   .owner = THIS_MODULE,
		   },
	.probe = scp1000_baro_probe,
	.remove = scp1000_baro_remove,
	.suspend = scp1000_baro_suspend,
	.resume = scp1000_baro_resume,
};

static int __init scp1000_baro_init(void)
{
	int retval;

	retval = spi_register_driver(&scp1000_baro_driver);
	if (retval)
		return retval;
	return 0;
}

module_init(scp1000_baro_init);

static void __exit scp1000_baro_exit(void)
{
	spi_unregister_driver(&scp1000_baro_driver);
}

module_exit(scp1000_baro_exit);

MODULE_DESCRIPTION("SCP1000 Barometer Driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR
    ("Rogier Stam <rogier.stam@tomtom.com> Jeroen Taverne <jeroen.taverne@tomtom.com>");
