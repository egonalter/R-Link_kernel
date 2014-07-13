/* drivers/tomtom/sound/tlv320adc3101.c
 *
 * Default platform implementation for the Nashville controls. Does nothing.
 *
 * Copyright (C) 2009 TomTom BV <http://www.tomtom.com/>
 * Author: Niels Langendorff <niels.langendorff@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

#include <plat/scenarii.h>
#include "../../../sound/soc/codecs/tlv320adc3101.h"

#define DUMMY_CALLBACK(name)	static int adc3101_##name(struct tt_scenario_context *c) \
				{							\
					printk("%s\n", __func__);			\
					/* Do nothing */					\
					return 0;							\
				}

DUMMY_CALLBACK(set_volume)
DUMMY_CALLBACK(set_volume_dB)

static void adc3101_mic_bias_enable(struct snd_soc_codec *codec)
{
	snd_soc_update_bits(codec, MICBIAS_CTRL,
		MICBIAS1_MASK, (MICBIAS_2V5 << MICBIAS1_SHIFT));
}

static void adc3101_mic_bias_disable(struct snd_soc_codec *codec)
{
	snd_soc_update_bits(codec, MICBIAS_CTRL,
		MICBIAS1_MASK, (MICBIAS_PWDWN << MICBIAS1_SHIFT));
}

static void adc3101_alc_disable(struct snd_soc_codec *codec)
{
	/* ALC off */
}

static void adc3101_line_in_enable(struct snd_soc_codec *codec)
{
	/* Not used */
}

static void adc3101_line_in_disable(struct snd_soc_codec *codec)
{
	/* Not used */
}

static void adc3101_mic_enable(struct snd_soc_codec *codec, int gain_dB, int HPF)
{
	adc3101_mic_bias_enable(codec);
	/* Not connected */
	snd_soc_update_bits(codec, LEFT_PGA_SEL_1, 0xFF, 
		(0x2 < 6) | \
		(0x2 < 4) | \
		(0x2 < 2) | \
		(0x2 < 0));	
	/* IN2R/IN3R 0dB */
	snd_soc_update_bits(codec, LEFT_PGA_SEL_2, 0x3F, 
		(0x2 < 4) | \
		(0x0 < 2) | \
		(0x2 < 0));
	snd_soc_update_bits(codec, RIGHT_PGA_SEL_1, 0xFF, 
		(0x2 < 6) | \
		(0x2 < 4) | \
		(0x2 < 2) | \
		(0x2 < 0));
	/* IN2R/IN3R 0dB */
	snd_soc_update_bits(codec, RIGHT_PGA_SEL_2, 0x3F, 
		(0x2 < 4) | \
		(0x0 < 2) | \
		(0x2 < 0));

	/* Unmute Left PGA + gain */
	snd_soc_update_bits(codec, LEFT_APGA_CTRL, 0xFF, (0 << 7) | (2 * gain_dB));
	/* Unmute Right PGA + gain */
	snd_soc_update_bits(codec, RIGHT_APGA_CTRL, 0xFF, (0 << 7) | (2 * gain_dB));
	/* Fine Gain Left/Right ADC unmute + 0 dB */
	snd_soc_update_bits(codec, ADC_FGA, 0xFF,
		(0 << 7) | \
		(0 << 4) | \
		(0 << 3) | \
		(0 << 0));

	/* Power up Left/Right digital ADC */
	snd_soc_update_bits(codec, ADC_DIGITAL, 0xC0, (1 << 7) | (1 << 6));

	adc3101_alc_disable(codec);

	snd_soc_dapm_enable_pin(codec, "MicIn");
}

static void adc3101_mic_disable(struct snd_soc_codec *codec)
{
	adc3101_mic_bias_disable(codec);

	/* Not connected */
	snd_soc_update_bits(codec, LEFT_PGA_SEL_1, 0xFF, 
		(0x2 < 6) | \
		(0x2 < 4) | \
		(0x2 < 2) | \
		(0x2 < 0));
	/* Not connected */
	snd_soc_update_bits(codec, LEFT_PGA_SEL_2, 0x3F, 
		(0x2 < 4) | \
		(0x2 < 2) | \
		(0x2 < 0));
	/* Not connected */
	snd_soc_update_bits(codec, RIGHT_PGA_SEL_1, 0xFF, 
		(0x2 < 6) | \
		(0x2 < 4) | \
		(0x2 < 2) | \
		(0x2 < 0));
	/* Not connected */
	snd_soc_update_bits(codec, RIGHT_PGA_SEL_2, 0x3F, 
		(0x2 < 4) | \
		(0x2 < 2) | \
		(0x2 < 0));

	/* Unmute Left PGA */
	snd_soc_update_bits(codec, LEFT_APGA_CTRL, 0x80, (1 << 7));
	/* Unmute Right PGA */
	snd_soc_update_bits(codec, RIGHT_APGA_CTRL, 0x80, (1 << 7));
	/* Fine Gain Left/Right ADC mute */
	snd_soc_update_bits(codec, ADC_FGA, 0x88,
		(1 << 7) | \
		(1 << 3));

	/* Power down Left/Right digital ADC */
	snd_soc_update_bits(codec, ADC_DIGITAL, 0xC0, (0 << 7) | (0 << 6));
	snd_soc_dapm_disable_pin(codec, "MicIn");
}

