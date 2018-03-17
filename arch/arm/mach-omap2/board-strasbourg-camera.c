/*
 * arch/arm/mach-omap2/board-strasbourg-camera.c
 *
 * Driver for Strasbourg TVP5151 video decoder
 *
 * arch/arm/mach-omap2/board-strasbourg-camera.h
 *
 * Copyright (C) 2010 Texas Instruments Inc
 * Author: Gilles Talis <g-talis@ti.com>
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

#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/mm.h>
#include <linux/videodev2.h>
#include <linux/i2c/twl.h>
#include <linux/delay.h>

#include <plat/mux.h>
#include <plat/board.h>
#include <plat/offenburg.h>

#include <media/v4l2-int-device.h>
#include <media/tvp5150.h>

/* Include V4L2 ISP-Camera driver related header file */
#include <../drivers/media/video/omap34xxcam.h>
#include <../drivers/media/video/isp/ispreg.h>

#include "mux.h"
#include "board-strasbourg-camera.h"

#define MODULE_NAME			"Strasbourg_tvp515x"

#define TVP515x_I2C_BUSNUM		2
#define TVP515x_I2C_ADDR_LO		0x5C // (0xB8)
#define TVP515x_I2C_ADDR_HI		0x5D // (0xBA)

#define ISP_TVP515x_MCLK		216000000

#ifndef TRUE
#define TRUE			1
#endif
#ifndef FALSE
#define FALSE			0
#endif

#if defined(CONFIG_VIDEO_TVP5150) || defined(CONFIG_VIDEO_TVP5150_MODULE)
#if defined(CONFIG_VIDEO_OMAP3) || defined(CONFIG_VIDEO_OMAP3_MODULE)
static struct omap34xxcam_hw_config decoder_hwc = {
	.dev_index		= 2,
	.dev_minor		= 5,
	.dev_type		= OMAP34XXCAM_SLAVE_SENSOR,
	.u.sensor.sensor_isp	= 1,
	.u.sensor.capture_mem	= PAGE_ALIGN(720*525*2*4), 
};

// Enable YUV_PARALLEL for PARALLEL mode support with DISCRETE SYNCS (HS/VS)
// Disable YUV_PARALLEL for PARALLEL mode support with Embedded SYNCS (BT 656) => not fully supported
#define YUV_PARALLEL 
static struct isp_interface_config tvp515x_if_config = {
#ifdef YUV_PARALLEL
	.ccdc_par_ser		= ISP_PARLL, // PARALLEL HS/VS
#else
	.ccdc_par_ser		= ISP_PARLL_YUV_BT, //BT656
#endif
	.dataline_shift		= 0x0,
	.hsvs_syncdetect	= ISPCTRL_SYNC_DETECT_VSRISE,
	.strobe			= 0x0,
	.prestrobe		= 0x0,
	.wenlog			= 0,
	.shutter			= 0x0,
	.cam_mclk		= ISP_TVP515x_MCLK,
	.wait_hs_vs		= 2,
#ifdef YUV_PARALLEL
	.crop.left	        = 128, // Upon HS int, start getting data from pixel 128
	.crop.top	        = 11,  // Upon VS int, start getting data from line 11
	.u.par.par_bridge	= 0x3, // set to 3 in ISP_PARLL
#else
	.u.par.par_bridge	= 0x0, // set to 0 in BT 656
#endif
	.u.par.par_clk_pol	= 0x0,
};
#endif


/**
 * @brief tvp515x_set_prv_data - Returns tvp515x omap34xx driver private data
 *
 * @param priv - pointer to omap34xxcam_hw_config structure
 *
 * @return result of operation - 0 is success
 */
static int tvp515x_set_prv_data(void *s, void *priv)
{
#if defined(CONFIG_VIDEO_OMAP3) || defined(CONFIG_VIDEO_OMAP3_MODULE)
	struct omap34xxcam_hw_config *hwc = priv;

	printk(KERN_DEBUG "tvp515x_set_prv_data\n");
	if (priv == NULL)
		return -EINVAL;

	hwc->u.sensor.sensor_isp = decoder_hwc.u.sensor.sensor_isp;
	hwc->u.sensor.capture_mem = decoder_hwc.u.sensor.capture_mem;
	hwc->dev_index = decoder_hwc.dev_index;
	hwc->dev_minor = decoder_hwc.dev_minor;
	hwc->dev_type = decoder_hwc.dev_type;
	return 0;
#else
	return -EINVAL;
#endif
}

