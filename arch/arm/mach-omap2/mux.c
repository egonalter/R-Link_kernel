/*
 * linux/arch/arm/mach-omap2/mux.c
 *
 * OMAP2 and OMAP3 pin multiplexing configurations
 *
 * Copyright (C) 2004 - 2008 Texas Instruments Inc.
 * Copyright (C) 2003 - 2008 Nokia Corporation
 *
 * Written by Tony Lindgren
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/spinlock.h>
#include <linux/list.h>
#include <linux/ctype.h>
#include <linux/debugfs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <asm/system.h>

#include <plat/control.h>
#include <plat/mux.h>
#include <mach/gpio.h>

#include "mux.h"

#define OMAP_MUX_BASE_OFFSET		0x30	/* Offset from CTRL_BASE */
#define OMAP_MUX_BASE_SZ		0x5ca

struct omap_mux_entry {
	struct omap_mux		mux;
	struct list_head	node;
};

static unsigned long __attribute__ ((unused)) mux_phys;
static void __iomem *mux_base;

static inline u16 omap_mux_read(u16 reg)
{
	if (cpu_is_omap24xx())
		return __raw_readb(mux_base + reg);
	else
		return __raw_readw(mux_base + reg);
}

static inline void omap_mux_write(u16 val, u16 reg)
{
	if (cpu_is_omap24xx())
		__raw_writeb(val, mux_base + reg);
	else
		__raw_writew(val, mux_base + reg);
}

#if defined(CONFIG_ARCH_OMAP24XX) && defined(CONFIG_OMAP_MUX)

static struct omap_mux_cfg arch_mux_cfg;

/* NOTE: See mux.h for the enumeration */

