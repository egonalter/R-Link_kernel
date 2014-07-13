/* linux/arch/arm/plat-s5p/devs.c
 *
 * Copyright (c) 2009 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * Base S5P resource and device definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/dm9000.h>
#include <linux/platform_device.h>

#include <asm/mach/arch.h>
#include <asm/mach/irq.h>
#include <mach/hardware.h>
#include <mach/map.h>
#include <mach/adc.h>

#include <plat/devs.h>
#include <plat/irqs.h>
#include <plat/fb.h>
#include <plat/fimc.h>
#include <plat/csis.h>

/* Android Gadget */
#include <linux/usb/android_composite.h>
#define S3C_VENDOR_ID		   0x18d1
#define S3C_PRODUCT_ID		   0x0002
#define S3C_ADB_PRODUCT_ID	   0x4E22
#define MAX_USB_SERIAL_NUM	   17
#define S3C_RNDIS_PRODUCT_ID	   0x4E23
#define S3C_RNDIS_ADB_PRODUCT_ID   0x4E24


static char *usb_functions_ums[] = {
	"usb_mass_storage",
};

static char *usb_functions_rndis[] = {
	"rndis",
};

static char *usb_functions_rndis_adb[] = {
	"rndis",
	"adb",
};
static char *usb_functions_ums_adb[] = {
	"usb_mass_storage",
	"adb",
};

static char *usb_functions_all[] = {
#ifdef CONFIG_USB_ANDROID_RNDIS
	"rndis",
#endif
	"usb_mass_storage",
	"adb",
#ifdef CONFIG_USB_ANDROID_ACM
	"acm",
#endif
};
#if 1 
static struct android_usb_product usb_products[] = {
	{
		.product_id	= S3C_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_ums),
		.functions	= usb_functions_ums,
	},
	{
		.product_id	= S3C_ADB_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_ums_adb),
		.functions	= usb_functions_ums_adb,
	},
	
	{
		.product_id	= S3C_RNDIS_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_rndis),
		.functions	= usb_functions_rndis,
	},
	{
		.product_id	= S3C_RNDIS_ADB_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_rndis_adb),
		.functions	= usb_functions_rndis_adb,
	},

	
};
// serial number should be changed as real device for commercial release
static char device_serial[MAX_USB_SERIAL_NUM]="0123456789ABCDEF";
/* standard android USB platform data */

// Information should be changed as real product for commercial release
static struct android_usb_platform_data android_usb_pdata = {
	.vendor_id		= S3C_VENDOR_ID,
	.product_id		= S3C_PRODUCT_ID,
	.manufacturer_name	= "Android",//"Samsung",
	.product_name		= "Android",//"Samsung SMDKV210",
	.serial_number		= device_serial,
	.num_products 		= ARRAY_SIZE(usb_products),
	.products 		= usb_products,
	.num_functions 		= ARRAY_SIZE(usb_functions_all),
	.functions 		= usb_functions_all,
};
#endif
struct platform_device s3c_device_android_usb = {
	.name	= "android_usb",
	.id	= -1,
	.dev	= {
		.platform_data	= &android_usb_pdata,
	},
};
EXPORT_SYMBOL(s3c_device_android_usb);
#if 1
static struct usb_mass_storage_platform_data ums_pdata = {
	.vendor			= "Android   ",//"Samsung",
	.product		= "UMS Composite",//"SMDKV210",
	.release		= 1,
};
#endif

static struct usb_ether_platform_data rndis_pdata = {
        /* ethaddr is filled by board_serialno_setup */
        .vendorID       = 0x18d1,
        .vendorDescr    = "Samsung",
};

struct platform_device s3c_device_rndis = {
        .name   = "rndis",
        .id     = -1,
        .dev    = {
                .platform_data = &rndis_pdata,
        },
};
EXPORT_SYMBOL(s3c_device_rndis);

