/* linux/drivers/input/touchscreen/s3c-ts.c
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 * Copyright (c) 2004 Arnaud Patard <arnaud.patard@rtp-net.org>
 * iPAQ H1940 touchscreen support
 *
 * ChangeLog
 *
 * 2004-09-05: Herbert Potzl <herbert@13thfloor.at>
 *	- added clock (de-)allocation code
 *
 * 2005-03-06: Arnaud Patard <arnaud.patard@rtp-net.org>
 *      - h1940_ -> s3c24xx (this driver is now also used on the n30
 *        machines :P)
 *      - Debug messages are now enabled with the config option
 *        TOUCHSCREEN_S3C_DEBUG
 *      - Changed the way the value are read
 *      - Input subsystem should now work
 *      - Use ioremap and readl/writel
 *
 * 2005-03-23: Arnaud Patard <arnaud.patard@rtp-net.org>
 *      - Make use of some undocumented features of the touchscreen
 *        controller
 *
 * 2006-09-05: Ryu Euiyoul <ryu.real@gmail.com>
 *      - added power management suspend and resume code
 *
 */

#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/serio.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#ifdef CONFIG_HAS_WAKELOCK
#include <linux/wakelock.h>
#include <linux/earlysuspend.h>
#include <linux/suspend.h>
#endif
#include <asm/io.h>
#include <asm/irq.h>
#include <mach/hardware.h>
#include <plat/regs-adc.h>
#include <plat/ts.h>
#include <mach/irqs.h>
#include <plat/adc.h>

#if CONFIG_CPU_FREQ
//#include <plat/s5pc11x-dvfs.h>
#endif

#ifdef CONFIG_TOMTOM_FDT
#include <plat/fdt.h>
#endif

#define CONFIG_TOUCHSCREEN_S3C_DEBUG
#undef CONFIG_TOUCHSCREEN_S3C_DEBUG
#define CONFIG_CPU_S5P6440_EVT1
#ifdef CONFIG_MACH_S5P6450_TOMTOM
#undef CONFIG_TOUCHSCREEN_NEW  //no swap, maybe smdk is swapped?
#undef CONFIG_CPU_S5P6440_EVT1 //use full 12 bit range
#endif

#define TS_ANDROID_PM_ON 1
#define USE_PERF_LEVEL_TS 1 
/* For ts->dev.id.version */
#define S3C_TSVERSION	0x0101

/* Just in case the fdt fails to return values */
#if defined(CONFIG_CPU_S5P6440_EVT0)
#define XMIN 400 
#define XMAX 3700
#define YMIN 870 
#define YMAX 3230
#elif defined(CONFIG_CPU_S5P6440_EVT1)
#define XMIN 85 
#define XMAX 3970
#define YMIN 280 
#define YMAX 3850
#else
#define XMIN 0
#define XMAX 0xFFF
#define YMIN 0 
#define YMAX 0xFFF               
#endif

#define WAIT4INT(x)  (((x)<<8) | \
		     S3C_ADCTSC_YM_SEN | S3C_ADCTSC_YP_SEN | S3C_ADCTSC_XP_SEN | \
		     S3C_ADCTSC_XY_PST(3))

#define AUTOPST	     (S3C_ADCTSC_YM_SEN | S3C_ADCTSC_YP_SEN | S3C_ADCTSC_XP_SEN | \
		     S3C_ADCTSC_AUTO_PST | S3C_ADCTSC_XY_PST(0))

#define TS_POLL_TIME	HZ/40
#define TS_PEN_UP	1
#define TS_PEN_DOWN	0

#define DEBUG_LVL    KERN_DEBUG

#ifdef CONFIG_HAS_WAKELOCK
	void ts_early_suspend(struct early_suspend *h);
	void ts_late_resume(struct early_suspend *h);
#endif

/* Touchscreen default configuration */
struct s3c_ts_mach_info s3c_ts_default_cfg __initdata = {
	.oversampling_shift =	2,
	.resol_bit = 		10
};

#ifdef CONFIG_HAS_WAKELOCK
	extern suspend_state_t get_suspend_state(void);
#endif

/*
 * Definitions & global arrays.
 */
