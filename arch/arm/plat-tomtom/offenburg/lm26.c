/*
 * Copyright (C) 2013 TomTom International BV
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
#include <linux/lm26.h>

#define LM26_ADC_CHANNEL	2
#define MAX_RETRIES		2

static int read_lm26_raw_adc(int *raw_adc_values)
{
	int rc;
	int retries = 0;

	while (1) {
		struct twl4030_madc_request req;

		req.channels = (1 << LM26_ADC_CHANNEL);
		req.do_avg = 0;
		req.method = TWL4030_MADC_SW1;
		req.active = 0;
		req.type = TWL4030_MADC_WAIT;
		req.func_cb = NULL;

		rc = twl4030_madc_conversion(&req);
		if (rc > 0) {
			/* successfully read ADC */
			*raw_adc_values = req.rbuf[LM26_ADC_CHANNEL];
			rc = 0;

			break;
		} else {
			if ((rc == -EAGAIN) && (retries < MAX_RETRIES)) {
				/* try again */
				retries++;
				printk(KERN_DEBUG"lm26:ADC not ready, -EAGAIN\n");
			} else {
				printk(KERN_ERR"lm26:read ADC fail, err=%d\n", rc);

				break;
			}
		}
	}

	return rc;
}

static struct lm26_platform_data lm26_pdata = {
	.sample_rate = 5,
	.read_raw_adc = read_lm26_raw_adc,
};

static struct platform_device lm26_pdev = {
	.name		= "lm26",
	.id		= -1,
	.dev		= {
		.platform_data = &lm26_pdata,
	},
};

static int __init lm26_init(void)
{
	printk(KERN_INFO "Initializing temperature sensor LM26\n");

	return platform_device_register(&lm26_pdev);
}

late_initcall(lm26_init);

