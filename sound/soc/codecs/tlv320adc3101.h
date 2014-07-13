/*
 * ALSA SoC TLV320ADC3101 codec driver
 *
 * Author:      Vladimir Barinov, <vbarinov@embeddedalley.com>
 * Copyright:   (C) 2007 MontaVista Software, Inc., <source@mvista.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _ADC3101_H
#define _ADC3101_H

/* ADC3101 register space */
#define ADC3101_CHAHEREGNUM		102

/******************************************
 * PAGE 0
 ******************************************/

/* Page select register */
#define ADC3101_PAGE_SELECT		0
/* Software reset register */
#define ADC3101_RESET			1
/* Revision ID */
#define ADC3101_REV_ID			2
/* Clock generator muxing */
#define ADC3101_CLK_GEN_MUXING		4
/* PLL divider P and PLL multiplier R */
#define ADC3101_PLL_P_R_VAL		5
/* PLL multiplier J */
#define ADC3101_PLL_J_VAL		6
/* PLL multipler D MSB */
#define ADC3101_PLL_D_VAL_MSB		7
/* PLL multipler D LSB */
#define ADC3101_PLL_D_VAL_LSB		8
/* ADC NADC divider */
#define ADC3101_ADC_NADC_VAL		18
/* ADC MADC divider */
#define ADC3101_ADC_MADC_VAL		19
/* ADC OSR AOSR */
#define ADC3101_ADC_AOSR_VAL		20
/* ADC IADC */
#define ADC3101_ADC_IADC_VAL		21
/* ADC MAC Engine Decimation */
#define ADC3101_ADC_MAC_VAL		22
/* ADC Clock MUX */
#define ADC3101_ADC_CLKOUT_MUX		25
/* ADC CLKOUT divider M */
#define ADC3101_CLKOUT_M_VAL		26
/* ADC interface Control1 */
#define ADC3101_ADC_INT_CONT1		27
/* Data Slot Offset Programmability1 */
#define ADC3101_DATA_SLOT_OFFSET1	28
/* ADC interface Control2 */
#define ADC3101_ADC_INT_CONT2		29
/* ADC BCLK N */
#define ADC3101_BCLK_N_VAL		30
/* Codec Secondary interface Cotnrol1 */
#define ADC3101_CODEC_INT_CONT1		31
/* Codec Secondary interface Control2 */
#define ADC3101_CODEC_INT_CONT2		32
/* Codec Secondary interface Control3 */
#define ADC3101_CODEC_INT_CONT3		33
/* I2S Sync */
#define ADC3101_I2S_SYNC		34
/* ADC Flag Register */
#define ADC3101_ADC_FLAG_REG		36
/* Data Slot Offset Programmability2 */
#define ADC3101_DATA_SLOT_OFFSET2	37
/* I2S TDM Control Register */
#define ADC3101_I2S_TDM_CONT		38
/* Interrupt Flags1 (Overflow) */
#define ADC3101_INTERRUPT_FLAGS1	42
/* Interrupt Flags2 (Overflow) */
#define ADC3101_INTTERUPT_FLAGS2	43
/* Interrupt Flags ADC1 */
#define ADC3101_INTERRUPT_FLAGS_ADC1	45
/* Interupt Flags ADC2 */
#define ADC3101_INTERRUPT_FLAGS_ADC2	47
/* INT1 interrupt Control */
#define ADC3101_INT1_INTERRUPT_CNT	48
/* INT2 interrupt Control */
#define ADC3101_INT2_INTERRUPT_CNT	49
/* DMCLK GPIO2 Control */
#define ADC3101_DMCLK_GPIO2_CNT		51
/* DMDIN GPIO1 Control */
#define ADC3101_DMDIN_GPIO1_CNT		52
/* DOUT pin Control */
#define ADC3101_DOUT_CNT		53
/* ADC Sync Control1 */
#define ADC3101_ADC_SYNC_CNT1		57
/* ADC Sync Control2 */
#define ADC3101_ADC_SYNC_CNT2		58
/* ADC CIC Filter Gain Control */
#define ADC3101_ADC_CIC_FILTER_GAIN	59
/* ADC Instruction Set */
#define ADC3101_ADC_INST_SET		61
/* Programmable Instruction Mode Control bit */
#define ADC3101_INST_MODE_CNT		62
/* ADC Digital Microphone Polarity Select */
#define ADC3101_ADC_DM_POL_CNT		80
/* ADC Digital */
#define ADC3101_ADC_DIGITAL		81
/* ADC Volume Control */
#define ADC3101_ADC_VOL_CNT		82
/* Left ADC Volume Control */
#define ADC3101_LADC_VOL_CNT		83
/* Right ADC Volume Control */
#define ADC3101_RADC_VOL_CNT		84
/* Left ADC Phase Compensation */
#define ADC3101_LADC_PHASE_CNT		85
/* Left AGC Control1 */
#define ADC3101_LAGC_CNT1		86
/* Left AGC Control2 */
#define ADC3101_LAGC_CNT2		87
/* Left AGC Maximum Gain */
#define ADC3101_LAGC_MAX_GAIN		88
/* Left AGC Attack Time */
#define ADC3101_LAGC_ATTACK_TIME	89
/* Left AGC Decay Time */
#define ADC3101_LAGC_DECAY_TIME		90
/* Left AGC Noise Debounce */
#define ADC3101_LAGC_NOISE_DEBOUNCE	91
/* Left AGC Signal Debounce */
#define ADC3101_LAGC_SIGNAL_DEBOUNCE	92
/* Left AGC Gain Applied */
#define ADC3101_LAGC_GAIN_APPLIED	93
/* Right AGC Control1 */
#define ADC3101_RAGC_CNT1		94
/* Right AGC Control2 */
#define ADC3101_RAGC_CNT2		95
/* Right AGC Maximum Gain */
#define ADC3101_RAGC_MAX_GAIN		96
/* Right AGC Attack Time */
#define ADC3101_RAGC_ATTACK_TIME	97
/* Right AGC Decay Time */
#define ADC3101_RAGC_DECAY_TIME		98
/* Right AGC Noise Debounce */
#define ADC3101_RAGC_NOISE_DEBOUNCE	99
/* Right AGC Signal Debounce */
#define ADC3101_RAGC_SIGNAL_DEBOUNCE	100
/* Right AGC Gain Applied */
#define ADC3101_RAGC_GAIN_APPLIED	101

