#ifndef _S3C_PP_COMMON_H_
#define _S3C_PP_COMMON_H_

#include "s3c_pp.h"

#define PP_MINOR    253		/* post processor is misc device driver */

typedef enum {
	HCLK		= 0,
	PLL_EXT		= 1,
	EXT_27MHZ	= 3
} s3c_pp_clk_src_t;

typedef enum {
	POST_IDLE	= 0,
	POST_BUSY
} s3c_pp_state_t;

typedef struct {
	unsigned int 		src_full_width;		
	unsigned int 		src_full_height;
	unsigned int 		src_start_x;
	unsigned int 		src_start_y;
	unsigned int 		src_width;
	unsigned int 		src_height;
	unsigned int 		src_buf_addr_phy;
	unsigned int 		src_next_buf_addr_phy;
	s3c_color_space_t 	src_color_space;
	
	unsigned int 		dst_full_width;
	unsigned int 		dst_full_height;
	unsigned int 		dst_start_x;
	unsigned int 		dst_start_y;
	unsigned int 		dst_width;
	unsigned int 		dst_height;
	unsigned int 		dst_buf_addr_phy;
	s3c_color_space_t 	dst_color_space;

	s3c_pp_out_path_t 	out_path;
	s3c_pp_scan_mode_t	scan_mode;

	unsigned int		in_pixel_size;
	unsigned int		out_pixel_size;

	unsigned int		instance_no;
	unsigned int		value_changed;
} s3c_pp_instance_context_t;

typedef struct {
	unsigned int pre_h_ratio; 
	unsigned int pre_v_ratio; 
	unsigned int h_shift; 
	unsigned int v_shift; 
	unsigned int sh_factor;
	unsigned int pre_dst_width; 
	unsigned int pre_dst_height; 
	unsigned int dx; 
	unsigned int dy;
} s3c_pp_scaler_info_t;

typedef struct {
	unsigned int offset_y;
	unsigned int offset_cb;
	unsigned int offset_cr;
	unsigned int src_start_y;
	unsigned int src_start_cb;
	unsigned int src_start_cr;
	unsigned int src_end_y;
	unsigned int src_end_cb;
	unsigned int src_end_cr;
	unsigned int start_pos_y;
	unsigned int end_pos_y;
	unsigned int start_pos_cb;
	unsigned int end_pos_cb;
	unsigned int start_pos_cr;
	unsigned int end_pos_cr;
	unsigned int start_pos_rgb;
	unsigned int end_pos_rgb;
	unsigned int dst_start_rgb;
	unsigned int dst_end_rgb;
	unsigned int src_frm_start_addr;

	unsigned int offset_rgb;
	unsigned int out_offset_cb;
	unsigned int out_offset_cr;
	unsigned int out_start_pos_cb;
	unsigned int out_start_pos_cr;
	unsigned int out_end_pos_cb;
	unsigned int out_end_pos_cr;
	unsigned int out_src_start_cb;
	unsigned int out_src_start_cr;
	unsigned int out_src_end_cb;
	unsigned int out_src_end_cr;
} s3c_pp_buf_addr_t;

/*
 * below functions are used for Post Processor commonly
 */
void set_data_format(s3c_pp_instance_context_t *pp_instance);
void set_src_addr(s3c_pp_instance_context_t *pp_instance);
void set_dest_addr(s3c_pp_instance_context_t *pp_instance);
void set_src_next_buf_addr(s3c_pp_instance_context_t *pp_instance);
void set_scaler(s3c_pp_instance_context_t *pp_instance);
int cal_data_size(s3c_color_space_t color_space, unsigned int width, 
			unsigned int height);

/*
 * below functions'body is implemented 
 *		in each post processor IP file (ex. s3c_pp_6400.c)
 */
void set_scaler_register(s3c_pp_scaler_info_t *scaler_info, 
				s3c_pp_instance_context_t *pp_instance);
void set_src_addr_register(s3c_pp_buf_addr_t *buf_addr, 
				s3c_pp_instance_context_t *pp_instance);
void set_dest_addr_register(s3c_pp_buf_addr_t *buf_addr, 
				s3c_pp_instance_context_t *pp_instance);
void set_src_next_addr_register(s3c_pp_buf_addr_t *buf_addr, 
				s3c_pp_instance_context_t *pp_instance);
void set_data_format_register(s3c_pp_instance_context_t *pp_instance);

s3c_pp_state_t post_get_processing_state(void);

#endif /* _S3C_PP_COMMON_H_ */

