/*
 * Buffered McSPI slave driver
 *
 * Copyright (C) 2010 Texas Instruments
 * Author: Fabrice Goucem <f-goucem@ti.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* TODO:
 * - Add the possibility of more than one device ?
 */

#include <linux/spi/spi.h>
#include <linux/fs.h>
#include <linux/poll.h>
#include <linux/io.h>
#include <linux/miscdevice.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/kfifo.h>
#include <plat/mcspi.h>
#include <plat/dma.h>

#undef DEBUG
#define DEVICE_NAME "ttyspibuf"
#define SPIBUF_SET_RXSIZE _IOWR('s', 2, int)
#define SW_FIFO_SIZE (OMAP2_MCSPI_DMA_BUF_SIZE)

/* Function prototypes */
static ssize_t spi_slave_buffered_write(struct file * fp,
					const char __user *buf,
					size_t len, loff_t *off);
static ssize_t spi_slave_buffered_read(struct file * fp,
					char __user *buf,
					size_t len, loff_t * off);
static int spi_slave_buffered_open(struct inode *ip, struct file *fp);

static struct spi_device *omap_slave_buffered_spi_device;
static struct miscdevice omap_slave_buffered_misc_device;

static unsigned char rx_buf[SW_FIFO_SIZE];
static unsigned char tx_buf[SW_FIFO_SIZE];
static unsigned char dump_buf[SW_FIFO_SIZE];
static unsigned int rx_thres = 16;
struct kfifo *rx_sw_fifo;
struct kfifo *tx_sw_fifo;

static spinlock_t fifo_lock;
static spinlock_t tx_dma_lock;
static wait_queue_head_t wait_q;

static int spi_slave_buffered_ioctl(struct inode *inode, struct file *file,
	    unsigned int cmd, unsigned long arg)
{
	unsigned long irqflags;
	switch(cmd)
	{
		case SPIBUF_SET_RXSIZE:
			spin_lock_irqsave(&fifo_lock, irqflags);
			rx_thres = arg;
			spin_unlock_irqrestore(&fifo_lock, irqflags);
			return 0;
		break;

		default:
		break;
	}

	return -ENOTTY;
}

static unsigned int kfifo_put_overwrite (struct kfifo *fifo, 
					 const unsigned char *buffer, unsigned int len)
{
	/* always keep one byte for the dummy zero */
	int to_remove = len - (kfifo_avail(fifo) - 1);
	if (to_remove > 0)
		kfifo_get(fifo, dump_buf, to_remove);
	return kfifo_put (fifo, buffer, len);
}

static void omap2_mcspi_dma_rx_timeout(unsigned long data)
{
	struct omap2_mcspi *mcspi = (struct omap2_mcspi *)data;	
	struct omap2_mcspi_dma *mcspi_dma = &mcspi->dma_channels[omap_slave_buffered_spi_device->chip_select];
	unsigned int curr_dma_pos, curr_transmitted_size;

	curr_dma_pos = omap_get_dma_dst_pos(mcspi_dma->dma_rx_channel);
	if ((curr_dma_pos == mcspi->slave_prev_rx_dma_pos) ||
			     (curr_dma_pos == 0)) {
		mod_timer(&mcspi->rx_dma_timer, jiffies + msecs_to_jiffies(OMAP2_MCSPI_DMA_RX_TIMEOUT_MS));
		return;
	}
			
	if (curr_dma_pos < mcspi->slave_prev_rx_dma_pos) {
		/* We wrapped around the buffer, push the data to tty */
		curr_transmitted_size = OMAP2_MCSPI_DMA_BUF_SIZE - 
			(mcspi->slave_prev_rx_dma_pos - mcspi->slave_rx_buf_dma_phys);
		kfifo_put_overwrite (rx_sw_fifo, mcspi->slave_rx_buf +
			(mcspi->slave_prev_rx_dma_pos -
			mcspi->slave_rx_buf_dma_phys), curr_transmitted_size);
		mcspi->slave_prev_rx_dma_pos = mcspi->slave_rx_buf_dma_phys;
	}

	curr_transmitted_size = curr_dma_pos - mcspi->slave_prev_rx_dma_pos;
	if (curr_transmitted_size > 0) {
		kfifo_put_overwrite (rx_sw_fifo, mcspi->slave_rx_buf +
				(mcspi->slave_prev_rx_dma_pos -
				mcspi->slave_rx_buf_dma_phys),
				curr_transmitted_size);
	}
	mcspi->slave_prev_rx_dma_pos = curr_dma_pos;
	mod_timer(&mcspi->rx_dma_timer, jiffies + msecs_to_jiffies(OMAP2_MCSPI_DMA_RX_TIMEOUT_MS));

	if (kfifo_len (rx_sw_fifo) >= rx_thres)
  		wake_up_interruptible(&wait_q);
}

