/* linux/arch/arm/plat-s5p64xx/include/plat/gpio-bank-r.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 * 	Ben Dooks <ben@simtec.co.uk>
 * 	http://armlinux.simtec.co.uk/
 *
 * GPIO Bank R register and configuration definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define S5P64XX_GPRCON0			(S5P64XX_GPR_BASE + 0x00)
#define S5P64XX_GPRCON1			(S5P64XX_GPR_BASE + 0x04)
#define S5P64XX_GPRDAT			(S5P64XX_GPR_BASE + 0x08)
#define S5P64XX_GPRPUD			(S5P64XX_GPR_BASE + 0x0c)
#define S5P64XX_GPRCONSLP		(S5P64XX_GPR_BASE + 0x10)
#define S5P64XX_GPRPUDSLP		(S5P64XX_GPR_BASE + 0x14)

#define S5P64XX_GPR_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5P64XX_GPR_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5P64XX_GPR_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5P64XX_GPR0_GPS_QSIGN          (0x02 << 0) //6450

#define S5P64XX_GPR1_GPS_QMAG           (0x02 << 4) //6450

#define S5P64XX_GPR2_GPS_ISIGN          (0x02 << 8) //6450

#define S5P64XX_GPR3_GPS_IMAG           (0x02 << 12) //6450

#define S5P64XX_GPR4_PCM_SOUT           (0x02 << 16) //6450
#define S5P64XX_GPR4_I2S_V40_DO0	(0x05 << 16)

#define S5P64XX_GPR5_I2S_V40_DO1	(0x05 << 20)

#define S5P64XX_GPR6_I2S_V40_DO2	(0x05 << 28)

#define S5P64XX_GPR7_PCM_FSYNC		(0x02 << 0)
#define S5P64XX_GPR7_I2S_V40_LRCLK	(0x05 << 0)

#define S5P64XX_GPR8_PCM_SIN		(0x02 << 4)
#define S5P64XX_GPR8_I2S_V40_DI		(0x05 << 4)

#define S5P64XX_GPR9_I2C_SCL1		(0x06 << 8)

#define S5P64XX_GPR10_I2C_SDA1		(0x06 << 12)

#define S5P64XX_GPR11_I2C_SCL0		(0x03 << 16)//6450

#define S5P64XX_GPR12_I2C_SDA0		(0x03 << 20)//6450

#define S5P64XX_GPR13_PCM_DCLK	        (0x02 << 24)//6450
#define S5P64XX_GPR13_I2S_V40_SCLK	(0x05 << 24)//6450

#define S5P64XX_GPR14_PCM_EXTCLK	(0x02 << 28)
#define S5P64XX_GPR14_I2S_V40_CDCLK	(0x05 << 28)
