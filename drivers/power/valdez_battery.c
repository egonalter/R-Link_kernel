/* Battery calculation routines for Valdez. */

/* A ttbattery module exists and was supposed to be reused. This in itself is
 * a good idea, however the calculations are so generic that they become quite
 * cpu intense. Thas is besides the fact that Samsung battery driver can only
 * read voltage level. It does not even support current readings. So these
 * routines were specifically written to support samsung ADC readings, but may
 * be used by other modules as well. Documentation is inline as much as possible
 */

#include <linux/types.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <plat/fdt.h>
 
 
#define ADC_VOLTAGE_SCALING_DEFAULT	1129	/* Makes it scale to uV */
#define ADC_AVERAGING_NOOF_SAMPLES_2	4
#define ADC_AVERAGING_NOOF_SAMPLES	16 	/* make 2^ADC_AVERAGING_NOOF_SAMPLES_2 */
#define ADC_AVERAGING_MIN_DIFF		4000	/* uV */
#define ADC_LUT_ENTRIES			32

typedef struct 
{
        unsigned int voltage;		/* uV */
        unsigned int capacity;		/* O/oo (promille) */
} battery_discharge_lut_t;

typedef struct
{
	int				count_lut;
        battery_discharge_lut_t	*lut;
} battery_discharge_curve_t;


static int battery_average_first_time = 2;
static unsigned int battery_average_total = 0;
static unsigned int battery_average_reported = 0;
static unsigned int battery_capacity_reported = 0;
static battery_discharge_curve_t *battery_discharge_curve = NULL;
static int lut_index;
static int battery_readout_stall = 0;
static int prev_charging_state;
static int adc_voltage_scaling = ADC_VOLTAGE_SCALING_DEFAULT;
static int debug_level = 0;

/* 
 * sysfs debug show
 */
static ssize_t debug_show(struct device *dev,
			  struct device_attribute *attr,
			  char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%x\n", debug_level);
}

/* 
 * sysfs debug store
 */
static ssize_t debug_store(struct device *dev,
			   struct device_attribute *attr,
			   const char *buf,
			   size_t count)
{
	ssize_t ret = count;

	if (count > 2)
		return -EINVAL;

	if ((count == 2) && (buf[1] != '\n'))
		return -EINVAL;

	if (buf[0] == '0')
		debug_level = 0;
	else if (buf[0] == '1')
		debug_level = 1;
	else
		ret = -EINVAL;

	return ret;
}

DEVICE_ATTR(debug, S_IRUGO| S_IWUGO, debug_show, debug_store);

/* 
 * sysfs lut show
 */
static ssize_t lut_show(struct device *dev,
			  struct device_attribute *attr,
			  char *buf)
{
	battery_discharge_lut_t	*lut;
	int			room_left;
	int 			used;
	int 			i;
	
	room_left = PAGE_SIZE;
	used = 0;
	
	lut = battery_discharge_curve->lut;
	for (i = 0; i < battery_discharge_curve->count_lut; i++) {
		used += snprintf(&buf[used], room_left, "\nlut[%02d], voltage=%d (uV), capcacity=%4d (O/oo)",
			i, lut[i].voltage, lut[i].capacity);
		if (used < PAGE_SIZE) {
			room_left = PAGE_SIZE - used;
		}
		else {
			return PAGE_SIZE;			
		}
	}		
	
	used += snprintf(&buf[used], room_left, 
		"\n\nTo update write \"<index> <voltage> <capacity>\"\n"
		"\tindex is in range of 0..31, be sure not to create gaps!\n"
		"\tvoltage is in uV range of 3200000..4400000, use 0 to clear entry\n"
		"\tcapacity is in promille with range of 0..1000\n");
	if (used < PAGE_SIZE) {
		room_left = PAGE_SIZE - used;
	}
	else {
		return PAGE_SIZE;			
	}
	used += snprintf(&buf[used], room_left, 
		"\nTo write new table, clear old first. Make sure that voltage lowers per index\n"
		"and have capacity also lower per index. If index is cleared then everything\n"
		"from that index and forward will be cleared.\n");
	if (used < PAGE_SIZE) {
		room_left = PAGE_SIZE - used;
	}
	else {
		return PAGE_SIZE;			
	}
	
	return used;
}

