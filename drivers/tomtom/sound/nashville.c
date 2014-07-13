/* drivers/tomtom/sound/nashville.c
 *
 * Implements the ALSA controls that are required by Nashville. Platform-specific code is deferred
 * to other files in this directory.
 *
 * Copyright (C) 2009 TomTom BV <http://www.tomtom.com/>
 * Author: Guillaume Ballet <Guillaume.Ballet@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <sound/soc.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/list.h>

#include <plat/scenarii.h>
#include <plat/nashville.h>
#include <plat/fdt.h>

#define MAXTESTGAINTYPE (65535)
#define MAXTESTGAINVALUE (65535)
#define TESTGAINRESET MAXTESTGAINVALUE

static struct tt_scenario_context *tt_kcontrol_context(struct snd_kcontrol* kcontrol)
{
	return (struct tt_scenario_context *) kcontrol->private_data;
}

static struct tt_scenario_context *tt_scenario_register(struct snd_soc_codec *, struct tt_scenario_ops *);
static void tt_scenario_unregister(struct tt_scenario_context *);

static const char *tt_scenarii[] = {
	"Init",
	"Off",
	"ASR",
	"VR",
	"Noise",
	"iPod",
	"Speaker",
	"LineOut",
	"FM",
	"iPodSpeaker",
	"iPodLineOut",
	"iPodFM",
	"HFSpeaker",
	"HFLineOut",
	"HFFM",
	"None",
};

static const char *tt_scenario_modes[] = {
	"Off",
	"SPP",
	"TTS",
	"MP3",
	"iPod",
	"HF",
	"ASR",
	"VR",
	"POI",
};

static const struct soc_enum tt_scenario_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(tt_scenarii),tt_scenarii),
};

static int tt_set_volume(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct tt_scenario_context *tt_context = tt_kcontrol_context(kcontrol);

	tt_context->tt_volume = ucontrol->value.integer.value[0];

	if (tt_context->ops && tt_context->ops->set_volume)
		tt_context->ops->set_volume(tt_context);
	return 1;
}

static int tt_get_volume(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct tt_scenario_context *tt_context = tt_kcontrol_context(kcontrol);
	ucontrol->value.integer.value[0] = tt_context->tt_volume;
	return 0;
}

/* user of this function will pass in a the gain as a 
   possitive value which should be interpreted as negative */
static int tt_set_volume_dB(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct tt_scenario_context *tt_context = tt_kcontrol_context(kcontrol);

	tt_context->tt_volume_dB = ucontrol->value.integer.value[0];

	if (tt_context->ops && tt_context->ops->set_volume_db)
		tt_context->ops->set_volume_db(tt_context);

	return 1;
}

static int tt_get_volume_dB(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct tt_scenario_context *tt_context = tt_kcontrol_context(kcontrol);
	ucontrol->value.integer.value[0] = tt_context->tt_volume_dB;
	return 0;
}

static int tt_set_mic_gain(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct tt_scenario_context *tt_context = tt_kcontrol_context(kcontrol);

	tt_context->tt_mic_gain = ucontrol->value.integer.value[0];

	if (tt_context->ops && tt_context->ops->set_mic_gain)
		tt_context->ops->set_mic_gain(tt_context);

	return 1;
}

static int tt_get_mic_gain(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct tt_scenario_context *tt_context = tt_kcontrol_context(kcontrol);
	ucontrol->value.integer.value[0] = tt_context->tt_mic_gain;
	return 0;
}

#ifdef INCLUDE_TEST_KCONTROLS
static int tt_set_gain_type(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct tt_scenario_context *tt_context = tt_kcontrol_context(kcontrol);
	tt_context->tt_test_gain_type = ucontrol->value.integer.value[0];

	return 1;
}

static int tt_get_gain_type(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct tt_scenario_context *tt_context = tt_kcontrol_context(kcontrol);
	ucontrol->value.integer.value[0] = tt_context->tt_test_gain_type;
	return 0;
}

static int tt_set_gain_value(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct tt_scenario_context *tt_context = tt_kcontrol_context(kcontrol);
	tt_context->tt_test_gain_value = ucontrol->value.integer.value[0];

	if (tt_context->tt_test_gain_value == TESTGAINRESET) {
		/* reset test gain mechanism */
		tt_context->tt_test_gain_type = -1;
	} else if (tt_context->ops && tt_context->ops->set_gain) {
		tt_context->ops->set_gain(tt_context);
	}


	return 1;
}

static int tt_get_gain_value(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct tt_scenario_context *tt_context = tt_kcontrol_context(kcontrol);
	ucontrol->value.integer.value[0] = tt_context->tt_test_gain_value;
	return 0;
}
#endif

