/*
 * drivers/input/touchscreen/hx8519.c
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/i2c/hx8519.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/hrtimer.h>
#include <linux/io.h>
#include <linux/platform_device.h>


#define TS_POLL_DELAY			1 /* ms delay between samples */
#define TS_POLL_PERIOD			1 /* ms delay between samples */

#define hx8519_MEASURE_TEMP0		(0x0 << 4)
#define hx8519_MEASURE_AUX		(0x2 << 4)
#define hx8519_MEASURE_TEMP1		(0x4 << 4)
#define hx8519_ACTIVATE_XN		(0x8 << 4)
#define hx8519_ACTIVATE_YN		(0x9 << 4)
#define hx8519_ACTIVATE_YP_XN		(0xa << 4)
#define hx8519_SETUP			(0xb << 4)
#define hx8519_MEASURE_X		(0xc << 4)
#define hx8519_MEASURE_Y		(0xd << 4)
#define hx8519_MEASURE_Z1		(0xe << 4)
#define hx8519_MEASURE_Z2		(0xf << 4)

#define hx8519_POWER_OFF_IRQ_EN	(0x0 << 2)
#define hx8519_ADC_ON_IRQ_DIS0		(0x1 << 2)
#define hx8519_ADC_OFF_IRQ_EN		(0x2 << 2)
#define hx8519_ADC_ON_IRQ_DIS1		(0x3 << 2)

#define hx8519_12BIT			(0x0 << 1)
#define hx8519_8BIT			(0x1 << 1)

#define	MAX_12BIT			((1 << 12) - 1)

#define ADC_ON_12BIT	(hx8519_12BIT | hx8519_ADC_ON_IRQ_DIS0)

#define READ_Y		(ADC_ON_12BIT | hx8519_MEASURE_Y)
#define READ_Z1		(ADC_ON_12BIT | hx8519_MEASURE_Z1)
#define READ_Z2		(ADC_ON_12BIT | hx8519_MEASURE_Z2)
#define READ_X		(ADC_ON_12BIT | hx8519_MEASURE_X)
#define PWRDOWN		(hx8519_12BIT | hx8519_POWER_OFF_IRQ_EN)

#define TOUCH_NUMBER 4
#define PENUPCOUNT 20
#define RTSST_EVENTS_AVAILABLE 0x01000000

static struct workqueue_struct *hx8519_wq;

struct ts_event {
	u16	x;
	u16	y;
	u16	z1, z2;
};

struct hx8519 {
	struct input_dev	*input;
	char			phys[32];
  struct hrtimer timer;
	struct delayed_work	work;

	struct i2c_client	*client;

	u16			model;
	u16			x_plate_ohms;

	bool			pendown;
  int       pendownhysteris;
  uint32_t  status;
	int			irq;

	int			(*get_pendown_state)(void);
	void			(*clear_penirq)(void);
};

