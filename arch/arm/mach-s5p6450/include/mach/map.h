/* linux/arch/arm/mach-s5p6450/include/mach/map.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S5P6450 - Memory map definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_MAP_H
#define __ASM_ARCH_MAP_H __FILE__

#include <plat/map-base.h>
#include <plat/map-s5p.h>

#define S5P6450_PA_CHIPID	(0xE0000000)
#define S5P_PA_CHIPID		S5P6450_PA_CHIPID

#define S5P6450_PA_SYSCON	(0xE0100000)
#define S5P6450_PA_CLK		(S5P6450_PA_SYSCON + 0x0)
#define S5P_PA_SYSCON		S5P6450_PA_SYSCON

#define S5P6450_PA_GPIO		(0xE0308000)
#define S5P_PA_GPIO		S5P6450_PA_GPIO

#define S5P64XX_VA_GPIO         S3C_ADDR(0x00500000)
#define S5P64XX_SZ_GPIO         SZ_4K


/* NAND flash controller */
#define S5P64XX_PA_NAND         (0xE7100000)
#define S5P64XX_SZ_NAND         SZ_1M
#define S5P_PA_NAND         	S5P64XX_PA_NAND
#define S5P_SZ_NAND             SZ_1M

#define S5P6450_PA_VIC0		(0xE4000000)
#define S5P_PA_VIC0		S5P6450_PA_VIC0

#define S5P6450_PA_VIC1		(0xE4100000)
#define S5P_PA_VIC1		S5P6450_PA_VIC1

#define S5P64XX_VA_LCD          S3C_VA_LCD
#define S5P64XX_PA_LCD	   	(0xEE000000)
#define S5P64XX_SZ_LCD		SZ_1M

#define S5P64XX_PA_ADC		(0xF3000000)
#define S3C_PA_ADC              S5P64XX_PA_ADC


#define S5P64XX_VA_SROMC        S3C_VA_SROMC
#define S5P64XX_PA_SROMC        (0xE7000000)
#define S5P64XX_SZ_SROMC        SZ_1M

#define S5P6450_PA_TIMER	(0xEA000000)
#define S5P_PA_TIMER		S5P6450_PA_TIMER

#define S5P6450_PA_RTC		(0xEA100000) //0xEA1000000
#define S5P_PA_RTC		S5P6450_PA_RTC

#define S5P6450_PA_WDT		(0xEA200000)
#define S5P_PA_WDT		S5P6450_PA_WDT

#define S5P6450_PA_UART		(0xEC800000)

#define S5P_PA_UART0		(S5P6450_PA_UART + 0x0)
#define S5P_PA_UART1		(S5P6450_PA_UART + 0x400)
#define S5P_PA_UART2		(S5P6450_PA_UART + 0x800)
#define S5P_PA_UART3		(S5P6450_PA_UART + 0xC00)
#define S5P_PA_UART4            (S5P6450_PA_UART + 0x1000)
#define S5P_PA_UART5            (0xEC000000)


/* DMC */
#define S5P64XX_PA_DMC          (0xE6000000)
#define S5P64XX_VA_DMC          S3C_ADDR(0x00200000)
#define S5P64XX_SZ_DMC          SZ_1M


#define S5P_SZ_UART		SZ_256

#define S5P6450_PA_IIC0		(0xEC100000)
#define S5P6450_PA_IIC1         (0xEC200000)

/* FIMC  */
#define S5P64XX_PA_FIMC0         (0xEE100000)
#define S5P64XX_SZ_FIMC0         SZ_1M 

#define S5P64XX_PA_ROTATOR       (0xEC700000)
#define S5P64XX_SZ_ROTATOR         SZ_1M 

#define S5P64XX_PA_G2D       (0xF0300000)
#define S5P64XX_SZ_G2D         SZ_1M
#define S5P64XX_PA_G3D       (0xF0000000)
#define S5P64XX_SZ_G3D         SZ_1M 


#define S5P6450_PA_HSOTG	(0xED100000) 
//#define S5P6450_PA_HSOTG	(0x74100000)
#define S5P6450_SZ_HSOTG        SZ_1M


#define S5P6450_PA_USB_EHCI     (0xED300000)
#define S5P_PA_USB_EHCI         S5P6450_PA_USB_EHCI
#define S5P_SZ_USB_EHCI         SZ_1M

