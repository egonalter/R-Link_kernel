/*
 * Copyright (C) 2008 TomTom BV <http://www.tomtom.com/>
 * Author: Jeroen Taverne <jeroen.taverne@tomtom.com>
 * Author: Rogier Stam <rogier.stam@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/
#include <linux/platform_device.h>
#include <linux/list.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/gadc.h>
#include <linux/gadc_titan2.h>

static int titan2_gadc_open( struct inode *inode, struct file *file )
{
	struct gadc_cdev		*chardev=(struct gadc_cdev *) get_gadc_cdev( inode->i_cdev );
	struct gadc_platform_interface	*pdata=(struct gadc_platform_interface *) chardev->private;
	struct gadc_platform_channel	*channel=&(pdata->channels[(iminor( file->f_dentry->d_inode ) - chardev->minor)]);

	if( pdata->is_running( channel ) )
		return -EBUSY;

	/* Initialize data. */
	gadc_init_buffer( channel->buffer, 1 );
	gadc_set_samplerate( channel->buffer, 1000 );
	pdata->start_channel( channel );
	file->private_data=chardev;
	return 0;
}

static int titan2_gadc_release(struct inode *inode, struct file *file )
{
	struct gadc_cdev		*chardev=(struct gadc_cdev *) get_gadc_cdev( inode->i_cdev );
	struct gadc_platform_interface	*pdata=(struct gadc_platform_interface *) chardev->private;
	struct gadc_platform_channel	*channel=&(pdata->channels[(iminor( file->f_dentry->d_inode ) - chardev->minor)]);

	pdata->stop_channel( channel );
	gadc_clear_buffer( channel->buffer );
	file->private_data=NULL;
	return 0;
}

static unsigned titan2_gadc_poll(struct file *file, poll_table *wait)
{
	struct gadc_cdev		*chardev=(struct gadc_cdev *) file->private_data;
	struct gadc_platform_interface	*pdata=(struct gadc_platform_interface *) chardev->private;
	struct gadc_platform_channel	*channel=&(pdata->channels[(iminor( file->f_dentry->d_inode ) - chardev->minor)]);
	int				tmpval;
	unsigned int			mask = 0;

	gadc_poll_wait( file, channel->buffer, wait );
	if ( gadc_get_samplecount( channel->buffer, &tmpval ) > 0 )
		mask |= POLLIN | POLLRDNORM;

	return mask;
}

static int titan2_gadc_ioctl( struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg )
{
	struct gadc_cdev		*chardev=(struct gadc_cdev *) file->private_data;
	struct gadc_platform_interface	*pdata=(struct gadc_platform_interface *) chardev->private;
	struct gadc_platform_channel	*channel=&(pdata->channels[(iminor( file->f_dentry->d_inode ) - chardev->minor)]);
	struct gadc_format		format;
	unsigned long int		samplerate;

	switch( cmd )
	{
		case ADC_SET_SAMPLERATE:
			/* We are limited by HZ and the ADC rate, so make sure the samplerate matches this. */
			samplerate=(unsigned long int) arg;
			if( samplerate > 1000 * HZ ) samplerate=1000 * HZ;
			if( samplerate > 1000 * TITAN2_GADC_MAX_SAMPLERATE ) samplerate=1000 * TITAN2_GADC_MAX_SAMPLERATE;
			if( samplerate == 0 )
				return -EINVAL;

			samplerate=1000 * HZ/((HZ * 1000 + (samplerate >> 1))/samplerate);
			gadc_set_samplerate( channel->buffer, samplerate );

			/* Restart the ADC channel. Otherwise we have to wait for 1 tick. */
			pdata->stop_channel( channel );
			pdata->start_channel( channel );
			break;

		case ADC_GET_SAMPLERATE:
			*((unsigned long int *) arg)=gadc_get_samplerate( channel->buffer );
			break;

		case ADC_GET_BUFFERSTATUS:
			*((unsigned long int *) arg)=gadc_get_status( channel->buffer );
			break;

		case ADC_GET_FORMAT:
			format.bitformat=GADC_BF_ULHA;
			format.accuracy=10;
			copy_to_user( (void *) arg, &format, sizeof( format ) );
			break;

		default :
			return -ENOTTY;
	}

	return 0;
}

