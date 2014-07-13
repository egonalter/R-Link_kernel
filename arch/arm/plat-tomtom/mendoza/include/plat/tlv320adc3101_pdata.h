/* tlv320adc3101_pdata.h
 *
 * Control driver for tlv320adc3101 .
 *
 * Copyright (C) 2009 TomTom BV <http://www.tomtom.com/>
 * Authors: Niels Langendorff <niels.langendorff@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef INCLUDE_PLAT_TLV320ADC3101_PDATA_H
#define INCLUDE_PLAT_TLV320ADC3101_PDATA_H

typedef void (* asset_pin_f)(int);

typedef struct
{
	asset_pin_f	mic_reset;

	void (*suspend)(void);
	void (*resume)(void);
	void (*config_gpio) (void);
	int (*request_gpio)  (void);
	void (*free_gpio) (void);
} tlv320adc3101_platform_t;

struct adc3101_priv {
	unsigned int sysclk;
	int master;
	int in_source;
	u8 page_no;

	struct work_struct resume_work;
	struct snd_soc_codec *codec;
	int dapm_state_suspend;
	struct platform_device *pdev;
	tlv320adc3101_platform_t *pdata;
};

#define TLV320ADC3101_DEVNAME			"TLV320ADC3101-Codec"

#endif /* INCLUDE_PLAT_TLV320ADC3101_PDATA_H */
