/* ltc3555-pmic.c
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
#include <linux/cdev.h>
#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/power_supply.h>
#include <linux/cpufreq.h>
#include <linux/ltc3555-pmic.h>
#include <asm/uaccess.h>
#include <mach/gpio.h>
#include <mach/hardware.h>
#include <plat/usbmode.h>
#include "ltc3555-pmic.h"

#define RAMP_UP_T_DVS   (130)

/* readback cache for what's in chip itself */
static uint8_t			ltc3555_register_cache[2];
static uint32_t			ltc3555_sw2_volttable[LTC3555_VOLTTBL_SIZE];
static uint32_t			ltc3555_sw3_volttable[LTC3555_VOLTTBL_SIZE];
static uint8_t			write_ltc3555[2]={0, 0};
struct semaphore		ltc3555_register_cache_lock;
static enum ltc3555_pmic_state	ltc3555_current_state=LTC3555_STATE_NONE;	/* State of the charger. */
static spinlock_t		ltc3555_pmic_state_lock=SPIN_LOCK_UNLOCKED;	/* Lock of the state var. */

#if defined(CONFIG_TOMTOM_PMIC_LTC3555_CPU_FREQ)
static void ltc3555_init_cpufreq(struct i2c_client *client);
#endif


static const struct i2c_device_id ltc3555_id[] = {
	{ LTC3555_DEVNAME, 9, },
	{ }
};

/* This routine fills the specified table with the supported voltages, as determined by */
/* the resistor dividers for the feedback pins R1 and R2. */
static void ltc3555_build_volttable( uint32_t *table, uint32_t r1, uint32_t r2 )
{
	uint32_t	count=0;

	/* Calculate the table, using: Vout=Vbx * (R1/R2 + 1) */
	/* Where Vbx is 425mV + 25mV * count, and count is max*/
	/* 16 entries. */
	for( count=0; count < LTC3555_VOLTTBL_SIZE; count++ )
	{
		table[count]=425 + 25 * count;
		table[count]+=(table[count] * r1)/r2;
	}
	return;
}

/* Find the closest matching voltage to the specified voltage, and return */
/* it's index in the table or -1 if it can't be found. */
static int ltc3555_get_closest_voltage_idx( uint32_t voltage, uint32_t *table )
{
	int		count=0;
	uint32_t	low_diff=0xFFFFFFFF;
	int		low_diff_idx=-1;
	uint32_t	diff;
	uint32_t	deviation=((table[LTC3555_VOLTTBL_SIZE - 1]*1000/800) * 125)/1000;
	uint32_t	min_voltage=table[0] - deviation;
	uint32_t	max_voltage=table[LTC3555_VOLTTBL_SIZE - 1] + deviation;

	/* Deviation is calculated by taking the multiplication factor from R1/R2 */
	/* and multiplying this by half the step (step=25mV). This gives us the */
	/* amount of deviation per step. */
	/* Check if the voltage is legal. */

	if( (voltage  < min_voltage) || (voltage > max_voltage) )
		return -1;

	/* Get nearest match. */
	for( count=0; count < LTC3555_VOLTTBL_SIZE; count++ )
	{
		if( table[count] > voltage )
			diff=table[count] - voltage;
		else
			diff=voltage - table[count];

		if( diff < low_diff )
		{
			low_diff=diff;
			low_diff_idx=count;
		}
	}
	return low_diff_idx;
}

static int ltc3555_i2c_commit( struct i2c_client * client, int lock )
{
	int		rc;
	
	/* Get the lock. This before someone else modifies something. */
	if (lock)
		down( &ltc3555_register_cache_lock );

	pr_debug( LTC3555_DEVNAME 
		" Regulator 2: 0x%x, Regulator 3: 0x%x\n", (write_ltc3555[0]&0xf0)>>4, write_ltc3555[0]&0x0f);
	pr_debug( LTC3555_DEVNAME 
		" Battery charge: %d\n", !((write_ltc3555[1]&0x80)>>7));

        /* Send the command over I2C. If it passes, we will save the written value. */
	rc=i2c_master_send( client, write_ltc3555, sizeof( write_ltc3555 ) );
	if( rc < 0 )
	{
		if (lock)
			up( &ltc3555_register_cache_lock );
		return rc;
	}

	/* It passed. Save in the cache. */
	ltc3555_register_cache[0] = write_ltc3555[0];
	ltc3555_register_cache[1] = write_ltc3555[1];

	if (lock)
		up( &ltc3555_register_cache_lock );
	return 0;	
}

