/*
 * linux/arch/arm/mach-omap2/usb-musb.c
 *
 * This file will contain the board specific details for the
 * MENTOR USB OTG controller on OMAP3430
 *
 * Copyright (C) 2007-2008 Texas Instruments
 * Copyright (C) 2008 Nokia Corporation
 * Author: Vikram Pandita
 *
 * Generalization by:
 * Felipe Balbi <felipe.balbi@nokia.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/types.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/clk.h>
#include <linux/dma-mapping.h>
#include <linux/io.h>

#include <linux/usb/android_composite.h>
#include <linux/usb/musb.h>

#include <asm/sizes.h>
#include <asm/mach-types.h>

#include <mach/hardware.h>
#include <mach/irqs.h>
#include <mach/mfd_feat.h>
#include <plat/mux.h>
#include <plat/usb.h>
#include <plat/omap-pm.h>

#define OTG_SYSCONFIG	   0x404
#define OTG_SYSC_SOFTRESET BIT(1)
#define OTG_SYSSTATUS     0x408
#define OTG_SYSS_RESETDONE BIT(0)

static struct platform_device dummy_pdev = {
	.dev = {
		.bus = &platform_bus_type,
	},
};

static void __iomem *otg_base;
static struct clk *otg_clk;

#define  MIDLEMODE       12      /* bit position */
#define  FORCESTDBY      (0 << MIDLEMODE)
#define  NOSTDBY         (1 << MIDLEMODE)
#define  SMARTSTDBY      (2 << MIDLEMODE)
#define  SIDLEMODE       3       /* bit position */
#define  FORCEIDLE       (0 << SIDLEMODE)
#define  NOIDLE          (1 << SIDLEMODE)
#define  SMARTIDLE       (2 << SIDLEMODE)

#define MFD_SANTIAGO(_feat)     MFD_FEAT_INIT(MACH_TYPE_SANTIAGO, 0, _feat)

static void __init usb_musb_pm_init(void)
{
	struct device *dev = &dummy_pdev.dev;

	if (!cpu_is_omap34xx())
		return;

	otg_base = ioremap(OMAP34XX_HSUSB_OTG_BASE, SZ_4K);
	if (WARN_ON(!otg_base))
		return;

	dev_set_name(dev, "musb_hdrc");
	otg_clk = clk_get(dev, "ick");

	if (otg_clk && clk_enable(otg_clk)) {
		printk(KERN_WARNING
			"%s: Unable to enable clocks for MUSB, "
			"cannot reset.\n",  __func__);
	} else {
		/* Reset OTG controller. After reset, it will be in
		 * force-idle, force-standby mode. */
		__raw_writel(OTG_SYSC_SOFTRESET, otg_base + OTG_SYSCONFIG);

		while (!(OTG_SYSS_RESETDONE &
					__raw_readl(otg_base + OTG_SYSSTATUS)))
			cpu_relax();
	}

	if (otg_clk)
		clk_disable(otg_clk);
}

#ifdef CONFIG_ANDROID

#ifdef CONFIG_ARCH_OMAP4
#define DIE_ID_REG_BASE 		(L4_44XX_PHYS + 0x2000)
#define DIE_ID_REG_OFFSET		0x200
#else
#define DIE_ID_REG_BASE 		(L4_WK_34XX_PHYS + 0xA000)
#define DIE_ID_REG_OFFSET		0x218
#endif /* CONFIG_ARCH_OMAP4 */

#define MAX_USB_SERIAL_NUM		17

#define TOMTOM_VENDOR_ID		0x1390
#define OMAP_VENDOR_ID			0x0451

#define TOMTOM_UMS_PRODUCT_ID		0xD100
#define TOMTOM_ADB_PRODUCT_ID		0xD101
#define TOMTOM_UMS_ADB_PRODUCT_ID	0xD102
#define TOMTOM_RNDIS_PRODUCT_ID		0x5454
#define TOMTOM_RNDIS_ADB_PRODUCT_ID	0x5454
#define TOMTOM_ACM_PRODUCT_ID		0xD105
#define TOMTOM_ACM_ADB_PRODUCT_ID	0xD106
#define TOMTOM_ACM_UMS_ADB_PRODUCT_ID	0xD107

