/*
 * s5m8752-regulator.c  --  S5M8752 Advanced PMIC with AUDIO Codec
 *
 * Copyright 2010 Samsung Electronics.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/platform_device.h>
#include <linux/err.h>
#include <linux/slab.h>
#include <linux/regulator/driver.h>
#include <linux/mfd/s5m8752/core.h>
#include <linux/mfd/s5m8752/regulator.h>

#include <mach/gpio.h>

struct s5m8752_regulator {
	char name[NUM_S5M8752_REGULATORS];
	struct regulator_desc desc;
	struct s5m8752 *s5m8752;
	struct regulator_dev *regul;
};

struct s5m8752_regulator_info {
	int def_vol;
	int min_vol;
	int max_vol;
	int step_vol;
};

struct s5m8752 *s5m8752_p;

/******************************************************************************
 * This structer consists of default voltage, minimum voltage, maximum voltage
 * and voltage step of S5M8752 regulators.
 ******************************************************************************/
const struct s5m8752_regulator_info info[] = {
	/* def_vol	min_vol		max_vol		step_vol */
	{ 3300,		1100,		4200,		100 }, /* SDC1 */
	{ 1200,		800,		1575,		25 }, /* SDC2 */
	{ 1200,		800,		1575,		25 }, /* SDC3 */
	{ 1800,		800,		2300,		100 }, /* SDC4 */
	{ 1100,		900,		1300,		50 }, /* LDO_KA */
	{ 1100,		900,		3300,		100 }, /* LDO1 */
	{ 1800,		900,		3300,		100 }, /* LDO2 */
	{ 1800,		900,		3300,		100 }, /* LDO3 */
	{ 3300,		900,		3300,		100 }, /* LDO4 */
	{ 1100,		900,		3300,		100 }, /* LDO5 */
	{ 2800,		900,		3300,		100 }, /* LDO6 */
	{ 3300,		900,		3300,		100 }, /* LDO7 */
};

/******************************************************************************
 * This function changes voltage level to regsiter bit value.
 ******************************************************************************/
static inline uint8_t s5m8752_mvolts_to_val(int mV, int min, int step)
{
	return (mV - min) / step;
}

/******************************************************************************
 * This function changes regsiter bit value to voltage level.
 ******************************************************************************/
static inline int s5m8752_val_to_mvolts(uint8_t val, int min, int step)
{
	return (val * step) + min;
}

/******************************************************************************
 * This function sets the voltage level of S5M8752 regulators.
 *
 * SDC2, SDC3 DVS pin status set
 *	DVS1 : dvsB = low, dvsA = low
 *	DVS2 : dvsB = low, dvsA = high
 *	DVS3 : dvsB = high, dvsA = low
 *	DVS4 : dvsB = high, dvsA = high
 ******************************************************************************/
