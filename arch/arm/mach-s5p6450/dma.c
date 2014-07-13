/* linux/arch/arm/mach-s5pv210/dma.c
 *
 * Copyright (c) 2009 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S5PV210 - DMA support
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/sysdev.h>
#include <linux/serial_core.h>

#include <mach/map.h>
#include <mach/dma.h>

#include <plat/cpu.h>
#include <plat/dma-s5p.h>

/* DMAC */
#define MAP0(x) { \
				[0]		= (x) | DMA_CH_VALID,	\
				[1]		= (x) | DMA_CH_VALID,	\
				[2]		= (x) | DMA_CH_VALID,	\
				[3]		= (x) | DMA_CH_VALID,	\
				[4]		= (x) | DMA_CH_VALID,	\
				[5]		= (x) | DMA_CH_VALID,	\
				[6]		= (x) | DMA_CH_VALID,	\
				[7]		= (x) | DMA_CH_VALID,	\
		}


/* DMA request sources	*/
#define S3C_DMA0_UART0CH0		0
#define S3C_DMA0_UART0CH1		1
#define S3C_DMA0_UART1CH0		2
#define S3C_DMA0_UART1CH1		3
#define S3C_DMA0_UART2CH0		4
#define S3C_DMA0_UART2CH1		5
#define S3C_DMA0_UART3CH0		6
#define S3C_DMA0_UART3CH1		7
#define S3C_DMA0_UART4CH0		8
#define S3C_DMA0_UART4CH1		9
#define S3C_DMA0_PCM0_TX		10
#define S3C_DMA0_PCM0_RX		11
#define S3C_DMA0_I2S0_TX		12
#define S3C_DMA0_I2S0_RX		13
#define S3C_DMA0_SPI0_TX		14
#define S3C_DMA0_SPI0_RX		15
#define S3C_DMA0_PCM1_TX		16
#define S3C_DMA0_PCM1_RX		17
#define S3C_DMA0_PCM2_TX		18
#define S3C_DMA0_PCM2_RX		19
#define S3C_DMA0_SPI1_TX		20
#define S3C_DMA0_SPI1_RX		21
#define S3C_DMA0_GPS			24
#define S3C_DMA0_I2S2_TX		27
#define S3C_DMA0_I2S2_RX		28
#define S3C_DMA0_PWM			29
#define S3C_DMA0_UART5CH0		30
#define S3C_DMA0_UART5CH1		31
//#define S3C_DMA0_EXTERNAL		  31 //mkh:what is this?

#define S3C_DMA_M2M				0


