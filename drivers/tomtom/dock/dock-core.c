/*
 * Generic dock driver for TomTom active docks
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

#include <linux/platform_device.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <asm/atomic.h>
#include <plat/dock.h>

#define PFX "Dock:"
#define DOCK_DEBUG
#ifdef DOCK_DEBUG
#define DBG(fmt, args...) printk(KERN_DEBUG PFX fmt, ## args)
#define DEV_DBG(dev, fmt, args...) dev_notice(dev, fmt, ## args)
#else
#define DBG(fmt, args...)
#define DEV_DBG(dev, fmt, args...)
#endif
#define DBG_PRINT_FUNC() DBG(" Function %s\n", __func__);

static int dock_get_line_sense0(struct dock_platform_data *pd);
static int dock_get_line_sense1(struct dock_platform_data *pd);
static int dock_get_line_pwr_det(struct dock_platform_data *pd);

static struct list_head dock_type_table = LIST_HEAD_INIT(dock_type_table);

static int dock_type_invalid_probe_id(void *arg)
{
	return -1;
}

static struct dock_type dock_type_invalid = {
	.dock_name	= "DOCK_INVALID",
	.detection_id	= -1,	/* NEVER matches any dock */
	.connect	= NULL,
	.disconnect	= NULL,
	.reset_devices	= NULL,
	.get_id		= NULL,
	.probe_id	= dock_type_invalid_probe_id,
};

static int dock_type_none_probe_id(void *arg)
{
	return 1;
}

static struct dock_type dock_type_none = {
	.dock_name	= "DOCK_NONE",
	.detection_id	=  DETID_NOPWR(1,1) | DETID_PWR(1,1),
	.connect	= NULL,
	.disconnect	= NULL,
	.reset_devices	= NULL,
	.get_id		= NULL,
	.probe_id	= dock_type_none_probe_id,
};

/* Global driver data - dock driver is not reentrant, driver supports only a
 * single dock at a given time
 */
struct dock_state dock_data;

/* A few sysfs attributes for debugging / testing */
static ssize_t show_gpio_sense(char *buf, int num)
{
	switch(num) {
		case 0:
			sprintf(buf, "%d\n", dock_get_line_sense0(dock_data.pd));
			break;
		case 1:
			sprintf(buf, "%d\n", dock_get_line_sense1(dock_data.pd));
			break;
		default:
			return -1;
			break;
	}
	return strnlen(buf, PAGE_SIZE);
}

/* Print *physical* state of gpio sense pin 0 (inversion not accounted for) */
static ssize_t show_gpio_sense0(
	struct device *dev,
	struct device_attribute *attr,
	char *buf
) {
	return show_gpio_sense(buf, 0);
}
DEVICE_ATTR(gpio_sense0, S_IRUGO, show_gpio_sense0, NULL);

/* Print *physical* state of gpio sense pin 1 (inversion not accounted for) */
static ssize_t show_gpio_sense1(
	struct device *dev,
	struct device_attribute *attr,
	char *buf
) {
	return show_gpio_sense(buf, 1);
}
DEVICE_ATTR(gpio_sense1, S_IRUGO, show_gpio_sense1, NULL);

/* Print state of gpio pwr detect */
static ssize_t show_gpio_pwr_det(
	struct device *dev,
	struct device_attribute *attr,
	char *buf
) {
	sprintf(buf, "%d\n", dock_get_line_pwr_det(dock_data.pd));
	return strnlen(buf, PAGE_SIZE);
}
DEVICE_ATTR(gpio_pwr_det, S_IRUGO, show_gpio_pwr_det, NULL);

/* Force a particular type of dock to be detected */
static ssize_t store_forced_dock(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count
);
DEVICE_ATTR(force_dock, S_IWUGO, NULL, store_forced_dock);

/* Start of Main userland sysfs interface */
static ssize_t show_dock_state(
	struct device *dev,
	struct device_attribute *attr,
	char *buf
) {
	sprintf(buf, "%s\n",
		dock_data.current_dockp->dock_name);
	return strnlen(buf, PAGE_SIZE);
}
DEVICE_ATTR(dock_state, S_IRUGO, show_dock_state, NULL);
/* End of main userland sysfs interface */

