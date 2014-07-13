/* sound/soc/s3c24xx/s3c64xx-i2s.c
 *
 * ALSA SoC Audio Layer - S3C64XX I2S driver
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *      Ben Dooks <ben@simtec.co.uk>
 *      http://armlinux.simtec.co.uk/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/delay.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/clk.h>
#include <linux/io.h>

#include <sound/soc.h>

#include <plat/regs-iis.h>
#include <plat/regs-s3c2412-iis.h>
#include <plat/audio.h>

#include <mach/dma.h>
//#include <mach/pd.h>

#include <mach/map.h>
#define S3C_VA_AUDSS    S3C_ADDR(0x01600000)    /* Audio SubSystem */
//#include <mach/regs-audss.h>

#include "s3c-dma.h"
#include "s3c64xx-i2s.h"

/* The value should be set to maximum of the total number
 * of I2Sv3 controllers that any supported SoC has.
 */
#define MAX_I2SV3 	3	

extern void s5p_i2s_sec_init(void *, dma_addr_t);

static struct s3c2410_dma_client s3c64xx_dma_client_out = {
	.name		= "I2S PCM Stereo out"
};

static struct s3c2410_dma_client s3c64xx_dma_client_in = {
	.name		= "I2S PCM Stereo in"
};

static struct snd_soc_dai_ops s3c64xx_i2s_dai_ops;
static struct s3c_dma_params s3c64xx_i2s_pcm_stereo_out[MAX_I2SV3];
static struct s3c_dma_params s3c64xx_i2s_pcm_stereo_in[MAX_I2SV3];
static struct s3c_i2sv2_info s3c64xx_i2s[MAX_I2SV3];

struct snd_soc_dai s3c64xx_i2s_dai[MAX_I2SV3];
EXPORT_SYMBOL_GPL(s3c64xx_i2s_dai);

/* For I2S Clock/Power Gating */
#define POWERUP_UDELAY	50
#if 0
static char *i2s_pd_name[3] = { "i2s0_pd", "i2s1_pd", "i2s2_pd" };
#endif
static int tx_clk_enabled = 0;
static int rx_clk_enabled = 0;
static int reg_saved_ok = 0;

static inline struct s3c_i2sv2_info *to_info(struct snd_soc_dai *cpu_dai)
{
	return cpu_dai->private_data;
}

struct clk *s3c64xx_i2s_get_clock(struct snd_soc_dai *dai)
{
	struct s3c_i2sv2_info *i2s = to_info(dai);
	u32 iismod = readl(i2s->regs + S3C2412_IISMOD);

	if (iismod & S3C64XX_IISMOD_IMS_SYSMUX)
		return i2s->iis_cclk;
	else
		return i2s->iis_pclk;
}
EXPORT_SYMBOL_GPL(s3c64xx_i2s_get_clock);

void s5p_i2s_set_clk_enabled(struct snd_soc_dai *dai, bool state)
{
	static int audio_clk_gated = 0; /* At first, clock & i2s0_pd is enabled in probe() */

	struct s3c_i2sv2_info *i2s = to_info(dai);

	pr_debug("..entering %s \n", __func__);

	if (i2s->iis_cclk == NULL) return;

	if (state) {
		if (!audio_clk_gated) {
			pr_debug("already audio clock is enabled! \n");
			return;
		}

		//s5pv210_pd_enable(i2s_pd_name[dai->id]);
		//udelay(POWERUP_UDELAY);

		clk_enable(i2s->sclk_audio);
		clk_enable(i2s->iis_cclk);
		clk_enable(i2s->iis_pclk);
		audio_clk_gated = 0;
	} else {
		if (audio_clk_gated) {
			pr_debug("already audio clock is gated! \n");
			return;
		}
		
		clk_disable(i2s->iis_pclk);
		clk_disable(i2s->iis_cclk);
		clk_disable(i2s->sclk_audio);

		//s5pv210_pd_disable(i2s_pd_name[dai->id]);

		audio_clk_gated = 1;
	}
}

static int s5p_i2s_wr_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params, struct snd_soc_dai *dai)
{
#ifdef CONFIG_S5P_LPAUDIO
	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		s5p_i2s_hw_params(substream, params, dai);
	} else {
		s3c2412_i2s_hw_params(substream, params, dai);
	}
#else
	s3c2412_i2s_hw_params(substream, params, dai);
