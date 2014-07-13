/* linux/drivers/media/video/samsung/g2d/fimg2d_3x.c
 *
 * Copyright  2008 Samsung Electronics Co, Ltd. All Rights Reserved. 
 *		      http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This file implements sec-g2d driver.
 */

#include <linux/init.h>

#include <linux/moduleparam.h>
#include <linux/timer.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/clk.h>
#include <asm/uaccess.h>
#include <linux/errno.h> /* error codes */
#include <asm/div64.h>
#include <linux/tty.h>
#include <asm/uaccess.h>
#include <linux/miscdevice.h>

#include <linux/version.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/slab.h>
#include <linux/poll.h>
#include <linux/signal.h>
#include <linux/ioport.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/kmod.h>
#include <linux/vmalloc.h>
#include <linux/init.h>
#include <linux/semaphore.h>

#include <asm/io.h>
#include <asm/page.h>
#include <asm/irq.h>
#include <linux/mm.h>
#include <linux/moduleparam.h>

#include <mach/hardware.h>
#include <mach/map.h>

#include "fimg2d_regs.h"
#include "fimg2d_3x.h"

//#include "plat/s5pc11x-dvfs.h"

#undef CONFIG_PM_PWR_GATING

#ifdef CONFIG_PM_PWR_GATING
#include <plat/pm.h>
#include <plat/regs-power.h>
#endif

#define G2D_CLK_GATING
//#define USE_G2D_TIMER_FOR_CLK
//#define G2D_DEBUG
//#define G2D_CHECK_HW_VERSION

extern int hw_version_check();
static int	g_hw_version = 0;

static int sec_g2d_irq_num = NO_IRQ;
static struct resource *sec_g2d_mem;
static void __iomem *sec_g2d_base;
static wait_queue_head_t waitq_g2d;


#ifdef	G2D_CLK_GATING 
static struct clk *sec_g2d_clock;
static struct clk *sec_g2d_pclock;
static struct clk *sec_g2d_hclk;
static int	g_flag_clk_enable = 0;
static int	g_num_of_g2d_object = 0;
static int	g_num_of_nonblock_object = 0;
#ifdef USE_G2D_TIMER_FOR_CLK
static struct timer_list  g_g2d_domain_timer;
static int g2d_pwr_off_flag = 0;
DEFINE_SPINLOCK(g2d_domain_lock);
#else
static struct mutex g_g2d_clk_mutex;
#endif // USE_G2D_TIMER_FOR_CLK
#endif

static struct mutex *h_rot_mutex;

static u16 sec_g2d_poll_flag 	= 0;
static u16 sec_g2d_irq_flag 	= 0;

int get_stride_from_color_space(u32 color_space)
{
	switch (color_space)
	{
		case G2D_XRGB_8888:
		case G2D_RGBX_8888:
		case G2D_XBGR_8888:
		case G2D_BGRX_8888:

		case G2D_ARGB_8888:
		case G2D_RGBA_8888:
		case G2D_ABGR_8888:
		case G2D_BGRA_8888:
			return 4;

		case G2D_PACKED_RGB_888:
		case G2D_PACKED_BGR_888:
			return 3;

		default:
			return 2;
	}
}

u32	get_color_mode_from_color_space(u32 color_space)
{
	switch (color_space)
	{
		case G2D_RGB_565:	return (G2D_CHL_ORDER_XRGB<<4)|(G2D_FMT_RGB_565);

		case G2D_XRGB_8888:	return (G2D_CHL_ORDER_XRGB<<4)|(G2D_FMT_XRGB_8888);
		case G2D_RGBX_8888:	return (G2D_CHL_ORDER_RGBX<<4)|(G2D_FMT_XRGB_8888);
		case G2D_XBGR_8888:	return (G2D_CHL_ORDER_XBGR<<4)|(G2D_FMT_XRGB_8888);
		case G2D_BGRX_8888:	return (G2D_CHL_ORDER_BGRX<<4)|(G2D_FMT_XRGB_8888);

		case G2D_ARGB_8888:	return (G2D_CHL_ORDER_XRGB<<4)|(G2D_FMT_ARGB_8888);
		case G2D_RGBA_8888:	return (G2D_CHL_ORDER_RGBX<<4)|(G2D_FMT_ARGB_8888);
		case G2D_ABGR_8888:	return (G2D_CHL_ORDER_XBGR<<4)|(G2D_FMT_ARGB_8888);
		case G2D_BGRA_8888:	return (G2D_CHL_ORDER_BGRX<<4)|(G2D_FMT_ARGB_8888);

		case G2D_XRGB_1555: return (G2D_CHL_ORDER_XRGB<<4)|(G2D_FMT_XRGB_1555); 
		case G2D_RGBX_5551:	return (G2D_CHL_ORDER_RGBX<<4)|(G2D_FMT_XRGB_1555);
		case G2D_XBGR_1555:	return (G2D_CHL_ORDER_XBGR<<4)|(G2D_FMT_XRGB_1555);
		case G2D_BGRX_5551:	return (G2D_CHL_ORDER_BGRX<<4)|(G2D_FMT_XRGB_1555);

		case G2D_ARGB_1555:	return (G2D_CHL_ORDER_XRGB<<4)|(G2D_FMT_ARGB_1555); 
		case G2D_RGBA_5551:	return (G2D_CHL_ORDER_RGBX<<4)|(G2D_FMT_ARGB_1555);
		case G2D_ABGR_1555:	return (G2D_CHL_ORDER_XBGR<<4)|(G2D_FMT_ARGB_1555);
		case G2D_BGRA_5551:	return (G2D_CHL_ORDER_BGRX<<4)|(G2D_FMT_ARGB_1555);

		case G2D_XRGB_4444:	return (G2D_CHL_ORDER_XRGB<<4)|(G2D_FMT_XRGB_4444);
		case G2D_RGBX_4444:	return (G2D_CHL_ORDER_RGBX<<4)|(G2D_FMT_XRGB_4444);
		case G2D_XBGR_4444:	return (G2D_CHL_ORDER_XBGR<<4)|(G2D_FMT_XRGB_4444);
		case G2D_BGRX_4444:	return (G2D_CHL_ORDER_BGRX<<4)|(G2D_FMT_XRGB_4444);

		case G2D_ARGB_4444:	return (G2D_CHL_ORDER_XRGB<<4)|(G2D_FMT_ARGB_4444);
		case G2D_RGBA_4444:	return (G2D_CHL_ORDER_RGBX<<4)|(G2D_FMT_ARGB_4444);
		case G2D_ABGR_4444:	return (G2D_CHL_ORDER_XBGR<<4)|(G2D_FMT_ARGB_4444);
		case G2D_BGRA_4444:	return (G2D_CHL_ORDER_BGRX<<4)|(G2D_FMT_ARGB_4444);

		case G2D_PACKED_RGB_888:	return (G2D_CHL_ORDER_XRGB<<4)|(G2D_FMT_PACKED_RGB_888);
		case G2D_PACKED_BGR_888:	return (G2D_CHL_ORDER_XBGR<<4)|(G2D_FMT_PACKED_RGB_888);
		default:
			return -1;
	}
}