int s5m8752_regulator_set_voltage(struct regulator_dev *rdev, int min_uV,
	int max_uV)
{
	struct s5m8752_regulator *regulator = rdev_get_drvdata(rdev);
	struct s5m8752 *s5m8752 = regulator->s5m8752;
	struct s5m8752_pdata *pdata = s5m8752->dev->platform_data;
	int dvs, ret;
	int volt_reg, regu = rdev_get_id(rdev), mV, min_mV = min_uV / 1000,
		max_mV = max_uV / 1000;
	uint8_t mask, val;

	if (min_mV < info[regu].min_vol || min_mV > info[regu].max_vol)
		return -EINVAL;
	if (max_mV < info[regu].min_vol || max_mV > info[regu].max_vol)
		return -EINVAL;

	mV = s5m8752_mvolts_to_val(min_mV, info[regu].min_vol,
						info[regu].step_vol);

	switch (regu) {
	case ID_S5M8752_SDC1:
		volt_reg = S5M8752_SDC1_VOL;
		mask = S5M8752_SDC1_VOL_MASK;
		break;
	case ID_S5M8752_SDC2:
		dvs = (gpio_get_value(pdata->dvs->gpio_dvs2b) << 1) +
					gpio_get_value(pdata->dvs->gpio_dvs2a);
		if (dvs == 3)
			volt_reg = S5M8752_SDC2_DVS4;
		else if (dvs == 2)
			volt_reg = S5M8752_SDC2_DVS3;
		else if (dvs == 1)
			volt_reg = S5M8752_SDC2_DVS2;
		else
			volt_reg = S5M8752_SDC2_DVS1;

		mask = S5M8752_SDC2_VOL_MASK;
		break;
	case ID_S5M8752_SDC3:
		dvs = (gpio_get_value(pdata->dvs->gpio_dvs3b) << 1) +
					gpio_get_value(pdata->dvs->gpio_dvs3a);
		if (dvs == 3)
			volt_reg = S5M8752_SDC3_DVS4;
		else if (dvs == 2)
			volt_reg = S5M8752_SDC3_DVS3;
		else if (dvs == 1)
			volt_reg = S5M8752_SDC3_DVS2;
		else
			volt_reg = S5M8752_SDC3_DVS1;

		mask = S5M8752_SDC3_VOL_MASK;
		break;
	case ID_S5M8752_SDC4:
		volt_reg = S5M8752_SDC4_VOL;
		mask = S5M8752_SDC4_VOL_MASK;
		break;
	case ID_S5M8752_LDO_KA:
		volt_reg = S5M8752_LDO_KA_SET;
		mask = S5M8752_LDO_KA_VOL_MASK;
		break;
	case ID_S5M8752_LDO1:
		volt_reg = S5M8752_LDO1_SET;
		mask = S5M8752_LDO1_VOL_MASK;
		break;
	case ID_S5M8752_LDO2:
		volt_reg = S5M8752_LDO2_SET;
		mask = S5M8752_LDO2_VOL_MASK;
		break;
	case ID_S5M8752_LDO3:
		volt_reg = S5M8752_LDO3_SET;
		mask = S5M8752_LDO3_VOL_MASK;
		break;
	case ID_S5M8752_LDO4:
		volt_reg = S5M8752_LDO4_SET;
		mask = S5M8752_LDO4_VOL_MASK;
		break;
	case ID_S5M8752_LDO5:
		volt_reg = S5M8752_LDO5_SET;
		mask = S5M8752_LDO5_VOL_MASK;
		break;
	case ID_S5M8752_LDO6:
		volt_reg = S5M8752_LDO6_SET;
		mask = S5M8752_LDO6_VOL_MASK;
		break;
	case ID_S5M8752_LDO7:
		volt_reg = S5M8752_LDO7_SET;
		mask = S5M8752_LDO7_VOL_MASK;
		break;
	default:
		return -EINVAL;
	}

	ret = s5m8752_reg_read(s5m8752, volt_reg, &val);
	if (ret)
		goto out;

	val &= ~mask;
	ret = s5m8752_reg_write(s5m8752, volt_reg, val | mV);
out:
	return ret;
}

/******************************************************************************
 * This function gets the voltage level of S5M8752 regulators.
 ******************************************************************************/
