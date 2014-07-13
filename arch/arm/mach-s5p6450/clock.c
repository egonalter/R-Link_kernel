/* linux/arch/arm/mach-s5p6450/clock.c
 *
 * Copyright (c) 2009 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S5P6450 - Clock support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/sysdev.h>
#include <linux/io.h>

#include <mach/hardware.h>
#include <mach/map.h>

#include <plat/cpu-freq.h>
#include <mach/regs-clock.h>
#include <plat/clock.h>
#include <plat/cpu.h>
#include <plat/clock-clksrc.h>
#include <plat/s5p-clock.h>
#include <plat/pll.h>
#include <plat/s5p6450.h>
#include <plat/s5p6450.h>
/* APLL Mux output clock */
static struct clksrc_clk clk_mout_apll = {
	.clk    = {
		.name           = "mout_apll",
		.id             = -1,
	},
	.sources        = &clk_src_apll,
	.reg_src        = { .reg = S5P_CLK_SRC0, .shift = 0, .size = 1 },
};

static struct clksrc_clk clk_mout_mpll = {
	.clk = {
		.name           = "mout_mpll",
		.id             = -1,
	},
	.sources        = &clk_src_mpll,
	.reg_src        = { .reg = S5P_CLK_SRC0, .shift = 1, .size = 1 },
};

static struct clksrc_clk clk_mout_epll = {
	.clk    = {
		.name           = "mout_epll",
		.id             = -1,
	},
	.sources        = &clk_src_epll,
	.reg_src        = { .reg = S5P_CLK_SRC0, .shift = 2, .size = 1 },
};

static struct clksrc_clk clk_mout_dpll = {
	.clk    = {
		.name           = "mout_dpll",
		.id             = -1,
	},
	.sources        = &clk_src_dpll,
	.reg_src        = { .reg = S5P_CLK_SRC0, .shift = 5, .size = 1 },
};

static int s5p6450_epll_enable(struct clk *clk, int enable)
{
	unsigned int ctrlbit = clk->ctrlbit;
	unsigned int epll_con = __raw_readl(S5P_EPLL_CON) & ~ctrlbit;

	if (enable)
		__raw_writel(epll_con | ctrlbit, S5P_EPLL_CON);
	else
		__raw_writel(epll_con, S5P_EPLL_CON);

	return 0;
}

static unsigned long s5p6450_epll_get_rate(struct clk *clk)
{
	return clk->rate;
}
static u32 epll_div[][6] = {
/*         FOUT         V,      M,      P,      S       K       if  FIN :19200000 */
        { 32768000,     1,      54,     2,      4,      40195 },
        { 36000000,     1,      60,     2,      4,      0     },
        { 45158000,     0,      75,     2,      4,      17258 },
        { 45158400,     0,      56,     3,      3,      29360 },
        { 48000000,     0,      40,     2,      3,      0     },
        { 49152000,     0,      40,     2,      3,      62915 },
        { 50000000,     0,      41,     2,      3,      43691 },
        { 60000000,     1,      50,     2,      3,      0     },
        { 67737600,     1,      56,     2,      3,      29360 },
        { 72000000,     1,      60,     2,      3,      0     },
        { 73728000,     1,      61,     2,      3,      28836 },
        { 84000000,     0,      35,     2,      2,      0     },
        { 96000000,     0,      40,     2,      2,      0     },
};

static int s5p6450_epll_set_rate(struct clk *clk, unsigned long rate)
{
	unsigned int epll_con, epll_con_k;
	unsigned int i;
	unsigned int clk_div2;
	

	if (clk->rate == rate)	/* Return if nothing changed */
		return 0;
	
	/*S5P_CLK_DIV2[11:8]=0 */
	clk_div2 = __raw_readl(S5P_CLK_DIV2 );
	clk_div2 &= ~(0xf<<8);
	__raw_writel(clk_div2, S5P_CLK_DIV2 );

	/*S5P_CLK_DIV2[27:24]=0 */
	clk_div2 = __raw_readl(S5P_CLK_DIV2 );
	clk_div2 &= ~(0xf<<24);
	__raw_writel(clk_div2, S5P_CLK_DIV2 );
//	printk("S5P_CLK_DIV2= %x\n", clk_div2);

	epll_con = __raw_readl(S5P_EPLL_CON);
	epll_con_k = __raw_readl(S5P_EPLL_CON_K);

	epll_con_k &= ~(PLL90XX_KDIV_MASK | PLL90XX_VDIV_MASK << 16);
	epll_con &= ~(0xff << PLL90XX_MDIV_SHIFT |   \
			PLL90XX_PDIV_MASK << PLL90XX_PDIV_SHIFT | \
			PLL90XX_SDIV_MASK << PLL90XX_SDIV_SHIFT);

	for (i = 0; i < ARRAY_SIZE(epll_div); i++) {
		 if (epll_div[i][0] == rate) {
			epll_con_k |= epll_div[i][1] << 16; /*VSEL */
			epll_con |= epll_div[i][2] << 16;   /* M */
			epll_con |= epll_div[i][3] << 8;    /* P */
			epll_con |= epll_div[i][4] << 0;    /* S */
			epll_con_k |= epll_div[i][5] << 0;  /* K */
			break;
		}
	}

	if (i == ARRAY_SIZE(epll_div)) {
		printk(KERN_ERR "%s: Invalid Clock EPLL Frequency\n", __func__);
		return -EINVAL;
	}

	__raw_writel(epll_con, S5P_EPLL_CON);
	__raw_writel(epll_con_k, S5P_EPLL_CON_K);

	clk->rate = rate;

	return 0;
}

static struct clk_ops s5p6450_epll_ops = {
	.get_rate = s5p6450_epll_get_rate,
	.set_rate = s5p6450_epll_set_rate,
};

struct Clock_table ctable;

