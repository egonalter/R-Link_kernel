/**
 * drivers/gpio/vgpio.c
 *
 * Copyright 2009 TomTom B.V.
 *	Marc Zyngier <marc.zyngier@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/list.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/vgpio.h>

#define VGPIO_NAME	"vgpio"
#define PFX			VGPIO_NAME ": "

struct vgpio {
	struct list_head	entry;
	struct vgpio_pin	*pins;
	struct gpio_chip	chip;
};

#define chip_to_vgpio(x)	container_of((x), struct vgpio, chip)

static LIST_HEAD(vgpio_list);

static inline int vgpio_is_valid(struct vgpio_pin *pin)
{
	return (pin != NULL && pin->vpin && pin->valid);
}

static inline int vgpio_is_nc(struct vgpio_pin *pin)
{
	return (pin == NULL || pin->notcon);
}

static inline struct vgpio_pin *vgpio_get_pin(struct vgpio *vg, unsigned offset)
{
	struct vgpio_pin *pin = &vg->pins[offset];

	WARN_ON(!vgpio_is_valid(pin));
	return pin;
}

static inline int normalize_value(struct vgpio_pin *pin, int value)
{
	return pin->inverted ^ !!value;
}

static int vgpio_direction_input(struct gpio_chip *chip, unsigned offset)
{
	struct vgpio *vg = chip_to_vgpio(chip);
	struct vgpio_pin *pin = vgpio_get_pin(vg, offset);

	if (unlikely(vgpio_is_nc(pin)))
		return 0;

	return gpio_direction_input(pin->tpin);
}

static int vgpio_direction_output(struct gpio_chip *chip, unsigned offset,
				    int value)
{
	struct vgpio *vg = chip_to_vgpio(chip);
	struct vgpio_pin *pin = vgpio_get_pin(vg, offset);

	if (unlikely(vgpio_is_nc(pin)))
		return 0;

	return gpio_direction_output(pin->tpin, normalize_value(pin, value));
}

static int vgpio_get(struct gpio_chip *chip, unsigned offset)
{
	int value;
	struct vgpio *vg = chip_to_vgpio(chip);
	struct vgpio_pin *pin = vgpio_get_pin(vg, offset);

	if (unlikely(vgpio_is_nc(pin)))
		return 0;

	if (gpio_cansleep (pin->tpin))
		value = gpio_get_value_cansleep(pin->tpin);
	else
		value = gpio_get_value(pin->tpin);	
	
	return normalize_value(pin, value);
}

static void vgpio_set(struct gpio_chip *chip, unsigned offset, int value)
{
	struct vgpio *vg = chip_to_vgpio(chip);
	struct vgpio_pin *pin = vgpio_get_pin(vg, offset);

	if (unlikely(vgpio_is_nc(pin)))
		return;

	if (gpio_cansleep (pin->tpin))
		gpio_set_value_cansleep(pin->tpin, normalize_value(pin, value));
	else
		gpio_set_value(pin->tpin, normalize_value(pin, value));
}

static int vgpio_to_irq(struct gpio_chip *chip, unsigned offset)
{
	struct vgpio *vg = chip_to_vgpio(chip);
	struct vgpio_pin *pin = vgpio_get_pin(vg, offset);

	if (!vgpio_is_valid(pin))
		return -EINVAL;

	if (unlikely(vgpio_is_nc(pin)))
		return -EINVAL;

	return gpio_to_irq(pin->tpin);
}

static int vgpio_request_all(struct platform_device *pdev, struct vgpio_pin *pins, int nr)
{
	int i;
	int err = 0;
	struct vgpio_pin *pin;

	for (i = 0; i < nr; i++) {
		pin = &pins[i];

		if (!vgpio_is_valid(pin))
			continue;

		if (vgpio_is_nc(pin)) {
			dev_warn(&pdev->dev, "pin #%d is tagged as not connected!\n", pin->vpin);
			continue;
		}

		if ((err = gpio_request(pin->tpin, VGPIO_NAME))) {
			dev_err(&pdev->dev, "Failed to request backend gpio %d, vpin %d (%d)\n", pin->tpin, pin->vpin, err);
			break;
		}
	}

	while (err && (--i >= 0)) {
		pin = &pins[i];

		if (vgpio_is_valid(pin) && !vgpio_is_nc(pin))
			gpio_free(pin->tpin);
	}

	return err;
}

static void vgpio_free_all(struct vgpio_pin *pins, int nr)
{
	int i;

	for (i = 0; i < nr; i++)
		if (vgpio_is_valid(&pins[i]) && !vgpio_is_nc(&pins[i]))
			gpio_free(pins[i].tpin);
}

int vgpio_to_gpio(unsigned gpio)
{
	struct vgpio *vg;
	struct vgpio_pin *pin;
	unsigned offset;

	list_for_each_entry(vg, &vgpio_list, entry) {
		if (gpio < vg->chip.base || gpio > (vg->chip.base + vg->chip.ngpio))
			continue;

		offset = gpio - vg->chip.base;
		pin = vgpio_get_pin(vg, offset);

		if (!vgpio_is_valid(pin) || vgpio_is_nc(pin))
			return -EINVAL;

		return pin->tpin;
	}

	return -ENODEV;
}

EXPORT_SYMBOL_GPL(vgpio_to_gpio);

static int __devinit vgpio_probe(struct platform_device *pdev)
{
	struct vgpio *vg;
	struct vgpio_platform_data *pdata = pdev->dev.platform_data;
	int err;

	if (!pdata || !pdata->gpio_base || !pdata->gpio_number || !pdata->pins)
		return -ENODEV;

	vg = kzalloc(sizeof(struct vgpio), GFP_KERNEL);
	if (!vg)
		return -ENOMEM;

	if ((err = vgpio_request_all(pdev, pdata->pins, pdata->gpio_number))) {
		dev_err(&pdev->dev, "Failed to grab backend gpios (%d)\n", err);
		kfree(vg);
		return err;
	}

	vg->pins			= pdata->pins;

	vg->chip.label			= VGPIO_NAME;
	vg->chip.direction_input	= vgpio_direction_input;
	vg->chip.get			= vgpio_get;
	vg->chip.direction_output	= vgpio_direction_output;
	vg->chip.set			= vgpio_set;
	vg->chip.base			= pdata->gpio_base;
	vg->chip.ngpio			= pdata->gpio_number;
	vg->chip.dev			= &pdev->dev;
	vg->chip.owner			= THIS_MODULE;
	vg->chip.to_irq			= vgpio_to_irq;

	dev_set_drvdata(&pdev->dev, vg);

	if ((err = gpiochip_add(&vg->chip))) {
		pr_err("Failed to register vgpio chip (%d)\n", err);
		dev_set_drvdata(&pdev->dev, NULL);
		vgpio_free_all(pdata->pins, pdata->gpio_number);
		kfree(vg);
		return err;
	}

	INIT_LIST_HEAD(&vg->entry);
	list_add(&vg->entry, &vgpio_list);

	dev_info(&pdev->dev, "registered %d GPIOs (@%d)\n", pdata->gpio_number, pdata->gpio_base);

	if (pdata->setup)
		if ((err = pdata->setup(pdev, pdata->context)))
			dev_warn(&pdev->dev, "setup returned %d\n", err);

	return 0;
}

static int __devexit vgpio_remove(struct platform_device *pdev)
{
	struct vgpio *vg = dev_get_drvdata(&pdev->dev);
	struct vgpio_platform_data *pdata = pdev->dev.platform_data;
	int err;

	if (!vg)
		return -ENODEV;

	if (pdata->teardown)
		if ((err = pdata->teardown(pdev, pdata->context)))
			dev_warn(&pdev->dev, "teardown returned %d\n", err);

	list_del(&vg->entry);

	if (!(err = gpiochip_remove(&vg->chip))) {
		dev_set_drvdata(&pdev->dev, NULL);
		vgpio_free_all(vg->pins, vg->chip.ngpio);
		kfree(vg);
	}

	return err;
}

static struct platform_driver vgpio_driver = {
	.driver = {
		.name		= VGPIO_NAME,
		.owner		= THIS_MODULE,
	},
	.probe		= vgpio_probe,
	.remove		= __devexit_p(vgpio_remove),
};

static int __init vgpio_init(void)
{
	return platform_driver_register(&vgpio_driver);
}

static void __exit vgpio_exit(void)
{
	platform_driver_unregister(&vgpio_driver);
}

#ifdef CONFIG_GPIO_VGPIO_EARLY
postcore_initcall(vgpio_init);
#else
module_init(vgpio_init);
#endif
module_exit(vgpio_exit);

