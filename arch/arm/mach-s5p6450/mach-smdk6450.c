/* linux/arch/arm/mach-s5p6450/mach-smdk6450.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/i2c.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/mtd/partitions.h>
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/pwm_backlight.h>
#include <linux/spi/spi.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>

#include <mach/hardware.h>
#include <mach/map.h>
#include <mach/regs-mem.h>

#include <asm/irq.h>
#include <asm/mach-types.h>

#include <plat/regs-serial.h>
#include <plat/iic.h>
#include <plat/fimc.h>

#include <plat/fb.h>

#include <plat/regs-otg.h>
#include <plat/nand.h>
#include <plat/partition.h>
//#include <plat/s5p6450.h>
#include <plat/s5p6450.h>
#include <plat/clock.h>
#include <mach/regs-clock.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <plat/ts.h>

#include <media/s5k4ba_platform.h>
#include <plat/adc.h>
#include <plat/pm.h>
#include <plat/pll.h>
#include <plat/regs-rtc.h>
#include <plat/spi.h>
#include <plat/media.h>
#include <mach/gpio.h>
#include <mach/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/gpio-bank-c.h>
#include <plat/gpio-bank-n.h>
#include <plat/gpio-bank-f.h>
#include <plat/mshci.h>

#include <linux/regulator/machine.h>
#if defined(CONFIG_MFD_S5M8752)
#include <linux/mfd/s5m8752/core.h>
#include <linux/mfd/s5m8752/pdata.h>
#endif
#if defined(CONFIG_MFD_S5M8751)
#include <linux/mfd/s5m8751/core.h>
#include <linux/mfd/s5m8751/pdata.h>
#endif

u32 otg_phy_clk_value;
u32 base_clk;

#define CONFIG_TLV_3101  1

#include <plat/regs-otg.h>
#if 0
//#define OTGH_PHY_CLK_VALUE      (0x02)  /* UTMI Interface, Cristal, 12Mhz clk for PLL */
#define OTGH_PHY_CLK_VALUE      (0x21)  /* UTMI Interface, Cristal, 12Mhz clk for PLL */
#endif
#include <linux/usb/ch9.h>

/*pmem */
#include <linux/android_pmem.h>


/* S3C_USB_CLKSRC 0: EPLL 1: CLK_48M */
#define S3C_USB_CLKSRC  1
//#define OTGH_PHY_CLK_VALUE      (0x02)  /* UTMI Interface, Cristal, 12Mhz clk for PLL */



#define S5P6450_UCON_DEFAULT    (S3C2410_UCON_TXILEVEL |	\
				S3C2410_UCON_RXILEVEL |		\
				S3C2410_UCON_TXIRQMODE |	\
				S3C2410_UCON_RXIRQMODE |	\
				S3C2410_UCON_RXFIFO_TOI |	\
				S3C2443_UCON_RXERR_IRQEN)

#define S5P6450_ULCON_DEFAULT	S3C2410_LCON_CS8

#define S5P6450_UFCON_DEFAULT   (S3C2410_UFCON_FIFOMODE |	\
				S3C2440_UFCON_TXTRIG16 |	\
				S3C2410_UFCON_RXTRIG8)
extern void s3c64xx_reserve_bootmem(void);

void __init s3c_spi_set_slaves(unsigned id, int n, struct s3c_spi_pdata const *dat);

#if defined(CONFIG_SPI_CNTRLR_0) || defined(CONFIG_SPI_CNTRLR_1)
static void s3c_cs_suspend(int pin, pm_message_t pm)
{
	/* Whatever need to be done */
}

static void s3c_cs_resume(int pin)
{
	/* Whatever need to be done */
}

static void cs_set(int pin, int lvl)
{
	unsigned int val;

	val = __raw_readl(S5P64XX_GPNDAT);
	val &= ~(1<<pin);

	if(lvl == CS_HIGH)
	   val |= (1<<pin);

	__raw_writel(val, S5P64XX_GPNDAT);
}

static void s3c_cs_setF13(int pin, int lvl)
{
	cs_set(13, lvl);
}

static void s3c_cs_setF14(int pin, int lvl)
{
	cs_set(14, lvl);
}

static void s3c_cs_setF15(int pin, int lvl)
{
	cs_set(15, lvl);
}

static void cs_cfg(int pin, int pull)
{
	unsigned int val;

	val = __raw_readl(S5P64XX_GPNCON);
	val &= ~(3<<(pin*2));
	val |= (1<<(pin*2)); /* Output Mode */
	__raw_writel(val, S5P64XX_GPNCON);

	val = __raw_readl(S5P64XX_GPNPUD);
	val &= ~(3<<(pin*2));

	if(pull == CS_HIGH)
	   val |= (2<<(pin*2));	/* PullUp */
	else
	   val |= (1<<(pin*2)); /* PullDown */

	__raw_writel(val, S5P64XX_GPNPUD);
}

static void s3c_cs_configF13(int pin, int mode, int pull)
{
	cs_cfg(13, pull);
}

static void s3c_cs_configF14(int pin, int mode, int pull)
{
	cs_cfg(14, pull);
}

static void s3c_cs_configF15(int pin, int mode, int pull)
{
	cs_cfg(15, pull);
}

static void s3c_cs_set(int pin, int lvl)
{
	if(lvl == CS_HIGH)
	   gpio_set_value(pin, 1);
	else
	   gpio_set_value(pin, 0);
}

static void s3c_cs_config(int pin, int mode, int pull)
{
	s3c_gpio_cfgpin(pin, mode);

	if(pull == CS_HIGH)
	   s3c_gpio_setpull(pin, S3C_GPIO_PULL_UP);
	else
	   s3c_gpio_setpull(pin, S3C_GPIO_PULL_DOWN);
}
#endif


#ifdef CONFIG_ANDROID_PMEM
// pmem 
static struct android_pmem_platform_data pmem_pdata = {
        .name = "pmem",
        .no_allocator = 1,
        .cached = 1,
        .start = 0, // will be set during proving pmem driver.
        .size = 0 // will be set during proving pmem driver.
};

static struct android_pmem_platform_data pmem_gpu1_pdata = {
   .name = "pmem_gpu1",
   .no_allocator = 1,
   .cached = 1,
   .buffered = 1,
   .start = 0,
   .size = 0,
};