/* 
 * sysfs lut store
 */
static ssize_t lut_store(struct device *dev,
			   struct device_attribute *attr,
			   const char *buf,
			   size_t count)
{
	battery_discharge_lut_t	*lut;
	int			index;
	int			program_index;
	int			program_voltage;
	int			program_capacity;
	int			i;
	
	index = 0;
	while ((index < count) && ((buf[index] < '0') || (buf[index] > '9'))) {
		index++;
	}
	if (index == count) {
		if (debug_level) {
			printk("NO NUMBER FOUND\n");
		}
		return -EINVAL;
	}
	program_index = simple_strtol(&buf[index], NULL, 10);
	if ((program_index < 0) || (program_index >= ADC_LUT_ENTRIES)) {
		if (debug_level) {
			printk("Invalid prorgram_index=%d\n", program_index);
		}
		return -EINVAL;
	}

	while ((index < count) && (buf[index] != ' ') && (buf[index] != '\t')) {
		index++;
	}
	while ((index < count) && ((buf[index] < '0') || (buf[index] > '9'))) {
		index++;
	}
	if (index == count) {
		if (debug_level) {
			printk("prorgram_index=%d, no more numbers\n", program_index);
		}
		return -EINVAL;
	}
	program_voltage = simple_strtol(&buf[index], NULL, 10);
	if ((program_voltage != 0) && 
	    ((program_voltage < 3000000) || (program_voltage > 4500000))) {
		if (debug_level) {
			printk("Invalid program_voltage=%d\n", program_voltage);
		}
		return -EINVAL;
	}
	
	while ((index < count) && (buf[index] != ' ') && (buf[index] != '\t')) {
		index++;
	}
	while ((index < count) && ((buf[index] < '0') || (buf[index] > '9'))) {
		index++;
	}
	if (index == count) {
		if (debug_level) {
			printk("program_voltage=%d, no more numbers\n", program_voltage);
		}
		return -EINVAL;
	}
	program_capacity = simple_strtol(&buf[index], NULL, 10);
	if ((program_capacity < 0) || (program_capacity > 1000)) {
		if (debug_level) {
			printk("Invalid program_capacity=%d\n", program_capacity);
		}
		return -EINVAL;
	}

	lut = battery_discharge_curve->lut;
	if (program_voltage == 0) {
		/* clear everything from this index and forward */
		if (program_index < battery_discharge_curve->count_lut) {
			i = program_index;
			while (i < battery_discharge_curve->count_lut) {
				lut[i].voltage = 0; 
				lut[i].capacity = 0;
				i++;
			}
			battery_discharge_curve->count_lut = program_index;
		}
	}
	else {
		i = program_index;
		do {
			lut[i].voltage = program_voltage; 
			lut[i].capacity = program_capacity;
			i--;
		} while (i >= battery_discharge_curve->count_lut);
			
		if (program_index >= battery_discharge_curve->count_lut) {
			battery_discharge_curve->count_lut = program_index + 1;
		}
	}
	lut_index = 0;
	battery_capacity_reported = 0;
	return count;
}

DEVICE_ATTR(lut, S_IRUGO| S_IWUGO, lut_show, lut_store);

/* 
 * Resets internal structs for history and level calculations. Reads FDT to 
 * initialize.
 */
