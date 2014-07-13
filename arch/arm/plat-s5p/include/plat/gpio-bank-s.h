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

#define S5P64XX_GPSCON			(S5P64XX_GPS_BASE + 0x00)
#define S5P64XX_GPSDAT			(S5P64XX_GPS_BASE + 0x04)
#define S5P64XX_GPSPUD			(S5P64XX_GPS_BASE + 0x08)
#define S5P64XX_GPSCONSLP		(S5P64XX_GPS_BASE + 0x0c)
#define S5P64XX_GPSPUDSLP		(S5P64XX_GPS_BASE + 0x10)

#define S5P64XX_GPS_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5P64XX_GPS_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5P64XX_GPS_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

#define S5P64XX_GPS0_GPIO0		(0x02 << 0)

#define S5P64XX_GPS1_GPIO1              (0x02 << 2)

#define S5P64XX_GPS2_GPIO2              (0x02 << 4)
#define S5P64XX_GPS2_SDA                (0x03 << 4)

#define S5P64XX_GPS3_GPIO3              (0x02 << 6)
#define S5P64XX_GPS3_SCL                (0x03 << 6)

#define S5P64XX_GPS4_GPIO4              (0x02 << 8)
#define S5P64XX_GPS4_UART4_RTSn         (0x03 << 8)

#define S5P64XX_GPS5_GPIO5              (0x02 << 10)
#define S5P64XX_GPS5_UART4_CTSn         (0x03 << 10)

#define S5P64XX_GPS6_GPIO6              (0x02 << 12)
#define S5P64XX_GPS6_UART4_RXD          (0x03 << 12)

#define S5P64XX_GPS7_GPIO7              (0x02 << 14)
#define S5P64XX_GPS7_UART4_TXD          (0x03 << 14)


