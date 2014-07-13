/*
 *  tps851_adc_lightsensor.c - Linux kernel modules for ambient light sensor
 *
 *  Copyright (C) 2008 Rogier Stam <rogier.stam@tomtom.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Ported from drivers/i2c/chips/tsl2550.c
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/hwmon.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/gadc.h>
#include <linux/tps851.h>

/*
 * SysFS support
 */

static ssize_t tps851_adc_lightsensor_read_lux(struct device *dev,
					struct device_attribute *attr,
					char *buf)
{
	struct tps851_adc_lightsensor	*platinfo=(struct tps851_adc_lightsensor *) dev->platform_data;
	struct gadc_sample		sample;
	int				nr_samples;

	gadc_get_samplecount( platinfo->buffer, &nr_samples );
	if( nr_samples > ADC_LIGHTSENSOR_NR_MOVING_AVG_VALUE )
		nr_samples=ADC_LIGHTSENSOR_NR_MOVING_AVG_VALUE;

	if( nr_samples == 0 )
		return -EBUSY;

	if( !gadc_get_moving_avg( platinfo->buffer, &sample, nr_samples ) )
		return -EIO;

	sample.value*=2;
	sample.value/=platinfo->Rload;
	return sprintf( buf, "%lu\n", sample.value );
}

static DEVICE_ATTR( lux, S_IRUGO, tps851_adc_lightsensor_read_lux, NULL );

/*
 * Init/probing/exit functions
 */

static int tps851_adc_lightsensor_probe( struct platform_device *pdevice )
{
	struct class_device		*cdev;
	struct tps851_adc_lightsensor	*platinfo=(struct tps851_adc_lightsensor *) pdevice->dev.platform_data;

	/* Under the hwmon class. */
	cdev=hwmon_device_register( &pdevice->dev );
	if( cdev == NULL )
		return -ENODEV;

	dev_set_drvdata( &pdevice->dev, cdev );

	/* Sysfs attributes. */
	if( device_create_file( &pdevice->dev, &dev_attr_lux ) )
	{
		hwmon_device_unregister( cdev );
		return -ENODEV;
	}
	
	/* Start the ADC channel. */
	platinfo->start_channel( platinfo );
	return 0;
}

static int tps851_adc_lightsensor_remove( struct platform_device *pdevice )
{
	struct class_device		*cdev=(struct class_device *) dev_get_drvdata( &pdevice->dev );
	struct tps851_adc_lightsensor	*platinfo=(struct tps851_adc_lightsensor *) pdevice->dev.platform_data;

	platinfo->stop_channel( platinfo );
	device_remove_file( &pdevice->dev, &dev_attr_lux );
	hwmon_device_unregister( cdev );
	return 0;
}

static void tps851_adc_lightsensor_shutdown( struct platform_device *pdevice )
{
	tps851_adc_lightsensor_remove( pdevice );
	return;
}

static struct platform_driver tps851_adc_lightsensor_driver =
{
	.probe		= tps851_adc_lightsensor_probe,
	.remove		= tps851_adc_lightsensor_remove,
	.shutdown	= tps851_adc_lightsensor_shutdown,
	.driver		=
	{
		.name	= ADC_LIGHTSENSOR_DRV_NAME,
		.owner	= THIS_MODULE,
	},
};

static int __init tps851_adc_lightsensor_init(void)
{
	return platform_driver_register(&tps851_adc_lightsensor_driver);
}

static void __exit tps851_adc_lightsensor_exit(void)
{
	platform_driver_unregister(&tps851_adc_lightsensor_driver);
}

MODULE_AUTHOR("Rogier Stam <rogier.stam@tomtom.com>");
MODULE_DESCRIPTION("TPS851 ADC based ambient light sensor driver");
MODULE_LICENSE("GPL");
MODULE_VERSION(DRIVER_VERSION);

module_init(tps851_adc_lightsensor_init);
module_exit(tps851_adc_lightsensor_exit);
