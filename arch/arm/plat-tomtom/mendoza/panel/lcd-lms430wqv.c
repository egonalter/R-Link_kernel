#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/vgpio.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/hardirq.h>
#include <asm/io.h>
#include <plat/gpio-cfg.h>
#include <mach/regs-gpio.h>
#include <mach/map.h>
#include <plat/gpio-bank-f.h>
#include <plat/gpio-bank-n.h>
#include <plat/mendoza.h>
#include <plat/mendoza-lcd.h>
#include <plat/fb.h>


static inline void panel_sleep(unsigned int ms)
{
	if (in_atomic()) {
		mdelay(ms);
	} else {
		msleep(ms);
	}
}

static int mendoza_fb_reboot(struct notifier_block *block, unsigned long code, void *null)
{
	pr_emerg("LCD: Shutting off LCM\n");
	gpio_set_value(TT_VGPIO_DISP_ON, 0);
	panel_sleep(100);
	return 0;
}

static int mendoza_reset_lcd(struct platform_device *pdev)
{
	return 0;
}

static int mendoza_reset_power_onoff(struct platform_device *pdev, int power)
{
	if(power)	
		gpio_set_value(TT_VGPIO_DISP_ON, 1);
	else
		gpio_set_value(TT_VGPIO_DISP_ON, 0);
	return 0;
}
/* TODO: correct init procedure according to spec */
static int mendoza_gpio_setup(struct platform_device *pdev)
{
	unsigned int cfg;
	int i;
	
	/* select TFT LCD type (RGB I/F) */
	cfg = readl(S5P64XX_SPC_BASE);
	cfg &= ~S5P6450_SPCON_LCD_SEL_MASK;
	cfg |= S5P6450_SPCON_LCD_SEL_RGB;
	writel(cfg, S5P64XX_SPC_BASE);

	for (i = 0; i < 16; i++)
		s3c_gpio_cfgpin(S5P6450_GPI(i), S3C_GPIO_SFN(2));

	for (i = 0; i < 12; i++)
		s3c_gpio_cfgpin(S5P6450_GPJ(i), S3C_GPIO_SFN(2));
	
        if (gpio_request(TT_VGPIO_LCM_PWR_EN, "TT_VGPIO_LCM_PWR_EN") ||
        	gpio_request(TT_VGPIO_DISP_ON, "TT_VGPIO_DISP_ON")) {
        	printk("LCD: Failed to request gpio\n");
        } else {
        	gpio_direction_output(TT_VGPIO_LCM_PWR_EN, 1);
        	gpio_direction_output(TT_VGPIO_DISP_ON, 1);
        }
        panel_sleep(100);

        return 0;
}

static struct s3cfb_lcd lms430wqv = {
	.width = 480,
	.height = 272,
	.bpp = 24,
	.freq = 40,

	.timing = {
		.h_fp = 8,
		.h_bp = 45,
		.h_sw = 41,
		.v_fp = 7,
		.v_fpe = 1,
		.v_bp = 11,
		.v_bpe = 1,
		.v_sw = 1,
	},

	.polarity = {
		.rise_vclk = 1,
		.inv_hsync = 1,
		.inv_vsync = 1,
		.inv_vden = 0,
	},
};

static int mendoza_reset_spi_lcd(struct platform_device *pdev)
{
	return 0;
}

static int mendoza_gpio_spi_setup(void)
{
	return 0;
}

static struct lcm_desc mendoza_lcm __initdata = {
	.lcm_name		= "LMS430WQV",
	.lcm_info		= &lms430wqv,
	.gpio_setup		= mendoza_gpio_setup,
	.reset_lcd		= mendoza_reset_lcd,
	.shutdown		= mendoza_fb_reboot,
	.reset_power_onoff	= mendoza_reset_power_onoff
};

struct lcd_paneldev LMS430WQV = {
	.panel = &mendoza_lcm
};