static struct android_pmem_platform_data pmem_adsp_pdata = {
   .name = "pmem_adsp",
   .no_allocator = 1,
   .cached = 1,
   .buffered = 1,
   .start = 0,
   .size = 0,
};

static struct platform_device pmem_device = {
   .name = "android_pmem",
   .id = 0,
   .dev = { .platform_data = &pmem_pdata },
}; 
static struct platform_device pmem_gpu1_device = {
        .name = "android_pmem",
        .id = 1,
        .dev = { .platform_data = &pmem_gpu1_pdata },
};

static struct platform_device pmem_adsp_device = {
        .name = "android_pmem",
        .id = 2,
        .dev = { .platform_data = &pmem_adsp_pdata },
};

#endif //PMEM

#if defined(CONFIG_SPI_CNTRLR_0)
static struct s3c_spi_pdata s3c_slv_pdata_0[] __initdata = {
	[0] = {	/* Slave-0 */
		.cs_level     = CS_FLOAT,
		.cs_pin       = S5P64XX_GPC(3),
		.cs_mode      = S5P64XX_GPC_OUTPUT(3),
		.cs_set       = s3c_cs_set,
		.cs_config    = s3c_cs_config,
		.cs_suspend   = s3c_cs_suspend,
		.cs_resume    = s3c_cs_resume,
	},
};
#endif

#if defined(CONFIG_SPI_CNTRLR_1)
static struct s3c_spi_pdata s3c_slv_pdata_1[] __initdata = {
	[0] = {	/* Slave-0 */
		.cs_level     = CS_FLOAT,
		.cs_pin       = S5P64XX_GPC(7),
		.cs_mode      = S5P64XX_GPC_OUTPUT(7),
		.cs_set       = s3c_cs_set,
		.cs_config    = s3c_cs_config,
		.cs_suspend   = s3c_cs_suspend,
		.cs_resume    = s3c_cs_resume,
	},
};
#endif

static struct spi_board_info s3c_spi_devs[] __initdata = {
#if defined(CONFIG_SPI_CNTRLR_0)
	[0] = {
		.modalias	 = "spidev", /* Test Interface */
		.mode		 = SPI_MODE_0,	/* CPOL=0, CPHA=0 */
		.max_speed_hz    = 100000,
		/* Connected to SPI-0 as 1st Slave */
		.bus_num	 = 0,
		.irq		 = IRQ_SPI0,
		.chip_select	 = 0,
	},
#endif
#if defined(CONFIG_SPI_CNTRLR_1)
	[1] = {
		.modalias	 = "spidev", /* Test Interface */
		.mode		 = SPI_MODE_0,	/* CPOL=0, CPHA=0 */
		.max_speed_hz    = 100000,
		/* Connected to SPI-1 as 1st Slave */
		.bus_num	 = 1,
		.irq		 = IRQ_SPI1,
		.chip_select	 = 0,
	},
#endif
};

static struct s3c2410_uartcfg smdk6450_uartcfgs[] __initdata = {
	[0] = {
		.hwport	     = 0,
		.flags	     = 0,
		.ucon	     = S5P6450_UCON_DEFAULT,
		.ulcon	     = S5P6450_ULCON_DEFAULT,
		.ufcon	     = S5P6450_UFCON_DEFAULT,
	},
	[1] = {
		.hwport	     = 1,
		.flags	     = 0,
		.ucon	     = S5P6450_UCON_DEFAULT,
		.ulcon	     = S5P6450_ULCON_DEFAULT,
		.ufcon	     = S5P6450_UFCON_DEFAULT,
	},
	[2] = {
		.hwport	     = 2,
		.flags	     = 0,
		.ucon	     = S5P6450_UCON_DEFAULT,
		.ulcon	     = S5P6450_ULCON_DEFAULT,
		.ufcon	     = S5P6450_UFCON_DEFAULT,
	},
	[3] = {
		.hwport	     = 3,
		.flags	     = 0,
		.ucon	     = S5P6450_UCON_DEFAULT,
		.ulcon	     = S5P6450_ULCON_DEFAULT,
		.ufcon	     = S5P6450_UFCON_DEFAULT,
	},

        [4] = { 
                .hwport      = 4,
                .flags       = 0,
                .ucon        = S5P6450_UCON_DEFAULT,
                .ulcon       = S5P6450_ULCON_DEFAULT,
                .ufcon       = S5P6450_UFCON_DEFAULT,
        },
        [5] = {
                .hwport      = 5,
                .flags       = 0,
                .ucon        = S5P6450_UCON_DEFAULT,
                .ulcon       = S5P6450_ULCON_DEFAULT,
                .ufcon       = S5P6450_UFCON_DEFAULT,
        },
};

struct platform_device sec_device_battery = {
        .name   = "sec-fake-battery",
        .id             = -1,
};

struct map_desc smdk6450_iodesc[] = {};

static struct platform_device *smdk6450_devices[] __initdata = {
	&s3c_device_i2c1,
	&s3c_device_i2c0,
#ifdef CONFIG_FB_S3C
	&s3c_device_fb,
#endif
#ifdef CONFIG_SMDK6450_SD_CH1
	&s3c_device_hsmmc1,
#endif
#ifdef CONFIG_S3C_DEV_MSHC
        &s3c_device_mshci,
#endif
#ifdef CONFIG_SMDK6450_SD_CH0
	&s3c_device_hsmmc0,
#endif
#ifdef CONFIG_SMDK6450_SD_CH2
	&s3c_device_hsmmc2,
#endif
//	&s3c_device_i2c1,
//	&s3c_device_i2c0,
	&s3c_device_wdt,
#ifdef CONFIG_ANDROID_PMEM
        &pmem_device,
        &pmem_gpu1_device,
        &pmem_adsp_device,
#endif



	&s5p_device_rtc,
//	&s3c_device_i2c0,
//	&s3c_device_i2c1,
#ifdef CONFIG_SPI_CNTRLR_0
	&s3c_device_spi0,
#endif
#ifdef CONFIG_SPI_CNTRLR_1
	&s3c_device_spi1,
#endif
	&s3c_device_ts,
	//&s3c_device_smc911x,
	&s3c_device_lcd,
	&s3c_device_nand,
	&s3c_device_usbgadget,
	&s3c_device_usb_otghcd,
#ifdef CONFIG_USB
        &s3c_device_usb_ehci,
        &s3c_device_usb_ohci,
#endif
#ifdef CONFIG_USB_GADGET
       // &s3c_device_usbgadget,
#endif