enum perf_level_1 {
	L0 = 532*1000,
	L1 = 266*1000,
	L2 = 133*1000,
};
enum perf_level_2 {
	F0 = 667200,
	F1 = 333*1000,
	F2 = 166*1000,
};
enum perf_level_3 {
	X0 = 800*1000,
	X1 = 400*1000,
	X2 = 100*1000,
};
static const u32 clock_table_533[][5] = {
    /*     ARM_CLK, 	     DIVarm 				DIVhclk				DIVpclk				DIVmali		*/
	{L0 * 1000, (0 << ARM_DIV_RATIO_SHIFT), (4 << S5P_CLKDIV0_HCLK166_SHIFT), (1 << S5P_CLKDIV0_PCLK83_SHIFT),(2 << S5P_CLKDIV3_MALI_SHIFT)},
	{L1 * 1000, (1 << ARM_DIV_RATIO_SHIFT), (4 << S5P_CLKDIV0_HCLK166_SHIFT), (1 << S5P_CLKDIV0_PCLK83_SHIFT),(2 << S5P_CLKDIV3_MALI_SHIFT)},
	{L2 * 1000, (3 << ARM_DIV_RATIO_SHIFT), (9 << S5P_CLKDIV0_HCLK166_SHIFT), (1 << S5P_CLKDIV0_PCLK83_SHIFT),(5 << S5P_CLKDIV3_MALI_SHIFT)},
};
static const u32 clock_table_667[][5]={
    /*    ARM_CLK, 	     DIVarm 				DIVhclk				DIVpclk				DIVmali		*/
	{F0 * 1000, (0 << ARM_DIV_RATIO_SHIFT), (3 << S5P_CLKDIV0_HCLK166_SHIFT), (1 << S5P_CLKDIV0_PCLK83_SHIFT),(2 << S5P_CLKDIV3_MALI_SHIFT)},
	{F1 * 1000, (1 << ARM_DIV_RATIO_SHIFT), (3 << S5P_CLKDIV0_HCLK166_SHIFT), (1 << S5P_CLKDIV0_PCLK83_SHIFT),(2 << S5P_CLKDIV3_MALI_SHIFT)},
	{F2 * 1000, (3 << ARM_DIV_RATIO_SHIFT), (7 << S5P_CLKDIV0_HCLK166_SHIFT), (1 << S5P_CLKDIV0_PCLK83_SHIFT),(5 << S5P_CLKDIV3_MALI_SHIFT)},
};
static const u32 clock_table_800[][5] = {
    /*    ARM_CLK, 	     DIVarm 				DIVhclk				DIVpclk				DIVmali		*/
	{X0 * 1000, (0 << ARM_DIV_RATIO_SHIFT), (3 << S5P_CLKDIV0_HCLK166_SHIFT), (1 << S5P_CLKDIV0_PCLK83_SHIFT),(2 << S5P_CLKDIV3_MALI_SHIFT)},
	{X1 * 1000, (1 << ARM_DIV_RATIO_SHIFT), (3 << S5P_CLKDIV0_HCLK166_SHIFT), (1 << S5P_CLKDIV0_PCLK83_SHIFT),(2 << S5P_CLKDIV3_MALI_SHIFT)},
	{X2 * 1000, (7 << ARM_DIV_RATIO_SHIFT), (7 << S5P_CLKDIV0_HCLK166_SHIFT), (1 << S5P_CLKDIV0_PCLK83_SHIFT),(5 << S5P_CLKDIV3_MALI_SHIFT)},
};


static unsigned long s5p6450_armclk_get_rate(struct clk *clk)
{
	unsigned long rate = clk_get_rate(clk->parent);
	u32 clkdiv;
	/* divisor mask starts at bit0, so no need to shift */
	clkdiv = __raw_readl(ARM_CLK_DIV) & ARM_DIV_MASK;

	return rate / (clkdiv + 1);
}

static unsigned long s5p6450_armclk_round_rate(struct clk *clk,
						unsigned long rate)
{
	u32 iter;
	for (iter = 1 ; iter < ctable.clock_table_size; iter++) {
		if (rate > ctable.clock_table[iter][0])
			return ctable.clock_table[iter-1][0];
	}

	return ctable.clock_table[ctable.clock_table_size - 1][0];
}

static int s5p6450_armclk_set_rate(struct clk *clk, unsigned long rate)
{
	u32 round_tmp;
	u32 iter;
	u32 clk_div0_tmp;
	u32 clk_div3_tmp;
	u32 cur_rate = clk->ops->get_rate(clk);
	unsigned long flags;

	round_tmp = clk->ops->round_rate(clk, rate);
	if (round_tmp == cur_rate)
		return 0;


	for (iter = 0 ; iter < ctable.clock_table_size ; iter++) {
		if (round_tmp == ctable.clock_table[iter][0])
			break;
	}

	if (iter >= ctable.clock_table_size)
		iter = ctable.clock_table_size - 1;
	

	local_irq_save(flags);
	if (cur_rate > round_tmp) {
		/* Frequency Down */
		clk_div0_tmp = __raw_readl(ARM_CLK_DIV) & ~(ARM_DIV_MASK);
		clk_div0_tmp |= ctable.clock_table[iter][1];
		__raw_writel(clk_div0_tmp, ARM_CLK_DIV);

		clk_div0_tmp = __raw_readl(ARM_CLK_DIV) &
				~(S5P_CLKDIV0_HCLK166_MASK);
		clk_div0_tmp |= ctable.clock_table[iter][2];
		__raw_writel(clk_div0_tmp, ARM_CLK_DIV);  //prob
		

#if 1
			clk_div0_tmp = __raw_readl(ARM_CLK_DIV) &
						~(S5P_CLKDIV0_PCLK83_MASK);
			clk_div0_tmp |= ctable.clock_table[iter][3];
			__raw_writel(clk_div0_tmp, ARM_CLK_DIV);  //prob
#endif			
			clk_div0_tmp = __raw_readl(S5P_CLK_DIV3) &
						~(S5P_CLKDIV3_MALI_MASK);
			clk_div0_tmp |= ctable.clock_table[iter][4];
			__raw_writel(clk_div0_tmp, S5P_CLK_DIV3);  //prob

	} else {
		/* Frequency Up */
		clk_div0_tmp = __raw_readl(ARM_CLK_DIV) &
				~(S5P_CLKDIV0_HCLK166_MASK);
		clk_div0_tmp |= ctable.clock_table[iter][2];
		__raw_writel(clk_div0_tmp, ARM_CLK_DIV);

#if 1		
			clk_div0_tmp = __raw_readl(ARM_CLK_DIV) &
						~(S5P_CLKDIV0_PCLK83_MASK);
			clk_div0_tmp |= ctable.clock_table[iter][3];
			__raw_writel(clk_div0_tmp, ARM_CLK_DIV);  //prob
			clk_div0_tmp = __raw_readl(S5P_CLK_DIV3) &
						~(S5P_CLKDIV3_MALI_MASK);
			clk_div0_tmp |= ctable.clock_table[iter][4];
			__raw_writel(clk_div0_tmp, S5P_CLK_DIV3);  //prob


#endif			

		clk_div0_tmp = __raw_readl(ARM_CLK_DIV) & ~(ARM_DIV_MASK);
		clk_div0_tmp |= ctable.clock_table[iter][1];
		__raw_writel(clk_div0_tmp, ARM_CLK_DIV);
		}
	local_irq_restore(flags);

	clk->rate = ctable.clock_table[iter][0];
	printk("ARM clk freq    -----   %u\n",clk->rate);
	return 0;
}

