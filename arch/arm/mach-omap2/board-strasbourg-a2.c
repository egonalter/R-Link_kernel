/*
 * board-strasbourg.c (Strasbourg)
 *
 * Initial code: Steve Sakoman <steve@sakoman.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <linux/clk.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/kernel.h>
#include <linux/bootmem.h>
#include <linux/platform_device.h>
#include <linux/i2c/twl.h>
#include <linux/lp855x.h>
#include <linux/regulator/machine.h>
#include <linux/pm.h>
#include <linux/vgpio.h>
#include <linux/spi/spi.h>
#include <linux/serial_reg.h>
#include <linux/input/lis3dh.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/flash.h>
#include <asm/mach/map.h>
#include <asm/kexec.h>

#include <mach/gpio.h>
#include <mach/hardware.h>
#include <mach/strasbourg_ver.h>

#include <plat/omap-serial.h>
#include <plat/board.h>
#include <plat/common.h>
#include <plat/display.h>
#include <plat/gpmc.h>
#include <plat/nand.h>
#include <plat/usb.h>
#include <plat/mcspi.h>
#include <plat/offenburg.h>
#include <plat/control.h>
#include <plat/vram.h>
#include <plat/fdt.h>
#include <plat/mcbsp.h>

#include "mux.h"
#include "sdram-samsung-k4x2g323pc-8gd8.h"
#include "sdram-micron-MT46H128M32L2KQ-5.h"
#include "mmc-twl4030.h"
#include "pm.h"
#include "omap3-opp.h"
#include "twl4030.h"
#include "board-strasbourg-whitelist.h"

/* Before including the padconfig settings, we have to set proper config */
#define KERNEL_PADCONFIG
#include <mach/padconfig_strasbourg_a2.h>

#define STRASBOURG_NORFLASH_CS	0
#define STRASBOURG_VRAM_ADDR	0x81020000

extern struct omap_dss_device offenburg_lcd_device;

static struct omap_dss_device *offenburg_dss_devices[] = {
	&offenburg_lcd_device,
};

static struct omap_dss_board_info offenburg_dss_data = {
	.get_last_off_on_transaction_id = get_last_off_on_transaction_id,
	.num_devices	= ARRAY_SIZE(offenburg_dss_devices),
	.devices	= offenburg_dss_devices,
	.default_device	= &offenburg_lcd_device,
};

static struct platform_device offenburg_dss_device = {
	.name          = "omapdss",
	.id            = -1,
	.dev            = {
		.platform_data = &offenburg_dss_data,
	},
};

static struct regulator_consumer_supply strasbourg_vdda_dac_supply = {
	.supply		= "vdda_dac",
	.dev		= &offenburg_dss_device.dev,
};

static struct regulator_consumer_supply strasbourg_vdds_dsi_supply = {
	.supply		= "vdds_dsi",
	.dev		= &offenburg_dss_device.dev,
};

static struct mtd_partition strasbourg_nor_partitions[] = {
	{
		.name           = "nor_part0",
		.offset         = 0,			/* Offset = 0x00000 */
		.size           = 0x400000,		/* Size = 4 MB */
	},
};

static struct flash_platform_data strasbourg_flash_platform_data = {
	.map_name	= "cfi_probe",
	.width		= 2,
	.parts		= strasbourg_nor_partitions,
	.nr_parts	= ARRAY_SIZE(strasbourg_nor_partitions),
};

static struct resource strasbourg_nor_resource = {
	.flags		= IORESOURCE_MEM,
};

static struct platform_device strasbourg_nor_device = {
	.name		= "omapflash",
	.id		= -1,
	.dev		= {
		.platform_data	= &strasbourg_flash_platform_data,
	},
	.num_resources	= 1,
	.resource	= &strasbourg_nor_resource,
};

static struct gpmc_timings strasbourg_flash_timings = {
	/* Minimum clock period for synchronous mode */
	.sync_clk = 0,

	/* Chip-select signal timings corresponding to GPMC_CS_CONFIG2 */
	.cs_on = 0,              /* Assertion time */
	.cs_rd_off = 155,        /* Read deassertion time */
	.cs_wr_off = 155,        /* Write deassertion time */

	/* ADV signal timings corresponding to GPMC_CONFIG3 */
	.adv_on = 10,            /* Assertion time */
	.adv_rd_off = 40,        /* Read deassertion time */
	.adv_wr_off = 40,	     /* Write deassertion time */

	/* WE signals timings corresponding to GPMC_CONFIG4 */
	.we_on = 45,             /* WE assertion time */
	.we_off = 140,           /* WE deassertion time */

	/* OE signals timings corresponding to GPMC_CONFIG4 */
	.oe_on = 45,             /* OE assertion time */
	.oe_off = 140,           /* OE deassertion time */

	/* Access time and cycle time timings corresponding to GPMC_CONFIG5 */
	.page_burst_access = 5,  /* Multiple access word delay */
	.access = 95,            /* Start-cycle to first data valid delay */
	.rd_cycle = 155,         /* Total read cycle time */
	.wr_cycle = 155,         /* Total write cycle time */

	/* The following are only on OMAP3430 */
	.wr_data_mux_bus = 75,   /* WRDATAONADMUXBUS */
	.wr_access = 155         /* WRACCESSTIME */
};

static void __init strasbourg_flash_init(void)
{
	unsigned long base;

	gpmc_cs_write_reg (STRASBOURG_NORFLASH_CS, GPMC_CS_CONFIG1,
			GPMC_CONFIG1_MUXADDDATA | GPMC_CONFIG1_DEVICESIZE_16);

	if (gpmc_cs_set_timings(STRASBOURG_NORFLASH_CS, &strasbourg_flash_timings)) {
		printk(KERN_ERR "Unable to set timings for CS%d\n", STRASBOURG_NORFLASH_CS);
		return;
	}

	if (gpmc_cs_request(STRASBOURG_NORFLASH_CS, SZ_4M, &base)) {
		printk(KERN_ERR "Failed to request GPMC mem for NOR flash\n");
		return;
	}
	strasbourg_nor_resource.start = base;
	strasbourg_nor_resource.end = base + SZ_4M - 1;

	if (platform_device_register(&strasbourg_nor_device) < 0)
		printk(KERN_ERR "Unable to register NAND device\n");
}

/*
 * Sequence to control the TRITON Power resources,
 * when the system goes into sleep.
 * Executed upon P1_P2/P3 transition for sleep.
 */
