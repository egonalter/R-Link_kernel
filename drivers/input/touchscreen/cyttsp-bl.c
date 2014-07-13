/* drivers/input/touchscreen/cyttsp-bl.c
 *
 * Bootloader driver for Cypress TTSP IC
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
#include <linux/firmware.h>
#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/jiffies.h>
#include <linux/ctype.h>
#include <plat/fdt.h>

static struct work_struct	update_work;
static 	struct work_struct	sys_info_work;
static 	struct work_struct	timeout_work;
static 	struct delayed_work	delayed_work;

static struct firmware cyttsp_firmware;
static int index_firmware = 0;
static char current_state = STATE_INITIAL;
static struct cyttsp_pdata pdata;
static struct cyttsp_drv_data *drv_data;
static int interrupt_triggered = 0;

static u16 app_ver = 0;

// Returns 0 if bootloader commands are accepted
// Returns 1 if bootloader commands are not accepted
// Returns -1 on communication errors
static int cyttsp_bootloader_accept_command(void)
{
	int ret;
	char buf[3];

	/* Due to limitation of TMA3xx we have to read from the beginning (so index '0') */
	ret = cyttsp_read_data(drv_data, BL_FILE, buf, 3);
	if (ret != 0) {
		printk (KERN_ERR CYTTSP_DEVNAME ": Error reading BL registers from the touch screen\n");
		return -1;
	}

	CYTTSP_DBG ("BL_ERROR = 0x%X\n", buf[2]);

	if (buf[2] == BL_ERROR_BOOTLOADING)
		return 0;

	printk (KERN_ERR CYTTSP_DEVNAME ": BL_ERROR value incorrect:\n");
	printk ("BL_ERROR  = 0x%X\n", buf[2]);

	return 1;
}

// Returns 0 if erase not finished
// Returns 1 if erase finished
// Returns -1 on communication errors
static int cyttsp_flash_erase_finished(void)
{
	int ret;
	char val;

	ret = cyttsp_read_register(drv_data, BL_STATUS, &val);
	if (ret != 0) {
		printk (KERN_ERR CYTTSP_DEVNAME ": Error reading BL registers from the touch screen\n");
		return -1;
	}

	CYTTSP_DBG ("BL_STATUS = 0x%X\n", val);

	if ((val & BL_STATUS_BUSY) != 0) 
		return 0;

	return 1;
}

static char cyttsp_atoc(char* buff, u8 * ptr)
{
	unsigned long val;
	char tmp[3];
	int ret;

	/* Convert to ascii delimited string */
	tmp[0] = buff[0];
	tmp[1] = buff[1];
	tmp[2] = '\0';

	ret = strict_strtoul(tmp, 16, &val);
	*ptr = (u8)val;

	return ret;
}

static int cyttsp_extract_line(char* buffer, char* last_line)
{
	int i = 0;

	while ((*cyttsp_firmware.data != '\n') 
			&& (*cyttsp_firmware.data != '\r') 
			&& (i < (MAX_CHAR_PER_LINE - 2))) {
		if (index_firmware == (int)cyttsp_firmware.size) {
			CYTTSP_DBG ("end of file detected\n");
			*last_line = 1;
			break;
		}
		buffer[i++] = *(cyttsp_firmware.data++);
		index_firmware++;
	}

	buffer[i] = 0;

	if (!i) {
		cyttsp_firmware.data++;
		index_firmware++;
	}

	return i;

}

static int cyttsp_write_generic(char* data, int length)
{
	char buf[MAX_LENGTH_I2C_FRAME + SIZE_PREFIX_BOOTLOADER_CMD];
	int len;
	short index = 0;
	int remaining;

	remaining = length;
	while (remaining > 0) {
		buf[0] = index >> 8;
		buf[1] = index & 0xFF;

		len = (remaining > MAX_LENGTH_I2C_FRAME) ? MAX_LENGTH_I2C_FRAME : remaining;
		memcpy((void *)&buf[SIZE_PREFIX_BOOTLOADER_CMD], 
				(void *)&data[index], len);
		cyttsp_i2c_send(drv_data->client, buf, len + SIZE_PREFIX_BOOTLOADER_CMD);

		index += len;
		remaining -= len;

		mdelay(DELAY_BETWEEN_CMD);
	}
	return 0;
}

