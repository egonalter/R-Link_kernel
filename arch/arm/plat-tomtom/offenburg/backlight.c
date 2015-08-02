#include <linux/init.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/completion.h>
#include <linux/device.h>
#include <linux/pwm_backlight.h>
#include <linux/interrupt.h>
#include <linux/leds.h>
#include <linux/clk.h>
#include <linux/i2c/twl.h>
#include <linux/platform_device.h>

#include <asm/mach-types.h>
#include <mach/strasbourg_ver.h>

#include <plat/dmtimer.h>
#include <plat/offenburg.h>

#define DM_TIMER_LOAD_MIN	0xFFFFFFFD
#define STRASBOURG_TIMER_ID     	10
#define STRASBOURG_PWM_PERIOD_NS	8000000
#define STRASBOURG_MAX_PWM_DUTY	75	/* 75% of full duty */
#define RENNES_B1_MAX_PWM_DUTY	((76 * STRASBOURG_MAX_PWM_DUTY) / 100)
#define TWL_LED_LEDEN		0
#define TWL_PWMA_PWMAON		0
#define TWL_PWMA_PWMAOFF	1
#define TWL_PWM0_PWM0ON		0
#define TWL_PWM0_PWM0OFF	1
#define TWL4030_REG_GPBR1	0x0C	/* Offset is 0x0C from the base, which is TWL4030_BASEADD_INTBR = 0x85 */
#define TWL4030_REG_VIBRA_CTL	0x45
#define TWL4030_REG_PMBR1	0x0D	/* Offset is 0x0D from the same base as GPBR1 */
#define TWL_MAX_PWMxOFF		0x7F
#define TWL_MIN_PWMxON		0x01

#define COUNTER_TO_MATCH_GUARD  80
#define TIMER_INT_FLAGS  (OMAP_TIMER_INT_MATCH | OMAP_TIMER_INT_OVERFLOW)

struct offenburg_backlight_pdata {
	struct led_classdev	cdev;
	void			(*brightness_set)(struct led_classdev *, enum led_brightness brightness);
};

struct omap_pwm_device {
	struct omap_dm_timer *dm_timer;
	bool enabled;
	unsigned int timer_id;
	atomic_t cached_match_val;
	struct completion irq_done;

	struct work_struct timer_match_work;

	/* suspend/resume state */
	u32 timer_ctrl_reg;
	u32 timer_match_reg;
};  

/* Refer to HESASW-385 for table detail  */
static int stuttgart_brightness_table[] = {
	0  , /* 0  % */
	1  , /* 1  % */
	1  , /* 2  % */
	3  , /* 3  % */
	3  , /* 4  % */
	3  , /* 5  % */
	4  , /* 6  % */
	4  , /* 7  % */
	4  , /* 8  % */
	6  , /* 9  % */
	6  , /* 10 % */
	6  , /* 11 % */
	7  , /* 12 % */
	7  , /* 13 % */
	7  , /* 14 % */
	8  , /* 15 % */
	8  , /* 16 % */
	8  , /* 17 % */
	8  , /* 18 % */
	8  , /* 19 % */
	8  , /* 20 % */
	9  , /* 21 % */
	9  , /* 22 % */
	9  , /* 23 % */
	10 , /* 24 % */
	10 , /* 25 % */
	10 , /* 26 % */
	12 , /* 27 % */
	12 , /* 28 % */
	12 , /* 29 % */
	13 , /* 30 % */
	13 , /* 31 % */
	13 , /* 32 % */
	13 , /* 33 % */
	13 , /* 34 % */
	14 , /* 35 % */
	15 , /* 36 % */
	16 , /* 37 % */
	16 , /* 38 % */
	17 , /* 39 % */
	18 , /* 40 % */
	19 , /* 41 % */
	20 , /* 42 % */
	21 , /* 43 % */
	22 , /* 44 % */
	24 , /* 45 % */
	24 , /* 46 % */
	26 , /* 47 % */
	27 , /* 48 % */
	28 , /* 49 % */
	30 , /* 50 % */
	31 , /* 51 % */
	32 , /* 52 % */
	34 , /* 53 % */
	35 , /* 54 % */
	37 , /* 55 % */
	39 , /* 56 % */
	40 , /* 57 % */
	42 , /* 58 % */
	43 , /* 59 % */
	45 , /* 60 % */
	46 , /* 61 % */
	48 , /* 62 % */
	49 , /* 63 % */
	51 , /* 64 % */
	54 , /* 65 % */
	55 , /* 66 % */
	58 , /* 67 % */
	59 , /* 68 % */
	62 , /* 69 % */
	65 , /* 70 % */
	66 , /* 71 % */
	69 , /* 72 % */
	72 , /* 73 % */
	75 , /* 74 % */
	79 , /* 75 % */
	82 , /* 76 % */
	85 , /* 77 % */
	88 , /* 78 % */
	93 , /* 79 % */
	96 , /* 80 % */
	99 , /* 81 % */
	103, /* 82 % */
	107, /* 83 % */
	112, /* 84 % */
	116, /* 85 % */
	121, /* 86 % */
	125, /* 87 % */
	130, /* 88 % */
	135, /* 89 % */
	141, /* 90 % */
	148, /* 91 % */
	152, /* 92 % */
	159, /* 93 % */
	164, /* 94 % */
	169, /* 95 % */
	175, /* 96 % */
	182, /* 97 % */
	189, /* 98 % */
	194, /* 99 % */
	199  /* 100% */
};
 
