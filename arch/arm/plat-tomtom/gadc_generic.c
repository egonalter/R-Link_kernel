/* drivers/barcelona/gadc/gadc_generic.c
 *
 * Implementation of the Generic ADC interface.
 *
 * Copyright (C) 2007,2008 TomTom BV <http://www.tomtom.com/>
 * Author: Rogier Stam <Rogier.Stam@tomtom.com>
 *         Jeroen Taverne <Jeroen.Taverne@tomtom.com>
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
#include <linux/wait.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/gadc.h>

struct gadc_cdev *get_gadc_cdev( struct cdev *cdev )
{
	return (struct gadc_cdev *) container_of( cdev, struct gadc_cdev, dev );
}
EXPORT_SYMBOL( get_gadc_cdev );

int register_gadc_cdev( struct gadc_cdev *cdev, unsigned int major, unsigned int minor, unsigned int count, const char *name, struct file_operations *fops )
{
	char *s;

	/* Keep registring until we find one that is available. */
	if( register_chrdev_region( MKDEV( major, minor ), count, name ) )
		return -1;

	cdev->name=(char *) name;
	cdev->major=major;
	cdev->minor=minor;
	cdev->minor_range=count;

	cdev_init( &cdev->dev, fops );
	cdev->dev.owner = fops->owner;
	cdev->dev.ops = fops;
	kobject_set_name(&cdev->dev.kobj, "%s", name);
	for (s = strchr(kobject_name(&cdev->dev.kobj),'/'); s; s = strchr(s, '/'))
		*s = '!';

	if( cdev_add(&cdev->dev, MKDEV(major, minor), count) )
		goto register_gadc_cdev_error;

	return 0;
register_gadc_cdev_error:
	kobject_put(&cdev->dev.kobj);
	unregister_chrdev_region(MKDEV(major, minor), count);
	return -2;
}
EXPORT_SYMBOL( register_gadc_cdev );

void unregister_gadc_cdev( struct gadc_cdev *cdev )
{
	kobject_put( &cdev->dev.kobj );
	unregister_chrdev_region( MKDEV( cdev->major, cdev->minor ), cdev->minor_range );
	cdev_del( &cdev->dev );
	return;
}
EXPORT_SYMBOL( unregister_gadc_cdev );

void gadc_init_buffer( struct gadc_buffer *adc, unsigned long int samplerate )
{
        memset( adc, 0, sizeof( *adc ) );
        spin_lock_init( &(adc->lock) );
        init_waitqueue_head( &(adc->poll_queue) );
        adc->sample_rate=samplerate;
	return;
}
EXPORT_SYMBOL( gadc_init_buffer );

void gadc_add_sample( struct gadc_buffer *adc, unsigned long int value )
{
	unsigned long int	flags;

	spin_lock_irqsave( &(adc->lock), flags );

	do_gettimeofday( &(adc->buffer[adc->curr_idx].timestamp) );
	adc->buffer[adc->curr_idx].value=value;

	adc->curr_idx=(adc->curr_idx + 1) % ADC_BUFFER_SIZE;
	if( adc->curr_idx == adc->last_read_idx )
	{
		/* Overflow!. Increment the last_read index also. */
		adc->last_read_idx=(adc->last_read_idx + 1) % ADC_BUFFER_SIZE;
		adc->flags|=ADC_SAMPLE_OVERFLOW;
	}

	spin_unlock_irqrestore( &(adc->lock), flags );
	return;
}
EXPORT_SYMBOL( gadc_add_sample );

void gadc_clear_buffer( struct gadc_buffer *adc )
{
	unsigned long int	flags;

	spin_lock_irqsave( &(adc->lock), flags );
	memset( adc->buffer, 0, sizeof( adc->buffer ) );
	adc->curr_idx=0;
	adc->last_read_idx=0;
	spin_unlock_irqrestore( &(adc->lock), flags );
	return;
}
EXPORT_SYMBOL( gadc_clear_buffer );