static struct clk_ops s5p6450_clkarm_ops = {
	.get_rate	= s5p6450_armclk_get_rate,
	.set_rate	= s5p6450_armclk_set_rate,
	.round_rate	= s5p6450_armclk_round_rate,
};

struct  clksrc_clk clk_armclk = {
#if 1	
	.clk	= {
		.name	= "armclk",
		.id	= -1,
		.parent	= &clk_mout_apll.clk,
		.ops	= &s5p6450_clkarm_ops,
	},
#endif
	.reg_div	= { .reg = S5P_CLK_DIV0, .shift = 0, .size = 4 },
};

static struct clksrc_clk clk_dout_mpll = {
	.clk	= {
		.name	= "dout_mpll",
		.id	= -1,
		.parent	= &clk_mout_mpll.clk,
	},
	.reg_div	= { .reg = S5P_CLK_DIV0, .shift = 4, .size = 1 },
};

static struct clksrc_clk clk_dout_epll = {
	.clk	= {
		.name	= "dout_epll",
		.id	= -1,
		.parent	= &clk_mout_epll.clk,
	},
	.reg_div	= { .reg = S5P_CLK_DIV1, .shift = 24, .size = 4 },
};

static struct clk *clkset_hclk_sel_list[] = {
	&clk_mout_apll.clk,
	&clk_mout_mpll.clk,
};

static struct clksrc_sources clkset_hclk_sel = {
	.sources	= clkset_hclk_sel_list,
	.nr_sources	= ARRAY_SIZE(clkset_hclk_sel_list),
};

static struct clksrc_clk clk_mout_hclk_sel = {
	.clk	= {
		.name	= "mout_hclk_sel",
		.id	= -1,
	},
	.sources	= &clkset_hclk_sel,
	.reg_src        = { .reg = S5P_OTHERS, .shift = 15, .size = 1 },
};

static struct clk *clkset_hclk166_list[] = {
	&clk_mout_hclk_sel.clk,
	&clk_armclk.clk,
};

static struct clksrc_sources clkset_hclk166 = {
	.sources	= clkset_hclk166_list,
	.nr_sources	= ARRAY_SIZE(clkset_hclk166_list),
};

static struct clksrc_clk clk_hclk166 = {
	.clk	= {
		.name	= "clk_hclk166",
		.id	= -1,
	},
	.sources	= &clkset_hclk166,
	.reg_src        = { .reg = S5P_OTHERS, .shift = 14, .size = 1 },
	.reg_div	= { .reg = S5P_CLK_DIV0, .shift = 8, .size = 4 },
};

static struct clksrc_clk clk_pclk83 = {
	.clk	= {
		.name	= "clk_pclk83",
		.id	= -1,
		.parent	= &clk_hclk166.clk,
	},
	.reg_div = { .reg = S5P_CLK_DIV0, .shift = 12, .size = 4 },
};

static struct clksrc_clk clk_hclk133 = {
	.clk	= {
		.name	= "clk_hclk133",
		.id	= -1,
	},
	.sources	= &clkset_hclk_sel,
	.reg_src        = { .reg = S5P_OTHERS, .shift = 6, .size = 1 },
	.reg_div	= { .reg = S5P_CLK_DIV3, .shift = 8, .size = 4 },
};

static struct clksrc_clk clk_pclk66 = {
	.clk	= {
		.name	= "clk_pclk66",
		.id	= -1,
		.parent	= &clk_hclk133.clk,
	},
	.reg_div = { .reg = S5P_CLK_DIV3, .shift = 12, .size = 4 },
};

static struct clksrc_clk clk_dout_pwm_ratio0 = {
	.clk	= {
		.name	= "clk_dout_pwm_ratio0",
		.id	= -1,
		.parent	= &clk_mout_hclk_sel.clk,
	},
	.reg_div	= { .reg = S5P_CLK_DIV3, .shift = 16, .size = 4 },
};

static struct clksrc_clk clk_pclk_to_wdt_pwm = {
	.clk	= {
		.name	= "clk_pclk_to_wdt_pwm",
		.id	= -1,
		.parent	= &clk_dout_pwm_ratio0.clk,
	},
	.reg_div	= { .reg = S5P_CLK_DIV3, .shift = 20, .size = 4 },
};

int s5p6450_clk48m_ctrl(struct clk *clk, int enable)
{
	unsigned long flags;
	u32 val;

	/* can't rely on clock lock, this register has other usages */
	local_irq_save(flags);

	val = __raw_readl(S5P_OTHERS);
	if (enable)
		val |= S5P_OTHERS_USB_SIG_MASK;
	else
		val &= ~S5P_OTHERS_USB_SIG_MASK;

	__raw_writel(val, S5P_OTHERS);

	local_irq_restore(flags);

	return 0;
}

static int s5p6450_pclk_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLK_GATE_PCLK, clk, enable);
}

