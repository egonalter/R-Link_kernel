/* drivers/input/touchscreen/cyttsp.c
 *
 * Control driver for Cypress TTSP IC
 *
 * Copyright (C) 2006 TomTom BV <http://www.tomtom.com/>
 * Authors: Vincent Dejouy <vincent.dejouy@tomtom.com>
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

#ifdef CONFIG_TOMTOM_FDT
#include <plat/fdt.h>
#endif

/* TODO: Check if we really need/want this stuff, the constants collide with
 * others in include/linux/input.h. if we really need/want this, the constants
 * should probably moved to this file */
#define ABS_FINGERS             0x29
#define ABS_FINGER2_X           0x2a
#define ABS_FINGER2_Y           0x2b
#define ABS_GESTURE             0x2c

static struct workqueue_struct	*readinfo_workqueue;
static struct workqueue_struct	*watchdog_workqueue;
static struct work_struct		read_info_work;
static struct delayed_work		watchdog_work;

static struct cyttsp_pdata pdata;
static struct input_dev *cyttsp_cap_ts_input_dev;     /* Representation of an input device */
static struct cyttsp_drv_data *drv_data;
static char* recalibrate = "false";

static u16 bl_ver = -1;
static u16 tts_ver = -1;
static u16 app_id = -1;
static u16 app_ver = -1;

static int cyttsp_restart_ts(void)
{
	pdata.reset_ts();
	msleep(DELAY_BETWEEN_WRITES);
	return (cyttsp_start_ts_app(drv_data, pdata.key, SIZE_BOOTLOADER_KEY));
}

static int cyttsp_get_finger_position(void)
{
	int ret;
	int touches;
	struct operation_mode op_mode;

	ret = cyttsp_i2c_recv(drv_data->client, (char *)&op_mode, SIZE_OPERATION_MODE);

	if (ret != SIZE_OPERATION_MODE) {
		CYTTSP_ALL(KERN_ERR CYTTSP_DEVNAME ": Error reading finger positions, return value of i2c read: [%d] instead of [%d]\n", ret, SIZE_OPERATION_MODE);

		return ERROR_I2C_ACCESS;
	}

	cyttsp_dump_rawdata("Get finger position", (char *)&op_mode, 
		SIZE_OPERATION_MODE);

	op_mode.touch1_x = be16_to_cpu(op_mode.touch1_x);
	op_mode.touch1_y = be16_to_cpu(op_mode.touch1_y);
	op_mode.touch2_x = be16_to_cpu(op_mode.touch2_x);
	op_mode.touch2_y = be16_to_cpu(op_mode.touch2_y);

	/* Check for invalid buffer */
	if (op_mode.tt_mode & TT_MODE_BUFFER_INVALID) {
		CYTTSP_DBG(CYTTSP_DEVNAME ":Warning invalid buffer received\n\n");

		return 0;
	}

	touches = op_mode.tt_stat & TT_STAT_NUMBER_OF_TOUCHES;

	CYTTSP_DBG(CYTTSP_DEVNAME ":i2c packet received after finger positions reversed:\n");
	CYTTSP_DBG("HST_MODE: 0x%x\n", op_mode.hst_mode);
	CYTTSP_DBG("TT_MODE : 0x%x\n", op_mode.tt_mode);
	CYTTSP_DBG("TT_STAT : 0x%x\n", op_mode.tt_stat);

	CYTTSP_DBG("\nFinger 1:\n");
	CYTTSP_DBG("X: 0x%x\n", op_mode.touch1_x);
	CYTTSP_DBG("Y: 0x%x\n", op_mode.touch1_y);
	CYTTSP_DBG("Z: 0x%x\n", op_mode.touch1_z);

	CYTTSP_DBG("\nFinger 2:\n");
	CYTTSP_DBG("X: 0x%x\n", op_mode.touch2_x);
	CYTTSP_DBG("Y: 0x%x\n", op_mode.touch2_y);
	CYTTSP_DBG("Z: 0x%x\n", op_mode.touch2_z);

	CYTTSP_DBG("\nFingers      : %d\n", touches);
	CYTTSP_DBG("Gesture Count  : %d\n", op_mode.gest_cnt & GEST_CNT_GESTURE_COUNT);
	CYTTSP_DBG("Gesture ID     : 0x%x\n", op_mode.gest_id);
	CYTTSP_DBG("\n");

	// Sometimes the TS reports more than max number of touches, they should not be reported to the input layer
	// Because in the datasheet it is mentioned that number of touches greater than max touches are "reserved"
	// Most probably debug purposes for Cypress
	if (touches <= MAXNUMTOUCHES) {
		// Report position of fingers to the input layer
		input_report_abs(cyttsp_cap_ts_input_dev, ABS_X, op_mode.touch1_x);
		input_report_abs(cyttsp_cap_ts_input_dev, ABS_Y, op_mode.touch1_y);
		input_report_abs(cyttsp_cap_ts_input_dev, ABS_FINGER2_X, op_mode.touch2_x);
		input_report_abs(cyttsp_cap_ts_input_dev, ABS_FINGER2_Y, op_mode.touch2_y);
		input_report_abs(cyttsp_cap_ts_input_dev, ABS_GESTURE, op_mode.gest_id);
		input_report_abs(cyttsp_cap_ts_input_dev, ABS_FINGERS, touches);

		/* Report a touch or release based on the number of touches */
		if (touches) {
			input_report_abs(cyttsp_cap_ts_input_dev, ABS_PRESSURE, 1);
			input_report_key(cyttsp_cap_ts_input_dev, BTN_TOUCH, 1);
		} else {
			input_report_abs(cyttsp_cap_ts_input_dev, ABS_PRESSURE, 0);
			input_report_key(cyttsp_cap_ts_input_dev, BTN_TOUCH, 0);
		}

		input_sync(cyttsp_cap_ts_input_dev);
	}

	return 0;
}

