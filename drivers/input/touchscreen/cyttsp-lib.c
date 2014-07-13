/* drivers/input/touchscreen/cyttsp-lib.c
 *
 * Library functions for Cypress TTSP IC
 *
 * Copyright (C) 2011 TomTom BV <http://www.tomtom.com/>
 * Authors: Niels Langendorff <niels.langendorff@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/i2c.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include <linux/err.h>
#include <linux/cyttsp.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/input.h>
#include <linux/timer.h>
#include <linux/jiffies.h>

static int loglevel = LOG_ERROR;
static int prev_loglevel = LOG_ERROR;

void cyttsp_printk(int level, const char *fmt, ...)
{
	va_list args;
	char buf[256];

	switch (level) {
		case LOG_DISABLED:
			/* Logging disabled so return */
			return;
			break;
		case LOG_ALL:
			/* Only log when loglevel is set */
			if (loglevel == LOG_DISABLED)
				return;
			break;
		case LOG_ERROR:
		case LOG_DEBUG:
		default:
			/* Only log when equal to loglevel */
			if (level != loglevel)
				return;
			break;
		case LOG_TUNING:
			/* Only log when loglevel is LOG_TUNING or LOG_DEBUG */
			if ((loglevel != LOG_TUNING) && (loglevel != LOG_DEBUG))
				return;
			break;
	}

	va_start(args, fmt);
	vsnprintf(buf, 256, fmt, args);
	va_end(args);

	printk("%s", buf);
}

/* Stupid way of changing the loglevel */
void cyttsp_set_loglevel(int new_loglevel)
{
	prev_loglevel = loglevel;
	loglevel = new_loglevel;
}

/* Stupid way of changing the loglevel */
void cyttsp_restore_loglevel(void)
{
	loglevel = prev_loglevel;
}

void cyttsp_dump_rawdata(char * msg, char * ptr, int len)
{
	int i;
	char header_tuning[] = {0x55, 0xAA};
	char footer_tuning[] = {0xFF, 0xFF};

	CYTTSP_TUN("\n");
	CYTTSP_TUN(CYTTSP_DEVNAME ":%s, raw data received:\n", msg);
	CYTTSP_TUN("%.2x %.2x %.2x ", header_tuning[0], header_tuning[1], 
			len);

	for (i = 0; i < len; i++) {
		CYTTSP_TUN("%.2x ", ptr[i]);
	}

	CYTTSP_TUN("%.2x %.2x\n", footer_tuning[0], footer_tuning[1]);
	CYTTSP_TUN("\n");
}

int cyttsp_i2c_recv(struct i2c_client * client, char *buf, int count)
{
	int ret;

	BUG_ON(!buf);
	BUG_ON(!client);

	ret = i2c_master_recv(client, buf, count);
	if (ret != count)
		CYTTSP_ALL(KERN_ERR CYTTSP_DEVNAME ": Error reading bootloader registers, return value of i2c read: [%d] instead of [%d]\n", ret, count);

	return ret;
}

int cyttsp_i2c_send(struct i2c_client * client, char *buf, int count)
{
	int ret;

	BUG_ON(!buf);
	BUG_ON(!client);

	ret = i2c_master_send(client, buf, count);
	if (ret != count)
		CYTTSP_ALL(KERN_ERR CYTTSP_DEVNAME ": Error writing bootloader registers, return value of i2c send: [%d] instead of [%d]\n", ret,count);

	return ret;
}

