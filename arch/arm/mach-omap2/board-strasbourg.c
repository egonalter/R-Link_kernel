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
#include <linux/platform_device.h>
#include <linux/i2c/twl.h>
#include <linux/regulator/machine.h>
#include <linux/pm.h>
#include <linux/vgpio.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>
#include <asm/mach/flash.h>
#include <asm/mach/map.h>

#include <plat/board.h>
#include <plat/common.h>
#include <plat/display.h>
#include <mach/gpio.h>
#include <plat/gpmc.h>
#include <mach/hardware.h>
#include <plat/nand.h>
#include <plat/usb.h>
#include <plat/offenburg.h>
#include <plat/control.h>
#include <plat/vram.h>
#include <plat/fdt.h>

#include "mux.h"
#include "sdram-micron-mt46h32m32lf-6.h"
#include "mmc-twl4030.h"
#include "pm.h"
#include "omap3-opp.h"
#include "twl4030.h"

/* Before including the padconfig settings, we have to set proper config */
#define KERNEL_PADCONFIG
#include <mach/padconfig_strasbourg.h>

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
		.size           = 0x1F0000,		/* Avoid the last 4 sectors */
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

/* XXX Set these up properly when we have final NOR chip */

#if 0 /* These values work in theory with s29ns016j, but not in practice */

static struct gpmc_timings strasbourg_flash_timings = {
	/* Minimum clock period for synchronous mode */
	.sync_clk = 0,

	/* Chip-select signal timings corresponding to GPMC_CS_CONFIG2 */
	.cs_on = 0,		/* Assertion time */
	.cs_rd_off = 90,	/* Read deassertion time */
	.cs_wr_off = 100,	/* Write deassertion time - 98.5 ns */

	/* ADV signal timings corresponding to GPMC_CONFIG3 */
	.adv_on = 5,		/* Assertion time */
	.adv_rd_off = 20,	/* Read deassertion time */
	.adv_wr_off = 15,	/* Write deassertion time */

	/* WE signals timings corresponding to GPMC_CONFIG4 */
	.we_on = 0,		/* WE assertion time */
	.we_off = 80,		/* WE deassertion time */

	/* OE signals timings corresponding to GPMC_CONFIG4 */
	.oe_on = 20,		/* OE assertion time */
	.oe_off = 90,		/* OE deassertion time */

	/* Access time and cycle time timings corresponding to GPMC_CONFIG5 */
	.page_burst_access = 5,	/* Multiple access word delay */
	.access = 75,		/* Start-cycle to first data valid delay */
	.rd_cycle = 100,	/* Total read cycle time */
	.wr_cycle = 100,	/* Total write cycle time */

	/* The following are only on OMAP3430 */
	.wr_data_mux_bus = 15,  /* WRACCESSTIME */
	.wr_access = 75		/* WRDATAONADMUXBUS */
};

#else /* These values work in practice with s29ns016j, but not in theory */

static struct gpmc_timings strasbourg_flash_timings = {
	/* Minimum clock period for synchronous mode */
	.sync_clk = 0,

	/* Chip-select signal timings corresponding to GPMC_CS_CONFIG2 */
	.cs_on = 5,		/* Assertion time */
	.cs_rd_off = 80,	/* Read deassertion time */
	.cs_wr_off = 80,	/* Write deassertion time - 98.5 ns */

	/* ADV signal timings corresponding to GPMC_CONFIG3 */
	.adv_on = 5,		/* Assertion time */
	.adv_rd_off = 10,	/* Read deassertion time */
	.adv_wr_off = 10,	/* Write deassertion time */

	/* WE signals timings corresponding to GPMC_CONFIG4 */
	.we_on = 15,		/* WE assertion time */
	.we_off = 80,		/* WE deassertion time */

	/* OE signals timings corresponding to GPMC_CONFIG4 */
	.oe_on = 15,		/* OE assertion time */
	.oe_off = 80,		/* OE deassertion time */

	/* Access time and cycle time timings corresponding to GPMC_CONFIG5 */
	.page_burst_access = 5,	/* Multiple access word delay */
	.access = 75,		/* Start-cycle to first data valid delay */
	.rd_cycle = 85,		/* Total read cycle time */
	.wr_cycle = 85,		/* Total write cycle time */

	/* The following are only on OMAP3430 */
	.wr_data_mux_bus = 15,  /* WRACCESSTIME */
	.wr_access = 75		/* WRDATAONADMUXBUS */
};

#endif