static int s5m8752_regulator_get_voltage(struct regulator_dev *rdev)
{
	struct s5m8752_regulator *regulator = rdev_get_drvdata(rdev);
	struct s5m8752 *s5m8752 = regulator->s5m8752;
	struct s5m8752_pdata *pdata = s5m8752->dev->platform_data;
	int dvs, ret;
	int volt_reg, regu = rdev_get_id(rdev);
	uint8_t mask, val;

	switch (regu) {
	case ID_S5M8752_SDC1:
		volt_reg = S5M8752_SDC1_VOL;
		mask = S5M8752_SDC1_VOL_MASK;
		break;
	case ID_S5M8752_SDC2:
		dvs = (gpio_get_value(pdata->dvs->gpio_dvs2b) << 1) +
					gpio_get_value(pdata->dvs->gpio_dvs2a);
		if (dvs == 3)
			volt_reg = S5M8752_SDC2_DVS4;
		else if (dvs == 2)
			volt_reg = S5M8752_SDC2_DVS3;
		else if (dvs == 1)
			volt_reg = S5M8752_SDC2_DVS2;
		else
			volt_reg = S5M8752_SDC2_DVS1;

		mask = S5M8752_SDC2_VOL_MASK;
		break;
	case ID_S5M8752_SDC3:
		dvs = (gpio_get_value(pdata->dvs->gpio_dvs3b) << 1) +
					gpio_get_value(pdata->dvs->gpio_dvs3a);
		if (dvs == 3)
			volt_reg = S5M8752_SDC3_DVS4;
		else if (dvs == 2)
			volt_reg = S5M8752_SDC3_DVS3;
		else if (dvs == 1)
			volt_reg = S5M8752_SDC3_DVS2;
		else
			volt_reg = S5M8752_SDC3_DVS1;

		mask = S5M8752_SDC3_VOL_MASK;
		break;
	case ID_S5M8752_SDC4:
		volt_reg = S5M8752_SDC4_VOL;
		mask = S5M8752_SDC4_VOL_MASK;
		break;
	case ID_S5M8752_LDO_KA:
		volt_reg = S5M8752_LDO_KA_SET;
		mask = S5M8752_LDO_KA_VOL_MASK;
		break;
	case ID_S5M8752_LDO1:
		volt_reg = S5M8752_LDO1_SET;
		mask = S5M8752_LDO1_VOL_MASK;
		break;
	case ID_S5M8752_LDO2:
		volt_reg = S5M8752_LDO2_SET;
		mask = S5M8752_LDO2_VOL_MASK;
		break;
	case ID_S5M8752_LDO3:
		volt_reg = S5M8752_LDO3_SET;
		mask = S5M8752_LDO3_VOL_MASK;
		break;
	case ID_S5M8752_LDO4:
		volt_reg = S5M8752_LDO4_SET;
		mask = S5M8752_LDO4_VOL_MASK;
		break;
	case ID_S5M8752_LDO5:
		volt_reg = S5M8752_LDO5_SET;
		mask = S5M8752_LDO5_VOL_MASK;
		break;
	case ID_S5M8752_LDO6:
		volt_reg = S5M8752_LDO6_SET;
		mask = S5M8752_LDO6_VOL_MASK;
		break;
	case ID_S5M8752_LDO7:
		volt_reg = S5M8752_LDO7_SET;
		mask = S5M8752_LDO7_VOL_MASK;
		break;
	default:
		return -EINVAL;
	}

	ret = s5m8752_reg_read(s5m8752, volt_reg, &val);
	if (ret)
		goto out;

	val &= mask;
	ret = s5m8752_val_to_mvolts(val, info[regu].min_vol,
					info[regu].step_vol) * 1000;
out:
	return ret;
}

/******************************************************************************
 * This function enables the selected S5M8752 regulator.
 ******************************************************************************/
static int s5m8752_regulator_enable(struct regulator_dev *rdev)
{
	struct s5m8752_regulator *regulator = rdev_get_drvdata(rdev);
	struct s5m8752 *s5m8752 = regulator->s5m8752;
	int ret;
	int regu = rdev_get_id(rdev);
	uint8_t enable_reg, shift;

	switch (regu) {
	case ID_S5M8752_SDC1:
		enable_reg = S5M8752_ONOFF3;
		shift = S5M8752_SDC1_SHIFT;
		break;
	case ID_S5M8752_SDC2:
		enable_reg = S5M8752_ONOFF3;
		shift = S5M8752_SDC2_SHIFT;
		break;
	case ID_S5M8752_SDC3:
		enable_reg = S5M8752_ONOFF3;
		shift = S5M8752_SDC3_SHIFT;
		break;
	case ID_S5M8752_SDC4:
		enable_reg = S5M8752_ONOFF3;
		shift = S5M8752_SDC4_SHIFT;
		break;
	case ID_S5M8752_LDO_KA:
		enable_reg = S5M8752_ONOFF2;
		shift = S5M8752_LDO_KA_SHIFT;
		break;
	case ID_S5M8752_LDO1:
		enable_reg = S5M8752_ONOFF2;
		shift = S5M8752_LDO1_SHIFT;
		break;
	case ID_S5M8752_LDO2:
		enable_reg = S5M8752_ONOFF2;
		shift = S5M8752_LDO2_SHIFT;
		break;
	case ID_S5M8752_LDO3:
		enable_reg = S5M8752_ONOFF2;
		shift = S5M8752_LDO3_SHIFT;
		break;
	case ID_S5M8752_LDO4:
		enable_reg = S5M8752_ONOFF2;
		shift = S5M8752_LDO4_SHIFT;
		break;
	case ID_S5M8752_LDO5:
		enable_reg = S5M8752_ONOFF2;
		shift = S5M8752_LDO5_SHIFT;
		break;
	case ID_S5M8752_LDO6:
		enable_reg = S5M8752_ONOFF3;
		shift = S5M8752_LDO6_SHIFT;
		break;
	case ID_S5M8752_LDO7:
		enable_reg = S5M8752_ONOFF3;
		shift = S5M8752_LDO7_SHIFT;
		break;
	default:
		return -EINVAL;
	}

	ret = s5m8752_set_bits(s5m8752, enable_reg, 1 << shift);

	return ret;
}

