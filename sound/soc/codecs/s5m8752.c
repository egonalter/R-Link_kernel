/*
 * s5m8752.c  --  S5M8752 Power-Audio IC ALSA Soc Audio driver
 *
 * Copyright 2010 Samsung Electronics.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/tlv.h>
#include <sound/initval.h>
#include <asm/div64.h>
#include <linux/mfd/s5m8752/core.h>
#include <linux/mfd/s5m8752/audio.h>
#include <linux/mfd/s5m8752/pdata.h>

#define S5M8752_VERSION "0.0"

struct s5m8752_priv {
	struct snd_soc_codec codec;
	struct s5m8752 *s5m8752;
};

struct sample_rate {
	unsigned int rate;
	uint8_t reg_rate;
};

/* S5M8752 audio codec sampling rate */
static struct sample_rate s5m8752_rate[] = {
	{ 8000, S5M8752_SAMP_RATE_8K },
	{ 11025, S5M8752_SAMP_RATE_11_025K },
	{ 16000, S5M8752_SAMP_RATE_16K },
	{ 22050, S5M8752_SAMP_RATE_22_05K },
	{ 32000, S5M8752_SAMP_RATE_32K },
	{ 44100, S5M8752_SAMP_RATE_44_1K },
	{ 48000, S5M8752_SAMP_RATE_48K },
	{ 64000, S5M8752_SAMP_RATE_64K },
	{ 88200, S5M8752_SAMP_RATE_88_2K },
	{ 96000, S5M8752_SAMP_RATE_96K },
};

/******************************************************************************
 * This function reads the I2C data of S5M8752 codec.
 ******************************************************************************/
static inline unsigned int s5m8752_codec_read(struct snd_soc_codec *codec,
					unsigned int reg)
{
	uint8_t value;
	struct s5m8752_priv *priv = snd_soc_codec_get_drvdata(codec);

	s5m8752_reg_read(priv->s5m8752, reg, &value);

	return value;
}

/******************************************************************************
 * This function writes the I2C data of S5M8752 codec.
 ******************************************************************************/
static inline int s5m8752_codec_write(struct snd_soc_codec *codec,
					unsigned int reg, unsigned int value)
{
	struct s5m8752_priv *priv = snd_soc_codec_get_drvdata(codec);
	return s5m8752_reg_write(priv->s5m8752, reg, value);
}

static const DECLARE_TLV_DB_SCALE(adc_tlv, -12, 6, 1);
static const DECLARE_TLV_DB_SCALE(hp_offset, -16, 14, 1);

static const struct snd_kcontrol_new s5m8752_snd_controls[] = {
SOC_DOUBLE_TLV("AD-line Volume", S5M8752_AD_LINEVOL, 4, 0, 0x7, 1, adc_tlv),
SOC_SINGLE("MIC Boost Gain", S5M8752_AD_BST, 4, 0x3, 1),
SOC_SINGLE("DIFFL Gain", S5M8752_AD_BST, 0, 0x7, 1),
SOC_SINGLE("RCH-PGA Gain", S5M8752_AD_PGAR, 0, 0x3F, 1),
SOC_DOUBLE_R("ADC Volume", S5M8752_AD_VOLL, S5M8752_AD_VOLR, 0, 0xFF, 1),
SOC_SINGLE("ADC PGA Max Gain", S5M8752_AD_ALC2, 0, 0x3F, 1),
SOC_SINGLE("ADC PGA Min Gain", S5M8752_AD_ALC3, 0, 0x3F, 1),
SOC_DOUBLE_R("ADC Input level", S5M8752_AD_ALC4, S5M8752_AD_ALC5, 0, 0x1F, 1),
SOC_SINGLE("AD ALC Hold Time", S5M8752_AD_ALC6, 0, 0x1F, 1),
SOC_SINGLE("AD ALC Decay Time", S5M8752_AD_ALC7, 0, 0x1F, 1),
SOC_SINGLE("AD ALC Attack Time", S5M8752_AD_ALC8, 0, 0x1F, 1),
SOC_SINGLE("AD ALC Noise Thr.", S5M8752_AD_ALC9, 3, 0x1F, 1),
SOC_SINGLE("DCT Current", S5M8752_REF1, 3, 0x7, 1),
SOC_SINGLE("Mixer Current", S5M8752_REF1, 0, 0x7, 1),
SOC_SINGLE("REF Current", S5M8752_REF2, 0, 0x7, 1),
SOC_SINGLE("Mixer CH1 level", S5M8752_DA_DMIX1, 0, 0x7, 1),
SOC_SINGLE("Mixer CH2 level", S5M8752_DA_DMIX1, 4, 0x7, 1),
SOC_SINGLE("Mixer CH3 level", S5M8752_DA_DMIX2, 0, 0x7, 1),
SOC_DOUBLE_R("DAC Volume", S5M8752_DA_DVO1, S5M8752_DA_DVO2, 0, 0xFF, 1),
SOC_SINGLE("DA Offset", S5M8752_DA_OFF, 0, 0xFF, 1),
SOC_SINGLE("DAC PGA Max Gain", S5M8752_DA_ALC2, 0, 0x3F, 1),
SOC_SINGLE("DAC PGA Min Gain", S5M8752_DA_ALC3, 0, 0x3F, 1),
SOC_DOUBLE_R("DAC Input level", S5M8752_DA_ALC4, S5M8752_DA_ALC5, 0, 0x1F, 1),
SOC_SINGLE("DA ALC Hold Time", S5M8752_DA_ALC6, 0, 0x1F, 1),
SOC_SINGLE("DA ALC Decay Time", S5M8752_DA_ALC7, 0, 0x1F, 1),
SOC_SINGLE("DA ALC Attack Time", S5M8752_DA_ALC8, 0, 0x1F, 1),
SOC_SINGLE("DA ALC Noise Thr.", S5M8752_DA_ALC9, 3, 0x1F, 1),
SOC_SINGLE("Line Volume", S5M8752_LINE_VLM, 0, 0x1F, 1),
SOC_DOUBLE_R("HP Preamp Volume", S5M8752_HPL_VLM, S5M8752_HPR_VLM, 0, 0x1F, 1),
SOC_SINGLE("SPK PWM Gain", S5M8752_SPK_VLM, 5, 0x2, 1),
SOC_SINGLE("SPK Preamp Volume", S5M8752_SPK_VLM, 0, 0x1F, 1),
SOC_SINGLE("Line AMP Voltage", S5M8752_LINE_HP_CONFIG2, 4, 0xF, 1),
SOC_SINGLE("HP DRVAMP Voltage", S5M8752_LINE_HP_CONFIG2, 0, 0xF, 1),
SOC_SINGLE("SW Size", S5M8752_SW_SIZE, 0, 0xF, 1),
SOC_SINGLE("SPK Dead Time", S5M8752_SPK_DT, 0, 0x1F, 1),
SOC_DOUBLE_TLV("HP Offset", S5M8752_HP_OFFSET, 4, 0, 0xF, 1, hp_offset),
SOC_SINGLE("SPK DC offset", S5M8752_SPK_OFFSET, 0, 0xF, 1),
};

/******************************************************************************
 * This function is S5M8752 Speaker PGA event.
 ******************************************************************************/
