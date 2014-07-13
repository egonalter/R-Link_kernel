/*
 * s5m8751-bl.c  --  S5M8751 Advanced PMIC with AUDIO DAC
 *
 * Copyright 2009 Samsung Electronics.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/fb.h>
#include <linux/backlight.h>
#include <linux/slab.h>

#include <linux/mfd/s5m8751/core.h>
#include <linux/mfd/s5m8751/pdata.h>
#include <linux/mfd/s5m8751/wled.h>
#include "leds.h"

struct s5m8751 *s5m8751;

struct s5m8751_backlight_data {
	struct s5m8751 *s5m8751;
	int current_brightness;
};

void s5m8751_brightness_set(struct led_classdev *led_cdev,enum led_brightness brightness)
{
	int ret = 0;
        uint8_t val;
	struct s5m8751_backlight_pdata *pdata = container_of(led_cdev,struct s5m8751_backlight_pdata,cdev);

	pdata->brightness = brightness;
		
        brightness = brightness/8;
        if (brightness > S5M8751_WLED_MAX_BRIGHTNESS)
                brightness = S5M8751_WLED_MAX_BRIGHTNESS;

        val = S5M8751_WLED_TRIM(brightness);
        val |= brightness ? S5M8751_WLED_EN : 0;
        ret = s5m8751_reg_write(s5m8751, S5M8751_WLED_CNTRL, val);

        if (ret)
		printk("failed to set brightness\n");
}

int s5m8751_brightness_get(struct led_classdev *led_cdev)
{
	int brightness;
	struct s5m8751_backlight_pdata *pdata = container_of(led_cdev,struct s5m8751_backlight_pdata,cdev);
	brightness = pdata->brightness;
}

static int s5m8751_backlight_probe(struct platform_device *pdev)
{
	s5m8751 = dev_get_drvdata(pdev->dev.parent);
	struct s5m8751_backlight_pdata *pdata;
	int err;
	struct s5m8751_pdata *data = s5m8751->dev->platform_data;
	if (!data) {
		dev_err(&pdev->dev, " No platform data \n");
		return -EINVAL;
	}
#if 0
	if (pdev->dev.parent->platform_data) {
		//s5m8751_pdata = pdev->dev.parent->platform_data;
		s5m8751_pdata = pdev->dev.parent->platform_data;
		pdata = s5m8751_pdata->backlight;
	} else
		pdata = NULL;
#endif
	pdata = data->backlight;
	if (!pdata) {
		dev_err(&pdev->dev, "No platform data supplies\n");
		return -EINVAL;
	}
        pdata->cdev = kzalloc(sizeof(pdata->cdev),GFP_KERNEL);
        if(pdata->cdev == NULL){
                printk("Error allocating memmory for lcddev !!!!!!\n");
                return -ENOMEM;
        }
	pdata->cdev->name = "lcd-backlight";
	pdata->cdev->brightness = 10;
	pdata->cdev->max_brightness = MAX_BRIGHTNESS;
        pdata->cdev->brightness_set = s5m8751_brightness_set;	
	pdata->cdev->brightness_get = s5m8751_brightness_get;
	err = led_classdev_register(&pdev->dev, pdata->cdev);
	if(err){
		printk("failed to register backlight LED driver");
		return -EINVAL;
	}

	return 0;
}

static int __devexit s5m8751_backlight_remove(struct platform_device *pdev)
{
	struct s5m8751_pdata *data = platform_get_drvdata(pdev);
	led_classdev_unregister(data->backlight->cdev);
	kfree(data->backlight->cdev);
	return 0;
}

static struct platform_driver s5m8751_backlight_driver = {
	.driver		= {
		.name	= "s5m8751-backlight",
		.owner	= THIS_MODULE,
	},
	.probe		= s5m8751_backlight_probe,
	.remove		= s5m8751_backlight_remove,
};

static int __init s5m8751_backlight_init(void)
{
	return platform_driver_register(&s5m8751_backlight_driver);
}
module_init(s5m8751_backlight_init);

static void __exit s5m8751_backlight_exit(void)
{
	platform_driver_unregister(&s5m8751_backlight_driver);
}
module_exit(s5m8751_backlight_exit);

MODULE_DESCRIPTION("S5M8751 backlight driver");
MODULE_AUTHOR("Jongbin, Won <jongbin.won@samsung.com>");
MODULE_LICENSE("GPL");
