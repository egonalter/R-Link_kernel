/*
 * pdata.h  --  S5M8752 Advanced PMIC with AUDIO Codec
 *
 * Copyright 2010 Samsung Electronics.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#ifndef __MFD_S5M8752_PDATA_H__
#define __MFD_S5M8752_PDATA_H__

struct s5m8752;
struct regulator_init_data;

/**
 * S5M8752 backlight pdata
 */
struct s5m8752_backlight_pdata {
	int pwm_freq;
	int brightness;
};

/**
 * S5M8752 battery charger pdata
 */
struct s5m8752_power_pdata {
	int charger_type;
	int current_limit;
	int gpio_adp;
	int gpio_ilim0;
	int gpio_ilim1;
};

/**
 * S5M8752 audio codec pdata
 */
struct s5m8752_audio_pdata {
	uint8_t linein_vol;
	uint8_t adc_vol;
	uint8_t dac_vol;
	uint8_t lineout_vol;
	uint8_t hp_preamp_vol;
	uint8_t spk_preamp_vol;
};

/**
 * S5M8752 buck2, buck3 dvs gpio data
 */
struct s5m8752_dvs_pdata {
	int gpio_dvs2a;
	int gpio_dvs2b;
	int gpio_dvs3a;
	int gpio_dvs3b;
};

#define S5M8752_MAX_REGULATOR			12

struct s5m8752_pdata {
	struct regulator_init_data *regulator[S5M8752_MAX_REGULATOR];
	struct s5m8752_backlight_pdata *backlight;
	struct s5m8752_power_pdata *power;
	struct s5m8752_audio_pdata *audio;

	int irq_base;
	int gpio_pshold;

	struct s5m8752_dvs_pdata *dvs;
};

#endif /* __LINUX_S5M8752_PDATA_H_ */