	//&s3c_device_gvg,
#ifdef CONFIG_SND_S3C24XX_SOC
        &s3c64xx_device_iis0,
#endif
#ifdef CONFIG_SND_S3C_SOC_AC97
        &s3c64xx_device_ac97,
#endif
#ifdef CONFIG_SND_S3C_SOC_PCM
        &s3c64xx_device_pcm0,
#endif
	&s3c_device_g2d,

	//&s3c_device_post,
#ifdef CONFIG_USB_ANDROID
        &s3c_device_android_usb,
        &s3c_device_usb_mass_storage,
	&s3c_device_rndis,
#endif

#ifdef CONFIG_S5P64XX_ADC
	&s3c_device_adc,
#endif
	&s3c_device_adc,

#ifdef CONFIG_HAVE_PWM
	&s3c_device_timer[0],
	&s3c_device_timer[1],
#endif
#ifdef CONFIG_KEYPAD_S3C
	&s3c_device_keypad,
#endif
	&sec_device_battery,
	&s3c_device_button,
	&s3c_device_fimc0,

#ifdef CONFIG_GIB_S5P64XX
	&s3c_device_gib,
#endif
};

#if defined(CONFIG_MFD_S5M8751)
/* SYS, EXT */
static struct regulator_init_data smdk6450_vddsys_ext = {
        .constraints = {
                .name = "PVDD_SYS/PVDD_EXT",
                .min_uV = 3300000,
                .max_uV = 3300000,
                .always_on = 1,
                .state_mem = {
                        .uV = 3300000,
                        .mode = REGULATOR_MODE_STANDBY,
                        .enabled = 1,
                },
                .initial_state = PM_SUSPEND_MEM,
        },
};

/* LCD */
static struct regulator_init_data smdk6450_vddlcd = {
        .constraints = {
                .name = "PVDD_LCD",
                .min_uV = 3300000,
                .max_uV = 3300000,
                .always_on = 1,
                .state_mem = {
                        .uV = 0,
                        .mode = REGULATOR_MODE_STANDBY,
			.enabled = 0,
                        //.disabled = 1,
                },
                .initial_state = PM_SUSPEND_MEM,
        },
};

/* ALIVE */
static struct regulator_init_data smdk6450_vddalive = {
        .constraints = {
                .name = "PVDD_ALIVE",
                .min_uV = 1100000,
                .max_uV = 1100000,
                .always_on = 1,
                .state_mem = {
                        .uV = 1100000,
                        .mode = REGULATOR_MODE_STANDBY,
                        .enabled = 1,
                },
                .initial_state = PM_SUSPEND_MEM,
        },
};
/* PLL */
static struct regulator_init_data smdk6450_vddpll = {
        .constraints = {
                .name = "PVDD_PLL",
                .min_uV = 1100000,
                .max_uV = 1100000,
                .always_on = 1,
                .state_mem = {
                        .uV = 0,
                        .mode = REGULATOR_MODE_STANDBY,
			.enabled = 0,
                        //.disabled = 1,
                },
                .initial_state = PM_SUSPEND_MEM,
        },
};

/* MEM/SS */
static struct regulator_init_data smdk6450_vddmem_ss = {
        .constraints = {
                .name = "PVDD_MEM/SS",
                .min_uV = 1800000,
                .max_uV = 1800000,
                .always_on = 1,
                .state_mem = {
                        .uV = 1800000,
                        .mode = REGULATOR_MODE_STANDBY,
                        .enabled = 1,
                },
                .initial_state = PM_SUSPEND_MEM,
        },
};

/* GPS */
static struct regulator_init_data smdk6450_vddgps = {
        .constraints = {
                .name = "PVDD_GPS",
                .min_uV = 1800000,
                .max_uV = 1800000,
                .always_on = 1,
                .state_mem = {
                        .uV = 0,
                        .mode = REGULATOR_MODE_STANDBY,
			.enabled = 0,
                        //.disabled = 1,
                },
                .initial_state = PM_SUSPEND_MEM,
        },
};
/* ARM_INT */
static struct regulator_consumer_supply smdk6450_vddarm_consumers[] = {
        {
                .supply = "vddarm_int",
        }
};
//INT

static struct regulator_init_data smdk6450_vddarm_int = {
        .constraints = {
                .name = "PVDD_ARM/INT",
//                .name = "vddint",
                .min_uV = 1000000,
                .max_uV = 1200000,
                .always_on = 1,
                .valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
                .state_mem = {
                        .uV = 0,
                        .mode = REGULATOR_MODE_STANDBY,
			.enabled = 0,
                        //.disabled = 1,
                },
                .initial_state = PM_SUSPEND_MEM,
        },
        .num_consumer_supplies = ARRAY_SIZE(smdk6450_vddarm_consumers),
        .consumer_supplies = smdk6450_vddarm_consumers,
};

static struct s5m8751_backlight_pdata smdk6450_backlight_pdata = {
        .brightness = 31,
};

static struct s5m8751_power_pdata smdk6450_power_pdata = {
        .constant_voltage = 4200,
        .fast_chg_current = 400,
        .gpio_hadp_lusb = S5P6450_GPN(15),
};

static struct s5m8751_audio_pdata smdk6450_audio_pdata = {
        .dac_vol = 0x18,
        .hp_vol = 0x14,
};
static struct s5m8751_pdata smdk6450_s5m8751_pdata = {
        .regulator = {
                &smdk6450_vddsys_ext,   /* LDO1 */
                &smdk6450_vddlcd,       /* LDO2 */
                &smdk6450_vddalive,     /* LDO3 */
                &smdk6450_vddpll,       /* LDO4 */
                &smdk6450_vddmem_ss,    /* LDO5 */
                NULL,
                &smdk6450_vddgps,       /* BUCK1 */
                NULL,
                &smdk6450_vddarm_int,   /* BUCK2 */
                NULL,
        },
        .backlight = &smdk6450_backlight_pdata,
        .power = &smdk6450_power_pdata,
        .audio = &smdk6450_audio_pdata,

