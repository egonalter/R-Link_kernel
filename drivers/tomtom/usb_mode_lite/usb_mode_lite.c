/* linux/drivers/tomtom/usb_mode_lite/usb_mode_lite.c
 *
 * Copyright (c) 2012 TomTom International BV
 *
 * Author: Matthias Kaehlcke <matthias.kaehlcke@tomtom.com
 *
 * USB charger detection and OTG mode switching driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/kthread.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/usb_mode_lite.h>

/********************************************************************************************************
 *
 * The purpose of this driver is to determine the type of "device" connected to the USB bus, switch
 * to the corresponding OTG mode (host or device) and inform the power supply driver about the
 * charger type (USB or wall charger)
 *
 * The supported "devices" are:
 *
 *  - USB host (device mode, slow charging)
 *  - ACA/Poznan (host mode, fast charging
 *  - wall charger (no usb, fast_ charging)
 *
 *
 * The driver is implemented as a finite state machine. The states and transitions of the state machine
 * are described in the following table:
 *
 *
 *      state / event     |     vbus_high      | vbus_low  |  usb_reset   |   usb_reset_timeout
 *  ----------------------+--------------------+-----------+--------------+--------------------------
 *   init                 | wait_for_usb_reset | unplugged |      -       |           -
 *   unplugged            | wait_for_usb_reset |     -     |      -       |           -
 *   wait_for_usb_reset   |         -          | unplugged | host_plugged | aca/wall_charger_plugged
 *   host_plugged         |         -          | unplugged |      -       |           -
 *   aca_plugged          |         -          | unplugged |      -       |           -
 *   wall_charger_plugged |         -          | unplugged |      -       |           -
 *   resume               | wait_for_usb_reset | unplugged |      -       |           -
 *
 ********************************************************************************************************/

#define USB_RESET_TIMEOUT	500
#define USB_RESET_TIMEOUT_BOOT	10000

#define EVENT_QUEUE_SIZE	10

typedef enum {
	StateInit = 0,
	StateResume,
	StateUnplugged,
	StateWaitForUsbReset,
	StateUsbHostPlugged,
	StateAcaPlugged,
	StateWallChargerPlugged,
} state_t;

typedef enum {
	EvVbusHigh = 0,
	EvVbusLow,
	EvUsbReset,
	EvTimeoutUsbReset,
	EvNone,
} event_t;

struct uml_event
{
	event_t event;
	struct list_head node;
};

static struct {
	struct device *dev;
	struct usb_mode_lite_platform_data *pdata;

	struct {
		state_t state;
		struct list_head event_queue;
		struct list_head event_mem_pool;
		spinlock_t lock_events;
		struct task_struct *thread;
		wait_queue_head_t wq;
		struct delayed_work usb_reset_work;
	} stm;
} uml;

static int uml_event_pending(void);
static event_t uml_get_next_event(void);
static void uml_queue_event(event_t event);
static void uml_flush_event_queue(void);

static inline int uml_is_vbus_on(void)
{
	return gpio_get_value(uml.pdata->gpio_vbus);
}

static void uml_enter_state_unplugged(void)
{
	dev_dbg(uml.dev, "entering state UNPLUGGED\n");

	cancel_delayed_work(&uml.stm.usb_reset_work);

	if (uml.stm.state == StateAcaPlugged) {
		dev_dbg(uml.dev, "switching to OTG device mode\n");

		if (uml.pdata->switch_otg_mode(OTG_DEVICE_MODE))
			dev_err(uml.dev, "Failed to switch to OTG device mode\n");
	}

	uml.stm.state = StateUnplugged;

	uml.pdata->set_charger_type(POWER_SUPPLY_TYPE_BATTERY);
}

static void uml_enter_state_wait_for_usb_reset(void)
{
	unsigned long timeout;

	dev_dbg(uml.dev, "entering state WAIT_FOR_USB_RESET\n");

	if (uml.stm.state == StateInit)
		timeout = USB_RESET_TIMEOUT_BOOT;
	else
		timeout = USB_RESET_TIMEOUT;

	schedule_delayed_work(&uml.stm.usb_reset_work, msecs_to_jiffies(timeout));

	uml.stm.state = StateWaitForUsbReset;
}

