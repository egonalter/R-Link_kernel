#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fcntl.h>
#include <linux/stat.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <plat/cpu-freq.h>

#include <mach/regs-gpio.h>
#include <plat/gpio-cfg.h>
#define ARM_LE		0
#define INT_LE		1

extern struct Clock_table ctable;

enum PMIC_VOLTAGE {
	VOUT_1_00,
	VOUT_1_05,
	VOUT_1_10,
	VOUT_1_15,
	VOUT_1_20,
	VOUT_1_25,
	VOUT_1_30,
	VOUT_1_35,
	VOUT_1_40,
	VOUT_1_45,
	VOUT_1_50
};

/* ltc3714 voltage table */


static const unsigned int voltage_table[11] = {
	0xf, 0xe, 0xd, 0xc, 0xb, 0xa, 0x9,
	0x8, 0x7, 0x6, 0x5,
};

#define L0      (667200)
//#define L0      (667*1000)
#define L1      (532*1000)
#define L2      (333*1000)
#define L3      (266*1000)
#define L4      (166*1000)
#define L5      (133*1000)

/* frequency voltage matching table */
static const unsigned int frequency_match[][3] = {
/*	     frequency,         VDD ARM voltage  	Matched VDD INT			*/
           {	L0,		   VOUT_1_20,	 	   VOUT_1_20	}, //667
   	   {	L1, 		   VOUT_1_10, 		   VOUT_1_10	}, //533
           {	L2, 		   VOUT_1_05, 		   VOUT_1_05	}, //333
           {	L3, 		   VOUT_1_05, 		   VOUT_1_05	}, //266
           {	L4, 		   VOUT_1_00, 		   VOUT_1_00	}, //166
           {	L5, 		   VOUT_1_00, 		   VOUT_1_00	}, //133
};

