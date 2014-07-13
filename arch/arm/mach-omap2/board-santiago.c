/*
 * board-santiago.c (Santiago)
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
#include <linux/ms5607.h>
#include <linux/gpio.h>
#include <linux/vgpio.h>
#include <linux/platform_device.h>
#include <linux/regulator/fixed.h>
#include <linux/i2c/twl.h>
#include <linux/regulator/machine.h>
#include <linux/pm.h>
#include <linux/spi/spi.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>

#include <linux/mmc/host.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/flash.h>
#include <asm/mach/map.h>

#include <plat/board.h>
#include <plat/common.h>
#include <plat/display.h>
#include <plat/gpmc.h>
#include <mach/hardware.h>
#include <plat/nand.h>
#include <plat/usb.h>
#include <plat/offenburg.h>
#include <plat/mcspi.h>
#include <plat/control.h>
#include <plat/vram.h>
#include <plat/fdt.h>

#include "mux.h"
#include "sdram-micron-MT46H128M32L2KQ-5.h"
#include "mmc-twl4030.h"
#include "pm.h"
#include "omap3-opp.h"

/* Before including the padconfig settings, we have to set proper config */
#define KERNEL_PADCONFIG
#include <mach/padconfig_santiago.h>

#define SANTIAGO_VRAM_SIZE	4
#define SANTIAGO_VRAM_ADDR	0x81020000

extern struct omap_dss_device offenburg_lcd_device;

static struct omap2_mcspi_device_config default_mcspi_config = {
	.turbo_mode             = 0,
	.single_channel         = 1,  /* 0: slave, 1: master */

};

static struct ms5607_platform_data ms5607_pdata = {
	.sample_rate	= 200,
	.osr		= MS5607_OSR_512	/* NOTE: adapt power consumption in libsensors when changing this value */
};

struct spi_board_info santiago_ld050wv1sp01_board_info __initdata = {
	.modalias               = "ld050wv1sp01_disp_spi",
	.bus_num                = 2,
	.chip_select            = 0,
	.max_speed_hz           = 375000,
	.controller_data        = &default_mcspi_config,
};

struct spi_board_info santiago_lms501kf03_board_info __initdata = {
	.modalias               = "lms501kf03_disp_spi",
	.bus_num                = 2,
	.chip_select            = 0,
	.max_speed_hz           = 375000,
	.controller_data        = &default_mcspi_config,
};

struct spi_board_info santiago_spi_board_info[] __initdata = {
	[0] = {
		.modalias               = "kxr94",
		.bus_num                = 1,
		.chip_select            = 0,
		.max_speed_hz           = 5000000,
		.controller_data        = &default_mcspi_config,
	},

	[1] = {
		.modalias		= "ms5607",
		.bus_num		= 1,
		.chip_select		= 2,
		.max_speed_hz		= 20000000,
		.controller_data	= &default_mcspi_config,
		.platform_data		= &ms5607_pdata
	},

	[2] = {
		.modalias               = "ld050wv1sp01_disp_spi",
		.bus_num                = 2,
		.chip_select            = 0,
		.max_speed_hz           = 375000,
		.controller_data        = &default_mcspi_config,
	},
};

static struct omap_dss_device *offenburg_dss_devices[] = {
	&offenburg_lcd_device,
};

static struct omap_dss_board_info offenburg_dss_data = {
	.get_last_off_on_transaction_id = get_last_off_on_transaction_id,
	.num_devices	= ARRAY_SIZE(offenburg_dss_devices),
	.devices	= offenburg_dss_devices,
	.default_device	= &offenburg_lcd_device,
};

struct platform_device offenburg_dss_device = {
	.name          = "omapdss",
	.id            = -1,
	.dev            = {
		.platform_data = &offenburg_dss_data,
	},
};

/* The TI drivers expect that supplies vdds_dsi and vdda_dac are both defined */
/* If not the there wont be a FB! So we define them both and connect them to  */
/* pll2, which powers the DSI pads (gpio70..gpio75)                           */
static struct regulator_consumer_supply santiago_vdds_dsi_supply[] = {
	{
		.supply		= "vdds_dsi",
		.dev		= &offenburg_dss_device.dev,
	},
	{
		.supply		= "vdda_dac",
		.dev		= &offenburg_dss_device.dev,
	}
};