/******************************************
 * PAGE 1
 ******************************************/

/* Page Control Register */
#define ADC3101_PAGE_CONTROL		0
/* Dither Control Register */
#define ADC3101_DITHER_CONTROL		26
/* Micbias Control */
#define ADC3101_MICBIAS_CONTROL		51
/* Left ADC Input Selection for Left PGA1 */
#define ADC3101_LADC_INPUT_LPGA1	52
/* Left ADC Input Selection for Left PGA2 */
#define ADC3101_LADC_INPUT_LPGA2	54
/* Right ADC Input Selection for Right PGA1 */
#define ADC3101_RADC_INPUT_RPGA1	55
/* Right ADC Input Selection for Right PGA2 */
#define ADC3101_RADC_INPUT_RPGA2	57
/* Left Analog PGA Setting */
#define ADC3101_LPGA_SETTING		59
/* Right Analog PGA Setting */
#define ADC3101_RPGA_SETTING		60
/* ADC Low Current Mode */
#define ADC3101_LOW_CURRENT_MODE	61
/* ADC Analog PGA Flags */
#define ADC3101_ADC_PGA_FLAGS		62

/* Page select register bits */
#define PAGE0_SELECT			0
#define PAGE1_SELECT			1

/* Software reset register bits */
#define SOFT_RESET			0x01

/* Default input volume */
#define DEFAULT_GAIN    		0x00

struct adc3101_setup_data {
	unsigned short i2c_address;
};

extern struct snd_soc_codec_device soc_codec_dev_adc3101;
extern struct snd_soc_dai tlv320adc3101_dai;

int tlv320adc3101_gpio_i2s_init(void);

#endif /* _TLV320ADC3101_H */
