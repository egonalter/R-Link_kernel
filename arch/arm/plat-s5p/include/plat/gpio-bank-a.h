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

#define S5P64XX_GPACON			(S5P64XX_GPA_BASE + 0x00)
#define S5P64XX_GPADAT			(S5P64XX_GPA_BASE + 0x04)
#define S5P64XX_GPAPUD			(S5P64XX_GPA_BASE + 0x08)
#define S5P64XX_GPACONSLP		(S5P64XX_GPA_BASE + 0x0c)
#define S5P64XX_GPAPUDSLP		(S5P64XX_GPA_BASE + 0x10)

#define S5P64XX_GPA_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5P64XX_GPA_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5P64XX_GPA_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5P64XX_GPA0_UART_RXD0		(0x02 << 0)
#define S5P64XX_GPA0_EINT_G1_0		(0x07 << 0)

#define S5P64XX_GPA1_UART_TXD0		(0x02 << 4)
#define S5P64XX_GPA1_EINT_G1_1		(0x07 << 4)

#define S5P64XX_GPA2_UART_nCTS0		(0x02 << 8)
#define S5P64XX_GPA2_EINT_G1_2		(0x07 << 8)

#define S5P64XX_GPA3_UART_nRTS0		(0x02 << 12)
#define S5P64XX_GPA3_EINT_G1_3		(0x07 << 12)

#define S5P64XX_GPA4_UART_RXD1		(0x02 << 16)
#define S5P64XX_GPA4_EINT_G1_4		(0x07 << 16)

#define S5P64XX_GPA5_UART_TXD1		(0x02 << 20)
#define S5P64XX_GPA5_EINT_G1_5		(0x07 << 20)