u32	get_color_format_from_color_space(u32 color_space)
{
	switch (color_space)
	{
		case G2D_RGB_565:	
			return G2D_FMT_RGB_565;

		case G2D_XRGB_8888:
		case G2D_RGBX_8888:
		case G2D_XBGR_8888:
		case G2D_BGRX_8888:
			return G2D_FMT_XRGB_8888;

		case G2D_ARGB_8888:	
		case G2D_RGBA_8888:
		case G2D_ABGR_8888:
		case G2D_BGRA_8888:
			return G2D_FMT_ARGB_8888;

		case G2D_XRGB_1555:
		case G2D_RGBX_5551:
		case G2D_XBGR_1555:
		case G2D_BGRX_5551:
			return G2D_FMT_XRGB_1555;

		case G2D_ARGB_1555:
		case G2D_RGBA_5551:
		case G2D_ABGR_1555:
		case G2D_BGRA_5551:	
			return G2D_FMT_ARGB_1555;

		case G2D_XRGB_4444:
		case G2D_RGBX_4444:
		case G2D_XBGR_4444:
		case G2D_BGRX_4444:	
			return G2D_FMT_XRGB_4444;

		case G2D_ARGB_4444:
		case G2D_RGBA_4444:
		case G2D_ABGR_4444:
		case G2D_BGRA_4444:	
			return G2D_FMT_ARGB_4444;

		case G2D_PACKED_RGB_888:	
		case G2D_PACKED_BGR_888:	
			return G2D_FMT_PACKED_RGB_888;

		default:
			return -1;
	}
}

static void get_rot_config(unsigned int rotate_value, u32 *rot, u32 *src_dir, u32 *dst_dir)
{
	switch(rotate_value)
	{
		case SEC_G2D_ROTATOR_90: 
			*rot = 1;   	// rotation = 1, src_y_dir == dst_y_dir, src_x_dir == dst_x_dir 
			*src_dir = 0;		
			*dst_dir = 0;		
			break;

		case SEC_G2D_ROTATOR_270: 
			*rot = 1;   	// rotation = 1, src_y_dir != dst_y_dir, src_x_dir != dst_x_dir
			*src_dir = 0;
			*dst_dir = 0x3;
			break;			

		case SEC_G2D_ROTATOR_180: 
			*rot = 0;    	// rotation = 0, src_y_dir != dst_y_dir, src_x_dir != dst_x_dir
			*src_dir = 0;
			*dst_dir = 0x3;
			break;

		case SEC_G2D_ROTATOR_X_FLIP: 
			*rot = 0;    	// rotation = 0, src_y_dir != dst_y_dir
			*src_dir = 0;
			*dst_dir = 0x2;
			break;

		case SEC_G2D_ROTATOR_Y_FLIP: 
			*rot = 0;    	// rotation = 0, src_x_dir != dst_y_dir
			*src_dir = 0;
			*dst_dir = 0x1;
			break;

		default :
			*rot = 0;   	// rotation = 0;
			*src_dir = 0;
			*dst_dir = 0;
			break;
	}
	
	return ;
}

