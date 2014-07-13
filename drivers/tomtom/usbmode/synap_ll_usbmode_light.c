/* drivers/tomtom/usbmode/synap_ll_usbmode_light.c

Copyright (C) 2008 TomTom BV <http://www.tomtom.com/>
Shamelessly Ripped from: drivers/tomtom/usbmode/s3c64xx_usbmode.c
Authors: Andrzej Zukowski <andrzej.zukowski@tomtom.com>
	Rogier Stam <rogier.stam@tomtom.com>

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

*/

#include <linux/module.h>
#include <linux/platform_device.h>
#include <plat/usbmode.h>
#include <plat/tt_vbusmon.h>
#include <plat/synap_ll_usbmode_light_plat.h>
#include <asm/io.h>

#include "synap_ll_usbmode_light.h"


static int synap_ll_usbmode_noop(struct usbmode_data *data)
{
	return 0;
};

static void synap_ll_usbmode_reset_timeout( unsigned long data )
{	
	/* Sent the event. */
	usbmode_push_event( INPUT_DEVICE_DETECT_TIMEOUT );
	return;
}

static int synap_ll_usbmode_stop_timer(struct usbmode_data *data)
{
	struct device *dev = (struct device *) data->u_impl_data->data;
	struct synap_ll_usbmode_priv *privates = dev_get_drvdata( dev );

	if( timer_pending( &(privates->timer) ) )
	{
		/* Delete the timer. */
		del_timer( &(privates->timer) );
	}
	return 0;
};

static int synap_ll_vbus_state_change(struct notifier_block *self, unsigned long vbus_on, void *arg)
{
	struct synap_ll_usbmode_priv		*usbmode_priv=container_of( self, struct synap_ll_usbmode_priv, vbus_change_nb );
	struct device				*dev = (struct device *) usbmode_priv->impl_data.data;
	struct platform_device			*pdev = to_platform_device(dev);
	struct synap_ll_usbmode_platform_data	*pdata = (struct synap_ll_usbmode_platform_data *) dev->platform_data;

	if (vbus_on) 
	{
		/* Start of session, e.g VBUS is here. */
		usbmode_push_event(INPUT_BUS_POWER_ON_EVENT);
	} 
	else 
	{
		/* VBUS dropped. */
		if( (pdata->is_usbhost != NULL) && pdata->is_usbhost( pdev ) )
			usbmode_push_event(INPUT_HOST_DEVICE_DISCONNECT);
		usbmode_push_event(INPUT_BUS_POWER_OFF_EVENT);
	}
	return 0;
}
					
static int synap_ll_idle_to_device_detect(struct usbmode_data *data)
{
	struct device *dev = (struct device *) 	data->u_impl_data->data;
	struct synap_ll_usbmode_platform_data	*pdata = (struct synap_ll_usbmode_platform_data *) dev->platform_data;
	struct synap_ll_usbmode_priv		*usbmode_priv = dev_get_drvdata( dev );

	if( !timer_pending( &(usbmode_priv->timer) ) )
	{
		init_timer( &(usbmode_priv->timer) );
		usbmode_priv->timer.function=synap_ll_usbmode_reset_timeout;
		usbmode_priv->timer.data=((unsigned long) dev);
		usbmode_priv->timer.expires = jiffies + pdata->device_detect_timeout * HZ;
		add_timer( &(usbmode_priv->timer) );
	}
	else
		printk( KERN_ERR "USBMode timer already running!\n" );
	return 0;
};

static irqreturn_t synap_ll_usbmode_vbus_irq(int irq, void *dev)
{
	struct platform_device			*pdev = to_platform_device(dev);
	struct synap_ll_usbmode_platform_data	*plat_data=(struct synap_ll_usbmode_platform_data *) pdev->dev.platform_data;

	return plat_data->irqhandler( pdev );	
}

static int synap_ll_device_detect_to_host_detect(struct usbmode_data *data)
{
	struct device *dev = (struct device *) 	data->u_impl_data->data;
	struct synap_ll_usbmode_platform_data	*pdata = (struct synap_ll_usbmode_platform_data *) dev->platform_data;
	struct platform_device			*pdev = to_platform_device(dev);

	if( (pdata->is_usbhost != NULL) && pdata->is_usbhost( pdev ) )
		usbmode_push_event( INPUT_HOST_DEVICE_CONNECT );
	else
		usbmode_push_event( INPUT_HOST_DETECT_TIMEOUT );

	return 0;
}

static struct usbmode_operations synap_ll_usbmode_ops = {
	.initial_to_idle		= synap_ll_usbmode_noop,
	.idle_to_device_detect		= synap_ll_idle_to_device_detect,
	.device_detect_to_idle		= synap_ll_usbmode_stop_timer,
	.device_detect_to_device_wait	= synap_ll_usbmode_stop_timer,
	.device_wait_to_idle		= synap_ll_usbmode_noop,
	.device_wait_to_device		= synap_ll_usbmode_noop,
	.device_to_idle			= synap_ll_usbmode_noop,
	.device_detect_to_host_detect	= synap_ll_device_detect_to_host_detect,
	.host_detect_to_idle		= synap_ll_usbmode_noop,
	.host_detect_to_cla		= synap_ll_usbmode_noop,
	.host_detect_to_host		= synap_ll_usbmode_noop,
	.cla_to_idle			= synap_ll_usbmode_noop,
	.host_to_idle			= synap_ll_usbmode_noop,
	.poll_controller		= synap_ll_usbmode_noop,
};

