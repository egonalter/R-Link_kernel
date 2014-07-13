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
#include <linux/gpio.h>
#include <linux/vgpio.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/pwm_backlight.h>
#include <mach/spi.h>

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

#include <plat/fb.h>

#include <plat/regs-otg.h>
#include <plat/s5p6450.h>
#include <plat/clock.h>
#include <mach/regs-clock.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <plat/mendoza.h>
#include <plat/fdt.h>
#include <plat/mendoza-lcd.h>
#include <plat/mendoza-lcd-data.h>

#include <plat/adc.h>
#include <plat/pm.h>
#include <plat/pll.h>
#include <plat/regs-rtc.h>
#include <plat/media.h>
#include <mach/gpio.h>
#include <mach/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/gpio-bank-c.h>
#include <plat/gpio-bank-n.h>
#include <plat/gpio-bank-f.h>
#include <plat/mshci.h>

#include <linux/regulator/machine.h>
#include <linux/mfd/s5m8751/core.h>
#include <linux/mfd/s5m8751/pdata.h>

#define CONFIG_TLV_3101  1

#include <plat/regs-otg.h>
#define OTGH_PHY_CLK_VALUE      (0x21)  /* UTMI Interface, Cristal, 12Mhz clk for PLL */
#include <linux/usb/ch9.h>

/*pmem */
#include <linux/android_pmem.h>

/* S3C_USB_CLKSRC 0: EPLL 1: CLK_48M */
#define S3C_USB_CLKSRC  1

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
	.dev = { 
		.platform_data = &pmem_gpu1_pdata 
	},
};

static struct platform_device pmem_adsp_device = {
	.name = "android_pmem",
	.id = 2,
	.dev = { 
		.platform_data = &pmem_adsp_pdata 
	},
};

#endif //PMEM

static struct s3c2410_uartcfg smdk6450_uartcfgs[] __initdata = {
	[0] = {
		.hwport		 = 0,
		.flags		 = 0,
		.ucon		 = S5P6450_UCON_DEFAULT,
		.ulcon		 = S5P6450_ULCON_DEFAULT,
		.ufcon		 = S5P6450_UFCON_DEFAULT,
	},
	[1] = {
		.hwport		 = 1,
		.flags		 = 0,
		.ucon		 = S5P6450_UCON_DEFAULT,
		.ulcon		 = S5P6450_ULCON_DEFAULT,
		.ufcon		 = S5P6450_UFCON_DEFAULT,
	},
	[2] = {
		.hwport		 = 2,
		.flags		 = 0,
		.ucon		 = S5P6450_UCON_DEFAULT,
		.ulcon		 = S5P6450_ULCON_DEFAULT,
		.ufcon		 = S5P6450_UFCON_DEFAULT,
	},
	[3] = {
		.hwport		 = 3,
		.flags		 = 0,
		.ucon		 = S5P6450_UCON_DEFAULT,
		.ulcon		 = S5P6450_ULCON_DEFAULT,
		.ufcon		 = S5P6450_UFCON_DEFAULT,
	},

		[4] = { 
				.hwport      = 4,
				.flags	     = 0,
				.ucon	     = S5P6450_UCON_DEFAULT,
				.ulcon	     = S5P6450_ULCON_DEFAULT,
				.ufcon	     = S5P6450_UFCON_DEFAULT,
		},
		[5] = {
				.hwport      = 5,
				.flags	     = 0,
				.ucon	     = S5P6450_UCON_DEFAULT,
				.ulcon	     = S5P6450_ULCON_DEFAULT,
				.ufcon	     = S5P6450_UFCON_DEFAULT,
		},
};

struct platform_device taranto_sec_device_battery = {
	.name	= "sec-fake-battery",
	.id	= -1,
};

struct map_desc taranto_iodesc[] = {};

/* SD slot connects to channel 2/3 which is switchable
   ch 2 is sd/mmc (sdhci), ch 3 is eMMC (mshci)     */
static struct platform_device sd_pwr_en_device = {
	.name	= "sd_pwr_en",
	.id 	= -1,
};

static struct platform_device taranto_suicide_alarm = {
	.name 	= "suicide_alarm",
	.id	= -1
};