#endif
	return 0;
}
static int s5p_i2s_wr_trigger(struct snd_pcm_substream *substream,
				int cmd, struct snd_soc_dai *dai)
{
#ifdef CONFIG_S5P_LPAUDIO
	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		s5p_i2s_trigger(substream, cmd, dai);
	} else {
		s3c2412_i2s_trigger(substream, cmd, dai);
	}
#else
	s3c2412_i2s_trigger(substream, cmd, dai);
#endif
	return 0;
} 

/*
 * Set S3C2412 I2S DAI format
 */
static int s5p_i2s_set_fmt(struct snd_soc_dai *cpu_dai,
			       unsigned int fmt)
{
	struct s3c_i2sv2_info *i2s = to_info(cpu_dai);
	u32 iismod;

	pr_debug("Entered %s\n", __func__);

	iismod = readl(i2s->regs + S3C2412_IISMOD);
	pr_debug("hw_params r: IISMOD: %x \n", iismod);

#if defined(CONFIG_CPU_S3C2412) || defined(CONFIG_CPU_S3C2413)
#define IISMOD_MASTER_MASK S3C2412_IISMOD_MASTER_MASK
#define IISMOD_SLAVE S3C2412_IISMOD_SLAVE
#define IISMOD_MASTER S3C2412_IISMOD_MASTER_INTERNAL
#endif

#if defined(CONFIG_PLAT_S3C64XX) || defined(CONFIG_PLAT_S5P)
/* From Rev1.1 datasheet, we have two master and two slave modes:
 * IMS[11:10]:
 *	00 = master mode, fed from PCLK
 *	01 = master mode, fed from CLKAUDIO
 *	10 = slave mode, using PCLK
 *	11 = slave mode, using I2SCLK
 */
#define IISMOD_MASTER_MASK (1 << 11)
#define IISMOD_SLAVE (1 << 11)
#define IISMOD_MASTER (0 << 11)
#endif

	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
		case SND_SOC_DAIFMT_CBM_CFM:
			i2s->master = 0;
			iismod &= ~IISMOD_MASTER_MASK;
			iismod |= IISMOD_SLAVE;
			break;
		case SND_SOC_DAIFMT_CBS_CFS:
			i2s->master = 1;
			iismod &= ~IISMOD_MASTER_MASK;
			iismod |= IISMOD_MASTER;
			break;
		default:
			pr_err("unknwon master/slave format\n");
			return -EINVAL;
	}

	iismod &= ~S3C2412_IISMOD_SDF_MASK;

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
		case SND_SOC_DAIFMT_RIGHT_J:
			iismod |= S3C2412_IISMOD_LR_RLOW;
			iismod |= S3C2412_IISMOD_SDF_MSB;
			break;
		case SND_SOC_DAIFMT_LEFT_J:
			iismod |= S3C2412_IISMOD_LR_RLOW;
			iismod |= S3C2412_IISMOD_SDF_LSB;
			break;
		case SND_SOC_DAIFMT_I2S:
			iismod &= ~S3C2412_IISMOD_LR_RLOW;
			iismod |= S3C2412_IISMOD_SDF_IIS;
			break;
		default:
			pr_err("Unknown data format\n");
			return -EINVAL;
	}

	writel(iismod, i2s->regs + S3C2412_IISMOD);
	pr_debug("hw_params w: IISMOD: %x \n", iismod);
	return 0;
}