static int ltc3555_i2c_change( struct i2c_client * client, uint16_t data, uint16_t mask, int lock )
{
	/* Get the lock. This before we modify anything. */
	if (lock)
		down( &ltc3555_register_cache_lock );

	/* Now its safe to prepare & modify. */
	write_ltc3555[0]=write_ltc3555[0] & ~((uint8_t) (mask & 0x00FF));
	write_ltc3555[1]=write_ltc3555[1] & ~((uint8_t) (mask >> 8));
	write_ltc3555[0]|=(uint8_t) ((data & mask) & 0x00FF);
	write_ltc3555[1]|=(uint8_t) ((data & mask) >> 8);
	
	if (lock)
		up( &ltc3555_register_cache_lock );
	return 0;
}

/* Mask has the bits that should change set to 1, the rest set to 0. Data specifies the value */
/* of these bits to write. */
static int ltc3555_i2c_write( struct i2c_client * client, uint16_t data, uint16_t mask )
{
	int		rc;

	/* do all in using single lock */
	down( &ltc3555_register_cache_lock );
	ltc3555_i2c_change(client, data, mask, 0);
	rc = ltc3555_i2c_commit(client, 0);
	up( &ltc3555_register_cache_lock );

	return rc;
}

static int ltc3555_set_voltage( struct i2c_client *client, uint32_t voltage, uint32_t *table )
{
	int		shift=(table == ltc3555_sw2_volttable ? 4 : 0);
	int		rc;

	/* Get the nearest voltage, and if found put it into the prepared message. */
	rc=ltc3555_get_closest_voltage_idx( voltage, table );
	if( rc < 0 ) return rc;

	/* Write to the power chip. */
	rc=ltc3555_i2c_change( client, (uint16_t) (rc << shift), (uint16_t) (0xF << shift), 1);
	if( rc < 0 ) return rc;
	return 0;
}

static int ltc3555_set_swmode( struct i2c_client *client, enum ltc3555_swmode swmode )
{
	return ltc3555_i2c_change( client, ((uint16_t) swmode) << 13, 0x0003 << 13, 1 );
}

static int ltc3555_set_battery_charge( struct i2c_client *client, int enable )
{
	return ltc3555_i2c_change( client, (enable ? 0x0000 : 0x8000), 0x8000, 1 );
}

static uint32_t ltc3555_get_match_volt( uint32_t frequency, struct ltc3555_volt *power_setting )
{
	int	count=0;

	for( count=0; (power_setting[count].frequency != 0) &&
	              (power_setting[count].voltage != 0); count++ )
	{
		if( frequency <= power_setting[count].frequency )
			return power_setting[count].voltage;
	}

	return 0;
}

/* For future use, when we are doing CPUFrequency scaling. */
static enum ltc3555_pmic_state	ltc3555_pmic_get_state( void )
{
	u32			flags;
	enum ltc3555_pmic_state	retval;

	spin_lock_irqsave( &ltc3555_pmic_state_lock, flags );
	retval=ltc3555_current_state;
	spin_unlock_irqrestore( &ltc3555_pmic_state_lock, flags );
	return retval;
}

static void ltc3555_pmic_set_state( enum ltc3555_pmic_state state )
{
	u32			flags;

	spin_lock_irqsave( &ltc3555_pmic_state_lock, flags );
	ltc3555_current_state=state;
	spin_unlock_irqrestore( &ltc3555_pmic_state_lock, flags );
}

enum power_supply_property	ltc3555_ac_properties[]=
{
	POWER_SUPPLY_PROP_PRESENT, POWER_SUPPLY_PROP_ONLINE,
};

