/*
 * can_micro.c
 *
 * Implementation of the CAN-microprocessor driver.
 *
 * Copyright (C) 2007, 2010 TomTom BV <http://www.tomtom.com/>
 * Author: Peter Wouters <peter.wouters@tomtom.com>
 *         Martin Jackson <martin.jackson@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/platform_device.h>
#include <linux/can_micro.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/signal.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/gpio.h>
#include <linux/fs.h>
#include <asm/uaccess.h>

static int micro_major = -1;
static struct class *micro_class;
module_param(micro_major, int, 0600);

MODULE_PARM_DESC(micro_major, "Major device number to use, automatically assigned if not specified");

static void micro_gpio_set_optional(int p, int v)
{
	if (p != -1)
		gpio_set_value(p, v);
}

static int micro_gpio_get_optional(int p) {
	return (p != -1) && gpio_get_value(p);
}

#define micro_gpio_set_mandatory(p, v) gpio_set_value((p), (v))
#define micro_gpio_get_mandatory(p) gpio_get_value((p))

#define USING_IRQ(x) (x >= 0)

static struct {
	dev_t firstdev; /* First char device of driver */
	struct can_micro_dev *devs[MICRO_MAX_DEVS];
	int single_dev; /* If a platform device has an index of -1, it's the only can micro */
} micro_driverdata;

#define DEVS_INDEX(pdev_ptr) (pdev_ptr->id == -1 ? 0 : pdev_ptr->id - MICRO_MINOR_OFFSET)

/* Caller should hold the can_micro_dev->lock, not allowed to sleep */
static void __micro_activate(struct can_micro_dev *micro)
{
	if (USING_IRQ(micro->irq))
		enable_irq(micro->irq);
}

/* Caller should hold the can_micro_dev->lock, not allowed to sleep */
static void __micro_deactivate(struct can_micro_dev *micro)
{
	if (USING_IRQ(micro->irq))
		disable_irq(micro->irq);
}

static void micro_activate(struct can_micro_dev *micro)
{
	spin_lock(&micro->lock);
	if (micro->count++ == 0)
		__micro_activate(micro);
	spin_unlock(&micro->lock);
}

static void micro_deactivate(struct can_micro_dev *micro)
{
	spin_lock(&micro->lock);
	if (--micro->count == 0)
		__micro_deactivate(micro);
	spin_unlock(&micro->lock);
}

static int micro_ioctl(struct inode *inode, 
                       struct file *file, 
                       unsigned int cmd,
                       unsigned long arg)
{
	int ret = 0;
	struct can_micro_dev *micro = file->private_data;

	switch (cmd) {
	case IO_nCAN_RST:
		/* Virtual gpio pin is active high, ioctl interface is active
		 * low. Physical pin is generally active low.  Therefore, the
		 * value is inverted once here and once in the vgpio layer.
		 */
		micro_gpio_set_mandatory(micro->gpio_pin_can_resetout, !arg);
		break;

	case IO_CAN_SYNC:
		/* Pulse the SYNC pin */
		micro_gpio_set_mandatory(micro->gpio_pin_can_sync, 0);
		udelay(1);
		micro_gpio_set_mandatory(micro->gpio_pin_can_sync, 1);
		break;

	case IO_CAN_BT_MD:
		micro_gpio_set_mandatory(micro->gpio_pin_can_bt_mode, !!arg);
		break;

	case IO_PWR_5V_ON_MCU:
		micro_gpio_set_optional(micro->gpio_pin_pwr_5v_on_mcu, !!arg);
		break;

	case IO_MCU_nPWR_ON:
		micro_gpio_set_optional(micro->gpio_pin_mcu_pwr_on, !!arg);
		break;

	case IO_CAN_nRST:
		ret = put_user(micro_gpio_get_mandatory(micro->gpio_pin_can_resetin),
			       (int __user *)arg);
		break;

	case IO_CAN_RES:
		ret = put_user(micro_gpio_get_optional(micro->gpio_pin_can_reserv),
			       (int __user *)arg);
		break;

	default:
		printk(KERN_WARNING "CAN micro: Invalid ioctl command %u\n", cmd);
		ret = -EINVAL;
		break;
	}
	return ret;
}

irqreturn_t micro_interrupt(int irq, void *dev)
{
	struct can_micro_dev *micro = dev;
	kill_fasync(&micro->fasync_listeners, SIGIO, POLL_IN);

	return IRQ_HANDLED;
}