struct platform_device s3c_device_usb_mass_storage= {
	.name	= "usb_mass_storage",
	.id	= -1,
	.dev	= {
		.platform_data = &ums_pdata,
	},
};
EXPORT_SYMBOL(s3c_device_usb_mass_storage);

// RTC 

static struct resource s5p_rtc_resource[] = {
        [0] = {
                .start = S5P_PA_RTC,
                .end   = S5P_PA_RTC + 0xff,
                .flags = IORESOURCE_MEM,
        },
        [1] = {
                .start = IRQ_RTC_ALARM,
                .end   = IRQ_RTC_ALARM,
                .flags = IORESOURCE_IRQ,
        },
        [2] = {
                .start = IRQ_RTC_TIC,
                .end   = IRQ_RTC_TIC,
                .flags = IORESOURCE_IRQ
        }
};

struct platform_device s5p_device_rtc = {
        .name             = "s3c2410-rtc",
        .id               = -1,
        .num_resources    = ARRAY_SIZE(s5p_rtc_resource),
        .resource         = s5p_rtc_resource,
};

/* WATCHDOG TIMER*/
static struct resource s3c_wdt_resource[] = {
	[0] = {
		.start = S5P6450_PA_WDT,
		.end = S5P6450_PA_WDT + 0xff,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_WDT,
		.end = IRQ_WDT,
		.flags = IORESOURCE_IRQ,
	},
};

struct platform_device s3c_device_wdt = {
	.name = "s3c2410-wdt",
	.id = -1,
	.num_resources = ARRAY_SIZE(s3c_wdt_resource),
	.resource = s3c_wdt_resource,
};
EXPORT_SYMBOL(s3c_device_wdt);


/* USB Device (OTG hcd)*/

static struct resource s3c_usb_otghcd_resource[] = {
        [0] = {
                .start = S5P_PA_OTG,
                .end   = S5P_PA_OTG + S5P_SZ_OTG - 1,
                .flags = IORESOURCE_MEM,
        },
        [1] = {
                .start = IRQ_OTG,
                .end   = IRQ_OTG,
                .flags = IORESOURCE_IRQ,
        }
};

static u64 s3c_device_usb_otghcd_dmamask = 0xffffffffUL;

struct platform_device s3c_device_usb_otghcd = {
        .name           = "s3c_otghcd",
        .id             = -1,
        .num_resources  = ARRAY_SIZE(s3c_usb_otghcd_resource),
        .resource       = s3c_usb_otghcd_resource,
        .dev              = {
                .dma_mask = &s3c_device_usb_otghcd_dmamask,
                .coherent_dma_mask = 0xffffffffUL
        }
};

/* FIMG-2D controller */
static struct resource s3c_g2d_resource[] = {
        [0] = {
                .start  = S5P64XX_PA_G2D,
                .end    = S5P64XX_PA_G2D + S5P64XX_SZ_G2D - 1,
                .flags  = IORESOURCE_MEM,
        },
        [1] = {
                .start  = IRQ_2D,
                .end    = IRQ_2D,
                .flags  = IORESOURCE_IRQ,
        }
};      

struct platform_device s3c_device_g2d = {
        .name           = "sec-g2d",
        .id             = -1,
        .num_resources  = ARRAY_SIZE(s3c_g2d_resource),
        .resource       = s3c_g2d_resource
};      
EXPORT_SYMBOL(s3c_device_g2d);


#ifdef CONFIG_USB_SUPPORT
#ifdef CONFIG_USB_ARCH_HAS_EHCI
 /* USB Host Controlle EHCI registrations */

static struct resource s3c_usb__ehci_resource[] = {
	[0] = {
		.start = S5P_PA_USB_EHCI ,
		.end   = S5P_PA_USB_EHCI  + S5P_SZ_USB_EHCI - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_UHOST,
		.end   = IRQ_UHOST,
		.flags = IORESOURCE_IRQ,
	}
};

static u64 s3c_device_usb_ehci_dmamask = 0xffffffffUL;

