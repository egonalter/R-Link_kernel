/* s3c-gib.c
 *
 * Copyright (C) 2010 Samsung Electronics Co. Ltd.
 *
 * S3C GPS Interface Block 
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/rfkill.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/pm.h>

#include <mach/hardware.h>
#include <linux/irq.h>
#include <linux/io.h>

#include <plat/clock.h>
#include <plat/gpio-cfg.h>
#include <plat/regs-rtc.h>

#include <mach/regs-gpio.h>
#include <mach/gpio.h>
#include <mach/regs-clock.h>
#include <mach/regs-gib.h>


#include "gib-dev.h"
#include "s3c-gib.h"

#ifdef debug
	#define DBG(x...)       printk(x)
	#define GIB_DBG		printk("%s :: %d\n",__FUNCTION__,__LINE__)
#else
	#define GIB_DBG
	#define DBG(x...)       do { } while (0)
#endif

#ifdef CONFIG_RFKILL

static int s3c_gib_resume(struct platform_device *pdev);
static int s3c_gib_suspend(struct platform_device *pdev, pm_message_t msg);

#define PFX "GIBDEV RFKILL:"
#define GIBNAME	"GIB"

static int gps_rfkill_set_block(void *data, bool blocked)
{       
	struct	platform_device	*pdev	= (struct platform_device *) data;
	pm_message_t m = PMSG_SUSPEND;

	if (blocked)
		s3c_gib_suspend(pdev, m);
	else
		s3c_gib_resume(pdev);

        return 0;
}

static const struct rfkill_ops gps_rfkill_ops = {
        .set_block = gps_rfkill_set_block,
};
#endif

struct bit_data {
	unsigned int	bb_scl;
	unsigned int	bb_sda;
	unsigned int	uart5_rxd;
	unsigned int	uart5_txd;

	unsigned int	gps_bb_mclk;
	unsigned int	gps_bb_sync;
	unsigned int	gps_bb_epoch;
	unsigned int	gps_slpn;
	unsigned int	gps_rstout;
	unsigned int	bt_rstout;

	unsigned int	gps_qsign;
	unsigned int	gps_qmag;
	unsigned int	gps_isign;
	unsigned int	gps_imag;

	unsigned int	gps_gpio0;
	unsigned int	gps_gpio1;
	unsigned int	gps_gpio2;
	unsigned int	gps_gpio3;
	unsigned int	gps_gpio4;
	unsigned int	gps_gpio5;
	unsigned int	gps_gpio6;
	unsigned int	gps_gpio7;
};




static void s3c_gibgpio_init(struct bit_data *gpio)
{
	GIB_DBG;

	gpio->uart5_rxd= S5P6450_GPD(2);
	gpio->uart5_txd= S5P6450_GPD(3);
	gpio->bb_sda = S5P6450_GPD(6);
	gpio->bb_scl = S5P6450_GPD(7);
	
	gpio->gps_bb_mclk= S5P6450_GPP(8);
	gpio->gps_bb_sync= S5P6450_GPP(9);
	gpio->gps_bb_epoch= S5P6450_GPP(10);
	gpio->gps_slpn= S5P6450_GPP(11);
	gpio->gps_rstout = S5P6450_GPP(13);
	gpio->bt_rstout = S5P6450_GPP(14);

	gpio->gps_qsign = S5P6450_GPR(0);
	gpio->gps_qmag= S5P6450_GPR(1);
	gpio->gps_isign= S5P6450_GPR(2);
	gpio->gps_imag= S5P6450_GPR(3);
	
	gpio->gps_gpio0 = S5P6450_GPS(0);
	gpio->gps_gpio1 = S5P6450_GPS(1);
	gpio->gps_gpio2 = S5P6450_GPS(2);
	gpio->gps_gpio3 = S5P6450_GPS(3);
	gpio->gps_gpio4 = S5P6450_GPS(4);
	gpio->gps_gpio5 = S5P6450_GPS(5);
	gpio->gps_gpio6 = S5P6450_GPS(6);
	gpio->gps_gpio7 = S5P6450_GPS(7);
	
	//GPIO: 0xE030_xxxx
	//GPD
	s3c_gpio_cfgpin(gpio->uart5_rxd, S3C_GPIO_SFN(2));
	s3c_gpio_cfgpin(gpio->uart5_txd, S3C_GPIO_SFN(2));

#if defined(CONFIG_MACH_S5P6450_TOMTOM)
	s3c_gpio_cfgpin(gpio->bb_scl, S3C_GPIO_SFN(0));
	s3c_gpio_cfgpin(gpio->bb_sda, S3C_GPIO_SFN(0));
#else
	s3c_gpio_cfgpin(gpio->bb_scl, S3C_GPIO_SFN(2));
	s3c_gpio_cfgpin(gpio->bb_sda, S3C_GPIO_SFN(2));
#endif

	//GPP
	//XARMCLK - Max 64MHz
	s3c_gpio_cfgpin(gpio->gps_bb_mclk, S3C_GPIO_SFN(2));
	//XARSYNC - 19 
	s3c_gpio_cfgpin(gpio->gps_bb_sync, S3C_GPIO_SFN(2));
	//XAREPOCH - 3
	s3c_gpio_cfgpin(gpio->gps_bb_epoch, S3C_GPIO_SFN(2));
	//XARGPSSLPN - 14
	s3c_gpio_cfgpin(gpio->gps_slpn, S3C_GPIO_SFN(2));
	//XARGPSRSTOUTN - 13
	s3c_gpio_cfgpin(gpio->gps_rstout, S3C_GPIO_SFN(2));
	//XARBTRSTOUTN - 1
	s3c_gpio_cfgpin(gpio->bt_rstout, S3C_GPIO_SFN(2));

	//GPR
	//XARQSIGN - 18
	s3c_gpio_cfgpin(gpio->gps_qsign, S3C_GPIO_SFN(2));
	//XARQMAG - 17
	s3c_gpio_cfgpin(gpio->gps_qmag, S3C_GPIO_SFN(2));
	//XARISIGN - 16
	s3c_gpio_cfgpin(gpio->gps_isign, S3C_GPIO_SFN(2));
	//XARIMAG - 15
	s3c_gpio_cfgpin(gpio->gps_imag, S3C_GPIO_SFN(2));

	//GPS
	// - 4
	s3c_gpio_cfgpin(gpio->gps_gpio0, S3C_GPIO_SFN(2));
	// - 5
	s3c_gpio_cfgpin(gpio->gps_gpio1, S3C_GPIO_SFN(2));

#if defined(CONFIG_MACH_S5P6450_TOMTOM)
	// - 6
	s3c_gpio_cfgpin(gpio->gps_gpio2, S3C_GPIO_SFN(3));
	// - 7
	s3c_gpio_cfgpin(gpio->gps_gpio3, S3C_GPIO_SFN(3));
	// - 8
	s3c_gpio_cfgpin(gpio->gps_gpio4, S3C_GPIO_SFN(0));
	// - 9
	s3c_gpio_cfgpin(gpio->gps_gpio5, S3C_GPIO_SFN(0));
	// - 10
	s3c_gpio_cfgpin(gpio->gps_gpio6, S3C_GPIO_SFN(0));
	// - 11
	s3c_gpio_cfgpin(gpio->gps_gpio7, S3C_GPIO_SFN(0));
#else
	// - 6
	s3c_gpio_cfgpin(gpio->gps_gpio2, S3C_GPIO_SFN(2));
	// - 7
	s3c_gpio_cfgpin(gpio->gps_gpio3, S3C_GPIO_SFN(2));
	// - 8
	s3c_gpio_cfgpin(gpio->gps_gpio4, S3C_GPIO_SFN(3));
	// - 9
	s3c_gpio_cfgpin(gpio->gps_gpio5, S3C_GPIO_SFN(3));
	// - 10
	s3c_gpio_cfgpin(gpio->gps_gpio6, S3C_GPIO_SFN(3));
	// - 11
	s3c_gpio_cfgpin(gpio->gps_gpio7, S3C_GPIO_SFN(3));
#endif
}

//Set MUX
//UART5 forwarding to GPS UART
//0: Connect debug, 1: Connect BB
//CLOCK Controller:SYS_OTHERS: 0xE010_011C
static void s3c_gib_ubp_debug( unsigned int flag)
{
	unsigned int  tmp;
	GIB_DBG;

	tmp = __raw_readl(S5P_SYS_OTHERS);
	if (flag) {
		tmp &= ~(0x1<<3);
		printk ("Changed to UBP Debug Mode\n");
	} else {
		tmp |= (0x1<<3);
		printk ("Connected: BB_UART1-->AP_UART5\n");
	}
	__raw_writel (tmp, S5P_SYS_OTHERS);
}


//GPS RF + BB SW Reset
//Clock Controller:SWRESET: 0xE010_0114
static void s3c_gib_core_reset(unsigned int flag)
{
	unsigned int  tmp;
	GIB_DBG;
	
	tmp = __raw_readl(S5P_SWRESET);

	if(flag == 0) {
		tmp |= (0x1<<16);
		__raw_writel(tmp, S5P_SWRESET);
	} else if (flag == 1) {
		tmp &= ~(0x1<<16);
		__raw_writel(tmp, S5P_SWRESET);
	}
}

static struct gib_reset s3c_gib_reset = {
	.core_reset	= s3c_gib_core_reset,
};

static struct gib_udp_debug s3c_gib_udp_debug = {
	.ubp_debug 	= s3c_gib_ubp_debug,
};

static struct s3c_gib s3c_gib[2] = {
	[0] = {
		.gibdev	= {
			.g_reset	= &s3c_gib_reset,
			.g_udp_debug	= &s3c_gib_udp_debug,

		}
	},
	[1] = {
		.gibdev	= {
			.g_reset	= &s3c_gib_reset,
			.g_udp_debug	= &s3c_gib_udp_debug,
		}
	},
};

static struct bit_data bit_data;

/* s3c_gib_probe
 *
 * called by the bus driver when a suitable device is found
*/