/* Read the dock detection ID from GPIO pins on the dock */
static void dock_config_lines(struct dock_platform_data *pd)
{
	printk(KERN_DEBUG "Configuring dock lines\n");

	BUG_ON(!pd->dock_line_config);
	pd->dock_line_config(pd, RDS_nRST, (pd->reset_inverted ? DOCK_LINE_DEASSERT : DOCK_LINE_ASSERT), DOCK_LINE_OUTPUT);
	pd->dock_line_config(pd, DC_OUT, DOCK_LINE_ASSERT, DOCK_LINE_OUTPUT);		/* A.k.a ACCESSORY_PWR_EN */
	pd->dock_line_config(pd, DOCK_PWR_DET, DOCK_LINE_ASSERT, DOCK_LINE_OUTPUT);
	pd->dock_line_config(pd, DOCK_I2C_PWR, DOCK_LINE_ASSERT, DOCK_LINE_OUTPUT);

	pd->dock_line_config(pd, DOCK_DET0, 0, DOCK_LINE_INPUT);
	pd->dock_line_config(pd, DOCK_DET1, 0, DOCK_LINE_INPUT);
	pd->dock_line_config(pd, FM_INT, 0, DOCK_LINE_INPUT);
}

static void dock_set_line_reset(struct dock_platform_data *pd, int value)
{
	pd->dock_line_set(pd, RDS_nRST, (pd->reset_inverted ? !value : value));
}

static void dock_set_line_dc_out(struct dock_platform_data *pd, int value)
{
	if (pd->dock_line_set)
		pd->dock_line_set(pd, DC_OUT, value);
	else {
		printk("%s: dock_line_set is NOT set\n", __func__);
	}
}

static void dock_set_line_pwr_det(struct dock_platform_data *pd, int value)
{
	pd->dock_line_set(pd, DOCK_PWR_DET, value);
}

static int dock_get_line_pwr_det(struct dock_platform_data *pd)
{
	return pd->dock_line_get(pd, DOCK_PWR_DET);
}

static int dock_get_line_sense0(struct dock_platform_data *pd)
{
	return pd->dock_line_get(pd, DOCK_DET0);
}

static int dock_get_line_sense1(struct dock_platform_data *pd)
{
	return pd->dock_line_get(pd, DOCK_DET1);
}

static int dock_get_detid(void)
{
	int detid;
	struct dock_platform_data *pd = dock_data.pd;

	BUG_ON(pd == NULL);

	printk(KERN_NOTICE PFX " Resetting dock\n");
	dock_set_line_reset(pd, DOCK_LINE_ASSERT);
	mdelay(100);

	/* Check the status of our pin with power disabled */
	detid = DETID_NOPWR(dock_get_line_sense0(pd), dock_get_line_sense1(pd));

	if (detid != 0) {
		/* Check the status of our pin with power enabled */
		printk(KERN_NOTICE PFX " Enable dock power dock\n");
		dock_set_line_dc_out(pd, DOCK_LINE_ASSERT);
		msleep(100); /* Let external signal stabilize */
		detid |= DETID_PWR(dock_get_line_sense0(pd), dock_get_line_sense1(pd));

		/* Deassert dock reset */
		printk(KERN_NOTICE PFX " Stop resetting dock\n");
		dock_set_line_reset(pd, DOCK_LINE_DEASSERT);
	}
	mdelay(100);

	printk(KERN_INFO PFX "Detected dock with detid [%d]\n", detid);
	return detid;
}