static struct pin_config __initdata_or_module omap24xx_pins[] = {
/*
 *	description			mux	mux	pull	pull	debug
 *					offset	mode	ena	type
 */

/* 24xx I2C */
MUX_CFG_24XX("M19_24XX_I2C1_SCL",	0x111,	0,	0,	0,	1)
MUX_CFG_24XX("L15_24XX_I2C1_SDA",	0x112,	0,	0,	0,	1)
MUX_CFG_24XX("J15_24XX_I2C2_SCL",	0x113,	0,	0,	1,	1)
MUX_CFG_24XX("H19_24XX_I2C2_SDA",	0x114,	0,	0,	0,	1)

/* Menelaus interrupt */
MUX_CFG_24XX("W19_24XX_SYS_NIRQ",	0x12c,	0,	1,	1,	1)

/* 24xx clocks */
MUX_CFG_24XX("W14_24XX_SYS_CLKOUT",	0x137,	0,	1,	1,	1)

/* 24xx GPMC chipselects, wait pin monitoring */
MUX_CFG_24XX("E2_GPMC_NCS2",		0x08e,	0,	1,	1,	1)
MUX_CFG_24XX("L2_GPMC_NCS7",		0x093,	0,	1,	1,	1)
MUX_CFG_24XX("L3_GPMC_WAIT0",		0x09a,	0,	1,	1,	1)
MUX_CFG_24XX("N7_GPMC_WAIT1",		0x09b,	0,	1,	1,	1)
MUX_CFG_24XX("M1_GPMC_WAIT2",		0x09c,	0,	1,	1,	1)
MUX_CFG_24XX("P1_GPMC_WAIT3",		0x09d,	0,	1,	1,	1)

/* 24xx McBSP */
MUX_CFG_24XX("Y15_24XX_MCBSP2_CLKX",	0x124,	1,	1,	0,	1)
MUX_CFG_24XX("R14_24XX_MCBSP2_FSX",	0x125,	1,	1,	0,	1)
MUX_CFG_24XX("W15_24XX_MCBSP2_DR",	0x126,	1,	1,	0,	1)
MUX_CFG_24XX("V15_24XX_MCBSP2_DX",	0x127,	1,	1,	0,	1)

/* 24xx GPIO */
MUX_CFG_24XX("M21_242X_GPIO11",		0x0c9,	3,	1,	1,	1)
MUX_CFG_24XX("P21_242X_GPIO12",		0x0ca,	3,	0,	0,	1)
MUX_CFG_24XX("AA10_242X_GPIO13",	0x0e5,	3,	0,	0,	1)
MUX_CFG_24XX("AA6_242X_GPIO14",		0x0e6,	3,	0,	0,	1)
MUX_CFG_24XX("AA4_242X_GPIO15",		0x0e7,	3,	0,	0,	1)
MUX_CFG_24XX("Y11_242X_GPIO16",		0x0e8,	3,	0,	0,	1)
MUX_CFG_24XX("AA12_242X_GPIO17",	0x0e9,	3,	0,	0,	1)
MUX_CFG_24XX("AA8_242X_GPIO58",		0x0ea,	3,	0,	0,	1)
MUX_CFG_24XX("Y20_24XX_GPIO60",		0x12c,	3,	0,	0,	1)
MUX_CFG_24XX("W4__24XX_GPIO74",		0x0f2,	3,	0,	0,	1)
MUX_CFG_24XX("N15_24XX_GPIO85",		0x103,	3,	0,	0,	1)
MUX_CFG_24XX("M15_24XX_GPIO92",		0x10a,	3,	0,	0,	1)
MUX_CFG_24XX("P20_24XX_GPIO93",		0x10b,	3,	0,	0,	1)
MUX_CFG_24XX("P18_24XX_GPIO95",		0x10d,	3,	0,	0,	1)
MUX_CFG_24XX("M18_24XX_GPIO96",		0x10e,	3,	0,	0,	1)
MUX_CFG_24XX("L14_24XX_GPIO97",		0x10f,	3,	0,	0,	1)
MUX_CFG_24XX("J15_24XX_GPIO99",		0x113,	3,	1,	1,	1)
MUX_CFG_24XX("V14_24XX_GPIO117",	0x128,	3,	1,	0,	1)
MUX_CFG_24XX("P14_24XX_GPIO125",	0x140,	3,	1,	1,	1)

/* 242x DBG GPIO */
MUX_CFG_24XX("V4_242X_GPIO49",		0xd3,	3,	0,	0,	1)
MUX_CFG_24XX("W2_242X_GPIO50",		0xd4,	3,	0,	0,	1)
MUX_CFG_24XX("U4_242X_GPIO51",		0xd5,	3,	0,	0,	1)
MUX_CFG_24XX("V3_242X_GPIO52",		0xd6,	3,	0,	0,	1)
MUX_CFG_24XX("V2_242X_GPIO53",		0xd7,	3,	0,	0,	1)
MUX_CFG_24XX("V6_242X_GPIO53",		0xcf,	3,	0,	0,	1)
MUX_CFG_24XX("T4_242X_GPIO54",		0xd8,	3,	0,	0,	1)
MUX_CFG_24XX("Y4_242X_GPIO54",		0xd0,	3,	0,	0,	1)
MUX_CFG_24XX("T3_242X_GPIO55",		0xd9,	3,	0,	0,	1)
MUX_CFG_24XX("U2_242X_GPIO56",		0xda,	3,	0,	0,	1)

/* 24xx external DMA requests */
MUX_CFG_24XX("AA10_242X_DMAREQ0",	0x0e5,	2,	0,	0,	1)
MUX_CFG_24XX("AA6_242X_DMAREQ1",	0x0e6,	2,	0,	0,	1)
MUX_CFG_24XX("E4_242X_DMAREQ2",		0x074,	2,	0,	0,	1)
MUX_CFG_24XX("G4_242X_DMAREQ3",		0x073,	2,	0,	0,	1)
MUX_CFG_24XX("D3_242X_DMAREQ4",		0x072,	2,	0,	0,	1)
MUX_CFG_24XX("E3_242X_DMAREQ5",		0x071,	2,	0,	0,	1)

/* UART3 */
MUX_CFG_24XX("K15_24XX_UART3_TX",	0x118,	0,	0,	0,	1)
MUX_CFG_24XX("K14_24XX_UART3_RX",	0x119,	0,	0,	0,	1)

/* MMC/SDIO */
MUX_CFG_24XX("G19_24XX_MMC_CLKO",	0x0f3,	0,	0,	0,	1)
MUX_CFG_24XX("H18_24XX_MMC_CMD",	0x0f4,	0,	0,	0,	1)
MUX_CFG_24XX("F20_24XX_MMC_DAT0",	0x0f5,	0,	0,	0,	1)
MUX_CFG_24XX("H14_24XX_MMC_DAT1",	0x0f6,	0,	0,	0,	1)
MUX_CFG_24XX("E19_24XX_MMC_DAT2",	0x0f7,	0,	0,	0,	1)
MUX_CFG_24XX("D19_24XX_MMC_DAT3",	0x0f8,	0,	0,	0,	1)
MUX_CFG_24XX("F19_24XX_MMC_DAT_DIR0",	0x0f9,	0,	0,	0,	1)
MUX_CFG_24XX("E20_24XX_MMC_DAT_DIR1",	0x0fa,	0,	0,	0,	1)
MUX_CFG_24XX("F18_24XX_MMC_DAT_DIR2",	0x0fb,	0,	0,	0,	1)
MUX_CFG_24XX("E18_24XX_MMC_DAT_DIR3",	0x0fc,	0,	0,	0,	1)
MUX_CFG_24XX("G18_24XX_MMC_CMD_DIR",	0x0fd,	0,	0,	0,	1)
MUX_CFG_24XX("H15_24XX_MMC_CLKI",	0x0fe,	0,	0,	0,	1)

/* Full speed USB */
MUX_CFG_24XX("J20_24XX_USB0_PUEN",	0x11d,	0,	0,	0,	1)
MUX_CFG_24XX("J19_24XX_USB0_VP",	0x11e,	0,	0,	0,	1)
MUX_CFG_24XX("K20_24XX_USB0_VM",	0x11f,	0,	0,	0,	1)
MUX_CFG_24XX("J18_24XX_USB0_RCV",	0x120,	0,	0,	0,	1)
MUX_CFG_24XX("K19_24XX_USB0_TXEN",	0x121,	0,	0,	0,	1)
MUX_CFG_24XX("J14_24XX_USB0_SE0",	0x122,	0,	0,	0,	1)
MUX_CFG_24XX("K18_24XX_USB0_DAT",	0x123,	0,	0,	0,	1)

MUX_CFG_24XX("N14_24XX_USB1_SE0",	0x0ed,	2,	0,	0,	1)
MUX_CFG_24XX("W12_24XX_USB1_SE0",	0x0dd,	3,	0,	0,	1)
MUX_CFG_24XX("P15_24XX_USB1_DAT",	0x0ee,	2,	0,	0,	1)
MUX_CFG_24XX("R13_24XX_USB1_DAT",	0x0e0,	3,	0,	0,	1)
MUX_CFG_24XX("W20_24XX_USB1_TXEN",	0x0ec,	2,	0,	0,	1)
MUX_CFG_24XX("P13_24XX_USB1_TXEN",	0x0df,	3,	0,	0,	1)
MUX_CFG_24XX("V19_24XX_USB1_RCV",	0x0eb,	2,	0,	0,	1)
MUX_CFG_24XX("V12_24XX_USB1_RCV",	0x0de,	3,	0,	0,	1)

MUX_CFG_24XX("AA10_24XX_USB2_SE0",	0x0e5,	2,	0,	0,	1)
MUX_CFG_24XX("Y11_24XX_USB2_DAT",	0x0e8,	2,	0,	0,	1)
MUX_CFG_24XX("AA12_24XX_USB2_TXEN",	0x0e9,	2,	0,	0,	1)
MUX_CFG_24XX("AA6_24XX_USB2_RCV",	0x0e6,	2,	0,	0,	1)
MUX_CFG_24XX("AA4_24XX_USB2_TLLSE0",	0x0e7,	2,	0,	0,	1)

/* Keypad GPIO*/
MUX_CFG_24XX("T19_24XX_KBR0",		0x106,	3,	1,	1,	1)
MUX_CFG_24XX("R19_24XX_KBR1",		0x107,	3,	1,	1,	1)
MUX_CFG_24XX("V18_24XX_KBR2",		0x139,	3,	1,	1,	1)
MUX_CFG_24XX("M21_24XX_KBR3",		0xc9,	3,	1,	1,	1)
MUX_CFG_24XX("E5__24XX_KBR4",		0x138,	3,	1,	1,	1)
MUX_CFG_24XX("M18_24XX_KBR5",		0x10e,	3,	1,	1,	1)
MUX_CFG_24XX("R20_24XX_KBC0",		0x108,	3,	0,	0,	1)
MUX_CFG_24XX("M14_24XX_KBC1",		0x109,	3,	0,	0,	1)
MUX_CFG_24XX("H19_24XX_KBC2",		0x114,	3,	0,	0,	1)
MUX_CFG_24XX("V17_24XX_KBC3",		0x135,	3,	0,	0,	1)
MUX_CFG_24XX("P21_24XX_KBC4",		0xca,	3,	0,	0,	1)
MUX_CFG_24XX("L14_24XX_KBC5",		0x10f,	3,	0,	0,	1)
MUX_CFG_24XX("N19_24XX_KBC6",		0x110,	3,	0,	0,	1)

/* 24xx Menelaus Keypad GPIO */
MUX_CFG_24XX("B3__24XX_KBR5",		0x30,	3,	1,	1,	1)
MUX_CFG_24XX("AA4_24XX_KBC2",		0xe7,	3,	0,	0,	1)
MUX_CFG_24XX("B13_24XX_KBC6",		0x110,	3,	0,	0,	1)

/* 2430 USB */
MUX_CFG_24XX("AD9_2430_USB0_PUEN",	0x133,	4,	0,	0,	1)
MUX_CFG_24XX("Y11_2430_USB0_VP",	0x134,	4,	0,	0,	1)
MUX_CFG_24XX("AD7_2430_USB0_VM",	0x135,	4,	0,	0,	1)
MUX_CFG_24XX("AE7_2430_USB0_RCV",	0x136,	4,	0,	0,	1)
MUX_CFG_24XX("AD4_2430_USB0_TXEN",	0x137,	4,	0,	0,	1)
MUX_CFG_24XX("AF9_2430_USB0_SE0",	0x138,	4,	0,	0,	1)
MUX_CFG_24XX("AE6_2430_USB0_DAT",	0x139,	4,	0,	0,	1)
MUX_CFG_24XX("AD24_2430_USB1_SE0",	0x107,	2,	0,	0,	1)
MUX_CFG_24XX("AB24_2430_USB1_RCV",	0x108,	2,	0,	0,	1)
MUX_CFG_24XX("Y25_2430_USB1_TXEN",	0x109,	2,	0,	0,	1)
MUX_CFG_24XX("AA26_2430_USB1_DAT",	0x10A,	2,	0,	0,	1)

/* 2430 HS-USB */
MUX_CFG_24XX("AD9_2430_USB0HS_DATA3",	0x133,	0,	0,	0,	1)
MUX_CFG_24XX("Y11_2430_USB0HS_DATA4",	0x134,	0,	0,	0,	1)
MUX_CFG_24XX("AD7_2430_USB0HS_DATA5",	0x135,	0,	0,	0,	1)
MUX_CFG_24XX("AE7_2430_USB0HS_DATA6",	0x136,	0,	0,	0,	1)
MUX_CFG_24XX("AD4_2430_USB0HS_DATA2",	0x137,	0,	0,	0,	1)
MUX_CFG_24XX("AF9_2430_USB0HS_DATA0",	0x138,	0,	0,	0,	1)
MUX_CFG_24XX("AE6_2430_USB0HS_DATA1",	0x139,	0,	0,	0,	1)
MUX_CFG_24XX("AE8_2430_USB0HS_CLK",	0x13A,	0,	0,	0,	1)
MUX_CFG_24XX("AD8_2430_USB0HS_DIR",	0x13B,	0,	0,	0,	1)
MUX_CFG_24XX("AE5_2430_USB0HS_STP",	0x13c,	0,	1,	1,	1)
MUX_CFG_24XX("AE9_2430_USB0HS_NXT",	0x13D,	0,	0,	0,	1)
MUX_CFG_24XX("AC7_2430_USB0HS_DATA7",	0x13E,	0,	0,	0,	1)

/* 2430 McBSP */
MUX_CFG_24XX("AD6_2430_MCBSP_CLKS",	0x011E,	0,	0,	0,	1)

MUX_CFG_24XX("AB2_2430_MCBSP1_CLKR",	0x011A,	0,	0,	0,	1)
MUX_CFG_24XX("AD5_2430_MCBSP1_FSR",	0x011B,	0,	0,	0,	1)
MUX_CFG_24XX("AA1_2430_MCBSP1_DX",	0x011C,	0,	0,	0,	1)
MUX_CFG_24XX("AF3_2430_MCBSP1_DR",	0x011D,	0,	0,	0,	1)
MUX_CFG_24XX("AB3_2430_MCBSP1_FSX",	0x011F,	0,	0,	0,	1)
MUX_CFG_24XX("Y9_2430_MCBSP1_CLKX",	0x0120,	0,	0,	0,	1)

MUX_CFG_24XX("AC10_2430_MCBSP2_FSX",	0x012E,	1,	0,	0,	1)
MUX_CFG_24XX("AD16_2430_MCBSP2_CLX",	0x012F,	1,	0,	0,	1)
MUX_CFG_24XX("AE13_2430_MCBSP2_DX",	0x0130,	1,	0,	0,	1)
MUX_CFG_24XX("AD13_2430_MCBSP2_DR",	0x0131,	1,	0,	0,	1)
MUX_CFG_24XX("AC10_2430_MCBSP2_FSX_OFF",0x012E,	0,	0,	0,	1)
MUX_CFG_24XX("AD16_2430_MCBSP2_CLX_OFF",0x012F,	0,	0,	0,	1)
MUX_CFG_24XX("AE13_2430_MCBSP2_DX_OFF",	0x0130,	0,	0,	0,	1)
MUX_CFG_24XX("AD13_2430_MCBSP2_DR_OFF",	0x0131,	0,	0,	0,	1)

MUX_CFG_24XX("AC9_2430_MCBSP3_CLKX",	0x0103,	0,	0,	0,	1)
MUX_CFG_24XX("AE4_2430_MCBSP3_FSX",	0x0104,	0,	0,	0,	1)
MUX_CFG_24XX("AE2_2430_MCBSP3_DR",	0x0105,	0,	0,	0,	1)
MUX_CFG_24XX("AF4_2430_MCBSP3_DX",	0x0106,	0,	0,	0,	1)

MUX_CFG_24XX("N3_2430_MCBSP4_CLKX",	0x010B,	1,	0,	0,	1)
MUX_CFG_24XX("AD23_2430_MCBSP4_DR",	0x010C,	1,	0,	0,	1)
MUX_CFG_24XX("AB25_2430_MCBSP4_DX",	0x010D,	1,	0,	0,	1)
MUX_CFG_24XX("AC25_2430_MCBSP4_FSX",	0x010E,	1,	0,	0,	1)

MUX_CFG_24XX("AE16_2430_MCBSP5_CLKX",	0x00ED,	1,	0,	0,	1)
MUX_CFG_24XX("AF12_2430_MCBSP5_FSX",	0x00ED,	1,	0,	0,	1)
MUX_CFG_24XX("K7_2430_MCBSP5_DX",	0x00EF,	1,	0,	0,	1)
MUX_CFG_24XX("M1_2430_MCBSP5_DR",	0x00F0,	1,	0,	0,	1)

/* 2430 MCSPI1 */
MUX_CFG_24XX("Y18_2430_MCSPI1_CLK",	0x010F,	0,	0,	0,	1)
MUX_CFG_24XX("AD15_2430_MCSPI1_SIMO",	0x0110,	0,	0,	0,	1)
MUX_CFG_24XX("AE17_2430_MCSPI1_SOMI",	0x0111,	0,	0,	0,	1)
MUX_CFG_24XX("U1_2430_MCSPI1_CS0",	0x0112,	0,	0,	0,	1)

/* Touchscreen GPIO */
MUX_CFG_24XX("AF19_2430_GPIO_85",	0x0113,	3,	0,	0,	1)

/* pico DLP */
MUX_CFG_34XX("DLP_4430_GPIO_40", 0x060,
		OMAP34XX_MUX_MODE3 | OMAP34XX_PIN_OUTPUT)
MUX_CFG_34XX("DLP_4430_GPIO_44", 0x068,
		OMAP34XX_MUX_MODE3 | OMAP34XX_PIN_INPUT)
MUX_CFG_34XX("DLP_4430_GPIO_45", 0x06A,
		OMAP34XX_MUX_MODE3 | OMAP34XX_PIN_OUTPUT)
MUX_CFG_34XX("DLP_4430_GPIO_49", 0x084,
		OMAP34XX_MUX_MODE3 | OMAP34XX_PIN_OUTPUT)
};

