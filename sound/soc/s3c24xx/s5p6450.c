/*
 * s5p6450_s5m8751.c  --  S5M8751 Power-Audio IC ALSA Soc Audio driver
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
#include <mach/gpio.h>
#include <plat/mendoza.h>
#include <asm/mach-types.h>
#include <plat/nashville.h>
#include <asm/mach-types.h>

#include <linux/mfd/s5m8751/core.h>
#include <linux/mfd/s5m8751/audio.h>

#include "s3c-dma.h"
#include "s3c-pcm.h"
#include "s3c64xx-i2s.h"
#include "../codecs/bcm2070.h"
#include "../codecs/tlv320adc3101.h"

#ifdef CONFIG_TOMTOM_FDT
#include <plat/fdt.h>
#endif

#ifdef CONFIG_TOMTOM_NASHVILLE_SCENARI_S5M8751
extern struct tt_scenario_ops s5m8751_scenario_ops;
#endif
#ifdef CONFIG_TOMTOM_NASHVILLE_SCENARI_TLV320ADC3101
extern struct tt_scenario_ops adc3101_scenario_ops;
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

static int get_epll_rate(unsigned long *rate)
{
	struct clk *fout_epll;

	fout_epll = clk_get(NULL, "fout_epll");
	if (IS_ERR(fout_epll)) {
		printk(KERN_ERR "%s: failed to get fout_epll\n", __func__);
		return -ENOENT;
	}

	*rate = clk_get_rate(fout_epll);

	clk_put(fout_epll);

	return 0;
}

/* Note that PCM works __only__ in AP-Master mode */
static int s5p6450_pcm_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->dai->cpu_dai;
	struct snd_soc_dai *codec_dai = rtd->dai->codec_dai;
	struct s3c_pcm_info *s3c_pcm = (struct s3c_pcm_info *)cpu_dai->private_data;
	int rfs, ret;
	unsigned long epll_out_rate;

	switch (params_rate(params)) {
	case 16000:
		rfs = 256;
		break;
	case 8000:
		rfs = 128;
		break;
	default:
		printk(KERN_ERR "%s:%d Sampling Rate %u not supported!\n",
			__func__, __LINE__, params_rate(params));
		return -EINVAL;
	}

	/* Set the AP DAI configuration */
	ret = snd_soc_dai_set_fmt(cpu_dai, SND_SOC_DAIFMT_DSP_B
					 | SND_SOC_DAIFMT_IB_NF
					 | SND_SOC_DAIFMT_CBS_CFS);
	if (ret < 0)
		return ret;

	/* Set MUX for PCM clock source to audio-bus */
	ret = snd_soc_dai_set_sysclk(cpu_dai, S3C_PCM_CLKSRC_MUX,
					epll_out_rate, SND_SOC_CLOCK_OUT);
	if (ret < 0)
		return ret;

	/* Get EPLL clock rate (don't want to mess around with I2S clock */
	ret = get_epll_rate(&epll_out_rate);
	if (ret < 0)
		return ret;

	/* Set SCLK_DIV for making bclk */
	ret = snd_soc_dai_set_clkdiv(cpu_dai, S3C_PCM_SCLK_PER_FS, rfs);
	if (ret < 0)
		return ret;

	/* s3c-pcm sets this value to 4, but with this value half of the frames are dropped */
	s3c_pcm->dma_playback->dma_size = 2;
	s3c_pcm->dma_capture->dma_size = 2;

	return 0;
}

static int s5p6450_hw_params(struct snd_pcm_substream *substream,
	struct snd_pcm_hw_params *params)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_dai *cpu_dai = rtd->dai->cpu_dai;
	struct snd_soc_dai *codec_dai = rtd->dai->codec_dai;

	int bfs, rfs, ret;
	unsigned int psr = 1, rclk;

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
static struct snd_soc_ops s5p6450_ops = {
	.hw_params = s5p6450_hw_params,
};

#ifdef CONFIG_SND_TLV320ADC3101
static int tlv320adc3101_startup(struct snd_pcm_substream *substream);
static void tlv320adc3101_shutdown(struct snd_pcm_substream *substream);

static struct snd_soc_ops tlv320adc3101_ops = {
	.hw_params = s5p6450_hw_params,
	.startup = tlv320adc3101_startup,
	.shutdown = tlv320adc3101_shutdown,
};
#endif

#ifdef CONFIG_SND_BCM2070
static struct snd_soc_ops bcm2070_ops = {
	.hw_params = s5p6450_pcm_hw_params,
};
#endif

