#include <linux/init.h>
#include <linux/suspend.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/crc32.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/serial_core.h>
#include <linux/io.h>
#include <linux/platform_device.h>

#include <linux/input.h>

#include <asm/cacheflush.h>
#include <mach/hardware.h>

#include <plat/map-base.h>
#include <plat/regs-serial.h>
#include <mach/regs-gpio.h>
#include <plat/gpio-cfg.h>

#include <mach/regs-mem.h>
#include <mach/regs-irq.h>
#include <asm/gpio.h>
#include <plat/gpio-bank-n.h>
#define		MAX_KEYPAD_NR		64

#define 	DEVICE_NAME "s3c-keypad"

#define KEYCODE_UNKNOWN  10

struct	s3c_keypad {
	struct	input_dev *dev;
	int		nr_rows;
	int		no_cols;
	int		total_keys;
	int		keycodes[MAX_KEYPAD_NR];
};

static struct	s3c_keypad *ps3c_keypad;

static irqreturn_t
s3c_button_interrupt_eint9(int irq, void *dev_id)
{
	struct	input_dev *dev = ps3c_keypad->dev;
	input_report_key(dev,26,1);   //sleep
	mdelay(10);
	input_report_key(dev,26,0);
	printk ( "key 26 reported\n");
//		input_sync(dev);

	return IRQ_HANDLED;

}
static irqreturn_t
s3c_button_interrupt(int irq, void *dev_id)
{

	struct	input_dev *dev = ps3c_keypad->dev;

	printk("Button Interrupt %d occured\n", irq);
	if( irq == IRQ_EINT(11))
	{
		input_report_key(dev,34,1);  //back key
		printk ( "key 34 reported\n");
		mdelay(10);
		input_report_key(dev,34,0);
//		input_sync(dev);


	}
	if( irq == IRQ_EINT(10))
	{
		input_report_key(dev,50,1); //menu
		printk ( "key 50 reported\n");
		mdelay(10);
		input_report_key(dev,50,0);
//		input_sync(dev);

	}
	
	
	return IRQ_HANDLED;
}

static struct irqaction s3c_button_irq = {
	.name		= "s3c button Tick",
	.flags		= 0,
	.handler	= s3c_button_interrupt,
};

