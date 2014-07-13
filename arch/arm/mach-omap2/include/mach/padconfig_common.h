/*
 * Copyright (C) 2009 TomTom BV <http://www.tomtom.com/>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef PADCONFIG_COMMON_H
#define PADCONFIG_COMMON_H

/*
 * INFO: this file was created to clean up the mux config settings which were
 * at the time of the creation of this file in the x-loader, u-boot and kernel.
 * All modules initialized all PADCONFIG. This ofcourse is overdone, but besides
 * being initialized three times, all the settings were defined three times,
 * using different defines each time. This modules first step will be to make
 * one single table with all the PADCONFIG configuration which can be used in
 * all three modules.
 */

/* To keep the names short: PC = PADCONF */

/* First the defines which allow the table to be mapped to the header files   */
/* from TI. These defines have to be copied into the module using this table  */
/* So all following defines are between #if 0 and #endif statement but should */
/* not be removed.                                                            */

/********************************************/
/************* SAMPLE DEFINES ***************/
/********************************************/
#if 0
/*********** BIT DEFINES, never used, just for info, but could be used :-) ****/
#define PC_MODE0        (0 << 0)
#define PC_MODE1        (1 << 0)
#define PC_MODE2        (2 << 0)
#define PC_MODE3        (3 << 0)
#define PC_MODE4        (4 << 0)
#define PC_MODE5        (5 << 0)
#define PC_MODE6        (6 << 0)
#define PC_MODE7        (7 << 0)

#define PC_INPUT        (1 << 8)
#define PC_OUTPUT       (0 << 8)

#define PC_PULL_ENA     (1 << 3)
#define PC_PULL_DIS     (0 << 3)

#define PC_PULL_UP      (1 << 4)
#define PC_PULL_DOWN    (0 << 4)
/********************************************/
/*********** END SAMPLE DEFINES *************/ 
/********************************************/
#endif // 0

/*********** BOOTLOADER/U-BOOT, used in case of bootloader or u-boot **********/
#ifdef BOOTLOADER_UBOOT_PADCONFIG

/* Brilliantly enough the naming of the PADs are different between the */
/* x-loader and kernel. So we use defines to overcome the problem      */