static int cyttsp_read_revj_systeminfo(struct cyttsp_drv_data *drv_data)
{
	struct sysinfo_mode *si_mode = &drv_data->si_mode.revj;
	int ret;

	CYTTSP_DBG("Enter in get sys info\n");

	ret = cyttsp_i2c_recv(drv_data->client, (char *)si_mode, SIZE_SYSINFO_MODE);

	if (ret != SIZE_SYSINFO_MODE) {
		CYTTSP_ALL(KERN_ERR CYTTSP_DEVNAME ": Error reading system \
				information, return value of i2c read: [%d] instead of [%d]\n", 
				ret, SIZE_SYSINFO_MODE);
		return 1;
	}

	cyttsp_dump_rawdata("Get system info", (char *)si_mode, SIZE_SYSINFO_MODE);

	si_mode->bl_ver       = be16_to_cpu(si_mode->bl_ver);
	si_mode->tts_ver      = be16_to_cpu(si_mode->tts_ver);
	si_mode->app_id       = be16_to_cpu(si_mode->app_id);
	si_mode->app_ver      = be16_to_cpu(si_mode->app_ver);

	CYTTSP_DBG(CYTTSP_DEVNAME ":i2c packet system info received:\n");
	CYTTSP_DBG("HST_MODE: 0x%x\n", si_mode->hst_mode);
	CYTTSP_DBG("MFG_STAT: 0x%x\n", si_mode->mfg_stat);
	CYTTSP_DBG("MFG_CMD: 0x%x\n", si_mode->mfg_cmd);
	CYTTSP_DBG("CID_0: 0x%x\n", si_mode->cid_0);
	CYTTSP_DBG("CID_1: 0x%x\n", si_mode->cid_1);
	CYTTSP_DBG("CID_2: 0x%x\n", si_mode->cid_2);

	CYTTSP_DBG("SILICON_ID0: 0x%x\n", si_mode->uid_0);
	CYTTSP_DBG("SILICON_ID1: 0x%x\n", si_mode->uid_1);
	CYTTSP_DBG("SILICON_ID2: 0x%x\n", si_mode->uid_2);
	CYTTSP_DBG("SILICON_ID3: 0x%x\n", si_mode->uid_3);
	CYTTSP_DBG("SILICON_ID4: 0x%x\n", si_mode->uid_4);
	CYTTSP_DBG("SILICON_ID5: 0x%x\n", si_mode->uid_5);
	CYTTSP_DBG("SILICON_ID6: 0x%x\n", si_mode->uid_6);
	CYTTSP_DBG("SILICON_ID7: 0x%x\n", si_mode->uid_7);

	CYTTSP_DBG("BOOTLOADER_VERSION: 0x%x\n", si_mode->bl_ver);
	CYTTSP_DBG("TRUETOUCH_STANDARD_PRODUCT_VERSION: 0x%x\n", si_mode->tts_ver);
	CYTTSP_DBG("APPLICATION_ID: 0x%x\n", si_mode->app_id);
	CYTTSP_DBG("APPLICATION_VERSION: 0x%x\n", si_mode->app_ver);
	
	CYTTSP_DBG("\n");

	printk (CYTTSP_DEVNAME ":Cypress Capacitive Touch Screen System Information:\n");
	printk ("Silicon ID0                         : 0x%x\n", si_mode->uid_0);
	printk ("Silicon ID1                         : 0x%x\n", si_mode->uid_1);
	printk ("Bootloader Version                  : 0x%x\n", si_mode->bl_ver);
	printk ("TrueTouch Standard Product Version  : 0x%x\n", si_mode->tts_ver);
	printk ("Application ID                      : 0x%x\n", si_mode->app_id);
	printk ("Application Version                 : %x.%.2x\n", (si_mode->app_ver & 0xFF00) >> 8, (si_mode->app_ver & 0x00FF));

	CYTTSP_DBG("Exit from get sys info\n");

	return 0;
}