static int sec_g2d_check_params(sec_g2d_params *params)
{
	/* source */
	if (0 == params->src_work_height || 0 == params->src_work_width) return -1;
	if (8000 < params->src_start_x+params->src_work_width
			|| 8000 < params->src_start_y+params->src_work_height) return -1;

	/* destination */
	if (0 == params->dst_work_height || 0 == params->dst_work_width) return -1;
	if (8000 < params->dst_start_x+params->dst_work_width
			|| 8000 < params->dst_start_y+params->dst_work_height) return -1;

	/* clipping */
#if 0
	if (params->cw_x1 < params->dst_start_x 
			|| params->cw_x2 > params->dst_start_x+params->dst_work_width) return -1;
	if (params->cw_y1 < params->dst_start_y
			|| params->cw_y2 > params->dst_start_y+params->dst_work_height) return -1;
#endif

	if (params->src_color_space >= G2D_MAX_COLOR_SPACE
			|| params->dst_color_space >= G2D_MAX_COLOR_SPACE) return -1;

	if (TRUE == params->alpha_mode && params->alpha_val > ALPHA_VALUE_MAX) return -1; 
}

static int sec_g2d_init_regs(sec_g2d_params *params, u32 rot_degree)
{
	u32		data;
	u32 	rot_config[3];
	u32 	rot=0, src_dir=0, dst_dir=0;
	u32		blt_cmd = 0;

	/* source image */		

	// select source
	__raw_writel(G2D_SRC_SELECT_R_NORMAL, sec_g2d_base + SRC_SELECT_REG);
	// set base address of source image
	__raw_writel(params->src_base_addr, sec_g2d_base + SRC_BASE_ADDR_REG);

	// set coordinate of source image
	data	= (params->src_start_y<<16)|(params->src_start_x);
	__raw_writel(data, sec_g2d_base + SRC_LEFT_TOP_REG);
	data	= ((params->src_start_y+params->src_work_height)<<16)|(params->src_start_x+params->src_work_width);
	__raw_writel(data, sec_g2d_base + SRC_RIGHT_BOTTOM_REG);
#if 0
	printk("fimg2d: src(l=%d,t=%d,r=%d,b=%d) dst(l=%d,t=%d,r=%d,b=%d)\n", 
			params->src_start_x, params->src_start_y, params->src_start_x+params->src_work_width, params->src_start_y+params->src_work_height,
			params->dst_start_x, params->dst_start_y, params->dst_start_x+params->dst_work_width, params->dst_start_y+params->dst_work_height);
#endif

	// set color mode & stride
	data	= get_color_mode_from_color_space(params->src_color_space);
	__raw_writel(data, sec_g2d_base + SRC_COLOR_MODE_REG);
	data	= get_stride_from_color_space(params->src_color_space);
	__raw_writel(params->src_full_width*data, sec_g2d_base + SRC_STRIDE_REG);

	data	= get_color_format_from_color_space(params->src_color_space);
	if(G2D_FMT_ARGB_8888 == data || G2D_FMT_ARGB_1555 == data || G2D_FMT_ARGB_4444 == data)
		blt_cmd |= G2D_BLT_CMD_R_ALPHA_BLEND_ALPHA_BLEND | G2D_BLT_CMD_R_SRC_NON_PRE_BLEND_PERPIXEL_ALPHA;
	
	/* destination image */		

	// select destination
	__raw_writel(G2D_DST_SELECT_R_NORMAL, sec_g2d_base + DST_SELECT_REG);
	// set base address of destination image
	__raw_writel(params->dst_base_addr, sec_g2d_base + DST_BASE_ADDR_REG);

	// set coordinate of destination image
	data	= (params->dst_start_y<<16)|(params->dst_start_x);
	__raw_writel(data, sec_g2d_base + DST_LEFT_TOP_REG);
	data	= ((params->dst_start_y+params->dst_work_height)<<16)|(params->dst_start_x+params->dst_work_width);
	__raw_writel(data, sec_g2d_base + DST_RIGHT_BOTTOM_REG);

	// set color mode & stride
	data	= get_color_mode_from_color_space(params->dst_color_space);
	__raw_writel(data, sec_g2d_base + DST_COLOR_MODE_REG);
	data	= get_stride_from_color_space(params->dst_color_space);
	__raw_writel(params->dst_full_width*data , sec_g2d_base + DST_STRIDE_REG);

	/* clipping */

	if(params->cw_x1 < params->cw_x2 && params->cw_y1 < params->cw_y2)
	{
		blt_cmd	|= G2D_BLT_CMD_R_CW_ENABLE;
		__raw_writel((params->cw_y1<<16)|(params->cw_x1), sec_g2d_base + CW_LEFT_TOP_REG);
		__raw_writel((params->cw_y2<<16)|(params->cw_x2), sec_g2d_base + CW_RIGHT_BOTTOM_REG);
	}

#if 0
	/* color */

	// foreground color
	__raw_writel(params->color_val[G2D_WHITE], sec_g2d_base + FG_COLOR_REG);
	// background color
	__raw_writel(params->color_val[G2D_BLACK], sec_g2d_base + BG_COLOR_REG);
	// bluescreen color
	__raw_writel(params->color_val[G2D_BLUE], sec_g2d_base + BS_COLOR_REG);
#endif

	/* rotation */

	get_rot_config(rot_degree, &rot, &src_dir, &dst_dir);
	__raw_writel(rot, sec_g2d_base + ROTATE_REG);
	__raw_writel(src_dir, sec_g2d_base + SRC_MSK_DIRECT_REG);
	__raw_writel(dst_dir, sec_g2d_base + DST_PAT_DIRECT_REG);

	/* color key */
	if (TRUE == params->color_key_mode)
		printk("fimg2d: color key mode is not yet implemented (%dL)\n", __LINE__);

	/* ROP4 operation */
	__raw_writel((0x3<<4)|(0x3<<0), sec_g2d_base + THIRD_OPERAND_REG);
	__raw_writel((0xCC<<8)|(0xCC<<0), sec_g2d_base + ROP4_REG);

	/* alpha blending */
	if (TRUE == params->alpha_mode)
	{
		if(params->alpha_val > ALPHA_VALUE_MAX){
			printk(KERN_ALERT"fimg2d: exceed alpha value range 0~255 (value=%d) (%dL)\n", params->alpha_val, __LINE__);
	           	 return -ENOENT;
		}

		//blt_cmd |= G2D_BLT_CMD_R_ALPHA_BLEND_ALPHA_BLEND | G2D_BLT_CMD_R_SRC_NON_PRE_BLEND_CONSTANT_ALPHA;
		blt_cmd |= G2D_BLT_CMD_R_ALPHA_BLEND_ALPHA_BLEND;
		__raw_writel((params->alpha_val&0xff), sec_g2d_base + ALPHA_REG);		
	}

	/* command */
	//if ((params->src_work_width != params->dst_work_width) || (params->src_work_height != params->dst_work_height))
	blt_cmd |= G2D_BLT_CMD_R_STRETCH_ENABLE;

	__raw_writel(blt_cmd, sec_g2d_base + BITBLT_COMMAND_REG);

#if 0
	printk("fimg2d: cmd=0x%x src(fw=%d,fh=%d,x=%d,y=%d,ww=%d,wh=%d,f=%d) dst(fw=%d,fh=%d,x=%d,y=%d,ww=%d,wh=%d,f=%d) cw(%d,%d,%d,%d) alpha(%d/%d) rot=%d\n", 
			blt_cmd, 
			params->src_full_width, params->src_full_height, 
			params->src_start_x, params->src_start_y, params->src_work_width, params->src_work_height, params->src_color_space, 
			params->dst_full_width, params->dst_full_height, 
			params->dst_start_x, params->dst_start_y, params->dst_work_width, params->dst_work_height, params->dst_color_space,
			params->cw_x1, params->cw_y1, params->cw_x2, params->cw_y2,
			params->alpha_mode, params->alpha_val, rot_degree);
#endif
#if 0
	{
		u32		temp[40], name[40];	
		int		i = 0;

		name[i] = CONRTOL_REG;
		temp[i++] = __raw_readl(sec_g2d_base+CONRTOL_REG);
		name[i] = SOFT_RESET_REG;
		temp[i++] = __raw_readl(sec_g2d_base+SOFT_RESET_REG);
		name[i] = INTEN_REG;
		temp[i++] = __raw_readl(sec_g2d_base+INTEN_REG);
		name[i] = INTC_PEND_REG;
		temp[i++] = __raw_readl(sec_g2d_base+INTC_PEND_REG);
		name[i] = FIFO_STAT_REG;
		temp[i++] = __raw_readl(sec_g2d_base+FIFO_STAT_REG);
		name[i] = AXI_ID_MODE_REG;
		temp[i++] = __raw_readl(sec_g2d_base+AXI_ID_MODE_REG);
		name[i] = CACHECTL_REG;
		temp[i++] = __raw_readl(sec_g2d_base+CACHECTL_REG);

		name[i] = BITBLT_START_REG;
		temp[i++] = __raw_readl(sec_g2d_base+BITBLT_START_REG);
		name[i] = BITBLT_COMMAND_REG;
		temp[i++] = __raw_readl(sec_g2d_base+BITBLT_COMMAND_REG);

		name[i] = ROTATE_REG;
		temp[i++] = __raw_readl(sec_g2d_base+ROTATE_REG);
		name[i] = SRC_MSK_DIRECT_REG;
		temp[i++] = __raw_readl(sec_g2d_base+SRC_MSK_DIRECT_REG);
		name[i] = DST_PAT_DIRECT_REG;
		temp[i++] = __raw_readl(sec_g2d_base+DST_PAT_DIRECT_REG);

		name[i] = SRC_SELECT_REG;
		temp[i++] = __raw_readl(sec_g2d_base+SRC_SELECT_REG);
		name[i] = SRC_BASE_ADDR_REG;
		temp[i++] = __raw_readl(sec_g2d_base+SRC_BASE_ADDR_REG);
		name[i] = SRC_STRIDE_REG;
		temp[i++] = __raw_readl(sec_g2d_base+SRC_STRIDE_REG);
		name[i] = SRC_COLOR_MODE_REG;
		temp[i++] = __raw_readl(sec_g2d_base+SRC_COLOR_MODE_REG);
		name[i] = SRC_LEFT_TOP_REG;
		temp[i++] = __raw_readl(sec_g2d_base+SRC_LEFT_TOP_REG);
		name[i] = SRC_RIGHT_BOTTOM_REG;
		temp[i++] = __raw_readl(sec_g2d_base+SRC_RIGHT_BOTTOM_REG);

		name[i] = DST_SELECT_REG;
		temp[i++] = __raw_readl(sec_g2d_base+DST_SELECT_REG);
		name[i] = DST_BASE_ADDR_REG;
		temp[i++] = __raw_readl(sec_g2d_base+DST_BASE_ADDR_REG);
		name[i] = DST_STRIDE_REG;
		temp[i++] = __raw_readl(sec_g2d_base+DST_STRIDE_REG);
		name[i] = DST_COLOR_MODE_REG;
		temp[i++] = __raw_readl(sec_g2d_base+DST_COLOR_MODE_REG);
		name[i] = DST_LEFT_TOP_REG;
		temp[i++] = __raw_readl(sec_g2d_base+DST_LEFT_TOP_REG);
		name[i] = DST_RIGHT_BOTTOM_REG;
		temp[i++] = __raw_readl(sec_g2d_base+DST_RIGHT_BOTTOM_REG);

#if 0
		name[i] = PAT_BASE_ADDR_REG;
		temp[i++] = __raw_readl(sec_g2d_base+PAT_BASE_ADDR_REG);
		name[i] = PAT_SIZE_REG;
		temp[i++] = __raw_readl(sec_g2d_base+PAT_SIZE_REG);
		name[i] = PAT_COLOR_MODE_REG;
		temp[i++] = __raw_readl(sec_g2d_base+PAT_COLOR_MODE_REG);
		name[i] = PAT_OFFSET_REG;
		temp[i++] = __raw_readl(sec_g2d_base+PAT_OFFSET_REG);
		name[i] = PAT_STRIDE_REG;
		temp[i++] = __raw_readl(sec_g2d_base+PAT_STRIDE_REG);

		name[i] = MASK_BASE_ADDR_REG;
		temp[i++] = __raw_readl(sec_g2d_base+MASK_BASE_ADDR_REG);
		name[i] = MASK_STRIDE_REG;
		temp[i++] = __raw_readl(sec_g2d_base+MASK_STRIDE_REG);
#endif

		name[i] = CW_LEFT_TOP_REG;
		temp[i++] = __raw_readl(sec_g2d_base+CW_LEFT_TOP_REG);
		name[i] = CW_RIGHT_BOTTOM_REG;
		temp[i++] = __raw_readl(sec_g2d_base+CW_RIGHT_BOTTOM_REG);

		name[i] = THIRD_OPERAND_REG;
		temp[i++] = __raw_readl(sec_g2d_base+THIRD_OPERAND_REG);
		name[i] = ROP4_REG;
		temp[i++] = __raw_readl(sec_g2d_base+ROP4_REG);
		name[i] = ALPHA_REG;
		temp[i++] = __raw_readl(sec_g2d_base+ALPHA_REG);

#if 0
		name[i] = FG_COLOR_REG;
		temp[i++] = __raw_readl(sec_g2d_base+FG_COLOR_REG);
		name[i] = BG_COLOR_REG;
		temp[i++] = __raw_readl(sec_g2d_base+BG_COLOR_REG);
		name[i] = BS_COLOR_REG;
		temp[i++] = __raw_readl(sec_g2d_base+BS_COLOR_REG);
#endif

		name[i] = 0xFFFF;

		for (i=0; name[i]!=0xFFFF; i++)
		{
			printk("0x%04x:0x%08X ", name[i], temp[i]);
			if (0 == ((i+1) % 4)) printk("\n");
		}
		printk("\n");
	}
#endif

	return 0;
}