/**
 * @brief tvp515x_reset - Performs a reset of the device
 */
static void tvp515x_reset(void)
{
	/* Make sure the SCL and SDA signals stay high */
	omap_mux_init_signal("i2c2_scl", OMAP_PIN_INPUT_PULLUP | OMAP_MUX_MODE4);
	omap_mux_init_signal("i2c2_sda", OMAP_PIN_INPUT_PULLUP | OMAP_MUX_MODE4);

	/* Make sure we don't pick up any rogue pulses on the PCLK pad */
	omap_mux_init_signal("cam_pclk", OMAP_PIN_INPUT_PULLUP | OMAP_MUX_MODE4);

	/* Perform the reset sequence; assert reset and cam_on */
	gpio_set_value(TT_VGPIO_CAM_RST, 1);
	udelay(10);
	gpio_set_value(TT_VGPIO_CAM_ON, 1);

	msleep(25); /* For safety, always adhere to TVP5151 spec time constant t1 (20ms) */

	gpio_set_value(TT_VGPIO_CAM_RST, 0);

	msleep(1); /* Block to avoid I2C traffic as per spec'd time constant t3 (200us) */

	/* Restore the PCLK pad now that the output from the TVP5151 should be stable */
	omap_mux_init_signal("cam_pclk", OMAP_PIN_INPUT_PULLUP | OMAP_MUX_MODE0);

	/* Reconfigure the mux to use I2C */
	omap_mux_init_signal("i2c2_scl", OMAP_PIN_INPUT | OMAP_MUX_MODE0);
	omap_mux_init_signal("i2c2_sda", OMAP_PIN_INPUT | OMAP_MUX_MODE0);
}

/**
 * @brief tvp5151_power_set - Power-on or power-off tvp5151 device
 *
 * @param power - enum, Power on/off, resume/standby
 *
 * @return result of operation - 0 is success
 */
static int initialized = 0;

static int tvp515x_power_set(void *v4l2_int_device, int on)
{
	struct v4l2_int_device *s;
	enum v4l2_power power;

	struct omap34xxcam_videodev *vdev;

	printk(KERN_DEBUG "tvp515x_power_set (%d)\n", on);

	power = (enum v4l2_power)on;
	s = (struct v4l2_int_device *)v4l2_int_device;
	vdev = s->u.slave->master->priv;

	switch (power) {
	case V4L2_POWER_OFF:
		// Power off external cam
		gpio_set_value(TT_VGPIO_CAM_PWR_ON, 0);
		// Power off video decoder
		gpio_set_value(TT_VGPIO_CAM_ON, 0);
		initialized = 0;
		break;

	case V4L2_POWER_STANDBY:
	case V4L2_POWER_ON:
		if (!initialized) {
			// Power on external cam
			gpio_set_value(TT_VGPIO_CAM_PWR_ON, 1);
			// Power on video decoder
			tvp515x_reset();
#if defined(CONFIG_VIDEO_OMAP3) || defined(CONFIG_VIDEO_OMAP3_MODULE)
			isp_configure_interface(vdev->cam->isp, &tvp515x_if_config);
#endif		
			initialized = 1;
		} else {
			// No action required
			printk(KERN_DEBUG "tvp515x already initialized, no action necessary\n");
		}

		break;

	default:
		return -ENODEV;
		break;
	}
	return 0;
}

/**
 * @brief tvp515x_suspend - Suspends the device
 *
 * @return result of operation - 0 is success
 */
static int tvp515x_suspend(void *v4l2_int_device)
{
	/* Set device in power-down mode */
	tvp515x_power_set(v4l2_int_device, V4L2_POWER_OFF);
	return 0;
}