struct platform_device s3c_device_usb_ehci = {
	.name		= "s5pv210-ehci",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s3c_usb__ehci_resource),
	.resource	= s3c_usb__ehci_resource,
	.dev		= {
		.dma_mask = &s3c_device_usb_ehci_dmamask,
		.coherent_dma_mask = 0xffffffffUL
	}
};
EXPORT_SYMBOL(s3c_device_usb_ehci);
#endif /* CONFIG_USB_ARCH_HAS_EHCI */

 /* USB Host Controlle OHCI registrations */
#ifdef CONFIG_USB_ARCH_HAS_OHCI
 static struct resource s3c_usb__ohci_resource[] = {
	[0] = {
		.start = S5P_PA_USB_OHCI ,
		.end   = S5P_PA_USB_OHCI  + S5P_SZ_USB_OHCI - 1,
		.flags = IORESOURCE_MEM,
	},
	[1] = {
		.start = IRQ_UHOST,
		.end   = IRQ_UHOST,
		.flags = IORESOURCE_IRQ,
	}
};

static u64 s3c_device_usb_ohci_dmamask = 0xffffffffUL;

struct platform_device s3c_device_usb_ohci = {
	.name		= "s5pv210-ohci",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s3c_usb__ohci_resource),
	.resource	= s3c_usb__ohci_resource,
	.dev		= {
		.dma_mask = &s3c_device_usb_ohci_dmamask,
		.coherent_dma_mask = 0xffffffffUL
	}
};
EXPORT_SYMBOL(s3c_device_usb_ohci);
#endif /* CONFIG_USB_ARCH_HAS_OHCI */
/* USB Device (Gadget)*/

static struct resource s3c_usbgadget_resource[] = {
	[0] = {
		.start = S5P_PA_OTG,
		.end   = S5P_PA_OTG+S5P_SZ_OTG-1,
		.flags = IORESOURCE_MEM,
		},
	[1] = {
		.start = IRQ_OTG,
		.end   = IRQ_OTG,
		.flags = IORESOURCE_IRQ,
	}
};

struct platform_device s3c_device_usbgadget = {
	.name		= "s3c-usbgadget",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s3c_usbgadget_resource),
	.resource	= s3c_usbgadget_resource,
};
EXPORT_SYMBOL(s3c_device_usbgadget);

#endif 

#if 0

/* JPEG controller  */
static struct resource s3c_jpeg_resource[] = {
        [0] = {
                .start = S5PV210_PA_JPEG,
                .end   = S5PV210_PA_JPEG + S5PV210_SZ_JPEG - 1,
                .flags = IORESOURCE_MEM,
        },
        [1] = {
                .start = IRQ_JPEG,
                .end   = IRQ_JPEG,
                .flags = IORESOURCE_IRQ,
        }

};

struct platform_device s3c_device_jpeg = {
        .name             = "s3c-jpg",
        .id               = -1,
        .num_resources    = ARRAY_SIZE(s3c_jpeg_resource),
        .resource         = s3c_jpeg_resource,
};
EXPORT_SYMBOL(s3c_device_jpeg);
#endif


#if defined (CONFIG_CPU_S5PV210_EVT1 )
/* rotator interface */
static struct resource s5p_rotator_resource[] = {
        [0] = {
                .start = S5PV210_PA_ROTATOR,
                .end   = S5PV210_PA_ROTATOR + S5PV210_SZ_ROTATOR - 1,
                .flags = IORESOURCE_MEM,
        },
        [1] = {
                .start = IRQ_ROTATOR,
                .end   = IRQ_ROTATOR,
                .flags = IORESOURCE_IRQ,
        }
};

struct platform_device s5p_device_rotator = {
        .name             = "s5p-rotator",
        .id               = -1,
        .num_resources    = ARRAY_SIZE(s5p_rotator_resource),
        .resource         = s5p_rotator_resource
};
EXPORT_SYMBOL(s5p_device_rotator);
#endif
#if 1 
/* Keypad interface */