static ssize_t titan2_gadc_read( struct file *file, char __user *buf, size_t count, loff_t * loff)
{
	struct gadc_cdev		*chardev=(struct gadc_cdev *) file->private_data;
	struct gadc_platform_interface	*plat_interface=(struct gadc_platform_interface *) chardev->private;
	struct gadc_platform_channel	*channel=&(plat_interface->channels[(iminor( file->f_dentry->d_inode ) - chardev->minor)]);
	struct gadc_sample		sample;
        int				samplecount;
	int				maxbufsamples;
	int				loop=0;

	/* Determine the amount of samples that'll fit in the buffer. */
	gadc_get_samplecount( channel->buffer, &samplecount );

	/* If no samples are present block & wait. */
	if( samplecount == 0 )
	{
		if( !(file->f_flags & O_NONBLOCK) )
		{
			if( gadc_wait_buffer_fill( channel->buffer, &samplecount ) )
				return -ERESTARTSYS;
		}
		else return -EAGAIN;
	}

	/* Determine how many samples to read. */
	maxbufsamples=count/sizeof( sample );
	if( samplecount > maxbufsamples ) samplecount=maxbufsamples;

	/* Copy sample by sample. Since we need to copy to user space, do this in two steps. */
	for( loop=0; loop < samplecount; loop++ )
	{
		if( !gadc_get_sample( channel->buffer, &sample ) )
			break;

		if( copy_to_user( &(((struct gadc_sample *) buf)[loop]), &sample, sizeof( sample ) ) )
			return -EINVAL;
	}

	/* Return the read bytes. */
	return loop * sizeof( struct gadc_sample );
}

struct file_operations titan2_gadc_fops=
{
	.owner	=THIS_MODULE,
	.read	=titan2_gadc_read,
	.poll	=titan2_gadc_poll,

	.ioctl	=titan2_gadc_ioctl,
	.open	=titan2_gadc_open,
	.release=titan2_gadc_release,
};

static int titan2_gadc_probe( struct platform_device *dev )
{
	struct gadc_platform_interface	*pdata=(struct gadc_platform_interface *) dev->dev.platform_data;
	struct gadc_cdev		*chardev;

	printk(KERN_INFO "TomTom GO TITAN2 Generic ADC Driver, (C) 2008 TomTom BV\n" );
	if( pdata == NULL )
	{
		printk( TITAN2_GADC_DRIVER_NAME": Missing platform data!\n" );
		return -ENODEV;
	}

	/* Determine the number of channels we'll register for this driver. */
	if( pdata->nr_channels == 0 )
	{
		printk( TITAN2_GADC_DRIVER_NAME": No channels detected!\n" );
		return -ENODEV;
	}

	chardev=(struct gadc_cdev *) kmalloc( sizeof( struct gadc_cdev ), GFP_KERNEL );
	if( chardev == NULL )
	{
		printk( TITAN2_GADC_DRIVER_NAME": Error allocating memory for cdev!\n" );
		return -ENOMEM;
	}

	/* Register the character device. */
	if( register_gadc_cdev( chardev, pdata->channels[0].channel.major, pdata->channels[0].channel.minor, pdata->nr_channels, TITAN2_GADC_DRIVER_NAME, &titan2_gadc_fops ) )
	{
		printk( TITAN2_GADC_DRIVER_NAME": Error registring cdev!\n" );
		kfree( chardev );
		return -ENODEV;
	}
	chardev->private=pdata;

	/* Set private device data. */
	dev->dev.driver_data=chardev;

	/* Done. */
	return 0;
}

static int titan2_gadc_remove( struct platform_device *dev )
{
	struct gadc_cdev	*chardev=(struct gadc_cdev *) dev->dev.driver_data;

	unregister_gadc_cdev( chardev );
	kfree( chardev );
	return 0;
}

static struct platform_driver	titan2_gadc_driver=
{
	.driver=
	{
		.name	= TITAN2_GADC_DRIVER_NAME,
		.owner	= THIS_MODULE,
	},
	.probe		= titan2_gadc_probe,
	.remove		= titan2_gadc_remove,
};

static int __init titan2_gadc_init( void )
{
	return platform_driver_register(&titan2_gadc_driver);
}

module_init( titan2_gadc_init );

static void __exit titan2_gadc_exit( void )
{
	platform_driver_unregister(&titan2_gadc_driver);
}
module_exit(titan2_gadc_exit);

MODULE_DESCRIPTION("TITAN2 GADC Driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Rogier Stam <rogier.stam@tomtom.com>");
