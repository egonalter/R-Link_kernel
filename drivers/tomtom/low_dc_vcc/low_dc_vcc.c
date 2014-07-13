/*
 * 
 *
 *
 *  Copyright (C) 2008 by TomTom International BV. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as 
 * published by the Free Software Foundation.
 *
 *
 *
 *
 */

#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/syscalls.h>
#include <linux/apm_bios.h>
#include <linux/apm-emulation.h>
#include <asm/atomic.h>
#include <plat/low-dc-vcc.h>

struct plat_data {
	struct detect_low_dc_platform_data *pd;
}driver_data;


static int triggered=0;

/* Setup gpio */
static int detect_low_dc_config_gpio(struct detect_low_dc_platform_data *pd)
{
	BUG_ON(!pd->config_gpio);
	
	return pd->config_gpio();
}

static int show_gpio_dc_low(struct detect_low_dc_platform_data *pd)
{
	return pd->get_gpio_value();
}

/* Userland sysfs interface */
static ssize_t show_gpio(struct device *dev,struct device_attribute *attr,char *buf) 
{
	snprintf (buf, PAGE_SIZE, "%d\n", show_gpio_dc_low(driver_data.pd));

	return strlen(buf)+1;
}
DEVICE_ATTR(gpio_dc_low, S_IRUGO, show_gpio, NULL);

/* Set up main userland sysfs interface */
int inline setup_main_interface(struct platform_device *pdev)
{
	if (device_create_file(&pdev->dev, &dev_attr_gpio_dc_low)) {
		dev_err(&pdev->dev,
			"Could not create gpio_dc_low sysfs attribute\n");
		return -1;
	}
	return 0;
}

void inline destroy_main_interface(struct platform_device *pdev)
{
	device_remove_file(&pdev->dev, &dev_attr_gpio_dc_low);
}

static irqreturn_t irq_handler(int irq, void *dev_id)
{
	if (triggered)
		return IRQ_HANDLED;

	printk("LOW DC VCC detected! Suspending unit..\n");
	apm_queue_event(APM_USER_SUSPEND);
	triggered=1;

	return IRQ_HANDLED;
}

static int detector_probe(struct platform_device *pdev)
{
	struct resource *irqres[1];
	int res = 0;
	struct detect_low_dc_platform_data *pd = pdev->dev.platform_data;

	/* Config pin */
	detect_low_dc_config_gpio(pd);

	/* Add this platform device to our global driver data struct */
	driver_data.pd = pdev->dev.platform_data;

	/* Get IRQ */
	irqres[0] = platform_get_resource(pdev, IORESOURCE_IRQ,0);
	
	if (!irqres[0]) {
		dev_err(&pdev->dev, "Missing IRQ resources!\n");
		return -EINVAL;
	}

	/* Register the detector gpio as interupt source */
	if ((res=request_irq(irqres[0]->start, irq_handler, IRQF_TRIGGER_FALLING, "low_dc_vcc",
		pdev)))
	{
		dev_err(&pdev->dev,
			"Can't register LOW_DC_VCC interrupt %d!\n",
			irqres[0]->start);
		return res;
	}

	/* Set up userland interface */
	setup_main_interface(pdev);

	return 0;
}

static int detector_remove(struct platform_device *pdev)
{
	struct resource *irqres;

	/* Freeing interrupt */
	if ((irqres = platform_get_resource(pdev, IORESOURCE_IRQ, 0))) {
		free_irq(irqres->start, pdev);
	}

	/* Remove userland interface */
	destroy_main_interface(pdev);

	return 0;
}

static int detector_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct detect_low_dc_platform_data *pd = pdev->dev.platform_data;

	if (state.event == PM_EVENT_SUSPEND) {
		struct resource *irqres;
		
		irqres = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
		disable_irq(irqres->start);

		if (pd->suspend)
			pd->suspend();
	}
	return 0;
}

static int detector_resume(struct platform_device *pdev)
{
	struct resource *irqres;
	struct detect_low_dc_platform_data *pd = pdev->dev.platform_data;

	if (pd->resume)
		pd->resume();

	/* Enable interrupt */
	irqres = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
	enable_irq(irqres->start);
	triggered=0;

	return 0;
}

static struct platform_driver detect_low_dc_driver = {
	.probe = detector_probe,
	.remove = detector_remove,
	.suspend = detector_suspend,
	.resume = detector_resume,
	.driver = {
		.owner = THIS_MODULE,
		.name = "low_dc_vcc",
	},
};

static int __init detector_init_module(void)
{
	int	ret;

	ret = platform_driver_register(&detect_low_dc_driver);
	if (ret) {
		printk(" Unable to register LOW_DC_VCC driver (%d)\n", ret);
		return ret;
	}

	return 0;
}

static void __exit detector_exit_module(void)
{
	platform_driver_unregister(&detect_low_dc_driver);
}

module_init(detector_init_module);
module_exit(detector_exit_module);

MODULE_DESCRIPTION("TomTom LOW_DC_VCC detector");
MODULE_LICENSE("GPL");

