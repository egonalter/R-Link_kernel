/*
 * drivers/video/omap2/displays/panel-samsung-lms501kf03.c
 *
 * Support for the Samsung LMS501KF03 panel
 *
 * Copyright (C) 2011 TomTom
 * Author: Niels Langendorff <niels.langendorff@tomtom.com>
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
#include <linux/platform_device.h>
#include <linux/i2c/twl.h>
#include <linux/spi/spi.h>

#include <plat/gpio.h>
#include <plat/mux.h>
#include <asm/mach-types.h>
#include <plat/control.h>

#include <plat/display.h>

static struct spi_device *myspi;

static struct omap_video_timings samsung_lms501kf03_timings = {
	.x_res          = 480,
	.y_res          = 800,
	.pixel_clock    = 24576,
	.hfp            = 8,
	.hsw            = 1,
	.hbp            = 8,
	.vfp            = 6,
	.vsw            = 1,
	.vbp            = 6,
};

static int samsung_lms501kf03_panel_probe(struct omap_dss_device *dssdev)
{
	dssdev->panel.config = OMAP_DSS_LCD_TFT | OMAP_DSS_LCD_IVS | OMAP_DSS_LCD_IHS;
	dssdev->panel.timings = samsung_lms501kf03_timings;

	return 0;
}

static void samsung_lms501kf03_panel_remove(struct omap_dss_device *dssdev)
{
	/* Empty */
}

static int init_lms501kf03_lcd(struct spi_device *spi);

static int samsung_lms501kf03_panel_enable(struct omap_dss_device *dssdev)
{
	int ret = 0;

	if (dssdev->platform_enable)
		ret = dssdev->platform_enable(dssdev);
	init_lms501kf03_lcd(myspi);

	return ret;
}

static void samsung_lms501kf03_panel_disable(struct omap_dss_device *dssdev)
{
	if (dssdev->platform_disable)
		dssdev->platform_disable(dssdev);
}

static int samsung_lms501kf03_panel_suspend(struct omap_dss_device *dssdev)
{
	samsung_lms501kf03_panel_disable(dssdev);
	return 0;
}

static int samsung_lms501kf03_panel_resume(struct omap_dss_device *dssdev)
{
	return samsung_lms501kf03_panel_enable(dssdev);
}

static struct omap_dss_driver samsung_lms501kf03_driver = {
	.probe		= samsung_lms501kf03_panel_probe,
	.remove		= samsung_lms501kf03_panel_remove,

	.enable		= samsung_lms501kf03_panel_enable,
	.disable	= samsung_lms501kf03_panel_disable,
	.suspend	= samsung_lms501kf03_panel_suspend,
	.resume		= samsung_lms501kf03_panel_resume,

	.driver         = {
		.name   = "samsung_lms501kf03_panel",
		.owner  = THIS_MODULE,
	},
};

/* Set password */
static unsigned char set_passwd[] = 
		{ 
			0xB9, 0xFF, 0x83, 0x69 
		};
#define SIZE_SET_PASSWD		(sizeof(set_passwd) / sizeof(char))

/* Set power */
static unsigned char set_power[] = 
		{ 
			0xB1, 0x01, 0x00, 0x34, 0x06, 0x00, 0x14, 0x14, 0x20, 0x28, 
			0x12, 0x12, 0x17, 0x0A, 0x01, 0xE6, 0xE6, 0xE6, 0xE6, 0xE6 
		};
#define SIZE_SET_POWER		(sizeof(set_power) / sizeof(char))

/* Set display */
static unsigned char set_display[] = 
		{ 
			0xB2, 0x00, 0x2B, 0x03, 0x03, 0x70, 0x00, 0xFF, 0x00, 0x00,
			0x00, 0x00, 0x03, 0x03, 0x00, 0x01
		};
#define SIZE_SET_DISPLAY	(sizeof(set_display) / sizeof(char))

/* Set RGBIF */
static unsigned char set_rgbif[] =
		{
			0xB3, 0x09
		};
#define SIZE_SET_RGBIF	(sizeof(set_rgbif) / sizeof(char))

/* Set Disp Inv. */
static unsigned char set_dispinv[] = 
		{
			0xB4, 0x01, 0x08, 0x77, 0x0E, 0x06
		};
#define SIZE_SET_DISPINV	(sizeof(set_dispinv) / sizeof(char))

/* Set Vcom */
static unsigned char set_vcom[] =
		{
			0xB6, 0x4C, 0x2E
		};
#define SIZE_SET_VCOM	(sizeof(set_vcom) / sizeof(char))