static int s5p6450_dapm_speaker_event(struct snd_soc_dapm_widget *w, 
	struct snd_kcontrol *k, int event)
{
	if (SND_SOC_DAPM_EVENT_ON(event)) {
		gpio_set_value(TT_VGPIO_AMP_PWR_EN, 1);
	} else if (SND_SOC_DAPM_EVENT_OFF(event)) {
		gpio_set_value(TT_VGPIO_AMP_PWR_EN, 0);
	} else {
		printk("%s(%d): Unknown event #%d\n", __func__, __LINE__, event);
	}

	return 0;
}

static int s5p6450_dapm_mic_event(struct snd_soc_dapm_widget *w, 
	struct snd_kcontrol *k, int event)
{
	if (SND_SOC_DAPM_EVENT_ON(event)) {
		gpio_set_value(TT_VGPIO_MIC_AMP_EN, 1);
	} else if (SND_SOC_DAPM_EVENT_OFF(event)) {
		gpio_set_value(TT_VGPIO_MIC_AMP_EN, 0);
	} else {
		printk("%s(%d): Unknown event #%d\n", __func__, __LINE__, event);
	}

	return 0;
}

/* Valdez Playback widgets */
static const struct snd_soc_dapm_widget valdez_dapm_widgets[] = {
	SND_SOC_DAPM_HP("Headphones", NULL),
	SND_SOC_DAPM_LINE("LineOut", NULL),
	SND_SOC_DAPM_SPK("Speaker", s5p6450_dapm_speaker_event),
	SND_SOC_DAPM_MIC("Microphone", s5p6450_dapm_mic_event),
};

/* Taranto Playback widgets */
static const struct snd_soc_dapm_widget taranto_dapm_widgets[] = {
	SND_SOC_DAPM_HP("Headphones", NULL),
	SND_SOC_DAPM_LINE("LineOut", s5p6450_dapm_speaker_event),
	SND_SOC_DAPM_SPK("Speaker", NULL),
	SND_SOC_DAPM_MIC("Microphone", s5p6450_dapm_mic_event),
};

/* s5p6450 connections */
static const struct snd_soc_dapm_route audio_map[] = {
	{"Headphones", NULL, "LH"},
	{"Headphones", NULL, "RH"},

	{"Speaker", NULL, "SPK"},

	{"LineOut", NULL, "LL"},
	{"LineOut", NULL, "RL"},
};

int s5p6450_s5m8751_init(struct snd_soc_codec *codec)
{
	if (machine_is_taranto())
		/* Add taranto specific Playback widgets */
		snd_soc_dapm_new_controls(codec, taranto_dapm_widgets,
				ARRAY_SIZE(taranto_dapm_widgets));
	else
		/* Add valdez specific Playback widgets */
		snd_soc_dapm_new_controls(codec, valdez_dapm_widgets,
				ARRAY_SIZE(valdez_dapm_widgets));

	/* set up cordoba specific audio paths */
	snd_soc_dapm_add_routes(codec, audio_map, ARRAY_SIZE(audio_map));

#ifdef CONFIG_TOMTOM_NASHVILLE_SCENARI_S5M8751
	/* Add nashville controls */
	tomtom_add_nashville_controls(codec, &s5m8751_scenario_ops);
#endif
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

#ifdef CONFIG_SND_TLV320ADC3101
static int tlv320adc3101_linein_func = 0;
static int tlv320adc3101_micin_func = 1;
static int tlv320adc3101_digimicin_func = 0;

static void tlv320adc3101_ext_control(struct snd_soc_codec *codec)
{
	if (tlv320adc3101_linein_func)
		snd_soc_dapm_enable_pin(codec, "Line Input");
	else
		snd_soc_dapm_disable_pin(codec, "Line Input");

	if (tlv320adc3101_micin_func)
		snd_soc_dapm_enable_pin(codec, "Mic Input");
	else
		snd_soc_dapm_disable_pin(codec, "Mic Input");

	if (tlv320adc3101_digimicin_func)
		snd_soc_dapm_enable_pin(codec, "Digital Mic Input");
	else
		snd_soc_dapm_disable_pin(codec, "Digital Mic Input");

	snd_soc_dapm_sync(codec);
}

static const struct snd_soc_dapm_widget tlv320adc3101_dapm_widgets[] = {
	/* Line Input */
	SND_SOC_DAPM_MIC("Line Input", NULL),
	/* Mic Input */
	SND_SOC_DAPM_MIC("Mic Input", NULL),
	/* Digital Mic Input */
	SND_SOC_DAPM_MIC("Digital Mic Input", NULL),
};

/* example machine audio_mapnections */
static const struct snd_soc_dapm_route tlv320adc3101_audio_map[] = {
/* Line Input to codec dapm Inputs */
	{"IN1_L", NULL, "Line Input"},
	{"IN1_R", NULL, "Line Input"},

/* Mic Input to codec dapm Inputs */
	{"IN1_L", NULL, "Mic Input"},
	{"IN1_R", NULL, "Mic Input"},

/* Digital Mic Input to codec dapm Inputs */
	{"DMic_L", NULL, "Digital Mic Input"},
	{"DMic_R", NULL, "Digital Mic Input"},
};

static const char *linein_function[] = { "Off", "On" };
static const char *micin_function[] = { "Off", "On" };
static const char *digimicin_function[] = { "Off", "On" };

static const struct soc_enum tlv320adc3101_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(linein_function), linein_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(micin_function), micin_function),
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(digimicin_function), digimicin_function),
};

