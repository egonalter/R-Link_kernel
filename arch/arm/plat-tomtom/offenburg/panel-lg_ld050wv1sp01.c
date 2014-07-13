/*
 * arch/arm/plat-tomtom/offenburg/
 *
 * Power on/off sequence and board glue for the LG LD050WV1SP01 panel
 *
 * Copyright (C) 2010 TomTom International BV
 * Author: Niels Langendorff <niels.langendorff@tomtom.com>
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
#include <plat/display.h>
#include <linux/gpio.h>
#include <linux/vgpio.h>
#include <plat/offenburg.h>

static int offenburg_panel_enable_lcd(struct omap_dss_device *dssdev)
{
	/* clear LCD reset */
	gpio_direction_output(TT_VGPIO_LCD_RESET, 0);
	
	/* activate LCD power supply */
	gpio_direction_output(TT_VGPIO_LCM_PWR_ON, 1);

	msleep(2);

	/* Set LCD reset */
	gpio_direction_output(TT_VGPIO_LCD_RESET, 1);
	
	msleep(11);

	/* clear LCD reset */
	gpio_direction_output(TT_VGPIO_LCD_RESET, 0);

	msleep(1);

	/* switch backlight on */
	gpio_direction_output(TT_VGPIO_LCM_BL_PWR_ON, 1);

	return 0;
}

static void offenburg_panel_disable_lcd(struct omap_dss_device *dssdev)
{
	/* switch backlight off */
	gpio_direction_output(TT_VGPIO_LCM_BL_PWR_ON, 0);

	/* T6 */
	msleep(2);

	/* set LCD reset */
	gpio_direction_output(TT_VGPIO_LCD_RESET, 1);

	/* cut LCD power supply */
	gpio_direction_output(TT_VGPIO_LCM_PWR_ON, 0);

	/* clear LCD reset */
	gpio_direction_output(TT_VGPIO_LCD_RESET, 0);
}

struct omap_dss_device offenburg_ld050wv1sp01_lcd_device = {
	.type				= OMAP_DISPLAY_TYPE_DPI,
	.name				= "lcd",
	.driver_name		= "lg_ld050wv1sp01_panel",
	.phy.dpi.data_lines	= 24,
	.platform_enable	= offenburg_panel_enable_lcd,
	.platform_disable	= offenburg_panel_disable_lcd,
};
EXPORT_SYMBOL(offenburg_ld050wv1sp01_lcd_device);