static int ltc3555_pwr_get_claproperty( struct power_supply *psy, enum power_supply_property psp, union power_supply_propval *val )
{
	switch( psp )
	{
		case POWER_SUPPLY_PROP_PRESENT :
		case POWER_SUPPLY_PROP_ONLINE :
			if( (psy->type == POWER_SUPPLY_TYPE_MAINS) && (ltc3555_pmic_get_state( ) == LTC3555_STATE_CLA) )
				val->intval=1;
			else
				val->intval=0;
			break;

		default :
			/* Not supported property. */
			return -EINVAL;
	}
	return 0;
}

static int ltc3555_pwr_get_usbproperty( struct power_supply *psy, enum power_supply_property psp, union power_supply_propval *val )
{
	switch( psp )
	{
		case POWER_SUPPLY_PROP_PRESENT :
		case POWER_SUPPLY_PROP_ONLINE :
			if( (psy->type == POWER_SUPPLY_TYPE_USB) && (ltc3555_pmic_get_state( ) == LTC3555_STATE_USB) )
				val->intval=1;
			else
				val->intval=0;
			break;

		default :
			/* Not supported property. */
			return -EINVAL;
	}
	return 0;
}

struct power_supply	ltc3555_power[]=
{
	{
		.name		= "usb",
		.type 		= POWER_SUPPLY_TYPE_USB,
		.properties	= ltc3555_ac_properties,
		.num_properties	= ARRAY_SIZE( ltc3555_ac_properties ),
		.get_property	= ltc3555_pwr_get_usbproperty,
//		.use_for_apm	= 1,
	},
	{
		.name		= "mains",
		.type		= POWER_SUPPLY_TYPE_MAINS,
		.properties	= ltc3555_ac_properties,
		.num_properties	= ARRAY_SIZE( ltc3555_ac_properties ),
		.get_property	= ltc3555_pwr_get_claproperty,
//		.use_for_apm	= 1,
	},
};

static int ltc3555_set_state_suspend( struct i2c_client *client )
{
	struct ltc3555_platform		*pdata=(struct ltc3555_platform *) client->dev.platform_data;
	struct power_supply		*psu=(struct power_supply *) i2c_get_clientdata( client );

	/* Set the LTC3555 to 500mA mode. This will allow the unit to charge while in suspend. */
	gpio_direction_output( pdata->wall_pwr_pin.pin, pdata->wall_pwr_pin.is_inverted );
	ltc3555_set_swmode( client, pdata->swmode.acadapt );
	ltc3555_set_battery_charge( client, 1 );
	ltc3555_pmic_set_state( LTC3555_STATE_SUSPEND );

	/* All PSUs are affected, so inform userspace. */
	power_supply_changed( &psu[LTC3555_CLA_POWER_SUPPLY_IDX] );
	power_supply_changed( &psu[LTC3555_USB_POWER_SUPPLY_IDX] );
	return ltc3555_i2c_commit( client, 1 );
}

