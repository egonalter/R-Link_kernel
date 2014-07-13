/* drivers/tomtom/sound/adau1761.c
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

#include "../../../sound/soc/codecs/adau1761.h"

#define DUMMY_CALLBACK(name)	static int adau1761_##name(struct tt_scenario_context *c) \
				{							\
					printk("%s\n", __func__);			\
					/* Do nothing */				\
				}

#define DVOLUME_STEP (384) /* 0.375*1024 */
#define LDVOL_STEP (768) /* 0.75*1024 */
#define LDVOL_0dB (16)

static int adau1761_set_volume(struct tt_scenario_context *c)
{
	struct snd_kcontrol	*kcontrol = (struct snd_kcontrol *) c->data;
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	int volume = 0xff; /* -95.625 dB attenuation on digital volume */

	switch(c->tt_scenario) {
	case TT_SCENARIO_LINEOUT:
	case TT_SCENARIO_iPod_LINEOUT:
		/* set lineout volume if connected */ 
		/*if linout plugged in {*/
			volume = 0; /* odB for fixed line out level */
			break;
		/*}*/ /* else nothing connected, set volume as if audio is off. */
	default:
	case TT_SCENARIO_AUDIO_INIT:
	case TT_SCENARIO_AUDIO_OFF:
		c->tt_volume = 0;
		/* mono output control */
		snd_soc_update_bits(codec, ADAU_PLBMNOC-ADAU_FIRSTREG, 
			ADAU1761_VOLUME_MASK | ADAU1761_MUTE_MASK, 0);
		/* line out output control */
		snd_soc_update_bits(codec, ADAU_PLBLOVL-ADAU_FIRSTREG, 
			ADAU1761_VOLUME_MASK | ADAU1761_MUTE_MASK, 0);
		snd_soc_update_bits(codec, ADAU_PLBLOVR-ADAU_FIRSTREG, 
			ADAU1761_VOLUME_MASK | ADAU1761_MUTE_MASK, 0);
		/* headphone output control */
		snd_soc_update_bits(codec, ADAU_PLBHPVL-ADAU_FIRSTREG,
			ADAU1761_VOLUME_MASK | ADAU1761_MUTE_MASK, 0);
		snd_soc_update_bits(codec, ADAU_PLBHPVR-ADAU_FIRSTREG, 
			ADAU1761_VOLUME_MASK | ADAU1761_MUTE_MASK, 0);
		break;
	case TT_SCENARIO_SPEAKER:
	case TT_SCENARIO_iPod_SPEAKER:
		switch(c->tt_scenario_mode){
		case TT_MODE_INIT:
		case TT_MODE_OFF:
		case TT_MODE_iPod:
		case TT_MODE_MP3:
		case TT_MODE_SPP:
		case TT_MODE_POI:
			/* use 45 dB range for now on speaker playback */
			volume = ((((100 - c->tt_volume)) * 512) / DVOLUME_STEP);
			break;
		case TT_MODE_TTS:
			/* use 45 dB range for now on speaker playback */
			volume = ((((100 - c->tt_volume)) * 512) / DVOLUME_STEP);
			/* scale to loudness of source (~ 9 - 12 dB difference
			   with SPP voices) audio engine will scale the max 
			   volumes with audio processing */ 
			volume -= ((9 * 1024) / DVOLUME_STEP);
			if (volume < 0)
				volume = 0;
			break;
		}
		break;
	case TT_SCENARIO_FM:
	case TT_SCENARIO_iPod_FM:
		volume = 48; /* 48 = ~ -18 dB */
		break;
	case TT_SCENARIO_HF_SPEAKER:
		/* use 30 dB range for now on speaker playback */
		if ((volume = c->tt_volume) < 15) {
			volume = 15;
		}
		if (volume > 85) {
			volume = 0; /* max volume 0dB */
		} else {
			volume = ((((85 - c->tt_volume)) * 512) / DVOLUME_STEP);
		}
		break;
	}
	snd_soc_update_bits(codec, ADAU_DACCTL1-ADAU_FIRSTREG, 
		ADAU1761_ADVOL_MASK, volume);
	snd_soc_update_bits(codec, ADAU_DACCTL2-ADAU_FIRSTREG, 
		ADAU1761_ADVOL_MASK, volume);
	return 1;
}