static int s5p_i2s_set_clkdiv(struct snd_soc_dai *cpu_dai,
				  int div_id, int div)
{
	struct s3c_i2sv2_info *i2s = to_info(cpu_dai);
	u32 reg;

	pr_debug("%s(%p, %d, %d)\n", __func__, cpu_dai, div_id, div);

	switch (div_id) {
		case S3C_I2SV2_DIV_BCLK:
			if (div > 3) {
				/* convert value to bit field */

				switch (div) {
					case 16:
						div = S3C2412_IISMOD_BCLK_16FS;
						break;

					case 32:
						div = S3C2412_IISMOD_BCLK_32FS;
						break;

					case 24:
						div = S3C2412_IISMOD_BCLK_24FS;
						break;

					case 48:
						div = S3C2412_IISMOD_BCLK_48FS;
						break;

					default:
						return -EINVAL;
				}
			}

			reg = readl(i2s->regs + S3C2412_IISMOD);
			reg &= ~S3C2412_IISMOD_BCLK_MASK;
			writel(reg | div, i2s->regs + S3C2412_IISMOD);

			pr_debug("%s: MOD=%08x\n", __func__, readl(i2s->regs + S3C2412_IISMOD));
			break;

		case S3C_I2SV2_DIV_RCLK:
			if (div > 3) {
				/* convert value to bit field */

				switch (div) {
					case 256:
						div = S3C2412_IISMOD_RCLK_256FS;
						break;

					case 384:
						div = S3C2412_IISMOD_RCLK_384FS;
						break;

					case 512:
						div = S3C2412_IISMOD_RCLK_512FS;
						break;

					case 768:
						div = S3C2412_IISMOD_RCLK_768FS;
						break;

					default:
						return -EINVAL;
				}
			}

			reg = readl(i2s->regs + S3C2412_IISMOD);
			reg &= ~S3C2412_IISMOD_RCLK_MASK;
			writel(reg | div, i2s->regs + S3C2412_IISMOD);
			pr_debug("%s: MOD=%08x\n", __func__, readl(i2s->regs + S3C2412_IISMOD));
			break;

		case S3C_I2SV2_DIV_PRESCALER:
			if (div >= 0) {
				writel((div << 8) | S3C2412_IISPSR_PSREN,
					i2s->regs + S3C2412_IISPSR);
			} else {
				writel(0x0, i2s->regs + S3C2412_IISPSR);
			}
			pr_debug("%s: PSR=%08x\n", __func__, readl(i2s->regs + S3C2412_IISPSR));
			break;

		default:
			return -EINVAL;
	}

	return 0;
}

static int s5p_i2s_set_sysclk(struct snd_soc_dai *cpu_dai,
				  int clk_id, unsigned int freq, int dir)
{
	struct s3c_i2sv2_info *i2s = to_info(cpu_dai);
	u32 iismod = readl(i2s->regs + S3C2412_IISMOD);

	switch (clk_id) {
		case S3C64XX_CLKSRC_PCLK:
			iismod &= ~S3C64XX_IISMOD_IMS_SYSMUX;
			break;

		case S3C64XX_CLKSRC_MUX:
			iismod |= S3C64XX_IISMOD_IMS_SYSMUX;
			break;

		case S3C64XX_CLKSRC_CDCLK:
			switch (dir) {
				case SND_SOC_CLOCK_IN:
					iismod |= S3C64XX_IISMOD_CDCLKCON;
					break;
				case SND_SOC_CLOCK_OUT:
					iismod &= ~S3C64XX_IISMOD_CDCLKCON;
					break;
				default:
					return -EINVAL;
			}
			break;

		default:
			return -EINVAL;
	}

	writel(iismod, i2s->regs + S3C2412_IISMOD);

	return 0;
}

static int s5p_i2s_wr_startup(struct snd_pcm_substream *substream,
				struct snd_soc_dai *dai)
{
	struct s3c_i2sv2_info *i2s = to_info(dai);

	s5p_i2s_set_clk_enabled(dai, 1);
	if (!tx_clk_enabled && !rx_clk_enabled) {
		if (reg_saved_ok) {
			writel(i2s->suspend_iismod, i2s->regs + S3C2412_IISMOD);
			writel(i2s->suspend_iiscon, i2s->regs + S3C2412_IISCON);
			writel(i2s->suspend_iispsr, i2s->regs + S3C2412_IISPSR);
			writel(i2s->suspend_iisahb, i2s->regs + S5P_IISAHB);
			/* Is this dai for I2Sv5? (I2S0) */
			if (dai->id == 0) {

#if 0			//TODO	
				writel(i2s->suspend_audss_clksrc, S5P_CLKSRC_AUDSS);
				writel(i2s->suspend_audss_clkdiv, S5P_CLKDIV_AUDSS);
				writel(i2s->suspend_audss_clkgate, S5P_CLKGATE_AUDSS);
#endif
			}
			reg_saved_ok = 0;
			pr_debug("I2S Audio Clock enabled and Registers restored...\n");
		}
	}

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		tx_clk_enabled = 1;
	} else {
		rx_clk_enabled = 1;
	}

#ifdef CONFIG_S5P_LPAUDIO
	if(substream->stream == SNDRV_PCM_STREAM_PLAYBACK) 
		s5p_i2s_startup(substream, dai);	
#endif
	return 0;
}

