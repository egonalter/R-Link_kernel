/* linux/arch/arm/mach-s5pv210/clock.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S5PV210 - Clock support
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

#include <mach/map.h>

#include <plat/cpu-freq.h>
#include <mach/regs-clock.h>
#include <plat/clock.h>
#include <plat/cpu.h>
#include <plat/pll.h>
#include <plat/s5p-clock.h>
#include <plat/clock-clksrc.h>
#include <plat/s5pv210.h>
#include <mach/regs-audss.h>

static int s5pv210_clk_ip0_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_IP0, clk, enable);
}

static int s5pv210_clk_ip1_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_IP1, clk, enable);
}

static int s5pv210_clk_ip2_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_IP2, clk, enable);
}

static int s5pv210_clk_ip3_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_IP3, clk, enable);
}

static int s5pv210_clk_ip4_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_IP4, clk, enable);
}

#if defined(CONFIG_CPU_S5PV210_EVT1)
static int s5pv210_clk_ip5_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_IP5, clk, enable);
}
#endif

static int s5pv210_clk_block_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_BLOCK, clk, enable);
}

static int s5pv210_clk_mask0_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLK_SRC_MASK0, clk, enable);
}

static int s5pv210_clk_mask1_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLK_SRC_MASK1, clk, enable);
}

static int s5pv210_clk_audss_ctrl(struct clk *clk, int enable)
{
	return s5p_gatectrl(S5P_CLKGATE_AUDSS, clk, enable);
}

/* MOUT APLL */
static struct clksrc_clk clk_mout_apll = {
	.clk	= {
		.name		= "mout_apll",
		.id		= -1,
	},
	.sources	= &clk_src_apll,
	.reg_src	= { .reg = S5P_CLK_SRC0, .shift = 0, .size = 1 },
};

/* MOUT MPLL */
static struct clksrc_clk clk_mout_mpll = {
	.clk = {
		.name		= "mout_mpll",
		.id		= -1,
	},
	.sources	= &clk_src_mpll,
	.reg_src	= { .reg = S5P_CLK_SRC0, .shift = 4, .size = 1 },
};

/* MOUT EPLL */
static struct clksrc_clk clk_mout_epll = {
	.clk	= {
		.name		= "mout_epll",
		.id		= -1,
	},
	.sources	= &clk_src_epll,
	.reg_src	= { .reg = S5P_CLK_SRC0, .shift = 8, .size = 1 },
};


/* SCLK HDMI27M */
static struct clk clk_sclk_hdmi27m = {
	.name		= "sclk_hdmi27m",
	.id		= -1,
};

static struct clk *clkset_mout_vpll_src_list[] = {
	[0] = &clk_fin_epll,
	[1] = &clk_sclk_hdmi27m,
};

static struct clksrc_sources clkset_mout_vpll_src = {
	.sources	= clkset_mout_vpll_src_list,
	.nr_sources	= ARRAY_SIZE(clkset_mout_vpll_src_list),
};

static struct clksrc_clk clk_mout_vpll_src = {
	.clk = {
		.name		= "mout_vpll_src",
		.id		= -1,
		.enable		= s5pv210_clk_mask0_ctrl,
		.ctrlbit	= (1 << 7),
	},
	.sources	= &clkset_mout_vpll_src,
	.reg_src	= { .reg = S5P_CLK_SRC1, .shift = 28, .size = 1 },
};

/* MOUT VPLL */
static struct clk *clkset_mout_vpll_list[] = {
	[0] = &clk_mout_vpll_src.clk,
	[1] = &clk_fout_vpll,
};

static struct clksrc_sources clkset_mout_vpll = {
	.sources	= clkset_mout_vpll_list,
	.nr_sources	= ARRAY_SIZE(clkset_mout_vpll_list),
};

static struct clksrc_clk clk_mout_vpll = {
	.clk		= {
		.name		= "mout_vpll",
		.id		= -1,
	},
	.sources	= &clkset_mout_vpll,
	.reg_src	= { .reg = S5P_CLK_SRC0, .shift = 12, .size = 1 },
};

/* SCLK A2M */
static struct clksrc_clk clk_sclk_a2m = {
	.clk = {
		.name		= "sclk_a2m",
		.id		= -1,
		.parent		= &clk_mout_apll.clk,
	},
	.reg_div = { .reg = S5P_CLK_DIV0, .shift = 4, .size = 3 },
};

/* ARMCLK */
static struct clk *clkset_armclk_list[] = {
	[0] = &clk_mout_apll.clk,
	[1] = &clk_mout_mpll.clk,
};

static struct clksrc_sources clkset_armclk = {
	.sources	= clkset_armclk_list,
	.nr_sources	= ARRAY_SIZE(clkset_armclk_list),
};

static struct clksrc_clk clk_armclk = {
	.clk	= {
		.name		= "armclk",
		.id		= -1,
	},
	.sources	= &clkset_armclk,
	.reg_src	= { .reg = S5P_CLK_SRC0, .shift = 16, .size = 1 },
	.reg_div	= { .reg = S5P_CLK_DIV0, .shift = 0, .size = 3 },
};

/* HCLK200 */
static struct clksrc_clk clk_hclk_200 = {
	.clk	= {
		.name		= "hclk_200",
		.id		= -1,
		.parent		= &clk_armclk.clk,
	},
	.reg_div	= { .reg = S5P_CLK_DIV0, .shift = 8, .size = 3 },
};

/* PCLK100 */
static struct clksrc_clk clk_pclk_100 = {
	.clk	= {
		.name		= "pclk_100",
		.id		= -1,
		.parent		= &clk_hclk_200.clk,
	},
	.reg_div	= { .reg = S5P_CLK_DIV0, .shift = 12, .size = 3 },
};

/* MOUT166 */
static struct clk *clkset_mout_166_list[] = {
	[0] = &clk_mout_mpll.clk,
	[1] = &clk_sclk_a2m.clk,
};

static struct clksrc_sources clkset_mout_166 = {
	.sources	= clkset_mout_166_list,
	.nr_sources	= ARRAY_SIZE(clkset_mout_166_list),
};

static struct clksrc_clk clk_mout_166 = {
	.clk	= {
		.name		= "mout_166",
		.id		= -1,
	},
	.sources	= &clkset_mout_166,
	.reg_src	= { .reg = S5P_CLK_SRC0, .shift = 20, .size = 1 },
};

/* HCLK166 */
static struct clksrc_clk clk_hclk_166 = {
	.clk	= {
		.name		= "hclk_166",
		.id		= -1,
		.parent		= &clk_mout_166.clk,
	},
	.reg_div	= { .reg = S5P_CLK_DIV0, .shift = 16, .size = 4 },
};


/* PCLK83 */
static struct clksrc_clk clk_pclk_83 = {
	.clk	= {
		.name		= "pclk_83",
		.id		= -1,
		.parent		= &clk_hclk_166.clk,
	},
	.reg_div = { .reg = S5P_CLK_DIV0, .shift = 20, .size = 3 },
};

/* HCLK133 */
static struct clksrc_clk clk_hclk_133 = {
	.clk	= {
		.name		= "hclk_133",
		.id		= -1,
	},
	.sources	= &clkset_mout_166,
	.reg_src	= { .reg = S5P_CLK_SRC0, .shift = 24, .size = 1 },
	.reg_div	= { .reg = S5P_CLK_DIV0, .shift = 24, .size = 4 },
};

/* PCLK66 */
static struct clksrc_clk clk_pclk_66 = {
	.clk	= {
		.name		= "pclk_66",
		.id		= -1,
		.parent		= &clk_hclk_133.clk,
	},
	.reg_div	= { .reg = S5P_CLK_DIV0, .shift = 28, .size = 3 },
};

/* SCLK ONENAND */
static struct clk *clkset_sclk_onenand_list[] = {
	[0] = &clk_hclk_133.clk,
	[1] = &clk_mout_166.clk,
};

static struct clksrc_sources clkset_sclk_onenand = {
	.sources	= clkset_sclk_onenand_list,
	.nr_sources	= ARRAY_SIZE(clkset_sclk_onenand_list),
};

/* SCLK USBPHY 0 */
static struct clk clk_sclk_usbphy0 = {
	.name		= "sclk_usbphy",
	.id		= 0,
};

/* SCLK USBPHY 1 */
static struct clk clk_sclk_usbphy1 = {
	.name		= "sclk_usbphy",
	.id		= 1,
};

