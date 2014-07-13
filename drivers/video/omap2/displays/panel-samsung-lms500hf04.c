/*
 * Support for the Samsung LMS500HF04 panel
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

static struct omap_video_timings samsung_lms500hf04_timings = {
	.x_res          = 480,
	.y_res          = 272,
	.pixel_clock    = 9200,

	.vsw            = 1,
	.vfp            = 7,
	.vbp            = 11-1,

	.hsw            = 41,
	.hfp            = 8,
	.hbp            = 45-41,

};

static int samsung_lms500hf04_panel_probe(struct omap_dss_device *dssdev)
{
	dssdev->panel.config = OMAP_DSS_LCD_TFT | OMAP_DSS_LCD_IVS | OMAP_DSS_LCD_IHS;
	dssdev->panel.timings = samsung_lms500hf04_timings;

	return 0;
}

static void samsung_lms500hf04_panel_remove(struct omap_dss_device *dssdev)
{
}

static int samsung_lms500hf04_panel_enable(struct omap_dss_device *dssdev)
{
	int r = 0;

	if (dssdev->platform_enable)
		r = dssdev->platform_enable(dssdev);

	return r;
}

static void samsung_lms500hf04_panel_disable(struct omap_dss_device *dssdev)
{
	if (dssdev->platform_disable)
		dssdev->platform_disable(dssdev);
}

static int samsung_lms500hf04_panel_suspend(struct omap_dss_device *dssdev)
{
	samsung_lms500hf04_panel_disable(dssdev);
	return 0;
}

static int samsung_lms500hf04_panel_resume(struct omap_dss_device *dssdev)
{
	return samsung_lms500hf04_panel_enable(dssdev);
}

static struct omap_dss_driver samsung_lms500hf04_driver = {
	.probe		= samsung_lms500hf04_panel_probe,
	.remove		= samsung_lms500hf04_panel_remove,

	.enable		= samsung_lms500hf04_panel_enable,
	.disable	= samsung_lms500hf04_panel_disable,
	.suspend	= samsung_lms500hf04_panel_suspend,
	.resume		= samsung_lms500hf04_panel_resume,

	.driver         = {
		.name   = "samsung_lms500hf04_panel",
		.owner  = THIS_MODULE,
	},
};

static int __init samsung_lms500hf04_panel_drv_init(void)
{
	return omap_dss_register_driver(&samsung_lms500hf04_driver);
}

static void __exit samsung_lms500hf04_panel_drv_exit(void)
{
	omap_dss_unregister_driver(&samsung_lms500hf04_driver);
}

module_init(samsung_lms500hf04_panel_drv_init);
module_exit(samsung_lms500hf04_panel_drv_exit);
MODULE_LICENSE("GPL");