static int tt_set_scenario(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct tt_scenario_context *tt_context = tt_kcontrol_context(kcontrol);

	if (tt_context->tt_scenario == ucontrol->value.integer.value[0])
		return 0;

	tt_context->tt_scenario = ucontrol->value.integer.value[0];

	if (tt_context->ops && tt_context->ops->set_scenario)
		tt_context->ops->set_scenario(tt_context);

	return 1;
}

static int tt_get_scenario(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct tt_scenario_context *tt_context = tt_kcontrol_context(kcontrol);
	ucontrol->value.integer.value[0] = tt_context->tt_scenario;
	return 0;
}

static int tt_get_scenario_mode(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct tt_scenario_context *tt_context = tt_kcontrol_context(kcontrol);
	ucontrol->value.integer.value[0] = tt_context->tt_scenario_mode;
	return 0;
}

static int tt_set_scenario_mode(struct snd_kcontrol *kcontrol,
	struct snd_ctl_elem_value *ucontrol)
{
	struct tt_scenario_context *tt_context = tt_kcontrol_context(kcontrol);

	if (tt_context->tt_scenario_mode == ucontrol->value.integer.value[0])
		return 0;

	tt_context->tt_scenario_mode = ucontrol->value.integer.value[0];

	/* XXX there was a call to set_scenario_endpoints, so check if something can be factorized there */

	if (tt_context->ops && tt_context->ops->set_scenario_mode)
		tt_context->ops->set_scenario_mode(tt_context);

	return 1;
}

static irqreturn_t tt_hpdetect_irq(int irq, void *dev_id)
{
	struct tt_scenario_context *tt_context = (struct tt_scenario_context *) dev_id;

	if (tt_context->ops && tt_context->ops->hpdetect_callback) {
		tt_context->tt_hpdetect = tt_context->ops->hpdetect_callback(tt_context);

		if (tt_context->tt_hpdetect_elem_id)
			snd_ctl_notify(tt_context->codec->card, SNDRV_CTL_EVENT_MASK_VALUE, tt_context->tt_hpdetect_elem_id);
	}

	return IRQ_HANDLED;
}

static int tt_hpdetect_get(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	struct tt_scenario_context *tt_context = tt_kcontrol_context(kcontrol);
	ucontrol->value.integer.value[0] = tt_context->tt_hpdetect;
	return 0;
}

static int tt_hpdetect_put(struct snd_kcontrol *kcontrol,
			struct snd_ctl_elem_value *ucontrol)
{
	return 1;
}

static const struct soc_enum tt_scenario_mode_enum[] = {
	SOC_ENUM_SINGLE_EXT(ARRAY_SIZE(tt_scenario_modes),tt_scenario_modes),
};

static const struct snd_kcontrol_new tt_controls[] = {
	SOC_ENUM_EXT("TT Scenario", tt_scenario_enum[0],
		tt_get_scenario, tt_set_scenario),
	SOC_ENUM_EXT("TT Mode", tt_scenario_mode_enum[0],
		tt_get_scenario_mode, tt_set_scenario_mode),
	SOC_SINGLE_EXT("TT Playback Volume %", 0, 0, 100, 0,
		tt_get_volume, tt_set_volume),
	SOC_SINGLE_EXT("TT Playback Volume dB", 0, 0, 90, 0,
		tt_get_volume_dB, tt_set_volume_dB),
	SOC_SINGLE_EXT("TT Mic Gain", 0, 0, 64, 0,
		tt_get_mic_gain, tt_set_mic_gain),
	SOC_SINGLE_BOOL_EXT("TT Headphones Detect", 0,
		tt_hpdetect_get, tt_hpdetect_put),
	#ifdef INCLUDE_TEST_KCONTROLS
	SOC_SINGLE_EXT("TT Test Gain Type", 0, 0, MAXTESTGAINTYPE, 0,
		tt_get_gain_type, tt_set_gain_type),
	SOC_SINGLE_EXT("TT Test Gain Value", 0, 0, MAXTESTGAINVALUE, 0,
		tt_get_gain_value, tt_set_gain_value),
	#endif
};

static struct tt_scenario_context *tt_alloc_context(void)
{
	struct tt_scenario_context *tt_context;
	void *ctx;

	int kc_size = ARRAY_SIZE(tt_controls) * sizeof(struct snd_kcontrol *);
	ctx = kzalloc(sizeof(*tt_context) + kc_size, GFP_KERNEL);
	if(!ctx)
		return NULL;

	tt_context = ctx;
	tt_context->kcontrols = ctx + sizeof(*tt_context);
	return tt_context;
}

static void tt_free_context(struct tt_scenario_context *tt_context)
{
	kfree(tt_context);
}

static LIST_HEAD(tt_scenario_contexts);

static struct tt_scenario_context *tt_codec_to_context(struct snd_soc_codec *codec)
{
	struct tt_scenario_context *ctx;

