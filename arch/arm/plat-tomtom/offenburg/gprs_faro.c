/*
 * Copyright (C) 2009 TomTom BV <http://www.tomtom.com/>
 * Author: Rogier Stam <rogier.stam@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/
 

#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/vgpio.h>
#include <plat/offenburg.h>
#include <plat/gprs_types.h>
#include <plat/gprs.h>

static int gprs_request_gpio( void )
{
	int err = 0;

	if ((err = gpio_request(TT_VGPIO_GSM_SYS_EN, "TT_VGPIO_GSM_SYS_EN"))) {
		printk("%s: Can't request TT_VGPIO_GSM_SYS_EN GPIO\n", __func__);
		return err;
	}

	if ((err = gpio_request(TT_VGPIO_GSM_SYS_RST, "TT_VGPIO_GSM_SYS_RST"))) {
		printk("%s: Can't request TT_VGPIO_GSM_SYS_RST GPIO\n", __func__);
		return err;
	}
	
	gpio_direction_output( TT_VGPIO_GSM_SYS_EN, 0 );
	gpio_direction_output( TT_VGPIO_GSM_SYS_RST, 0 );

	return err;
}

static int gprs_power( int ena_dis )
{
	if( ena_dis )
		gpio_direction_output( TT_VGPIO_GSM_SYS_EN, 1 );
	else
		gpio_direction_output( TT_VGPIO_GSM_SYS_EN, 0 );
	return 0;
}

static int gprs_reset( int ena_dis )
{
	if( ena_dis )
		gpio_direction_output( TT_VGPIO_GSM_SYS_RST, 1 );
	else
		gpio_direction_output( TT_VGPIO_GSM_SYS_RST, 0 );
	
	return 0;
}

static void gprs_suspend( void )
{
	gpio_direction_output( TT_VGPIO_GSM_SYS_RST, 0 );
	gpio_direction_output( TT_VGPIO_GSM_SYS_EN, 0 );
}

static void gprs_setup_port(void)
{
	/* Do nothing for now. */
}

static struct gprs_platform_data gprs_pdata=
{
	.gprs_power	=gprs_power,
	.gprs_reset	=gprs_reset,
	.gprs_suspend   =gprs_suspend,
	.gprs_setup_port=gprs_setup_port,
	.gprs_id	=GOGPRS_FARO,
};

struct platform_device gprs_pdev=
{
	.name	="gprs",
	.id	=-1,
	.dev.platform_data=((void *) &gprs_pdata),
};

int gprs_init( void )
{
	int err =0;
	printk(KERN_INFO"TomTom gprs device registered\n");
	
	if( ( err = gprs_request_gpio()) ){
		printk(KERN_INFO"GPRS: error requesting gpio\n");
		return 1;
	}
	return platform_device_register( &gprs_pdev );
}
arch_initcall( gprs_init );