static int cyttsp_toggle_data_read_bit(void)
{
	char val;

	CYTTSP_DBG("Enter in toggle data read bit\n");

	// Read value of HOST MODE register
	if (cyttsp_read_register(drv_data, HST_MODE, &val))
		return ERROR_I2C_ACCESS;

	CYTTSP_DBG("Switch toggle bit from 0x%02X to ", val); 

	/* Toggle the bit for handshaking */
	if (val & HST_MODE_DATA_READ_TOGGLE)
		val &= ~(HST_MODE_DATA_READ_TOGGLE);
	else
		val |= HST_MODE_DATA_READ_TOGGLE;

	CYTTSP_DBG("0x%02X\n", val); 

	// Write value of HOST MODE register
	if (cyttsp_write_register(drv_data, HST_MODE, val))
		return ERROR_I2C_ACCESS;

	CYTTSP_DBG("Exit from toggle data read bit\n");

	return 0;
}

static int cyttsp_read_mode(void)
{
	char val;

	CYTTSP_DBG("Enter in read mode\n");

	// Read value of HOST MODE register
	if (cyttsp_read_register(drv_data, TT_MODE, &val))
		return ERROR_I2C_ACCESS;

	CYTTSP_DBG("Read mode, tt mode register value: 0x%x\n", val);
	CYTTSP_DBG("Exit from read mode\n");

	if ((val & TT_MODE_BOOTLOADER_FLAG) != 0)
		return ERROR_TS_REBOOTED;

	return 0;
}

static int cyttsp_get_raw_counts(void)
{
	int ret;
	struct test_mode te_mode;

	ret = cyttsp_i2c_recv(drv_data->client, (char *)&te_mode, SIZE_TEST_MODE);

	if (ret != SIZE_TEST_MODE) {
		CYTTSP_ALL(KERN_ERR CYTTSP_DEVNAME ": Error reading raw counts, return value of i2c read: [%d] instead of [%d]\n", ret, SIZE_TEST_MODE);

		return ERROR_I2C_ACCESS;
	}

	cyttsp_dump_rawdata("Get test data", (char *)&te_mode, 
		SIZE_TEST_MODE);

	return 0;
}

static ssize_t cyttsp_bl_ver_show(struct device *dev,
					struct device_attribute *attr, 
					char *buf)
{
	ssize_t temp;

	temp = sprintf(buf, "%04X\n", bl_ver);

	return temp;
}

static ssize_t cyttsp_tts_ver_show(struct device *dev,
					struct device_attribute *attr, 
					char *buf)
{
	ssize_t temp;

	temp = sprintf(buf, "%04X\n", tts_ver);

	return temp;
}

static ssize_t cyttsp_app_id_show(struct device *dev,
					struct device_attribute *attr, 
					char *buf)
{
	ssize_t temp;

	temp = sprintf(buf, "%04X\n", app_id);

	return temp;
}

