/*
 * uda1334.c  --  ALSA Soc UDA1334 codec support
 *
 * Copyright 2010, Tom Tom International.
 * Author: Oreste Salerno
 *         oreste.salerno@tomtom.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  Revision history
 *    25th Nov 2010   Initial version.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/initval.h>
#include <sound/soc.h>

#include "uda1334.h"

#define AUDIO_NAME	"UDA1334"
#define AUDIO_VERSION	"0.1"

#define UDA1334_RATES (SNDRV_PCM_RATE_CONTINUOUS)

#define UDA1334_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S24_LE)
  
struct snd_soc_dai uda1334_dai = {
	.name = "UDA1334",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 2,
		.channels_max = 2,
		.rates = UDA1334_RATES,
		.rate_min = 16000,
		.rate_max = 100000,
		.formats = UDA1334_FORMATS},
};
EXPORT_SYMBOL_GPL(uda1334_dai);

static int uda1334_soc_probe(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec;
	int ret = 0;

	pr_info("%s Audio Codec %s\n", AUDIO_NAME, AUDIO_VERSION);

	codec = kzalloc(sizeof(struct snd_soc_codec), GFP_KERNEL);
	if (codec == NULL)
		return -ENOMEM;

	mutex_init(&codec->mutex);
	codec->name = AUDIO_NAME;
	codec->owner = THIS_MODULE;
	codec->dai = &uda1334_dai;
	codec->num_dai = 1;
	socdev->card->codec = codec;
	INIT_LIST_HEAD(&codec->dapm_widgets);
	INIT_LIST_HEAD(&codec->dapm_paths);

	/* register pcms */
	ret = snd_soc_new_pcms(socdev, SNDRV_DEFAULT_IDX1, SNDRV_DEFAULT_STR1);
	if (ret < 0) {
		printk(KERN_ERR "uda1334: failed to create pcms\n");
		goto pcm_err;
	}

	ret = snd_soc_init_card(socdev);
	if (ret < 0) {
		printk(KERN_ERR "uda1334: failed to register card\n");
		goto register_err;
	}

	printk(KERN_INFO "%s completed\n", __func__);
	return ret;

register_err:
	snd_soc_free_pcms(socdev);
pcm_err:
	kfree(socdev->card->codec);
	socdev->card->codec = NULL;
	return ret;
}

static int uda1334_soc_remove(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->card->codec;

	if (codec == NULL)
		return 0;
	snd_soc_free_pcms(socdev);
	kfree(codec);
	return 0;
}

struct snd_soc_codec_device soc_codec_dev_uda1334 = {
	.probe = 	uda1334_soc_probe,
	.remove = 	uda1334_soc_remove,
};
EXPORT_SYMBOL_GPL(soc_codec_dev_uda1334);

static int __init uda1334_init(void)
{
	return snd_soc_register_dai(&uda1334_dai);
}
module_init(uda1334_init);

static void __exit uda1334_exit(void)
{
	snd_soc_unregister_dai(&uda1334_dai);
}
module_exit(uda1334_exit);

MODULE_DESCRIPTION("ASoC UDA1334 driver");
MODULE_AUTHOR("Oreste Salerno <oreste.salerno@tomtom.com>");
MODULE_LICENSE("GPL");