static int micro_fasync(int fd, struct file *file, int mode)
{
	struct can_micro_dev *micro = file->private_data;
	
	printk(KERN_CRIT "FASYNCFASYNC\n");
	return fasync_helper(fd, file, mode, &micro->fasync_listeners);
}

static int micro_open(struct inode *inode, struct file *file)
{
	struct can_micro_dev *micro;

	if (iminor(inode) < MICRO_MINOR_OFFSET ||
	    iminor(inode) >= MICRO_MINOR_OFFSET + MICRO_MAX_DEVS) {
		return -ENODEV;
	}

	micro = micro_driverdata.devs[iminor(inode) - MICRO_MINOR_OFFSET];
	if (micro == NULL) {
		return -ENODEV;
	}

	file->private_data = micro;
	micro_activate(micro);

	return 0;
}

static int micro_release(struct inode *inode, struct file *file)
{
	struct can_micro_dev *micro = file->private_data;

	micro_deactivate(micro);
	fasync_helper(-1, file, 0, &micro->fasync_listeners);

	file->private_data = NULL;

	return 0;
}

static struct file_operations micro_fops = {
	.owner   = THIS_MODULE,
	.ioctl   = micro_ioctl,
	.open    = micro_open,
	.release = micro_release,
	.fasync  = micro_fasync,
};

static int micro_probe(struct platform_device *pdev)
{
	struct device * micro_dev;
	int ret=0;

	struct can_micro_dev *micro = dev_get_platdata(&pdev->dev);
	if (micro == NULL) {
		dev_err(&pdev->dev, "No platform data");
		ret = -1;
		goto err;
	}

	if (pdev->id >= MICRO_MAX_DEVS) {
		ret = -1;
		goto err;
	}

	if (micro_driverdata.single_dev) {
		ret = -ENODEV;
		goto err;
	}

	if (pdev->id == -1) /* If we have a platform device of index -1, it's the only one */
		micro_driverdata.single_dev = 1;

	micro->cdev = cdev_alloc();
	if (micro->cdev == NULL) {
		ret = -ENOMEM;
		goto err;
	}
		
	micro_driverdata.devs[DEVS_INDEX(pdev)] = micro;
	micro->cdev->owner = THIS_MODULE;
	cdev_init(micro->cdev, &micro_fops);

	spin_lock_init(&micro->lock);
	micro->count = 0;
	micro->fasync_listeners = NULL;

	micro->irq = platform_get_irq_byname(pdev, "CAN IRQ");
	if (micro->irq < 0) {
		dev_warn(&pdev->dev, "Couldn't get CAN irq\n"); /* Not critical failure */
	} else {
		struct resource *res = platform_get_resource_byname(pdev, IORESOURCE_IRQ, "CAN IRQ");
		unsigned long flags = res->flags;
		if (!flags ||
		    request_irq(micro->irq, micro_interrupt, flags, "tomtomgo-canmicro", micro) != 0) {
			/* Not critical failure */
			dev_warn(&pdev->dev, "Unable to install CAN micro interrupt handler(%d)\n", ret);
		} else /* Begin with the irq disabled */
			disable_irq(micro->irq);
	}

	ret = cdev_add(micro->cdev, MKDEV(MAJOR(micro_driverdata.firstdev), DEVS_INDEX(pdev)), 1);
	if (ret != 0) {
		dev_err(&pdev->dev, "Unable to add chardev major=%d, minor=%d (%d)\n",
		        MAJOR(micro_driverdata.firstdev), DEVS_INDEX(pdev), ret);
		goto cdev_add_err;
	}

	micro_dev = device_create(micro_class, NULL, MKDEV(MAJOR(micro_driverdata.firstdev), DEVS_INDEX(pdev)), 
	                          NULL, "%s", MICRO_DEVNAME);
	if (IS_ERR(micro_dev)) {
		dev_err(&pdev->dev, "Unable to create device with major=%d, minor=%d (%d)\n",
		        MAJOR(micro_driverdata.firstdev), DEVS_INDEX(pdev), ret);
		ret = PTR_ERR(micro_dev);
		goto device_create_err;
	}
	return 0;

device_create_err:
	cdev_del(micro->cdev);
cdev_add_err:
	kfree(micro->cdev);
	micro->cdev = NULL;
	micro_driverdata.devs[DEVS_INDEX(pdev)] = NULL;
err: 
	return ret;
}

