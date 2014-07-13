/*
 * arch/arm/plat-tomtom/offenburg/panel-cmo_g070y2l01.c
 *
 * Power on/off sequence and board glue for the CMO G070Y2L01 panel
 *
 * Copyright (C) 2010 TomTom International BV
 * Author: Matthias Kaehlcke <matthias.kaehlcke@tomtom.com>
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/vgpio.h>
#include <linux/i2c/twl.h>
#include <asm/mach-types.h>
#include <plat/display.h>
#include <plat/offenburg.h>
#include <mach/strasbourg_ver.h>
#include <mach/../../mux.h>

#define TWL4030_REG_GPBR1	0x0C	/* Offset is 0x0C from the base, which is TWL4030_BASEADD_INTBR = 0x85 */
#define SENSE_11V0_MAX_RETRY    5   /* Max number of attempts to power on the LCM */

static unsigned long ts_lcd_disabled = 0;

static int offenburg_panel_enable_lcd(struct omap_dss_device *dssdev)
{
	u8 gpbr1;
	
	if (ts_lcd_disabled != 0) {
		const unsigned long t4_expiration = ts_lcd_disabled + msecs_to_jiffies(500);

		if (time_before(jiffies, t4_expiration))
			/* the G070Y2 requires a minimum delay of 500ms between disabling and enabling */
			msleep(jiffies_to_msecs(t4_expiration - jiffies));
	}

	if (machine_is_strasbourg_a2() && get_strasbourg_ver() >= STRASBOURG_B2_SAMPLE) {
		int i;
		for (i = 0; i < SENSE_11V0_MAX_RETRY; i++) {
			/* attempt to activate LCD power supply */
			gpio_set_value (TT_VGPIO_LCM_PWR_ON, 1);
			/* T8 */
			msleep(2);
			if (gpio_get_value(TT_VGPIO_SENSE_11V0) == 1)
				break;
		}
		if (i == SENSE_11V0_MAX_RETRY) {
			printk(KERN_ERR "Failed to power on the LCM! Giving up ...\n");
			return -EIO;
		}
	}
	else {
		/* attempt to activate LCD power supply */
		gpio_set_value (TT_VGPIO_LCM_PWR_ON, 1);
		/* T8 */
		msleep(2);
	}

	/* switch LVDS transmitter on */
	gpio_set_value(TT_VGPIO_LCD_PWR_ON, 1);

	/* clear LCD reset */
	gpio_set_value(TT_VGPIO_LCD_RESET, 0);

	/* T5 - according to the LCM datasheet this delay should be 20 ms but we have to increase it 
	 * to 100 ms to avoid displaying brief white flashes on the screen.
	 */
	msleep(100);

	/* switch backlight on */
	gpio_set_value(TT_VGPIO_LCM_BL_PWR_ON, 1);
	gpio_set_value(TT_VGPIO_BACKLIGHT_ON, 1);
	msleep(1);
	if (machine_is_strasbourg_a2()) {
		/* Enable the PWM after switching on the backlight */
		if (get_strasbourg_ver() == STRASBOURG_B1_SAMPLE) {
			twl_i2c_read_u8(TWL4030_MODULE_INTBR, &gpbr1, 
				TWL4030_REG_GPBR1);
			twl_i2c_write_u8(TWL4030_MODULE_INTBR, 
				gpbr1 | ((1 << 2) | (1)), TWL4030_REG_GPBR1);
		}
		else if (get_strasbourg_ver() >= STRASBOURG_B2_SAMPLE) {
			omap_mux_init_signal ("gpmc_ncs5", 
				OMAP_PIN_INPUT_PULLDOWN | OMAP_MUX_MODE3);
		}
	}

	return 0;
}

static void offenburg_panel_disable_lcd(struct omap_dss_device *dssdev)
{
	u8 gpbr1;
	
	/* switch backlight off */
	if (machine_is_strasbourg_a2()) {
		/* Disable the PWM before switching off the backlight */
		if (get_strasbourg_ver() == STRASBOURG_B1_SAMPLE) {		
			twl_i2c_read_u8(TWL4030_MODULE_INTBR, &gpbr1, 
				TWL4030_REG_GPBR1);
			twl_i2c_write_u8(TWL4030_MODULE_INTBR, 
				gpbr1 & ~((1 << 2)), TWL4030_REG_GPBR1);
			twl_i2c_write_u8(TWL4030_MODULE_INTBR, 
				gpbr1 & ~((1 << 2) | (1)), TWL4030_REG_GPBR1);
		}
		else if (get_strasbourg_ver() >= STRASBOURG_B2_SAMPLE) {
			omap_mux_init_signal ("gpmc_ncs5", 
				OMAP_PIN_INPUT_PULLDOWN | OMAP_MUX_MODE4);
		}		
	}
	msleep(1);
	gpio_set_value(TT_VGPIO_LCM_BL_PWR_ON, 0);
	gpio_set_value(TT_VGPIO_BACKLIGHT_ON, 0);

	/* T6 */
	msleep(10);

	/* switch LVDS transmitter off */
	gpio_set_value(TT_VGPIO_LCD_PWR_ON, 0);

	/* LCD power supply */
	gpio_set_value(TT_VGPIO_LCM_PWR_ON, 0);

	/* Activate LCD_RESET */
	msleep(100);
	gpio_set_value(TT_VGPIO_LCD_RESET, 1);	

	ts_lcd_disabled = jiffies;
}

struct omap_dss_device offenburg_cmog070_lcd_device = {
	.type			= OMAP_DISPLAY_TYPE_DPI,
	.name			= "lcd",
	.driver_name		= "cmo_g070y2l01_panel",
	.phy.dpi.data_lines	= 24,
	.phy.dpi.use_dsi_clk	= true,
	.phy.dpi.use_dsi_ssc	= true,
	.platform_enable	= offenburg_panel_enable_lcd,
	.platform_disable	= offenburg_panel_disable_lcd,
};

