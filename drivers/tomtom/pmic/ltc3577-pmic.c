/* ltc3577-pmic.c
 *
 * Control driver for LTC3577 PMIC.
 *
 * Copyright (C) 2006 TomTom BV <http://www.tomtom.com/>
 * Authors: Benoit Leffray <benoit.leffray@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/platform_device.h>
#include <linux/cdev.h>
#include <linux/i2c.h>
#include <linux/workqueue.h>
#include <linux/spinlock.h>
#include <linux/ltc3577-pmic.h>
#include <plat/usbmode.h>
#include <linux/power_supply.h>
#include <linux/list.h>
#include <linux/err.h>

#define LTC3577_NB_COMMAND	5

#define LTC3577_USB_PSY_IDX	0
#define LTC3577_CLA_PSY_IDX	1

typedef enum
{
	LTC3577_STATE_NONE,
	LTC3577_STATE_CLA,
	LTC3577_STATE_USB,
	LTC3577_STATE_SUSPEND,
} ltc3577_pmic_state_e;

typedef struct _ltc3577_command
{
	struct list_head list;
	
	ltc3577_msg_t *msgs;
	int num;
	free_msgs callback;
} ltc3577_command_t;

typedef struct
{
	ltc3577_pmic_state_e	state;
	struct power_supply		*psu;
	struct semaphore		mutex;
} ltc3577_drv_data_t;

/* For future use, when we are doing CPUFrequency scaling. */
static ltc3577_pmic_state_e	ltc3577_pmic_get_state(ltc3577_drv_data_t *drv_data)
{
	BUG_ON(!drv_data);
	return drv_data->state;
}

static void ltc3577_pmic_set_state(ltc3577_drv_data_t *drv_data, ltc3577_pmic_state_e state)
{
	BUG_ON(!drv_data);
	drv_data->state = state;
}

enum power_supply_property	ltc3577_ac_properties[]=
{
	POWER_SUPPLY_PROP_PRESENT, POWER_SUPPLY_PROP_ONLINE,
};

static int ltc3577_pwr_get_claproperty( struct power_supply *psy, enum power_supply_property psp, union power_supply_propval *val )
{
	ltc3577_drv_data_t *drv_data = dev_get_drvdata(psy->dev->parent);
	BUG_ON(!drv_data);

	switch( psp )
	{
		case POWER_SUPPLY_PROP_PRESENT :
		case POWER_SUPPLY_PROP_ONLINE :
			if(    (psy->type == POWER_SUPPLY_TYPE_MAINS)
			    && (ltc3577_pmic_get_state(drv_data) == LTC3577_STATE_CLA))
				val->intval=1;
			else
				val->intval=0;
			break;

		default :
			/* Not supported property. */
			return -EINVAL;
	}
	return 0;
}

static int ltc3577_pwr_get_usbproperty( struct power_supply *psy, enum power_supply_property psp, union power_supply_propval *val)
{
	ltc3577_drv_data_t *drv_data = dev_get_drvdata(psy->dev->parent);
	BUG_ON(!drv_data);

	switch( psp )
	{
		case POWER_SUPPLY_PROP_PRESENT :
		case POWER_SUPPLY_PROP_ONLINE :
			if(    (psy->type == POWER_SUPPLY_TYPE_USB) 
			    && (ltc3577_pmic_get_state(drv_data) == LTC3577_STATE_USB))
				val->intval=1;
			else
				val->intval=0;
			break;

		default :
			/* Not supported property. */
			return -EINVAL;
	}
	return 0;
}

struct power_supply	ltc3577_power[]=
{
	{
		.name		= "usb",
		.type 		= POWER_SUPPLY_TYPE_USB,
		.properties	= ltc3577_ac_properties,
		.num_properties	= ARRAY_SIZE( ltc3577_ac_properties ),
		.get_property	= ltc3577_pwr_get_usbproperty,
	},
	{
		.name		= "mains",
		.type		= POWER_SUPPLY_TYPE_MAINS,
		.properties	= ltc3577_ac_properties,
		.num_properties	= ARRAY_SIZE( ltc3577_ac_properties ),
		.get_property	= ltc3577_pwr_get_claproperty,
	},
};