static void spi_slave_start_dma_tx(void)
{
	struct spi_device *spi = omap_slave_buffered_spi_device;
	struct omap2_mcspi *mcspi = spi_master_get_devdata(spi->master);
	struct omap2_mcspi_dma *mcspi_dma = &mcspi->dma_channels[spi->chip_select];
	unsigned int fifo_size;
	unsigned long irqflags;

	fifo_size = kfifo_len(tx_sw_fifo);
	if (fifo_size == 0)
		return;

	spin_lock_irqsave(&tx_dma_lock, irqflags);
	if (mcspi->slave_tx_dma_used) {
		spin_unlock_irqrestore(&tx_dma_lock, irqflags);
		return;
	}
	mcspi->slave_tx_dma_used = true;
	spin_unlock_irqrestore(&tx_dma_lock, irqflags);

	/* This should never happen - max fifo size = (dma size - 1) */
	if (fifo_size > OMAP2_MCSPI_DMA_BUF_SIZE - 1)
		fifo_size = OMAP2_MCSPI_DMA_BUF_SIZE - 1;

	kfifo_get(tx_sw_fifo, mcspi->slave_tx_buf, fifo_size);

	/* Add a final dummy zero */
	mcspi->slave_tx_buf[fifo_size] = 0;
	
	omap_set_dma_transfer_params(mcspi_dma->dma_tx_channel,
				OMAP_DMA_DATA_TYPE_S8, fifo_size + 1, 1,
				OMAP_DMA_SYNC_ELEMENT, mcspi_dma->dma_tx_sync_dev, 0);

	omap_set_dma_src_params(mcspi_dma->dma_tx_channel, 0,
				OMAP_DMA_AMODE_POST_INC,
				mcspi->slave_tx_buf_dma_phys, 0, 0);

	wmb();
	omap_start_dma(mcspi_dma->dma_tx_channel);
}

static ssize_t spi_slave_buffered_write(struct file * fp,
					const char __user *buf,
					size_t len, loff_t *off)
{
	int ret = 0;
	char *local_ptr = NULL;

	/* never write more than the fifo can take */
	if(len > SW_FIFO_SIZE) {
		ret = -EFAULT;
		goto out;
	}

	local_ptr = kmalloc(len, GFP_KERNEL);
	if(!local_ptr) {
		ret = -EFAULT;
		goto out;
	}

	ret = copy_from_user(local_ptr, buf, len);
	if(ret != 0)
		goto copy_failed;

	ret = kfifo_put_overwrite(tx_sw_fifo, local_ptr, len);
	spi_slave_start_dma_tx();	

copy_failed:	
	kfree(local_ptr);
out:
	return (ret);
}

static ssize_t spi_slave_buffered_read(struct file *fp,
					char __user *buf,
					size_t len,
					loff_t *off)
{
	int ret = 0;
	int actual_len;
	char *local_ptr = NULL;

	/* never read more than the fifo size */
	if(len > SW_FIFO_SIZE) {
		ret = -EFAULT;
		goto out;
	}

	local_ptr = kmalloc(len, GFP_KERNEL);
	if(!local_ptr) {
		ret = -EFAULT;
		goto out;
	}

	actual_len = kfifo_get(rx_sw_fifo, local_ptr, len);	
	if(actual_len && (copy_to_user(buf, local_ptr, actual_len) != 0) ) {
		ret = -EFAULT;
		goto copy_failed;
	}
		
	ret = actual_len;

copy_failed:	
	kfree(local_ptr);
out:
	return (ret);
}

static int spi_slave_buffered_open(struct inode *ip, struct file *fp)
{
	return 0;
}

static unsigned int spi_slave_buffered_poll(struct file *file,
					    struct poll_table_struct *wait)
{
	unsigned int mask = 0;

	poll_wait(file, &wait_q, wait);

	if (kfifo_len (rx_sw_fifo) >= rx_thres)
		mask |= POLLIN | POLLRDNORM;

	return mask;
}

static const struct file_operations spi_slave_ops = {
	.open  = spi_slave_buffered_open,
	.read  = spi_slave_buffered_read,
	.write = spi_slave_buffered_write,
	.ioctl = spi_slave_buffered_ioctl,
	.poll  = spi_slave_buffered_poll,
};