void battery_calc_init(struct platform_device *pdev)
{
	battery_discharge_lut_t	*lut;
	int			rc;
	
	battery_average_first_time = 2;
	battery_average_total = 0;
	battery_average_reported = 0;
	
	if (battery_discharge_curve == NULL) {
		battery_discharge_curve = 
			kzalloc(sizeof(battery_discharge_curve_t), GFP_KERNEL);
		battery_discharge_curve->lut = 
			kzalloc(sizeof(battery_discharge_lut_t) * ADC_LUT_ENTRIES, GFP_KERNEL);
		battery_discharge_curve->count_lut = ADC_LUT_ENTRIES;
	}
	
	/* The next table was determined in the following way: Take a full */
	/* charged battery and have it discharged. Sample time and voltage */
	/* every mV. Then the timestampe determines level (when device     */
	/* almost died is taken as level 0). From the log take a number of */
	/* samples of mV/Time and translate time in capacity.              */
	lut = battery_discharge_curve->lut;
	lut_index = 0;
	lut[lut_index].voltage = 4050000; lut[lut_index++].capacity = 1000;
	lut[lut_index].voltage = 3979000; lut[lut_index++].capacity = 917;
	lut[lut_index].voltage = 3905000; lut[lut_index++].capacity = 829;
	lut[lut_index].voltage = 3852000; lut[lut_index++].capacity = 759;
	lut[lut_index].voltage = 3808000; lut[lut_index++].capacity = 689;
	lut[lut_index].voltage = 3768000; lut[lut_index++].capacity = 620;
	lut[lut_index].voltage = 3736000; lut[lut_index++].capacity = 551;
	lut[lut_index].voltage = 3710000; lut[lut_index++].capacity = 482;
	lut[lut_index].voltage = 3686000; lut[lut_index++].capacity = 420;
	lut[lut_index].voltage = 3668000; lut[lut_index++].capacity = 363;
	lut[lut_index].voltage = 3652000; lut[lut_index++].capacity = 300;
	lut[lut_index].voltage = 3637000; lut[lut_index++].capacity = 243;
	lut[lut_index].voltage = 3600000; lut[lut_index++].capacity = 180;
	lut[lut_index].voltage = 3572000; lut[lut_index++].capacity = 95;
	lut[lut_index].voltage = 3500000; lut[lut_index++].capacity = 0;
	battery_discharge_curve->count_lut = lut_index;
	lut_index = 0;
	
	/* Read the voltage scaler from FDT. The default value was determined */
	/* using one valdez unit and measuring voltage against reported adc   */
	adc_voltage_scaling = (int)fdt_get_ulong("/options/battery", "adc-scaling", 
		ADC_VOLTAGE_SCALING_DEFAULT);
	printk("battery_calc_init: adc_voltage_scaling=%d\n", adc_voltage_scaling);
	
	if (pdev) {
		rc = device_create_file(&pdev->dev, &dev_attr_debug);
		if (rc < 0) {
			printk(KERN_ERR "battery_calc_init failed to setup debug interface: %d\n", rc);
		}
		rc = device_create_file(&pdev->dev, &dev_attr_lut);
		if (rc < 0) {
			printk(KERN_ERR "battery_calc_init failed to setup lut interface: %d\n", rc);
		}
	}
}


/* 
 * in	adc, of type unsigned int, can be negative and positive. 
 * in   charging, tells whether device is charging or not.
 * out	voltage, averaged voltage level typically between 3300 and 4200 in mV
 * out  capacity, level of battery in percentage between 0 and 100
 *
 * This function will convert adc to voltage, then average it with history. 
 * and translate the averaged value into capacity.
 */