static int ltc3577_set_state_suspend(struct i2c_client *client)
{
	ltc3577_drv_data_t		*drv_data	= dev_get_drvdata(&client->dev);
	ltc3577_pdata_t *pdata		= (ltc3577_pdata_t *)client->dev.platform_data;

	BUG_ON(!drv_data);
	BUG_ON(!drv_data->psu);
	BUG_ON(!pdata);

	/* Set the LTC3577 to 500mA mode. This will allow the unit to charge while in suspend. */
	pdata->set_charge(eCHARGING_500mA);

	ltc3577_pmic_set_state(drv_data, LTC3577_STATE_SUSPEND);

	/* All PSUs are affected, so inform userspace. */
	power_supply_changed(&drv_data->psu[LTC3577_CLA_PSY_IDX]);
	power_supply_changed(&drv_data->psu[LTC3577_USB_PSY_IDX]);

	return 0;
}

static int ltc3577_state_change(struct i2c_client *client, USB_STATE previous_state, USB_STATE current_state )
{
	ltc3577_drv_data_t		*drv_data	= dev_get_drvdata(&client->dev);
	ltc3577_pdata_t	*pdata		= (ltc3577_pdata_t *) client->dev.platform_data;

	BUG_ON(!drv_data);
	BUG_ON(!drv_data->psu);
	BUG_ON(!pdata);

	switch (current_state)
	{
		case USB_STATE_IDLE :
		default:
			/* nothing connected, or in transition between states, disable charging */
			pdata->set_charge(eCHARGING_500mA);
			ltc3577_pmic_set_state(drv_data, LTC3577_STATE_NONE);
			printk(KERN_INFO LTC3577_DEVNAME 
						": USB_IDLE [%i]\n", current_state);
			break;
		case USB_STATE_HOST:
		case USB_STATE_CLA:
			/* CLA or wall pwr connected, enable 1A charging. */
			pdata->set_charge(eCHARGING_1A);
			ltc3577_pmic_set_state(drv_data, LTC3577_STATE_CLA);
			printk(KERN_INFO LTC3577_DEVNAME 
						": USB_HOST|CLA [%i]\n", current_state);
			break;
		case USB_STATE_DEVICE :
		case USB_STATE_DEVICE_WAIT :
			/* something connected, enable 500mA charging. */
			pdata->set_charge(eCHARGING_500mA);
			ltc3577_pmic_set_state(drv_data, LTC3577_STATE_USB);
			printk(KERN_INFO LTC3577_DEVNAME 
						": USB_DEVICE | DEVICE_WAIT [%i]\n", current_state);
			break;
	}

	/* Send power supply changed event. */
	if(previous_state == USB_STATE_INITIAL)
	{
		power_supply_changed(&drv_data->psu[LTC3577_CLA_PSY_IDX]);
		power_supply_changed(&drv_data->psu[LTC3577_USB_PSY_IDX]);
	} else {
		if(    (previous_state == USB_STATE_HOST) || (current_state == USB_STATE_HOST)
            || (previous_state == USB_STATE_CLA)  || (current_state == USB_STATE_CLA)) {
			power_supply_changed(&drv_data->psu[LTC3577_CLA_PSY_IDX]);
		}
		if((previous_state == USB_STATE_DEVICE) || (current_state == USB_STATE_DEVICE)) {
			power_supply_changed(&drv_data->psu[LTC3577_USB_PSY_IDX]);
		}
	}

	return 0;
}
	
/* This routine exists to change charging mode if the device is powered from USB. */
static void ltc3577_state_change_listener(USB_STATE previous_state, USB_STATE current_state, void* arg)
{
	ltc3577_state_change(arg, previous_state, current_state);
	return;
}

