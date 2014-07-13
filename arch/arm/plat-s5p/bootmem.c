/* linux/arch/arm/plat-s5p/bootmem.c
 *
 * Copyright (c) 2009 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Bootmem helper functions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/mm.h>
#include <linux/bootmem.h>
#include <linux/swap.h>
#include <asm/setup.h>
#include <asm/io.h>
#include <mach/memory.h>

#include "plat/media.h"

static struct s3c_media_device s3c_mdevs[S3C_MDEV_MAX] = {
	{
		.id = S3C_MDEV_FIMC,
		.name = "s3c-fimc",

#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC0
		.memsize = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMC0*SZ_1K,
#else
		.memsize = 0,
#endif
		.paddr = 0,
	},
	{
		.id = S3C_MDEV_GVG,
		.name = "s3c-gvg",

#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_GVG
		.memsize = CONFIG_VIDEO_SAMSUNG_MEMSIZE_GVG * SZ_1K,
#else
		.memsize = 0,
#endif
		.paddr = 0,
	},


#ifdef  CONFIG_ANDROID_PMEM_MEMSIZE_PMEM
/* pmem */
        {
                .id = S3C_MDEV_PMEM,
                .name = "pmem",
                .memsize = CONFIG_ANDROID_PMEM_MEMSIZE_PMEM * SZ_1K,
                .paddr = 0,
                .node = 0, //OneDRAM
        },
#endif

#ifdef CONFIG_ANDROID_PMEM_MEMSIZE_PMEM_GPU1
        {
                .id = S3C_MDEV_PMEM_GPU1,
                .name = "pmem_gpu1",
                .memsize = CONFIG_ANDROID_PMEM_MEMSIZE_PMEM_GPU1 * SZ_1K,
                .paddr = 0,
                .node = 0, //OneDRAM 
        },
        {
                .id = S3C_MDEV_PMEM_ADSP,
                .name = "pmem_adsp",
                .memsize = CONFIG_ANDROID_PMEM_MEMSIZE_PMEM_ADSP * SZ_1K,
                .paddr = 0,
                .node = 0, //OneDRAM
        },
#endif


#ifdef CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMD
        {
                .id = S3C_MDEV_FIMD,
                .name = "fimd",
                .node = 1,
                .memsize = CONFIG_VIDEO_SAMSUNG_MEMSIZE_FIMD * SZ_1K,
                .paddr = 0,
        },
#endif


};

static struct s3c_media_device *s3c_get_media_device(int dev_id)
{
	struct s3c_media_device *mdev = NULL;
	int i, found;

	if (dev_id < 0 || dev_id >= S3C_MDEV_MAX)
		return NULL;

	i = 0;
	found = 0;
	while (!found && (i < S3C_MDEV_MAX)) {
		mdev = &s3c_mdevs[i];
		if (mdev->id == dev_id)
			found = 1;
		else
			i++;
	}

	if (!found)
		mdev = NULL;

	return mdev;
}

dma_addr_t s3c_get_media_memory(int dev_id)
{
	struct s3c_media_device *mdev;

	mdev = s3c_get_media_device(dev_id);
	if (!mdev){
		printk(KERN_ERR "invalid media device\n");
		return 0;
	}

	if (!mdev->paddr) {
		printk(KERN_ERR "no memory for %s\n", mdev->name);
		return 0;
	}

	return mdev->paddr;
}

size_t s3c_get_media_memsize(int dev_id)
{
	struct s3c_media_device *mdev;

	mdev = s3c_get_media_device(dev_id);
	if (!mdev){
		printk(KERN_ERR "invalid media device\n");
		return 0;
	}

	return mdev->memsize;
}

void s3c64xx_reserve_bootmem(void)
{
	struct s3c_media_device *mdev;
	int i;

	for(i = 0; i < sizeof(s3c_mdevs) / sizeof(s3c_mdevs[0]); i++) {
		mdev = &s3c_mdevs[i];
		if (mdev->memsize > 0) {
			mdev->paddr = virt_to_phys(alloc_bootmem_low(mdev->memsize));
			printk(KERN_INFO \
				"s3c64xx: %lu bytes SDRAM reserved "
				"for %s at 0x%08x\n",
				(unsigned long) mdev->memsize, \
				mdev->name, mdev->paddr);
		}
	}
}

/* FIXME: temporary implementation to avoid compile error */
int dma_needs_bounce(struct device *dev, dma_addr_t addr, size_t size)
{
	return 0;
}