#define OMAP_UMS_PRODUCT_ID		0xD100
#define OMAP_ADB_PRODUCT_ID		0xD101
#define OMAP_UMS_ADB_PRODUCT_ID		0xD102
#define OMAP_RNDIS_PRODUCT_ID		0xD103
#define OMAP_RNDIS_ADB_PRODUCT_ID	0xD104
#define OMAP_ACM_PRODUCT_ID		0xD105
#define OMAP_ACM_ADB_PRODUCT_ID		0xD106
#define OMAP_ACM_UMS_ADB_PRODUCT_ID	0xD107
#define OMAP_ECM_PRODUCT_ID		0xD108
#define OMAP_YUCATAN_PRODUCT_ID		0xD200

#define MFD_ECM_PRODUCT_ID		0x0007
#define MFD_ADB_PRODUCT_ID		0xD101
#define MFD_UMS_ADB_PRODUCT_ID		0xD102
#define MFD_RNDIS_PRODUCT_ID		0xD103
#define MFD_RNDIS_ADB_PRODUCT_ID	0xD104
#define MFD_ACM_PRODUCT_ID		0xD105
#define MFD_ACM_ADB_PRODUCT_ID		0xD106
#define MFD_ACM_UMS_ADB_PRODUCT_ID	0xD107
#define MFD_UMS_PRODUCT_ID		0xD108
#define MFD_YUCATAN_PRODUCT_ID		0xD200

static char device_serial[MAX_USB_SERIAL_NUM];

static char *usb_functions_ums[] = {
	"usb_mass_storage",
};

static char *usb_functions_adb[] = {
	"adb",
};

static char *usb_functions_ums_adb[] = {
	"usb_mass_storage",
	"adb",
};

static char *usb_functions_rndis[] = {
	"rndis",
};

static char *usb_functions_rndis_adb[] = {
	"rndis",
	"adb",
};

static char *usb_functions_acm[] = {
	"acm",
};

static char *usb_functions_acm_adb[] = {
	"acm",
	"adb",
};

static char *usb_functions_acm_ums_adb[] = {
	"acm",
	"usb_mass_storage",
	"adb",
};

static char *usb_functions_yucatan[] = {
	"fadc",
	"fvdc",
	"fhid",
};

static char *usb_functions_ecm[] = {
	"cdc_ethernet",
};
static char *usb_functions_all[] = {
#ifdef CONFIG_USB_ANDROID_ECM
	"cdc_ethernet",
#endif
#ifdef CONFIG_USB_ANDROID_RNDIS
	"rndis",
#endif
#ifdef CONFIG_USB_ANDROID_ACM
	"acm",
#endif
#ifdef CONFIG_USB_ANDROID_MASS_STORAGE
	"usb_mass_storage",
#endif
#ifdef CONFIG_USB_ANDROID_ADB
	"adb",
#endif
#ifdef CONFIG_USB_ANDROID_VDC
	"fvdc",
#endif
#ifdef CONFIG_USB_ANDROID_HIDC
	"fhid",
#endif
#ifdef CONFIG_USB_ANDROID_ADC
	"fadc",
#endif
};