static void sec_g2d_rotate_with_bitblt(sec_g2d_params *params)
{
	// enable interrupt
	__raw_writel(0x1, sec_g2d_base + INTC_PEND_REG);
	__raw_writel(0x1, sec_g2d_base + INTEN_REG);

	sec_g2d_irq_flag 	= 0;
	__raw_writel(0x1, sec_g2d_base + BITBLT_START_REG);
}

#if 0
static int sec_g2d_rotator_start(sec_g2d_params *params, ROT_DEG rot_degree)
{
	int	ret = 0;
	ret = sec_g2d_init_regs(params, rot_degree);
	if(0 > ret) return -1;

	sec_g2d_rotate_with_bitblt(params);

	return 0;
}
#endif

irqreturn_t sec_g2d_irq(int irq, void *dev_id)
{
#if 0
//#ifdef G2D_DEBUG
	printk("fimg2d:%s: 0x%x\n", __func__, __raw_readl(sec_g2d_base + INTC_PEND_REG));
#endif
	if(__raw_readl(sec_g2d_base + INTC_PEND_REG) & G2D_INTC_PEND_R_INTP_CMD_FIN)
	{
		__raw_writel(G2D_INTC_PEND_R_INTP_CMD_FIN, sec_g2d_base + INTC_PEND_REG);
		wake_up_interruptible(&waitq_g2d);
		sec_g2d_poll_flag = 1;
		sec_g2d_irq_flag = 1;
	}
	return IRQ_HANDLED;
}