static int s5p6450_hclk0_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLK_GATE_HCLK0, clk, enable);
}

static int s5p6450_hclk1_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLK_GATE_HCLK1, clk, enable);
}

static int s5p6450_sclk_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLK_GATE_SCLK0, clk, enable);
}

static int s5p6450_sclk1_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLK_GATE_SCLK1, clk, enable);
}
static int s5p6450_mem_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLK_GATE_MEM0, clk, enable);
}

/*
 * The following clocks will be disabled during clock initialization. It is
 * recommended to keep the following clocks disabled until the driver requests
 * for enabling the clock.
 */
static struct clk init_clocks_disable[] = {

/* 		HCLK  0 GATING    */
       	{
                .name           = "hclk_2d",
                .id             = -1,
                .parent         = &clk_hclk166.clk,
                .enable         = s5p6450_hclk0_ctrl,
                .ctrlbit        = S5P_CLKCON_HCLK0_2D,
        },{
                .name           = "hclk_fimc",
                .id             = -1,
                .parent         = &clk_hclk133.clk,
                .enable         = s5p6450_hclk0_ctrl,
                .ctrlbit        = S5P_CLKCON_HCLK0_FIMC,
        },{
                .name           = "hsmmc_44",
                .id             = 0,
                .parent         = &clk_hclk133.clk,
                .enable         = s5p6450_hclk0_ctrl,
                .ctrlbit        = S5P_CLKCON_HCLK0_HSMMC44,
        },{
                .name           = "hsmmc",
                .id             = 0,
                .parent         = &clk_hclk133.clk,
                .enable         = s5p6450_hclk0_ctrl,
                .ctrlbit        = S5P_CLKCON_HCLK0_HSMMC0,
        },{
                .name           = "hsmmc",
                .id             = 2,
                .parent         = &clk_hclk133.clk,
                .enable         = s5p6450_hclk0_ctrl,
                .ctrlbit        = S5P_CLKCON_HCLK0_HSMMC2,
        },{
                .name           = "hsmmc",
                .id             = 1,
                .parent         = &clk_hclk133.clk,
                .enable         = s5p6450_hclk0_ctrl,
                .ctrlbit        = S5P_CLKCON_HCLK0_HSMMC1,
        },{
                .name           = "usbotg",
                .id             = -1,
                .parent         = &clk_hclk133.clk,
                .enable         = s5p6450_hclk0_ctrl,
                .ctrlbit        = S5P_CLKCON_HCLK0_USB,
        },{
                .name           = "usbhost",
                .id             = -1,
                .parent         = &clk_hclk133.clk,
                .enable         = s5p6450_hclk0_ctrl,
                .ctrlbit        = S5P_CLKCON_HCLK0_HOST,
        },{
                .name           = "adc",
                .id             = -1,
                .parent         = &clk_pclk66.clk,
                .enable         = s5p6450_pclk_ctrl,
                .ctrlbit         = (1 << 12),
        },{
                .name           = "hclk_rotator",
                .id             = -1,
                .parent         = &clk_hclk133.clk,
                .enable         = s5p6450_hclk0_ctrl,
                .ctrlbit         = S5P_CLKCON_HCLK0_ROTATOR,
        },{
                .name           = "hclk_tzic1",
                .id             = -1,
                .parent         = &clk_hclk166.clk,
                .enable         = s5p6450_hclk0_ctrl,
		.ctrlbit	= S5P_CLKCON_HCLK0_TZIC1,	
	},{
                .name           = "hclk_tzic0",
                .id             = -1,
                .parent         = &clk_hclk166.clk,
                .enable         = s5p6450_hclk0_ctrl,
		.ctrlbit	= S5P_CLKCON_HCLK0_TZIC0,	
	},{
		.name           = "hclk_sss",
                .id             = -1,
                .parent         = &clk_hclk133.clk,
                .enable         = s5p6450_hclk0_ctrl,
                .ctrlbit        = S5P_CLKCON_HCLK0_SSS,
	},{
                .name           = "hclk_tsi",
                .id             = -1,
                .parent         = &clk_hclk133.clk,
                .enable         = s5p6450_hclk1_ctrl,
                .ctrlbit        = S5P_CLKCON_HCLK1_TSI,
        },
/* 		  PCLK			*/
	{                
                .name           = "pclk_2d",
                .id             = -1,
                .parent         = &clk_pclk83.clk,
                .enable         = s5p6450_pclk_ctrl,
                .ctrlbit        = S5P_CLKCON_PCLK_2D,
        },{                
                .name           = "pclk_etm",
                .id             = -1,
                .enable         = s5p6450_pclk_ctrl,
                .ctrlbit        = S5P_CLKCON_PCLK_ETM,
        },{
                .name           = "i2c",
                .id             = 0,
                .parent         = &clk_pclk66.clk,
                .enable         = s5p6450_pclk_ctrl,
                .ctrlbit        = S5P_CLKCON_PCLK_IIC0,
        },{
                .name           = "i2c",
                .id             = 1,
                .parent         = &clk_pclk66.clk,
                .enable         = s5p6450_pclk_ctrl,
                .ctrlbit        = S5P_CLKCON_PCLK_IIC1,
        },{
                .name           = "iis",
                .id             = 0,
                .parent         = &clk_pclk66.clk,
                .enable         = s5p6450_pclk_ctrl,
                .ctrlbit        = S5P_CLKCON_PCLK_I2S0,
        },{
                .name           = "iis",
                .id             = 1,
                .parent         = &clk_pclk66.clk,
                .enable         = s5p6450_pclk_ctrl,
                .ctrlbit        = S5P_CLKCON_PCLK_I2S1,
        },{
                .name           = "iis",
                .id             = 2,
                .parent         = &clk_pclk66.clk,
                .enable         = s5p6450_pclk_ctrl,
                .ctrlbit        = S5P_CLKCON_PCLK_I2S2,
        },{
                .name           = "pclk_tzpc0",
                .id             = -1,
                .parent         = &clk_pclk83.clk,
                .enable         = s5p6450_pclk_ctrl,
                .ctrlbit        = S5P_CLKCON_PCLK_TZPC0,
        },{
                .name           = "pclk_tzpc1",
                .id             = -1,
                .parent         = &clk_pclk66.clk,
                .enable         = s5p6450_pclk_ctrl,
                .ctrlbit        = S5P_CLKCON_PCLK_TZPC1,
        },{
                .name           = "pclk_dsim",
                .id             = -1,
                .parent         = &clk_pclk66.clk,
                .enable         = s5p6450_pclk_ctrl,
                .ctrlbit        = S5P_CLKCON_PCLK_DSIM,
        },{
                .name           = "pclk_spi",
                .id             = 0,
		.parent         = &clk_pclk66.clk,
                .enable         = s5p6450_pclk_ctrl,
                .ctrlbit        = S5P_CLKCON_PCLK_SPI0,
        },{
                .name           = "pclk_spi",
                .id             = 1,
		.parent         = &clk_pclk66.clk,
                .enable         = s5p6450_pclk_ctrl,
                .ctrlbit        = S5P_CLKCON_PCLK_SPI1,
        },{
                .name           = "pclk_rotator",
                .id             = -1,
                .parent         = &clk_pclk66.clk,
                .enable         = s5p6450_pclk_ctrl,
                .ctrlbit        = S5P_CLKCON_PCLK_ROTATOR,
        },{
                .name           = "pclk_usi",
                .id             = -1,
                .parent         = &clk_pclk66.clk,
                .enable         = s5p6450_pclk_ctrl,
                .ctrlbit        = S5P_CLKCON_PCLK_USI,
        },{
                .name           = "pclk_tsadc",
                .id             = -1,
                .parent         = &clk_pclk66.clk,
                .enable         = s5p6450_pclk_ctrl,
                .ctrlbit        = S5P_CLKCON_PCLK_TSADC,
        },{
                .name           = "watchdog",
                .id             = -1,
                .parent         = &clk_pclk66.clk,
                .enable         = s5p6450_pclk_ctrl,
                .ctrlbit        = S5P_CLKCON_PCLK_WDT,
        },{
                .name           = "pclk_pcm",
                .id             = 1,
 		.parent         = &clk_pclk66.clk,
                .enable         = s5p6450_pclk_ctrl,
                .ctrlbit        = S5P_CLKCON_PCLK_PCM1,
        },{
                .name           = "pclk_pcm",
                .id             = 0,
 		.parent         = &clk_pclk66.clk,
                .enable         = s5p6450_pclk_ctrl,
                .ctrlbit        = S5P_CLKCON_PCLK_PCM0,
        },

#if 0	
	{
                .name           = "pclk_uart",
                .id             =  2,
		.parent		= &clk_pclk66.clk,
                .enable         = s5p6450_pclk_ctrl,
                .ctrlbit        = S5P_CLKCON_PCLK_UART2,
        },{
                .name           = "pclk_uart",
                .id             =  3,
		.parent		= &clk_pclk66.clk,
                .enable         = s5p6450_pclk_ctrl,
                .ctrlbit        = S5P_CLKCON_PCLK_UART3,
        },{
                .name           = "pclk_uart",
                .id             =  5,
		.parent		= &clk_pclk66.clk,
                .enable         = s5p6450_pclk_ctrl,
                .ctrlbit        = S5P_CLKCON_PCLK_UART5,
        },
#endif

};