/* SCLK HDMIPHY */
static struct clk clk_sclk_hdmiphy = {
	.name		= "sclk_hdmiphy",
	.id		= -1,
};

/* PCMCDCLK0 */
static struct clk clk_pcmcdclk0 = {
	.name		= "pcmcdclk",
	.id		= 0,
};

/* PCMCDCLK1 */
static struct clk clk_pcmcdclk1 = {
	.name		= "pcmcdclk",
	.id		= 1,
};
/* I2SCDCLK0 */
static struct clk clk_i2scdclk0 = {
	.name		= "i2scdclk",
	.id		= 0,
};

/* SCLK DAC */
static struct clk *clkset_sclk_dac_list[] = {
	[0] = &clk_mout_vpll.clk,
	[1] = &clk_sclk_hdmiphy,
};

static struct clksrc_sources clkset_sclk_dac = {
	.sources	= clkset_sclk_dac_list,
	.nr_sources	= ARRAY_SIZE(clkset_sclk_dac_list),
};

static struct clksrc_clk clk_sclk_dac = {
	.clk		= {
		.name		= "sclk_dac",
		.id		= -1,
		.enable		= s5pv210_clk_mask0_ctrl,
		.ctrlbit	= (1 << 2),
	},
	.sources	= &clkset_sclk_dac,
	.reg_src	= { .reg = S5P_CLK_SRC1, .shift = 8, .size = 1 },
};

/* SCLK PIXEL */
static struct clksrc_clk clk_sclk_pixel = {
	.clk		= {
		.name		= "sclk_pixel",
		.id		= -1,
		.parent		= &clk_mout_vpll.clk,
	},
	.reg_div	= { .reg = S5P_CLK_DIV1, .shift = 0, .size = 4},
};

/* SCLK HDMI */
static struct clk *clkset_sclk_hdmi_list[] = {
	[0] = &clk_sclk_pixel.clk,
	[1] = &clk_sclk_hdmiphy,
};

static struct clksrc_sources clkset_sclk_hdmi = {
	.sources	= clkset_sclk_hdmi_list,
	.nr_sources	= ARRAY_SIZE(clkset_sclk_hdmi_list),
};

static struct clksrc_clk clk_sclk_hdmi = {
	.clk		= {
		.name		= "sclk_hdmi",
		.id		= -1,
		.enable		= s5pv210_clk_mask0_ctrl,
		.ctrlbit	= (1 << 0),
	},
	.sources	= &clkset_sclk_hdmi,
	.reg_src	= { .reg = S5P_CLK_SRC1, .shift = 0, .size = 1 },
};

/* SCLK MIXER */
static struct clk *clkset_sclk_mixer_list[] = {
	[0] = &clk_sclk_dac.clk,
	[1] = &clk_sclk_hdmi.clk,
};

static struct clksrc_sources clkset_sclk_mixer = {
	.sources	= clkset_sclk_mixer_list,
	.nr_sources	= ARRAY_SIZE(clkset_sclk_mixer_list),
};

/* Clock Source Group 1 */
static struct clk *clkset_group1_list[] = {
	[0] = &clk_ext_xtal_mux,
	[1] = &clk_xusbxti,
	[2] = &clk_sclk_hdmi27m,
	[3] = &clk_sclk_usbphy0,
	[4] = &clk_sclk_usbphy1,
	[5] = &clk_sclk_hdmiphy,
	[6] = &clk_mout_mpll.clk,
	[7] = &clk_mout_epll.clk,
	[8] = &clk_mout_vpll.clk,
};

static struct clksrc_sources clkset_group1 = {
	.sources	= clkset_group1_list,
	.nr_sources	= ARRAY_SIZE(clkset_group1_list),
};

/* Clock sources for Audio 1 SCLK */
static struct clk *clkset_mout_audio1_list[] = {
	[0] = &clk_ext_xtal_mux,
	[1] = &clk_pcmcdclk1,
	[2] = &clk_sclk_hdmi27m,
	[3] = &clk_sclk_usbphy0,
	[4] = &clk_sclk_usbphy1,
	[5] = &clk_sclk_hdmiphy,
	[6] = &clk_mout_mpll.clk,
	[7] = &clk_mout_epll.clk,
	[8] = &clk_mout_vpll.clk,
};

static struct clksrc_sources clkset_mout_audio1 = {
	.sources	= clkset_mout_audio1_list,
	.nr_sources	= ARRAY_SIZE(clkset_mout_audio1_list),
};

/* Clock sources for Audio 2 SCLK */
static struct clk *clkset_mout_audio2_list[] = {
	[0] = &clk_ext_xtal_mux,
	[1] = &clk_pcmcdclk0,
	[2] = &clk_sclk_hdmi27m,
	[3] = &clk_sclk_usbphy0,
	[4] = &clk_sclk_usbphy1,
	[5] = &clk_sclk_hdmiphy,
	[6] = &clk_mout_mpll.clk,
	[7] = &clk_mout_epll.clk,
	[8] = &clk_mout_vpll.clk,
};

static struct clksrc_sources clkset_mout_audio2 = {
	.sources	= clkset_mout_audio2_list,
	.nr_sources	= ARRAY_SIZE(clkset_mout_audio2_list),
};

/* Clock Source Group 2 */
static struct clk *clkset_group2_list[] = {
	[0] = &clk_sclk_a2m.clk,
	[1] = &clk_mout_mpll.clk,
	[2] = &clk_mout_epll.clk,
	[3] = &clk_mout_vpll.clk,
};

static struct clksrc_sources clkset_group2 = {
	.sources	= clkset_group2_list,
	.nr_sources	= ARRAY_SIZE(clkset_group2_list),
};

/* MOUT CAM0 */
static struct clksrc_clk clk_mout_cam0 = {
	.clk		= {
		.name		= "mout_cam0",
		.id		= -1,
		.enable		= s5pv210_clk_mask0_ctrl,
		.ctrlbit	= (1 << 3),
	},
	.sources	= &clkset_group1,
	.reg_src	= { .reg = S5P_CLK_SRC1, .shift = 12, .size = 4 },
};

/* MOUT CAM1 */
static struct clksrc_clk clk_mout_cam1 = {
	.clk		= {
		.name		= "mout_cam1",
		.id		= -1,
		.enable		= s5pv210_clk_mask0_ctrl,
		.ctrlbit	= (1 << 4),
	},
	.sources	= &clkset_group1,
	.reg_src	= { .reg = S5P_CLK_SRC1, .shift = 16, .size = 4 },
};

/* MOUT FIMD */
static struct clksrc_clk clk_mout_fimd = {
	.clk		= {
		.name		= "mout_fimd",
		.id		= -1,
		.enable		= s5pv210_clk_mask0_ctrl,
		.ctrlbit	= (1 << 5),
	},
	.sources	= &clkset_group1,
	.reg_src	= { .reg = S5P_CLK_SRC1, .shift = 20, .size = 4 },
};

/* MOUT MMC0 */
static struct clksrc_clk clk_mout_mmc0 = {
	.clk		= {
		.name		= "mout_mmc",
		.id		= 0,
		.enable		= s5pv210_clk_mask0_ctrl,
		.ctrlbit	= (1 << 8),
	},
	.sources	= &clkset_group1,
	.reg_src	= { .reg = S5P_CLK_SRC4, .shift = 0, .size = 4 },
};

/* MOUT MMC1 */
static struct clksrc_clk clk_mout_mmc1 = {
	.clk		= {
		.name		= "mout_mmc",
		.id		= 1,
		.enable		= s5pv210_clk_mask0_ctrl,
		.ctrlbit	= (1 << 9),
	},
	.sources	= &clkset_group1,
	.reg_src	= { .reg = S5P_CLK_SRC4, .shift = 4, .size = 4 },
};

/* MOUT MMC2 */
static struct clksrc_clk clk_mout_mmc2 = {
	.clk		= {
		.name		= "mout_mmc",
		.id		= 2,
		.enable		= s5pv210_clk_mask0_ctrl,
		.ctrlbit	= (1 << 10),
	},
	.sources	= &clkset_group1,
	.reg_src	= { .reg = S5P_CLK_SRC4, .shift = 8, .size = 4 },
};

/* MOUT MMC3 */
static struct clksrc_clk clk_mout_mmc3 = {
	.clk		= {
		.name		= "mout_mmc",
		.id		= 3,
		.enable		= s5pv210_clk_mask0_ctrl,
		.ctrlbit	= (1 << 11),
	},
	.sources	= &clkset_group1,
	.reg_src	= { .reg = S5P_CLK_SRC4, .shift = 12, .size = 4 },
};