static int spk_pga_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;
	uint8_t reg;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		reg = s5m8752_codec_read(codec, S5M8752_SPK_VLM);
		reg &= ~S5M8752_MUTE_SPK_MASK;
		s5m8752_codec_write(codec, S5M8752_SPK_VLM, reg);
		break;

	case SND_SOC_DAPM_PRE_PMD:
		reg = s5m8752_codec_read(codec, S5M8752_SPK_VLM);
		reg |= S5M8752_MUTE_SPK_MASK;
		s5m8752_codec_write(codec, S5M8752_SPK_VLM, reg);
		break;
	}

	return 0;
}

/******************************************************************************
 * This function is S5M8752 Left Headphone PGA event.
 ******************************************************************************/
static int hpl_pga_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;
	uint8_t reg;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		reg = s5m8752_codec_read(codec, S5M8752_AMP_EN);
		reg |= S5M8752_EN_HP_PRE_L_MASK;
		s5m8752_codec_write(codec, S5M8752_AMP_EN, reg);

		reg = s5m8752_codec_read(codec, S5M8752_HPL_VLM);
		reg &= ~(S5M8752_MUTE_HPL_MASK | S5M8752_DRV_MUTE_L_MASK);
		s5m8752_codec_write(codec, S5M8752_HPL_VLM, reg);
		break;

	case SND_SOC_DAPM_PRE_PMD:
		reg = s5m8752_codec_read(codec, S5M8752_HPL_VLM);
		reg |= (S5M8752_MUTE_HPL_MASK | S5M8752_DRV_MUTE_L_MASK);
		s5m8752_codec_write(codec, S5M8752_HPL_VLM, reg);

		reg = s5m8752_codec_read(codec, S5M8752_AMP_EN);
		reg &= ~S5M8752_EN_HP_PRE_L_MASK;
		s5m8752_codec_write(codec, S5M8752_AMP_EN, reg);
		break;
	}

	return 0;
}

/******************************************************************************
 * This function is S5M8752 Right Headphone PGA event.
 ******************************************************************************/
static int hpr_pga_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;
	uint8_t reg;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		reg = s5m8752_codec_read(codec, S5M8752_AMP_EN);
		reg |= S5M8752_EN_HP_PRE_R_MASK;
		s5m8752_codec_write(codec, S5M8752_AMP_EN, reg);

		reg = s5m8752_codec_read(codec, S5M8752_HPR_VLM);
		reg &= ~(S5M8752_MUTE_HPR_MASK | S5M8752_DRV_MUTE_R_MASK);
		s5m8752_codec_write(codec, S5M8752_HPR_VLM, reg);
		break;

	case SND_SOC_DAPM_PRE_PMD:
		reg = s5m8752_codec_read(codec, S5M8752_HPR_VLM);
		reg |= (S5M8752_MUTE_HPR_MASK | S5M8752_DRV_MUTE_R_MASK);
		s5m8752_codec_write(codec, S5M8752_HPR_VLM, reg);

		reg = s5m8752_codec_read(codec, S5M8752_AMP_EN);
		reg &= ~S5M8752_EN_HP_PRE_R_MASK;
		s5m8752_codec_write(codec, S5M8752_AMP_EN, reg);
		break;
	}

	return 0;
}

/******************************************************************************
 * This function is S5M8752 Lineout PGA event.
 ******************************************************************************/
static int line_pga_event(struct snd_soc_dapm_widget *w,
				struct snd_kcontrol *kcontrol, int event)
{
	struct snd_soc_codec *codec = w->codec;
	uint8_t reg;

	switch (event) {
	case SND_SOC_DAPM_POST_PMU:
		reg = s5m8752_codec_read(codec, S5M8752_LINE_VLM);
		reg &= ~S5M8752_MUTE_LINE_MASK;
		s5m8752_codec_write(codec, S5M8752_LINE_VLM, reg);
		break;

	case SND_SOC_DAPM_PRE_PMD:
		reg = s5m8752_codec_read(codec, S5M8752_LINE_VLM);
		reg |= S5M8752_MUTE_LINE_MASK;
		s5m8752_codec_write(codec, S5M8752_LINE_VLM, reg);
		break;
	}

	return 0;
}

static const struct snd_kcontrol_new s5m8752_dapm_lchmix_controls[] = {
SOC_DAPM_SINGLE("DLine to ADMIXL", S5M8752_AD_MUX,
					S5M8752_SEL_DLINE_ADMIXL_SHIFT, 1, 0),
SOC_DAPM_SINGLE("LLine to ADMIXL", S5M8752_AD_MUX,
					S5M8752_SEL_LLINE_ADMIXL_SHIFT, 1, 0),
};

static const struct snd_kcontrol_new s5m8752_dapm_rchmix_controls[] = {
SOC_DAPM_SINGLE("MIC to ADMIXR", S5M8752_AD_MUX,
					S5M8752_SEL_DLINE_ADMIXR_SHIFT, 1, 0),
SOC_DAPM_SINGLE("RLine to ADMIXR", S5M8752_AD_MUX,
					S5M8752_SEL_RLINE_ADMIXR_SHIFT, 1, 0),
};

static const struct snd_kcontrol_new s5m8752_dapm_digmix_controls[] = {
SOC_DAPM_SINGLE("DIG MIXER CH1", S5M8752_DA_DMIX1,
					S5M8752_MIX_EN1_MASK, 1, 0),
SOC_DAPM_SINGLE("DIG MIXER CH2", S5M8752_DA_DMIX1,
					S5M8752_MIX_EN2_MASK, 1, 0),
SOC_DAPM_SINGLE("DIG MIXER CH3", S5M8752_DA_DMIX2,
					S5M8752_MIX_EN3_MASK, 1, 0),
};

static const struct snd_kcontrol_new s5m8752_dapm_doutsel1_controls[] = {
SOC_DAPM_DOUBLE("DAC to DOUTSEL1", S5M8752_DOUTMX1, 1, 0, 2, 0, 0),
SOC_DAPM_DOUBLE("ADC to DOUTSEL1", S5M8752_DOUTMX1, 1, 0, 2, 0, 1),
SOC_DAPM_DOUBLE("CH2 to DOUTSEL1", S5M8752_DOUTMX1, 1, 0, 2, 0, 2),
};

static const struct snd_kcontrol_new s5m8752_dapm_doutsel2_controls[] = {
SOC_DAPM_DOUBLE("DAC to DOUTSEL2", S5M8752_DOUTMX1, 3, 2, 2, 0, 0),
SOC_DAPM_DOUBLE("ADC to DOUTSEL2", S5M8752_DOUTMX1, 3, 2, 2, 0, 1),
SOC_DAPM_DOUBLE("CH1 to DOUTSEL2", S5M8752_DOUTMX1, 3, 2, 2, 0, 2),
};