#define OMAP24XX_PINS_SZ	ARRAY_SIZE(omap24xx_pins)

#if defined(CONFIG_OMAP_MUX_DEBUG) || defined(CONFIG_OMAP_MUX_WARNINGS)

static void __init_or_module omap2_cfg_debug(const struct pin_config *cfg, u16 reg)
{
	u16 orig;
	u8 warn = 0, debug = 0;

	orig = omap_mux_read(cfg->mux_reg - OMAP_MUX_BASE_OFFSET);

#ifdef	CONFIG_OMAP_MUX_DEBUG
	debug = cfg->debug;
#endif
	warn = (orig != reg);
	if (debug || warn)
		printk(KERN_WARNING
			"MUX: setup %s (0x%p): 0x%04x -> 0x%04x\n",
			cfg->name, omap_ctrl_base_get() + cfg->mux_reg,
			orig, reg);
}
#else
#define omap2_cfg_debug(x, y)	do {} while (0)
#endif

static int __init_or_module omap24xx_cfg_reg(const struct pin_config *cfg)
{
	static DEFINE_SPINLOCK(mux_spin_lock);
	unsigned long flags;
	u8 reg = 0;

	spin_lock_irqsave(&mux_spin_lock, flags);
	reg |= cfg->mask & 0x7;
	if (cfg->pull_val)
		reg |= OMAP2_PULL_ENA;
	if (cfg->pu_pd_val)
		reg |= OMAP2_PULL_UP;
	omap2_cfg_debug(cfg, reg);
	omap_mux_write(reg, cfg->mux_reg - OMAP_MUX_BASE_OFFSET);
	spin_unlock_irqrestore(&mux_spin_lock, flags);

	return 0;
}

int __init omap2_mux_init(void)
{
	u32 mux_pbase;

	if (cpu_is_omap2420())
		mux_pbase = OMAP2420_CTRL_BASE + OMAP_MUX_BASE_OFFSET;
	else if (cpu_is_omap2430())
		mux_pbase = OMAP243X_CTRL_BASE + OMAP_MUX_BASE_OFFSET;
	else
		return -ENODEV;

	mux_base = ioremap(mux_pbase, OMAP_MUX_BASE_SZ);
	if (!mux_base) {
		printk(KERN_ERR "mux: Could not ioremap\n");
		return -ENODEV;
	}

	if (cpu_is_omap24xx()) {
		arch_mux_cfg.pins	= omap24xx_pins;
		arch_mux_cfg.size	= OMAP24XX_PINS_SZ;
		arch_mux_cfg.cfg_reg	= omap24xx_cfg_reg;

		return omap_mux_register(&arch_mux_cfg);
	}

	return 0;
}

#else
int __init omap2_mux_init(void)
{
	return 0;
}
#endif	/* CONFIG_OMAP_MUX */

/*----------------------------------------------------------------------------*/

#ifdef CONFIG_ARCH_OMAP34XX
static LIST_HEAD(muxmodes);
static DEFINE_MUTEX(muxmode_mutex);

#ifdef CONFIG_OMAP_MUX

static char *omap_mux_options;

int __init omap_mux_init_gpio(int gpio, int val)
{
	struct omap_mux_entry *e;
	int found = 0;

	if (!gpio)
		return -EINVAL;

	list_for_each_entry(e, &muxmodes, node) {
		struct omap_mux *m = &e->mux;
		if (gpio == m->gpio) {
			u16 old_mode;
			u16 mux_mode;

			old_mode = omap_mux_read(m->reg_offset);
			mux_mode = val & ~(OMAP_MUX_NR_MODES - 1);
			mux_mode |= OMAP_MUX_MODE4;
			printk(KERN_DEBUG "mux: Setting signal "
				"%s.gpio%i 0x%04x -> 0x%04x\n",
				m->muxnames[0], gpio, old_mode, mux_mode);
			omap_mux_write(mux_mode, m->reg_offset);
			found++;
		}
	}

	if (found == 1)
		return 0;

	if (found > 1) {
		printk(KERN_ERR "mux: Multiple gpio paths for gpio%i\n", gpio);
		return -EINVAL;
	}

	printk(KERN_ERR "mux: Could not set gpio%i\n", gpio);

	return -ENODEV;
}

static int omap_mux_process_signal(char *muxname, int newval, int *oldval, bool write)
{
	struct omap_mux_entry *e;
	char *m0_name = NULL, *mode_name = NULL;
	int found = 0;
	char *muxname_local;

	muxname_local = kzalloc(sizeof(strlen(muxname)+1), GFP_KERNEL);

	if (!muxname_local)
		return -ENOMEM;

	strcpy(muxname_local, muxname);

	mode_name = strchr(muxname_local, '.');
	if (mode_name) {
		*mode_name = '\0';
		mode_name++;
		m0_name = muxname_local;
	} else {
		mode_name = muxname_local;
	}

	list_for_each_entry(e, &muxmodes, node) {
		struct omap_mux *m = &e->mux;
		char *m0_entry = m->muxnames[0];
		int i;

		if (!m0_entry || (m0_name && strcmp(m0_name, m0_entry)))
			continue;

		for (i = 0; i < OMAP_MUX_NR_MODES; i++) {
			char *mode_cur = m->muxnames[i];

			if (!mode_cur)
				continue;

			if (!strcmp(mode_name, mode_cur)) {
				u16 old_mode;
				u16 mux_mode;

				old_mode = omap_mux_read(m->reg_offset);
				if (write) {
					mux_mode = newval | i;
					printk(KERN_DEBUG "mux: Setting signal "
						"%s.%s 0x%04x -> 0x%04x\n",
						m0_entry, muxname_local, old_mode, mux_mode);
					omap_mux_write(mux_mode, m->reg_offset);
				} else {
					*oldval = old_mode;
				}
				found++;
			}
		}
	}

	kfree(muxname_local);

	if (found == 1)
		return 0;

	if (found > 1) {
		printk(KERN_ERR "mux: Multiple signal paths (%i) for %s\n",
				found, muxname);
		return -EINVAL;
	}

	printk(KERN_ERR "mux: Could not set signal %s\n", muxname);

	return -ENODEV;
}

int omap_mux_init_signal(char *muxname, int val)
{
	return omap_mux_process_signal(muxname, val, NULL, true);
}

int omap_mux_get_signal(char *muxname, int *val)
{
	return omap_mux_process_signal(muxname, 0, val, false);
}

