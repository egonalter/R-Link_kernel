/* si4703_attr.c
 *
 * Sysfs interface to the SI4703 driver.
 *
 * Copyright (C) 2008 TomTom BV <http://www.tomtom.com/>
 * Authors: Guillaume Ballet <Guillaume.Ballet@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/device.h>

#include "si4703.h"
#include "fmreceiver.h"

extern int	si470x_reg_read(int, u16 *);
extern int	si4703_fifo_get(struct si4703dev *, unsigned char *, int);
extern int	si470x_seek(struct fmr_seek_param_type *);
extern u8	si470x_get_current_rssi(struct si4703dev *);

/* Simply dump the content of all registers */
ssize_t
si4703_show_regs(struct device		*dev,
		struct device_attribute	*attr,
		char			*buf)
{
 int			reg_num;
 ssize_t		count;
 struct si4703dev	*si4703dev = dev->driver_data;
 u16			*regs = (u16 *) &si4703dev->shadow_regs;

	si470x_reg_read(NR_OF_REGISTERS, regs);

	count	= 0;
	for (reg_num=0; reg_num < NR_OF_REGISTERS; reg_num++) {
		count	+= snprintf(&buf[count], 10, "%02x:\t%04x\n", reg_num, regs[reg_num]);
	}

	return count;
}

#define SI4703_GVAR_READER(name)			\
ssize_t							\
si4703_show_##name(struct device		*dev,	\
			struct device_attribute	*attr,	\
			char		*buf)		\
{							\
 struct si4703dev *gdev;				\
							\
	gdev	= (struct si4703dev*) dev->driver_data;	\
	return sprintf(buf, "%d\n", gdev->name);	\
}

/* SI4703_GVAR_WRITER - This macro is a template for all sysfs store * 
 * functions. It translates the given value into an integer so as to *
 * call the actual function responsible for updating the hardware.   *
 * The actual function is responsible for updating the variable that *
 * is kept as a cache in 'gdev'.                                     */
#define SI4703_GVAR_WRITER(name)			\
ssize_t							\
si4703_store_##name(struct device		*dev,	\
			struct device_attribute	*attr,	\
			const char		*buf,	\
			size_t			count)	\
{							\
 struct si4703dev	*gdev;				\
 typeof(gdev->name)	value;				\
							\
	gdev		= dev->driver_data;		\
	if (sscanf(buf, "%hd", &value) == 1) {		\
 		gdev->set_##name (gdev, value);		\
		return count;				\
	} else {					\
		printk("Invalid number!\n");		\
		return -1;				\
	}						\
}

#define SI4703_GVAR_ACCESSORS(name)			\
	SI4703_GVAR_READER(name)			\
	SI4703_GVAR_WRITER(name)

/* si470x_show_packets - Show RDS packets content, extracting them from the *
 * FIFO.                                                                    *
 * XXX - the ARM9 version of this driver had a weird locking scheme, I need *
 * to make sure it is no longer needed.                                     */
ssize_t
si470x_show_packets(struct device	*dev,
		struct device_attribute	*attr,
		char			*buf)
{
 struct fmr_rds_type	rdsdata;
 int			ret_count;
 struct si4703dev	*gdev;

	ENTER();

	gdev		= dev->driver_data;

	/* Seek operations car empty the fifo, which is why the return value is decided */
	/* AFTER reading its content. Kernel fifo functions use locking facilities.     */
	ret_count	= si4703_fifo_get(gdev, (u8 *) &rdsdata, sizeof(struct fmr_rds_type));

	if (ret_count == sizeof(struct fmr_rds_type)) {
		memcpy(buf, &rdsdata, sizeof(struct fmr_rds_type));
	} else
		ret_count	= 0;

	EXIT(ret_count);
}

ssize_t
si470x_show_seek(struct device		*dev,
		struct device_attribute	*attr,
		char			*buf)
{
 struct si4703dev	*gdev;

	gdev		= dev->driver_data;

	/* Note: 10 bytes should be more than enough. */
	EXIT(snprintf(buf, 10, "%d\n", gdev->last_found_freq));
}

ssize_t
si470x_store_seek(struct device		*dev,
		struct device_attribute	*attr,
		const char		*buf,
		size_t			count)
{
 struct si4703dev		*gdev;
 struct fmr_seek_param_type	rds_seek_data;

	ENTER();

	gdev		= dev->driver_data;

	if (count != sizeof(struct fmr_seek_param_type)) {
		printk("The size is invalid, assuming test mode: seeking upward from current frequency\n");
		rds_seek_data.up	= 1;
		rds_seek_data.wrap	= 1;
		rds_seek_data.threshold	= 13;
		rds_seek_data.freq	= gdev->freq;
	} else {
		memcpy(&rds_seek_data, buf, sizeof(struct fmr_seek_param_type));
	}

#if 1
	/* Seek purges the fifo! As a result, care must be taken that a fifo read */
	/* operation is not taking place at the same time.                        */
	si470x_seek(&rds_seek_data);
#endif

	EXIT(sizeof(struct fmr_seek_param_type));
}