struct  platform_device s3c_device_keypad = {
        .name = "s3c-keypad",
        .id = -1,
        .dev.platform_data = NULL,
//      .num_resources = ARRAY_SIZE(s3c_keypad_resource),
     };
EXPORT_SYMBOL(s3c_device_keypad);
//




#endif
/* ADC */
static struct resource s3c_adc_resource[] = {
        [0] = {
                .start = S5P64XX_PA_ADC,
                .end   = S5P64XX_PA_ADC + SZ_4K - 1,
                .flags = IORESOURCE_MEM,
        },
        [1] = {
                .start = IRQ_ADC,
                .end   = IRQ_ADC,
                .flags = IORESOURCE_IRQ,
        }

};

struct platform_device s3c_device_adc = {
        .name             = "s3c-adc",
        .id               = -1,
        .num_resources    = ARRAY_SIZE(s3c_adc_resource),
        .resource         = s3c_adc_resource,
};

void __init s3c_adc_set_platdata(struct s3c_adc_mach_info *pd)
{
        struct s3c_adc_mach_info *npd;

        npd = kmalloc(sizeof(*npd), GFP_KERNEL);
        if (npd) {
                memcpy(npd, pd, sizeof(*npd));
                s3c_device_adc.dev.platform_data = npd;
        } else {
                printk(KERN_ERR "no memory for ADC platform data\n");
        }
}
#if defined (CONFIG_CPU_S5PV210_EVT1)
/* TVOUT interface */
static struct resource s5p_tvout_resources[] = {
        [0] = {
                .start  = S5P_PA_TVENC,
                .end    = S5P_PA_TVENC + S5P_SZ_TVENC - 1,
                .flags  = IORESOURCE_MEM,
        },
        [1] = {
                .start  = S5P_PA_VP,
                .end    = S5P_PA_VP + S5P_SZ_VP - 1,
                .flags  = IORESOURCE_MEM,
        },
        [2] = {
                .start  = S5P_PA_MIXER,
                .end    = S5P_PA_MIXER + S5P_SZ_MIXER - 1,
                .flags  = IORESOURCE_MEM,
        },
        [3] = {
                .start  = S5P_PA_HDMI,
                .end    = S5P_PA_HDMI + S5P_SZ_HDMI - 1,
                .flags  = IORESOURCE_MEM,
        },
        [4] = {
                .start  = S5P_I2C_HDMI_PHY,
                .end    = S5P_I2C_HDMI_PHY + S5P_I2C_HDMI_SZ_PHY - 1,
                .flags  = IORESOURCE_MEM,
        },
        [5] = {
                .start  = IRQ_MIXER,
                .end    = IRQ_MIXER,
                .flags  = IORESOURCE_IRQ,
        },
        [6] = {
                .start  = IRQ_HDMI,
                .end    = IRQ_HDMI,
                .flags  = IORESOURCE_IRQ,
        },
        [7] = {
                .start  = IRQ_TVENC,
                .end    = IRQ_TVENC,
                .flags  = IORESOURCE_IRQ,
        },
        [8] = {
                .start  = IRQ_EINT5,
                .end    = IRQ_EINT5,
                .flags  = IORESOURCE_IRQ,
        }
};

struct platform_device s5p_device_tvout = {
	.name           = "s5p-tvout",
	.id             = -1,
	.num_resources  = ARRAY_SIZE(s5p_tvout_resources),
	.resource       = s5p_tvout_resources,
};
EXPORT_SYMBOL(s5p_device_tvout);

/* CEC */
static struct resource s5p_cec_resources[] = {
	[0] = {
		.start  = S5P_PA_CEC,
		.end    = S5P_PA_CEC + S5P_SZ_CEC - 1,
		.flags  = IORESOURCE_MEM,
	},
	[1] = {
		.start  = IRQ_CEC,
		.end    = IRQ_CEC,
		.flags  = IORESOURCE_IRQ,
	},
};

