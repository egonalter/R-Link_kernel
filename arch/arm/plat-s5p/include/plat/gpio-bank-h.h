/* linux/arch/arm/plat-s5p64xx/include/plat/gpio-bank-h.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 * 	Ben Dooks <ben@simtec.co.uk>
 * 	http://armlinux.simtec.co.uk/
 *
 * GPIO Bank H register and configuration definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define S5P64XX_GPHCON0			(S5P64XX_GPH_BASE + 0x00)
#define S5P64XX_GPHCON1			(S5P64XX_GPH_BASE + 0x04)
#define S5P64XX_GPHDAT			(S5P64XX_GPH_BASE + 0x08)
#define S5P64XX_GPHPUD			(S5P64XX_GPH_BASE + 0x0c)
#define S5P64XX_GPHCONSLP		(S5P64XX_GPH_BASE + 0x10)
#define S5P64XX_GPHPUDSLP		(S5P64XX_GPH_BASE + 0x14)

#define S5P64XX_GPH_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5P64XX_GPH_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5P64XX_GPH_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5P64XX_GPH0_MMC1_CLK		(0x02 << 0)
#define S5P64XX_GPH0_EINT_G6_0		(0x07 << 0)

#define S5P64XX_GPH1_MMC1_CMD		(0x02 << 4)
#define S5P64XX_GPH1_EINT_G6_1		(0x07 << 4)

#define S5P64XX_GPH2_MMC1_DATA0		(0x02 << 8)
#define S5P64XX_GPH2_EINT_G6_2		(0x07 << 8)

#define S5P64XX_GPH3_MMC1_DATA1		(0x02 << 12)
#define S5P64XX_GPH3_EINT_G6_3		(0x07 << 12)

#define S5P64XX_GPH4_MMC1_DATA2		(0x02 << 16)
#define S5P64XX_GPH4_EINT_G6_4		(0x07 << 16)

#define S5P64XX_GPH5_MMC1_DATA3		(0x02 << 20)
#define S5P64XX_GPH5_EINT_G6_5		(0x07 << 20)

#define S5P64XX_GPH6_MMC1_DATA4		(0x02 << 24)
#define S5P64XX_GPH6_EINT_G6_6		(0x07 << 24)

#define S5P64XX_GPH7_MMC1_DATA5		(0x02 << 28)
#define S5P64XX_GPH7_EINT_G6_7		(0x07 << 28)

#define S5P64XX_GPH8_MMC1_DATA6		(0x02 << 0)
#define S5P64XX_GPH8_EINT_G6_8		(0x07 << 0)

#define S5P64XX_GPH9_MMC1_DATA7		(0x02 << 4)
#define S5P64XX_GPH9_EINT_G6_9		(0x07 << 4)