static struct _ltc3577_i2c {
	struct i2c_client	*client;
	struct work_struct	work;
} ltc3577_i2c;

static spinlock_t ltc3577_available_cmd_lock;
static spinlock_t ltc3577_scheduled_cmd_lock;
static ltc3577_command_t ltc3577_available_cmd_head;
static ltc3577_command_t ltc3577_scheduled_cmd_head;
static ltc3577_command_t ltc3577_commands[LTC3577_NB_COMMAND];

static struct workqueue_struct *ltc3577_workqueue;


static ltc3577_command_t * ltc3577_i2c_get_free_cmd(void)
{
	ltc3577_command_t *tmp;

	spin_lock(&ltc3577_available_cmd_lock);

	if (list_empty(&(ltc3577_available_cmd_head.list))) {
		spin_unlock(&ltc3577_available_cmd_lock);
		return NULL;
	}

	tmp = list_first_entry(&(ltc3577_available_cmd_head.list), ltc3577_command_t, list);
	BUG_ON(!tmp);
	list_del(&(tmp->list));
	
	spin_unlock(&ltc3577_available_cmd_lock);

	return tmp;
}

static void ltc3577_i2c_schedule_cmd(ltc3577_command_t *command)
{
	BUG_ON(!command);
	BUG_ON(!ltc3577_workqueue);

	spin_lock(&ltc3577_scheduled_cmd_lock);

	list_add_tail(&(command->list), &(ltc3577_scheduled_cmd_head.list));
	queue_work(ltc3577_workqueue, &ltc3577_i2c.work);

	spin_unlock(&ltc3577_scheduled_cmd_lock);
}

static ltc3577_command_t * ltc3577_i2c_get_scheduled_cmd(void)
{
	ltc3577_command_t *tmp;

	spin_lock(&ltc3577_scheduled_cmd_lock);

	if (list_empty(&(ltc3577_scheduled_cmd_head.list)))
		tmp = NULL;
	else
		tmp = list_first_entry(&(ltc3577_scheduled_cmd_head.list), ltc3577_command_t, list);

	if (NULL != tmp)
		list_del(&(tmp->list));

	spin_unlock(&ltc3577_scheduled_cmd_lock);

	return tmp;
}

static void ltc3577_i2c_free_cmd(ltc3577_command_t *command)
{
	BUG_ON(!command);

	spin_lock(&ltc3577_available_cmd_lock);
	INIT_LIST_HEAD(&(command->list));
	list_add(&(command->list), &(ltc3577_available_cmd_head.list));
	spin_unlock(&ltc3577_available_cmd_lock);
}


static void ltc3577_i2c_work(struct work_struct *work)
{
	int ret;
	ltc3577_command_t *tmp;

	BUG_ON(!work);

	do {
		tmp = ltc3577_i2c_get_scheduled_cmd();

		if (NULL != tmp) {
			ret = ltc3577_i2c_transfer(tmp->msgs, tmp->num);
			if (ret == tmp->num)
				printk(KERN_INFO LTC3577_DEVNAME ": [workqueue] i2c transfer successfull\n");
			else
				printk(KERN_INFO LTC3577_DEVNAME ": [workqueue] i2c transfer failure !!!\n");

			if (NULL != tmp->callback)
				tmp->callback(tmp->msgs);

			ltc3577_i2c_free_cmd(tmp);
		}
	} while (NULL != tmp);
}

static void ltc3577_i2c_init(struct i2c_client *client)
{
	BUG_ON(!client);
	memset(&ltc3577_i2c, 0x00, sizeof(ltc3577_i2c));
	ltc3577_i2c.client = client;
	INIT_WORK(&ltc3577_i2c.work, ltc3577_i2c_work);
}

