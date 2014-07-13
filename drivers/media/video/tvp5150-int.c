/*
 * tvp5150-int.c - Texas Instruments TVP5150/TVP5151 INT driver
 *
 * Copyright (c) 2005,2006 Mauro Carvalho Chehab (mchehab@infradead.org)
 * This code is placed under the terms of the GNU General Public License v2
 */
/*
 * tvp5150-int.c - Texas Instruments TVP5150/TVP5151 V4L2 INT driver
 *
 *
 * Copyright (C) 2010 Gilles TALIS <g-talis@ti.com>
 *
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <linux/i2c.h>
#include <linux/videodev2.h>
#include <linux/delay.h>
#include <media/v4l2-device.h>
#include <media/tvp5150.h>
#include <media/v4l2-i2c-drv.h>
#include <media/v4l2-chip-ident.h>
#include <media/v4l2-int-device.h>

#include "tvp5150_reg.h"

MODULE_DESCRIPTION("Texas Instruments TVP5150/TVP5151 video decoder V4L2 int driver");
MODULE_AUTHOR("Gilles TALIS");
MODULE_LICENSE("GPL");

// For test purposes, we can force TVP5151 to generate a black output on the digital interface
//#define FORCE_BLACK_OUTPUT
// Should we autodetect analog input signal or select it by default?
#define AUTODETECT
// Enable/disable registers logging
#define TVP5151_DEBUG

static int debug;
module_param(debug, int, 0);
MODULE_PARM_DESC(debug, "Debug level (0-2)");

#define dprintk(msg...) if (debug) { printk(msg); }

/* supported controls */
static struct v4l2_queryctrl tvp5150_qctrl[] = {
	{
		.id = V4L2_CID_BRIGHTNESS,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Brightness",
		.minimum = 0,
		.maximum = 255,
		.step = 1,
		.default_value = 128,
		.flags = 0,
	}, {
		.id = V4L2_CID_CONTRAST,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Contrast",
		.minimum = 0,
		.maximum = 255,
		.step = 0x1,
		.default_value = 128,
		.flags = 0,
	}, {
		 .id = V4L2_CID_SATURATION,
		 .type = V4L2_CTRL_TYPE_INTEGER,
		 .name = "Saturation",
		 .minimum = 0,
		 .maximum = 255,
		 .step = 0x1,
		 .default_value = 128,
		 .flags = 0,
	}, {
		.id = V4L2_CID_HUE,
		.type = V4L2_CTRL_TYPE_INTEGER,
		.name = "Hue",
		.minimum = -128,
		.maximum = 127,
		.step = 0x1,
		.default_value = 0,
		.flags = 0,
	}
};

/**
 * enum tvp5150_std - enum for supported standards
 */
enum tvp5150_std {
	STD_NTSC_MJ = 0,
	STD_PAL_BDGHIN,
	STD_PAL_M,
	STD_PAL_CombN,
	STD_NTSC_443,
	STD_SECAM,
	STD_INVALID
};


/* List of image formats supported by TVP515x decoder
 * Currently we are using 8 bit mode only, but can be
 * extended to 10/20 bit mode.
 */
static const struct v4l2_fmtdesc tvp5150_fmt_list[] = {
	{
	 .index = 0,
	 .type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
	 .flags = 0,
	 .description = "8-bit UYVY 4:2:2 Format",
	 .pixelformat = V4L2_PIX_FMT_UYVY,
	},
};

#define NUM_CAPTURE_FORMATS ARRAY_SIZE(tvp5150_fmt_list)

/**
 * struct tvp5150_std_info - Structure to store standard informations
 * @width: Line width in pixels
 * @height:Number of active lines
 * @video_std: Value to write in REG_VIDEO_STD register
 * @standard: v4l2 standard structure information
 */
struct tvp5150_std_info {
	unsigned long width;
	unsigned long height;
	u8 video_std;
	struct v4l2_standard standard;
};


/*
 * Supported standards -
 *
 * Currently supports (video standard register)
 *	- 0010 = (M, J) NTSC ITU-R BT.601
 *	- 0011 = Reserved
 *	- 0100 = (B, G, H, I, N) PAL ITU-R BT.601
 *	- 0101 = Reserved
 *	- 0110 = (M) PAL ITU-R BT.601
 *	- 0111 = Reserved
 *	- 1000 = (Combination-N) PAL ITU-R BT.601
 *	- 1001 = Reserved
 *	- 1010 = NTSC 4.43 ITU-R BT.601
 *	- 1011 = Reserved
 *	- 1100 = SECAM ITU-R BT.601
 */
static struct tvp5150_std_info tvp515x_std_list[] = {
	/* Standard: STD_NTSC_MJ */
	[STD_NTSC_MJ] = {
	 .width = NTSC_NUM_ACTIVE_PIXELS,
	 .height = NTSC_NUM_ACTIVE_LINES,
	 .video_std = VIDEO_STD_NTSC_MJ_BIT,
	 .standard = {
		      .index = 0,
		      .id = V4L2_STD_NTSC,
		      .name = "NTSC",
		      .frameperiod = {1001, 30000},
		      .framelines = 525
		     },
	},
	/* Standard: STD_PAL_BDGHIN */
	[STD_PAL_BDGHIN] = {
	 .width = PAL_NUM_ACTIVE_PIXELS,
	 .height = PAL_NUM_ACTIVE_LINES,
	 .video_std = VIDEO_STD_PAL_BDGHIN_BIT,
	 .standard = {
		      .index = 1,
		      .id = V4L2_STD_PAL,
		      .name = "PAL",
		      .frameperiod = {1, 25},
		      .framelines = 625
		     },
	},
	/* Standard: STD_PAL_BDGHIN */
	[STD_PAL_M] = {
	 .width = PAL_NUM_ACTIVE_PIXELS,
	 .height = PAL_NUM_ACTIVE_LINES,
	 .video_std = VIDEO_STD_PAL_M_BIT,
	 .standard = {
		      .index = 2,
		      .id = V4L2_STD_PAL,
		      .name = "PAL",
		      .frameperiod = {1, 25},
		      .framelines = 625
		     },
	},
	/* Standard: STD_PAL_CombN */
	[STD_PAL_CombN] = {
	 .width = PAL_NUM_ACTIVE_PIXELS,
	 .height = PAL_NUM_ACTIVE_LINES,
	 .video_std = VIDEO_STD_PAL_COMBINATION_N_BIT,
	 .standard = {
		      .index = 3,
		      .id = V4L2_STD_PAL,
		      .name = "PAL",
		      .frameperiod = {1, 25},
		      .framelines = 625
		     },
	},
	/* Standard: STD_NTSC_443 */
	[STD_NTSC_443] = {
	 .width = NTSC_NUM_ACTIVE_PIXELS,
	 .height = NTSC_NUM_ACTIVE_LINES,
	 .video_std = VIDEO_STD_NTSC_4_43_BIT,
	 .standard = {
		      .index = 4,
		      .id = V4L2_STD_NTSC,
		      .name = "NTSC",
		      .frameperiod = {1001, 30000},
		      .framelines = 525
		     },
	},
	/* Standard: STD_SECAM */
	[STD_SECAM] = {
	 .width = PAL_NUM_ACTIVE_PIXELS,
	 .height = PAL_NUM_ACTIVE_LINES,
	 .video_std = VIDEO_STD_SECAM_BIT,
	 .standard = {
		      .index = 5,
		      .id = V4L2_STD_SECAM,
		      .name = "SECAM",
		      .frameperiod = {1, 25},
		      .framelines = 625
		     },
	},
};