static struct twl4030_hsmmc_info mmc[] = {
	{
		.mmc		= 1,
		.wires		= 4,
		.gpio_cd	= TT_VGPIO_CD_SD_MICRO,
		.gpio_wp	= -EINVAL,
		.power_saving	= true,
		/* NOTE: the micro-SD card is certainly not non-removable, but
		   this option is needed to make suspend work. TI reconfirmed
		   that their customers use this configuration on millions of
		   devices without having problems with data corruption, etc
		   (CONFIG_MMC_UNSAFE_RESUME, which is equivalent to
		   non-removable contains a big fat warning about that) */
		.nonremovable	= true,
	},
	{
		.mmc		= 2,
		.wires		= 8,
		.gpio_cd	= -EINVAL,
		.gpio_wp	= -EINVAL,
		.nonremovable	= true,
	},
	{
		.mmc		= 3,
		.wires		= 4,
		.gpio_wp	= -EINVAL,
		.gpio_cd	= -EINVAL,
		.ocr_mask	= MMC_VDD_32_33 | MMC_VDD_165_195,
		.power_saving	= true,
		.nonremovable	= true,
	},
	{}	/* Terminator */
};

static struct regulator_consumer_supply santiago_vmmc1_supply = {
	.supply			= "vmmc",
};

static struct regulator_consumer_supply santiago_vmmc2_supply = {
	.supply			= "vmmc",
};

static struct platform_device santiago_pmic_vgpio;

static int santiago_twl_gpio_setup(struct device *dev,
		unsigned gpio, unsigned ngpio)
{
	twl4030_mmc_init(mmc);

	santiago_vmmc1_supply.dev = mmc[0].dev;
	santiago_vmmc2_supply.dev = mmc[1].dev;

	platform_device_register(&santiago_pmic_vgpio);

	/* Fix the padconfig, so we get better suspend, no pullup on datalines */
	omap_mux_init_signal("sdmmc2_clk", OMAP_PIN_INPUT_PULLUP | OMAP_PIN_OFF_OUTPUT_LOW);
	omap_mux_init_signal("sdmmc2_cmd", OMAP_PIN_INPUT_PULLUP | OMAP_PIN_OFF_OUTPUT_LOW);
	omap_mux_init_signal("sdmmc2_dat0", OMAP_PIN_INPUT_PULLUP | OMAP_PIN_OFF_OUTPUT_LOW);
	omap_mux_init_signal("sdmmc2_dat1", OMAP_PIN_INPUT_PULLUP | OMAP_PIN_OFF_OUTPUT_LOW);
	omap_mux_init_signal("sdmmc2_dat2", OMAP_PIN_INPUT_PULLUP | OMAP_PIN_OFF_OUTPUT_LOW);
	omap_mux_init_signal("sdmmc2_dat3", OMAP_PIN_INPUT_PULLUP | OMAP_PIN_OFF_OUTPUT_LOW);
	omap_mux_init_signal("sdmmc2_dat4.sdmmc2_dat4", OMAP_PIN_INPUT_PULLUP | OMAP_PIN_OFF_OUTPUT_LOW);
	omap_mux_init_signal("sdmmc2_dat5.sdmmc2_dat5", OMAP_PIN_INPUT_PULLUP | OMAP_PIN_OFF_OUTPUT_LOW);
	omap_mux_init_signal("sdmmc2_dat6.sdmmc2_dat6", OMAP_PIN_INPUT_PULLUP | OMAP_PIN_OFF_OUTPUT_LOW);
	omap_mux_init_signal("sdmmc2_dat7.sdmmc2_dat7", OMAP_PIN_INPUT_PULLUP | OMAP_PIN_OFF_OUTPUT_LOW);

	omap_mux_init_signal("sdmmc1_clk", OMAP_PIN_INPUT_PULLUP | OMAP_PIN_OFF_OUTPUT_LOW);
	omap_mux_init_signal("sdmmc1_cmd", OMAP_PIN_INPUT_PULLUP | OMAP_PIN_OFF_OUTPUT_LOW);
	omap_mux_init_signal("sdmmc1_dat0", OMAP_PIN_INPUT_PULLUP | OMAP_PIN_OFF_OUTPUT_LOW);
	omap_mux_init_signal("sdmmc1_dat1", OMAP_PIN_INPUT_PULLUP | OMAP_PIN_OFF_OUTPUT_LOW);
	omap_mux_init_signal("sdmmc1_dat2", OMAP_PIN_INPUT_PULLUP | OMAP_PIN_OFF_OUTPUT_LOW);
	omap_mux_init_signal("sdmmc1_dat3", OMAP_PIN_INPUT_PULLUP | OMAP_PIN_OFF_OUTPUT_LOW);
	
	return 0;
}