/* Take actions when a dock is detected. Also called when a dock is forced. */
static int _dock_state_change_handler(struct dock_type *newdockp)
{
	dock_t			currdock;
	struct dock_type	*currdockp;

	currdockp	= dock_data.current_dockp;
	currdock	= atomic_read(&dock_data.current_dock);		/* Leave it as an atomic for now */

	if (newdockp == &dock_type_invalid) {
		printk(KERN_NOTICE PFX " Unrecognized dock inserted.\n");
	}

	if (currdockp != newdockp) {
		printk(KERN_INFO PFX " Dock change detected: %s [%d] => "
			"%s [%d]\n", currdockp->dock_name, currdockp->detection_id,
			newdockp->dock_name, newdockp->detection_id);

		/* Deinitialize the old dock */
		if (currdockp->disconnect && currdockp->disconnect(&dock_data)) {
			printk(KERN_ERR PFX " Disconnect of current dock [%d] "
				"unsuccessful\n", currdock);
		}

		/* Disable dock power if we don't have a valid dock */
		if (newdockp == &dock_type_none || newdockp == &dock_type_invalid) {
			printk(KERN_INFO PFX " Disabling dock power and activate reset\n");
			dock_set_line_reset(dock_data.pd, DOCK_LINE_ASSERT);
			dock_set_line_dc_out(dock_data.pd, DOCK_LINE_DEASSERT);
		}

		/* Set currentdockp right now since:
		 * 1) the choice has been made and,
		 * 2) the dock-specific part can relies on that pointer being up to date.
		 */
		dock_data.current_dockp	= newdockp;

		/* Initialize the new dock */
		if (newdockp->connect && newdockp->connect(&dock_data)) {
			printk(KERN_ERR PFX " Connect of new dock [%d] "
				"unsuccessful\n", newdockp->detection_id);
		} else if (newdockp->reset_devices) {
			/* Reset devices embedded in the dock, if necessary */
			printk(KERN_INFO PFX "Resetting devices on the dock\n");
			if (newdockp->reset_devices(&dock_data)) {
				printk(KERN_ERR PFX " Error resetting dock devices.\n");
			}
		}

		/* Notify userland that a dock has been plugged / unplugged.
		 * This also happens when a dock is forced */
		atomic_set(&dock_data.current_dock, newdockp->detection_id);
		kobject_uevent(&dock_data.pdev->dev.kobj, KOBJ_CHANGE);		/* Fix: Notify of the change AFTER it has occured! */
	}
	else {
		printk(KERN_NOTICE PFX " No change.\n");
	}

	return 0;

}

static struct dock_type *_dock_lookup(int detid)
{
	int			prio = 0, t;
	struct dock_type	*best_type = &dock_type_invalid;
	struct list_head	*entry;

	list_for_each(entry, &dock_type_table) {
		struct dock_type *dtp = list_entry(entry, struct dock_type, dock_type_list);

		if (dtp->detection_id == detid) {
			/* First of all, check if it is supported by the platform */
			if (dock_data.pd->present && !dock_data.pd->present(dtp->dock_name, detid))
				continue;

			t = dtp->probe_id((void *) detid);
			if (t > prio) {
				printk(KERN_NOTICE PFX "Priority is %d > %d\n", t, prio);
				prio		= t;
				best_type	= dtp;
			}
		}
	}

	printk(KERN_NOTICE PFX "Found new dock: %s\n", best_type->dock_name);

	return best_type;
}

/* Figure out which dock has been plugged and take appropriate action */
static int _dock_check_state(void)
{
	int			id;
	struct dock_type	*dtp;

	if (!dock_data.started)
		return 0;

	id = dock_get_detid();
	dtp = _dock_lookup(id);

	return _dock_state_change_handler(dtp);
}

static int dock_check_state(void)
{
	int ret;

	mutex_lock(&dock_data.lock);

	ret = _dock_check_state();

	mutex_unlock(&dock_data.lock);

	return ret;
}

/* Delayed work from interrupt handler, performed at end of debounce period */
static void dock_irq_debounce_handler(struct work_struct* _notused)
{
	dock_check_state();
}

/* Force the driver to think a particular dock is inserted. To force a dock
 * removal, use e.g. dock_force_state(DOCK_NONE)
 */
static int _dock_force_state(struct dock_type *dtp)
{
	BUG_ON(!dtp);

	printk(KERN_INFO PFX " Forcing dock state to %s [%d] ...\n",
		(dtp->dock_name != NULL) ? dtp->dock_name : "?", dtp->detection_id);

	return _dock_state_change_handler(dtp);
}

