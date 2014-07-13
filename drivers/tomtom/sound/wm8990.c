/* drivers/tomtom/sound/wm8990.c
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
#include "../../../sound/soc/codecs/wm8990.h"

#define DUMMY_CALLBACK(name)	static int wm8990_##name(struct tt_scenario_context *c) \
				{							\
					printk("%s\n", __func__);			\
					/* Do nothing */				\
				}


#define VOLUME_MUTE		0x2F /* Table 36: < -73 dB */
#define VOLUME_0DB		0x79 /* Table 36: 0 dB */
#define VOLUME_RANGE_DB	(VOLUME_0DB - VOLUME_MUTE)

#define LDVOL_STEP (1536) /* 1.5*1024 */
#define LDVOL_0dB (0x0B)

static int wm8990_set_volume(struct tt_scenario_context *c)
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
#ifdef _ENABLE_SCENARIO_
		/* Speaker, OUT3, OUT4, LOUT, ROUT */
		snd_soc_update_bits(codec, WM8990_POWER_MANAGEMENT_1, 
			WM8990_SPK_ENA | WM8990_OUT3_ENA | WM8990_OUT4_ENA |\
			WM8990_LOUT_ENA | WM8990_ROUT_ENA, 
			(0 << WM8990_SPK_ENA_BIT) | (0 << WM8990_OUT3_ENA_BIT) |\
			(0 << WM8990_OUT4_ENA_BIT) | (0 << WM8990_LOUT_ENA_BIT) |\
			(0 << WM8990_ROUT_ENA_BIT));

		/* LON, LOP, RON, ROP */
		snd_soc_update_bits(codec, WM8990_POWER_MANAGEMENT_3, 
			WM8990_LON_ENA | WM8990_LOP_ENA | WM8990_RON_ENA | WM8990_ROP_ENA,
			(0 << WM8990_LON_ENA_BIT) | (0 << WM8990_LOP_ENA_BIT) |\
			(0 << WM8990_RON_ENA_BIT) | (0 << WM8990_ROP_ENA_BIT));
#endif /* _ENABLE_SCENARIO_ */
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
			volume = VOLUME_MUTE + (((c->tt_volume * 1000) / 100) * VOLUME_RANGE_DB) / 1000;
			break;
		case TT_MODE_TTS:
			volume = VOLUME_MUTE + (((c->tt_volume * 1000) / 100) * VOLUME_RANGE_DB) / 1000;
			/* scale to loudness of source (~ 9 - 12 dB difference
			   with SPP voices) audio engine will scale the max 
			   volumes with audio processing */ 
			volume -= 9;
			if (volume < 0)
				volume = 0;
			break;
		}
		snd_soc_update_bits(codec, WM8990_CLASSD4,
			WM8990_SPKVOL_MASK, volume);
		break;
	case TT_SCENARIO_FM:
	case TT_SCENARIO_iPod_FM:
		volume = 48; /* 48 = ~ -18 dB */
		snd_soc_update_bits(codec, WM8990_OUT3_4_VOLUME,
			WM8990_OUT3MUTE | WM8990_OUT4MUTE, 
			(1 << WM8990_OUT3MUTE_BIT) | (1 << WM8990_OUT4MUTE_BIT));
		break;
	case TT_SCENARIO_HF_SPEAKER:
		/* use 30 dB range for now on speaker playback */
		if ((volume = c->tt_volume) < 15) {
			volume = VOLUME_MUTE;
		}
		if (volume > 85) {
			volume = VOLUME_0DB; /* max volume 0dB */
		} else {
			volume = VOLUME_MUTE + (((c->tt_volume * 1000) / 100) * VOLUME_RANGE_DB) / 1000;
		}
		snd_soc_update_bits(codec, WM8990_LEFT_OUTPUT_VOLUME,
			WM8990_LOUTVOL_MASK, volume);
		snd_soc_update_bits(codec, WM8990_RIGHT_OUTPUT_VOLUME,
			WM8990_ROUTVOL_MASK, volume);
		snd_soc_update_bits(codec, WM8990_LEFT_OUTPUT_VOLUME,
			WM8990_OPVU, (1 << 8));
		snd_soc_update_bits(codec, WM8990_RIGHT_OUTPUT_VOLUME,
			WM8990_OPVU, (1 << 8));
		break;
	}
	return 1;
}