int omap3_mux_config(char *group)
{
	printk(KERN_ERR "set mux for group mode = %s\n", group);
	if (strcmp(group, "OMAP_MCBSP2_SLAVE") == 0) {
		omap_mux_init_signal("mcbsp2_fsx.mcbsp2_fsx",
						OMAP_PIN_INPUT);
		omap_mux_init_signal("mcbsp2_clkx.mcbsp2_clkx",
						OMAP_PIN_INPUT);
		omap_mux_init_signal("mcbsp2_dr.mcbsp2_dr",
						OMAP_PIN_INPUT);
		omap_mux_init_signal("mcbsp2_dx.mcbsp2_dx",
						OMAP_PIN_OUTPUT);
		return 0;
	} else if (strcmp(group, "OMAP_MCBSP3_SLAVE") == 0) {
		omap_mux_init_signal("mcbsp3_fsx.mcbsp3_fsx",
						OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("mcbsp3_clkx.mcbsp3_clkx",
						OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("mcbsp3_dr.mcbsp3_dr",
						OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("mcbsp3_dx.mcbsp3_dx",
						OMAP_PIN_OUTPUT);
		return 0;
	} else if (strcmp(group, "OMAP_MCBSP3_MASTER") == 0) {
		omap_mux_init_signal("mcbsp_clks.mcbsp_clks",
						OMAP_PIN_INPUT);
		omap_mux_init_signal("mcbsp3_fsx.mcbsp3_fsx",
						OMAP_PIN_OUTPUT);
		omap_mux_init_signal("mcbsp3_clkx.mcbsp3_clkx",
						OMAP_PIN_OUTPUT);
		omap_mux_init_signal("mcbsp3_dr.mcbsp3_dr",
						OMAP_PIN_INPUT_PULLDOWN);
		omap_mux_init_signal("mcbsp3_dx.mcbsp3_dx",
						OMAP_PIN_OUTPUT);
		return 0;
	} else if (strcmp(group, "OMAP_MCBSP3_TRISTATE") == 0) {
		omap_mux_init_signal("mcbsp3_fsx.safe_mode",
						OMAP_PIN_INPUT);
		omap_mux_init_signal("mcbsp3_clkx.safe_mode",
						OMAP_PIN_INPUT);
		omap_mux_init_signal("mcbsp3_dr.safe_mode",
						OMAP_PIN_INPUT);
		omap_mux_init_signal("mcbsp3_dx.safe_mode",
						OMAP_PIN_INPUT);
		return 0;
	} else if (strcmp(group, "OMAP_MCBSP4_MASTER") == 0) {
		omap_mux_init_signal("mcbsp_clks.mcbsp_clks",
						OMAP_PIN_INPUT);
		omap_mux_init_signal("mcbsp4_fsx.mcbsp4_fsx",
						OMAP_PIN_OUTPUT);
		omap_mux_init_signal("gpmc_ncs4.mcbsp4_clkx",
						OMAP_PIN_OUTPUT);
		omap_mux_init_signal("gpmc_ncs5.mcbsp4_dr",
						OMAP_PIN_INPUT);
		omap_mux_init_signal("gpmc_ncs6.mcbsp4_dx",
						OMAP_PIN_OUTPUT);
		return 0;
	} else if (strcmp(group, "OMAP_MCBSP4_SLAVE") == 0) {
		omap_mux_init_signal("mcbsp4_fsx.mcbsp4_fsx",
						OMAP_PIN_INPUT);
		omap_mux_init_signal("gpmc_ncs4.mcbsp4_clkx",
						OMAP_PIN_INPUT);
		omap_mux_init_signal("gpmc_ncs5.mcbsp4_dr",
						OMAP_PIN_INPUT);
		omap_mux_init_signal("gpmc_ncs6.mcbsp4_dx",
						OMAP_PIN_OUTPUT);
		return 0;
	}
	return -ENODEV;
}

#ifdef CONFIG_DEBUG_FS

#define OMAP_MUX_MAX_NR_FLAGS	10
#define OMAP_MUX_TEST_FLAG(val, mask)				\
	if (((val) & (mask)) == (mask)) {			\
		i++;						\
		flags[i] =  #mask;				\
	}

/* REVISIT: Add checking for non-optimal mux settings */
static inline void omap_mux_decode(struct seq_file *s, u16 val)
{
	char *flags[OMAP_MUX_MAX_NR_FLAGS];
	char mode[14];
	int i = -1;

	sprintf(mode, "OMAP_MUX_MODE%d", val & 0x7);
	i++;
	flags[i] = mode;

	OMAP_MUX_TEST_FLAG(val, OMAP_PIN_OFF_WAKEUPENABLE);
	if (val & OMAP_OFF_EN) {
		if (val & OMAP_OFFOUT_EN) {
			if (!(val & OMAP_OFF_PULL_UP)) {
				OMAP_MUX_TEST_FLAG(val,
					OMAP_PIN_OFF_INPUT_PULLDOWN);
			} else {
				OMAP_MUX_TEST_FLAG(val,
					OMAP_PIN_OFF_INPUT_PULLUP);
			}
		} else {
			if (!(val & OMAP_OFFOUT_VAL)) {
				OMAP_MUX_TEST_FLAG(val,
					OMAP_PIN_OFF_OUTPUT_LOW);
			} else {
				OMAP_MUX_TEST_FLAG(val,
					OMAP_PIN_OFF_OUTPUT_HIGH);
			}
		}
	}

	if (val & OMAP_INPUT_EN) {
		if (val & OMAP_PULL_ENA) {
			if (!(val & OMAP_PULL_UP)) {
				OMAP_MUX_TEST_FLAG(val,
					OMAP_PIN_INPUT_PULLDOWN);
			} else {
				OMAP_MUX_TEST_FLAG(val, OMAP_PIN_INPUT_PULLUP);
			}
		} else {
			OMAP_MUX_TEST_FLAG(val, OMAP_PIN_INPUT);
		}
	} else {
		i++;
		flags[i] = "OMAP_PIN_OUTPUT";
	}

	do {
		seq_printf(s, "%s", flags[i]);
		if (i > 0)
			seq_printf(s, " | ");
	} while (i-- > 0);
}

#define OMAP_MUX_DEFNAME_LEN	16

static int omap_mux_dbg_board_show(struct seq_file *s, void *unused)
{
	struct omap_mux_entry *e;

	list_for_each_entry(e, &muxmodes, node) {
		struct omap_mux *m = &e->mux;
		char m0_def[OMAP_MUX_DEFNAME_LEN];
		char *m0_name = m->muxnames[0];
		u16 val;
		int i, mode;

		if (!m0_name)
			continue;

		for (i = 0; i < OMAP_MUX_DEFNAME_LEN; i++) {
			if (m0_name[i] == '\0') {
				m0_def[i] = m0_name[i];
				break;
			}
			m0_def[i] = toupper(m0_name[i]);
		}
		val = omap_mux_read(m->reg_offset);
		mode = val & OMAP_MUX_MODE7;

		seq_printf(s, "OMAP%i_MUX(%s, ",
					cpu_is_omap34xx() ? 3 : 0, m0_def);
		omap_mux_decode(s, val);
		seq_printf(s, "),\n");
	}

	return 0;
}

static int omap_mux_dbg_board_open(struct inode *inode, struct file *file)
{
	return single_open(file, omap_mux_dbg_board_show, &inode->i_private);
}

static const struct file_operations omap_mux_dbg_board_fops = {
	.open		= omap_mux_dbg_board_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int omap_mux_dbg_signal_show(struct seq_file *s, void *unused)
{
	struct omap_mux *m = s->private;
	const char *none = "NA";
	u16 val;
	int mode;

	val = omap_mux_read(m->reg_offset);
	mode = val & OMAP_MUX_MODE7;

	seq_printf(s, "name: %s.%s (0x%08lx/0x%03x = 0x%04x), b %s, t %s\n",
			m->muxnames[0], m->muxnames[mode],
			mux_phys + m->reg_offset, m->reg_offset, val,
			m->balls[0] ? m->balls[0] : none,
			m->balls[1] ? m->balls[1] : none);
	seq_printf(s, "mode: ");
	omap_mux_decode(s, val);
	seq_printf(s, "\n");
	seq_printf(s, "signals: %s | %s | %s | %s | %s | %s | %s | %s\n",
			m->muxnames[0] ? m->muxnames[0] : none,
			m->muxnames[1] ? m->muxnames[1] : none,
			m->muxnames[2] ? m->muxnames[2] : none,
			m->muxnames[3] ? m->muxnames[3] : none,
			m->muxnames[4] ? m->muxnames[4] : none,
			m->muxnames[5] ? m->muxnames[5] : none,
			m->muxnames[6] ? m->muxnames[6] : none,
			m->muxnames[7] ? m->muxnames[7] : none);

	return 0;
}

#define OMAP_MUX_MAX_ARG_CHAR  7

static ssize_t omap_mux_dbg_signal_write(struct file *file,
						const char __user *user_buf,
						size_t count, loff_t *ppos)
{
	char buf[OMAP_MUX_MAX_ARG_CHAR];
	struct seq_file *seqf;
	struct omap_mux *m;
	unsigned long val;
	int buf_size, ret;

	if (count > OMAP_MUX_MAX_ARG_CHAR)
		return -EINVAL;

	memset(buf, 0, sizeof(buf));
	buf_size = min(count, sizeof(buf) - 1);

	if (copy_from_user(buf, user_buf, buf_size))
		return -EFAULT;

	ret = strict_strtoul(buf, 0x10, &val);
	if (ret < 0)
		return ret;

	if (val > 0xffff)
		return -EINVAL;

	seqf = file->private_data;
	m = seqf->private;

	omap_mux_write((u16)val, m->reg_offset);
	*ppos += count;

	return count;
}

static int omap_mux_dbg_signal_open(struct inode *inode, struct file *file)
{
	return single_open(file, omap_mux_dbg_signal_show, inode->i_private);
}

static const struct file_operations omap_mux_dbg_signal_fops = {
	.open		= omap_mux_dbg_signal_open,
	.read		= seq_read,
	.write		= omap_mux_dbg_signal_write,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static struct dentry *mux_dbg_dir;

static void __init omap_mux_dbg_init(void)
{
	struct omap_mux_entry *e;

	mux_dbg_dir = debugfs_create_dir("omap_mux", NULL);
	if (!mux_dbg_dir)
		return;

	(void)debugfs_create_file("board", S_IRUGO, mux_dbg_dir,
					NULL, &omap_mux_dbg_board_fops);

	list_for_each_entry(e, &muxmodes, node) {
		struct omap_mux *m = &e->mux;

		(void)debugfs_create_file(m->muxnames[0], S_IWUGO, mux_dbg_dir,
					m, &omap_mux_dbg_signal_fops);
	}
}

#else
static inline void omap_mux_dbg_init(void)
{
}
#endif	/* CONFIG_DEBUG_FS */

static void __init omap_mux_free_names(struct omap_mux *m)
{
	int i;

	for (i = 0; i < OMAP_MUX_NR_MODES; i++)
		kfree(m->muxnames[i]);

#ifdef CONFIG_DEBUG_FS
	for (i = 0; i < OMAP_MUX_NR_SIDES; i++)
		kfree(m->balls[i]);
#endif

}

#ifdef CONFIG_OMAP3_PADCONF_DUMP
static const u16 padcfg_1_offs[] = {
	/* Offset: 0x0 */
	OMAP3_CONTROL_PADCONF_SDRC_D0_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_D1_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_D2_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_D3_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_D4_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_D5_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_D6_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_D7_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_D8_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_D9_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_D10_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_D11_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_D12_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_D13_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_D14_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_D15_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_D16_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_D17_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_D18_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_D19_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_D20_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_D21_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_D22_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_D23_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_D24_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_D25_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_D26_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_D27_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_D28_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_D29_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_D30_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_D31_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_CLK_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_DQS0_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_DQS1_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_DQS2_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_DQS3_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_A1_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_A2_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_A3_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_A4_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_A5_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_A6_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_A7_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_A8_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_A9_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_A10_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_D0_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_D1_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_D2_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_D3_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_D4_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_D5_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_D6_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_D7_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_D8_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_D9_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_D10_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_D11_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_D12_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_D13_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_D14_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_D15_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_NCS0_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_NCS1_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_NCS2_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_NCS3_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_NCS4_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_NCS5_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_NCS6_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_NCS7_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_CLK_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_NADV_ALE_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_NOE_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_NWE_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_NBE0_CLE_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_NBE1_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_NWP_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_WAIT0_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_WAIT1_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_WAIT2_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_WAIT3_OFFSET,
	OMAP3_CONTROL_PADCONF_DSS_PCLK_OFFSET,
	OMAP3_CONTROL_PADCONF_DSS_HSYNC_OFFSET,
	OMAP3_CONTROL_PADCONF_DSS_VSYNC_OFFSET,
	OMAP3_CONTROL_PADCONF_DSS_ACBIAS_OFFSET,
	OMAP3_CONTROL_PADCONF_DSS_DATA0_OFFSET,
	OMAP3_CONTROL_PADCONF_DSS_DATA1_OFFSET,
	OMAP3_CONTROL_PADCONF_DSS_DATA2_OFFSET,
	OMAP3_CONTROL_PADCONF_DSS_DATA3_OFFSET,
	OMAP3_CONTROL_PADCONF_DSS_DATA4_OFFSET,
	OMAP3_CONTROL_PADCONF_DSS_DATA5_OFFSET,
	OMAP3_CONTROL_PADCONF_DSS_DATA6_OFFSET,
	OMAP3_CONTROL_PADCONF_DSS_DATA7_OFFSET,
	OMAP3_CONTROL_PADCONF_DSS_DATA8_OFFSET,
	OMAP3_CONTROL_PADCONF_DSS_DATA9_OFFSET,
	OMAP3_CONTROL_PADCONF_DSS_DATA10_OFFSET,
	OMAP3_CONTROL_PADCONF_DSS_DATA11_OFFSET,
	OMAP3_CONTROL_PADCONF_DSS_DATA12_OFFSET,
	OMAP3_CONTROL_PADCONF_DSS_DATA13_OFFSET,
	OMAP3_CONTROL_PADCONF_DSS_DATA14_OFFSET,
	OMAP3_CONTROL_PADCONF_DSS_DATA15_OFFSET,
	OMAP3_CONTROL_PADCONF_DSS_DATA16_OFFSET,
	OMAP3_CONTROL_PADCONF_DSS_DATA17_OFFSET,
	OMAP3_CONTROL_PADCONF_DSS_DATA18_OFFSET,
	OMAP3_CONTROL_PADCONF_DSS_DATA19_OFFSET,
	OMAP3_CONTROL_PADCONF_DSS_DATA20_OFFSET,
	OMAP3_CONTROL_PADCONF_DSS_DATA21_OFFSET,
	OMAP3_CONTROL_PADCONF_DSS_DATA22_OFFSET,
	OMAP3_CONTROL_PADCONF_DSS_DATA23_OFFSET,
	OMAP3_CONTROL_PADCONF_CAM_HS_OFFSET,
	OMAP3_CONTROL_PADCONF_CAM_VS_OFFSET,
	OMAP3_CONTROL_PADCONF_CAM_XCLKA_OFFSET,
	OMAP3_CONTROL_PADCONF_CAM_PCLK_OFFSET,
	OMAP3_CONTROL_PADCONF_CAM_FLD_OFFSET,
	OMAP3_CONTROL_PADCONF_CAM_D0_OFFSET,
	OMAP3_CONTROL_PADCONF_CAM_D1_OFFSET,
	OMAP3_CONTROL_PADCONF_CAM_D2_OFFSET,
	OMAP3_CONTROL_PADCONF_CAM_D3_OFFSET,
	OMAP3_CONTROL_PADCONF_CAM_D4_OFFSET,
	OMAP3_CONTROL_PADCONF_CAM_D5_OFFSET,
	OMAP3_CONTROL_PADCONF_CAM_D6_OFFSET,
	OMAP3_CONTROL_PADCONF_CAM_D7_OFFSET,
	OMAP3_CONTROL_PADCONF_CAM_D8_OFFSET,
	OMAP3_CONTROL_PADCONF_CAM_D9_OFFSET,
	OMAP3_CONTROL_PADCONF_CAM_D10_OFFSET,
	OMAP3_CONTROL_PADCONF_CAM_D11_OFFSET,
	OMAP3_CONTROL_PADCONF_CAM_XCLKB_OFFSET,
	OMAP3_CONTROL_PADCONF_CAM_WEN_OFFSET,
	OMAP3_CONTROL_PADCONF_CAM_STROBE_OFFSET,
	OMAP3_CONTROL_PADCONF_CSI2_DX0_OFFSET,
	OMAP3_CONTROL_PADCONF_CSI2_DY0_OFFSET,
	OMAP3_CONTROL_PADCONF_CSI2_DX1_OFFSET,
	OMAP3_CONTROL_PADCONF_CSI2_DY1_OFFSET,
	OMAP3_CONTROL_PADCONF_MCBSP2_FSX_OFFSET,
	OMAP3_CONTROL_PADCONF_MCBSP2_CLKX_OFFSET,
	OMAP3_CONTROL_PADCONF_MCBSP2_DR_OFFSET,
	OMAP3_CONTROL_PADCONF_MCBSP2_DX_OFFSET,
	OMAP3_CONTROL_PADCONF_SDMMC1_CLK_OFFSET,
	OMAP3_CONTROL_PADCONF_SDMMC1_CMD_OFFSET,
	OMAP3_CONTROL_PADCONF_SDMMC1_DAT0_OFFSET,
	OMAP3_CONTROL_PADCONF_SDMMC1_DAT1_OFFSET,
	OMAP3_CONTROL_PADCONF_SDMMC1_DAT2_OFFSET,
	OMAP3_CONTROL_PADCONF_SDMMC1_DAT3_OFFSET,
	OMAP3_CONTROL_PADCONF_SDMMC1_DAT4_OFFSET,
	OMAP3_CONTROL_PADCONF_SDMMC1_DAT5_OFFSET,
	OMAP3_CONTROL_PADCONF_SDMMC1_DAT6_OFFSET,
	OMAP3_CONTROL_PADCONF_SDMMC1_DAT7_OFFSET,
	OMAP3_CONTROL_PADCONF_SDMMC2_CLK_OFFSET,
	OMAP3_CONTROL_PADCONF_SDMMC2_CMD_OFFSET,
	OMAP3_CONTROL_PADCONF_SDMMC2_DAT0_OFFSET,
	OMAP3_CONTROL_PADCONF_SDMMC2_DAT1_OFFSET,
	OMAP3_CONTROL_PADCONF_SDMMC2_DAT2_OFFSET,
	OMAP3_CONTROL_PADCONF_SDMMC2_DAT3_OFFSET,
	OMAP3_CONTROL_PADCONF_SDMMC2_DAT4_OFFSET,
	OMAP3_CONTROL_PADCONF_SDMMC2_DAT5_OFFSET,
	OMAP3_CONTROL_PADCONF_SDMMC2_DAT6_OFFSET,
	OMAP3_CONTROL_PADCONF_SDMMC2_DAT7_OFFSET,
	OMAP3_CONTROL_PADCONF_MCBSP3_DX_OFFSET,
	OMAP3_CONTROL_PADCONF_MCBSP3_DR_OFFSET,
	OMAP3_CONTROL_PADCONF_MCBSP3_CLKX_OFFSET,
	OMAP3_CONTROL_PADCONF_MCBSP3_FSX_OFFSET,
	OMAP3_CONTROL_PADCONF_UART2_CTS_OFFSET,
	OMAP3_CONTROL_PADCONF_UART2_RTS_OFFSET,
	OMAP3_CONTROL_PADCONF_UART2_TX_OFFSET,
	OMAP3_CONTROL_PADCONF_UART2_RX_OFFSET,
	OMAP3_CONTROL_PADCONF_UART1_TX_OFFSET,
	OMAP3_CONTROL_PADCONF_UART1_RTS_OFFSET,
	OMAP3_CONTROL_PADCONF_UART1_CTS_OFFSET,
	OMAP3_CONTROL_PADCONF_UART1_RX_OFFSET,
	OMAP3_CONTROL_PADCONF_MCBSP4_CLKX_OFFSET,
	OMAP3_CONTROL_PADCONF_MCBSP4_DR_OFFSET,
	OMAP3_CONTROL_PADCONF_MCBSP4_DX_OFFSET,
	OMAP3_CONTROL_PADCONF_MCBSP4_FSX_OFFSET,
	OMAP3_CONTROL_PADCONF_MCBSP1_CLKR_OFFSET,
	OMAP3_CONTROL_PADCONF_MCBSP1_FSR_OFFSET,
	OMAP3_CONTROL_PADCONF_MCBSP1_DX_OFFSET,
	OMAP3_CONTROL_PADCONF_MCBSP1_DR_OFFSET,
	OMAP3_CONTROL_PADCONF_MCBSP_CLKS_OFFSET,
	OMAP3_CONTROL_PADCONF_MCBSP1_FSX_OFFSET,
	OMAP3_CONTROL_PADCONF_MCBSP1_CLKX_OFFSET,
	OMAP3_CONTROL_PADCONF_UART3_CTS_RCTX_OFFSET,
	OMAP3_CONTROL_PADCONF_UART3_RTS_SD_OFFSET,
	OMAP3_CONTROL_PADCONF_UART3_RX_IRRX_OFFSET,
	OMAP3_CONTROL_PADCONF_UART3_TX_IRTX_OFFSET,
	OMAP3_CONTROL_PADCONF_HSUSB0_CLK_OFFSET,
	OMAP3_CONTROL_PADCONF_HSUSB0_STP_OFFSET,
	OMAP3_CONTROL_PADCONF_HSUSB0_DIR_OFFSET,
	OMAP3_CONTROL_PADCONF_HSUSB0_NXT_OFFSET,
	OMAP3_CONTROL_PADCONF_HSUSB0_DATA0_OFFSET,
	OMAP3_CONTROL_PADCONF_HSUSB0_DATA1_OFFSET,
	OMAP3_CONTROL_PADCONF_HSUSB0_DATA2_OFFSET,
	OMAP3_CONTROL_PADCONF_HSUSB0_DATA3_OFFSET,
	OMAP3_CONTROL_PADCONF_HSUSB0_DATA4_OFFSET,
	OMAP3_CONTROL_PADCONF_HSUSB0_DATA5_OFFSET,
	OMAP3_CONTROL_PADCONF_HSUSB0_DATA6_OFFSET,
	OMAP3_CONTROL_PADCONF_HSUSB0_DATA7_OFFSET,
	OMAP3_CONTROL_PADCONF_I2C1_SCL_OFFSET,
	OMAP3_CONTROL_PADCONF_I2C1_SDA_OFFSET,
	OMAP3_CONTROL_PADCONF_I2C2_SCL_OFFSET,
	OMAP3_CONTROL_PADCONF_I2C2_SDA_OFFSET,
	OMAP3_CONTROL_PADCONF_I2C3_SCL_OFFSET,
	OMAP3_CONTROL_PADCONF_I2C3_SDA_OFFSET,
	OMAP3_CONTROL_PADCONF_HDQ_SIO_OFFSET,
	OMAP3_CONTROL_PADCONF_MCSPI1_CLK_OFFSET,
	OMAP3_CONTROL_PADCONF_MCSPI1_SIMO_OFFSET,
	OMAP3_CONTROL_PADCONF_MCSPI1_SOMI_OFFSET,
	OMAP3_CONTROL_PADCONF_MCSPI1_CS0_OFFSET,
	OMAP3_CONTROL_PADCONF_MCSPI1_CS1_OFFSET,
	OMAP3_CONTROL_PADCONF_MCSPI1_CS2_OFFSET,
	OMAP3_CONTROL_PADCONF_MCSPI1_CS3_OFFSET,
	OMAP3_CONTROL_PADCONF_MCSPI2_CLK_OFFSET,
	OMAP3_CONTROL_PADCONF_MCSPI2_SIMO_OFFSET,
	OMAP3_CONTROL_PADCONF_MCSPI2_SOMI_OFFSET,
	OMAP3_CONTROL_PADCONF_MCSPI2_CS0_OFFSET,
	OMAP3_CONTROL_PADCONF_MCSPI2_CS1_OFFSET,
	OMAP3_CONTROL_PADCONF_SYS_NIRQ_OFFSET,
	OMAP3_CONTROL_PADCONF_SYS_CLKOUT2_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_MCAD0_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_MCAD1_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_MCAD2_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_MCAD3_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_MCAD4_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_MCAD5_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_MCAD6_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_MCAD7_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_MCAD8_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_MCAD9_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_MCAD10_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_MCAD11_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_MCAD12_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_MCAD13_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_MCAD14_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_MCAD15_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_MCAD16_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_MCAD17_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_MCAD18_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_MCAD19_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_MCAD20_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_MCAD21_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_MCAD22_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_MCAD23_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_MCAD24_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_MCAD25_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_MCAD26_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_MCAD27_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_MCAD28_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_MCAD29_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_MCAD30_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_MCAD31_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_MCAD32_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_MCAD33_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_MCAD34_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_MCAD35_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_MCAD36_OFFSET,
	OMAP3_CONTROL_PADCONF_CHASSIS_CLK26MI_OFFSET,
	OMAP3_CONTROL_PADCONF_CHASSIS_NRESPWRON_OFFSET,
	OMAP3_CONTROL_PADCONF_CHASSIS_NRESWARW_OFFSET,
	OMAP3_CONTROL_PADCONF_CHASSIS_NIRQ_OFFSET,
	OMAP3_CONTROL_PADCONF_CHASSIS_FIQ_OFFSET,
	OMAP3_CONTROL_PADCONF_CHASSIS_ARMIRQ_OFFSET,
	OMAP3_CONTROL_PADCONF_CHASSIS_IVAIRQ_OFFSET,
	OMAP3_CONTROL_PADCONF_CHASSIS_DMAREQ0_OFFSET,
	OMAP3_CONTROL_PADCONF_CHASSIS_DMAREQ1_OFFSET,
	OMAP3_CONTROL_PADCONF_CHASSIS_DMAREQ2_OFFSET,
	OMAP3_CONTROL_PADCONF_CHASSIS_DMAREQ3_OFFSET,
	OMAP3_CONTROL_PADCONF_CHASSIS_NTRST_OFFSET,
	OMAP3_CONTROL_PADCONF_CHASSIS_TDI_OFFSET,
	OMAP3_CONTROL_PADCONF_CHASSIS_TDO_OFFSET,
	OMAP3_CONTROL_PADCONF_CHASSIS_TMS_OFFSET,
	OMAP3_CONTROL_PADCONF_CHASSIS_TCK_OFFSET,
	OMAP3_CONTROL_PADCONF_CHASSIS_RTCK_OFFSET,
	OMAP3_CONTROL_PADCONF_CHASSIS_MSTDBY_OFFSET,
	OMAP3_CONTROL_PADCONF_CHASSIS_IDLEREQ_OFFSET,
	OMAP3_CONTROL_PADCONF_CHASSIS_IDLEACK_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_MWRITE_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_SWRITE_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_MREAD_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_SREAD_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_MBUSFLAG_OFFSET,
	OMAP3_CONTROL_PADCONF_SAD2D_SBUSFLAG_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_CKE0_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_CKE1_OFFSET,
	OMAP3_CONTROL_PADCONF_GPMC_A11_OFFSET,
};

static const u16 padcfg_2_offs[] = {
	/* Offset jump: 0x570 */
	OMAP3_CONTROL_PADCONF_SDRC_BA0_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_BA1_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_A0_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_A1_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_A2_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_A3_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_A4_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_A5_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_A6_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_A7_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_A8_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_A9_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_A10_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_A11_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_A12_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_A13_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_A14_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_NCS0_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_NCS1_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_NCLK_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_NRAS_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_NCAS_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_NWE_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_DM0_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_DM1_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_DM2_OFFSET,
	OMAP3_CONTROL_PADCONF_SDRC_DM3_OFFSET,
	OMAP3_CONTROL_PADCONF_ETK_CLK_OFFSET,
	OMAP3_CONTROL_PADCONF_ETK_CTL_OFFSET,
	OMAP3_CONTROL_PADCONF_ETK_D0_OFFSET,
	OMAP3_CONTROL_PADCONF_ETK_D1_OFFSET,
	OMAP3_CONTROL_PADCONF_ETK_D2_OFFSET,
	OMAP3_CONTROL_PADCONF_ETK_D3_OFFSET,
	OMAP3_CONTROL_PADCONF_ETK_D4_OFFSET,
	OMAP3_CONTROL_PADCONF_ETK_D5_OFFSET,
	OMAP3_CONTROL_PADCONF_ETK_D6_OFFSET,
	OMAP3_CONTROL_PADCONF_ETK_D7_OFFSET,
	OMAP3_CONTROL_PADCONF_ETK_D8_OFFSET,
	OMAP3_CONTROL_PADCONF_ETK_D9_OFFSET,
	OMAP3_CONTROL_PADCONF_ETK_D10_OFFSET,
	OMAP3_CONTROL_PADCONF_ETK_D11_OFFSET,
	OMAP3_CONTROL_PADCONF_ETK_D12_OFFSET,
	OMAP3_CONTROL_PADCONF_ETK_D13_OFFSET,
	OMAP3_CONTROL_PADCONF_ETK_D14_OFFSET,
	OMAP3_CONTROL_PADCONF_ETK_D15_OFFSET,
};

static const u16 padcfg_3_offs[] = {
	/* Offset jump: 0x9d0 */
	OMAP3_CONTROL_PADCONF_I2C4_SCL_OFFSET,
	OMAP3_CONTROL_PADCONF_I2C4_SDA_OFFSET,
	OMAP3_CONTROL_PADCONF_SYS_32K_OFFSET,
	OMAP3_CONTROL_PADCONF_SYS_CLKREQ_OFFSET,
	OMAP3_CONTROL_PADCONF_SYS_NRESWARM_OFFSET,
	OMAP3_CONTROL_PADCONF_SYS_BOOT0_OFFSET,
	OMAP3_CONTROL_PADCONF_SYS_BOOT1_OFFSET,
	OMAP3_CONTROL_PADCONF_SYS_BOOT2_OFFSET,
	OMAP3_CONTROL_PADCONF_SYS_BOOT3_OFFSET,
	OMAP3_CONTROL_PADCONF_SYS_BOOT4_OFFSET,
	OMAP3_CONTROL_PADCONF_SYS_BOOT5_OFFSET,
	OMAP3_CONTROL_PADCONF_SYS_BOOT6_OFFSET,
	OMAP3_CONTROL_PADCONF_SYS_OFF_MODE_OFFSET,
	OMAP3_CONTROL_PADCONF_SYS_CLKOUT1_OFFSET,
	OMAP3_CONTROL_PADCONF_JTAG_NTRST_OFFSET,
	OMAP3_CONTROL_PADCONF_JTAG_TCK_OFFSET,
	OMAP3_CONTROL_PADCONF_JTAG_TMS_TMSC_OFFSET,
	OMAP3_CONTROL_PADCONF_JTAG_TDI_OFFSET,
	OMAP3_CONTROL_PADCONF_JTAG_EMU0_OFFSET,
	OMAP3_CONTROL_PADCONF_JTAG_EMU1_OFFSET,
	/* Offset jump: 0x9f6 - 0xa1c */
	OMAP3_CONTROL_PADCONF_SAD2D_SWAKEUP_OFFSET,
	OMAP3_CONTROL_PADCONF_JTAG_RTCK_OFFSET,
	OMAP3_CONTROL_PADCONF_JTAG_TDO_OFFSET,
	OMAP3_CONTROL_PADCONF_GPIO127_OFFSET ,
	OMAP3_CONTROL_PADCONF_GPIO126_OFFSET ,
	OMAP3_CONTROL_PADCONF_GPIO128_OFFSET ,
	OMAP3_CONTROL_PADCONF_GPIO129_OFFSET ,
};

static const u16 gpio_offs[] = {
	OMAP24XX_GPIO_OE,
	OMAP24XX_GPIO_DATAIN,
	OMAP24XX_GPIO_DATAOUT,
};


#define PADCFG_1_SIZE (1 + (OMAP3_CONTROL_PADCONF_GPMC_A11_OFFSET - OMAP3_CONTROL_PADCONF_SDRC_D0_OFFSET) / sizeof(u16))
#define PADCFG_2_SIZE (1 + (OMAP3_CONTROL_PADCONF_ETK_D15_OFFSET - OMAP3_CONTROL_PADCONF_SDRC_BA0_OFFSET) / sizeof(u16))
#define PADCFG_3_SIZE (1 + (OMAP3_CONTROL_PADCONF_GPIO129_OFFSET - OMAP3_CONTROL_PADCONF_I2C4_SCL_OFFSET) / sizeof(u16))
#define DUMP_GPIO_SIZE (3) /* GPIO_OE, GPIO_DATAIN, GPIO_DATAOUT */

/* Size is in _elements_ */
static unsigned int pads_1[PADCFG_1_SIZE];
static unsigned int pads_2[PADCFG_2_SIZE];
static unsigned int pads_3[PADCFG_3_SIZE];
static unsigned int gpio_dump[6 * DUMP_GPIO_SIZE];

/* Note that the order of these entries is hard-coded in omap_padconfig_update_base_vaddr()
 * and omap_padconfig_dump_table(), so don't change this without also changing those functions.
 */
struct omap_mux_dumpregs omap_mux_dump_table[] = {
/*	         base_offset,  reg_size  ,   array size  ,                  array         , offset array ,      offset array size */
	{0,            0x000, sizeof(u16),  PADCFG_1_SIZE,                          pads_1, padcfg_1_offs, sizeof(padcfg_1_offs) / sizeof(padcfg_1_offs[0])},
	{0,            0x570, sizeof(u16),  PADCFG_2_SIZE,                          pads_2, padcfg_2_offs, sizeof(padcfg_2_offs) / sizeof(padcfg_2_offs[0])},
	{0,            0x9d0, sizeof(u16),  PADCFG_3_SIZE,                          pads_3, padcfg_3_offs, sizeof(padcfg_3_offs) / sizeof(padcfg_3_offs[0])},
	{0, OMAP24XX_GPIO_OE, sizeof(u32), DUMP_GPIO_SIZE,  &gpio_dump[0 * DUMP_GPIO_SIZE],     gpio_offs, sizeof(gpio_offs) / sizeof(gpio_offs[0])},
	{0, OMAP24XX_GPIO_OE, sizeof(u32), DUMP_GPIO_SIZE,  &gpio_dump[1 * DUMP_GPIO_SIZE],     gpio_offs, sizeof(gpio_offs) / sizeof(gpio_offs[0])},
	{0, OMAP24XX_GPIO_OE, sizeof(u32), DUMP_GPIO_SIZE,  &gpio_dump[2 * DUMP_GPIO_SIZE],     gpio_offs, sizeof(gpio_offs) / sizeof(gpio_offs[0])},
	{0, OMAP24XX_GPIO_OE, sizeof(u32), DUMP_GPIO_SIZE,  &gpio_dump[3 * DUMP_GPIO_SIZE],     gpio_offs, sizeof(gpio_offs) / sizeof(gpio_offs[0])},
	{0, OMAP24XX_GPIO_OE, sizeof(u32), DUMP_GPIO_SIZE,  &gpio_dump[4 * DUMP_GPIO_SIZE],     gpio_offs, sizeof(gpio_offs) / sizeof(gpio_offs[0])},
	{0, OMAP24XX_GPIO_OE, sizeof(u32), DUMP_GPIO_SIZE,  &gpio_dump[5 * DUMP_GPIO_SIZE],     gpio_offs, sizeof(gpio_offs) / sizeof(gpio_offs[0])},
	{0,                0,           0,              0,                               0,             0,             0},
};

int padconf_dump_on_suspend_enabled = 0;

static void omap_mux_dbg_print_padconfig(const struct omap_mux_dumpregs* row)
{
	int i;
	int padcfg_offs_index = 0;
	char buf[80];
	int buf_index = 0;
	bool buf_dirty = false;
	u16 *regs_ptr = (u16*)row->dump;

	for (i = 0; ; ++i) {
		const int reg_offs = (i * sizeof(u16)) + row->base_offset;
		u16 current_padcfg_offs = -1;
		u16 reg_val = 0xdeaf;

		if (padcfg_offs_index < row->padcfg_offs_size) {
			current_padcfg_offs = row->padcfg_offs[padcfg_offs_index];
		} else {
			/* Done, dump the remainder of the buffer if needed */
			if (i != 0) {
				printk(KERN_INFO "%s\n", buf);
				return;
			}
		}

		/* Don't print if we're not in the right range for the next padcfg_offs */
		if ((reg_offs & 0xfff0) == (current_padcfg_offs & 0xfff0)) {
			/* Start a new line with the offset every 16 bytes */
			if ((i % 8) == 0) {
				buf_index = snprintf(buf, sizeof buf, "0x%03x ", reg_offs);
			}

			/* If this offset is in the array, put the value in reg_val */
			if (reg_offs == current_padcfg_offs) {
				reg_val = regs_ptr[i];
				padcfg_offs_index++;
				buf_dirty = true;
			}

			/* Print formatted value to buffer */
			buf_index += snprintf(buf + buf_index, (sizeof buf) - buf_index, " 0x%04x", reg_val);
		}
		/* Print the buffer to kernel log if needed */
		if ((i % 8 == 7) && buf_dirty) {
			printk(KERN_INFO "%s\n", buf);
			buf_dirty = false;
		}
	}
}

static void omap_mux_dbg_print_gpio(const struct omap_mux_dumpregs* row, int nr) {
	printk(KERN_INFO "GPIO%d_OE,%x\n", nr, row->dump[0]);
	printk(KERN_INFO "GPIO%d_DATAIN,%x\n", nr, row->dump[1]);
	printk(KERN_INFO "GPIO%d_DATAOUT,%x\n", nr, row->dump[2]);
}

void omap_padconfig_dump_table(const char *devicestate)
{
	int i;

	printk(KERN_INFO "Padconfig dump start -------\n");
	printk(KERN_INFO "Device State: %s\n", devicestate);

	for (i = 0; omap_mux_dump_table[i].dump_size != 0; i++) {
		if (i < 3) {
			omap_mux_dbg_print_padconfig(&omap_mux_dump_table[i]);
		} else {
			omap_mux_dbg_print_gpio(&omap_mux_dump_table[i], i - 2);
		}

		/* Zero out the tables after printing */
		memset(omap_mux_dump_table[i].dump, 0, omap_mux_dump_table[i].dump_size * sizeof(u16));
	}
	printk(KERN_INFO "Padconfig dump finish -------\n");
}

static void omap_padconfig_read(struct omap_mux_dumpregs* row)
{
	int i, i_offset;
	u16 *regs_ptr = (u16*) row->dump;

	for (i = 0, i_offset = 0;
			i < row->dump_size
			&& i_offset < row->padcfg_offs_size;
			i++) {
		u16 offset_addr =  row->base_offset + (i * row->reg_size);

		/* If we're interested in this offset, read it out. Otherwise zero it out. */
		if (offset_addr == row->padcfg_offs[i_offset]) {
			/* 16 bit values are mux registers, 32 bit values are other registers */
			if (row->reg_size == sizeof(u16))
				regs_ptr[i] = omap_mux_read(offset_addr);
			else
				row->dump[i] = readl(row->base_vaddr + offset_addr);

			++i_offset;
		} else {
			if (row->reg_size == sizeof(u16))
				regs_ptr[i] = 0;
			else
				row->dump[i] = 0;
		}
	}
}

void omap_padconfig_read_table()
{
	int i;
	for (i = 0; omap_mux_dump_table[i].dump_size != 0; i++) {
		omap_padconfig_read(&omap_mux_dump_table[i]);
	}
}

static void omap_padconfig_update_base_vaddr(unsigned long base_vaddr)
{
	int i;
	const long gpio_bases[] = {
		OMAP34XX_GPIO1_BASE,
		OMAP34XX_GPIO2_BASE,
		OMAP34XX_GPIO3_BASE,
		OMAP34XX_GPIO4_BASE,
		OMAP34XX_GPIO5_BASE,
		OMAP34XX_GPIO6_BASE
	};

	printk(KERN_DEBUG "omap_padconfig_update_base_vaddr: 0x%lx\n", base_vaddr);
	for (i = 0; i < 3; i++) {
		omap_mux_dump_table[i].base_vaddr = base_vaddr;
	}

	for (i = 0; i < (sizeof gpio_bases / sizeof gpio_bases[0]); ++i) {
		void *addr = ioremap(gpio_bases[i], PAGE_SIZE);

		if (!addr) {
			printk(KERN_ERR "mux: Could not ioremap 0x%lx\n", gpio_bases[i]);
			return;
		} else {
			printk(KERN_DEBUG "mux: Remapped 0x%lx to 0x%p\n", gpio_bases[i], addr);
		}
		omap_mux_dump_table[i + 3].base_vaddr = (unsigned long) addr;
	}

}

static int padconf_dump_proc_read(char *page, char **start, off_t off,
			  int count, int *eof, void *data)
{
	printk(KERN_INFO "mux: Triggering padconf dump\n");
	/* Trigger a padconf dump */
	omap_padconfig_read_table();
	omap_padconfig_dump_table("Active");

	*eof = 1;
	return 0;
}

static int padconf_dump_suspend_proc_write(struct file *file, const char __user *buffer,
			   unsigned long count, void *data)
{
	char kbuf[8];

	if (count > 2)
		return -EINVAL;
	if (copy_from_user(kbuf, buffer, count))
		return -EFAULT;

	if (kbuf[0] == '0') {
		padconf_dump_on_suspend_enabled = 0;
		printk(KERN_INFO "mux: Padconf will not be dumped on suspend/resume.\n");
	} else {
		padconf_dump_on_suspend_enabled = 1;
		printk(KERN_INFO "mux: Padconf will be dumped on suspend/resume.\n");
	}

	return count;
}

static int padconf_dump_suspend_proc_read(char *page, char **start, off_t off,
			  int count, int *eof, void *data)
{
	*eof = 1;
	return sprintf(page, "%d\n", padconf_dump_on_suspend_enabled);
}


#endif /* CONFIG_OMAP3_PADCONF_DUMP */

/* Free all data except for GPIO pins unless CONFIG_DEBUG_FS is set */
static int __init omap_mux_late_init(void)
{
	struct omap_mux_entry *e, *tmp;
#ifdef CONFIG_OMAP3_PADCONF_DUMP
	struct proc_dir_entry *entry;
#endif

	list_for_each_entry_safe(e, tmp, &muxmodes, node) {
		struct omap_mux *m = &e->mux;
		u16 mode = omap_mux_read(m->reg_offset);

		if (OMAP_MODE_GPIO(mode))
			continue;
#if !defined(CONFIG_DEBUG_FS) && !(defined(CONFIG_MACH_STRASBOURG) || defined(CONFIG_MACH_STRASBOURG_XENIA))
		mutex_lock(&muxmode_mutex);
		list_del(&e->node);
		mutex_unlock(&muxmode_mutex);
		omap_mux_free_names(m);
		kfree(m);
#endif

	}

	omap_mux_dbg_init();

#ifdef CONFIG_OMAP3_PADCONF_DUMP
	/* Create procfs file to trigger a padconf dump at runtime */
	entry = create_proc_entry("dump_padconf", S_IFREG | S_IWUSR, NULL);
	if (!entry) {
		printk(KERN_ERR "mux: failed to create padconf proc entry\n");
	} else {
		entry->read_proc = padconf_dump_proc_read;
	}

	entry = create_proc_entry("dump_padconf_on_suspend", S_IFREG | S_IWUSR | S_IRUSR, NULL);
	if (!entry) {
		printk(KERN_ERR "mux: failed to create padconf proc entry\n");
	} else {
		entry->read_proc = padconf_dump_suspend_proc_read;
		entry->write_proc = padconf_dump_suspend_proc_write;
	}
#endif /* CONFIG_OMAP3_PADCONF_DUMP */

	return 0;
}
late_initcall(omap_mux_late_init);

static void __init omap_mux_package_fixup(struct omap_mux *p,
					struct omap_mux *superset)
{
	while (p->reg_offset !=  OMAP_MUX_TERMINATOR) {
		struct omap_mux *s = superset;
		int found = 0;

		while (s->reg_offset != OMAP_MUX_TERMINATOR) {
			if (s->reg_offset == p->reg_offset) {
				*s = *p;
				found++;
				break;
			}
			s++;
		}
		if (!found)
			printk(KERN_ERR "mux: Unknown entry offset 0x%x\n",
					p->reg_offset);
		p++;
	}
}

#ifdef CONFIG_DEBUG_FS

static void __init omap_mux_package_init_balls(struct omap_ball *b,
				struct omap_mux *superset)
{
	while (b->reg_offset != OMAP_MUX_TERMINATOR) {
		struct omap_mux *s = superset;
		int found = 0;

		while (s->reg_offset != OMAP_MUX_TERMINATOR) {
			if (s->reg_offset == b->reg_offset) {
				s->balls[0] = b->balls[0];
				s->balls[1] = b->balls[1];
				found++;
				break;
			}
			s++;
		}
		if (!found)
			printk(KERN_ERR "mux: Unknown ball offset 0x%x\n",
					b->reg_offset);
		b++;
	}
}

#else	/* CONFIG_DEBUG_FS */

static inline void omap_mux_package_init_balls(struct omap_ball *b,
					struct omap_mux *superset)
{
}

#endif	/* CONFIG_DEBUG_FS */

static int __init omap_mux_setup(char *options)
{
	if (!options)
		return 0;

	omap_mux_options = options;

	return 1;
}
__setup("omap_mux=", omap_mux_setup);

/*
 * Note that the omap_mux=some.signal1=0x1234,some.signal2=0x1234
 * cmdline options only override the bootloader values.
 * During development, please enable CONFIG_DEBUG_FS, and use the
 * signal specific entries under debugfs.
 */
static void __init omap_mux_set_cmdline_signals(void)
{
	char *options, *next_opt, *token;

	if (!omap_mux_options)
		return;

	options = kmalloc(strlen(omap_mux_options) + 1, GFP_KERNEL);
	if (!options)
		return;

	strcpy(options, omap_mux_options);
	next_opt = options;

	while ((token = strsep(&next_opt, ",")) != NULL) {
		char *keyval, *name;
		unsigned long val;

		keyval = token;
		name = strsep(&keyval, "=");
		if (name) {
			int res;

			res = strict_strtoul(keyval, 0x10, &val);
			if (res < 0)
				continue;

			omap_mux_init_signal(name, (u16)val);
		}
	}

	kfree(options);
}

static void __init omap_mux_set_board_signals(struct omap_board_mux *board_mux)
{
	while (board_mux->reg_offset !=  OMAP_MUX_TERMINATOR) {
		omap_mux_write(board_mux->value, board_mux->reg_offset);
		board_mux++;
	}
}

static int __init omap_mux_copy_names(struct omap_mux *src,
					struct omap_mux *dst)
{
	int i;

	for (i = 0; i < OMAP_MUX_NR_MODES; i++) {
		if (src->muxnames[i]) {
			dst->muxnames[i] =
				kmalloc(strlen(src->muxnames[i]) + 1,
					GFP_KERNEL);
			if (!dst->muxnames[i])
				goto free;
			strcpy(dst->muxnames[i], src->muxnames[i]);
		}
	}

#ifdef CONFIG_DEBUG_FS
	for (i = 0; i < OMAP_MUX_NR_SIDES; i++) {
		if (src->balls[i]) {
			dst->balls[i] =
				kmalloc(strlen(src->balls[i]) + 1,
					GFP_KERNEL);
			if (!dst->balls[i])
				goto free;
			strcpy(dst->balls[i], src->balls[i]);
		}
	}
#endif

	return 0;

free:
	omap_mux_free_names(dst);
	return -ENOMEM;

}

#endif	/* CONFIG_OMAP_MUX */

static u16 omap_mux_get_by_gpio(int gpio)
{
	struct omap_mux_entry *e;
	u16 offset = OMAP_MUX_TERMINATOR;

	list_for_each_entry(e, &muxmodes, node) {
		struct omap_mux *m = &e->mux;
		if (m->gpio == gpio) {
			offset = m->reg_offset;
			break;
		}
	}

	return offset;
}

/* Needed for dynamic muxing of GPIO pins for off-idle */
u16 omap_mux_get_gpio(int gpio)
{
	u16 offset;

	offset = omap_mux_get_by_gpio(gpio);
	if (offset == OMAP_MUX_TERMINATOR) {
		printk(KERN_ERR "mux: Could not get gpio%i\n", gpio);
		return offset;
	}

	return omap_mux_read(offset);
}

/* Needed for dynamic muxing of GPIO pins for off-idle */
void omap_mux_set_gpio(u16 val, int gpio)
{
	u16 offset;

	offset = omap_mux_get_by_gpio(gpio);
	if (offset == OMAP_MUX_TERMINATOR) {
		printk(KERN_ERR "mux: Could not set gpio%i\n", gpio);
		return;
	}

	omap_mux_write(val, offset);
}

static struct omap_mux * __init omap_mux_list_add(struct omap_mux *src)
{
	struct omap_mux_entry *entry;
	struct omap_mux *m;

	entry = kzalloc(sizeof(struct omap_mux_entry), GFP_KERNEL);
	if (!entry)
		return NULL;

	m = &entry->mux;
	memcpy(m, src, sizeof(struct omap_mux_entry));

#ifdef CONFIG_OMAP_MUX
	if (omap_mux_copy_names(src, m)) {
		kfree(entry);
		return NULL;
	}
#endif

	mutex_lock(&muxmode_mutex);
	list_add_tail(&entry->node, &muxmodes);
	mutex_unlock(&muxmode_mutex);

	return m;
}

/*
 * Note if CONFIG_OMAP_MUX is not selected, we will only initialize
 * the GPIO to mux offset mapping that is needed for dynamic muxing
 * of GPIO pins for off-idle.
 */
static void __init omap_mux_init_list(struct omap_mux *superset)
{
	while (superset->reg_offset !=  OMAP_MUX_TERMINATOR) {
		struct omap_mux *entry;

#ifndef CONFIG_OMAP_MUX
		/* Skip pins that are not muxed as GPIO by bootloader */
		if (!OMAP_MODE_GPIO(omap_mux_read(superset->reg_offset))) {
			superset++;
			continue;
		}
#endif

#if defined(CONFIG_OMAP_MUX) && defined(CONFIG_DEBUG_FS)
		if (!superset->muxnames || !superset->muxnames[0]) {
			superset++;
			continue;
		}
#endif

		entry = omap_mux_list_add(superset);
		if (!entry) {
			printk(KERN_ERR "mux: Could not add entry\n");
			return;
		}
		superset++;
	}
}

int __init omap_mux_init(u32 mux_pbase, u32 mux_size,
				struct omap_mux *superset,
				struct omap_mux *package_subset,
				struct omap_board_mux *board_mux,
				struct omap_ball *package_balls)
{
	if (mux_base)
		return -EBUSY;

	mux_phys = mux_pbase;
	mux_base = ioremap(mux_pbase, mux_size);

	if (!mux_base) {
		printk(KERN_ERR "mux: Could not ioremap\n");
		return -ENODEV;
	}

#ifdef CONFIG_OMAP3_PADCONF_DUMP
	omap_padconfig_update_base_vaddr((unsigned long)mux_base);
#endif /* CONFIG_OMAP3_PADCONF_DUMP */

#ifdef CONFIG_OMAP_MUX
	omap_mux_package_fixup(package_subset, superset);
	omap_mux_package_init_balls(package_balls, superset);
	omap_mux_set_cmdline_signals();
	omap_mux_set_board_signals(board_mux);
#endif

	omap_mux_init_list(superset);

	return 0;
}

#endif	/* CONFIG_ARCH_OMAP34XX */

