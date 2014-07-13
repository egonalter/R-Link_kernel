/* linux/arch/arm/mach-s5p6450/pm.c
 *
 * Copyright (c) 2006 Samsung Electronics
 *
 *
 * S3C6410 (and compatible) Power Manager (Suspend-To-RAM) support
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

#include <linux/init.h>
#include <linux/suspend.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/sysdev.h>
#include <linux/io.h>
#include <plat/gpio-bank-r.h>
#include <mach/hardware.h>
#include <mach/regs-gpio.h>
#include <asm/mach-types.h>

//#include <plat/regs-gpio.h>
#include <plat/regs-clock.h>
#include <mach/regs-clock.h>
//#include <asm/plat-s3c/regs-rtc.h>
#include <plat/cpu.h>
#include <plat/pm.h>
#include <mach/regs-irq.h>
#ifdef CONFIG_S3C2410_PM_DEBUG
extern void pm_dbg(const char *fmt, ...);
#define DBG(fmt...) pm_dbg(fmt)
#else
#define DBG(fmt...) printk(KERN_DEBUG fmt)
#endif

//static void s5p6450_cpu_suspend(void)
void s5p6450_cpu_suspend(void)
{
	volatile unsigned long tmp;

	/* issue the standby signal into the pm unit. Note, we
	 * issue a write-buffer drain just in case */

	tmp = 0;

	asm("b 1f\n\t"
	    ".align 5\n\t"
	    "1:\n\t"
	    "mcr p15, 0, %0, c7, c10, 5\n\t"
	    "mcr p15, 0, %0, c7, c10, 4\n\t"
	    "mcr p15, 0, %0, c7, c0, 4" :: "r" (tmp));

	/* we should never get past here */

	panic("sleep resumed to originator?");
}

static void s5p6450_pm_prepare(void)
{
	unsigned int tmp;
	
	__raw_writel(virt_to_phys(s3c_cpu_resume), S5P_INFORM0);

        tmp = __raw_readl(S5P_SLEEP_CFG);
        tmp &= ~(S5P_SLEEP_CFG_OSC_EN);   /* Disables the osc pad XXT */
        __raw_writel(tmp, S5P_SLEEP_CFG);
	
	 tmp = __raw_readl(S5P_PWR_CFG);
        tmp &= S5P_CFG_WFI_CLEAN;
        tmp |= S5P_CFG_WFI_SLEEP;
        __raw_writel(tmp, S5P_PWR_CFG);

	/* HWEG-153 keep XnRSTOUT high during sleep, so the movinand stays powered during 
	   suspend which allows us to resume */
	tmp = __raw_readl(S5P6450_SPCONSLP);
	tmp |= 1<<12;
	__raw_writel(tmp, S5P6450_SPCONSLP);

        /* SYSCON interrupt handling disable */
        tmp = __raw_readl(S5P_OTHERS);
        //tmp |= (S5P_OTHER_SYSC_INTOFF|0x00800000);   /*Disables the interrupt when the CPU enters the power mode. Has to be written one*/
        tmp |= (S5P_OTHER_SYSC_INTOFF);   /*Disables the interrupt when the CPU enters the power mode. Has to be written one*/
        __raw_writel(tmp, S5P_OTHERS);

	__raw_writel(0xffffffff, S3C_VIC0REG(VIC_INT_ENABLE_CLEAR));
        __raw_writel(0xffffffff, S3C_VIC1REG(VIC_INT_ENABLE_CLEAR));

	__raw_writel(__raw_readl(S5P_WAKEUP_STAT),S5P_WAKEUP_STAT);

	__raw_writel(0x4, S5P_PWR_STABLE);
	
	__raw_writel(0x2,S5P6450_SLPEN);

	/* Configuring EINT4/PB as interrupt source */
	tmp = __raw_readl(S5P64XX_GPNCON);
	tmp &= ~(3<<(8));
	tmp |=  (2<<(8));
	__raw_writel(tmp, S5P64XX_GPNCON);

	/* Set falling/rising edge trigger on GPN4/EINT4 (PB) */
	tmp = __raw_readl(S5P6450_EINT0CON0);
	tmp &= ~(7<<(8));
	tmp |=  (7<<(8));
	__raw_writel(tmp, S5P6450_EINT0CON0);

	/* Select wakeup source; EINT4(pb) and EINT2(irqb of pmic) for cable insertion */
	tmp = __raw_readl(S5P_EINT_WAKEUP_MASK);
	tmp &= ~(0x12);   			
	__raw_writel(tmp, S5P_EINT_WAKEUP_MASK);
}

static int s5p6450_pm_add(struct sys_device *sysdev)
{
	pm_cpu_prep = s5p6450_pm_prepare;
	pm_cpu_sleep = s5p6450_cpu_suspend;

	return 0;
}

static struct sleep_save s5p6450_sleep[] = {

};

static int s5p6450_pm_resume(struct sys_device *dev)
{
	u32 temp;

	__raw_writel(0, S5P_EINT_WAKEUP_MASK);
	s3c_pm_do_restore(s5p6450_sleep, ARRAY_SIZE(s5p6450_sleep));

	//__raw_writel(0xffff, S5P_EINT0PEND);
	__raw_writel((0xff << 16)|0x7ff, S5P6450_EINT12PEND);
	__raw_writel((0x3ff<<16 | 0x7f), S5P6450_EINT56PEND);
	__raw_writel(0x5ffc << 16, S5P6450_EINT8PEND);

	/* Set low level trigger on GPN4/EINT4 */
	temp = __raw_readl(S5P6450_EINT0CON0);
	temp &= ~(7<<(8));
	__raw_writel(temp, S5P6450_EINT0CON0);

	temp = __raw_readl(S5P6450_SERVICE);

	return 0;
}

static struct sysdev_driver s5p6450_pm_driver = {
	.add		= s5p6450_pm_add,
	.resume		= s5p6450_pm_resume,
};

static __init int s5p6450_pm_drvinit(void)
{
	return sysdev_driver_register(&s5p6450_sysclass, &s5p6450_pm_driver);
}

arch_initcall(s5p6450_pm_drvinit);