static int ltc3555_state_change( struct i2c_client *client, USB_STATE previous_state, USB_STATE current_state )
{
	struct ltc3555_platform		*pdata=(struct ltc3555_platform *) client->dev.platform_data;
	struct power_supply		*psu=(struct power_supply *) i2c_get_clientdata( client );
	int				retval;

	switch (current_state)
	{
		case USB_STATE_IDLE :
		default:
			/* nothing connected, or in transition between states, disable charging */
			printk (KERN_INFO LTC3555_DEVNAME
					" USB_IDLE => Drawing 500mA\n");
			gpio_direction_output( pdata->wall_pwr_pin.pin, pdata->wall_pwr_pin.is_inverted );
			ltc3555_set_swmode( client, pdata->swmode.batt );
			ltc3555_set_battery_charge( client, 0 );
			ltc3555_pmic_set_state( LTC3555_STATE_NONE );
			break;
		case USB_STATE_HOST:
		case USB_STATE_CLA:
			/* CLA or wall pwr connected, enable 1A charging. */
			printk (KERN_INFO LTC3555_DEVNAME 
					" USB_HOST | CLA [%i] => Drawing 1A\n", current_state);
			gpio_direction_output( pdata->wall_pwr_pin.pin, !pdata->wall_pwr_pin.is_inverted );
			ltc3555_set_swmode( client, pdata->swmode.acadapt );
			ltc3555_set_battery_charge( client, 1 );
			ltc3555_pmic_set_state( LTC3555_STATE_CLA );
			break;
		case USB_STATE_DEVICE :
		case USB_STATE_DEVICE_WAIT :
			/* something connected, enable 500mA charging. */
			printk (KERN_INFO LTC3555_DEVNAME 
					" USB_DEVICE | DEVICE_WAIT [%i] => Drawing 500mA\n", current_state);
			gpio_direction_output( pdata->wall_pwr_pin.pin, pdata->wall_pwr_pin.is_inverted );
			ltc3555_set_swmode( client, pdata->swmode.acadapt );
			ltc3555_set_battery_charge( client, 1 );
			ltc3555_pmic_set_state( LTC3555_STATE_USB );
			break;
	}

	/* write changes to chip */
	retval=ltc3555_i2c_commit( client, 1 );
	if( retval == 0 )
	{
		/* Send power supply changed event. */
		if( previous_state == USB_STATE_INITIAL )
		{
			power_supply_changed( &psu[LTC3555_CLA_POWER_SUPPLY_IDX] );
			power_supply_changed( &psu[LTC3555_USB_POWER_SUPPLY_IDX] );
		}
		else
		{
			if( (previous_state == USB_STATE_HOST) || (current_state == USB_STATE_HOST) ||
			    (previous_state == USB_STATE_CLA) || (current_state == USB_STATE_CLA) )
				power_supply_changed( &psu[LTC3555_CLA_POWER_SUPPLY_IDX] );

			if( (previous_state == USB_STATE_DEVICE) || (current_state == USB_STATE_DEVICE) )
				power_supply_changed( &psu[LTC3555_USB_POWER_SUPPLY_IDX] );
		}
	}
	return retval;
}
	
/* This routine exists to change charging mode if the device is powered from USB. */
static void ltc3555_state_change_listener( USB_STATE previous_state, USB_STATE current_state, void* arg )
{
	ltc3555_state_change( (struct i2c_client *) arg, previous_state, current_state );
	return;
}

