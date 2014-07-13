/* linux/arch/arm/plat-s5p/include/plat/media.h
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Samsung Media device descriptions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _S5P_MEDIA_H
#define _S5P_MEDIA_H

#include <linux/types.h>
#include <asm/setup.h>

/* 3 fimc indexes should be fixed as n, n+1 and n+2 */
#if 0
#define S3C_MDEV_JPEG		0
#define S3C_MDEV_MFC		1
#define S3C_MDEV_FIMD		2
#define S3C_MDEV_FIMC0		3
#define S3C_MDEV_FIMC1		4
#define S3C_MDEV_FIMC2		5
#define S3C_MDEV_CMM		6
#define S3C_MDEV_PMEM		7
#endif

#define S3C_MDEV_FIMC           0
#define S3C_MDEV_GVG            1
#define S3C_MDEV_CMM            3
#define S3C_MDEV_PMEM           4
#define S3C_MDEV_PMEM_GPU1      5
#define S3C_MDEV_PMEM_ADSP      6
#define S3C_MDEV_FIMD           7
#define S3C_MDEV_MAX            8


struct s3c_media_device {
	u32		id;
	const char 	*name;
	int             node;
	u32		bank;
	size_t		memsize;
	dma_addr_t	paddr;
};

extern struct meminfo meminfo;


                
extern dma_addr_t s3c_get_media_memory(int dev_id);
extern dma_addr_t s3c_get_media_memory_node(int dev_id, int node);
extern size_t s3c_get_media_memsize(int dev_id);
extern size_t s3c_get_media_memsize_node(int dev_id, int node);



#endif