static int cyttsp_write_block_state(char* buff, int length)
{
	int error = 0;
	int retries = 50;

	CYTTSP_DBG ("Write Block %.2X%.2X\n", buff[10], buff[11]);
	cyttsp_write_generic(buff, length);
	msleep(DELAY_AFTER_WRITE);

	while (cyttsp_bootloader_accept_command() > 0) {
		msleep(DELAY_BETWEEN_READS);
		retries--;
		if (!retries) {
			error = 1;
			break;
		}
	}

	if (error)
		printk (KERN_ERR CYTTSP_DEVNAME ": Error of status in state write block\n");

	return error;
}

static int cyttsp_enter_bootloader_state(char* buff, int length)
{
	int error = 0;
	int retries  = 500;
	
	CYTTSP_DBG ("Enter Bootloader Mode\n");
	CYTTSP_DBG ("Key: %.2X%.2X%.2X%.2X%.2X%.2X%.2X%.2X\n", buff[2], buff[3], buff[4], buff[5], buff[6], buff[7], buff[8], buff[9]);

	cyttsp_write_generic(buff, length);
	msleep(DELAY_AFTER_ERASE);

	cyttsp_set_loglevel(LOG_DISABLED);

	while (!cyttsp_flash_erase_finished()) {
		msleep(DELAY_BETWEEN_READS);
		retries--;
		if (!retries) {
			error = 1;
			break;
		}
	}
	cyttsp_restore_loglevel();

	if (error) {
		printk (KERN_ERR CYTTSP_DEVNAME ": Error of flash erase status in state enter bootloader\n");
		return error;
	}

	retries = 50;

	while(cyttsp_bootloader_accept_command() > 0) {
		msleep(DELAY_BETWEEN_READS);
		retries--;
		if (!retries) {
			error = 1;
			break;
		}
	}

	if (error)
		printk (KERN_ERR CYTTSP_DEVNAME ": Error of status in state enter bootloader\n");

	return error;
}

static int cyttsp_exit_bootloader_state(char* buff, int length)
{
	CYTTSP_DBG ("Exit Bootloader Mode\n");
	CYTTSP_DBG ("Key: %.2X%.2X%.2X%.2X%.2X%.2X%.2X%.2X\n", buff[2], buff[3], buff[4], buff[5], buff[6], buff[7], buff[8], buff[9]);

	cyttsp_write_generic(buff, length);

	return 0;
}

static int cyttsp_execute_state(char state, char* buffer, int length)
{
	int ret = 0; /* To prevent compiler warning */

	switch (state) {
		case STATE_ENTER_BOOTLOADER: 
			ret = cyttsp_enter_bootloader_state(buffer, length);
			break;
		case STATE_WRITE_BLOCK: 
			ret = cyttsp_write_block_state(buffer, length);
			break;
		case STATE_EXIT_BOOTLOADER: 
			ret = cyttsp_exit_bootloader_state(buffer, length);
			break;
		default: 
			ret = ERROR_UNKNOWN_STATE;
			break;
	}

	return ret;
}

static int cyttsp_process_line(char* buffer, int length)
{
	const char cmd = buffer[1];

	if (buffer[0] != CHECKSUM_SEED_CMD)
		return ERROR_INVALID_FIRMWARE_FORMAT;

	switch (cmd) {
		case INITIATE_BOOTLOAD_CMD:
		case WRITE_BLOCK_CMD:
		case TERMINATE_BOOTLOAD_CMD:
			break;
		default: 
			return ERROR_UNKNOWN_CMD;
	}

	switch (current_state) {
		case STATE_INITIAL: 
			if (cmd == INITIATE_BOOTLOAD_CMD)
				current_state = STATE_ENTER_BOOTLOADER;
			else
				return ERROR_FIRMWARE_DOES_NOT_START_WITH_ENTER_BL_CMD;
			break;
		case STATE_ENTER_BOOTLOADER: 	
			if (cmd == INITIATE_BOOTLOAD_CMD)
				current_state = STATE_ENTER_BOOTLOADER;
			else if (cmd == WRITE_BLOCK_CMD)
				current_state = STATE_WRITE_BLOCK;
			else
				return ERROR_INVALID_CMD_SEQUENCE;
			break;
		case STATE_WRITE_BLOCK: 
			if (cmd == WRITE_BLOCK_CMD)
				current_state = STATE_WRITE_BLOCK;
			else if (cmd == TERMINATE_BOOTLOAD_CMD)
				current_state = STATE_EXIT_BOOTLOADER;
			else
				return ERROR_INVALID_CMD_SEQUENCE;
			break;
		case STATE_EXIT_BOOTLOADER: 
			if (cmd == TERMINATE_BOOTLOAD_CMD)
				current_state = STATE_EXIT_BOOTLOADER;
			else
				return ERROR_INVALID_CMD_SEQUENCE;
			break;
		default:
			return ERROR_UNKNOWN_STATE;
	}

	CYTTSP_DBG ("\nExecute state %d\n", current_state);

	return cyttsp_execute_state(current_state, buffer, length);
}