/*
 * The following clocks will be enabled during clock initialization.
 */
static struct clk init_clocks[] = {
	 {
                .name           = "nand",
                .id             = -1,
                .parent         = &clk_h,
                .enable         = s5p6450_mem_ctrl,
                .ctrlbit        = S5P_CLKCON_MEM0_HCLK_NFCON,
        },
	{
		.name           = "lcd",
		.id             = -1,
		.parent         = &clk_h_low,
		.enable         = s5p6450_hclk1_ctrl,
		.ctrlbit        = S5P_CLKCON_HCLK1_DISPCON,
	}, {
		.name		= "gpio",
		.id		= -1,
		.parent		= &clk_pclk66.clk,
		.enable		= s5p6450_pclk_ctrl,
		.ctrlbit	= S5P_CLKCON_PCLK_GPIO,
	}, {
		.name		= "uart",
		.id		= 0,
		.parent		= &clk_pclk66.clk,
		.enable		= s5p6450_pclk_ctrl,
		.ctrlbit	= S5P_CLKCON_PCLK_UART0,
	},
	 {
		.name		= "uart",
		.id		= 1,
		.parent		= &clk_pclk66.clk,
		.enable		= s5p6450_pclk_ctrl,
		.ctrlbit	= S5P_CLKCON_PCLK_UART1,
	}, 
	{
		.name		= "uart",
		.id		= 2,
		.parent		= &clk_pclk66.clk,
		.enable		= s5p6450_pclk_ctrl,
		.ctrlbit	= S5P_CLKCON_PCLK_UART2,
	},{
		.name		= "uart",
		.id		= 3,
		.parent		= &clk_pclk66.clk,
		.enable		= s5p6450_pclk_ctrl,
		.ctrlbit	= S5P_CLKCON_PCLK_UART3,
	},
	{
		.name		= "uart",
		.id		= 4,
		.parent		= &clk_pclk66.clk,
		.enable		= s5p6450_pclk_ctrl,
		.ctrlbit	= S5P_CLKCON_PCLK_UART4,
	},{
		.name		= "uart",
		.id		= 5,
		.parent		= &clk_pclk66.clk,
		.enable		= s5p6450_pclk_ctrl,
		.ctrlbit	= S5P_CLKCON_PCLK_UART5,
	},{
	
		.name		= "mem",
		.id		= -1,
		.parent		= &clk_hclk166.clk,
		.enable		= s5p6450_hclk0_ctrl,
		.ctrlbit	= S5P_CLKCON_HCLK0_MEM0,
	},{
		.name           = "dmc0",
		.id             = -1,
		.parent         = &clk_pclk83.clk,
		.enable         = s5p6450_pclk_ctrl,
		.ctrlbit        = (1 << 30),
	},{
		.name		= "intc",
		.id		= -1,
		.parent		= &clk_hclk166.clk,
		.enable		= s5p6450_hclk0_ctrl,
		.ctrlbit	= S5P_CLKCON_HCLK0_INTC,
	},{
                .name           = "dma",
                .id             = -1,
                .parent         = &clk_hclk133.clk,
                .enable         = s5p6450_hclk0_ctrl,
                .ctrlbit        = S5P_CLKCON_HCLK0_DMA0,
        },{
                .name           = "timers",
                .id             = -1,
                .parent         = &clk_pclk_to_wdt_pwm.clk,
                .enable         = s5p6450_pclk_ctrl,
                .ctrlbit        = S5P_CLKCON_PCLK_PWM,
        },
};