#define NUM_OF_SUPPORTED_SIZES	2

#define TVP5150_MOD_NAME "TVP5150:"
#define TVP5150_DRIVER_NAME "tvp5150"

struct tvp5150_decoder {
	const struct tvp5150_platform_data *pdata;
	struct v4l2_int_device *v4l2_int_device;
	struct i2c_client *i2c_client;
	struct v4l2_pix_format pix;
	struct v4l2_fract timeperframe;
	v4l2_std_id norm;	/* Current set standard */
	int num_fmts;
	const struct v4l2_fmtdesc *fmt_list;
	enum tvp5150_std current_std;
	int num_stds;
	struct tvp5150_std_info *std_list;

	u32 input;
	int enable;
	int bright;
	int contrast;
	int hue;
	int sat;
	enum v4l2_power state;

};
static struct tvp5150_decoder tvp5150;




static int tvp5150_read(struct i2c_client *client, unsigned char addr)
{
	unsigned char buffer[1];
	int rc;

	buffer[0] = addr;
	if (1 != (rc = i2c_master_send(client, buffer, 1)))
		v4l_err(client, "i2c i/o error: rc == %d (should be 1)\n", rc);

	msleep(10);

	if (1 != (rc = i2c_master_recv(client, buffer, 1)))
		v4l_err(client, "i2c i/o error: rc == %d (should be 1)\n", rc);

	return (buffer[0]);
}

// new style write using i2c_client
static inline void tvp5150_write(struct i2c_client *client, unsigned char addr,
				 unsigned char value)
{

	unsigned char buffer[2];
	int rc;

	buffer[0] = addr;
	buffer[1] = value;
	if (2 != (rc = i2c_master_send(client, buffer, 2)))
		v4l_err(client, "i2c i/o error: rc == %d (should be 2)\n", rc);
}

static void dump_reg_range(struct i2c_client *client, char *s, u8 init,
				const u8 end, int max_line)
{
	int i = 0;

	while (init != (u8)(end + 1)) {
		if ((i % max_line) == 0) {
			if (i > 0)
				printk("\n");
			printk("tvp5150: %s reg 0x%02x = ", s, init);
		}
		printk("%02x ", tvp5150_read(client, init));

		init++;
		i++;
	}
	printk("\n");
}