/* SCLK MMC0 */
static struct clksrc_clk clk_sclk_mmc0 = {
	.clk		= {
		.name		= "sclk_mmc",
		.id		= 0,
		.parent		= &clk_mout_mmc0.clk,
		.enable		= s5pv210_clk_ip2_ctrl,
		.ctrlbit	= (1 << 16),
	},
	.reg_div = { .reg = S5P_CLK_DIV4, .shift = 0, .size = 4 },
};

/* SCLK MMC1 */
static struct clksrc_clk clk_sclk_mmc1 = {
	.clk		= {
		.name		= "sclk_mmc",
		.id		= 1,
		.parent		= &clk_mout_mmc1.clk,
		.enable		= s5pv210_clk_ip2_ctrl,
		.ctrlbit	= (1 << 17),
	},
	.reg_div = { .reg = S5P_CLK_DIV4, .shift = 4, .size = 4 },
};

/* SCLK MMC2 */
static struct clksrc_clk clk_sclk_mmc2 = {
	.clk		= {
		.name		= "sclk_mmc",
		.id		= 2,
		.parent		= &clk_mout_mmc2.clk,
		.enable		= s5pv210_clk_ip2_ctrl,
		.ctrlbit	= (1 << 18),
	},
	.reg_div = { .reg = S5P_CLK_DIV4, .shift = 8, .size = 4 },
};

/* SCLK MMC3 */
static struct clksrc_clk clk_sclk_mmc3 = {
	.clk		= {
		.name		= "sclk_mmc",
		.id		= 3,
		.parent		= &clk_mout_mmc3.clk,
		.enable		= s5pv210_clk_ip2_ctrl,
		.ctrlbit	= (1 << 19),
	},
	.reg_div = { .reg = S5P_CLK_DIV4, .shift = 12, .size = 4 },
};

/* MOUT AUDIO 0 */
static struct clksrc_clk clk_mout_audio0 = {
	.clk		= {
		.name		= "mout_audio",
		.id		= 0,
		.enable		= s5pv210_clk_mask0_ctrl,
		.ctrlbit	= (1 << 24),
	},
	.sources	= &clkset_group1,
	.reg_src	= { .reg = S5P_CLK_SRC6, .shift = 0, .size = 4 },
};

/* SCLK AUDIO 0 */
static struct clksrc_clk clk_sclk_audio0 = {
	.clk		= {
		.name		= "sclk_audio",
		.id		= 0,
		.parent		= &clk_mout_audio0.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 4),
	},
	.reg_div	= { .reg = S5P_CLK_DIV6, .shift = 0, .size = 4 },
};

static struct clk *clkset_mout_audss_list[] = {
	NULL,
	&clk_fout_epll,
};

static struct clksrc_sources clkset_mout_audss = {
	.sources	= clkset_mout_audss_list,
	.nr_sources	= ARRAY_SIZE(clkset_mout_audss_list),
};

static struct clksrc_clk clk_mout_audss = {
	.clk		= {
		.name		= "mout_audss",
		.id		= -1,
	},
	.sources	= &clkset_mout_audss,
	.reg_src	= { .reg = S5P_CLKSRC_AUDSS, .shift = 0, .size = 1 },
};

static struct clk *clkset_mout_i2s_a_list[] = {
	&clk_mout_audss.clk,
	&clk_i2scdclk0,
	&clk_sclk_audio0.clk,
};

static struct clksrc_sources clkset_mout_i2s_a = {
	.sources	= clkset_mout_i2s_a_list,
	.nr_sources	= ARRAY_SIZE(clkset_mout_i2s_a_list),
};

static struct clksrc_clk clk_mout_i2s_a = {
	.clk		= {
		.name		= "audio-bus",
		.id		= 0,
		.enable		= s5pv210_clk_audss_ctrl,
		.ctrlbit	= (1 << 6),
	},
	.sources	= &clkset_mout_i2s_a,
	.reg_src	= { .reg = S5P_CLKSRC_AUDSS, .shift = 2, .size = 2 },
	.reg_div	= { .reg = S5P_CLKDIV_AUDSS, .shift = 4, .size = 4 },
};

/* MOUT AUDIO 1 */
static struct clksrc_clk clk_mout_audio1 = {
	.clk		= {
		.name		= "mout_audio",
		.id		= 1,
		.enable		= s5pv210_clk_mask0_ctrl,
		.ctrlbit	= (1 << 25),
	},
	.sources	= &clkset_mout_audio1,
	.reg_src	= { .reg = S5P_CLK_SRC6, .shift = 4, .size = 4 },
};

/* SCLK AUDIO 1 */
static struct clksrc_clk clk_sclk_audio1 = {
	.clk		= {
		.name		= "sclk_audio",
		.id		= 1,
		.parent		= &clk_mout_audio1.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 5),
	},
	.reg_div	= { .reg = S5P_CLK_DIV6, .shift = 4, .size = 4 },
};

/* MOUT AUDIO 2 */
static struct clksrc_clk clk_mout_audio2 = {
	.clk		= {
		.name		= "mout_audio",
		.id		= 2,
		.enable		= s5pv210_clk_mask0_ctrl,
		.ctrlbit	= (1 << 26),
	},
	.sources	= &clkset_mout_audio2,
	.reg_src	= { .reg = S5P_CLK_SRC6, .shift = 8, .size = 4 },
};

/* SCLK AUDIO 2 */
static struct clksrc_clk clk_sclk_audio2 = {
	.clk		= {
		.name		= "sclk_audio",
		.id		= 2,
		.parent		= &clk_mout_audio2.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 6),
	},
	.reg_div	= { .reg = S5P_CLK_DIV6, .shift = 8, .size = 4 },
};

static struct clksrc_clk clk_dout_audio_bus_clk_i2s = {
	.clk		= {
		.name		= "dout_audio_bus_clk_i2s",
		.id		= -1,
		.parent 	= &clk_mout_audss,
		.enable 	= s5pv210_clk_audss_ctrl,
		.ctrlbit	= (1 << 5),
	},
	.reg_div	= { .reg = S5P_CLKDIV_AUDSS, .shift = 0, .size = 4 },
};

/* MOUT FIMC LCLK 0 */
static struct clksrc_clk clk_mout_fimc_lclk0 = {
	.clk		= {
		.name		= "mout_fimc_lclk",
		.id		= 0,
		.enable		= s5pv210_clk_mask1_ctrl,
		.ctrlbit	= (1 << 2),
	},
	.sources	= &clkset_group1,
	.reg_src	= { .reg = S5P_CLK_SRC3, .shift = 12, .size = 4 },
};

/* MOUT FIMC LCLK 1 */
static struct clksrc_clk clk_mout_fimc_lclk1 = {
	.clk		= {
		.name		= "mout_fimc_lclk",
		.id		= 1,
		.enable		= s5pv210_clk_mask1_ctrl,
		.ctrlbit	= (1 << 3),
	},
	.sources	= &clkset_group1,
	.reg_src	= { .reg = S5P_CLK_SRC3, .shift = 16, .size = 4 },
};

/* MOUT FIMC LCLK 2 */
static struct clksrc_clk clk_mout_fimc_lclk2 = {
	.clk		= {
		.name		= "mout_fimc_lclk",
		.id		= 2,
		.enable		= s5pv210_clk_mask1_ctrl,
		.ctrlbit	= (1 << 4),
	},
	.sources	= &clkset_group1,
	.reg_src	= { .reg = S5P_CLK_SRC3, .shift = 20, .size = 4 },
};

/* MOUT CSIS */
static struct clksrc_clk clk_mout_csis = {
	.clk		= {
		.name		= "mout_csis",
		.id		= -1,
		.enable		= s5pv210_clk_mask0_ctrl,
		.ctrlbit	= (1 << 6),
	},
	.sources	= &clkset_group1,
	.reg_src	= { .reg = S5P_CLK_SRC1, .shift = 24, .size = 4 },
};

/* MOUT SPI 0 */
static struct clksrc_clk clk_mout_spi0 = {
	.clk		= {
		.name		= "mout_spi",
		.id		= 0,
		.enable		= s5pv210_clk_mask0_ctrl,
		.ctrlbit	= (1 << 16),
	},
	.sources	= &clkset_group1,
	.reg_src	= { .reg = S5P_CLK_SRC5, .shift = 0, .size = 4 },
};