#ifdef G2D_CLK_GATING
static int sec_g2d_clk_enable(void)
{
	if(g_flag_clk_enable == 0)
	{
		// power gating
#ifdef CONFIG_PM_PWR_GATING
		s5p_power_gating(S5PC100_POWER_DOMAIN_LCD, DOMAIN_ACTIVE_MODE);
#endif

		// clock gating
		clk_enable(sec_g2d_pclock);
		clk_enable(sec_g2d_clock);

		g_flag_clk_enable = 1;

		//printk("!!!!!!!!!!!!!! g2d ON\n");
	}

	return 0;
}

static int sec_g2d_clk_disable(void)
{
	if(g_flag_clk_enable        == 1
		&& g_num_of_nonblock_object == 0)
	{
		// clock gating
		clk_disable(sec_g2d_clock);
		clk_disable(sec_g2d_pclock);

		// power gating
#ifdef CONFIG_PM_PWR_GATING
		s5p_power_gating(S5PC100_POWER_DOMAIN_LCD, DOMAIN_LP_MODE);
#endif

		g_flag_clk_enable = 0;

		//printk("!!!!!!!!!!!!! g2d OFF (jiffies=%u)\n", jiffies);
	}

	return 0;
}

#ifdef USE_G2D_TIMER_FOR_CLK
static int sec_g2d_domain_timer(void)
{
	spin_lock(&g2d_domain_lock);
	if(g2d_pwr_off_flag){
		if(    g_flag_clk_enable        == 1
                && g_num_of_nonblock_object == 0) {
                clk_disable(sec_g2d_clock);
#ifdef CONFIG_PM_PWR_GATING
                s5p_power_gating(S5PC100_POWER_DOMAIN_LCD, DOMAIN_LP_MODE);

#endif
                g_flag_clk_enable = 0;
                //printk("----------------- g2d OFF (jiffies=%u)\n", jiffies);
            }
	
	}
	else {
		if(g_flag_clk_enable        == 1
                && g_num_of_nonblock_object == 0) {
		  mod_timer(&g_g2d_domain_timer, jiffies + HZ);		
          //printk("----------------- jiffies=%u HZ=%u\n", jiffies, HZ);
		}
	}

	spin_unlock(&g2d_domain_lock);
}
#endif
#endif

 int sec_g2d_open(struct inode *inode, struct file *file)
{
	sec_g2d_params *params;

	clk_enable(sec_g2d_hclk);

#ifdef G2D_CHECK_HW_VERSION
	if (1 != g_hw_version)
		return 0;
#endif

	params = (sec_g2d_params *)kmalloc(sizeof(sec_g2d_params), GFP_KERNEL);
	if(params == NULL)
	{
		printk(KERN_ERR "fimg2d: instance memory allocation was failed\n");
		return -1;
	}

	memset(params, 0, sizeof(sec_g2d_params));

	file->private_data	= (sec_g2d_params *)params;

#ifdef G2D_CLK_GATING
	g_num_of_g2d_object++;

	if(file->f_flags & O_NONBLOCK)
		g_num_of_nonblock_object++;
		
#ifndef USE_G2D_TIMER_FOR_CLK
	if(g_num_of_g2d_object == 1)
	{
		mutex_init(&g_g2d_clk_mutex);
	}
#endif
#endif
	
#ifdef G2D_DEBUG
	printk("fimg2d: open ok!\n"); 	
#endif
	return 0;
}