#define S5P6450_PA_USB_OHCI     (0xED400000)
#define S5P_PA_USB_OHCI         S5P6450_PA_USB_OHCI
#define S5P_SZ_USB_OHCI         SZ_1M


#define S5P64XX_VA_HOSTIFB      S3C_ADDR(0x00C00000)

#define S5P64XX_PA_HOSTIFB      (0xED300000)
#define S5P64XX_SZ_HOSTIFB      SZ_1M


#define S5P6450_PA_HSMMC0	(0xED800000)
#define S5P6450_PA_HSMMC1	(0xED900000)
#define S5P6450_PA_HSMMC2	(0xEDA00000)

/* compatibiltiy defines. */
#define S3C_PA_TIMER            S5P6450_PA_TIMER
#define S3C_PA_HSMMC0           S5P6450_PA_HSMMC0
#define S3C_PA_HSMMC1           S5P6450_PA_HSMMC1
#define S3C_PA_HSMMC2           S5P6450_PA_HSMMC2
#define S3C_SZ_HSMMC            SZ_4K 



#define S5P6450_PA_EMMC44       (0xED500000)
#define S5P_PA_MSHC				S5P6450_PA_EMMC44

#define S5P6450_PA_SDRAM	(0x20000000)
#define S5P_PA_SDRAM		S5P6450_PA_SDRAM

#define S5P64XX_PA_IIS_V40      (0xF2000000)
#define S5P_SZ_IIS              SZ_8K

#define S5P64XX_PA_IIS1         (0xF2800000)
#define S5P_SZ_IIS1              SZ_8K

#define S5P64XX_PA_IIS2         (0xF2900000)
#define S5P_SZ_IIS2              SZ_8K

#define S5P64XX_PA_PCM0         (0xF2100000)
#define S5P_SZ_PCM0              SZ_8K

#define S5P64XX_PA_PCM1         (0xF2200000)
#define S5P_SZ_PCM1		SZ_8K

#define S5P64XX_PA_PCM2         (0xF2300000)
#define S5P_SZ_PCM2             SZ_8K

/* SPI */
#define S5P6450_PA_SPI0		(0xEC400000)
#define S5P6450_PA_SPI1		(0xEC500000)
#define S5C_SZ_SPI0		SZ_4K
#define S5C_SZ_SPI1		SZ_4K

/* GPS */
#define S5P64XX_PA_GPS_APBIF   (0xEC900000)
#define S5P_PA_GPS		S5P64XX_PA_GPS_APBIF
#define S5P_SZ_GPS		SZ_256
/* DMA controller */
#define S5P64XX_PA_DMA          (0xE9000000)

#define S5P_PA_DMA              S5P64XX_PA_DMA

#define S5P_PA_PDMA             S5P64XX_PA_DMA

/* compatibiltiy defines. */
#define S3C_PA_UART		S5P6450_PA_UART
#define S3C_PA_UART5 		S5P_PA_UART5
#define S3C_PA_IIC		S5P6450_PA_IIC0
#define S3C_PA_IIC1             S5P6450_PA_IIC1


/* USB OTG */
#define S5P_VA_OTG              S3C_VA_OTG
#define S5P_PA_OTG             (0xED100000)
#define S5P_PA_OTGPHY          (0xED200000)
/*-----------------------------------------------------------------------*/
#define S5P64XX_VA_OTG          S3C_ADDR(0x00E00000)
#define S5P64XX_PA_OTG          (0xED100000)
#define S5P64XX_SZ_OTG          SZ_1M

#define S5P64XX_VA_OTGSFR       S3C_ADDR(0x00F00000)
#define S5P64XX_PA_OTGSFR       (0xED200000)
#define S5P64XX_SZ_OTGSFR       SZ_1M
#define S5P_SZ_OTG              S5P64XX_SZ_OTG
/*-----------------------------------------------------------------------*/
#define S5P64XX_SZ_OTG          SZ_1M


#define S5P64XX_PA_ADC          (0xF3000000)
#define S3C_PA_ADC              S5P64XX_PA_ADC



/* place VICs close together */
#define S3C_VA_VIC0             (S3C_VA_IRQ + 0x00)
#define S3C_VA_VIC1             (S3C_VA_IRQ + 0x10000)

#endif /* __ASM_ARCH_MAP_H */