void battery_calc_add_adc(unsigned int adc, unsigned int charging,
	                  unsigned int *voltage, unsigned int *capacity)
{
	unsigned int level;
	unsigned int average;
	unsigned int temp;
	battery_discharge_lut_t	*lut = battery_discharge_curve->lut;
	
	/* The adc value which comes in is first translated into uV. Simply  */
	/* Because the Samsung pic delivers a lineair reading from 0 to 1024 */
	/* Which corresponds to 0..xxx. In the original formalut there are   */
	/* 4 inputs to calculate the translated value: VminADC, VmaxADC,     */
	/* VminEXT and VmaxEXT, which gets interpolated. Also an offset due  */
	/* charging or discharging might be possible. Basically VminADC and  */
	/* VminEXT are both 0 and offset due to charging is compensated by   */
	/* Samsung. So the value just needs to be multiplied/dived.          */ 
	adc = (unsigned int)adc * adc_voltage_scaling;
	
	/* When state of charging changes then it takes a few samples before  */
	/* the new ADC readout stabilizes. To ignore this effect a countdown  */
	/* is used on change where the prev value is being reported till it   */
	/* the ADC has stabilized.                                            */
	if ((!battery_average_first_time) && (prev_charging_state != charging)) {
		battery_readout_stall = 4;
		prev_charging_state = charging;
	}
	if (battery_readout_stall > 1) {
		battery_readout_stall--;
		*voltage = battery_average_reported;
		*capacity = battery_capacity_reported;
		if (debug_level) {
			printk("battery_calc: ignoring ADC, charging switched, adc=%d (%d,%d)\n", 
				adc/1000, *voltage / 1000, *capacity);
		}
		return;
	}
	if (battery_readout_stall == 1) {
		battery_readout_stall--;
		battery_average_first_time = 1;
	}
		
	/* The next step is averaging the newly read value with the previous  */
	/* values. Averaging can be done in many ways. But some thoughts here */
	/* The number of calculations should be minimal. With averaging the   */
	/* goal is to remove spikes (errors) in the measurements so the       */
	/* reported value does not fluctuate that much. When doing for        */
	/* example averaging by just tracking x samples and remove the last   */
	/* sample and add the new sample, add them together and divide by x   */
	/* then due to spikes in measurement it will still fluctuate. This is */
	/* not particulary nice. So here is what we do. We take x where x is  */
	/* always a 2^, since that makes it possible to just apply shift.     */
	/* When an a new value results in a change of the average then        */
	/* history gets replaced with the new average value. This way it will */
	/* at least take longer before it averages the other way (though      */
	/* still possible). An other addition might be to track the direction */
	/* of the averaging, and differentiate in "hysteresis" on that.       */
	if (battery_average_first_time) {
		battery_average_first_time--;
		battery_average_total = adc << ADC_AVERAGING_NOOF_SAMPLES_2;
		battery_average_reported = adc;
		lut_index = 0;
		battery_capacity_reported = 0;
		prev_charging_state = charging;
		if (debug_level) {
			printk("battery_calc: first_time, adc=%d\n", adc/1000);
		}
	}
	else {
		/* The adc is in uV. So the max is around 4200000, In an     */
		/* unsigned int we can have plenty of those. The accuracy of */
		/* ADC is relatively low. The real ADC has a rango of        */ 
		/* 0..1024. This represents roughly 0..4600 mV. So this      */
		/* means that the voltage should make steps of 4.5 mV. Or    */
		/* one can make steps of 1 mV but only when difference of    */
		/* new average is 4.5 mV. The latter is preferred since it   */
		/* makes the change go relative slow and behind, but avoids  */
		/* it going in the wrong direction. The only slight problem  */
		/* is that it is in general 4 mV wrong. But since that is    */
		/* the maximum accuracy of the ADC, that should not be       */
		/* a problem at all.                                         */
		battery_average_total -= (battery_average_total >> ADC_AVERAGING_NOOF_SAMPLES_2);
		battery_average_total += adc;
		
		average = battery_average_total >> ADC_AVERAGING_NOOF_SAMPLES_2;
		if (average > battery_average_reported) {
			if ((average - battery_average_reported) > ADC_AVERAGING_MIN_DIFF) {
				battery_average_reported += 
				    (average - battery_average_reported - ADC_AVERAGING_MIN_DIFF + 1000);
		 	 	/* Force recalculation of capacity */
		 	 	battery_capacity_reported = 0;
		 	 	if ((average - battery_average_reported) > 1000000) {
		 	 		printk("Battery drive ERROR, voltage diff larger then 1V %d,%d\n",
		 	 			average, battery_average_reported);
		 	 		battery_average_reported = average;
		 	 	}
		 	}
		}
		else {
			if ((battery_average_reported - average) > ADC_AVERAGING_MIN_DIFF) {
				battery_average_reported -= 
				    (battery_average_reported - average - ADC_AVERAGING_MIN_DIFF + 1000);
				/* Force recalculation of capacity */
				battery_capacity_reported = 0;
		 	 	if ((battery_average_reported - average) > 1000000) {
		 	 		printk("Battery drive ERROR, voltage diff larger then 1V %d,%d\n",
		 	 			average, battery_average_reported);
		 	 		battery_average_reported = average;
		 	 	}
			}
			 
		}
	}
	*voltage = battery_average_reported;
	/* First step done. Next step is dermining level (in percentage) Only */
	/* do the calculation if voltage changed (battery_capacity_reported   */
	/* was reset to 0. Level report of 0 will force recalculation each    */
	/* time, but that should not happen anyway. System should already     */
	/* have shutdown.                                                     */
	
	/* When the battery is charging the voltage readout is 90mV higher    */
	/* then when it is not charging. When this is not compensated for     */
	/* then switching from charge to non-charge and vice versa will       */
	/* generate quite big jumps in level and voltage. Battery gauges do   */
	/* report the actual voltage. But the level is still "matching". To   */
	/* calculate a correct level the voltage should be compensated for    */
	/* when charging. The difference in voltage is different on situation */
	/* and actual voltage. But the smallest difference is 90mV. This was  */
	/* measured between 3500-4050 (higher we get smaller difference, but  */
	/* level is already 100% there. The maximum                           */
	/* difference is close to 100mV. But we dont want to overcompensate.  */
	/* When charging is started and voltage raises quickly (due to only   */
	/* compensating for with 90mV) then level will raise as well little   */
	/* bit. User can accept that. However if the voltage is compensated   */
	/* with 90mV and difference is only 80mV then battery level will      */
	/* go down on insertion of charger and that is very strange behavior  */
	/* towards user. So therefore we compensate with lowest value: 90mV   */
	level = battery_average_reported;
	if (charging) {
		level -= 90000; /* uV */
	}
	if (battery_discharge_curve->count_lut == 0) {
		battery_capacity_reported = 100;
	}
	if (battery_capacity_reported == 0) {
		while ((lut_index > 0) && (level > lut[lut_index].voltage)) {
			lut_index--;
		}
		while ((lut_index < battery_discharge_curve->count_lut - 1) && 
		       (level <= lut[lut_index + 1].voltage)) {
			lut_index++;
		}
		if (level > lut[lut_index].voltage) {
			battery_capacity_reported = lut[lut_index].capacity / 10; /* should be 100 */
		}
		else if (lut_index == battery_discharge_curve->count_lut - 1) {
			battery_capacity_reported = lut[lut_index].capacity / 10; /* should be 0 */
		}
		else {
			/* Interpolate. This will all work with uV and promille. It */
			/* all fits in unsigned int for normal range of battery     */
			/* voltage. And it only requires one division               */
			temp = level - lut[lut_index + 1].voltage;
			temp = temp * (lut[lut_index].capacity - lut[lut_index + 1].capacity);
			temp = temp / (lut[lut_index].voltage - lut[lut_index + 1].voltage);
			temp = temp + lut[lut_index + 1].capacity;
			battery_capacity_reported = temp / 10;
		}
	}
	*capacity = battery_capacity_reported;
	if (debug_level) {
		printk("battery_calc: adc=%d, voltage=%d, percentage=%d\n",
			adc/1000, battery_average_reported/1000, battery_capacity_reported/1000);
	}
}