static struct twl4030_ins __initdata sleep_on_seq[] = {
	/* Broadcast message to put res to sleep */
	{MSG_BROADCAST(DEV_GRP_ALL, RES_GRP_ALL, RES_TYPE_R0, RES_TYPE2_R1,
							RES_STATE_SLEEP), 2},
	{MSG_BROADCAST(DEV_GRP_ALL, RES_GRP_ALL, RES_TYPE_R0, RES_TYPE2_R2,
							RES_STATE_SLEEP), 2},
};

static struct twl4030_script sleep_on_script __initdata = {
	.script	= sleep_on_seq,
	.size	= ARRAY_SIZE(sleep_on_seq),
	.flags	= TWL4030_SLEEP_SCRIPT,
};

/*
 * Sequence to control the TRITON Power resources,
 * when the system wakeup from sleep.
 * Executed upon P1_P2 transition for wakeup.
 */
static struct twl4030_ins wakeup_p12_seq[] __initdata = {
	/* Broadcast message to put res to active */
	{MSG_BROADCAST(DEV_GRP_ALL, RES_GRP_ALL, RES_TYPE_R0, RES_TYPE2_R1,
							RES_STATE_ACTIVE), 2},
};

static struct twl4030_script wakeup_p12_script __initdata = {
	.script	= wakeup_p12_seq,
	.size	= ARRAY_SIZE(wakeup_p12_seq),
	.flags	= TWL4030_WAKEUP12_SCRIPT,
};

/*
 * Sequence to control the TRITON Power resources,
 * when the system wakeup from sleep.
 * Executed upon P3 transition for wakeup.
 */
static struct twl4030_ins wakeup_p3_seq[] __initdata = {
	/* Broadcast message to put res to active */
	{MSG_BROADCAST(DEV_GRP_ALL, RES_GRP_ALL, RES_TYPE_R0, RES_TYPE2_R2,
							RES_STATE_ACTIVE), 2},
};

static struct twl4030_script wakeup_p3_script __initdata = {
	.script = wakeup_p3_seq,
	.size   = ARRAY_SIZE(wakeup_p3_seq),
	.flags  = TWL4030_WAKEUP3_SCRIPT,
};

/*
 * Sequence to reset the TRITON Power resources,
 * when the system gets warm reset.
 * Executed upon warm reset signal.
 */
static struct twl4030_ins wrst_seq[] __initdata = {
/*
 * Reset twl4030.
 * Reset Main_Ref.
 * Reset VUSB_3v1.
 * Reset RC.
 * Reenable twl4030.
 */
	{MSG_SINGULAR(DEV_GRP_NULL, RES_RESET, RES_STATE_OFF), 2},
	{MSG_SINGULAR(DEV_GRP_NULL, RES_Main_Ref, RES_STATE_WRST), 2},
	{MSG_SINGULAR(DEV_GRP_NULL, RES_VUSB_3V1, RES_STATE_WRST), 2},
	{MSG_BROADCAST(DEV_GRP_NULL, RES_GRP_RC, RES_TYPE_ALL, RES_TYPE2_R0,
							RES_STATE_WRST), 2},
	{MSG_SINGULAR(DEV_GRP_NULL, RES_RESET, RES_STATE_ACTIVE), 2},
};

static struct twl4030_script wrst_script __initdata = {
	.script = wrst_seq,
	.size   = ARRAY_SIZE(wrst_seq),
	.flags  = TWL4030_WRST_SCRIPT,
};

/* TRITON script for sleep, wakeup & warm_reset */
static struct twl4030_script *twl4030_scripts[] __initdata = {
	&wakeup_p12_script,
	&wakeup_p3_script,
	&sleep_on_script,
	&wrst_script,
};

static struct twl4030_resconfig twl4030_rconfig[] = {
	{ .resource = RES_VAUX1, .devgroup = DEV_GRP_P1, .type = 0,
		.type2 = 1, .remap_sleep = RES_STATE_OFF },
	{ .resource = RES_VAUX2, .devgroup = DEV_GRP_P1, .type = 0,
		.type2 = 1, .remap_sleep = RES_STATE_OFF },
	{ .resource = RES_VAUX3, .devgroup = DEV_GRP_P1, .type = 0,
		.type2 = 1, .remap_sleep = RES_STATE_OFF },
	{ .resource = RES_VAUX4, .devgroup = DEV_GRP_P1, .type = 0,
		.type2 = 1, .remap_sleep = RES_STATE_OFF },
	{ .resource = RES_VPLL1, .devgroup = DEV_GRP_P1, .type = 3,
		.type2 = 1, .remap_sleep = RES_STATE_OFF },
	{ .resource = RES_VINTANA1, .devgroup = DEV_GRP_ALL, .type = 1,
		.type2 = 2, .remap_sleep = RES_STATE_SLEEP },
	{ .resource = RES_VINTANA2, .devgroup = DEV_GRP_ALL, .type = 0,
		.type2 = 2, .remap_sleep = RES_STATE_SLEEP },
	{ .resource = RES_VINTDIG, .devgroup = DEV_GRP_ALL, .type = 1,
		.type2 = 2, .remap_sleep = RES_STATE_SLEEP },
	{ .resource = RES_VIO, .devgroup = DEV_GRP_ALL, .type = 2,
		.type2 = 2, .remap_sleep = RES_STATE_SLEEP },
	{ .resource = RES_VDD1, .devgroup = DEV_GRP_P1,
		.type = 4, .type2 = 1, .remap_sleep = RES_STATE_OFF },
	{ .resource = RES_VDD2, .devgroup = DEV_GRP_P1,
		.type = 3, .type2 = 1, .remap_sleep = RES_STATE_OFF },
	{ .resource = RES_REGEN, .devgroup = DEV_GRP_ALL, .type = 2,
		.type2 = 1, .remap_sleep = RES_STATE_OFF },
	{ .resource = RES_NRES_PWRON, .devgroup = DEV_GRP_ALL, .type = 0,
		.type2 = 1, .remap_sleep = RES_STATE_SLEEP },
	{ .resource = RES_CLKEN, .devgroup = DEV_GRP_ALL, .type = 3,
		.type2 = 2, .remap_sleep = RES_STATE_SLEEP },
	{ .resource = RES_SYSEN, .devgroup = DEV_GRP_ALL, .type = 6,
		.type2 = 1, .remap_sleep = RES_STATE_OFF },
	{ .resource = RES_HFCLKOUT, .devgroup = DEV_GRP_P3,
		.type = 0, .type2 = 2, .remap_sleep = RES_STATE_SLEEP },
	{ 0, 0},
};