static int dock_force_state(struct dock_type *dtp)
{
	int ret;

	mutex_lock(&dock_data.lock);
	ret = _dock_force_state(dtp);
	mutex_unlock(&dock_data.lock);

	return ret;
}

static irqreturn_t dock_irq_handler(int irq, void *dev_id)
{
	/* Handle interrupt generated when dock plugged / unplugged.
	 * A workqueue can be used to debounce the dock detection signals - if
	 * another interrupt occurs during the debounce period, no new work will
	 * be scheduled here (since it is still pending during the debounce
	 * period) and the work will only be done once.
	 */
	schedule_delayed_work(&dock_data.work, HZ/10);

	return IRQ_HANDLED;
}

/* Set up main userland sysfs interface */
int setup_main_interface(struct platform_device *pdev)
{
	if (device_create_file(&pdev->dev, &dev_attr_dock_state)) {
		dev_err(&pdev->dev,
			"Could not create dock_state sysfs attribute\n");
		return -1;
	}
	return 0;
}

void destroy_main_interface(struct platform_device *pdev)
{
	device_remove_file(&pdev->dev, &dev_attr_dock_state);
}

#ifdef DOCK_DEBUG
/* Sysfs attributes for test / debugging - if registration fails, just carry on
 */
void setup_test_interface(struct platform_device *pdev)
{
	if (device_create_file(&pdev->dev, &dev_attr_gpio_sense0)) {
		dev_err(&pdev->dev,
			"Could not create gpio_sense0 sysfs attribute\n");
	}
	if (device_create_file(&pdev->dev, &dev_attr_gpio_sense1)) {
		dev_err(&pdev->dev,
			"Could not create gpio_sense0 sysfs attribute\n");
	}
	if (device_create_file(&pdev->dev, &dev_attr_gpio_pwr_det)) {
		dev_err(&pdev->dev,
			"Could not create gpio_pwr_det sysfs attribute\n");
	}
	if (device_create_file(&pdev->dev, &dev_attr_force_dock)) {
		dev_err(&pdev->dev,
			"Could not create force_dock sysfs attribute\n");
	}
}

void destroy_test_interface(struct platform_device *pdev)
{
	device_remove_file(&pdev->dev, &dev_attr_gpio_sense0);
	device_remove_file(&pdev->dev, &dev_attr_gpio_sense1);
	device_remove_file(&pdev->dev, &dev_attr_gpio_pwr_det);
	device_remove_file(&pdev->dev, &dev_attr_force_dock);
}
#else
void setup_test_interface(struct platform_device *pdev) {}
void destroy_test_interface(struct platform_device *pdev) {}
#endif

/* Get the irq number of the I2C device irq pin on the dock, this is used by the
 * dock function driver. It is up to the I2C device itself to register the IRQ
 */
int dock_get_i2c_irq(void)
{
	struct resource *irqres;

	irqres = platform_get_resource(dock_data.pdev, IORESOURCE_IRQ, DOCK_IRQ_I2C);
	if (!irqres) {
		printk(KERN_ERR PFX  " Missing IRQ resources!\n");
		return -EINVAL;
	}
	return irqres->start;
}

/* Get the index number of the I2C adapter on the dock, this is used by the
 * dock function driver. 
 */
int dock_get_i2c_adapter_index(void)
{
	/* TODO: implement for non-sirf socs   */
	return dock_data.pd->i2cbusnumber;
}
EXPORT_SYMBOL(dock_get_i2c_adapter_index);



void dock_type_register(struct dock_type *dtp)
{
	mutex_lock(&dock_data.lock);

	BUG_ON(!dtp->probe_id);

	list_add_tail(&dtp->dock_type_list, &dock_type_table);
	printk(KERN_INFO PFX "Registered new dock %s\n", dtp->dock_name);

	// re-check; perhaps the correct dock is now there
	_dock_check_state();

	mutex_unlock(&dock_data.lock);

}
EXPORT_SYMBOL(dock_type_register);