/**
 * ltc3577_i2c_send - issue a single I2C message in master transmit mode
 * @buf: Data that will be written to the slave
 * @count: How many bytes to write
 *
 * Returns negative errno, or else the number of bytes written.
 */
int ltc3577_i2c_send(const char *buf, int count)
{
	int ret = -ENODEV;
	
	ltc3577_drv_data_t *drv_data = dev_get_drvdata(&ltc3577_i2c.client->dev);

	BUG_ON(!buf);

	if (down_interruptible(&drv_data->mutex))
		return -ERESTARTSYS;
	ret = i2c_master_send(ltc3577_i2c.client, buf, count);
	up(&drv_data->mutex);

	return ret;
}
EXPORT_SYMBOL(ltc3577_i2c_send);

/**
 * ltc3577_i2c_recv - issue a single I2C message in master receive mode
 * @buf: Where to store data read from slave
 * @count: How many bytes to read
 *
 * Returns negative errno, or else the number of bytes read.
 */
int ltc3577_i2c_recv(char *buf, int count)
{
	int ret = -ENODEV;

	ltc3577_drv_data_t *drv_data = dev_get_drvdata(&ltc3577_i2c.client->dev);
	BUG_ON(!buf);

	if (down_interruptible(&drv_data->mutex))
		return -ERESTARTSYS;
	ret = i2c_master_recv(ltc3577_i2c.client, buf, count);
	up(&drv_data->mutex);

	return ret;
}
EXPORT_SYMBOL(ltc3577_i2c_recv);

int ltc3577_i2c_transfer(ltc3577_msg_t *msgs, int num)
{
	int ret = -ENODEV;
	int i;

	ltc3577_drv_data_t *drv_data = dev_get_drvdata(&ltc3577_i2c.client->dev);
	BUG_ON(!msgs);

	if (down_interruptible(&drv_data->mutex))
		return -ERESTARTSYS;
	ret = 0;
	for (i = 0 ; i < num ; i++) {
		int count;

		count = i2c_master_send(ltc3577_i2c.client, msgs[i].buf, msgs[i].len);
		if (count == msgs[i].len)			
			ret += 1;
	}
	up(&drv_data->mutex);

	return ret;
}
EXPORT_SYMBOL(ltc3577_i2c_transfer);

int ltc3577_i2c_transfer_async(ltc3577_msg_t *msgs, int num, free_msgs callback)
{
	ltc3577_command_t *tmp;

	BUG_ON(!msgs);

	tmp = ltc3577_i2c_get_free_cmd();
	if (NULL == tmp)
		return -EAGAIN;

	INIT_LIST_HEAD(&(tmp->list));
	tmp->msgs		= msgs;
	tmp->num		= num;
	tmp->callback	= callback;

	ltc3577_i2c_schedule_cmd(tmp);

	return 0;
}
EXPORT_SYMBOL(ltc3577_i2c_transfer_async);

static int ltc3577_i2c_read_status_reg(struct i2c_client *client)
{
	ltc3577_pdata_t	*pdata = (ltc3577_pdata_t *) client->dev.platform_data;
    int send_ret, recv_ret;
	unsigned char reg_recv = 0x00;

	send_ret = ltc3577_i2c_send(&pdata->i2c_addr, sizeof(pdata->i2c_addr));

	if (sizeof(pdata->i2c_addr) == send_ret) {
		printk(KERN_INFO LTC3577_DEVNAME 
					": Read Request Sent, -%i- byte written\n", send_ret);

	 	recv_ret = ltc3577_i2c_recv(&reg_recv, sizeof(reg_recv));
		if (sizeof(reg_recv) == recv_ret) {	
    		printk(KERN_INFO LTC3577_DEVNAME
					": Received -%i- byte: [%x]\n", recv_ret, reg_recv);
		
			if (0x01 & reg_recv) {
				printk(KERN_INFO LTC3577_DEVNAME ": Status: --     Charging --\n");
			} else {
				printk(KERN_INFO LTC3577_DEVNAME ": Status: -- Not Charging --\n");
			}
		} else {
    		printk(KERN_INFO LTC3577_DEVNAME
					": Reception FAILURE. Err Code -%i-\n", recv_ret);
		}
	} else {
		printk(KERN_INFO LTC3577_DEVNAME 
					": Read Request Sending FAILURE. Err Code: -%i-\n", send_ret);
	}

	return 0;
};

