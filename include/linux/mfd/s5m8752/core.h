/*
 * core.h  --  S5M8752 Advanced PMIC with AUDIO Codec
 *
 * Copyright (c) 2010 Samsung Electronics
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#ifndef __LINUX_S5M8752_CORE_H_
#define __LINUX_S5M8752_CORE_H_

#include <linux/mfd/s5m8752/pdata.h>
#include <linux/interrupt.h>

/*****************************************************************************
 * S5M8752 Register Address
 *****************************************************************************/
#define S5M8752_FRT_REGISTER			0x00
#define S5M8752_MAX_REGISTER			0x74
#define S5M8752_CHIP_ID				0xD0

#define S5M8752_IRQB_EVENT1			0x00
#define S5M8752_IRQB_EVENT2			0x01
#define S5M8752_IRQB_MASK1			0x02
#define S5M8752_IRQB_MASK2			0x03

#define S5M8752_ONOFF1				0x04
#define S5M8752_ONOFF2				0x05
#define S5M8752_ONOFF3				0x06

#define S5M8752_SLEEP_CNTL1			0x07
#define S5M8752_SLEEP_CNTL2			0x08
#define S5M8752_O_UVLO				0x09

#define S5M8752_SDC1_SET			0x0A
#define S5M8752_SDC1_VOL			0x0B
#define S5M8752_SDC2_SET			0x0C
#define S5M8752_SDC2_DVS4			0x0D
#define S5M8752_SDC2_DVS3			0x0E
#define S5M8752_SDC2_DVS2			0x0F
#define S5M8752_SDC2_DVS1			0x10
#define S5M8752_SDC3_SET			0x11
#define S5M8752_SDC3_DVS4			0x12
#define S5M8752_SDC3_DVS3			0x13
#define S5M8752_SDC3_DVS2			0x14
#define S5M8752_SDC3_DVS1			0x15
#define S5M8752_SDC4_SET			0x16
#define S5M8752_SDC4_VOL			0x17

#define S5M8752_LDO_RTC_SET			0x18
#define S5M8752_LDO_KA_SET			0x19
#define S5M8752_LDO_1P5_SET			0x1A
#define S5M8752_LDO1_SET			0x1B
#define S5M8752_LDO2_SET			0x1C
#define S5M8752_LDO3_SET			0x1D
#define S5M8752_LDO4_SET			0x1E
#define S5M8752_LDO5_SET			0x1F
#define S5M8752_LDO6_SET			0x20
#define S5M8752_LDO7_SET			0x21

#define S5M8752_WLED_CNTL1			0x22
#define S5M8752_WLED_CNTL2			0x23

#define S5M8752_CHG_CNTL			0x24

#define S5M8752_AD_PDB1				0x25
#define S5M8752_AD_PDB2				0x26
#define S5M8752_AD_PDB3				0x27
#define S5M8752_AD_MUX				0x28
#define S5M8752_AD_LINEVOL			0x29
#define S5M8752_AD_BST				0x2A
#define S5M8752_AD_PGAR				0x2B
#define S5M8752_AD_DIG				0x2C
#define S5M8752_AD_DMUX				0x2D
#define S5M8752_AD_VOLL				0x2E
#define S5M8752_AD_VOLR				0x2F

#define S5M8752_AD_ALC1				0x30
#define S5M8752_AD_ALC2				0x31
#define S5M8752_AD_ALC3				0x32
#define S5M8752_AD_ALC4				0x33
#define S5M8752_AD_ALC5				0x34
#define S5M8752_AD_ALC6				0x35
#define S5M8752_AD_ALC7				0x36
#define S5M8752_AD_ALC8				0x37
#define S5M8752_AD_ALC9				0x38

#define S5M8752_REF1				0x39
#define S5M8752_REF2				0x3A
#define S5M8752_REF3				0x3B

#define S5M8752_DA_PDB				0x3C
#define S5M8752_DA_AMIX0			0x3D
#define S5M8752_DA_AMIX1			0x3E
#define S5M8752_DA_AMIX2			0x3F
#define S5M8752_DA_AMIX3			0x40
#define S5M8752_DA_AMIX4			0x41

#define S5M8752_DA_DEN				0x42
#define S5M8752_DA_DMIX1			0x43
#define S5M8752_DA_DMIX2			0x44
#define S5M8752_DA_DVO1				0x45
#define S5M8752_DA_DVO2				0x46
#define S5M8752_DA_CTL				0x47
#define S5M8752_DA_OFF				0x48
#define S5M8752_DOUTMX1				0x49

#define S5M8752_DA_ALC1				0x4A
#define S5M8752_DA_ALC2				0x4B
#define S5M8752_DA_ALC3				0x4C
#define S5M8752_DA_ALC4				0x4D
#define S5M8752_DA_ALC5				0x4E
#define S5M8752_DA_ALC6				0x4F
#define S5M8752_DA_ALC7				0x50
#define S5M8752_DA_ALC8				0x51
#define S5M8752_DA_ALC9				0x52

#define S5M8752_SYS_CTRL1			0x53
#define S5M8752_SYS_CTRL2			0x54
#define S5M8752_SYS_CTRL3			0x55
#define S5M8752_SYS_CTRL4			0x56
#define S5M8752_IN1_CTRL1			0x57
#define S5M8752_IN1_CTRL2			0x58
#define S5M8752_IN1_CTRL3			0x59
#define S5M8752_IN2_CTRL1			0x5A
#define S5M8752_IN2_CTRL2			0x5B
#define S5M8752_IN2_CTRL3			0x5C