static void s5p_i2s_wr_shutdown(struct snd_pcm_substream *substream, struct snd_soc_dai *dai)
{
	struct s3c_i2sv2_info *i2s = to_info(dai);

	if (substream->stream == SNDRV_PCM_STREAM_PLAYBACK) {
		tx_clk_enabled = 0;
	} else {
		rx_clk_enabled = 0;
	}

	if (!tx_clk_enabled && !rx_clk_enabled) {
		i2s->suspend_iismod = readl(i2s->regs + S3C2412_IISMOD);
		i2s->suspend_iiscon = readl(i2s->regs + S3C2412_IISCON);
		i2s->suspend_iispsr = readl(i2s->regs + S3C2412_IISPSR);
		i2s->suspend_iisahb = readl(i2s->regs + S5P_IISAHB);
		/* Is this dai for I2Sv5? (I2S0) */
		if (dai->id == 0) {
#if 0	//TODO
			i2s->suspend_audss_clksrc = readl(S5P_CLKSRC_AUDSS);
			i2s->suspend_audss_clkdiv = readl(S5P_CLKDIV_AUDSS);
			i2s->suspend_audss_clkgate = readl(S5P_CLKGATE_AUDSS);
#endif
		}
		reg_saved_ok = 1;
		s5p_i2s_set_clk_enabled(dai, 0);
		pr_debug("I2S Audio Clock disabled and Registers stored...\n");
	}

	return;
}

#define S3C64XX_I2S_RATES \
	(SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_11025 | SNDRV_PCM_RATE_16000 | \
	SNDRV_PCM_RATE_22050 | SNDRV_PCM_RATE_32000 | SNDRV_PCM_RATE_44100 | \
	SNDRV_PCM_RATE_48000 | SNDRV_PCM_RATE_88200 | SNDRV_PCM_RATE_96000 | \
	SNDRV_PCM_RATE_KNOT)

#define S3C64XX_I2S_FMTS \
	(SNDRV_PCM_FMTBIT_S8 | SNDRV_PCM_FMTBIT_S16_LE |\
	 SNDRV_PCM_FMTBIT_S24_LE)

static void s3c64xx_iis_dai_init(struct snd_soc_dai *dai)
{	
	dai->name = "s3c64xx-i2s";
	dai->symmetric_rates = 1;
	dai->playback.channels_min = 2;
	dai->playback.channels_max = 2;
	dai->playback.rates = S3C64XX_I2S_RATES;
	dai->playback.formats = S3C64XX_I2S_FMTS;
	dai->capture.channels_min = 2;
	dai->capture.channels_max = 2;
	dai->capture.rates = S3C64XX_I2S_RATES;
	dai->capture.formats = S3C64XX_I2S_FMTS;
	dai->ops = &s3c64xx_i2s_dai_ops;
}

//#if 1	/* suspend/resume are not necessary due to Clock/Pwer gating scheme... */
#ifdef CONFIG_PM
#include <mach/map.h>
//#define S3C_VA_AUDSS	S3C_ADDR(0x01600000)	/* Audio SubSystem */
//#include <mach/regs-audss.h>
static int s5p_i2s_suspend(struct snd_soc_dai *dai)
{
	struct s3c_i2sv2_info *i2s = to_info(dai);
	u32 iismod;

	i2s->suspend_iismod = readl(i2s->regs + S3C2412_IISMOD);
	i2s->suspend_iiscon = readl(i2s->regs + S3C2412_IISCON);
	i2s->suspend_iispsr = readl(i2s->regs + S3C2412_IISPSR);
	i2s->suspend_iisahb = readl(i2s->regs + S5P_IISAHB);

	/* some basic suspend checks */

	iismod = readl(i2s->regs + S3C2412_IISMOD);

	if (iismod & S3C2412_IISCON_RXDMA_ACTIVE)
		pr_warning("%s: RXDMA active?\n", __func__);

	if (iismod & S3C2412_IISCON_TXDMA_ACTIVE)
		pr_warning("%s: TXDMA active?\n", __func__);

	if (iismod & S3C2412_IISCON_IIS_ACTIVE)
		pr_warning("%s: IIS active\n", __func__);

	return 0;
}

static int s5p_i2s_resume(struct snd_soc_dai *dai)
{
	struct s3c_i2sv2_info *i2s = to_info(dai);

	pr_info("dai_active %d, IISMOD %08x, IISCON %08x\n",
		dai->active, i2s->suspend_iismod, i2s->suspend_iiscon);

	writel(i2s->suspend_iiscon, i2s->regs + S3C2412_IISCON);
	writel(i2s->suspend_iismod, i2s->regs + S3C2412_IISMOD);
	writel(i2s->suspend_iispsr, i2s->regs + S3C2412_IISPSR);
	writel(i2s->suspend_iisahb, i2s->regs + S5P_IISAHB);

	writel(S3C2412_IISFIC_RXFLUSH | S3C2412_IISFIC_TXFLUSH,
		i2s->regs + S3C2412_IISFIC);

	ndelay(250);
	writel(0x0, i2s->regs + S3C2412_IISFIC);

	return 0;
}
#else
#define s5p_i2s_suspend NULL
#define s5p_i2s_resume  NULL
#endif	/* CONFIG_PM */
//#endif