        .irq_base = S5P_IRQ_EINT_BASE,
        .gpio_pshold = S5P6450_GPN(12),
};

#endif

#ifdef CONFIG_MFD_S5M8752
/* VDDARM */
static struct regulator_consumer_supply smdk6450_vddarm_consumers[] = {
        {
                .supply = "vddarm",
        }
};

static struct regulator_init_data smdk6450_vddarm = {
        .constraints = {
                .name = "VDD_ARM",
                .min_uV = 800000,
                .max_uV = 1575000,
                .always_on = 1,
                .valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
                .state_mem = {
                        .uV = 0,
                        .mode = REGULATOR_MODE_STANDBY,
                },
                .initial_state = PM_SUSPEND_MEM,
        },
        .num_consumer_supplies = ARRAY_SIZE(smdk6450_vddarm_consumers),
        .consumer_supplies = smdk6450_vddarm_consumers,
};

/* VDDINT */
static struct regulator_consumer_supply smdk6450_vddint_consumers[] = {
        {
                .supply = "vddint",
        }
};
static struct regulator_init_data smdk6450_vddint = {
        .constraints = {
                .name = "VDD_INT",
                .min_uV = 800000,
                .max_uV = 1575000,
                .always_on = 1,
                .valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
                .state_mem = {
                        .uV = 0,
                        .mode = REGULATOR_MODE_STANDBY,
                },
                .initial_state = PM_SUSPEND_MEM,
        },
        .num_consumer_supplies = ARRAY_SIZE(smdk6450_vddint_consumers),
        .consumer_supplies = smdk6450_vddint_consumers,
};

/* VDDQM1 */
static struct regulator_init_data smdk6450_vddqm1 = {
        .constraints = {
                .name = "VDD_QM1",
                .min_uV = 1800000,
                .max_uV = 1800000,
                .always_on = 1,
                .state_mem = {
                        .uV = 1800000,
                        .mode = REGULATOR_MODE_STANDBY,
                        .enabled = 1,
                },
                .initial_state = PM_SUSPEND_MEM,
        },
};

/* VDDALIVE */
static struct regulator_init_data smdk6450_vddalive = {
        .constraints = {
                .name = "VDD_ALIVE",
                .min_uV = 1100000,
                .max_uV = 1100000,
                .always_on = 1,
                .state_mem = {
                        .uV = 1100000,
                        .mode = REGULATOR_MODE_STANDBY,
                        .enabled = 1,
                },
                .initial_state = PM_SUSPEND_MEM,
        },
};
/* VDDPLL */
static struct regulator_init_data smdk6450_vddpll = {
        .constraints = {
                .name = "VDD_PLL",
                .min_uV = 1100000,
                .max_uV = 1100000,
                .always_on = 1,
                .state_mem = {
                        .uV = 0,
                        .mode = REGULATOR_MODE_STANDBY,
                },
                .initial_state = PM_SUSPEND_MEM,
        },
};

/* VDDGPS */
static struct regulator_init_data smdk6450_vddgps = {
        .constraints = {
                .name = "VDD_GPS",
                .min_uV = 1800000,
                .max_uV = 1800000,
                .always_on = 1,
                .state_mem = {
                        .uV = 0,
                        .mode = REGULATOR_MODE_STANDBY,
                },
                .initial_state = PM_SUSPEND_MEM,
        },
};

/* VDDMIPI18 */
static struct regulator_init_data smdk6450_vddmipi18 = {
        .constraints = {
                .name = "VDD_MIPI18",
                .min_uV = 1800000,
                .max_uV = 1800000,
                .always_on = 1,
                .state_mem = {
                        .uV = 0,
                        .mode = REGULATOR_MODE_STANDBY,
                },
                .initial_state = PM_SUSPEND_MEM,
        },
};
/* VDDSYS0 */
static struct regulator_init_data smdk6450_vddsys0 = {
        .constraints = {
                .name = "VDD_SYS0",
                .min_uV = 3300000,
                .max_uV = 3300000,
                .always_on = 1,
                .state_mem = {
                        .uV = 3300000,
                        .mode = REGULATOR_MODE_STANDBY,
                        .enabled = 1,
                },
                .initial_state = PM_SUSPEND_MEM,
        },
};

/* VDDMIPI11 */
static struct regulator_init_data smdk6450_vddmipi11 = {
        .constraints = {
                .name = "VDD_MIPI11",
                .min_uV = 1100000,
                .max_uV = 1100000,
                .always_on = 1,
                .state_mem = {
                        .uV = 0,
                        .mode = REGULATOR_MODE_STANDBY,
                },
                .initial_state = PM_SUSPEND_MEM,
        },
};

/* VDDOTG33 */
static struct regulator_init_data smdk6450_vddotg33 = {
        .constraints = {
                .name = "VDD_OTG33",
                .min_uV = 3300000,
                .max_uV = 3300000,
                .always_on = 1,
                .state_mem = {
                        .uV = 0,
                        .mode = REGULATOR_MODE_STANDBY,
                },
                .initial_state = PM_SUSPEND_MEM,
        },
};
/* S5M8752 Backlight platform data */
static struct s5m8752_backlight_pdata smdk6450_backlight_pdata = {
        .pwm_freq = 1200,
        .brightness = 20,
};

/* S5M8752 battery charger platform data */
static struct s5m8752_power_pdata smdk6450_power_pdata = {
        .charger_type = 0,
        //.charger_type = 0,
//        .current_limit = 1,
        .current_limit = 2,
        .gpio_adp = S5P6450_GPN(15),
        .gpio_ilim0 = S5P6450_GPN(14),
        .gpio_ilim1 = S5P6450_GPN(3),
};

/* S5M8752 SDC2, SDC3 dvs gpio information */
static struct s5m8752_dvs_pdata smdk6450_dvs_pdata = {
        .gpio_dvs2a = S5P6450_GPN(10),
        .gpio_dvs2b = S5P6450_GPN(9),
        .gpio_dvs3a = S5P6450_GPN(2),
        .gpio_dvs3b = S5P6450_GPN(4),
};