int sec_g2d_release(struct inode *inode, struct file *file)
{
	sec_g2d_params	*params;

#ifdef G2D_CHECK_HW_VERSION
	if (1 != g_hw_version)
		return 0;
#endif

	params	= (sec_g2d_params *)file->private_data;
	if (params == NULL) {
		printk(KERN_ERR "fimg2d: cannot release\n");
		return -1;
	}

	kfree(params);

#ifdef G2D_CLK_GATING
	g_num_of_g2d_object--;

	if(file->f_flags & O_NONBLOCK)
		g_num_of_nonblock_object--;

	sec_g2d_clk_disable();

	clk_disable(sec_g2d_hclk);

#ifndef USE_G2D_TIMER_FOR_CLK	
	if(g_num_of_g2d_object == 0)
	{
		mutex_destroy(&g_g2d_clk_mutex);
	}
#endif
#endif
	
#ifdef G2D_DEBUG
	printk("fimg2d: release ok! \n"); 
#endif
	return 0;
}


int sec_g2d_mmap(struct file* filp, struct vm_area_struct *vma) 
{
	unsigned long pageFrameNo=0;
	unsigned long size;
	
	size = vma->vm_end - vma->vm_start;

	// page frame number of the address for a source G2D_SFR_SIZE to be stored at. 
	//pageFrameNo = __phys_to_pfn(SEC6400_PA_G2D);
	
	if(size > G2D_SFR_SIZE) {
		printk("fimg2d: the size of G2D_SFR_SIZE mapping is too big!\n");
		return -EINVAL;
	}
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot); 
	
	if((vma->vm_flags & VM_WRITE) && !(vma->vm_flags & VM_SHARED)) {
		printk("fimg2d: writable G2D_SFR_SIZE mapping must be shared !\n");
		return -EINVAL;
	}
	
	if(remap_pfn_range(vma, vma->vm_start, pageFrameNo, size, vma->vm_page_prot))
		return -EINVAL;
	
	return 0;
}


