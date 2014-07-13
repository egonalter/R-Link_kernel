#include <linux/init.h>
#include <linux/suspend.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/input.h>
#include <mach/hardware.h>
#include <plat/map-base.h>

#define DEVICE_NAME "s3c-keypad"
#define KEYCODE_UNKNOWN  KEY_POWER

struct	s3c_keypad {
	struct	input_dev *dev;
};

static struct	s3c_keypad *ps3c_keypad;

int keypad_keycode[] = { 50, };

static int __init s3c_button_probe(struct platform_device *pdev)
{
	struct	input_dev *input_dev;
	int	ret = 0;
	
	ps3c_keypad = kzalloc(sizeof(struct s3c_keypad), GFP_KERNEL );
	input_dev = input_allocate_device();
	
	if (!input_dev) {
		ret = -ENOMEM;
		printk ( "input device allocate error\n");
		return ret;
	}

	set_bit(EV_KEY, input_dev->evbit);
	set_bit(KEYCODE_UNKNOWN & KEY_MAX, input_dev->keybit);

	input_dev->name = DEVICE_NAME;
	input_dev->phys = "s3c-keypad/input0";
	
	input_dev->id.bustype = BUS_HOST;
	input_dev->id.vendor = 0x0001;
	input_dev->id.product = 0x0001;
	input_dev->id.version = 0x0001;

	input_dev->keycode = keypad_keycode;

	ret = input_register_device(input_dev);
	if (ret) {
		printk("Unable to register s3c-keypad input device!!!\n");
		goto out;
	}

	ps3c_keypad->dev = input_dev;

	return 0;
out:
	input_free_device(input_dev);
	kfree(ps3c_keypad);

	return ret;
}

#ifdef CONFIG_PM
#include <plat/pm.h>

static int s3c_button_suspend(struct platform_device *dev, pm_message_t state)
{
        return 0;
}

static int s3c_button_resume(struct platform_device *dev)
{
        struct input_dev *iDev = ps3c_keypad->dev;

        input_report_key(iDev, KEYCODE_UNKNOWN, 1);
        udelay(5);
        input_report_key(iDev, KEYCODE_UNKNOWN, 0);

        return 0i;
}

#else
#define s3c_button_suspend NULL
#define s3c_button_resume  NULL
#endif /* CONFIG_PM */

static struct platform_driver s3c_button_driver = {
        .probe          = s3c_button_probe,
        .suspend        = s3c_button_suspend,
        .resume         = s3c_button_resume,
        .driver         = {
                .owner  = THIS_MODULE,
                .name   = "s3c-button",
        },
};

static int __init s3c_button_init(void)
{
        int ret;
	
	printk("Dummy button init function \n");

        ret = platform_driver_register(&s3c_button_driver);

        if(!ret)
        	printk( "S3C button Driver\n");

	return ret;

}

device_initcall(s3c_button_init);