static int adc3101_set_scenario_endpoints(struct tt_scenario_context *c)
{
	struct snd_soc_codec *codec = c->codec;

	/* check whether the new scenario is already active, if so,
	   don't set it again as this has been seen to result in non
	   wanted register changes e.g. setting scenario TT_SCENARIO_ASR
	   twice result in non ASR scenario registers being set. */
	if (c->tt_current_scenario == c->tt_scenario)
		return 0;
	
	/* scenario routing switching */
	switch(c->tt_scenario) {
	default:
	case TT_SCENARIO_AUDIO_INIT:
		/* enable mic bias (not if we use mic pre-amp) */
		adc3101_mic_bias_enable(codec);
	case TT_SCENARIO_AUDIO_OFF:
		c->tt_volume = 0;
		switch(c->tt_current_scenario) {
		case TT_SCENARIO_VR:
			adc3101_alc_disable(codec);
		case TT_SCENARIO_NOISE:
		case TT_SCENARIO_ASR:
		case TT_SCENARIO_HF_LINEOUT:
		case TT_SCENARIO_HF_FM:
		case TT_SCENARIO_HF_SPEAKER:
			adc3101_mic_disable(codec);
			break;
		case TT_SCENARIO_SPEAKER:
		case TT_SCENARIO_LINEOUT:
		case TT_SCENARIO_FM:
			break;
			adc3101_mic_disable(codec);
			break;
		case TT_SCENARIO_iPod:
			adc3101_line_in_disable(codec);
			adc3101_mic_bias_enable(codec);
			break;
		case TT_SCENARIO_iPod_SPEAKER:
			adc3101_line_in_disable(codec);
			adc3101_mic_bias_enable(codec);
			break;
		case TT_SCENARIO_iPod_LINEOUT:
		case TT_SCENARIO_iPod_FM:
			adc3101_line_in_disable(codec);
			adc3101_mic_bias_enable(codec);
			break;
		default:
			snd_soc_dapm_disable_pin(codec, "MicIn");
			break;
		}
		break;
	case TT_SCENARIO_ASR:
		adc3101_mic_enable(codec, 25, 1);
		break;
	case TT_SCENARIO_VR:
		adc3101_mic_enable(codec, 25, 1);
		break;
	case TT_SCENARIO_NOISE:
		adc3101_mic_enable(codec, 15, 1);
		break;
	case TT_SCENARIO_iPod:
		adc3101_line_in_enable(codec);
		break;
	case TT_SCENARIO_SPEAKER:
		break;
	case TT_SCENARIO_LINEOUT:
		break;
	case TT_SCENARIO_FM:
		break;
	case TT_SCENARIO_iPod_SPEAKER:
		adc3101_line_in_enable(codec);
		break;
	case TT_SCENARIO_iPod_LINEOUT:
	case TT_SCENARIO_iPod_FM:
		adc3101_line_in_enable(codec);
		break;
	case TT_SCENARIO_HF_SPEAKER:
		adc3101_mic_enable(codec, 13, 1);
		break;
	case TT_SCENARIO_HF_LINEOUT:
	case TT_SCENARIO_HF_FM:
		adc3101_mic_enable(codec, 13, 1);
		break;
	}

	c->tt_current_scenario = c->tt_scenario;
	snd_soc_dapm_sync(codec);

	switch(c->tt_scenario) {
	case TT_SCENARIO_LINEOUT:
	case TT_SCENARIO_iPod_LINEOUT:
	case TT_SCENARIO_FM:
	case TT_SCENARIO_iPod_FM:
		break;
	default:
	case TT_SCENARIO_AUDIO_INIT:
	case TT_SCENARIO_AUDIO_OFF:
		break;
	case TT_SCENARIO_SPEAKER:
	case TT_SCENARIO_iPod_SPEAKER:
	case TT_SCENARIO_HF_SPEAKER:
		break;
	}

	return 1;
}

static int adc3101_set_scenario(struct tt_scenario_context *context)
{
	adc3101_set_scenario_endpoints(context);
	return 1;
}

static int adc3101_set_scenario_mode(struct tt_scenario_context *context)
{
	adc3101_set_scenario_endpoints(context);
	return 1;
}

struct tt_scenario_ops adc3101_scenario_ops = {
	.set_volume			= adc3101_set_volume,
	.set_volume_db		= adc3101_set_volume_dB,
	.set_scenario		= adc3101_set_scenario,
	.set_scenario_mode	= adc3101_set_scenario_mode
};
EXPORT_SYMBOL(adc3101_scenario_ops);
