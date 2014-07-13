/* linux/arch/arm/plat-s5p64xx/include/plat/gpio-bank-b.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 * 	Ben Dooks <ben@simtec.co.uk>
 * 	http://armlinux.simtec.co.uk/
 *
 * GPIO Bank B register and configuration definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define S5P64XX_GPBCON			(S5P64XX_GPB_BASE + 0x00)
#define S5P64XX_GPBDAT			(S5P64XX_GPB_BASE + 0x04)
#define S5P64XX_GPBPUD			(S5P64XX_GPB_BASE + 0x08)
#define S5P64XX_GPBCONSLP		(S5P64XX_GPB_BASE + 0x0c)
#define S5P64XX_GPBPUDSLP		(S5P64XX_GPB_BASE + 0x10)

#define S5P64XX_GPB_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5P64XX_GPB_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5P64XX_GPB_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5P64XX_GPB0_UART_RXD2		(0x02 << 0)
#define S5P64XX_GPB0_EINT_G1_8		(0x07 << 0)

#define S5P64XX_GPB1_UART_TXD2		(0x02 << 4)
#define S5P64XX_GPB1_EINT_G1_9		(0x07 << 4)

#define S5P64XX_GPB2_UART_RXD3		(0x02 << 8)
#define S5P64XX_GPB2_I2C_SCL1		(0x06 << 8)
#define S5P64XX_GPB2_EINT_G1_10		(0x07 << 8)

#define S5P64XX_GPB3_UART_TXD3		(0x02 << 12)
#define S5P64XX_GPB3_I2C_SDA1		(0x06 << 12)
#define S5P64XX_GPB3_EINT_G1_11		(0x07 << 12)

#define S5P64XX_GPB4_PCM1_DCLK          (0x04 << 16)//6450
#define S5P64XX_GPB4_I2S_V40_BCLK       (0x05 << 16)//6450
#define S5P64XX_GPB4_ts_ERROR		(0x06 << 16)
#define S5P64XX_GPB4_EINT_G1_12		(0x07 << 16)

#define S5P64XX_GPB5_I2C_SCL0		(0x02 << 20)
#define S5P64XX_GPB5_UART_nCTS1         (0x03 << 20)
#define S5P64XX_GPB5_EINT_G1_13		(0x07 << 20)

#define S5P64XX_GPB6_I2C_SDA0		(0x02 << 24)
#define S5P64XX_GPB6_UART_nRTS1         (0x03 << 24)
#define S5P64XX_GPB6_EINT_G1_14		(0x07 << 24)