static void uml_enter_state_usb_host_plugged(void)
{
	dev_dbg(uml.dev, "entering state USB_HOST_PLUGGED\n");

	uml.stm.state = StateUsbHostPlugged;

	uml.pdata->set_charger_type(POWER_SUPPLY_TYPE_USB);
}

static void uml_enter_state_aca_plugged(void)
{
	dev_dbg(uml.dev, "entering state ACA_PLUGGED\n");

	uml.stm.state = StateAcaPlugged;

	dev_dbg(uml.dev, "switching to OTG host mode\n");

	if (uml.pdata->switch_otg_mode(OTG_HOST_MODE))
		dev_err(uml.dev, "failed to switch to OTG host mode\n");

	uml.pdata->set_charger_type(POWER_SUPPLY_TYPE_MAINS);
}

static void uml_enter_state_wall_charger_plugged(void)
{
	dev_dbg(uml.dev, "entering state WALL_CHARGER_PLUGGED\n");

	uml.stm.state = StateWallChargerPlugged;

	uml.pdata->set_charger_type(POWER_SUPPLY_TYPE_MAINS);
}

static void uml_handle_unexpected_event(int event)
{
	dev_warn(uml.dev, "ignoring unexpected event %d in state %d\n", event, uml.stm.state);
}

static void _uml_do_meta_state_init_or_resume(int event)
{
	switch (event) {
		case EvVbusHigh:
			uml_enter_state_wait_for_usb_reset();

			break;

		case EvVbusLow:
			uml_enter_state_unplugged();

			break;

		default:
			uml_handle_unexpected_event(event);
	}
}

static void uml_do_state_init(int event)
{
	_uml_do_meta_state_init_or_resume(event);
}

static void uml_do_state_resume(int event)
{
	_uml_do_meta_state_init_or_resume(event);
}

static void uml_do_state_unplugged(int event)
{
	switch (event) {
		case EvVbusHigh:
			uml_enter_state_wait_for_usb_reset();

			break;

		default:
			uml_handle_unexpected_event(event);
	}
}

static void uml_do_state_wait_for_usb_reset(int event)
{
	switch (event) {
		case EvUsbReset:
			uml_enter_state_usb_host_plugged();
			break;

		case EvTimeoutUsbReset:
			if (uml.pdata->is_aca_plugged())
				uml_enter_state_aca_plugged();
			else
				uml_enter_state_wall_charger_plugged();

			break;

		case EvVbusLow:
			uml_enter_state_unplugged();
			break;

		default:
			uml_handle_unexpected_event(event);
	}
}

static void _uml_do_meta_state_host_aca_or_wall_charger_plugged(int event)
{
	switch (event) {
		case EvVbusLow:
			uml_enter_state_unplugged();
			break;

		default:
			uml_handle_unexpected_event(event);
	}
}

static void uml_do_state_usb_host_plugged(int event)
{
	_uml_do_meta_state_host_aca_or_wall_charger_plugged(event);
}

static void uml_do_state_aca_plugged(int event)
{
	_uml_do_meta_state_host_aca_or_wall_charger_plugged(event);
}

static void uml_do_state_wall_charger_plugged(int event)
{
	_uml_do_meta_state_host_aca_or_wall_charger_plugged(event);
}

static int uml_state_machine(void *data)
{
	/* delay the start of the state machine to give the power supply
	   and the USB driver time to initialize. in case we receive events
	   in the meantime they are stored in the event queue and we handle
	   them afterwards */
	ssleep(1);

	dev_dbg(uml.dev, "starting state machine\n");

	while (1) {
		int event;

		wait_event_interruptible(uml.stm.wq,
			   uml_event_pending() || kthread_should_stop());

		if (kthread_should_stop()) {
			dev_dbg(uml.dev, "exiting state machine\n");
			break;
		}

		event = uml_get_next_event();

		switch (uml.stm.state) {
			case StateUnplugged:
				uml_do_state_unplugged(event);
				break;

			case StateWaitForUsbReset:
				uml_do_state_wait_for_usb_reset(event);
				break;

			case StateUsbHostPlugged:
				uml_do_state_usb_host_plugged(event);
				break;

			case StateAcaPlugged:
				uml_do_state_aca_plugged(event);
				break;

			case StateWallChargerPlugged:
				uml_do_state_wall_charger_plugged(event);
				break;

			case StateResume:
				uml_do_state_resume(event);
				break;

			case StateInit:
				uml_do_state_init(event);
				break;

			default:
				dev_err(uml.dev, "reached undefined state %d\n", uml.stm.state);
		}
	}

	return 0;
}