static int synap_ll_usbmode_probe(struct platform_device *pdev)
{
	struct synap_ll_usbmode_priv		*usbmode_priv = NULL;
	struct synap_ll_usbmode_platform_data	*plat_data=(struct synap_ll_usbmode_platform_data *) pdev->dev.platform_data;
	int					irq, res, err;

	printk(KERN_INFO DRIVER_DESC_LONG "Generic/light version\n");
	err = plat_data->setup( pdev );
	if( err < 0 )
	{
		printk( KERN_ERR "%s arch setup code is failing!\n", __func__ );
		res = -ENXIO;
		goto err_no_mem;
	}

	usbmode_priv = kmalloc(sizeof(struct synap_ll_usbmode_priv), GFP_KERNEL);
	if (!usbmode_priv) 
	{
		printk(KERN_ERR "%s unable to allocated memory for "
			"usbmode_priv\n", __func__);
		res = -ENOMEM;
		goto err_no_mem;
	}
	memset(usbmode_priv, 0, sizeof( *usbmode_priv ));

	platform_set_drvdata(pdev, usbmode_priv);
	init_timer( &(usbmode_priv->timer) );
	usbmode_priv->timer.function = synap_ll_usbmode_reset_timeout;
	usbmode_priv->timer.data = ((unsigned long) usbmode_priv);
	usbmode_priv->impl_data.id = 1;
	usbmode_priv->impl_data.data = (void *) &(pdev->dev);
	usbmode_priv->vbus_change_nb.notifier_call = synap_ll_vbus_state_change;

	irq = platform_get_irq( pdev, 0 );
	if( irq < 0 )
	{
		printk(KERN_ERR "Can't get USBMode IRQ ! Not in platform device!\n" );
		res = -ENXIO;
		goto err_no_irq;
	}

	err = request_irq(irq, synap_ll_usbmode_vbus_irq, IRQF_SHARED, "synap_vbus_usbmode", &(pdev->dev));
	if (err)
	{
		printk(KERN_ERR "Can't get USBMode IRQ! Request error!\n" );
		res = -ENODEV;
		goto err_no_irq;
	}

	err = register_usbmode_driver( &synap_ll_usbmode_ops, &(usbmode_priv->impl_data));
	if (err)
	{
		printk(KERN_ERR "Can't register to USBMode!\n" );
		res = -ENODEV;
		goto err_no_usbdrv;
	}

	err = vbusmon_register_notifier(&(usbmode_priv->vbus_change_nb));
	if (err)
	{
		printk(KERN_ERR "Can't register VBUS monitor function!\n" );
		res = -ENODEV;
		goto err_no_vbusmon;
	}
	return 0;

err_no_vbusmon:
	unregister_usbmode_driver();

err_no_usbdrv:
	free_irq( irq, &(pdev->dev) );

err_no_irq:
	kfree( usbmode_priv );

err_no_mem:
	return res;
}

static int synap_ll_usbmode_remove(struct platform_device *pdev)
{
	struct synap_ll_usbmode_priv *usbmode_priv = platform_get_drvdata(pdev);

	vbusmon_unregister_notifier(&(usbmode_priv->vbus_change_nb));
	del_timer_sync(&usbmode_priv->timer);
	unregister_usbmode_driver();
	free_irq( platform_get_irq( pdev, 0 ), &(pdev->dev) );
	kfree( usbmode_priv );
	return 0;
}

static struct platform_driver synap_ll_usbmode_platform_driver =
{
	.probe   = synap_ll_usbmode_probe,
	.remove  = synap_ll_usbmode_remove,
	.driver  =
	{
		.owner = THIS_MODULE,
		.name = "synap-ll-usbmode",
	},
};

static int __init synap_ll_usbmode_init(void)
{
	if( platform_driver_register(&synap_ll_usbmode_platform_driver) )
	{
		printk(KERN_ERR " unable to register USBMode synap_ll driver\n");
		return -ENODEV;
	}
	return 0;
}

static void __exit synap_ll_usbmode_exit(void)
{
	platform_driver_unregister(&synap_ll_usbmode_platform_driver);
	return;
}

module_init( synap_ll_usbmode_init );
module_exit( synap_ll_usbmode_exit );

MODULE_DESCRIPTION(DRIVER_DESC_LONG);
MODULE_AUTHOR( "Andrzej Zukowski (andrzej.zukowski@tomtom.com)" );
MODULE_AUTHOR( "Rogier Stam (rogier.stam@tomtom.com)" );
MODULE_LICENSE("GPL");