static enum led_brightness stuttgart_convert(enum led_brightness brightness)
{
	if (brightness > LED_FULL)
		brightness = LED_FULL;
	else if (brightness < LED_OFF)
		brightness = LED_OFF;
	/*
	 * conversion step:
	 * 1. convert high level brightness (0~255) to user slider (0~100)
	 *	user slider = round(brightness * 100 / 255)
	 *	            = (int)(brightness * 100 / 255 + 0.5)
	 *	            = (int)((brightness * 100 + 127.5) / 255)
	 *	            = (brightness * 100 + 127.5) * 2 / 255 * 2
	 *	            = (brightness * 100 *2 + 255) / 255 * 2
	 * 2. take user slider as index and look up low level brightness in table
	 */
	return stuttgart_brightness_table[((brightness * 100 * 2) + LED_FULL) / (LED_FULL * 2)];
}

static inline int omap_pwm_calc_value(unsigned long clk_rate, int ns)
{
	const unsigned long nanoseconds_per_second = 1000000000;
	int cycles;
	__u64 c;

	c = (__u64)clk_rate * ns;
	do_div(c, nanoseconds_per_second);
	cycles = c;

	return DM_TIMER_LOAD_MIN - cycles;
}

static inline void omap_pwm_set_match(struct omap_dm_timer *timer, unsigned int val)
{
	omap_dm_timer_enable(timer);
	omap_dm_timer_set_match(timer, 1, val);
	omap_dm_timer_enable(timer);
	omap_dm_timer_set_int_disable(timer, TIMER_INT_FLAGS);
	omap_dm_timer_enable(timer);
}

static irqreturn_t intensity_timer_match_interrupt(int irq, void *_pwm)
{
	struct omap_pwm_device *pwm = _pwm;
	unsigned int status;

	/* get int status */
	omap_dm_timer_enable(pwm->dm_timer);
	status = omap_dm_timer_read_status(pwm->dm_timer);

	/* acknowledge interrupts */
	omap_dm_timer_write_status(pwm->dm_timer, status);

	schedule_work(&pwm->timer_match_work);
	return IRQ_HANDLED;
}

static void omap_pwm_timer_match_work(struct work_struct *work)
{
	struct omap_pwm_device *pwm;
	unsigned int counter;
	unsigned int match_val;
	unsigned int current_match_val;
	unsigned int updated=0;

	pwm = container_of(work, struct omap_pwm_device, timer_match_work);

	match_val = atomic_read(&pwm->cached_match_val);

	/* get current match value */
	current_match_val = omap_dm_timer_get_match(pwm->dm_timer);

	/* We must update match register only in case:
	* - new match value is bigger than old one
	* - when old match value is bigger than new one, current
	* counter value must be bigger than old match value or
	* lower than new match value.
	*
	* If this conditions are not met, we will write a match value,
	* at moment when match event doesn't trigered yet and the new
	* match value is lower than counter. This will result in missing
	* the match event for this period.
	*/
	omap_dm_timer_enable(pwm->dm_timer);
	counter = omap_dm_timer_read_counter(pwm->dm_timer);

	if ((counter + COUNTER_TO_MATCH_GUARD) < match_val) {
		omap_pwm_set_match(pwm->dm_timer, match_val);
		updated = 1;
	} else if (counter > current_match_val) {
		omap_pwm_set_match(pwm->dm_timer, match_val);
		updated = 1;
	}

	if (updated)
		complete(&pwm->irq_done);
}

