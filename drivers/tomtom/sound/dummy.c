/* drivers/tomtom/sound/dummy.c
 *
 * Default platform implementation for the Nashville controls. Does nothing.
 *
 * Copyright (C) 2009 TomTom BV <http://www.tomtom.com/>
 * Author: Guillaume Ballet <Guillaume.Ballet@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

#include <plat/scenarii.h>

#define VERBOSE

#ifdef VERBOSE
# define DUMMY_TRACE()		printk("%s\n", __func__);
#else
# define DUMMY_TRACE()		while (0);
#endif

#define DUMMY_CALLBACK(name) \
	static int dummy_##name(struct tt_scenario_context *c)	\
				{							\
		DUMMY_TRACE();					\
				}

DUMMY_CALLBACK(set_volume)
DUMMY_CALLBACK(set_volume_db)
DUMMY_CALLBACK(set_scenario)
DUMMY_CALLBACK(set_scenario_mode)

static struct tt_scenario_ops dummy_scenario_ops = {
	.set_volume		= dummy_set_volume,
	.set_volume_db		= dummy_set_volume_db,
	.set_scenario		= dummy_set_scenario,
	.set_scenario_mode	= dummy_set_scenario_mode,
};

static int dummy_scenario_register(void)
{
	tt_scenario_register(&dummy_scenario_ops);

	return 0;
}

late_initcall(dummy_scenario_register);