static struct twl4030_gpio_platform_data santiago_gpio_data = {
	.gpio_base	= TWL4030_GPIO_BASE,
	.irq_base	= TWL4030_GPIO_IRQ_BASE,
	.irq_end	= TWL4030_GPIO_IRQ_END,
	.setup		= santiago_twl_gpio_setup,
};

static struct twl4030_usb_data santiago_usb_data = {
	.usb_mode	= T2_USB_MODE_ULPI,
};

/* VMMC1 is also used for level shifters !!! */
static struct regulator_init_data santiago_vmmc1 = {
	.constraints = {
		.min_uV			= 3000000,
		.max_uV			= 3000000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
		.always_on		= true,
		.boot_on		= true,
		.apply_uV		= true,
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &santiago_vmmc1_supply,
};

/* VMMC2 is used for powering sensors (not tilt) */
static struct regulator_init_data santiago_vmmc2 = {
	.constraints = {
		.min_uV			= 3150000,
		.max_uV			= 3150000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
		.always_on		= true,
		.boot_on		= true,
		.apply_uV		= true,
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &santiago_vmmc2_supply,
};

/* VPLL1 for SoC DPLL */
static struct regulator_init_data santiago_vpll1 = {
	.constraints = {
		.name			= "VDD_1V8_PLL1",
		.min_uV			= 1800000,
		.max_uV			= 1800000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
		.boot_on		= true,
	},
	.num_consumer_supplies	= 0,
};

/* VPLL2 for digital video outputs, need to be on for the pads. Not depending */
/* on DSI.                                                                    */
static struct regulator_init_data santiago_vpll2 = { 
	.constraints = {
		.name			= "VDD_1V8_PLL2", /* Note: name on schematic is 1v3 */
		.min_uV			= 1800000,
		.max_uV			= 1800000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
		.always_on		= true,
		.boot_on		= false,
		.apply_uV		= true,
	},
	.num_consumer_supplies	= ARRAY_SIZE(santiago_vdds_dsi_supply),
	.consumer_supplies	= &santiago_vdds_dsi_supply[0],
};

/* VAUX1 for i2c3 and TS */
static struct regulator_consumer_supply santiago_vaux1_supply = {
	.supply			= "vaux1",
	.dev			= NULL,
};

static struct regulator_init_data santiago_vaux1 = {
	.constraints = {
		.name			= "VDD_3V0_AUX1",
		.min_uV			= 3000000,
		.max_uV			= 3000000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
		.always_on		= true,
		.boot_on		= true,
		.apply_uV		= true,
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &santiago_vaux1_supply,
};

/* VAUX3 for BC06, BRCM4329, TS, */
static struct regulator_consumer_supply santiago_vaux3_supply = {
	.supply			= "vaux3",
	.dev			= NULL,
};

static struct regulator_init_data santiago_vaux3 = {
	.constraints = {
		.name			= "VDD_1V8_AUX3",
		.min_uV			= 1800000,
		.max_uV			= 1800000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
		.always_on		= true,
		.boot_on		= false,
		.apply_uV		= true,
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &santiago_vaux3_supply,
};

/* VAUX4 for UART, LCM */
static struct regulator_consumer_supply santiago_vaux4_supply = {
	.supply			= "vaux4",
	.dev			= NULL,
};

static struct regulator_init_data santiago_vaux4 = {
	.constraints = {
		.name			= "VDD_1V8_AUX4",
		.min_uV			= 1800000,
		.max_uV			= 1800000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
		.always_on		= true,
		.boot_on		= true,
		.apply_uV		= true,
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &santiago_vaux4_supply,
};

/* VDR_3V Regulator */

static struct regulator_consumer_supply santiago_pwr_vdr3v_consumers[] = {
	/* Accelerometer (KXR94) */
	{
		.supply		= "VDD",
		.dev_name	= "spi1.0",
	},

	{
		.supply		= "IO_VDD",
		.dev_name	= "spi1.0",
	},

	/* Pressure sensor (MS5607) */
	{
		.supply		= "VDD",
		.dev_name	= "spi1.2",
	},