/* Set Gate */
static unsigned char set_gate[] =
		{
			0xD5, 0x00, 0x05, 0x03, 0x29, 0x01, 0x07, 0x17, 0x68, 0x13,
			0x37, 0x20, 0x31, 0x8A, 0x46, 0x9B, 0x57, 0x13, 0x02, 0x75,
			0xB9, 0x64, 0xA8, 0x07, 0x0F, 0x04, 0x07
		};
#define SIZE_SET_GATE	(sizeof(set_gate) / sizeof(char))

/* Set Panel */
static unsigned char set_panel[] =
		{
			0xCC, 0x0A
		};
#define SIZE_SET_PANEL	(sizeof(set_panel) / sizeof(char))

/* Set COLMOD */
static unsigned char set_colmod[] =
		{
			0x3A, 0x77
		};
#define SIZE_SET_COLMOD	(sizeof(set_colmod) / sizeof(char))

/* Set W Gamma */
static unsigned char set_w_gamma[] =
		{
			0xE0, 0x00, 0x04, 0x09, 0x0F, 0x1F, 0x3F, 0x1F, 0x2F, 0x0A,
			0x0F, 0x10, 0x16, 0x18, 0x16, 0x17, 0x0D, 0x15, 0x00, 0x04,
			0x09, 0x0F, 0x38, 0x3F, 0x20, 0x39, 0x0A, 0x0F, 0x10, 0x16, 
			0x18, 0x16, 0x17, 0x0D, 0x15 
		};
#define SIZE_SET_W_GAMMA	(sizeof(set_w_gamma) / sizeof(char))

/* Set RGB Gamma */
static unsigned char set_rgb_gamma[] =
		{
			0xC1, 0x01, 0x03, 0x07, 0x0F, 0x1A, 0x22, 0x2C, 0x33, 0x3C, 
			0x46, 0x4F, 0x58, 0x60, 0x69, 0x71, 0x79, 0x82, 0x89, 0x92, 
			0x9A, 0xA1, 0xA9, 0xB1, 0xB9, 0xC1, 0xC9, 0xCF, 0xD6, 0xDE, 
			0xE5, 0xEC, 0xF3, 0xF9, 0xFF, 0xDD, 0x39, 0x07, 0x1C, 0xCB, 
			0xAB, 0x5F, 0x49, 0x80, 0x03, 0x07, 0x0F, 0x19, 0x20, 0x2A, 
			0x31, 0x39, 0x42, 0x4B, 0x53, 0x5B, 0x63, 0x6B, 0x73, 0x7B, 
			0x83, 0x8A, 0x92, 0x9B, 0xA2, 0xAA, 0xB2, 0xBA, 0xC2, 0xCA, 
			0xD0, 0xD8, 0xE1, 0xE8, 0xF0, 0xF8, 0xFF, 0xF7, 0xD8, 0xBE, 
			0xA7, 0x39, 0x40, 0x85, 0x8C, 0xC0, 0x04, 0x07, 0x0C, 0x17, 
			0x1C, 0x23, 0x2B, 0x34, 0x3B, 0x43, 0x4C, 0x54, 0x5B, 0x63, 
			0x6A, 0x73, 0x7A, 0x82, 0x8A, 0x91, 0x98, 0xA1, 0xA8, 0xB0, 
			0xB7, 0xC1, 0xC9, 0xCF, 0xD9, 0xE3, 0xEA, 0xF4, 0xFF, 0x00, 
			0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
		};
#define SIZE_SET_RGB_GAMMA	(sizeof(set_rgb_gamma) / sizeof(char))

/* Set UPDN */
static unsigned char set_updn[] =
		{
			0x36, 0x10
		};
#define SIZE_SET_UPDN	(sizeof(set_updn) / sizeof(char))

/* Sleep Out */
static unsigned char set_sleep_out[] =  { 0x11 };
#define SIZE_SLEEP_OUT	(sizeof(set_sleep_out) / sizeof(char))

/* Disp On */
static unsigned char set_disp_on[] = { 0x29 };
#define SIZE_DISP_ON	(sizeof(set_disp_on) / sizeof(char))

/* Sleep in */
static unsigned char set_sleep_in[] = { 0x10 };
#define SIZE_SLEEP_IN	(sizeof(set_sleep_in) / sizeof(char))

#define SPI_COMMAND	(0x00)
#define SPI_DATA	(0x01)

