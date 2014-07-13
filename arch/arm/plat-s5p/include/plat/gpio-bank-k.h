/* linux/arch/arm/plat-s5p/include/plat/gpio-bank-k.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 * 	Ben Dooks <ben@simtec.co.uk>
 * 	http://armlinux.simtec.co.uk/
 *
 * GPIO Bank A register and configuration definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define S5P64XX_GPKCON			(S5P64XX_GPK_BASE + 0x00)
#define S5P64XX_GPKDAT			(S5P64XX_GPK_BASE + 0x04)
#define S5P64XX_GPKPUD			(S5P64XX_GPK_BASE + 0x08)
#define S5P64XX_GPKCONSLP		(S5P64XX_GPK_BASE + 0x0c)
#define S5P64XX_GPKPUDSLP		(S5P64XX_GPK_BASE + 0x10)

#define S5P64XX_GPK_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5P64XX_GPK_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5P64XX_GPK_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5P64XX_GPK0_PCM2_SOUT		(0x02 << 0)
#define S5P64XX_GPK0_I2S2_SDO		(0x05 << 0)

#define S5P64XX_GPK1_PCM2_FSYNC		(0x02 << 4)
#define S5P64XX_GPK1_I2S2_LRCLK		(0x05 << 4)

#define S5P64XX_GPK2_PCM2_SIN		(0x02 << 8)
#define S5P64XX_GPK2_I2S2_SDI		(0x05 << 8)

#define S5P64XX_GPK3_PCM2_DCLK		(0x02 << 12)
#define S5P64XX_GPK3_I2S2_SCLK		(0x05 << 12)

#define S5P64XX_GPK4_PCM2_EXTCLK	(0x02 << 16)
#define S5P64XX_GPK4_I2S2_CDCLK		(0x05 << 16)

