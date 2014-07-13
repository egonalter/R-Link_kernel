/* linux/drivers/video/samsung/s3cfb_lms500.c
 LMS500 LCD panel support
 *
 * Frank Hofmann, (C) TomTom NV 2010
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include "s3cfb.h"

static struct s3cfb_lcd lms500 = {
       .width = 480,
       .height = 272,
       .bpp = 16,
       .freq = 60,

       .timing = {
               .h_fp = 8,
               .h_bp = 46,
               .h_sw = 42,
               .v_fp = 3,
               .v_fpe = 1,
               .v_bp = 1,
               .v_bpe = 1,
               .v_sw = 11,
       },

       .polarity = {
               .rise_vclk = 0,
               .inv_hsync = 1,
               .inv_vsync = 1,
               .inv_vden = 0,
       },
};

/* name should be fixed as 's3cfb_set_lcd_info' */
void s3cfb_set_lcd_info(struct s3cfb_global *ctrl)
{
       lms500.init_ldi = NULL;
       ctrl->lcd = &lms500;
}