int s5p_i2sv5_register_dai(struct snd_soc_dai *dai)
{
	struct snd_soc_dai_ops *ops = dai->ops;

	ops->trigger = s5p_i2s_wr_trigger;
	ops->hw_params = s5p_i2s_wr_hw_params;
	ops->set_fmt = s5p_i2s_set_fmt;
	ops->set_clkdiv = s5p_i2s_set_clkdiv;
	ops->set_sysclk = s5p_i2s_set_sysclk;
	ops->startup   = s5p_i2s_wr_startup;
	ops->shutdown = s5p_i2s_wr_shutdown;
#if 1	/* suspend/resume are not necessary due to Clock/Pwer gating scheme... */
	dai->suspend = s5p_i2s_suspend;
	dai->resume = s5p_i2s_resume;
#endif
	return snd_soc_register_dai(dai);
}

static __devinit int s3c64xx_iis_dev_probe(struct platform_device *pdev)
{
	struct s3c_audio_pdata *i2s_pdata;
	struct s3c_i2sv2_info *i2s;
	struct snd_soc_dai *dai;
	struct resource *res;
	struct clk *fout_epll, *mout_audss;
	unsigned long base;
	unsigned int  iismod;
	int ret;

	if (pdev->id >= MAX_I2SV3) {
		dev_err(&pdev->dev, "id %d out of range\n", pdev->id);
		return -EINVAL;
	}
#if 0 
	s5pv210_pd_enable(i2s_pd_name[pdev->id]);	/* Enable Power domain */
	udelay(POWERUP_UDELAY);
#endif
	i2s = &s3c64xx_i2s[pdev->id];
	dai = &s3c64xx_i2s_dai[pdev->id];
	dai->dev = &pdev->dev;
	dai->id = pdev->id;

	s3c64xx_iis_dai_init(dai);

	i2s->dma_capture = &s3c64xx_i2s_pcm_stereo_in[pdev->id];
	i2s->dma_playback = &s3c64xx_i2s_pcm_stereo_out[pdev->id];

	res = platform_get_resource(pdev, IORESOURCE_DMA, 0);
	if (!res) {
		dev_err(&pdev->dev, "Unable to get I2S-TX dma resource\n");
		return -ENXIO;
	}
	i2s->dma_playback->channel = res->start;

	res = platform_get_resource(pdev, IORESOURCE_DMA, 1);
	if (!res) {
		dev_err(&pdev->dev, "Unable to get I2S-RX dma resource\n");
		return -ENXIO;
	}
	i2s->dma_capture->channel = res->start;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "Unable to get I2S SFR address\n");
		return -ENXIO;
	}

	if (!request_mem_region(res->start, resource_size(res), "s3c64xx-i2s")) {
		dev_err(&pdev->dev, "Unable to request SFR region\n");
		return -EBUSY;
	}

	i2s->dma_capture->dma_addr = res->start + S3C2412_IISRXD;
	i2s->dma_playback->dma_addr = res->start + S3C2412_IISTXD;

	i2s->dma_capture->client = &s3c64xx_dma_client_in;
	i2s->dma_capture->dma_size = 4;
	i2s->dma_playback->client = &s3c64xx_dma_client_out;
	i2s->dma_playback->dma_size = 4;

	i2s_pdata = pdev->dev.platform_data;

	dai->private_data = i2s;
	base = i2s->dma_playback->dma_addr - S3C2412_IISTXD;

	i2s->regs = ioremap(base, 0x100);
	if (i2s->regs == NULL) {
		dev_err(&pdev->dev, "cannot ioremap registers\n");
		return -ENXIO;
	}

	/* Configure the I2S pins if MUX'ed */
	if (i2s_pdata && i2s_pdata->cfg_gpio && i2s_pdata->cfg_gpio(pdev)) {
		dev_err(&pdev->dev, "Unable to configure gpio\n");
		return -EINVAL;
	}
#if 0
	i2s->sclk_audio = clk_get(&pdev->dev, "sclk_audio0");
	if (IS_ERR(i2s->sclk_audio)) {
		dev_err(&pdev->dev, "failed to get sclk_audio0\n");
		ret = PTR_ERR(i2s->sclk_audio);
		iounmap(i2s->regs);
		goto err_sclk_audio;
	}
	clk_enable(i2s->sclk_audio);
