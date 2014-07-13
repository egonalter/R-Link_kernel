/*
 * Copyright (C) 2006 TomTom BV <http://www.tomtom.com/>
 * Authors: Pepijn de Langen <pepijn.delangen@tomtom.com>
 *
 * This file is licensed under the terms of the GNU General Public
 * License version 2. This program is licensed "as is" without any
 * warranty of any kind, whether express or implied.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/gpio.h>

#include <plat/tt_vbusmon.h>
#include <plat/irvine.h>

static int (*vbusdetect_handler)(struct device *, int);
static atomic_t configured = ATOMIC_INIT(0);

struct platform_device device_gpio_vbusmon;

/**
 * Read out the value of the vbus pin
 **/
static irqreturn_t vbus_irq_handler(int irqnumber, void *dev_id)
{
	if (vbusdetect_handler) {
    if (vbusdetect_handler(&device_gpio_vbusmon.dev, gpio_get_value(TT_VGPIO_USB_HOST_DETECT))) {
      printk(KERN_DEBUG "%s: vbus handler failed\n",__FILE__);
    }
	}
	return IRQ_HANDLED;
}

static int config_vbusdetect(int (*hndlr)(struct device *, int))
{
  vbusdetect_handler = hndlr;
  return 0;
}

static void cleanup_vbusdetect(void)
{
  vbusdetect_handler = NULL;
}

/**
 * Poll the vbus pin and return its status
 * Return codes:
 *  0             - vbus is not detected
 *  anything else - vbus is detected
 **/
static int poll_vbusdetect(void)
{
  WARN_ONCE(!atomic_read(&configured), "gpio-vbusmon is not configured\n");
	return gpio_get_value(TT_VGPIO_USB_HOST_DETECT);
}

static void vbus_suspend(void)
{
  int int_nr = gpio_to_irq(TT_VGPIO_USB_HOST_DETECT);
  int ret;

  pr_info("gpio-vbusmon: suspending\n");
	if (device_may_wakeup(&device_gpio_vbusmon.dev)) {
	  ret = enable_irq_wake(int_nr);
	  if (ret) {
	    printk("gpio-vbusmon: Failed to enable irq for wake up (%d).\n", ret);
	  }
	}
}

static void vbus_resume(void)
{
  int int_nr = gpio_to_irq(TT_VGPIO_USB_HOST_DETECT);
  int ret;

  pr_info("gpio-vbusmon: resuming\n");

  // enable/disable_irq_wake must be balanced, so lets disable it now
  if (device_may_wakeup(&device_gpio_vbusmon.dev)) {
    ret = disable_irq_wake(int_nr);
    if (ret) {
      printk("gpio-vbusmon: Failed to disable irq for wake up (%d).\n", ret);
    }
  }
}

static struct vbus_pdata pdata = {
  .name = "gpio-vbusmon",
  .config_vbusdetect = config_vbusdetect,
  .cleanup_vbusdetect = cleanup_vbusdetect,
  .poll_vbusdetect = poll_vbusdetect,
  .do_suspend = vbus_suspend,
  .do_resume = vbus_resume,
};

struct platform_device device_gpio_vbusmon = {
  .name = "tomtom-vbus",
  .id = 1,
  .dev  = {
    .platform_data = &pdata
  },
};

/**
 * Configure the vbus-detect pin and register it as an interrupt
 **/
static int setup_gpio_vbusdetect(void)
{
  int int_nr = gpio_to_irq(TT_VGPIO_USB_HOST_DETECT);
  int ret;

  BUG_ON(atomic_read(&configured));

  ret = gpio_request(TT_VGPIO_USB_HOST_DETECT, "gpio_vbusmon");
  if (ret) {
    printk ("Could not get gpio %d (%d)\n", TT_VGPIO_USB_HOST_DETECT, ret);
    return ret;
  }

  /* Set up the USB detect pin */
  ret = gpio_direction_input(TT_VGPIO_USB_HOST_DETECT);
  if (ret) {
    printk ("Could not set gpio direction (%d)\n", ret);
    goto err_gpio;
  }

  /* We are configured now, unset this if we fail later */
  atomic_set(&configured, 1);

  /* WARNING! There is a design flaw in the s3c external interrupt
   * wakeup source handling: configuring IRQT_FALLING will resume the
   * device ONLY if the signal is HIGH during suspend. If signal is
   * LOW at the time of suspend, device will not resume after the
   * sequence LOW->HIGH->LOW! */
  ret = request_irq(int_nr, vbus_irq_handler, IRQ_TYPE_EDGE_FALLING, "gpio-vbusmon", &device_gpio_vbusmon);
  if (ret) {
    printk(KERN_ERR "%s: could not register interrupt for gpio-vbusmon (%d)\n", __FILE__, ret);
    goto err_gpio;
  }

  ret = platform_device_register(&device_gpio_vbusmon);
  if (ret) {
    printk(KERN_ERR "%s: could not create vbus sysfs file for gpio-vbusmon (%d)\n", __FILE__, ret);
    goto err_irq;
  }

  // this one can wake the device, but disabled by default
  device_set_wakeup_capable(&device_gpio_vbusmon.dev, 1);
  device_set_wakeup_enable(&device_gpio_vbusmon.dev, 0);

  return 0;

err_irq:
  free_irq(int_nr, &device_gpio_vbusmon);
err_gpio:
  gpio_free(TT_VGPIO_USB_HOST_DETECT);
  atomic_set(&configured, 0);
  return ret;
}

device_initcall(setup_gpio_vbusdetect);