	/* Angular rate sensor (EWTS98) */
	{
		.supply		= "VDD",
		.dev_name	= "ewts98",
	},
};

static struct regulator_init_data santiago_pwr_vdr3v_init_data = {
	/* .supply_regulator	= we have two of them ... */
	.constraints = {
		.valid_ops_mask = REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies = ARRAY_SIZE(santiago_pwr_vdr3v_consumers),
	.consumer_supplies = santiago_pwr_vdr3v_consumers,
};

static struct fixed_voltage_config santiago_pwr_vdr3v_pdata = {
	.supply_name		= "VDR_3V",
	.microvolts		= 3000000,
	.enabled_at_boot	= 0,
	.init_data		= &santiago_pwr_vdr3v_init_data,
	.gpio			= TT_VPGIO_DR_POWER_EN,
	.enable_high		= 1,
};

static struct platform_device santiago_pwr_vdr3v_pdev = {
	.name          = "reg-fixed-voltage",
	.id            = -1,
	.dev = {
		.platform_data = &santiago_pwr_vdr3v_pdata,
	},
};

/* mmc2 (WLAN) and Bluetooth don't use twl4030 regulators */

static struct twl4030_codec_audio_data santiago_audio_data = {
	.audio_mclk = 26000000,
};

static struct twl4030_codec_data santiago_codec_data = {
	.audio_mclk = 26000000,
	.audio = &santiago_audio_data,
};

/* mmc2 (WLAN) and Bluetooth don't use twl4030 regulators */

static struct twl4030_madc_platform_data santiago_madc_data = {
	.irq_line	= 1,
};

/*
 * Sequence to control the TRITON Power resources,
 * when the system goes into sleep.
 * Executed upon P1_P2/P3 transition for sleep.
 */
static struct twl4030_ins __initdata santiago_sleep_on_seq[] = {
	/* Broadcast message to put res to sleep */
	{MSG_BROADCAST(DEV_GRP_ALL, RES_GRP_ALL, RES_TYPE_ALL, RES_TYPE2_R0, RES_STATE_SLEEP), 2},
	{MSG_BROADCAST(DEV_GRP_ALL, RES_GRP_ALL, RES_TYPE_R0, RES_TYPE2_R1, RES_STATE_SLEEP), 2},
	{MSG_BROADCAST(DEV_GRP_ALL, RES_GRP_ALL, RES_TYPE_R0, RES_TYPE2_R2, RES_STATE_SLEEP), 2},
};

static struct twl4030_script santiago_sleep_on_script __initdata = {
	.script	= santiago_sleep_on_seq,
	.size	= ARRAY_SIZE(santiago_sleep_on_seq),
	.flags	= TWL4030_SLEEP_SCRIPT,
};

/*
 * Sequence to control the TRITON Power resources,
 * when the system wakeup from sleep.
 * Executed upon P1_P2 transition for wakeup.
 */
static struct twl4030_ins santiago_wakeup_p12_seq[] __initdata = {
	/* Broadcast message to put res to active */
	{MSG_BROADCAST(DEV_GRP_ALL, RES_GRP_ALL, RES_TYPE_R0, RES_TYPE2_R2, RES_STATE_ACTIVE), 2},
	{MSG_BROADCAST(DEV_GRP_ALL, RES_GRP_ALL, RES_TYPE_R0, RES_TYPE2_R1, RES_STATE_ACTIVE), 2},
	{MSG_BROADCAST(DEV_GRP_ALL, RES_GRP_ALL, RES_TYPE_ALL, RES_TYPE2_R0, RES_STATE_ACTIVE), 2},
};

static struct twl4030_script santiago_wakeup_p12_script __initdata = {
	.script	= santiago_wakeup_p12_seq,
	.size	= ARRAY_SIZE(santiago_wakeup_p12_seq),
	.flags	= TWL4030_WAKEUP12_SCRIPT,
};

/*
 * Sequence to control the TRITON Power resources,
 * when the system wakeup from sleep.
 * Executed upon P3 transition for wakeup.
 */
static struct twl4030_ins santiago_wakeup_p3_seq[] __initdata = {
	/* Broadcast message to put res to active */
	{MSG_BROADCAST(DEV_GRP_ALL, RES_GRP_ALL, RES_TYPE_ALL, RES_TYPE2_R2, RES_STATE_ACTIVE), 2},
};

static struct twl4030_script santiago_wakeup_p3_script __initdata = {
	.script = santiago_wakeup_p3_seq,
	.size   = ARRAY_SIZE(santiago_wakeup_p3_seq),
	.flags  = TWL4030_WAKEUP3_SCRIPT,
};

/*
 * Sequence to reset the TRITON Power resources,
 * when the system gets warm reset.
 * Executed upon warm reset signal.
 */
static struct twl4030_ins santiago_wrst_seq[] __initdata = {
/*
 * Set the TRITON_RESET resource to off mode.
 * Reset Main_Ref.
 * turn off P1 group (type2_group0).
 * Reset PP All 
 * Reset RC.
 * Release the TRITON_RESET by setting resource to on mode.
 ******
 * On the turn off of P1 we have to add delay to make sure everything powers
 * down. The delay is supposed to be in 32khz cycles, but it is not. This is 
 * rough measurement on some numbers:
 * 50 : 2.9 ms 
 * 60 : 3.4 ms 
 * 70 : 7.2 ms 
 * 80 : 8.2 ms 
 * 90 : 10 ms 
 * 100 : 24 ms
 * For P1 we wait 24msec to make sure MMC1 is completely reset. Failures were
 * observed on SD boot when delay is not added.
 */
	{MSG_SINGULAR( DEV_GRP_NULL, RES_RESET, RES_STATE_OFF), 2},
	{MSG_SINGULAR( DEV_GRP_NULL, RES_Main_Ref, RES_STATE_WRST), 2},
	{MSG_BROADCAST(DEV_GRP_P1,   RES_GRP_PP, RES_TYPE_R0, RES_TYPE2_R0, RES_STATE_OFF), 100}, 
	{MSG_BROADCAST(DEV_GRP_NULL, RES_GRP_PP, RES_TYPE_ALL, RES_TYPE_ALL, RES_STATE_WRST), 2},
	{MSG_BROADCAST(DEV_GRP_NULL, RES_GRP_RC, RES_TYPE_ALL, RES_TYPE2_R0, RES_STATE_WRST), 2},
	{MSG_SINGULAR( DEV_GRP_NULL, RES_RESET, RES_STATE_ACTIVE), 2},
};

static struct twl4030_script santiago_wrst_script __initdata = {
	.script = santiago_wrst_seq,
	.size   = ARRAY_SIZE(santiago_wrst_seq),
	.flags  = TWL4030_WRST_SCRIPT,
};

/* TRITON script for sleep, wakeup & warm_reset */
static struct twl4030_script *santiago_twl4030_scripts[] __initdata = {
	&santiago_wakeup_p12_script,
	&santiago_wakeup_p3_script,
	&santiago_sleep_on_script,
	&santiago_wrst_script,
};

static struct twl4030_resconfig santiago_twl4030_rconfig[] = {
	{ .resource = RES_VAUX1,      .devgroup = DEV_GRP_P1,               .type = 0, .type2 = 0, .remap_sleep = RES_STATE_OFF },
	{ .resource = RES_VAUX3,      .devgroup = DEV_GRP_P1,               .type = 0, .type2 = 0, .remap_sleep = RES_STATE_OFF },
	{ .resource = RES_VAUX4,      .devgroup = DEV_GRP_P1,               .type = 0, .type2 = 0, .remap_sleep = RES_STATE_OFF },
	{ .resource = RES_VMMC1,      .devgroup = DEV_GRP_P1,               .type = 0, .type2 = 0, .remap_sleep = RES_STATE_OFF },
	{ .resource = RES_VMMC2,      .devgroup = DEV_GRP_P1,               .type = 0, .type2 = 0, .remap_sleep = RES_STATE_OFF },
	{ .resource = RES_VPLL1,      .devgroup = DEV_GRP_P1,               .type = 3, .type2 = 1, .remap_sleep = RES_STATE_OFF }, 
	{ .resource = RES_VPLL2,      .devgroup = DEV_GRP_P1,               .type = 0, .type2 = 1, .remap_sleep = RES_STATE_OFF },
	{ .resource = RES_VINTANA1,   .devgroup = DEV_GRP_P1 | DEV_GRP_P3,  .type = 1, .type2 = 2, .remap_sleep = RES_STATE_SLEEP },
	{ .resource = RES_VINTANA2,   .devgroup = DEV_GRP_P1 | DEV_GRP_P3,  .type = 0, .type2 = 2, .remap_sleep = RES_STATE_SLEEP },
	{ .resource = RES_VINTDIG,    .devgroup = DEV_GRP_P1 | DEV_GRP_P3,  .type = 1, .type2 = 2, .remap_sleep = RES_STATE_SLEEP },
	{ .resource = RES_VIO,        .devgroup = DEV_GRP_P1 | DEV_GRP_P3,  .type = 2, .type2 = 2, .remap_sleep = RES_STATE_SLEEP },
	{ .resource = RES_VDD1,       .devgroup = DEV_GRP_P1,               .type = 4, .type2 = 1, .remap_sleep = RES_STATE_OFF },
	{ .resource = RES_VDD2,       .devgroup = DEV_GRP_P1,               .type = 3, .type2 = 1, .remap_sleep = RES_STATE_OFF },
	{ .resource = RES_NRES_PWRON, .devgroup = DEV_GRP_ALL,              .type = 0, .type2 = 1, .remap_sleep = RES_STATE_SLEEP },
	{ .resource = RES_CLKEN,      .devgroup = DEV_GRP_P1 | DEV_GRP_P3,  .type = 3, .type2 = 2, .remap_sleep = RES_STATE_SLEEP },
	{ .resource = RES_SYSEN,      .devgroup = DEV_GRP_P1 | DEV_GRP_P3,  .type = 6, .type2 = 2, .remap_sleep = RES_STATE_OFF },
	{ .resource = RES_HFCLKOUT,   .devgroup = DEV_GRP_P3,               .type = 0, .type2 = 2, .remap_sleep = RES_STATE_SLEEP },
	{ 0, 0},
};

static struct twl4030_power_data santiago_t2scripts_data __initdata = {
	.scripts	= santiago_twl4030_scripts,
	.num		= ARRAY_SIZE(santiago_twl4030_scripts),
	.resource_config = santiago_twl4030_rconfig,
};

static struct twl4030_clock_init_data santiago_clock_data = {
	.clk_slicer_bypass = 1,	
	.high_perf_sq_disable = 1,	
};

static struct twl4030_platform_data santiago_twldata = {
	.irq_base	= TWL4030_IRQ_BASE,
	.irq_end	= TWL4030_IRQ_END,
	.clock		= &santiago_clock_data,
	.gpio		= &santiago_gpio_data,
	.madc		= &santiago_madc_data,
	.usb		= &santiago_usb_data,
	.power		= &santiago_t2scripts_data,
	.codec		= &santiago_codec_data,
	/* regulators: */
	.vdac		= NULL, /* not used */
	.vaux1		= &santiago_vaux1,
	.vaux2		= NULL, /* not used */
	.vaux3		= &santiago_vaux3,
	.vpll1		= &santiago_vpll1,
	.vpll2		= &santiago_vpll2,
	.vmmc1		= &santiago_vmmc1,
	.vmmc2		= &santiago_vmmc2,
	.vsim		= NULL,
	.vaux4		= &santiago_vaux4,
	.vio		= NULL, /* Main IO regulator 1V8, no init.  */
	.vdd1		= NULL, /* Main SoC regulator 1V2, no init. */
	.vdd2		= NULL, /* Main SoC regulator 1V2, no init. */
	.vintana1	= NULL,
	.vintana2	= NULL,
	.vintdig	= NULL
};

static struct i2c_board_info __initdata santiago_i2c_boardinfo[] = {
	{
		I2C_BOARD_INFO("tps65950", 0x48),
		.flags = I2C_CLIENT_WAKE,
		.irq = INT_34XX_SYS_NIRQ,
		.platform_data = &santiago_twldata,
	},
};

static void __init santiago_i2c_init(void)
{
	omap_register_i2c_bus(1, 400, NULL, santiago_i2c_boardinfo,
			ARRAY_SIZE(santiago_i2c_boardinfo));
	omap_register_i2c_bus(2, 100, NULL, NULL, 0);
	omap_register_i2c_bus(3, 50, NULL, NULL, 0);
}

static void __init santiago_init_irq(void)
{
	omap_init_irq();
	omap2_init_common_hw(mt46h128m32l2kq5_sdrc_params,
	     	 	     mt46h128m32l2kq5_sdrc_params,
			     omap3630_mpu_rate_table,
			     omap3630_dsp_rate_table,
			     omap3630_l3_rate_table);
}

static void santiago_syson_init(void)
{
	u32 reg;

	/* Enable GPIO_126 and GPIO_127 */
	reg = omap_ctrl_readl(OMAP343X_CONTROL_WKUP_CTRL);
	reg |= OMAP343X_GPIO_IO_PWRDNZ;
	omap_ctrl_writel(reg, OMAP343X_CONTROL_WKUP_CTRL);
}

static void santiago_power_off(void)
{
	/* Delay 500 ms to avoid corrupting the MoviNAND */
	mdelay (500);
	
	twl_i2c_write_u8(TWL4030_MODULE_PM_MASTER, 0x01, 0x10);
	
	while (1) {;}
}

static struct platform_device *santiago_devices[] __initdata = {
	&offenburg_dss_device,
	&santiago_pwr_vdr3v_pdev
};

#ifdef CONFIG_OMAP_MUX
static struct omap_board_mux board_mux[] __initdata = {

