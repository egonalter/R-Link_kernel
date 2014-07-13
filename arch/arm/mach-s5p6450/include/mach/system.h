/* linux/arch/arm/mach-s5p6450/include/mach/system.h
 *
 * Copyright (c) 2009 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S5P6450 - system support header
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_SYSTEM_H
#define __ASM_ARCH_SYSTEM_H __FILE__
#include <linux/io.h>
#include <mach/map.h>
#include <linux/clk.h>
#include <mach/regs-clock.h>

void (*s5p64xx_idle)(void);
//void (*s5p64xx_reset_hook)(void);

void s5p64xx_default_idle(void)
{
        /* nothing here yet */
}

static void arch_idle(void)
{
        if (s5p64xx_idle != NULL)
                (s5p64xx_idle)();
        else
                s5p64xx_default_idle();
}

static void arch_reset(char mode, const char *cmd)
{
	/* Bypass PLL's (feed osc directly in clk groups (see manpage 203)) */
	writel((readl(S5P_CLK_SRC0) & ~0x00000027), S5P_CLK_SRC0);

	/* switch off PLL's */
	writel((readl(S5P_APLL_CON) & ~0x80000000), S5P_APLL_CON);
	writel((readl(S5P_DPLL_CON) & ~0x80000000), S5P_DPLL_CON);
	writel((readl(S5P_EPLL_CON) & ~0x80000000), S5P_EPLL_CON);
	writel((readl(S5P_MPLL_CON) & ~0x80000000), S5P_MPLL_CON);

	/* perform the soft reset */
	writel(0x6450, S5P_SWRESET);
	
}

#endif /* __ASM_ARCH_SYSTEM_H */