ssize_t
si4703_show_rssi(struct device	*dev,
		struct device_attribute	*attr,
		char			*buf)
{
 struct si4703dev	*gdev;

	gdev		= dev->driver_data;

	/* RSSI is not cached because it evolves more often than the frequency is being changed. */
	EXIT(snprintf(buf, 10, "%d\n", si470x_get_current_rssi(gdev)));
}

SI4703_GVAR_ACCESSORS(freq);
SI4703_GVAR_ACCESSORS(band);
SI4703_GVAR_ACCESSORS(volume);
SI4703_GVAR_READER(device_id);
SI4703_GVAR_ACCESSORS(rds_enabled);
SI4703_GVAR_READER(irq);

static DEVICE_ATTR(registers, S_IRUGO, si4703_show_regs, NULL);
static DEVICE_ATTR(rdsdata, S_IRUGO, si470x_show_packets, NULL);
static DEVICE_ATTR(freq, S_IRUGO | S_IWUGO, si4703_show_freq, si4703_store_freq);
static DEVICE_ATTR(band, S_IRUGO | S_IWUGO, si4703_show_band, si4703_store_band);
static DEVICE_ATTR(device_id, S_IRUGO, si4703_show_device_id, NULL);
static DEVICE_ATTR(volume, S_IRUGO | S_IWUGO, si4703_show_volume, si4703_store_volume);
static DEVICE_ATTR(rds_enabled, S_IRUGO | S_IWUGO, si4703_show_rds_enabled, si4703_store_rds_enabled);
static DEVICE_ATTR(irq, S_IRUGO, si4703_show_irq, NULL);
static DEVICE_ATTR(seek_next, S_IRUGO | S_IWUGO, si470x_show_seek, si470x_store_seek);
static DEVICE_ATTR(rssi, S_IRUGO, si4703_show_rssi, NULL);

#ifdef RDSQ_LOGGING
ssize_t
si470x_show_rdsq(struct device	*dev,
		struct device_attribute	*attr,
		char			*buf)
{
 struct si4703dev		*gdev;

	gdev	= dev->driver_data;

	EXIT(snprintf(buf, 64, "good=%d, bad=%d, rdsq=%d\n", gdev->rds_good, gdev->rds_bad, gdev->rdsq));
}

static DEVICE_ATTR(rdsq, S_IRUGO, si470x_show_rdsq, NULL);
#endif

/* XXX guba - change it to return an error code */
void si4703_sysfs_register(struct device	*dev)
{
	ENTER();

	/* Create all the sysfs files. The weird syntax is because not */
	/* checking the return type triggers a warning.                */
	if (device_create_file(dev, &dev_attr_registers)
	|| device_create_file(dev, &dev_attr_rdsdata)
	|| device_create_file(dev, &dev_attr_volume)
	|| device_create_file(dev, &dev_attr_freq)
	|| device_create_file(dev, &dev_attr_band)
	|| device_create_file(dev, &dev_attr_device_id)
	|| device_create_file(dev, &dev_attr_rds_enabled)
	|| device_create_file(dev, &dev_attr_irq)
	|| device_create_file(dev, &dev_attr_seek_next)
	|| device_create_file(dev, &dev_attr_rssi))
		printk("Error creating the sysfs files!\n");
#ifdef RDSQ_LOGGING
	if (device_create_file(dev, &dev_attr_rdsq))
		printk("Error creating the RDSQ sysfs file.\n");
#endif
}

void si4703_sysfs_unregister(struct device	*dev)
{
	ENTER();

	device_remove_file(dev, &dev_attr_registers);
	device_remove_file(dev, &dev_attr_rdsdata);
	device_remove_file(dev, &dev_attr_volume);
	device_remove_file(dev, &dev_attr_freq);
	device_remove_file(dev, &dev_attr_band);
	device_remove_file(dev, &dev_attr_device_id);
	device_remove_file(dev, &dev_attr_rds_enabled);
	device_remove_file(dev, &dev_attr_irq);
	device_remove_file(dev, &dev_attr_seek_next);
	device_remove_file(dev, &dev_attr_rssi);
#ifdef RDSQ_LOGGING
	device_remove_file(dev, &dev_attr_rdsq);
#endif
}