/* user of this function will pass in a the gain as a 
   possitive value which should be interpreted as negative */
static int adau1761_set_volume_dB(struct tt_scenario_context *c)
{
	struct snd_kcontrol	*kcontrol = (struct snd_kcontrol *) c->data;
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	int volume = 0xff; /* -95.625 dB attenuation on digital volume */

	switch(c->tt_scenario) {
	case TT_SCENARIO_LINEOUT:
	case TT_SCENARIO_iPod_LINEOUT:
		/* set lineout volume if connected */ 
		/*if linout plugged in {*/
			volume = 0; /* odB for fixed line out level */
			break;
		/*}*/ /* else nothing connected, set volume as if audio is off. */
	default:
	case TT_SCENARIO_AUDIO_INIT:
	case TT_SCENARIO_AUDIO_OFF:
		c->tt_volume = 0;
		/* mono output control */
		snd_soc_update_bits(codec, ADAU_PLBMNOC-ADAU_FIRSTREG, 
			ADAU1761_VOLUME_MASK | ADAU1761_MUTE_MASK, 0);
		/* line out output control */
		snd_soc_update_bits(codec, ADAU_PLBLOVL-ADAU_FIRSTREG, 
			ADAU1761_VOLUME_MASK | ADAU1761_MUTE_MASK, 0);
		snd_soc_update_bits(codec, ADAU_PLBLOVR-ADAU_FIRSTREG, 
			ADAU1761_VOLUME_MASK | ADAU1761_MUTE_MASK, 0);
		/* headphone output control */
		snd_soc_update_bits(codec, ADAU_PLBHPVL-ADAU_FIRSTREG,
			ADAU1761_VOLUME_MASK | ADAU1761_MUTE_MASK, 0);
		snd_soc_update_bits(codec, ADAU_PLBHPVR-ADAU_FIRSTREG, 
			ADAU1761_VOLUME_MASK | ADAU1761_MUTE_MASK, 0);
		break;
	case TT_SCENARIO_SPEAKER:
	case TT_SCENARIO_iPod_SPEAKER:
	case TT_SCENARIO_HF_SPEAKER:
		switch(c->tt_scenario_mode){
		default:
			volume = ((c->tt_volume_dB * 1024) / DVOLUME_STEP);
			break;
		}
		break;
	case TT_SCENARIO_FM:
	case TT_SCENARIO_iPod_FM:
		volume = 48; /* 48 = ~ -18 dB */
		break;
	}
	snd_soc_update_bits(codec, ADAU_DACCTL1-ADAU_FIRSTREG, 
		ADAU1761_ADVOL_MASK, volume);
	snd_soc_update_bits(codec, ADAU_DACCTL2-ADAU_FIRSTREG, 
		ADAU1761_ADVOL_MASK, volume);
	return 1;
}

static void adau1761_mic_bias_enable(struct snd_soc_codec *codec)
{
	/* enable mic bias (not if we use mic pre-amp) */
	snd_soc_update_bits(codec, ADAU_RECMBIA-ADAU_FIRSTREG, 
		RECMBIA_MBIEN_MASK | RECMBIA_MBI_MASK | RECMBIA_MPERF_MASK,
		(1 << RECMBIA_MBIEN_BITS) |(0 << RECMBIA_MBI_BITS) |
		(1 << RECMBIA_MPERF_BITS));
}

static void adau1761_mic_bias_disable(struct snd_soc_codec *codec)
{
	/* disable mic bias` */
	snd_soc_update_bits(codec, ADAU_RECMBIA-ADAU_FIRSTREG, 
		RECMBIA_MBIEN_MASK | RECMBIA_MBI_MASK | RECMBIA_MPERF_MASK, 0);
}