/******************************************************************************
 * This function disables the selected S5M8752 regulator.
 ******************************************************************************/
static int s5m8752_regulator_disable(struct regulator_dev *rdev)
{
	struct s5m8752_regulator *regulator = rdev_get_drvdata(rdev);
	struct s5m8752 *s5m8752 = regulator->s5m8752;
	int regu = rdev_get_id(rdev);
	int ret;
	uint8_t enable_reg, shift;

	switch (regu) {
	case ID_S5M8752_SDC1:
		enable_reg = S5M8752_ONOFF3;
		shift = S5M8752_SDC1_SHIFT;
		break;
	case ID_S5M8752_SDC2:
		enable_reg = S5M8752_ONOFF3;
		shift = S5M8752_SDC2_SHIFT;
		break;
	case ID_S5M8752_SDC3:
		enable_reg = S5M8752_ONOFF3;
		shift = S5M8752_SDC3_SHIFT;
		break;
	case ID_S5M8752_SDC4:
		enable_reg = S5M8752_ONOFF3;
		shift = S5M8752_SDC4_SHIFT;
		break;
	case ID_S5M8752_LDO_KA:
		enable_reg = S5M8752_ONOFF2;
		shift = S5M8752_LDO_KA_SHIFT;
		break;
	case ID_S5M8752_LDO1:
		enable_reg = S5M8752_ONOFF2;
		shift = S5M8752_LDO1_SHIFT;
		break;
	case ID_S5M8752_LDO2:
		enable_reg = S5M8752_ONOFF2;
		shift = S5M8752_LDO2_SHIFT;
		break;
	case ID_S5M8752_LDO3:
		enable_reg = S5M8752_ONOFF2;
		shift = S5M8752_LDO3_SHIFT;
		break;
	case ID_S5M8752_LDO4:
		enable_reg = S5M8752_ONOFF2;
		shift = S5M8752_LDO4_SHIFT;
		break;
	case ID_S5M8752_LDO5:
		enable_reg = S5M8752_ONOFF2;
		shift = S5M8752_LDO5_SHIFT;
		break;
	case ID_S5M8752_LDO6:
		enable_reg = S5M8752_ONOFF3;
		shift = S5M8752_LDO6_SHIFT;
		break;
	case ID_S5M8752_LDO7:
		enable_reg = S5M8752_ONOFF3;
		shift = S5M8752_LDO7_SHIFT;
		break;
	default:
		return -EINVAL;
	}

	ret = s5m8752_clear_bits(s5m8752, enable_reg, 1 << shift);

	return ret;
}

static int s5m8752_regulator_list_voltage(struct regulator_dev *rdev,
					unsigned selector)
{
	int regu = rdev_get_id(rdev);

	if (selector > S5M8752_LDO1_VOL_MASK)
		return -EINVAL;
	return s5m8752_val_to_mvolts(selector, info[regu].min_vol,
						info[regu].step_vol) * 1000;
}

/******************************************************************************
 * This function enables the selected S5M8752 regulator in suspend mode.
 ******************************************************************************/
static int s5m8752_regulator_set_suspend_enable(struct regulator_dev *rdev)
{
	struct s5m8752_regulator *regulator = rdev_get_drvdata(rdev);
	struct s5m8752 *s5m8752 = regulator->s5m8752;
	int ret;
	int regu = rdev_get_id(rdev);
	uint8_t enable_reg, shift;

	switch (regu) {
	case ID_S5M8752_SDC1:
		enable_reg = S5M8752_SLEEP_CNTL2;
		shift = S5M8752_SDC1_SHIFT;
		break;
	case ID_S5M8752_SDC2:
		enable_reg = S5M8752_SLEEP_CNTL2;
		shift = S5M8752_SDC2_SHIFT;
		break;
	case ID_S5M8752_SDC3:
		enable_reg = S5M8752_SLEEP_CNTL2;
		shift = S5M8752_SDC3_SHIFT;
		break;
	case ID_S5M8752_SDC4:
		enable_reg = S5M8752_SLEEP_CNTL2;
		shift = S5M8752_SDC4_SHIFT;
		break;
	case ID_S5M8752_LDO_KA:
		enable_reg = S5M8752_SLEEP_CNTL1;
		shift = S5M8752_LDO_KA_SHIFT;
		break;
	case ID_S5M8752_LDO1:
		enable_reg = S5M8752_SLEEP_CNTL1;
		shift = S5M8752_LDO1_SHIFT;
		break;
	case ID_S5M8752_LDO2:
		enable_reg = S5M8752_SLEEP_CNTL1;
		shift = S5M8752_LDO2_SHIFT;
		break;
	case ID_S5M8752_LDO3:
		enable_reg = S5M8752_SLEEP_CNTL1;
		shift = S5M8752_LDO3_SHIFT;
		break;
	case ID_S5M8752_LDO4:
		enable_reg = S5M8752_SLEEP_CNTL1;
		shift = S5M8752_LDO4_SHIFT;
		break;
	case ID_S5M8752_LDO5:
		enable_reg = S5M8752_SLEEP_CNTL1;
		shift = S5M8752_LDO5_SHIFT;
		break;
	case ID_S5M8752_LDO6:
		enable_reg = S5M8752_SLEEP_CNTL1;
		shift = S5M8752_LDO6_SHIFT;
		break;
	case ID_S5M8752_LDO7:
		enable_reg = S5M8752_SLEEP_CNTL1;
		shift = S5M8752_LDO7_SHIFT;
		break;
	default:
		return -EINVAL;
	}

	ret = s5m8752_set_bits(s5m8752, enable_reg, 1 << shift);

	return ret;
}