static struct twl4030_power_data strasbourg_t2scripts_data __initdata = {
	.scripts	= twl4030_scripts,
	.num		= ARRAY_SIZE(twl4030_scripts),
	.resource_config = twl4030_rconfig,
};

static struct twl4030_hsmmc_info mmc[] = {
	{
		.mmc		= 1,
		.wires		= 4,
		.gpio_cd	= -EINVAL,
		.gpio_wp	= -EINVAL,
		.no_off_init	= true,
	},
	{}	/* Terminator */
};

static struct regulator_consumer_supply strasbourg_vmmc1_supply[] = {
	{
		.supply			= "vmmc",
	},
	{
		/* Accelemeter (LIS3DHTR) */
		.supply			= "vmmc1",
		.dev_name		= "3-0018", //LIS3DH_ACC_DEV_NAME,
	},
};

static struct platform_device* strasbourg_pmic_vgpio;

static int strasbourg_twl_gpio_setup(struct device *dev,
		unsigned gpio, unsigned ngpio)
{
	twl4030_mmc_init(mmc);

	strasbourg_vmmc1_supply[0].dev = mmc[0].dev;

	platform_device_register(strasbourg_pmic_vgpio);

	return 0;
}

static struct twl4030_gpio_platform_data strasbourg_gpio_data = {
	.gpio_base	= TWL4030_GPIO_BASE,
	.irq_base	= TWL4030_GPIO_IRQ_BASE,
	.irq_end	= TWL4030_GPIO_IRQ_END,
	.setup		= strasbourg_twl_gpio_setup,
};

static struct twl4030_usb_data strasbourg_usb_data = {
	.usb_mode	= T2_USB_MODE_ULPI,
};