static int s3c_gib_probe(struct platform_device *pdev)
{
	struct s3c_gib *gib = &s3c_gib[pdev->id];
	struct resource *res;
	int ret;
	struct bit_data *g_gpio = &bit_data;

	GIB_DBG;

	gib->nr = pdev->id;
	gib->dev = &pdev->dev;

	/* map the registers */
	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);

	if (res == NULL) {
		dev_err(&pdev->dev, "cannot find IO resource\n");
		ret = -ENOENT;
		goto out;
	}

	gib->regs = ioremap(res->start, (res->end - res->start) + 1);

	if (gib->regs == NULL) {
		dev_err(&pdev->dev, "cannot map IO\n");
		ret = -ENXIO;
		goto out;
	}

	/* setup info block for the gib core */
	gib->gibdev.dev.parent = &pdev->dev;
	gib->gibdev.minor = gib->nr;

	ret = gib_attach_gibdev(&gib->gibdev);

	if (ret < 0) {
		dev_err(&pdev->dev, "failed to add adapter to gib core\n");
		goto out;
	}

	dev_set_drvdata(&pdev->dev, gib);

	s3c_gibgpio_init(g_gpio);

	s3c_gib_ubp_debug(0);

#ifdef CONFIG_RFKILL
	/* Register as rfkill device */
	gib->name = GIBNAME;
	gib->rfk_data = rfkill_alloc(gib->name, &pdev->dev,
					RFKILL_TYPE_GPS, &gps_rfkill_ops,
					(void *)pdev);

	if (unlikely(!gib->rfk_data)) {
	ret = -ENOMEM;
	goto rfkill_alloc_failed;
	}

	/* mark device as persistent */
	rfkill_init_sw_state(gib->rfk_data, 0);

	printk(KERN_INFO PFX "Registering rfkill for GPS.\n");
	ret = rfkill_register(gib->rfk_data);
	if (unlikely(ret)) {
		printk (KERN_INFO PFX "Rfkill registration failed\n");
		goto rfkill_register_failed;
	}

	printk(KERN_INFO PFX "Initialized.\n");
	return 0;