static const struct snd_kcontrol_new s5m8752_dapm_loutmix_controls[] = {
SOC_DAPM_SINGLE("LDAC to LOUT", S5M8752_DA_AMIX0,
			S5M8752_LDAC_LOUTMIX_SHIFT, 1, 0),
SOC_DAPM_SINGLE("LLine to LOUT", S5M8752_DA_AMIX0,
			S5M8752_LLINE_LOUTMIX_SHIFT, 1, 0),
SOC_DAPM_SINGLE("DLine to LOUT", S5M8752_DA_AMIX0,
			S5M8752_DLINE_LOUTMIX_SHIFT, 1, 0),
SOC_DAPM_SINGLE("RLine to LOUT", S5M8752_DA_AMIX0,
			S5M8752_RLINE_LOUTMIX_SHIFT, 1, 0),
SOC_DAPM_SINGLE("MIC to LOUT", S5M8752_DA_AMIX0,
			S5M8752_MIC_LOUTMIX_SHIFT, 1, 0),
SOC_DAPM_SINGLE("RDAC to LOUT", S5M8752_DA_AMIX0,
			S5M8752_RDAC_LOUTMIX_SHIFT, 1, 0),
};

static const struct snd_kcontrol_new s5m8752_dapm_routmix_controls[] = {
SOC_DAPM_SINGLE("LDAC to ROUT", S5M8752_DA_AMIX1,
			S5M8752_LDAC_ROUTMIX_SHIFT, 1, 0),
SOC_DAPM_SINGLE("LLine to ROUT", S5M8752_DA_AMIX1,
			S5M8752_LLINE_ROUTMIX_SHIFT, 1, 0),
SOC_DAPM_SINGLE("DLine to ROUT", S5M8752_DA_AMIX1,
			S5M8752_DLINE_ROUTMIX_SHIFT, 1, 0),
SOC_DAPM_SINGLE("RLine to ROUT", S5M8752_DA_AMIX1,
			S5M8752_RLINE_ROUTMIX_SHIFT, 1, 0),
SOC_DAPM_SINGLE("MIC to ROUT", S5M8752_DA_AMIX1,
			S5M8752_MIC_ROUTMIX_SHIFT, 1, 0),
SOC_DAPM_SINGLE("RDAC to ROUT", S5M8752_DA_AMIX1,
			S5M8752_RDAC_ROUTMIX_SHIFT, 1, 0),
};

static const struct snd_kcontrol_new s5m8752_dapm_lhpmix_controls[] = {
SOC_DAPM_SINGLE("LDAC to LHP", S5M8752_DA_AMIX2,
			S5M8752_LDAC_HPLMIX_SHIFT, 1, 0),
SOC_DAPM_SINGLE("LLine to LHP", S5M8752_DA_AMIX2,
			S5M8752_LLINE_HPLMIX_SHIFT, 1, 0),
SOC_DAPM_SINGLE("DLine to LHP", S5M8752_DA_AMIX2,
			S5M8752_DLINE_HPLMIX_SHIFT, 1, 0),
SOC_DAPM_SINGLE("RLine to LHP", S5M8752_DA_AMIX2,
			S5M8752_RLINE_HPLMIX_SHIFT, 1, 0),
SOC_DAPM_SINGLE("MIC to LHP", S5M8752_DA_AMIX2,
			S5M8752_MIC_HPLMIX_SHIFT, 1, 0),
SOC_DAPM_SINGLE("RDAC to LHP", S5M8752_DA_AMIX2,
			S5M8752_RDAC_HPLMIX_SHIFT, 1, 0),
};

static const struct snd_kcontrol_new s5m8752_dapm_rhpmix_controls[] = {
SOC_DAPM_SINGLE("LDAC to RHP", S5M8752_DA_AMIX3,
			S5M8752_LDAC_HPRMIX_SHIFT, 1, 0),
SOC_DAPM_SINGLE("LLine to RHP", S5M8752_DA_AMIX3,
			S5M8752_LLINE_HPRMIX_SHIFT, 1, 0),
SOC_DAPM_SINGLE("DLine to RHP", S5M8752_DA_AMIX3,
			S5M8752_DLINE_HPRMIX_SHIFT, 1, 0),
SOC_DAPM_SINGLE("RLine to RHP", S5M8752_DA_AMIX3,
			S5M8752_RLINE_HPRMIX_SHIFT, 1, 0),
SOC_DAPM_SINGLE("MIC to RHP", S5M8752_DA_AMIX3,
			S5M8752_MIC_HPRMIX_SHIFT, 1, 0),
SOC_DAPM_SINGLE("RDAC to RHP", S5M8752_DA_AMIX3,
			S5M8752_RDAC_HPRMIX_SHIFT, 1, 0),
};

static const struct snd_kcontrol_new s5m8752_dapm_spkmix_controls[] = {
SOC_DAPM_SINGLE("LDAC to SPK", S5M8752_DA_AMIX4,
			S5M8752_LDAC_SPKMIX_SHIFT, 1, 0),
SOC_DAPM_SINGLE("LLine to SPK", S5M8752_DA_AMIX4,
			S5M8752_LLINE_SPKMIX_SHIFT, 1, 0),
SOC_DAPM_SINGLE("DLine to SPK", S5M8752_DA_AMIX4,
			S5M8752_DLINE_SPKMIX_SHIFT, 1, 0),
SOC_DAPM_SINGLE("RLine to SPK", S5M8752_DA_AMIX4,
			S5M8752_RLINE_SPKMIX_SHIFT, 1, 0),
SOC_DAPM_SINGLE("MIC to SPK", S5M8752_DA_AMIX4,
			S5M8752_MIC_SPKMIX_SHIFT, 1, 0),
SOC_DAPM_SINGLE("RDAC to SPK", S5M8752_DA_AMIX4,
			S5M8752_RDAC_SPKMIX_SHIFT, 1, 0),
};

