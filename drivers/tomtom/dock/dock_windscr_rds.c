/*
 * Windscreen dock with RDSTMC receiver
 *
 *
 *  Copyright (C) 2008 by TomTom International BV. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as 
 * published by the Free Software Foundation.
 *
 *
 * ** See include/asm-arm/plat-tomtom/dock.h for documentation **
 *
 */

#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <plat/dock.h>

#define PFX "RDSDOCK windscr_rds"
#ifdef DOCK_DEBUG
#define DBG(fmt, args...) printk(KERN_DEBUG PFX fmt, ## args)
#else
#define DBG(fmt, args...)
#endif
#define DBG_PRINT_FUNC() DBG(" Function %s\n", __func__);

static int dock_windscr_rds_connected = 0;
struct i2c_client *rdstmc_client;
struct i2c_board_info rdstmc_info = {
	I2C_BOARD_INFO("si4703", 0x10),
};

static int dock_windscr_rds_reset_devices(void *fn_data);

/* Specific initialization of windscreen dock with RDS TMC receiver */
static int dock_windscr_rds_connect(void *fn_data)
{
	int index;
	struct i2c_adapter* adapter;
	struct dock_state *dsp = fn_data;
	struct dock_platform_data *dpp = dsp->pd;
	struct dock_type *dtp = dsp->current_dockp;
	
	DBG("Connecting windscreen with rds dock\n");

	if (dock_windscr_rds_connected) {
		printk(KERN_WARNING PFX
			"Windscreen with rds dock is already connected\n");
		return -1;
	}
	
	rdstmc_info.irq = dpp->dock_line_get_irq(dpp, FM_INT); /* Get I2C irq on dock */
	DBG("I2C IRQ is %d\n", rdstmc_info.irq);
	
	index = dock_get_i2c_adapter_index();
	printk("adapter index is %d\n", index);
	adapter = i2c_get_adapter(index);
	if (!adapter) {
		printk(KERN_WARNING PFX "no dock i2c adapter on index %d\n", index);
		return -1;
	}

	/* Reset the device */
	dock_windscr_rds_reset_devices(dsp);

	/* Register device kobjects corresponding to devices on dock */
	rdstmc_info.platform_data	= dtp;
	rdstmc_client = i2c_new_device(adapter, &rdstmc_info);
	if (rdstmc_client == NULL) {
		printk(KERN_WARNING PFX "i2c_new_device() %s failed\n", 
			rdstmc_info.type);
		return -1;
	} else {
		DBG("i2c_new_device() %s success\n", rdstmc_info.type);
		// check functionality of adapter
		if (i2c_check_functionality(adapter, I2C_FUNC_I2C) == 0) {
			printk(KERN_WARNING PFX "i2c_new_device() %s: soc i2c adapter not" \
				"supporting required functionality\n", rdstmc_info.type);
			i2c_unregister_device(rdstmc_client);
			return -1;
		}
		// client is already attached, and device is already registered
	}

	dock_windscr_rds_connected = 1;
	return 0;
}

/* Specific deinitialization of windscreen dock with RDS TMC receiver */
static int dock_windscr_rds_disconnect(void *fn_data)
{
	printk(" Disconnecting windscreen with rds dock\n");
	
	if (!dock_windscr_rds_connected) {
		printk(KERN_WARNING PFX
			" Windscreen with rds dock was never connected\n");
		return -1;
	}

	i2c_unregister_device(rdstmc_client);
	dock_windscr_rds_connected = 0;
	return 0;
}

/* Reset the FM receiver on the dock. */
static int dock_windscr_rds_reset_devices(void *fn_data)
{
	struct dock_state *dsp = fn_data;
	struct dock_platform_data *pd = dsp->pd;

	printk(KERN_INFO "dsp=%p\n", dsp);
	printk(KERN_INFO "pd->line_set=%p\n", pd->dock_line_set);

	/* Drive SDIO low, as required by the si4703 */
	pd->dock_line_set(pd, DOCK_I2C_PWR, DOCK_LINE_DEASSERT);

	/* Assert reset (inverted) */
	pd->dock_line_set(pd, RDS_nRST, pd->reset_inverted ? DOCK_LINE_DEASSERT : DOCK_LINE_ASSERT);
	mdelay(100);

	/* Enable power */
	pd->dock_line_set(pd, DC_OUT, DOCK_LINE_ASSERT);
	mdelay(100); /* Let external signal stabilize */

	/* Stop resetting the board */
	pd->dock_line_set(pd, RDS_nRST, pd->reset_inverted ? DOCK_LINE_ASSERT : DOCK_LINE_DEASSERT);
	mdelay(100);

	/* Let SDIO2 float again */
	pd->dock_line_set(pd, DOCK_I2C_PWR, DOCK_LINE_ASSERT);
	mdelay(100);

	return 0;
}

static int dock_windscr_rds_probe_id(void *fn_data)
{
	/* Priority 1, if another dock type with the same detid is more appropriate, its lookup function will return a higher priority */
	return 1;
}

struct dock_type dock_type_windscr = {
	.dock_name	= "DOCK_WINDSCR_RDS",
	.detection_id	= DETID_NOPWR(1,0) | DETID_PWR(1,0),
	.connect	= dock_windscr_rds_connect,
	.disconnect	= dock_windscr_rds_disconnect,
	.reset_devices	= NULL,
	.probe_id	= dock_windscr_rds_probe_id,
};

int __init dock_windscr_rds_init(void)
{
	dock_type_register(&dock_type_windscr);
	return 0;
}