	PADCONFIG_SETTINGS_COMMON
	PADCONFIG_SETTINGS_KERNEL

	{ .reg_offset = OMAP_MUX_TERMINATOR },
};
#else
#define board_mux	NULL
#endif

static struct omap_musb_board_data musb_board_data = {
	.interface_type		= MUSB_INTERFACE_ULPI,
	.mode			= MUSB_OTG,
	.power			= 100,
};

struct vgpio_pin santiago_soc_vgpio_pins[] = {
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_GPS_RESET,		160),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_GPS_POWER),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_GPS_TCXO_ON),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_GPS_ANT_SHORT),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_GPS_1PPS,		42),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_BT_RST,		60),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_LCD_PWR_ON,		40),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_LCD_RESET,		156),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_LCD_STBY,		41),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_TP_IRQ,		157),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_TP_RESET,		54),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_CAM_PWR_ON,		96),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_ON_OFF,		55),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_TILT_PWR),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_TILT_OUT,		49),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_USB_DETECT,		52),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_USB_CHRG_DET,		53),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_WALL_SENSE_CONNECT,	186),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_AMP_PWR_EN,		127),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_GSM_SYS_EN,		37),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_GSM_SYS_RST,		50),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_AUX_MOVI,		27),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_AUX_SD,		28),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_CD_SD_MICRO,		39),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VPGIO_DR_POWER_EN,		24),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_EEPROM_WP,		46),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_BACKLIGHT_DIM),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_WIFI_RST,		29),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_BT_WAKE,		16),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_BT_HOST_WAKE,		152),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_BT_RST,		60),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_BT_EN,		14),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_WIFI_EN,		15),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_ACCESSORY_PWREN,	126),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_BACKLIGHT_ON),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_SENSE_11V0),
};