static struct platform_device *smdk6450_devices[] __initdata = {
	&s3c_device_i2c1,
	&s3c_device_i2c0,
#ifdef CONFIG_FB_S3C
	&s3c_device_fb,
#endif
#ifdef CONFIG_SMDK6450_SD_CH0
	&s3c_device_hsmmc0,
#endif
#ifdef CONFIG_SMDK6450_SD_CH1
	&s3c_device_hsmmc1,
#endif
#ifdef CONFIG_SMDK6450_SD_CH2
	&s3c_device_hsmmc2,
#endif
#ifdef CONFIG_S3C_DEV_MSHC
	&s3c_device_mshci,
#endif
	&s3c_device_wdt,
#ifdef CONFIG_ANDROID_PMEM
	&pmem_device,
	&pmem_gpu1_device,
	&pmem_adsp_device,
#endif
	&s5p_device_rtc,
#ifdef CONFIG_SPI_CNTRLR_0
	&s3c_device_spi0,
#endif
#ifdef CONFIG_SPI_CNTRLR_1
	&s3c_device_spi1,
#endif
	&s3c_device_lcd,
	&s3c_device_nand,
	&s3c_device_usbgadget,
	&s3c_device_usb_otghcd,
#ifdef CONFIG_USB
	&s3c_device_usb_ehci,
	&s3c_device_usb_ohci,
#endif
#ifdef CONFIG_SND_S3C24XX_SOC
	&s3c64xx_device_iis2,
#endif
#ifdef CONFIG_SND_S3C_SOC_AC97
	&s3c64xx_device_ac97,
#endif
#ifdef CONFIG_SND_S3C_SOC_PCM
	&s3c64xx_device_pcm1,
#endif
	&s3c_device_g2d,

	&s3c_device_android_usb,
	&s3c_device_usb_mass_storage,
	&s3c_device_rndis,
	&s3c_device_adc,
#ifdef CONFIG_HAVE_PWM
	&s3c_device_timer[0],
	&s3c_device_timer[1],
#endif
#ifdef CONFIG_KEYPAD_S3C
	&s3c_device_keypad,
#endif
	&taranto_sec_device_battery,

#ifdef CONFIG_GIB_S5P64XX
	&s3c_device_gib,
#endif
#ifdef CONFIG_TOMTOM_TT_SETUP
	&tomtom_device_ttsetup,
#endif
#ifdef CONFIG_TOMTOM_SUICIDE_ALARM
	&taranto_suicide_alarm,
#endif
	&sd_pwr_en_device
};

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
			.disabled = 0,
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
			.disabled = 1,
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
			.disabled = 0,
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
			.disabled = 1,
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
			.disabled = 0,
		},
		.initial_state = PM_SUSPEND_MEM,
	},
};