/* S5M8752 audio platform data */
static struct s5m8752_audio_pdata smdk6450_audio_pdata = {
        .linein_vol = 0x2,
        .adc_vol = 0x18,
        .dac_vol = 0x19,
        .lineout_vol = 0xC,
        .hp_preamp_vol = 0xC,
        .spk_preamp_vol = 0x2,
};
/* S5M8752 platform data optimized at SMDK6450 */
static struct s5m8752_pdata smdk6450_s5m8752_pdata = {
        .regulator = {
                NULL,
                &smdk6450_vddarm,       /* BUCK2 */
                &smdk6450_vddint,       /* BUCK3 */
                &smdk6450_vddqm1,       /* BUCK4 */
                &smdk6450_vddalive,     /* LDO_KA */
                &smdk6450_vddpll,       /* LDO1 */
                &smdk6450_vddgps,       /* LDO2 */
                &smdk6450_vddmipi18,    /* LDO3 */
                &smdk6450_vddsys0,      /* LDO4 */
                &smdk6450_vddmipi11,    /* LDO5 */
                NULL,
                &smdk6450_vddotg33,     /* LDO7 */
        },
        .backlight = &smdk6450_backlight_pdata,
        .power = &smdk6450_power_pdata,
        .audio = &smdk6450_audio_pdata,

        .irq_base = S5P_IRQ_EINT_BASE,
        .gpio_pshold = S5P6450_GPN(12),
        .dvs = &smdk6450_dvs_pdata,
};
#endif


static struct i2c_board_info i2c_devs0[] __initdata = {
	{ I2C_BOARD_INFO("24c08", 0x50), },
	{ I2C_BOARD_INFO("WM8580", 0x1b), },
};

static struct i2c_board_info i2c_devs1[] __initdata = {
	{ I2C_BOARD_INFO("24c128", 0x57), },	/* Samsung S524AD0XD1 */
//	{ I2C_BOARD_INFO("WM8580", 0x1a), },
//	{ I2C_BOARD_INFO("s5m8751", 0x68), },
#if defined(CONFIG_MFD_S5M8751)
/* S5M8751_SLAVE_ADDRESS >> 1 = 0xD0 >> 1 = 0x68 */
        { I2C_BOARD_INFO("s5m8751", (0xD0 >> 1)),
        .platform_data = &smdk6450_s5m8751_pdata,
        .irq = S5P_EINT(7),
        },
#if defined(CONFIG_TLV_3101)
        { I2C_BOARD_INFO("TLV320ADC3101", 0x18),
//        .platform_data = &smdk6450_tlv3101_pdata,
	},
#endif
#endif
#if defined(CONFIG_MFD_S5M8752)
	/* S5M8752_SLAVE_ADDRESS >> 1 = 0xD0 >> 1 = 0x68 */
        { I2C_BOARD_INFO("s5m8752", (0xD0 >> 1)),
        .platform_data = &smdk6450_s5m8752_pdata,
        .irq = S5P_EINT(7),
        },
#endif

};


static struct s3c_ts_mach_info s3c_ts_platform __initdata = {
	.delay 			= 10000,
	.presc 			= 49,
	.oversampling_shift	= 2,
	.resol_bit 		= 12,
	.s3c_adc_con		= ADC_TYPE_2,
};

static struct s3c_adc_mach_info s3c_adc_platform = {
	/* s3c6410 support 12-bit resolution */
	.delay	= 	10000,
	.presc 	= 	49,
	.resolution = 	12,
};

#if defined(CONFIG_HAVE_PWM)
static struct platform_pwm_backlight_data smdk_backlight_data = {
	.pwm_id		= 1,
	.max_brightness	= 255,
	.dft_brightness	= 255,
	.pwm_period_ns	= 78770,
};

static struct platform_device smdk_backlight_device = {
	.name		= "pwm-backlight",
	.dev		= {
		.parent = &s3c_device_timer[1].dev,
		.platform_data = &smdk_backlight_data,
	},
};

static void __init smdk_backlight_register(void)
{
	int ret = platform_device_register(&smdk_backlight_device);
	if (ret)
		printk(KERN_ERR "smdk: failed to register backlight device: %d\n", ret);
}
#else
#define smdk_backlight_register()	do { } while (0)
#endif
#ifdef CONFIG_S3C_DEV_MSHC
static struct s3c_mshci_platdata smdk6450_mshc_pdata __initdata = {
#if defined(CONFIG_S5P6450_SD_CH3_8BIT)
        .max_width              = 8,
        .host_caps              = MMC_CAP_8_BIT_DATA,
#endif
#if defined(CONFIG_S5P6450_SD_CH3_DDR)
        .host_caps              = MMC_CAP_DDR,
#endif
	.cd_type		= S3C_MSHCI_CD_GPIO,
};
#endif

void smdk6450_setup_sdhci0 (void);

static void __init smdk6450_map_io(void)
{
	int xfsel=0;

	s5p_init_io(smdk6450_iodesc, ARRAY_SIZE(smdk6450_iodesc), S5P_SYS_ID);
	s3c_device_nand.name = "s5p6450-nand";

	xfsel = __raw_readl(S5P_VA_CHIPID+0x8);

	if((xfsel >> 24) &0x1){
		base_clk = 19;
		s3c24xx_init_clocks(19200000);
#if defined(CONFIG_XXTI_12_MHZ)
		otg_phy_clk_value =  0x21; 		/* [6:5] External clcok connected to XO pin: [1:0] Ref clk freq for PLL 19.2Mhz */
#elif defined(CONFIG_XXTI_19_MHZ)
		otg_phy_clk_value =  0x61; 		/* [6:5] External clcok connected to XXTI pin: [1:0] Ref clk freq for PLL 19.2Mhz */
#endif
	}
	else{
		base_clk = 12;
		s3c24xx_init_clocks(12000000);
		otg_phy_clk_value = 0x62;		/* [6:5] On chip clock connected to CLKCORE:  [1:0] Ref clk freq for PLL 12 MHz */ 
	}
	
	s3c24xx_init_uarts(smdk6450_uartcfgs, ARRAY_SIZE(smdk6450_uartcfgs));
#if defined(CONFIG_SERIAL_S5P6450_HSUART)
        writel((readl(S5P_CLK_DIV2) & ~(0x000f0000)) | 0x00040000, S5P_CLK_DIV2);
#endif
	s3c64xx_reserve_bootmem();

	smdk6450_setup_sdhci0();
} 
	
