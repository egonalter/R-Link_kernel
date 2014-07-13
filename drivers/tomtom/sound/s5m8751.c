/* drivers/tomtom/sound/s5m8751.c
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
#include <linux/mfd/s5m8751/audio.h>
#include <linux/mfd/s5m8751/core.h>

#define DUMMY_CALLBACK(name)	static int s5m8751_##name(struct tt_scenario_context *c) \
				{							\
					printk("%s\n", __func__);			\
					/* Do nothing */					\
					return 1;							\
				}

/* s5m8751_set_volume is never used by Nashville and will not be implemented */
DUMMY_CALLBACK(set_volume)

static int db_to_dvol(int volume_db)
{
	int out = -1;

	/* volume_db := [0, +90] */
	volume_db = clamp(volume_db, 0, 90);
	/* volume_db := [-78, +12] */
	volume_db = volume_db - 78;

	if (volume_db <= 12 && volume_db > -55)
		out = 24 - 2 * volume_db;
	else if (volume_db <= -55 && volume_db > -66)
		out = 79 - volume_db;
	else if (volume_db <= -66 && volume_db > -72)
		out = 112 - volume_db / 2;
	else
		out = 151;

	return out;
}

/* user of this function will pass in a the gain as a 
   possitive value which should be interpreted as negative */
static int s5m8751_set_volume_dB(struct tt_scenario_context *c)
{
	struct snd_soc_codec *codec = c->codec;
	int volume;

	switch(c->tt_scenario) {
		case TT_SCENARIO_LINEOUT:
		case TT_SCENARIO_iPod_LINEOUT:
			/* set lineout volume if connected */ 
			c->tt_volume_dB = 0; /* 0dB for fixed line out level */
			volume = db_to_dvol(c->tt_volume_dB);
			snd_soc_update_bits(codec, S5M8751_DA_VOLL,
				S5M8751_DA_VOLL_MASK, volume << S5M8751_DA_VOLL_SHIFT);
			snd_soc_update_bits(codec, S5M8751_DA_VOLR,
				S5M8751_DA_VOLR_MASK, volume << S5M8751_DA_VOLR_SHIFT);
			break;
		default:
		case TT_SCENARIO_AUDIO_INIT:
		case TT_SCENARIO_AUDIO_OFF:
			c->tt_volume_dB = 0;
			volume = db_to_dvol(c->tt_volume_dB); 
			snd_soc_update_bits(codec, S5M8751_DA_VOLL,
				S5M8751_DA_VOLL_MASK, volume << S5M8751_DA_VOLL_SHIFT);
			snd_soc_update_bits(codec, S5M8751_DA_VOLR,
				S5M8751_DA_VOLR_MASK, volume << S5M8751_DA_VOLR_SHIFT);
			break;
		case TT_SCENARIO_SPEAKER:
		case TT_SCENARIO_iPod_SPEAKER:
			volume = db_to_dvol(c->tt_volume_dB);
			snd_soc_update_bits(codec, S5M8751_SPK_S2D,
				S5M8751_SPK_PAMP_GAIN_MASK, 
				S5M8751_SPK_PAMP_GAIN_0DB << S5M8751_SPK_PAMP_GAIN_SHIFT);
			snd_soc_update_bits(codec, S5M8751_DA_VOLL,
				S5M8751_DA_VOLL_MASK, volume << S5M8751_DA_VOLL_SHIFT);
			snd_soc_update_bits(codec, S5M8751_DA_VOLR,
				S5M8751_DA_VOLR_MASK, volume << S5M8751_DA_VOLR_SHIFT);
			break;
		case TT_SCENARIO_HF_SPEAKER:
			volume = db_to_dvol(c->tt_volume_dB);
			snd_soc_update_bits(codec, S5M8751_SPK_S2D,
				S5M8751_SPK_PAMP_GAIN_MASK, 
				S5M8751_SPK_PAMP_GAIN_0DB << S5M8751_SPK_PAMP_GAIN_SHIFT);
			snd_soc_update_bits(codec, S5M8751_DA_VOLL,
				S5M8751_DA_VOLL_MASK, volume << S5M8751_DA_VOLL_SHIFT);
			snd_soc_update_bits(codec, S5M8751_DA_VOLR,
				S5M8751_DA_VOLR_MASK, volume << S5M8751_DA_VOLR_SHIFT);
			break;
		case TT_SCENARIO_FM:
		case TT_SCENARIO_iPod_FM:
			c->tt_volume_dB = 0;
			volume = db_to_dvol(c->tt_volume_dB); 
			snd_soc_update_bits(codec, S5M8751_DA_VOLL,
				S5M8751_DA_VOLL_MASK, volume << S5M8751_DA_VOLL_SHIFT);
			snd_soc_update_bits(codec, S5M8751_DA_VOLR,
				S5M8751_DA_VOLR_MASK, volume << S5M8751_DA_VOLR_SHIFT);
			break;
	}
	snd_soc_dapm_sync(codec);
	return 1;
}

