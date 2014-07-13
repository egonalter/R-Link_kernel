/*
 * Support for the LG LD050WV1SP01 panel
 *
 * Copyright (C) 2010 TomTom
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

#define XRES		480
#define YRES		800
#define HBP			0x1F	// tHBP (or 0x20 in datasheet)
#define VBP			0x1C	// tVBP
#define PIXEL_CLK	31000	// KHz
#define HP			624		// tHP 520..624
#define VP			840		// tVP 830..840
#define WHA			XRES	// tWHA 480
#define WVA			YRES	// tWVA 800
#define VFP			(VP-WVA-VBP)
#define HFP			(HP-WHA-HBP)

static struct spi_device *myspi;

static struct omap_video_timings lg_ld050wv1sp01_timings = {
	.x_res          = XRES,
	.y_res          = YRES,
	.pixel_clock    = PIXEL_CLK,

	.vsw            = 1,
	.vfp            = VFP,
	.vbp            = VBP-1,

	.hsw            = 1,
	.hfp            = HFP,
	.hbp            = HBP-1,
};

static int lg_ld050wv1sp01_panel_probe(struct omap_dss_device *dssdev)
{
	dssdev->panel.config = OMAP_DSS_LCD_TFT | OMAP_DSS_LCD_IVS;
	dssdev->panel.timings = lg_ld050wv1sp01_timings;

	return 0;
}

static void lg_ld050wv1sp01_panel_remove(struct omap_dss_device *dssdev)
{
	/* Empty */
}

static int init_ld050wv1sp01_lcd(struct spi_device *spi);

static int lg_ld050wv1sp01_panel_enable(struct omap_dss_device *dssdev)
{
	int ret = 0;

	if (dssdev->platform_enable)
		ret = dssdev->platform_enable(dssdev);
	init_ld050wv1sp01_lcd(myspi);

	return ret;
}

static void lg_ld050wv1sp01_panel_disable(struct omap_dss_device *dssdev)
{
	if (dssdev->platform_disable)
		dssdev->platform_disable(dssdev);
}

static int lg_ld050wv1sp01_panel_suspend(struct omap_dss_device *dssdev)
{
	lg_ld050wv1sp01_panel_disable(dssdev);
	return 0;
}

static int lg_ld050wv1sp01_panel_resume(struct omap_dss_device *dssdev)
{
	return lg_ld050wv1sp01_panel_enable(dssdev);
}

static struct omap_dss_driver lg_ld050wv1sp01_driver = {
	.probe		= lg_ld050wv1sp01_panel_probe,
	.remove		= lg_ld050wv1sp01_panel_remove,

	.enable		= lg_ld050wv1sp01_panel_enable,
	.disable	= lg_ld050wv1sp01_panel_disable,
	.suspend	= lg_ld050wv1sp01_panel_suspend,
	.resume		= lg_ld050wv1sp01_panel_resume,

	.driver         = {
		.name   = "lg_ld050wv1sp01_panel",
		.owner  = THIS_MODULE,
	},
};

#define SPI_START	0x70
#define ID_BIT		(1 << 2)
#define RS_BIT		(1 << 1)
#define RW_BIT		(1 << 0)
#define SPI_INDEX	(SPI_START | ID_BIT)
#define SPI_DATA	(SPI_START | RS_BIT | ID_BIT)

