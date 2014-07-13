/*
 * arch/arm/plat-tomtom/offenburg/ewts98.c
 *
 * Platform part of EWTS98 driver
 *
 * Copyright (C) 2011 TomTom International BV
 * Author: Matthias Kaehlcke <matthias.kaehlcke@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation version 2.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <linux/i2c/twl.h>
#include <linux/i2c/twl4030-madc.h>
#include <linux/ewts98.h>

#define ADC_X_ID	TWL4030_MADC_ADCIN6
#define ADC_X_CHAN_NO	6
#define ADC_Y_ID	TWL4030_MADC_ADCIN3
#define ADC_Y_CHAN_NO	3
#define MAX_RETRIES	2

static int offenburg_ewts98_read_adcs(struct ewts98_adc_values *adc_values)
{
	int rc;
	int retries = 0;

	while (1) {
		struct twl4030_madc_request req;

		req.channels = ADC_X_ID | ADC_Y_ID;
		req.do_avg = 1;
		req.method = TWL4030_MADC_SW1;
		req.active = 0;
		req.type = TWL4030_MADC_WAIT;
		req.func_cb = NULL;

		rc = twl4030_madc_conversion(&req);
		if (rc > 0) {
			/* successfully read ADC */
			adc_values->x = req.rbuf[ADC_X_CHAN_NO];
			adc_values->y = req.rbuf[ADC_Y_CHAN_NO];
			rc = 0;

			break;
		} else {
			if ((rc == -EAGAIN) && (retries < MAX_RETRIES)) {
				/* try again */

				retries++;

				printk(KERN_DEBUG "ewts98: ADC not ready, trying again\n");
			} else {
				printk(KERN_ERR "ewts98: Failed to read ADCs (error = %d)\n", rc);
				rc = EWTS98_ERROR_X | EWTS98_ERROR_Y;

				break;
			}
		}
	}

	return rc;
}

static struct ewts98_platform_data offenburg_ewts98_pdata = {
	.model		= EWTS98PA21,
	.sample_rate	= 20, /* without PMIC averaging: 10 */
	.averaging	= {
		.enabled = 1,
		.num_samples = 5 /* without PMIC averaging: 10 */
	},

	.adc_x		= {
		.resolution 		= 10,
		/* in an ideal world we'd get calibration from factory data */
		.zero_point_offset	= 614, /* (1.5V / 2.5V) * 1024 */
		.min_input_mV		= 0,
		.max_input_mV		= 2500,
	},

	.adc_y		= {
		.resolution 		= 10,
		/* in an ideal world we'd get calibration from factory data */
		.zero_point_offset	= 614, /* (1.5V / 2.5V) * 1024 */
		.min_input_mV		= 0,
		.max_input_mV		= 2500,
	},

	.read_adcs		= offenburg_ewts98_read_adcs,

	.fuzz_x		= 0,
	.fuzz_y		= 0
};

static struct platform_device offenburg_ewts98_pdev = {
	.name		= "ewts98",
	.id		= -1,
	.num_resources	= 0,
	.dev		= {
		.platform_data = &offenburg_ewts98_pdata,
	},
};

static int __init offenburg_ewts98_init(void)
{
	return platform_device_register(&offenburg_ewts98_pdev);
}

late_initcall(offenburg_ewts98_init);