static void s5m8751_mic_bias_enable(struct snd_soc_codec *codec)
{
	/* Microphone input not supported */
}

static void s5m8751_mic_bias_disable(struct snd_soc_codec *codec)
{
	/* Microphone input not supported */
}

static void s5m8751_line_in_enable(struct snd_soc_codec *codec)
{
	/* Line in input not supported */
}

static void s5m8751_line_in_disable(struct snd_soc_codec *codec)
{
	/* Line in input not supported */
}

static void s5m8751_mic_enable(struct snd_soc_codec *codec, int gain_dB, int HPF)
{
	/* Microphone input not supported */
}

static void s5m8751_mic_disable(struct snd_soc_codec *codec)
{
	/* Microphone input not supported */
}

static void s5m8751_alc_disable(struct snd_soc_codec *codec)
{
	/* ALC not supported */
}

static int s5m8751_set_scenario_endpoints(struct tt_scenario_context *c)
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
			s5m8751_mic_bias_enable(codec);
		case TT_SCENARIO_AUDIO_OFF:
			c->tt_volume = 0;
			snd_soc_update_bits(codec, S5M8751_AMP_MUTE,
				S5M8751_AMP_MUTE_SPK_S2D_MASK | \
				S5M8751_AMP_MUTE_LHP_MASK | S5M8751_AMP_MUTE_RHP_MASK | \
				S5M8751_AMP_MUTE_LLINE_MASK | S5M8751_AMP_MUTE_RLINE_MASK, 
				(0 << S5M8751_AMP_MUTE_SPK_S2D_SHIFT) | \
				(0 << S5M8751_AMP_MUTE_LHP_SHIFT) | (0 << S5M8751_AMP_MUTE_RHP_SHIFT) | \
				(0 << S5M8751_AMP_MUTE_LLINE_SHIFT) | (0 << S5M8751_AMP_MUTE_RLINE_SHIFT));
			snd_soc_update_bits(codec, S5M8751_AMP_EN,
				S5M8751_AMP_EN_SPK_MASK | \
				S5M8751_AMP_EN_LHP_MASK | S5M8751_AMP_EN_RHP_MASK | \
				S5M8751_AMP_EN_LLINE_MASK | S5M8751_AMP_EN_RLINE_MASK, 
				(0 << S5M8751_AMP_EN_SPK_SHIFT) | \
				(0 << S5M8751_AMP_EN_LHP_SHIFT) | (0 << S5M8751_AMP_EN_RHP_SHIFT) | \
				(0 << S5M8751_AMP_EN_LLINE_SHIFT) | (0 << S5M8751_AMP_EN_RLINE_SHIFT));
			switch(c->tt_current_scenario) {
				case TT_SCENARIO_VR:
					s5m8751_alc_disable(codec);
				case TT_SCENARIO_NOISE:
				case TT_SCENARIO_ASR:
					s5m8751_mic_disable(codec);
					break;
				case TT_SCENARIO_HF_SPEAKER:
					s5m8751_mic_disable(codec);
				case TT_SCENARIO_SPEAKER:
					snd_soc_dapm_disable_pin(codec, "Speaker");
					break;
				case TT_SCENARIO_LINEOUT:
					snd_soc_dapm_disable_pin(codec, "Headphones");
					break;
				case TT_SCENARIO_FM:
					snd_soc_dapm_disable_pin(codec, "Headphones");
					break;
				case TT_SCENARIO_HF_LINEOUT:
				case TT_SCENARIO_HF_FM:
					s5m8751_mic_disable(codec);
					snd_soc_dapm_disable_pin(codec, "Headphones");
					break;
				case TT_SCENARIO_iPod:
					s5m8751_line_in_disable(codec);
					s5m8751_mic_bias_enable(codec);
					break;
				case TT_SCENARIO_iPod_SPEAKER:
					s5m8751_line_in_disable(codec);
					s5m8751_mic_bias_enable(codec);
					snd_soc_dapm_disable_pin(codec, "Speaker");
					break;
				case TT_SCENARIO_iPod_LINEOUT:
				case TT_SCENARIO_iPod_FM:
					s5m8751_line_in_disable(codec);
					s5m8751_mic_bias_enable(codec);
					snd_soc_dapm_disable_pin(codec, "Headphones");
					break;
				default:
					snd_soc_dapm_disable_pin(codec, "Headphones");
					snd_soc_dapm_disable_pin(codec, "Speaker");
					break;
			}
			break;
		case TT_SCENARIO_ASR:
			break;
		case TT_SCENARIO_VR:
			break;
		case TT_SCENARIO_NOISE:
			break;
		case TT_SCENARIO_iPod:
			break;
		case TT_SCENARIO_SPEAKER:
			snd_soc_dapm_enable_pin(codec, "Speaker");
			break;
		case TT_SCENARIO_LINEOUT:
			snd_soc_dapm_enable_pin(codec, "Headphones");
			break;
		case TT_SCENARIO_FM:
			snd_soc_dapm_enable_pin(codec, "Headphones");
			break;
		case TT_SCENARIO_iPod_SPEAKER:
			snd_soc_dapm_enable_pin(codec, "Speaker");
			break;
		case TT_SCENARIO_iPod_LINEOUT:
		case TT_SCENARIO_iPod_FM:
			snd_soc_dapm_enable_pin(codec, "Headphones");
			break;
		case TT_SCENARIO_HF_SPEAKER:
			snd_soc_dapm_enable_pin(codec, "Speaker");
			break;
		case TT_SCENARIO_HF_LINEOUT:
		case TT_SCENARIO_HF_FM:
			snd_soc_dapm_enable_pin(codec, "Headphones");
			break;
	}

	c->tt_current_scenario = c->tt_scenario;
	snd_soc_dapm_sync(codec);

	switch(c->tt_scenario) {
	case TT_SCENARIO_LINEOUT:
	case TT_SCENARIO_iPod_LINEOUT:
	case TT_SCENARIO_FM:
	case TT_SCENARIO_iPod_FM:
		snd_soc_update_bits(codec, S5M8751_DA_PDB1, S5M8751_DA_PDB1_MASK,
			S5M8751_PDB_REF | S5M8751_PDB_DACL | S5M8751_PDB_DACR | \
			S5M8751_PDB_MR | S5M8751_PDB_ML);

		snd_soc_update_bits(codec, S5M8751_DA_AMIX1, S5M8751_DA_AMIX1_MASK,
			S5M8751_RDAC_MR | S5M8751_LDAC_ML);

		snd_soc_update_bits(codec, S5M8751_DA_AMIX2, S5M8751_DA_AMIX2_MASK, 0);

		snd_soc_update_bits(codec, S5M8751_AMP_EN,
			S5M8751_AMP_EN_LLINE_MASK | S5M8751_AMP_EN_RLINE_MASK,
			1 << S5M8751_AMP_EN_LLINE_SHIFT | 1 << S5M8751_AMP_EN_RLINE_SHIFT);
		snd_soc_update_bits(codec, S5M8751_AMP_MUTE,
			S5M8751_AMP_MUTE_LLINE_MASK | S5M8751_AMP_MUTE_RLINE_MASK, 
			1 << S5M8751_AMP_MUTE_LLINE_SHIFT | 1 << S5M8751_AMP_MUTE_RLINE_SHIFT);
		break;
	default:
	case TT_SCENARIO_AUDIO_INIT:
	case TT_SCENARIO_AUDIO_OFF:
		break;
	case TT_SCENARIO_SPEAKER:
	case TT_SCENARIO_iPod_SPEAKER:
	case TT_SCENARIO_HF_SPEAKER:
		snd_soc_update_bits(codec, S5M8751_DA_PDB1, S5M8751_DA_PDB1_MASK,
			S5M8751_PDB_REF | S5M8751_PDB_DACL | S5M8751_PDB_DACR | \
			S5M8751_PDB_MM);

		snd_soc_update_bits(codec, S5M8751_DA_AMIX1, S5M8751_DA_AMIX1_MASK,
			S5M8751_RDAC_MM | S5M8751_LDAC_MM);

		snd_soc_update_bits(codec, S5M8751_DA_AMIX2, S5M8751_DA_AMIX2_MASK, 0);

		snd_soc_update_bits(codec, S5M8751_AMP_EN,
			S5M8751_AMP_EN_SPK_MASK, 1 << S5M8751_AMP_EN_SPK_SHIFT);
		snd_soc_update_bits(codec, S5M8751_AMP_MUTE,
			S5M8751_AMP_MUTE_SPK_S2D_MASK, 1 << S5M8751_AMP_MUTE_SPK_S2D_SHIFT);
		break;
	}

	snd_soc_dapm_sync(codec);
	return 1;
}

static int s5m8751_set_scenario(struct tt_scenario_context *context)
{
	s5m8751_set_scenario_endpoints(context);
	return 1;
}

static int s5m8751_set_scenario_mode(struct tt_scenario_context *context)
{
	s5m8751_set_scenario_endpoints(context);
	return 1;
}

struct tt_scenario_ops s5m8751_scenario_ops = {
	.set_volume		= s5m8751_set_volume,
	.set_volume_db		= s5m8751_set_volume_dB,
	.set_scenario		= s5m8751_set_scenario,
	.set_scenario_mode	= s5m8751_set_scenario_mode
};

EXPORT_SYMBOL(s5m8751_scenario_ops);