static char *s3c_ts_name = "S3C TouchScreen";
static struct s3c_ts_info 	*ts;
static struct s3c_adc_client 	*ts_adc_client;

static void ts_poll(unsigned long data)
{
	/* We were called from the updown irq or FINISHED task */
	s3c_adc_start (ts_adc_client, 0, 1<<ts->shift);
}

static struct timer_list touch_timer = TIMER_INITIALIZER(ts_poll, 0, 0);

static void ts_select (struct s3c_adc_client *c, unsigned int selected)
{
	switch (selected) {
		case INIT:
			s3c_adc_set_ts_control(ts_adc_client, S3C_ADCTSC, WAIT4INT(1));
			return;
		break;
		case SELECTED:
	                s3c_adc_set_ts_control(ts_adc_client, S3C_ADCTSC, 
					       S3C_ADCTSC_PULL_UP_DISABLE | AUTOPST);
			return;
		break;
		case FINISHED:
	                s3c_adc_set_ts_control(ts_adc_client , S3C_ADCTSC, WAIT4INT(1));

               	        mod_timer(&touch_timer, jiffies+TS_POLL_TIME);
		break;
		default:
			printk(KERN_ERR "s3c-ts.c Unsupported value in ts_select!\n");
			return;
		break;
	}

	if(ts->pressure){
		if(ts->count > 4) {
			ts->xp = ts->xp / ts->count;
			ts->yp = ts->yp / ts->count;
			if (ts->xp != ts->xp_old || ts->yp != ts->yp_old) {
				input_report_abs(ts->dev, ABS_X, ts->xp);
				input_report_abs(ts->dev, ABS_Y, ts->yp);
		
				input_report_key(ts->dev, BTN_TOUCH, 1);
				input_report_abs(ts->dev, ABS_PRESSURE, 1);
				input_sync(ts->dev);
			}
			ts->xp_old = ts->xp;
			ts->yp_old = ts->yp;
			ts->xp = 0;
			ts->yp = 0; 
			ts->count = 0;
		}
	} else {
		ts->xp = 0;
		ts->yp = 0;
		ts->count = 0;
	}
}


static void ts_convert (struct s3c_adc_client *c, unsigned data0, unsigned data1, int samples_left)
{
	ts->pressure  = (!(data0 & S3C_ADCDAT0_UPDOWN)  && (!(data1 & S3C_ADCDAT1_UPDOWN)));
	
	if (ts->pressure) { /* Down */
		ts->count++;
		ts->xp += S3C_ADCDAT0_XPDATA_MASK_12BIT - (data0 & S3C_ADCDAT0_XPDATA_MASK_12BIT);
		ts->yp += S3C_ADCDAT1_YPDATA_MASK_12BIT - (data1 & S3C_ADCDAT1_YPDATA_MASK_12BIT);
	} 
}

static irqreturn_t stylus_updown(int irqno, void *param)
{
	unsigned long val, up;
	unsigned long flags;

	local_irq_save(flags);

	val = s3c_adc_get_ts_control(ts_adc_client, S3C_ADCTSC);
	up = (val & 0x100);

	if (up) {
		del_timer_sync (&touch_timer);

		s3c_adc_set_ts_control (ts_adc_client, S3C_ADCTSC, WAIT4INT(0));

		input_report_abs(ts->dev, ABS_X, ts->xp_old);
		input_report_abs(ts->dev, ABS_Y, ts->yp_old);
		input_report_abs(ts->dev, ABS_Z, 0);

		input_report_key(ts->dev, BTN_TOUCH, 0);
		input_report_abs(ts->dev, ABS_PRESSURE, 0);
		input_sync(ts->dev);
		ts->xp_old =0;
		ts->yp_old =0;
		ts->xp = 0;
		ts->yp = 0; 
		ts->count = 0;
	} else {
		mod_timer (&touch_timer, jiffies+1);
	}
	/*ack interrupt*/
	s3c_adc_set_ts_control (ts_adc_client, S3C_ADCCLRWK, 0x0);
	local_irq_restore(flags);

	return IRQ_HANDLED;
}

static struct s3c_ts_mach_info *s3c_ts_get_platdata (struct device *dev)
{
	if (dev->platform_data != NULL)
		return (struct s3c_ts_mach_info *)dev->platform_data;