static void adau1761_alc_disable(struct snd_soc_codec *codec)
{
	/* ALC off */
	snd_soc_update_bits(codec, ADAU_ALCCTR0-ADAU_FIRSTREG, 
		ALCCTR0_PGASLEW_MASK | ALCCTR0_ALCSEL_MASK, 
		(0x3 << ALCCTR0_PGASLEW_BITS) | (0x0 << ALCCTR0_ALCSEL_BITS));
	snd_soc_update_bits(codec, ADAU_ALCCTR3-ADAU_FIRSTREG, 
		ALCCTR3_NGEN_MASK, 0);
}

static void adau1761_line_in_enable(struct snd_soc_codec *codec)
{
	adau1761_mic_bias_disable(codec);
	#if 0
		regvalue = (adau1761_read_reg_cache(codec,ADAU_RECVLCL) 
				& RECVLC_DISABLE_MASK);
		adau1761_write_reg_byte(codec, ADAU_RECVLCL, regvalue);
		regvalue = (adau1761_read_reg_cache(codec,ADAU_RECVLCR)
				& RECVLC_DISABLE_MASK);
		adau1761_write_reg_byte(codec, ADAU_RECVLCR, regvalue);
		adau1761_write_reg_byte(codec, ADAU_RECMLC1, RECMLC_LINE_0DB);
		adau1761_write_reg_byte(codec, ADAU_RECMRC1, RECMLC_LINE_0DB);
	#endif
	adau1761_alc_disable(codec);
	snd_soc_dapm_enable_pin(codec, "LineIn");
}

static void adau1761_line_in_disable(struct snd_soc_codec *codec)
{
	snd_soc_dapm_disable_pin(codec, "LineIn");
}

static void adau1761_ecm_mic_enable(struct snd_soc_codec *codec, int gain_dB, int HPF)
{
	int ldvol = LDVOL_0dB; 
	int ldboost = RECMLC_MIC_0DB;

	if (gain_dB > 20) {
		gain_dB -= 20;
		ldboost = RECMLC_MIC_20DB;
	}
	ldvol += ((gain_dB * 1024) / LDVOL_STEP); 

	adau1761_mic_bias_enable(codec);

	/* left and right differential input control */
	snd_soc_update_bits(codec, ADAU_RECVLCR-ADAU_FIRSTREG, 
		RECVLC_LDEN_MASK | RECVLC_LDMUTE_MASK, 0);
	snd_soc_update_bits(codec, ADAU_RECVLCL-ADAU_FIRSTREG, 
		RECVLC_LDEN_MASK | RECVLC_LDMUTE_MASK | RECVLC_LDVOL_MASK,
		(1 << RECVLC_LDEN_BITS) | (1 << RECVLC_LDMUTE_BITS) |
		(ldvol << RECVLC_LDVOL_BITS));

	/* left channel mixer 1 mic boost, right channel mixer 2 mute */
	snd_soc_update_bits(codec, ADAU_RECMRC1-ADAU_FIRSTREG, 
		RECMLC_MX1AUXG_MASK | RECMLC_LDBOOST_MASK, 0);
	snd_soc_update_bits(codec, ADAU_RECMLC1-ADAU_FIRSTREG, 
		RECMLC_MX1AUXG_MASK | RECMLC_LDBOOST_MASK, 0 | ldboost);

	/* mixer 1 enable */
	snd_soc_update_bits(codec, ADAU_RECMLC0-ADAU_FIRSTREG, 
		RECMLC_MX1EN_MASK, (1 << RECMLC_MX1EN_BITS));

	adau1761_alc_disable(codec);

	snd_soc_dapm_enable_pin(codec, "MicIn");
}

static void adau1761_ecm_mic_disable(struct snd_soc_codec *codec)
{
	/* adau1761_mic_bias_disable(codec); */

	/* left differential input control mute */
	snd_soc_update_bits(codec, ADAU_RECVLCL-ADAU_FIRSTREG, 
		RECVLC_LDEN_MASK | RECVLC_LDMUTE_MASK | RECVLC_LDVOL_MASK, 0);

	/* left channel mixer 1 mute */
	snd_soc_update_bits(codec, ADAU_RECMLC1-ADAU_FIRSTREG, 
		RECMLC_LDBOOST_MASK, 0);

	snd_soc_dapm_disable_pin(codec, "MicIn");
}