/* AUDIO */
static struct regulator_init_data smdk6450_vdd_audio = {
	.constraints = {
		.name = "VDD_AUDIO",
		.min_uV = 1800000,
		.max_uV = 3300000,
		.always_on = 1,
		.state_mem = {
			.uV = 0,
			.mode = REGULATOR_MODE_STANDBY,
			.enabled = 0,
			.disabled = 1,
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
			.enabled = 1,
			.disabled = 0,
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

/* INT */
static struct regulator_init_data smdk6450_vddarm_int = {
        .constraints = {
                .name = "PVDD_ARM/INT",
//		.name = "vddint",
                .min_uV = 1000000,
                .max_uV = 1300000,
                .always_on = 1,
                .valid_ops_mask = REGULATOR_CHANGE_VOLTAGE,
                .state_mem = {
                        .uV = 0,
                        .mode = REGULATOR_MODE_STANDBY,
			.enabled = 0,
			.disabled = 1,
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
	.gpio_hadp_lusb = S5P6450_GPN(9),
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
		&smdk6450_vdd_audio,    /* LDO6 */
		&smdk6450_vddgps,       /* BUCK1 */
		NULL,
		&smdk6450_vddarm_int,   /* BUCK2 */
		NULL,
	},
	.backlight = &smdk6450_backlight_pdata,
	.power = &smdk6450_power_pdata,
	.audio = &smdk6450_audio_pdata,

//	.irq_base = S5P_IRQ_EINT_BASE,
	.irq_base = 12,
	.gpio_pshold = S5P6450_GPN(12),
};

static struct i2c_board_info i2c_devs0[] __initdata = {
};

static struct i2c_board_info i2c_devs1[] __initdata = {
/* S5M8751_SLAVE_ADDRESS >> 1 = 0xD0 >> 1 = 0x68 */
	{ I2C_BOARD_INFO("s5m8751", (0xD0 >> 1)),
	.platform_data = &smdk6450_s5m8751_pdata,
	.irq = S5P_EINT(1),
	},
};

static struct s3c_adc_mach_info s3c_adc_platform = {
	/* s3c6410 support 12-bit resolution */
	.delay	=	10000,
	.presc	=	49,
	.resolution =	12,
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
	.max_width		        = 8,
	.host_caps		        = MMC_CAP_8_BIT_DATA,
#endif
#if defined(CONFIG_S5P6450_SD_CH3_DDR)
	.host_caps		        = MMC_CAP_DDR,
#endif
	.cd_type			= S3C_MSHCI_CD_INTERNAL,
};
#endif

void smdk6450_setup_sdhci0 (void);

static void __init smdk6450_map_io(void)
{
	s5p_init_io(taranto_iodesc, ARRAY_SIZE(taranto_iodesc), S5P_SYS_ID);
	s3c_device_nand.name = "s5p6450-nand";
	//s5p64xx_init_io(taranto_iodesc, ARRAY_SIZE(taranto_iodesc));
	s3c24xx_init_clocks(19200000);
	s3c24xx_init_uarts(smdk6450_uartcfgs, ARRAY_SIZE(smdk6450_uartcfgs));
#if defined(CONFIG_SERIAL_S5P6450_HSUART)
	writel((readl(S5P_CLK_DIV2) & ~(0x000f0000)) | 0x00040000, S5P_CLK_DIV2);
#endif

	/* this has to be done before any device bootmem allocation */
	fdt_reserve_bootmem();
	s3c64xx_reserve_bootmem();

	smdk6450_setup_sdhci0();
}

static void __init android_pmem_set_platdata(void)
{
	pmem_pdata.start = (u32)s3c_get_media_memory(S3C_MDEV_PMEM);
	pmem_pdata.size = (u32)s3c_get_media_memsize(S3C_MDEV_PMEM);

	pmem_gpu1_pdata.start = (u32)s3c_get_media_memory(S3C_MDEV_PMEM_GPU1);
	pmem_gpu1_pdata.size = (u32)s3c_get_media_memsize(S3C_MDEV_PMEM_GPU1);

	pmem_adsp_pdata.start = (u32)s3c_get_media_memory(S3C_MDEV_PMEM_ADSP);
	pmem_adsp_pdata.size = (u32)s3c_get_media_memsize(S3C_MDEV_PMEM_ADSP);
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

// Some macros, just to save some typing...
#define TT_DEF_PIN0(p,t)         VGPIO_DEF_PIN(TT_VGPIO0_BASE, TT_VGPIO_##p, t)
#define TT_DEF_INVPIN0(p,t)      VGPIO_DEF_INVPIN(TT_VGPIO0_BASE, TT_VGPIO_##p, t)
#define TT_DEF_NCPIN0(p)         VGPIO_DEF_NCPIN(TT_VGPIO0_BASE, TT_VGPIO_##p)

// Pin tables
static struct vgpio_pin taranto_vgpio0_pins[] = {
	TT_DEF_INVPIN0(WIFI_RST, S5P6450_GPP(3)),
	TT_DEF_NCPIN0(TILT_PWR),
	TT_DEF_NCPIN0(TILT_OUT),
	TT_DEF_NCPIN0(TP_RESET),
	TT_DEF_NCPIN0(TP_IRQ),
	TT_DEF_INVPIN0(ON_OFF, 	S5P6450_GPN(4)),
	TT_DEF_PIN0(RFS_BOOT_CLK, S5P6450_GPQ(8)),
	TT_DEF_PIN0(RFS_BOOT_Q, S5P6450_GPQ(9)),
	TT_DEF_PIN0(EEPROM_WP,	S5P6450_GPB(2)),
	TT_DEF_INVPIN0(MIC_RESET, S5P6450_GPF(15)),
	TT_DEF_PIN0(AMP_PWR_EN, S5P6450_GPN(8)),
	TT_DEF_PIN0(MIC_AMP_EN, S5P6450_GPF(14)),
	TT_DEF_PIN0(PU_I2C1, 	S5P6450_GPB(3)),
	TT_DEF_NCPIN0(WALL_ON),
	TT_DEF_NCPIN0(HRESET),
	TT_DEF_PIN0(PWR_KILL, 	S5P6450_GPN(12)),
	TT_DEF_NCPIN0(BOOT_DEV0),
	TT_DEF_PIN0(LCM_PWR_EN, S5P6450_GPN(7)),
	TT_DEF_PIN0(DISP_ON, 	S5P6450_GPN(10)),
	TT_DEF_NCPIN0(LCM_CS),
	TT_DEF_NCPIN0(CHARGING),
	TT_DEF_INVPIN0(USB_HOST_DETECT, S5P6450_GPN(5)),
	TT_DEF_NCPIN0(GPS_RESET),
	TT_DEF_NCPIN0(GPS_1PPS),
	TT_DEF_INVPIN0(BT_RST,	S5P6450_GPP(6)),
	TT_DEF_PIN0(BT_EN,		S5P6450_GPP(5)),
	TT_DEF_NCPIN0(SD_BASE),
	TT_DEF_NCPIN0(SD_BOOT_BASE),
	TT_DEF_NCPIN0(LCD_SPI_CS),
	TT_DEF_NCPIN0(LCD_SPI_SDI),
	TT_DEF_NCPIN0(LCD_SPI_SCL),
	TT_DEF_NCPIN0(GPS_STANDBY),
	TT_DEF_NCPIN0(MIC_STBY),
	TT_DEF_NCPIN0(DOCK_DET_PWR),
	TT_DEF_NCPIN0(DOCK_RESET),
	TT_DEF_NCPIN0(DOCK_DET0),
	TT_DEF_NCPIN0(DOCK_DET1),
	TT_DEF_NCPIN0(DOCK_I2C_EN),
	TT_DEF_NCPIN0(DOCK_FM_INT),
	TT_DEF_NCPIN0(PU_I2C0),
	TT_DEF_PIN0(SD_PWR_EN,S5P6450_GPI(9)),
	TT_DEF_NCPIN0(MUTE),
	TT_DEF_NCPIN0(SD_CD),
	TT_DEF_NCPIN0(ACCESSORY_PWR_EN),
	TT_DEF_NCPIN0(GSM_SYS_EN),
	TT_DEF_NCPIN0(GSM_SYS_RST),

};

// Platform data & device
static struct vgpio_platform_data taranto_vgpio_pdata[] = {
	[0] = {
        	.gpio_base      = TT_VGPIO0_BASE,
        	.gpio_number    = ARRAY_SIZE(taranto_vgpio0_pins),
        	.pins           = taranto_vgpio0_pins,
	},
};

static struct platform_device taranto_vgpio[] = {
	[0] = {
        	.name           = "vgpio",
        	.id             = 0,
        	.dev            = {
                .platform_data  = &taranto_vgpio_pdata[0],
        	},
	},
};

static void __init taranto_lcd_init(void)
{
	mendoza_lcd_register(&LMS430WQV);
	mendoza_lcd_register(&LMS500HF04);
	mendoza_lcd_register(&LMS500HF13);
}

static void s5p6450_gpio_sleep_config(void)
{
	int tmp = 0 ;
	
/*--------Below are the sleep mode configurations for HRESETB singal (GPR6)----------*/

	tmp = __raw_readl(S5P64XX_GPRCON0);
	tmp |= (1<<28);
        __raw_writel(tmp,S5P64XX_GPRCON0);

	tmp = __raw_readl(S5P64XX_GPRDAT);
	tmp |= (1<<6);
        __raw_writel(tmp,S5P64XX_GPRDAT);
	
	tmp = __raw_readl(S5P64XX_GPRPUD);
	tmp |= (1<<13);
	tmp &= ~(1<<12);
        __raw_writel(tmp,S5P64XX_GPRPUD);

	tmp = __raw_readl(S5P64XX_GPRCONSLP);
	tmp |= (1<<12);
	tmp &= ~(1<<13);
        __raw_writel(tmp,S5P64XX_GPRCONSLP);

        tmp = __raw_readl(S5P64XX_GPRPUDSLP);
	tmp |= (1<<13);
	tmp &= ~(1<<12);
        __raw_writel(tmp,S5P64XX_GPRPUDSLP);

/*--------Below are the sleep mode configurations for eMMC singal (GPH0-9)--*/

	/* output low so we dont leak into vdd lines */
	tmp = __raw_readl(S5P64XX_GPHCONSLP);
	tmp &= ~0x000FFFFF; 
        __raw_writel(tmp,S5P64XX_GPHCONSLP);
}

static void s5m8751_pmic_poweroff(void)
{
	int tmp;

	/* switch from PSHOLD to gpio function on gpn12 */
	tmp = __raw_readl(S5P6450_PSHOLD_CON);
	tmp &= ~0x1;
	__raw_writel(tmp,S5P6450_PSHOLD_CON);

	/* movinand sync to disk delay */
	mdelay(500);
	gpio_direction_output(TT_VGPIO_PWR_KILL, 0);
}

static void __init taranto_machine_init(void)
{
	platform_device_register(&taranto_vgpio);

	s3c_i2c0_set_platdata(NULL);
	s3c_i2c1_set_platdata(NULL);

#ifdef CONFIG_S3C_DEV_MSHC
	s5p6450_default_mshci();
#endif

	s3c_adc_set_platdata(&s3c_adc_platform);

	i2c_register_board_info(0, i2c_devs0, ARRAY_SIZE(i2c_devs0));
	i2c_register_board_info(1, i2c_devs1, ARRAY_SIZE(i2c_devs1));
	
	/* enable i2c1 pullups */
	gpio_request(TT_VGPIO_PU_I2C1, "TT_VGPIO_PU_I2C1");
	gpio_direction_output(TT_VGPIO_PU_I2C1, 1);
	
	/* setup kill power (suicide) */
	if (gpio_request(TT_VGPIO_PWR_KILL, "TT_VGPIO_PWR_KILL")){
		printk("Can't request TT_VGPIO_PWR_KILL GPIO\n");
	} else {		
		int tmp;

		gpio_direction_output(TT_VGPIO_PWR_KILL,1);

		tmp = __raw_readl(S5P64XX_GPNPUD);
		tmp &= ~0x03000000;
		__raw_writel(tmp,S5P64XX_GPNPUD);

		pm_power_off = s5m8751_pmic_poweroff;
	}

	s3c_spi_init();

#ifdef CONFIG_ANDROID_PMEM
	android_pmem_set_platdata();
#endif

#ifdef CONFIG_S3C_DEV_MSHC
	s3c_mshci_set_platdata(&smdk6450_mshc_pdata);
#endif
	platform_add_devices(smdk6450_devices, ARRAY_SIZE(smdk6450_devices));

	s3c_pm_init();
	
	/* Add lcd data to list */
	taranto_lcd_init();
	
	/* Match lcd data to fdt */	
	mendoza_lcd_lookup();
	
	smdk_backlight_register();

	/* Configure GPIO's to retain it value in the during suspend time.  */
	s5p6450_gpio_sleep_config();
}

MACHINE_START(TARANTO, "Taranto")
	.phys_io	= S3C_PA_UART & 0xfff00000,
	.io_pg_offst	= (((u32)S3C_VA_UART) >> 18) & 0xfffc,
	.boot_params	= S5P_PA_SDRAM + 0x100,
	.fixup			= smdk6450_fixup,
	.init_irq	= s5p6450_init_irq,
	.map_io		= smdk6450_map_io,
	.init_machine	= taranto_machine_init,
	.timer		= &s5p_systimer,
MACHINE_END