static int ltc3555_probe( struct i2c_client *client )
{
	struct ltc3555_platform		*pdata=(struct ltc3555_platform *) client->dev.platform_data;
	int				rc;
	uint16_t			ltc3555_reg;
	uint32_t			curr_cpufreq;
	uint32_t			curr_sw2volt;
	uint32_t			curr_sw3volt;
	struct clk			*cpu;
	USB_STATE			curr_usb_state;

	/* Signon. */
	printk( "LTC3555 Power Management IC I2C driver v1.0 (C) 2008 TomTom B.V.\n" );

	/* First build the voltage tables. This we need for later lookup. */
	ltc3555_build_volttable( ltc3555_sw2_volttable, pdata->sw2.R1, pdata->sw2.R2 );
	ltc3555_build_volttable( ltc3555_sw3_volttable, pdata->sw3.R1, pdata->sw3.R2 );

	/* Now initialize the semaphore. */
	init_MUTEX( &ltc3555_register_cache_lock );

	/* Now initialize the initial values for the PMIC. */
	/* Default values are 0. */
	memset( ltc3555_register_cache, 0, sizeof( ltc3555_register_cache ) );

	if (gpio_request(pdata->wall_pwr_pin.pin, "WALL_PWR")) {
		dev_err(&client->dev, "Can't request WALL_PWR pin\n");
		return -ENODEV;
	}

	/* Determine the current clock frequency. */
	cpu=clk_get( &client->dev, pdata->cpu_clk );
	if( cpu == NULL )
	{
		printk( KERN_ERR "LTC3555: Can't get current CPU frequency. Aborting.\n" );
		return -ENODEV;
	}

	curr_cpufreq=clk_get_rate( cpu ) / 1000;
	pr_info("LTC3555: Current cpu frequency is %d\n", curr_cpufreq);

	/* Determine the current matching voltages. */
	curr_sw2volt=ltc3555_get_match_volt( curr_cpufreq, pdata->sw2.power_setting );
	curr_sw3volt=ltc3555_get_match_volt( curr_cpufreq, pdata->sw3.power_setting );

	/* Set the default state of all pins aside from voltage. We'll assume no USB charging, */
	/* all regulators enabled, and 100mA limit set. */
	ltc3555_reg = pdata->initial_state << 8;

	/* Since we don't want any invalid values we will now write the whole register */
	/* in one go. */
	rc=ltc3555_get_closest_voltage_idx( curr_sw2volt, ltc3555_sw2_volttable );
	if( rc < 0 )
	{
		printk( KERN_ERR "LTC3555: Can't get current SW2 voltage. Aborting.\n" );
		return -ENODEV;
	}

	/* Set the SW2 voltage. */
	ltc3555_reg|=(((uint16_t) rc) & 0x0F) << 4;

	/* Get the SW3 voltage. */
	rc=ltc3555_get_closest_voltage_idx( curr_sw3volt, ltc3555_sw3_volttable );
	if( rc < 0 )
	{
		printk( KERN_ERR "LTC3555: Can't get current SW3 voltage. Aborting.\n" );
		return -ENODEV;
	}

	/* Set the SW3 voltage. */
	ltc3555_reg|=(((uint16_t) rc) & 0x0F) << 0;

	/* We're done determining and registring. Write the whole word and update the I2C chip. */
	/* This means from now on we have a known state. We can then determine the real state */
	/* and modify it. */
	rc=ltc3555_i2c_write( client, ltc3555_reg, 0xFFFF );
	if( rc < 0 )
	{
		printk( KERN_ERR "LTC3555: Error writing initial value to PMIC. Aborting.\n" );
		return -ENODEV;
	}

	/* Register the power supply info. */
	if( power_supply_register( &client->dev, &(ltc3555_power[LTC3555_USB_POWER_SUPPLY_IDX]) ) )
	{
		printk( KERN_ERR "LTC3555: Can't register usb power supply. Aborting.\n" );
		return -ENODEV;
	}

	if( power_supply_register( &client->dev, &(ltc3555_power[LTC3555_CLA_POWER_SUPPLY_IDX]) ) )
	{
		printk( KERN_ERR "LTC3555: Can't register cla power supply. Aborting.\n" );
		return -ENODEV;
	}

	/* Save pointer to power supply info. */
	i2c_set_clientdata( client, ltc3555_power );

#if defined(CONFIG_TOMTOM_PMIC_LTC3555_CPU_FREQ)
	/* Configure the CPUFREQ notifiers */
	ltc3555_init_cpufreq(client);
#endif

	/* Register the usbmode statechange listener. This will inform us when to change the */
	/* PMIC's settings. */
	if( add_usb_state_change_listener( ltc3555_state_change_listener, client, &curr_usb_state ) != 0 )
	{
		printk( KERN_ERR "LTC3555: Can't register USBMODE change state listener. Aborting.\n" );
		return -ENODEV;
	}

	/* Determine and adjust the real state. */
	if( ltc3555_state_change( client, USB_STATE_INITIAL, curr_usb_state ) != 0 )
	{
		printk( KERN_ERR "LTC3555: Can't change to current state. Aborting.\n" );
		return -EFAULT;
	}


	/* Done initializing. */
	return 0;
}

static int ltc3555_remove( struct i2c_client *c )
{
	/* First unregister the I2C client's members. */
	if( remove_usb_state_change_listener( ltc3555_state_change_listener, NULL ) != 0 )
		printk( KERN_ERR "LTC3555: Couldn't unregister USBMODE change state listener!\n" );

	power_supply_unregister( &(ltc3555_power[LTC3555_USB_POWER_SUPPLY_IDX]) );
	power_supply_unregister( &(ltc3555_power[LTC3555_CLA_POWER_SUPPLY_IDX]) );
	return 0;
}