static struct clk *clkset_group1_list[] = {
	&clk_mout_epll.clk,
	&clk_dout_mpll.clk,
	&clk_fin_epll,
};

static struct clksrc_sources clkset_group1 = {
	.sources	= clkset_group1_list,
	.nr_sources	= ARRAY_SIZE(clkset_group1_list),
};

static struct clk *clkset_uart_list[] = {
	&clk_dout_epll.clk,
	&clk_dout_mpll.clk,
};

static struct clksrc_sources clkset_uart = {
	.sources	= clkset_uart_list,
	.nr_sources	= ARRAY_SIZE(clkset_uart_list),
};

static struct clk *clkset_mali_list[] = {
	&clk_mout_epll.clk,
	&clk_mout_apll.clk,
	&clk_mout_mpll.clk,
};

static struct clksrc_sources clkset_mali = {
	.sources	= clkset_mali_list,
	.nr_sources	= ARRAY_SIZE(clkset_mali_list),
};

static struct clk *clkset_group2_list[] = {
	&clk_dout_epll.clk,
	&clk_dout_mpll.clk,
	&clk_ext_xtal_mux,
};

static struct clksrc_sources clkset_group2 = {
	.sources	= clkset_group2_list,
	.nr_sources	= ARRAY_SIZE(clkset_group2_list),
};

static struct clk *clkset_dispcon_list[] = {
	&clk_dout_epll.clk,
	&clk_dout_mpll.clk,
	&clk_ext_xtal_mux,
	&clk_mout_dpll.clk,
};

static struct clksrc_sources clkset_dispcon = {
	.sources	= clkset_dispcon_list,
	.nr_sources	= ARRAY_SIZE(clkset_dispcon_list),
};

static struct clk *clkset_hsmmc44_list[] = {
	&clk_dout_epll.clk,
	&clk_dout_mpll.clk,
	&clk_ext_xtal_mux,
	&s5p_clk_27m,
	&clk_48m,
};

static struct clksrc_sources clkset_hsmmc44 = {
	.sources	= clkset_hsmmc44_list,
	.nr_sources	= ARRAY_SIZE(clkset_hsmmc44_list),
};

static struct clk *clkset_sclk_audio0_list[] = {
	[0] = &clk_dout_epll.clk,
	[1] = &clk_dout_mpll.clk,
	[2] = &clk_ext_xtal_mux,
	[3] = NULL,
	[4] = NULL,
};

static struct clksrc_sources clkset_sclk_audio0 = {
	.sources	= clkset_sclk_audio0_list,
	.nr_sources	= ARRAY_SIZE(clkset_sclk_audio0_list),
};

static struct clk *clkset_sclk_audio1_list[] = {
	[0] = &clk_dout_epll.clk,
	[1] = &clk_dout_mpll.clk,
	[2] = &clk_ext_xtal_mux,
	[3] = NULL,
	[4] = NULL,
};

static struct clksrc_sources clkset_sclk_audio1 = {
	.sources	= clkset_sclk_audio1_list,
	.nr_sources	= ARRAY_SIZE(clkset_sclk_audio1_list),
};

static struct clk *clkset_sclk_audio2_list[] = {
	[0] = &clk_dout_epll.clk,
	[1] = &clk_dout_mpll.clk,
	[2] = &clk_ext_xtal_mux,
	[3] = NULL,
	[4] = NULL,
};

static struct clksrc_sources clkset_sclk_audio2 = {
	.sources	= clkset_sclk_audio2_list,
	.nr_sources	= ARRAY_SIZE(clkset_sclk_audio2_list),
};

static struct clksrc_clk clksrc_audio[] = {
	{
		.clk	= {
			.name		= "audio-bus",
			.id			= 0,
			.enable		= s5p6450_sclk_ctrl,
			.ctrlbit	= S5P_CLKCON_SCLK0_AUDIO0,
			.parent		= &clk_dout_epll.clk,
		},
		.sources = &clkset_sclk_audio0,
		.reg_src = { .reg = S5P_CLK_SRC1, .shift = 10, .size = 3 },
		.reg_div = { .reg = S5P_CLK_DIV2, .shift = 8, .size = 4 },
	}, {
		.clk		= {
			.name		= "audio-bus",
			.id			= 1,
			.enable		= s5p6450_sclk_ctrl,
			.ctrlbit	= S5P_CLKCON_SCLK0_AUDIO1,
			.parent		= &clk_dout_epll.clk,
		},
		.sources = &clkset_sclk_audio1,
		.reg_src = { .reg = S5P_CLK_SRC1, .shift = 13, .size = 3 },
		.reg_div = { .reg = S5P_CLK_DIV2, .shift = 12, .size = 4 },
	}, {
		.clk		= {
			.name		= "audio-bus",
			.id			= 2,
			.enable		= s5p6450_sclk_ctrl,
			.ctrlbit	= S5P_CLKCON_SCLK0_AUDIO2,
			.parent		= &clk_dout_epll.clk,
		},
		.sources = &clkset_sclk_audio2,
		.reg_src = { .reg = S5P_CLK_SRC1, .shift = 0, .size = 3 },
		.reg_div = { .reg = S5P_CLK_DIV2, .shift = 24, .size = 4 },
	},
};