static int
spi_command(struct spi_device *spi, unsigned char index)
{
	int ret = 0;
	unsigned int cmd = 0;

	cmd = (SPI_COMMAND << 8) | index;

	if (spi_write(spi,  (unsigned char *)&cmd, 2))
		printk(KERN_WARNING "error in spi_write %04X\n", cmd);

	udelay(10);
	return ret;
}

static int
spi_data(struct spi_device *spi, unsigned char data)
{
	int ret = 0;
	unsigned int cmd = 0;

	cmd = (SPI_DATA << 8) | data;

	if (spi_write(spi,  (unsigned char *)&cmd, 2))
		printk(KERN_WARNING "error in spi_write %04X\n", cmd);

	udelay(10);
	return ret;
}

static int spi_write_set(struct spi_device *spi, unsigned char * data, int len)
{
	int ret;
	int i;

	ret = spi_command(spi, data[0]);
	if (ret < 0) {
		printk ("Error sending command\n");
		return ret;
	}

	for (i = 1; i < len; i++) {
		ret = spi_data(spi, data[i]);
		if (ret < 0) {
			printk ("Error sending data\n");
			break;
		}
	}
	return ret;
}

static int init_lms501kf03_lcd(struct spi_device *spi)
{
	int ret;

	printk ("Initialising Samsung lms501 display\n");
	if (spi_write_set(spi, set_passwd, SIZE_SET_PASSWD) || 
		spi_write_set(spi, set_power, SIZE_SET_POWER) ||
		spi_write_set(spi, set_display, SIZE_SET_DISPLAY) ||
		spi_write_set(spi, set_rgbif, SIZE_SET_RGBIF) ||
		spi_write_set(spi, set_dispinv, SIZE_SET_DISPINV) ||
		spi_write_set(spi, set_vcom, SIZE_SET_VCOM) ||
		spi_write_set(spi, set_gate, SIZE_SET_GATE) ||
		spi_write_set(spi, set_panel, SIZE_SET_PANEL) ||
		spi_write_set(spi, set_colmod, SIZE_SET_COLMOD) ||
		spi_write_set(spi, set_w_gamma, SIZE_SET_W_GAMMA) ||
		spi_write_set(spi, set_rgb_gamma, SIZE_SET_RGB_GAMMA) ||
		spi_write_set(spi, set_updn, SIZE_SET_UPDN) ||
		spi_write_set(spi, set_sleep_out, SIZE_SLEEP_OUT)) {
		return -1;
	}

	mdelay(120);

	ret = spi_write_set(spi, set_disp_on, SIZE_DISP_ON);
	if (ret < 1) {
		return -1;
	}

	return 0;
}

static int lms501kf03_spi_suspend(struct spi_device *spi,  pm_message_t mesg)
{
	int ret;

	/* Sleep in */
	ret = spi_write_set(spi, set_sleep_in, SIZE_SLEEP_IN);
	if (ret < 1) {
		// Might fail when going to suspend, return 0, otherwise suspend
		// will be aborted.
		return 0;
	}
	mdelay(120);

	return 0;
}

static int lms501kf03_spi_resume(struct spi_device *spi)
{
	int ret;

	ret = spi_write_set(spi, set_sleep_out, SIZE_SLEEP_OUT);
	if (ret < 1) {
		return -1;
	}
	mdelay(120);

	return 0;
}

static int lms501kf03_spi_probe(struct spi_device *spi)
{
	spi->mode = SPI_MODE_0;
	spi->bits_per_word = 9;
	spi_setup(spi);

	/* Save spi device */
	myspi = spi;

	return omap_dss_register_driver(&samsung_lms501kf03_driver);
}

static int lms501kf03_spi_remove(struct spi_device *spi)
{
	omap_dss_unregister_driver(&samsung_lms501kf03_driver);

	return 0;
}

static struct spi_driver lms501kf03_spi_driver = {
	.probe		= lms501kf03_spi_probe,
	.remove		= __devexit_p(lms501kf03_spi_remove),
	.suspend	= lms501kf03_spi_suspend,
	.resume		= lms501kf03_spi_resume,
	.driver		= {
		.name	= "lms501kf03_disp_spi",
		.bus	= &spi_bus_type,
		.owner	= THIS_MODULE,
	},
};

static int __init samsung_lms501kf03_panel_drv_init(void)
{
	return spi_register_driver(&lms501kf03_spi_driver);
}

static void __exit samsung_lms501kf03_panel_drv_exit(void)
{
	spi_unregister_driver(&lms501kf03_spi_driver);
}

module_init(samsung_lms501kf03_panel_drv_init);
module_exit(samsung_lms501kf03_panel_drv_exit);
MODULE_LICENSE("GPL");
