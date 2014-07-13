/*
 * power.h  --  S5M8752 Advanced PMIC with AUDIO Codec
 *
 * Copyright 2010 Samsung Electronics.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#ifndef __LINUX_S5M8752_POWER_H_
#define __LINUX_S5M8752_POWER_H_

#define ID_S5M8752_POWER			12

#define S5M8752_PWR_USB				0
#define S5M8752_PWR_WALL			1

/*
 * Charger TEST1 (0xC4)
 */
#define S5M8752_CHG_THERMRENB			0x80
#define S5M8752_CHG_CHGENB			0x40
#define S5M8752_CHG_OVTEMPENB			0x20
#define S5M8752_CHG_NTCENB			0x10
#define S5M8752_CHG_OSCENB			0x08
#define S5M8752_CHG_CLK				0x04
#define S5M8752_CHG_EOCT			0x02
#define S5M8752_CHG_PCHGT			0x01

/*
 * Charger TEST2 (0xC5)
 */
#define S5M8752_CHG_LIMIENB_MASK		0x01

/*
 * Charger TEST3 (0xD6)
 */
#define S5M8752_CHG_STATE_OFF			0
#define S5M8752_CHG_STATE_ON			1

#define S5M8752_CHG_STATUS_MASK			0x01

/*
 * Charger TEST4 (0xD7)
 */
#define S5M8752_CHG_LIM_FLAG_MASK		0x10
#define S5M8752_CHG_LIM_FLAG_SHIFT		4
#define S5M8752_CHG_CHGDET_MASK			0x08
#define S5M8752_CHG_CHGDET_SHIFT		3
#define S5M8752_CHG_OVP_MASK			0x04
#define S5M8752_CHG_OVP_SHIFT			2
#define S5M8752_CHG_VIN_CHK_MASK		0x02
#define S5M8752_CHG_VIN_CHK_SHIFT		1
#define S5M8752_CHG_UVLO_MASK			0x01
#define S5M8752_CHG_UVLO_SHIFT			0

#define S5M8752_EXT_PWR_DETECTED		1
#define S5M8752_EXT_PWR_NOT_DETECTED		0

/*
 * Charger TEST5 (0xD8)
 */
#define S5M8752_CHG_STATE_TRICKLE		0
#define S5M8752_CHG_STATE_FAST			1

#define S5M8752_CHG_EOC_MASK			0x08
#define S5M8752_CHG_EOC_SHIFT			3
#define S5M8752_CHG_UNDER_RC_MASK		0x04
#define S5M8752_CHG_UNDER_RC_SHIFT		2
#define S5M8752_CHG_VM_FLAG_MASK		0x02
#define S5M8752_CHG_VM_FLAG_SHIFT		1
#define S5M8752_CHG_TYPE_MASK			0x01
#define S5M8752_CHG_TYPE_SHIFT			0

#endif /* __LINUX_S5M8752_POWER_H_ */