#ifdef CONFIG_PM
static int ltc3555_suspend( struct i2c_client *client, pm_message_t state )
{
	struct ltc3555_platform		*pdata=(struct ltc3555_platform *) client->dev.platform_data;

	/* Remove the state change listener. This since we want to ensure we're NOT receiving a */
	/* usbmode event while our driver is in suspend. */
	if( remove_usb_state_change_listener( ltc3555_state_change_listener, NULL ) != 0 )
		printk( KERN_ERR "LTC3555: Couldn't unregister USBMODE change state listener!\n" );

	if( ltc3555_set_state_suspend( client ) != 0 )
		printk( KERN_ERR "LTC3555: Can't change state to suspend!\n" );
	
	/* Upon suspend we should make sure the right Switching Regulator Mode is set. */
	ltc3555_set_swmode( client, pdata->swmode.suspend );
	ltc3555_i2c_commit( client, 1 );
	return 0;
}

static int ltc3555_resume(struct i2c_client *client)
{
	struct ltc3555_platform		*pdata=(struct ltc3555_platform *) client->dev.platform_data;
	struct clk			*cpu;
	USB_STATE			curr_usb_state;
	uint32_t			curr_cpufreq;

	cpu=clk_get( &client->dev, pdata->cpu_clk );
	if( cpu == NULL )
	{
		printk( KERN_ERR "LTC3555: Can't get current CPU frequency during resume. Aborting.\n" );
		return -ENODEV;
	}

	curr_cpufreq=clk_get_rate( cpu );

	if( add_usb_state_change_listener( ltc3555_state_change_listener, client, &curr_usb_state ) != 0 )
	{
		printk( KERN_ERR "LTC3555: Can't register USBMODE change state listener. Aborting.\n" );
		return -ENODEV;
	}

	if( ltc3555_state_change( client, USB_STATE_INITIAL, curr_usb_state ) != 0 )
		printk( KERN_ERR "LTC3555: Can't change to current state. Aborting.\n" );

	/* Set the right voltages for the right frequencies. */
	ltc3555_set_sw2_volt_from_freq( client, curr_cpufreq, pdata );
	ltc3555_set_sw3_volt_from_freq( client, curr_cpufreq, pdata );
	ltc3555_i2c_commit( client, 1 );
	return 0;
}
#else
#define ltc3555_suspend		NULL
#define ltc3555_resume		NULL
#endif /* CONFIG_PM	*/

#if defined(CONFIG_TOMTOM_PMIC_LTC3555_CPU_FREQ)
static int ltc3555_freq_transition(struct notifier_block *nb, unsigned long val, void *data)
{
	struct ltc3555_notifiers	*notif = container_of(nb, struct ltc3555_notifiers, transition);
	struct i2c_client		*client = notif->client;
	struct ltc3555_platform		*pdata = client->dev.platform_data;
	struct cpufreq_freqs		*f = data;

	pr_info("ltc3555_freq_transition: going from %d to %d\n", f->old, f->new);

	switch( val )
	{
		case CPUFREQ_PRECHANGE :
			/* If we are going from a lower to a higher frequency, */
			/* set the voltage now, before any frequency change. */
			if( f->new > f->old )
			{
				ltc3555_set_sw2_volt_from_freq( client, f->new, pdata );
				ltc3555_set_sw3_volt_from_freq( client, f->new, pdata );
				ltc3555_i2c_commit( client , 1);
				/* Wait for the CPU to stabilize at the new, higher voltage... */
				udelay(RAMP_UP_T_DVS);
				
			}
			break;

		case CPUFREQ_POSTCHANGE :
			/* If we are going from a higher to a lower frequency,  */
			/* set the voltage now, after the frequency has changed.*/
			if( f->new < f->old )
			{
				ltc3555_set_sw2_volt_from_freq( client, f->new, pdata );
				ltc3555_set_sw3_volt_from_freq( client, f->new, pdata );
				ltc3555_i2c_commit( client, 1 );
			}
			break;

		/* Suspend and resume are pretty much the same, in the sense that the voltage */
		/* is set as the handler is called. However, since during resume the I2C is still */
		/* not available at this time, we do the resume part in the resume handler for this*/
		/* driver, not in this code. */
		case CPUFREQ_SUSPENDCHANGE :
			ltc3555_set_swmode( client, LTC3555_BURST );
			ltc3555_set_sw2_volt_from_freq( client, f->new, pdata );
			ltc3555_set_sw3_volt_from_freq( client, f->new, pdata );
			ltc3555_i2c_commit(client, 1);
			break;

	}
	return 0;
}