static int cyttsp_parse_lines(void)
{
	int ascii_length = 0;
	int len = 0;
	char flag_last_line = 0;
	int i, j, k;
	int ret;
	u8	hexval;
	char buf_ascii[MAX_CHAR_PER_LINE];
	char buf_hex[MAX_BYTE_PER_LINE];

	k = 0;

	while (!flag_last_line) {
		ascii_length = cyttsp_extract_line(buf_ascii, &flag_last_line);

		if (ascii_length) {
			len = ascii_length / 2;
			j = 0;

			for (i = 0; i < ascii_length; i += 2) {
				if (cyttsp_atoc(buf_ascii + i, &hexval)) {
					printk (KERN_ERR CYTTSP_DEVNAME ": Error processing line %d invalid character detected\n", k);
					return -1;
				}
				buf_hex[j++] = hexval;
			}

			ret = cyttsp_process_line(buf_hex, len);

			if (ret) {
				printk (KERN_ERR CYTTSP_DEVNAME ": Error processing line %d\n", k);
				return -1;
			}
			k++;
		}
	}

	return 0;

}

static int cyttsp_try_i2c_communication(void)
{
	int i = 0;
	char test;

	cyttsp_set_loglevel(LOG_ERROR);

	while (i < MAX_RETRIES_BL_ACCESS) {
		msleep(DELAY_AFTER_RESET);
		if (cyttsp_i2c_recv(drv_data->client, &test, 1) == 1)
			break;
		i++;
	}

	cyttsp_restore_loglevel();

	if (i < MAX_RETRIES_BL_ACCESS)
		return 0;
	else
		return 1;
}

static void cyttsp_loading_finished(const struct firmware *fw, void* context)
{

	int ret;

	printk (KERN_ERR CYTTSP_DEVNAME ": request firmware finished\n");
	if (fw != NULL)
		memcpy((void*) &cyttsp_firmware, (void*) fw, sizeof(struct firmware));
	else
		return;

	printk (KERN_ERR CYTTSP_DEVNAME ": firmware received\n");
	printk (KERN_ERR CYTTSP_DEVNAME ": update in progress...\n");

	ret = cyttsp_parse_lines();

	msleep(DELAY_AFTER_UPDATE);
	pdata.reset_ts();
	cyttsp_try_i2c_communication();
	cyttsp_start_ts_app(drv_data, pdata.key, SIZE_BOOTLOADER_KEY);	
	cyttsp_recalibrate(drv_data);

	if (ret == 0)
		printk (KERN_ERR CYTTSP_DEVNAME ": update of the capacitive touch screen firmware successful ...\n");
	else
		printk (KERN_ERR CYTTSP_DEVNAME ": update of the capacitive touch screen firmware failed ...\n");
}

static ssize_t cyttsp_fw_version_show(struct device *dev,
                                     struct device_attribute *attr, char *buf)
{
	ssize_t temp;

	mutex_lock(&drv_data->lock);

	temp = sprintf(buf, "%X.%.2X\n", 
			(app_ver & 0xFF00) >> 8,
			app_ver & 0x00FF);

	mutex_unlock(&drv_data->lock);

	return temp;
}

static ssize_t cyttsp_fw_version_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	return count;
}

static DEVICE_ATTR(fw_version, 0664, cyttsp_fw_version_show, cyttsp_fw_version_store);

static struct attribute *cyttsp_attributes[] = {
	&dev_attr_fw_version.attr,
	NULL,
};

static struct attribute_group cyttsp_attr_group = {
	.attrs = cyttsp_attributes,
};

static void cyttsp_update_work(struct work_struct *work)
{
	int ret;

	ret = request_firmware_nowait(THIS_MODULE, FW_ACTION_HOTPLUG, 
			"fw_cap_ts.dld", &drv_data->client->dev, NULL, cyttsp_loading_finished);

	if (ret != 0)
		printk (KERN_ERR CYTTSP_DEVNAME ": Request firmware failed, return code = %d\n", ret);
}