/* user of this function will pass in a the gain as a 
   possitive value which should be interpreted as negative */
static int wm8990_set_volume_dB(struct tt_scenario_context *c)
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
#ifdef _ENABLE_SCENARIO_
		/* Speaker, OUT3, OUT4, LOUT, ROUT */
		snd_soc_update_bits(codec, WM8990_POWER_MANAGEMENT_1, 
			WM8990_SPK_ENA | WM8990_OUT3_ENA | WM8990_OUT4_ENA |\
			WM8990_LOUT_ENA | WM8990_ROUT_ENA, 
			(0 << WM8990_SPK_ENA_BIT) | (0 << WM8990_OUT3_ENA_BIT) |\
			(0 << WM8990_OUT4_ENA_BIT) | (0 << WM8990_LOUT_ENA_BIT) |\
			(0 << WM8990_ROUT_ENA_BIT));

		/* LON, LOP, RON, ROP */
		snd_soc_update_bits(codec, WM8990_POWER_MANAGEMENT_3, 
			WM8990_LON_ENA | WM8990_LOP_ENA | WM8990_RON_ENA | WM8990_ROP_ENA,
			(0 << WM8990_LON_ENA_BIT) | (0 << WM8990_LOP_ENA_BIT) |\
			(0 << WM8990_RON_ENA_BIT) | (0 << WM8990_ROP_ENA_BIT));
#endif /* _ENABLE_SCENARIO_ */
		break;
	case TT_SCENARIO_SPEAKER:
	case TT_SCENARIO_iPod_SPEAKER:
		switch(c->tt_scenario_mode){
		default:
			volume = VOLUME_0DB - (((c->tt_volume_dB * 1000) / 90) * VOLUME_RANGE_DB) / 1000;
			break;
		}
		snd_soc_update_bits(codec, WM8990_CLASSD4,
			WM8990_SPKVOL_MASK, volume);
		break;
	case TT_SCENARIO_HF_SPEAKER:
		switch(c->tt_scenario_mode){
		default:
			volume = VOLUME_0DB - (((c->tt_volume_dB * 1000) / 90) * VOLUME_RANGE_DB) / 1000;
			break;
		}
		snd_soc_update_bits(codec, WM8990_CLASSD4,
			WM8990_SPKVOL_MASK, volume);
		break;
	case TT_SCENARIO_FM:
	case TT_SCENARIO_iPod_FM:
		volume = 48; /* 48 = ~ -18 dB */
		snd_soc_update_bits(codec, WM8990_OUT3_4_VOLUME,
			WM8990_OUT3MUTE | WM8990_OUT4MUTE, 
			(1 << WM8990_OUT3MUTE_BIT) | (1 << WM8990_OUT4MUTE_BIT));
		break;
	}
	return 1;
}

static void wm8990_mic_bias_enable(struct snd_soc_codec *codec)
{
	/* Set MBSEL to 0.9 * AVDD  (0 = 0.65*AVDD, 1=0.9*AVDD) */
	snd_soc_update_bits(codec, WM8990_MICBIAS, WM8990_MBSEL, (0 << 0));

	/* Microphone bias on */
	snd_soc_update_bits(codec, WM8990_POWER_MANAGEMENT_1,
		WM8990_MICBIAS_ENA, (1 << WM8990_MICBIAS_ENA_BIT));
}

static void wm8990_mic_bias_disable(struct snd_soc_codec *codec)
{
	/* Microphone bias off */
	snd_soc_update_bits(codec, WM8990_POWER_MANAGEMENT_1,
		WM8990_MICBIAS_ENA, (0 << WM8990_MICBIAS_ENA_BIT));
}

static void wm8990_alc_disable(struct snd_soc_codec *codec)
{
	/* ALC off */
}

