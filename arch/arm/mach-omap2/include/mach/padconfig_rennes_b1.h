/*
 * Copyright (C) 2009 TomTom BV <http://www.tomtom.com/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
 
#ifndef PADCONFIG_RENNES_B1_H
#define PADCONFIG_RENNES_B1_H
 
#include "padconfig_common.h"

/*******************************/
/*********** TABLES ************/ 
/*******************************/

#define PADCONFIG_SETTINGS_XLOADER_RENNES_B1   \
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
	PC_DEFINE(CP(SDMMC1_CLK),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN   | PC_MODE0)                      \
	PC_DEFINE(CP(SDMMC1_CMD),    PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN   | PC_MODE0)                      \
	PC_DEFINE(CP(SDMMC1_DAT0),   PC_INPUT  | PC_PULL_ENA | PC_PULL_UP     | PC_MODE0)                      \
	PC_DEFINE(CP(SDMMC1_DAT1),   PC_INPUT  | PC_PULL_ENA | PC_PULL_UP     | PC_MODE0)                      \
	PC_DEFINE(CP(SDMMC1_DAT2),   PC_INPUT  | PC_PULL_ENA | PC_PULL_UP     | PC_MODE0)                      \
	PC_DEFINE(CP(SDMMC1_DAT3),   PC_INPUT  | PC_PULL_ENA | PC_PULL_UP     | PC_MODE0)                      \
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
	/* USB1 lines */                                                                                       \
	PC_DEFINE(CP(ETK_D0),        PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* MM1_RXRCV */        \
	PC_DEFINE(CP(ETK_D1),        PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* MM1_TXSE0 */        \
	PC_DEFINE(CP(ETK_D2),        PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* MM1_TXDAT */        \
	PC_DEFINE(CP(ETK_D7),        PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* MM1_TXEN */         \
	PC_DEFINE(CP(MCBSP3_DR),     PC_OUTPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* USB1_SPEED */       \
	PC_DEFINE(CP(MCBSP3_DX),     PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE4) /* USB1_SUSPEND */     \
                                                                                                               \
	/* USB2 lines - TI CSR ticket OMAPS00262728: HSUSB2_DATA2 and HSUSB2_DATA6 */                          \
	PC_DEFINE(CP(MCSPI1_CS3),    PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* HSUSB2_DATA2 */     \
	PC_DEFINE(CP(MCSPI2_CS0),    PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* HSUSB2_DATA6 */     \
                                                                                                               \
	/* The following settings are there to overcome TS problem */                                          \
                                                                                                               \
	/* UART1 lines */                                                                                      \
	PC_DEFINE(CP(UART1_CTS),     PC_OUTPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* GPS_RTS_SOC_CTS */  \
	PC_DEFINE(CP(UART1_RTS),     PC_OUTPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* GPS_CTS_SOC_RTS */  \
	PC_DEFINE(CP(UART1_RX),      PC_OUTPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* GPS_TX_SOC_RX */    \
	PC_DEFINE(CP(UART1_TX),      PC_OUTPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* GPS_RX_SOC_TX */    \
                                                                                                               \
	/* UART2 lines */                                                                                      \
	PC_DEFINE(CP(MCBSP3_CLKX),   PC_OUTPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* BT_RX */            \
	PC_DEFINE(CP(MCBSP3_FSX),    PC_OUTPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* BT_TX */            \
                                                                                                               \
	/* SDARS lines */                                                                                      \
	PC_DEFINE(CP(ETK_D8),        PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE4) /* nTUN_PWREN_UB */    \
	                                                                                                       \
	/* GPIO lines */                                                                                       \
	PC_DEFINE(CP(CAM_WEN),       PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* GPS_CORE_PWR_EN */  \
	PC_DEFINE(CP(GPMC_NBE0_CLE), PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* nBT_RST */          \
	PC_DEFINE(CP(CAM_XCLKA),     PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE4) /* CAM_PWR_ON */       \
	PC_DEFINE(CP(SDMMC2_DAT1),   PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE4) /* nCAM_RST */         \
	PC_DEFINE(CP(MCBSP_CLKS),    PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* nGPS2_RESET */      \
	PC_DEFINE(CP(UART3_RTS_SD),  PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* GPS_HOST_REQ */     \
                                                                                                               \
	/* MCBSP1 lines */                                                                                     \
	PC_DEFINE(CP(MCBSP1_CLKX),   PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* BT_PCM_CLK */       \
	PC_DEFINE(CP(MCBSP1_DR),     PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* BT_PCM_OUT */       \
	PC_DEFINE(CP(MCBSP1_DX),     PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* BT_PCM_IN  */       \
	PC_DEFINE(CP(MCBSP1_FSX),    PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* BT_PCM_SYNC */      \

#ifndef PADCONFIG_SETTINGS_XLOADER
#define PADCONFIG_SETTINGS_XLOADER   PADCONFIG_SETTINGS_XLOADER_RENNES_B1
#endif /* PADCONFIG_SETTINGS_XLOADER */

#define PADCONFIG_SETTINGS_UBOOT_RENNES_B1     \
/* LCD lines */                                                                                        \
	PC_DEFINE(CP(DSS_ACBIAS),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_DAT_EN */       \
	PC_DEFINE(CP(DSS_DATA0),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D0 */           \
	PC_DEFINE(CP(DSS_DATA1),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D1 */           \
	PC_DEFINE(CP(DSS_DATA2),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D2 */           \
	PC_DEFINE(CP(DSS_DATA3),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D3 */           \
	PC_DEFINE(CP(DSS_DATA4),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D4 */           \
	PC_DEFINE(CP(DSS_DATA5),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D5 */           \
	PC_DEFINE(CP(DSS_DATA6),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D6 */           \
	PC_DEFINE(CP(DSS_DATA7),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D7 */           \
	PC_DEFINE(CP(DSS_DATA8),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D8 */           \
	PC_DEFINE(CP(DSS_DATA9),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D9 */           \
	PC_DEFINE(CP(DSS_DATA10),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D10 */          \
	PC_DEFINE(CP(DSS_DATA11),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D11 */          \
	PC_DEFINE(CP(DSS_DATA12),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D12 */          \
	PC_DEFINE(CP(DSS_DATA13),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D13 */          \
	PC_DEFINE(CP(DSS_DATA14),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D14 */          \
	PC_DEFINE(CP(DSS_DATA15),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D15 */          \
	PC_DEFINE(CP(DSS_DATA16),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D16 */          \
	PC_DEFINE(CP(DSS_DATA17),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D17 */          \
	PC_DEFINE(CP(DSS_DATA18),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D18 */          \
	PC_DEFINE(CP(DSS_DATA19),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D19 */          \
	PC_DEFINE(CP(DSS_DATA20),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D20 */          \
	PC_DEFINE(CP(DSS_DATA21),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D21 */          \
	PC_DEFINE(CP(DSS_DATA22),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D22 */          \
	PC_DEFINE(CP(DSS_DATA23),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D23 */          \
	PC_DEFINE(CP(DSS_PCLK),      PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_PCLK */         \
	PC_DEFINE(CP(SDMMC2_DAT5),   PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* LCM_PWR_ON */       \
	PC_DEFINE(CP(MCBSP1_CLKR),   PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* nLCD_RESET */       \
                                                                                                      \

#ifndef PADCONFIG_SETTINGS_UBOOT
#define PADCONFIG_SETTINGS_UBOOT     PADCONFIG_SETTINGS_UBOOT_RENNES_B1
#endif /* PADCONFIG_SETTINGS_UBOOT */

#define PADCONFIG_SETTINGS_KERNEL_RENNES_B1    \
	/* UART1 lines */                                                                                      \
	PC_DEFINE(CP(UART1_CTS),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* GPS_RTS_SOC_CTS */  \
	PC_DEFINE(CP(UART1_RTS),     PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0 | PC_OFF_OUT_LOW) /* GPS_CTS_SOC_RTS */  \
	PC_DEFINE(CP(UART1_RX),      PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0 | PC_OFF_OUT_LOW) /* GPS_TX_SOC_RX */    \
	PC_DEFINE(CP(UART1_TX),      PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* GPS_RX_SOC_TX */    \
                                                                                                               \
	/* UART2 lines */                                                                                      \
	PC_DEFINE(CP(MCBSP3_CLKX),   PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE1 | PC_OFF_OUT_LOW) /* BT_RX */            \
	PC_DEFINE(CP(MCBSP3_FSX),    PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE1 | PC_OFF_OUT_LOW) /* BT_TX */            \
                                                                                                               \
	/* SDARS lines */                                                                                      \
	PC_DEFINE(CP(ETK_D8),        PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE4 | PC_OFF_OUT_HIGH) /* nTUN_PWREN_UB */   \
	                                                                                                       \
	/* MCBSP1 lines */                                                                                     \
	PC_DEFINE(CP(MCBSP1_CLKX),   PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* BT_PCM_CLK */       \
	PC_DEFINE(CP(MCBSP1_DR),     PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* BT_PCM_OUT */       \
	PC_DEFINE(CP(MCBSP1_DX),     PC_OUTPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* BT_PCM_IN  */       \
	PC_DEFINE(CP(MCBSP1_FSX),    PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* BT_PCM_SYNC */      \
	                                                                                                       \
	/* USB1 lines */                                                                                       \
	PC_DEFINE(CP(ETK_D0),        PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* MM1_RXRCV */        \
	PC_DEFINE(CP(ETK_D1),        PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* MM1_TXSE0 */        \
	PC_DEFINE(CP(ETK_D2),        PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* MM1_TXDAT */        \
	PC_DEFINE(CP(ETK_D7),        PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4 | PC_OFF_OUT_LOW) /* MM1_TXEN */         \
	PC_DEFINE(CP(MCBSP3_DR),     PC_OUTPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* USB1_SPEED */       \
	PC_DEFINE(CP(MCBSP3_DX),     PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE4 | PC_OFF_OUT_HIGH) /* USB1_SUSPEND */    \
	                                                                                                       \
	/* USB2 lines - TI CSR ticket OMAPS00262728: HSUSB2_DATA2 and HSUSB2_DATA6 */                          \
	PC_DEFINE(CP(MCSPI1_CS3),    PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* HSUSB2_DATA2 */     \
	PC_DEFINE(CP(MCSPI2_CS0),    PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* HSUSB2_DATA6 */     \
                                                                                                               \
	/* LCD lines */                                                                                        \
	PC_DEFINE(CP(DSS_ACBIAS),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_DAT_EN */       \
	PC_DEFINE(CP(DSS_DATA0),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D0 */           \
	PC_DEFINE(CP(DSS_DATA1),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D1 */           \
	PC_DEFINE(CP(DSS_DATA2),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D2 */           \
	PC_DEFINE(CP(DSS_DATA3),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D3 */           \
	PC_DEFINE(CP(DSS_DATA4),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D4 */           \
	PC_DEFINE(CP(DSS_DATA5),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D5 */           \
	PC_DEFINE(CP(DSS_DATA6),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D6 */           \
	PC_DEFINE(CP(DSS_DATA7),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D7 */           \
	PC_DEFINE(CP(DSS_DATA8),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D8 */           \
	PC_DEFINE(CP(DSS_DATA9),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D9 */           \
	PC_DEFINE(CP(DSS_DATA10),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D10 */          \
	PC_DEFINE(CP(DSS_DATA11),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D11 */          \
	PC_DEFINE(CP(DSS_DATA12),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D12 */          \
	PC_DEFINE(CP(DSS_DATA13),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D13 */          \
	PC_DEFINE(CP(DSS_DATA14),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D14 */          \
	PC_DEFINE(CP(DSS_DATA15),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D15 */          \
	PC_DEFINE(CP(DSS_DATA16),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D16 */          \
	PC_DEFINE(CP(DSS_DATA17),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D17 */          \
	PC_DEFINE(CP(DSS_DATA18),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D18 */          \
	PC_DEFINE(CP(DSS_DATA19),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D19 */          \
	PC_DEFINE(CP(DSS_DATA20),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D20 */          \
	PC_DEFINE(CP(DSS_DATA21),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D21 */          \
	PC_DEFINE(CP(DSS_DATA22),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D22 */          \
	PC_DEFINE(CP(DSS_DATA23),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_D23 */          \
	PC_DEFINE(CP(DSS_PCLK),      PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* LCD_PCLK */         \
                                                                                                           \
	/* JTAG lines */										                                               \
	PC_DEFINE(CP(JTAG_NTRST),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* uP_nTRST */         \
	PC_DEFINE(CP(JTAG_TCK),	     PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* uP_TCK   */         \
	PC_DEFINE(CP(JTAG_TMS_TMSC), PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* uP_nTMS */          \
	PC_DEFINE(CP(JTAG_RTCK),     PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE0) /* uP_nRTCK */         \
	PC_DEFINE(CP(JTAG_NTRST),    PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* uP_nTRST */         \
	PC_DEFINE(CP(JTAG_TDI),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* uP_TDI */           \
	PC_DEFINE(CP(JTAG_TDO),      PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* uP_TDO */           \
                                                                                                           \
	/* GPIO lines */                                                                                       \
	PC_DEFINE(CP(CAM_WEN),       PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4 | PC_OFF_OUT_LOW) /* GPS_CORE_PWR_EN */  \
	PC_DEFINE(CP(CAM_XCLKA),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4 | PC_OFF_OUT_LOW) /* CAM_PWR_ON */       \
	PC_DEFINE(CP(SDMMC2_DAT1),   PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4 | PC_OFF_OUT_LOW) /* nCAM_RST */         \
	PC_DEFINE(CP(GPMC_NBE0_CLE), PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4 | PC_OFF_OUT_LOW) /* nBT_RST */          \
	PC_DEFINE(CP(MCBSP_CLKS),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4 | PC_OFF_OUT_LOW) /* nGPS_RESET */       \
	PC_DEFINE(CP(UART3_RTS_SD),  PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4 | PC_OFF_OUT_LOW) /* GPS_HOST_REQ */     \
	PC_DEFINE(CP(GPIO126),       PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4 | PC_OFF_OUT_LOW) /* nAUTH_RST */        \
	PC_DEFINE(CP(GPIO129),       PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4 | PC_OFF_OUT_LOW) /* CAM_ON */           \
	PC_DEFINE(CP(GPMC_NBE1),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4 | OMAP_PIN_OFF_OUTPUT_HIGH)/* PWR_HOLD */         \

#ifndef PADCONFIG_SETTINGS_KERNEL
#define PADCONFIG_SETTINGS_KERNEL    PADCONFIG_SETTINGS_KERNEL_RENNES_B1
#endif /* PADCONFIG_SETTINGS_KERNEL */

#define PADCONFIG_SETTINGS_COMMON_RENNES_B1    \
	/* CAM lines */                                                                                        \
	PC_DEFINE(CP(CAM_D0),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* CAM_D0 */           \
	PC_DEFINE(CP(CAM_D1),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* CAM_D1 */           \
	PC_DEFINE(CP(CAM_D2),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* CAM_D2 */           \
	PC_DEFINE(CP(CAM_D3),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* CAM_D3 */           \
	PC_DEFINE(CP(CAM_D4),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* CAM_D4 */           \
	PC_DEFINE(CP(CAM_D5),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* CAM_D5 */           \
	PC_DEFINE(CP(CAM_D6),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* CAM_D6 */           \
	PC_DEFINE(CP(CAM_D7),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* CAM_D7 */           \
	PC_DEFINE(CP(CAM_FLD),       PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE0) /* CAM_FLD */          \
	PC_DEFINE(CP(CAM_HS),        PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE0) /* CAM_HS */           \
	PC_DEFINE(CP(CAM_PCLK),      PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE0) /* CAM_PCLK */         \
	PC_DEFINE(CP(CAM_VS),        PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE0) /* CAM_VS */           \
	                                                                                                       \
	/* GPMC lines */                                                                                       \
	PC_DEFINE(CP(GPMC_A1),       PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_A1 */          \
	PC_DEFINE(CP(GPMC_A2),       PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_A2 */          \
	PC_DEFINE(CP(GPMC_A3),       PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_A3 */          \
	PC_DEFINE(CP(GPMC_A4),       PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_A4 */          \
	PC_DEFINE(CP(GPMC_A5),       PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_A5 */          \
	PC_DEFINE(CP(GPMC_A6),       PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_A6 */          \
	PC_DEFINE(CP(GPMC_CLK),      PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_CLK */         \
	PC_DEFINE(CP(GPMC_D0),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_D0 */          \
	PC_DEFINE(CP(GPMC_D1),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_D1 */          \
	PC_DEFINE(CP(GPMC_D2),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_D2 */          \
	PC_DEFINE(CP(GPMC_D3),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_D3 */          \
	PC_DEFINE(CP(GPMC_D4),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_D4 */          \
	PC_DEFINE(CP(GPMC_D5),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_D5 */          \
	PC_DEFINE(CP(GPMC_D6),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_D6 */          \
	PC_DEFINE(CP(GPMC_D7),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_D7 */          \
	PC_DEFINE(CP(GPMC_D8),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_D8 */          \
	PC_DEFINE(CP(GPMC_D9),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_D9 */          \
	PC_DEFINE(CP(GPMC_D10),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_D10 */         \
	PC_DEFINE(CP(GPMC_D11),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_D11 */         \
	PC_DEFINE(CP(GPMC_D12),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_D12 */         \
	PC_DEFINE(CP(GPMC_D13),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_D13 */         \
	PC_DEFINE(CP(GPMC_D14),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_D14 */         \
	PC_DEFINE(CP(GPMC_D15),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_D15 */         \
	PC_DEFINE(CP(GPMC_NWP),      PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_nWP */         \
	PC_DEFINE(CP(GPMC_NCS0),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_nCS0 */        \
	PC_DEFINE(CP(GPMC_NCS1),     PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* GPMC_nCS1 */        \
	PC_DEFINE(CP(GPMC_NCS2),     PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* GPMC_nCS2 */        \
	PC_DEFINE(CP(GPMC_NOE),      PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_nOE */         \
	PC_DEFINE(CP(GPMC_NADV_ALE), PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_nADV_ALE */    \
	PC_DEFINE(CP(GPMC_NWE),      PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_nWE */         \
                                                                                                               \
	/* MCBSP4 lines */                                                                                     \
	PC_DEFINE(CP(GPMC_NCS4),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE2 | PC_OFF_OUT_LOW) /* MCBSP4_CLK */       \
	PC_DEFINE(CP(GPMC_NCS6),     PC_OUTPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE2 | PC_OFF_OUT_LOW) /* MCBSP4_DX */        \
	PC_DEFINE(CP(GPMC_NCS7),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE2 | PC_OFF_OUT_LOW) /* MCBSP4_FSX */       \
                                                                                                               \
	/* I2C lines */                                                                                        \
	PC_DEFINE(CP(I2C2_SCL),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0)                        \
	PC_DEFINE(CP(I2C2_SDA),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0)                        \
	PC_DEFINE(CP(I2C3_SCL),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0)                        \
	PC_DEFINE(CP(I2C3_SDA),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0)                        \
	PC_DEFINE(CP(I2C4_SCL),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE7)                        \
	PC_DEFINE(CP(I2C4_SDA),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE7)                        \
	PC_DEFINE(CP(I2C1_SCL),      PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0)                        \
	PC_DEFINE(CP(I2C1_SDA),      PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0)                        \
                                                                                                               \
	/* MCBSP2 lines */                                                                                     \
	PC_DEFINE(CP(MCBSP2_CLKX),   PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE0 | PC_OFF_IN_NOPULL) /* MCBSP2_CLK */       \
	PC_DEFINE(CP(MCBSP2_DR),     PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE0 | PC_OFF_IN_NOPULL) /* MCBSP2_DR */        \
	PC_DEFINE(CP(MCBSP2_DX),     PC_OUTPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW)   /* MCBSP2_DX */        \
	PC_DEFINE(CP(MCBSP2_FSX),    PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE0 | PC_OFF_IN_NOPULL) /* MCBSP2_FSX */       \
                                                                                                               \
	/* MCSPI1 lines */                                                                                     \
	PC_DEFINE(CP(MCSPI1_CLK),    PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE0 | PC_OFF_IN_PULLDOWN) /* SPI_CAN_CLK */  \
	PC_DEFINE(CP(MCSPI1_SIMO),   PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE0 | PC_OFF_IN_PULLDOWN) /* SPI_CAN_SIMO */ \
	PC_DEFINE(CP(MCSPI1_SOMI),   PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* SPI_CAN_SOMI */     \
	PC_DEFINE(CP(MCSPI1_CS0),    PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE0 | PC_OFF_IN_PULLDOWN) /* SPI_CAN_CS0 */  \
                                                                                                               \
	/* SYS lines */                                                                                        \
	PC_DEFINE(CP(SYS_BOOT0),     PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* SYS_BOOT0 */        \
	PC_DEFINE(CP(SYS_BOOT1),     PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* SYS_BOOT1 */        \
	PC_DEFINE(CP(SYS_BOOT2),     PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* SYS_BOOT2 */        \
	PC_DEFINE(CP(SYS_BOOT3),     PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* SYS_BOOT3 */        \
	PC_DEFINE(CP(SYS_BOOT4),     PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* SYS_BOOT4 */        \
	PC_DEFINE(CP(SYS_BOOT5),     PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* SYS_BOOT5 */        \
	PC_DEFINE(CP(SYS_BOOT6),     PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* SYS_BOOT6 */        \
	PC_DEFINE(CP(SYS_CLKREQ),    PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* SYS_CLKREQ */       \
	PC_DEFINE(CP(SYS_NIRQ),      PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0 | PC_WAKEUP_EN) /* SYS_NIRQ */ \
	PC_DEFINE(CP(SYS_NRESWARM),  PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0) /* SYS_NRESWARM */     \
	PC_DEFINE(CP(SYS_OFF_MODE),  PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* SYS_OFF_MODE */     \
                                                                                                               \
	/* UART3 lines */                                                                                      \
	PC_DEFINE(CP(UART3_RX_IRRX), PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0 | PC_OFF_IN_PULLDOWN) /* DEBUG_RX */     \
	PC_DEFINE(CP(UART3_TX_IRTX), PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | PC_OFF_OUT_LOW) /* DEBUG_TX */         \
	                                                                                                       \
	/* BACKLIGHT lines */                                                                                  \
	PC_DEFINE(CP(GPMC_NCS5),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE3 | PC_OFF_IN_NOPULL) /* BACKLIGHT_PWM */  \
	PC_DEFINE(CP(GPMC_WAIT3),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4 | PC_OFF_OUT_LOW)   /* BACKLIGHT_ON  */  \
	                                                                                                       \
	/* nACCEL_IRQ and nGYRO_IRQ lines */                                                                                  \
	PC_DEFINE(CP(ETK_D5),        PC_INPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4 | PC_OFF_OUT_LOW) /* nGYRO_IRQ */      \
	PC_DEFINE(CP(CAM_D10),       PC_INPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4 | PC_OFF_OUT_LOW) /* nACCEL_IRQ */     \
	                                                                                                       \
	/* Unused lines set as GPIOs */                                                                        \
	PC_DEFINE(CP(UART2_CTS),     PC_INPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* UART_CTS as GPIO */   \
	PC_DEFINE(CP(UART2_RTS),     PC_INPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* UART_RTS as GPIO */   \
	PC_DEFINE(CP(UART2_RX),      PC_INPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* UART_RX as GPIO */    \
	PC_DEFINE(CP(UART2_TX),      PC_INPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* UART_TX as GPIO */    \
	PC_DEFINE(CP(GPMC_A7),       PC_INPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* GPMC_A7 as GPIO */    \
	PC_DEFINE(CP(GPMC_A10),      PC_INPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* GPMC_A10 as GPIO */   \
	PC_DEFINE(CP(SDMMC2_CLK),    PC_INPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4 | PC_OFF_OUT_LOW) /* MCSPI3_CLK */      \
	PC_DEFINE(CP(SDMMC2_CMD),    PC_INPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4 | PC_OFF_OUT_LOW) /* MCSPI3_SIMO */     \
	PC_DEFINE(CP(SDMMC2_DAT0),   PC_INPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4 | PC_OFF_OUT_LOW) /* MCSPI3_SOMI */     \
	PC_DEFINE(CP(SDMMC2_DAT2),   PC_INPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* SDMMC2_DAT2 as GPIO */\
	PC_DEFINE(CP(SDMMC2_DAT3),   PC_INPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4 | PC_OFF_OUT_LOW) /* MCSPI3_CS0 */      \
	PC_DEFINE(CP(SDMMC2_DAT6),   PC_INPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* SDMMC2_DAT6 as GPIO */\
	PC_DEFINE(CP(CAM_STROBE),    PC_INPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* CAM_STROBE as GPIO */\
	PC_DEFINE(CP(DSS_HSYNC),     PC_INPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* LCD_HSYNC as GPIO */  \
	PC_DEFINE(CP(DSS_VSYNC),     PC_INPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* LCD_VSYNC as GPIO */  \
	PC_DEFINE(CP(ETK_CTL),       PC_INPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* ETK_CTL as GPIO */    \
	PC_DEFINE(CP(ETK_D4),        PC_INPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* ETK_D4 as GPIO */     \
	PC_DEFINE(CP(ETK_D6),        PC_INPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* ETK_D6 as GPIO */     \
	PC_DEFINE(CP(ETK_D9),        PC_INPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* ETK_D9 as GPIO */     \
	PC_DEFINE(CP(MCBSP4_CLKX),   PC_INPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* MCBSP4_CLKX as GPIO */\
	PC_DEFINE(CP(MCBSP4_DR),     PC_INPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* MCBSP4_DR as GPIO */  \
	PC_DEFINE(CP(MCBSP4_DX),     PC_INPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4 | PC_OFF_OUT_LOW) /* MCBSP4_DX as GPIO */  \
	PC_DEFINE(CP(MCBSP4_FSX),    PC_INPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4 | PC_OFF_OUT_LOW) /* MCBSP4_FSX as GPIO */ \
	PC_DEFINE(CP(MCSPI1_CS1),    PC_INPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* MCSPI1_CS1 as GPIO */ \
	PC_DEFINE(CP(MCSPI1_CS2),    PC_INPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* MCSPI1_CS2 as GPIO */ \
	PC_DEFINE(CP(GPMC_NCS1),     PC_INPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* GPMC_nCS1 as GPIO */  \
	PC_DEFINE(CP(GPMC_NCS2),     PC_INPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* GPMC_nCS2 as GPIO */  \
	PC_DEFINE(CP(GPMC_WAIT1),    PC_INPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* GPMC_WAIT1 as GPIO */ \
	PC_DEFINE(CP(GPMC_WAIT2),    PC_INPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* GPMC_WAIT2 as GPIO */ \
	PC_DEFINE(CP(CSI2_DX0),      PC_INPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* CSI2_DX0 as GPIO */   \
	PC_DEFINE(CP(CSI2_DY0),      PC_INPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* CSI2_DY0 as GPIO */   \
	PC_DEFINE(CP(CSI2_DX1),      PC_INPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* CSI2_DX1 as GPIO */   \
	PC_DEFINE(CP(CSI2_DY1),      PC_INPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* CSI2_DY1 as GPIO */   \
                                                                                                            \
	/* GPIO lines */                                                                                        \
	PC_DEFINE(CP(CAM_D8),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4 | PC_OFF_IN_PULLDOWN) /* nCAM_IRQ */    \
	PC_DEFINE(CP(CAM_D9),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* nGPS_ANT_SHORT */   \
	PC_DEFINE(CP(CAM_D11),       PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4 | PC_OFF_OUT_LOW) /* nUSB2_RESET */      \
	PC_DEFINE(CP(CAM_XCLKB),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4 | PC_OFF_OUT_LOW) /* MIC_ON */           \
	PC_DEFINE(CP(SDMMC2_DAT4),   PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4 | PC_OFF_OUT_LOW) /* nCAN_RST */         \
	PC_DEFINE(CP(SDMMC2_DAT5),   PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4 | PC_OFF_OUT_LOW) /* LCM_PWR_ON */       \
	PC_DEFINE(CP(SDMMC2_DAT7),   PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4 | PC_OFF_OUT_LOW) /* nCAN_IRQ */         \
	PC_DEFINE(CP(GPMC_A8),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4 | PC_OFF_IN_NOPULL) /* nDIAGSYS_BOOT */  \
	PC_DEFINE(CP(GPMC_A9),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4 | PC_OFF_OUT_LOW) /* GPS_1PPS */         \
	PC_DEFINE(CP(GPMC_NCS3),     PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE4 | PC_OFF_OUT_LOW) /* nTP_IRQ */          \
	PC_DEFINE(CP(HDQ_SIO),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4 | PC_OFF_IN_PULLDOWN) /* CAN_RST_MON */  \
	PC_DEFINE(CP(JTAG_EMU0),     PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE4 | PC_OFF_IN_NOPULL) /* nUSB3_POWER_FAULT */\
	PC_DEFINE(CP(JTAG_EMU1),     PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE4 | PC_OFF_IN_NOPULL) /* nUSB1_POWER_FAULT */\
	PC_DEFINE(CP(MCBSP1_CLKR),   PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4 | PC_OFF_OUT_LOW) /* nLCD_RESET */       \
	PC_DEFINE(CP(MCBSP1_FSR),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4 | PC_OFF_OUT_LOW) /* nTP_RESET */        \
	PC_DEFINE(CP(SYS_CLKOUT1),   PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* nON_OFF */          \
	PC_DEFINE(CP(SYS_CLKOUT2),   PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4 | PC_OFF_OUT_LOW) /* CAN_BT_MD */        \
	PC_DEFINE(CP(UART3_CTS_RCTX),PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4 | PC_OFF_OUT_LOW) /* CAN_SYNC */         \
	PC_DEFINE(CP(ETK_CLK),       PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* LCD_UD */           \
	PC_DEFINE(CP(ETK_D3),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4 | PC_OFF_OUT_LOW) /* SENSE_11V0 */       \

#ifndef PADCONFIG_SETTINGS_COMMON
#define PADCONFIG_SETTINGS_COMMON    PADCONFIG_SETTINGS_COMMON_RENNES_B1
#endif /* PADCONFIG_SETTINGS_COMMON */
	
#endif // PADCONFIG_RENNES_B1_H