static int adau1761_set_scenario_endpoints(struct tt_scenario_context *c)
{
	struct snd_kcontrol	*kcontrol = (struct snd_kcontrol *) c->data;
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

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
		adau1761_mic_bias_enable(codec);
	case TT_SCENARIO_AUDIO_OFF:
		c->tt_volume = 0;
		/* mono output control */
		snd_soc_update_bits(codec, ADAU_PLBMNOC-ADAU_FIRSTREG, 
			ADAU1761_MUTE_MASK, 0);
		snd_soc_update_bits(codec, ADAU_PLBMNOC-ADAU_FIRSTREG, 
			ADAU1761_VOLUME_MASK, 0);
		/* line out output control */
		snd_soc_update_bits(codec, ADAU_PLBLOVL-ADAU_FIRSTREG, 
			ADAU1761_MUTE_MASK, 0);
		snd_soc_update_bits(codec, ADAU_PLBLOVR-ADAU_FIRSTREG, 
			ADAU1761_MUTE_MASK, 0);
		snd_soc_update_bits(codec, ADAU_PLBLOVL-ADAU_FIRSTREG, 
			ADAU1761_VOLUME_MASK, 0);
		snd_soc_update_bits(codec, ADAU_PLBLOVR-ADAU_FIRSTREG, 
			ADAU1761_VOLUME_MASK, 0);
		/* headphone output control */
		snd_soc_update_bits(codec, ADAU_PLBHPVL-ADAU_FIRSTREG,
			ADAU1761_MUTE_MASK, 0);
		snd_soc_update_bits(codec, ADAU_PLBHPVR-ADAU_FIRSTREG, 
			ADAU1761_MUTE_MASK, 0);
		snd_soc_update_bits(codec, ADAU_PLBHPVL-ADAU_FIRSTREG, 
			ADAU1761_VOLUME_MASK, 0);
		snd_soc_update_bits(codec, ADAU_PLBHPVR-ADAU_FIRSTREG, 
			ADAU1761_VOLUME_MASK, 0);
		switch(c->tt_scenario) {
		case TT_SCENARIO_VR:
			adau1761_alc_disable(codec);
		case TT_SCENARIO_NOISE:
		case TT_SCENARIO_ASR:
			adau1761_ecm_mic_disable(codec);
			break;
		case TT_SCENARIO_HF_SPEAKER:
			adau1761_ecm_mic_disable(codec);
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
			adau1761_ecm_mic_disable(codec);
			snd_soc_dapm_disable_pin(codec, "Headphones");
			break;
		case TT_SCENARIO_iPod:
			adau1761_line_in_disable(codec);
			adau1761_mic_bias_enable(codec);
			break;
		case TT_SCENARIO_iPod_SPEAKER:
			adau1761_line_in_disable(codec);
			adau1761_mic_bias_enable(codec);
			snd_soc_dapm_disable_pin(codec, "Speaker");
			break;
		case TT_SCENARIO_iPod_LINEOUT:
		case TT_SCENARIO_iPod_FM:
			adau1761_line_in_disable(codec);
			adau1761_mic_bias_enable(codec);
			snd_soc_dapm_disable_pin(codec, "Headphones");
			break;
		default:
			snd_soc_dapm_disable_pin(codec, "Headphones");
			snd_soc_dapm_disable_pin(codec, "Speaker");
			snd_soc_dapm_disable_pin(codec, "MonoOut");
			snd_soc_dapm_disable_pin(codec, "MicIn");
			snd_soc_dapm_disable_pin(codec, "LineIn");
			break;
		}
		break;
	case TT_SCENARIO_ASR:
		adau1761_ecm_mic_enable(codec, 42, 1);
		break;
	case TT_SCENARIO_VR:
		adau1761_ecm_mic_enable(codec, 45, 1);
		break;
	case TT_SCENARIO_NOISE:
		adau1761_ecm_mic_enable(codec, 30, 1);
		break;
	case TT_SCENARIO_iPod:
		adau1761_line_in_enable(codec);
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
		adau1761_line_in_enable(codec);
		snd_soc_dapm_enable_pin(codec, "Speaker");
		break;
	case TT_SCENARIO_iPod_LINEOUT:
	case TT_SCENARIO_iPod_FM:
		adau1761_line_in_enable(codec);
		snd_soc_dapm_enable_pin(codec, "Headphones");
		break;
	case TT_SCENARIO_HF_SPEAKER:
		adau1761_ecm_mic_enable(codec, 28, 1);
		snd_soc_dapm_enable_pin(codec, "Speaker");
		break;
	case TT_SCENARIO_HF_LINEOUT:
	case TT_SCENARIO_HF_FM:
		adau1761_ecm_mic_enable(codec, 28, 1);
		snd_soc_dapm_enable_pin(codec, "Headphones");
		break;
	}

	c->tt_current_scenario = c->tt_scenario;
	snd_soc_dapm_sync(codec);

	/* output volume preparations (digital control as master control) */ 
	snd_soc_update_bits(codec, ADAU_DACCTL1-ADAU_FIRSTREG, 
		ADAU1761_ADVOL_MASK, 0xff);
	snd_soc_update_bits(codec, ADAU_DACCTL2-ADAU_FIRSTREG, 
		ADAU1761_ADVOL_MASK, 0xff);
	switch(c->tt_scenario) {
	case TT_SCENARIO_LINEOUT:
	case TT_SCENARIO_iPod_LINEOUT:
	case TT_SCENARIO_FM:
	case TT_SCENARIO_iPod_FM:
		snd_soc_update_bits(codec, ADAU_PLBHPVL-ADAU_FIRSTREG, 
			ADAU1761_MUTE_MASK, ADAU1761_MUTE_MASK);
		snd_soc_update_bits(codec, ADAU_PLBHPVR-ADAU_FIRSTREG, 
			ADAU1761_MUTE_MASK, ADAU1761_MUTE_MASK);
		snd_soc_update_bits(codec, ADAU_PLBHPVL-ADAU_FIRSTREG, 
			ADAU1761_VOLUME_MASK, (57 << ADAU1761_VOLUME_BITS));
		snd_soc_update_bits(codec, ADAU_PLBHPVR-ADAU_FIRSTREG, 
			ADAU1761_VOLUME_MASK, (57 << ADAU1761_VOLUME_BITS));
		break;
	default:
	case TT_SCENARIO_AUDIO_INIT:
	case TT_SCENARIO_AUDIO_OFF:
		break;
	case TT_SCENARIO_SPEAKER:
	case TT_SCENARIO_iPod_SPEAKER:
	case TT_SCENARIO_HF_SPEAKER:
		snd_soc_update_bits(codec, ADAU_PLBLOVR-ADAU_FIRSTREG, 
			ADAU1761_VOLUME_MASK, (57 << ADAU1761_VOLUME_BITS));
		snd_soc_update_bits(codec, ADAU_PLBLOVR-ADAU_FIRSTREG, 
			ADAU1761_MUTE_MASK, ADAU1761_MUTE_MASK);
		break;
	}

	return 1;
}

static int adau1761_set_scenario(struct tt_scenario_context *context)
{
	adau1761_set_scenario_endpoints(context);
	return 1;
}

static int adau1761_set_scenario_mode(struct tt_scenario_context *context)
{
	adau1761_set_scenario_endpoints(context);
	return 1;
}

static struct tt_scenario_ops adau1761_scenario_ops = {
	.set_volume			= adau1761_set_volume,
	.set_volume_db		= adau1761_set_volume_dB,
	.set_scenario		= adau1761_set_scenario,
	.set_scenario_mode	= adau1761_set_scenario_mode
};

static int adau1761_scenario_register(void)
{
	tt_scenario_register(&adau1761_scenario_ops);

	return 0;
}

late_initcall(adau1761_scenario_register);
