/*
 * Copyright (C) 2010 TomTom BV <http://www.tomtom.com/>
 * Authors: Andrzej Zukowski <andrzej.zukowski@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/platform_device.h>
#include <plat/tt_idpinmon.h>
#include <plat/mendoza.h>
#include <mach/gpio.h>
#include <plat/adc.h>

#define ADC_REFERENCE		(3300)
#define ADC_HOST_MIN		(1865 - 100)
#define ADC_HOST_MAX		(1865 + 100)
#define ADC_ACCURACY		(12)

static struct idpinmon_adc adc_idpin = {
	.channel		= AIN1,
	.samples		= 1,
	.otgmode		= IDPINMON_DEVICE,
};

static void idpinmon_adc_callback(unsigned int data0,
		unsigned int data1,
		void *data)
{
	struct idpinmon_adc *adc = (struct idpinmon_adc *) data;
	unsigned int value;

	value = (ADC_REFERENCE * data0) >> ADC_ACCURACY;

	if ((value >= ADC_HOST_MIN) && (value <= ADC_HOST_MAX))
		adc->otgmode = IDPINMON_HOST;
	else
		adc->otgmode = IDPINMON_DEVICE;

	complete(&adc->complete);
}

static int idpinmon_gpio_request(void)
{
	int ret;

	if ((ret = gpio_request(TT_VGPIO_ID_PIN, "TT_VGPIO_ID_PIN")))
	{
		printk(KERN_ERR "Unable to request TT_VGPIO_ID_PIN GPIO (%d)\n", ret);
		return ret;
	}
	return ret;
}

static void idpinmon_gpio_free(void)
{
	gpio_free(TT_VGPIO_ID_PIN);
}

static void idpinmon_otg_host(void)
{
	gpio_direction_output(TT_VGPIO_ID_PIN, IDPINMON_HOST);
}

static void idpinmon_otg_device(void)
{
	gpio_direction_output(TT_VGPIO_ID_PIN, IDPINMON_DEVICE);
}

static unsigned int idpinmon_otg_getmode(void)
{
	unsigned int otgmode = IDPINMON_DEVICE;

	mutex_lock(&adc_idpin.mutex);

	init_completion(&adc_idpin.complete);
	s3c_adc_start(adc_idpin.client, adc_idpin.channel, adc_idpin.samples);
	wait_for_completion(&adc_idpin.complete);

	mutex_unlock(&adc_idpin.mutex);
	return adc_idpin.otgmode;
}

static struct idpinmon_pdata pdata = {
	.idpinmon_state		= IDPINMON_DEVICE,

	.request_gpio		= idpinmon_gpio_request,
	.free_gpio		= idpinmon_gpio_free,

	.otg_host		= idpinmon_otg_host,
	.otg_device		= idpinmon_otg_device,
	.otg_getmode		= idpinmon_otg_getmode,

	.delay			= HZ,
};

struct platform_device idpinmon_pdev = {
	.name	= "tomtom-idpinmon",
	.id	= -1,
	.dev	= {
		.platform_data = &pdata,
	},
};

int idpinmon_init( void )
{
	struct s3c_adc_client *client;
	int ret;

	mutex_init(&adc_idpin.mutex);

	client = s3c_adc_register( &idpinmon_pdev,
			NULL,
			idpinmon_adc_callback,
			&adc_idpin,
			0);

	if (client == NULL) {
		printk(KERN_ERR "Unable to register ADC client\n");
		ret = -ENOENT;
		goto err_adc;
	}

	adc_idpin.client = client;

	if ((ret = platform_device_register( &idpinmon_pdev )))
	{
		printk(KERN_ERR "Unable to register TomTom ID Pin device (%d)\n", ret);
		goto err_pdev;
	}


	printk(KERN_INFO "TomTom ID Pin device registered\n");
	return 0;

err_pdev:
	s3c_adc_release(client);
err_adc:
	return ret;
}
arch_initcall(idpinmon_init);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("S3C64XX ID Pin Arch code");