#ifdef TVP5151_DEBUG
static int tvp5150_log_status(struct v4l2_int_device *s)
{
	struct tvp5150_decoder *decoder = s->priv;
	struct i2c_client *sd = decoder->i2c_client;

	dprintk(KERN_DEBUG "tvp5150: Video input source selection #1 = 0x%02x\n",
			tvp5150_read(sd, TVP5150_VD_IN_SRC_SEL_1));
#if 0
	dprintk("tvp5150: Analog channel controls = 0x%02x\n",
			tvp5150_read(sd, TVP5150_ANAL_CHL_CTL));
#endif
	dprintk(KERN_DEBUG "tvp5150: Operation mode controls = 0x%02x\n",
			tvp5150_read(sd, TVP5150_OP_MODE_CTL));
	dprintk(KERN_DEBUG "tvp5150: Miscellaneous controls = 0x%02x\n",
			tvp5150_read(sd, TVP5150_MISC_CTL));
	dprintk(KERN_DEBUG "tvp5150: Autoswitch mask= 0x%02x\n",
			tvp5150_read(sd, TVP5150_AUTOSW_MSK));
#if 0
	dprintk("tvp5150: Color killer threshold control = 0x%02x\n",
			tvp5150_read(sd, TVP5150_COLOR_KIL_THSH_CTL));
	dprintk("tvp5150: Luminance processing controls #1 #2 and #3 = %02x %02x %02x\n",
			tvp5150_read(sd, TVP5150_LUMA_PROC_CTL_1),
			tvp5150_read(sd, TVP5150_LUMA_PROC_CTL_2),
			tvp5150_read(sd, TVP5150_LUMA_PROC_CTL_3));
	printk("tvp5150: Brightness control = 0x%02x\n",
			tvp5150_read(sd, TVP5150_BRIGHT_CTL));
	printk("tvp5150: Color saturation control = 0x%02x\n",
			tvp5150_read(sd, TVP5150_SATURATION_CTL));
	printk("tvp5150: Hue control = 0x%02x\n",
			tvp5150_read(sd, TVP5150_HUE_CTL));
	printk("tvp5150: Contrast control = 0x%02x\n",
			tvp5150_read(sd, TVP5150_CONTRAST_CTL));
#endif
	dprintk(KERN_DEBUG "tvp5150: Outputs and data rates select = 0x%02x\n",
			tvp5150_read(sd, TVP5150_DATA_RATE_SEL));
	dprintk(KERN_DEBUG "tvp5150: Configuration shared pins = 0x%02x\n",
			tvp5150_read(sd, TVP5150_CONF_SHARED_PIN));
#if 0
	printk("tvp5150: Active video cropping start = 0x%02x%02x\n",
			tvp5150_read(sd, TVP5150_ACT_VD_CROP_ST_MSB),
			tvp5150_read(sd, TVP5150_ACT_VD_CROP_ST_LSB));
	printk("tvp5150: Active video cropping stop  = 0x%02x%02x\n",
			tvp5150_read(sd, TVP5150_ACT_VD_CROP_STP_MSB),
			tvp5150_read(sd, TVP5150_ACT_VD_CROP_STP_LSB));
	printk("tvp5150: Genlock/RTC = 0x%02x\n",
			tvp5150_read(sd, TVP5150_GENLOCK));
	printk("tvp5150: Horizontal sync start = 0x%02x\n",
			tvp5150_read(sd, TVP5150_HORIZ_SYNC_START));
	printk("tvp5150: Vertical blanking start = 0x%02x\n",
			tvp5150_read(sd, TVP5150_VERT_BLANKING_START));
	printk("tvp5150: Vertical blanking stop = 0x%02x\n",
			tvp5150_read(sd, TVP5150_VERT_BLANKING_STOP));
	printk("tvp5150: Chrominance processing control #1 and #2 = %02x %02x\n",
			tvp5150_read(sd, TVP5150_CHROMA_PROC_CTL_1),
			tvp5150_read(sd, TVP5150_CHROMA_PROC_CTL_2));
	printk("tvp5150: Interrupt reset register B = 0x%02x\n",
			tvp5150_read(sd, TVP5150_INT_RESET_REG_B));
	printk("tvp5150: Interrupt enable register B = 0x%02x\n",
			tvp5150_read(sd, TVP5150_INT_ENABLE_REG_B));
	printk("tvp5150: Interrupt configuration register B = 0x%02x\n",
			tvp5150_read(sd, TVP5150_INTT_CONFIG_REG_B));
#endif
	dprintk(KERN_DEBUG "tvp5150: Video standard = 0x%02x\n",
			tvp5150_read(sd, TVP5150_VIDEO_STD));
#if 0
	printk("tvp5150: Chroma gain factor: Cb=0x%02x Cr=0x%02x\n",
			tvp5150_read(sd, TVP5150_CB_GAIN_FACT),
			tvp5150_read(sd, TVP5150_CR_GAIN_FACTOR));
	printk("tvp5150: Macrovision on counter = 0x%02x\n",
			tvp5150_read(sd, TVP5150_MACROVISION_ON_CTR));
	printk("tvp5150: Macrovision off counter = 0x%02x\n",
			tvp5150_read(sd, TVP5150_MACROVISION_OFF_CTR));
	printk("tvp5150: ITU-R BT.656.%d timing(TVP5150AM1 only)\n",
			(tvp5150_read(sd, TVP5150_REV_SELECT) & 1) ? 3 : 4);
	printk("tvp5150: Device ID = %02x%02x\n",
			tvp5150_read(sd, TVP5150_MSB_DEV_ID),
			tvp5150_read(sd, TVP5150_LSB_DEV_ID));
	printk("tvp5150: ROM version = (hex) %02x.%02x\n",
			tvp5150_read(sd, TVP5150_ROM_MAJOR_VER),
			tvp5150_read(sd, TVP5150_ROM_MINOR_VER));
#endif
	dprintk("tvp5150: Vertical line count = 0x%02x%02x\n",
			tvp5150_read(sd, TVP5150_VERT_LN_COUNT_MSB),
			tvp5150_read(sd, TVP5150_VERT_LN_COUNT_LSB));
#if 0
	printk("tvp5150: Interrupt status register B = 0x%02x\n",
			tvp5150_read(sd, TVP5150_INT_STATUS_REG_B));
	printk("tvp5150: Interrupt active register B = 0x%02x\n",
			tvp5150_read(sd, TVP5150_INT_ACTIVE_REG_B));
#endif
	dprintk("tvp5150: Status regs #1 to #5 = %02x %02x %02x %02x %02x\n",
			tvp5150_read(sd, TVP5150_STATUS_REG_1),
			tvp5150_read(sd, TVP5150_STATUS_REG_2),
			tvp5150_read(sd, TVP5150_STATUS_REG_3),
			tvp5150_read(sd, TVP5150_STATUS_REG_4),
			tvp5150_read(sd, TVP5150_STATUS_REG_5));
#if 0
	dump_reg_range(sd, "Teletext filter 1",   TVP5150_TELETEXT_FIL1_INI,
			TVP5150_TELETEXT_FIL1_END, 8);
	dump_reg_range(sd, "Teletext filter 2",   TVP5150_TELETEXT_FIL2_INI,
			TVP5150_TELETEXT_FIL2_END, 8);

	printk("tvp5150: Teletext filter enable = 0x%02x\n",
			tvp5150_read(sd, TVP5150_TELETEXT_FIL_ENA));
#endif
	dprintk("tvp5150: Interrupt status register A = 0x%02x\n",
			tvp5150_read(sd, TVP5150_INT_STATUS_REG_A));
#if 0
	printk("tvp5150: Interrupt enable register A = 0x%02x\n",
			tvp5150_read(sd, TVP5150_INT_ENABLE_REG_A));
	printk("tvp5150: Interrupt configuration = 0x%02x\n",
			tvp5150_read(sd, TVP5150_INT_CONF));
	printk("tvp5150: VDP status register = 0x%02x\n",
			tvp5150_read(sd, TVP5150_VDP_STATUS_REG));
	printk("tvp5150: FIFO word count = 0x%02x\n",
			tvp5150_read(sd, TVP5150_FIFO_WORD_COUNT));
	printk("tvp5150: FIFO interrupt threshold = 0x%02x\n",
			tvp5150_read(sd, TVP5150_FIFO_INT_THRESHOLD));
	printk("tvp5150: FIFO reset = 0x%02x\n",
			tvp5150_read(sd, TVP5150_FIFO_RESET));
	printk("tvp5150: Line number interrupt = 0x%02x\n",
			tvp5150_read(sd, TVP5150_LINE_NUMBER_INT));
	printk("tvp5150: Pixel alignment register = 0x%02x%02x\n",
			tvp5150_read(sd, TVP5150_PIX_ALIGN_REG_HIGH),
			tvp5150_read(sd, TVP5150_PIX_ALIGN_REG_LOW));
	printk("tvp5150: FIFO output control = 0x%02x\n",
			tvp5150_read(sd, TVP5150_FIFO_OUT_CTRL));
	printk("tvp5150: Full field enable = 0x%02x\n",
			tvp5150_read(sd, TVP5150_FULL_FIELD_ENA));
	printk("tvp5150: Full field mode register = 0x%02x\n",
			tvp5150_read(sd, TVP5150_FULL_FIELD_MODE_REG));

	dump_reg_range(sd, "CC   data",   TVP5150_CC_DATA_INI,
			TVP5150_CC_DATA_END, 8);

	dump_reg_range(sd, "WSS  data",   TVP5150_WSS_DATA_INI,
			TVP5150_WSS_DATA_END, 8);

	dump_reg_range(sd, "VPS  data",   TVP5150_VPS_DATA_INI,
			TVP5150_VPS_DATA_END, 8);

	dump_reg_range(sd, "VITC data",   TVP5150_VITC_DATA_INI,
			TVP5150_VITC_DATA_END, 10);

	dump_reg_range(sd, "Line mode",   TVP5150_LINE_MODE_INI,
			TVP5150_LINE_MODE_END, 8);
#endif
	return 0;
}
#endif // eof TVP5151_DEBUG