#endif
	fout_epll = clk_get(&pdev->dev, "fout_epll");
	if (IS_ERR(fout_epll)) {
		dev_err(&pdev->dev, "failed to get fout_epll\n");
		ret = PTR_ERR(fout_epll);
		iounmap(i2s->regs);
		clk_put(fout_epll);
		goto err_sclk_audio;
	}
	clk_enable(fout_epll);

	mout_audss = clk_get(&pdev->dev, "mout_epll");
	if (IS_ERR(mout_audss)) {
		dev_err(&pdev->dev, "failed to get mout_epll\n");
		ret = PTR_ERR(mout_audss);
		iounmap(i2s->regs);
		clk_put(mout_audss);
		goto err_sclk_audio;
	}
	clk_enable(mout_audss);
	clk_set_parent(mout_audss, fout_epll);

//	i2s->iis_cclk = clk_get(&pdev->dev, "sclk_audio0");
	i2s->iis_cclk = clk_get(&pdev->dev, "audio-bus");
	if (IS_ERR(i2s->iis_cclk)) {
		dev_err(&pdev->dev, "failed to get audio-bus\n");
		ret = PTR_ERR(i2s->iis_cclk);
		iounmap(i2s->regs);
		goto err_cclk;
	}
	clk_set_parent(i2s->iis_cclk, mout_audss);
	clk_enable(i2s->iis_cclk);
#if 0 
#ifdef CONFIG_MACH_VALDEZ
	i2s->iis_pclk = clk_get(&pdev->dev, "iis");
#elif CONFIG_SND_S3C24XX_SOC
	i2s->iis_pclk = clk_get(&pdev->dev, "i2s_v40");
#endif
#endif
	i2s->iis_pclk = clk_get(&pdev->dev, "iis");

	if (IS_ERR(i2s->iis_pclk)) {
		dev_err(&pdev->dev, "failed to get iis_pclock \n");
		ret = PTR_ERR(i2s->iis_pclk);
		iounmap(i2s->regs);
		goto err_pclk;
	}
	clk_enable(i2s->iis_pclk);

#if defined(CONFIG_PLAT_S5P)
	writel(((1<<0)|(1<<31)), i2s->regs + S3C2412_IISCON);
#endif

	/* Mark ourselves as in TXRX mode so we can run through our cleanup
	 * process without warnings. */
	iismod = readl(i2s->regs + S3C2412_IISMOD);
	iismod |= S3C2412_IISMOD_MODE_TXRX;
	writel(iismod, i2s->regs + S3C2412_IISMOD);

#ifdef CONFIG_S5P_LPAUDIO
	s5p_i2s_sec_init(i2s->regs, base);
#endif

	ret = s5p_i2sv5_register_dai(dai);
	if (ret != 0)
		goto err_i2sv5;

#if 0 	/* leave power on due to open new pcm (preallocating dma buffer & codec initializing.) */
	pr_debug("%s: Completed... Now disable clock & power...\n", __FUNCTION__);
	s5p_i2s_set_clk_enabled(dai, 0);
#endif

	return 0;

err_i2sv5:
	/* Not implemented for I2Sv5 core yet */
err_cclk:
	clk_put(i2s->iis_cclk);
err_pclk:
	clk_put(i2s->iis_pclk);
err_sclk_audio:
	clk_put(i2s->sclk_audio);
//err:
	return ret;
}

static __devexit int s3c64xx_iis_dev_remove(struct platform_device *pdev)
{
	dev_err(&pdev->dev, "Device removal not yet supported\n");
	return 0;
}

static struct platform_driver s3c64xx_iis_driver = {
	.probe  = s3c64xx_iis_dev_probe,
	.remove = s3c64xx_iis_dev_remove,
	.driver = {
		.name = "s3c64xx-iis",
		.owner = THIS_MODULE,
	},
};

static int __init s3c64xx_i2s_init(void)
{
	return platform_driver_register(&s3c64xx_iis_driver);
}
module_init(s3c64xx_i2s_init);

static void __exit s3c64xx_i2s_exit(void)
{
	platform_driver_unregister(&s3c64xx_iis_driver);
}
module_exit(s3c64xx_i2s_exit);

/* Module information */
MODULE_AUTHOR("Ben Dooks, <ben@simtec.co.uk>");
MODULE_DESCRIPTION("S3C64XX I2S SoC Interface");
MODULE_LICENSE("GPL");
