#include <linux/init.h>
#include <linux/suspend.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/serial_core.h>
#include <linux/io.h>
#include <linux/platform_device.h>

#include <linux/input.h>

#include <mach/hardware.h>

#include <plat/map-base.h>
#include <mach/regs-gpio.h>
#include <plat/gpio-cfg.h>

#include <mach/regs-mem.h>
#include <mach/regs-irq.h>
#include <asm/gpio.h>
#include <plat/gpio-bank-n.h>
#define		MAX_KEYPAD_NR		36

#define 	DEVICE_NAME "s3c-keypad"

#define KEYCODE_UNKNOWN  50

struct	s3c_keypad {
	struct	input_dev *dev;
	int		nr_rows;
	int		no_cols;
	int		total_keys;
	int		keycodes[MAX_KEYPAD_NR];
};

static struct	s3c_keypad *ps3c_keypad;

#ifdef CONFIG_ANDROID
int keypad_keycode[] = {
		0,	0,	0,	0,	0, 
		0,	0,	0,	0,	26,
		50,	34,	0,	0,	0,

};
#else
int keypad_keycode[] = {
		1, 2, KEY_1, KEY_Q, KEY_A, 6, 7, KEY_LEFT,
		9, 10, KEY_2, KEY_W, KEY_S, KEY_Z, KEY_RIGHT, 16,
		17, 18, KEY_3, KEY_E, KEY_D, KEY_X, 23, KEY_UP,
		25, 26, KEY_4, KEY_R, KEY_F, KEY_C, 31, 32,
		33, KEY_O, KEY_5, KEY_T, KEY_G, KEY_V, KEY_DOWN, KEY_BACKSPACE,
		KEY_P, KEY_0, KEY_6, KEY_Y, KEY_H, KEY_SPACE, 47, 48,
		KEY_M, KEY_L, KEY_7, KEY_U, KEY_J, KEY_N, 55, KEY_ENTER,
		KEY_LEFTSHIFT, KEY_9, KEY_8, KEY_I, KEY_K, KEY_B, 63, KEY_COMMA
	};
#endif




static irqreturn_t
s3c_button_interrupt(int irq, void *dev_id)
{

	struct	input_dev *dev = ps3c_keypad->dev;
	int key;
	uint detect;

	detect = readl(S5P64XX_GPNDAT);


	key = irq - IRQ_EINT(0);

	//printk("Button key %d code %d  %s\n", key,keypad_keycode[key],(detect & (1<< key))?"release":"press");

	if(detect & (1 << key))
		input_report_key(dev,keypad_keycode[key],0); 
	else
		input_report_key(dev,keypad_keycode[key],1); 
	
	//mdelay(10);
	
	return IRQ_HANDLED;
}

static struct irqaction s3c_button_irq = {
	.name		= "s3c button Tick",
	.flags		= 0,
	.handler	= s3c_button_interrupt,
};


static int s3c_button_gpio_init(void)
{ 
	int  err;

	err = gpio_request(S5P6450_GPN(9),"GPN");
	if (err){
		printk("gpio request error : %d\n",err);
	}else{
		s3c_gpio_cfgpin(S5P6450_GPN(9), S5P64XX_GPN9_EINT9);
		s3c_gpio_setpull(S5P6450_GPN(9), S3C_GPIO_PULL_NONE);
	}

	err = gpio_request(S5P6450_GPN(10),"GPN");
	if (err){
		printk("gpio request error : %d\n",err);
	}else{
		s3c_gpio_cfgpin(S5P6450_GPN(10), S5P64XX_GPN10_EINT10);
		s3c_gpio_setpull(S5P6450_GPN(10), S3C_GPIO_PULL_NONE);
	}

	err = gpio_request(S5P6450_GPN(11),"GPN");
	if (err){
		printk("gpio request error : %d\n",err);
	}else{
		s3c_gpio_cfgpin(S5P6450_GPN(11), S5P64XX_GPN11_EINT11);
		s3c_gpio_setpull(S5P6450_GPN(11), S3C_GPIO_PULL_NONE);
	}

	return err;
}

