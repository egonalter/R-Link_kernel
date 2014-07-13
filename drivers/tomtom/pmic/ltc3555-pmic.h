/* ltc3555-pmic.h
 *
 * Control driver for LTC3555 PMIC.
 *
 * Copyright (C) 2006 TomTom BV <http://www.tomtom.com/>
 * Authors: Rogier Stam <rogier.stam@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef DRIVERS_I2C_CHIPS_LTC3555_PMIC_H
#define DRIVERS_I2C_CHIPS_LTC3555_PMIC_H

enum ltc3555_pmic_state
{
	LTC3555_STATE_NONE,
	LTC3555_STATE_CLA,
	LTC3555_STATE_USB,
	LTC3555_STATE_SUSPEND,
};

#define ltc3555_set_sw2_voltage( client, voltage )		(ltc3555_set_voltage( (client), (voltage), ltc3555_sw2_volttable ))
#define ltc3555_set_sw3_voltage( client, voltage )		(ltc3555_set_voltage( (client), (voltage), ltc3555_sw3_volttable ))
#define ltc3555_set_sw2_volt_from_freq( client, freq, pdata )	(ltc3555_set_sw2_voltage( (client), ltc3555_get_match_volt( (freq), (pdata)->sw2.power_setting ) ))
#define ltc3555_set_sw3_volt_from_freq( client, freq, pdata )	(ltc3555_set_sw3_voltage( (client), ltc3555_get_match_volt( (freq), (pdata)->sw3.power_setting ) ))
#define ltc3555_get_sw2_voltage( ) )				(ltc3555_get_voltage( ltc3555_sw2_volttable ))
#define ltc3555_get_sw3_voltage( ) )				(ltc3555_get_voltage( ltc3555_sw3_volttable ))

#define LTC3555_VOLTTBL_SIZE            16		/* Maximum number of entries in the table. */

/* Register bit definitions. */
#define LTC3555_USB_POWER_SUPPLY_IDX	0
#define LTC3555_CLA_POWER_SUPPLY_IDX	1
#endif