void dock_type_unregister(struct dock_type *dtp)
{
	struct list_head	*entry;

	mutex_lock(&dock_data.lock);

	list_for_each(entry, &dock_type_table) {
		struct dock_type *list_dtp = list_entry(entry, struct dock_type, dock_type_list);

		if (list_dtp == dtp) {
			list_del_init(&dtp->dock_type_list);

			/* re-check; if the dock is in use it will be released */
			_dock_check_state();

			break;
		}
	}

	mutex_lock(&dock_data.lock);

	printk(KERN_INFO PFX "Unregistered dock %s\n", dtp->dock_name);
}
EXPORT_SYMBOL(dock_type_unregister);



static int dock_probe(struct platform_device *pdev)
{
	struct resource *irqres[2];
	int res = 0;
	struct dock_platform_data *pd = pdev->dev.platform_data;

	// can't have any more unchecked registrations
	dock_data.started = 1;

	pd->dock_device	= &pdev->dev;

	/* Put platform data in global driver data - we assume it is only
	 * possible to plug one dock at a time
	 */
	dock_data.pd = pdev->dev.platform_data;
	BUG_ON(pd == NULL);

	/* Add this platform device to our global driver data struct */
	dock_data.pdev = pdev;

	/* Manage inverted pins */
#ifdef CONFIG_MACH_TORINOS
	pd->reset_inverted	= 0;
#else
	pd->reset_inverted	= 1;
#endif

	if (pd->dock_machine_init != NULL && pd->dock_machine_init(pd)) {
		return -EINVAL;
	}

	dock_config_lines(pd);

	INIT_DELAYED_WORK(&dock_data.work, dock_irq_debounce_handler);

	/* Disable dock power and activate reset */
	dock_set_line_reset(pd, DOCK_LINE_ASSERT);
	dock_set_line_dc_out(pd, DOCK_LINE_DEASSERT);

	/* Set interrupt pin to input */
#if 0
	/* TODO */
	gpio_direction_input(&pd->gpio_i2c_int);
#endif

	/* Enable detection pullups */
	dock_set_line_pwr_det(pd, DOCK_LINE_ASSERT);	/* This should be moved to machine_init or configure */

	/* Get relevant IRQs. We use separate resource entries for each IRQ
	 * line in case they are not consecutive.
	 */
	irqres[0] = platform_get_resource(pdev, IORESOURCE_IRQ, DOCK_IRQ_SENSE0);
	irqres[1] = platform_get_resource(pdev, IORESOURCE_IRQ, DOCK_IRQ_SENSE1);
	if (!irqres[0] || !irqres[1]) {
		dev_err(&pdev->dev, "Missing IRQ resources!\n");
		return -EINVAL;
	}

	/* Register the dock detection ID pins as interrupts */
	if ((res=request_irq(irqres[0]->start, dock_irq_handler,
		IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "active-dock-irq",
		pdev)))
	{
		dev_err(&pdev->dev,
			"Can't register gpio_sense[0] interrupt %d!\n",
			irqres[0]->start);
		return res;
	}
	if ((res=request_irq(irqres[1]->start, dock_irq_handler,
		IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "active-dock-irq",
		pdev)))
	{
		dev_err(&pdev->dev,
			"Can't register gpio_sense[1] interrupt %d!\n",
			irqres[1]->start);
		free_irq(irqres[0]->start, pdev);
		return res;
	}

	/* Get and update dock state */
	dev_info(&pdev->dev, "Getting dock initial state\n");
	atomic_set(&dock_data.current_dock, DOCK_NONE);
	dock_check_state();

	/* Set up userland interface */
	setup_main_interface(pdev);
	setup_test_interface(pdev);

	return 0;
}

static int dock_remove(struct platform_device *pdev)
{
	struct resource *irqres;
	struct dock_platform_data *pd = pdev->dev.platform_data;

	/* Act as if the dock has been unplugged on module exit */
	dock_force_state(&dock_type_none);

	/* Free the interrupts */
	if ((irqres = platform_get_resource(pdev, IORESOURCE_IRQ, 0))) {
		free_irq(irqres->start, pdev);
	}
	if ((irqres = platform_get_resource(pdev, IORESOURCE_IRQ, 1))) {
		free_irq(irqres->start, pdev);
	}

	/* Remove userland interface */
	destroy_main_interface(pdev);
	destroy_test_interface(pdev);

	/* Disable the dock power line, leave pullups as they were */
	dock_set_line_dc_out(pd, DOCK_LINE_DEASSERT);

	if (pd->dock_machine_release != NULL) {
		pd->dock_machine_release(pd);
	}

	return 0;
}