static void uml_usb_reset_timeout(struct work_struct *work)
{
	uml_queue_event(EvTimeoutUsbReset);
}

void usb_mode_lite_handle_usb_reset(void)
{
	if ((uml.stm.state == StateWaitForUsbReset) &&
	    delayed_work_pending(&uml.stm.usb_reset_work)) {
		/* usually several USB resets are received, it's enough to
		   handle the first one */

		dev_dbg(uml.dev, "reveived USB reset\n");

		cancel_delayed_work(&uml.stm.usb_reset_work);

		uml_queue_event(EvUsbReset);
	}
}
EXPORT_SYMBOL(usb_mode_lite_handle_usb_reset);

static irqreturn_t uml_irq(int irq, void *data)
{
	if (uml_is_vbus_on())
		uml_queue_event(EvVbusHigh);
	else
		uml_queue_event(EvVbusLow);

	return IRQ_HANDLED;
}

static __devinit int uml_validate_platform_data(struct usb_mode_lite_platform_data *pdata)
{
	if (pdata == NULL) {
		dev_err(uml.dev, "no platform data provided\n");
		return -1;
	}

	if (pdata->set_charger_type == NULL) {
		dev_err(uml.dev, "no function provided for setting charger type\n");
		return -1;
	}

	if (pdata->switch_otg_mode == NULL) {
		dev_err(uml.dev, "no function provided for switching OTG mode\n");
		return -1;
	}

	if (pdata->is_aca_plugged == NULL) {
		dev_err(uml.dev, "no function for ACA detection provided\n");
		return -1;
	}

	return 0;
}

static void uml_free_events(struct list_head *queue)
{
	struct uml_event *event, *n;

	list_for_each_entry_safe(event, n, queue, node) {
		list_del(&event->node);
		kfree(event);
	}
}

static int __devinit uml_probe(struct platform_device *pdev)
{
	int rc;
	int vbus_irq;
	struct usb_mode_lite_platform_data *pdata = (struct usb_mode_lite_platform_data *)pdev->dev.platform_data;

	if (uml_validate_platform_data(pdata))
		return -1;

	uml.pdata = pdata;
	uml.dev = &pdev->dev;

	uml.stm.state = StateInit;

	init_waitqueue_head(&uml.stm.wq);
	INIT_DELAYED_WORK(&uml.stm.usb_reset_work, uml_usb_reset_timeout);

	spin_lock_init(&uml.stm.lock_events);
	INIT_LIST_HEAD(&uml.stm.event_queue);
	INIT_LIST_HEAD(&uml.stm.event_mem_pool);

	{
		int i;

		for (i = 0; i < EVENT_QUEUE_SIZE; i++) {
			struct uml_event *event = kmalloc(sizeof(*event), GFP_KERNEL);
			if (event == NULL) {
				dev_err(uml.dev, "unable to allocate memory pool for events\n");
				goto err_event_pool_alloc;
			}

			list_add(&event->node, &uml.stm.event_mem_pool);
		}
	}

	/* queue event for StateInit */
	if (uml_is_vbus_on())
		uml_queue_event(EvVbusHigh);
	else
		uml_queue_event(EvVbusLow);

	vbus_irq = gpio_to_irq(uml.pdata->gpio_vbus);
	if (vbus_irq == -EINVAL) {
		dev_err(&pdev->dev, "Failed to get IRQ for VBUS GPIO\n");
		goto err_gpio_to_irq;
	}

	rc = request_irq(vbus_irq, uml_irq, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "usb_mode_lite", NULL);
	if (rc) {
		dev_err(&pdev->dev, "Failed to request VBUS IRQ\n");
		goto err_request_irq;
	}

	uml.stm.thread = kthread_run(uml_state_machine, NULL, "usb_mode_lite");
	if (NULL == uml.stm.thread) {
		dev_err(uml.dev, "Failed to launch state machine thread\n");
		goto err_kthread;
	}

	dev_info(&pdev->dev, "Initialized S5P charger detection and USB mode switching module\n");

	return 0;

  err_kthread:
	free_irq(vbus_irq, NULL);

  err_request_irq:
  err_gpio_to_irq:
	uml_free_events(&uml.stm.event_mem_pool);

  err_event_pool_alloc:
	return -1;
}