/****************************************************************************
			Basic functions
 ****************************************************************************/
 /**
 * tvp515x_configure - default configuration of the TVP515x
 * @s: pointer to standard V4L2 device structure
 *
 * Configures the TVP515x for normal operation
 */
static void tvp515x_configure(struct v4l2_int_device *s)
{
	struct tvp5150_decoder *decoder = s->priv;
	struct i2c_client *sd = decoder->i2c_client;
	unsigned char misc_reg;
	unsigned char vidin_sel_reg;
	
	if (decoder->pdata->digital_input_mode == TVP5150_DISCRETE_SYNCS)
	{
		// We use HS/VS sync signals. Enable CLK, DATA and Hs/VS
		misc_reg = TVP5150_MISC_CLK_EN | TVP5150_MISC_VBLK_EN | \
				  TVP5150_MISC_SYNCS_EN | TVP5150_MISC_YCbCr_EN;
	}
	else
	{
		// We are in BT656 mode. Just enable CLK and DATA
		misc_reg = TVP5150_MISC_CLK_EN | TVP5150_MISC_YCbCr_EN;
	}	
	
	// misc controls
 	tvp5150_write(sd, TVP5150_MISC_CTL, misc_reg);

	// autoswitch mask register: enable all formats
 	tvp5150_write(sd, TVP5150_AUTOSW_MSK, 0xc0);


#ifdef AUTODETECT
	misc_reg = 0;
#else
	misc_reg = VIDEO_STD_NTSC_MJ_BIT; // Force NTSC M, J
	//misc_reg = VIDEO_STD_PAL_BDGHIN_BIT; // force PAL B,G, H, I, N
#endif
 	tvp5150_write(sd, TVP5150_VIDEO_STD, misc_reg);


	// Select analog input channel
	switch(decoder->pdata->analog_input_channel)
	{
		case TVP5150_COMPOSITE0:
			// select composite A input
			vidin_sel_reg = TVP5150_VDIN_COMPOSITEA;
			break;
		case TVP5150_COMPOSITE1:	
			// select composite B input
			vidin_sel_reg = TVP5150_VDIN_COMPOSITEB;	
			break;
		case TVP5150_SVIDEO:
			// select SVIDEO input
			vidin_sel_reg = TVP5150_VDIN_SVIDEO;	
			break;
		default:
			// select composite A input
			vidin_sel_reg = TVP5150_VDIN_COMPOSITEA;	
			break;
	}

#ifdef FORCE_BLACK_OUTPUT
	vidin_sel_reg |= TVP5150_BLACKOUTPUT; // TEST: force black screen output
 #endif
  	tvp5150_write(sd, TVP5150_VD_IN_SRC_SEL_1, vidin_sel_reg); 
	
#ifdef TVP5151_DEBUG
	tvp5150_log_status(s);	
#endif
}

 /**
 * tvp515x_start - enable the TVP5151 outputs
 * @s: pointer to standard V4L2 device structure
 *
 * Enables the TVP5151 outputs
 */
static void tvp515x_start(struct v4l2_int_device *s)
{

	struct tvp5150_decoder *decoder;
	struct i2c_client *sd;
	unsigned char reg_value = 0;

	decoder =  s->priv;
	sd = decoder->i2c_client;

	if (decoder->pdata->digital_input_mode == TVP5150_DISCRETE_SYNCS)
	{
		reg_value = TVP5150_OUTPUT_EXTENDED_CODE_RANGE | \
					TVP5150_OUTPUT_DISCRETE_SYNC;
	}
	else
	{
		reg_value = TVP5150_OUTPUT_EXTENDED_CODE_RANGE | \
					TVP5150_OUTPUT_BT656_MODE;
	}	

	// Configures outputs and data rates select register
 	tvp5150_write(sd, TVP5150_DATA_RATE_SEL, reg_value);

	// Paranoid check - Is TVP5151 locked to Analog signal?
	reg_value = tvp5150_read(sd, TVP5150_INT_STATUS_REG_A);
	
	if (! (reg_value & TVP5150_LOCKED_TO_VIDEO_SIGNAL)) {
		printk(KERN_ERR "/!/ %s TVP515x NOT LOCKED TO VIDEO SIGNAL /!/ \n", TVP5150_MOD_NAME);
		// should we return an error here?
	}

#ifdef TVP5151_DEBUG
	tvp5150_log_status(s);	
#endif
}

 /**
 * tvp515x_start - enable the TVP5151 outputs
 * @s: pointer to standard V4L2 device structure
 *
 * Enables the TVP5151 outputs
 */
static void tvp515x_stop(struct v4l2_int_device *s)
{
	struct tvp5150_decoder *decoder;
	struct i2c_client *sd;

	decoder =  s->priv;
	sd = decoder->i2c_client;
}

/*
 * tvp5150_get_current_std:
 * Returns the current standard detected by TVP5150
 */
static enum tvp5150_std tvp5150_get_current_std(struct tvp5150_decoder
						*decoder)
{
	u8 std, std_status;
	enum tvp5150_std current_std;
	struct i2c_client *sd = decoder->i2c_client;

	std = tvp5150_read(sd, TVP5150_VIDEO_STD);

	if ((std & VIDEO_STD_MASK) == VIDEO_STD_AUTO_SWITCH_BIT) {
		/* use the standard status register */
		std_status = tvp5150_read(sd,
				TVP5150_STATUS_REG_5);
		std_status += 1;
	} else
	{
		std_status = std;	/* use the standard register itself */
	}

	switch (std_status & VIDEO_STD_MASK) {
	case VIDEO_STD_NTSC_MJ_BIT:
		current_std = STD_NTSC_MJ;
		break;

	case VIDEO_STD_PAL_BDGHIN_BIT:
		current_std = STD_PAL_BDGHIN;
		break;

	case VIDEO_STD_PAL_M_BIT:
		current_std = STD_PAL_M;
		break;

	case VIDEO_STD_PAL_COMBINATION_N_BIT:
		current_std = STD_PAL_CombN;
		break;

	case VIDEO_STD_NTSC_4_43_BIT:
		current_std = STD_NTSC_443;
		break;

	case VIDEO_STD_SECAM_BIT:
		current_std = STD_SECAM;
		break;

	default:
		current_std = STD_INVALID;
		break;
	}

	dprintk(KERN_DEBUG "tvp5150_get_current_std = %d\n", current_std);
	return current_std;
}