static ssize_t cyttsp_app_ver_show(struct device *dev,
					struct device_attribute *attr, 
					char *buf)
{
	ssize_t temp;

	temp = sprintf(buf, "%x.%.2x\n", (app_ver & 0xFF00) >> 8, app_ver & 0x00FF);

	return temp;
}

static ssize_t cyttsp_mode_show(struct device *dev, 
					struct device_attribute *attr, 
					char *buf)
{
	ssize_t temp;

	mutex_lock(&drv_data->lock);
	temp = sprintf(buf, "%d\n", drv_data->current_mode);
	mutex_unlock(&drv_data->lock);

	return temp;
}

static ssize_t cyttsp_mode_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	long i;
	ssize_t ret = count;

	mutex_lock(&drv_data->lock);

	if (strict_strtoul(buf, 10, &i)) {
		ret = -EINVAL;
		goto error_strtoul;
	}

	if (cyttsp_switch_to_other_mode(drv_data, (int)i) != 0)
		ret = -EINVAL;

error_strtoul:
	mutex_unlock(&drv_data->lock);

	return count;
}

static DEVICE_ATTR(mode, S_IRUGO | S_IWUGO, cyttsp_mode_show, cyttsp_mode_store);
static DEVICE_ATTR(bl_ver, S_IRUGO, cyttsp_bl_ver_show, NULL);
static DEVICE_ATTR(tts_ver, S_IRUGO, cyttsp_tts_ver_show, NULL);
static DEVICE_ATTR(app_id, S_IRUGO, cyttsp_app_id_show, NULL);
static DEVICE_ATTR(app_ver, S_IRUGO, cyttsp_app_ver_show, NULL);

static struct attribute *cyttsp_attributes[] = {
	&dev_attr_mode.attr,
	&dev_attr_bl_ver.attr,
	&dev_attr_tts_ver.attr,
	&dev_attr_app_id.attr,
	&dev_attr_app_ver.attr,
	NULL,
};

static struct attribute_group cyttsp_attr_group = {
	.attrs = cyttsp_attributes,
};


static void cyttsp_watchdog(struct work_struct *work)
{
	int err;

	mutex_lock(&drv_data->lock);

	err = cyttsp_read_mode();

	if (err == ERROR_TS_REBOOTED )
		cyttsp_start_ts_app(drv_data, pdata.key, SIZE_BOOTLOADER_KEY);
	else if (err == ERROR_I2C_ACCESS)
		cyttsp_restart_ts();

	mutex_unlock(&drv_data->lock);

	queue_delayed_work(watchdog_workqueue, &watchdog_work, msecs_to_jiffies(5000));
}

static void cyttsp_read_info_work(struct work_struct *work)
{
	int ret;

	mutex_lock(&drv_data->lock);

	switch (drv_data->current_mode) {
		case OPERATING_MODE:
			if (cyttsp_set_offset_register(drv_data, HST_MODE) == ERROR_I2C_ACCESS)
				cyttsp_restart_ts();

			udelay(DELAY_BETWEEN_WRITES);

			// Replace loglevel level by loglevel tuning for final tuning of ttsp
			if ( cyttsp_get_finger_position() == ERROR_I2C_ACCESS)
				cyttsp_restart_ts();
			break;

		case SYS_INFO_MODE:
			if (cyttsp_set_offset_register(drv_data, HST_MODE) == ERROR_I2C_ACCESS)
				cyttsp_restart_ts();

			udelay(DELAY_BETWEEN_WRITES);

			if (cyttsp_read_systeminfo(drv_data) == ERROR_I2C_ACCESS)
				cyttsp_restart_ts();

			if (cyttsp_toggle_data_read_bit() == ERROR_I2C_ACCESS)
				cyttsp_restart_ts();

			if (cyttsp_switch_to_other_mode(drv_data, OPERATING_MODE) == ERROR_I2C_ACCESS)
				cyttsp_restart_ts();
			break;

		case RAW_COUNTS_TEST_MODE:
		case DIFFERENCE_COUNTS_TEST_MODE:
		case BASELINE_TEST_MODE:
		case IDAC_SETTINGS_TEST_MODE:
			if (cyttsp_set_offset_register(drv_data, HST_MODE) == ERROR_I2C_ACCESS)
				cyttsp_restart_ts();

			udelay(DELAY_BETWEEN_WRITES);

			cyttsp_set_loglevel(LOG_TUNING);
			ret = cyttsp_get_raw_counts();
			cyttsp_restore_loglevel();
			if (ret == ERROR_I2C_ACCESS)
				cyttsp_restart_ts();

			if (cyttsp_toggle_data_read_bit() == ERROR_I2C_ACCESS)
				cyttsp_restart_ts();
			break;

		default:
			break;
	}

	mutex_unlock(&drv_data->lock);
}

