#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/vgpio.h>
#include <plat/offenburg.h>
#include <plat/display.h>
#include <plat/fdt.h>

extern void dispc_enable_lcd_out(bool enable);

extern struct omap_dss_device offenburg_lms500_lcd_device;
extern struct omap_dss_device offenburg_lms700_lcd_device;
extern struct omap_dss_device offenburg_cmog070_lcd_device;
extern struct omap_dss_device offenburg_ld050wv1sp01_lcd_device;
extern struct omap_dss_device offenburg_lms501kf03_lcd_device;

struct omap_dss_device offenburg_lcd_device;

struct lcm_desc {
	char					*lcm_name;
	struct omap_dss_device	*lcm_info;
};

static struct lcm_desc offenburg_lcm[] __initdata = {
	{
		.lcm_name	= "g070y2l01",
		.lcm_info	= &offenburg_cmog070_lcd_device,
	},
	{
		.lcm_name	= "DD070NA_H8519",
		.lcm_info	= &offenburg_cmog070_lcd_device,
	},
	{
		.lcm_name	= "DD070NA_A336",
		.lcm_info	= &offenburg_cmog070_lcd_device,
	},
	{
		.lcm_name	= "DD070NA_A336_1",
		.lcm_info	= &offenburg_cmog070_lcd_device,
	},
	{
		.lcm_name	= "DD070NA_A336_2",
		.lcm_info	= &offenburg_cmog070_lcd_device,
	},
	{
		.lcm_name	= "lms700kf19",
		.lcm_info	= &offenburg_lms700_lcd_device,
	},
	{
		.lcm_name	= "lms500hf04",
		.lcm_info	= &offenburg_lms500_lcd_device,
	},
	{
		.lcm_name	= "ld050wv1sp01",
		.lcm_info	= &offenburg_ld050wv1sp01_lcd_device,
	},
	{
		.lcm_name	= "lms501kf03",
		.lcm_info	= &offenburg_lms501kf03_lcd_device,
	},
};

static struct omap_dss_device *get_lcm_info(void)
{
	int i;
	const char *screen = fdt_get_string ("/features", "tft", DEFAULT_LCM);

	for (i = 0; i < ARRAY_SIZE(offenburg_lcm); i++)
		if (strcmp(offenburg_lcm[i].lcm_name, screen) == 0)
			return offenburg_lcm[i].lcm_info;

#if defined(CONFIG_MACH_STRASBOURG) || defined(CONFIG_MACH_STRASBOURG_A2) || defined(CONFIG_MACH_STRASBOURG_XENIA)
	printk(KERN_WARNING "LCM in FDT not found, using built-in default g070y2l01\n");
	return &offenburg_cmog070_lcd_device; /* g070y2l01 */
#elif defined(CONFIG_MACH_SANTIAGO)
	printk(KERN_WARNING "LCM in FDT not found, using built-in default ld050wv1sp01\n");
	return &offenburg_ld050wv1sp01_lcd_device; /* ld050wv1sp01 */
#endif
}

static int __init init_lcm(void)
{
	offenburg_lcd_device = *get_lcm_info();

	return  gpio_request(TT_VGPIO_LCD_RESET, "LCD RESET")   ||
		gpio_request(TT_VGPIO_LCD_PWR_ON, "LCD PWR ON") ||
		gpio_request(TT_VGPIO_LCD_STBY, "LCD STBY")     ||
		gpio_request(TT_VGPIO_LCM_BL_PWR_ON, "LCM BL PWR ON") ||
		gpio_request(TT_VGPIO_LCM_PWR_ON, "LCM PWR ON") ||
		gpio_request(TT_VGPIO_BACKLIGHT_DIM, "BACKLIGHT_DIM") ||
		gpio_request(TT_VGPIO_BACKLIGHT_ON, "BACKLIGHT_ON") ||
		gpio_request(TT_VGPIO_SENSE_11V0, "SENSE_11V0") ||
		gpio_direction_input(TT_VGPIO_SENSE_11V0) ||
#ifdef CONFIG_FB_OMAP_BOOTLOADER_INIT
		gpio_direction_output(TT_VGPIO_LCD_RESET, 0)    ||
		gpio_direction_output(TT_VGPIO_LCD_PWR_ON, 0)   ||
		gpio_direction_output(TT_VGPIO_LCD_STBY, 0)     ||
		gpio_direction_output(TT_VGPIO_LCM_BL_PWR_ON, 1) ||
		gpio_direction_output(TT_VGPIO_LCM_PWR_ON, 1)   ||
		gpio_direction_output(TT_VGPIO_BACKLIGHT_DIM, 0) ||
		gpio_direction_output(TT_VGPIO_BACKLIGHT_ON, 1);
#else
		gpio_direction_output(TT_VGPIO_LCD_RESET, 1)    ||
		gpio_direction_output(TT_VGPIO_LCD_PWR_ON, 0)   ||
		gpio_direction_output(TT_VGPIO_LCD_STBY, 0)     ||
		gpio_direction_output(TT_VGPIO_LCM_BL_PWR_ON, 0) ||
		gpio_direction_output(TT_VGPIO_LCM_PWR_ON, 0)   ||
		gpio_direction_output(TT_VGPIO_BACKLIGHT_DIM, 0) ||
		gpio_direction_output(TT_VGPIO_BACKLIGHT_ON, 0);
#endif
}

subsys_initcall_sync(init_lcm);