static struct s5p_dma_map __initdata s5pv210_dma_mappings[] = {
	[DMACH_I2S_IN] = {
		.name			= "i2s0-in",
		.channels		= MAP0(S3C_DMA0_I2S0_RX),
		.hw_addr.from	= S3C_DMA0_I2S0_RX,
	},
	[DMACH_I2S_OUT] = {
		.name			= "i2s0-out",
		.channels		= MAP0(S3C_DMA0_I2S0_TX),
		.hw_addr.to		= S3C_DMA0_I2S0_TX,
	},
	[DMACH_SPI0_RX] = {
		.name			= "spi0-in",
		.channels		= MAP0(S3C_DMA0_SPI0_RX),
		.hw_addr.from	= S3C_DMA0_SPI0_RX,
	},
	[DMACH_SPI0_TX] = {
		.name			= "spi0-out",
		.channels		= MAP0(S3C_DMA0_SPI0_TX),
		.hw_addr.to		= S3C_DMA0_SPI0_TX,
	},
	[DMACH_SPI1_RX] = {
		.name			= "spi1-in",
		.channels		= MAP0(S3C_DMA0_SPI1_RX),
		.hw_addr.from	= S3C_DMA0_SPI1_RX,
	},
	[DMACH_SPI1_TX] = {
		.name			= "spi1-out",
		.channels		= MAP0(S3C_DMA0_SPI1_TX),
		.hw_addr.to		= S3C_DMA0_SPI1_TX,
	},
	[DMACH_PCM0_TX] = {
		.name			= "pcm0-out",
		.channels		= MAP0(S3C_DMA0_PCM0_TX),
		.hw_addr.to		= S3C_DMA0_PCM0_TX,
	},
	[DMACH_PCM0_RX] = {
		.name			= "pcm0-in",
		.channels		= MAP0(S3C_DMA0_PCM0_RX),
		.hw_addr.from	= S3C_DMA0_PCM0_RX,
	},
	[DMACH_PCM1_TX] = {
		.name			= "pcm1-out",
		.channels		= MAP0(S3C_DMA0_PCM1_TX),
		.hw_addr.to		= S3C_DMA0_PCM1_TX,
	},
	[DMACH_PCM1_RX] = {
		.name			= "pcm1-in",
		.channels		= MAP0(S3C_DMA0_PCM1_RX),
		.hw_addr.from	= S3C_DMA0_PCM1_RX,
	},
	[DMACH_PCM2_TX] = {
		.name			= "pcm2-out",
		.channels		= MAP0(S3C_DMA0_PCM2_TX),
		.hw_addr.to		= S3C_DMA0_PCM2_TX,
	},
	[DMACH_PCM2_RX] = {
		.name			= "pcm2-in",
		.channels		= MAP0(S3C_DMA0_PCM2_RX),
		.hw_addr.from	= S3C_DMA0_PCM2_RX,
	},
	[DMACH_3D_M2M] = {
		.name			= "3D-M2M",
		.channels		= MAP0(S3C_DMA_M2M),
		.hw_addr.from	= 0,
	},
	[DMACH_I2S_V40_IN] = {
		.name			= "i2s-v40-in",
		.channels		= MAP0(S3C_DMA0_I2S0_RX),
		.hw_addr.from	= S3C_DMA0_I2S0_RX,
		.sdma_sel		= 1 << S3C_DMA0_I2S0_RX,
	},
	[DMACH_I2S_V40_OUT] = {
		.name			= "i2s-v40-out",
		.channels		= MAP0(S3C_DMA0_I2S0_TX),
		.hw_addr.to		= S3C_DMA0_I2S0_TX,
		.sdma_sel		= 1 << S3C_DMA0_I2S0_TX,
	},
	[DMACH_I2S2_IN] = {
		.name			= "i2s2-in",
		.channels		= MAP0(S3C_DMA0_I2S2_RX),
		.hw_addr.from	= S3C_DMA0_I2S2_RX,
		.sdma_sel		= 1 << S3C_DMA0_I2S2_RX,
	},
	[DMACH_I2S2_OUT] = {
		.name			= "i2s2-out",
		.channels		= MAP0(S3C_DMA0_I2S2_TX),
		.hw_addr.to		= S3C_DMA0_I2S2_TX,
		.sdma_sel		= 1 << S3C_DMA0_I2S2_TX,
	},
	[DMACH_UART0]	= {
		.name		= "uart0-dma-tx",
		.channels	= MAP0(S3C_DMA0_UART0CH0),
		.hw_addr.to 	= S3C_DMA0_UART0CH0,
	},
	[DMACH_UART0_SRC2] = {
		.name		= "uart0-dma-rx",
		.channels	= MAP0(S3C_DMA0_UART0CH1),
		.hw_addr.from	= S3C_DMA0_UART0CH1,
	},	
	[DMACH_UART1]	= {
		.name		= "uart1-dma-tx",
		.channels	= MAP0(S3C_DMA0_UART1CH0),
		.hw_addr.to 	= S3C_DMA0_UART1CH0,
	},
	[DMACH_UART1_SRC2] = {
		.name		= "uart1-dma-rx",
		.channels	= MAP0(S3C_DMA0_UART1CH1),
		.hw_addr.from	= S3C_DMA0_UART1CH1,
	},	
	[DMACH_UART2]	= {
		.name		= "uart2-dma-tx",
		.channels	= MAP0(S3C_DMA0_UART2CH0),
		.hw_addr.to 	= S3C_DMA0_UART2CH0,
	},
	[DMACH_UART2_SRC2] = {
		.name		= "uart2-dma-rx",
		.channels	= MAP0(S3C_DMA0_UART2CH1),
		.hw_addr.from	= S3C_DMA0_UART2CH1,
	},	
	[DMACH_UART3]	= {
		.name		= "uart3-dma-tx",
		.channels	= MAP0(S3C_DMA0_UART3CH0),
		.hw_addr.to 	= S3C_DMA0_UART3CH0,
	},
	[DMACH_UART3_SRC2] = {
		.name		= "uart3-dma-rx",
		.channels	= MAP0(S3C_DMA0_UART3CH1),
		.hw_addr.from	= S3C_DMA0_UART3CH1,
	},	
	[DMACH_UART4]	= {
		.name		= "uart4-dma-tx",
		.channels	= MAP0(S3C_DMA0_UART4CH0),
		.hw_addr.to 	= S3C_DMA0_UART4CH0,
	},
	[DMACH_UART4_SRC2] = {
		.name		= "uart4-dma-rx",
		.channels	= MAP0(S3C_DMA0_UART4CH1),
		.hw_addr.from	= S3C_DMA0_UART4CH1,
	},	
	[DMACH_UART5]	= {
		.name		= "uart5-dma-tx",
		.channels	= MAP0(S3C_DMA0_UART5CH0),
		.hw_addr.to 	= S3C_DMA0_UART5CH0,
	},
	[DMACH_UART5_SRC2] = {
		.name		= "uart5-dma-rx",
		.channels	= MAP0(S3C_DMA0_UART5CH1),
		.hw_addr.from	= S3C_DMA0_UART5CH1,
	},
};

static void s5pv210_dma_select(struct s3c2410_dma_chan *chan,
				   struct s5p_dma_map *map)
{
	chan->map = map;
}

static struct s5p_dma_selection __initdata s5pv210_dma_sel = {
	.select		= s5pv210_dma_select,
	.dcon_mask	= 0,
	.map		= s5pv210_dma_mappings,
	.map_size	= ARRAY_SIZE(s5pv210_dma_mappings),
};

static int __init s5pv210_dma_add(struct sys_device *sysdev)
{
	s5p_dma_init(S3C_DMA_CHANNELS, IRQ_DMA0, 0x20);
	return s5p_dma_init_map(&s5pv210_dma_sel);
}

static struct sysdev_driver s5pv210_dma_driver = {
	.add	= s5pv210_dma_add,
};

static int __init s5pv210_dma_init(void)
{
	return sysdev_driver_register(&s5p6450_sysclass, &s5pv210_dma_driver);
}

arch_initcall(s5pv210_dma_init);