static int dock_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct dock_platform_data *pd = pdev->dev.platform_data;
	DBG_PRINT_FUNC();
	if (state.event == PM_EVENT_SUSPEND) {
		struct resource *irqres;
		
		/* Seems safest to disconnect the dock on suspend. It is unclear
		 * if this will cause problems if the dock devices have already
		 * been suspended */
		dock_force_state(&dock_type_invalid);
		/* Avoid spurious interrupts */
		irqres = platform_get_resource(pdev, IORESOURCE_IRQ, DOCK_IRQ_SENSE0);
		disable_irq(irqres->start);
		irqres = platform_get_resource(pdev, IORESOURCE_IRQ, DOCK_IRQ_SENSE1);
		disable_irq(irqres->start);

		dock_set_line_pwr_det(pd, DOCK_LINE_DEASSERT);

		if (pd->suspend)
			pd->suspend();
	}
	return 0;
}

static int dock_resume(struct platform_device *pdev)
{
	struct resource *irqres;
	struct dock_platform_data *pd = pdev->dev.platform_data;
	DBG_PRINT_FUNC();

	dock_set_line_pwr_det(pd, DOCK_LINE_ASSERT);

	if (pd->resume)
		pd->resume();

	/* Treat the dock as if it has just been plugged in when we resume */
	dock_check_state();

	/* Re-enable interrupts */
	irqres = platform_get_resource(pdev, IORESOURCE_IRQ, DOCK_IRQ_SENSE0);
	enable_irq(irqres->start);
	irqres = platform_get_resource(pdev, IORESOURCE_IRQ, DOCK_IRQ_SENSE1);
	enable_irq(irqres->start);

	return 0;
}

/* Force a particular type of dock to be detected */
static ssize_t store_forced_dock(
	struct device *dev,
	struct device_attribute *attr,
	const char *buf,
	size_t count
) {
	uint newdock_detid = 0;
	struct dock_type *newdockp;

	newdock_detid = simple_strtoul(buf, NULL, 16);

	mutex_lock(&dock_data.lock);

	printk(KERN_INFO PFX " Forcing dock detection ID to [%#x]\n", newdock_detid);
	newdockp = _dock_lookup(newdock_detid);
	_dock_force_state(newdockp);

	mutex_unlock(&dock_data.lock);

	return strnlen(buf, PAGE_SIZE);
}

static struct platform_driver dock_driver = {
	.probe = dock_probe,
	.remove = dock_remove,
	.suspend = dock_suspend,
	.resume = dock_resume,
	.driver = {
		.owner = THIS_MODULE,
		.name = "dock",
	},
};

static int __init dock_init_module(void)
{
	int	ret;

	mutex_init(&dock_data.lock);

	dock_type_register(&dock_type_invalid);
	dock_type_register(&dock_type_none);
	dock_windscr_rds_init();
	dock_windscr_nords_init();

	dock_data.current_dockp = &dock_type_none;

	ret = platform_driver_register(&dock_driver);
	if (ret) {
		printk(KERN_ERR PFX " Unable to register driver (%d)\n", ret);
		return ret;
	}

	return 0;
}

static void __exit dock_exit_module(void)
{
	cancel_delayed_work(&dock_data.work);
	platform_driver_unregister(&dock_driver);
}

module_init(dock_init_module);
module_exit(dock_exit_module);

MODULE_DESCRIPTION("TomTom Active Dock");
MODULE_AUTHOR(
	"Ithamar R. Adema (ithamar.adema@tomtom.com), "
	"Martin Jackson (martin.jackson@tomtom.com), "
	"Jeroen Taverne (jeroen.taverne@tomtom.com), "
	"Niels Langendorff (niels.langendorff@tomtom.com), "
	"Guillaume Ballet (guillaume.ballet@tomtom.com)"
);
MODULE_LICENSE("GPL");