	return &s3c_ts_default_cfg;
}
/* for universal */
#define TOUCH_SCREEN1 0x1
#define TSSEL 17
/*
 * The functions for inserting/removing us as a module.
 */
static int __init s3c_ts_probe(struct platform_device *pdev)
{
	struct device *dev;
	struct input_dev *input_dev;
	struct s3c_ts_mach_info * s3c_ts_cfg;
	int ret;
	int xmin, xmax, ymin, ymax;

	dev = &pdev->dev;

	s3c_ts_cfg = s3c_ts_get_platdata(&pdev->dev);

	ts = kzalloc(sizeof(struct s3c_ts_info), GFP_KERNEL);
	if (!ts) {
		ret = -ENOMEM;
		goto err_alloc_ts;
	}
	
	input_dev = input_allocate_device();

	if (!input_dev) {
		ret = -ENOMEM;
		goto err_alloc_input_dev;
	}
	
	ts->dev = input_dev;

	ts->dev->evbit[0] = ts->dev->evbit[0] = BIT_MASK(EV_SYN) | BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	ts->dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);

	/* Get XMIN/MAX and YMIN/MAX from fdt */
	xmin = (int)fdt_get_ulong ("/options/touchscreen", "xmin", XMIN);
	xmax = (int)fdt_get_ulong ("/options/touchscreen", "xmax", XMAX);
	ymin = (int)fdt_get_ulong ("/options/touchscreen", "ymin", YMIN);
	ymax = (int)fdt_get_ulong ("/options/touchscreen", "ymax", YMAX);
	
	if (s3c_ts_cfg->resol_bit==12) {
		xmax &= 0xFFF;
		ymax &= 0xFFF;
	} else {
		xmax &= 0x3FF;
		ymax &= 0x3FF;
	}
	
	input_set_abs_params(ts->dev, ABS_X, xmin, xmax, 64, 0);
	input_set_abs_params(ts->dev, ABS_Y, ymin, ymax, 64, 0);

	input_set_abs_params(ts->dev, ABS_PRESSURE, 0, 1, 0, 0);
	//input_set_abs_params(ts->dev, ABS_TOOL_WIDTH, 0, 15, 0, 0);

	sprintf(ts->phys, "input(ts)");

	ts->dev->name = s3c_ts_name;
	ts->dev->phys = ts->phys;
	ts->dev->id.bustype = BUS_RS232;
	ts->dev->id.vendor = 0xDEAD;
	ts->dev->id.product = 0xBEEF;
	ts->dev->id.version = S3C_TSVERSION;

	ts->shift = s3c_ts_cfg->oversampling_shift;
	ts->resol_bit = s3c_ts_cfg->resol_bit;

#if TS_ANDROID_PM_ON
#ifdef CONFIG_HAS_WAKELOCK	
#ifdef CONFIG_EARLYSUSPEND
	ts->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	ts->early_suspend.suspend = ts_early_suspend;
	ts->early_suspend.resume =ts_late_resume;
	register_early_suspend(&ts->early_suspend);
	/* If, is in USER_SLEEP status and no active auto expiring wake lock */ 
	if (has_wake_lock(WAKE_LOCK_SUSPEND) == 0 && get_suspend_state() == PM_SUSPEND_ON) {
		//printk("s3c-ts.c: touch screen early suspended by APM.\n");
		ts_early_suspend(&ts->early_suspend);
	}