static struct android_usb_product usb_products_santiago[] = {
	{
		.product_id     = TOMTOM_UMS_PRODUCT_ID,
		.num_functions  = ARRAY_SIZE(usb_functions_ums),
		.functions      = usb_functions_ums,
	},
	{
		.product_id     = TOMTOM_ADB_PRODUCT_ID,
		.num_functions  = ARRAY_SIZE(usb_functions_adb),
		.functions      = usb_functions_adb,
	},
	{
		.product_id     = TOMTOM_UMS_ADB_PRODUCT_ID,
		.num_functions  = ARRAY_SIZE(usb_functions_ums_adb),
		.functions      = usb_functions_ums_adb,
	},
	{
		.product_id     = TOMTOM_RNDIS_PRODUCT_ID,
		.num_functions  = ARRAY_SIZE(usb_functions_rndis),
		.functions      = usb_functions_rndis,
	},
	{
		.product_id     = TOMTOM_RNDIS_ADB_PRODUCT_ID,
		.num_functions  = ARRAY_SIZE(usb_functions_rndis_adb),
		.functions      = usb_functions_rndis_adb,
	},
	{
		.product_id     = TOMTOM_ACM_PRODUCT_ID,
		.num_functions  = ARRAY_SIZE(usb_functions_acm),
		.functions      = usb_functions_acm,
	},
	{
		.product_id     = TOMTOM_ACM_ADB_PRODUCT_ID,
		.num_functions  = ARRAY_SIZE(usb_functions_acm_adb),
		.functions      = usb_functions_acm_adb,
	},
	{
		.product_id     = TOMTOM_ACM_UMS_ADB_PRODUCT_ID,
		.num_functions  = ARRAY_SIZE(usb_functions_acm_ums_adb),
		.functions      = usb_functions_acm_ums_adb,
	},
};

static struct android_usb_product usb_products_ti[] = {
	{
		.product_id     = OMAP_UMS_PRODUCT_ID,
		.num_functions  = ARRAY_SIZE(usb_functions_ums),
		.functions      = usb_functions_ums,
	},
	{
		.product_id     = OMAP_ADB_PRODUCT_ID,
		.num_functions  = ARRAY_SIZE(usb_functions_adb),
		.functions      = usb_functions_adb,
	},
	{
		.product_id     = OMAP_UMS_ADB_PRODUCT_ID,
		.num_functions  = ARRAY_SIZE(usb_functions_ums_adb),
		.functions      = usb_functions_ums_adb,
	},
	{
		.product_id     = OMAP_RNDIS_PRODUCT_ID,
		.num_functions  = ARRAY_SIZE(usb_functions_rndis),
		.functions      = usb_functions_rndis,
	},
	{
		.product_id     = OMAP_RNDIS_ADB_PRODUCT_ID,
		.num_functions  = ARRAY_SIZE(usb_functions_rndis_adb),
		.functions      = usb_functions_rndis_adb,
	},
	{
		.product_id     = OMAP_ACM_PRODUCT_ID,
		.num_functions  = ARRAY_SIZE(usb_functions_acm),
		.functions      = usb_functions_acm,
	},
	{
		.product_id     = OMAP_ACM_ADB_PRODUCT_ID,
		.num_functions  = ARRAY_SIZE(usb_functions_acm_adb),
		.functions      = usb_functions_acm_adb,
	},
	{
		.product_id     = OMAP_ACM_UMS_ADB_PRODUCT_ID,
		.num_functions  = ARRAY_SIZE(usb_functions_acm_ums_adb),
		.functions      = usb_functions_acm_ums_adb,
	},
	{
		.product_id	= OMAP_YUCATAN_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_yucatan),
		.functions	= usb_functions_yucatan,
	},
	{
		.product_id	= OMAP_ECM_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_ecm),
		.functions	= usb_functions_ecm,
	},
};