/**
 * ioctl_querystd - V4L2 decoder interface handler for VIDIOC_QUERYSTD ioctl
 * @s: pointer to standard V4L2 device structure
 * @std_id: standard V4L2 std_id ioctl enum
 *
 * Returns the current standard detected by TVP5151. If no active input is
 * detected, returns -EINVAL
 */
static int ioctl_querystd(struct v4l2_int_device *s, v4l2_std_id *std_id)
{
	struct tvp5150_decoder *decoder = s->priv;
	enum tvp5150_std current_std;

	if (std_id == NULL)
		return -EINVAL;

	/* get the current standard */
	current_std = tvp5150_get_current_std(decoder);

	dprintk(KERN_DEBUG "%s  ioctl_querystd  current std = %d\n", TVP5150_MOD_NAME, (int)current_std);

	if (current_std == STD_INVALID)
		return -EINVAL;

	*std_id = decoder->std_list[current_std].standard.id;

	return 0;
}

static int tvp5150_set_std(struct v4l2_int_device *s, v4l2_std_id std)
{
	int fmt = 0;

	struct tvp5150_decoder *decoder = s->priv;
	struct i2c_client *sd = decoder->i2c_client;
	decoder->norm = std;

	/* First tests should be against specific std */

	if (std == V4L2_STD_ALL) {
		fmt = VIDEO_STD_AUTO_SWITCH_BIT;	/* Autodetect mode */
	} else if (std & V4L2_STD_NTSC_443) {
		fmt = VIDEO_STD_NTSC_4_43_BIT;
	} else if (std & V4L2_STD_PAL_M) {
		fmt = VIDEO_STD_PAL_M_BIT;
	} else if (std & (V4L2_STD_PAL_N | V4L2_STD_PAL_Nc)) {
		fmt = VIDEO_STD_PAL_COMBINATION_N_BIT;
	} else {
		/* Then, test against generic ones */
		if (std & V4L2_STD_NTSC)
			fmt = 0x2;
		else if (std & V4L2_STD_PAL)
			fmt = VIDEO_STD_PAL_BDGHIN_BIT;
		else if (std & V4L2_STD_SECAM)
			fmt = VIDEO_STD_SECAM_BIT;
	}

	dprintk(KERN_DEBUG "Set video std register to %d.\n", fmt);
	tvp5150_write(sd, TVP5150_VIDEO_STD, fmt);
	return 0;
}

static int ioctl_s_std(struct v4l2_int_device *s, v4l2_std_id std)
{
	struct tvp5150_decoder *decoder = s->priv;

	if (decoder->norm == std)
		return 0;

	return tvp5150_set_std(s, std);
}

/**
 * ioctl_g_ctrl - V4L2 sensor interface handler for VIDIOC_G_CTRL ioctl
 * @s: pointer to standard V4L2 device structure
 * @vc: standard V4L2 VIDIOC_G_CTRL ioctl structure
 *
 * If the requested control is supported, returns the control's current
 * value from the imx046sensor_video_control[] array.
 * Otherwise, returns -EINVAL if the control is not supported.
 */
static int ioctl_g_ctrl(struct v4l2_int_device *s, struct v4l2_control *ctrl)
{
	struct tvp5150_decoder *decoder = s->priv;
	struct i2c_client *sd = decoder->i2c_client;

	switch (ctrl->id) {
	case V4L2_CID_BRIGHTNESS:
		ctrl->value = tvp5150_read(sd, TVP5150_BRIGHT_CTL);
		return 0;
	case V4L2_CID_CONTRAST:
		ctrl->value = tvp5150_read(sd, TVP5150_CONTRAST_CTL);
		return 0;
	case V4L2_CID_SATURATION:
		ctrl->value = tvp5150_read(sd, TVP5150_SATURATION_CTL);
		return 0;
	case V4L2_CID_HUE:
		ctrl->value = tvp5150_read(sd, TVP5150_HUE_CTL);
		return 0;
	}
	return -EINVAL;
}

/**
 * ioctl_s_ctrl - V4L2 sensor interface handler for VIDIOC_S_CTRL ioctl
 * @s: pointer to standard V4L2 device structure
 * @vc: standard V4L2 VIDIOC_S_CTRL ioctl structure
 *
 * If the requested control is supported, sets the control's current
 * value in HW (and updates the imx046sensor_video_control[] array).
 * Otherwise, * returns -EINVAL if the control is not supported.
 */
static int ioctl_s_ctrl(struct v4l2_int_device *s, struct v4l2_control *ctrl)
{
	struct tvp5150_decoder *decoder = s->priv;
	struct i2c_client *sd = decoder->i2c_client;
	u8 i;

	for (i = 0; i < ARRAY_SIZE(tvp5150_qctrl); i++) {
		if (ctrl->id != tvp5150_qctrl[i].id)
			continue;
		if (ctrl->value < tvp5150_qctrl[i].minimum ||
		    ctrl->value > tvp5150_qctrl[i].maximum)
			return -ERANGE;
		dprintk(KERN_DEBUG "ioctl_s_ctrl: id=%d, value=%d\n",
					ctrl->id, ctrl->value);
		break;
	}
	
	switch (ctrl->id) {
	case V4L2_CID_BRIGHTNESS:
		tvp5150_write(sd, TVP5150_BRIGHT_CTL, ctrl->value);
		return 0;
	case V4L2_CID_CONTRAST:
		tvp5150_write(sd, TVP5150_CONTRAST_CTL, ctrl->value);
		return 0;
	case V4L2_CID_SATURATION:
		tvp5150_write(sd, TVP5150_SATURATION_CTL, ctrl->value);
		return 0;
	case V4L2_CID_HUE:
		tvp5150_write(sd, TVP5150_HUE_CTL, ctrl->value);
		return 0;
	}
	return -EINVAL;
}


/****************************************************************************
			V4L2 INT functions
 ****************************************************************************/
/**
 * ioctl_enum_fmt_cap - Implement the CAPTURE buffer VIDIOC_ENUM_FMT ioctl
 * @s: pointer to standard V4L2 device structure
 * @fmt: standard V4L2 VIDIOC_ENUM_FMT ioctl structure
 *
 * Implement the VIDIOC_ENUM_FMT ioctl to enumerate supported formats
 */