/******************************************************************************
 * This function disables the selected S5M8752 regulator in suspend mode.
 ******************************************************************************/
static int s5m8752_regulator_set_suspend_disable(struct regulator_dev *rdev)
{
	struct s5m8752_regulator *regulator = rdev_get_drvdata(rdev);
	struct s5m8752 *s5m8752 = regulator->s5m8752;
	int ret;
	int regu = rdev_get_id(rdev);
	uint8_t enable_reg, shift;
	switch (regu) {
	case ID_S5M8752_SDC1:
		enable_reg = S5M8752_SLEEP_CNTL2;
		shift = S5M8752_SDC1_SHIFT;
		break;
	case ID_S5M8752_SDC2:
		enable_reg = S5M8752_SLEEP_CNTL2;
		shift = S5M8752_SDC2_SHIFT;
		break;
	case ID_S5M8752_SDC3:
		enable_reg = S5M8752_SLEEP_CNTL2;
		shift = S5M8752_SDC3_SHIFT;
		break;
	case ID_S5M8752_SDC4:
		enable_reg = S5M8752_SLEEP_CNTL2;
		shift = S5M8752_SDC4_SHIFT;
		break;
	case ID_S5M8752_LDO_KA:
		enable_reg = S5M8752_SLEEP_CNTL1;
		shift = S5M8752_LDO_KA_SHIFT;
		break;
	case ID_S5M8752_LDO1:
		enable_reg = S5M8752_SLEEP_CNTL1;
		shift = S5M8752_LDO1_SHIFT;
		break;
	case ID_S5M8752_LDO2:
		enable_reg = S5M8752_SLEEP_CNTL1;
		shift = S5M8752_LDO2_SHIFT;
		break;
	case ID_S5M8752_LDO3:
		enable_reg = S5M8752_SLEEP_CNTL1;
		shift = S5M8752_LDO3_SHIFT;
		break;
	case ID_S5M8752_LDO4:
		enable_reg = S5M8752_SLEEP_CNTL1;
		shift = S5M8752_LDO4_SHIFT;
		break;
	case ID_S5M8752_LDO5:
		enable_reg = S5M8752_SLEEP_CNTL1;
		shift = S5M8752_LDO5_SHIFT;
		break;
	case ID_S5M8752_LDO6:
		enable_reg = S5M8752_SLEEP_CNTL1;
		shift = S5M8752_LDO6_SHIFT;
		break;
	case ID_S5M8752_LDO7:
		enable_reg = S5M8752_SLEEP_CNTL1;
		shift = S5M8752_LDO7_SHIFT;
		break;
	default:
		return -EINVAL;
	}

	ret = s5m8752_clear_bits(s5m8752, enable_reg, 1 << shift);

	return ret;
}

/******************************************************************************
 * This function checkes the OnOff status of the selected S5M8752 regulator.
 ******************************************************************************/
