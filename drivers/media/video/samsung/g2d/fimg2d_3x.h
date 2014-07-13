/* linux/drivers/media/video/samsung/g2d/fimg2d_3x.h
 *
 * Copyright  2008 Samsung Electronics Co, Ltd. All Rights Reserved.
 *		      http://www.samsungsemi.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef _SEC_G2D_DRIVER_H_
#define _SEC_G2D_DRIVER_H_

#define G2D_SFR_SIZE        0x1000

#define TRUE		1
#define FALSE	0

#define G2D_MINOR  220                     // Just some number

#define G2D_IOCTL_MAGIC 'G'

#define SEC_G2D_ROTATOR_0			_IO(G2D_IOCTL_MAGIC,0)
#define SEC_G2D_ROTATOR_90			_IO(G2D_IOCTL_MAGIC,1)
#define SEC_G2D_ROTATOR_180			_IO(G2D_IOCTL_MAGIC,2)
#define SEC_G2D_ROTATOR_270			_IO(G2D_IOCTL_MAGIC,3)
#define SEC_G2D_ROTATOR_X_FLIP		_IO(G2D_IOCTL_MAGIC,4)
#define SEC_G2D_ROTATOR_Y_FLIP		_IO(G2D_IOCTL_MAGIC,5)
#define SEC_G2D_GET_VERSION			_IO(G2D_IOCTL_MAGIC,6)

#define G2D_TIMEOUT		100
#define ALPHA_VALUE_MAX	255


#define G2D_MAX_WIDTH				(2048)
#define G2D_MAX_HEIGHT				(2048)

#define G2D_ROP_SRC_ONLY				(0xf0)
#define G2D_ROP_3RD_OPRND_ONLY		(0xaa)
#define G2D_ROP_DST_ONLY				(0xcc)
#define G2D_ROP_SRC_OR_DST			(0xfc)
#define G2D_ROP_SRC_OR_3RD_OPRND		(0xfa)
#define G2D_ROP_SRC_AND_DST			(0xc0) //(pat==1)? src:dst
#define G2D_ROP_SRC_AND_3RD_OPRND	(0xa0)
#define G2D_ROP_SRC_XOR_3RD_OPRND	(0x5a)
#define G2D_ROP_DST_OR_3RD_OPRND		(0xee)

#define ABS(v)                          (((v)>=0) ? (v):(-(v)))
#define FIFO_NUM			32

typedef enum
{
	ROT_0, ROT_90, ROT_180, ROT_270, ROT_X_FLIP, ROT_Y_FLIP
} ROT_DEG;


typedef enum
{
	ROP_DST_ONLY,
	ROP_SRC_ONLY, 
	ROP_3RD_OPRND_ONLY,
	ROP_SRC_AND_DST,
	ROP_SRC_AND_3RD_OPRND,
	ROP_SRC_OR_DST,
	ROP_SRC_OR_3RD_OPRND,	
	ROP_DST_OR_3RD,
	ROP_SRC_XOR_3RD_OPRND

} G2D_ROP_TYPE;

typedef enum
{
	G2D_NO_ALPHA_MODE,
	G2D_PP_ALPHA_SOURCE_MODE,
	G2D_ALPHA_MODE,
	G2D_FADING_MODE
} G2D_ALPHA_BLENDING_MODE;

typedef enum
{
	G2D_BLACK = 0, G2D_RED = 1, G2D_GREEN = 2, G2D_BLUE = 3, G2D_WHITE = 4, 
	G2D_YELLOW = 5, G2D_CYAN = 6, G2D_MAGENTA = 7
} G2D_COLOR;


typedef enum
{
	G2D_RGB_565 = 0,
	G2D_XRGB_8888, G2D_RGBX_8888, G2D_XBGR_8888, G2D_BGRX_8888,
	G2D_ARGB_8888, G2D_RGBA_8888, G2D_ABGR_8888, G2D_BGRA_8888,
	G2D_XRGB_1555, G2D_RGBX_5551, G2D_XBGR_1555, G2D_BGRX_5551,
	G2D_ARGB_1555, G2D_RGBA_5551, G2D_ABGR_1555, G2D_BGRA_5551,
	G2D_XRGB_4444, G2D_RGBX_4444, G2D_XBGR_4444, G2D_BGRX_4444,
	G2D_ARGB_4444, G2D_RGBA_4444, G2D_ABGR_4444, G2D_BGRA_4444,
	G2D_PACKED_RGB_888, G2D_PACKED_BGR_888,
	G2D_MAX_COLOR_SPACE
} G2D_COLOR_SPACE;

typedef enum
{
	G2D_CHL_ORDER_XRGB = 0,	// ARGB,XRGB
	G2D_CHL_ORDER_RGBX,		// RGBA,RGBX
	G2D_CHL_ORDER_XBGR,		// ABGR,XBGR
	G2D_CHL_ORDER_BGRX		// BGRA,BGRX
} G2D_CHANNEL_ORDER;

typedef enum
{
	G2D_FMT_XRGB_8888 = 0,
	G2D_FMT_ARGB_8888,
	G2D_FMT_RGB_565,
	G2D_FMT_XRGB_1555,
	G2D_FMT_ARGB_1555,
	G2D_FMT_XRGB_4444,
	G2D_FMT_ARGB_4444,
	G2D_FMT_PACKED_RGB_888
} G2D_COLOR_FORMAT;

typedef struct
{
	u32	src_base_addr;			//Base address of the source image
	u32	src_full_width;			//source image full width
	u32	src_full_height;		//source image full height
	u32	src_start_x;			//coordinate start x of source image
	u32	src_start_y;			//coordinate start y of source image
	u32	src_work_width;			//source image width for work
	u32 	src_work_height;		//source image height for work
	G2D_COLOR_SPACE src_color_space;

	u32	dst_base_addr;			//Base address of the destination image	
	u32	dst_full_width;			//destination screen full width
	u32	dst_full_height;		//destination screen full width
	u32	dst_start_x;			//coordinate start x of destination screen
	u32	dst_start_y;			//coordinate start y of destination screen
	u32	dst_work_width;			//destination screen width for work
	u32 	dst_work_height;		//destination screen height for work
	G2D_COLOR_SPACE dst_color_space;

	// Coordinate (X, Y) of clipping window
	u32 cw_x1, cw_y1;
	u32 cw_x2, cw_y2;

	u32 color_val[8];

	u32	alpha_mode;			//true : enable, false : disable
	u32	alpha_val;
	u32	color_key_mode;			//treu : enable, false : disable
	u32	color_key_val;			//transparent color value
	
}sec_g2d_params;

/**** function declearation***************************/
static int sec_g2d_init_regs(sec_g2d_params *params, u32 rot_degree);
void sec_g2d_bitblt(u16 src_x1, u16 src_y1, u16 src_x2, u16 src_y2,
  	                  u16 dst_x1, u16 dst_y1, u16 dst_x2, u16 dst_y2);
static void sec_g2d_rotate_with_bitblt(sec_g2d_params *params);
static void sec_g2d_rotator_start(sec_g2d_params *params,ROT_DEG rot_degree);
void sec_g2d_check_fifo(int empty_fifo);
#if 0
void sec_g2d_set_xy_incr_format(u32 uDividend, u32 uDivisor, u32* uResult);
static void sec_g2d_bitblt_start(sec_g2d_params *params);
static void sec_g2d_set_alpha_blending(sec_g2d_params *params);
void sec_2d_disable_effect(void);
#endif
int sec_g2d_open(struct inode *inode, struct file *file);
int sec_g2d_release(struct inode *inode, struct file *file);
int sec_g2d_mmap(struct file* filp, struct vm_area_struct *vma) ;
static int sec_g2d_ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg);
static unsigned int sec_g2d_poll(struct file *file, poll_table *wait); 

#endif /*_SEC_G2D_DRIVER_H_*/