static struct regulator_init_data strasbourg_vmmc1 = {
	.constraints = {
		.min_uV			= 3000000,
		.max_uV			= 3000000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_VOLTAGE
					| REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= ARRAY_SIZE(strasbourg_vmmc1_supply),
	.consumer_supplies	= strasbourg_vmmc1_supply,
};

/* VDAC for DSS driving S-Video (8 mA unloaded, max 65 mA) */
static struct regulator_init_data strasbourg_vdac = {
	.constraints = {
		.min_uV			= 1800000,
		.max_uV			= 1800000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &strasbourg_vdda_dac_supply,
};

/* VPLL2 for digital video outputs */
static struct regulator_init_data strasbourg_vpll2 = {
	.constraints = {
		.name			= "VDVI",
		.min_uV			= 1800000,
		.max_uV			= 1800000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &strasbourg_vdds_dsi_supply,
};

/* VAUX1 for i2c3 */
static struct regulator_consumer_supply strasbourg_vaux1_supply = {
	.supply			= "vaux1",
	.dev			= NULL,
};

static struct regulator_init_data strasbourg_vaux1 = {
	.constraints = {
		.min_uV			= 3000000,
		.max_uV			= 3000000,
		.apply_uV		= true,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
		.boot_on		= true,
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &strasbourg_vaux1_supply,
};

/* VAUX2 for Video decoder */
static struct regulator_consumer_supply strasbourg_vaux2_supply = {
	.supply			= "vaux2",
	.dev			= NULL,
};

static struct regulator_init_data strasbourg_vaux2 = {
	.constraints = {
		.min_uV			= 1800000,
		.max_uV			= 1800000,
		.apply_uV		= true,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
		.boot_on		= true,
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &strasbourg_vaux2_supply,
};

/* VAUX3 for i2c2 */
static struct regulator_consumer_supply strasbourg_vaux3_supply[] = {
	{
		.supply			= "vaux3",
		.dev			= NULL,
	},{
		.supply			= "vdd",
		.dev_name		= "tomtom-gps",
	},
};

static struct regulator_init_data strasbourg_vaux3 = {
	.constraints = {
		.min_uV			= 1800000,
		.max_uV			= 1800000,
		.apply_uV		= true,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
		.boot_on		= true,
	},
	.num_consumer_supplies	= ARRAY_SIZE(strasbourg_vaux3_supply),
	.consumer_supplies	= strasbourg_vaux3_supply,
};

/* VAUX4 for sensors and apple chip */
static struct regulator_consumer_supply strasbourg_vaux4_supply[] = {
	{
		.supply			= "vaux4",
		.dev_name		= "3-0018", //LIS3DH_ACC_DEV_NAME,
	},
};

static struct regulator_init_data strasbourg_vaux4 = {
	.constraints = {
		.min_uV			= 1800000,
		.max_uV			= 1800000,
		.apply_uV		= true,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS
					| REGULATOR_CHANGE_VOLTAGE,
		.boot_on		= true,
	},
	.num_consumer_supplies	= ARRAY_SIZE(strasbourg_vaux4_supply),
	.consumer_supplies	= strasbourg_vaux4_supply,
};

/* mmc2 (WLAN) and Bluetooth don't use twl4030 regulators */

static struct twl4030_codec_audio_data strasbourg_audio_data = {
	.audio_mclk = 26000000,
};

static struct twl4030_codec_data strasbourg_codec_data = {
	.audio_mclk = 26000000,
	.audio = &strasbourg_audio_data,
};

/* mmc2 (WLAN) and Bluetooth don't use twl4030 regulators */

static struct twl4030_madc_platform_data strasbourg_madc_data = {
	.irq_line	= 1,
};

static struct twl4030_clock_init_data strasbourg_clock_data = {
	.high_perf_sq_disable = 1,	
};

static struct twl4030_platform_data __initdata strasbourg_twldata = {
	.irq_base	= TWL4030_IRQ_BASE,
	.irq_end	= TWL4030_IRQ_END,
	.clock		= &strasbourg_clock_data,
	.gpio		= &strasbourg_gpio_data,
	.madc		= &strasbourg_madc_data,
	.usb		= &strasbourg_usb_data,
	.power		= &strasbourg_t2scripts_data,
	.codec		= &strasbourg_codec_data,
	.vmmc1		= &strasbourg_vmmc1,
	.vdac		= &strasbourg_vdac,
	.vpll2		= &strasbourg_vpll2,
	.vaux1		= &strasbourg_vaux1,
	.vaux2  	= &strasbourg_vaux2,
	.vaux3		= &strasbourg_vaux3,
	.vaux4		= &strasbourg_vaux4,
};

static struct i2c_board_info __initdata strasbourg_i2c1_boardinfo[] = {
	{
		I2C_BOARD_INFO("tps65950", 0x48),
		.flags = I2C_CLIENT_WAKE,
		.irq = INT_34XX_SYS_NIRQ,
		.platform_data = &strasbourg_twldata,
	},
};

static struct lp855x_platform_data __initdata rennes_b1_lp855x_pdata = {
	.mode = PWM_BASED,
};

static struct i2c_board_info __initdata rennes_b1_i2c2_boardinfo[] = {
#ifdef CONFIG_BACKLIGHT_LP855X
	{
		I2C_BOARD_INFO("lp8553", 0x2c),
		.irq = INT_24XX_I2C2_IRQ,
		.platform_data = &rennes_b1_lp855x_pdata,
	},
#endif
};

static void __init strasbourg_i2c_init(void)
{
	struct i2c_board_info *i2c2_boardinfo = NULL;
	int i2c2_boardinfo_size = 0;

	if (cpu_is_omap3630()) {

		u32 prog_io;

		prog_io = omap_ctrl_readl(OMAP343X_CONTROL_PROG_IO1);
		/* Program (bit 19)=1 to disable internal pull-up on I2C1 */
		prog_io |= OMAP3630_PRG_I2C1_PULLUPRESX;
		/* Program (bit 0)=1 to disable internal pull-up on I2C2 */
		prog_io |= OMAP3630_PRG_I2C2_PULLUPRESX;
		omap_ctrl_writel(prog_io, OMAP343X_CONTROL_PROG_IO1);

		prog_io = omap_ctrl_readl(OMAP36XX_CONTROL_PROG_IO2);
		/* Program (bit 7)=1 to disable internal pull-up on I2C3 */
		prog_io |= OMAP3630_PRG_I2C3_PULLUPRESX;
		omap_ctrl_writel(prog_io, OMAP36XX_CONTROL_PROG_IO2);
	}

	if (get_strasbourg_ver() == RENNES_B1_SAMPLE) {
		i2c2_boardinfo = rennes_b1_i2c2_boardinfo;
		i2c2_boardinfo_size = ARRAY_SIZE(rennes_b1_i2c2_boardinfo);
	}
	if (get_strasbourg_ver() == STUTTGART_B1_SAMPLE) {
		i2c2_boardinfo = rennes_b1_i2c2_boardinfo;
		i2c2_boardinfo_size = ARRAY_SIZE(rennes_b1_i2c2_boardinfo);
	}

	omap_register_i2c_bus(1, 400, NULL, strasbourg_i2c1_boardinfo,
			ARRAY_SIZE(strasbourg_i2c1_boardinfo));
	omap_register_i2c_bus(2, 400, NULL, i2c2_boardinfo,
			i2c2_boardinfo_size);
	omap_register_i2c_bus(3, 50, NULL, NULL, 0);
}

static void __init strasbourg_mmc_init(int pu_strength)
{
	u32 prog_io = omap_ctrl_readl(OMAP343X_CONTROL_PROG_IO0);

	if (pu_strength)
		/* Select 10k-50k Ohms strength on SDMMC IO cells */
		prog_io |= OMAP3630_PRG_SDMMC_PUSTRENGTH;
	else
		/* Select 50k-100k Ohms strength on SDMMC IO cells */
		prog_io &= ~OMAP3630_PRG_SDMMC_PUSTRENGTH;

	omap_ctrl_writel(prog_io, OMAP343X_CONTROL_PROG_IO0);
}

static struct platform_device strasbourg_hwmon_device = {
	.name = "omap-hwmon",
	.id = -1,
};

static struct platform_device offenburg_usb_stat_device = {
	.name	= "usb_status",
};

static struct platform_device strasbourg_suicide_alm_device = {
	.name	= "suicide_alarm",
};

static int __init strasbourg_init_rotation_gpio(int gpio, const char* label, int value)
{
	int ret;

	pr_debug("LCD image rotation - label: %s, gpio: %d, value: %d\n", label, gpio, value);

	ret = gpio_request(gpio, label);
	if(ret < 0) {
		pr_err("unable to request %s\n", label);
		return ret;
	}

	ret = gpio_direction_output(gpio, value);
	if (ret < 0) {
		pr_err("unable to set %s output direction\n", label);
		goto err;
	}

	ret = gpio_export(gpio, 0);
	if (ret < 0) {
		pr_err("unable to export %s\n", label);
		goto err;
	}

	ret = gpio_export_link(&offenburg_dss_device.dev, label, gpio);
	if (ret < 0) {
		pr_err("unable to export link %s\n", label);
		goto err2;
	}

	return 0;

err2:
	gpio_unexport(gpio);
err:
	gpio_free(gpio);
	return ret;
}

static void __init strasbourg_init_image_rotation(void)
{
	if (fdt_get_ulong("/features", "screen-rotation", 0) == 1) {
		strasbourg_init_rotation_gpio(TT_VGPIO_LCD_UD, "lcd_ud", 0);
		strasbourg_init_rotation_gpio(TT_VGPIO_LCD_LR, "lcd_lr", 1);
	}
	else {
		strasbourg_init_rotation_gpio(TT_VGPIO_LCD_UD, "lcd_ud", 1);
		strasbourg_init_rotation_gpio(TT_VGPIO_LCD_LR, "lcd_lr", 0);
	}
}

static void __init strasbourg_usb_init(void)
{
	int r;

	/* USB1 power fault line */
	r =	gpio_request(TT_VGPIO_USB1_POWER_FAULT, "USB1_POWER_FAULT") ||
		gpio_direction_input(TT_VGPIO_USB1_POWER_FAULT) ||
		gpio_export(TT_VGPIO_USB1_POWER_FAULT, 0) ||
		gpio_export_link(&offenburg_usb_stat_device.dev, "usb1_power_fault", TT_VGPIO_USB1_POWER_FAULT);

	if (r != 0)	
		pr_err("usb1_power_fault line couldn't be set\n");

	/* USB3 power fault line */
	r =	gpio_request(TT_VGPIO_USB3_POWER_FAULT, "USB3_POWER_FAULT") ||
		gpio_direction_input(TT_VGPIO_USB3_POWER_FAULT) ||
		gpio_export(TT_VGPIO_USB3_POWER_FAULT, 0) ||
		gpio_export_link(&offenburg_usb_stat_device.dev, "usb3_power_fault", TT_VGPIO_USB3_POWER_FAULT);

	if (r != 0)	
		pr_err("usb3_power_fault line couldn't be set\n");

	/* Full-speed PHY GPIOs */
	r =	gpio_request(TT_VGPIO_USB1_SPEED, "USB1 speed") ||
		gpio_direction_output(TT_VGPIO_USB1_SPEED, 1) ||
		gpio_request(TT_VGPIO_USB1_SUSPEND, "USB1 suspend") ||
		gpio_direction_output(TT_VGPIO_USB1_SUSPEND, 0);

	if (r != 0)
		pr_err("Full speed PHY GPIOs couldn't be set\n");
}

static void __init strasbourg_special_gpios_init(void)
{
	u32 reg;

	/* Enable GPIO_126 and GPIO_129 */
	reg = omap_ctrl_readl(OMAP343X_CONTROL_WKUP_CTRL);
	reg |= OMAP343X_GPIO_IO_PWRDNZ;
	omap_ctrl_writel(reg, OMAP343X_CONTROL_WKUP_CTRL);
}

static void __init strasbourg_init_irq(void)
{
    omap_init_irq();
	switch (get_strasbourg_ver()) {
		case RENNES_A1_SAMPLE:
			omap2_init_common_hw(mt46h128m32l2kq5_sdrc_params,
				mt46h128m32l2kq5_sdrc_params,
				omap3630_mpu_rate_table,
				omap3630_dsp_rate_table,
				omap3630_l3_rate_table);
			break;
		default:
			omap2_init_common_hw(k4x2g323pc8gd8_sdrc_params,
				k4x2g323pc8gd8_sdrc_params,
				omap3630_mpu_rate_table,
				omap3630_dsp_rate_table,
				omap3630_l3_rate_table);
		break;
	}
}

static void strasbourg_syson_init (void)
{
	gpio_request(TT_VGPIO_PWR_HOLD, "PWR HOLD");
	gpio_direction_output(TT_VGPIO_PWR_HOLD, 1);
}

static void strasbourg_power_off(void)
{
	/* Delay 500 ms to avoid corrupting the MoviNAND */
	mdelay (500);

	/* This is the end, my only friend. The end. */
	gpio_set_value(TT_VGPIO_PWR_HOLD, 0);

	/* Delay 500 ms to let SYSTEM_ON go down */
	mdelay (500);

	/* Force reboot in case we are still alive
	 * (SYSTEM_ON might have been set to ON in the meantime) */
	BUG();
}

#ifdef CONFIG_KEXEC
static void strasbourg_kexec_reinit(void)
{
	/* Make sure UART interrupts are disabled before kexec'ing */
	omap_writew(0x0, OMAP_UART1_BASE + (UART_IER << 2));
	omap_writew(0x0, OMAP_UART2_BASE + (UART_IER << 2));
	omap_writew(0x0, OMAP_UART3_BASE + (UART_IER << 2));
}
#endif

static struct resource strasbourg_ram_console_resource = {
	.flags		= IORESOURCE_MEM,
};

static struct platform_device strasbourg_ram_console_device = {
	.name		= "ram_console",
	.id		= -1,
	.resource	= &strasbourg_ram_console_resource,
	.num_resources	= 1,
};

/**** */

static struct platform_device *strasbourg_devices[] __initdata = {
	&offenburg_dss_device,
	&offenburg_usb_stat_device,
	&strasbourg_hwmon_device,
	&strasbourg_ram_console_device,
	&strasbourg_suicide_alm_device
};

static void usbhs_reset_vbus_rennes_b1(struct platform_device *pdev, int port, bool toggle_vbus)
{
	if (port != 1)
		return;

	msleep(100);
	if (toggle_vbus) {
	    dev_info(&pdev->dev, "Resetting the 5V75 VBUS line...\n");
	    /* Switch off and on 5V75 line to off and on CPEN (Drive VBUS, CPEN drive high) */
	    
	    gpio_direction_output(TT_VGPIO_5V75_ON, 0);
	    msleep(10);
	    gpio_direction_output(TT_VGPIO_5V75_ON, 1);
	}
}

/* EHCI/OHCI platform data for the A2/A3 samples */
static const struct usbhs_omap_board_data usbhs_bdata_a2 __initconst = {
	.port_mode[0] = OMAP_EHCI_PORT_MODE_PHY,
	.port_mode[1] = OMAP_EHCI_PORT_MODE_PHY,
	.port_mode[2] = OMAP_USBHS_PORT_MODE_UNUSED,

	.phy_reset  = true,
	.reset_gpio_port[0]  = TT_VGPIO_USB1_RESET,
	.reset_gpio_port[1]  = TT_VGPIO_USB2_RESET,
	.reset_gpio_port[2]  = -EINVAL
};

/* EHCI/OHCI platform data for the B samples */
static const struct usbhs_omap_board_data usbhs_bdata_b __initconst = {
	.port_mode[0] = OMAP_OHCI_PORT_MODE_PHY_4PIN_DPDM,
	.port_mode[1] = OMAP_EHCI_PORT_MODE_PHY,
	.port_mode[2] = OMAP_USBHS_PORT_MODE_UNUSED,

	.phy_reset  = true,
	.reset_gpio_port[0]  = -EINVAL,
	.reset_gpio_port[1]  = TT_VGPIO_USB2_RESET,
	.reset_gpio_port[2]  = -EINVAL
};

/* EHCI/OHCI platform data with 5V75 line reset*/
static const struct usbhs_omap_board_data usbhs_bdata_5V75_reset __initconst = {
	.port_mode[0] = OMAP_OHCI_PORT_MODE_PHY_4PIN_DPDM,
	.port_mode[1] = OMAP_EHCI_PORT_MODE_PHY,
	.port_mode[2] = OMAP_USBHS_PORT_MODE_UNUSED,

	.phy_reset  = true,
	.reset_gpio_port[0]  = -EINVAL,
	.reset_gpio_port[1]  = TT_VGPIO_USB2_RESET,
	.reset_gpio_port[2]  = -EINVAL,

	.reset_vbus[1] = usbhs_reset_vbus_rennes_b1,
};

/* OMAP Serial platform data */
static struct omap_uart_port_info strasbourg_serial_pdata[] __initdata = {
	{
		/* UART1 - debug console */
		.dma_enabled	= false,
		.dma_rx_buf_size = OMAP_UART_DEF_RXDMA_BUFSIZE,
		.dma_rx_timeout = OMAP_UART_DEF_RXDMA_POLL_RATE,
		.rts_mux_driver_control = 0,
		.hwmod_name = "uart1",
	},
	{
		/* UART2 - Bluetooth */
		.dma_enabled	= true,
		.dma_rx_buf_size = OMAP_UART_DEF_RXDMA_BUFSIZE,
		.dma_rx_timeout = OMAP_UART_DEF_RXDMA_POLL_RATE,
		.rts_mux_driver_control = 0,
		.hwmod_name = "uart2",
	},
	{
		/* UART3 - GPS */
		.dma_enabled	= true,
		.dma_rx_buf_size = OMAP_UART_DEF_RXDMA_BUFSIZE,
		.dma_rx_timeout = OMAP_UART_DEF_RXDMA_POLL_RATE,
		.rts_mux_driver_control = 0,
		.hwmod_name = "uart3",
	},
};

static struct omap_mcbsp_platform_data uda1334_mcbsp_pdata = {
	.phys_base	= OMAP34XX_MCBSP4_BASE,
	.dma_rx_sync	= OMAP24XX_DMA_MCBSP4_RX,
	.dma_tx_sync	= OMAP24XX_DMA_MCBSP4_TX,
	.rx_irq		= INT_24XX_MCBSP4_IRQ_RX,
	.tx_irq		= INT_24XX_MCBSP4_IRQ_TX,
	.buffer_size	= 0x80, /* The FIFO has 128 locations */
	.clk_always_on	= true,
};

#ifdef CONFIG_OMAP_MUX

static struct omap_board_mux board_mux[] __initdata = {

