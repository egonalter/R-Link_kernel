/*
 * Copyright (C) 2009 TomTom BV <http://www.tomtom.com/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef PADCONFIG_SANTIAGO_H
#define PADCONFIG_SANTIAGO_H

#include "padconfig_common.h"

/*******************************/
/*********** TABLES ************/
/*******************************/


#define PADCONFIG_SETTINGS_XLOADER      \
	/*SDRC*/                                                                                                \
	PC_DEFINE(CP(SDRC_D0),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D0*/            \
	PC_DEFINE(CP(SDRC_D1),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D1*/            \
	PC_DEFINE(CP(SDRC_D2),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D2*/            \
	PC_DEFINE(CP(SDRC_D3),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D3*/            \
	PC_DEFINE(CP(SDRC_D4),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D4*/            \
	PC_DEFINE(CP(SDRC_D5),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D5*/            \
	PC_DEFINE(CP(SDRC_D6),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D6*/            \
	PC_DEFINE(CP(SDRC_D7),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D7*/            \
	PC_DEFINE(CP(SDRC_D8),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D8*/            \
	PC_DEFINE(CP(SDRC_D9),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D9*/            \
	PC_DEFINE(CP(SDRC_D10),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D10*/           \
	PC_DEFINE(CP(SDRC_D11),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D11*/           \
	PC_DEFINE(CP(SDRC_D12),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D12*/           \
	PC_DEFINE(CP(SDRC_D13),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D13*/           \
	PC_DEFINE(CP(SDRC_D14),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D14*/           \
	PC_DEFINE(CP(SDRC_D15),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D15*/           \
	PC_DEFINE(CP(SDRC_D16),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D16*/           \
	PC_DEFINE(CP(SDRC_D17),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D17*/           \
	PC_DEFINE(CP(SDRC_D18),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D18*/           \
	PC_DEFINE(CP(SDRC_D19),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D19*/           \
	PC_DEFINE(CP(SDRC_D20),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D20*/           \
	PC_DEFINE(CP(SDRC_D21),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D21*/           \
	PC_DEFINE(CP(SDRC_D22),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D22*/           \
	PC_DEFINE(CP(SDRC_D23),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D23*/           \
	PC_DEFINE(CP(SDRC_D24),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D24*/           \
	PC_DEFINE(CP(SDRC_D25),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D25*/           \
	PC_DEFINE(CP(SDRC_D26),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D26*/           \
	PC_DEFINE(CP(SDRC_D27),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D27*/           \
	PC_DEFINE(CP(SDRC_D28),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D28*/           \
	PC_DEFINE(CP(SDRC_D29),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D29*/           \
	PC_DEFINE(CP(SDRC_D30),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D30*/           \
	PC_DEFINE(CP(SDRC_D31),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_D31*/           \
	PC_DEFINE(CP(SDRC_CLK),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_CLK*/           \
	PC_DEFINE(CP(SDRC_DQS0),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_DQS0*/          \
	PC_DEFINE(CP(SDRC_DQS1),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_DQS1*/          \
	PC_DEFINE(CP(SDRC_DQS2),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_DQS2*/          \
	PC_DEFINE(CP(SDRC_DQS3),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /*SDRC_DQS3*/          \
                                                                                                                \
	/* MMC lines */                                                                                         \
	PC_DEFINE(CP(SDMMC1_CLK),     PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0)                        \
	PC_DEFINE(CP(SDMMC1_CMD),     PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0)                        \
	PC_DEFINE(CP(SDMMC1_DAT0),    PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0)                        \
	PC_DEFINE(CP(SDMMC1_DAT1),    PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0)                        \
	PC_DEFINE(CP(SDMMC1_DAT2),    PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0)                        \
	PC_DEFINE(CP(SDMMC1_DAT3),    PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0)                        \
	PC_DEFINE(CP(SDMMC2_CLK),     PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0)                        \
	PC_DEFINE(CP(SDMMC2_CMD),     PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0)                        \
	PC_DEFINE(CP(SDMMC2_DAT0),    PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0)                        \
	PC_DEFINE(CP(SDMMC2_DAT1),    PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0)                        \
	PC_DEFINE(CP(SDMMC2_DAT2),    PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0)                        \
	PC_DEFINE(CP(SDMMC2_DAT3),    PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0)                        \
	PC_DEFINE(CP(SDMMC2_DAT4),    PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0)                        \
	PC_DEFINE(CP(SDMMC2_DAT5),    PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0)                        \
	PC_DEFINE(CP(SDMMC2_DAT6),    PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0)                        \
	PC_DEFINE(CP(SDMMC2_DAT7),    PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0)                        \
	                                                                                                        \
	/* UART4 lines */                                                                                       \
	PC_DEFINE(CP(GPMC_WAIT2),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE2) /* DEBUG_TX */         \
	PC_DEFINE(CP(GPMC_WAIT3),     PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE2) /* DEBUG_RX */         \
	                                                                                                        \
	/* GPIO lines */                                                                                        \
	PC_DEFINE(CP(ETK_D13),        PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /*  27: AUX_MOVI */    \
	PC_DEFINE(CP(ETK_D14),        PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /*  28: AUX_SD */      \

#define PADCONFIG_SETTINGS_UBOOT        \



#define PADCONFIG_SETTINGS_KERNEL       \
	/* GPIO lines */                                                                                                                  \
	PC_DEFINE(CP(GPIO126),        PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* 126: GSM_SYS_RST */                           \
	PC_DEFINE(CP(GPIO127),        PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* 127: GSM_SYS_EN */                            \
	                                                                                                                                  \
	/* UART1 lines */                                                                                                                 \
	PC_DEFINE(CP(UART1_TX),       PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | OMAP_PIN_OFF_OUTPUT_LOW) /* uP_UART1_TX */      \
	PC_DEFINE(CP(UART1_RTS),      PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0 | OMAP_PIN_OFF_OUTPUT_LOW) /* uP_UART1_RTS */     \
	PC_DEFINE(CP(UART1_CTS),      PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | OMAP_PIN_OFF_OUTPUT_LOW) /* uP_UART1_CTS */     \
	PC_DEFINE(CP(UART1_RX),       PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0 | OMAP_PIN_OFF_OUTPUT_LOW) /* uP_UART1_RX */      \
	                                                                                                                                  \
	/* UART2 lines */                                                                                                                 \
	PC_DEFINE(CP(UART2_CTS),      PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | OMAP_PIN_OFF_OUTPUT_HIGH) /* uP_UART2_CTS */    \
	PC_DEFINE(CP(UART2_RTS),      PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0 | OMAP_PIN_OFF_OUTPUT_HIGH) /* uP_UART2_RTS */   \
	PC_DEFINE(CP(UART2_TX),       PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | OMAP_PIN_OFF_OUTPUT_HIGH) /* uP_UART2_TX */     \
	PC_DEFINE(CP(UART2_RX),       PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0 | OMAP_PIN_OFF_OUTPUT_HIGH) /* uP_UART2_RX */    \
                                                                                                                                          \
	/* UART3 lines */                                                                                                                 \
	PC_DEFINE(CP(UART3_CTS_RCTX), PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | OMAP_PIN_OFF_OUTPUT_LOW) /* uP_UART3_CTS */     \
	PC_DEFINE(CP(UART3_RTS_SD),   PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0 | OMAP_PIN_OFF_OUTPUT_LOW) /* uP_UART3_RTS */     \
	PC_DEFINE(CP(UART3_RX_IRRX),  PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0 | OMAP_PIN_OFF_OUTPUT_LOW) /* uP_UART3_RX */      \
	PC_DEFINE(CP(UART3_TX_IRTX),  PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | OMAP_PIN_OFF_OUTPUT_LOW) /* uP_UART3_TX */      \
                                                                                                                                          \
	/* UART4 lines */                                                                                                                 \
	PC_DEFINE(CP(GPMC_WAIT2),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE2 | OMAP_PIN_OFF_OUTPUT_LOW) /* DEBUG_TX */         \
	PC_DEFINE(CP(GPMC_WAIT3),     PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE2 | OMAP_PIN_OFF_OUTPUT_LOW) /* DEBUG_RX */         \
                                                                                                                                          \
	/* PCM BT lines */                                                                                                                \
	PC_DEFINE(CP(MCBSP1_CLKX),    PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | OMAP_PIN_OFF_OUTPUT_LOW) /* BT_PCM_CLK */       \
	PC_DEFINE(CP(MCBSP1_DR),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | OMAP_PIN_OFF_OUTPUT_LOW) /* BT_PCM_OUT */       \
	PC_DEFINE(CP(MCBSP1_DX),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | OMAP_PIN_OFF_OUTPUT_LOW) /* BT_PCM_IN */        \
	PC_DEFINE(CP(MCBSP1_FSX),     PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | OMAP_PIN_OFF_OUTPUT_LOW) /* BT_PCM_SYNC */      \
                                                                                                                                          \
	/* PCM PMIC lines */                                                                                                              \
	PC_DEFINE(CP(MCBSP3_CLKX),    PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | OMAP_PIN_OFF_OUTPUT_LOW) /* PCM_VDX */          \
	PC_DEFINE(CP(MCBSP3_DR),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | OMAP_PIN_OFF_OUTPUT_LOW) /* PCM_VFS */          \
	PC_DEFINE(CP(MCBSP3_DX),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | OMAP_PIN_OFF_OUTPUT_LOW) /* PCM_VCK */          \
	PC_DEFINE(CP(MCBSP3_FSX),     PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | OMAP_PIN_OFF_OUTPUT_LOW) /* PCM_VDR */          \
                                                                                                                                          \
	/* PMIC I2S lines */                                                                                                              \
	PC_DEFINE(CP(MCBSP2_CLKX),    PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | OMAP_PIN_OFF_OUTPUT_LOW) /* MCBSP2_CLK */       \
	PC_DEFINE(CP(MCBSP2_DR),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | OMAP_PIN_OFF_OUTPUT_LOW) /* MCBSP2_DR */        \
	PC_DEFINE(CP(MCBSP2_DX),      PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | OMAP_PIN_OFF_OUTPUT_LOW) /* MCBSP2_DX */        \
	PC_DEFINE(CP(MCBSP2_FSX),     PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | OMAP_PIN_OFF_OUTPUT_LOW) /* MCBSP2_FSX */       \
                                                                                                                                          \
	/* MCSPI1 lines */                                                                                                                \
	PC_DEFINE(CP(MCSPI1_CLK),     PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | OMAP_PIN_OFF_OUTPUT_LOW) /* DR_SCLK */          \
	PC_DEFINE(CP(MCSPI1_SIMO),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | OMAP_PIN_OFF_OUTPUT_LOW) /* DR_SDI */           \
	PC_DEFINE(CP(MCSPI1_SOMI),    PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | OMAP_PIN_OFF_OUTPUT_LOW) /* DR_SDO */           \
	PC_DEFINE(CP(MCSPI1_CS0),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | OMAP_PIN_OFF_OUTPUT_LOW) /* ACC_CS0 */          \
	PC_DEFINE(CP(MCSPI1_CS1),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | OMAP_PIN_OFF_OUTPUT_LOW) /* TP196 */            \
	PC_DEFINE(CP(MCSPI1_CS2),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | OMAP_PIN_OFF_OUTPUT_LOW) /* BARO_Csn */         \
	PC_DEFINE(CP(MCSPI1_CS3),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE7 | OMAP_PIN_OFF_OUTPUT_LOW) /* TP181 */            \
                                                                                                                                          \
	/* MCSPI2 (LCM) lines */                                                                                                          \
	PC_DEFINE(CP(MCSPI2_CLK),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | OMAP_PIN_OFF_OUTPUT_LOW) /* SPI_LCM_CLK */      \
	PC_DEFINE(CP(MCSPI2_SIMO),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | OMAP_PIN_OFF_OUTPUT_LOW) /* SPI_LCM_OUT */      \
	PC_DEFINE(CP(MCSPI2_SOMI),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE7 | OMAP_PIN_OFF_OUTPUT_LOW) /* TP182 */            \
	PC_DEFINE(CP(MCSPI2_CS0),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0 | OMAP_PIN_OFF_OUTPUT_LOW) /* SPI_LCM_CS0 */      \
	PC_DEFINE(CP(MCSPI2_CS1),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE7 | OMAP_PIN_OFF_OUTPUT_LOW) /* TP186 */            \
                                                                                                                                          \
	/* WiFi lines */                                                                                                                  \
	PC_DEFINE(CP(ETK_CLK),        PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE2 | OMAP_PIN_OFF_OUTPUT_LOW) /* WIFI_CLK */         \
	PC_DEFINE(CP(ETK_CTL),        PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE2 | OMAP_PIN_OFF_OUTPUT_LOW) /* WIFI_CMD */         \
	PC_DEFINE(CP(ETK_D3),         PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE2 | OMAP_PIN_OFF_OUTPUT_LOW) /* WIFI_DATA3 */       \
	PC_DEFINE(CP(ETK_D4),         PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE2 | OMAP_PIN_OFF_OUTPUT_LOW) /* WIFI_DATA0 */       \
	PC_DEFINE(CP(ETK_D5),         PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE2 | OMAP_PIN_OFF_OUTPUT_LOW) /* WIFI_DATA1 */       \
	PC_DEFINE(CP(ETK_D6),         PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE2 | OMAP_PIN_OFF_OUTPUT_LOW) /* WIFI_DATA2 */       \
                                                                                                                                          \
	/* GPIO lines */                                                                                                                  \
	PC_DEFINE(CP(MCBSP1_CLKR),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4 | OMAP_PIN_OFF_OUTPUT_LOW) /* 156: nLCD_RESET */  \
	PC_DEFINE(CP(GPMC_NCS3),      PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4 | OMAP_PIN_OFF_OUTPUT_LOW) /*  54: nTP_IRQ */     \
	PC_DEFINE(CP(GPMC_A6),        PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE4 | OMAP_PIN_OFF_OUTPUT_LOW) /*  39: nCD_SD_micro */\
	PC_DEFINE(CP(GPMC_D15),       PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE4 | OMAP_PIN_OFF_OUTPUT_LOW) /*  51: HPDETECTn */   \
	PC_DEFINE(CP(GPMC_NCS1),      PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE4 | OMAP_PIN_OFF_OUTPUT_LOW) /*  52: USB_DETECTn */ \
	PC_DEFINE(CP(GPMC_A4),        PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4 | OMAP_PIN_OFF_OUTPUT_LOW) /*  37: AMP_PWR_EN */  \
	PC_DEFINE(CP(ETK_D13),        PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4 | OMAP_PIN_OFF_OUTPUT_LOW) /*  27: AUX_MOVI */    \
	PC_DEFINE(CP(ETK_D14),        PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4 | OMAP_PIN_OFF_OUTPUT_LOW) /*  28: AUX_SD */      \
	PC_DEFINE(CP(MCBSP1_FSR),     PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE4 | OMAP_PIN_OFF_OUTPUT_LOW) /* 157: TSP_RST */     \
	PC_DEFINE(CP(ETK_D10),        PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4 | OMAP_PIN_OFF_OUTPUT_LOW) /*  24: DR_POWER_EN */ \
	PC_DEFINE(CP(GPMC_NBE0_CLE),  PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4 | OMAP_PIN_OFF_OUTPUT_LOW) /*  60: nBT_RST */     \
	PC_DEFINE(CP(ETK_D15),        PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4 | OMAP_PIN_OFF_OUTPUT_LOW) /*  29: WIFI_RSTn */   \
	PC_DEFINE(CP(ETK_D1),         PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4 | OMAP_PIN_OFF_OUTPUT_LOW) /*  15: WIFI_EN */     \


#define PADCONFIG_SETTINGS_COMMON	\
	/* CAM lines */ \
	PC_DEFINE(CP(CAM_D0),         PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* CAM_D0 */           \
	PC_DEFINE(CP(CAM_D1),         PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* CAM_D1 */           \
	PC_DEFINE(CP(CAM_D2),         PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* CAM_D2 */           \
	PC_DEFINE(CP(CAM_D3),         PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* CAM_D3 */           \
	PC_DEFINE(CP(CAM_D4),         PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* CAM_D4 */           \
	PC_DEFINE(CP(CAM_D5),         PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* CAM_D5 */           \
	PC_DEFINE(CP(CAM_D6),         PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* CAM_D6 */           \
	PC_DEFINE(CP(CAM_D7),         PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* CAM_D7 */           \
	PC_DEFINE(CP(CAM_D8),         PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* CAM_D8 */           \
	PC_DEFINE(CP(CAM_D9),         PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* CAM_D9 */           \
	PC_DEFINE(CP(CAM_XCLKB),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* h_CAM_XCLKB */      \
	PC_DEFINE(CP(CAM_PCLK),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* CAM_PCLK */         \
	PC_DEFINE(CP(CAM_VS),         PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* CAM_VS */           \
	PC_DEFINE(CP(CAM_HS),         PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* CAM_HS */           \
	PC_DEFINE(CP(CAM_FLD),        PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE2) /* CAM_nRESET */       \
	PC_DEFINE(CP(CSI2_DX0),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* CSI1_DX0 */         \
	PC_DEFINE(CP(CSI2_DY0),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* CSI1_DX0 */         \
	PC_DEFINE(CP(CSI2_DX1),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* CSI1_DX0 */         \
	PC_DEFINE(CP(CSI2_DY1),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* CSI1_DX0 */         \
                                                                                                                \
	/* LCD lines */                                                                                         \
	PC_DEFINE(CP(DSS_ACBIAS),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_DAT_EN */       \
	PC_DEFINE(CP(DSS_DATA0),      PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D0 */           \
	PC_DEFINE(CP(DSS_DATA1),      PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D1 */           \
	PC_DEFINE(CP(DSS_DATA2),      PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D2 */           \
	PC_DEFINE(CP(DSS_DATA3),      PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D3 */           \
	PC_DEFINE(CP(DSS_DATA4),      PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D4 */           \
	PC_DEFINE(CP(DSS_DATA5),      PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D5 */           \
	PC_DEFINE(CP(DSS_DATA6),      PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D6 */           \
	PC_DEFINE(CP(DSS_DATA7),      PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D7 */           \
	PC_DEFINE(CP(DSS_DATA8),      PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D8 */           \
	PC_DEFINE(CP(DSS_DATA9),      PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D9 */           \
	PC_DEFINE(CP(DSS_DATA10),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D10 */          \
	PC_DEFINE(CP(DSS_DATA11),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D11 */          \
	PC_DEFINE(CP(DSS_DATA12),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D12 */          \
	PC_DEFINE(CP(DSS_DATA13),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D13 */          \
	PC_DEFINE(CP(DSS_DATA14),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D14 */          \
	PC_DEFINE(CP(DSS_DATA15),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D15 */          \
	PC_DEFINE(CP(DSS_DATA16),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D16 */          \
	PC_DEFINE(CP(DSS_DATA17),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D17 */          \
	PC_DEFINE(CP(DSS_DATA18),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D18 */          \
	PC_DEFINE(CP(DSS_DATA19),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D19 */          \
	PC_DEFINE(CP(DSS_DATA20),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D20 */          \
	PC_DEFINE(CP(DSS_DATA21),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D21 */          \
	PC_DEFINE(CP(DSS_DATA22),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D22 */          \
	PC_DEFINE(CP(DSS_DATA23),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_D23 */          \
	PC_DEFINE(CP(DSS_PCLK),       PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_DCLK */         \
	PC_DEFINE(CP(DSS_HSYNC),      PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_HSYNC */        \
	PC_DEFINE(CP(DSS_VSYNC),      PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* LCD_VSYNC */        \
                                                                                                                \
	/* WiFi lines */                                                                                        \
	PC_DEFINE(CP(ETK_D9),         PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE7)                        \
	PC_DEFINE(CP(ETK_D10),        PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE7)                        \
	PC_DEFINE(CP(ETK_D11),        PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE7)                        \
	PC_DEFINE(CP(ETK_D12),        PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE7)                        \
                                                                                                                \
	/* USB0 lines */                                                                                        \
	PC_DEFINE(CP(HSUSB0_DATA0),   PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* USB0_ULPI_DATA0 */  \
	PC_DEFINE(CP(HSUSB0_DATA1),   PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* USB0_ULPI_DATA1 */  \
	PC_DEFINE(CP(HSUSB0_DATA2),   PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* USB0_ULPI_DATA2 */  \
	PC_DEFINE(CP(HSUSB0_DATA3),   PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* USB0_ULPI_DATA3 */  \
	PC_DEFINE(CP(HSUSB0_DATA4),   PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* USB0_ULPI_DATA4 */  \
	PC_DEFINE(CP(HSUSB0_DATA5),   PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* USB0_ULPI_DATA5 */  \
	PC_DEFINE(CP(HSUSB0_DATA6),   PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* USB0_ULPI_DATA6 */  \
	PC_DEFINE(CP(HSUSB0_DATA7),   PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* USB0_ULPI_DATA7 */  \
	PC_DEFINE(CP(HSUSB0_CLK),     PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* USB0_ULPI_CLK */    \
	PC_DEFINE(CP(HSUSB0_STP),     PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* USB0_ULPI_STP */    \
	PC_DEFINE(CP(HSUSB0_DIR),     PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* USB0_ULPI_DIR */    \
	PC_DEFINE(CP(HSUSB0_NXT),     PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* USB0_ULPI_NXT */    \
                                                                                                                \
	/* SYS lines */                                                                                         \
	PC_DEFINE(CP(SYS_BOOT0),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* SYS_BOOT0 */        \
	PC_DEFINE(CP(SYS_BOOT1),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* SYS_BOOT1 */        \
	PC_DEFINE(CP(SYS_BOOT2),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* SYS_BOOT2 */        \
	PC_DEFINE(CP(SYS_BOOT3),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* SYS_BOOT3 */        \
	PC_DEFINE(CP(SYS_BOOT4),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* SYS_BOOT4 */        \
	PC_DEFINE(CP(SYS_BOOT5),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* SYS_BOOT5 */        \
	PC_DEFINE(CP(SYS_BOOT6),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* SYS_BOOT6 */        \
	PC_DEFINE(CP(SYS_CLKREQ),     PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* SYS_CLKREQ */       \
	PC_DEFINE(CP(SYS_NIRQ),       PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0 | PC_WAKEUP_EN) /* SYS_NIRQ */         \
	PC_DEFINE(CP(SYS_NRESWARM),   PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE0) /* SYS_NRESWARM */     \
	PC_DEFINE(CP(SYS_OFF_MODE),   PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* SYS_OFF_MODE */     \
	PC_DEFINE(CP(SYS_CLKOUT1),    PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE7) /* TP32 */             \
                                                                                                                \
	/* Unused GPMC lines (no mode7 for d0..d7 !) */                                                         \
	PC_DEFINE(CP(GPMC_D0),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_D0 */          \
	PC_DEFINE(CP(GPMC_D1),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_D1 */          \
	PC_DEFINE(CP(GPMC_D2),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_D2 */          \
	PC_DEFINE(CP(GPMC_D3),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_D3 */          \
	PC_DEFINE(CP(GPMC_D4),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_D4 */          \
	PC_DEFINE(CP(GPMC_D5),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_D5 */          \
	PC_DEFINE(CP(GPMC_D6),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_D6 */          \
	PC_DEFINE(CP(GPMC_D7),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* GPMC_D7 */          \
	PC_DEFINE(CP(GPMC_A5),        PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE7) /* TP183 */            \
	PC_DEFINE(CP(GPMC_NCS0),      PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* TP144 */            \
	PC_DEFINE(CP(GPMC_NCS6),      PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE7) /* TP145 */            \
	PC_DEFINE(CP(GPMC_NCS7),      PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE7) /* TP146 */            \
	PC_DEFINE(CP(GPMC_CLK),       PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE7) /* TP2 */              \
	PC_DEFINE(CP(GPMC_NWE),       PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE7) /* TP4 */              \
	PC_DEFINE(CP(GPMC_NOE),       PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE7) /* TP10 */             \
	PC_DEFINE(CP(GPMC_NADV_ALE),  PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE7) /* TP3 */              \
	PC_DEFINE(CP(GPMC_NWP),       PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE7) /* TP7 */              \
	PC_DEFINE(CP(GPMC_WAIT0),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0) /* TP23 */             \
	PC_DEFINE(CP(GPMC_WAIT1),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE7) /* TP19 */             \
                                                                                                                \
	/* I2C lines */                                                                                         \
	PC_DEFINE(CP(I2C2_SCL),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0)                        \
	PC_DEFINE(CP(I2C2_SDA),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0)                        \
	PC_DEFINE(CP(I2C3_SCL),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0)                        \
	PC_DEFINE(CP(I2C3_SDA),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0)                        \
	PC_DEFINE(CP(I2C4_SCL),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0)                        \
	PC_DEFINE(CP(I2C4_SDA),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE0)                        \
                                                                                                                \
	/* Unused JTAG lines */                                                                                 \
	PC_DEFINE(CP(JTAG_EMU0),      PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE7) /* TP24 */             \
	PC_DEFINE(CP(JTAG_EMU1),      PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE7) /* TP29 */             \
	                                                                                                        \
	/* GPIO lines */                                                                                        \
	PC_DEFINE(CP(CAM_XCLKA),      PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /*  96: CAM_PWR_ON */  \
	PC_DEFINE(CP(CAM_WEN),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* 167: TP33 */   \
	PC_DEFINE(CP(CAM_D10),        PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* 109: DISP_ON */     \
	PC_DEFINE(CP(CAM_D11),        PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* 110: LCM_PWR_ENn_1 */ \
	PC_DEFINE(CP(ETK_D0),         PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /*  14: BT_EN */       \
	PC_DEFINE(CP(ETK_D2),         PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /*  16: BT_WAKE */     \
	PC_DEFINE(CP(ETK_D7),         PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /*  21: HPWR */        \
	PC_DEFINE(CP(ETK_D8),         PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /*  22: USB_SUSP */    \
	PC_DEFINE(CP(MCBSP4_CLKX),    PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* 152: BT_HOST_WAKE */\
	PC_DEFINE(CP(MCBSP4_DR),      PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* 153: nGPS_TCXO_ON */\
	PC_DEFINE(CP(MCBSP4_DX),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* 154: LCM_ID */      \
	PC_DEFINE(CP(MCBSP_CLKS),     PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /* 160: nGPS_RESET */  \
	PC_DEFINE(CP(GPMC_A1),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /*  34: RDS_IRQ */     \
	PC_DEFINE(CP(GPMC_A2),        PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /*  35: RDS_RSTn */    \
	PC_DEFINE(CP(GPMC_A3),        PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /*  36: MUTE */        \
	PC_DEFINE(CP(GPMC_A7),        PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /*  40: LCD_PWR_ON */  \
	PC_DEFINE(CP(GPMC_A8),        PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /*  41: nLCD_STBY */   \
	PC_DEFINE(CP(GPMC_A9),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /*  42: GPS_1PPS */    \
	PC_DEFINE(CP(GPMC_A10),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /*  43: CPU_WAKE */    \
	PC_DEFINE(CP(GPMC_D8),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /*  44: DOCK_DET1 */    \
	PC_DEFINE(CP(GPMC_D9),        PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /*  45: DOCK_DET0 */            \
	PC_DEFINE(CP(GPMC_D10),       PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /*  46: EEPROM_WP */   \
	PC_DEFINE(CP(GPMC_D11),       PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /*  47: AR_BTRSTOUTn */\
	PC_DEFINE(CP(GPMC_D12),       PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /*  48: TILT_PWR */    \
	PC_DEFINE(CP(GPMC_D13),       PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /*  49: TILT_OUT */    \
	PC_DEFINE(CP(GPMC_D14),       PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /*  50: ACCESSORY_PWREN */ \
	PC_DEFINE(CP(GPMC_NCS2),      PC_INPUT  | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /*  53: USB_CHRG_DET */\
	PC_DEFINE(CP(GPMC_NCS4),      PC_INPUT  | PC_PULL_ENA | PC_PULL_UP   | PC_MODE4) /*  55: nON_OFF */     \
	PC_DEFINE(CP(GPMC_NCS5),      PC_OUTPUT | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /*  56: LCM_nCS */     \
	PC_DEFINE(CP(GPMC_NBE1),      PC_INPUT  | PC_PULL_DIS | PC_PULL_DOWN | PC_MODE4) /*  61: LOW_DC_VDD */  \
	PC_DEFINE(CP(SYS_CLKOUT2),    PC_OUTPUT | PC_PULL_ENA | PC_PULL_DOWN | PC_MODE4) /* 186: WALL_SENSE_CONNECT */
#endif // PADCONFIG_SANTIAGO_H