static void hx8519_work(struct work_struct *work)
{
	struct hx8519 *ts = container_of(to_delayed_work(work), struct hx8519, work);
  int i;
  int ret;
  unsigned long x, y;
  struct i2c_msg msg[2];
  uint8_t start_reg;
  uint32_t buf[4];
  struct input_dev *input = ts->input;

  // Read status
  start_reg = 0x84;

  msg[0].addr = ts->client->addr;
  msg[0].flags = 0;
  msg[0].len = 1;
  msg[0].buf = &start_reg;

  msg[1].addr = ts->client->addr;
  msg[1].flags = I2C_M_RD;
  msg[1].len = 4 * TOUCH_NUMBER;
  msg[1].buf = (__u8 *) &buf[0];

  ret = i2c_transfer(ts->client->adapter, msg, 2);
  if (buf[0] & RTSST_EVENTS_AVAILABLE)
  {
    // Read all events
    start_reg = 0x86;
  
    msg[0].addr = ts->client->addr;
    msg[0].flags = 0;
    msg[0].len = 1;
    msg[0].buf = &start_reg;
  
    msg[1].addr = ts->client->addr;
    msg[1].flags = I2C_M_RD;
    msg[1].len = 4 * TOUCH_NUMBER;
    msg[1].buf = (__u8 *) &buf[0];
  
    ret = i2c_transfer(ts->client->adapter, msg, 2);
    if (ret != 0) 
    {
      for(i=0; i<TOUCH_NUMBER; i++) 
      {
        if (buf[i] != 0xffffffff)
        {
          x = ((buf[i] & 0xff) << 8) | ((buf[i] & 0xff00) >> 8);
          y = ((buf[i] & 0xff0000) >> 8) | ((buf[i] & 0xff000000) >> 24);
          // tweak x/y to hx8519 touchpanel geometry
          y=((y*1500) >> 8) + 200;
          x=((x*1006)  >> 8) + 78;
  
          if ( x>78 && y>200 && x<MAX_12BIT && y<MAX_12BIT)
          {
            if(!ts->pendown)
            {
              ts->pendown = true;
              input_report_key(input, BTN_TOUCH, 1);
            }
  
            ts->pendownhysteris = 0;
            input_report_abs(input, ABS_X, x);
            input_report_abs(input, ABS_Y, y);
            input_report_abs(input, ABS_PRESSURE, 4080);
            input_sync(input);
          }
        }
        else
        {
          if(ts->pendown && (ts->pendownhysteris++ > PENUPCOUNT))
          {
            ts->pendown = false;
            input_report_key(input, BTN_TOUCH, 0);
            input_report_abs(input, ABS_PRESSURE, 0);
            input_sync(input);
            ts->pendownhysteris = PENUPCOUNT;
          }
        }
      }
    }
  }
}

static irqreturn_t hx8519_irq(int irq, void *handle)
{
	struct hx8519 *ts = handle;
	if (!ts->get_pendown_state || likely(ts->get_pendown_state()))
  {
		disable_irq_nosync(ts->irq);
		schedule_delayed_work(&ts->work,
				      msecs_to_jiffies(TS_POLL_DELAY));
	}

	if (ts->clear_penirq)
		ts->clear_penirq();

	return IRQ_HANDLED;
}
static enum hrtimer_restart himax_ts_timer_func(struct hrtimer *timer)
{
    struct hx8519 *ts = container_of(timer, struct hx8519, timer);

    queue_work(hx8519_wq, &ts->work);

    hrtimer_start(&ts->timer, ktime_set(0, 12500000), HRTIMER_MODE_REL);
    return HRTIMER_NORESTART;
}

static void hx8519_free_irq(struct hx8519 *ts)
{
	free_irq(ts->irq, ts);
	if (cancel_delayed_work_sync(&ts->work)) {
		/*
		 * Work was pending, therefore we need to enable
		 * IRQ here to balance the disable_irq() done in the
		 * interrupt handler.
		 */
		enable_irq(ts->irq);
	}
}