/* MOUT SPI 1 */
static struct clksrc_clk clk_mout_spi1 = {
	.clk		= {
		.name		= "mout_spi",
		.id		= 1,
		.enable		= s5pv210_clk_mask0_ctrl,
		.ctrlbit	= (1 << 17),
	},
	.sources	= &clkset_group1,
	.reg_src	= { .reg = S5P_CLK_SRC5, .shift = 4, .size = 4 },
};

/* MOUT PWI */
static struct clksrc_clk clk_mout_pwi = {
	.clk		= {
		.name		= "mout_pwi",
		.id		= -1,
		.enable		= s5pv210_clk_mask0_ctrl,
		.ctrlbit	= (1 << 29),
	},
	.sources	= &clkset_group1,
	.reg_src	= { .reg = S5P_CLK_SRC6, .shift = 20, .size = 4 },
};

/* MOUT UART 0 */
static struct clksrc_clk clk_mout_uart0 = {
	.clk		= {
		.name		= "mout_uart",
		.id		= 0,
		.enable		= s5pv210_clk_mask0_ctrl,
		.ctrlbit	= (1 << 12),
	},
	.sources	= &clkset_group1,
	.reg_src	= { .reg = S5P_CLK_SRC4, .shift = 16, .size = 4 },
};

/* MOUT UART 1 */
static struct clksrc_clk clk_mout_uart1 = {
	.clk		= {
		.name		= "mout_uart",
		.id		= 1,
		.enable		= s5pv210_clk_mask0_ctrl,
		.ctrlbit	= (1 << 13),
	},
	.sources	= &clkset_group1,
	.reg_src	= { .reg = S5P_CLK_SRC4, .shift = 20, .size = 4 },
};

/* MOUT UART 2 */
static struct clksrc_clk clk_mout_uart2 = {
	.clk		= {
		.name		= "mout_uart",
		.id		= 2,
		.enable		= s5pv210_clk_mask0_ctrl,
		.ctrlbit	= (1 << 14),
	},
	.sources	= &clkset_group1,
	.reg_src	= { .reg = S5P_CLK_SRC4, .shift = 24, .size = 4 },
};

/* MOUT UART 3 */
static struct clksrc_clk clk_mout_uart3 = {
	.clk		= {
		.name		= "mout_uart",
		.id		= 3,
		.enable		= s5pv210_clk_mask0_ctrl,
		.ctrlbit	= (1 << 15),
	},
	.sources	= &clkset_group1,
	.reg_src	= { .reg = S5P_CLK_SRC4, .shift = 28, .size = 4 },
};

/* MOUT PWM */
static struct clksrc_clk clk_mout_pwm = {
	.clk		= {
		.name		= "mout_pwm",
		.id		= -1,
		.enable		= s5pv210_clk_mask0_ctrl,
		.ctrlbit	= (1 << 19),
	},
	.sources	= &clkset_group1,
	.reg_src	= { .reg = S5P_CLK_SRC5, .shift = 12, .size = 4 },
};

/* SCLK SPDIF */
static struct clk *clkset_sclk_spdif_list[] = {
	[0] = &clk_sclk_audio0.clk,
	[1] = &clk_sclk_audio1.clk,
	[2] = &clk_sclk_audio2.clk,
};

static struct clksrc_sources clkset_sclk_spdif = {
	.sources	= clkset_sclk_spdif_list,
	.nr_sources	= ARRAY_SIZE(clkset_sclk_spdif_list),
};

/* MOUT MDNIE */
static struct clksrc_clk clk_mout_mdnie = {
	.clk		= {
		.name		= "mout_mdnie",
		.id		= -1,
		.enable		= s5pv210_clk_mask1_ctrl,
		.ctrlbit	= (1 << 0),
	},
	.sources	= &clkset_group1,
	.reg_src	= { .reg = S5P_CLK_SRC3, .shift = 0, .size = 4 },
};

/* MOUT MDNIE PWM */
static struct clksrc_clk clk_mout_mdnie_pwm = {
	.clk		= {
		.name		= "mout_mdnie_pwm",
		.id		= -1,
		.enable		= s5pv210_clk_mask1_ctrl,
		.ctrlbit	= (1 << 1),
	},
	.sources	= &clkset_group1,
	.reg_src	= { .reg = S5P_CLK_SRC3, .shift = 4, .size = 4 },
};

/* COPY */
static struct clk *clkset_copy_list[] = {
	[6] = &clk_mout_apll.clk,
	[7] = &clk_mout_mpll.clk,
};

static struct clksrc_sources clkset_copy = {
	.sources	= clkset_copy_list,
	.nr_sources	= ARRAY_SIZE(clkset_copy_list),
};

static struct clksrc_clk clk_copy = {
	.clk		= {
		.name		= "copy",
		.id		= -1,
	},
	.sources	= &clkset_copy,
	.reg_src	= { .reg = S5P_CLK_SRC6, .shift = 16, .size = 1 },
	.reg_div	= { .reg = S5P_CLK_DIV6, .shift = 16, .size = 3 },
};