rfkill_register_failed:
	rfkill_destroy(gib->rfk_data);
rfkill_alloc_failed:
#endif


out:

	return ret;
}

/* s3c_gib_remove
 *
 * called when device is removed from the bus
*/
static int s3c_gib_remove(struct platform_device *pdev)
{
	struct s3c_gib *gib = dev_get_drvdata(&pdev->dev);

	GIB_DBG;
	
	if (gib != NULL) {
		gib_detach_gibdev(&gib->gibdev);
		dev_set_drvdata(&pdev->dev, NULL);
	}

	return 0;
}

#ifdef CONFIG_PM
static int s3c_gib_suspend(struct platform_device *pdev, pm_message_t msg)
{
	unsigned int tmp;

	tmp = __raw_readl(S5P6450_GPS_GPSIO);

       if ( tmp|(0x1<<18) ) {
		//printk("GPS BB is now in sleep state. \n");
	 }else {
	 	printk("GPS BB has not been in sleep state. Please wait. \n");

	 	do {
			tmp = __raw_readl(S5P6450_GPS_GPSIO);
	 	} while ( (tmp|(0x1<<18)) );
	 	     
		printk("GPS BB is now in sleep state. \n");
	 }
	 
	return 0;
}

static int s3c_gib_resume(struct platform_device *pdev)
{
//	struct s3c_gib *hw = platform_get_drvdata(pdev);

	unsigned int tmp;

	//UART5 forwarding to GPS UART
	tmp = __raw_readl(S5P_SYS_OTHERS);
	tmp &= ~(0xF<<0);
	tmp |= (0x8<<0);
	__raw_writel(tmp, S5P_SYS_OTHERS);
	
	return 0;
}
#else
#define s3c_gib_suspend NULL
#define s3c_gib_resume  NULL
#endif

/* device driver for platform bus bits */
static struct platform_driver s3c_gib_driver = {
	.probe		= s3c_gib_probe,
	.remove		= s3c_gib_remove,
#ifdef CONFIG_PM
	.suspend	= s3c_gib_suspend,
	.resume		= s3c_gib_resume,
#endif
	.driver		= {
		.name	= "s3c-gib",
		.owner	= THIS_MODULE,
		.bus    = &platform_bus_type,
	},
};

static int __init s3c_gib_driver_init(void)
{
	GIB_DBG;

	return platform_driver_register(&s3c_gib_driver);
}

static void __exit s3c_gib_driver_exit(void)
{
	GIB_DBG;
	platform_driver_unregister(&s3c_gib_driver);
}

module_init(s3c_gib_driver_init);
module_exit(s3c_gib_driver_exit);

MODULE_DESCRIPTION("S3C GIB driver");
MODULE_AUTHOR("Taeyong Lee<taeyong00.lee@samsung.com>");
MODULE_LICENSE("GPL");