static const struct snd_soc_dapm_widget s5m8752_dapm_widgets[] = {
/* Input Lines */
SND_SOC_DAPM_INPUT("MICN"),
SND_SOC_DAPM_INPUT("MICP"),
SND_SOC_DAPM_INPUT("LIN"),
SND_SOC_DAPM_INPUT("RIN"),
SND_SOC_DAPM_INPUT("DIGMIC"),
SND_SOC_DAPM_INPUT("DIFFIN"),
SND_SOC_DAPM_INPUT("Internal ADC Source"),

/* Input path */
SND_SOC_DAPM_PGA("LIN DCTL Enable", S5M8752_AD_PDB2,
			S5M8752_PDB_DCTL_SHIFT, 0, NULL, 0),
SND_SOC_DAPM_PGA("LIN DSML Enable", S5M8752_AD_PDB2,
			S5M8752_PDB_DSML_SHIFT, 0, NULL, 0),
SND_SOC_DAPM_PGA("RIN DCTR Enable", S5M8752_AD_PDB2,
			S5M8752_PDB_DCTR_SHIFT, 0, NULL, 0),
SND_SOC_DAPM_PGA("RIN DSMR Enable", S5M8752_AD_PDB2,
			S5M8752_PDB_DSMR_SHIFT, 0, NULL, 0),
SND_SOC_DAPM_PGA("MIC Enable", S5M8752_AD_PDB2,
			S5M8752_PDB_MICR_SHIFT, 0, NULL, 0),
SND_SOC_DAPM_PGA("DIFFIN Enable", S5M8752_AD_PDB2,
			S5M8752_PDB_DIFFL_SHIFT, 0, NULL, 0),

/* MIC bias set */
SND_SOC_DAPM_MICBIAS("MIC Bias On", S5M8752_AD_PDB3,
			S5M8752_PDB_MBIAS_SHIFT, 0),

SND_SOC_DAPM_ADC("ADC Enable", "Capture", S5M8752_DA_DEN, 5, 0),

/* Input Analog Mixer set */
SND_SOC_DAPM_MIXER("ADMIXL", S5M8752_AD_PDB2, S5M8752_PDB_ADMIXL_SHIFT, 0,
			&s5m8752_dapm_lchmix_controls[0],
			ARRAY_SIZE(s5m8752_dapm_lchmix_controls)),
SND_SOC_DAPM_MIXER("ADMIXR", S5M8752_AD_PDB2, S5M8752_PDB_ADMIXR_SHIFT, 0,
			&s5m8752_dapm_rchmix_controls[0],
			ARRAY_SIZE(s5m8752_dapm_rchmix_controls)),

/* DOUT Mixer set */
SND_SOC_DAPM_MIXER("DOUT Enable", S5M8752_DA_DEN, S5M8752_DOUT_EN_SHIFT, 0,
			&s5m8752_dapm_doutsel1_controls[0],
			ARRAY_SIZE(s5m8752_dapm_doutsel1_controls)),
SND_SOC_DAPM_MIXER("DOUT Enable", S5M8752_DA_DEN, S5M8752_DOUT_EN_SHIFT, 0,
			&s5m8752_dapm_doutsel2_controls[0],
			ARRAY_SIZE(s5m8752_dapm_doutsel2_controls)),

/* Left/Right DAC set */
SND_SOC_DAPM_DAC("DAC Enable", "Playback", S5M8752_DA_DEN, 4, 0),

SND_SOC_DAPM_DAC("LDAC Enable", NULL, S5M8752_DA_PDB,
						S5M8752_PDB_DACL_SHIFT, 0),
SND_SOC_DAPM_DAC("RDAC Enable", NULL, S5M8752_DA_PDB,
						S5M8752_PDB_DACR_SHIFT, 0),

/* Mixer */
SND_SOC_DAPM_MIXER("SPK Mixer", S5M8752_DA_PDB, S5M8752_PDB_SPKM_SHIFT, 0,
			&s5m8752_dapm_spkmix_controls[0],
			ARRAY_SIZE(s5m8752_dapm_spkmix_controls)),
SND_SOC_DAPM_MIXER("LHP Mixer", S5M8752_DA_PDB, S5M8752_PDB_HPML_SHIFT, 0,
			&s5m8752_dapm_lhpmix_controls[0],
			ARRAY_SIZE(s5m8752_dapm_lhpmix_controls)),
SND_SOC_DAPM_MIXER("RHP Mixer", S5M8752_DA_PDB, S5M8752_PDB_HPMR_SHIFT, 0,
			&s5m8752_dapm_rhpmix_controls[0],
			ARRAY_SIZE(s5m8752_dapm_rhpmix_controls)),
SND_SOC_DAPM_MIXER("LOUT Mixer", S5M8752_DA_PDB, S5M8752_PDB_LDML_SHIFT, 0,
			&s5m8752_dapm_loutmix_controls[0],
			ARRAY_SIZE(s5m8752_dapm_loutmix_controls)),
SND_SOC_DAPM_MIXER("ROUT Mixer", S5M8752_DA_PDB, S5M8752_PDB_LDMR_SHIFT, 0,
			&s5m8752_dapm_routmix_controls[0],
			ARRAY_SIZE(s5m8752_dapm_routmix_controls)),

/* AMP Enable */
SND_SOC_DAPM_PGA_E("SPK Enable", S5M8752_AMP_EN,
		S5M8752_SPKAMP_EN_SHIFT, 0, NULL, 0,
		spk_pga_event, SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD),
SND_SOC_DAPM_PGA_E("LHP Enable", S5M8752_AMP_EN,
		S5M8752_EN_HP_DRV_L_SHIFT, 0, NULL, 0,
		hpl_pga_event, SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD),
SND_SOC_DAPM_PGA_E("RHP Enable", S5M8752_AMP_EN,
		S5M8752_EN_HP_DRV_R_SHIFT, 0, NULL, 0,
		hpr_pga_event, SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD),
SND_SOC_DAPM_PGA_E("LINEOUT Enable", S5M8752_AMP_EN,
		S5M8752_EN_LOUT_SHIFT, 0, NULL, 0,
		line_pga_event, SND_SOC_DAPM_POST_PMU | SND_SOC_DAPM_PRE_PMD),

/* Output Lines */
SND_SOC_DAPM_OUTPUT("Internal DAC Sink"),
SND_SOC_DAPM_OUTPUT("SPK"),
SND_SOC_DAPM_OUTPUT("LHP"),
SND_SOC_DAPM_OUTPUT("RHP"),
SND_SOC_DAPM_OUTPUT("LOUT"),
SND_SOC_DAPM_OUTPUT("ROUT"),
};

static const struct snd_soc_dapm_route audio_map[] = {
	{"ADC Enable", NULL, "Internal ADC Source"},

	{"Internal DAC Sink", NULL, "DAC Enable"},

	/* Input Enable setting */
	{"LIN DCTL Enable", NULL, "LIN"},
	{"RIN DCTR Enable", NULL, "RIN"},
	{"MIC Enable", NULL, "MICN"},
	{"MIC Enable", NULL, "MICP"},
	{"DIFFIN Enable", NULL, "DIFFIN"},

	{"LIN DCTL Enable", NULL, "DIFFIN Enable"},

	{"LIN DSML Enable", NULL, "LIN DCTL Enable"},
	{"RIN DSMR Enable", NULL, "RIN DCTR Enable"},

	/* Mixer setting */
	{"ADMIXL", "DLine to ADMIXL", "DIFFIN Enable"},
	{"ADMIXL", "LLine to ADMIXL", "LIN DSML Enable"},
	{"ADMIXR", "MIC to ADMIXR", "MIC Enable"},
	{"ADMIXR", "RLine to ADMIXR", "RIN DSMR Enable"},

	/* ADC Enable setting */
	{"ADC Enable", NULL, "ADMIXL"},
	{"ADC Enable", NULL, "ADMIXR"},

	/* DOUT Mixer setting */
	{"ADC Enable", "ADC to DOUTSEL1", "DOUT Enable"},
	{"ADC Enable", "ADC to DOUTSEL2", "DOUT Enable"},

	/* DAC Enable setting */
	{"LDAC Enable", NULL, "DAC Enable"},
	{"RDAC Enable", NULL, "DAC Enable"},

	/* Speaker Mixer setting */
	{"SPK Mixer", "LDAC to SPK", "LDAC Enable"},
	{"SPK Mixer", "LLine to SPK", "LIN DSML Enable"},
	{"SPK Mixer", "DLine to SPK", "DIFFIN Enable"},
	{"SPK Mixer", "RLine to SPK", "RIN DSMR Enable"},
	{"SPK Mixer", "MIC to SPK", "MIC Enable"},
	{"SPK Mixer", "RDAC to SPK", "RDAC Enable"},

	/* Left Headphone Mixer setting */
	{"LHP Mixer", "LDAC to LHP", "LDAC Enable"},
	{"LHP Mixer", "LLine to LHP", "LIN DSML Enable"},
	{"LHP Mixer", "DLine to LHP", "DIFFIN Enable"},
	{"LHP Mixer", "RLine to LHP", "RIN DSMR Enable"},
	{"LHP Mixer", "MIC to LHP", "MIC Enable"},
	{"LHP Mixer", "RDAC to LHP", "RDAC Enable"},

	/* Right Headphone Mixer setting */
	{"RHP Mixer", "LDAC to RHP", "LDAC Enable"},
	{"RHP Mixer", "LLine to RHP", "LIN DSML Enable"},
	{"RHP Mixer", "DLine to RHP", "DIFFIN Enable"},
	{"RHP Mixer", "RLine to RHP", "RIN DSMR Enable"},
	{"RHP Mixer", "MIC to RHP", "MIC Enable"},
	{"RHP Mixer", "RDAC to RHP", "RDAC Enable"},

	/* Left Lineout Mixer setting */
	{"LOUT Mixer", "LDAC to LOUT", "LDAC Enable"},
	{"LOUT Mixer", "LLine to LOUT", "LIN DSML Enable"},
	{"LOUT Mixer", "DLine to LOUT", "DIFFIN Enable"},
	{"LOUT Mixer", "RLine to LOUT", "RIN DSMR Enable"},
	{"LOUT Mixer", "MIC to LOUT", "MIC Enable"},
	{"LOUT Mixer", "RDAC to LOUT", "RDAC Enable"},

	/* Right Lineout Mixer setting */
	{"ROUT Mixer", "LDAC to ROUT", "LDAC Enable"},
	{"ROUT Mixer", "LLine to ROUT", "LIN DSML Enable"},
	{"ROUT Mixer", "DLine to ROUT", "DIFFIN Enable"},
	{"ROUT Mixer", "RLine to ROUT", "RIN DSMR Enable"},
	{"ROUT Mixer", "MIC to ROUT", "MIC Enable"},
	{"ROUT Mixer", "RDAC to ROUT", "RDAC Enable"},

	/* Amp path setting */
	{"SPK Enable", NULL, "SPK Mixer"},
	{"LHP Enable", NULL, "LHP Mixer"},
	{"RHP Enable", NULL, "RHP Mixer"},
	{"LINEOUT Enable", NULL, "LOUT Mixer"},
	{"LINEOUT Enable", NULL, "ROUT Mixer"},

	{"SPK", NULL, "SPK Enable"},
	{"LHP", NULL, "LHP Enable"},
	{"RHP", NULL, "RHP Enable"},
	{"LOUT", NULL, "LINEOUT Enable"},
	{"ROUT", NULL, "LINEOUT Enable"},
};