	PADCONFIG_SETTINGS_COMMON
	PADCONFIG_SETTINGS_KERNEL
	
	{ .reg_offset = OMAP_MUX_TERMINATOR },
};

#include <mach/padconfig_rennes_b1.h>

static struct omap_board_mux board_mux_rennes_b1[] __initdata = {

	PADCONFIG_SETTINGS_COMMON_RENNES_B1
	PADCONFIG_SETTINGS_KERNEL_RENNES_B1
	
	{ .reg_offset = OMAP_MUX_TERMINATOR },
};

#include <mach/padconfig_stuttgart_b1.h>

static struct omap_board_mux board_mux_stuttgart_b1[] __initdata = {

	PADCONFIG_SETTINGS_COMMON_STUTTGART_B1
	PADCONFIG_SETTINGS_KERNEL_STUTTGART_B1
	
	{ .reg_offset = OMAP_MUX_TERMINATOR },
};

static const struct mfd_feat ram_console_feats[] = {
	MFD_1_0((void *) 0x8fffc000),
	MFD_DEFAULT((void *) 0x9fffc000),
};

static struct mfd_feat mux_feats[] = {
	MFD_2(&board_mux_stuttgart_b1),
	MFD_1_1(&board_mux_rennes_b1),
	MFD_DEFAULT(&board_mux),
};

#else

static struct mfd_feat mux_feats[] = {
	MFD_DEFAULT(NULL),
};

#endif

/* Strasbourg (MFD 1.0 and 1.0.5) uses OTG DEVICE mode */
static struct omap_musb_board_data musb_board_data = {
	.interface_type		= MUSB_INTERFACE_ULPI,
	.mode			= MUSB_PERIPHERAL,
	.power			= 100,
};

/* Rennes MFD 1.1 uses (?) OTG HOST mode */
static struct omap_musb_board_data musb_board_data_rennes_b1 = {
	.interface_type		= MUSB_INTERFACE_ULPI,
	.mode			= MUSB_HOST,
};

/* A2/A3 samples */
static struct vgpio_pin strasbourg_a2_soc_vgpio_pins[] =
{
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_GPS_RESET,            160),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_GPS_POWER,            167),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_GPS_TCXO_ON,          138),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_GPS_ANT_SHORT,        108),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_BT_RST,                60),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_LCM_PWR_ON,           137),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_BACKLIGHT_DIM,         65),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_LCD_RESET,            156),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_TP_IRQ,                54),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_TP_RESET,             157),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_USB1_RESET,           109),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_USB2_RESET,           110),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_USB1_POWER_FAULT,      31),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_USB3_POWER_FAULT,      11),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_CAM_IRQ,              107),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_CAM_PWR_ON,           129),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_CAM_ON,                96),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_CAM_RST,              133),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_MIC_ON,               111),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_CAN_SYNC,             163),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_CAN_RST,              136),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_CAN_RST_MON,          170),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_CAN_IRQ,              139),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_CAN_BT_MD,            186),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_APPLE_AUTH_RST,       126),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_APPLE_AUTH_MODE0,     130),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_APPLE_AUTH_MODE1,     135),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_PWR_HOLD,              61),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_ON_OFF,                10),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_DIAGSYS_BOOT,          41),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_GPS_1PPS,              42),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_LCD_PWR_ON               ),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_LCD_STBY                 ),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_USB1_SPEED               ),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_USB1_SUSPEND             ),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_LCD_UD                   ),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_LCD_LR                   ),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_BACKLIGHT_ON             ),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_SENSE_11V0               ),
};

