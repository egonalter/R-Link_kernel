/* linux/arch/arm/plat-s5p64xx/include/plat/gpio-bank-f.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 * 	Ben Dooks <ben@simtec.co.uk>
 * 	http://armlinux.simtec.co.uk/
 *
 * GPIO Bank F register and configuration definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define S5P64XX_GPFCON			(S5P64XX_GPF_BASE + 0x00)
#define S5P64XX_GPFDAT			(S5P64XX_GPF_BASE + 0x04)
#define S5P64XX_GPFPUD			(S5P64XX_GPF_BASE + 0x08)
#define S5P64XX_GPFCONSLP		(S5P64XX_GPF_BASE + 0x0c)
#define S5P64XX_GPFPUDSLP		(S5P64XX_GPF_BASE + 0x10)

#define S5P64XX_GPF_CONMASK(__gpio)	(0x3 << ((__gpio) * 2))
#define S5P64XX_GPF_INPUT(__gpio)	(0x0 << ((__gpio) * 2))
#define S5P64XX_GPF_OUTPUT(__gpio)	(0x1 << ((__gpio) * 2))

#define S5P64XX_GPF14_PWM_TOUT0		(0x02 << 28)

#define S5P64XX_GPF15_PWM_TOUT1		(0x02 << 30)

