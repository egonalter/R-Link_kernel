#include <linux/init.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/pwm_backlight.h>
#include <linux/leds.h>
#include <linux/clk.h>
#include <linux/i2c/twl.h>
#include <linux/platform_device.h>

#include <asm/mach-types.h>
#include <mach/strasbourg_ver.h>

#include <plat/dmtimer.h>
#include <plat/offenburg.h>

#define DM_TIMER_LOAD_MIN	0xFFFFFFFE
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

struct offenburg_backlight_pdata {
	struct led_classdev	cdev;
	void			(*brightness_set)(struct led_classdev *, enum led_brightness brightness);
};

struct omap_pwm_device {
	struct omap_dm_timer *dm_timer;
	bool enabled;
	unsigned int timer_id;
};

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

static int omap_pwm_config(struct omap_pwm_device *pwm, enum led_brightness brightness, 
		int max_brightness, int pwm_duty_off, int max_pwm_duty, int pwm_period_ns)
{
	int status = 0;
	int load_value, match_value, duty_ns;
	unsigned long clk_rate;

	if (pwm->dm_timer == NULL) {
		if (pwm->enabled) {
			/* PWM is already enabled - request the timer without resetting it */
			pwm->dm_timer = omap_dm_timer_request_specific_noreset(pwm->timer_id);
			if (pwm->dm_timer == NULL) {
				printk (KERN_ERR "omap_pwm_config: dm timer %d not available\n", 
					pwm->timer_id);
				status = -ENOENT;
				goto done;
			}
			/* Clock is set as ENABLE_ON_INIT - now we can balance the usecount 
			 * by disabling it
			 */
			clk_disable (omap_dm_timer_get_fclk(pwm->dm_timer));
		}
		else
			pwm->dm_timer = omap_dm_timer_request_specific(pwm->timer_id);
			if (pwm->dm_timer == NULL) {
				printk (KERN_ERR "omap_pwm_config: dm timer %d not available\n", 
					pwm->timer_id);
				status = -ENOENT;
				goto done;
			}
	}

	if (brightness == 0) {
		if (pwm->enabled) {
			omap_dm_timer_stop(pwm->dm_timer);
			omap_dm_timer_disable(pwm->dm_timer);
			pwm->enabled = false;
		}
		goto done;
	}

	duty_ns = (brightness * max_pwm_duty / 100) * pwm_period_ns / max_brightness;
	duty_ns += pwm_duty_off * pwm_period_ns / 10000;

	printk(KERN_DEBUG "brightness: %d, max_brightness: %d, max_duty: %d, period_ns: %d, duty_ns: %d\n",
		brightness, max_brightness, max_pwm_duty, pwm_period_ns, duty_ns);

	/* Configure the source for the dual-mode timer backing this
	 * generic PWM device. The clock source will ultimately determine
	 * how small or large the PWM frequency can be.
	 *
	 * At some point, it's probably worth revisiting moving this to
	 * the configure method and choosing either the slow- or
	 * system-clock source as appropriate for the desired PWM period.
	 */
	if (!pwm->enabled) {
		omap_dm_timer_enable(pwm->dm_timer);
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

	/* We MUST enable yet stop the associated dual-mode timer before
	 * attempting to write its registers.
	 */
	omap_dm_timer_stop(pwm->dm_timer);

	omap_dm_timer_set_load(pwm->dm_timer, true, load_value);
	omap_dm_timer_set_match(pwm->dm_timer, true, match_value);

	omap_dm_timer_set_pwm(pwm->dm_timer, 0, true, 
			OMAP_TIMER_TRIGGER_OVERFLOW_AND_COMPARE);

	/* Enable the counter--always--before attempting to write its
	 * registers and then set the timer to its minimum load value to
	 * ensure we get an overflow event right away once we start it.
	 */
	omap_dm_timer_write_counter(pwm->dm_timer, DM_TIMER_LOAD_MIN);
	omap_dm_timer_start(pwm->dm_timer);

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
		
static void backlight_notify(struct led_classdev *cdev, enum led_brightness brightness)
{
	static struct omap_pwm_device strasbourg_pwm = { 
		.timer_id = STRASBOURG_TIMER_ID,
#ifdef CONFIG_FB_OMAP_BOOTLOADER_INIT
		.enabled = true
#endif
	};
	
	//printk ("Setting brightness to %d (%d)\n", brightness, cdev->max_brightness);

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

static struct platform_driver offenburg_backlight_driver = {
	.probe	= offenburg_backlight_probe,
	.remove	= offenburg_backlight_remove,
	.driver	= {
		.name	= "backlight_led",
		.owner	= THIS_MODULE,
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