static irqreturn_t cyttsp_irq_handler(int irq, void *dev_id)
{
	queue_work(readinfo_workqueue, &read_info_work);
	return IRQ_HANDLED;
};

static void cyttsp_get_fw_versions(void)
{
	/* Reset the touchscreen to enter bootloader mode */
	pdata.reset_ts();
	msleep(DELAY_AFTER_RESET);

	/* Read bootloader information to retrieve TTSP revision */
	cyttsp_read_bootloader(drv_data);

	/* Start touchscreen operation and enter operation mode */
	cyttsp_start_ts_app(drv_data, pdata.key, SIZE_BOOTLOADER_KEY);

	drv_data->ttsp_revision = cyttsp_get_ttsp_revision(drv_data);
	BUG_ON(drv_data->ttsp_revision == TTSP_REVISION_UNKNOWN);

	/* Switch to sysinfo mode */
	cyttsp_switch_to_other_mode(drv_data, SYS_INFO_MODE);
	msleep(DELAY_BETWEEN_WRITES);
	cyttsp_set_offset_register(drv_data, HST_MODE);
	msleep(DELAY_BETWEEN_WRITES);

	/* Read system information */
	cyttsp_read_systeminfo(drv_data);

	/* Switch back to operation mode */
	cyttsp_switch_to_other_mode(drv_data, OPERATING_MODE);
	msleep(DELAY_BETWEEN_WRITES);

	switch (drv_data->ttsp_revision) {
		case TTSP_REVISION_C:
			bl_ver = drv_data->si_mode.revc.bl_ver;
			tts_ver = drv_data->si_mode.revc.tts_ver;
			app_id = drv_data->si_mode.revc.app_id;
			app_ver = drv_data->si_mode.revc.app_ver;
			break;
		case TTSP_REVISION_J_OR_HIGHER:
			bl_ver = drv_data->bl_mode.revj.blver;
			tts_ver = drv_data->bl_mode.revj.ttspver;
			app_id = drv_data->bl_mode.revj.appid;
			app_ver = drv_data->bl_mode.revj.appver;
			break;
		case TTSP_REVISION_UNKNOWN:
		default:
			break;
	}
}