static int sec_g2d_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	sec_g2d_params	*params;
	ROT_DEG 		eRotDegree;
	int             ret = 0;

#ifdef G2D_CHECK_HW_VERSION
	if (SEC_G2D_GET_VERSION == cmd)
	{
		copy_to_user((int *)arg, &g_hw_version, sizeof(int));	
		return 0;
	}

	if (1 != g_hw_version)
		return 0;
#endif

	params	= (sec_g2d_params*)file->private_data;
	copy_from_user(params, (sec_g2d_params*)arg, sizeof(sec_g2d_params));
	
	mutex_lock(h_rot_mutex);

#ifdef G2D_CLK_GATING
#ifndef USE_G2D_TIMER_FOR_CLK
	mutex_lock(&g_g2d_clk_mutex);
	sec_g2d_clk_enable();
#else
	spin_lock(&g2d_domain_lock);
	g2d_pwr_off_flag = 0;	
	sec_g2d_clk_enable();
	spin_unlock(&g2d_domain_lock);
#endif
#endif

#ifdef G2D_DEBUG
	printk("fimg2d:%s: start\n", __func__);
#endif
	switch(cmd)
	{
		case SEC_G2D_ROTATOR_0:
		case SEC_G2D_ROTATOR_90:
		case SEC_G2D_ROTATOR_180:
		case SEC_G2D_ROTATOR_270:
			// initialize
			ret = sec_g2d_init_regs(params, cmd);
			if (0 > ret) break; 
		
			// bitblit
			sec_g2d_rotate_with_bitblt(params);
			break;
	}

	if (0 > ret)
	{
		mutex_unlock(h_rot_mutex);
		return ret;
	}

	if(!(file->f_flags & O_NONBLOCK)){
#if 0
		if (0 == sec_g2d_irq_flag && 
				interruptible_sleep_on_timeout(&waitq_g2d, G2D_TIMEOUT) == 0) {
#else
		if (interruptible_sleep_on_timeout(&waitq_g2d, G2D_TIMEOUT) == 0) {
#endif
			printk(KERN_ERR "fimg2d:%s: waiting for interrupt is timeout : src(%d,%d,%d,%d,%d,%d,0x%x) dst(%d,%d,%d,%d,%d,%d,0x%x) alpha=%d\n",
					__FUNCTION__, 
					params->src_full_width, params->src_full_height, params->src_start_x, params->src_start_y, 
					params->src_work_width, params->src_work_height, params->src_color_space,
					params->dst_full_width, params->dst_full_height, params->dst_start_x, params->dst_start_y, 
					params->dst_work_width, params->dst_work_height, params->dst_color_space,
					params->alpha_mode?params->alpha_val:255);
		}
	}

#ifdef G2D_CLK_GATING
#ifdef USE_G2D_TIMER_FOR_CLK
	g2d_pwr_off_flag = 1;
	mod_timer(&g_g2d_domain_timer, jiffies + HZ);	
	//printk(">>>>>> jiffies=%u HZ=%u\n", jiffies, HZ);
#else
	sec_g2d_clk_disable();
	mutex_unlock(&g_g2d_clk_mutex);
#endif // USE_G2D_TIMER_FOR_CLK
#endif

	mutex_unlock(h_rot_mutex);

#ifdef G2D_DEBUG
	printk("fimg2d:%s: end\n", __func__);
#endif
	return ret;
}

static unsigned int sec_g2d_poll(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;
#ifdef G2D_CHECK_HW_VERSION
	if (1 != g_hw_version)
		return 0;
#endif

	poll_wait(file, &waitq_g2d, wait);
	if(sec_g2d_poll_flag == 1)
	{
		mask = POLLOUT|POLLWRNORM;
		sec_g2d_poll_flag = 0;
	}

	return mask;
}
 struct file_operations sec_g2d_fops = {
	.owner 		= THIS_MODULE,
	.open 		= sec_g2d_open,
	.release 	= sec_g2d_release,
	.mmap 		= sec_g2d_mmap,
	.ioctl		= sec_g2d_ioctl,
	.poll		= sec_g2d_poll,
};


static struct miscdevice sec_g2d_dev = {
	.minor		= G2D_MINOR,
	.name		= "sec-g2d",
	.fops		= &sec_g2d_fops,
};