static int tlv320adc3101_get_linein(struct snd_kcontrol *kcontrol,
			   struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = tlv320adc3101_linein_func;

	return 0;
}

static int tlv320adc3101_set_linein(struct snd_kcontrol *kcontrol,
			   struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	if (tlv320adc3101_linein_func == ucontrol->value.integer.value[0])
		return 0;

	tlv320adc3101_linein_func = ucontrol->value.integer.value[0];
	tlv320adc3101_ext_control(codec);

	return 1;
}

static int tlv320adc3101_get_micin(struct snd_kcontrol *kcontrol,
			  struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = tlv320adc3101_micin_func;

	return 0;
}

static int tlv320adc3101_set_micin(struct snd_kcontrol *kcontrol,
			  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	if (tlv320adc3101_micin_func == ucontrol->value.integer.value[0])
		return 0;

	tlv320adc3101_micin_func = ucontrol->value.integer.value[0];
	tlv320adc3101_ext_control(codec);

	return 1;
}

static int tlv320adc3101_get_digimicin(struct snd_kcontrol *kcontrol,
			  struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = tlv320adc3101_digimicin_func;

	return 0;
}

static int tlv320adc3101_set_digimicin(struct snd_kcontrol *kcontrol,
			  struct snd_ctl_elem_value *ucontrol)
{
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	if (tlv320adc3101_digimicin_func == ucontrol->value.integer.value[0])
		return 0;

	tlv320adc3101_digimicin_func = ucontrol->value.integer.value[0];
	tlv320adc3101_ext_control(codec);

	return 1;
}

static const struct snd_kcontrol_new tlv320adc3101_controls[] = {
/* Line In Jack control */
	SOC_ENUM_EXT("Line In Jack", tlv320adc3101_enum[0], 
			tlv320adc3101_get_linein, tlv320adc3101_set_linein),
/* Mic In Jack control */
	SOC_ENUM_EXT("MIC In Jack", tlv320adc3101_enum[1], 
			tlv320adc3101_get_micin, tlv320adc3101_set_micin),
/* Digital Mic In control */
	SOC_ENUM_EXT("Digital MIC In", tlv320adc3101_enum[2], 
			tlv320adc3101_get_digimicin, tlv320adc3101_set_digimicin),
};

static int tlv320adc3101_init(struct snd_soc_codec *codec)
{
	int i, err;

	/* Add adc3101 specific controls */
	for (i = 0; i < ARRAY_SIZE(tlv320adc3101_controls); i++) {
		err = snd_ctl_add(codec->card, 
				snd_soc_cnew(&tlv320adc3101_controls[i], codec, NULL));
	if (err < 0)
		return err;
	}

	/* Add tlv320adc3101 specific widgets */
	snd_soc_dapm_new_controls(codec, tlv320adc3101_dapm_widgets,
		ARRAY_SIZE(tlv320adc3101_dapm_widgets));

#ifdef CONFIG_TOMTOM_NASHVILLE_SCENARI_TLV320ADC3101
	tomtom_add_nashville_controls(codec, &adc3101_scenario_ops);
#endif

	/* set up tlv320adc3101 specific audio paths */
	snd_soc_dapm_add_routes(codec, tlv320adc3101_audio_map,ARRAY_SIZE(tlv320adc3101_audio_map));

	snd_soc_dapm_sync(codec);

	return 0;
}

static int tlv320adc3101_startup(struct snd_pcm_substream *substream)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_codec *codec = rtd->socdev->codec;

	tlv320adc3101_ext_control(codec);
	return 0;
}

static void tlv320adc3101_shutdown(struct snd_pcm_substream *substream)
{
}
#endif


#ifdef CONFIG_SND_BCM2070
static int bcm2070_init(struct snd_soc_codec *codec)
{
	return 0;
}
#endif

