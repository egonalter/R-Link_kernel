/**************************************************************************** 
 *  (C) Copyright 2008 Samsung Electronics Co., Ltd., All rights reserved
 *
 *  [File Name]   : IntTransferChecker.c
 *  [Description] : The Source file implements the external and internal functions of IntTransferChecker
 *  [Author]      : Yang Soon Yeal { syatom.yang@samsung.com }
 *  [Department]  : System LSI Division/System SW Lab
 *  [Created Date]: 2008/06/19
 *  [Revision History] 	     
 *      (1) 2008/06/18   by Yang Soon Yeal { syatom.yang@samsung.com }
 *          - Created this file and implements functions of IntTransferChecker
 *
 ****************************************************************************/
/****************************************************************************
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 ****************************************************************************/


#include "s3c-otg-transferchecker-iso.h"

#include <linux/interrupt.h>
static void s3c_otg_transferchecker_tasklet(unsigned long);
DECLARE_TASKLET(s3c_otg_tl, s3c_otg_transferchecker_tasklet, 0);
td_t *iso_td[16];

/* ISO transfer completion callback tasklet */
static void s3c_otg_transferchecker_tasklet(unsigned long data)
{
	u32 cnt;

	for (cnt = 0; cnt < 16; cnt++ )
	{
		if (iso_td[cnt] != NULL)
		{
			otg_usbcore_giveback(iso_td[cnt]);
			release_trans_resource(iso_td[cnt]);
			iso_td[cnt] = NULL;
			break;
		}
	}
}

u8 process_iso_transfer(td_t *result_td, hc_info_t *hc_reg_data)
{
	hcintn_t	hc_intr_info;
	u8		ret_val=0;

	//we just deal with the interrupts to be unmasked.
	hc_intr_info.d32 = hc_reg_data->hc_int.d32 &
				result_td->cur_stransfer.hc_reg.hc_int_msk.d32;

	if(hc_intr_info.b.chhltd)
	{
		ret_val = process_chhltd_on_iso(result_td, hc_reg_data);
	}
	return ret_val;
}

u8 process_chhltd_on_iso(td_t *result_td, hc_info_t *hc_reg_data)
{
	if (hc_reg_data->hc_int.b.xfercompl)
		return process_xfercompl_on_iso(result_td, hc_reg_data);
	else if (hc_reg_data->hc_int.b.frmovrun)
		return  process_frmovrrun_on_iso(result_td,hc_reg_data);
	else
	{
		clear_ch_intr(result_td->cur_stransfer.alloc_chnum, CH_STATUS_ALL);
		return RE_SCHEDULE;
	}

	return 0;
}

u8 process_xfercompl_on_iso(td_t *result_td, hc_info_t *hc_reg_data)
{
	u8	ret_val=0;
	u32	actual_length;
	u32	index;
	struct urb *urb_p = NULL;

	index = result_td->isoch_packet_index;		

	result_td->err_cnt =0;	

	clear_ch_intr(result_td->cur_stransfer.alloc_chnum, CH_STATUS_ALL);
	
	//Mask ack Interrupt..
	mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum, CH_STATUS_ACK);
	mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum, CH_STATUS_NAK);
	mask_channel_interrupt(result_td->cur_stransfer.alloc_chnum, CH_STATUS_DataTglErr);

	actual_length = calc_transferred_size(true,result_td, hc_reg_data);
	result_td->transferred_szie += actual_length;

	urb_p = (struct urb *)result_td->td_private;	
	urb_p->iso_frame_desc[index].actual_length = actual_length;
	urb_p->iso_frame_desc[index].status = 0;

	result_td->isoch_packet_index++;

	if (result_td->isoch_packet_index != result_td->isoch_packet_num)
	{
		update_datatgl(hc_reg_data->hc_size.b.pid, result_td);
		//result_td->parent_ed_p->ed_desc.sched_frame = oci_get_frame_num() + 1;
		//result_td->parent_ed_p->ed_desc.sched_frame &= HFNUM_MAX_FRNUM;
		ret_val = RE_SCHEDULE;
	}
	else
	{
		ret_val = DE_ALLOCATE;
		update_datatgl(hc_reg_data->hc_size.b.pid, result_td);
		result_td->error_code   =       USB_ERR_STATUS_COMPLETE;
		
		//result_td->parent_ed_p->ed_desc.sched_frame = oci_get_frame_num() + 1;
                //result_td->parent_ed_p->ed_desc.sched_frame &= HFNUM_MAX_FRNUM;
		
		iso_td[result_td->cur_stransfer.alloc_chnum] = result_td;
		tasklet_schedule(&s3c_otg_tl);
	}

	return(ret_val);
}

u8 process_frmovrrun_on_iso(td_t *result_td, hc_info_t *hc_reg_data)
{
	u8      ret_val=0;

        clear_ch_intr(result_td->cur_stransfer.alloc_chnum, CH_STATUS_ALL);
        update_datatgl(hc_reg_data->hc_size.b.pid, result_td);
	
	result_td->parent_ed_p->ed_desc.sched_frame = oci_get_frame_num() + 1;
	result_td->parent_ed_p->ed_desc.sched_frame &= HFNUM_MAX_FRNUM;
        return RE_SCHEDULE;
}

