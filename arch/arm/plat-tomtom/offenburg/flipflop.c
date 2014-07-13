/*
 * Copyright (C) 2010 TomTom BV <http://www.tomtom.com/>
 * Author: Matthias Kaehlcke <matthias.kaehlcke@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

/*
 * Uses a bit in the scratchpad memory bank of the OMAP, whose contents are not
 * affected by a warm reset (lost upon cold reset)
 */

#include <plat/control.h>
#include <linux/bitops.h>
#include <linux/platform_device.h>

#include <plat/flipflop.h>

#define OFFSET_SCRATCHPAD 0x0600

/*
 * Bit 31 of this register is our flipflop. We don't use the rest of the
 * register, so it isn't registered as a resource
 */
#define FLIPFLOP_ADDR (OFFSET_SCRATCHPAD + 0x3FC)
#define FLIPFLOP_BIT 31
#define EPICFAIL_BIT 30 /* Used to say whether backup system was successful. Not visible from userland */

#define PFX "offenburg::flipflop: "

static int flipflop_init(struct platform_device *pdev)
{
	return 0;
}

static void flipflop_set(int state)
{
	unsigned long flags, value;

	local_irq_save(flags);

	value = omap_ctrl_readl(FLIPFLOP_ADDR);

	if (state)
		__set_bit(FLIPFLOP_BIT, &value);
	else
	{
		__clear_bit(FLIPFLOP_BIT, &value);
		/* Once the epic fail bit has been cleared, it cannot be set anymore */
		__clear_bit(EPICFAIL_BIT, &value);
	}

	omap_ctrl_writel(value, FLIPFLOP_ADDR);

	local_irq_restore(flags);
}

static int flipflop_get(void)
{
	unsigned long flags, value;
	int state;

	local_irq_save(flags);

	value = omap_ctrl_readl(FLIPFLOP_ADDR);
	state = test_bit(FLIPFLOP_BIT, &value);

	local_irq_restore(flags);

	return state;
}

static struct flipflop_info flipflop_info = {
	.init      = flipflop_init,
	.set_value = flipflop_set,
	.get_value = flipflop_get
};

static struct platform_device flipflop_pdev =
{
	.name   = "flipflop",
	.id     = -1,
	.dev = {
		.platform_data = &flipflop_info,
	},
};

static int __init flipflop_register(void)
{
	platform_device_register(&flipflop_pdev);

	printk(KERN_INFO PFX "Registered\n");
	return 0;
};

arch_initcall(flipflop_register);
