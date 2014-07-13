/* linux/arch/arm/mach-s5p6440/include/mach/system.h
 *
 * Copyright (c) 2009 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S5P6440 - system support header
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef __ASM_ARCH_SYSTEM_H
#define __ASM_ARCH_SYSTEM_H __FILE__
#include <linux/io.h>
#include <mach/map.h>


void (*s5p64xx_idle)(void);
//void (*s5p64xx_reset_hook)(void);

void s5p64xx_default_idle(void)
{
        /* nothing here yet */
}

static void arch_idle(void)
{
        if (s5p64xx_idle != NULL)
                (s5p64xx_idle)();
        else
                s5p64xx_default_idle();
}

static void arch_reset(char mode, const char *cmd)
{
	/* nothing here yet */
}

#endif /* __ASM_ARCH_SYSTEM_H */
