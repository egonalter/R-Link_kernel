/* arch/arm/plat-samsung/include/plat/audio.h
 *
 * Copyright (c) 2009 Samsung Electronics Co. Ltd
 * Author: Jaswinder Singh <jassi.brar@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/* The machine init code calls s3c*_ac97_setup_gpio with
 * one of these defines in order to select appropriate bank
 * of GPIO for AC97 pins
 */

#ifndef __PLAT_AUDIO_H__

#define S3C64XX_AC97_GPD  0
#define S3C64XX_AC97_GPE  1

#include <sound/pcm.h>

extern void s3c64xx_ac97_setup_gpio(int);

/**
 * struct s3c_audio_pdata - common platform data for audio device drivers
 * @cfg_gpio: Callback function to setup mux'ed pins in I2S/PCM/AC97 mode
 */
struct s3c_audio_pdata {
	int (*cfg_gpio)(struct platform_device *);
};

/* struct s3c24xx_iis_ops
 *  *
 *   * called from the s3c24xx audio core to deal with the architecture
 *    * or the codec's setup and control.
 *     *
 *      * the pointer to itself is passed through in case the caller wants to
 *       * embed this in an larger structure for easy reference to it's context.
 *       */

struct s3c24xx_iis_ops {
        struct module *owner;

        int     (*startup)(struct s3c24xx_iis_ops *me);
        void    (*shutdown)(struct s3c24xx_iis_ops *me);
        int     (*suspend)(struct s3c24xx_iis_ops *me);
        int     (*resume)(struct s3c24xx_iis_ops *me);

        int     (*open)(struct s3c24xx_iis_ops *me, struct snd_pcm_substream *strm);
        int     (*close)(struct s3c24xx_iis_ops *me, struct snd_pcm_substream *strm);
        int     (*prepare)(struct s3c24xx_iis_ops *me, struct snd_pcm_substream *strm, struct snd_pcm_runtime *rt);
};

struct s3c24xx_platdata_iis {
        const char              *codec_clk;
        struct s3c24xx_iis_ops  *ops;
        int                     (*match_dev)(struct device *dev);
};

#endif /* __PLAT_AUDIO_H__ */