struct vgpio_pin santiago_pmic_vgpio_pins[] = {
	VGPIO_DEF_PIN    (TT_PMIC_VGPIO_BASE, TT_VGPIO_LCM_PWR_ON,	TWL4030_GPIO_BASE +  7),
	VGPIO_DEF_PIN    (TT_PMIC_VGPIO_BASE, TT_VGPIO_LCM_BL_PWR_ON,	TWL4030_GPIO_BASE + 13),
};

static struct vgpio_platform_data santiago_soc_vgpio_pdata =
{
        .gpio_base      = TT_VGPIO_BASE,
        .gpio_number    = ARRAY_SIZE(santiago_soc_vgpio_pins),
        .pins           = santiago_soc_vgpio_pins,
};

static struct platform_device santiago_soc_vgpio =
{
        .name   = "vgpio",
        .id     = 0,
        .dev    = {
                .platform_data  = &santiago_soc_vgpio_pdata,
        },
};

static struct vgpio_platform_data santiago_pmic_vgpio_pdata =
{
        .gpio_base      = TT_PMIC_VGPIO_BASE,
        .gpio_number    = ARRAY_SIZE(santiago_pmic_vgpio_pins),
        .pins           = santiago_pmic_vgpio_pins,
};

static struct platform_device santiago_pmic_vgpio =
{
        .name   = "vgpio",
        .id     = 1,
        .dev    = {
                .platform_data  = &santiago_pmic_vgpio_pdata,
        },
};