static struct clksrc_clk clksrcs[] = {
	{
		.clk	= {
			.name		= "sclk_onenand",
			.id		= -1,
		},
		.sources = &clkset_sclk_onenand,
		.reg_src = { .reg = S5P_CLK_SRC0, .shift = 28, .size = 1 },
		.reg_div = { .reg = S5P_CLK_DIV6, .shift = 12, .size = 3 },
	}, {
		.clk	= {
			.name		= "sclk_fimc",
			.id		= -1,
			.parent		= &clk_mout_166.clk,
		},
		.reg_div = { .reg = S5P_CLK_DIV1, .shift = 8, .size = 4 },
	}, {
		.clk	= {
			.name		= "sclk_mixer",
			.id		= -1,
			.enable 	= s5pv210_clk_mask0_ctrl,
			.ctrlbit	= (1 << 1),
		},
		.sources = &clkset_sclk_mixer,
		.reg_src = { .reg = S5P_CLK_SRC1, .shift = 4, .size = 1 },
	}, {
		.clk		= {
			.name		= "sclk_cam0",
			.id		= -1,
			.parent		= &clk_mout_cam0.clk,
		},
		.reg_div = { .reg = S5P_CLK_DIV1, .shift = 12, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_cam1",
			.id		= -1,
			.parent		= &clk_mout_cam1.clk,
		},
		.reg_div = { .reg = S5P_CLK_DIV1, .shift = 16, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_fimd",
			.id		= -1,
			.parent		= &clk_mout_fimd.clk,
			.enable		= s5pv210_clk_ip1_ctrl,
			.ctrlbit	= (1 << 0),
		},
		.reg_div = { .reg = S5P_CLK_DIV1, .shift = 20, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_fimc_lclk",
			.id		= 0,
			.parent		= &clk_mout_fimc_lclk0.clk,
			.enable		= s5pv210_clk_ip0_ctrl,
			.ctrlbit	= (1 << 24),
		},
		.reg_div = { .reg = S5P_CLK_DIV3, .shift = 12, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_fimc_lclk",
			.id		= 1,
			.parent		= &clk_mout_fimc_lclk1.clk,
			.enable		= s5pv210_clk_ip0_ctrl,
			.ctrlbit	= (1 << 25),
		},
		.reg_div = { .reg = S5P_CLK_DIV3, .shift = 16, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_fimc_lclk",
			.id		= 2,
			.parent		= &clk_mout_fimc_lclk2.clk,
			.enable		= s5pv210_clk_ip0_ctrl,
			.ctrlbit	= (1 << 26),
		},
		.reg_div = { .reg = S5P_CLK_DIV3, .shift = 20, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_onedram",
			.id		= -1,
			.enable		= s5pv210_clk_ip0_ctrl,
			.ctrlbit	= (1 << 1),
		},
		.sources = &clkset_group2,
		.reg_src = { .reg = S5P_CLK_SRC6, .shift = 24, .size = 2 },
		.reg_div = { .reg = S5P_CLK_DIV6, .shift = 28, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_mfc",
			.id		= -1,
			.enable		= s5pv210_clk_ip0_ctrl,
			.ctrlbit	= (1 << 16),
		},
		.sources = &clkset_group2,
		.reg_src = { .reg = S5P_CLK_SRC2, .shift = 4, .size = 2 },
		.reg_div = { .reg = S5P_CLK_DIV2, .shift = 4, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_g2d",
			.id		= -1,
			.enable		= s5pv210_clk_ip0_ctrl,
			.ctrlbit	= (1 << 12),
		},
		.sources = &clkset_group2,
		.reg_src = { .reg = S5P_CLK_SRC2, .shift = 8, .size = 2 },
		.reg_div = { .reg = S5P_CLK_DIV2, .shift = 8, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_g3d",
			.id		= -1,
			.enable		= s5pv210_clk_ip0_ctrl,
			.ctrlbit	= (1 << 8),
		},
		.sources = &clkset_group2,
		.reg_src = { .reg = S5P_CLK_SRC2, .shift = 0, .size = 2 },
		.reg_div = { .reg = S5P_CLK_DIV2, .shift = 0, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_csis",
			.id		= -1,
			.parent		= &clk_mout_csis.clk,
			.enable		= s5pv210_clk_ip0_ctrl,
			.ctrlbit	= (1 << 31),
		},
		.reg_div = { .reg = S5P_CLK_DIV1, .shift = 28, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_spi",
			.id		= 0,
			.parent		= &clk_mout_spi0.clk,
			.enable		= s5pv210_clk_ip3_ctrl,
			.ctrlbit	= (1 << 12),
		},
		.reg_div = { .reg = S5P_CLK_DIV5, .shift = 0, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_spi",
			.id		= 1,
			.parent		= &clk_mout_spi1.clk,
			.enable		= s5pv210_clk_ip3_ctrl,
			.ctrlbit	= (1 << 13),
		},
		.reg_div = { .reg = S5P_CLK_DIV5, .shift = 4, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_spi",
			.id		= 2,
			.enable		= &s5pv210_clk_ip3_ctrl,
			.ctrlbit	= (1 << 14),
		},
		.sources = &clkset_group1,
		.reg_src = { .reg = S5P_CLK_SRC5, .shift = 8, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_pwi",
			.id		= -1,
			.parent		= &clk_mout_pwi.clk,
			.enable		= &s5pv210_clk_ip4_ctrl,
			.ctrlbit	= (1 << 2),
		},
		.sources = &clkset_group1,
		.reg_div = { .reg = S5P_CLK_DIV6, .shift = 24, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_uart",
			.id		= 0,
			.parent		= &clk_mout_uart0.clk,
			.enable		= s5pv210_clk_ip3_ctrl,
			.ctrlbit	= (1 << 17),
		},
		.reg_div = { .reg = S5P_CLK_DIV4, .shift = 16, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_uart",
			.id		= 1,
			.parent		= &clk_mout_uart1.clk,
			.enable		= s5pv210_clk_ip3_ctrl,
			.ctrlbit	= (1 << 18),
		},
		.reg_div = { .reg = S5P_CLK_DIV4, .shift = 20, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_uart",
			.id		= 2,
			.parent		= &clk_mout_uart2.clk,
			.enable		= s5pv210_clk_ip3_ctrl,
			.ctrlbit	= (1 << 19),
		},
		.reg_div = { .reg = S5P_CLK_DIV4, .shift = 24, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_uart",
			.id		= 3,
			.parent		= &clk_mout_uart3.clk,
			.enable		= s5pv210_clk_ip3_ctrl,
			.ctrlbit	= (1 << 20),
		},
		.reg_div = { .reg = S5P_CLK_DIV4, .shift = 28, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_pwm",
			.id		= -1,
			.parent		= &clk_mout_pwm.clk,
			.enable		= s5pv210_clk_ip3_ctrl,
			.ctrlbit	= (1 << 23),
		},
		.reg_div = { .reg = S5P_CLK_DIV5, .shift = 12, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_spdif",
			.id		= -1,
			.enable		= s5pv210_clk_mask0_ctrl,
			.ctrlbit	= (1 << 27),
		},
		.sources = &clkset_sclk_spdif,
		.reg_src = { .reg = S5P_CLK_SRC6, .shift = 12, .size = 2 },
	}, {
		.clk		= {
			.name		= "sclk_mdnie",
			.id		= -1,
			.parent		= &clk_mout_mdnie.clk,
		},
		.reg_div = { .reg = S5P_CLK_DIV3, .shift = 0, .size = 4 },
	}, {
		.clk		= {
			.name		= "sclk_mdnie_pwm",
			.id		= -1,
			.parent		= &clk_mout_mdnie_pwm.clk,
		},
		.reg_div = { .reg = S5P_CLK_DIV3, .shift = 4, .size = 7 },
	}, {
		.clk		= {
			.name		= "sclk_hpm",
			.id		= -1,
			.parent		= &clk_copy.clk,
		},
		.reg_div = { .reg = S5P_CLK_DIV6, .shift = 20, .size = 3 },
	},
};

static struct clk init_clocks_disable[] = {
	/*Disable: IP0*/
	{
		.name		= "csis",
		.id		= -1,
		.parent		= &clk_pclk_83.clk,
		.enable		= s5pv210_clk_ip0_ctrl,
		.ctrlbit	= (1 << 31),
	}, {
		.name		= "rot",
		.id		= -1,
		.parent		= &clk_hclk_166.clk,
		.enable		= s5pv210_clk_ip0_ctrl,
		.ctrlbit	= (1 << 29),
	}, {
		.name		= "jpeg",
		.id		= -1,
		.parent		= &clk_hclk_166.clk,
#if defined(CONFIG_CPU_S5PV210_EVT1)
		.enable		= s5pv210_clk_ip5_ctrl,
		.ctrlbit	= (1 << 29),
#else
		.enable		= s5pv210_clk_ip0_ctrl,
		.ctrlbit	= (1 << 28),
#endif
	}, {
		.name           = "fimc",
		.id             = 2,
		.parent         = &clk_hclk_166.clk,
		.enable         = s5pv210_clk_ip0_ctrl,
		.ctrlbit        = (1 << 26),
	}, {
		.name           = "fimc",
		.id             = 1,
		.parent         = &clk_hclk_166.clk,
		.enable         = s5pv210_clk_ip0_ctrl,
		.ctrlbit        = (1 << 25),
	}, {
		.name           = "fimc",
		.id             = 0,
		.parent         = &clk_hclk_166.clk,
		.enable         = s5pv210_clk_ip0_ctrl,
		.ctrlbit        = (1 << 24),
	}, {
		.name		= "mfc",
		.id		= -1,
		.parent		= &clk_hclk_200.clk,
		.enable		= s5pv210_clk_ip0_ctrl,
		.ctrlbit	= (1 << 16),
	}, {
		.name		= "g2d",
		.id		= -1,
		.parent		= &clk_hclk_166.clk,
		.enable		= s5pv210_clk_ip0_ctrl,
		.ctrlbit	= (1 << 12),
	},


	/*Disable: IP1*/
	{
		.name		= "nfcon",
		.id		= -1,
		.parent		= &clk_hclk_133.clk,
		.enable		= s5pv210_clk_ip1_ctrl,
		.ctrlbit	= (1 << 28),
	}, {
		.name		= "cfcon",
		.id		= 0,
		.parent		= &clk_hclk_133.clk,
		.enable		= s5pv210_clk_ip1_ctrl,
		.ctrlbit	= (1 << 25),
	}, {
		.name		= "hdmi",
		.id		= -1,
		.parent		= &clk_hclk_166.clk,
		.enable		= s5pv210_clk_ip1_ctrl,
		.ctrlbit	= (1 << 11),
	}, {
		.name		= "tvenc",
		.id		= -1,
		.parent		= &clk_hclk_166.clk,
		.enable		= s5pv210_clk_ip1_ctrl,
		.ctrlbit	= (1 << 10),
	}, {
		.name		= "mixer",
		.id		= -1,
		.parent		= &clk_hclk_166.clk,
		.enable		= s5pv210_clk_ip1_ctrl,
		.ctrlbit	= (1 << 9),
	}, {
		.name		= "vp",
		.id		= -1,
		.parent		= &clk_hclk_166.clk,
		.enable		= s5pv210_clk_ip1_ctrl,
		.ctrlbit	= (1 << 8),
	}, {
		.name		= "dsim",
		.id		= -1,
		.parent		= &clk_hclk_166.clk,
		.enable		= s5pv210_clk_ip1_ctrl,
		.ctrlbit	= (1 << 2),
	}, {
		.name		= "mie",
		.id		= -1,
		.parent		= &clk_hclk_166.clk,
		.enable		= s5pv210_clk_ip1_ctrl,
		.ctrlbit	= (1 << 1),
	},


