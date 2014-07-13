/* linux/arch/arm/plat-s3c/dev-audio.c
 *
 * Copyright (c) 2009 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Author: Jaswinder Singh <jassi.brar@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/string.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/io.h>
#include <linux/dma-mapping.h>

#include <mach/map.h>
#include <mach/dma.h>
#include <mach/irqs.h>

#include <plat/devs.h>
#include <plat/audio.h>
#include <plat/gpio-cfg.h>
#include <mach/regs-clock.h>

static int s3c64xx_cfg_i2s(struct platform_device *pdev)
{
	/* configure GPIO for i2s port */
	switch (pdev->id) {
	case 0:

                s3c_gpio_cfgpin(S5P64XX_GPR(4),(0x05<<16)); //I2S_V40_D0 [0]
                s3c_gpio_cfgpin(S5P64XX_GPR(5),(0x05<<20)); //I2S_V40_D0 [1]
                s3c_gpio_cfgpin(S5P64XX_GPR(6),(0x05<<28)); //I2S_V40_D0 [2]
                s3c_gpio_cfgpin(S5P64XX_GPR(7),(0x05<<0));  //I2S_V40_LRCLK
                s3c_gpio_cfgpin(S5P64XX_GPR(8),(0x05<<4));  //I2S_V40_DI
                s3c_gpio_cfgpin(S5P64XX_GPR(13),(0x05<<24));//I2S_V40_SCLK
                s3c_gpio_cfgpin(S5P64XX_GPR(14),(0x05<<28));//I2S_V40_CDCLK

		break;

	case 1:
		/*TODO: No I2S1 SCLK..??? */
		s3c_gpio_cfgpin(S5P64XX_GPC(0), (0x5<<0));	//I2S1_SDI
		s3c_gpio_cfgpin(S5P64XX_GPC(1), (0x5<<4));	//I2S1_SDO
		s3c_gpio_cfgpin(S5P64XX_GPC(2), (0x5<<8));	//I2S1_LRCLK
		s3c_gpio_cfgpin(S5P64XX_GPC(3), (0x5<<12));	//I2S1_CDCLK
		break;

	case 2:
		s3c_gpio_cfgpin(S5P64XX_GPK(0), (0x5<<0));	//I2S2_SDO
		s3c_gpio_cfgpin(S5P64XX_GPK(1), (0x5<<4));	//I2S2_LRCLK
		s3c_gpio_cfgpin(S5P64XX_GPK(2), (0x5<<8));	//I2S2_SDI
		s3c_gpio_cfgpin(S5P64XX_GPK(3), (0x5<<12));	//I2S2_SCLK
		s3c_gpio_cfgpin(S5P64XX_GPK(4), (0x5<<16));	//I2S2_CDCLK
		break;

	default:
		printk("Invalid Device %d!\n", pdev->id);
		return -EINVAL;
	}

	return 0;
}

static struct s3c_audio_pdata s3c64xx_i2s_pdata = {
	.cfg_gpio = s3c64xx_cfg_i2s,
};

static struct resource s3c64xx_iis0_resource[] = {
	[0] = {
		.start = S5P64XX_PA_IIS_V40, 
		.end   = S5P64XX_PA_IIS_V40 + S5P_SZ_IIS,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = DMACH_I2S_V40_OUT,
		.end   = DMACH_I2S_V40_OUT,
		.flags = IORESOURCE_DMA,
	},
	[2] = {
		.start = DMACH_I2S_V40_IN,
		.end   = DMACH_I2S_V40_IN,
		.flags = IORESOURCE_DMA,
	},
};

struct platform_device s3c64xx_device_iis0 = {
	.name		  = "s3c64xx-iis",
	.id		  = 0,
	.num_resources	  = ARRAY_SIZE(s3c64xx_iis0_resource),
	.resource	  = s3c64xx_iis0_resource,
	.dev = {
		.platform_data = &s3c64xx_i2s_pdata,
	},
};
EXPORT_SYMBOL(s3c64xx_device_iis0);

static struct resource s3c64xx_iis1_resource[] = {
	[0] = {
		.start = S5P64XX_PA_IIS1,
		.end   = S5P64XX_PA_IIS1 + S5P_SZ_IIS,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = DMACH_I2S1_OUT,
		.end   = DMACH_I2S1_OUT,
		.flags = IORESOURCE_DMA,
	},
	[2] = {
		.start = DMACH_I2S1_IN,
		.end   = DMACH_I2S1_IN,
		.flags = IORESOURCE_DMA,
	},
};