static struct clksrc_clk clksrcs[] = {
        {
		.clk	= {
			.name		= "aclk_mali",
			.id		= -1,
			.ctrlbit        = S5P_CLKCON_SCLK1_MALI,
			.enable		= s5p6450_sclk1_ctrl,
		},
		.sources = &clkset_mali,
		.reg_src = { .reg = S5P_CLK_SRC1, .shift = 8, .size = 2 },
		.reg_div = { .reg = S5P_CLK_DIV3, .shift = 4, .size = 4 },
	},{
                        .clk    = {
                        .name           = "sclk_dispcon",
                        .id             = -1,
                        .ctrlbit        = S5P_CLKCON_SCLK1_DISPCON,
                        .enable         = s5p6450_sclk1_ctrl,
                },
                .sources = &clkset_dispcon,
                .reg_src = { .reg = S5P_CLK_SRC1, .shift = 4, .size = 2 },
                .reg_div = { .reg = S5P_CLK_DIV3, .shift = 0, .size = 4 },
        },{
		.clk	= {
			.name		= "sclk_2d",
			.id		= -1,
			.ctrlbit        = S5P_CLKCON_SCLK0_2D,
			.enable		= s5p6450_sclk_ctrl,
		},
		.sources = &clkset_mali,
		.reg_src = { .reg = S5P_CLK_SRC0, .shift = 30, .size = 2 },
		.reg_div = { .reg = S5P_CLK_DIV2, .shift = 20, .size = 4 },
	},{
                        .clk    = {
                        .name           = "sclk_fimc",
                        .id             = -1,
                        .ctrlbit        = S5P_CLKCON_SCLK0_FIMC,
                        .enable         = s5p6450_sclk_ctrl,
                },
                .sources = &clkset_group2,
                .reg_src = { .reg = S5P_CLK_SRC0, .shift = 26, .size = 2 },
                .reg_div = { .reg = S5P_CLK_DIV1, .shift = 12, .size = 4 },
       },{
		.clk	= {
			.name		= "sclk_usi",
			.id		= -1,
			.ctrlbit	= S5P_CLKCON_SCLK0_USI,
			.enable		= s5p6450_sclk_ctrl,
		},
		.sources = &clkset_group2,
		.reg_src = { .reg = S5P_CLK_SRC0, .shift = 10, .size = 2 },
		.reg_div = { .reg = S5P_CLK_DIV1, .shift = 16, .size = 4 },
	},{
                .clk    = {
                        .name           = "uclk1",
                        .id             = -1,
                        .ctrlbit        = S5P_CLKCON_SCLK0_UART,
                        .enable         = s5p6450_sclk_ctrl,
                },
                .sources = &clkset_uart,
                .reg_src = { .reg = S5P_CLK_SRC0, .shift = 13, .size = 1 },
                .reg_div = { .reg = S5P_CLK_DIV2, .shift = 16, .size = 4 },
        },{
			.clk	= {
			.name		= "sclk_camif",
			.id		= -1,
			.ctrlbit	= S5P_CLKCON_SCLK0_CAMIF,
			.enable		= s5p6450_sclk_ctrl,
		},
		.sources = &clkset_group2,
		.reg_src = { .reg = S5P_CLK_SRC0, .shift = 28, .size = 2 },
		.reg_div = { .reg = S5P_CLK_DIV1, .shift = 20, .size = 4 },
	},{

		.clk		= {
			.name		= "sclk_spi",
			.id		= 0,
			.ctrlbit	= S5P_CLKCON_SCLK0_SPI0,
			.enable		= s5p6450_sclk_ctrl,
		},
		.sources = &clkset_group2,
		.reg_src = { .reg = S5P_CLK_SRC0, .shift = 14, .size = 2 },
		.reg_div = { .reg = S5P_CLK_DIV2, .shift = 0, .size = 4 },
	},{
		.clk		= {
			.name		= "sclk_spi",
			.id		= 1,
			.ctrlbit	= S5P_CLKCON_SCLK0_SPI1,
			.enable		= s5p6450_sclk_ctrl,
		},
		.sources = &clkset_group2,
		.reg_src = { .reg = S5P_CLK_SRC0, .shift = 16, .size = 2 },
		.reg_div = { .reg = S5P_CLK_DIV2, .shift = 4, .size = 4 },
	},{
			.clk		= {
			.name		= "sclk_hsmmc44",
			.id		= -1,
			.ctrlbit	= S5P_CLKCON_SCLK0_MMC44,
			.enable		= s5p6450_sclk_ctrl,
		},
		.sources = &clkset_hsmmc44,
		.reg_src = { .reg = S5P_CLK_SRC0, .shift = 6, .size = 3 },
		.reg_div = { .reg = S5P_CLK_DIV1, .shift = 28, .size = 4 },
	},{
		.clk		= {
			.name		= "sclk_mmc",
			.id		= 0,
			.ctrlbit	= S5P_CLKCON_SCLK0_MMC0,
			.enable		= s5p6450_sclk_ctrl,
		},
		.sources = &clkset_group2,
		.reg_src = { .reg = S5P_CLK_SRC0, .shift = 18, .size = 2 },
		.reg_div = { .reg = S5P_CLK_DIV1, .shift = 0, .size = 4 },
	},{
		.clk		= {
			.name		= "sclk_mmc",
			.id		= 1,
			.ctrlbit	= S5P_CLKCON_SCLK0_MMC1,
			.enable		= s5p6450_sclk_ctrl,
		},
		.sources = &clkset_group2,
		.reg_src = { .reg = S5P_CLK_SRC0, .shift = 20, .size = 2 },
		.reg_div = { .reg = S5P_CLK_DIV1, .shift = 4, .size = 4 },
	},{
		.clk		= {
			.name		= "sclk_mmc",
			.id		= 2,
			.ctrlbit	= S5P_CLKCON_SCLK0_MMC2,
			.enable		= s5p6450_sclk_ctrl,
		},
		.sources = &clkset_group2,
		.reg_src = { .reg = S5P_CLK_SRC0, .shift = 22, .size = 2 },
		.reg_div = { .reg = S5P_CLK_DIV1, .shift = 8, .size = 4 },
	},
};

