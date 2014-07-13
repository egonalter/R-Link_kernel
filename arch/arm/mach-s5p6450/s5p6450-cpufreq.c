/*  linux/arch/arm/mach-s5p6450/cpu-freq.c
 *
 *  Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *
 *  CPU frequency scaling for S5PC110
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/regulator/consumer.h>
#include <linux/gpio.h>
#include <asm/system.h>

#include <plat/pll.h>
#include <plat/clock.h>
#include <plat/gpio-cfg.h>
#include <plat/cpu-freq.h>
#include <mach/map.h>
#include <mach/cpu-freq-s5p6450.h>
#include <mach/regs-clock.h>
#include <mach/regs-gpio.h>

extern struct Clock_table ctable;
static struct clk *mpu_clk;

static struct regulator *arm_regulator;
static struct regulator *internal_regulator;
static enum perf_level bootup_level;

#if defined(CONFIG_S5P64XX_LTC3714)             
extern int set_power(unsigned int freq);
extern void ltc3714_init(void);
#endif

struct s5p6450_cpufreq_freqs s5p6450_freqs;

/* Based on MAX8698C 
 * RAMP time : 1
 **/
#define PMIC_RAMP_UP	10
static unsigned long previous_arm_volt;

/* If you don't need to wait exact ramp up delay
 * you can use fixed delay time
 **/
//#define USE_FIXED_RAMP_UP_DELAY

/* frequency table*/
#if 0
static struct cpufreq_frequency_table s5p6450_freq_table[] = {
        { 0, 800000},  //*KHZ_T },
        { 1, 667200},  //*KHZ_T },
        { 2, 532000},  //*KHZ_T },
        { 3, 400000},  //*KHZ_T },
        { 4, 333000},  //*KHZ_T },
        { 5, 266000},  //*KHZ_T },
        { 6, 166000},
        { 7, 133000},
        { 8, 100000},
        { 9, CPUFREQ_TABLE_END },
};
#endif 
static struct cpufreq_frequency_table s5p6450_freq_table_800[] = {
        { 0, 800000},  //*KHZ_T },
        { 1, 400000},  //*KHZ_T },
        { 2, 100000},
        { 3, CPUFREQ_TABLE_END },
};

#if 1
static struct cpufreq_frequency_table s5p6450_freq_table_667[] = {
        { 0, 667200},  //*KHZ_T },
//        { 0, 667*KHZ_T },
        { 1, 333*KHZ_T },
        { 2, 166*KHZ_T },
        { 3, CPUFREQ_TABLE_END },
};
static struct cpufreq_frequency_table s5p6450_freq_table_533[] = {
        { 0, 533*KHZ_T },
        { 1, 266*KHZ_T },
        { 2, 133*KHZ_T },
        { 3, CPUFREQ_TABLE_END },
};
#endif
//#if defined(CONFIG_S5P6450_S5M8752)

//
//
static struct s5p6450_dvs_conf s5p6450_dvs_confs[] = {
	{
		.lvl		= L0,                    //800
		.arm_volt	= 1300000,
		.int_volt	= 1100000,
	},{
		.lvl		= L1,			 //667
		.arm_volt	= 1200000,	
		.int_volt	= 1100000,
	},{
		.lvl		= L2,                    //553
		.arm_volt	= 1100000,
		.int_volt	= 1100000,
	},{
		.lvl		= L3,                     //400 
		.arm_volt	= 1100000,
		.int_volt	= 1100000,
	},{
		.lvl		= L4,                    //333 
		.arm_volt	= 1100000,
		.int_volt	= 1100000,
	},{
		.lvl		= L5,                    //266
		.arm_volt	= 1050000,
		.int_volt	= 1050000,
	},{
		.lvl		= L6,                   //166
		.arm_volt	= 1000000,
		.int_volt	= 1000000,
	},{
		.lvl		= L7,                   //133 
		.arm_volt	= 1000000,
		.int_volt	= 1000000,
	},{
		.lvl		= L8,                   //100
		.arm_volt	= 1000000,
		.int_volt	= 1000000,
	},
};
//
#if 0
static struct s5p6450_dvs_conf s5p6450_dvs_confs[] = {
	{
		.lvl		= L0,
		.arm_volt	= 1200000,
		.int_volt	= 1200000,
	}, {
		.lvl		= L1,
		.arm_volt	= 1100000,
		.int_volt	= 1100000,

	}, {
		.lvl		= L2,
		.arm_volt	= 1100000,
		.int_volt	= 1100000,
	},
};