static int s5m8752_regulator_is_enabled(struct regulator_dev *rdev)
{
	struct s5m8752_regulator *regulator = rdev_get_drvdata(rdev);
	struct s5m8752 *s5m8752 = regulator->s5m8752;
	int ret = 0;
	int regu = rdev_get_id(rdev), shift;
	uint8_t enable_reg, val;

	switch (regu) {
	case ID_S5M8752_SDC1:
		enable_reg = S5M8752_ONOFF3;
		shift = S5M8752_SDC1_SHIFT;
		break;
	case ID_S5M8752_SDC2:
		enable_reg = S5M8752_ONOFF3;
		shift = S5M8752_SDC2_SHIFT;
		break;
	case ID_S5M8752_SDC3:
		enable_reg = S5M8752_ONOFF3;
		shift = S5M8752_SDC3_SHIFT;
		break;
	case ID_S5M8752_SDC4:
		enable_reg = S5M8752_ONOFF3;
		shift = S5M8752_SDC4_SHIFT;
		break;
	case ID_S5M8752_LDO_KA:
		enable_reg = S5M8752_ONOFF2;
		shift = S5M8752_LDO_KA_SHIFT;
		break;
	case ID_S5M8752_LDO1:
		enable_reg = S5M8752_ONOFF2;
		shift = S5M8752_LDO1_SHIFT;
		break;
	case ID_S5M8752_LDO2:
		enable_reg = S5M8752_ONOFF2;
		shift = S5M8752_LDO2_SHIFT;
		break;
	case ID_S5M8752_LDO3:
		enable_reg = S5M8752_ONOFF2;
		shift = S5M8752_LDO3_SHIFT;
		break;
	case ID_S5M8752_LDO4:
		enable_reg = S5M8752_ONOFF2;
		shift = S5M8752_LDO4_SHIFT;
		break;
	case ID_S5M8752_LDO5:
		enable_reg = S5M8752_ONOFF2;
		shift = S5M8752_LDO5_SHIFT;
		break;
	case ID_S5M8752_LDO6:
		enable_reg = S5M8752_ONOFF3;
		shift = S5M8752_LDO6_SHIFT;
		return;
//		break;
	case ID_S5M8752_LDO7:
		enable_reg = S5M8752_ONOFF3;
		shift = S5M8752_LDO7_SHIFT;
		break;
	default:
		return -EINVAL;
	}

	ret = s5m8752_reg_read(s5m8752, enable_reg, &val);
	if (ret)
		goto out;

	ret = val & (1 << shift);
out:
	return ret;
}

/******************************************************************************
 * This function can directly control the status of S5M8752 regulator OnOff.
 ******************************************************************************/
int s5m8752_regulator_onoff_direct(int regu, int onoff)
{
	struct s5m8752 *s5m8752 = s5m8752_p;
	uint8_t reg;
	int shift;

	switch (regu) {
	case S5M8752_SDC1:
		shift = S5M8752_SDC1_SHIFT;
		reg = S5M8752_ONOFF3;
		break;
	case S5M8752_SDC2_1:
	case S5M8752_SDC2_2:
	case S5M8752_SDC2_3:
	case S5M8752_SDC2_4:
		shift = S5M8752_SDC2_SHIFT;
		reg = S5M8752_ONOFF3;
		break;
	case S5M8752_SDC3_1:
	case S5M8752_SDC3_2:
	case S5M8752_SDC3_3:
	case S5M8752_SDC3_4:
		shift = S5M8752_SDC3_SHIFT;
		reg = S5M8752_ONOFF3;
		break;
	case S5M8752_SDC4:
		shift = S5M8752_SDC4_SHIFT;
		reg = S5M8752_ONOFF3;
		break;
	case S5M8752_LDO_KA:
		shift = S5M8752_LDO_KA_SHIFT;
		reg = S5M8752_ONOFF2;
		break;
	case S5M8752_LDO1:
		shift = S5M8752_LDO1_SHIFT;
		reg = S5M8752_ONOFF2;
		break;
	case S5M8752_LDO2:
		shift = S5M8752_LDO2_SHIFT;
		reg = S5M8752_ONOFF2;
		break;
	case S5M8752_LDO3:
		shift = S5M8752_LDO3_SHIFT;
		reg = S5M8752_ONOFF2;
		break;
	case S5M8752_LDO4:
		shift = S5M8752_LDO4_SHIFT;
		reg = S5M8752_ONOFF2;
		break;
	case S5M8752_LDO5:
		shift = S5M8752_LDO5_SHIFT;
		reg = S5M8752_ONOFF2;
		break;
	case S5M8752_LDO6:
		shift = S5M8752_LDO6_SHIFT;
		reg = S5M8752_ONOFF2;
		break;
	case S5M8752_LDO7:
		shift = S5M8752_LDO7_SHIFT;
		reg = S5M8752_ONOFF2;
		break;
	default:
		return -EINVAL;
	}

	if (onoff)
		s5m8752_set_bits(s5m8752, reg, 0x1 << shift);
	else
		s5m8752_clear_bits(s5m8752, reg, 0x1 << shift);

	return 0;
}
EXPORT_SYMBOL_GPL(s5m8752_regulator_onoff_direct);

