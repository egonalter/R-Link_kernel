/*
 * Include file for the TomTom Alsa System on chip.
 * This file serves as a contract between the platform
 * device created in the arch/platform and the platform
 * driver accepting the driver
 *
 * Author: Kees Jongenburger <kees.jongenburger@tomtom.com>
 *  * (C) Copyright 2008 TomTom International BV. 
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation version 2.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#ifndef __ASOC_TOMTOM_H__
#define __ASOC_TOMTOM_H__

#include <linux/ttio.h>
#include <linux/workqueue.h>

typedef enum {
    ESTATE_UNKNOWN,
    ESTATE_PLUGGED_OUT,
    ESTATE_PLUGGED_IN
} e_hp_state_t;


struct asoc_tomtom {	
	struct tt_pin dac_power_on;
	struct tt_pin amp_1_on;
	struct tt_pin amp_2_on;
	struct tt_pin vcc_amp_on;
	struct tt_pin cdclk;
	struct tt_pin hp_detect;
	struct tt_pin mic_stby;
	uint16_t codec_i2c_address;
	int cdclk_pwm;
	uint32_t codec_i2c_bus;
	int second_amplifier;  
 
	/**
	 * The kernel work queue used in order to minimize the impact of
	 * the headphones events management on kernel responsiveness
	 **/
	struct delayed_work asoc_work;
	e_hp_state_t e_hp_state;
};

#endif /* __ASOC_TOMTOM_H__ */