struct platform_device s5p_device_cec = {
	.name           = "s5p-cec",
	.id             = -1,
	.num_resources  = ARRAY_SIZE(s5p_cec_resources),
	.resource       = s5p_cec_resources,
};
EXPORT_SYMBOL(s5p_device_cec);
#endif

/* HPD */
struct platform_device s5p_device_hpd = {
	.name           = "s5p-hpd",
	.id             = -1,
};
EXPORT_SYMBOL(s5p_device_hpd);

/* FIMG-2D controller */
static struct resource s5p_g2d_resource[] = {
	[0] = {
		.start	= S5P64XX_PA_G2D,
		.end	= S5P64XX_PA_G2D + S5P64XX_SZ_G2D - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_2D,
		.end	= IRQ_2D,
		.flags	= IORESOURCE_IRQ,
	}
};

struct platform_device s5p_device_g2d = {
	.name		= "s5p-g2d",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s5p_g2d_resource),
	.resource	= s5p_g2d_resource
};
EXPORT_SYMBOL(s5p_device_g2d);

/* LCD Controller */

static struct resource s3c_lcd_resource[] = {
        [0] = {
                .start = S5P64XX_PA_LCD,
                .end   = S5P64XX_PA_LCD + SZ_1M - 1,
                .flags = IORESOURCE_MEM,
        },
        [1] = {
                .start = IRQ_DISPCON1,
                .end   = IRQ_DISPCON2,
                .flags = IORESOURCE_IRQ,
        }
};

static u64 s3c_device_lcd_dmamask = 0xffffffffUL;

struct platform_device s3c_device_lcd = {
        .name             = "s3c-lcd",
        .id               = -1,
        .num_resources    = ARRAY_SIZE(s3c_lcd_resource),
        .resource         = s3c_lcd_resource,
        .dev              = {
                .dma_mask               = &s3c_device_lcd_dmamask,
                .coherent_dma_mask      = 0xffffffffUL
        }
};

#ifdef CONFIG_VIDEO_FIMC