/******************************************************************************
 * This function can directly control the voltage level of S5M8752 regulator.
 ******************************************************************************/
int s5m8752_regulator_voltage_direct(int regu, int uV)
{
	struct s5m8752 *s5m8752 = s5m8752_p;
	uint8_t reg, val, mV;
	int mask, info_num, ret, voltage = uV / 1000;

	switch (regu) {
	case S5M8752_SDC1:
		info_num = ID_S5M8752_SDC1;
		mask = S5M8752_SDC1_VOL_MASK;
		reg = S5M8752_SDC1_VOL;
		break;
	case S5M8752_SDC2_1:
		info_num = ID_S5M8752_SDC2;
		mask = S5M8752_SDC2_VOL_MASK;
		reg = S5M8752_SDC2_DVS1;
		break;
	case S5M8752_SDC2_2:
		info_num = ID_S5M8752_SDC2;
		mask = S5M8752_SDC2_VOL_MASK;
		reg = S5M8752_SDC2_DVS2;
		break;
	case S5M8752_SDC2_3:
		info_num = ID_S5M8752_SDC2;
		mask = S5M8752_SDC2_VOL_MASK;
		reg = S5M8752_SDC2_DVS3;
		break;
	case S5M8752_SDC2_4:
		info_num = ID_S5M8752_SDC2;
		mask = S5M8752_SDC2_VOL_MASK;
		reg = S5M8752_SDC2_DVS4;
		break;
	case S5M8752_SDC3_1:
		info_num = ID_S5M8752_SDC3;
		mask = S5M8752_SDC3_VOL_MASK;
		reg = S5M8752_SDC3_DVS1;
		break;
	case S5M8752_SDC3_2:
		info_num = ID_S5M8752_SDC3;
		mask = S5M8752_SDC3_VOL_MASK;
		reg = S5M8752_SDC3_DVS2;
		break;
	case S5M8752_SDC3_3:
		info_num = ID_S5M8752_SDC3;
		mask = S5M8752_SDC3_VOL_MASK;
		reg = S5M8752_SDC3_DVS3;
		break;
	case S5M8752_SDC3_4:
		info_num = ID_S5M8752_SDC3;
		mask = S5M8752_SDC3_VOL_MASK;
		reg = S5M8752_SDC3_DVS4;
		break;
	case S5M8752_SDC4:
		info_num = ID_S5M8752_SDC4;
		mask = S5M8752_SDC4_VOL_MASK;
		reg = S5M8752_SDC4_VOL;
		break;
	case S5M8752_LDO_KA:
		info_num = ID_S5M8752_LDO_KA;
		mask = S5M8752_SDC4_VOL_MASK;
		mask = S5M8752_LDO_KA_VOL_MASK;
		reg = S5M8752_LDO_KA_SET;
		break;
	case S5M8752_LDO1:
		info_num = ID_S5M8752_LDO1;
		mask = S5M8752_LDO1_VOL_MASK;
		reg = S5M8752_LDO1_SET;
		break;
	case S5M8752_LDO2:
		info_num = ID_S5M8752_LDO2;
		mask = S5M8752_LDO2_VOL_MASK;
		reg = S5M8752_LDO2_SET;
		break;
	case S5M8752_LDO3:
		info_num = ID_S5M8752_LDO3;
		mask = S5M8752_LDO3_VOL_MASK;
		reg = S5M8752_LDO3_SET;
		break;
	case S5M8752_LDO4:
		info_num = ID_S5M8752_LDO4;
		mask = S5M8752_LDO4_VOL_MASK;
		reg = S5M8752_LDO4_SET;
		break;
	case S5M8752_LDO5:
		info_num = ID_S5M8752_LDO5;
		mask = S5M8752_LDO5_VOL_MASK;
		reg = S5M8752_LDO5_SET;
		break;
	case S5M8752_LDO6:
		info_num = ID_S5M8752_LDO6;
		mask = S5M8752_LDO6_VOL_MASK;
		reg = S5M8752_LDO6_SET;
		break;
	case S5M8752_LDO7:
		info_num = ID_S5M8752_LDO7;
		mask = S5M8752_LDO7_VOL_MASK;
		reg = S5M8752_LDO7_SET;
		break;
	default:
		return -EINVAL;
	}

	if (voltage < info[info_num].min_vol ||
					voltage > info[info_num].max_vol)
		return -EINVAL;

	mV = s5m8752_mvolts_to_val(voltage, info[info_num].min_vol,
						info[info_num].step_vol);

	ret = s5m8752_reg_read(s5m8752, reg, &val);
	if (ret)
		goto out;

	val &= ~mask;
	ret = s5m8752_reg_write(s5m8752, reg, val | mV);
out:
	return ret;
}
EXPORT_SYMBOL_GPL(s5m8752_regulator_voltage_direct);