static struct android_usb_product usb_products_mfd[] = {
	{
		.product_id     = MFD_UMS_PRODUCT_ID,
		.num_functions  = ARRAY_SIZE(usb_functions_ums),
		.functions      = usb_functions_ums,
	},
	{
		.product_id     = MFD_ADB_PRODUCT_ID,
		.num_functions  = ARRAY_SIZE(usb_functions_adb),
		.functions      = usb_functions_adb,
	},
	{
		.product_id     = MFD_UMS_ADB_PRODUCT_ID,
		.num_functions  = ARRAY_SIZE(usb_functions_ums_adb),
		.functions      = usb_functions_ums_adb,
	},
	{
		.product_id     = MFD_RNDIS_PRODUCT_ID,
		.num_functions  = ARRAY_SIZE(usb_functions_rndis),
		.functions      = usb_functions_rndis,
	},
	{
		.product_id     = MFD_RNDIS_ADB_PRODUCT_ID,
		.num_functions  = ARRAY_SIZE(usb_functions_rndis_adb),
		.functions      = usb_functions_rndis_adb,
	},
	{
		.product_id     = MFD_ACM_PRODUCT_ID,
		.num_functions  = ARRAY_SIZE(usb_functions_acm),
		.functions      = usb_functions_acm,
	},
	{
		.product_id     = MFD_ACM_ADB_PRODUCT_ID,
		.num_functions  = ARRAY_SIZE(usb_functions_acm_adb),
		.functions      = usb_functions_acm_adb,
	},
	{
		.product_id     = MFD_ACM_UMS_ADB_PRODUCT_ID,
		.num_functions  = ARRAY_SIZE(usb_functions_acm_ums_adb),
		.functions      = usb_functions_acm_ums_adb,
	},
	{
		.product_id	= MFD_YUCATAN_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_yucatan),
		.functions	= usb_functions_yucatan,
	},
	{
		.product_id	= MFD_ECM_PRODUCT_ID,
		.num_functions	= ARRAY_SIZE(usb_functions_ecm),
		.functions	= usb_functions_ecm,
	},
};

/* standard android USB platform data */
static struct android_usb_platform_data andusb_plat_santiago = {
	.vendor_id		= TOMTOM_VENDOR_ID,
	.product_id		= TOMTOM_UMS_PRODUCT_ID,
	.manufacturer_name      = "TomTom International B.V.",
	.product_name           = "Santiago",
	.serial_number          = device_serial,
	.num_products = ARRAY_SIZE(usb_products_santiago),
	.products = usb_products_santiago,
	.num_functions = ARRAY_SIZE(usb_functions_all),
	.functions = usb_functions_all,
};

static struct android_usb_platform_data andusb_plat_ti = {
	.vendor_id		= OMAP_VENDOR_ID,
	.product_id		= OMAP_UMS_PRODUCT_ID,
	.manufacturer_name      = "Texas Instruments Inc.",
	.product_name           = "OMAP-3/4",
	.serial_number          = device_serial,
	.num_products = ARRAY_SIZE(usb_products_ti),
	.products = usb_products_ti,
	.num_functions = ARRAY_SIZE(usb_functions_all),
	.functions = usb_functions_all,
};

static struct android_usb_platform_data andusb_plat_mfd = {
	.vendor_id		= TOMTOM_VENDOR_ID,
	.product_id		= MFD_ECM_PRODUCT_ID,
	.manufacturer_name      = "TomTom International B.V.",
	.product_name           = "MFD",
	.serial_number          = device_serial,
	.num_products = ARRAY_SIZE(usb_products_mfd),
	.products = usb_products_mfd,
	.num_functions = ARRAY_SIZE(usb_functions_all),
	.functions = usb_functions_all,
};

static const struct mfd_feat andusb_feats[] = {
	MFD_SANTIAGO(&andusb_plat_santiago),
	MFD_DEFAULT(&andusb_plat_mfd),
	MFD_DEFAULT(&andusb_plat_ti), /* never reached */
};

static struct platform_device androidusb_device = {
	.name   = "android_usb",
	.id     = -1,
};

#ifdef CONFIG_USB_ANDROID_RNDIS
static struct usb_ether_platform_data rndis_pdata = {
	/* ethaddr is filled by board_serialno_setup */
#ifdef CONFIG_MACH_SANTIAGO
	.vendorID       = TOMTOM_VENDOR_ID,
	.vendorDescr    = "TomTom International BV.",
#else	
	.vendorID       = OMAP_VENDOR_ID,
	.vendorDescr    = "Texas Instruments Inc.",
#endif	
	};

static struct platform_device rndis_device = {
	.name   = "rndis",
	.id     = -1,
	.dev    = {
		.platform_data = &rndis_pdata,
	},
};
#endif

