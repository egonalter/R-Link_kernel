/*
 * arch/arm/plat-tomtom/offenburg/samsung_
 *
 * Power on/off sequence and board glue for the Samsung LMS700KF19 panel
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
#include <plat/display.h>
#include <plat/offenburg.h>

static int offenburg_panel_enable_lcd(struct omap_dss_device *dssdev)
{
	/* activate LCD power supply */
	gpio_direction_output(TT_VGPIO_LCM_PWR_ON, 1);

	/* switch LMS700KF19 to normal mode */
	gpio_direction_output(TT_VGPIO_LCD_STBY, 1);

	/* T2 */
	msleep(50);

	/* clear LCD reset */
	gpio_direction_output(TT_VGPIO_LCD_RESET, 0);

	/* T3 */
	msleep(1);

	/* switch the display on */
	gpio_direction_output(TT_VGPIO_LCD_PWR_ON, 1);

	/* T4 (need more margin?) */
	msleep(11);

	/* switch backlight on */
	gpio_direction_output(TT_VGPIO_LCM_BL_PWR_ON, 1);
	gpio_direction_output(TT_VGPIO_BACKLIGHT_ON, 1);

	return 0;
}

static void offenburg_panel_disable_lcd(struct omap_dss_device *dssdev)
{
	/* switch backlight off */
	gpio_direction_output(TT_VGPIO_LCM_BL_PWR_ON, 0);
	gpio_direction_output(TT_VGPIO_BACKLIGHT_ON, 0);

	/* switch LMS700KF19 to standby mode */
	gpio_direction_output(TT_VGPIO_LCD_STBY, 0);

	/* T5 */
	msleep(8);

	/* switch the display off */
	gpio_direction_output(TT_VGPIO_LCD_PWR_ON, 0);

	/* T6 */
	msleep(2);

	/* set LCD reset */
	gpio_direction_output(TT_VGPIO_LCD_RESET, 1);

	/* cut LCD power supply */
	gpio_direction_output(TT_VGPIO_LCM_PWR_ON, 0);
}

struct omap_dss_device offenburg_lms700_lcd_device = {
	.type			= OMAP_DISPLAY_TYPE_DPI,
	.name			= "lcd",
	.driver_name		= "samsung_lms700kf19_panel",
	.phy.dpi.data_lines	= 24,
	.platform_enable	= offenburg_panel_enable_lcd,
	.platform_disable	= offenburg_panel_disable_lcd,
};
EXPORT_SYMBOL(offenburg_lms700_lcd_device);