int sec_g2d_probe(struct platform_device *pdev)
{

	struct resource *res;
	int ret;

#ifdef G2D_DEBUG
	printk(KERN_ALERT"fimg2d: start probe : name=%s num=%d res[0].start=0x%x res[1].start=0x%x\n", pdev->name, pdev->num_resources, pdev->resource[0].start, pdev->resource[1].start);
#endif

#ifdef G2D_CHECK_HW_VERSION
	g_hw_version = hw_version_check();
	if (1 != g_hw_version)
	{
		printk("fimg2d: dummy probe - s5pc110 evt%d has no fimg2d\n", __func__, g_hw_version);
		return 0;
	}
#endif
	
	/* find the IRQs */
	sec_g2d_irq_num = platform_get_irq(pdev, 0);
	if(sec_g2d_irq_num <= 0) {
		printk(KERN_ERR "fimg2d: failed to get irq resouce\n");
                return -ENOENT;
	}
#ifdef G2D_DEBUG
	printk("fimg2d:%s: sec_g2d_irq_num=%d\n", __func__, sec_g2d_irq_num);
#endif

	ret = request_irq(sec_g2d_irq_num, sec_g2d_irq, IRQF_DISABLED, pdev->name, NULL);
	if (ret) {
		printk("fimg2d: request_irq(g2d) failed.\n");
		return ret;
	}

	/* get the memory region */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if(res == NULL) 
	{
		printk(KERN_ERR "fimg2d: failed to get memory region resouce\n");
		return -ENOENT;
	}

	sec_g2d_mem = request_mem_region(res->start, res->end-res->start+1, pdev->name);
	if(sec_g2d_mem == NULL) {
		printk(KERN_ERR "fimg2d: failed to reserve memory region\n");
                return -ENOENT;
	}

	sec_g2d_base = ioremap(sec_g2d_mem->start, sec_g2d_mem->end - res->start + 1);
	if(sec_g2d_base == NULL) {
		printk(KERN_ERR "fimg2d: failed ioremap\n");
                return -ENOENT;
	}

#ifdef G2D_CLK_GATING
	sec_g2d_hclk = clk_get("NULL","hclk_2d");

	sec_g2d_clock = clk_get(&pdev->dev, "sclk_2d");
	sec_g2d_pclock = clk_get(&pdev->dev, "pclk_2d");
	if(sec_g2d_clock == NULL) {
			printk(KERN_ERR "fimg2d: failed to find g2d clock source\n");
			return -ENOENT;
	}
#endif

	init_waitqueue_head(&waitq_g2d);

	ret = misc_register(&sec_g2d_dev);
	if (ret) {
		printk (KERN_ERR "fimg2d: cannot register miscdev on minor=%d (%d)\n",
			G2D_MINOR, ret);
		return ret;
	}

	h_rot_mutex = (struct mutex *)kmalloc(sizeof(struct mutex), GFP_KERNEL);
	if (h_rot_mutex == NULL)
		return -1;
	
	mutex_init(h_rot_mutex);

#ifdef USE_G2D_TIMER_FOR_CLK
// init domain timer
        init_timer(&g_g2d_domain_timer);
        g_g2d_domain_timer.function = sec_g2d_domain_timer;
#endif

#ifdef G2D_DEBUG
	printk(KERN_ALERT"fimg2d: sec_g2d_probe ok!\n");
#endif

	return 0;  
}

static int sec_g2d_remove(struct platform_device *dev)
{
#ifdef G2D_DEBUG
	printk(KERN_INFO "fimg2d: sec_g2d_remove called !\n");
#endif

	free_irq(sec_g2d_irq_num, NULL);
	
	if (sec_g2d_mem != NULL) {   
		printk(KERN_INFO "fimg2d: releasing resource\n");
		iounmap(sec_g2d_base);
		release_resource(sec_g2d_mem);
		kfree(sec_g2d_mem);
	}
	
	misc_deregister(&sec_g2d_dev);

#ifdef G2D_CLK_GATING
	if (g_flag_clk_enable)
	{
		clk_disable(sec_g2d_clock);
#ifdef CONFIG_PM_PWR_GATING
		s5p_power_gating(S5PC100_POWER_DOMAIN_LCD, DOMAIN_LP_MODE);
#endif
		g_flag_clk_enable = 0;
	}
	
	if (sec_g2d_clock)
	{
		clk_put(sec_g2d_clock);
		sec_g2d_clock = NULL;
	}
#endif

#ifdef G2D_DEBUG
	printk(KERN_INFO "fimg2d: sec_g2d_remove ok!\n");
#endif
	return 0;
}

static int sec_g2d_suspend(struct platform_device *dev, pm_message_t state)
{
	//clk_disable(sec_g2d_clock);
	return 0;
}
static int sec_g2d_resume(struct platform_device *pdev)
{
	//clk_enable(sec_g2d_clock);
	return 0;
}


static struct platform_driver sec_g2d_driver = {
       .probe          = sec_g2d_probe,
       .remove         = sec_g2d_remove,
       .suspend        = sec_g2d_suspend,
       .resume         = sec_g2d_resume,
       .driver		= {
		.owner	= THIS_MODULE,
		.name	= "sec-g2d",
	},
};

int __init  sec_g2d_init(void)
{
 	if(platform_driver_register(&sec_g2d_driver)!=0)
  	{
   		printk("fimg2d: platform device register Failed \n");
   		return -1;
  	}

#ifdef G2D_DEBUG
	printk("fimg2d: init ok!\n");
#endif
	return 0;
}

void  sec_g2d_exit(void)
{
	platform_driver_unregister(&sec_g2d_driver);
	mutex_destroy(h_rot_mutex);

#ifdef G2D_DEBUG
 	printk("fimg2d: exit ok!\n");
#endif
}

module_init(sec_g2d_init);
module_exit(sec_g2d_exit);

MODULE_AUTHOR("");
MODULE_DESCRIPTION("SEC G2D Device Driver");
MODULE_LICENSE("GPL");
