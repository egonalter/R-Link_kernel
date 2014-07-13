/* linux/arch/arm/plat-s5p/include/plat/fb.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Core file for Samsung Display Controller (FIMD) driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_PLAT_FB_H
#define __ASM_PLAT_FB_H __FILE__

#define FB_SWAP_WORD	(1 << 24)
#define FB_SWAP_HWORD	(1 << 16)
#define FB_SWAP_BYTE	(1 << 8)
#define FB_SWAP_BIT	(1 << 0)

struct platform_device;
struct clk;

/*
 * struct s3cfb_lcd_polarity
 * @rise_vclk:	if 1, video data is fetched at rising edge
 * @inv_hsync:	if HSYNC polarity is inversed
 * @inv_vsync:	if VSYNC polarity is inversed
 * @inv_vden:	if VDEN polarity is inversed
*/
struct s3cfb_lcd_polarity {
	int	rise_vclk;
	int	inv_hsync;
	int	inv_vsync;
	int	inv_vden;
};

/*
 * struct s3cfb_lcd_timing
 * @h_fp:	horizontal front porch
 * @h_bp:	horizontal back porch
 * @h_sw:	horizontal sync width
 * @v_fp:	vertical front porch
 * @v_fpe:	vertical front porch for even field
 * @v_bp:	vertical back porch
 * @v_bpe:	vertical back porch for even field
*/
struct s3cfb_lcd_timing {
	int	h_fp;
	int	h_bp;
	int	h_sw;
	int	v_fp;
	int	v_fpe;
	int	v_bp;
	int	v_bpe;
	int	v_sw;
};

/*
 * struct s3cfb_lcd
 * @width:		horizontal resolution
 * @height:		vertical resolution
 * @bpp:		bits per pixel
 * @freq:		vframe frequency
 * @timing:		timing values
 * @polarity:		polarity settings
 * @init_ldi:		pointer to LDI init function
 *
*/
struct s3cfb_lcd {
	int	width;
	int	height;
	int	bpp;
	int	freq;
	struct 	s3cfb_lcd_timing timing;
	struct 	s3cfb_lcd_polarity polarity;

	void 	(*init_ldi)(void);
	void 	(*deinit_ldi)(void);
};

struct s3c_platform_fb {
	int		hw_ver;
	char		clk_name[16];
	int		nr_wins;
	int		nr_buffers[5];
	int		default_win;
	int		swap;

	struct 		s3cfb_lcd *lcd;
	
	int		(*cfg_gpio)(struct platform_device *dev);
	int		(*backlight_on)(struct platform_device *dev);
	int		(*backlight_onoff)(struct platform_device *dev,int onoff);
	int		(*reset_lcd)(struct platform_device *dev);
	int		(*clk_on)(struct platform_device *pdev, struct clk **s3cfb_clk);
	int		(*clk_off)(struct platform_device *pdev, struct clk **clk);
	int		(*reset_lcd_onoff)(struct platform_device *dev,int onoff);
};

extern void s3cfb_set_platdata(struct s3c_platform_fb *fimd);

/* defined by architecture to configure gpio */
extern void s3cfb_cfg_gpio(struct platform_device *pdev);
extern int s3cfb_backlight_on(struct platform_device *pdev);
extern int s3cfb_reset_lcd(struct platform_device *pdev);
extern int s3cfb_clk_on(struct platform_device *pdev, struct clk **s3cfb_clk);
extern int s3cfb_clk_off(struct platform_device *pdev, struct clk **clk);
extern void s3cfb_get_clk_name(char *clk_name);
extern int s3cfb_backlight_onoff(struct platform_device *pdev,int onoff);
extern int s3cfb_reset_lcd_onoff(struct platform_device *pdev,int onoff);

#endif /* __ASM_PLAT_FB_H */