static struct resource s3c_fimc0_resource[] = {
	[0] = {
		.start	= S5P64XX_PA_FIMC0,
		.end	= S5P64XX_PA_FIMC0 + S5P64XX_SZ_FIMC0 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_FIMC0,
		.end	= IRQ_FIMC0,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device s3c_device_fimc0 = {
	.name		= "s3c-fimc",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(s3c_fimc0_resource),
	.resource	= s3c_fimc0_resource,
};

static struct s3c_platform_fimc default_fimc0_data __initdata = {
	.default_cam	= CAMERA_PAR_A,
	.hw_ver		= 0x43,
};

void __init s3c_fimc0_set_platdata(struct s3c_platform_fimc *pd)
{
	struct s3c_platform_fimc *npd;

	if (!pd)
		pd = &default_fimc0_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_fimc), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
	else {
		if (!npd->cfg_gpio)
			npd->cfg_gpio = s3c_fimc0_cfg_gpio;

		if (!npd->clk_on)
			npd->clk_on = s3c_fimc_clk_on;

		npd->hw_ver		= 0x43;
		s3c_device_fimc0.dev.platform_data = npd;
	}
}

#if defined(CONFIG_CPU_S5PV210_EVT1)
static struct resource s3c_fimc1_resource[] = {
	[0] = {
		.start	= S5P_PA_FIMC1,
		.end	= S5P_PA_FIMC1 + S5P_SZ_FIMC1 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_FIMC1,
		.end	= IRQ_FIMC1,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device s3c_device_fimc1 = {
	.name		= "s3c-fimc",
	.id		= 1,
	.num_resources	= ARRAY_SIZE(s3c_fimc1_resource),
	.resource	= s3c_fimc1_resource,
};

static struct s3c_platform_fimc default_fimc1_data __initdata = {
	.default_cam	= CAMERA_PAR_A,
#if defined(CONFIG_CPU_S5PV210_EVT1)
	.hw_ver	= 0x50,
#else
	.hw_ver	= 0x43,
#endif
};

void __init s3c_fimc1_set_platdata(struct s3c_platform_fimc *pd)
{
	struct s3c_platform_fimc *npd;

	if (!pd)
		pd = &default_fimc1_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_fimc), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
	else {
		if (!npd->cfg_gpio)
			npd->cfg_gpio = s3c_fimc1_cfg_gpio;

		if (!npd->clk_on)
			npd->clk_on = s3c_fimc_clk_on;

#if defined(CONFIG_CPU_S5PV210_EVT1)
		npd->hw_ver = 0x50;
#else
		npd->hw_ver = 0x43;
#endif

		s3c_device_fimc1.dev.platform_data = npd;
	}
}

static struct resource s3c_fimc2_resource[] = {
	[0] = {
		.start	= S5P_PA_FIMC2,
		.end	= S5P_PA_FIMC2 + S5P_SZ_FIMC2 - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_FIMC2,
		.end	= IRQ_FIMC2,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device s3c_device_fimc2 = {
	.name		= "s3c-fimc",
	.id		= 2,
	.num_resources	= ARRAY_SIZE(s3c_fimc2_resource),
	.resource	= s3c_fimc2_resource,
};

static struct s3c_platform_fimc default_fimc2_data __initdata = {
	.default_cam	= CAMERA_PAR_A,
	.hw_ver		= 0x43,
};

void __init s3c_fimc2_set_platdata(struct s3c_platform_fimc *pd)
{
	struct s3c_platform_fimc *npd;

	if (!pd)
		pd = &default_fimc2_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_fimc), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);
	else {
		if (!npd->cfg_gpio)
			npd->cfg_gpio = s3c_fimc2_cfg_gpio;

		if (!npd->clk_on)
			npd->clk_on = s3c_fimc_clk_on;

		s3c_device_fimc2.dev.platform_data = npd;
	}
}
#endif
static struct resource s3c_ipc_resource[] = {
	[0] = {
		.start	= S5P_PA_IPC,
		.end	= S5P_PA_IPC + S5P_SZ_IPC - 1,
		.flags	= IORESOURCE_MEM,
	},
};

struct platform_device s3c_device_ipc = {
	.name		= "s3c-ipc",
	.id		= -1,
	.num_resources	= ARRAY_SIZE(s3c_ipc_resource),
	.resource	= s3c_ipc_resource,
};

static struct resource s3c_csis_resource[] = {
	[0] = {
		.start	= S5P_PA_CSIS,
		.end	= S5P_PA_CSIS + S5P_SZ_CSIS - 1,
		.flags	= IORESOURCE_MEM,
	},
	[1] = {
		.start	= IRQ_MIPICSI,
		.end	= IRQ_MIPICSI,
		.flags	= IORESOURCE_IRQ,
	},
};

struct platform_device s3c_device_csis = {
	.name		= "s3c-csis",
	.id		= 0,
	.num_resources	= ARRAY_SIZE(s3c_csis_resource),
	.resource	= s3c_csis_resource,
};

static struct s3c_platform_csis default_csis_data __initdata = {
	.srclk_name	= "mout_mpll",
	.clk_name	= "sclk_csis",
	.clk_rate	= 166000000,
};

void __init s3c_csis_set_platdata(struct s3c_platform_csis *pd)
{
	struct s3c_platform_csis *npd;

	if (!pd)
		pd = &default_csis_data;

	npd = kmemdup(pd, sizeof(struct s3c_platform_csis), GFP_KERNEL);
	if (!npd)
		printk(KERN_ERR "%s: no memory for platform data\n", __func__);

	npd->cfg_gpio = s3c_csis_cfg_gpio;
	npd->cfg_phy_global = s3c_csis_cfg_phy_global;

	s3c_device_csis.dev.platform_data = npd;
}
#endif