static void __init android_pmem_set_platdata(void)
{
#ifdef CONFIG_ANDROID_PMEM 
        pmem_pdata.start = (u32)s3c_get_media_memory(S3C_MDEV_PMEM);
        pmem_pdata.size = (u32)s3c_get_media_memsize(S3C_MDEV_PMEM);

        pmem_gpu1_pdata.start = (u32)s3c_get_media_memory(S3C_MDEV_PMEM_GPU1);
        pmem_gpu1_pdata.size = (u32)s3c_get_media_memsize(S3C_MDEV_PMEM_GPU1);

        pmem_adsp_pdata.start = (u32)s3c_get_media_memory(S3C_MDEV_PMEM_ADSP);
        pmem_adsp_pdata.size = (u32)s3c_get_media_memsize(S3C_MDEV_PMEM_ADSP);
#endif /* CONFIG_ANDROID_PMEM */
}

static void __init smdk6450_smc911x_set(void)
{ 
#if 0  //Need to enable on requirement
	unsigned int tmp;

	tmp = __raw_readl(S5P64XX_SROM_BW);
	tmp &= ~(S5P64XX_SROM_BW_WAIT_ENABLE1_MASK | S5P64XX_SROM_BW_WAIT_ENABLE1_MASK |
		S5P64XX_SROM_BW_DATA_WIDTH1_MASK);
	tmp |= S5P64XX_SROM_BW_BYTE_ENABLE1_ENABLE | S5P64XX_SROM_BW_WAIT_ENABLE1_ENABLE |
		S5P64XX_SROM_BW_DATA_WIDTH1_16BIT;

	__raw_writel(tmp, S5P64XX_SROM_BW);

	__raw_writel(S5P64XX_SROM_BCn_TACS(0) | S5P64XX_SROM_BCn_TCOS(4) |
			S5P64XX_SROM_BCn_TACC(13) | S5P64XX_SROM_BCn_TCOH(1) |
			S5P64XX_SROM_BCn_TCAH(4) | S5P64XX_SROM_BCn_TACP(6) |
			S5P64XX_SROM_BCn_PMC_NORMAL, S5P64XX_SROM_BC1);
#endif
}
static void __init smdk6450_fixup(struct machine_desc *desc,
                                        struct tag *tags, char **cmdline,
                                        struct meminfo *mi)
{
        mi->bank[0].start = 0x20000000;
        mi->bank[0].size = 256 * SZ_1M;
        mi->bank[0].node = 0;
        mi->nr_banks = 1;
}

#define S5K4BA_ENABLED
static int smdk6450_cam1_power(int onoff)
{
        int err;

        /* Camera B */
        err = gpio_request(S5P6450_GPQ(1), "GPH0");
        if (err)
                printk(KERN_ERR "#### failed to request GPH0 for CAM_2V8\n");

        s3c_gpio_setpull(S5P6450_GPQ(1), S3C_GPIO_PULL_NONE);
        gpio_direction_output(S5P6450_GPQ(1), 0);
        gpio_direction_output(S5P6450_GPQ(1), 1);
        gpio_free(S5P6450_GPQ(1));

        return 0;
}


#ifdef S5K4BA_ENABLED
static struct s5k4ba_platform_data s5k4ba_plat = {
        .default_width = 1600,
        .default_height = 1200,
        .pixelformat = V4L2_PIX_FMT_RGB565,
        .freq = 44000000,
        .is_mipi = 0,
};

static struct i2c_board_info  s5k4ba_i2c_info = {
        I2C_BOARD_INFO("S5K4BA", 0x2d),
        .platform_data = &s5k4ba_plat,
};

static struct s3c_platform_camera s5k4ba = {
        .id             = CAMERA_PAR_A,
        .type           = CAM_TYPE_ITU,
        .fmt            = ITU_656_YCBCR422_8BIT,
        .order422       = CAM_ORDER422_8BIT_CBYCRY,
        .i2c_busnum     = 0,
        .info           = &s5k4ba_i2c_info,
        .pixelformat    = V4L2_PIX_FMT_RGB565,
        .srclk_name     = "dout_mpll",
        .clk_name       = "sclk_camif",
        .clk_rate       = 44000000,
        .line_length    = 1600,
        .width          = 1600,
        .height         = 1200,
        .window         = {
        .left   = 0,
                .top    = 0,
                .width  = 1600,
                .height = 1200,
        },

        /* Polarity */
        .inv_pclk       = 0,
        .inv_vsync      = 1,
        .inv_href       = 0,
        .inv_hsync      = 0,

        .initialized    = 0,
        .cam_power      = smdk6450_cam1_power,
};
#endif




static struct s3c_platform_fimc fimc_plat = {
        .srclk_name     = "dout_mpll",
//        .clk_name       = "fimc",
        .clk_name       = "sclk_fimc",
        .clk_rate       = 166000000,

        .default_cam    = CAMERA_PAR_A,
        
        .camera         = {
                &s5k4ba,
        //        &writeback,
        },
        .hw_ver         = 0x43,
};

static void __init smdk6450_machine_init(void)
{
	s3c_device_nand.dev.platform_data = &s3c_nand_mtd_part_info;

	smdk6450_smc911x_set();

	s3c_i2c0_set_platdata(NULL);
	s3c_i2c1_set_platdata(NULL);

#ifdef CONFIG_S3C_DEV_MSHC
        s5p6450_default_mshci();
#endif

	s3c_ts_set_platdata(&s3c_ts_platform);
	s3c_adc_set_platdata(&s3c_adc_platform);

	i2c_register_board_info(0, i2c_devs0, ARRAY_SIZE(i2c_devs0));
	i2c_register_board_info(1, i2c_devs1, ARRAY_SIZE(i2c_devs1));

#if defined(CONFIG_SPI_CNTRLR_0)
	s3cspi_set_slaves(BUSNUM(0), ARRAY_SIZE(s3c_slv_pdata_0), s3c_slv_pdata_0);
#endif
#if defined(CONFIG_SPI_CNTRLR_1)
	s3cspi_set_slaves(BUSNUM(1), ARRAY_SIZE(s3c_slv_pdata_1), s3c_slv_pdata_1);
#endif
	spi_register_board_info(s3c_spi_devs, ARRAY_SIZE(s3c_spi_devs));

/* fb */
#ifdef CONFIG_FB_S3C
	s3cfb_set_platdata(NULL);
#endif
#ifdef CONFIG_ANDROID_PMEM
	android_pmem_set_platdata();
#endif
	s3c_fimc0_set_platdata(&fimc_plat);

#ifdef CONFIG_S3C_DEV_MSHC
	s3c_mshci_set_platdata(&smdk6450_mshc_pdata);
#endif
	platform_add_devices(smdk6450_devices, ARRAY_SIZE(smdk6450_devices));

	//s5p6440_pm_init();
	s3c_pm_init();

	smdk_backlight_register();

}