static void __init strasbourg_flash_init(void)
{
	u32 regVal;
	unsigned long base;

	regVal = gpmc_cs_read_reg (STRASBOURG_NORFLASH_CS, GPMC_CS_CONFIG1);
	regVal |= GPMC_CONFIG1_WAIT_READ_MON | GPMC_CONFIG1_WAIT_WRITE_MON | GPMC_CONFIG1_MUXADDDATA;
	gpmc_cs_write_reg (STRASBOURG_NORFLASH_CS, GPMC_CS_CONFIG1, regVal);

	if (gpmc_cs_set_timings(STRASBOURG_NORFLASH_CS, &strasbourg_flash_timings)) {
		printk(KERN_ERR "Unable to set timings for CS%d\n", STRASBOURG_NORFLASH_CS);
		return;
	}

	if (gpmc_cs_request(STRASBOURG_NORFLASH_CS, SZ_2M, &base)) {
		printk(KERN_ERR "Failed to request GPMC mem for NOR flash\n");
		return;
	}
	strasbourg_nor_resource.start = base;
	strasbourg_nor_resource.end = base + SZ_2M - 1;

	if (platform_device_register(&strasbourg_nor_device) < 0)
		printk(KERN_ERR "Unable to register NAND device\n");
}

static struct __initdata twl4030_power_data strasbourg_t2scripts_data;

static struct twl4030_hsmmc_info mmc[] = {
	{
		.mmc		= 1,
		.wires		= 4,
		.gpio_cd	= -EINVAL,
		.gpio_wp	= -EINVAL,
	},
	{
		.mmc		= 2,
		.wires		= 8,
		.gpio_cd	= -EINVAL,
		.gpio_wp	= -EINVAL,
//		.transceiver	= true,
//		.ocr_mask	= 0x00100000,	/* 3.3V */
	},
	{}	/* Terminator */
};

static struct regulator_consumer_supply strasbourg_vmmc1_supply = {
	.supply			= "vmmc",
};

static struct regulator_consumer_supply strasbourg_vmmc2_supply = {
	.supply			= "vmmc",
};

static struct platform_device strasbourg_pmic_vgpio;

static int strasbourg_twl_gpio_setup(struct device *dev,
		unsigned gpio, unsigned ngpio)
{
	twl4030_mmc_init(mmc);

	strasbourg_vmmc1_supply.dev = mmc[0].dev;
	strasbourg_vmmc2_supply.dev = mmc[1].dev;

	platform_device_register(&strasbourg_pmic_vgpio);

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
		.min_uV			= 1850000,
		.max_uV			= 3150000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_VOLTAGE
					| REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &strasbourg_vmmc1_supply,
};