static u32 clkdiv_val[3][5] = {
/*{ ARM, HCLK_HIGH, PCLK_HIGH,
 *	HCLK_LOW, PCLK_LOW}
 */
	{0, 3, 1, 4, 1, },
	{1, 3, 1, 4, 1, },
	{3, 3, 1, 4, 1, },
};
#endif
static struct s5p6450_domain_freq s5p6450_clk_info[] = {
	{
		.apll_out 	= 800000,
		.armclk 	= 800000,
		.hclk_high 	= 166000,
		.pclk_high 	= 83000,
		.hclk_low 	= 133000,
		.pclk_low 	= 66000,
	}, {
		.apll_out 	= 667000,
		.armclk 	= 333000,
		.hclk_high 	= 166000,
		.pclk_high 	= 83000,
		.hclk_low 	= 133000,
		.pclk_low 	= 66000,
	}, {
		.apll_out 	= 533000,
		.armclk 	= 533000,
		.hclk_high 	= 133000,
		.pclk_high 	= 66000,
		.hclk_low 	= 133000,
		.pclk_low 	= 66000,
	}, 
};

int s5p6450_verify_speed(struct cpufreq_policy *policy)
{

	if (policy->cpu)
		return -EINVAL;

	return cpufreq_frequency_table_verify(policy, ctable.freq_table);
}

unsigned int s5p6450_getspeed(unsigned int cpu)
{
	unsigned long rate;

	if (cpu)
		return 0;

	rate = clk_get_rate(mpu_clk) / KHZ_T;

	return rate;
}

static int s5p6450_target(struct cpufreq_policy *policy,
		       unsigned int target_freq,
		       unsigned int relation)
{
	int ret = 0;
	unsigned long arm_clk;
	unsigned int index, arm_volt, int_volt;
	unsigned int bus_speed_changing = 0;
	unsigned int i;

	s5p6450_freqs.freqs.old = s5p6450_getspeed(0);
	if (cpufreq_frequency_table_target(policy, ctable.freq_table,
		target_freq, relation, &index)) {
		ret = -EINVAL;
		goto out;
	}

	arm_clk = ctable.freq_table[index].frequency;

	for(i = 0;i<sizeof(s5p6450_dvs_confs);i++){
		if(arm_clk == s5p6450_dvs_confs[i].lvl){
			 index = i;
			 break;
		}
	}

	s5p6450_freqs.freqs.new = arm_clk;
	s5p6450_freqs.freqs.cpu = 0;

#if (defined(CONFIG_S5P6450_S5M8752) || defined(CONFIG_S5P6450_S5M8751))
	if (s5p6450_freqs.freqs.new == s5p6450_freqs.freqs.old){
		if(s5p6450_dvs_confs[index].arm_volt != regulator_get_voltage(arm_regulator)){
			printk(KERN_INFO "adjusting ARM_VDD and  INT_VDD\n");
			regulator_set_voltage(arm_regulator, s5p6450_dvs_confs[index].arm_volt,s5p6450_dvs_confs[index].arm_volt);
#if defined(CONFIG_S5P6450_S5M8752)
			regulator_set_voltage(internal_regulator, s5p6450_dvs_confs[index].int_volt,s5p6450_dvs_confs[index].int_volt);
#endif
		}
		return 0;
	}
#endif

#if (defined(CONFIG_S5P6450_S5M8752) || defined(CONFIG_S5P6450_S5M8751))
	arm_volt = s5p6450_dvs_confs[index].arm_volt;
#endif

#if defined(CONFIG_S5P6450_S5M8752)
	int_volt = s5p6450_dvs_confs[index].int_volt;
#endif 

	/* Check if there need to change System bus clock */
	if (s5p6450_freqs.new.hclk_high != s5p6450_freqs.old.hclk_high)
	        bus_speed_changing = 1;

	/* iNew clock inforamtion update */
	memcpy(&s5p6450_freqs.new, &s5p6450_clk_info[index],
					sizeof(struct s5p6450_domain_freq));
	cpufreq_notify_transition(&s5p6450_freqs.freqs, CPUFREQ_PRECHANGE);

	if (s5p6450_freqs.freqs.new > s5p6450_freqs.freqs.old) {
		/*voltage up */		
#if defined(CONFIG_S5P64XX_LTC3714)             
                set_power(s5p6450_freqs.freqs.new);
                ret = clk_set_rate(mpu_clk, s5p6450_freqs.freqs.new * KHZ_T);
                if (ret != 0)
                        pr_info("frequency scaling error\n");
#endif

#if (defined(CONFIG_S5P6450_S5M8752) || defined(CONFIG_S5P6450_S5M8751))
		regulator_set_voltage(arm_regulator, arm_volt, arm_volt);
#if defined(CONFIG_S5P6450_S5M8752)
		regulator_set_voltage(internal_regulator, int_volt,	int_volt);
#endif
//		s5p6450_set_dvi_for_dvfs(index);
                ret = clk_set_rate(mpu_clk, s5p6450_freqs.freqs.new * KHZ_T);
#endif
		if(bus_speed_changing)
			__raw_writel(0X50E, S3C_VA_MEM + 0x30);
	} else {
		/* Voltage down */
		if(bus_speed_changing)
			__raw_writel(0X287, S3C_VA_MEM + 0x30);
#if defined(CONFIG_S5P64XX_LTC3714)             
                 ret = clk_set_rate(mpu_clk, s5p6450_freqs.freqs.new * KHZ_T);
                if (ret != 0)
                        pr_info("frequency scaling error\n");
                set_power(s5p6450_freqs.freqs.new);
#endif

#if (defined(CONFIG_S5P6450_S5M8752) || defined(CONFIG_S5P6450_S5M8751))
                ret = clk_set_rate(mpu_clk, s5p6450_freqs.freqs.new * KHZ_T);
		regulator_set_voltage(arm_regulator, arm_volt,arm_volt);
#endif
#if defined(CONFIG_S5P6450_S5M8752)
		regulator_set_voltage(internal_regulator, int_volt,	int_volt);
#endif
	}

	printk(KERN_INFO "Tfreq: %u\tOfreq: %u\tPolfreqs: %u   Volt: %u\n",target_freq,s5p6450_freqs.freqs.old,s5p6450_freqs.freqs.new,arm_volt);
	cpufreq_notify_transition(&s5p6450_freqs.freqs, CPUFREQ_POSTCHANGE);

	memcpy(&s5p6450_freqs.old, &s5p6450_freqs.new, sizeof(struct s5p6450_domain_freq));
	printk(KERN_INFO "Perf changed[L%d]\n", index);

#if (defined(CONFIG_S5P6450_S5M8752) || defined(CONFIG_S5P6450_S5M8751))
	previous_arm_volt = s5p6450_dvs_confs[index].arm_volt;
#endif

out:
	return ret;
}

