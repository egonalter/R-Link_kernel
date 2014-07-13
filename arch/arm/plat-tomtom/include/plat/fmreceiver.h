/*
 * Generic FM receiver include file
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 * Author:		Xander Hover, <Xander.Hoverm@tomtom.com>
 * Adjusted from fmtransmitter.h
 */

#ifndef __INCLUDE_LINUX_FMRECEIVER_H
#    define __INCLUDE_LINUX_FMRECEIVER_H	__FILE__

#ifdef __cplusplus
extern "C" {
#endif

#    include <linux/ioctl.h>
#    include <linux/major.h>
#    include <linux/types.h>

#    define FMRECEIVER_MAJOR    		(247)
#    define FMRECEIVER_MINOR    		(0)

#    define FMRECEIVER_DRIVER_MAGIC     'T'


/* up: 1 = seek direction upwards, 0 = seek direcrtion downwards
 * wrap: 1 = wrap arount at fm bounds, 0 = stop at fm bounds
 * freq: freq in 10 kHz steps to start seeking from
 * threshold: threshold: minimum rssi value from  0..75 dBuV which should yield a hit.
 */
struct fmr_seek_param_type {
	unsigned char up;
	unsigned char wrap;
	unsigned char threshold;
	unsigned short freq;
};

struct fmr_rds_type {
	unsigned char      sf_bandlimit;
	unsigned char      rssi;
	unsigned short int freq;
	unsigned short int blocka;
	unsigned short int blockb;
	unsigned short int blockc;
	unsigned short int blockd;
} __attribute__ ((__packed__));


/* Defined IOCTLS for the fm receiver */

#    define TMCIOC_QUERY_DEVICE_ID	_IOR(FMRECEIVER_DRIVER_MAGIC, 0, __u16)
#    define TMCIOC_G_FREQUENCY		_IOR(FMRECEIVER_DRIVER_MAGIC, 1, __u16)
#    define TMCIOC_S_FREQUENCY		_IOW(FMRECEIVER_DRIVER_MAGIC, 2, __u16)
#    define TMCIOC_S_BAND		_IOW(FMRECEIVER_DRIVER_MAGIC, 3, __u16)
#    define TMCIOC_S_VOLUME		_IOW(FMRECEIVER_DRIVER_MAGIC, 4, __u16)
#    define TMCIOC_SEEK			_IOW(FMRECEIVER_DRIVER_MAGIC, 5, struct fmr_seek_param_type)
#    define TMCIOC_Q_RSSI		_IOR(FMRECEIVER_DRIVER_MAGIC, 6, __u16)
#    define TMCIOC_RDS_START		_IOW(FMRECEIVER_DRIVER_MAGIC, 7, __u16)
#    define TMCIOC_RDS_STOP		_IOW(FMRECEIVER_DRIVER_MAGIC, 8, __u16)

#    ifdef __KERNEL__

struct fm_receiver_info {
	dev_t device_nr;	/* defined by platform definition, use MKDEV( major, minor) */
	unsigned fm_power_pin;
	unsigned fm_reset_pin;
	unsigned fm_sen_pin;
	unsigned fm_en_rds_clk;
	unsigned fm_irq_pin;
	unsigned fm_dock_i2c_en_pin;
	unsigned char slave_address;
};

#    endif			/* __KERNEL__    */

#ifdef __cplusplus
}
#endif


#endif				/* __INCLUDE_LINUX_FMRECEIVER_H */
