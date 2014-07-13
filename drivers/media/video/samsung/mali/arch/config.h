/*
 * This confidential and proprietary software may be used only as
 * authorised by a licensing agreement from ARM Limited
 * (C) COPYRIGHT 2008-2009 ARM Limited
 * ALL RIGHTS RESERVED
 * The entire notice above must be reproduced on all authorised
 * copies and copies may only be made to the extent permitted
 * by a licensing agreement from ARM Limited.
 */

#ifndef __ARCH_CONFIG_H__
#define __ARCH_CONFIG_H__

/* Configuration for the EB platform with ZBT memory enabled */
#define MALI_BASE_ADDR 0xf0000000
#define GP_ADDR MALI_BASE_ADDR
#define L2_ADDR MALI_BASE_ADDR+0x1000
#define PMU_ADDR MALI_BASE_ADDR+0x2000
#define GP_MMU_ADDR MALI_BASE_ADDR+0x3000
#define PP_MMU_ADDR MALI_BASE_ADDR+0x4000
#define PP_ADDR MALI_BASE_ADDR+0x8000

// See 3-11 page in trm. It describes control register address map. cglee

static _mali_osk_resource_t arch_configuration [] =
{
#if USING_MALI_PMU
    {
                .type = PMU,
                .description = "Mali-400 PMU",
                .base = PMU_ADDR,
                .irq = -1,
                .mmu_id = 0
        },
#endif   
	{
		.type = MALI400GP,
		.description = "Mali-400 GP",
		.base = GP_ADDR,
		.irq = 18+32,
		.mmu_id = 1
	},
	{
		.type = MALI400PP,
		.base = PP_ADDR,
		.irq = 16+32,
		.description = "Mali-400 PP 0",
		.mmu_id = 2
	},
#if USING_MMU
	{
		.type = MMU,
		.base = GP_MMU_ADDR,
		.irq = 19+32,
		.description = "Mali-400 MMU for GP",
		.mmu_id = 1
	},
	{
		.type = MMU,
		.base = PP_MMU_ADDR,
		.irq = 17+32,
		.description = "Mali-400 MMU for PP 0",
		.mmu_id = 2
	},
	{
		.type = OS_MEMORY,
		.description = "System Memory",
		.size = 0x06000000,
		.flags = _MALI_CPU_WRITEABLE | _MALI_CPU_READABLE | _MALI_PP_READABLE | _MALI_PP_WRITEABLE | _MALI_GP_READABLE | _MALI_GP_WRITEABLE
	},
	{
		.type = MEM_VALIDATION,
		.description = "memory validation",
/*zeppln 2010.07.16 vega1 test*/
#if 0 // zeppln 2010.07.16_BEGIN
		.base = 0x22000000,
		.size = 0x06000000,
#else
		.base = 0x20000000,
		.size = 0x10000000,
#endif // zeppln 2010.07.16_END
		.flags = _MALI_CPU_WRITEABLE | _MALI_CPU_READABLE | _MALI_PP_READABLE | _MALI_PP_WRITEABLE | _MALI_GP_READABLE | _MALI_GP_WRITEABLE |
			_MALI_MMU_READABLE | _MALI_MMU_WRITEABLE
	},
#else
	{
		.type = MEMORY,
		.description = "Dedicated Memory",
		.base = 0x20000000,
		.size = 0x02000000,
		.flags = _MALI_CPU_WRITEABLE | _MALI_CPU_READABLE | _MALI_PP_READABLE | _MALI_PP_WRITEABLE | _MALI_GP_READABLE | _MALI_GP_WRITEABLE |
			_MALI_MMU_READABLE | _MALI_MMU_WRITEABLE
	},
#endif
	{
		.type = MALI400L2,
		.base = L2_ADDR,
		.description = "Mali-400 L2 cache"
	},
};

#endif /* __ARCH_CONFIG_H__ */