struct platform_device s3c64xx_device_iis1 = {
	.name		  = "s3c64xx-iis",
	.id		  = 1,
	.num_resources	  = ARRAY_SIZE(s3c64xx_iis1_resource),
	.resource	  = s3c64xx_iis1_resource,
	.dev = {
		.platform_data = &s3c64xx_i2s_pdata,
	},
};
EXPORT_SYMBOL(s3c64xx_device_iis1);

static struct resource s3c64xx_iis2_resource[] = {
	[0] = {
		.start = S5P64XX_PA_IIS2,
		.end   = S5P64XX_PA_IIS2 + S5P_SZ_IIS,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = DMACH_I2S2_OUT,
		.end   = DMACH_I2S2_OUT,
		.flags = IORESOURCE_DMA,
	},
	[2] = {
		.start = DMACH_I2S2_IN,
		.end   = DMACH_I2S2_IN,
		.flags = IORESOURCE_DMA,
	},
};

struct platform_device s3c64xx_device_iis2 = {
	.name		  = "s3c64xx-iis",
	.id		  = 2,
	.num_resources	  = ARRAY_SIZE(s3c64xx_iis2_resource),
	.resource	  = s3c64xx_iis2_resource,
	.dev = {
		.platform_data = &s3c64xx_i2s_pdata,
	},
};
EXPORT_SYMBOL(s3c64xx_device_iis2);

/* PCM Controller platform_devices */

static int s3c64xx_pcm_cfg_gpio(struct platform_device *pdev)
{
	switch (pdev->id) {
	case 0:
		s3c_gpio_cfgpin(S5P64XX_GPR(4),0x2<<16);	//PCM_SOUT
		s3c_gpio_cfgpin(S5P64XX_GPR(7),0x2<<0);		//PCM_FSYNC
		s3c_gpio_cfgpin(S5P64XX_GPR(8),0x2<<4);		//PCM_SIN
		s3c_gpio_cfgpin(S5P64XX_GPR(13),0x2<<24);	//PCM_DCLK
//		s3c_gpio_cfgpin(S5P64XX_GPR(14),0x2<<28);	//PCM_EXTCLK
		/* XXX WORKAROUND:- we r using IIS clock for pcm */
		s3c_gpio_cfgpin(S5P64XX_GPR(14),0x5<<28);	//PCM_EXTCLK
		break;
	case 1:
		s3c_gpio_cfgpin(S5P64XX_GPB(4),0x4<<16);	//PCM_DCLK1
		s3c_gpio_cfgpin(S5P64XX_GPC(0),0x4<<0);		//PCM_IN1
		s3c_gpio_cfgpin(S5P64XX_GPC(1),0x4<<4);		//PCM_SOUT1
		s3c_gpio_cfgpin(S5P64XX_GPC(2),0x4<<8);		//PCM_FSYNC1
		s3c_gpio_cfgpin(S5P64XX_GPC(3),0x4<<12);	//PCM_EXTCLK1
		break;
	case 2:
		s3c_gpio_cfgpin(S5P64XX_GPK(0),0x2<<0);		//PCM2_SOUT
		s3c_gpio_cfgpin(S5P64XX_GPK(1),0x2<<4);		//PCM2_FSYNC
		s3c_gpio_cfgpin(S5P64XX_GPK(2),0x2<<8);		//PCM2_SIN
		s3c_gpio_cfgpin(S5P64XX_GPK(3),0x2<<12);	//PCM2_DCLK
		s3c_gpio_cfgpin(S5P64XX_GPK(4),0x2<<16);	//PCM2_EXTCLK
		break;
	default:
		printk(KERN_DEBUG "Invalid PCM Controller number!");
		return -EINVAL;
	}

	return 0;
}

static struct s3c_audio_pdata s3c_pcm_pdata = {
	.cfg_gpio = s3c64xx_pcm_cfg_gpio,
};

static struct resource s3c64xx_pcm0_resource[] = {
	[0] = {
		.start = S5P64XX_PA_PCM0,
		.end   = S5P64XX_PA_PCM0 + S5P_SZ_PCM0,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = DMACH_PCM0_TX,
		.end   = DMACH_PCM0_TX,
		.flags = IORESOURCE_DMA,
	},
	[2] = {
		.start = DMACH_PCM0_RX,
		.end   = DMACH_PCM0_RX,
		.flags = IORESOURCE_DMA,
	},
};