static int cyttsp_read_revc_systeminfo(struct cyttsp_drv_data *drv_data)
{
	struct sysinfo_mode_revc *si_mode = &drv_data->si_mode.revc;
	int ret;

	CYTTSP_DBG("Enter in get sys info\n");

	ret = cyttsp_i2c_recv(drv_data->client, (char *)si_mode, SIZE_REVC_SYSINFO_MODE);
	if (ret != SIZE_REVC_SYSINFO_MODE) {
		CYTTSP_ALL(KERN_ERR CYTTSP_DEVNAME ": Error reading system \
				information, return value of i2c read: [%d] instead of [%d]\n",
				ret, SIZE_REVC_SYSINFO_MODE);
		return 1;
	}

	cyttsp_dump_rawdata("Get system info", (char *)si_mode, SIZE_REVC_SYSINFO_MODE);

	si_mode->bl_ver       = be16_to_cpu(si_mode->bl_ver);
	si_mode->tts_ver      = be16_to_cpu(si_mode->tts_ver);
	si_mode->app_id       = be16_to_cpu(si_mode->app_id);
	si_mode->app_ver      = be16_to_cpu(si_mode->app_ver);

	CYTTSP_DBG(CYTTSP_DEVNAME ":i2c packet system info received:\n");
	CYTTSP_DBG("HST_MODE: 0x%x\n", si_mode->hst_mode);
	CYTTSP_DBG("BIST_COMM: 0x%x\n", si_mode->bist_comm);
	CYTTSP_DBG("BIST_STAT: 0x%x\n", si_mode->bist_stat);

	CYTTSP_DBG("SILICON_ID0: 0x%x\n", si_mode->uid_0);
	CYTTSP_DBG("SILICON_ID1: 0x%x\n", si_mode->uid_1);
	CYTTSP_DBG("SILICON_ID2: 0x%x\n", si_mode->uid_2);
	CYTTSP_DBG("SILICON_ID3: 0x%x\n", si_mode->uid_3);
	CYTTSP_DBG("SILICON_ID4: 0x%x\n", si_mode->uid_4);
	CYTTSP_DBG("SILICON_ID5: 0x%x\n", si_mode->uid_5);
	CYTTSP_DBG("SILICON_ID6: 0x%x\n", si_mode->uid_6);
	CYTTSP_DBG("SILICON_ID7: 0x%x\n", si_mode->uid_7);

	CYTTSP_DBG("BOOTLOADER_VERSION: 0x%x\n", si_mode->bl_ver);
	CYTTSP_DBG("TRUETOUCH_STANDARD_PRODUCT_VERSION: 0x%x\n", 
			si_mode->tts_ver);
	CYTTSP_DBG("APPLICATION_ID: 0x%x\n", si_mode->app_id);
	CYTTSP_DBG("APPLICATION_VERSION: 0x%x\n", si_mode->app_ver);

	CYTTSP_DBG("RESOLUTION_X: 0x%x\n", 
			((u16)si_mode->cid_0 << 8) | si_mode->cid_1);
	CYTTSP_DBG("RESOLUTION_Y: 0x%x\n", 
			((u16)si_mode->cid_2 << 8) | si_mode->cid_3);
	CYTTSP_DBG("NUMBER_OF_GESTURES: 0x%x\n", si_mode->cid_4);

	printk(CYTTSP_DEVNAME ":Cypress Capacitive Touch Screen System Information:\n");

	printk("Silicon ID0                         : 0x%x\n", si_mode->uid_0);
	printk("Silicon ID1                         : 0x%x\n", si_mode->uid_1);

	printk("Bootloader Version                  : 0x%x\n", si_mode->bl_ver);
	printk("TrueTouch Standard Product Version  : 0x%x\n", si_mode->tts_ver);
	printk("Application ID                      : 0x%x\n", si_mode->app_id);
	printk("Application Version                 : %x.%.2x\n", 
		(si_mode->app_ver & 0xFF00) >> 8, (si_mode->app_ver & 0x00FF));
	printk("Resolution X                        : 0x%x\n", 
		((u16)si_mode->cid_0 << 8) | si_mode->cid_1);
	printk("Resolution Y                        : 0x%x\n", 
		((u16)si_mode->cid_2 << 8) | si_mode->cid_3);
	printk("Number of Gestures Supported        : %d\n", si_mode->cid_4);

	CYTTSP_DBG("Exit from get sys info\n");

	return 0;
}

int cyttsp_read_systeminfo(struct cyttsp_drv_data *drv_data)
{
	int ret = 0;

	switch (drv_data->ttsp_revision) {
		case TTSP_REVISION_C:
			ret = cyttsp_read_revc_systeminfo(drv_data);
			break;
		case TTSP_REVISION_J_OR_HIGHER:
			ret = cyttsp_read_revj_systeminfo(drv_data);
			break;
		case TTSP_REVISION_UNKNOWN:
			BUG_ON(drv_data->ttsp_revision == TTSP_REVISION_UNKNOWN);
		default:
			break;
	}

	return ret;
}