	/*Disable: IP2*/
	{
		.name		= "tzic3",
		.id		= -1,
		.parent		= &clk_hclk_200.clk,
		.enable		= s5pv210_clk_ip2_ctrl,
		.ctrlbit	= (1 << 31),
	}, {
		.name		= "tzic2",
		.id		= -1,
		.parent		= &clk_hclk_200.clk,
		.enable		= s5pv210_clk_ip2_ctrl,
		.ctrlbit	= (1 << 30),
	}, {
		.name		= "tzic1",
		.id		= -1,
		.parent		= &clk_hclk_200.clk,
		.enable		= s5pv210_clk_ip2_ctrl,
		.ctrlbit	= (1 << 29),
	}, {
		.name		= "tzic0",
		.id		= -1,
		.parent		= &clk_hclk_200.clk,
		.enable		= s5pv210_clk_ip2_ctrl,
		.ctrlbit	= (1 << 28),
	}, {
		.name		= "tsi",
		.id		= -1,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip2_ctrl,
		.ctrlbit	= (1 << 20),
	}, {
		.name		= "secjtag",
		.id		= -1,
		.parent		= &clk_hclk_133.clk,
		.enable		= s5pv210_clk_ip2_ctrl,
		.ctrlbit	= (1 << 11),
	}, {
		.name		= "hostif",
		.id		= -1,
		.parent		= &clk_hclk_133.clk,
		.enable		= s5pv210_clk_ip2_ctrl,
		.ctrlbit	= (1 << 10),
	}, {
		.name		= "modem",
		.id		= -1,
		.parent		= &clk_hclk_133.clk,
		.enable		= s5pv210_clk_ip2_ctrl,
		.ctrlbit	= (1 << 9),
	}, {
		.name		= "sdm",
		.id		= -1,
		.parent		= &clk_hclk_200.clk,
		.enable		= s5pv210_clk_ip2_ctrl,
		.ctrlbit	= (1 << 1),
	}, {
		.name		= "secss",
		.id		= -1,
		.parent		= &clk_hclk_133.clk,
		.enable		= s5pv210_clk_ip2_ctrl,
		.ctrlbit	= (1 << 0),
	},

	/*Disable: IP3*/
	{
		.name		= "pcm",
		.id		= 2,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 30),
	}, {
		.name		= "pcm",
		.id		= 1,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 29),
	}, {
		.name		= "pcm",
		.id		= 0,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 28),
	}, {
		.name		= "adc",
		.id		= -1,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 24),
	}, {
		.name		= "pwm",
		.id		= -1,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 23),
	}, {
		.name		= "watchdog",
		.id		= -1,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 22),
	}, {
		.name		= "keypad",
		.id		= -1,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 21),
	}, {
		.name		= "spi",
		.id		= 2,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 14),
	}, {
		.name		= "spi",
		.id		= 1,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 13),
	}, {
		.name		= "spi",
		.id		= 0,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 12),
	}, {
		.name           = "i2c-hdmiphy",
		.id             = -1,
		.parent         = &clk_pclk_83.clk,
		.enable         = s5pv210_clk_ip3_ctrl,
		.ctrlbit        = (1 << 11),
	}, {
		.name		= "i2c-hdmiddc",
		.id		= -1,
		.parent		= &clk_pclk_83.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 10),
	}, {
		.name		= "iis",
		.id		= 2,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 6),
	}, {
		.name		= "iis",
		.id		= 1,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 5),
	}, {
		.name		= "iis",
		.id		= 0,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 4),
	}, {
		.name		= "ac97",
		.id		= -1,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 1),
	}, {
		.name		= "spdif",
		.id		= -1,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 0),
	},

	/*Disable: IP4*/
	{
		.name		= "tzpc",
		.id		= 0,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip4_ctrl,
		.ctrlbit	= (1 << 8),
	}, {
		.name		= "tzpc",
		.id		= 1,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip4_ctrl,
		.ctrlbit	= (1 << 7),
	}, {
		.name		= "tzpc",
		.id		= 2,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip4_ctrl,
		.ctrlbit	= (1 << 6),
	}, {
		.name		= "tzpc",
		.id		= 3,
		.parent		= &clk_pclk_100.clk,
		.enable		= s5pv210_clk_ip4_ctrl,
		.ctrlbit	= (1 << 5),
	}, {
		.name		= "seckey",
		.id		= -1,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip4_ctrl,
		.ctrlbit	= (1 << 3),
	}, {
		.name	   	= "iem_apc",
		.id	     	= -1,
		.parent	 	= &clk_pclk_66.clk,
		.enable	 	= s5pv210_clk_ip4_ctrl,
		.ctrlbit	= (1 << 2),
	}, {
		.name	   	= "iem_iec",
		.id	     	= -1,
		.parent	 	= &clk_pclk_66.clk,
		.enable	 	= s5pv210_clk_ip4_ctrl,
		.ctrlbit	= (1 << 1),
	},
};

static struct clk init_clocks[] = {
	/*Enable: IP1*/
	{
		.name		= "ipc",
		.id		= -1,
		.parent		= &clk_hclk_166.clk,
		.enable		= s5pv210_clk_ip0_ctrl,
		.ctrlbit	= (1 << 30),
	}, {
		.name		= "g3d",
		.id		= -1,
		.parent		= &clk_hclk_200.clk,
		.enable		= s5pv210_clk_ip0_ctrl,
		.ctrlbit	= (1 << 8),
	}, {
		.name		= "imem",
		.id		= -1,
		.parent		= &clk_pclk_100.clk,
		.enable		= s5pv210_clk_ip0_ctrl,
		.ctrlbit	= (1 << 5),
	}, {
		.name		= "pdma",
		.id		= 1,
		.parent		= &clk_hclk_133.clk,
		.enable		= s5pv210_clk_ip0_ctrl,
		.ctrlbit	= (1 << 4),
	}, {
		.name		= "pdma",
		.id		= 0,
		.parent		= &clk_hclk_133.clk,
		.enable		= s5pv210_clk_ip0_ctrl,
		.ctrlbit	= (1 << 3),
	}, {
		.name		= "mdma",
		.id		= -1,
		.parent		= &clk_hclk_166.clk,
		.enable		= s5pv210_clk_ip0_ctrl,
		.ctrlbit	= (1 << 2),
	}, {
		.name		= "dmc",
		.id		= 1,
		.parent		= &clk_hclk_200.clk,
		.enable		= s5pv210_clk_ip0_ctrl,
		.ctrlbit	= (1 << 1),
	}, {
		.name		= "dmc",
		.id		= 0,
		.parent		= &clk_hclk_200.clk,
		.enable		= s5pv210_clk_ip0_ctrl,
		.ctrlbit	= (1 << 0),
	},

	/*Enable: IP1*/
	{
		.name		= "sromc",
		.id		= -1,
		.parent		= &clk_hclk_133.clk,
		.enable		= s5pv210_clk_ip1_ctrl,
		.ctrlbit	= (1 << 26),
	}, {
		.name		= "nandxl",
		.id		= -1,
		.parent		= &clk_hclk_133.clk,
		.enable		= s5pv210_clk_ip1_ctrl,
		.ctrlbit	= (1 << 24),
	}, {
		.name		= "usbhost",
		.id		= -1,
		.parent		= &clk_hclk_133.clk,
		.enable		= s5pv210_clk_ip1_ctrl,
		.ctrlbit	= (1 << 17),
	}, {
		.name		= "usbotg",
		.id		= -1,
		.parent		= &clk_hclk_133.clk,
		.enable		= s5pv210_clk_ip1_ctrl,
		.ctrlbit	= (1 << 16),
	}, {
		.name		= "fimd",
		.id		= -1,
		.parent		= &clk_hclk_166.clk,
		.enable		= s5pv210_clk_ip1_ctrl,
		.ctrlbit	= (1 << 0),
	},