static int __devinit hx8519_probe(struct i2c_client *client,
				   const struct i2c_device_id *id)
{
	struct hx8519 *ts;
	struct hx8519_platform_data *pdata = pdata = client->dev.platform_data;
	struct input_dev *input_dev;
	int err;
  uint8_t buf0[6];
printk("hx8119 probe\n");
	if (!pdata) {
		dev_err(&client->dev, "platform data is required!\n");
		return -EINVAL;
	}

	if (!i2c_check_functionality(client->adapter,
				     I2C_FUNC_SMBUS_READ_WORD_DATA))
		return -EIO;

	ts = kzalloc(sizeof(struct hx8519), GFP_KERNEL);
	input_dev = input_allocate_device();
	if (!ts || !input_dev) {
		err = -ENOMEM;
		goto err_free_mem;
	}

	ts->client = client;
	ts->irq = client->irq;
	ts->input = input_dev;
	INIT_DELAYED_WORK(&ts->work, hx8519_work);

	ts->model             = pdata->model;
	ts->x_plate_ohms      = pdata->x_plate_ohms;
	ts->get_pendown_state = pdata->get_pendown_state;
	ts->clear_penirq      = pdata->clear_penirq;

	snprintf(ts->phys, sizeof(ts->phys),
		 "%s/input0", dev_name(&client->dev));

	input_dev->name = "hx8519 Touchscreen";
	input_dev->phys = ts->phys;
	input_dev->id.bustype = BUS_I2C;

	input_dev->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);
	input_dev->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);

	input_set_abs_params(input_dev, ABS_X, 0, MAX_12BIT, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, 0, MAX_12BIT, 0, 0);
	input_set_abs_params(input_dev, ABS_PRESSURE, 0, MAX_12BIT, 0, 0);

  buf0[0] = 0x76;
  buf0[1] = 0x01;
  buf0[2] = 0x2D;
  err = i2c_master_send(ts->client, buf0, 3);
  if(err < 0) {
    return -EIO;
  }

  /* IC internal power on */
  buf0[0] = 0x81;
  err = i2c_master_send(ts->client, buf0, 1);
  if(err < 0) {
    return -EIO;
  }
  msleep(300);
  /* Drawing */
  buf0[0] = 0xE9;
  buf0[1] = 0x00;
  err = i2c_master_send(ts->client, buf0, 2);
  if(err < 0) {
    return -EIO;
  }
  msleep(300);
  /* MCU power on */
  buf0[0] = 0x35;
  buf0[1] = 0x02;
  err = i2c_master_send(ts->client, buf0, 2);
  if(err < 0) {
    return -EIO;
  }
  msleep(300);
  /* flash power on */
  buf0[0] = 0x36;
  buf0[1] = 0x01;
  err = i2c_master_send(ts->client, buf0, 2);
  if(err < 0) {
    return -EIO;
  }
  msleep(300);

  if (pdata->init_platform_hw)
    pdata->init_platform_hw();

  hrtimer_init(&ts->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
  ts->timer.function = himax_ts_timer_func;
  hrtimer_start(&ts->timer, ktime_set(1, 0), HRTIMER_MODE_REL);

  /* Start touch panel sensing */
  buf0[0] = 0x83;
  err = i2c_master_send(ts->client, buf0, 1);
  if(err < 0) {
    goto err_free_irq;
  }
  msleep(300);

	if (err < 0)
		goto err_free_irq;

	err = input_register_device(input_dev);
	if (err)
		goto err_free_irq;

	i2c_set_clientdata(client, ts);

	return 0;

 err_free_irq:
	hx8519_free_irq(ts);
	if (pdata->exit_platform_hw)
		pdata->exit_platform_hw();
 err_free_mem:
	input_free_device(input_dev);
	kfree(ts);
	return err;
}

static int __devexit hx8519_remove(struct i2c_client *client)
{
	struct hx8519	*ts = i2c_get_clientdata(client);
	struct hx8519_platform_data *pdata = client->dev.platform_data;

	hx8519_free_irq(ts);

	if (pdata->exit_platform_hw)
		pdata->exit_platform_hw();

	input_unregister_device(ts->input);
	kfree(ts);

	return 0;
}

static struct i2c_device_id hx8519_idtable[] = {
	{ "hx8519", 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, hx8519_idtable);

static struct i2c_driver hx8519_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= "hx8519"
	},
	.id_table	= hx8519_idtable,
	.probe		= hx8519_probe,
	.remove		= __devexit_p(hx8519_remove),
};

static int __init hx8519_init(void)
{
    hx8519_wq = create_singlethread_workqueue("hx8519_wq");
    if (!hx8519_wq)
        return -ENOMEM;
    return i2c_add_driver(&hx8519_driver);
}

static void __exit hx8519_exit(void)
{
    if (hx8519_wq)
        destroy_workqueue(hx8519_wq);
	i2c_del_driver(&hx8519_driver);
}

module_init(hx8519_init);
module_exit(hx8519_exit);

MODULE_AUTHOR("");
MODULE_DESCRIPTION("hx8519 TouchScreen Driver");
MODULE_LICENSE("GPL");
