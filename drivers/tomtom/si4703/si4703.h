/*
 * Driver for the si4703 FM/RDS/TMC Receiver
 *
 *	This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version
 *  2 of the License, or (at your option) any later version.
 *
 *  Author:      Xander.Hover, <Xander.Hover@tomtom.com>
 */

#ifndef __INCLUDE_LINUX_SI4703_H
#    define __INCLUDE_LINUX_SI4703_H       __FILE__

#	include <linux/ioctl.h>
#	include <linux/major.h>
#	include <linux/i2c.h>
#	include <linux/cdev.h>

#    define SI4703_DEVNAME  		"SI4703"

#    define SI4703_MAJOR    		FMRECEIVER_MAJOR

/*#    define SI4703_MINOR    		(FMRECEIVER_MINOR + 1) */
#    define SI4703_MINOR    		FMRECEIVER_MINOR

#    define SI4703_I2C_SLAVE_ADDR         (0x10)

/* ** Driver-wide switches *********************************************************************************** */
#	define	USE_CRYSTAL			1	/* 0 = external RCLK, 1 = crystal oscillator */

#	define	LOGGING				1	/* Various error messages */
#	if (LOGGING == 0)
#		undef	LOGGING
#	endif

#	define RDSQ_LOGGING			1	/* RDS reception quality, includes additional */
#	if (RDSQ_LOGGING == 0)				/* logging and more sysfs files.              */
#		undef	RDSQ_LOGGING
#	endif

/* ** Chip-specific defines ********************************************************************************** */
#	define 	NR_OF_REGISTERS   		16
#	define	NR_OF_READABLE_REGISTERS	(NR_OF_REGISTERS/2)	/* XXX - should be useless now */
#	define	NR_OF_WRITE_REGS   		6

#	define	POWERUP_TIME			110	/* Powerup delay in milliseconds */
#	define	CRYSTAL_TIME			500	/* Crystal stabilization delay in milliseconds */

#	define	TUNE_TIMEOUT			10
#	define	SEEK_TIMEOUT			15

/* ** Some logging facilities ******************************************************************************** */
#	ifdef	LOGGING
#		define	ENTER()		printk("> %s,%d\n", __func__, __LINE__);
#		define	YELL(str, opt...)	printk("- %s,%d:" str "\n", __func__, __LINE__, ##opt);
#		define	EXIT(code)	do {				\
						printk("< %s,%d - returns %d\n", __func__, __LINE__, code);	\
						return	code;							\
					} while (0)
#	else
#		define	ENTER()
#		define	YELL(str, opt...)
#		define EXIT(code)	return code
#	endif

/* ** Misc type definitions ********************************************************************************** */

struct si4703dev;
typedef	int (*si4703dev_write_func_t)(struct si4703dev *, __u16);
typedef int (*si4703dev_read_func_t)(struct si4703dev *);

/* ** Data structures **************************************************************************************** */

struct si4703_registers {
	u16 deviceid;
	u16 chipid;
	u16 powercfg;
	u16 channel;
	u16 sysconfig1;
	u16 sysconfig2;
	u16 sysconfig3;
	u16 test1;
	u16 test2;
	u16 bootconfig;
	u16 statusrssi;
	u16 readchan;
	u16 rdsa;		/* RDS received packets */
	u16 rdsb;
	u16 rdsc;
	u16 rdsd;
} __attribute__ ((__packed__));

struct si4703dev {
	wait_queue_head_t	wq; /* wait queue for read */
	struct cdev		cdev;
	struct i2c_client	*client;
	spinlock_t		sp_lock;
	struct kfifo		*fifo;
	int			irq;

	/* "Global" variables */
	u16			freq;
	si4703dev_write_func_t	set_freq;
	u16			last_found_freq;	/* Result of the last seek operation */
	u16			band;
	si4703dev_write_func_t	set_band;
	u16			volume;
	si4703dev_write_func_t	set_volume;
	u16			device_id;
	u16			rds_enabled;
	si4703dev_write_func_t	set_rds_enabled;

	/* Shadow copy of the device's registers. */
	struct si4703_registers	shadow_regs;

	/* RDS quality logging variables */
	u32			rds_bad;		/* Counter for corrupt rds blocks */
	u32			rds_good;		/* Counter for valid rds blocks */
#	ifdef RDSQ_LOGGING
	u32			prev_rds_good;
	u32			rdsq;
	unsigned long		rds_timestamp_ms;
#	endif
};

#endif				/* __INCLUDE_LINUX_SI4703_H */