static int s5m8752_add_widgets(struct snd_soc_codec *codec)
{
	snd_soc_dapm_new_controls(codec, s5m8752_dapm_widgets,
				ARRAY_SIZE(s5m8752_dapm_widgets));
	snd_soc_dapm_add_routes(codec, audio_map, ARRAY_SIZE(audio_map));
	snd_soc_dapm_new_widgets(codec);

	return 0;
}

/******************************************************************************
 * This function sets that S5M8752 I2S length and sampling rate.
 ******************************************************************************/
static int s5m8752_hw_params(struct snd_pcm_substream *substream,
				struct snd_pcm_hw_params *params,
				struct snd_soc_dai *dai)
{
	struct snd_soc_pcm_runtime *rtd = substream->private_data;
	struct snd_soc_device *socdev = rtd->socdev;
	struct snd_soc_codec *codec = socdev->card->codec;
	int count, ret = 0;
	uint8_t val;

	/* I2S length set */
	val = s5m8752_codec_read(codec, S5M8752_IN1_CTRL2);
	val &= ~S5M8752_I2S_DL1_MASK;

	switch (params_format(params)) {
	case SNDRV_PCM_FORMAT_S16_LE:
		val |= S5M8752_I2S_LENGTH_16;
		break;
	case SNDRV_PCM_FORMAT_S18_3LE:
		val |= S5M8752_I2S_LENGTH_18;
		break;
	case SNDRV_PCM_FORMAT_S20_3LE:
		val |= S5M8752_I2S_LENGTH_20;
		break;
	case SNDRV_PCM_FORMAT_S24_LE:
	case SNDRV_PCM_FORMAT_S32_LE:
		val |= S5M8752_I2S_LENGTH_24;
		break;
	default:
		return -EINVAL;
	}
	s5m8752_codec_write(codec, S5M8752_IN1_CTRL2, val);

	/* Sampling rate set */
	val = s5m8752_codec_read(codec, S5M8752_SYS_CTRL2);
	val &= ~S5M8752_SAMP_RATE_MASK;

	for (count = 0; count < ARRAY_SIZE(s5m8752_rate); count++) {
		if (params_rate(params) == s5m8752_rate[count].rate) {
			val |= s5m8752_rate[count].reg_rate;
			ret = 0;
			break;
		} else
			ret = -EINVAL;
	}
	s5m8752_codec_write(codec, S5M8752_SYS_CTRL2, val);
	return ret;
}

/******************************************************************************
 * This function sets that S5M8752 master/slave mode and several formats
 ******************************************************************************/
static int s5m8752_set_dai_fmt(struct snd_soc_dai *codec_dai,
					unsigned int fmt)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	uint8_t sys_ctrl1, sys_ctrl3, sys_ctrl4;
	uint8_t in1_ctrl1, in1_ctrl2;

	sys_ctrl1 = s5m8752_codec_read(codec, S5M8752_SYS_CTRL1);
	sys_ctrl3 = s5m8752_codec_read(codec, S5M8752_SYS_CTRL3);
	sys_ctrl4 = s5m8752_codec_read(codec, S5M8752_SYS_CTRL4);

	in1_ctrl1 = s5m8752_codec_read(codec, S5M8752_IN1_CTRL1);
	in1_ctrl2 = s5m8752_codec_read(codec, S5M8752_IN1_CTRL2);

	/* Master/Slave mode set */
	switch (fmt & SND_SOC_DAIFMT_MASTER_MASK) {
	case SND_SOC_DAIFMT_CBS_CFS:
		sys_ctrl1 &= ~(S5M8752_MASTER_MASK | S5M8752_MASTER_SW1_MASK |
						S5M8752_MASTER_SW2_MASK);
		break;
	case SND_SOC_DAIFMT_CBM_CFM:
		sys_ctrl1 |= (S5M8752_MASTER_MASK | S5M8752_MASTER_SW1_MASK |
						S5M8752_MASTER_SW2_MASK);
		break;
	default:
		return -EINVAL;
	}

	/* Audio codec format set */
	in1_ctrl2 &= ~S5M8752_I2S_DF1_MASK;

	switch (fmt & SND_SOC_DAIFMT_FORMAT_MASK) {
	case SND_SOC_DAIFMT_I2S:
		in1_ctrl1 &= ~S5M8752_I2S_PCM1_MASK;
		in1_ctrl2 |= S5M8752_I2S_STANDARD;
		sys_ctrl3 &= ~S5M8752_I2S_PCM_MASK;
		break;
	case SND_SOC_DAIFMT_LEFT_J:
		in1_ctrl1 &= ~S5M8752_I2S_PCM1_MASK;
		in1_ctrl2 |= S5M8752_I2S_LJ;
		sys_ctrl3 &= ~S5M8752_I2S_PCM_MASK;
		break;
	case SND_SOC_DAIFMT_RIGHT_J:
		in1_ctrl1 &= ~S5M8752_I2S_PCM1_MASK;
		in1_ctrl2 |= S5M8752_I2S_RJ;
		sys_ctrl3 &= ~S5M8752_I2S_PCM_MASK;
		break;
	case SND_SOC_DAIFMT_DSP_B:
		in1_ctrl1 |= S5M8752_I2S_PCM1_MASK;
		break;
	default:
		return -EINVAL;
	}

	/* SCK, LRCK polarity set */
	switch (fmt & SND_SOC_DAIFMT_INV_MASK) {
	case SND_SOC_DAIFMT_NB_NF:
		in1_ctrl1 &= ~S5M8752_SCK_POL1_MASK;
		in1_ctrl2 &= ~S5M8752_LRCK_POL1_MASK;
		break;
	case SND_SOC_DAIFMT_IB_IF:
		in1_ctrl1 |= S5M8752_SCK_POL1_MASK;
		in1_ctrl2 |= S5M8752_LRCK_POL1_MASK;
		break;
	case SND_SOC_DAIFMT_IB_NF:
		in1_ctrl1 |= S5M8752_SCK_POL1_MASK;
		in1_ctrl2 &= ~S5M8752_LRCK_POL1_MASK;
		break;
	case SND_SOC_DAIFMT_NB_IF:
		in1_ctrl1 &= ~S5M8752_SCK_POL1_MASK;
		in1_ctrl2 |= S5M8752_LRCK_POL1_MASK;
		break;
	default:
		printk(KERN_ERR "Inv-combo(%d) not supported!\n", fmt &
					SND_SOC_DAIFMT_FORMAT_MASK);
		return -EINVAL;
	}

	s5m8752_codec_write(codec, S5M8752_SYS_CTRL1, sys_ctrl1);
	s5m8752_codec_write(codec, S5M8752_SYS_CTRL3, sys_ctrl3);
	s5m8752_codec_write(codec, S5M8752_SYS_CTRL4, sys_ctrl4);

	s5m8752_codec_write(codec, S5M8752_IN1_CTRL1, in1_ctrl1);
	s5m8752_codec_write(codec, S5M8752_IN1_CTRL2, in1_ctrl2);

	return 0;
}

