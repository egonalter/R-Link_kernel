/* linux/arch/arm/plat-s5p/include/plat/gpio-bank-d.h
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

#define S5P64XX_GPDCON			(S5P64XX_GPD_BASE + 0x00)
#define S5P64XX_GPDDAT			(S5P64XX_GPD_BASE + 0x04)
#define S5P64XX_GPDPUD			(S5P64XX_GPD_BASE + 0x08)
#define S5P64XX_GPDCONSLP		(S5P64XX_GPD_BASE + 0x0c)
#define S5P64XX_GPDPUDSLP		(S5P64XX_GPD_BASE + 0x10)

#define S5P64XX_GPD_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5P64XX_GPD_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5P64XX_GPD_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5P64XX_GPD0_UART4_RXD		(0x02 << 0)
#define S5P64XX_GPD0_USI_SDA_MI_RXD     (0x03 << 0)

#define S5P64XX_GPD1_UART4_TXD          (0x02 << 4)
#define S5P64XX_GPD1_USI_SCL_SI         (0x03 << 4)

#define S5P64XX_GPD2_UART5_RXD          (0x02 << 8)

#define S5P64XX_GPD3_UART5_TXD          (0x02 << 12)

#define S5P64XX_GPD4_UART4_CTS          (0x02 << 16)
#define S5P64XX_GPD4_USI_SPICLK_CTS     (0x03 << 16)

#define S5P64XX_GPD5_UART4_RTS		(0x02 << 20)
#define S5P64XX_GPD5_USI_NSS_RTS        (0x03 << 20)

#define S5P64XX_GPD6_GPS_BB_SDA         (0x02 << 24)
#define S5P64XX_GPD6_UART5_CTS          (0x03 << 24)

#define S5P64XX_GPD7_GPS_BB_SCL         (0x02 << 28)
#define S5P64XX_GPD7_UART5_RTS          (0x03 << 28)