static struct regulator_init_data strasbourg_vmmc2 = {
	.constraints = {
		.min_uV			= 1850000,
		.max_uV			= 1850000,
		.valid_modes_mask	= REGULATOR_MODE_NORMAL
					| REGULATOR_MODE_STANDBY,
		.valid_ops_mask		= REGULATOR_CHANGE_VOLTAGE
					| REGULATOR_CHANGE_MODE
					| REGULATOR_CHANGE_STATUS,
	},
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &strasbourg_vmmc2_supply,
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

/* VAUX3 for i2c2 */
static struct regulator_consumer_supply strasbourg_vaux3_supply = {
	.supply			= "vaux3",
	.dev			= NULL,
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
	.num_consumer_supplies	= 1,
	.consumer_supplies	= &strasbourg_vaux3_supply,
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

static struct twl4030_platform_data strasbourg_twldata = {
	.irq_base	= TWL4030_IRQ_BASE,
	.irq_end	= TWL4030_IRQ_END,
	.gpio		= &strasbourg_gpio_data,
	.madc		= &strasbourg_madc_data,
	.usb		= &strasbourg_usb_data,
	.power		= &strasbourg_t2scripts_data,
	.codec		= &strasbourg_codec_data,
	.vmmc1		= &strasbourg_vmmc1,
	.vmmc2		= &strasbourg_vmmc2,
	.vdac		= &strasbourg_vdac,
	.vpll2		= &strasbourg_vpll2,
	.vaux1		= &strasbourg_vaux1,
	.vaux3		= &strasbourg_vaux3,
};

static struct i2c_board_info __initdata strasbourg_i2c_boardinfo[] = {
	{
		I2C_BOARD_INFO("tps65950", 0x48),
		.flags = I2C_CLIENT_WAKE,
		.irq = INT_34XX_SYS_NIRQ,
		.platform_data = &strasbourg_twldata,
	},
};

static int __init strasbourg_i2c_init(void)
{
	omap_register_i2c_bus(1, 400, NULL, strasbourg_i2c_boardinfo,
			ARRAY_SIZE(strasbourg_i2c_boardinfo));
	omap_register_i2c_bus(2, 100, NULL, NULL, 0);
	omap_register_i2c_bus(3, 50, NULL, NULL, 0);
	return 0;
}

static struct platform_device offenburg_usb_stat_device = {
	.name	= "usb_status",
};

static void __init strasbourg_usb_stat_init(void)
{
	/* USB1 power fault line */
	gpio_request(TT_VGPIO_USB1_POWER_FAULT, "USB1_POWER_FAULT");
	gpio_direction_input(TT_VGPIO_USB1_POWER_FAULT);
	gpio_export(TT_VGPIO_USB1_POWER_FAULT, 0);
	gpio_export_link(&offenburg_usb_stat_device.dev, "usb1_power_fault", TT_VGPIO_USB1_POWER_FAULT);
	
	/* USB3 power fault line */
	gpio_request(TT_VGPIO_USB3_POWER_FAULT, "USB3_POWER_FAULT");
	gpio_direction_input(TT_VGPIO_USB3_POWER_FAULT);
	gpio_export(TT_VGPIO_USB3_POWER_FAULT, 0);
	gpio_export_link(&offenburg_usb_stat_device.dev, "usb3_power_fault", TT_VGPIO_USB3_POWER_FAULT);
}

static void __init strasbourg_special_gpios_init(void)
{
	u32 reg;

	/* Enable GPIO_126 and GPIO_129 */
	reg = omap_ctrl_readl(OMAP343X_CONTROL_WKUP_CTRL);
	reg |= OMAP343X_GPIO_IO_PWRDNZ;
	omap_ctrl_writel(reg, OMAP343X_CONTROL_WKUP_CTRL);
}

static void __init strasbourg_apple_auth_init(void)
{
	/* Enable Apple chip */
	gpio_request(TT_VGPIO_APPLE_AUTH_RST, "APPLE_AUTH_RST");
	gpio_direction_output (TT_VGPIO_APPLE_AUTH_RST, 0);
}

static void __init strasbourg_1v8_init(void)
{
	gpio_request(TT_VGPIO_1V8_ON, "1V8_ON");
	gpio_direction_output (TT_VGPIO_1V8_ON, 1);
}

static void __init strasbourg_init_irq(void)
{
    omap_init_irq();
    omap2_init_common_hw(mt46h32m32lf6_sdrc_params,
	     	 	     mt46h32m32lf6_sdrc_params,
			     omap3630_mpu_rate_table,
			     omap3630_dsp_rate_table,
			     omap3630_l3_rate_table);
}

static void strasbourg_syson_init (void)
{
	gpio_request(TT_VGPIO_PWR_HOLD, "PWR HOLD");
	gpio_direction_output(TT_VGPIO_PWR_HOLD, 1);
}

static void strasbourg_power_off(void)
{
	/* Sleep 500 ms to avoid corrupting the MoviNAND */
	msleep (500); /* ==> mdelay !? */

	/* This is the end, my only friend. The end. */
	gpio_set_value(TT_VGPIO_PWR_HOLD, 0);
}

/**** */

static struct platform_device *strasbourg_devices[] __initdata = {
	&offenburg_dss_device,
	&offenburg_usb_stat_device,
};

static const struct usbhs_omap_board_data usbhs_bdata __initconst = {
	.port_mode[0] = OMAP_EHCI_PORT_MODE_PHY,
	.port_mode[1] = OMAP_EHCI_PORT_MODE_PHY,
	.port_mode[2] = OMAP_USBHS_PORT_MODE_UNUSED,

	.phy_reset  = true,
	.reset_gpio_port[0]  = TT_VGPIO_USB1_RESET,
	.reset_gpio_port[1]  = TT_VGPIO_USB2_RESET,
	.reset_gpio_port[2]  = -EINVAL
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

static struct vgpio_pin strasbourg_soc_vgpio_pins[] =
{
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_GPS_RESET,		156),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_GPS_POWER,		 65),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_BT_RST,		 61),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_LCM_PWR_ON, 		 39),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_LCD_PWR_ON, 		 40),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_LCD_RESET, 		 38),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_LCD_STBY, 		 41),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_TP_IRQ, 		 54),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_USB1_RESET, 		109),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_USB2_RESET, 		110),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_USB1_POWER_FAULT,	 31),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_USB3_POWER_FAULT,	 11),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_CAM_IRQ, 		111),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_CAM_PWR_ON, 		 96),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_CAM_RST, 		 98),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_CAN_SYNC, 		167),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_CAN_RST,	 	 43),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_CAN_RST_MON,		170),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_CAN_BT_MD, 		186),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_APPLE_AUTH_RST,	126),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_1V8_ON,		129),
	VGPIO_DEF_PIN    (TT_VGPIO_BASE, TT_VGPIO_PWR_HOLD, 		 42),
	VGPIO_DEF_INVPIN (TT_VGPIO_BASE, TT_VGPIO_ON_OFF,		 10),
	VGPIO_DEF_PIN	 (TT_VGPIO_BASE, TT_VGPIO_GPS_1PPS,		 60),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_EEPROM_WP		   ),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_BACKLIGHT_DIM		   ),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_CAM_ON		   ),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_GPS_TCXO_ON		   ),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_GPS_ANT_SHORT),
	VGPIO_DEF_NCPIN  (TT_VGPIO_BASE, TT_VGPIO_SENSE_11V0),
};