static int
spi_index(struct spi_device *spi, unsigned char index)
{
	int ret = 0;
	unsigned int cmd = 0;

	cmd = (SPI_INDEX << 8) | index;

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

static int init_ld050wv1sp01_lcd(struct spi_device *spi)
{
	/* Initialization Sequence */
	spi_index(spi, 0xC0);
	spi_data(spi, 0x01);
	spi_data(spi, 0x18); 
	mdelay(10);     

	spi_index(spi, 0x20);
	spi_data(spi, 0x00); 

	spi_index(spi, 0x36);
	spi_data(spi, 0x00);

	spi_index(spi, 0x3A);
	spi_data(spi, 0x70);

	spi_index(spi, 0xB1);
	spi_data(spi, 0x12); 
	spi_data(spi, HBP); // tHBP
	spi_data(spi, VBP); // tVBP

	spi_index(spi, 0xB2);
	spi_data(spi, 0x20); 
	spi_data(spi, 0xC8);

	spi_index(spi, 0xB3);
	spi_data(spi, 0x00);

	spi_index(spi, 0xB4);
	spi_data(spi, 0x04); 

	spi_index(spi, 0xB5);
	spi_data(spi, 0x12);
	spi_data(spi, 0x0F); 
	spi_data(spi, 0x0F);
	spi_data(spi, 0x00); 
	spi_data(spi, 0x00);

	spi_index(spi, 0xB6);
	spi_data(spi, 0x0B);
	spi_data(spi, 0x18);
	spi_data(spi, 0x02);
	spi_data(spi, 0x40);
	spi_data(spi, 0x10);
	spi_data(spi, 0x33);
	mdelay(10);     

	spi_index(spi, 0xC0); // Not set in datasheet
	spi_data(spi, 0x01);
	spi_data(spi, 0x18);
	mdelay(10);     

	spi_index(spi, 0xC3);
	spi_data(spi, 0x07);
	spi_data(spi, 0x05);
	spi_data(spi, 0x04);
	spi_data(spi, 0x04);
	spi_data(spi, 0x03);

	spi_index(spi, 0xC4);
	spi_data(spi, 0x12);
	spi_data(spi, 0x34);
	spi_data(spi, 0x13);
	spi_data(spi, 0x13);
	spi_data(spi, 0x00);
	spi_data(spi, 0x0F); // 0x0C according to the datasheet
	mdelay(10); 

	spi_index(spi, 0xC5);
	spi_data(spi, 0x76);
	mdelay(10); 

	spi_index(spi, 0xC6);
	spi_data(spi, 0x23);
	spi_data(spi, 0x50);
	spi_data(spi, 0x00);
	mdelay(10); 

	spi_index(spi, 0xC7);
	spi_data(spi, 0x00);
	spi_data(spi, 0xFF); 

	// Gamma
	spi_index(spi, 0xD0);
	spi_data(spi, 0x00); // PKP1  PKP0
	spi_data(spi, 0x04); // PKP3  PKP2      
	spi_data(spi, 0x57); // PKP5  PKP4      
	spi_data(spi, 0x02); // PRP1  PRP0      
	spi_data(spi, 0x00); // VRP0    
	spi_data(spi, 0x00); // VRP1    
	spi_data(spi, 0x02); // PFP1  PFP0     
	spi_data(spi, 0x00); // PFP3  PFP2      
	spi_data(spi, 0x03); //       PMP

	spi_index(spi, 0xD2);
	spi_data(spi, 0x00); // PKP1  PKP0     
	spi_data(spi, 0x04); // PKP3  PKP2      
	spi_data(spi, 0x57); // PKP5  PKP4      
	spi_data(spi, 0x02); // PRP1  PRP0      
	spi_data(spi, 0x00); // VRP0    
	spi_data(spi, 0x00); // VRP1    
	spi_data(spi, 0x02); // PFP1  PFP0     
	spi_data(spi, 0x00); // PFP3  PFP2      
	spi_data(spi, 0x03); //       PMP

	spi_index(spi, 0xD4);
	spi_data(spi, 0x00); // PKP1  PKP0     
	spi_data(spi, 0x04); // PKP3  PKP2      
	spi_data(spi, 0x57); // PKP5  PKP4      
	spi_data(spi, 0x02); // PRP1  PRP0      
	spi_data(spi, 0x00); // VRP0    
	spi_data(spi, 0x00); // VRP1    
	spi_data(spi, 0x02); // PFP1  PFP0     
	spi_data(spi, 0x00); // PFP3  PFP2      
	spi_data(spi, 0x03); //       PMP      

	spi_index(spi, 0xD1);
	spi_data(spi, 0x00); // PKN1  PKN0
	spi_data(spi, 0x04); // PKN3  PKN2      
	spi_data(spi, 0x57); // PKN5  PKN4      
	spi_data(spi, 0x02); // PRN1  PRN0      
	spi_data(spi, 0x00); // VRN0    
	spi_data(spi, 0x00); // VRN1    
	spi_data(spi, 0x00); // PFN1  PFN0     
	spi_data(spi, 0x00); // PFN3  PFN2      
	spi_data(spi, 0x03); //       PMN      

	spi_index(spi, 0xD3);
	spi_data(spi, 0x00); // PKN1  PKN0     
	spi_data(spi, 0x04); // PKN3  PKN2      
	spi_data(spi, 0x57); // PKN5  PKN4      
	spi_data(spi, 0x02); // PRN1  PRN0      
	spi_data(spi, 0x00); // VRN0    
	spi_data(spi, 0x00); // VRN1    
	spi_data(spi, 0x00); // PFN1  PFN0
	spi_data(spi, 0x00); // PFN3  PFN2
	spi_data(spi, 0x03); //       PMN       

	spi_index(spi, 0xD5);
	spi_data(spi, 0x00); // PKN1  PKN0     
	spi_data(spi, 0x04); // PKN3  PKN2      
	spi_data(spi, 0x57); // PKN5  PKN4      
	spi_data(spi, 0x02); // PRN1  PRN0      
	spi_data(spi, 0x00); // VRN0    
	spi_data(spi, 0x00); // VRN1    
	spi_data(spi, 0x00); // PFN1  PFN0     
	spi_data(spi, 0x00); // PFN3  PFN2      
	spi_data(spi, 0x03); //       PMN 

	spi_index(spi, 0x11); // Sleep out
	spi_data(spi, 0x00);
	mdelay(150);  

	spi_index(spi, 0x29); //Display On
	spi_data(spi, 0x00);

	return 0;
}

static int ld050wv1sp01_spi_suspend(struct spi_device *spi,  pm_message_t mesg)
{
	/* Sleep in */
	spi_index(spi, 0x10);
	spi_data(spi, 0x00);

	/* Display off */
	spi_index(spi,  0x28);
	spi_data(spi, 0x00);
	mdelay(40);

	return 0;
}

static int ld050wv1sp01_spi_resume(struct spi_device *spi)
{
	/* Sleep out */
	spi_index(spi, 0x11);
	spi_data(spi, 0x00);
	mdelay(150);

	/* Display on */
	spi_index(spi, 0x29);
	spi_data(spi, 0x00);

	return 0;
}

static int ld050wv1sp01_spi_probe(struct spi_device *spi)
{
	spi->mode = SPI_MODE_0;
	spi->bits_per_word = 16;
	spi_setup(spi);

	/* Save spi device */
	myspi = spi;

	return omap_dss_register_driver(&lg_ld050wv1sp01_driver);
}

static int ld050wv1sp01_spi_remove(struct spi_device *spi)
{
	omap_dss_unregister_driver(&lg_ld050wv1sp01_driver);

	return 0;
}

static struct spi_driver ld050wv1sp01_spi_driver = {
	.probe		= ld050wv1sp01_spi_probe,
	.remove		= __devexit_p(ld050wv1sp01_spi_remove),
	.suspend	= ld050wv1sp01_spi_suspend,
	.resume		= ld050wv1sp01_spi_resume,
	.driver		= {
		.name	= "ld050wv1sp01_disp_spi",
		.bus	= &spi_bus_type,
		.owner	= THIS_MODULE,
	},
};

static int __init lg_ld050wv1sp01_panel_drv_init(void)
{
	return spi_register_driver(&ld050wv1sp01_spi_driver);
}

static void __exit lg_ld050wv1sp01_panel_drv_exit(void)
{
	spi_unregister_driver(&ld050wv1sp01_spi_driver);
}

module_init(lg_ld050wv1sp01_panel_drv_init);
module_exit(lg_ld050wv1sp01_panel_drv_exit);
MODULE_LICENSE("GPL");