	/*Enable: IP2*/
	{
		.name		= "vic3",
		.id		= -1,
		.parent		= &clk_hclk_200.clk,
		.enable		= s5pv210_clk_ip2_ctrl,
		.ctrlbit	= (1 << 27),
	}, {
		.name		= "vic2",
		.id		= -1,
		.parent		= &clk_hclk_200.clk,
		.enable		= s5pv210_clk_ip2_ctrl,
		.ctrlbit	= (1 << 26),
	}, {
		.name		= "vic1",
		.id		= -1,
		.parent		= &clk_hclk_200.clk,
		.enable		= s5pv210_clk_ip2_ctrl,
		.ctrlbit	= (1 << 25),
	}, {
		.name		= "vic0",
		.id		= -1,
		.parent		= &clk_hclk_200.clk,
		.enable		= s5pv210_clk_ip2_ctrl,
		.ctrlbit	= (1 << 24),
	}, 	{
		.name		= "hsmmc",
		.id		= 3,
		.parent		= &clk_hclk_133.clk,
		.enable		= s5pv210_clk_ip2_ctrl,
		.ctrlbit	= (1 << 19),
	}, {
		.name		= "hsmmc",
		.id		= 2,
		.parent		= &clk_hclk_133.clk,
		.enable		= s5pv210_clk_ip2_ctrl,
		.ctrlbit	= (1 < 18),
	}, {
		.name		= "hsmmc",
		.id		= 1,
		.parent		= &clk_hclk_133.clk,
		.enable		= s5pv210_clk_ip2_ctrl,
		.ctrlbit	= (1 << 17),
	}, {
		.name		= "hsmmc",
		.id		= 0,
		.parent		= &clk_hclk_133.clk,
		.enable		= s5pv210_clk_ip2_ctrl,
		.ctrlbit	= (1 << 16),
	}, {
		.name		= "coresight",
		.id		= -1,
		.parent		= &clk_hclk_200.clk,
		.enable		= s5pv210_clk_ip2_ctrl,
		.ctrlbit	= (1 << 16),
	},

	/*Enable: IP3*/
	{
		.name		= "syscon",
		.id		= -1,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 27),
	}, {
		.name		= "gpio",
		.id		= -1,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 26),
	}, /*{
		.name		= "adc",
		.id		= -1,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 24),
	}, {
		.name		= "pwm",
		.id		= -1,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 23),
	}, {
		.name		= "watchdog",
		.id		= -1,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 22),
	}, {
		.name		= "keypad",
		.id		= -1,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 21),
	},*/ {
		.name		= "uart",
		.id		= 3,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 20),
	}, {
		.name		= "uart",
		.id		= 2,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 19),
	}, {
		.name		= "uart",
		.id		= 1,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 18),
	}, {
		.name		= "uart",
		.id		= 0,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 17),
	}, {
		.name		= "systimer",
		.id		= -1,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 16),
	}, {
		.name		= "rtc",
		.id		= -1,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 15),
	}, /*{
		.name		= "spi",
		.id		= 2,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 14),
	}, {
		.name		= "spi",
		.id		= 1,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 13),
	}, {
		.name		= "spi",
		.id		= 0,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 12),
	},*/ {
		.name		= "i2c",
		.id		= 2,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 9),
	}, {
		.name		= "i2c",
		.id		= 1,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 8),
	}, {
		.name		= "i2c",
		.id		= 0,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 7),
	},

	/*Enable: IP4*/
	{
		.name		= "chipid",
		.id		= -1,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip4_ctrl,
		.ctrlbit	= (1 << 0),
	},

	/* default enable clocks */
	/*{
		.name		= "uart",
		.id		= 0,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 17),
	}, {
		.name		= "uart",
		.id		= 1,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 18),
	}, {
		.name		= "uart",
		.id		= 2,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 19),
	}, {
		.name		= "uart",
		.id		= 3,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 20),
	}, {
		.name		= "dmc",
		.id		= 1,
		.parent		= &clk_hclk_200.clk,
		.enable		= s5pv210_clk_ip0_ctrl,
		.ctrlbit	= (1 << 1),
	}, {
		.name		= "dmc",
		.id		= 0,
		.parent		= &clk_hclk_200.clk,
		.enable		= s5pv210_clk_ip0_ctrl,
		.ctrlbit	= (1 << 0),
	}, {
		.name		= "syscon",
		.id		= -1,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 27),
	}, {
		.name		= "gpio",
		.id		= -1,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip3_ctrl,
		.ctrlbit	= (1 << 26),
	}, {
		.name		= "chipid",
		.id		= -1,
		.parent		= &clk_pclk_66.clk,
		.enable		= s5pv210_clk_ip4_ctrl,
		.ctrlbit	= (1 << 0),
	}, */

	/*Enable: BLOCK*/
	{
		.name		= "block_intc",
		.id		= -1,
		.enable		= s5pv210_clk_block_ctrl,
		.ctrlbit	= (1 << 10),
	}, {
		.name		= "block_hsmmc",
		.id		= -1,
		.enable		= s5pv210_clk_block_ctrl,
		.ctrlbit	= (1 << 9),
	}, {
		.name		= "block_debug",
		.id		= -1,
		.enable		= s5pv210_clk_block_ctrl,
		.ctrlbit	= (1 << 8),
	}, {
		.name		= "block_security",
		.id		= -1,
		.enable		= s5pv210_clk_block_ctrl,
		.ctrlbit	= (1 << 7),
	}, {
		.name		= "block_memory",
		.id		= -1,
		.enable		= s5pv210_clk_block_ctrl,
		.ctrlbit	= (1 << 6),
	}, {
		.name		= "block_usb",
		.id		= -1,
		.enable		= s5pv210_clk_block_ctrl,
		.ctrlbit	= (1 << 5),
	}, {
		.name		= "block_tv",
		.id		= -1,
		.enable		= s5pv210_clk_block_ctrl,
		.ctrlbit	= (1 << 4),
	}, {
		.name		= "block_lcd",
		.id		= -1,
		.enable		= s5pv210_clk_block_ctrl,
		.ctrlbit	= (1 << 3),
	}, {
		.name		= "block_img",
		.id		= -1,
		.enable		= s5pv210_clk_block_ctrl,
		.ctrlbit	= (1 << 2),
	}, {
		.name		= "block_mfc",
		.id		= -1,
		.enable		= s5pv210_clk_block_ctrl,
		.ctrlbit	= (1 << 1),
	}, {
		.name		= "block_g3d",
		.id		= -1,
		.enable		= s5pv210_clk_block_ctrl,
		.ctrlbit	= (1 << 0),
	},
};

static struct clksrc_clk *sys_clksrc[] = {
	&clk_mout_apll,
	&clk_mout_epll,
	&clk_mout_mpll,
	&clk_armclk,
	&clk_mout_166,
	&clk_sclk_a2m,
	&clk_hclk_200,
	&clk_pclk_100,
	&clk_hclk_166,
	&clk_pclk_83,
	&clk_hclk_133,
	&clk_pclk_66,
	&clk_mout_vpll_src,
	&clk_mout_vpll,
	&clk_sclk_dac,
	&clk_sclk_pixel,
	&clk_sclk_hdmi,
	&clk_mout_cam0,
	&clk_mout_cam1,
	&clk_mout_fimd,
	&clk_mout_mmc0,
	&clk_mout_mmc1,
	&clk_mout_mmc2,
	&clk_mout_mmc3,
	&clk_sclk_mmc0,
	&clk_sclk_mmc1,
	&clk_sclk_mmc2,
	&clk_sclk_mmc3,
	&clk_mout_audio0,
	&clk_sclk_audio0,
	&clk_mout_audio1,
	&clk_sclk_audio1,
	&clk_mout_audio2,
	&clk_sclk_audio2,
	&clk_mout_fimc_lclk0,
	&clk_mout_fimc_lclk1,
	&clk_mout_fimc_lclk2,
	&clk_mout_csis,
	&clk_mout_spi0,
	&clk_mout_spi1,
	&clk_mout_pwi,
	&clk_mout_uart0,
	&clk_mout_uart1,
	&clk_mout_uart2,
	&clk_mout_uart3,
	&clk_mout_pwm,
	&clk_mout_mdnie,
	&clk_mout_mdnie_pwm,
	&clk_copy,
	&clk_mout_i2s_a,
	&clk_mout_audss,
	&clk_dout_audio_bus_clk_i2s,
};



static int s5pv210_epll_enable(struct clk *clk, int enable)
{
	unsigned int ctrlbit = clk->ctrlbit;
	unsigned int epll_con = __raw_readl(S5P_EPLL_CON) & ~ctrlbit;

	if (enable)
		__raw_writel(epll_con | ctrlbit, S5P_EPLL_CON);
	else
		__raw_writel(epll_con, S5P_EPLL_CON);

	return 0;
}

static unsigned long s5pv210_epll_get_rate(struct clk *clk)
{
	return clk->rate;
}