static struct irqaction s3c_button_irq_eint9 = {
	.name		= "s3c button Tick",
	.flags		= 0 ,
	.handler	= s3c_button_interrupt_eint9,
};
#ifdef CONFIG_ANDROID
int keypad_keycode[] = {
		1,2,3,4,5,6,7,8,
		9,10,11,12,13,14,15,16,
		17,18,19,20,21,22,23,24,
		25,26,27,28,29,30,31,32,
		33,34,35,36,37,38,39,40,
		41,42,43,44,45,46,47,48,
		49,50,51,52,53,54,55,56,
		57,58,59,60,61,62,63,64

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


static unsigned int s3c_button_gpio_init(void)
{ 
	printk("\n %s",__func__);
	u32 err;

	err = gpio_request(S5P6440_GPN(9),"GPN");
	if (err){
		printk("gpio request error : %d\n",err);
	}else{
		s3c_gpio_cfgpin(S5P6440_GPN(9), S5P64XX_GPN9_EINT9);
		s3c_gpio_setpull(S5P6440_GPN(9), S3C_GPIO_PULL_NONE);
	}

	err = gpio_request(S5P6440_GPN(10),"GPN");
	if (err){
		printk("gpio request error : %d\n",err);
	}else{
		s3c_gpio_cfgpin(S5P6440_GPN(10), S5P64XX_GPN10_EINT10);
		s3c_gpio_setpull(S5P6440_GPN(10), S3C_GPIO_PULL_NONE);
	}

	err = gpio_request(S5P6440_GPN(11),"GPN");
	if (err){
		printk("gpio request error : %d\n",err);
	}else{
		s3c_gpio_cfgpin(S5P6440_GPN(11), S5P64XX_GPN11_EINT11);
		s3c_gpio_setpull(S5P6440_GPN(11), S3C_GPIO_PULL_NONE);
	}

	return err;
}

static int __init s3c_button_probe(struct platform_device *pdev)
{

	u32 tmp;

	printk("\n %s::line =%d",__func__,__LINE__);
	struct	input_dev *input_dev;

	int		ret, size;
	int		key, code;
	
	printk("SMDK6440 Button init function \n");

	if (s3c_button_gpio_init()) {
		printk(KERN_ERR "%s failed\n", __FUNCTION__);
		return;
	}	
	
	printk("\n %s::line =%d",__func__,__LINE__);
	ps3c_keypad = kzalloc(sizeof(struct s3c_keypad), GFP_KERNEL );
	input_dev = input_allocate_device();
	
	if ( !input_dev) {
//		ret = -ENOMEM;
		printk ( "input device allocate error\n");
	}

	printk("\n %s::line =%d",__func__,__LINE__);
	set_bit(EV_KEY, input_dev->evbit);

	ps3c_keypad->nr_rows = /* KEYPAD_ROWS */ 8;
	ps3c_keypad->no_cols = /* KEYPAD_COLUMNS */ 8;
	ps3c_keypad->total_keys = MAX_KEYPAD_NR;

	for(key = 0; key < ps3c_keypad->total_keys; key++){
		code = ps3c_keypad->keycodes[key] = keypad_keycode[key];
		if(code<=0)
			continue;
		set_bit(code & KEY_MAX, input_dev->keybit);
	}
	printk("\n %s::line =%d",__func__,__LINE__);

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

	/* For IRQ_KEYPAD */
/*
	keypad_irq = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	if (keypad_irq == NULL) {
		dev_err(&pdev->dev, "no irq resource specified\n");
		ret = -ENOENT;
		goto err_irq;
	}
	ret = request_irq(keypad_irq->start, s3c_keypad_isr, IRQF_SAMPLE_RANDOM,
		DEVICE_NAME, (void *) pdev);
	if (ret) {
		printk("request_irq failed (IRQ_KEYPAD) !!!\n");
		ret = -EIO;
		goto err_irq;
	}
*/
	printk("\n %s::line =%d",__func__,__LINE__);
	ret = input_register_device(input_dev);
	if (ret) {
		printk("Unable to register s3c-keypad input device!!!\n");
//		goto out;
	}


	printk("\n %s::line =%d",__func__,__LINE__);
//	platform_set_drvdata(pdev, s3c_keypad);
	ps3c_keypad->dev = input_dev;
	printk("\n %s::line =%d",__func__,__LINE__);

	set_irq_type(IRQ_EINT(9), IRQF_TRIGGER_FALLING);
        set_irq_wake(IRQ_EINT(9), 1);
	setup_irq(IRQ_EINT(9), &s3c_button_irq_eint9);	
	printk("\n %s::line =%d",__func__,__LINE__);
	set_irq_type(IRQ_EINT(10), IRQF_TRIGGER_FALLING);
        set_irq_wake(IRQ_EINT(10), 1);
	setup_irq(IRQ_EINT(10), &s3c_button_irq);	
	printk("\n %s::line =%d",__func__,__LINE__);
	set_irq_type(IRQ_EINT(11), IRQF_TRIGGER_FALLING);
        set_irq_wake(IRQ_EINT(11), 1);
	setup_irq(IRQ_EINT(11), &s3c_button_irq);
	printk("\n %s::line =%d",__func__,__LINE__);	
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

        printk("++++ %s\n", __FUNCTION__ );

        input_report_key(iDev, KEYCODE_UNKNOWN, 1);
        udelay(5);
        input_report_key(iDev, KEYCODE_UNKNOWN, 0);

        enable_irq(IRQ_EINT(9));
        enable_irq(IRQ_EINT(10));
        enable_irq(IRQ_EINT(11));

        printk("---- %s\n", __FUNCTION__ );
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



static void __init s3c_button_init(void)
{
	u32 tmp;
        int ret;
	
	printk("SMDKC110 Button init function \n");

        ret = platform_driver_register(&s3c_button_driver);

        if(!ret)
           printk( "S3C button Driver\n");

}



device_initcall(s3c_button_init);