static void transmit_data(void *any)
{
	unsigned long irqflags;
	struct spi_device *spi = omap_slave_buffered_spi_device;
	struct omap2_mcspi *mcspi = spi_master_get_devdata(spi->master);
	struct omap2_mcspi_dma *mcspi_dma = &mcspi->dma_channels[spi->chip_select];
	omap_stop_dma(mcspi_dma->dma_tx_channel);
	spin_lock_irqsave(&tx_dma_lock, irqflags);
	mcspi->slave_tx_dma_used = false;
	spin_unlock_irqrestore(&tx_dma_lock, irqflags);
	spi_slave_start_dma_tx();
}

static int spi_slave_buffered_probe(struct spi_device *spi)
{
	struct omap2_mcspi_device_config *device_config;
	int ret;

	spin_lock_init(&fifo_lock);
	spin_lock_init(&tx_dma_lock);
	init_waitqueue_head(&wait_q);

	/* Initialize fifos */
	tx_sw_fifo = kfifo_init(tx_buf, SW_FIFO_SIZE, GFP_KERNEL, &fifo_lock);
	if (tx_sw_fifo == NULL) {
		printk(KERN_ERR "spi_slave_buffered_probe: tx_sw_fifo allocation failure\n");
		ret = -EFAULT;
		goto tx_fifo_alloc_failed;
	}
	rx_sw_fifo = kfifo_init(rx_buf, SW_FIFO_SIZE, GFP_KERNEL, &fifo_lock);
	if (rx_sw_fifo == NULL) {
		printk(KERN_ERR "spi_slave_buffered_probe: rx_sw_fifo allocation failure\n");
		ret = -EFAULT;
		goto rx_fifo_alloc_failed;
	}

	/* Store pointer to spi device */
	omap_slave_buffered_spi_device = spi;
	/* Configure spi */
	spi->mode = SPI_MODE_0;
	spi->bits_per_word = 8;

	/* Now set SPI in slave mode */
	device_config = spi->controller_data;
	device_config->slave = 1; /* put controller in slave mode */
	device_config->tx_callback = transmit_data;
	device_config->rx_dma_timeout_func = omap2_mcspi_dma_rx_timeout;
	ret = spi_setup(spi); 
	if (ret < 0) {
		printk(KERN_ERR "spi_slave_buffered_probe: spi_setup() failure. %d\n", ret);
		goto spi_setup_failed;
	}

	/* Register as a misc device.  Get minor number. */
	omap_slave_buffered_misc_device.minor = MISC_DYNAMIC_MINOR;
	omap_slave_buffered_misc_device.name = DEVICE_NAME;
	omap_slave_buffered_misc_device.fops = &spi_slave_ops;
	ret = misc_register(&omap_slave_buffered_misc_device);
	if (ret < 0) {
		printk(KERN_ERR "spi_slave_buffered_probe: can't get minor number. %d\n", ret);
		goto misc_register_failed;
	}
	return 0;

misc_register_failed:
spi_setup_failed:
	kfree (rx_sw_fifo);
rx_fifo_alloc_failed:
	kfree (tx_sw_fifo);
tx_fifo_alloc_failed:
	return ret;
}

static int spi_slave_buffered_remove(struct spi_device *spi)
{
	kfree (rx_sw_fifo);
	kfree (tx_sw_fifo);
	return misc_deregister (&omap_slave_buffered_misc_device);
}

static int
spi_slave_buffered_suspend(struct spi_device *spi, pm_message_t mesg)
{
	wake_up_interruptible(&wait_q);
	return 0;
}

static int spi_slave_buffered_resume(struct spi_device *spi)
{
	return 0;
}

static struct spi_driver spi_slave_buffered_driver = {
	.probe           = spi_slave_buffered_probe,
	.remove	= __devexit_p(spi_slave_buffered_remove),
	.suspend         = spi_slave_buffered_suspend,
	.resume          = spi_slave_buffered_resume,
	.driver         = {
		.name   = "spi_slave_buffered",
		.bus    = &spi_bus_type,
		.owner  = THIS_MODULE,
	},
};

static int __init spi_slave_buffered_init(void)
{
	return spi_register_driver(&spi_slave_buffered_driver);
}

static void __exit spi_slave_buffered_exit(void)
{
	return spi_unregister_driver(&spi_slave_buffered_driver);
}


module_init(spi_slave_buffered_init);
module_exit(spi_slave_buffered_exit);
MODULE_LICENSE("GPL");

