/*
 * regulator.h --  S5M8752 Advanced PMIC with AUDIO Codec
 *
 * Copyright 2010 Samsung Electronics.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#ifndef __LINUX_S5M8752_REGULATOR_H_
#define __LINUX_S5M8752_REGULATOR_H_

#define NUM_S5M8752_REGULATORS			12

/* Regulators */
enum {
	ID_S5M8752_SDC1 = 0,
	ID_S5M8752_SDC2,
	ID_S5M8752_SDC3,
	ID_S5M8752_SDC4,
	ID_S5M8752_LDO_KA,
	ID_S5M8752_LDO1,
	ID_S5M8752_LDO2,
	ID_S5M8752_LDO3,
	ID_S5M8752_LDO4,
	ID_S5M8752_LDO5,
	ID_S5M8752_LDO6,
	ID_S5M8752_LDO7,
};

enum {
	S5M8752_SDC1 = 0,
	S5M8752_SDC2_1,
	S5M8752_SDC2_2,
	S5M8752_SDC2_3,
	S5M8752_SDC2_4,
	S5M8752_SDC3_1,
	S5M8752_SDC3_2,
	S5M8752_SDC3_3,
	S5M8752_SDC3_4,
	S5M8752_SDC4,
	S5M8752_LDO_KA,
	S5M8752_LDO1,
	S5M8752_LDO2,
	S5M8752_LDO3,
	S5M8752_LDO4,
	S5M8752_LDO5,
	S5M8752_LDO6,
	S5M8752_LDO7,
};

/* ONOFF1 register (0x04) */
#define S5M8752_SLEEPB_PIN_EN			(1 << 1)
#define S5M8752_SLEEP_I2C			(1 << 0)

/* ONOFF2 & SLEEP_CNTL1 register */
#define S5M8752_LDO7_SHIFT			7
#define S5M8752_LDO6_SHIFT			6
#define S5M8752_LDO5_SHIFT			5
#define S5M8752_LDO4_SHIFT			4
#define S5M8752_LDO3_SHIFT			3
#define S5M8752_LDO2_SHIFT			2
#define S5M8752_LDO1_SHIFT			1
#define S5M8752_LDO_KA_SHIFT			0

/* ONOFF2 & SLEEP_CNTL2 register */
#define S5M8752_SDC4_SHIFT			3
#define S5M8752_SDC3_SHIFT			2
#define S5M8752_SDC2_SHIFT			1
#define S5M8752_SDC1_SHIFT			0

/* Regulator Voltage Mask */
#define S5M8752_SDC1_VOL_MASK			0x1F
#define S5M8752_SDC2_VOL_MASK			0x1F
#define S5M8752_SDC3_VOL_MASK			0x1F
#define S5M8752_SDC4_VOL_MASK			0x0F
#define S5M8752_LDO_RTC_VOL_MASK		0x03
#define S5M8752_LDO_KA_VOL_MASK			0x0F
#define S5M8752_LDO1_VOL_MASK			0x1F
#define S5M8752_LDO_VOL_MASK			0x1F
#define S5M8752_LDO2_VOL_MASK			0x1F
#define S5M8752_LDO3_VOL_MASK			0x1F
#define S5M8752_LDO4_VOL_MASK			0x1F
#define S5M8752_LDO5_VOL_MASK			0x1F
#define S5M8752_LDO6_VOL_MASK			0x1F
#define S5M8752_LDO7_VOL_MASK			0x1F

int s5m8752_regulator_onoff_direct(int regu, int onoff);
int s5m8752_regulator_voltage_direct(int regu, int uV);

#endif /* __LINUX_S5M8752_REGULATOR_H_ */
