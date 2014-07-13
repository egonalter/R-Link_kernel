#include <linux/gpio.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/vgpio.h>
#include <linux/usb_mode_lite.h>
#include <plat/adc.h>
#include <plat/gpio-cfg.h>
#include <plat/mendoza.h>

#define VBUS_PIN		"usb_detect"

#define	ADC_CHAN_USB_ID		3
#define ADC_RESOLUTION		12
#define	uV_PER_ADC_COUNT	(3300000 / (1 << ADC_RESOLUTION))

/* calculated value for 124k (nominal R_idA for ACA) is 1.82V. having a margin
   of ~0.3V in both directions is ok as the other possible values are far away
   (0V for 0k and 3.3V for not grounded */
#define	uV_ACA_MAX		2100000
#define	uV_ACA_MIN		1500000

extern struct s3c_adc_client *client;
extern int s5m8751_set_charger_type(int charger_type);
extern int s3c_change_usb_mode(int mode);

static int is_aca_plugged(void);

static struct usb_mode_lite_platform_data usb_mode_lite_pdata = {
	.gpio_vbus		= TT_VGPIO_USB_HOST_DETECT,
	.set_charger_type	= s5m8751_set_charger_type,
	.switch_otg_mode	= s3c_change_usb_mode,
	.is_aca_plugged		= is_aca_plugged
};

static struct platform_device usb_mode_lite_pdev = {
	.name		= "usb_mode_lite",
	.dev            = {
		.platform_data = &usb_mode_lite_pdata
	},
};

static int is_aca_plugged(void)
{
	int rc = 0;
	const int adc_val_raw = s3c_adc_read(client, ADC_CHAN_USB_ID);
	const int adc_val_uV = adc_val_raw * uV_PER_ADC_COUNT;

	if ((adc_val_uV >= uV_ACA_MIN) &&
	    (adc_val_uV <= uV_ACA_MAX))
		rc = 1;

	return rc;
}

static int __init usb_mode_lite_init(void)
{
	int err;
	const int gpio_pin = TT_VGPIO_USB_HOST_DETECT;

	err = gpio_request(gpio_pin, VBUS_PIN);
	if (err) {
		printk(KERN_ERR "Failed to request '%s' GPIO\n", VBUS_PIN);
		goto err_gpio_req;
	}

	err = gpio_direction_input(gpio_pin);
	if (err) {
		printk(KERN_ERR "Failed to configure '%s' GPIO as input\n", VBUS_PIN);
		goto err_gpio_dir;
	}

	s3c_gpio_setpull(vgpio_to_gpio(gpio_pin), S3C_GPIO_PULL_NONE);

	return platform_device_register(&usb_mode_lite_pdev);

  err_gpio_dir:
	gpio_free(gpio_pin);

  err_gpio_req:

	return err;
}

device_initcall(usb_mode_lite_init);
