/* linux/arch/arm/plat-s5p64xx/include/plat/gpio-bank-g.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 * 	Ben Dooks <ben@simtec.co.uk>
 * 	http://armlinux.simtec.co.uk/
 *
 * GPIO Bank G register and configuration definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define S5P64XX_GPGCON0			(S5P64XX_GPG_BASE + 0x00)//6440
#define S5P64XX_GPGCON1                 (S5P64XX_GPG_BASE + 0x04)//6440
#define S5P64XX_GPGDAT			(S5P64XX_GPG_BASE + 0x08)
#define S5P64XX_GPGPUD			(S5P64XX_GPG_BASE + 0x0c)
#define S5P64XX_GPGCONSLP		(S5P64XX_GPG_BASE + 0x10)
#define S5P64XX_GPGPUDSLP		(S5P64XX_GPG_BASE + 0x14)

#define S5P64XX_GPG_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5P64XX_GPG_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5P64XX_GPG_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5P64XX_GPG0_MMC0_CLK		(0x02 << 0)
#define S5P64XX_GPG0_EINT_G5_0		(0x07 << 0)

#define S5P64XX_GPG1_MMC0_CMD		(0x02 << 4)
#define S5P64XX_GPG1_EINT_G5_1		(0x07 << 4)

#define S5P64XX_GPG2_MMC0_DATA0		(0x02 << 8)
#define S5P64XX_GPG2_EINT_G5_2		(0x07 << 8)

#define S5P64XX_GPG3_MMC0_DATA1		(0x02 << 12)
#define S5P64XX_GPG3_EINT_G5_3		(0x07 << 12)

#define S5P64XX_GPG4_MMC0_DATA2		(0x02 << 16)
#define S5P64XX_GPG4_EINT_G5_4		(0x07 << 16)

#define S5P64XX_GPG5_MMC0_DATA3		(0x02 << 20)
#define S5P64XX_GPG5_EINT_G5_5		(0x07 << 20)

#define S5P64XX_GPG6_MMC0_CDn		(0x02 << 24)
#define S5P64XX_GPG6_EINT_G5_6		(0x07 << 24)

#define S5P64XX_GPG7_MMC3_CLK           (0x02 << 28)//6450
#define S5P64XX_GPG7_MMC2_CLK           (0x03 << 28)//6450

#define S5P64XX_GPG8_MMC3_CMD           (0x02 << 0)//6450
#define S5P64XX_GPG8_MMC2_CMD           (0x03 << 0)//6450

#define S5P64XX_GPG9_MMC3_DAT0          (0x02 << 4)//6450
#define S5P64XX_GPG9_MMC2_DAT0          (0x03 << 4)//6450

#define S5P64XX_GPG10_MMC3_DAT1         (0x02 << 8)//6450
#define S5P64XX_GPG10_MMC2_DAT1         (0x03 << 8)//6450

#define S5P64XX_GPG11_MMC3_DAT2         (0x02 << 12)//6450
#define S5P64XX_GPG11_MMC2_DAT2         (0x03 << 12)//6450

#define S5P64XX_GPG12_MMC3_DAT3         (0x02 << 16)//6450
#define S5P64XX_GPG12_MMC2_DAT3         (0x03 << 16)//6450

#define S5P64XX_GPG13_MMC3_DAT_CDN      (0x02 << 20)//6450