int gadc_get_sample( struct gadc_buffer *adc, struct gadc_sample *target )
{
	unsigned long int	flags;

	spin_lock_irqsave( &(adc->lock), flags );

	if( adc->last_read_idx == adc->curr_idx )
	{
		/* Underflow!. Don't return anything. */
		adc->flags|=ADC_SAMPLE_UNDERFLOW;

		spin_unlock_irqrestore( &(adc->lock), flags );
		return 0;
	}

	memcpy( target, &(adc->buffer[adc->last_read_idx]), sizeof( *target ) );

	adc->last_read_idx=(adc->last_read_idx + 1) % ADC_BUFFER_SIZE;

	spin_unlock_irqrestore( &(adc->lock), flags );
	return 1;
}
EXPORT_SYMBOL( gadc_get_sample );

int gadc_get_moving_avg( struct gadc_buffer *adc, struct gadc_sample *target, int nr_samples )
{
	unsigned long int	flags;
	int			last_read_idx;
	u64			hpdiv;
	int			count;

	spin_lock_irqsave( &(adc->lock), flags );

	hpdiv=0;
	last_read_idx=adc->last_read_idx;
	for( count=0; count < nr_samples; count++ )
	{
		if( last_read_idx == adc->curr_idx )
		{
			/* Not enough items for a moving average. We don't update the underflow for this! */
			spin_unlock_irqrestore( &(adc->lock), flags );
			return 0;
		}

		hpdiv+=(u64) (adc->buffer[last_read_idx].value);
		last_read_idx=(last_read_idx + 1) % ADC_BUFFER_SIZE;
	}
	spin_unlock_irqrestore( &(adc->lock), flags );
	do_div( hpdiv, nr_samples );
	target->value=(unsigned long int) hpdiv;
	return 1;
}
EXPORT_SYMBOL( gadc_get_moving_avg );

int gadc_get_samplecount( struct gadc_buffer *adc, int *samplecount )
{
	unsigned long int	flags;

	spin_lock_irqsave( &(adc->lock), flags );

	*samplecount=adc->curr_idx - adc->last_read_idx;
	if( *samplecount < 0 ) *samplecount+=ADC_BUFFER_SIZE;

	spin_unlock_irqrestore( &(adc->lock), flags );
	return *samplecount;
}
EXPORT_SYMBOL( gadc_get_samplecount );

unsigned long int gadc_get_status( struct gadc_buffer *adc )
{
	unsigned long int	flags;
	unsigned long int	retval;
	int			samplecount;

	spin_lock_irqsave( &(adc->lock), flags );

	retval=((unsigned long int) adc->flags);
	samplecount=adc->curr_idx - adc->last_read_idx;
	if( samplecount < 0 ) samplecount+=ADC_BUFFER_SIZE;
	samplecount<<=16;
	retval+=samplecount;

	adc->flags=0;

	spin_unlock_irqrestore( &(adc->lock), flags );
	return retval;
}
EXPORT_SYMBOL( gadc_get_status );

void gadc_set_samplerate( struct gadc_buffer *adc, unsigned long int sample_rate )
{
	unsigned long int	flags;

	spin_lock_irqsave( &(adc->lock), flags );
	adc->sample_rate=sample_rate;
	spin_unlock_irqrestore( &(adc->lock), flags );
	return;
}
EXPORT_SYMBOL( gadc_set_samplerate );

unsigned long int gadc_get_samplerate( struct gadc_buffer *adc )
{
	unsigned long int	flags;
	unsigned long int	retval;

	spin_lock_irqsave( &(adc->lock), flags );
	retval=adc->sample_rate;
	spin_unlock_irqrestore( &(adc->lock), flags );
	return retval;
}
EXPORT_SYMBOL( gadc_get_samplerate );

int gadc_wait_buffer_fill( struct gadc_buffer *adc, int *samplecount )
{
	return wait_event_interruptible( (adc->poll_queue), (gadc_get_samplecount( adc, samplecount ) != 0) );
}
EXPORT_SYMBOL( gadc_wait_buffer_fill );

int gadc_wait_buffer_overflow( struct gadc_buffer *adc )
{
	return wait_event_interruptible( (adc->poll_queue), (gadc_get_status( adc ) & ADC_SAMPLE_OVERFLOW) );
}
EXPORT_SYMBOL( gadc_wait_buffer_overflow );

void gadc_poll_wait( struct file *file, struct gadc_buffer *adc, poll_table *wait )
{
	poll_wait( file, &(adc->poll_queue), wait );
}
EXPORT_SYMBOL( gadc_poll_wait );