/******************************************************************************
 * This function sets that S5M8752 master clock rate and BCLK ratio.
 ******************************************************************************/
static int s5m8752_set_dai_clkdiv(struct snd_soc_dai *codec_dai,
				 int div_id, int div)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	unsigned char val;

	switch (div_id) {
	case S5M8752_MCLKRATIO:
		val = s5m8752_codec_read(codec, S5M8752_SYS_CTRL4);
		val &= ~S5M8752_MCK_SEL_MASK;

		switch (div) {
		case 256:
			val |= S5M8752_MCLK_RATE_256;
			break;
		case 384:
			val |= S5M8752_MCLK_RATE_384;
			break;
		case 512:
			val |= S5M8752_MCLK_RATE_512;
			break;
		default:
			return -EINVAL;
		}
		s5m8752_codec_write(codec, S5M8752_SYS_CTRL4, val);
		break;

	case S5M8752_BCLKRATIO:
		val = s5m8752_codec_read(codec, S5M8752_IN1_CTRL2);
		val &= ~S5M8752_I2S_XFS1_MASK;

		switch (div) {
		case 32:
			val |= S5M8752_BCLK_RATE_32;
			break;
		case 48:
			val |= S5M8752_BCLK_RATE_48;
			break;
		case 64:
			val |= S5M8752_BCLK_RATE_64;
			break;
		default:
			return -EINVAL;
		}
		s5m8752_codec_write(codec, S5M8752_IN1_CTRL2, val);
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

/******************************************************************************
 * This function sets that S5M8752 PLL fuction in master mode.
 ******************************************************************************/
static int s5m8752_set_dai_pll(struct snd_soc_dai *codec_dai, int pll_id,
			int source, unsigned int freq_in, unsigned int freq_out)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	unsigned int reg;

	reg = s5m8752_codec_read(codec, S5M8752_SYS_CTRL2);
	reg &= ~S5M8752_PLL_SEL_MASK;

	switch (freq_in) {
	case 12000000:
		reg |= S5M8752_PLL_CLK_12MHz << S5M8752_PLL_SEL_SHIFT;
		break;
	case 13000000:
		reg |= S5M8752_PLL_CLK_13MHz << S5M8752_PLL_SEL_SHIFT;
		break;
	case 26000000:
		reg |= S5M8752_PLL_CLK_26MHz << S5M8752_PLL_SEL_SHIFT;
		break;
	case 19200000:
		reg |= S5M8752_PLL_CLK_19_2MHz << S5M8752_PLL_SEL_SHIFT;
		break;
	default:
		return -EINVAL;
	}
	s5m8752_codec_write(codec, S5M8752_SYS_CTRL2, reg);

	reg = s5m8752_codec_read(codec, S5M8752_PLL_CTRL);

	reg |= S5M8752_PLL_RESETB_MASK;
	reg &= ~S5M8752_BYPASS_MASK;

	s5m8752_codec_write(codec, S5M8752_PLL_CTRL, reg);

	return 0;
}

/******************************************************************************
 * This function controls S5M8752 DAC mute function.
 ******************************************************************************/
static int s5m8752_digital_dac_mute(struct snd_soc_dai *codec_dai, int mute)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	unsigned int reg;

	reg = s5m8752_codec_read(codec, S5M8752_DA_CTL);

	if (mute)
		reg |= S5M8752_MUTE_DAC_MASK;
	else
		reg &= ~S5M8752_MUTE_DAC_MASK;

	s5m8752_codec_write(codec, S5M8752_DA_CTL, reg);

	return 0;
}

/******************************************************************************
 * This function controls S5M8752 ADC mute function.
 ******************************************************************************/
static int s5m8752_digital_adc_mute(struct snd_soc_dai *codec_dai, int mute)
{
	struct snd_soc_codec *codec = codec_dai->codec;
	unsigned int reg;

	reg = s5m8752_codec_read(codec, S5M8752_AD_DIG);

	if (mute)
		reg |= S5M8752_MUTE_MASK;
	else
		reg &= ~S5M8752_MUTE_MASK;

	s5m8752_codec_write(codec, S5M8752_AD_DIG, reg);

	return 0;
}

/******************************************************************************
 * This function sets S5M8752 codec bias level.
 ******************************************************************************/
static int s5m8752_set_bias_level(struct snd_soc_codec *codec,
	enum snd_soc_bias_level level)
{
	int reg;

	switch (level) {
	case SND_SOC_BIAS_ON:
		reg = s5m8752_codec_read(codec, S5M8752_REF1) &
			~S5M8752_VMIDSEL_MASK;
		s5m8752_codec_write(codec, S5M8752_REF1, reg |
			(VMID_5k << S5M8752_VMIDSEL_SHIFT));

		mdelay(50);

		s5m8752_codec_write(codec, S5M8752_REF1, reg |
			(VMID_50k << S5M8752_VMIDSEL_SHIFT));
		break;

	case SND_SOC_BIAS_STANDBY:
		s5m8752_codec_write(codec, S5M8752_AD_DIG,
					0x1 << S5M8752_MUTE_SHIFT);
		break;
	case SND_SOC_BIAS_PREPARE:
		break;
	case SND_SOC_BIAS_OFF:
		s5m8752_codec_write(codec, S5M8752_LINE_VLM, 0x8C);
		s5m8752_codec_write(codec, S5M8752_HPL_VLM, 0xCC);
		s5m8752_codec_write(codec, S5M8752_HPR_VLM, 0xCC);
		s5m8752_codec_write(codec, S5M8752_SPK_VLM, 0x86);
		s5m8752_codec_write(codec, S5M8752_AMP_EN, 0x00);
		break;
	}
	codec->bias_level = level;
	return 0;
}