	list_for_each_entry(ctx, &tt_scenario_contexts, list)
		if (ctx->codec == codec)
			return ctx;

	return NULL;
}

int tomtom_add_nashville_controls(struct snd_soc_codec *codec, struct tt_scenario_ops *scenario_ops)
{
	int idx, ret = 0;
	struct snd_kcontrol *kcontrol;
	struct tt_scenario_context *tt_context;

	tt_context = tt_scenario_register(codec, scenario_ops);
	if (IS_ERR(tt_context))
		return PTR_ERR(tt_context);

	for (idx=0; idx<ARRAY_SIZE(tt_controls); idx++) {
		kcontrol = snd_ctl_new1(&tt_controls[idx], tt_context);
		if (!kcontrol) {
			ret = -ENOMEM;
			goto err;
		}

		tt_context->kcontrols[idx] = kcontrol;
		ret = snd_ctl_add(codec->card, kcontrol);
		if (ret < 0)
			goto err;

		if (!strcmp(tt_controls[idx].name, "TT Headphones Detect"))
			tt_context->tt_hpdetect_elem_id = &kcontrol->id;
	}
	return 0;

err:
	for (; idx >= 0; idx--) {
		if (!tt_context->kcontrols[idx])
			continue;
		snd_ctl_remove(codec->card, tt_context->kcontrols[idx]);
		snd_ctl_free_one(tt_context->kcontrols[idx]);
	}

	tt_scenario_unregister(tt_context);
	return ret;
}

void tomtom_remove_nashville_controls(struct snd_soc_codec *codec)
{
	struct tt_scenario_context *tt_context = tt_codec_to_context(codec);
	struct snd_kcontrol **kc, **kc_end;

	if (WARN_ON(tt_context == NULL))
		return;

	kc = tt_context->kcontrols;
	kc_end = kc + ARRAY_SIZE(tt_controls);
	for (; kc != kc_end; kc++) {
		snd_ctl_remove(codec->card, *kc);
		snd_ctl_free_one(*kc);
	}

	tt_scenario_unregister(tt_context);
}

#define DEFAULT_SPEAKER_VOLUME (15)
static struct tt_scenario_context *tt_scenario_register(struct snd_soc_codec *codec, struct tt_scenario_ops *ops)
{
	int ret;
	struct tt_scenario_context *tt_context;

	tt_context = tt_alloc_context();
	if(!tt_context)
		return ERR_PTR(-ENOMEM);

	tt_context->tt_volume = 0;
	tt_context->tt_volume_dB = 90;
	tt_context->tt_scenario = TT_SCENARIO_NONE;
	tt_context->tt_scenario_mode = TT_MODE_INIT;
	tt_context->tt_mic_gain = 0;
	tt_context->tt_scenario_speaker_volume = DEFAULT_SPEAKER_VOLUME;
	tt_context->tt_scenario_iPod_speaker_volume = DEFAULT_SPEAKER_VOLUME;
	#ifdef INCLUDE_TEST_KCONTROLS
	tt_context->tt_test_gain_type = -1;
	tt_context->tt_test_gain_value = -1;
	#endif
	tt_context->dapm_enabled = /* TT_DAPM_ENABLED */ 0;
	tt_context->has_5V_booster = fdt_get_ulong("/options/audio", "has-booster", 2);
	if (tt_context->has_5V_booster == 2) {
		/* for the moment fall back to checking hansfree has-bluetooth as this currently implies 5V booster */
		tt_context->has_5V_booster = fdt_get_ulong("/features", "has-bluetooth", 0);
	}

	tt_context->codec = codec;
	tt_context->ops = ops;

	if (ops && ops->hpdetect_callback) {
		tt_context->tt_hpdetect = ops->hpdetect_callback(tt_context);

		if (ops->get_hpdetect_irq) {
			ret = request_irq(ops->get_hpdetect_irq(tt_context), tt_hpdetect_irq,
			                  IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "hpdetect-irq",
			                  tt_context);
			if (ret < 0)
				goto err;
		}
	}

	INIT_LIST_HEAD(&tt_context->list);
	list_add(&tt_context->list, &tt_scenario_contexts);
	return tt_context;

err:
	kfree(tt_context);
	return ERR_PTR(ret);
}

static void tt_scenario_unregister(struct tt_scenario_context *tt_context)
{
	struct tt_scenario_ops *ops = tt_context->ops;

	if (ops && ops->hpdetect_callback && ops->get_hpdetect_irq)
		free_irq(ops->get_hpdetect_irq(tt_context), tt_context);

	list_del(&tt_context->list);
	tt_free_context(tt_context);
}

EXPORT_SYMBOL(tomtom_add_nashville_controls);
EXPORT_SYMBOL(tomtom_remove_nashville_controls);

MODULE_LICENSE("GPL");
