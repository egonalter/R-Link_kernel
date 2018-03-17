/*******************************************************************************
*
* File Name		: i3g4250d.h
* Description		: I3G4250D digital output gyroscope sensor API
*
********************************************************************************
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
* THE PRESENT SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES
* OR CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, FOR THE SOLE
* PURPOSE TO SUPPORT YOUR APPLICATION DEVELOPMENT.
* AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY DIRECT,
* INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING FROM THE
* CONTENT OF SUCH SOFTWARE AND/OR THE USE MADE BY CUSTOMERS OF THE CODING
* INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
*
********************************************************************************/

#ifndef __I3G4250D_H__
#define __I3G4250D_H__

#define I3G4250D_DEV_NAME		"i3g4250d"

#define I3G4250D_ENABLED		1
#define I3G4250D_DISABLED		0

#ifdef __KERNEL__

#define I3G4250D_SAD0L			(0x00)
#define I3G4250D_SAD0H			(0x01)
#define I3G4250D_I2C_SADROOT		(0x34)

/* I2C address if gyr SA0 pin to GND */
#define I3G4250D_I2C_SAD_L		((I3G4250D_I2C_SADROOT<<1)| \
							I3G4250D_SAD0L)
/* I2C address if gyr SA0 pin to Vdd */
#define I3G4250D_I2C_SAD_H		((I3G4250D_I2C_SADROOT<<1)| \
							I3G4250D_SAD0H)


/* to set gpios numb connected to gyro interrupt pins,
 * the unused ones havew to be set to -EINVAL
 */
#define I3G4250D_DEFAULT_INT1_GPIO	(-EINVAL)
#define I3G4250D_DEFAULT_INT2_GPIO	(-EINVAL)

#define I3G4250D_MIN_POLL_PERIOD_MS	2

#define I3G4250D_FS_245DPS		(0x00)
#define I3G4250D_FS_500DPS		(0x10)
#define I3G4250D_FS_2000DPS		(0x30)

struct i3g4250d_platform_data {
	int (*init)(void);
	void (*exit)(void);
	int (*power_on)(void);
	int (*power_off)(void);
	unsigned int poll_interval;
	unsigned int min_interval;

	u8 fs_range;

	/* gpio ports for interrupt pads */
	int gpio_int1;
	int gpio_int2;		/* int for fifo */

	u8 avg_samples;
	/* axis mapping */
	u8 axis_map_x;
	u8 axis_map_y;
	u8 axis_map_z;

	u8 negate_x;
	u8 negate_y;
	u8 negate_z;
};
#endif /* __KERNEL__ */

#endif  /* __I3G4250D_H__ */