struct platform_device s3c64xx_device_pcm0 = {
	.name		  = "samsung-pcm",
	.id		  = 0,
	.num_resources	  = ARRAY_SIZE(s3c64xx_pcm0_resource),
	.resource	  = s3c64xx_pcm0_resource,
	.dev = {
		.platform_data = &s3c_pcm_pdata,
	},
};
EXPORT_SYMBOL(s3c64xx_device_pcm0);

static struct resource s3c64xx_pcm1_resource[] = {
	[0] = {
		.start = S5P64XX_PA_PCM1,
		.end   = S5P64XX_PA_PCM1 + S5P_SZ_PCM1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = DMACH_PCM1_TX,
		.end   = DMACH_PCM1_TX,
		.flags = IORESOURCE_DMA,
	},
	[2] = {
		.start = DMACH_PCM1_RX,
		.end   = DMACH_PCM1_RX,
		.flags = IORESOURCE_DMA,
	},
};

struct platform_device s3c64xx_device_pcm1 = {
	.name		  = "samsung-pcm",
	.id		  = 1,
	.num_resources	  = ARRAY_SIZE(s3c64xx_pcm1_resource),
	.resource	  = s3c64xx_pcm1_resource,
	.dev = {
		.platform_data = &s3c_pcm_pdata,
	},
};
EXPORT_SYMBOL(s3c64xx_device_pcm1);

static struct resource s3c64xx_pcm2_resource[] = {
	[0] = {
		.start = S5P64XX_PA_PCM2,
		.end   = S5P64XX_PA_PCM2 + S5P_SZ_PCM2,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = DMACH_PCM2_TX,
		.end   = DMACH_PCM2_TX,
		.flags = IORESOURCE_DMA,
	},
	[2] = {
		.start = DMACH_PCM2_RX,
		.end   = DMACH_PCM2_RX,
		.flags = IORESOURCE_DMA,
	},
};

struct platform_device s3c64xx_device_pcm2 = {
	.name		  = "samsung-pcm",
	.id		  = 2,
	.num_resources	  = ARRAY_SIZE(s3c64xx_pcm2_resource),
	.resource	  = s3c64xx_pcm2_resource,
	.dev = {
		.platform_data = &s3c_pcm_pdata,
	},
};
EXPORT_SYMBOL(s3c64xx_device_pcm2);
#if 0 /*No ac97 in 6450*/
/* AC97 Controller platform devices */

static int s3c64xx_ac97_cfg_gpio(struct platform_device *pdev)
{
	s3c_gpio_cfgpin(S5PV210_GPC0(0), (0x4)<<0);
	s3c_gpio_cfgpin(S5PV210_GPC0(1), (0x4)<<4);
	s3c_gpio_cfgpin(S5PV210_GPC0(2), (0x4)<<8);
	s3c_gpio_cfgpin(S5PV210_GPC0(3), (0x4)<<12);
	s3c_gpio_cfgpin(S5PV210_GPC0(4), (0x4)<<16);

	return 0;
}

static struct resource s3c64xx_ac97_resource[] = {
	[0] = {
		.start = S5PV210_PA_AC97,
		.end   = S5PV210_PA_AC97 + 0x100 - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = DMACH_AC97_PCMOUT,
		.end   = DMACH_AC97_PCMOUT,
		.flags = IORESOURCE_DMA,
	},
	[2] = {
		.start = DMACH_AC97_PCMIN,
		.end   = DMACH_AC97_PCMIN,
		.flags = IORESOURCE_DMA,
	},
	[3] = {
		.start = DMACH_AC97_MICIN,
		.end   = DMACH_AC97_MICIN,
		.flags = IORESOURCE_DMA,
	},
	[4] = {
		.start = IRQ_AC97,
		.end   = IRQ_AC97,
		.flags = IORESOURCE_IRQ,
	},
};

static struct s3c_audio_pdata s3c_ac97_pdata = {
	.cfg_gpio = s3c64xx_ac97_cfg_gpio,
};

static u64 s3c64xx_ac97_dmamask = DMA_BIT_MASK(32);

struct platform_device s3c64xx_device_ac97 = {
	.name		  = "s3c-ac97",
	.id		  = -1,
	.num_resources	  = ARRAY_SIZE(s3c64xx_ac97_resource),
	.resource	  = s3c64xx_ac97_resource,
	.dev = {
		.platform_data = &s3c_ac97_pdata,
		.dma_mask = &s3c64xx_ac97_dmamask,
		.coherent_dma_mask = DMA_BIT_MASK(32),
	},
};
EXPORT_SYMBOL(s3c64xx_device_ac97);
#endif