/**
 * @brief tvp515x_resume - Resumes the device
 *
 * @param on - power state in which to resume the device
 *
 * @return result of operation - 0 is success
 */
static int tvp515x_resume(void *v4l2_int_device, int on)
{
	/* Reset the device - required to put the device out of its unknown state */
	tvp515x_reset();

	/* Put the device into the required power state */
	tvp515x_power_set(v4l2_int_device, on);

	return 0;
}

static struct tvp5150_platform_data tvp515x_pdata = {
	.master			= "omap34xxcam",
	.power_set		= tvp515x_power_set,
	.suspend		= tvp515x_suspend,
	.resume			= tvp515x_resume,
	.priv_data_set		= tvp515x_set_prv_data,
	/* Some interface dependent params */
	.analog_input_channel 	= TVP5150_COMPOSITE0,
#ifdef YUV_PARALLEL
	.digital_input_mode = TVP5150_DISCRETE_SYNCS, // with HS/VS
#else
	.digital_input_mode = TVP5150_BT656, // without HS/VS
#endif
	
};


static struct i2c_board_info __initdata tvp515x_i2c_board_info = {
	I2C_BOARD_INFO("tvp5150", 0),
	.platform_data	= &tvp515x_pdata,
};

#endif				/* #ifdef CONFIG_VIDEO_TVP5150 */

/**
 * @brief strasbourg_mdc_config - GPIO configuration for
 *                          TVP515x Reset and Power Down
 *
 * @return result of operation - 0 is success
 */
static int strasbourg_mdc_config(void)
{
	/* Enable Video Decoder */
	if (gpio_request(TT_VGPIO_CAM_ON, "CAM_ON") < 0) {
		printk(KERN_ERR "Failed to get GPIO %d for CAM_ON\n", TT_VGPIO_CAM_ON);
		return -EINVAL;
	}
	gpio_direction_output(TT_VGPIO_CAM_ON, 0);

	/* Camera power is off when booting */
	if (gpio_request(TT_VGPIO_CAM_PWR_ON, "TVP515x PDN") < 0) {
		printk(KERN_ERR "Failed to get GPIO %d for TVP515x PDN\n", TT_VGPIO_CAM_PWR_ON);
		return -EINVAL;
	}
	gpio_direction_output(TT_VGPIO_CAM_PWR_ON, 0);

	if (gpio_request(TT_VGPIO_CAM_RST, "TPV515x reset") < 0) {
		printk(KERN_ERR "failed to get GPIO %d for TVP515x RESETB\n", TT_VGPIO_CAM_RST);
		return -EINVAL;
	}
	gpio_direction_output(TT_VGPIO_CAM_RST, 0);

	/* Reset the device - required to put the device out of its unknown state */
	tvp515x_reset();

	return 0;
}

/**
 * @brief strasbourg_tvp5151_init - module init function. Should be called before any
 *                          client driver init call
 *
 * @return result of operation - 0 is success
 */
int __init strasbourg_tvp515x_init(void)
{
	int err;
	printk(KERN_INFO MODULE_NAME ": Driver registration Starting \n");

	// configure GPIOs
	err = strasbourg_mdc_config();
	if (err) {
		printk(KERN_ERR MODULE_NAME ": MDC configuration failed \n");
		return err;
	}


	/*
	 * Register the I2C devices present in the board to the I2C
	 * framework.
	 * If more I2C devices are added, then each device information should
	 * be registered with I2C using i2c_register_board_info().
	 */
#if defined(CONFIG_VIDEO_TVP5150) || defined(CONFIG_VIDEO_TVP5150_MODULE)
	tvp515x_i2c_board_info.addr = TVP515x_I2C_ADDR_HI;

	err = i2c_register_board_info(TVP515x_I2C_BUSNUM,
					&tvp515x_i2c_board_info, 1);
	if (err) {
		printk(KERN_ERR MODULE_NAME \
				": TVP515x I2C Board Registration failed \n");
		return err;
	}
#endif

	printk(KERN_INFO MODULE_NAME ": Driver registration complete \n");

	return 0;
}
arch_initcall(strasbourg_tvp515x_init);