static void ltc3577_init_lists(void)
{
	int i;

	spin_lock_init(&ltc3577_scheduled_cmd_lock);
	spin_lock_init(&ltc3577_available_cmd_lock);

	INIT_LIST_HEAD(&(ltc3577_available_cmd_head.list));
	INIT_LIST_HEAD(&(ltc3577_scheduled_cmd_head.list));

	for (i = 0 ; i < LTC3577_NB_COMMAND ; i++) {
		INIT_LIST_HEAD(&(ltc3577_commands[i].list));
		list_add(&(ltc3577_commands[i].list), &(ltc3577_available_cmd_head.list));
	}
}

extern struct platform_driver ltc3577_reg_driver;

static int ltc3577_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	ltc3577_drv_data_t *drv_data = NULL;

	ltc3577_pdata_t	* pdata = 
				(ltc3577_pdata_t *)client->dev.platform_data;
	USB_STATE curr_usb_state;

	printk("TomTom GO LTC3577 Driver, (C) 2008 TomTom BV\n" );

	if(NULL == pdata) {
		printk(KERN_ERR LTC3577_DEVNAME 
				": Missing platform data!\n");
		return -ENODEV;
	}

	ltc3577_init_lists();

	BUG_ON(!client);
	ltc3577_i2c_init(client);

	ltc3577_workqueue = create_singlethread_workqueue(LTC3577_DEVNAME);
	if(NULL == ltc3577_workqueue) {
		printk(KERN_ERR LTC3577_DEVNAME 
				": Can allocate workqueue!\n");
		return -ENOMEM;
	}

	drv_data = kmalloc(sizeof(ltc3577_drv_data_t), GFP_KERNEL);

	if(NULL == drv_data) {
		destroy_workqueue(ltc3577_workqueue);
		printk(KERN_ERR LTC3577_DEVNAME 
				": Can allocate driver data!\n");
		return -ENOMEM;
	}

	dev_set_drvdata(&client->dev, drv_data);

	drv_data->state 	= LTC3577_STATE_NONE;
	drv_data->psu   	= ltc3577_power; /* attach the power supplies to the platform_data. */
	init_MUTEX(&drv_data->mutex);

	pdata->init();

	/* I2c client is up and running, the i2c management is ready,       */
	/* the LTC3577 Regulator can be registered.                         */
	platform_driver_register(&ltc3577_reg_driver);

	/* In case commands have been submitted, schedule the workqueue now */
	/* to execute them asap.                                            */ 
	queue_work(ltc3577_workqueue, &ltc3577_i2c.work);

	/* Register the power supply info. */
	if(power_supply_register(&client->dev, &(drv_data->psu[LTC3577_USB_PSY_IDX])))
	{
		printk(KERN_ERR LTC3577_DEVNAME ": Can't register usb power supply. Aborting.\n");
		return -ENODEV;
	}

	if(power_supply_register(&client->dev, &(drv_data->psu[LTC3577_CLA_PSY_IDX])))
	{
		printk(KERN_ERR LTC3577_DEVNAME ": Can't register CLA power supply. Aborting.\n");
		return -ENODEV;
	}

	/* Register the usbmode statechange listener. This will inform us when to change the */
	/* PMIC's settings. */
	if(0 != add_usb_state_change_listener(ltc3577_state_change_listener, client, &curr_usb_state))
	{
		printk(KERN_ERR LTC3577_DEVNAME ": Can't register USBMODE change state listener. Aborting.\n");
		return -ENODEV;
	}

	/* Determine and adjust the real state. */
	if(0 != ltc3577_state_change(client, USB_STATE_INITIAL, curr_usb_state))
	{
		printk(KERN_ERR LTC3577_DEVNAME ": Can't change to current state. Aborting.\n");
		return -EFAULT;
	}

	ltc3577_i2c_read_status_reg(client);

	return 0;
}