static struct vgpio_platform_data strasbourg_a2_soc_vgpio_pdata =
{
	.gpio_base      = TT_VGPIO_BASE,
	.gpio_number    = ARRAY_SIZE(strasbourg_a2_soc_vgpio_pins),
	.pins           = strasbourg_a2_soc_vgpio_pins,
};

static struct platform_device strasbourg_a2_soc_vgpio =
{
	.name   = "vgpio",
	.id     = 0,
	.dev    = {
		.platform_data  = &strasbourg_a2_soc_vgpio_pdata,
	},
};

/* B1 Sample */
static struct vgpio_pin strasbourg_b1_soc_vgpio_pins[] =
{
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_GPS_RESET,            160),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_GPS_POWER,            167),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_GPS_TCXO_ON,          138),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_GPS_ANT_SHORT,        108),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_BT_RST,                60),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_LCM_PWR_ON,           137),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_BACKLIGHT_DIM            ),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_LCD_RESET,            156),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_TP_IRQ,                54),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_TP_RESET,             157),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_USB1_RESET               ),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_USB2_RESET,           110),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_USB1_POWER_FAULT,      31),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_USB3_POWER_FAULT,      11),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_CAM_IRQ,              107),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_CAM_PWR_ON,           129),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_CAM_ON,                96),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_CAM_RST,              133),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_MIC_ON,               111),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_CAN_SYNC,             163),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_CAN_RST,              136),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_CAN_RST_MON,          170),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_CAN_IRQ,              139),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_CAN_BT_MD,            186),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_APPLE_AUTH_RST,       126),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_APPLE_AUTH_MODE0,     130),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_APPLE_AUTH_MODE1,     135),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_PWR_HOLD,              61),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_ON_OFF,                10),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_DIAGSYS_BOOT,          41),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_GPS_1PPS,              42),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_USB1_SPEED,           141),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_USB1_SUSPEND,         140),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_LCD_PWR_ON               ),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_LCD_STBY                 ),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_EEPROM_WP                ),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_LCD_UD                   ),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_LCD_LR                   ),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_BACKLIGHT_ON             ),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_SENSE_11V0               ),
};

