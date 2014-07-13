/*
 * This driver provides a generic PPS driver that builds upon LinuxPPS.
 * For more information on LinuxPPS, please see Documentation/pps/pps.txt.
 *
 * Author: Rein Achten <rein.achten@tomtom.com>
 *  * (C) Copyright 2010 TomTom International BV.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation version 2.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/err.h>
#include <linux/ktime.h>
#include <linux/pps_kernel.h>

#define DRIVER_DESC_LONG	"TomTom - PPS driver, (C) 2010 TomTom BV "
#define PFX 			"tomtom-pps: "
#define PPS_IRQ_RESOURCE	0

//#define DEBUG_PPS

/* PPS driver data, containing source id and irq */
struct pps_driver_data {
	int source;
	int irq;
};

static irqreturn_t pps_irq_handler(int irq, void *dev_id)
{
	struct platform_device *pdev = (struct platform_device *) dev_id;
	struct pps_driver_data *pps_priv = platform_get_drvdata(pdev);
	struct timespec ts;
	struct pps_ktime pps_ts;

	/* acquire timestamp */
	ktime_get_ts(&ts);

#ifdef DEBUG_PPS
	dev_info(&pdev->dev, "[ source = %d ] PPS event at %llu\n", pps_priv->source, get_jiffies_64());
#endif

	/* and translate it to PPS time data struct */
	pps_ts.sec = ts.tv_sec;
	pps_ts.nsec = ts.tv_nsec;

	pps_event(pps_priv->source, &pps_ts, PPS_CAPTUREASSERT, NULL);

	return IRQ_HANDLED;
}

static struct pps_source_info pps_info = {
	.name		= "tomtom-pps",
	.path		= "",
	.mode		= PPS_CAPTUREASSERT |
			  PPS_OFFSETASSERT |
			  PPS_CANWAIT |
			  PPS_TSFMT_TSPEC,
	.echo 		= NULL,
	.owner		= THIS_MODULE,
};

static int pps_probe(struct platform_device *pdev)
{
	int res;
	struct resource *irq_res		= NULL;
	struct pps_driver_data *pps_priv	= NULL;

	pps_priv = kmalloc(sizeof(struct pps_driver_data), GFP_KERNEL);
	if (!pps_priv) 
	{
		dev_err(&pdev->dev, "error allocating memory for pps_driver_data\n");
		res = -ENOMEM;
		goto err_allocate_mem;
	}
	memset(pps_priv, 0, sizeof(*pps_priv));

	if ((res = pps_register_source(&pps_info, PPS_CAPTUREASSERT | PPS_OFFSETASSERT)) < 0) {
		dev_err(&pdev->dev, "cannot register pps source\n");
		goto err_register_source;
	}

	/* store PPS source */
	pps_priv->source = res;

	if ((irq_res = platform_get_resource(pdev, IORESOURCE_IRQ, PPS_IRQ_RESOURCE)) == NULL) {
		dev_err(&pdev->dev, "failed to get IRQ resource\n");
		res = -EINVAL;
		goto err_get_resource;
	}

	/* store irq and driver data */
	pps_priv->irq = irq_res->start;
	platform_set_drvdata(pdev, pps_priv);

	if ((res = request_irq(irq_res->start, pps_irq_handler, IRQF_TRIGGER_RISING, irq_res->name, pdev)))
	{
		dev_err(&pdev->dev, "unable to register pps interrupt %d\n", irq_res->start);
		free_irq(irq_res->start, NULL);
		goto err_request_irq;
	}

	dev_info(&pdev->dev, "successfully registered pps source at %d.\n", pps_priv->source);

	return  0;

err_request_irq:
	platform_set_drvdata(pdev, NULL);
err_get_resource:
	pps_unregister_source(pps_priv->source);
err_register_source:
	kfree(pps_priv);
err_allocate_mem:
	return res;
}

static int pps_remove(struct platform_device *pdev)
{
	struct pps_driver_data *pps_priv = platform_get_drvdata(pdev);

	dev_info(&pdev->dev, "unregistering pps source %d.\n", pps_priv->source);

	free_irq(pps_priv->irq, pdev);
	platform_set_drvdata(pdev, NULL);
	pps_unregister_source(pps_priv->source);
	kfree(pps_priv);

	return 0;
}

static int pps_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct pps_driver_data *pps_priv = platform_get_drvdata(pdev);
	disable_irq(pps_priv->irq);
	return 0;
}

static int pps_resume(struct platform_device *pdev)
{
	struct pps_driver_data *pps_priv = platform_get_drvdata(pdev);
	enable_irq(pps_priv->irq);
	return 0;
}

static struct platform_driver pps_driver =
{
	.driver  = {
		.owner	= THIS_MODULE,
		.name	= "tomtom-pps",
	},
	.probe		= pps_probe,
	.remove		= pps_remove,
	.suspend	= pps_suspend,
	.resume		= pps_resume,
};

static int __init pps_init(void)
{
	int ret = platform_driver_register(&pps_driver);
	if (ret)
		pr_err(PFX "unable to register driver.\n");
	else
		pr_info(PFX "successfully registered driver.\n");
	return ret;
}

static void __exit pps_exit(void)
{
	platform_driver_unregister(&pps_driver);
	pr_info(PFX "unregistered driver.\n");
	return;
}

module_init(pps_init);
module_exit(pps_exit);

MODULE_DESCRIPTION(DRIVER_DESC_LONG);
MODULE_AUTHOR("Rein Achten <rein.achten@tomtom.com>");
MODULE_LICENSE("GPL");