static int omap_pwm_init(struct omap_pwm_device *pwm)
{
	int err;

	atomic_set(&pwm->cached_match_val, 0);

	/* request the timer without resetting it */
	pwm->dm_timer = omap_dm_timer_request_specific_noreset(pwm->timer_id);
	if (pwm->dm_timer == NULL) {
		printk (KERN_ERR "omap_pwm_config: dm timer %d not available\n",
			pwm->timer_id);
		return -ENOENT;
	}

	/* Clock is set as ENABLE_ON_INIT - now we can balance the usecount
	 * by disabling it
	 */
	clk_disable(omap_dm_timer_get_fclk(pwm->dm_timer));

	INIT_WORK(&pwm->timer_match_work, omap_pwm_timer_match_work);

	/* register timer match and overflow interrupts */
	err = request_irq(omap_dm_timer_get_irq(pwm->dm_timer),
			intensity_timer_match_interrupt,
			IRQF_DISABLED, "PWM DM Timer", pwm);
	if (err) {
		printk(KERN_ERR "%s(%s) : unable to get gptimer%d IRQ\n",
			__func__, __FILE__, omap_dm_timer_get_irq(pwm->dm_timer));
		return -EBUSY;
	}

	return 0;
}

static int omap_pwm_config(struct omap_pwm_device *pwm, enum led_brightness brightness, 
		int max_brightness, int pwm_duty_off, int max_pwm_duty, int pwm_period_ns)
{
	int status = 0;
	int load_value, match_value, duty_ns;
	unsigned long clk_rate;
	unsigned int match_val;
	unsigned int current_match_val;

	if (pwm->dm_timer == NULL)
		omap_pwm_init(pwm);

	if (brightness == 0) {
		if (pwm->enabled) {
			omap_dm_timer_stop(pwm->dm_timer);
			pwm->enabled = false;
		}
		goto done;
	}

	duty_ns = (brightness * max_pwm_duty / 100) * pwm_period_ns / max_brightness;
	duty_ns += pwm_duty_off * pwm_period_ns / 10000;

	/* Configure the source for the dual-mode timer backing this
	 * generic PWM device. The clock source will ultimately determine
	 * how small or large the PWM frequency can be.
	 *
	 * At some point, it's probably worth revisiting moving this to
	 * the configure method and choosing either the slow- or
	 * system-clock source as appropriate for the desired PWM period.
	 */
	if (!pwm->enabled) {
		omap_dm_timer_set_source(pwm->dm_timer, OMAP_TIMER_SRC_SYS_CLK);
		pwm->enabled = true;
	}

	clk_rate = clk_get_rate(omap_dm_timer_get_fclk(pwm->dm_timer));

	/* Calculate the appropriate load and match values based on the
	 * specified period and duty cycle. The load value determines the
	 * cycle time and the match value determines the duty cycle.
	 */

	load_value = omap_pwm_calc_value(clk_rate, STRASBOURG_PWM_PERIOD_NS);
	match_value = omap_pwm_calc_value(clk_rate, STRASBOURG_PWM_PERIOD_NS - duty_ns);

	current_match_val = atomic_read(&pwm->cached_match_val);
	match_val = match_value;

	if (current_match_val < match_val) {
		omap_dm_timer_set_match(pwm->dm_timer, 1, match_val);
		omap_dm_timer_start(pwm->dm_timer);
		atomic_set(&pwm->cached_match_val, match_val);
	} else {
		INIT_COMPLETION(pwm->irq_done);
		atomic_set(&pwm->cached_match_val, match_val);
		omap_dm_timer_set_int_enable(pwm->dm_timer, TIMER_INT_FLAGS);
		omap_dm_timer_start(pwm->dm_timer);

		if (!wait_for_completion_timeout(&pwm->irq_done, 3*HZ)) {
			printk(KERN_ERR "PWM completion timed out\n");
			status = -EBUSY;
		}
	}

done:
	return status;
}