static struct vgpio_platform_data strasbourg_b1_soc_vgpio_pdata =
{
	.gpio_base      = TT_VGPIO_BASE,
	.gpio_number    = ARRAY_SIZE(strasbourg_b1_soc_vgpio_pins),
	.pins           = strasbourg_b1_soc_vgpio_pins,
};

static struct platform_device strasbourg_b1_soc_vgpio =
{
	.name   = "vgpio",
	.id     = 0,
	.dev    = {
		.platform_data  = &strasbourg_b1_soc_vgpio_pdata,
	},
};

/* B2 Sample */
static struct vgpio_pin strasbourg_b2_soc_vgpio_pins[] =
{
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_GPS_RESET,            160),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_GPS_POWER,            167),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_GPS_TCXO_ON              ),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_GPS_ANT_SHORT,        108),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_BT_RST,                60),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_LCM_PWR_ON,           137),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_BACKLIGHT_DIM            ),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_LCD_RESET,            156),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_TP_IRQ,                54),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_TP_RESET,             157),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_USB1_RESET               ),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_USB2_RESET,           110),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_USB1_POWER_FAULT,      31),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_USB3_POWER_FAULT,      11),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_CAM_IRQ,              107),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_CAM_PWR_ON,           129),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_CAM_ON,                96),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_CAM_RST,              133),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_MIC_ON,               111),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_CAN_SYNC,             163),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_CAN_RST,              136),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_CAN_RST_MON,          170),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_CAN_IRQ,              139),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_CAN_BT_MD,            186),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_APPLE_AUTH_RST,       126),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_APPLE_AUTH_MODE0,     130),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_APPLE_AUTH_MODE1,     135),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_PWR_HOLD,              61),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_ON_OFF,                10),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_DIAGSYS_BOOT,          41),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_GPS_1PPS,              42),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_USB1_SPEED,           141),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_USB1_SUSPEND,         140),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_LCD_PWR_ON               ),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_LCD_STBY                 ),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_EEPROM_WP                ),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_LCD_UD,                12),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_LCD_LR,                19),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_BACKLIGHT_ON,          65),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_SENSE_11V0,            17),
};

static struct vgpio_platform_data strasbourg_b2_soc_vgpio_pdata =
{
	.gpio_base      = TT_VGPIO_BASE,
	.gpio_number    = ARRAY_SIZE(strasbourg_b2_soc_vgpio_pins),
	.pins           = strasbourg_b2_soc_vgpio_pins,
};

static struct platform_device strasbourg_b2_soc_vgpio =
{
	.name   = "vgpio",
	.id     = 0,
	.dev    = {
		.platform_data  = &strasbourg_b2_soc_vgpio_pdata,
	},
};

/* MFD 1.1 */
static struct vgpio_pin rennes_b1_soc_vgpio_pins[] =
{
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_GPS_RESET,            160),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_GPS_POWER,            167),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_GPS_TCXO_ON              ),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_GPS_ANT_SHORT,        108),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_BT_RST,                60),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_LCM_PWR_ON,           137),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_BACKLIGHT_DIM            ),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_LCD_RESET,            156),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_TP_IRQ,                54),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_TP_RESET,             157),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_USB1_RESET               ),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_USB2_RESET,           110),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_USB1_POWER_FAULT,      31),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_USB3_POWER_FAULT,      11),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_CAM_IRQ,              107),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_CAM_PWR_ON,           129),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_CAM_ON,                96),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_CAM_RST,              133),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_MIC_ON,               111),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_CAN_SYNC,             163),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_CAN_RST,              136),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_CAN_RST_MON,          170),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_CAN_IRQ,              139),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_CAN_BT_MD,            186),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_APPLE_AUTH_RST,       126),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_APPLE_AUTH_MODE0,     184),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_APPLE_AUTH_MODE1,     185),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_PWR_HOLD,              61),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_ON_OFF,                10),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_DIAGSYS_BOOT,          41),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_GPS_1PPS,              42),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_USB1_SPEED,           141),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_USB1_SUSPEND,         140),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_LCD_PWR_ON               ),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_LCD_STBY                 ),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_EEPROM_WP                ),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_LCD_UD,                12),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_LCD_LR,                19),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_BACKLIGHT_ON,          65),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_SENSE_11V0,            17),
};

static struct vgpio_platform_data rennes_b1_soc_vgpio_pdata =
{
	.gpio_base      = TT_VGPIO_BASE,
	.gpio_number    = ARRAY_SIZE(rennes_b1_soc_vgpio_pins),
	.pins           = rennes_b1_soc_vgpio_pins,
};

static struct platform_device rennes_b1_soc_vgpio =
{
	.name   = "vgpio",
	.id     = 0,
	.dev    = {
		.platform_data  = &rennes_b1_soc_vgpio_pdata,
	},
};

/* PMIC in A2/A3 samples */
static struct vgpio_pin strasbourg_a2_pmic_vgpio_pins[] =
{
	VGPIO_DEF_PIN(TT_PMIC_VGPIO_BASE, TT_VGPIO_LCM_BL_PWR_ON, TWL4030_GPIO_BASE + 13),
	VGPIO_DEF_PIN(TT_PMIC_VGPIO_BASE, TT_VGPIO_3V3_ON,        TWL4030_GPIO_BASE +  2),
	VGPIO_DEF_PIN(TT_PMIC_VGPIO_BASE, TT_VGPIO_6V_ON,         TWL4030_GPIO_BASE + 15),
	VGPIO_DEF_PIN(TT_PMIC_VGPIO_BASE, TT_VGPIO_5V75_ON,       TWL4030_GPIO_BASE + 16),
	VGPIO_DEF_PIN(TT_PMIC_VGPIO_BASE, TT_VGPIO_5V_ON,         TWL4030_GPIO_BASE +  7),
};

static struct vgpio_platform_data strasbourg_a2_pmic_vgpio_pdata =
{
	.gpio_base      = TT_PMIC_VGPIO_BASE,
	.gpio_number    = ARRAY_SIZE(strasbourg_a2_pmic_vgpio_pins),
	.pins           = strasbourg_a2_pmic_vgpio_pins,
};

static struct platform_device strasbourg_a2_pmic_vgpio =
{
	.name   = "vgpio",
	.id     = 1,
	.dev    = {
		.platform_data  = &strasbourg_a2_pmic_vgpio_pdata,
	},
};