MACHINE_START(SMDK6450, "SMDK6450")
	/* Maintainer: Kukjin Kim <kgene.kim@samsung.com> */
	.phys_io	= S3C_PA_UART & 0xfff00000,
	.io_pg_offst	= (((u32)S3C_VA_UART) >> 18) & 0xfffc,
	.boot_params	= S5P_PA_SDRAM + 0x100,
	.fixup          = smdk6450_fixup,
	.init_irq	= s5p6450_init_irq,
	.map_io		= smdk6450_map_io,
	.init_machine	= smdk6450_machine_init,
#ifdef CONFIG_S5P_HIGH_RES_TIMERS
	.timer		= &s5p_systimer,
#else
	.timer		= &s3c64xx_timer,
#endif
MACHINE_END

#ifdef  CONFIG_USB_SUPPORT
void usb_host_phy_init(void)
{
        struct clk *otg_clk;

        otg_clk = clk_get(NULL, "usbotg");
        clk_enable(otg_clk);

        if (readl(S5P_OTHERS) & (0x1<<16))
                return;

        __raw_writel(__raw_readl(S5P_OTHERS)
                |(0x1<<16), S5P_OTHERS);
	
	__raw_writel(__raw_readl(S5P_OTHERS)
                |(0x0<<17), S5P_OTHERS);

        __raw_writel((__raw_readl(S3C_USBOTG_PHYPWR)
                &~(0x1<<7)&~(0x1<<6))|(0x1<<8)|(0x1<<5), S3C_USBOTG_PHYPWR);
        __raw_writel((__raw_readl(S3C_USBOTG_PHYCLK)
                &~(0x1<<7))|(0x1<<0), S3C_USBOTG_PHYCLK);
        __raw_writel((__raw_readl(S3C_USBOTG_RSTCON))
                |(0x1<<4)|(0x1<<3), S3C_USBOTG_RSTCON);
        __raw_writel(__raw_readl(S3C_USBOTG_RSTCON)
                &~(0x1<<4)&~(0x1<<3), S3C_USBOTG_RSTCON);
}
EXPORT_SYMBOL(usb_host_phy_init);

void usb_host_phy_off(void)
{
        __raw_writel(__raw_readl(S3C_USBOTG_PHYPWR)
                |(0x1<<7)|(0x1<<6), S3C_USBOTG_PHYPWR);
        __raw_writel(__raw_readl(S5P_OTHERS)
                &~(1<<16), S5P_OTHERS);
}
EXPORT_SYMBOL(usb_host_phy_off);

/* Initializes OTG Phy. */
void otg_phy_init(void) {

	int err;
#if 0
	if (gpio_is_valid(S5P64XX_GPN(1))) {
		err = gpio_request(S5P64XX_GPN(1), "GPN");

		if(err)
			printk(KERN_ERR "failed to request GPN1\n");

		gpio_direction_output(S5P64XX_GPN(1), 1);
	}
#endif
    writel(readl(S5P_OTHERS)&~S5P_OTHERS_USB_SIG_MASK, S5P_OTHERS);
	writel(readl(S3C_USBOTG_PHYPWR) & ~0x1F, S3C_USBOTG_PHYPWR);		/* Power up */
        writel(otg_phy_clk_value, S3C_USBOTG_PHYCLK);
	writel(0x1, S3C_USBOTG_RSTCON);

	udelay(50);
	writel(0x0, S3C_USBOTG_RSTCON);
	udelay(50);
}
EXPORT_SYMBOL(otg_phy_init);

/* USB Control request data struct must be located here for DMA transfer */
//struct usb_ctrlrequest usb_ctrl __attribute__((aligned(8)));
//EXPORT_SYMBOL(usb_ctrl);

/* OTG PHY Power Off */
void otg_phy_off(void) {
	writel(readl(S3C_USBOTG_PHYPWR)|(0x1F<<1), S3C_USBOTG_PHYPWR);
	writel(readl(S5P_OTHERS)&~S5P_OTHERS_USB_SIG_MASK, S5P_OTHERS);

//	gpio_free(S5P64XX_GPN(1));
}
EXPORT_SYMBOL(otg_phy_off);

#endif

#if defined(CONFIG_RTC_DRV_S3C)
/* RTC common Function for samsung APs*/
unsigned int s3c_rtc_set_bit_byte(void __iomem *base, uint offset, uint val)
{
	writeb(val, base + offset);

	return 0;
}

unsigned int s3c_rtc_read_alarm_status(void __iomem *base)
{
	return 1;
}

void s3c_rtc_set_pie(void __iomem *base, uint to)
{
	unsigned int tmp;

	tmp = readw(base + S3C2410_RTCCON) & ~S3C_RTCCON_TICEN;

        if (to)
                tmp |= S3C_RTCCON_TICEN;

        writew(tmp, base + S3C2410_RTCCON);
}

void s3c_rtc_set_freq_regs(void __iomem *base, uint freq, uint s3c_freq)
{
	unsigned int tmp;

        tmp = readw(base + S3C2410_RTCCON) & (S3C_RTCCON_TICEN | S3C2410_RTCCON_RTCEN );
        writew(tmp, base + S3C2410_RTCCON);
        s3c_freq = freq;
        tmp = (32768 / freq)-1;
        writel(tmp, base + S3C2410_TICNT);
}