static int
ioctl_enum_fmt_cap(struct v4l2_int_device *s, struct v4l2_fmtdesc *fmt)
{
	struct tvp5150_decoder *decoder = s->priv;
	int index;

	if (fmt == NULL)
		return -EINVAL;

	index = fmt->index;
	if ((index >= decoder->num_fmts) || (index < 0))
		return -EINVAL;	/* Index out of bound */

	if (fmt->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;	/* only capture is supported */

	memcpy(fmt, &decoder->fmt_list[index],
		sizeof(struct v4l2_fmtdesc));

	dprintk(KERN_DEBUG
			"Current FMT: index - %d (%s)\n",
			decoder->fmt_list[index].index,
			decoder->fmt_list[index].description);
	return 0;
}

/**
 * ioctl_try_fmt_cap - Implement the CAPTURE buffer VIDIOC_TRY_FMT ioctl
 * @s: pointer to standard V4L2 device structure
 * @f: pointer to standard V4L2 VIDIOC_TRY_FMT ioctl structure
 *
 * Implement the VIDIOC_TRY_FMT ioctl for the CAPTURE buffer type. This
 * ioctl is used to negotiate the image capture size and pixel format
 * without actually making it take effect.
 */
static int
ioctl_try_fmt_cap(struct v4l2_int_device *s, struct v4l2_format *f)
{
	struct tvp5150_decoder *decoder = s->priv;
	int ifmt;
	struct v4l2_pix_format *pix;
	enum tvp5150_std current_std;

	if (f == NULL)
		return -EINVAL;

	if (f->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		f->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	pix = &f->fmt.pix;

	/* Calculate height and width based on current standard */
	current_std = tvp5150_get_current_std(decoder);
	if (current_std == STD_INVALID)
		return -EINVAL;


	decoder->current_std = current_std;
	pix->width = decoder->std_list[current_std].width;
	pix->height = decoder->std_list[current_std].height;

	for (ifmt = 0; ifmt < decoder->num_fmts; ifmt++) {
		if (pix->pixelformat ==
			decoder->fmt_list[ifmt].pixelformat)
			break;
	}
	if (ifmt == decoder->num_fmts)
		ifmt = 0;	/* None of the format matched, select default */
	pix->pixelformat = decoder->fmt_list[ifmt].pixelformat;

	pix->field = V4L2_FIELD_INTERLACED;
	pix->bytesperline = pix->width * 2;
	pix->sizeimage = pix->bytesperline * pix->height;
	pix->colorspace = V4L2_COLORSPACE_SMPTE170M;
	pix->priv = 0;

	dprintk(KERN_DEBUG 
			"Try FMT: pixelformat - %s, bytesperline - %d"
			"Width - %d, Height - %d\n",
			decoder->fmt_list[ifmt].description, pix->bytesperline,
			pix->width, pix->height);
	return 0;
}


/**
 * ioctl_s_fmt_cap - V4L2 sensor interface handler for VIDIOC_S_FMT ioctl
 * @s: pointer to standard V4L2 device structure
 * @f: pointer to standard V4L2 VIDIOC_S_FMT ioctl structure
 *
 * If the requested format is supported, configures the HW to use that
 * format, returns error code if format not supported or HW can't be
 * correctly configured.
 */
static int ioctl_s_fmt_cap(struct v4l2_int_device *s, struct v4l2_format *f)
{
	struct tvp5150_decoder *decoder = s->priv;
	struct v4l2_pix_format *pix;
	int rval;

	if (f == NULL)
		return -EINVAL;

	if (f->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;	/* only capture is supported */

	pix = &f->fmt.pix;
	rval = ioctl_try_fmt_cap(s, f);
	if (rval)
		return rval;

		decoder->pix = *pix;

	return rval;
}

/**
 * ioctl_g_fmt_cap - V4L2 sensor interface handler for ioctl_g_fmt_cap
 * @s: pointer to standard V4L2 device structure
 * @f: pointer to standard V4L2 v4l2_format structure
 *
 * Returns the sensor's current pixel format in the v4l2_format
 * parameter.
 */
static int ioctl_g_fmt_cap(struct v4l2_int_device *s, struct v4l2_format *f)
{
	struct tvp5150_decoder *decoder = s->priv;

	if (f == NULL)
		return -EINVAL;


	f->fmt.pix = decoder->pix;

	dprintk(KERN_DEBUG
			"Current FMT: bytesperline - %d "
			"Width - %d, Height - %d",
			decoder->pix.bytesperline,
			decoder->pix.width, decoder->pix.height);
		
	return 0;
}

/*
 * Detect if an tvp514x is present, and if so which revision.
 * A device is considered to be detected if the chip ID (LSB and MSB)
 * registers match the expected values.
 * Any value of the rom version register is accepted.
 * Returns ENODEV error number if no device is detected, or zero
 * if a device is detected.
 */
static int tvp5150_detect(struct tvp5150_decoder *decoder)
{
	struct i2c_client *sd = decoder->i2c_client;
	int rev = -EIO;	
	
	rev = tvp5150_read(sd, TVP5150_MSB_DEV_ID) << 8 |
	      tvp5150_read(sd, TVP5150_LSB_DEV_ID);

	return rev;
}

/**
 * ioctl_queryctrl - V4L2 sensor interface handler for VIDIOC_QUERYCTRL ioctl
 * @s: pointer to standard V4L2 device structure
 * @qc: standard V4L2 VIDIOC_QUERYCTRL ioctl structure
 *
 * If the requested control is supported, returns the control information
 * from the imx046sensor_video_control[] array.
 * Otherwise, returns -EINVAL if the control is not supported.
 */
static int ioctl_queryctrl(struct v4l2_int_device *s, struct v4l2_queryctrl *qc)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(tvp5150_qctrl); i++)
		if (qc->id && qc->id == tvp5150_qctrl[i].id) {
			memcpy(qc, &(tvp5150_qctrl[i]),
			       sizeof(*qc));
			return 0;
		}

	return -EINVAL;
}

/**
 * ioctl_g_parm - V4L2 sensor interface handler for VIDIOC_G_PARM ioctl
 * @s: pointer to standard V4L2 device structure
 * @a: pointer to standard V4L2 VIDIOC_G_PARM ioctl structure
 *
 * Returns the sensor's video CAPTURE parameters.
 */
static int ioctl_g_parm(struct v4l2_int_device *s,
			     struct v4l2_streamparm *a)
{
	struct tvp5150_decoder *decoder = s->priv;
	struct v4l2_captureparm *cparm;
	enum tvp5150_std current_std;

	if (a == NULL)
		return -EINVAL;

	if (a->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;	/* only capture is supported */

	memset(a, 0, sizeof(*a));
	a->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	/* get the current standard */
	current_std = tvp5150_get_current_std(decoder);
	if (current_std == STD_INVALID)
		return -EINVAL;

	decoder->current_std = current_std;

	cparm = &a->parm.capture;
	cparm->capability = V4L2_CAP_TIMEPERFRAME;
	cparm->timeperframe =
		decoder->std_list[current_std].standard.frameperiod;

	return 0;
}

/**
 * ioctl_s_parm - V4L2 sensor interface handler for VIDIOC_S_PARM ioctl
 * @s: pointer to standard V4L2 device structure
 * @a: pointer to standard V4L2 VIDIOC_S_PARM ioctl structure
 *
 * Configures the sensor to use the input parameters, if possible.  If
 * not possible, reverts to the old parameters and returns the
 * appropriate error code.
 */


static int ioctl_s_parm(struct v4l2_int_device *s,
			     struct v4l2_streamparm *a)
{
	struct tvp5150_decoder *decoder = s->priv;
	struct v4l2_fract *timeperframe;
	enum tvp5150_std current_std;

	if (a == NULL)
		return -EINVAL;

	if (a->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;	/* only capture is supported */

	timeperframe = &a->parm.capture.timeperframe;

	/* get the current standard */
	current_std = tvp5150_get_current_std(decoder);
	if (current_std == STD_INVALID)
		return -EINVAL;

	decoder->current_std = current_std;

	*timeperframe =
	    decoder->std_list[current_std].standard.frameperiod;

	return 0;
}


/**
 * ioctl_g_priv - V4L2 sensor interface handler for vidioc_int_g_priv_num
 * @s: pointer to standard V4L2 device structure
 * @p: void pointer to hold sensor's private data address
 *
 * Returns device's (sensor's) private data area address in p parameter
 */
static int ioctl_g_priv(struct v4l2_int_device *s, void *p)
{
	struct tvp5150_decoder *sensor = s->priv;
	return sensor->pdata->priv_data_set(s, p);
}

/**
 * ioctl_s_power - V4L2 sensor interface handler for vidioc_int_s_power_num
 * @s: pointer to standard V4L2 device structure
 * @on: power state to which device is to be set
 *
 * Sets devices power state to requrested state, if possible.
 */
static int ioctl_s_power(struct v4l2_int_device *s, enum v4l2_power on)
{
	struct tvp5150_decoder *decoder = s->priv;
	int err = 0;
	
	if (decoder->state == on) {
		printk(KERN_DEBUG "NO CHANGE IN POWER STATE");
		return 0;
	}

	switch (on) {
	case V4L2_POWER_OFF:
		/* Power Down Sequence */
		tvp515x_stop(s);
		printk(KERN_DEBUG "V4L2_POWER_OFF\n");
		/* Power down TVP5151 */
		if (decoder->pdata->power_set)
			err = decoder->pdata->power_set(s, on);
		break;

	case V4L2_POWER_STANDBY:
		printk(KERN_DEBUG "V4L2_POWER_STANDBY\n");
		if (decoder->pdata->power_set)
			err = decoder->pdata->power_set(s, on);
		
		/* Configure platform */
		tvp515x_configure(s);
		break;

	case V4L2_POWER_ON:
		printk(KERN_DEBUG "V4L2_POWER_ON\n");
		err = decoder->pdata->power_set(s, on);
		
		/* Power Up Sequence */
		tvp515x_start(s);
		break;

	default:
		err = -ENODEV;
		break;
	}

	decoder->state = on;

	return err;
}

/**
 * ioctl_init - V4L2 sensor interface handler for VIDIOC_INT_INIT
 * @s: pointer to standard V4L2 device structure
 *
 * Initialize the sensor device (call imx046_configure())
 */
static int ioctl_init(struct v4l2_int_device *s)
{
//	tvp5150_reset(s, 1);
	return 0;
}

/**
 * ioctl_dev_exit - V4L2 sensor interface handler for vidioc_int_dev_exit_num
 * @s: pointer to standard V4L2 device structure
 *
 * Delinitialise the dev. at slave detach.  The complement of ioctl_dev_init.
 */
static int ioctl_dev_exit(struct v4l2_int_device *s)
{
	return 0;
}

/**
 * ioctl_dev_init - V4L2 sensor interface handler for vidioc_int_dev_init_num
 * @s: pointer to standard V4L2 device structure
 *
 * Initialise the device when slave attaches to the master.  Returns 0 if
 * imx046 device could be found, otherwise returns appropriate error.
 */
static int ioctl_dev_init(struct v4l2_int_device *s)
{
	int rev = -EIO;
	struct tvp5150_decoder *decoder = s->priv;
	
	rev = tvp5150_detect(decoder);

	if (rev < 0) {
		printk( KERN_ERR "Unable to detect decoder\n");
	}
	else {
		printk( KERN_INFO "Video decoder chip id %x detected!\n", rev);
		rev = 0;
	}

	return rev;
}

/**
 * ioctl_enum_framesizes - V4L2 sensor if handler for vidioc_int_enum_framesizes
 * @s: pointer to standard V4L2 device structure
 * @frms: pointer to standard V4L2 framesizes enumeration structure
 *
 * Returns possible framesizes depending on choosen pixel format
 **/
static int ioctl_enum_framesizes(struct v4l2_int_device *s,
					struct v4l2_frmsizeenum *frms)
{
	int ifmt;

	for (ifmt = 0; ifmt < NUM_CAPTURE_FORMATS; ifmt++) {
		if (frms->pixel_format == tvp5150_fmt_list[ifmt].pixelformat)
			break;
	}
	/* Is requested pixelformat not found on sensor? */
	if (ifmt == NUM_CAPTURE_FORMATS)
		return -EINVAL;

	/* Check that the index we are being asked for is not
	   out of bounds. */
	if (frms->index >= NUM_OF_SUPPORTED_SIZES)
		return -EINVAL;

	frms->type = V4L2_FRMSIZE_TYPE_DISCRETE;
	frms->discrete.width = tvp515x_std_list[frms->index].width;
	frms->discrete.height = tvp515x_std_list[frms->index].height;

	return 0;

}

static int ioctl_enum_frameintervals(struct v4l2_int_device *s,
					struct v4l2_frmivalenum *frmi)
{
	int ifmt;

	/* Check that the requested format is one we support */
	for (ifmt = 0; ifmt < NUM_CAPTURE_FORMATS; ifmt++) {
		if (frmi->pixel_format == tvp5150_fmt_list[ifmt].pixelformat)
			break;
	}

	if (ifmt == NUM_CAPTURE_FORMATS)
		return -EINVAL;

	/* Check that the index we are being asked for is not
	   out of bounds. */
	if (frmi->index >=NUM_OF_SUPPORTED_SIZES)
		return -EINVAL;

	frmi->type = V4L2_FRMIVAL_TYPE_DISCRETE;
	frmi->discrete.numerator =
				tvp515x_std_list[frmi->index].standard.frameperiod.numerator;
	frmi->discrete.denominator =
				tvp515x_std_list[frmi->index].standard.frameperiod.denominator;

	return 0;
}

static struct v4l2_int_ioctl_desc tvp5150_ioctl_desc[] = {
	{ .num = vidioc_int_enum_framesizes_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_enum_framesizes},
	{ .num = vidioc_int_enum_frameintervals_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_enum_frameintervals},
	{ .num = vidioc_int_dev_init_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_dev_init},
	{ .num = vidioc_int_dev_exit_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_dev_exit},
	{ .num = vidioc_int_s_power_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_s_power },
	{ .num = vidioc_int_g_priv_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_g_priv },
	{ .num = vidioc_int_init_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_init },
	{ .num = vidioc_int_enum_fmt_cap_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_enum_fmt_cap },
	{ .num = vidioc_int_try_fmt_cap_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_try_fmt_cap },
	{ .num = vidioc_int_g_fmt_cap_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_g_fmt_cap },
	{ .num = vidioc_int_s_fmt_cap_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_s_fmt_cap },
	{ .num = vidioc_int_g_parm_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_g_parm },
	{ .num = vidioc_int_s_parm_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_s_parm },
	{ .num = vidioc_int_queryctrl_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_queryctrl },
	{ .num = vidioc_int_g_ctrl_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_g_ctrl },
	{ .num = vidioc_int_s_ctrl_num,
	  .func = (v4l2_int_ioctl_func *)ioctl_s_ctrl },
	{.num = vidioc_int_querystd_num, 
	  .func =(v4l2_int_ioctl_func *) ioctl_querystd},
	{.num = vidioc_int_s_std_num, 
	  .func =(v4l2_int_ioctl_func *) ioctl_s_std},
};

static struct v4l2_int_slave tvp5150_slave = {
	.ioctls = tvp5150_ioctl_desc,
	.num_ioctls = ARRAY_SIZE(tvp5150_ioctl_desc),
};

static struct v4l2_int_device tvp5150_int_device = {
	.module = THIS_MODULE,
	.name = "tvp5150",
	.priv = &tvp5150,
	.type = v4l2_int_type_slave,
	.u = {
		.slave = &tvp5150_slave,
	},
};

/****************************************************************************
			I2C Client & Driver
 ****************************************************************************/
static  int tvp5150_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	int err = 0;
	struct tvp5150_decoder *decoder = &tvp5150;

	/* Check if the adapter supports the needed features */
	if (!i2c_check_functionality(client->adapter,
	     I2C_FUNC_SMBUS_READ_BYTE | I2C_FUNC_SMBUS_WRITE_BYTE_DATA))
		return -EIO;

	if (i2c_get_clientdata(client))
		return -EBUSY;

	decoder->pdata = client->dev.platform_data;

	if (!decoder->pdata) {
		v4l_err(client, "no platform data?\n");
		return -ENODEV;
	}

	decoder->v4l2_int_device = &tvp5150_int_device;
	decoder->i2c_client = client;

	i2c_set_clientdata(client, decoder);

	err = v4l2_int_device_register(&tvp5150_int_device);
	if (err) {
		printk(KERN_ERR "Failed to register" TVP5150_DRIVER_NAME ".\n");
		i2c_set_clientdata (client, NULL);		

		return err;
	} else {
		printk(KERN_INFO "Registered " TVP5150_DRIVER_NAME " video decoder\n");	
	}

	decoder->input = decoder->pdata->analog_input_channel;
	decoder->enable = 1;
	decoder->bright = 128;
	decoder->contrast = 128;
	decoder->hue = 0;
	decoder->sat = 128;

	/* Default to NTSC 8-bit YUV 422 */
	decoder->norm = V4L2_STD_NTSC;
	decoder->pix.width = NTSC_NUM_ACTIVE_PIXELS;
	decoder->pix.height = NTSC_NUM_ACTIVE_LINES;
	decoder->pix.pixelformat = V4L2_PIX_FMT_UYVY;
	decoder->pix.field = V4L2_FIELD_INTERLACED;
	decoder->pix.bytesperline = NTSC_NUM_ACTIVE_PIXELS * 2;
	decoder->pix.sizeimage = NTSC_NUM_ACTIVE_PIXELS * 2 * NTSC_NUM_ACTIVE_LINES;
	decoder->pix.colorspace = V4L2_COLORSPACE_SMPTE170M;

	decoder->fmt_list = tvp5150_fmt_list;
	decoder->num_fmts = ARRAY_SIZE(tvp5150_fmt_list);

	decoder->std_list = tvp515x_std_list;
	decoder->num_stds = ARRAY_SIZE(tvp515x_std_list);

	decoder->state = V4L2_POWER_OFF;

	return 0;
}


/**
 * imx046_remove - sensor driver i2c remove handler
 * @client: i2c driver client device structure
 *
 * Unregister sensor as an i2c client device and V4L2
 * device.  Complement of imx046_probe().
 */
static int tvp5150_remove(struct i2c_client *client)
{
	struct tvp5150_decoder *sensor = i2c_get_clientdata(client);

	if (!client->adapter)
		return -ENODEV;	/* our client isn't attached */

	v4l2_int_device_unregister(sensor->v4l2_int_device);
	i2c_set_clientdata(client, NULL);

	return 0;
}

/**
 * tvp5150_suspend - sensor driver i2c suspend handler
 */
static int tvp5150_suspend(struct i2c_client *client, pm_message_t state)
{
	int err = 0;
	struct tvp5150_decoder *decoder = i2c_get_clientdata(client);	
	struct v4l2_int_device *s = decoder->v4l2_int_device;
	
	if (decoder->pdata->suspend)
		err = decoder->pdata->suspend(s);

	return err;
}

/**
 * tvp5150_resume - sensor driver i2c resume handler
 */
static int tvp5150_resume(struct i2c_client *client)
{
	int err = 0;
	struct tvp5150_decoder *decoder = i2c_get_clientdata(client);	
	struct v4l2_int_device *s = decoder->v4l2_int_device;
	
	if (decoder->pdata->resume)
		err = decoder->pdata->resume(s, decoder->state);

	return err;
}

/* ----------------------------------------------------------------------- */

static const struct i2c_device_id tvp5150_id[] = {
	{ "tvp5150", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, tvp5150_id);


static struct v4l2_i2c_driver_data v4l2_i2c_data = {
	.name = "tvp5150",
	.probe = tvp5150_probe,
	.remove = tvp5150_remove,
	.suspend = tvp5150_suspend,
	.resume = tvp5150_resume,	
	.id_table = tvp5150_id,
};