/* PMIC in B samples */
static struct vgpio_pin strasbourg_b_pmic_vgpio_pins[] =
{
	VGPIO_DEF_NCPIN (TT_PMIC_VGPIO_BASE, TT_VGPIO_LCM_BL_PWR_ON                   ),
	VGPIO_DEF_PIN   (TT_PMIC_VGPIO_BASE, TT_VGPIO_3V3_ON,   TWL4030_GPIO_BASE +  2),
	VGPIO_DEF_NCPIN (TT_PMIC_VGPIO_BASE, TT_VGPIO_6V_ON                           ),
	VGPIO_DEF_PIN   (TT_PMIC_VGPIO_BASE, TT_VGPIO_5V75_ON,  TWL4030_GPIO_BASE + 16),
	VGPIO_DEF_PIN   (TT_PMIC_VGPIO_BASE, TT_VGPIO_5V_ON,    TWL4030_GPIO_BASE +  7),
};

static struct vgpio_platform_data strasbourg_b_pmic_vgpio_pdata =
{
	.gpio_base      = TT_PMIC_VGPIO_BASE,
	.gpio_number    = ARRAY_SIZE(strasbourg_b_pmic_vgpio_pins),
	.pins           = strasbourg_b_pmic_vgpio_pins,
};

static struct platform_device strasbourg_b_pmic_vgpio =
{
	.name   = "vgpio",
	.id     = 1,
	.dev    = {
		.platform_data  = &strasbourg_b_pmic_vgpio_pdata,
	},
};

static void __init strasbourg_init(void)
{
	struct platform_device* strasbourg_soc_vgpio;
	struct omap_musb_board_data* musb_data;
	const struct usbhs_omap_board_data* usbhs_bdata;
	struct omap_board_mux *mux;
	bool mmc_strength = 1;
	unsigned long ram_console_base;

#ifdef CONFIG_FB_OMAP_BOOTLOADER_INIT
	/*
	 * Clock is set as ENABLE_ON_INIT but it's not needed on
	 * A2 and B1 - balance the usecount by disabling it
	 */
	struct clk *gpt10_fck = clk_get(NULL, "gpt10_fck");
	if (get_strasbourg_ver() < STRASBOURG_B2_SAMPLE)
		clk_disable(gpt10_fck);
#endif

	ram_console_base = (unsigned long) mfd_feature(ram_console_feats);
	strasbourg_ram_console_resource.start = ram_console_base;
	strasbourg_ram_console_resource.end = ram_console_base + SZ_16K - 1;

	mux = mfd_feature(mux_feats);

	switch (get_strasbourg_ver()) {
		case STUTTGART_B1_SAMPLE:
		case RENNES_B1_SAMPLE:
			strasbourg_soc_vgpio = &rennes_b1_soc_vgpio;
			strasbourg_pmic_vgpio = &strasbourg_b_pmic_vgpio;
			usbhs_bdata = &usbhs_bdata_5V75_reset;
			musb_data = &musb_board_data;
			mmc_strength = 0;
			break;
		case STRASBOURG_B2_SAMPLE:
		case RENNES_A1_SAMPLE:
			strasbourg_soc_vgpio = &strasbourg_b2_soc_vgpio;
			strasbourg_pmic_vgpio = &strasbourg_b_pmic_vgpio;
			usbhs_bdata = &usbhs_bdata_5V75_reset;
			musb_data = &musb_board_data;
			break;
		case STRASBOURG_B1_SAMPLE:
			strasbourg_soc_vgpio = &strasbourg_b1_soc_vgpio;
			strasbourg_pmic_vgpio = &strasbourg_b_pmic_vgpio;
			usbhs_bdata = &usbhs_bdata_b;
			musb_data = &musb_board_data;
			break;
		default:
			strasbourg_soc_vgpio = &strasbourg_a2_soc_vgpio;
			strasbourg_pmic_vgpio = &strasbourg_a2_pmic_vgpio;
			usbhs_bdata = &usbhs_bdata_a2;
			musb_data = &musb_board_data;
			break;
	}

	omap3_mux_init(mux, OMAP_PACKAGE_CUS);
	platform_device_register(strasbourg_soc_vgpio);

	strasbourg_special_gpios_init();
	strasbourg_i2c_init();
	strasbourg_mmc_init(mmc_strength);
	strasbourg_syson_init();
	platform_add_devices(strasbourg_devices, ARRAY_SIZE(strasbourg_devices));

	omap_serial_board_init(strasbourg_serial_pdata);
	strasbourg_flash_init();
	strasbourg_init_image_rotation();
	strasbourg_usb_init();
	usb_musb_init(musb_data);
	usbhs_init(usbhs_bdata);
	omap2_mcbsp_set_pdata(OMAP_MCBSP4, &uda1334_mcbsp_pdata);

	/* Ensure SDRC pins are mux'd for self-refresh */
	omap_mux_init_signal("sdrc_cke0", OMAP_PIN_OUTPUT);
	omap_mux_init_signal("sdrc_cke1", OMAP_PIN_OUTPUT);

	/* Register Strasbourg specific power-off method */
	pm_power_off = strasbourg_power_off;
#ifdef CONFIG_KEXEC
	kexec_reinit = strasbourg_kexec_reinit;
#endif
}

static void __init strasbourg_map_io(void)
{
	unsigned long ram_console_base;

	/* NOTE: omap2_map_common_io intializes the VRAM areas
	 * we need to set the vram size earlier */
#ifndef CONFIG_FB_OMAP_BOOTLOADER_INIT
	omap_vram_set_sdram_vram (CONFIG_OMAP2_VRAM_SIZE*1024*1024, STRASBOURG_VRAM_ADDR);
#endif

	fdt_reserve_bootmem();

	omap2_set_globals_343x();
	omap2_map_common_io();

	ram_console_base = (unsigned long) mfd_feature(ram_console_feats);
	if (reserve_bootmem(ram_console_base, SZ_16K, BOOTMEM_EXCLUSIVE))
		printk(KERN_WARNING "Unable to reserve memory for RAM console\n");
}

MACHINE_START(STRASBOURG_A2, "Strasbourg A2")
	.phys_io	= 0x48000000,
	.io_pg_offst	= ((0xfa000000) >> 18) & 0xfffc,
	.boot_params	= 0x80000100,
	.map_io		= strasbourg_map_io,
	.init_irq	= strasbourg_init_irq,
	.init_machine	= strasbourg_init,
	.timer		= &omap_timer,
MACHINE_END