static u32 epll_div[][6] = {
	{  48000000, 0, 48, 3, 3, 0 },
	{  96000000, 0, 48, 3, 2, 0 },
	{ 144000000, 1, 72, 3, 2, 0 },
	{ 192000000, 0, 48, 3, 1, 0 },
	{ 288000000, 1, 72, 3, 1, 0 },
	{  32750000, 1, 65, 3, 4, 35127 },
	{  32768000, 1, 65, 3, 4, 35127 },
	{  45158400, 0, 45, 3, 3, 10355 },
	{  45000000, 0, 45, 3, 3, 10355 },
	{  45158000, 0, 45, 3, 3, 10355 },
	{  49125000, 0, 49, 3, 3, 9961 },
	{  49152000, 0, 49, 3, 3, 9961 },
	{  67737600, 1, 67, 3, 3, 48366 },
	{  67738000, 1, 67, 3, 3, 48366 },
	{  73800000, 1, 73, 3, 3, 47710 },
	{  73728000, 1, 73, 3, 3, 47710 },
	{  36000000, 1, 32, 3, 4, 0 },
	{  60000000, 1, 60, 3, 3, 0 },
	{  72000000, 1, 72, 3, 3, 0 },
	{  80000000, 1, 80, 3, 3, 0 },
	{  84000000, 0, 42, 3, 2, 0 },
	{  50000000, 0, 50, 3, 3, 0 },
};

static int s5pv210_epll_set_rate(struct clk *clk, unsigned long rate)
{
	unsigned int epll_con, epll_con_k;
	unsigned int i;

	/* Return if nothing changed */
	if (clk->rate == rate)
		return 0;

	epll_con = __raw_readl(S5P_EPLL_CON);
	epll_con_k = __raw_readl(S5P_EPLL_CON_K);

	epll_con_k &= ~(PLL90XX_KDIV_MASK);
	epll_con &= ~(PLL90XX_MDIV_MASK << PLL90XX_MDIV_SHIFT |   \
			PLL90XX_PDIV_MASK << PLL90XX_PDIV_SHIFT | \
			PLL90XX_VDIV_MASK << PLL90XX_VDIV_SHIFT | \
			PLL90XX_SDIV_MASK << PLL90XX_SDIV_SHIFT);

	for (i = 0; i < ARRAY_SIZE(epll_div); i++) {
		if (epll_div[i][0] == rate) {
			epll_con_k |= epll_div[i][5] << 0;
			epll_con |= epll_div[i][1] << 27;
			epll_con |= epll_div[i][2] << 16;
			epll_con |= epll_div[i][3] << 8;
			epll_con |= epll_div[i][4] << 0;
			break;
		}
	}

	if (i == ARRAY_SIZE(epll_div)) {
		printk(KERN_ERR "%s: Invalid Clock EPLL Frequency\n",
				__func__);
		return -EINVAL;
	}

	__raw_writel(epll_con, S5P_EPLL_CON);
	__raw_writel(epll_con_k, S5P_EPLL_CON_K);

	clk->rate = rate;

	return 0;
}

static struct clk_ops s5pv210_epll_ops = {
	.get_rate = s5pv210_epll_get_rate,
	.set_rate = s5pv210_epll_set_rate,
};

static int s5pv210_vpll_enable(struct clk *clk, int enable)
{
	unsigned int ctrlbit	= clk->ctrlbit;
	unsigned int vpll_con	= __raw_readl(S5P_VPLL_CON) & ~ctrlbit;

	if (enable)
		__raw_writel(vpll_con | ctrlbit, S5P_VPLL_CON);
	else
		__raw_writel(vpll_con, S5P_VPLL_CON);

	return 0;
}

static unsigned long s5pv210_vpll_get_rate(struct clk *clk)
{
	return clk->rate;
}

static int s5pv210_vpll_set_rate(struct clk *clk, unsigned long rate)
{
	unsigned int vpll_con;

	switch (rate) {
	case 54000000:
		vpll_con = PDIV(6) | MDIV(108) | SDIV(3);
		break;
	case 27000000:
		vpll_con = PDIV(6) | MDIV(108) | SDIV(4);
		break;
	default:
		goto err_on_invalid_rate;
	}
	
	__raw_writel(vpll_con, S5P_VPLL_CON);

	clk->rate = rate;

	return 0;
	
err_on_invalid_rate:
	return -EINVAL;
}

static struct clk_ops s5pv210_vpll_ops = {
	.get_rate = s5pv210_vpll_get_rate,
	.set_rate = s5pv210_vpll_set_rate,
};

#define GET_DIV(clk, field) ((((clk) & field##_MASK) >> field##_SHIFT) + 1)

void __init_or_cpufreq s5pv210_setup_clocks(void)
{
	struct clk *xtal_clk;
	unsigned long xtal;
	unsigned long apll;
	unsigned long mpll;
	unsigned long epll;
	u32 clkdiv0, clkdiv1;
	struct clk *clk_mmc;

	clk_fout_epll.enable = s5pv210_epll_enable;
	clk_fout_epll.ops = &s5pv210_epll_ops;

	clk_fout_vpll.enable	= s5pv210_vpll_enable;
	clk_fout_vpll.ops	= &s5pv210_vpll_ops;

	printk(KERN_DEBUG "%s: registering clocks\n", __func__);

	clkdiv0 = __raw_readl(S5P_CLK_DIV0);
	clkdiv1 = __raw_readl(S5P_CLK_DIV1);

	printk(KERN_DEBUG "%s: clkdiv0 = %08x, clkdiv1 = %08x\n",
				__func__, clkdiv0, clkdiv1);

	xtal_clk = clk_get(NULL, "xtal");
	BUG_ON(IS_ERR(xtal_clk));

	xtal = clk_get_rate(xtal_clk);
	clk_put(xtal_clk);

	printk(KERN_DEBUG "%s: xtal is %ld\n", __func__, xtal);

	apll = s5p_get_pll45xx(xtal, __raw_readl(S5P_APLL_CON), pll_4508);
	mpll = s5p_get_pll45xx(xtal, __raw_readl(S5P_MPLL_CON), pll_4502);
	epll = s5p_get_pll90xx(xtal, __raw_readl(S5P_EPLL_CON),
			__raw_readl(S5P_EPLL_CON_K));

	printk(KERN_INFO "S5PV210: PLL settings, A=%ld, M=%ld, E=%ld\n",
			apll, mpll, epll);

	clk_fout_apll.rate = apll;
	clk_fout_mpll.rate = mpll;
	clk_fout_epll.rate = epll;

	clk_f.rate = clk_get_rate(&clk_armclk.clk);
	clk_h.rate = clk_get_rate(&clk_hclk_133.clk);
	clk_p.rate = clk_get_rate(&clk_pclk_66.clk);

	clk_set_parent(&clk_mout_mmc0.clk, &clk_mout_mpll.clk);
	clk_set_parent(&clk_mout_mmc1.clk, &clk_mout_mpll.clk);
	clk_set_parent(&clk_mout_mmc2.clk, &clk_mout_mpll.clk);
	clk_set_parent(&clk_mout_mmc3.clk, &clk_mout_mpll.clk);

	clk_set_parent(&clk_mout_audio0.clk, &clk_mout_epll.clk);

	clk_set_rate(&clk_sclk_mmc0.clk, 50*MHZ);
	clk_set_rate(&clk_sclk_mmc1.clk, 50*MHZ);
	clk_set_rate(&clk_sclk_mmc2.clk, 50*MHZ);
	clk_set_rate(&clk_sclk_mmc3.clk, 50*MHZ);
}

static struct clk *clks[] __initdata = {
	&clk_sclk_usbphy0,
	&clk_sclk_usbphy1,
	&clk_sclk_hdmiphy,
	&clk_pcmcdclk0,
	&clk_pcmcdclk1,
	&clk_i2scdclk0,
};

void __init s5pv210_register_clocks(void)
{
	struct clk *clkp;
	int ret;
	int ptr;

	ret = s3c24xx_register_clocks(clks, ARRAY_SIZE(clks));
	if (ret > 0)
		printk(KERN_ERR "Failed to register %u clocks\n", ret);

	s3c_register_clksrc(clksrcs, ARRAY_SIZE(clksrcs));
	s3c_register_clocks(init_clocks, ARRAY_SIZE(init_clocks));

	for (ptr = 0; ptr < ARRAY_SIZE(sys_clksrc); ptr++)
		s3c_register_clksrc(sys_clksrc[ptr], 1);

	clkp = init_clocks_disable;
	for (ptr = 0; ptr < ARRAY_SIZE(init_clocks_disable); ptr++, clkp++) {
		ret = s3c24xx_register_clock(clkp);
		if (ret < 0) {
			printk(KERN_ERR "Failed to register clock %s (%d)\n",
			       clkp->name, ret);
		}
		(clkp->enable)(clkp, 0);
	}

	s3c_pwmclk_init();
}