#define S5M8752_SLOT_L2				0x5D
#define S5M8752_SLOT_L1				0x5E
#define S5M8752_SLOT_R2				0x5F
#define S5M8752_SLOT_R1				0x60
#define S5M8752_TSLOT				0x61
#define S5M8752_PLL_P				0x62
#define S5M8752_PLL_M1				0x63
#define S5M8752_PLL_M2				0x64
#define S5M8752_PLL_S				0x65
#define S5M8752_PLL_CTRL			0x66

#define S5M8752_AMP_EN				0x67
#define S5M8752_LINE_VLM			0x68
#define S5M8752_HPL_VLM				0x69
#define S5M8752_HPR_VLM				0x6A
#define S5M8752_SPK_VLM				0x6B
#define S5M8752_HP_CONFIG1			0x6C
#define S5M8752_LINE_HP_CONFIG2			0x6D
#define S5M8752_DUMMY_HPLINE			0x6E
#define S5M8752_FOSC				0x6F
#define S5M8752_SW_SIZE				0x70
#define S5M8752_SPK_DT				0x71
#define S5M8752_SPK_CONFIG_SL			0x72
#define S5M8752_HP_OFFSET			0x73
#define S5M8752_SPK_OFFSET			0x74

/* Charger Test Registers */
#define S5M8752_CHG_TRIM1			0xC0
#define S5M8752_CHG_TRIM2			0xC1
#define S5M8752_CHG_TRIM3			0xC2
#define S5M8752_CHG_TRIM4			0xC3

#define S5M8752_CHG_TEST1			0xC4
#define S5M8752_CHG_TEST2			0xC5
#define S5M8752_CHG_TEST3			0xD6
#define S5M8752_CHG_TEST4			0xD7
#define S5M8752_CHG_TEST5			0xD8

/*****************************************************************************
 * S5M8752 IRQ Number and Mask bits
 *****************************************************************************/
#define S5M8752_IRQ_PK1B			0
#define S5M8752_MASK_PK1B			0x04
#define S5M8752_IRQ_PK2B			1
#define S5M8752_MASK_PK2B			0x02
#define S5M8752_IRQ_PKY3			2
#define S5M8752_MASK_PKY3			0x01
#define S5M8752_IRQ_VCHG_DET			3
#define S5M8752_MASK_VCHG_DET			0x08
#define S5M8752_IRQ_VCHG_REM			4
#define S5M8752_MASK_VCHG_REM			0x04
#define S5M8752_IRQ_CHG_TOUT			5
#define S5M8752_MASK_CHG_TOUT			0x02
#define S5M8752_IRQ_CHG_EOC			6
#define S5M8752_MASK_CHG_EOC			0x01

#define S5M8752_NUM_IRQ				7

/* S5M8752 sleepb */
#define S5M8752_SLEEPB_ENABLE			1
#define S5M8752_SLEEPB_DISABLE			0
#define S5M8752_SLEEPB_PIN_ENABLE		0x02

struct s5m8752;

struct s5m8752 {
	struct device *dev;

	/* device IO */
	struct i2c_client *i2c_client;

	int (*read_dev)(struct s5m8752 *s5m8752, uint8_t reg, uint8_t *val);
	int (*write_dev)(struct s5m8752 *s5m8752, uint8_t reg, uint8_t val);

	int (*read_block_dev)(struct s5m8752 *s5m8752, uint8_t reg, int len,
							uint8_t *val);
	int (*write_block_dev)(struct s5m8752 *s5m8752, uint8_t reg, int len,
							uint8_t *val);
	uint8_t reg_cache[S5M8752_MAX_REGISTER + 1];
	void *control_data;

	/* Interrupt handling */
	struct mutex irq_lock;
	int irq_base;
	int core_irq;
};

/**
 * S5M8752 device init and exit.
 */
int s5m8752_device_init(struct s5m8752 *s5m8752, int irq,
						struct s5m8752_pdata *pdata);
void s5m8752_device_exit(struct s5m8752 *s5m8752);

/**
 * S5M8752 device IO
 */
int s5m8752_reg_cache_read(struct s5m8752 *s5m8752, uint8_t reg, uint8_t *val);
int s5m8752_get_bit(struct s5m8752 *s5m8752, uint8_t reg, int bit);
int s5m8752_clear_bits(struct s5m8752 *s5m8752, uint8_t reg, uint8_t mask);
int s5m8752_set_bits(struct s5m8752 *s5m8752, uint8_t reg, uint8_t mask);
int s5m8752_reg_read(struct s5m8752 *s5m8752, uint8_t reg, uint8_t *val);
int s5m8752_reg_write(struct s5m8752 *s5m8752, uint8_t reg, uint8_t val);
int s5m8752_block_read(struct s5m8752 *s5m8752, uint8_t reg, int len,
							uint8_t *val);
int s5m8752_block_write(struct s5m8752 *s5m8752, uint8_t reg, int len,
							uint8_t *val);

#endif /* __LINUX_S5M8752_CORE_H_ */