static void wm8990_line_in_enable(struct snd_soc_codec *codec)
{
	wm8990_mic_bias_disable(codec);

#ifdef _ENABLE_SCENARIO_
	/* Unmute and set volume to 0 dB */
	snd_soc_update_bits(codec, WM8990_LEFT_LINE_INPUT_1_2_VOLUME,
		WM8990_LI12MUTE | WM8990_LIN12VOL_MASK,
		(0 << WM8990_LI12MUTE_BIT) | (LDVOL_0dB));
	snd_soc_update_bits(codec, WM8990_RIGHT_LINE_INPUT_1_2_VOLUME,
		WM8990_RI12MUTE | WM8990_RIN12VOL_MASK,
		(0 << WM8990_RI12MUTE_BIT) | (LDVOL_0dB));

	/* INMIXL & INMIXR */
	snd_soc_update_bits(codec, WM8990_INPUT_MIXER1,
		WM8990_AINLMODE_MASK | WM8990_AINRMODE_MASK,
		(0 < WM8990_AINLMODE_SHIFT) | (0 < WM8990_AINRMODE_SHIFT));

	/* Mute LIN34, unmute LIN12, 0dB, 0 dB */ 
	snd_soc_update_bits(codec, WM8990_INPUT_MIXER3,
		WM8990_L34MNB | WM8990_L12MNB | WM8990_L12MNBST | WM8990_LDBVOL_MASK,
		(0 << WM8990_L34MNB_BIT) | (1 << WM8990_L12MNB_BIT) |\
		(0 << WM8990_L12MNBST_BIT) | (5 << WM8990_LDBVOL_SHIFT));

	/* Mute RIN34, unmute RIN12, 0dB, 0 dB */ 
	snd_soc_update_bits(codec, WM8990_INPUT_MIXER4,
		WM8990_R34MNB | WM8990_R12MNB | WM8990_R12MNBST | WM8990_RDBVOL_MASK,
		(0 << WM8990_R34MNB_BIT) | (1 << WM8990_R12MNB_BIT) |\
		(0 << WM8990_R12MNBST_BIT) | (5 << WM8990_RDBVOL_SHIFT));

	/* Enable LIN2 & RIN2 */
	snd_soc_update_bits(codec, WM8990_INPUT_MIXER2,
		WM8990_LMN1 | WM8990_LMP2 | WM8990_RMP2 | WM8990_RMN1,
		(0 << WM8990_LMN1_BIT) | (1 << WM8990_LMP2_BIT) | \
		(1 << WM8990_RMP2_BIT) | (0 << WM8990_RMN1_BIT));

	/* Disable LIN34 & RIN34, enable LIN2 & RIN2 */
	snd_soc_update_bits(codec, WM8990_POWER_MANAGEMENT_2,
		WM8990_LIN12_ENA | WM8990_RIN12_ENA | \
		WM8990_LIN34_ENA | WM8990_RIN34_ENA | \
		WM8990_AINL_ENA | WM8990_AINR_ENA,
		(1 << WM8990_LIN12_ENA_BIT) | (1 << WM8990_RIN12_ENA) |\
		(0 << WM8990_LIN34_ENA_BIT) | (0 << WM8990_RIN34_ENA_BIT) |\
		(1 << WM8990_AINL_ENA_BIT) | (1 << WM8990_AINR_ENA_BIT));
#endif /* _ENABLE_SCENARIO_ */

	wm8990_alc_disable(codec);
	snd_soc_dapm_enable_pin(codec, "LineIn");
}

static void wm8990_line_in_disable(struct snd_soc_codec *codec)
{
	snd_soc_dapm_disable_pin(codec, "LineIn");
}