extern void config_bcm4329_power(void);

#define DEFAULT_LCM		"ld050wv1sp01"
#define REV5_LCM		"lms501kf03"

static void santiago_patch_revision(void)
{
	unsigned long revision = fdt_get_ulong("/features", "hardware-version", 0x4);
	const char *screen = fdt_get_string ("/features", "tft", DEFAULT_LCM);
	u32	reg;

	/* 
	 * Differences between Rev3 and Rev4, default configuration revision 4
	 *          Rev 4            Rev 3
	 * GPIO_37  GSM_SYS_EN       AMP_PWR_EN
	 * GPIO_127 AMP_PWR_EN       GSM_SYS_EN
	 * GPIO_126 ACCESSORY_PWR_EN GSM_SYS_RST
	 * GPIO_50  GSM_SYS_RST      ACCESSORY_PWR_EN
	 */
	switch (revision) {
		case 3:
			santiago_soc_vgpio_pins[TT_VGPIO_GSM_SYS_EN - TT_VGPIO_BASE].tpin = 127;
			santiago_soc_vgpio_pins[TT_VGPIO_GSM_SYS_RST - TT_VGPIO_BASE].tpin = 126;
			santiago_soc_vgpio_pins[TT_VGPIO_AMP_PWR_EN - TT_VGPIO_BASE].tpin = 37;
			santiago_soc_vgpio_pins[TT_VGPIO_ACCESSORY_PWREN - TT_VGPIO_BASE].tpin = 50;
			/* Drop through to configure PBIAS */
			/* See the comment below for reasoning - on a rev 3 board this allows the GPRS */
			/* control signals to work */
		case 4:
		case 5:
			reg = omap_ctrl_readl(OMAP343X_CONTROL_PBIAS_LITE);

			if ((reg & OMAP343X_PBIASLITESUPPLY_HIGH1) == 0) {
				/* 
				* If SIM_VDD not set meaning we booted from MOVI which breaks
				* the functionality of Extended Drian I/O (so AMP_PWR_EN)
				* We have to set 3v0 on PBIAS1 ourselfs
				*/
				reg |= OMAP343X_PBIASLITEVMODE1;
				reg |= OMAP343X_PBIASLITEPWRDNZ1;
				omap_ctrl_writel(reg, OMAP343X_CONTROL_PBIAS_LITE);
			}
			break;
		default:
			break;
	}

	/* Depending on the facvtory data select correct display */
	if (!strcmp(REV5_LCM, screen)) {
		santiago_spi_board_info[2] = santiago_lms501kf03_board_info;
	}
}

