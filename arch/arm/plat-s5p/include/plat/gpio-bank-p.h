/* linux/arch/arm/plat-s5p64xx/include/plat/gpio-bank-p.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 * 	Ben Dooks <ben@simtec.co.uk>
 * 	http://armlinux.simtec.co.uk/
 *
 * GPIO Bank P register and configuration definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define S5P64XX_GPPCON			(S5P64XX_GPP_BASE + 0x00)
#define S5P64XX_GPPDAT			(S5P64XX_GPP_BASE + 0x04)
#define S5P64XX_GPPPUD			(S5P64XX_GPP_BASE + 0x08)
#define S5P64XX_GPPCONSLP		(S5P64XX_GPP_BASE + 0x0c)
#define S5P64XX_GPPPUDSLP		(S5P64XX_GPP_BASE + 0x10)
#define S5P64XX_GPPCON_GPS		(S5P64XX_GPP_BASE + 0x14)

#define S5P64XX_GPP_CONMASK(__gpio)	(0x3 << ((__gpio) * 2))
#define S5P64XX_GPP_INPUT(__gpio)	(0x0 << ((__gpio) * 2))
#define S5P64XX_GPP_OUTPUT(__gpio)	(0x1 << ((__gpio) * 2))

#define S5P64XX_GPP3_MEM0_ALE		(0x02 << 6)
#define S5P64XX_GPP3_EINT_G8_3		(0x03 << 6)

#define S5P64XX_GPP4_MEM0_CLE		(0x02 << 8)
#define S5P64XX_GPP4_EINT_G8_4		(0x03 << 8)

#define S5P64XX_GPP5_MEM0_FWE		(0x02 << 10)
#define S5P64XX_GPP5_EINT_G8_5		(0x03 << 10)

#define S5P64XX_GPP6_MEM0_FRE		(0x02 << 12)
#define S5P64XX_GPP6_EINT_G8_6		(0x03 << 12)

#define S5P64XX_GPP7_MEM0_RnB		(0x02 << 14)
#define S5P64XX_GPP7_EINT_G8_7		(0x03 << 14)

#define S5P64XX_GPP8_GPS_BB_MCLK        (0x02 << 16)//6450
#define S5P64XX_GPP8_EINT_G8_8		(0x03 << 16)

#define S5P64XX_GPP9_GPS_BB_SYNC        (0x02 << 18)//6450
#define S5P64XX_GPP9_EINT_G8_9		(0x03 << 18)

#define S5P64XX_GPP10_GPS_BB_EPOCH      (0x02 << 20)//6450
#define S5P64XX_GPP10_EINT_G8_10	(0x03 << 20)

#define S5P64XX_GPP11_GPS_SLPn          (0x02 << 22)//6450
#define S5P64XX_GPP11_EINT_G8_11        (0x03 << 22)//6450

#define S5P64XX_GPP13_GPSRSTOUT         (0x02 << 26)//6450
#define S5P64XX_GPP13_EINT_G8_13        (0x03 << 26)//6450

#define S5P64XX_GPP14_BTRSTOUT          (0x02 << 28)//6450
#define S5P64XX_GPP14_EINT_G8_14        (0x03 << 28)//6450