int micro_remove(struct platform_device *pdev)
{
	struct can_micro_dev *micro = dev_get_platdata(&pdev->dev);

	dev_info(&pdev->dev, "Removing micro\n");

	__micro_deactivate(micro);
	micro_driverdata.devs[DEVS_INDEX(pdev)] = NULL;
	if USING_IRQ(micro)
		free_irq(micro->irq, micro);
	device_destroy(micro_class, MKDEV(MAJOR(micro_driverdata.firstdev), DEVS_INDEX(pdev)));
	cdev_del(micro->cdev);
	kfree(micro->cdev);

	if (pdev->id == -1)
		micro_driverdata.single_dev = 0;

	return 0;
}

static void micro_shutdown(struct platform_device *pdev)
{
	struct can_micro_dev *micro = dev_get_platdata(&pdev->dev);

	dev_info(&pdev->dev, "Shutting down micro\n");
	__micro_deactivate(micro);
}

#ifdef CONFIG_PM

static int micro_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct can_micro_dev *micro = dev_get_platdata(&pdev->dev);

	__micro_deactivate(micro);
	micro_gpio_set_mandatory(micro->gpio_pin_can_resetout, 1);
	micro_gpio_set_mandatory(micro->gpio_pin_can_sync, 0);

	return 0;
}

static int micro_resume(struct platform_device *pdev)
{
	struct can_micro_dev *micro = dev_get_platdata(&pdev->dev);
	if (micro->count)
		__micro_activate(micro);
	micro_gpio_set_mandatory(micro->gpio_pin_can_resetout, 0);
	micro_gpio_set_mandatory(micro->gpio_pin_can_sync, 1);
	return 0;
}

#else /* CONFIG_PM */
# define micro_suspend NULL
# define micro_resume  NULL
#endif /* CONFIG_PM */

static struct platform_driver micro_driver = {
	.driver = {
		.name   = "tomtomgo-canmicro",
		.owner  = THIS_MODULE,
	},
	.probe    = micro_probe,
	.remove   = micro_remove,
	.shutdown = micro_shutdown,
	.suspend  = micro_suspend,
	.resume   = micro_resume,
};

static int __init micro_mod_init(void)
{
	int ret;

	printk(KERN_INFO "TomTom CAN-Microprocessor Driver, (C) 2008 TomTom BV\n");

	micro_class = class_create(THIS_MODULE, "can_micro");
	ret = PTR_ERR(micro_class);
	if (IS_ERR(micro_class)) {
		printk(KERN_ERR "Unable to create can_micro class (%d)\n", ret);
		goto err;
	}

	if (micro_major == -1) {
		ret = alloc_chrdev_region(&micro_driverdata.firstdev, MICRO_MINOR_OFFSET, MICRO_MAX_DEVS, MICRO_DEVNAME);
		if (ret) {
			printk(KERN_ERR "Unable to allocate character device region (%d)\n", ret);
			goto alloc_chrdev_err;
		}
	} else {
		micro_driverdata.firstdev = MKDEV(micro_major, MICRO_MINOR_OFFSET);
		ret = register_chrdev_region(micro_driverdata.firstdev, MICRO_MAX_DEVS, MICRO_DEVNAME);
		if (ret) {
			printk(KERN_ERR "Unable to register character device region (%d)\n", ret);
			goto alloc_chrdev_err;
		}
	}

	ret = platform_driver_register(&micro_driver);
	if (ret) {
		printk(KERN_ERR "Unable to register driver (%d)\n", ret);
		goto platform_driver_reg_err;
	}

	return 0;

platform_driver_reg_err:
	unregister_chrdev_region(micro_driverdata.firstdev, MICRO_MAX_DEVS);
alloc_chrdev_err:
	class_destroy(micro_class);
err:
	return ret;
}

static void __exit micro_mod_exit(void)
{
	printk(KERN_DEBUG "Unregistering driver\n");
	unregister_chrdev_region(micro_driverdata.firstdev, MICRO_MAX_DEVS);
	platform_driver_unregister(&micro_driver);
	class_destroy(micro_class);
}

module_init(micro_mod_init);
module_exit(micro_mod_exit);

MODULE_AUTHOR("Peter Wouters <peter.wouters@tomtom.com>");
MODULE_DESCRIPTION("TomTom CAN-Microprocessor Driver");
MODULE_LICENSE("GPL");

/* EOF */ 