int cyttsp_read_bootloader(struct cyttsp_drv_data * drv_data)
{
	int ret;
	char reg = BL_FILE;
	struct bootloader_mode *bl_mode = &drv_data->bl_mode.revj;

	ret = cyttsp_i2c_send(drv_data->client, &reg, 1);
	if (ret != 1) {
		printk (KERN_ERR CYTTSP_DEVNAME ": Error writing 1 byte to the touch screen\n");
		return -1;
	}

	ret = cyttsp_i2c_recv(drv_data->client, (char *)bl_mode, SIZE_BL_MODE);
	if (ret != SIZE_BL_MODE) {
		printk (KERN_ERR CYTTSP_DEVNAME ": Error reading 3 bytes from the touch screen\n");
		return -1;
	}

	cyttsp_dump_rawdata("Get bootloader", (char *)bl_mode, SIZE_BL_MODE);

	bl_mode->blver     = be16_to_cpu(bl_mode->blver);
	bl_mode->bld_blver = be16_to_cpu(bl_mode->bld_blver);
	bl_mode->ttspver   = be16_to_cpu(bl_mode->ttspver);
	bl_mode->appid     = be16_to_cpu(bl_mode->appid);
	bl_mode->appver    = be16_to_cpu(bl_mode->appver);

	CYTTSP_DBG("Result from getting status:\n");
	CYTTSP_DBG("BL_FILE   %02X\n", bl_mode->bl_file);
	CYTTSP_DBG("BL_STATUS %02X\n", bl_mode->bl_status);
	CYTTSP_DBG("BL_ERROR  %02X\n", bl_mode->bl_error);
	CYTTSP_DBG("BL_VER    %04X\n", bl_mode->blver);

	if (bl_mode->blver > REVC_BLVER) {
		CYTTSP_DBG("BLD_BLVER %04X\n", bl_mode->bld_blver);
		CYTTSP_DBG("TTSP_VER  %04X\n", bl_mode->ttspver);
		CYTTSP_DBG("APPID     %04X\n", bl_mode->appid);
		CYTTSP_DBG("APPVER    %04X\n", bl_mode->appver);
	}

	return 0;
}

int cyttsp_start_ts_app(struct cyttsp_drv_data *drv_data, void *key, int len)
{
	char *buf;
	int size_buf;
	int ret;

	printk(KERN_DEBUG CYTTSP_DEVNAME ": Start the touch screen application\n");
	size_buf = SIZE_PREFIX_BOOTLOADER_CMD + SIZE_BOOTLOADER_CMD + len;
	buf = (char*) kmalloc(size_buf, GFP_KERNEL);

	// Command prefix
	buf[0] = 0x00;
	buf[1] = 0x00;

	// Bootloader command
	buf[2] = CHECKSUM_SEED_CMD;
	buf[3] = LAUNCH_APPLICATION_CMD;

	// Bootloader key
	memcpy((void*)(buf + 4), key, len);

	ret = cyttsp_i2c_send(drv_data->client, buf, size_buf);
	if (ret != size_buf) {
		kfree(buf);
		return -1;
	}

	/* Allow the HW to launch the touchscreen application */
	msleep(DELAY_LAUNCH_APPLICATION);

	drv_data->current_mode = OPERATING_MODE;

	kfree(buf);
	return 0;
}

TTSP_REVISION cyttsp_get_ttsp_revision(struct cyttsp_drv_data *drv_data)
{
	struct bootloader_mode *bl_mode = &drv_data->bl_mode.revj;

	TTSP_REVISION revision = TTSP_REVISION_UNKNOWN; 

	switch (bl_mode->blver) {
		case REVC_BLVER:
			revision = TTSP_REVISION_C;
			break;
		default:
			revision = TTSP_REVISION_J_OR_HIGHER;
			break;
	}
	return revision;
}

int cyttsp_set_offset_register(struct cyttsp_drv_data *drv_data, char offset)
{
	int ret;

	ret = cyttsp_i2c_send(drv_data->client, &offset, 1);
	if (ret != 1) {
		CYTTSP_ALL(KERN_ERR CYTTSP_DEVNAME ": Error setting offset register, return value of i2c send: [%d] instead of 1\n", ret);
		return ERROR_I2C_ACCESS;
	}

	CYTTSP_DBG(CYTTSP_DEVNAME ":set offset register to: 0x%x\n",offset);

	return 0;
}