static void wm8990_mic_enable(struct snd_soc_codec *codec, int gain_dB, int HPF)
{
	int ldvol = LDVOL_0dB; 

	if (gain_dB > 30) {
		gain_dB -= 30;
	}
	ldvol += ((gain_dB * 1024) / LDVOL_STEP); 

	wm8990_mic_bias_enable(codec);

	/* Unmute and set volume to 0 dB */
	snd_soc_update_bits(codec, WM8990_RIGHT_LINE_INPUT_3_4_VOLUME,
		WM8990_IPVU | WM8990_RI34MUTE | WM8990_RIN34VOL_MASK,
		WM8990_IPVU | (0 << WM8990_RI34MUTE_BIT) | (ldvol & 0x001F));

	/* Unmute RIN34, 0dB 0dB */ 
	snd_soc_update_bits(codec, WM8990_INPUT_MIXER4,
		WM8990_R12MNB | WM8990_R34MNB | WM8990_R34MNBST | WM8990_RDBVOL_MASK,
		(0 << WM8990_R12MNB_BIT) | (1 << WM8990_R34MNB_BIT) |\
		(0 << WM8990_R34MNBST_BIT) | (5 << WM8990_RDBVOL_SHIFT));

	/* INMIXR */
	snd_soc_update_bits(codec, WM8990_INPUT_MIXER1,
		WM8990_AINRMODE_MASK, (0 < WM8990_AINRMODE_SHIFT));

	/* Enable RIN34 */
	snd_soc_update_bits(codec, WM8990_INPUT_MIXER2,
		WM8990_LMP4 | WM8990_LMN3 | WM8990_LMP2 | WM8990_LMN1 |\
		WM8990_RMP4 | WM8990_RMN3 | WM8990_RMP2 | WM8990_RMN1,
		(1 << WM8990_RMP4_BIT) | (1 << WM8990_RMN3_BIT));

	/* Enable RIN34, disable LIN2 & RIN2 */
	snd_soc_update_bits(codec, WM8990_POWER_MANAGEMENT_2,
		WM8990_LIN12_ENA | WM8990_RIN12_ENA | \
		WM8990_LIN34_ENA | WM8990_RIN34_ENA | \
		WM8990_AINL_ENA | WM8990_AINR_ENA |\
		WM8990_ADCL_ENA | WM8990_ADCR_ENA,
		(0 << WM8990_LIN12_ENA_BIT) | (0 << WM8990_RIN12_ENA) |\
		(0 << WM8990_LIN34_ENA_BIT) | (1 << WM8990_RIN34_ENA_BIT) |\
		(0 << WM8990_AINL_ENA_BIT) | (1 << WM8990_AINR_ENA_BIT) |\
		(0 << WM8990_ADCL_ENA_BIT) | (1 << WM8990_ADCR_ENA_BIT));

	/* Right digital volume 0dB */
	snd_soc_update_bits(codec, WM8990_RIGHT_ADC_DIGITAL_VOLUME, 
		WM8990_ADC_VU | WM8990_ADCR_VOL_MASK,
		WM8990_ADCR_VOL_MASK | (0xC0 << WM8990_ADCR_VOL_SHIFT));

	wm8990_alc_disable(codec);

	snd_soc_dapm_enable_pin(codec, "MicIn");
}

static void wm8990_mic_disable(struct snd_soc_codec *codec)
{
	wm8990_mic_bias_disable(codec);

	/* Mute RI34 */
	snd_soc_update_bits(codec, WM8990_RIGHT_LINE_INPUT_3_4_VOLUME,
		WM8990_RI34MUTE, (1 << WM8990_RI34MUTE_BIT));

	/* Mute RIN34 */ 
	snd_soc_update_bits(codec, WM8990_INPUT_MIXER4,
		WM8990_R34MNB, (0 << WM8990_R34MNB_BIT));

	/* Disable RIN34 */
	snd_soc_update_bits(codec, WM8990_INPUT_MIXER2,
		WM8990_RMP4 | WM8990_RMN3, 
		(0 << WM8990_RMP4_BIT) | (0 << WM8990_RMN3_BIT));

	/* Disable RIN34 */
	snd_soc_update_bits(codec, WM8990_POWER_MANAGEMENT_2,
		WM8990_RIN34_ENA | WM8990_AINR_ENA | WM8990_ADCR_ENA,
		(0 << WM8990_RIN34_ENA_BIT) | (0 << WM8990_AINR_ENA_BIT) |\
		(0 << WM8990_ADCR_ENA_BIT));

	/* Disable right input path and RIN34 input PGA */
	snd_soc_update_bits(codec, WM8990_POWER_MANAGEMENT_2,
		WM8990_RIN34_ENA | WM8990_AINR_ENA,
		(0 << WM8990_RIN34_ENA_BIT) | (0 << WM8990_AINR_ENA_BIT));

	/* Mute right digital volume */
	snd_soc_update_bits(codec, WM8990_RIGHT_ADC_DIGITAL_VOLUME, 
		WM8990_ADC_VU | WM8990_ADCR_VOL_MASK,
		WM8990_ADCR_VOL_MASK | (0x00 << WM8990_ADCR_VOL_SHIFT));

	snd_soc_dapm_disable_pin(codec, "MicIn");
}

