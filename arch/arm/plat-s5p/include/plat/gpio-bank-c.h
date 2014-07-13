/* linux/arch/arm/plat-s5p64xx/include/plat/gpio-bank-c.h
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 * 	Ben Dooks <ben@simtec.co.uk>
 * 	http://armlinux.simtec.co.uk/
 *
 * GPIO Bank C register and configuration definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#define S5P64XX_GPCCON			(S5P64XX_GPC_BASE + 0x00)
#define S5P64XX_GPCDAT			(S5P64XX_GPC_BASE + 0x04)
#define S5P64XX_GPCPUD			(S5P64XX_GPC_BASE + 0x08)
#define S5P64XX_GPCCONSLP		(S5P64XX_GPC_BASE + 0x0c)
#define S5P64XX_GPCPUDSLP		(S5P64XX_GPC_BASE + 0x10)

#define S5P64XX_GPC_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5P64XX_GPC_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5P64XX_GPC_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5P64XX_GPC0_SPI_MISO0		(0x02 << 0)
#define S5P64XX_GPC0_ts_CLK		(0x03 << 0)
#define S5P64XX_GPC0_PCM_SIN1           (0x04 << 0)//6450
#define S5P64XX_GPC0_I2S1_SDI           (0x05 << 0)//6450
#define S5P64XX_GPC0_EINT_G2_0		(0x07 << 0)

#define S5P64XX_GPC1_SPI_CLK0		(0x02 << 4)
#define S5P64XX_GPC1_ts_SYNC		(0x03 << 4)
#define S5P64XX_GPC1_PCM_SOUT1          (0x04 << 4)//6450
#define S5P64XX_GPC1_I2S1_SDO           (0x05 << 4)//6450
#define S5P64XX_GPC1_EINT_G2_1		(0x07 << 4)

#define S5P64XX_GPC2_SPI_MOSI0		(0x02 << 8)
#define S5P64XX_GPC2_ts_VALID		(0x03 << 8)
#define S5P64XX_GPC2_pcmFSYNC1           (0x04 << 8)//6450
#define S5P64XX_GPC2_i2sLRCK1           (0x05 << 8)//6450
#define S5P64XX_GPC2_EINT_G2_2		(0x07 << 8)

#define S5P64XX_GPC3_SPI_nCS0		(0x02 << 12)
#define S5P64XX_GPC3_ts_DATA		(0x03 << 12)
#define S5P64XX_GPC3_pcmEXTCLK1         (0x04 << 12)//6450
#define S5P64XX_GPC3_I2S1_CDCLK         (0x05 << 12)//6450
#define S5P64XX_GPC3_EINT_G2_3		(0x07 << 12)

#define S5P64XX_GPC4_SPI_MISO1		(0x02 << 16)
#define S5P64XX_GPC4_EINT_G2_4		(0x07 << 16)

#define S5P64XX_GPC5_SPI_CLK1		(0x02 << 20)
#define S5P64XX_GPC5_EINT_G2_5		(0x07 << 20)

#define S5P64XX_GPC6_SPI_MOSI1		(0x02 << 24)
#define S5P64XX_GPC6_EINT_G2_6		(0x07 << 24)

#define S5P64XX_GPC7_SPI_nCS1		(0x02 << 28)
#define S5P64XX_GPC7_EINT_G2_7		(0x07 << 28)