#define CONTROL_PADCONF_SYS_NIRQ        CONTROL_PADCONF_SYS_nIRQ
#define CONTROL_PADCONF_SYS_NRESWARM    0x0A08
#define CONTROL_PADCONF_MCBSP3_CLKX     CONTROL_PADCONF_McBSP3_CLKX
#define CONTROL_PADCONF_MCBSP3_DR       CONTROL_PADCONF_McBSP3_DR
#define CONTROL_PADCONF_MCBSP3_DX       CONTROL_PADCONF_McBSP3_DX
#define CONTROL_PADCONF_MCBSP3_FSX      CONTROL_PADCONF_McBSP3_FSX
#define CONTROL_PADCONF_MCBSP4_FSX      CONTROL_PADCONF_McBSP4_FSX
#define CONTROL_PADCONF_MCBSP1_CLKX     CONTROL_PADCONF_McBSP1_CLKX
#define CONTROL_PADCONF_MCBSP1_DR       CONTROL_PADCONF_McBSP1_DR
#define CONTROL_PADCONF_MCBSP1_DX       CONTROL_PADCONF_McBSP1_DX
#define CONTROL_PADCONF_MCBSP1_FSX      CONTROL_PADCONF_McBSP1_FSX
#define CONTROL_PADCONF_MCBSP2_CLKX     CONTROL_PADCONF_McBSP2_CLKX
#define CONTROL_PADCONF_MCBSP2_DR       CONTROL_PADCONF_McBSP2_DR
#define CONTROL_PADCONF_MCBSP2_DX       CONTROL_PADCONF_McBSP2_DX
#define CONTROL_PADCONF_MCBSP2_FSX      CONTROL_PADCONF_McBSP2_FSX
#define CONTROL_PADCONF_MCSPI1_CLK      CONTROL_PADCONF_McSPI1_CLK
#define CONTROL_PADCONF_MCSPI1_SIMO     CONTROL_PADCONF_McSPI1_SIMO
#define CONTROL_PADCONF_MCSPI1_SOMI     CONTROL_PADCONF_McSPI1_SOMI
#define CONTROL_PADCONF_MCSPI1_CS0      CONTROL_PADCONF_McSPI1_CS0
#define CONTROL_PADCONF_MCSPI1_CS1      CONTROL_PADCONF_McSPI1_CS1
#define CONTROL_PADCONF_MCSPI1_CS2      CONTROL_PADCONF_McSPI1_CS2
#define CONTROL_PADCONF_MCSPI1_CS3      CONTROL_PADCONF_McSPI1_CS3
#define CONTROL_PADCONF_MCSPI2_CLK      CONTROL_PADCONF_McSPI2_CLK
#define CONTROL_PADCONF_MCSPI2_SIMO     CONTROL_PADCONF_McSPI2_SIMO
#define CONTROL_PADCONF_MCSPI2_SOMI     CONTROL_PADCONF_McSPI2_SOMI
#define CONTROL_PADCONF_MCSPI2_CS0      CONTROL_PADCONF_McSPI2_CS0
#define CONTROL_PADCONF_MCSPI2_CS1      CONTROL_PADCONF_McSPI2_CS1
#define CONTROL_PADCONF_SDMMC1_CLK      CONTROL_PADCONF_MMC1_CLK
#define CONTROL_PADCONF_SDMMC1_CMD      CONTROL_PADCONF_MMC1_CMD
#define CONTROL_PADCONF_SDMMC1_DAT0     CONTROL_PADCONF_MMC1_DAT0
#define CONTROL_PADCONF_SDMMC1_DAT1     CONTROL_PADCONF_MMC1_DAT1
#define CONTROL_PADCONF_SDMMC1_DAT2     CONTROL_PADCONF_MMC1_DAT2
#define CONTROL_PADCONF_SDMMC1_DAT3     CONTROL_PADCONF_MMC1_DAT3
#define CONTROL_PADCONF_SDMMC1_DAT6     CONTROL_PADCONF_MMC1_DAT6
#define CONTROL_PADCONF_SDMMC2_CLK      CONTROL_PADCONF_MMC2_CLK
#define CONTROL_PADCONF_SDMMC2_CMD      CONTROL_PADCONF_MMC2_CMD
#define CONTROL_PADCONF_SDMMC2_DAT0     CONTROL_PADCONF_MMC2_DAT0
#define CONTROL_PADCONF_SDMMC2_DAT1     CONTROL_PADCONF_MMC2_DAT1
#define CONTROL_PADCONF_SDMMC2_DAT2     CONTROL_PADCONF_MMC2_DAT2
#define CONTROL_PADCONF_SDMMC2_DAT3     CONTROL_PADCONF_MMC2_DAT3
#define CONTROL_PADCONF_SDMMC2_DAT4     CONTROL_PADCONF_MMC2_DAT4
#define CONTROL_PADCONF_SDMMC2_DAT5     CONTROL_PADCONF_MMC2_DAT5
#define CONTROL_PADCONF_SDMMC2_DAT6     CONTROL_PADCONF_MMC2_DAT6
#define CONTROL_PADCONF_SDMMC2_DAT7     CONTROL_PADCONF_MMC2_DAT7
#define CONTROL_PADCONF_GPMC_NCS0       CONTROL_PADCONF_GPMC_nCS0
#define CONTROL_PADCONF_GPMC_NCS6       CONTROL_PADCONF_GPMC_nCS6
#define CONTROL_PADCONF_GPMC_NCS7       CONTROL_PADCONF_GPMC_nCS7
#define CONTROL_PADCONF_GPMC_NWE        CONTROL_PADCONF_GPMC_nWE
#define CONTROL_PADCONF_GPMC_NOE        CONTROL_PADCONF_GPMC_nOE
#define CONTROL_PADCONF_GPMC_NADV_ALE   CONTROL_PADCONF_GPMC_nADV_ALE
#define CONTROL_PADCONF_GPMC_NWP        CONTROL_PADCONF_GPMC_nWP
#define CONTROL_PADCONF_MCBSP4_CLKX     CONTROL_PADCONF_McBSP4_CLKX
#define CONTROL_PADCONF_MCBSP4_DR       CONTROL_PADCONF_McBSP4_DR
#define CONTROL_PADCONF_MCBSP4_DX       CONTROL_PADCONF_McBSP4_DX
#define CONTROL_PADCONF_MCBSP1_CLKR     CONTROL_PADCONF_McBSP1_CLKR
#define CONTROL_PADCONF_MCBSP1_FSR      CONTROL_PADCONF_McBSP1_FSR
#define CONTROL_PADCONF_MCBSP_CLKS      CONTROL_PADCONF_McBSP_CLKS
#define CONTROL_PADCONF_SDMMC1_DAT4     CONTROL_PADCONF_MMC1_DAT4
#define CONTROL_PADCONF_SDMMC1_DAT5     CONTROL_PADCONF_MMC1_DAT5
#define CONTROL_PADCONF_SDMMC1_DAT7     CONTROL_PADCONF_MMC1_DAT7
#define CONTROL_PADCONF_GPMC_NCS1       CONTROL_PADCONF_GPMC_nCS1
#define CONTROL_PADCONF_GPMC_NCS2       CONTROL_PADCONF_GPMC_nCS2
#define CONTROL_PADCONF_GPMC_NCS3       CONTROL_PADCONF_GPMC_nCS3
#define CONTROL_PADCONF_GPMC_NCS4       CONTROL_PADCONF_GPMC_nCS4
#define CONTROL_PADCONF_GPMC_NCS5       CONTROL_PADCONF_GPMC_nCS5
#define CONTROL_PADCONF_GPMC_NBE0_CLE   CONTROL_PADCONF_GPMC_nBE0_CLE
#define CONTROL_PADCONF_GPMC_NBE1       CONTROL_PADCONF_GPMC_nBE1

