/*
 * s5m8752-bl.c  --  S5M8752 Advanced PMIC with AUDIO Codec
 *
 * Copyright 2010 Samsung Electronics.
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

#include <linux/mfd/s5m8752/core.h>
#include <linux/mfd/s5m8752/pdata.h>
#include <linux/mfd/s5m8752/wled.h>

struct s5m8752_backlight_data {
	struct s5m8752 *s5m8752;
	int current_brightness;
};

/******************************************************************************
 * This function write s5m8752 backlight data via I2C functions.
 ******************************************************************************/
static int s5m8752_backlight_set(struct backlight_device *bl,
							int brightness)
{
	struct s5m8752_backlight_data *data = bl_get_data(bl);
	struct s5m8752 *s5m8752 = data->s5m8752;
	int ret = 0;
	uint8_t val;

	if (brightness > S5M8752_WLED_MAX_BRIGHTNESS)
		brightness = S5M8752_WLED_MAX_BRIGHTNESS;

	val = S5M8752_WLED_TRIM(brightness);
	s5m8752_clear_bits(s5m8752, S5M8752_WLED_CNTL1, S5M8752_WLED2_EN);

	if (brightness) {
		s5m8752_reg_write(s5m8752, S5M8752_WLED_CNTL2, val);
		s5m8752_set_bits(s5m8752, S5M8752_WLED_CNTL1, S5M8752_WLED_EN);
	} else
		s5m8752_clear_bits(s5m8752, S5M8752_WLED_CNTL1,
							S5M8752_WLED_EN);

	data->current_brightness = brightness;

	return ret;
}

/******************************************************************************
 * This function checks backlight status and calls backlight_set function.
 ******************************************************************************/
static int s5m8752_backlight_update_status(struct backlight_device *bl)
{

	int brightness = bl->props.brightness/8;
	
	if (bl->props.power == FB_BLANK_UNBLANK)
		brightness = 0;

	if (bl->props.fb_blank != FB_BLANK_UNBLANK)
		brightness = 0;

	if (bl->props.state & BL_CORE_SUSPENDED)
		brightness = 0;


	return s5m8752_backlight_set(bl, brightness);
}

/******************************************************************************
 * This function gets s5m8752 backlight data.
 ******************************************************************************/
static int s5m8752_backlight_get_brightness(struct backlight_device *bl)
{
	struct s5m8752_backlight_data *data = bl_get_data(bl);
	return data->current_brightness;
}

static const struct backlight_ops s5m8752_backlight_ops = {
	.options = BL_CORE_SUSPENDRESUME,
	.update_status	= s5m8752_backlight_update_status,
	.get_brightness	= s5m8752_backlight_get_brightness,
};

static int s5m8752_backlight_probe(struct platform_device *pdev)
{
	struct s5m8752 *s5m8752 = dev_get_drvdata(pdev->dev.parent);
	struct s5m8752_pdata *s5m8752_pdata;
	struct s5m8752_backlight_pdata *pdata;
	struct s5m8752_backlight_data *data;
	struct backlight_device *bl;
	struct backlight_properties props;

	if (pdev->dev.parent->platform_data) {
		s5m8752_pdata = pdev->dev.parent->platform_data;
		pdata = s5m8752_pdata->backlight;
	} else
		pdata = NULL;

	if (!pdata) {
		dev_err(&pdev->dev, "No platform data supplies\n");
		return -EINVAL;
	}

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (data == NULL)
		return -ENOMEM;

	data->s5m8752 = s5m8752;
	data->current_brightness = 0;

	props.max_brightness = S5M8752_WLED_MAX_BRIGHTNESS;
	bl = backlight_device_register("pwm-backlight.0", &pdev->dev, data,
				&s5m8752_backlight_ops);
				//&s5m8752_backlight_ops, &props);

	if (IS_ERR(bl)) {
		dev_err(&pdev->dev, "failed to register backlight\n");
		kfree(data);
		return PTR_ERR(bl);
	}

	bl->props.brightness = pdata->brightness;
	bl->props.fb_blank = 0;
	bl->props.power = 1;
	bl->props.state = 0;
	bl->props.max_brightness = S5M8752_WLED_MAX_BRIGHTNESS;

	platform_set_drvdata(pdev, bl);
	backlight_update_status(bl);

	return 0;
}

static int __devexit s5m8752_backlight_remove(struct platform_device *pdev)
{
	struct backlight_device *bl = platform_get_drvdata(pdev);
	struct s5m8752_backlight_data *data = bl_get_data(bl);

	backlight_device_unregister(bl);
	kfree(data);
	return 0;
}

static struct platform_driver s5m8752_backlight_driver = {
	.driver		= {
		.name	= "s5m8752-backlight",
		.owner	= THIS_MODULE,
	},
	.probe		= s5m8752_backlight_probe,
	.remove		= s5m8752_backlight_remove,
};

static int __init s5m8752_backlight_init(void)
{
	return platform_driver_register(&s5m8752_backlight_driver);
}
module_init(s5m8752_backlight_init);

static void __exit s5m8752_backlight_exit(void)
{
	platform_driver_unregister(&s5m8752_backlight_driver);
}
module_exit(s5m8752_backlight_exit);

MODULE_DESCRIPTION("S5M8752 backlight driver");
MODULE_AUTHOR("Jongbin, Won <jongbin.won@samsung.com>");
MODULE_LICENSE("GPL");