static int cyttsp_get_app_ver(void)
{
	int ret = 0;

	switch (drv_data->ttsp_revision) {
		case TTSP_REVISION_C:
			ret = drv_data->si_mode.revc.app_ver;
			break;
		case TTSP_REVISION_J_OR_HIGHER:
			ret =  drv_data->si_mode.revj.app_ver;
			break;
		case TTSP_REVISION_UNKNOWN:
		default:
			BUG_ON(drv_data->ttsp_revision == TTSP_REVISION_UNKNOWN);
			break;
	}

	return ret;
}

static void cyttsp_sys_info_work(struct work_struct *work)
{

	mutex_lock(&drv_data->lock);

	interrupt_triggered = 1;

	if (drv_data->current_mode == SYS_INFO_MODE)  {
		cyttsp_set_offset_register(drv_data, HST_MODE);

		/* Get system information */
		cyttsp_read_systeminfo(drv_data);

		/* Store application version */
		app_ver = cyttsp_get_app_ver();

		drv_data->current_mode=OPERATING_MODE;
		disable_irq(drv_data->client->irq);
		pdata.reset_ts();
		cyttsp_try_i2c_communication();
		printk (KERN_ERR CYTTSP_DEVNAME ": Capacitive Touch Screen Reset\n");

		printk (KERN_ERR CYTTSP_DEVNAME ": Start the update firmware application\n");
		schedule_work(&update_work);
	}
	
	mutex_unlock(&drv_data->lock);
}

static void cyttsp_timeout_work(struct work_struct *work)
{
	mutex_lock(&drv_data->lock);

	if (!interrupt_triggered) {
		app_ver = 0x0FFE;

		printk (KERN_ERR CYTTSP_DEVNAME ": Can't read firmware version, most probably previous firmware update interrupted\n");
		printk (KERN_ERR CYTTSP_DEVNAME ": Now the touch screen is blanck\n");
		drv_data->current_mode=OPERATING_MODE;
		disable_irq(drv_data->client->irq);
		pdata.reset_ts();
		cyttsp_try_i2c_communication();
		printk (KERN_ERR CYTTSP_DEVNAME ": Capacitive Touch Screen Reset\n");

		printk (KERN_ERR CYTTSP_DEVNAME ": Start the update firmware application\n");
		schedule_work(&update_work);
	}

	mutex_unlock(&drv_data->lock);
}

static void cyttsp_timer(struct work_struct *work)
{
	schedule_work(&timeout_work);
}

static irqreturn_t cyttsp_irq_handler(int irq, void *dev_id)
{
	schedule_work(&sys_info_work);
	return IRQ_HANDLED;
};