static void twl4030_pwm0_config (enum led_brightness brightness, int max_brightness)
{
	u8	on, off, interval;
	u8	gpbr1, pmbr1;

	/* Normalize interval to TWL_MAX_PWMxOFF */
	interval = (u8) (((unsigned long)brightness * TWL_MAX_PWMxOFF) / ((unsigned long)max_brightness));

	if (interval > 0) {
		/* Calculate on and off settings */
		on = TWL_MIN_PWMxON;
		if (interval == TWL_MAX_PWMxOFF)
			off = on; /* Fully on LED */
		else
			off = on + interval;

		/* Set PWM0ON and PWM0OFF */
		twl_i2c_write_u8(TWL4030_MODULE_PWM0, on, TWL_PWM0_PWM0ON);
		twl_i2c_write_u8(TWL4030_MODULE_PWM0, off, TWL_PWM0_PWM0OFF);

		/* PMBR[3:2] <- 1, pin muxing for PWM0 */
		twl_i2c_read_u8(TWL4030_MODULE_INTBR, &pmbr1, TWL4030_REG_PMBR1);
		twl_i2c_write_u8(TWL4030_MODULE_INTBR, pmbr1 | (1 << 2), TWL4030_REG_PMBR1);

		/* GPBR1[2] <- 1, enables PWM0 ; GPBR1[0] <- 1, PWM0 is clocked */
		twl_i2c_read_u8(TWL4030_MODULE_INTBR, &gpbr1, TWL4030_REG_GPBR1);
		twl_i2c_write_u8(TWL4030_MODULE_INTBR, gpbr1 | (1 << 2) | (1), TWL4030_REG_GPBR1);
	} else {
		/* GPBR1[2] <- 0, disables PWM0 ; GPBR1[0] <- 0, PWM0 clock is gated */
		twl_i2c_read_u8(TWL4030_MODULE_INTBR, &gpbr1, TWL4030_REG_GPBR1);
		twl_i2c_write_u8(TWL4030_MODULE_INTBR, gpbr1 & ~((1 << 2)), TWL4030_REG_GPBR1);
		twl_i2c_write_u8(TWL4030_MODULE_INTBR, gpbr1 & ~((1 << 2) | (1)), TWL4030_REG_GPBR1);
	}
}

static void twl4030_pwmA_config (enum led_brightness brightness, int max_brightness)
{
	u8 on, off, vibra_ctl;

	on = (u8) (((unsigned long)brightness * TWL_MAX_PWMxOFF) / ((unsigned long)max_brightness + 1));
	off = TWL_MAX_PWMxOFF;

	/* De-activate the H-bridge */
	twl_i2c_read_u8(TWL4030_MODULE_AUDIO_VOICE, &vibra_ctl, TWL4030_REG_VIBRA_CTL);
	twl_i2c_write_u8(TWL4030_MODULE_AUDIO_VOICE, vibra_ctl & ~(1), TWL4030_REG_VIBRA_CTL);

	/* LEDEN[0] */
	twl_i2c_write_u8(TWL4030_MODULE_LED, 0x11, TWL_LED_LEDEN);

	/* PWMAON */
	twl_i2c_write_u8(TWL4030_MODULE_PWMA, on, TWL_PWMA_PWMAON);

	/* PWMAOFF */
	twl_i2c_write_u8(TWL4030_MODULE_PWMA, off, TWL_PWMA_PWMAOFF);
}

static struct omap_pwm_device strasbourg_pwm = {
	.timer_id = STRASBOURG_TIMER_ID,
	.irq_done = COMPLETION_INITIALIZER(strasbourg_pwm.irq_done),
#ifdef CONFIG_FB_OMAP_BOOTLOADER_INIT
	.enabled = true
#endif
};