#define S5M8752_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE |\
			SNDRV_PCM_FMTBIT_S24_LE | SNDRV_PCM_FMTBIT_S32_LE)

static struct snd_soc_dai_ops s5m8752_dai_ops_playback = {
	.hw_params	= s5m8752_hw_params,
	.set_fmt	= s5m8752_set_dai_fmt,
	.set_clkdiv	= s5m8752_set_dai_clkdiv,
	.set_pll	= s5m8752_set_dai_pll,
	.digital_mute	= s5m8752_digital_dac_mute,
};

static struct snd_soc_dai_ops s5m8752_dai_ops_capture = {
	.hw_params	= s5m8752_hw_params,
	.set_fmt	= s5m8752_set_dai_fmt,
	.set_clkdiv	= s5m8752_set_dai_clkdiv,
	.set_pll	= s5m8752_set_dai_pll,
	.digital_mute	= s5m8752_digital_adc_mute,
};

struct snd_soc_dai s5m8752_dai[] = {
	{
		.name = "S5M8752 Audio RX",
		.id = ID_S5M8752_DAC,
		.playback = {
			.stream_name = "Playback",
			.channels_min = 1,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_96000,
			.formats = S5M8752_FORMATS,
		},
		.ops = &s5m8752_dai_ops_playback,
	},
	{
		.name = "S5M8752 Audio TX",
		.id = ID_S5M8752_ADC,
		.capture = {
			.stream_name = "Capture",
			.channels_min = 2,
			.channels_max = 2,
			.rates = SNDRV_PCM_RATE_8000_96000,
			.formats = S5M8752_FORMATS,
		},
		.ops = &s5m8752_dai_ops_capture,
	},
};
EXPORT_SYMBOL_GPL(s5m8752_dai);

static struct snd_soc_codec *s5m8752_codec;

static int s5m8752_probe(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec;
	int ret = 0;

	if (s5m8752_codec == NULL) {
		dev_err(&pdev->dev, "Codec device isn't registered\n");
		return -ENOMEM;
	}

	socdev->card->codec = s5m8752_codec;
	codec = s5m8752_codec;

	/* register pcm */
	ret = snd_soc_new_pcms(socdev, SNDRV_DEFAULT_IDX1, SNDRV_DEFAULT_STR1);
	if (ret < 0) {
		dev_err(codec->dev, "Failed to create pcm: %d\n", ret);
		goto pcm_err;
	}

	snd_soc_add_controls(codec, s5m8752_snd_controls,
				ARRAY_SIZE(s5m8752_snd_controls));
	s5m8752_add_widgets(codec);

        ret = snd_soc_init_card(socdev);
        if (ret < 0) {
                dev_err(codec->dev, "failed to register card: %d\n", ret);
                goto card_err;
        }
	return ret;

card_err:
        snd_soc_free_pcms(socdev);
        snd_soc_dapm_free(socdev);
pcm_err:
	return ret;
}

static int s5m8752_remove(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);

	snd_soc_free_pcms(socdev);
	snd_soc_dapm_free(socdev);

	return 0;
}

struct snd_soc_codec_device soc_codec_dev_s5m8752 = {
	.probe =	s5m8752_probe,
	.remove =	s5m8752_remove,
};
EXPORT_SYMBOL_GPL(soc_codec_dev_s5m8752);

/******************************************************************************
 * This function initials the several register settings of S5M8752 codecs.
 * Initial setting
 *	ADC, DAC Power On
 *	Left/Right Line In path ON
 *	Speaker path and Amp On
 *	ADC, DAC and Amp volume are set as the codec platform data
 ******************************************************************************/
static int s5m8752_codec_initial(struct snd_soc_codec *codec,
					struct s5m8752_audio_pdata *pdata)
{
	uint8_t reg_val;

	/* Normal operation of Analog Modulator, Averaging control On */
	s5m8752_codec_write(codec, S5M8752_AD_PDB1,
				0x1 << S5M8752_RSTB_ADC_SHIFT |
				0x1 << S5M8752_EN_DWA_SHIFT);

	/* Left/Right channel Line In power On */
	s5m8752_codec_write(codec, S5M8752_AD_PDB2,
				0x1 << S5M8752_PDB_ADMIXL_SHIFT |
				0x1 << S5M8752_PDB_DCTL_SHIFT |
				0x1 << S5M8752_PDB_DSML_SHIFT |
				0x1 << S5M8752_PDB_ADMIXR_SHIFT |
				0x1 << S5M8752_PDB_DCTR_SHIFT |
				0x1 << S5M8752_PDB_DSMR_SHIFT);

	/* ADC reference Power On */
	s5m8752_codec_write(codec, S5M8752_AD_PDB3,
				0x1 << S5M8752_PDB_ADREF_SHIFT);

	/* Left/Right ADC Mixer Power On */
	s5m8752_codec_write(codec, S5M8752_AD_MUX,
				0x1 << S5M8752_SEL_LLINE_ADMIXL_SHIFT |
				0x1 << S5M8752_SEL_RLINE_ADMIXR_SHIFT);

	/* Left/Right ADC Mixer Gain Control */
	s5m8752_codec_write(codec, S5M8752_AD_LINEVOL,
				pdata->linein_vol << S5M8752_VOL_RIN_SHIFT |
				pdata->linein_vol << S5M8752_VOL_LIN_SHIFT);

	/* ADC Hard-Mute set */
	s5m8752_codec_write(codec, S5M8752_AD_DIG,
				0x1 << S5M8752_MUTE_SHIFT);

	/* Left/Right ADC Channel DSM input set */
	s5m8752_codec_write(codec, S5M8752_AD_DMUX,
				LDSM_to_LADC << S5M8752_DISL_SHIFT |
				RDSM_to_RADC << S5M8752_DISR_SHIFT);
	/* Left/Right ADC volume set */
	s5m8752_codec_write(codec, S5M8752_AD_VOLL,
				pdata->adc_vol << S5M8752_VOLL_SHIFT);
	s5m8752_codec_write(codec, S5M8752_AD_VOLR,
				pdata->adc_vol << S5M8752_VOLR_SHIFT);

	/* ALC function Enable set */
	s5m8752_codec_write(codec, S5M8752_AD_ALC1,
				0x1 << S5M8752_ALC_EN_SHIFT);

	/* ADC Enable, DAC Enable, Mixer Enable, SRC1 Enable, DOUT Enable */
	s5m8752_codec_write(codec, S5M8752_DA_DEN,
				0x1 << S5M8752_ADC_EN_SHIFT |
				0x1 << S5M8752_DAC_EN_SHIFT |
				0x1 << S5M8752_MIX_EN_SHIFT |
				0x1 << S5M8752_SRC1_EN_SHIFT |
				0x1 << S5M8752_DOUT_EN_SHIFT);

