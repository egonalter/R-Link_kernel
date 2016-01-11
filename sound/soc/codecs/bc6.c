/*
 * bc6.c  --  ALSA Soc bc6 codec support
 *
 * Copyright 2010, Tom Tom International.
 * Author: Niels Langendorff
 *         niels.langendorff@tomtom.com
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  Revision history
 *    11th Aug 2010   Initial version.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/ac97_codec.h>
#include <sound/initval.h>
#include <sound/soc.h>

#include "bc6.h"

#define AUDIO_NAME		"BC6"
#define AUDIO_VERSION	"0.1"

// PCM supports 8000 and 16000 sample rate.
#define BC6_RATES (SNDRV_PCM_RATE_8000 | SNDRV_PCM_RATE_16000)

#define BC6_FORMATS (SNDRV_PCM_FMTBIT_S16_LE)
  
struct snd_soc_dai bc6_dai = {
	.name = "BC6",
	.playback = {
		.stream_name = "Playback",
		.channels_min = 1,
		.channels_max = 1,
		.rates = BC6_RATES,
		.formats = BC6_FORMATS},
	.capture = {
		.stream_name = "Capture",
		.channels_min = 1,
		.channels_max = 1,
		.rates = BC6_RATES,
		.formats = BC6_FORMATS},
};
EXPORT_SYMBOL_GPL(bc6_dai);

static int bc6_soc_probe(struct platform_device *pdev)
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
	codec->dai = &bc6_dai;
	codec->num_dai = 1;
	socdev->card->codec = codec;
	INIT_LIST_HEAD(&codec->dapm_widgets);
	INIT_LIST_HEAD(&codec->dapm_paths);

	/* register pcms */
	ret = snd_soc_new_pcms(socdev, SNDRV_DEFAULT_IDX1, SNDRV_DEFAULT_STR1);
	if (ret < 0) {
		printk(KERN_ERR "bc6: failed to create pcms\n");
		goto pcm_err;
	}

	ret = snd_soc_init_card(socdev);
	if (ret < 0) {
		printk(KERN_ERR "bc6: failed to register card\n");
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

static int bc6_soc_remove(struct platform_device *pdev)
{
	struct snd_soc_device *socdev = platform_get_drvdata(pdev);
	struct snd_soc_codec *codec = socdev->card->codec;

	if (codec == NULL)
		return 0;
	snd_soc_free_pcms(socdev);
	kfree(codec);
	return 0;
}

struct snd_soc_codec_device soc_codec_dev_bc6 = {
	.probe = 	bc6_soc_probe,
	.remove = 	bc6_soc_remove,
};
EXPORT_SYMBOL_GPL(soc_codec_dev_bc6);

static int __init bc6_init(void)
{
	return snd_soc_register_dai(&bc6_dai);
}
module_init(bc6_init);

static void __exit bc6_exit(void)
{
	snd_soc_unregister_dai(&bc6_dai);
}
module_exit(bc6_exit);

MODULE_DESCRIPTION("ASoC BC6 driver");
MODULE_AUTHOR("Niels Langendorff <niels.langendorff@tomtom.com>");
MODULE_LICENSE("GPL");