static void backlight_notify(struct led_classdev *cdev, enum led_brightness brightness)
{
	enum led_brightness lowlevel_brightness;
	/* Of course, if max_brightness is LED_FULL=255, then this line is useless. */
	if (brightness > cdev->max_brightness)
		brightness	= cdev->max_brightness;

	switch(machine_arch_type) {
	case MACH_TYPE_SANTIAGO:
	case MACH_TYPE_MONOPOLI:
		/* Santiago functions inverted compared to Strasbourg. */
		brightness = cdev->max_brightness - brightness;
		twl4030_pwm0_config (brightness, cdev->max_brightness);
		break;
	case MACH_TYPE_STRASBOURG_A2:
		switch (get_strasbourg_ver()) {
		case STRASBOURG_B1_SAMPLE:
			twl4030_pwm0_config (brightness, cdev->max_brightness);
			break;
		case STUTTGART_B1_SAMPLE:
			lowlevel_brightness = stuttgart_convert(brightness);
			omap_pwm_config (&strasbourg_pwm, lowlevel_brightness,
				LED_FULL, 38, RENNES_B1_MAX_PWM_DUTY,
				STRASBOURG_PWM_PERIOD_NS);
			break;
		case RENNES_B1_SAMPLE:
			omap_pwm_config (&strasbourg_pwm, brightness, cdev->max_brightness,
				38, RENNES_B1_MAX_PWM_DUTY, STRASBOURG_PWM_PERIOD_NS);
			break;
		default:
			omap_pwm_config (&strasbourg_pwm, brightness, cdev->max_brightness,
				0, STRASBOURG_MAX_PWM_DUTY, STRASBOURG_PWM_PERIOD_NS);
			break;
		}
		break;
	case MACH_TYPE_STRASBOURG:
	default:	/* By default, assume A1 */
		/* +1 because if on == off, it doesn't work. */
		twl4030_pwmA_config (brightness, cdev->max_brightness);
		break;
	}
}

static int offenburg_backlight_probe(struct platform_device *pdev)
{
	struct offenburg_backlight_pdata	*pdata = pdev->dev.platform_data;
	struct led_classdev			*cdev = &pdata->cdev;

	cdev->name		= "lcd-backlight";	/* So that the android layer can find it */
	cdev->brightness_set	= pdata->brightness_set;
	cdev->max_brightness	= LED_FULL;
	cdev->flags		= LED_CORE_SUSPENDRESUME;

	return led_classdev_register(&pdev->dev, cdev);
}

static int offenburg_backlight_remove(struct platform_device *pdev)
{
	struct offenburg_backlight_pdata	*pdata = pdev->dev.platform_data;
	struct led_classdev			*cdev = &pdata->cdev;

	led_classdev_unregister(cdev);
	platform_device_unregister(pdev);

	return 0;
}

#ifdef CONFIG_PM

static int backlight_suspend(struct device *dev)
{
	struct omap_pwm_device *pwm = &strasbourg_pwm;

	pwm->timer_ctrl_reg = omap_dm_timer_get_ctrl(pwm->dm_timer);
	pwm->timer_match_reg = omap_dm_timer_get_match(pwm->dm_timer);
	omap_dm_timer_disable(pwm->dm_timer);

	return 0;
}

static int backlight_resume(struct device *dev)
{
	struct omap_pwm_device *pwm = &strasbourg_pwm;
	unsigned long clk_rate;
	u32 load_value;

	omap_dm_timer_enable(pwm->dm_timer);

	clk_rate = clk_get_rate(omap_dm_timer_get_fclk(pwm->dm_timer));
	load_value = omap_pwm_calc_value(clk_rate, STRASBOURG_PWM_PERIOD_NS);

	omap_dm_timer_set_ctrl(pwm->dm_timer, pwm->timer_ctrl_reg);
	omap_dm_timer_set_load(pwm->dm_timer, 1, load_value);
	omap_dm_timer_set_match(pwm->dm_timer, 1, pwm->timer_match_reg);

	return 0;
}

static const struct dev_pm_ops backlight_pm_ops = {
	.suspend = backlight_suspend,
	.resume = backlight_resume,
};

#endif /* CONFIG_PM */


static struct platform_driver offenburg_backlight_driver = {
	.probe	= offenburg_backlight_probe,
	.remove	= offenburg_backlight_remove,
	.driver	= {
		.name	= "backlight_led",
		.owner	= THIS_MODULE,
#ifdef CONFIG_PM
		.pm	= &backlight_pm_ops,
#endif
	}
};

static struct offenburg_backlight_pdata offenburg_backlight_pdata = {
	.brightness_set	= backlight_notify,
};

static struct platform_device offenburg_backlight_device = {
	.name	= "backlight_led",
	.dev	= {
		.platform_data	= &offenburg_backlight_pdata,
	}
};

static int __init backlight_init(void)
{
	platform_device_register(&offenburg_backlight_device);
	return platform_driver_register(&offenburg_backlight_driver);
}

static void __exit backlight_exit(void)
{
	platform_driver_unregister(&offenburg_backlight_driver);
}

late_initcall_sync(backlight_init);
module_exit(backlight_exit);