void s3c_rtc_enable_set(struct platform_device *pdev,void __iomem *base, int en)
{
	unsigned int tmp;

	if (!en) {
		tmp = readw(base + S3C2410_RTCCON);
		writew(tmp & ~ (S3C2410_RTCCON_RTCEN | S3C_RTCCON_TICEN), base + S3C2410_RTCCON);
	} else {
		/* re-enable the device, and check it is ok */
		if ((readw(base+S3C2410_RTCCON) & S3C2410_RTCCON_RTCEN) == 0){
			dev_info(&pdev->dev, "rtc disabled, re-enabling\n");

			tmp = readw(base + S3C2410_RTCCON);
			writew(tmp|S3C2410_RTCCON_RTCEN, base+S3C2410_RTCCON);
		}

		if ((readw(base + S3C2410_RTCCON) & S3C2410_RTCCON_CNTSEL)){
			dev_info(&pdev->dev, "removing RTCCON_CNTSEL\n");

			tmp = readw(base + S3C2410_RTCCON);
			writew(tmp& ~S3C2410_RTCCON_CNTSEL, base+S3C2410_RTCCON);
		}

		if ((readw(base + S3C2410_RTCCON) & S3C2410_RTCCON_CLKRST)){
			dev_info(&pdev->dev, "removing RTCCON_CLKRST\n");

			tmp = readw(base + S3C2410_RTCCON);
			writew(tmp & ~S3C2410_RTCCON_CLKRST, base+S3C2410_RTCCON);
		}
	}
}
#endif

void s3c_setup_uart_cfg_gpio(unsigned char port)
{
        switch(port)
        {
        case 0: 
                s3c_gpio_cfgpin(S5P64XX_GPA(0), S3C_GPIO_SFN(2));
                s3c_gpio_setpull(S5P64XX_GPA(0), S3C_GPIO_PULL_NONE);
                s3c_gpio_cfgpin(S5P64XX_GPA(1), S3C_GPIO_SFN(2));
                s3c_gpio_setpull(S5P64XX_GPA(1), S3C_GPIO_PULL_NONE);
                s3c_gpio_cfgpin(S5P64XX_GPA(2), S3C_GPIO_SFN(2));
                s3c_gpio_setpull(S5P64XX_GPA(2), S3C_GPIO_PULL_NONE);
                s3c_gpio_cfgpin(S5P64XX_GPA(3), S3C_GPIO_SFN(2));
                s3c_gpio_setpull(S5P64XX_GPA(3), S3C_GPIO_PULL_NONE);
                break;
        case 1:
                s3c_gpio_cfgpin(S5P64XX_GPA(4), S3C_GPIO_SFN(2));
                s3c_gpio_setpull(S5P64XX_GPA(4), S3C_GPIO_PULL_NONE);
                s3c_gpio_cfgpin(S5P64XX_GPA(5), S3C_GPIO_SFN(2));
                s3c_gpio_setpull(S5P64XX_GPA(5), S3C_GPIO_PULL_NONE);
                s3c_gpio_cfgpin(S5P64XX_GPB(5), S3C_GPIO_SFN(3));
                s3c_gpio_setpull(S5P64XX_GPB(5), S3C_GPIO_PULL_NONE);
                s3c_gpio_cfgpin(S5P64XX_GPB(6), S3C_GPIO_SFN(3));
                s3c_gpio_setpull(S5P64XX_GPB(6), S3C_GPIO_PULL_NONE);
                break;
	case 2:
                s3c_gpio_cfgpin(S5P64XX_GPB(0), S3C_GPIO_SFN(2));
                s3c_gpio_setpull(S5P64XX_GPB(0), S3C_GPIO_PULL_NONE);
                s3c_gpio_cfgpin(S5P64XX_GPB(1), S3C_GPIO_SFN(2));
                s3c_gpio_setpull(S5P64XX_GPB(1), S3C_GPIO_PULL_NONE);
                break;
        case 3:
                s3c_gpio_cfgpin(S5P64XX_GPB(2), S3C_GPIO_SFN(2));
                s3c_gpio_setpull(S5P64XX_GPB(2), S3C_GPIO_PULL_NONE);
                s3c_gpio_cfgpin(S5P64XX_GPB(3), S3C_GPIO_SFN(2));
                s3c_gpio_setpull(S5P64XX_GPB(3), S3C_GPIO_PULL_NONE);
                break;
	case 4:
                s3c_gpio_cfgpin(S5P64XX_GPD(0), S3C_GPIO_SFN(2));
                s3c_gpio_setpull(S5P64XX_GPD(0), S3C_GPIO_PULL_NONE);
                s3c_gpio_cfgpin(S5P64XX_GPD(1), S3C_GPIO_SFN(2));
                s3c_gpio_setpull(S5P64XX_GPD(1), S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(S5P64XX_GPD(4), S3C_GPIO_SFN(2));
                s3c_gpio_setpull(S5P64XX_GPD(4), S3C_GPIO_PULL_NONE);
                s3c_gpio_cfgpin(S5P64XX_GPD(5), S3C_GPIO_SFN(2));
                s3c_gpio_setpull(S5P64XX_GPD(5), S3C_GPIO_PULL_NONE);
                break;
        case 5:
                s3c_gpio_cfgpin(S5P64XX_GPD(2), S3C_GPIO_SFN(2));
                s3c_gpio_setpull(S5P64XX_GPD(2), S3C_GPIO_PULL_NONE);
                s3c_gpio_cfgpin(S5P64XX_GPD(3), S3C_GPIO_SFN(2));
                s3c_gpio_setpull(S5P64XX_GPD(3), S3C_GPIO_PULL_NONE);
		s3c_gpio_cfgpin(S5P64XX_GPD(6), S3C_GPIO_SFN(3));
                s3c_gpio_setpull(S5P64XX_GPD(6), S3C_GPIO_PULL_NONE);
                s3c_gpio_cfgpin(S5P64XX_GPD(7), S3C_GPIO_SFN(3));
                s3c_gpio_setpull(S5P64XX_GPD(7), S3C_GPIO_PULL_NONE);
                break;

        default:
                break;
        }
}
EXPORT_SYMBOL(s3c_setup_uart_cfg_gpio);