#endif //CONFIG_EARLYSUSPEND
#endif //CONFIG_HAS_WAKELOCK
#endif //TS_ANDROID_PM_ON

	/* For IRQ_PENDOWN/UP */
	ts->irq = platform_get_irq(pdev,0);
	if (ts->irq <= 0) {
		dev_err(dev, "no irq resource specified\n");
		ret = -ENOENT;
		goto err_get_irq;
	}

	ret = request_irq(ts->irq, stylus_updown, IRQF_SAMPLE_RANDOM, "s3c_updown", ts);
	if (ret != 0) {
		dev_err(dev,"s3c_ts.c: Could not allocate ts IRQ_PENDN !\n");
		ret = -EIO;
		goto err_get_irq;
	}

	/* Register the ADC client */
	ts_adc_client = s3c_adc_register (pdev, ts_select, ts_convert, 1);
	if (ts_adc_client == NULL) {
		printk (KERN_ERR "s3c_ts.c: Unable to get adc client\n");
		ret = -ENOENT;
		goto err_adc_register;
	}
	printk(KERN_INFO "%s got loaded successfully : %d bits\n", s3c_ts_name, s3c_ts_cfg->resol_bit);

	/* All went ok, so register to the input system */
	ret = input_register_device(ts->dev);
	
	if(ret) {
		dev_err(dev, "s3c_ts.c: Could not register input device(touchscreen)!\n");
		ret = -EIO;
		goto err_input_register;
	}
	
	/* Now that everything is properly registered, setup the ADCTSC to get the pen up/down intr */
	s3c_adc_set_ts_control (ts_adc_client, S3C_ADCTSC, WAIT4INT(0));

	return 0;

err_input_register:
	s3c_adc_release(ts_adc_client);

err_adc_register:
	free_irq(ts->irq, ts->dev);

err_get_irq:
	input_free_device(input_dev);
	
err_alloc_input_dev:
	kfree(ts);

err_alloc_ts:
	return ret;
}

static int s3c_ts_remove(struct platform_device *dev)
{
	printk(KERN_INFO "s3c_ts_remove() of TS called !\n");

	disable_irq(ts->irq);
	
	del_timer_sync (&touch_timer);
	free_irq(ts->irq, ts);
	s3c_adc_release (ts_adc_client);

	input_unregister_device(ts->dev);

#if TS_ANDROID_PM_ON
#ifdef CONFIG_HAS_WAKELOCK
     unregister_early_suspend(&ts->early_suspend);
#endif
#endif //TS_ANDROID_PM_ON

	return 0;
}

#ifdef CONFIG_PM
static int s3c_ts_suspend(struct platform_device *dev, pm_message_t state)
{
	disable_irq(ts->irq);
	del_timer_sync (&touch_timer);

	/* Leave the input driver in a known state */
	input_report_key(ts->dev, BTN_TOUCH, 0);
	input_report_abs(ts->dev, ABS_PRESSURE, 0);
	input_sync(ts->dev);

	ts->xp = 0;
	ts->yp = 0;

	return 0;
}

static int s3c_ts_resume(struct platform_device *pdev)
{
	s3c_adc_set_ts_control (ts_adc_client, S3C_ADCTSC, WAIT4INT(0));
	enable_irq(ts->irq);
	
	return 0;
}
#else
#define s3c_ts_suspend NULL
#define s3c_ts_resume  NULL
#endif

#if TS_ANDROID_PM_ON
#ifdef CONFIG_HAS_WAKELOCK
void ts_early_suspend(struct early_suspend *h)
{
	struct s3c_ts_info *ts;
	ts = container_of(h, struct s3c_ts_info, early_suspend);
	s3c_ts_suspend(NULL, PMSG_SUSPEND); // platform_device is now used
}

void ts_late_resume(struct early_suspend *h)
{
	struct s3c_ts_info *ts;
	ts = container_of(h, struct s3c_ts_info, early_suspend);
	s3c_ts_resume(NULL); // platform_device is now used
}
#endif
#endif //TS_ANDROID_PM_ON

static struct platform_driver s3c_ts_driver = {
       .probe          = s3c_ts_probe,
       .remove         = s3c_ts_remove,
       .suspend        = s3c_ts_suspend,
       .resume         = s3c_ts_resume,
       .driver		= {
		.owner	= THIS_MODULE,
		.name	= "s3c-ts",
	},
};

static char banner[] __initdata = KERN_INFO "S3C Touchscreen driver, (c) 2008 Samsung Electronics\n";

static int __init s3c_ts_init(void)
{
	printk(banner);
	return platform_driver_register(&s3c_ts_driver);
}

static void __exit s3c_ts_exit(void)
{
	platform_driver_unregister(&s3c_ts_driver);
}

module_init(s3c_ts_init);
module_exit(s3c_ts_exit);

MODULE_AUTHOR("Samsung AP");
MODULE_DESCRIPTION("S3C touchscreen driver");
MODULE_LICENSE("GPL");