static int wm8990_set_scenario_endpoints(struct tt_scenario_context *c)
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
		wm8990_mic_bias_enable(codec);
	case TT_SCENARIO_AUDIO_OFF:
		c->tt_volume = 0;
#ifdef _ENABLE_SCENARIO_
		/* Speaker, OUT3, OUT4, LOUT, ROUT */
		snd_soc_update_bits(codec, WM8990_POWER_MANAGEMENT_1, 
			WM8990_SPK_ENA | WM8990_OUT3_ENA | WM8990_OUT4_ENA |\
			WM8990_LOUT_ENA | WM8990_ROUT_ENA, 
			(0 << WM8990_SPK_ENA_BIT) | (0 << WM8990_OUT3_ENA_BIT) |\
			(0 << WM8990_OUT4_ENA_BIT) | (0 << WM8990_LOUT_ENA_BIT) |\
			(0 << WM8990_ROUT_ENA_BIT));

		/* LON, LOP, RON, ROP */
		snd_soc_update_bits(codec, WM8990_POWER_MANAGEMENT_3, 
			WM8990_LON_ENA | WM8990_LOP_ENA | WM8990_RON_ENA | WM8990_ROP_ENA,
			(0 << WM8990_LON_ENA_BIT) | (0 << WM8990_LOP_ENA_BIT) |\
			(0 << WM8990_RON_ENA_BIT) | (0 << WM8990_ROP_ENA_BIT));
#endif /* _ENABLE_SCENARIO_ */
		switch(c->tt_current_scenario) {
		case TT_SCENARIO_VR:
			wm8990_alc_disable(codec);
		case TT_SCENARIO_NOISE:
		case TT_SCENARIO_ASR:
			wm8990_mic_disable(codec);
			break;
		case TT_SCENARIO_HF_SPEAKER:
			wm8990_mic_disable(codec);
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
			wm8990_mic_disable(codec);
			snd_soc_dapm_disable_pin(codec, "Headphones");
			break;
		case TT_SCENARIO_iPod:
			wm8990_line_in_disable(codec);
			wm8990_mic_bias_enable(codec);
			break;
		case TT_SCENARIO_iPod_SPEAKER:
			wm8990_line_in_disable(codec);
			wm8990_mic_bias_enable(codec);
			snd_soc_dapm_disable_pin(codec, "Speaker");
			break;
		case TT_SCENARIO_iPod_LINEOUT:
		case TT_SCENARIO_iPod_FM:
			wm8990_line_in_disable(codec);
			wm8990_mic_bias_enable(codec);
			snd_soc_dapm_disable_pin(codec, "Headphones");
			break;
		default:
			snd_soc_dapm_disable_pin(codec, "Headphones");
			snd_soc_dapm_disable_pin(codec, "Speaker");
			snd_soc_dapm_disable_pin(codec, "MicIn");
			snd_soc_dapm_disable_pin(codec, "LineIn");
			break;
		}
		break;
	case TT_SCENARIO_ASR:
		wm8990_mic_enable(codec, 25, 1);
		break;
	case TT_SCENARIO_VR:
		wm8990_mic_enable(codec, 25, 1);
		break;
	case TT_SCENARIO_NOISE:
		wm8990_mic_enable(codec, 15, 1);
		break;
	case TT_SCENARIO_iPod:
		wm8990_line_in_enable(codec);
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
		wm8990_line_in_enable(codec);
		snd_soc_dapm_enable_pin(codec, "Speaker");
		break;
	case TT_SCENARIO_iPod_LINEOUT:
	case TT_SCENARIO_iPod_FM:
		wm8990_line_in_enable(codec);
		snd_soc_dapm_enable_pin(codec, "Headphones");
		break;
	case TT_SCENARIO_HF_SPEAKER:
		wm8990_mic_enable(codec, 13, 1);
		snd_soc_dapm_enable_pin(codec, "Speaker");
		break;
	case TT_SCENARIO_HF_LINEOUT:
	case TT_SCENARIO_HF_FM:
		wm8990_mic_enable(codec, 13, 1);
		snd_soc_dapm_enable_pin(codec, "Headphones");
		break;
	}

	c->tt_current_scenario = c->tt_scenario;
	snd_soc_dapm_sync(codec);