int cyttsp_read_register(struct cyttsp_drv_data *drv_data, char reg, char* value)
{
	int ret;

	ret = cyttsp_i2c_send(drv_data->client, &reg, 1);
	if (ret != 1) {
		CYTTSP_DBG(KERN_ERR CYTTSP_DEVNAME ": Error setting offset register, return value of i2c send: [%d] instead of 1\n", ret);
		return ERROR_I2C_ACCESS;
	}

	udelay(DELAY_BETWEEN_WRITES);

	ret = cyttsp_i2c_recv(drv_data->client, value, 1);
	if (ret != 1) {
		CYTTSP_ALL(KERN_ERR CYTTSP_DEVNAME ": Error reading register value, return value of i2c read: [%d] instead of 1\n", ret);
		return ERROR_I2C_ACCESS;
	}

	return 0;
}

int cyttsp_read_data(struct cyttsp_drv_data *drv_data, char reg, char* buf, int len)
{
	int ret;

	ret = cyttsp_i2c_send(drv_data->client, &reg, 1);
	if (ret != 1) {
		CYTTSP_DBG(KERN_ERR CYTTSP_DEVNAME ": Error setting offset register, return value of i2c send: [%d] instead of 1\n", ret);
		return ERROR_I2C_ACCESS;
	}

	udelay(DELAY_BETWEEN_WRITES);

	ret = cyttsp_i2c_recv(drv_data->client, buf, len);
	if (ret != len) {
		CYTTSP_ALL(KERN_ERR CYTTSP_DEVNAME ": Error reading register value, return value of i2c read: [%d] instead of %d\n", ret, len);
		return ERROR_I2C_ACCESS;
	}

	return 0;
}

int cyttsp_write_register(struct cyttsp_drv_data *drv_data, char reg, char value)
{
	int ret;
	char buf[2];

	buf[0] = reg;
	buf[1] = value;

	ret = cyttsp_i2c_send(drv_data->client, buf, 2);

	if (ret != 2) {
		CYTTSP_ALL(KERN_ERR CYTTSP_DEVNAME ": Error setting register value, return value of i2c send: [%d] instead of 2\n", ret);
		return ERROR_I2C_ACCESS;
	}

	CYTTSP_DBG(CYTTSP_DEVNAME ":register offset = 0x%X value set: 0x%X\n", reg, value);

	return 0;
}

int cyttsp_switch_to_other_mode(struct cyttsp_drv_data *drv_data, char device_mode)
{
	char val;

	CYTTSP_DBG("Enter in switch to other mode, target mode=%d\n", device_mode);

	switch (device_mode) {
		case OPERATING_MODE:
		case SYS_INFO_MODE:
		case RAW_COUNTS_TEST_MODE:
		case DIFFERENCE_COUNTS_TEST_MODE:
		case IDAC_SETTINGS_TEST_MODE:
		case BASELINE_TEST_MODE:
			break;
		default:
			return ERROR_UNKNOWN_MODE;
			break;
	}

	// Read value of HOST MODE register
	if (cyttsp_read_register(drv_data, HST_MODE, &val))
		return ERROR_I2C_ACCESS;

	CYTTSP_DBG("Switch mode from 0x%02X to 0x%02X\n", val, 
			(val & ~HST_MODE_DEVICE_MODE) | (device_mode << HST_MODE_DEVICE_MODE_SHIFT));
	val = (val & ~HST_MODE_DEVICE_MODE) | (device_mode << HST_MODE_DEVICE_MODE_SHIFT);

	// Write value of HOST MODE register
	if (cyttsp_write_register(drv_data, HST_MODE, val))
		return ERROR_I2C_ACCESS;

	drv_data->current_mode = (int)device_mode;

	CYTTSP_DBG("Exit switch to other mode\n");

	return 0;
}

void cyttsp_recalibrate(struct cyttsp_drv_data *drv_data)
{
	char cmd_sysinfo_mode[] = { 0x00, 0x10 };
	char cmd_test_mode[] = { 0x00, 0x10, 0x30 };
	int ret;

	cyttsp_set_loglevel(LOG_DEBUG);

	// Switch to sys info mode
	ret = cyttsp_i2c_send(drv_data->client, cmd_sysinfo_mode, 2);
	if (ret != 2)
		goto cmd_error;

	msleep(DELAY_SWITCH_MODES);

	// Run mfg test command
	ret = cyttsp_i2c_send(drv_data->client, cmd_test_mode, 3);
	if (ret != 3)
		goto cmd_error;

	msleep(DELAY_RECALIBRATION);

cmd_error:
	cyttsp_restore_loglevel();
	return;
}