static struct snd_soc_dai_link s5p6450_dai[] = {
{ /* Primary Playback i/f */
	.name = "S5M8751",
	.stream_name = "Playback",
	.codec_dai = &s5m8751_dai,
	.init = s5p6450_s5m8751_init,
	.ops = &s5p6450_ops,
},
#ifdef CONFIG_SND_TLV320ADC3101
{ /* Primary Capture i/f */
	.name = "TLV320ADC3101",
	.stream_name = "Capture",
	.codec_dai = &tlv320adc3101_dai,
	.init = tlv320adc3101_init,
	.ops = &tlv320adc3101_ops,
},
#endif
#ifdef CONFIG_SND_BCM2070
{ /* Primary Capture i/f */
	.name = "Bluetooth PCM",
	.stream_name = "Bluetooth PCM",
	.cpu_dai = &s3c_pcm_dai[1],
	.codec_dai = &bcm2070_dai,
	.init = bcm2070_init,
	.ops = &bcm2070_ops,
},
#endif
};

static struct snd_soc_card s5p6450 = {
	.name = "smdk64xx",
	.platform = &s3c24xx_soc_platform,
	.dai_link = s5p6450_dai,
	.num_links = ARRAY_SIZE(s5p6450_dai),
};

struct snd_soc_device s5p6450_snd_devdata = {
	.card = &s5p6450,
	.codec_dev = &soc_codec_dev_s5m8751,
};

static struct platform_device *s5p6450_snd_device;

static int __init s5p6450_audio_init(void)
{
	int ret;
	int i2s_bus_nr;

	if (!machine_is_taranto() && !machine_is_valdez()) {
		pr_debug("s5p6450_audio_init : not part of MACH_S5P6450_TOMTOM family\n");
		return -ENODEV;
	}

	if (gpio_is_valid(TT_VGPIO_AMP_PWR_EN) ||
	    gpio_is_valid(TT_VGPIO_MIC_AMP_EN)) {
		ret = gpio_request(TT_VGPIO_AMP_PWR_EN, "TT_VGPIO_AMP_PWR_EN") ||
		      gpio_request(TT_VGPIO_MIC_AMP_EN, "TT_VGPIO_MIC_AMP_EN");
		if (ret)
			goto err1;

		ret = gpio_direction_output(TT_VGPIO_AMP_PWR_EN, 0) ||
		      gpio_direction_output(TT_VGPIO_MIC_AMP_EN, 0);
		if (ret)
			goto err2;
	}

	s5p6450_snd_device = platform_device_alloc("soc-audio", -1);
	if (!s5p6450_snd_device) {
		ret = -ENOMEM;
		goto err2;
	}

	platform_set_drvdata(s5p6450_snd_device, &s5p6450_snd_devdata);
	s5p6450_snd_devdata.dev = &s5p6450_snd_device->dev;

	i2s_bus_nr = fdt_get_ulong ("/options/sound", "i2s-bus-num", 0);
	s5p6450_dai[0].cpu_dai = &s3c64xx_i2s_dai[i2s_bus_nr];
#ifdef CONFIG_SND_TLV320ADC3101
	s5p6450_dai[1].cpu_dai = &s3c64xx_i2s_dai[i2s_bus_nr];
#endif

	ret = platform_device_add(s5p6450_snd_device);
	if (ret)
		goto err3;

	return 0;

err3:
	printk(KERN_ERR "Unable to add platform device\n");
	platform_device_put(s5p6450_snd_device);
err2:
	if (gpio_is_valid(TT_VGPIO_AMP_PWR_EN))
		gpio_free(TT_VGPIO_AMP_PWR_EN);
	if (gpio_is_valid(TT_VGPIO_MIC_AMP_EN))
		gpio_free(TT_VGPIO_MIC_AMP_EN);
err1:
	return ret;
}
module_init(s5p6450_audio_init);

static void __exit s5p6450_audio_exit(void)
{
	if (gpio_is_valid(TT_VGPIO_AMP_PWR_EN))
		gpio_free(TT_VGPIO_AMP_PWR_EN);
	if (gpio_is_valid(TT_VGPIO_MIC_AMP_EN))
		gpio_free(TT_VGPIO_MIC_AMP_EN);
	platform_device_unregister(s5p6450_snd_device);
}
module_exit(s5p6450_audio_exit);

/* Module Information */
MODULE_DESCRIPTION("ALSA SoC S5M8751 audio driver with SMDK64XX");
MODULE_AUTHOR("Jongbin, Won <jongbin.won@samsung.com>");
MODULE_LICENSE("GPL");
