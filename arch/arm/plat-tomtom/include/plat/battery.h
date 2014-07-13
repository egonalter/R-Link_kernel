#ifndef __INCLUDE_LINUX_TTBATTERY_H__
#define __INCLUDE_LINUX_TTBATTERY_H__

#define	BATT_POLL_AVG_COUNT	50		/* Number of samples per average value. */
#define BATT_POLL_INTERVAL	HZ		/* In jiffies, once a second */
#define TTBATTERY_DRIVER	"battery"


#define	INIT_ATOMIC_ADC_VALUE( val )	\
{\
	.lock			= SPIN_LOCK_UNLOCKED,\
	.value			= val,\
}

struct atomic_adc_value
{
	spinlock_t		lock;
	u32			value;
};

typedef struct 
{
        u32 vbatt;
        u32 remaining_capacity;
        u32 remaining_time;
} batt_discharge_lut_t;

typedef struct
{
        u32 current_current;
        batt_discharge_lut_t *lut;
	u8 dclut;
} batt_discharge_curve_t;


struct ttbatt_info
{
	struct
	{
		u32		battery_voltage_resistor_1;
		u32		battery_voltage_resistor_2;
		u32		charge_current_resistor;
		u32		reference_voltage;
		u32		accuracy;
	} adc;

	struct
	{
		batt_discharge_curve_t	*discharge_curves;
		u8			 curves;
		
		/* Taken from a reference battery */
		u32 Vmax; 
		u32 Vmin; 
		/* Taken from battery that actually goes in the unit;
			measured in the factory and put in the dts */
		u32 Vmaxf; 
		u32 Vminf; 
		u32 VminEXT;
		u32 VmaxEXT;
		u32 VminADC;
		u32 VmaxADC;
		u32 IminEXT;
		u32 ImaxEXT;
		u32 IminADC;
		u32 ImaxADC;
/*
		u32		charge_full_design;
		u32		charge_empty_design;
		u32		fast_charge_current_limit_high;
		u32		fast_charge_current_limit_low;
		u32		charge_volt_75_percent;
		u32		charge_volt_100_percent;
		u32		charge_volt_critical_high;
		u32		charge_volt_critical_low;*/
	} battery;
};

struct ttbattery_platform_info
{
	int (*platform_init)( struct platform_device *pdevice );
	void (*platform_remove)( struct platform_device *pdevice );
	int (*platform_suspend)(struct platform_device *pdevice);
	int (*platform_resume)(struct platform_device *pdevice);

	u32 (*get_charge_current_adc)( void );
	u32 (*get_battery_voltage_adc)( void );

	struct ttbatt_info      *battery_info;
};
#endif /* __INCLUDE_LINUX_TTBATTERY_H__ */
