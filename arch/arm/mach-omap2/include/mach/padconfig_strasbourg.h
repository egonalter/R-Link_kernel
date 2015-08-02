/*
 * Copyright (C) 2009 TomTom BV <http://www.tomtom.com/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
 
#ifndef PADCONFIG_STRASBOURG_H
#define PADCONFIG_STRASBOURG_H

#include "padconfig_common.h"

/*******************************/
/*********** TABLES ************/ 
/*******************************/

#define PADCONFIG_SETTINGS_XLOADER      \
	/*SDRC*/                                                                                               \
	PC_DEFINE(CP(SDRC_D0),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D0*/            \
	PC_DEFINE(CP(SDRC_D1),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D1*/            \
	PC_DEFINE(CP(SDRC_D2),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D2*/            \
	PC_DEFINE(CP(SDRC_D3),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D3*/            \
	PC_DEFINE(CP(SDRC_D4),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D4*/            \
	PC_DEFINE(CP(SDRC_D5),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D5*/            \
	PC_DEFINE(CP(SDRC_D6),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D6*/            \
	PC_DEFINE(CP(SDRC_D7),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D7*/            \
	PC_DEFINE(CP(SDRC_D8),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D8*/            \
	PC_DEFINE(CP(SDRC_D9),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D9*/            \
	PC_DEFINE(CP(SDRC_D10),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D10*/           \
	PC_DEFINE(CP(SDRC_D11),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D11*/           \
	PC_DEFINE(CP(SDRC_D12),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D12*/           \
	PC_DEFINE(CP(SDRC_D13),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D13*/           \
	PC_DEFINE(CP(SDRC_D14),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D14*/           \
	PC_DEFINE(CP(SDRC_D15),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D15*/           \
	PC_DEFINE(CP(SDRC_D16),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D16*/           \
	PC_DEFINE(CP(SDRC_D17),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D17*/           \
	PC_DEFINE(CP(SDRC_D18),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D18*/           \
	PC_DEFINE(CP(SDRC_D19),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D19*/           \
	PC_DEFINE(CP(SDRC_D20),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D20*/           \
	PC_DEFINE(CP(SDRC_D21),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D21*/           \
	PC_DEFINE(CP(SDRC_D22),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D22*/           \
	PC_DEFINE(CP(SDRC_D23),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D23*/           \
	PC_DEFINE(CP(SDRC_D24),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D24*/           \
	PC_DEFINE(CP(SDRC_D25),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D25*/           \
	PC_DEFINE(CP(SDRC_D26),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D26*/           \
	PC_DEFINE(CP(SDRC_D27),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D27*/           \
	PC_DEFINE(CP(SDRC_D28),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D28*/           \
	PC_DEFINE(CP(SDRC_D29),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D29*/           \
	PC_DEFINE(CP(SDRC_D30),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D30*/           \
	PC_DEFINE(CP(SDRC_D31),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D31*/           \
	PC_DEFINE(CP(SDRC_CLK),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_CLK*/           \
	PC_DEFINE(CP(SDRC_DQS0),     PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_DQS0*/          \
	PC_DEFINE(CP(SDRC_DQS1),     PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_DQS1*/          \
	PC_DEFINE(CP(SDRC_DQS2),     PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_DQS2*/          \
	PC_DEFINE(CP(SDRC_DQS3),     PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_DQS3*/          \
                                                                                                               \
	/* MMC lines */                                                                                        \
	PC_DEFINE(CP(SDMMC1_CLK),    PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0)                        \
	PC_DEFINE(CP(SDMMC1_CMD),    PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0)                        \
	PC_DEFINE(CP(SDMMC1_DAT0),   PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0)                        \
	PC_DEFINE(CP(SDMMC1_DAT1),   PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0)                        \
	PC_DEFINE(CP(SDMMC1_DAT2),   PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0)                        \
	PC_DEFINE(CP(SDMMC1_DAT3),   PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0)                        \
	PC_DEFINE(CP(SDMMC1_DAT4),   PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0)                        \
	PC_DEFINE(CP(SDMMC1_DAT5),   PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0)                        \
	PC_DEFINE(CP(SDMMC1_DAT6),   PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0)                        \
	PC_DEFINE(CP(SDMMC1_DAT7),   PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0)                        \
	PC_DEFINE(CP(SDMMC2_CLK),    PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0)                        \
	PC_DEFINE(CP(SDMMC2_CMD),    PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0)                        \
	PC_DEFINE(CP(SDMMC2_DAT0),   PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0)                        \
	PC_DEFINE(CP(SDMMC2_DAT1),   PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0)                        \
	PC_DEFINE(CP(SDMMC2_DAT2),   PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0)                        \
	PC_DEFINE(CP(SDMMC2_DAT3),   PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0)                        \
	PC_DEFINE(CP(SDMMC2_DAT4),   PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0)                        \
	PC_DEFINE(CP(SDMMC2_DAT5),   PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0)                        \
	PC_DEFINE(CP(SDMMC2_DAT6),   PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0)                        \
	PC_DEFINE(CP(SDMMC2_DAT7),   PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0)                        \
                                                                                                               \
	/* USB0 lines */                                                                                       \
	PC_DEFINE(CP(HSUSB0_CLK),    PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0)                        \
	PC_DEFINE(CP(HSUSB0_DATA0),  PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0)                        \
	PC_DEFINE(CP(HSUSB0_DATA1),  PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0)                        \
	PC_DEFINE(CP(HSUSB0_DATA2),  PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0)                        \
	PC_DEFINE(CP(HSUSB0_DATA3),  PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0)                        \
	PC_DEFINE(CP(HSUSB0_DATA4),  PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0)                        \
	PC_DEFINE(CP(HSUSB0_DATA5),  PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0)                        \
	PC_DEFINE(CP(HSUSB0_DATA6),  PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0)                        \
	PC_DEFINE(CP(HSUSB0_DATA7),  PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0)                        \
	PC_DEFINE(CP(HSUSB0_DIR),    PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0)                        \
	PC_DEFINE(CP(HSUSB0_NXT),    PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0)                        \
	PC_DEFINE(CP(HSUSB0_STP),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0)                        \
                                                                                                               \
	/* The following settings are there to overcome TS problem */                                          \
                                                                                                               \
	/* UART2 lines */                                                                                      \
	PC_DEFINE(CP(MCBSP3_CLKX),   PC_OUTPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* BT_RX */            \
	PC_DEFINE(CP(MCBSP3_DR),     PC_OUTPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* BT_CTS */           \
	PC_DEFINE(CP(MCBSP3_DX),     PC_OUTPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* BT_RTS */           \
	PC_DEFINE(CP(MCBSP3_FSX),    PC_OUTPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* BT_TX */            \
                                                                                                               \
	/* UART3 lines */                                                                                      \
	PC_DEFINE(CP(UART3_CTS_RCTX),PC_OUTPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* GPS_RTS_SOC_CTS */  \
	PC_DEFINE(CP(UART3_RTS_SD),  PC_OUTPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* GPS_CTS_SOC_RTS */  \
	PC_DEFINE(CP(UART3_RX_IRRX), PC_OUTPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* GPS_TX_SOC_RX */    \
	PC_DEFINE(CP(UART3_TX_IRTX), PC_OUTPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* GPS_RX_SOC_TX */    \
                                                                                                               \
	/* GPIO lines */                                                                                       \
	PC_DEFINE(CP(GPMC_WAIT3),    PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* GPS2_CORE_PWR_EN */ \
	PC_DEFINE(CP(GPMC_NBE1),     PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* nBT_RST */          \
	PC_DEFINE(CP(CAM_XCLKA),     PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE4) /* CAM_PWR_ON */       \
	PC_DEFINE(CP(CAM_FLD),       PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE4) /* nCAM_RST */         \
	PC_DEFINE(CP(MCBSP_CLKS),    PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* nGPS2_RESET */      \
	PC_DEFINE(CP(MCBSP1_FSR),    PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* GPS2_HOST_REQ */    \
                                                                                                               \
	/* MCBSP1 lines */                                                                                     \
	PC_DEFINE(CP(MCBSP1_CLKX),   PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* BT_PCM_CLK */       \
	PC_DEFINE(CP(MCBSP1_DR),     PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* BT_PCM_OUT */       \
	PC_DEFINE(CP(MCBSP1_DX),     PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* BT_PCM_IN  */       \
	PC_DEFINE(CP(MCBSP1_FSX),    PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* BT_PCM_SYNC */      \
	

	
#define PADCONFIG_SETTINGS_UBOOT        \

	
	
#define PADCONFIG_SETTINGS_KERNEL       \
	/* UART2 lines */                                                                                      \
	PC_DEFINE(CP(MCBSP3_CLKX),   PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE1) /* BT_RX */            \
	PC_DEFINE(CP(MCBSP3_DR),     PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE1) /* BT_CTS */           \
	PC_DEFINE(CP(MCBSP3_DX),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE1) /* BT_RTS */           \
	PC_DEFINE(CP(MCBSP3_FSX),    PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE1) /* BT_TX */            \
                                                                                                               \
	/* UART3 lines */                                                                                      \
	PC_DEFINE(CP(UART3_CTS_RCTX),PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPS_RTS_SOC_CTS */  \
	PC_DEFINE(CP(UART3_RTS_SD),  PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0) /* GPS_CTS_SOC_RTS */  \
	PC_DEFINE(CP(UART3_RX_IRRX), PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0) /* GPS_TX_SOC_RX */    \
	PC_DEFINE(CP(UART3_TX_IRTX), PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPS_RX_SOC_TX */    \
                                                                                                               \
	/* GPIO lines */                                                                                       \
	PC_DEFINE(CP(GPMC_WAIT3),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* GPS2_CORE_PWR_EN */ \
	PC_DEFINE(CP(GPMC_NBE1),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* nBT_RST */          \
	PC_DEFINE(CP(CAM_XCLKA),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* CAM_PWR_ON */       \
	PC_DEFINE(CP(CAM_FLD),       PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* nCAM_RST */         \
	PC_DEFINE(CP(MCBSP1_CLKR),   PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* nGPS1_RESET */      \
	PC_DEFINE(CP(MCBSP_CLKS),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* nGPS2_RESET */      \
	PC_DEFINE(CP(MCBSP1_FSR),    PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* GPS2_HOST_REQ */    \
	PC_DEFINE(CP(GPIO126),       PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* nAUTH_RST */        \
	PC_DEFINE(CP(GPIO129),       PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* 1V8_ON */           \
                                                                                                               \
	/* MCBSP1 lines */                                                                                     \
	PC_DEFINE(CP(MCBSP1_CLKX),   PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* BT_PCM_CLK */       \
	PC_DEFINE(CP(MCBSP1_DR),     PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* BT_PCM_OUT */       \
	PC_DEFINE(CP(MCBSP1_DX),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* BT_PCM_IN  */       \
	PC_DEFINE(CP(MCBSP1_FSX),    PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* BT_PCM_SYNC */      \
                                                                                                               \
	/* USB1/2 lines */                                                                                       \
	PC_DEFINE(CP(ETK_CLK),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE3)                        \
	PC_DEFINE(CP(ETK_CTL),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE3)                        \
	PC_DEFINE(CP(ETK_D0),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE3)                        \
	PC_DEFINE(CP(ETK_D1),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE3)                        \
	PC_DEFINE(CP(ETK_D2),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE3)                        \
	PC_DEFINE(CP(ETK_D3),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE3)                        \
	PC_DEFINE(CP(ETK_D4),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE3)                        \
	PC_DEFINE(CP(ETK_D5),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE3)                        \
	PC_DEFINE(CP(ETK_D6),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE3)                        \
	PC_DEFINE(CP(ETK_D7),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE3)                        \
	PC_DEFINE(CP(ETK_D8),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE3)                        \
	PC_DEFINE(CP(ETK_D9),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE3)                        \
	PC_DEFINE(CP(ETK_D10),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE3)                        \
	PC_DEFINE(CP(ETK_D11),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE3)                        \
	PC_DEFINE(CP(ETK_D12),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE3)                        \
	PC_DEFINE(CP(ETK_D13),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE3)                        \
	PC_DEFINE(CP(ETK_D14),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE3)                        \
	PC_DEFINE(CP(ETK_D15),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE3)                        \
	PC_DEFINE(CP(MCSPI1_CS3),    PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE3)                        \
	PC_DEFINE(CP(MCSPI2_CLK),    PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE3)                        \
	PC_DEFINE(CP(MCSPI2_SIMO),   PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE3)                        \
	PC_DEFINE(CP(MCSPI2_SOMI),   PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE3)                        \
	PC_DEFINE(CP(MCSPI2_CS0),    PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE3)                        \
	PC_DEFINE(CP(MCSPI2_CS1),    PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE3)                        \
                                                                                                               \
	/* LCD lines */                                                                                        \
	PC_DEFINE(CP(DSS_ACBIAS),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_DAT_EN */       \
	PC_DEFINE(CP(DSS_DATA0),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D0 */           \
	PC_DEFINE(CP(DSS_DATA1),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D1 */           \
	PC_DEFINE(CP(DSS_DATA2),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D2 */           \
	PC_DEFINE(CP(DSS_DATA3),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D3 */           \
	PC_DEFINE(CP(DSS_DATA4),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D4 */           \
	PC_DEFINE(CP(DSS_DATA5),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D5 */           \
	PC_DEFINE(CP(DSS_DATA6),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D6 */           \
	PC_DEFINE(CP(DSS_DATA7),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D7 */           \
	PC_DEFINE(CP(DSS_DATA8),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D8 */           \
	PC_DEFINE(CP(DSS_DATA9),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D9 */           \
	PC_DEFINE(CP(DSS_DATA10),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D10 */          \
	PC_DEFINE(CP(DSS_DATA11),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D11 */          \
	PC_DEFINE(CP(DSS_DATA12),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D12 */          \
	PC_DEFINE(CP(DSS_DATA13),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D13 */          \
	PC_DEFINE(CP(DSS_DATA14),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D14 */          \
	PC_DEFINE(CP(DSS_DATA15),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D15 */          \
	PC_DEFINE(CP(DSS_DATA16),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D16 */          \
	PC_DEFINE(CP(DSS_DATA17),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D17 */          \
	PC_DEFINE(CP(DSS_DATA18),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D18 */          \
	PC_DEFINE(CP(DSS_DATA19),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D19 */          \
	PC_DEFINE(CP(DSS_DATA20),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D20 */          \
	PC_DEFINE(CP(DSS_DATA21),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D21 */          \
	PC_DEFINE(CP(DSS_DATA22),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D22 */          \
	PC_DEFINE(CP(DSS_DATA23),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D23 */          \
	PC_DEFINE(CP(DSS_HSYNC),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_HSYNC */        \
	PC_DEFINE(CP(DSS_PCLK),      PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_PCLK */         \
	PC_DEFINE(CP(DSS_VSYNC),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_VSYNC */        \
	
	
#define PADCONFIG_SETTINGS_COMMON       \
	/* CAM lines */                                                                                        \
	PC_DEFINE(CP(CAM_D0),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* CAM_D0 */           \
	PC_DEFINE(CP(CAM_D1),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* CAM_D1 */           \
	PC_DEFINE(CP(CAM_D2),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* CAM_D2 */           \
	PC_DEFINE(CP(CAM_D3),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* CAM_D3 */           \
	PC_DEFINE(CP(CAM_D4),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* CAM_D4 */           \
	PC_DEFINE(CP(CAM_D5),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* CAM_D5 */           \
	PC_DEFINE(CP(CAM_D6),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* CAM_D6 */           \
	PC_DEFINE(CP(CAM_D7),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* CAM_D7 */           \
	PC_DEFINE(CP(CAM_HS),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* CAM_HS */           \
	PC_DEFINE(CP(CAM_PCLK),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* CAM_PCLK */         \
	PC_DEFINE(CP(CAM_STROBE),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* CAM_STROBE */       \
	PC_DEFINE(CP(CAM_VS),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* CAM_VS */           \
                                                                                                               \
	/* GPMC lines */                                                                                       \
	PC_DEFINE(CP(GPMC_A1),       PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_A1 */          \
	PC_DEFINE(CP(GPMC_A2),       PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_A2 */          \
	PC_DEFINE(CP(GPMC_A3),       PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_A3 */          \
	PC_DEFINE(CP(GPMC_A4),       PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_A4 */          \
	PC_DEFINE(CP(GPMC_CLK),      PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_CLK */         \
	PC_DEFINE(CP(GPMC_D8),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_D8 */          \
	PC_DEFINE(CP(GPMC_D9),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_D9 */          \
	PC_DEFINE(CP(GPMC_D10),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_D10 */         \
	PC_DEFINE(CP(GPMC_D11),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_D11 */         \
	PC_DEFINE(CP(GPMC_D12),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_D12 */         \
	PC_DEFINE(CP(GPMC_D13),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_D13 */         \
	PC_DEFINE(CP(GPMC_D14),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_D14 */         \
	PC_DEFINE(CP(GPMC_D15),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_D15 */         \
	PC_DEFINE(CP(GPMC_NWP),      PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_nWP */         \
                                                                                                               \
	/* PCM lines */                                                                                        \
	PC_DEFINE(CP(GPMC_NCS4),     PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE2) /* PCM_VCK */          \
	PC_DEFINE(CP(GPMC_NCS5),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE2) /* PCM_VDX */          \
	PC_DEFINE(CP(GPMC_NCS6),     PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE2) /* PCM_VDR */          \
	PC_DEFINE(CP(GPMC_NCS7),     PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE2) /* PCM_VFS */          \
                                                                                                               \
	/* I2C lines */                                                                                        \
	PC_DEFINE(CP(I2C2_SCL),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0)                        \
	PC_DEFINE(CP(I2C2_SDA),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0)                        \
	PC_DEFINE(CP(I2C3_SCL),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0)                        \
	PC_DEFINE(CP(I2C3_SDA),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0)                        \
	PC_DEFINE(CP(I2C4_SCL),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE7)                        \
	PC_DEFINE(CP(I2C4_SDA),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE7)                        \
                                                                                                               \
	/* MCBSP2 lines */                                                                                     \
	PC_DEFINE(CP(MCBSP2_CLKX),   PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* MCBSP2_CLK */       \
	PC_DEFINE(CP(MCBSP2_DR),     PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* MCBSP2_DR */        \
	PC_DEFINE(CP(MCBSP2_DX),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* MCBSP2_DX */        \
	PC_DEFINE(CP(MCBSP2_FSX),    PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* MCBSP2_FSX */       \
                                                                                                               \
	/* MCSPI1 lines */                                                                                     \
	PC_DEFINE(CP(MCSPI1_CLK),    PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0)                        \
	PC_DEFINE(CP(MCSPI1_SIMO),   PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0)                        \
	PC_DEFINE(CP(MCSPI1_SOMI),   PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0)                        \
	PC_DEFINE(CP(MCSPI1_CS0),    PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0)                        \
                                                                                                               \
	/* SYS lines */                                                                                        \
	PC_DEFINE(CP(SYS_BOOT0),     PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* SYS_BOOT0 */        \
	PC_DEFINE(CP(SYS_BOOT1),     PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* SYS_BOOT1 */        \
	PC_DEFINE(CP(SYS_BOOT2),     PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* SYS_BOOT2 */        \
	PC_DEFINE(CP(SYS_BOOT6),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* SYS_BOOT6 */        \
	PC_DEFINE(CP(SYS_CLKREQ),    PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* SYS_CLKREQ */       \
	PC_DEFINE(CP(SYS_NIRQ),      PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0 | PC_WAKEUP_EN) /* SYS_NIRQ */ \
	PC_DEFINE(CP(SYS_NRESWARM),  PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0) /* SYS_NRESWARM */     \
	PC_DEFINE(CP(SYS_OFF_MODE),  PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* SYS_OFF_MODE */     \
                                                                                                               \
	/* UART1 lines */                                                                                      \
	PC_DEFINE(CP(UART1_RX),      PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0) /* DEBUG_RX */ \
	PC_DEFINE(CP(UART1_TX),      PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* DEBUG_TX */         \
                                                                                                               \
	/* Unused lines set as GPIOs */                                                                        \
	PC_DEFINE(CP(UART2_CTS),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* UART2_CTS as GPIO */\
	PC_DEFINE(CP(UART2_RTS),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* UART_RTS as GPIO */ \
	PC_DEFINE(CP(UART2_RX),      PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* UART_RX as GPIO */  \
	PC_DEFINE(CP(UART2_TX),      PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* UART_TX as GPIO */  \
                                                                                                               \
	/* GPIO lines */                                                                                       \
	PC_DEFINE(CP(CAM_D8),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* nGPS1_ANT_SHORT */  \
	PC_DEFINE(CP(CAM_D9),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* nGPS2_ANT_SHORT */  \
	PC_DEFINE(CP(CAM_D10),       PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* nUSB1_RESET */      \
	PC_DEFINE(CP(CAM_D11),       PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* nUSB2_RESET */      \
	PC_DEFINE(CP(CAM_WEN),       PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* CAN_SYNC */         \
	PC_DEFINE(CP(CAM_XCLKB),     PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* nCAM_IRQ */         \
	PC_DEFINE(CP(GPMC_A5),       PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* nLCD_RESET */       \
	PC_DEFINE(CP(GPMC_A6),       PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* LCM_PWR_ON */       \
	PC_DEFINE(CP(GPMC_A7),       PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* LCD_PWR_ON */       \
	PC_DEFINE(CP(GPMC_A8),       PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* nLCD_STBY */        \
	PC_DEFINE(CP(GPMC_A9),       PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* PWR_HOLD */         \
	PC_DEFINE(CP(GPMC_A10),      PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* nCAN_RST */         \
	PC_DEFINE(CP(GPMC_NBE0_CLE), PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* GPS_1PPS */         \
	PC_DEFINE(CP(GPMC_NCS3),     PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE4) /* nTP_IRQ */          \
	PC_DEFINE(CP(HDQ_SIO),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* CAN_RST_MON */      \
	PC_DEFINE(CP(JTAG_EMU0),     PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* nUSB3_POWER_FAULT */\
	PC_DEFINE(CP(JTAG_EMU1),     PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* nUSB1_POWER_FAULT */\
	PC_DEFINE(CP(SYS_BOOT3),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* GPIO_DIAG0 */       \
	PC_DEFINE(CP(SYS_BOOT4),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* GPIO_DIAG1 */       \
	PC_DEFINE(CP(SYS_BOOT5),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* GPIO_DIAG2 */       \
	PC_DEFINE(CP(SYS_CLKOUT1),   PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* nON_OFF */          \
	PC_DEFINE(CP(SYS_CLKOUT2),   PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* CAN_BT_MD */        \
	PC_DEFINE(CP(UART1_CTS),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* nGPS1_TCXO_ON */    \
	PC_DEFINE(CP(UART1_RTS),     PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* CPU_WAKE */         \

	
#endif // PADCONFIG_STRASBOURG_H