#define PC_DEFINE               MUX_VAL
#define PC_MODE0                M0
#define PC_MODE1                M1
#define PC_MODE2                M2
#define PC_MODE3                M3
#define PC_MODE4                M4
#define PC_MODE5                M5
#define PC_MODE6                M6
#define PC_MODE7                M7
#define PC_INPUT                IEN
#define PC_OUTPUT               IDIS
#define PC_PULL_ENA             EN
#define PC_PULL_DIS             DIS
#define PC_PULL_UP              PTU
#define PC_PULL_DOWN            PTD
#define PC_WAKEUP_EN            (1 << 14)
#define PC_OFF_EN               (1 << 9)
#define PC_OFFOUT_EN		(1 << 10)
#define PC_OFF_OUT_LOW		(PC_OFF_EN)
#define PC_OFF_IN_NOPULL	(PC_OFF_EN | PC_OFFOUT_EN)
#endif // BOOTLOADER_UBOOT_PADCONFIG

/*********** KERNEL, used in case of kernel build, board-santiago.c ***********/
#ifdef KERNEL_PADCONFIG

#define PC_DEFINE(x,y)          OMAP3_MUX(x,y),
#define CP(x)                   x
#define PC_MODE0                OMAP_MUX_MODE0
#define PC_MODE1                OMAP_MUX_MODE1
#define PC_MODE2                OMAP_MUX_MODE2
#define PC_MODE3                OMAP_MUX_MODE3
#define PC_MODE4                OMAP_MUX_MODE4
#define PC_MODE5                OMAP_MUX_MODE5
#define PC_MODE6                OMAP_MUX_MODE6
#define PC_MODE7                OMAP_MUX_MODE7
#define PC_INPUT                OMAP_PIN_INPUT
#define PC_OUTPUT               OMAP_PIN_OUTPUT
#define PC_PULL_ENA             OMAP_PULL_ENA
#define PC_PULL_DIS             (0 << 3)
#define PC_PULL_UP              OMAP_PULL_UP
#define PC_PULL_DOWN            (0 << 4)
#define PC_WAKEUP_EN            OMAP_WAKEUP_EN
#define PC_OFF_EN               OMAP_OFF_EN
#define PC_OFFOUT_EN            OMAP_OFFOUT_EN
#define PC_OFFOUT_VAL           OMAP_OFFOUT_VAL
#define PC_OFF_PULL_EN          OMAP_OFF_PULL_EN
#define PC_OFF_PULL_UP          OMAP_OFF_PULL_UP
#define PC_OFF_OUT_LOW		OMAP_PIN_OFF_OUTPUT_LOW
#define PC_OFF_IN_NOPULL	OMAP_PIN_OFF_INPUT_NOPULL

#endif // KERNEL_PADCONFIG

#endif