static int cyttsp_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret;
	int err;
	int xmin, xmax, ymin, ymax;

	printk(KERN_DEBUG CYTTSP_DEVNAME " :probing driver\n");
	drv_data = NULL;

	printk(KERN_INFO CYTTSP_DEVNAME " :Cypress TTSP Driver, (C) 2008 TomTom BV\n" );

	BUG_ON(!client);
	BUG_ON(!client->dev.platform_data);

	drv_data = kmalloc(sizeof(struct cyttsp_drv_data), GFP_KERNEL);
	if (NULL == drv_data) {
		printk(KERN_ERR CYTTSP_DEVNAME ": Cannot allocate driver data!\n");
		return -ENOMEM;
	}
	drv_data->client = client;
	drv_data->ttsp_revision = TTSP_REVISION_UNKNOWN;
	drv_data->current_mode = OPERATING_MODE;

	memcpy((void*) &pdata, (void*) client->dev.platform_data, sizeof(struct cyttsp_pdata));
	INIT_WORK(&read_info_work, cyttsp_read_info_work);
	INIT_DELAYED_WORK(&watchdog_work, cyttsp_watchdog);

	readinfo_workqueue = create_singlethread_workqueue("cyttsp_read_info");
	if(!readinfo_workqueue) {
		printk(KERN_ERR CYTTSP_DEVNAME ": Cannot allocate workqueue!\n");
		ret = -ENOMEM;
		goto error_create_readinfo_workqueue;
	}

	watchdog_workqueue = create_singlethread_workqueue("cyttsp_watchdog");
	if(!watchdog_workqueue) {
		printk(KERN_ERR CYTTSP_DEVNAME ": Cannot allocate workqueue!\n");
		ret = -ENOMEM;
		goto error_create_watchdog_workqueue;
	}

	dev_set_drvdata(&client->dev, drv_data);
	mutex_init(&drv_data->lock);

	cyttsp_get_fw_versions();

	cyttsp_cap_ts_input_dev = input_allocate_device();
	if (!cyttsp_cap_ts_input_dev) {
		printk(KERN_ERR CYTTSP_DEVNAME ": Badf init_alloc_device()\n");
		ret = -ENOMEM;
		goto error_alloc_input_device;
	}

	printk(KERN_DEBUG CYTTSP_DEVNAME ": Input device allocated\n");

	/* Announce that the ts will generate absolute coordinates */
	set_bit(EV_ABS, cyttsp_cap_ts_input_dev->evbit);
	
	/* Get XMIN/MAX and YMIN/MAX from fdt */
	xmin = (int)fdt_get_ulong ("/options/touchscreen", "xmin", XMIN);
	xmax = (int)fdt_get_ulong ("/options/touchscreen", "xmax", XMAX);
	ymin = (int)fdt_get_ulong ("/options/touchscreen", "ymin", YMIN);
	ymax = (int)fdt_get_ulong ("/options/touchscreen", "ymax", YMAX);

	// Declare max and min of X, Y and pressure
	input_set_abs_params(cyttsp_cap_ts_input_dev, ABS_X, xmin, pdata.xmax, 0, 0);
	input_set_abs_params(cyttsp_cap_ts_input_dev, ABS_Y, ymin, ymax, 0, 0);
	input_set_abs_params(cyttsp_cap_ts_input_dev, ABS_PRESSURE, PRESSUREMIN, PRESSUREMAX, 0, 0);

	input_set_abs_params(cyttsp_cap_ts_input_dev, ABS_FINGERS, FINGERSMIN, FINGERSMAX, 0, 0);
	input_set_abs_params(cyttsp_cap_ts_input_dev, ABS_FINGER2_X, xmin, pdata.xmax, 0, 0);
	input_set_abs_params(cyttsp_cap_ts_input_dev, ABS_FINGER2_Y, ymin, ymax, 0, 0);
	input_set_abs_params(cyttsp_cap_ts_input_dev, ABS_GESTURE, GESTUREMIN, GESTUREMAX, 0, 0);

	set_bit(EV_KEY, cyttsp_cap_ts_input_dev->evbit);
	set_bit(BTN_TOUCH, cyttsp_cap_ts_input_dev->keybit);

	cyttsp_cap_ts_input_dev->name = "PSOC Cypress Capacitive TouchScreen";
	cyttsp_cap_ts_input_dev->phys = "input(ts)";
	cyttsp_cap_ts_input_dev->id.vendor = 0xDEAD;
	cyttsp_cap_ts_input_dev->id.product = 0xBEEF;
	cyttsp_cap_ts_input_dev->id.version = 0x0101;

	/* Register with the input subsystem */
	ret = input_register_device(cyttsp_cap_ts_input_dev);
	if (ret) {
		printk(KERN_ERR CYTTSP_DEVNAME ": Could not register input device(capacitive touchscreen)!\n");
		ret = -EIO;
		goto error_input_register_device;
	}

	printk(KERN_DEBUG CYTTSP_DEVNAME ": Input device registered\n");

	if (cyttsp_restart_ts()) {
		printk(KERN_ERR CYTTSP_DEVNAME ": I2C Communication Impossible with the Bootloader\n" );
		printk(KERN_ERR CYTTSP_DEVNAME ": Most Probably Wrong Factory Data\n" );
		printk(KERN_ERR CYTTSP_DEVNAME ": Check the Field /features/ts\n" );
		printk(KERN_ERR CYTTSP_DEVNAME ": Change the Value 300E into 300E_FW_V2\n" );
		printk(KERN_ERR CYTTSP_DEVNAME ": Or the Other Way Around\n" );
	}

	if (strncmp(recalibrate, "true", 4) == 0) {
		cyttsp_recalibrate(drv_data);
		return 0;
	}

	printk(KERN_DEBUG CYTTSP_DEVNAME ": irq number [%d]\n", client->irq);
	set_irq_type(client->irq, IRQ_TYPE_EDGE_RISING);
	if (request_irq(client->irq, &cyttsp_irq_handler, IRQ_TYPE_EDGE_FALLING, "cyttsp_1", NULL)) {
		printk(KERN_ERR CYTTSP_DEVNAME " Could not allocate IRQ (cyttsp_1)!\n");
		ret = -EIO;
		goto error_request_irq;
	}

	err = sysfs_create_group(&cyttsp_cap_ts_input_dev->dev.kobj, &cyttsp_attr_group);
	if (err)
		goto error_sysfs_create_group;

	queue_delayed_work(watchdog_workqueue, &watchdog_work, msecs_to_jiffies(5000));
	
	return 0;

