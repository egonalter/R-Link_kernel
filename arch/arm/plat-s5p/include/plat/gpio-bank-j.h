/* linux/arch/arm/plat-s5p64xx/include/plat/gpio-bank-j.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 * 	Ben Dooks <ben@simtec.co.uk>
 * 	http://armlinux.simtec.co.uk/
 *
 * GPIO Bank J register and configuration definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define S5P64XX_GPJCON			(S5P64XX_GPJ_BASE + 0x00)
#define S5P64XX_GPJDAT			(S5P64XX_GPJ_BASE + 0x04)
#define S5P64XX_GPJPUD			(S5P64XX_GPJ_BASE + 0x08)
#define S5P64XX_GPJCONSLP		(S5P64XX_GPJ_BASE + 0x0c)
#define S5P64XX_GPJPUDSLP		(S5P64XX_GPJ_BASE + 0x10)

#define S5P64XX_GPJ_CONMASK(__gpio)	(0x3 << ((__gpio) * 2))
#define S5P64XX_GPJ_INPUT(__gpio)	(0x0 << ((__gpio) * 2))
#define S5P64XX_GPJ_OUTPUT(__gpio)	(0x1 << ((__gpio) * 2))

#define S5P64XX_GPJ0_VD16			(0x02 << 0)
#define S5P64XX_GPJ0_EXTSRAM_nCS		(0x03 << 0)

#define S5P64XX_GPJ1_VD17			(0x02 << 2)
#define S5P64XX_GPJ1_EXTSRAM_nOE		(0x03 << 2)

#define S5P64XX_GPJ2_VD18			(0x02 << 4)
#define S5P64XX_GPJ2_EXTSRAM_nWE		(0x03 << 4)

#define S5P64XX_GPJ3_VD19			(0x02 << 6)
#define S5P64XX_GPJ3_EXTSRAM_nBE0		(0x03 << 6)

#define S5P64XX_GPJ4_VD20			(0x02 << 8)
#define S5P64XX_GPJ4_EXTSRAM_nBE1		(0x03 << 8)

#define S5P64XX_GPJ5_VD21			(0x02 << 10)
#define S5P64XX_GPJ5_EXTSRAM_ADDR1		(0x03 << 10)

#define S5P64XX_GPJ6_VD22			(0x02 << 12)
#define S5P64XX_GPJ6_EXTSRAM_ADDR2		(0x03 << 12)

#define S5P64XX_GPJ7_VD23			(0x02 << 14)
#define S5P64XX_GPJ7_EXTSRAM_ADDR3		(0x03 << 14)

#define S5P64XX_GPJ8_LCD_HSYNC			(0x02 << 16)
#define S5P64XX_GPJ8_EXTSRAM_ADDR4		(0x03 << 16)

#define S5P64XX_GPJ9_LCD_VSYNC			(0x02 << 18)
#define S5P64XX_GPJ9_EXTSRAM_ADDR5		(0x03 << 18)

#define S5P64XX_GPJ10_LCD_VDEN			(0x02 << 20)
#define S5P64XX_GPJ10_EXTSRAM_ADDR6		(0x03 << 20)

#define S5P64XX_GPJ11_LCD_VCLK			(0x02 << 22)
#define S5P64XX_GPJ11_EXTSRAM_ADDR7		(0x03 << 22)