static void usb_gadget_init(void)
{
	unsigned int val[4] = { 0 };
	unsigned int reg;
#ifdef CONFIG_USB_ANDROID_RNDIS
	int i;
	char *src;
#endif
	reg = DIE_ID_REG_BASE + DIE_ID_REG_OFFSET;

	if (cpu_is_omap44xx()) {
		val[0] = omap_readl(reg);
		val[1] = omap_readl(reg + 0x8);
		val[2] = omap_readl(reg + 0xC);
		val[3] = omap_readl(reg + 0x10);
	} else if (cpu_is_omap34xx()) {
		val[0] = omap_readl(reg);
		val[1] = omap_readl(reg + 0x4);
		val[2] = omap_readl(reg + 0x8);
		val[3] = omap_readl(reg + 0xC);
	}

	snprintf(device_serial, MAX_USB_SERIAL_NUM, "%08X%08X%08X%08X", val[3], val[2], val[1], val[0]);

#ifdef CONFIG_USB_ANDROID_RNDIS
	/* create a fake MAC address from our serial number.
	 * first byte is 0x02 to signify locally administered.
	 */
	rndis_pdata.ethaddr[0] = 0x02;
	src = device_serial;
	for (i = 0; *src; i++) {
		/* XOR the USB serial across the remaining bytes */
		rndis_pdata.ethaddr[i % (ETH_ALEN - 1) + 1] ^= *src++;
	}

	platform_device_register(&rndis_device);
#endif

	mfd_set_pdata(&androidusb_device, andusb_feats);
	platform_device_register(&androidusb_device);
}

#else

static void usb_gadget_init(void)
{
}

#endif /* CONFIG_ANDROID */

void usb_musb_disable_autoidle(void)
{
	__raw_writel(0, otg_base + OTG_SYSCONFIG);
}


void musb_disable_idle(int on)
{
	u32 reg;

	if (!cpu_is_omap34xx())
		return;

	reg = omap_readl(OMAP34XX_HSUSB_OTG_BASE + OTG_SYSCONFIG);

	if (on) {
		reg &= ~SMARTSTDBY;    /* remove possible smartstdby */
		reg |= NOSTDBY;        /* enable no standby */
		reg |= NOIDLE;        /* enable no idle */
	} else {
		reg &= ~NOSTDBY;          /* remove possible nostdby */
		reg &= ~NOIDLE;          /* remove possible noidle */
		reg |= SMARTSTDBY;        /* enable smart standby */
	}

	omap_writel(reg, OMAP34XX_HSUSB_OTG_BASE + OTG_SYSCONFIG);
}

#ifdef CONFIG_USB_MUSB_SOC

static struct resource musb_resources[] = {
	[0] = { /* start and end set dynamically */
		.flags	= IORESOURCE_MEM,
	},
	[1] = {	/* general IRQ */
		.start	= INT_243X_HS_USB_MC,
		.flags	= IORESOURCE_IRQ,
	},
	[2] = {	/* DMA IRQ */
		.start	= INT_243X_HS_USB_DMA,
		.flags	= IORESOURCE_IRQ,
	},
};

static int clk_on;

static int musb_set_clock(struct clk *clk, int state)
{
	if (state) {
		if (clk_on > 0)
			return -ENODEV;

		clk_enable(clk);
		clk_on = 1;
	} else {
		if (clk_on == 0)
			return -ENODEV;

		clk_disable(clk);
		clk_on = 0;
	}

	return 0;
}

static struct musb_hdrc_eps_bits musb_eps[] = {
	{	"ep1_tx", 10,	},
	{	"ep1_rx", 10,	},
	{	"ep2_tx", 9,	},
	{	"ep2_rx", 9,	},
	{	"ep3_tx", 3,	},
	{	"ep3_rx", 3,	},
	{	"ep4_tx", 3,	},
	{	"ep4_rx", 3,	},
	{	"ep5_tx", 3,	},
	{	"ep5_rx", 3,	},
	{	"ep6_tx", 3,	},
	{	"ep6_rx", 3,	},
	{	"ep7_tx", 3,	},
	{	"ep7_rx", 3,	},
	{	"ep8_tx", 2,	},
	{	"ep8_rx", 2,	},
	{	"ep9_tx", 2,	},
	{	"ep9_rx", 2,	},
	{	"ep10_tx", 2,	},
	{	"ep10_rx", 2,	},
	{	"ep11_tx", 2,	},
	{	"ep11_rx", 2,	},
	{	"ep12_tx", 2,	},
	{	"ep12_rx", 2,	},
	{	"ep13_tx", 2,	},
	{	"ep13_rx", 2,	},
	{	"ep14_tx", 2,	},
	{	"ep14_rx", 2,	},
	{	"ep15_tx", 2,	},
	{	"ep15_rx", 2,	},
};