/* Clock initialisation code */
static struct clksrc_clk *sysclks[] = {
	&clk_mout_apll,
	&clk_mout_epll,
	&clk_dout_epll,
	&clk_mout_mpll,
	&clk_dout_mpll,
	&clk_armclk,
	&clk_mout_hclk_sel,
	&clk_dout_pwm_ratio0,
	&clk_pclk_to_wdt_pwm,
	&clk_hclk166,
	&clk_pclk83,
	&clk_hclk133,
	&clk_pclk66,
};

void __init_or_cpufreq s5p6450_setup_clocks(void)
{
	struct clk *xtal_clk;
	unsigned long xtal;
	unsigned long fclk;
	unsigned long hclk;
	unsigned long hclk_low;
	unsigned long pclk;
	unsigned long pclk_low;
	unsigned long epll;
	unsigned long dpll;
	unsigned long apll;
	unsigned long mpll;
	unsigned int ptr;
	
	/* Set S5P6450 functions for clk_fout_epll */
	clk_fout_epll.enable = s5p6450_epll_enable;
	clk_fout_epll.ops = &s5p6450_epll_ops;

	clk_48m.enable = s5p6450_clk48m_ctrl;

	xtal_clk = clk_get(NULL, "ext_xtal");
	BUG_ON(IS_ERR(xtal_clk));

	xtal = clk_get_rate(xtal_clk);
	clk_put(xtal_clk);

	epll = s5p_get_pll90xx(xtal, __raw_readl(S5P_EPLL_CON),
				__raw_readl(S5P_EPLL_CON_K));
	dpll = s5p_get_pll46xx(xtal, __raw_readl(S5P_DPLL_CON),
				__raw_readl(S5P_DPLL_CON_K), pll_4650c);
	mpll = s5p_get_pll45xx(xtal, __raw_readl(S5P_MPLL_CON), pll_4502);
	apll = s5p_get_pll45xx(xtal, __raw_readl(S5P_APLL_CON), pll_4502);

	clk_fout_mpll.rate = mpll;
	clk_fout_epll.rate = epll;
	clk_fout_dpll.rate = dpll;
	clk_fout_apll.rate = apll;
	
	ctable.apll_rate = clk_fout_apll.rate/1000000;
	
	switch (ctable.apll_rate)
	{
		case IS_ARM_800 :
	                ctable.clock_table_size = ARRAY_SIZE(clock_table_800);
        	        ctable.clock_table = clock_table_800;
			break;
		case IS_ARM_667:
	                ctable.clock_table_size = ARRAY_SIZE(clock_table_667);
        	        ctable.clock_table = clock_table_667;

			break;
		case IS_ARM_533:
                	ctable.clock_table_size = ARRAY_SIZE(clock_table_533);
	                ctable.clock_table = clock_table_533;

			break;
		default:
	  		printk("------------------------- UNKNOW FREQ   WARNING !!!!!!!!!---------------  e   \n");
	
	}
/*
	if(ctable.apll_rate == IS_ARM_667){
		printk("-----------------------------------------  667 Freq table   \n");
		ctable.clock_table_size = ARRAY_SIZE(clock_table_667);
		ctable.clock_table = clock_table_667;
	}else if(ctable.apll_rate == IS_ARM_533) 
	{
		printk("-----------------------------------------  533 Freq table   \n");
		ctable.clock_table_size = ARRAY_SIZE(clock_table_533);
		ctable.clock_table = clock_table_533;	
	}

*/
	printk(KERN_INFO "S5P6450: PLL settings, A=%ld.%ldMHz, M=%ld.%ldMHz," \
			" E=%ld.%ldMHz, D=%ld.%ldMHz\n",
			print_mhz(apll), print_mhz(mpll), print_mhz(epll), print_mhz(dpll));

	fclk = clk_get_rate(&clk_armclk.clk);
	hclk = clk_get_rate(&clk_hclk166.clk);
	pclk = clk_get_rate(&clk_pclk83.clk);
	hclk_low = clk_get_rate(&clk_hclk133.clk);
	pclk_low = clk_get_rate(&clk_pclk66.clk);

	printk(KERN_INFO "S5P6450: HCLK166=%ld.%ldMHz, HCLK133=%ld.%ldMHz," \
			" PCLK83=%ld.%ldMHz, PCLK66=%ld.%ldMHz\n",
			print_mhz(hclk), print_mhz(hclk_low),
			print_mhz(pclk), print_mhz(pclk_low));

	clk_f.rate = fclk;
	clk_h.rate = hclk;
	clk_p.rate = pclk;
	clk_h_low.rate = hclk_low;

	for (ptr = 0; ptr < ARRAY_SIZE(clksrc_audio); ptr++)
		s3c_set_clksrc(clksrc_audio + ptr, true);

	for (ptr = 0; ptr < ARRAY_SIZE(clksrcs); ptr++)
		s3c_set_clksrc(&clksrcs[ptr], true);
}

void __init s5p6450_register_clocks(void)
{
	struct clk *clkp;
	struct clksrc_clk *clks;
	int ret;
	int ptr;
	
	for (ptr = 0; ptr < ARRAY_SIZE(sysclks); ptr++)
		s3c_register_clksrc(sysclks[ptr], 1);
	s3c_register_clksrc(clksrc_audio, ARRAY_SIZE(clksrc_audio));
	s3c_register_clksrc(clksrcs, ARRAY_SIZE(clksrcs));
	s3c_register_clocks(init_clocks, ARRAY_SIZE(init_clocks));
	clkp = init_clocks_disable;
	for (ptr = 0; ptr < ARRAY_SIZE(init_clocks_disable); ptr++, clkp++) {

		ret = s3c24xx_register_clock(clkp);
		if (ret < 0) {
			printk(KERN_ERR "Failed to register clock %s (%d)\n",
			       clkp->name, ret);
		}
		(clkp->enable)(clkp, 0);
	}
	clks = clksrcs + 1; // A one is added here to intensionally keep the sclk_mali clk enabled.
	for (ptr = 0; ptr < (ARRAY_SIZE(clksrcs)-1); ptr++,clks++)
	{
		clks->clk.enable(&clks->clk,0);	
	}
	s3c_pwmclk_init();
}