#ifdef _ENABLE_SCENARIO_
	/* output volume preparations (digital control as master control) */ 
	snd_soc_update_bits(codec, WM8990_LEFT_DAC_DIGITAL_VOLUME,
		WM8990_DAC_VU | WM8990_DACL_VOL_MASK, WM8990_DAC_VU | 0x00);
	snd_soc_update_bits(codec, WM8990_RIGHT_DAC_DIGITAL_VOLUME,
		WM8990_DAC_VU | WM8990_DACR_VOL_MASK, WM8990_DAC_VU | 0x00);
#endif /* _ENABLE_SCENARIO_ */
	switch(c->tt_scenario) {
	case TT_SCENARIO_LINEOUT:
	case TT_SCENARIO_iPod_LINEOUT:
	case TT_SCENARIO_FM:
	case TT_SCENARIO_iPod_FM:
#ifdef _ENABLE_SCENARIO_
		/* OUT3, OUT4, LOUT, ROUT */
		snd_soc_update_bits(codec, WM8990_POWER_MANAGEMENT_1, 
			WM8990_SPK_ENA | WM8990_OUT3_ENA | WM8990_OUT4_ENA |\
			WM8990_LOUT_ENA | WM8990_ROUT_ENA, 
			(1 << WM8990_OUT3_ENA_BIT) | (1 << WM8990_OUT4_ENA_BIT) |\
			(1 << WM8990_LOUT_ENA_BIT) | (1 << WM8990_ROUT_ENA_BIT));
#endif /* _ENABLE_SCENARIO_ */
		break;
	default:
	case TT_SCENARIO_AUDIO_INIT:
	case TT_SCENARIO_AUDIO_OFF:
		break;
	case TT_SCENARIO_SPEAKER:
	case TT_SCENARIO_iPod_SPEAKER:
	case TT_SCENARIO_HF_SPEAKER:
#ifdef _ENABLE_SCENARIO_
		/* Speaker, OUT3, OUT4, LOUT, ROUT */
		snd_soc_update_bits(codec, WM8990_POWER_MANAGEMENT_1, 
			WM8990_SPK_ENA, (1 << WM8990_SPK_ENA_BIT));
#endif /* _ENABLE_SCENARIO_ */
		break;
	}

	return 1;
}

static int wm8990_set_scenario(struct tt_scenario_context *context)
{
	wm8990_set_scenario_endpoints(context);
	return 1;
}

static int wm8990_set_scenario_mode(struct tt_scenario_context *context)
{
	wm8990_set_scenario_endpoints(context);
	return 1;
}

static struct tt_scenario_ops wm8990_scenario_ops = {
	.set_volume			= wm8990_set_volume,
	.set_volume_db		= wm8990_set_volume_dB,
	.set_scenario		= wm8990_set_scenario,
	.set_scenario_mode	= wm8990_set_scenario_mode
};

static int wm8990_scenario_register(void)
{
	tt_scenario_register(&wm8990_scenario_ops);

	return 0;
}

late_initcall(wm8990_scenario_register);
