/* linux/arch/arm/plat-s5p64xx/include/plat/gpio-bank-a.h
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

#define S5P64XX_GPQCON			(S5P64XX_GPQ_BASE + 0x00)
#define S5P64XX_GPQDAT			(S5P64XX_GPQ_BASE + 0x04)
#define S5P64XX_GPQPUD			(S5P64XX_GPQ_BASE + 0x08)
#define S5P64XX_GPQCONSLP		(S5P64XX_GPQ_BASE + 0x0c)
#define S5P64XX_GPQPUDSLP		(S5P64XX_GPQ_BASE + 0x10)

#define S5P64XX_GPQ_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5P64XX_GPQ_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5P64XX_GPQ_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5P64XX_GPQ0_CAMIF_CLK_OUT	(0x02 << 0)

#define S5P64XX_GPQ1_CAMIF_RSTOUTn      (0x02 << 2)

#define S5P64XX_GPQ2_CAMIF_Pixel_CLK_IN (0x02 << 4)

#define S5P64XX_GPQ3_CAMIF_VSYNC        (0x02 << 6)

#define S5P64XX_GPQ4_CAMIF_HREF         (0x02 << 8)

#define S5P64XX_GPQ5_CAMIF_FIELD        (0x02 << 10)

#define S5P64XX_GPQ6_CAMIF_DATA0       	(0x02 << 12)

#define S5P64XX_GPQ7_CAMIF_DATA1        (0x02 << 14)

#define S5P64XX_GPQ8_CAMIF_DATA2       	(0x02 << 16)

#define S5P64XX_GPQ9_CAMIF_DATA3        (0x02 << 18)

#define S5P64XX_GPQ10_CAMIF_DATA4       (0x02 << 20)

#define S5P64XX_GPQ11_CAMIF_DATA5       (0x02 << 22)

#define S5P64XX_GPQ12_CAMIF_DATA6       (0x02 << 24)

#define S5P64XX_GPQ13_CAMIF_DATA7       (0x02 << 26)