	/* DAC Analog Reset Power On, reference Power On, MID Power On */
	s5m8752_codec_write(codec, S5M8752_REF3,
				0x1 << S5M8752_RSTB_DAC_SHIFT |
				0x1 << S5M8752_PDB_DAREF_SHIFT |
				0x1 << S5M8752_PDB_MID_SHIFT);

	/* Speaker Mixer Power On, DAC Left/Right Channel Power On */
	s5m8752_codec_write(codec, S5M8752_DA_PDB,
				0x1 << S5M8752_PDB_SPKM_SHIFT |
				0x1 << S5M8752_PDB_DACL_SHIFT |
				0x1 << S5M8752_PDB_DACR_SHIFT);

	/* Right DAC to speaker driver Mixer On */
	s5m8752_codec_write(codec, S5M8752_DA_AMIX2,
				0x1 << S5M8752_LDAC_HPLMIX_SHIFT);
	s5m8752_codec_write(codec, S5M8752_DA_AMIX3,
				0x1 << S5M8752_RDAC_HPRMIX_SHIFT);
	/* Left/Right DAC Volume Control */
	s5m8752_codec_write(codec, S5M8752_DA_DVO1,
				pdata->dac_vol << S5M8752_DAC_VOLL_SHIFT);
	s5m8752_codec_write(codec, S5M8752_DA_DVO2,
				pdata->dac_vol << S5M8752_DAC_VOLR_SHIFT);

	/* DAC Mute On */
	s5m8752_codec_write(codec, S5M8752_DA_CTL,
				0x1 << S5M8752_MUTE_DAC_SHIFT);
	/* Ch1 Digital Output : ADC Data */
	s5m8752_codec_write(codec, S5M8752_DOUTMX1,
				ADC_to_DOUTSEL1 << S5M8752_DOUT_SEL1_SHIFT);
	/* PCM Total Used Slot Number set */
	s5m8752_codec_write(codec, S5M8752_TSLOT,
				0xF << S5M8752_TSLOT_SHIFT);

	/* Lineout Amp Volume set */
	s5m8752_codec_write(codec, S5M8752_LINE_VLM,
				0x1 << S5M8752_MUTE_LINE_SHIFT |
				pdata->lineout_vol << S5M8752_LINE_VLM_SHIFT);

	/* Left Headphone Amp Volume set */
	s5m8752_codec_write(codec, S5M8752_HPL_VLM,
				0x1 << S5M8752_MUTE_HPL_SHIFT |
				0x1 << S5M8752_DRV_MUTE_L_SHIFT |
				pdata->hp_preamp_vol << S5M8752_HPL_VLM_SHIFT);

	/* Left Headphone Amp Volume set */
	s5m8752_codec_write(codec, S5M8752_HPR_VLM,
				0x1 << S5M8752_MUTE_HPR_SHIFT |
				0x1 << S5M8752_DRV_MUTE_R_SHIFT |
				pdata->hp_preamp_vol << S5M8752_HPR_VLM_SHIFT);

	/* Speaker Amp Volume set */
	s5m8752_codec_write(codec, S5M8752_SPK_VLM,
				0x1 << S5M8752_MUTE_SPK_SHIFT |
				pdata->spk_preamp_vol << S5M8752_SPK_VLM_SHIFT);

	/* LDO6 regulator Power On for Headphone and Lineout Amp */
	reg_val = s5m8752_codec_read(codec, S5M8752_ONOFF2);
	reg_val |= (0x1 << 6);
	s5m8752_codec_write(codec, S5M8752_ONOFF2, reg_val);

	/* NCP Enable, Oscillator Enable */
	reg_val = s5m8752_codec_read(codec, S5M8752_AMP_EN);
	reg_val |= (0x1 << S5M8752_EN_CP_SHIFT | 0x1 << S5M8752_EN_RING_SHIFT);
	s5m8752_codec_write(codec, S5M8752_AMP_EN, reg_val);

	return 0;
}

static int s5m8752_codec_probe(struct platform_device *pdev)
{
	struct s5m8752_priv *priv;
	struct s5m8752 *s5m8752 = dev_get_drvdata(pdev->dev.parent);
	struct s5m8752_pdata *s5m8752_pdata = s5m8752->dev->platform_data;
	struct s5m8752_audio_pdata *pdata = s5m8752_pdata->audio;
	struct snd_soc_codec *codec;
	int i, ret = 0;
	priv = kzalloc(sizeof(struct s5m8752_priv), GFP_KERNEL);
	if (priv == NULL)
		goto err;

	priv->s5m8752 = s5m8752;
	codec = &priv->codec;

	mutex_init(&codec->mutex);
	INIT_LIST_HEAD(&codec->dapm_widgets);
	INIT_LIST_HEAD(&codec->dapm_paths);

	snd_soc_codec_set_drvdata(codec, priv);

	codec->name = "S5M8752";
	codec->owner = THIS_MODULE;
	codec->read = s5m8752_codec_read;
	codec->write = s5m8752_codec_write;
	codec->bias_level = SND_SOC_BIAS_OFF;
	codec->set_bias_level = s5m8752_set_bias_level;
	codec->dai = s5m8752_dai;
	codec->num_dai = ARRAY_SIZE(s5m8752_dai);

	s5m8752_codec_initial(codec, pdata);
	codec->dev = &pdev->dev;

	for (i = 0; i < ARRAY_SIZE(s5m8752_dai); i++)
		s5m8752_dai[i].dev = codec->dev;

	s5m8752_set_bias_level(codec, SND_SOC_BIAS_STANDBY);
	s5m8752_codec = codec;

	ret = snd_soc_register_codec(codec);
	if (ret != 0) {
		dev_err(codec->dev, "Failed to register codec: %d\n", ret);
		goto err_codec;
	}

	ret = snd_soc_register_dais(s5m8752_dai, ARRAY_SIZE(s5m8752_dai));
	if (ret != 0) {
		dev_err(codec->dev, "Failed to register DAI: %d\n", ret);
		goto err_codec;
	}

	return 0;

err_codec:
	snd_soc_unregister_codec(codec);

err:
	kfree(priv);
	return ret;
}

static int s5m8752_codec_remove(struct platform_device *pdev)
{
	struct s5m8752_priv *priv = snd_soc_codec_get_drvdata(s5m8752_codec);

	snd_soc_unregister_dai(s5m8752_dai);
	snd_soc_unregister_codec(s5m8752_codec);

	kfree(priv);
	s5m8752_codec = NULL;

	return 0;
}

static struct platform_driver s5m8752_codec_driver = {
	.driver = {
		.name = "s5m8752-codec",
		.owner = THIS_MODULE,
	},
	.probe = s5m8752_codec_probe,
	.remove	= s5m8752_codec_remove,
};

static int __init s5m8752_codec_init(void)
{
	return platform_driver_register(&s5m8752_codec_driver);
}
module_init(s5m8752_codec_init);

static void __exit s5m8752_codec_exit(void)
{
	platform_driver_unregister(&s5m8752_codec_driver);
}
module_exit(s5m8752_codec_exit);

MODULE_DESCRIPTION("S5M8752 Alsa Audio Codec driver");
MODULE_AUTHOR("Jongbin, Won <jongbin.won@samsung.com>");
MODULE_LICENSE("GPL");
