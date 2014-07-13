/*
 * smdk64xx_s5m8751.c  --  S5M8751 Power-Audio IC ALSA Soc Audio driver
 *
 * Copyright 2009 Samsung Electronics.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/platform_device.h>
#include <linux/clk.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

#include <linux/mfd/s5m8751/core.h>
#include <linux/mfd/s5m8751/audio.h>

#include "s3c-dma.h"
#include "s3c64xx-i2s.h"
#if defined(CONFIG_MACH_S5P6450_TOMTOM)
#define I2S_NUM 2 
#else
#define I2S_NUM 0 
#endif

static int set_epll_rate(unsigned long rate)
{
	struct clk *fout_epll;

	fout_epll = clk_get(NULL, "fout_epll");
	if (IS_ERR(fout_epll)) {
		printk(KERN_ERR "%s: failed to get fout_epll\n", __func__);
		return -ENOENT;
	}

	if (rate == clk_get_rate(fout_epll))
		goto out;

	clk_disable(fout_epll);
	clk_set_rate(fout_epll, rate);
	clk_enable(fout_epll);

out:
	clk_put(fout_epll);

	return 0;
}

static int smdk64xx_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->dai->cpu_dai;
        struct snd_soc_dai *codec_dai = rtd->dai->codec_dai;

	int bfs, rfs, ret;
	unsigned int psr = 1, rclk;
	unsigned long epll_out_rate;

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_U24:
	case SNDRV_PCM_FORMAT_S24:
		bfs = 48;
		break;
	case SNDRV_PCM_FORMAT_U16_LE:
	case SNDRV_PCM_FORMAT_S16_LE:
		bfs = 32;
		break;
	case SNDRV_PCM_FORMAT_U32_LE:
	case SNDRV_PCM_FORMAT_S32_LE:
		bfs = 64;
	default:
		return -EINVAL;
	}
#if 0
	switch (params_rate(params)) {
	case 8000:
	case 12000:
	case 16000:
	case 24000:
	case 32000:
	case 48000:
	case 64000:
	case 96000:
		epll_out_rate = 49152000;
		psr = 12;
		break;
	case 11025:
	case 22050:
	case 44100:
	case 88200:
		epll_out_rate = 67738000;
		break;
	default:
		printk(KERN_ERR "%s:%d Sampling Rate %u not supported!\n",
			__func__, __LINE__, params_rate(params));
		return -EINVAL;
	}

	switch (params_rate(params)) {
	case 16000:
	case 22050:
	case 22025:
	case 32000:
	case 44100:
	case 48000:
	case 96000:
	case 24000:
		rfs = 256;
		break;
	case 64000:
		rfs = 384;
		break;
	case 8000:
	case 11025:
	case 12000:
		rfs = 512;
		break;
	case 88200:
		rfs = 128;
		break;
	default:
		printk(KERN_ERR "%s:%d Sampling Rate %u not supported!\n",
			__func__, __LINE__, params_rate(params));
		return -EINVAL;
	}

	ret = set_epll_rate(epll_out_rate);
#else
        switch (params_rate(params)) {
        case 16000:
        case 22050:
        case 24000:
        case 32000:
        case 44100:
        case 48000:
        case 88200:
        case 96000:
                if (bfs == 48)
                        rfs = 384;
                else
                        rfs = 256;
                break;
        case 64000:
                rfs = 384;
                break;
        case 8000:
        case 11025:
        case 12000:
                if (bfs == 48)
                        rfs = 768;
                else
                        rfs = 512;
                break;
        default:
                return -EINVAL;
        }

        rclk = params_rate(params) * rfs;
        switch (rclk) {
        case 4096000:
        case 5644800:
        case 6144000:
        case 8467200:
        case 9216000:
                psr = 8;
                break;
        case 8192000:
        case 11289600:
        case 12288000:
        case 16934400:
        case 18432000:
                psr = 4;
                break;
        case 22579200:
        case 24576000:
        case 33868800:
        case 36864000:
                psr = 2;
                break;
        case 67737600:
        case 73728000:
                psr = 1;
                break;
        default:
                printk("Not yet supported!\n");
                return -EINVAL;
        }

        ret = set_epll_rate(rclk * psr);

#endif
	if (ret < 0)
		return ret;

	/* Set the Codec DAI configuration */
	ret = snd_soc_dai_set_fmt(codec_dai,
					SND_SOC_DAIFMT_I2S
					 | SND_SOC_DAIFMT_NB_NF
					 | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;
	/* Set the AP DAI configuration */
	ret = snd_soc_dai_set_fmt(cpu_dai,
					SND_SOC_DAIFMT_I2S
					 | SND_SOC_DAIFMT_NB_NF
					 | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;
		
	ret = snd_soc_dai_set_sysclk(cpu_dai, S3C64XX_CLKSRC_CDCLK,
					0, SND_SOC_CLOCK_OUT);
	if (ret < 0)
		return ret;

	/* We use MUX for basic ops in SoC-Master mode */
	ret = snd_soc_dai_set_sysclk(cpu_dai, S3C64XX_CLKSRC_MUX,
					0, SND_SOC_CLOCK_IN);
	if (ret < 0)
		return ret;

        ret = snd_soc_dai_set_clkdiv(cpu_dai, S3C_I2SV2_DIV_PRESCALER, psr-1);
        if (ret < 0)
                return ret;

	ret = snd_soc_dai_set_clkdiv(cpu_dai, S3C_I2SV2_DIV_BCLK, bfs);
	if (ret < 0)
		return ret;

	ret = snd_soc_dai_set_clkdiv(cpu_dai, S3C_I2SV2_DIV_RCLK, rfs);
	if (ret < 0)
		return ret;


	return 0;
}

/*
 * SMDK64XX S5M8751 DAI operations.
 */
static struct snd_soc_ops smdk64xx_ops = {
	.hw_params = smdk64xx_hw_params,
};

/* SMDK64xx Playback widgets */
static const struct snd_soc_dapm_widget s5m8751_dapm_widgets[] = {
	SND_SOC_DAPM_HP("Headphones", NULL),
	SND_SOC_DAPM_LINE("LineOut", NULL),
	SND_SOC_DAPM_SPK("Speaker", NULL),
};

/* SMDK-PAIFRX connections */
static const struct snd_soc_dapm_route audio_map[] = {
	{"Headphones", NULL, "LH"},
	{"Headphones", NULL, "RH"},

	{"Speaker", NULL, "SPK"},

	{"LineOut", NULL, "LL"},
	{"LineOut", NULL, "RL"},
};

int smdk64xx_s5m8751_init(struct snd_soc_codec *codec)
{
	int ret;

	/* Add smdk64xx specific Playback widgets */
	snd_soc_dapm_new_controls(codec, s5m8751_dapm_widgets,
				  ARRAY_SIZE(s5m8751_dapm_widgets));

	/* set up cordoba specific audio paths */
	snd_soc_dapm_add_routes(codec, audio_map, ARRAY_SIZE(audio_map));

	/* always connected */
	snd_soc_dapm_enable_pin(codec, "Headphones");
	snd_soc_dapm_enable_pin(codec, "Speaker");
	snd_soc_dapm_enable_pin(codec, "LineOut");

	snd_soc_dapm_enable_pin(codec, "RL");
	snd_soc_dapm_enable_pin(codec, "LL");
	snd_soc_dapm_enable_pin(codec, "RH");
	snd_soc_dapm_enable_pin(codec, "LH");
	snd_soc_dapm_enable_pin(codec, "SPK");

	/* signal a DAPM event */
	snd_soc_dapm_sync(codec);

	return 0;
}

int smdk64xx_tlv320adc3101_init_tx(void *tmp)
{
        return 0;
}


static struct snd_soc_dai_link smdk64xx_dai[] = {
{ /* Primary Playback i/f */
	.name = "S5M8751",
	.stream_name = "Playback",
	//cpu_dai = &s3c64xx_i2s_v4_dai,
        .cpu_dai = &s3c64xx_i2s_dai[I2S_NUM],
	.codec_dai = &s5m8751_dai,
	.init = smdk64xx_s5m8751_init,
	.ops = &smdk64xx_ops,
},
{ /* Primary Capture i/f */
        .name = "TLV320ADC3101 I2S TX",
        .stream_name = "Capture",
        .cpu_dai = &s3c64xx_i2s_dai[I2S_NUM],
        .codec_dai = &tlv320adc3101_dai,
        .init = smdk64xx_tlv320adc3101_init_tx,
        .ops = &smdk64xx_ops,

},
};

static struct snd_soc_card smdk64xx = {
	.name = "smdk64xx",
	.platform = &s3c24xx_soc_platform,
	.dai_link = smdk64xx_dai,
	.num_links = ARRAY_SIZE(smdk64xx_dai),
};

struct snd_soc_device smdk64xx_snd_devdata = {
	.card = &smdk64xx,
	.codec_dev = &soc_codec_dev_s5m8751,
};

static struct platform_device *smdk64xx_snd_device;

static int __init smdk64xx_audio_init(void)
{
	int ret;

printk("===========8751 %s\n",__func__);
	smdk64xx_snd_device = platform_device_alloc("soc-audio", -1);
	if (!smdk64xx_snd_device)
		return -ENOMEM;

	platform_set_drvdata(smdk64xx_snd_device, &smdk64xx_snd_devdata);
	smdk64xx_snd_devdata.dev = &smdk64xx_snd_device->dev;
	ret = platform_device_add(smdk64xx_snd_device);

	if (ret)
		platform_device_put(smdk64xx_snd_device);

	return ret;
}
module_init(smdk64xx_audio_init);

/* Module Information */
MODULE_DESCRIPTION("ALSA SoC S5M8751 audio driver with SMDK64XX");
MODULE_AUTHOR("Jongbin, Won <jongbin.won@samsung.com>");
MODULE_LICENSE("GPL");