static struct musb_hdrc_config musb_config = {
	.multipoint	= 1,
	.dyn_fifo	= 1,
	.soft_con	= 1,
	.dma		= 1,
	.num_eps	= 16,
	.dma_channels	= 7,
	.dma_req_chan	= (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3),
	.ram_bits	= 12,
	.eps_bits	= musb_eps,
};

static struct musb_hdrc_platform_data musb_plat = {
#ifdef CONFIG_USB_MUSB_OTG
	.mode		= MUSB_OTG,
#elif defined(CONFIG_USB_MUSB_HDRC_HCD)
	.mode		= MUSB_HOST,
#elif defined(CONFIG_USB_GADGET_MUSB_HDRC)
	.mode		= MUSB_PERIPHERAL,
#endif
	/* .clock is set dynamically */
	.set_clock	= musb_set_clock,
	.config		= &musb_config,

	/* REVISIT charge pump on TWL4030 can supply up to
	 * 100 mA ... but this value is board-specific, like
	 * "mode", and should be passed to usb_musb_init().
	 */
	.power		= 50,			/* up to 100 mA */
	.context_loss_counter = NULL,
#ifdef CONFIG_ARCH_OMAP3
	.set_vdd1_opp	= NULL,
#endif
};

static u64 musb_dmamask = DMA_BIT_MASK(32);

static struct platform_device musb_device = {
	.name		= "musb_hdrc",
	.id		= -1,
	.dev = {
		.dma_mask		= &musb_dmamask,
		.coherent_dma_mask	= DMA_BIT_MASK(32),
		.platform_data		= &musb_plat,
	},
	.num_resources	= ARRAY_SIZE(musb_resources),
	.resource	= musb_resources,
};

void __init usb_musb_init(struct omap_musb_board_data *board_data)
{
	if (cpu_is_omap243x()) {
		musb_resources[0].start = OMAP243X_HS_BASE;
	} else if (cpu_is_omap34xx()) {
		musb_resources[0].start = OMAP34XX_HSUSB_OTG_BASE;
	} else if (cpu_is_omap44xx()) {
		musb_resources[0].start = OMAP44XX_HSUSB_OTG_BASE;
		musb_resources[1].start = INT_44XX_HS_USB_MC;
		musb_resources[2].start = INT_44XX_HS_USB_DMA;
	}

	if (cpu_is_omap3630()) {
		musb_plat.max_vdd1_opp = S600M;
		musb_plat.min_vdd1_opp = S300M;
	} else if (cpu_is_omap3430()) {
		musb_plat.max_vdd1_opp = S500M;
		musb_plat.min_vdd1_opp = S125M;
	} else if (cpu_is_omap44xx()) {
		/* REVISIT later for OMAP4 */
		musb_plat.set_vdd1_opp = NULL;
	} else
		musb_plat.set_vdd1_opp = NULL;

	musb_resources[0].end = musb_resources[0].start + SZ_8K - 1;

	/*
	 * REVISIT: This line can be removed once all the platforms using
	 * musb_core.c have been converted to use use clkdev.
	 */
	musb_plat.clock = "ick";
	musb_plat.board_data = board_data;
	musb_plat.power = board_data->power >> 1;
	musb_plat.mode = board_data->mode;

	if (platform_device_register(&musb_device) < 0) {
		printk(KERN_ERR "Unable to register HS-USB (MUSB) device\n");
		return;
	}

	usb_musb_pm_init();
	usb_gadget_init();
}

#else
void __init usb_musb_init(struct omap_musb_board_data *board_data)
{
	usb_musb_pm_init();
}
#endif /* CONFIG_USB_MUSB_SOC */
