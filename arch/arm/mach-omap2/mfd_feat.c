/*
 * Copyright (C) 2012 TomTom International BV
 * Written by Domenico Andreoli <domenico.andreoli@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <asm/mach-types.h>
#include <mach/mfd_feat.h>

unsigned int get_mfd_rev(void)
{
	return system_rev;
}

const struct mfd_feat * __init mfd_lookup(const struct mfd_feat *feat)
{
	BUG_ON(!feat);

	for (;; feat++) {
		if (feat->mach && feat->mach != machine_arch_type)
			continue;
		if (feat->rev && feat->rev != system_rev)
			continue;

		/* there must be a default entry that breaks the loop */
		break;
	}

	return feat;
}

void mfd_set_pdata(struct platform_device *pdev, const struct mfd_feat *feat)
{
	pdev->dev.platform_data = mfd_feature(feat);
}