error_sysfs_create_group:
	free_irq(client->irq, NULL);
error_request_irq:
	input_unregister_device(cyttsp_cap_ts_input_dev);
error_input_register_device:
	input_free_device(cyttsp_cap_ts_input_dev);
error_alloc_input_device:
	destroy_workqueue(watchdog_workqueue);
error_create_watchdog_workqueue:
	destroy_workqueue(readinfo_workqueue);
error_create_readinfo_workqueue:
	mutex_destroy(&drv_data->lock);
	kfree(drv_data);
	return ret;
}

static int cyttsp_i2c_remove(struct i2c_client *client)
{

	printk(KERN_INFO CYTTSP_DEVNAME ": start remove\n");

	if (delayed_work_pending(&watchdog_work))
		cancel_delayed_work(&watchdog_work);

	flush_scheduled_work();

	sysfs_remove_group(&cyttsp_cap_ts_input_dev->dev.kobj, &cyttsp_attr_group);

	disable_irq(client->irq);

	free_irq(client->irq, NULL);

	input_unregister_device(cyttsp_cap_ts_input_dev);
	input_free_device(cyttsp_cap_ts_input_dev);

	destroy_workqueue(watchdog_workqueue);
	destroy_workqueue(readinfo_workqueue);

	mutex_destroy(&drv_data->lock);

	kfree(drv_data);

	printk(KERN_INFO CYTTSP_DEVNAME ": exit remove\n");

	return 0;
}

static int cyttsp_i2c_suspend(struct i2c_client *client, pm_message_t mesg)
{
	printk(KERN_INFO CYTTSP_DEVNAME ": start suspending\n");

	cancel_delayed_work(&watchdog_work);

	return 0;
}

static int cyttsp_i2c_resume(struct i2c_client *client)
{
	printk(KERN_INFO CYTTSP_DEVNAME ": start resuming\n");

	if (drv_data->ttsp_revision == TTSP_REVISION_J_OR_HIGHER)
		cyttsp_restart_ts();

	if (!delayed_work_pending(&watchdog_work))
		queue_delayed_work(watchdog_workqueue, &watchdog_work, msecs_to_jiffies(1000));


	return 0;
}

static const struct i2c_device_id cyttsp_id[] = {
	{ CYTTSP_DEVNAME, 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, cyttsp_id);

static struct i2c_driver cyttsp_i2c_driver = {
	/* TODO: verify if we need this (probably not, see include/linux/i2c-id.h)
	.id	= I2C_DRIVERID_PSOC_CTSIC,
	*/
	.probe	= cyttsp_i2c_probe,
	.remove	= cyttsp_i2c_remove,
	.suspend= cyttsp_i2c_suspend,
	.resume	= cyttsp_i2c_resume,
	.id_table = cyttsp_id,
	.driver = {
		.name	= CYTTSP_DEVNAME,
		.owner	= THIS_MODULE,
	},
};

static int __init cyttsp_init(void)
{
	int err;

	printk(KERN_INFO CYTTSP_DEVNAME " :start init\n");

	if ((err = i2c_add_driver(&cyttsp_i2c_driver)))
		printk(KERN_ERR CYTTSP_DEVNAME ": Could Not Be Added. Err Code: [%i]\n", err);

	return err;
}

static void __exit cyttsp_exit(void)
{
	printk(KERN_INFO CYTTSP_DEVNAME " :start exit\n");
	i2c_del_driver(&cyttsp_i2c_driver);
}

module_init(cyttsp_init);
module_exit(cyttsp_exit);
module_param(recalibrate, charp, S_IRUGO);

MODULE_AUTHOR("Vincent Dejouy <vincent.dejouy@tomtom.com>, Niels Langendorff <niels.langendorff@tomtom.com>");
MODULE_DESCRIPTION("Driver for I2C connected TTSP Capacitive Touchscreen IC.");
MODULE_LICENSE("GPL");
