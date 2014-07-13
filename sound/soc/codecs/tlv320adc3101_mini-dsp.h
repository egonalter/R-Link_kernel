/*
 * linux/sound/soc/codecs/tlv320adc3101_mini-dsp.h
 *
 *
 * Copyright (C) 2009 Texas Instruments, Inc.
 *
 *
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * THIS PACKAGE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * History:
 *
 * 
 *
 * 
 */

#ifndef _TLV320ADC3101_MINI_DSP_H
#define _TLV320ADC3101_MINI_DSP_H

/* typedefs required for the included header files */
typedef char *string;

/* Include header file generated from PPS */
#include "adc3101_rate44_pps_driver.h"

/* defines */

/* Select the functionalities to be used in mini dsp module */
#define PROGRAM_MINI_DSP_A
#define PROGRAM_CODEC_REG_SECTIONS
#define ADD_MINI_DSP_CONTROLS

/* volume ranges from -110db to 6db
 * amixer controls doesnot accept negative values
 * Therefore we are normalizing vlues to start from value 0
 * value 0 corresponds to -110db and 116 to 6db
 */
#define MAX_VOLUME_CONTROLS				2
#define MIN_VOLUME						0
#define MAX_VOLUME						116
#define VOLUME_REG_SIZE					2	/*  2 bytes */
#define VOLUME_KCONTROL_NAME			" (0=-110dB, 116=+6dB)"

#define MINIDSP_PARSING_START			0
#define MINIDSP_PARSING_END				(-1)

#define CODEC_REG_DONT_IGNORE			0
#define CODEC_REG_IGNORE				1

#define CODEC_REG_PRE_INIT				0
#define CODEC_REG_POST_INIT				1
#define INIT_SEQ_DELIMITER				255	/* Delimiter register */
#define DELIMITER_COUNT					1	/* 1 delimiter entries */

/* Parser info structure */
typedef struct {
	char page_num;
	char burst_array[129];
	int burst_size;
	int current_loc;
} minidsp_parser_data;

#define ADC3101_IOC_MAGIC	0xE0
#define ADC3101_IOMAGICNUM_GET	_IOR(ADC3101_IOC_MAGIC, 1, int)
#define ADC3101_IOMAGICNUM_SET	_IOW(ADC3101_IOC_MAGIC, 2, int)

#endif