static int ltc3555_freq_policy (struct notifier_block *nb, unsigned long val, void *data)
{
	struct cpufreq_policy		*policy=data;
	struct ltc3555_notifiers	*notif = container_of(nb, struct ltc3555_notifiers, policy);
	struct i2c_client		*client = notif->client;
	struct ltc3555_platform		*pdata = client->dev.platform_data;
	unsigned int			min_freq=0xFFFFFFFF;
	unsigned int			max_freq=0;
	int				count;

	/* Check which are the highest/lowest frequencies we accept. */
	for( count=0; (pdata->sw2.power_setting[count].frequency != 0) && (pdata->sw2.power_setting[count].voltage != 0); count++ )
	{
		if( pdata->sw2.power_setting[count].frequency > max_freq )
			max_freq=pdata->sw2.power_setting[count].frequency;
		if( pdata->sw2.power_setting[count].frequency < min_freq )
			min_freq=pdata->sw2.power_setting[count].frequency;
	}
	for( count=0; (pdata->sw3.power_setting[count].frequency != 0) && (pdata->sw3.power_setting[count].voltage != 0); count++ )
	{
		if( pdata->sw3.power_setting[count].frequency > max_freq )
			max_freq=pdata->sw3.power_setting[count].frequency;
		if( pdata->sw3.power_setting[count].frequency < min_freq )
			min_freq=pdata->sw3.power_setting[count].frequency;
	}

	/* Lowest/highest frequencies are now known. */
	switch (val)
	{
		case CPUFREQ_ADJUST :
		case CPUFREQ_INCOMPATIBLE :
			if( policy->min < min_freq )
				policy->min=min_freq;
			if( policy->max > max_freq )
				policy->max=max_freq;
			break;

		case CPUFREQ_NOTIFY:
			/* Can't do much about it. Live with it. */
			break;
	}
	return 0;
}

static void ltc3555_init_cpufreq(struct i2c_client *client)
{
	struct ltc3555_platform *pdata = client->dev.platform_data;

	pdata->notifiers.client = client;
	pdata->notifiers.transition.notifier_call = ltc3555_freq_transition;
	pdata->notifiers.policy.notifier_call = ltc3555_freq_policy;

        if (cpufreq_register_notifier(&pdata->notifiers.transition, CPUFREQ_TRANSITION_NOTIFIER) ||
	    cpufreq_register_notifier(&pdata->notifiers.policy, CPUFREQ_POLICY_NOTIFIER))
                pr_err("ltc3555_init_cpufreq: Failed to setup cpufreq notifier\n");
}
#endif

static struct i2c_driver ltc3555_driver=
{
	.id	= -1,
	
        .probe	= ltc3555_probe,
        .remove	= ltc3555_remove,
	.suspend= ltc3555_suspend,
	.resume	= ltc3555_resume,
	.id_table = ltc3555_id,
        .driver = {
		.name		= LTC3555_DEVNAME,
		.owner		= THIS_MODULE,
	},
};

static int __init ltc3555_init(void)
{
	return i2c_add_driver( &ltc3555_driver );
}

static void __exit ltc3555_exit(void)
{
	i2c_del_driver( &ltc3555_driver );
	return;
}

MODULE_AUTHOR("Rogier Stam <rogier.stam@tomtom.com>");
MODULE_DESCRIPTION("Driver for I2C connected LTC3555 PM IC.");
MODULE_LICENSE("GPL");

module_init( ltc3555_init );
module_exit( ltc3555_exit );