static unsigned int gpn_dat_reg;
/* LTC3714 Setting Routine */
static int ltc3714_gpio_setting(void)
{
	unsigned int gpio_num[]={2,3,4,9,10,13,14,15};
	int i = 0,err;
	
	gpio_free(S5P6450_GPN(9));
	gpio_free(S5P6450_GPN(10));
	
	for(i=0;i<ARRAY_SIZE(gpio_num);i++){
		gpio_request(S5P64XX_GPN(gpio_num[i]),"GPN");
		 if (err) {
                	printk(KERN_ERR "failed to request GPN for "
                        "PMIC Voltage control\n");
	                return err;
		}
	}
	
	gpio_direction_output(S5P64XX_GPN(9), 0);
	gpio_direction_output(S5P64XX_GPN(10), 0);
	gpio_direction_output(S5P64XX_GPN(13), 0);
	gpio_direction_output(S5P64XX_GPN(14), 0);
	gpio_direction_output(S5P64XX_GPN(15), 0);
	gpio_direction_output(S5P64XX_GPN(2), 0);
	gpio_direction_output(S5P64XX_GPN(3), 0);
	gpio_direction_output(S5P64XX_GPN(4), 0);


	s3c_gpio_setpull(S5P64XX_GPN(9), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(S5P64XX_GPN(10), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(S5P64XX_GPN(13), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(S5P64XX_GPN(14), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(S5P64XX_GPN(15), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(S5P64XX_GPN(2), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(S5P64XX_GPN(3), S3C_GPIO_PULL_NONE);
	s3c_gpio_setpull(S5P64XX_GPN(4), S3C_GPIO_PULL_NONE);
	
	for(i=0;i<ARRAY_SIZE(gpio_num);i++){
		gpio_free(S5P64XX_GPN(gpio_num[i]));
	}

	return 0;
}

static int set_ltc3714(unsigned int pwr, unsigned int index)
{
	int gpio_val;
	int voltage = frequency_match[index][pwr + 1];
	
	gpio_val = voltage_table[voltage];
	gpio_val &=0x1f;
	
	gpio_set_value(S5P64XX_GPN(10),(gpio_val >> 1)&0x1);
	gpio_set_value(S5P64XX_GPN(13),(gpio_val >> 2)&0x1);
	gpio_set_value(S5P64XX_GPN(14),(gpio_val >> 3)&0x1);
	gpio_set_value(S5P64XX_GPN(15),(gpio_val >> 4)&0x1);
	gpio_set_value(S5P64XX_GPN(9),(gpio_val >> 0)&0x1);
	
	mdelay(20);
	if(pwr == ARM_LE) {
		gpio_set_value(S5P64XX_GPN(2), 1);
		udelay(10);
		gpio_set_value(S5P64XX_GPN(2), 0);
	} else if(pwr == INT_LE) {
		gpio_set_value(S5P64XX_GPN(4), 1);
		udelay(10);
		gpio_set_value(S5P64XX_GPN(4), 0);
	} else {
		printk("[error]: set_power, check mode [pwr] value\n");
		return -EINVAL;
	}
	return 0;
}

static int find_voltage(int freq)
{
	int index = 0;

	if(freq > frequency_match[0][0]){
		printk(KERN_ERR "frequecy is over then support frequency\n");
		return 0;
	}

	for(index = 0 ; index < ARRAY_SIZE(frequency_match) ; index++){
		if(freq >= frequency_match[index][0])
			return index;
	}

	printk("Cannot find matched voltage on table\n");

	return 0;
}

int set_power(unsigned int freq)
{
	int index,temp;

	gpio_free(S5P6450_GPN(9));
	gpio_free(S5P6450_GPN(10));
	gpio_free(S5P6450_GPN(14));
	gpio_free(S5P6450_GPN(15));
		
	gpio_request(S5P64XX_GPN(9),"GPN");
	gpio_request(S5P64XX_GPN(10),"GPN");
	gpio_request(S5P64XX_GPN(14),"GPN");
	gpio_request(S5P64XX_GPN(15),"GPN");
	
	/*Restore GPN9 & GPN10 for LTC3714*/
	gpio_direction_output(S5P64XX_GPN(9), 0);
	s3c_gpio_setpull(S5P64XX_GPN(9), S3C_GPIO_PULL_NONE);
	
	gpio_direction_output(S5P64XX_GPN(10), 0);
	s3c_gpio_setpull(S5P64XX_GPN(10), S3C_GPIO_PULL_NONE);
	
	gpio_direction_output(S5P64XX_GPN(14), 0);
	s3c_gpio_setpull(S5P64XX_GPN(14), S3C_GPIO_PULL_NONE);
	
	gpio_direction_output(S5P64XX_GPN(15), 0);
	s3c_gpio_setpull(S5P64XX_GPN(15), S3C_GPIO_PULL_NONE);
	
	index = find_voltage(freq);
	set_ltc3714(ARM_LE, index);
	set_ltc3714(INT_LE, index);
	
	gpio_free(S5P6450_GPN(9));
	gpio_free(S5P6450_GPN(10));
	gpio_free(S5P6450_GPN(14));
	gpio_free(S5P6450_GPN(15));
	
	/* Reconfigure GPN9 & GPN10 for EINT9 & EINT10 */
	
 	temp =  (gpn_dat_reg & 0xffffffff);
	
	s3c_gpio_cfgpin(S5P64XX_GPN(9), S5P64XX_GPN9_EINT9);
        s3c_gpio_setpull(S5P64XX_GPN(9), S3C_GPIO_PULL_NONE);
	gpio_set_value(S5P64XX_GPN(9),(temp >> 9) &0x1);
	
        s3c_gpio_cfgpin(S5P64XX_GPN(10), S5P64XX_GPN10_EINT10);
        s3c_gpio_setpull(S5P64XX_GPN(10), S3C_GPIO_PULL_NONE);
	gpio_set_value(S5P64XX_GPN(10),(temp >> 10) &0x1);
 	
	return 0;
}

EXPORT_SYMBOL(set_power);

void ltc3714_init(void)
{
	gpn_dat_reg = __raw_readl(S5P64XX_GPNDAT);
	ltc3714_gpio_setting();
	if(ctable.apll_rate == IS_ARM_667){
		set_power(L0);
	}else if(ctable.apll_rate == IS_ARM_533)
	{
		set_power(L1);
	}
	gpio_set_value(S5P64XX_GPN(3), 1);
}

EXPORT_SYMBOL(ltc3714_init);
