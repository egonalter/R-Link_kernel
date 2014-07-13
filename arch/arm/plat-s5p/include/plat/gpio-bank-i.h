/* linux/arch/arm/plat-s5p64xx/include/plat/gpio-bank-i.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 * 	Ben Dooks <ben@simtec.co.uk>
 * 	http://armlinux.simtec.co.uk/
 *
 * GPIO Bank I register and configuration definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define S5P64XX_GPICON			(S5P64XX_GPI_BASE + 0x00)
#define S5P64XX_GPIDAT			(S5P64XX_GPI_BASE + 0x04)
#define S5P64XX_GPIPUD			(S5P64XX_GPI_BASE + 0x08)
#define S5P64XX_GPICONSLP		(S5P64XX_GPI_BASE + 0x0c)
#define S5P64XX_GPIPUDSLP		(S5P64XX_GPI_BASE + 0x10)

#define S5P64XX_GPI_CONMASK(__gpio)	(0x3 << ((__gpio) * 2))
#define S5P64XX_GPI_INPUT(__gpio)	(0x0 << ((__gpio) * 2))
#define S5P64XX_GPI_OUTPUT(__gpio)	(0x1 << ((__gpio) * 2))

#define S5P64XX_GPI0_VD0			(0x02 << 0)
#define S5P64XX_GPI0_EXTSRAM_DATA0		(0x03 << 0)

#define S5P64XX_GPI1_VD1			(0x02 << 2)
#define S5P64XX_GPI1_EXTSRAM_DATA1		(0x03 << 2)

#define S5P64XX_GPI2_VD2			(0x02 << 4)
#define S5P64XX_GPI2_EXTSRAM_DATA2		(0x03 << 4)

#define S5P64XX_GPI3_VD3			(0x02 << 6)
#define S5P64XX_GPI3_EXTSRAM_DATA3		(0x03 << 6)

#define S5P64XX_GPI4_VD4			(0x02 << 8)
#define S5P64XX_GPI4_EXTSRAM_DATA4		(0x03 << 8)

#define S5P64XX_GPI5_VD5			(0x02 << 10)
#define S5P64XX_GPI5_EXTSRAM_DATA5		(0x03 << 10)

#define S5P64XX_GPI6_VD6			(0x02 << 12)
#define S5P64XX_GPI6_EXTSRAM_DATA6		(0x03 << 12)

#define S5P64XX_GPI7_VD7			(0x02 << 14)
#define S5P64XX_GPI7_EXTSRAM_DATA7		(0x03 << 14)

#define S5P64XX_GPI8_VD8			(0x02 << 16)
#define S5P64XX_GPI8_EXTSRAM_DATA8		(0x03 << 16)

#define S5P64XX_GPI9_VD9			(0x02 << 18)
#define S5P64XX_GPI9_EXTSRAM_DATA9		(0x03 << 18)

#define S5P64XX_GPI10_VD10			(0x02 << 20)
#define S5P64XX_GPI10_EXTSRAM_DATA10		(0x03 << 20)

#define S5P64XX_GPI11_VD11			(0x02 << 22)
#define S5P64XX_GPI11_EXTSRAM_DATA11		(0x03 << 22)

#define S5P64XX_GPI12_VD12			(0x02 << 24)
#define S5P64XX_GPI12_EXTSRAM_DATA12		(0x03 << 24)

#define S5P64XX_GPI13_VD13			(0x02 << 26)
#define S5P64XX_GPI13_EXTSRAM_DATA13		(0x03 << 26)

#define S5P64XX_GPI14_VD14			(0x02 << 28)
#define S5P64XX_GPI14_EXTSRAM_DATA14		(0x03 << 28)

#define S5P64XX_GPI15_VD15			(0x02 << 30)
#define S5P64XX_GPI15_EXTSRAM_DATA15		(0x03 << 30)