static struct vgpio_pin strasbourg_pmic_vgpio_pins[] =
{
	VGPIO_DEF_PIN(TT_PMIC_VGPIO_BASE, TT_VGPIO_LCM_BL_PWR_ON,	TWL4030_GPIO_BASE + 13),
	VGPIO_DEF_PIN(TT_PMIC_VGPIO_BASE, TT_VGPIO_3V3_ON,		TWL4030_GPIO_BASE +  2),
	VGPIO_DEF_PIN(TT_PMIC_VGPIO_BASE, TT_VGPIO_6V_ON,		TWL4030_GPIO_BASE + 15),
	VGPIO_DEF_PIN(TT_PMIC_VGPIO_BASE, TT_VGPIO_5V75_ON,		TWL4030_GPIO_BASE +  6),
	VGPIO_DEF_PIN(TT_PMIC_VGPIO_BASE, TT_VGPIO_5V_ON,		TWL4030_GPIO_BASE +  7),
};

static struct vgpio_platform_data strasbourg_soc_vgpio_pdata =
{
	.gpio_base      = TT_VGPIO_BASE,
	.gpio_number    = ARRAY_SIZE(strasbourg_soc_vgpio_pins),
	.pins           = strasbourg_soc_vgpio_pins,
};

static struct platform_device strasbourg_soc_vgpio =
{
	.name   = "vgpio",
	.id     = 0,
	.dev    = {
		.platform_data  = &strasbourg_soc_vgpio_pdata,
	},
};

static struct vgpio_platform_data strasbourg_pmic_vgpio_pdata =
{
	.gpio_base      = TT_PMIC_VGPIO_BASE,
	.gpio_number    = ARRAY_SIZE(strasbourg_pmic_vgpio_pins),
	.pins           = strasbourg_pmic_vgpio_pins,
};

static struct platform_device strasbourg_pmic_vgpio =
{
	.name   = "vgpio",
	.id     = 1,
	.dev    = {
		.platform_data  = &strasbourg_pmic_vgpio_pdata,
	},
};

static void __init strasbourg_init(void)
{
	omap3_mux_init(board_mux, OMAP_PACKAGE_CUS);
	twl4030_get_scripts(&strasbourg_t2scripts_data);

	platform_device_register(&strasbourg_soc_vgpio);

	strasbourg_special_gpios_init();
	strasbourg_i2c_init();
	strasbourg_syson_init();
	platform_add_devices(strasbourg_devices, ARRAY_SIZE(strasbourg_devices));

	omap_serial_init();
	strasbourg_flash_init();
	strasbourg_usb_stat_init();
	usb_musb_init(&musb_board_data);
	usbhs_init(&usbhs_bdata);
	strasbourg_apple_auth_init();
	strasbourg_1v8_init();

	/* Ensure SDRC pins are mux'd for self-refresh */
	omap_mux_init_signal("sdrc_cke0", OMAP_PIN_OUTPUT);
	omap_mux_init_signal("sdrc_cke1", OMAP_PIN_OUTPUT);

	/* Register Strasbourg specific power-off method */
	pm_power_off = strasbourg_power_off;
}

static void __init strasbourg_map_io(void)
{
	/* Set the VRAM size appropriately, needs to be done 
	 * before map_common_io is called */
	omap_vram_set_sdram_vram (CONFIG_OMAP2_VRAM_SIZE*1024*1024, STRASBOURG_VRAM_ADDR);

	fdt_reserve_bootmem();

	omap2_set_globals_343x();
	omap2_map_common_io();
}

MACHINE_START(STRASBOURG, "Strasbourg")
	.phys_io	= 0x48000000,
	.io_pg_offst	= ((0xfa000000) >> 18) & 0xfffc,
	.boot_params	= 0x80000100,
	.map_io		= strasbourg_map_io,
	.init_irq	= strasbourg_init_irq,
	.init_machine	= strasbourg_init,
	.timer		= &omap_timer,
MACHINE_END
