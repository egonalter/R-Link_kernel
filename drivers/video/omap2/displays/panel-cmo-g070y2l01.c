/*
 * Support for the Chi Mei Optoelectronics G070Y2-L01 panel
 *
 * Copyright (C) 2010 TomTom
 * Author: Oreste Salerno <oreste.salerno@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <linux/module.h>
#include <linux/delay.h>

#include <plat/display.h>

static struct omap_video_timings cmo_g070y2l01_timings = {
	.x_res = 800,
	.y_res = 480,

	.hsw            = 1,     /* note: DE only display */
        .hfp            = 96,    
        .hbp            = 96,    /* whole line should have 992 clocks */

        .vsw            = 1,     /* note: DE only display */
        .vfp            = 10,    
        .vbp            = 10,    /* whole frame should have 500 clocks */

	.pixel_clock	= 28800
};

static int cmo_g070y2l01_panel_probe(struct omap_dss_device *dssdev)
{
	/* Keep VSYNC and HSYNC constantly low */ 
	dssdev->panel.config = OMAP_DSS_LCD_TFT;
	dssdev->panel.timings = cmo_g070y2l01_timings;

	return 0;
}

static void cmo_g070y2l01_panel_remove(struct omap_dss_device *dssdev)
{
}

static int cmo_g070y2l01_panel_enable(struct omap_dss_device *dssdev)
{
	int r = 0;

	if (dssdev->platform_enable)
		r = dssdev->platform_enable(dssdev);

	return r;
}

static void cmo_g070y2l01_panel_disable(struct omap_dss_device *dssdev)
{
	if (dssdev->platform_disable)
		dssdev->platform_disable(dssdev);
}

static int cmo_g070y2l01_panel_suspend(struct omap_dss_device *dssdev)
{
	cmo_g070y2l01_panel_disable(dssdev);
	return 0;
}

static int cmo_g070y2l01_panel_resume(struct omap_dss_device *dssdev)
{
	return cmo_g070y2l01_panel_enable(dssdev);
}

static struct omap_dss_driver cmo_g070y2l01_driver = {
	.probe		= cmo_g070y2l01_panel_probe,
	.remove		= cmo_g070y2l01_panel_remove,

	.enable		= cmo_g070y2l01_panel_enable,
	.disable	= cmo_g070y2l01_panel_disable,
	.suspend	= cmo_g070y2l01_panel_suspend,
	.resume		= cmo_g070y2l01_panel_resume,

	.driver         = {
		.name   = "cmo_g070y2l01_panel",
		.owner  = THIS_MODULE,
	},
};

static int __init cmo_g070y2l01_panel_drv_init(void)
{
	return omap_dss_register_driver(&cmo_g070y2l01_driver);
}

static void __exit cmo_g070y2l01_panel_drv_exit(void)
{
	omap_dss_unregister_driver(&cmo_g070y2l01_driver);
}

module_init(cmo_g070y2l01_panel_drv_init);
module_exit(cmo_g070y2l01_panel_drv_exit);
MODULE_LICENSE("GPL");