static struct regulator_ops s5m8752_regulator_ops = {
	.set_voltage = s5m8752_regulator_set_voltage,
	.get_voltage = s5m8752_regulator_get_voltage,
	.list_voltage = s5m8752_regulator_list_voltage,
	.enable = s5m8752_regulator_enable,
	.disable = s5m8752_regulator_disable,
	.is_enabled = s5m8752_regulator_is_enabled,
	.set_suspend_enable = s5m8752_regulator_set_suspend_enable,
	.set_suspend_disable = s5m8752_regulator_set_suspend_disable,
};

static __devinit int s5m8752_regulator_probe(struct platform_device *pdev)
{
	struct s5m8752 *s5m8752 = dev_get_drvdata(pdev->dev.parent);
	struct s5m8752_pdata *pdata = s5m8752->dev->platform_data;
	int id = pdev->id % ARRAY_SIZE(pdata->regulator);
	struct s5m8752_regulator *regulator;
	struct resource *res;
	int ret;


	if (pdata == NULL || pdata->regulator[id] == NULL)
		return -ENODEV;

	regulator = kzalloc(sizeof(struct s5m8752_regulator), GFP_KERNEL);
	if (regulator == NULL) {
		dev_err(&pdev->dev, "Unable to allocate private data\n");
		return -ENOMEM;
	}

	regulator->s5m8752 = s5m8752;

	res = platform_get_resource(pdev, IORESOURCE_IO, 0);
	if (res == NULL) {
		dev_err(&pdev->dev, "No I/O resource\n");
		ret = -EINVAL;
		goto err;
	}

	regulator->desc.name = regulator->name;
	regulator->desc.id = id;
	regulator->desc.type = REGULATOR_VOLTAGE;
	regulator->desc.ops = &s5m8752_regulator_ops;
	regulator->desc.owner = THIS_MODULE;

	regulator->regul = regulator_register(&regulator->desc, &pdev->dev,
			pdata->regulator[id], regulator);

	if (IS_ERR(regulator->regul)) {
		ret = PTR_ERR(regulator->regul);
		dev_err(s5m8752->dev, "Failed to register Regulator%d: %d\n",
			id + 1, ret);
		goto err;
	}

	platform_set_drvdata(pdev, regulator);

	s5m8752_p = s5m8752;
	

	return 0;

err:
	kfree(regulator);
	return ret;
}

static __devexit int s5m8752_regulator_remove(struct platform_device *pdev)
{
	struct s5m8752_regulator *regulator = platform_get_drvdata(pdev);

	regulator_unregister(regulator->regul);
	kfree(regulator);

	s5m8752_p = NULL;

	return 0;
}

static struct platform_driver s5m8752_regulator_driver = {
	.probe = s5m8752_regulator_probe,
	.remove = __devexit_p(s5m8752_regulator_remove),
	.driver		= {
		.name	= "s5m8752-regulator",
		.owner	= THIS_MODULE,
	},
};

static int __init s5m8752_regulator_init(void)
{
	int ret;

	ret = platform_driver_register(&s5m8752_regulator_driver);
	if (ret != 0)
		pr_err("Failed to register S5M8752 Regulator driver: %d\n",
									ret);
	return ret;
}
module_init(s5m8752_regulator_init);

static void __exit s5m8752_regulator_exit(void)
{
	platform_driver_unregister(&s5m8752_regulator_driver);
}
module_exit(s5m8752_regulator_exit);

/* Module Information */
MODULE_DESCRIPTION("S5M8752 Regulator driver");
MODULE_AUTHOR("Jongbin, Won <jongbin.won@samsung.com>");
MODULE_LICENSE("GPL");