static void __init santiago_pm_init(void)
{
	int irq;	

	/* Enable the wakeup IRQ. So in suspend the press of powerbutton will */
	/* wake the system up. Only set IRQ !! the rest is done elsewhere     */
	irq = gpio_to_irq(TT_VGPIO_ON_OFF);
	enable_irq_wake(irq);
	
	/* Be sure CAM_PWR_ON is initialized and set to 0. If not then it */
	/* floats and turns on LDOs. These consume 0.8mA in total.        */
	gpio_request(TT_VGPIO_CAM_PWR_ON, "CAM power"); 
	gpio_direction_output(TT_VGPIO_CAM_PWR_ON, 0); 
}

static void __init santiago_init(void)
{

	omap3_mux_init(board_mux, OMAP_PACKAGE_CBP);

	/* Execute before registering vgpios */
	santiago_patch_revision();

	platform_device_register(&santiago_soc_vgpio);
#if defined (CONFIG_TOMTOM_BCM4329) && defined (CONFIG_BCM4329)
	config_bcm4329_power();
#endif
	santiago_syson_init();
	santiago_i2c_init();
	platform_add_devices(santiago_devices, ARRAY_SIZE(santiago_devices));

	omap_serial_init();
	usb_musb_init(&musb_board_data);

	printk(KERN_INFO "Registering spi board_info\n");
	spi_register_board_info(santiago_spi_board_info, 
		ARRAY_SIZE(santiago_spi_board_info));

	/* Ensure SDRC pins are mux'd for self-refresh */
	omap_mux_init_signal("sdrc_cke0", OMAP_PIN_OUTPUT);
	omap_mux_init_signal("sdrc_cke1", OMAP_PIN_OUTPUT);

	/* Register Santiago specific power-off method */
	pm_power_off = santiago_power_off;
	santiago_pm_init();
}

static void __init santiago_map_io(void)
{
	omap_vram_set_sdram_vram (SANTIAGO_VRAM_SIZE*1024*1024, SANTIAGO_VRAM_ADDR);

	fdt_reserve_bootmem();

	omap2_set_globals_343x();
	omap2_map_common_io();
}

MACHINE_START(SANTIAGO, "Santiago")
	.phys_io	= 0x48000000,
	.io_pg_offst	= ((0xfa000000) >> 18) & 0xfffc,
	.boot_params	= 0x80000100,
	.map_io		= santiago_map_io,
	.init_irq	= santiago_init_irq,
	.init_machine	= santiago_init,
	.timer		= &omap_timer,
MACHINE_END