static int s5p6450_cpufreq_suspend(struct cpufreq_policy *policy,
			pm_message_t pmsg)
{
	int ret = 0;
return ret;
}

static int s5p6450_cpufreq_resume(struct cpufreq_policy *policy)
{
	int ret = 0;
	/* Clock inforamtion update with wakeup value */
	memcpy(&s5p6450_freqs.old, &s5p6450_clk_info[bootup_level],
			sizeof(struct s5p6450_domain_freq));
	previous_arm_volt = s5p6450_dvs_confs[bootup_level].arm_volt;
	return ret;
}

static int __init s5p6450_cpu_init(struct cpufreq_policy *policy)
{
	u32 reg, rate;

#ifdef CLK_OUT_PROBING
	reg = __raw_readl(S5P_CLK_OUT);
	reg &= ~(0x1f << 12 | 0xf << 20);
	reg |= (0xf << 12 | 0x1 << 20);
	__raw_writel(reg, S5P_CLK_OUT);
#endif
	mpu_clk = clk_get(NULL, MPU_CLK);
	if (IS_ERR(mpu_clk))
		return PTR_ERR(mpu_clk);
#if defined(CONFIG_REGULATOR)
	arm_regulator = regulator_get(NULL, "vddarm_int");
	if (IS_ERR(arm_regulator)) {
		printk(KERN_ERR "failed to get resource %s\n", "vddarm");
		return PTR_ERR(arm_regulator);
	}
//	internal_regulator = regulator_get(NULL, "vddint");
//	if (IS_ERR(internal_regulator)) {
///		printk(KERN_ERR "failed to get resource %s\n", "vddint");
//		return PTR_ERR(internal_regulator);
//	}

#endif
#if defined(CONFIG_S5P64XX_LTC3714)             
        ltc3714_init();
#endif
	if (policy->cpu != 0)
		return -EINVAL;
	policy->cur = policy->min = policy->max = s5p6450_getspeed(0);

	cpufreq_frequency_table_get_attr(ctable.freq_table, policy->cpu);

	policy->cpuinfo.transition_latency = 40000;

	rate = clk_get_rate(mpu_clk);

	switch (rate) {
	case 800000000:	
		bootup_level = L0;
		break;
	case 667200000:
		bootup_level = L1;
		break;
	default:
		printk(KERN_ERR "[%s] cannot find matching clock"
				"[%s] rate [%d]\n"
				, __FUNCTION__, MPU_CLK, rate);
		bootup_level = L0;
		break;
	}
	memcpy(&s5p6450_freqs.old, &s5p6450_clk_info[bootup_level],
			sizeof(struct s5p6450_domain_freq));
	
	previous_arm_volt = s5p6450_dvs_confs[bootup_level].arm_volt;
	
	return cpufreq_frequency_table_cpuinfo(policy, ctable.freq_table);
}

static struct cpufreq_driver s5p6450_driver = {
	.flags		= CPUFREQ_STICKY,
	.verify		= s5p6450_verify_speed,
	.target		= s5p6450_target,
	.get		= s5p6450_getspeed,
	.init		= s5p6450_cpu_init,
	.name		= "s5p6450",
	.suspend	= s5p6450_cpufreq_suspend,
	.resume		= s5p6450_cpufreq_resume,
};

static int __init s5p6450_cpufreq_init(void)
{
#if 1
	if(ctable.apll_rate == IS_ARM_667){
                ctable.freq_table = s5p6450_freq_table_667;
        }else if(ctable.apll_rate == IS_ARM_533){
                ctable.freq_table = s5p6450_freq_table_533;
        }else{
                ctable.freq_table = s5p6450_freq_table_800;
	}
#endif
	return cpufreq_register_driver(&s5p6450_driver);
}

late_initcall(s5p6450_cpufreq_init);