static int cyttsp_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	char tmp;
	int err;

	app_ver = 0;

	index_firmware = 0;
	interrupt_triggered = 0;

	printk (KERN_INFO CYTTSP_DEVNAME ": probing driver\n");

	printk(KERN_ERR CYTTSP_DEVNAME ": Cypress TTSP Bootloader Driver, (C) 2011 TomTom BV\n" );

	BUG_ON(!client);
	BUG_ON(!client->dev.platform_data);

	drv_data = kmalloc(sizeof(struct cyttsp_drv_data), GFP_KERNEL);
	if(NULL == drv_data) {
		printk (KERN_ERR CYTTSP_DEVNAME ": Can allocate driver data!\n");
		return -ENOMEM;
	}
	drv_data->client = client;
	drv_data->ttsp_revision = TTSP_REVISION_UNKNOWN;
	drv_data->current_mode = OPERATING_MODE;

	memcpy((void*) &pdata, (void*) client->dev.platform_data, sizeof(struct cyttsp_pdata));
	INIT_WORK(&update_work, cyttsp_update_work);
	INIT_WORK(&sys_info_work, cyttsp_sys_info_work);
	INIT_WORK(&timeout_work, cyttsp_timeout_work);

	dev_set_drvdata(&client->dev, drv_data);
	mutex_init(&drv_data->lock);

	pdata.reset_ts();

	if (!cyttsp_try_i2c_communication()) {
		/* Read bootloader information to retrieve TTSP revision */
		cyttsp_read_bootloader(drv_data);

		drv_data->ttsp_revision = cyttsp_get_ttsp_revision(drv_data);

		cyttsp_start_ts_app(drv_data, pdata.key, SIZE_BOOTLOADER_KEY);	

		// If it is possible to read ts app it means that the fw version is
		// 3.32 or above so it is possible to read the fw version
		// Otherwise version 0.00 is returned in sysfs
		cyttsp_set_loglevel(LOG_DISABLED);
		err = cyttsp_read_register(drv_data, 0, &tmp);
		cyttsp_restore_loglevel();

		if (!err) {
			printk (KERN_INFO CYTTSP_DEVNAME ": irq number [%d]\n", client->irq);

			set_irq_type(client->irq, IRQ_TYPE_EDGE_RISING);
			if (request_irq(client->irq, &cyttsp_irq_handler, IRQ_TYPE_EDGE_FALLING, "cyttsp_1", NULL)) {
				printk (KERN_ERR CYTTSP_DEVNAME " Could not allocate IRQ (cyttsp_1)!\n");
				err = -EIO;
				goto error_request_irq;
			}

			mutex_lock(&drv_data->lock);

			// Switch to system information mode
			cyttsp_switch_to_other_mode(drv_data, SYS_INFO_MODE);

			mutex_unlock(&drv_data->lock);

			INIT_DELAYED_WORK(&delayed_work, cyttsp_timer);
			schedule_delayed_work(&delayed_work, msecs_to_jiffies(2000));
		} else {
			pdata.reset_ts();
			cyttsp_try_i2c_communication();
			printk (KERN_ERR CYTTSP_DEVNAME ": Capacitive Touch Screen Reset\n");	
			schedule_work(&update_work);
		}
	} else {
		app_ver = 0x0FFF;

		printk (KERN_ERR CYTTSP_DEVNAME ": I2C Communication Impossible with the Bootloader\n");
		printk (KERN_ERR CYTTSP_DEVNAME ": Most Probably Wrong Factory Data\n");
		printk (KERN_ERR CYTTSP_DEVNAME ": Check the Field /features/ts\n");
		printk (KERN_ERR CYTTSP_DEVNAME ": Change the Value 300E into 300E_FW_V2\n");
		printk (KERN_ERR CYTTSP_DEVNAME ": Or the Other Way Around\n");
	}

	err = sysfs_create_group(&client->dev.kobj, &cyttsp_attr_group);
	if (err)
		goto error_sysfs_create_group;

	return 0;

error_sysfs_create_group:
	cancel_delayed_work(&delayed_work);
	flush_scheduled_work();
	free_irq(client->irq, NULL);
error_request_irq:
	mutex_destroy(&drv_data->lock);
	kfree(drv_data);

	return err;
}

static int cyttsp_i2c_remove(struct i2c_client *client)
{
	printk (KERN_INFO CYTTSP_DEVNAME ": start remove\n");

	sysfs_remove_group(&client->dev.kobj, &cyttsp_attr_group);

	cancel_delayed_work(&delayed_work);
	flush_scheduled_work();

	disable_irq(client->irq);
	free_irq(client->irq, NULL);
	mutex_destroy(&drv_data->lock);
	kfree(drv_data);

	return 0;
}

static int cyttsp_i2c_suspend(struct i2c_client *client, pm_message_t mesg)
{
	printk (KERN_INFO CYTTSP_DEVNAME ": start suspending\n");
	return 0;
}

static int cyttsp_i2c_resume(struct i2c_client *client)
{
	printk (KERN_INFO CYTTSP_DEVNAME ": start resuming\n");
	return 0;
}

static const struct i2c_device_id cyttsp_id[] = {
	{ CYTTSP_DEVNAME, 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, cyttsp_id);

static struct i2c_driver cyttsp_i2c_driver=
{
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

	printk (KERN_ERR CYTTSP_DEVNAME ": start init\n");

	if ((err = i2c_add_driver(&cyttsp_i2c_driver))) {
		printk (KERN_ERR CYTTSP_DEVNAME ": Could Not Be Added. Err Code: [%i]\n", err);
	}

	return err;
}

static void __exit cyttsp_exit(void)
{
	printk (KERN_INFO CYTTSP_DEVNAME ": start exit\n");
	i2c_del_driver(&cyttsp_i2c_driver);
}

module_init(cyttsp_init);
module_exit(cyttsp_exit);

MODULE_AUTHOR("Vincent Dejouy <vincent.dejouy@tomtom.com>, Niels Langendorff <niels.langendorff@tomtom.com>");
MODULE_DESCRIPTION("Driver for Bootloader Of TTSP, Capacitive Touch Screen Controller.");
MODULE_LICENSE("GPL");
