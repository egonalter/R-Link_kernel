/* linux/arch/arm/plat-s5p64xx/include/plat/gpio-bank-n.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 * 	Ben Dooks <ben@simtec.co.uk>
 * 	http://armlinux.simtec.co.uk/
 *
 * GPIO Bank N register and configuration definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define S5P64XX_GPNCON			(S5P64XX_GPN_BASE + 0x00)
#define S5P64XX_GPNDAT			(S5P64XX_GPN_BASE + 0x04)
#define S5P64XX_GPNPUD			(S5P64XX_GPN_BASE + 0x08)

#define S5P64XX_GPN_CONMASK(__gpio)	(0x3 << ((__gpio) * 2))
#define S5P64XX_GPN_INPUT(__gpio)	(0x0 << ((__gpio) * 2))
#define S5P64XX_GPN_OUTPUT(__gpio)	(0x1 << ((__gpio) * 2))

#define S5P64XX_GPN0_EINT0		(0x02 << 0)
#define S5P64XX_GPN1_EINT1		(0x02 << 2)
#define S5P64XX_GPN2_EINT2		(0x02 << 4)
#define S5P64XX_GPN3_EINT3		(0x02 << 6)
#define S5P64XX_GPN4_EINT4		(0x02 << 8)
#define S5P64XX_GPN5_EINT5		(0x02 << 10)
#define S5P64XX_GPN6_EINT6		(0x02 << 12)
#define S5P64XX_GPN7_EINT7		(0x02 << 14)
#define S5P64XX_GPN8_EINT8		(0x02 << 16)
#define S5P64XX_GPN9_EINT9		(0x02 << 18)
#define S5P64XX_GPN10_EINT10		(0x02 << 20)
#define S5P64XX_GPN11_EINT11		(0x02 << 22)
#define S5P64XX_GPN12_EINT12		(0x02 << 24)
#define S5P64XX_GPN13_EINT13		(0x02 << 26)
#define S5P64XX_GPN14_EINT14		(0x02 << 28)
#define S5P64XX_GPN15_EINT15		(0x02 << 30)