static __devexit int uml_remove(struct platform_device *pdev)
{
	const int vbus_irq = gpio_to_irq(uml.pdata->gpio_vbus);

	free_irq(vbus_irq, NULL);

	kthread_stop(uml.stm.thread);

	uml_free_events(&uml.stm.event_mem_pool);
	uml_free_events(&uml.stm.event_queue);

	return 0;
}

#ifdef CONFIG_PM

static int uml_suspend(struct platform_device *pdev, pm_message_t state)
{
	disable_irq(gpio_to_irq(uml.pdata->gpio_vbus));

	cancel_delayed_work(&uml.stm.usb_reset_work);

	uml_flush_event_queue();

	return 0;
}

static int uml_resume(struct platform_device *pdev)
{
	uml.stm.state = StateResume;

	/* the interrupt will fire after enabling it, therefore we don't need
	   to check the VBUS level and queue an event */

	enable_irq(gpio_to_irq(uml.pdata->gpio_vbus));

	return 0;
}

#else
#define uml_suspend  NULL
#define uml_resume   NULL
#endif

static int uml_event_pending(void)
{
	return !list_empty(&uml.stm.event_queue);
}

static void uml_queue_event(event_t event)
{
	struct uml_event *ev = NULL;
	struct list_head *node;
	unsigned long flags;

	spin_lock_irqsave(&uml.stm.lock_events, flags);

	if (list_empty(&uml.stm.event_mem_pool)) {
		dev_warn(uml.dev, "events memory pool is empty!!!\n");
		goto err_no_mem;
	}

	/* use first free entry of memory pool */
	node = uml.stm.event_mem_pool.next;

	ev = list_entry(node, struct uml_event, node);
	ev->event = event;

	list_move_tail(node, &uml.stm.event_queue);

	wake_up(&uml.stm.wq);

  err_no_mem:
	spin_unlock_irqrestore(&uml.stm.lock_events, flags);
}

static event_t uml_get_next_event(void)
{
	event_t event = EvNone;
	unsigned long flags;

	spin_lock_irqsave(&uml.stm.lock_events, flags);

	if (!list_empty(&uml.stm.event_queue)) {
		struct list_head *node = uml.stm.event_queue.next;
		struct uml_event *ev = list_entry(node, struct uml_event, node);

		event = ev->event;

		/* put the event structure back in the memory pool */
		list_move(node, &uml.stm.event_mem_pool);
	} else {
		/* we should never be called with an empty event queue */
		dev_warn(uml.dev, "event queue is empty!!!");
	}

	spin_unlock_irqrestore(&uml.stm.lock_events, flags);

	return event;
}

static void uml_flush_event_queue(void)
{
	struct list_head *l, *n;
	unsigned long flags;

	spin_lock_irqsave(&uml.stm.lock_events, flags);

	list_for_each_safe(l, n, &uml.stm.event_queue) {
		list_move(l, &uml.stm.event_mem_pool);
	}

	spin_unlock_irqrestore(&uml.stm.lock_events, flags);
}

static struct platform_driver uml_driver = {
	.driver.name	= "usb_mode_lite",
	.driver.owner	= THIS_MODULE,
	.probe		= uml_probe,
	.remove		= __devexit_p(uml_remove),
	.suspend	= uml_suspend,
	.resume		= uml_resume,
};

static int __init uml_init(void)
{
	return platform_driver_register(&uml_driver);
}

static void __exit uml_exit(void)
{
	platform_driver_unregister(&uml_driver);
}

module_init(uml_init);
module_exit(uml_exit);

MODULE_DESCRIPTION("USB charger detection and OTG mode switching driver");
MODULE_AUTHOR("Matthias Kaehlcke <matthias.kaehlcke@tomtom.com>");
MODULE_LICENSE("GPL");