static int ltc3577_i2c_remove(struct i2c_client *client)
{
	ltc3577_drv_data_t *drv_data = dev_get_drvdata(&client->dev);
	BUG_ON(!drv_data);

	/* First unregister the I2C client's members. */
	if( remove_usb_state_change_listener( ltc3577_state_change_listener, NULL ) != 0 )
		printk(KERN_ERR LTC3577_DEVNAME 
				": Couldn't unregister USBMODE change state listener!\n" );

	power_supply_unregister(&(drv_data->psu[LTC3577_USB_PSY_IDX]));
	power_supply_unregister(&(drv_data->psu[LTC3577_CLA_PSY_IDX]));

	kfree(drv_data);

	flush_workqueue(ltc3577_workqueue);
	destroy_workqueue(ltc3577_workqueue);

	return 0;
}

#ifdef CONFIG_PM
static int ltc3577_i2c_suspend(struct i2c_client *client, pm_message_t state)
{
	/* Remove the state change listener. This since we want to ensure we're NOT receiving a */
	/* usbmode event while our driver is in suspend. */
	if(remove_usb_state_change_listener(ltc3577_state_change_listener, NULL) != 0)
		printk(KERN_ERR LTC3577_DEVNAME 
				": Couldn't unregister USBMODE change state listener!\n");

	if(ltc3577_set_state_suspend(client) != 0)
		printk(KERN_ERR LTC3577_DEVNAME ": Can't change state to suspend!\n");
	
	return 0;
}

static int ltc3577_i2c_resume(struct i2c_client *client)
{
	ltc3577_pdata_t * pdata = 
				(ltc3577_pdata_t *) client->dev.platform_data;
	USB_STATE curr_usb_state;

	pdata->init();

	if( add_usb_state_change_listener( ltc3577_state_change_listener, client, &curr_usb_state ) != 0) {
		printk( KERN_ERR LTC3577_DEVNAME 
				": Can't register USBMODE change state listener. Aborting.\n" );
		return -ENODEV;
	}

	if( ltc3577_state_change(client, USB_STATE_INITIAL, curr_usb_state ) != 0) {
		printk( KERN_ERR LTC3577_DEVNAME
				": Can't change to current state. Aborting.\n" );
		return -ENODEV;
	}

	return 0;
}

#else
#define ltc3577_suspend		NULL
#define ltc3577_resume		NULL
#endif /* CONFIG_PM	*/

static struct i2c_driver ltc3577_i2c_driver=
{
	.id     = -1,
	.probe	= ltc3577_i2c_probe,
	.remove	= ltc3577_i2c_remove,
	.suspend= ltc3577_i2c_suspend,
	.resume	= ltc3577_i2c_resume,
	.driver = {
		.name	= LTC3577_DEVNAME,
		.owner	= THIS_MODULE,
	},
};

static int __init ltc3577_init(void)
{
	int err;

    if ((err = i2c_add_driver(&ltc3577_i2c_driver))) {
		printk(KERN_ERR LTC3577_DEVNAME 
					": Could Not Be Added. Err Code: [%i]\n", err);
	}

	return err;
}

static void __exit ltc3577_exit(void)
{
	i2c_del_driver(&ltc3577_i2c_driver);
}




MODULE_AUTHOR("Benoit Leffray <benoit.leffray@tomtom.com>");
MODULE_DESCRIPTION("Driver for I2C connected LTC3577 PM IC.");
MODULE_LICENSE("GPL");

module_init(ltc3577_init);
module_exit(ltc3577_exit);