static int __init s3c_button_probe(struct platform_device *pdev)
{

	struct	input_dev *input_dev;

	int		ret = 0,key,code;
	
	printk("SMDK6450 Button init function \n");

	if ((ret = s3c_button_gpio_init())) {
		printk(KERN_ERR "%s failed\n", __FUNCTION__);
		return ret;
	}	
	
	ps3c_keypad = kzalloc(sizeof(struct s3c_keypad), GFP_KERNEL );
	input_dev = input_allocate_device();
	
	if ( !input_dev) {
		ret = -ENOMEM;
		printk ( "input device allocate error\n");
		return ret;
	}

	set_bit(EV_KEY, input_dev->evbit);

	ps3c_keypad->nr_rows = /* KEYPAD_ROWS */ 6;
	ps3c_keypad->no_cols = /* KEYPAD_COLUMNS */ 6;
	ps3c_keypad->total_keys = MAX_KEYPAD_NR;

	for(key = 0; key < ps3c_keypad->total_keys; key++){
		code = ps3c_keypad->keycodes[key] = keypad_keycode[key];
		if(code<=0)
			continue;
		set_bit(code & KEY_MAX, input_dev->keybit);
	}

	input_dev->name = DEVICE_NAME;
	input_dev->phys = "s3c-keypad/input0";
	
	input_dev->id.bustype = BUS_HOST;
	input_dev->id.vendor = 0x0001;
	input_dev->id.product = 0x0001;
	input_dev->id.version = 0x0001;

	input_dev->keycode = keypad_keycode;

	/* Scan timer init */
//	init_timer(&keypad_timer);
//	keypad_timer.function = keypad_timer_handler;
//	keypad_timer.data = (unsigned long)s3c_keypad;



	ret = input_register_device(input_dev);
	if (ret) {
		printk("Unable to register s3c-keypad input device!!!\n");
		goto out;
	}

//	platform_set_drvdata(pdev, s3c_keypad);
	ps3c_keypad->dev = input_dev;

	set_irq_type(IRQ_EINT(9),IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING );
        set_irq_wake(IRQ_EINT(9), 1);
	setup_irq(IRQ_EINT(9), &s3c_button_irq);	

	set_irq_type(IRQ_EINT(10),IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING);
        set_irq_wake(IRQ_EINT(10), 1);
	setup_irq(IRQ_EINT(10), &s3c_button_irq);	

	set_irq_type(IRQ_EINT(11), IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING);
        set_irq_wake(IRQ_EINT(11), 1);
	setup_irq(IRQ_EINT(11), &s3c_button_irq);

	return ret;
out:
	input_free_device(input_dev);
	kfree(ps3c_keypad);

	return ret;
}

#ifdef CONFIG_PM
#include <plat/pm.h>


static int s3c_button_suspend(struct platform_device *dev, pm_message_t state)
{

        disable_irq(IRQ_EINT(9));
        disable_irq(IRQ_EINT(10));
        disable_irq(IRQ_EINT(11));

        return 0;
}

static int s3c_button_resume(struct platform_device *dev)
{
        struct input_dev           *iDev = ps3c_keypad->dev;


        input_report_key(iDev, KEYCODE_UNKNOWN, 1);
        udelay(5);
        input_report_key(iDev, KEYCODE_UNKNOWN, 0);

        enable_irq(IRQ_EINT(9));
        enable_irq(IRQ_EINT(10));
        enable_irq(IRQ_EINT(11));

        return 0;
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
	
	printk("SMDKC110 Button init function \n");

        ret = platform_driver_register(&s3c_button_driver);

        if(!ret)
        	printk( "S3C button Driver\n");

	return ret;

}



device_initcall(s3c_button_init);
