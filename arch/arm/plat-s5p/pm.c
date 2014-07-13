/* linux/arch/arm/plat-s5p/pm.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 *              http://www.samsung.com/
 *
 * S5P Power Manager (Suspend-To-RAM) support
 *
 * Based on arch/arm/mach-pxa/pm.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/suspend.h>
#include <linux/errno.h>
#include <linux/time.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/crc32.h>
#include <linux/ioport.h>
#include <linux/delay.h>
#include <linux/serial_core.h>
#include <linux/io.h>
#include <linux/platform_device.h>

#include <asm/cacheflush.h>
#include <mach/hardware.h>

#include <plat/map-base.h>
#include <plat/regs-serial.h>
#include <plat/regs-timer.h>
#include <mach/regs-clock.h>
#include <plat/gpio-cfg.h>
#include <mach/regs-mem.h>
#include <mach/regs-irq.h>
#include <plat/regs-clock.h>
#include <linux/gpio.h>

#include <asm/mach/time.h>
#include <asm/gpio.h>
#include <mach/regs-gpio.h>

#include <plat/pm.h>

#define PFX "s5p pm: "

static struct sleep_save core_save[] = {
        SAVE_ITEM(S3C2410_TCFG0),
        SAVE_ITEM(S3C2410_TCFG1),
        SAVE_ITEM(S3C2410_TCON),
        SAVE_ITEM(S3C_APLL_LOCK),
        SAVE_ITEM( S3C_MPLL_LOCK),
        SAVE_ITEM( S3C_EPLL_LOCK),
        SAVE_ITEM( S3C_APLL_CON ),
        SAVE_ITEM( S3C_MPLL_CON),
        SAVE_ITEM( S3C_EPLL_CON),
        SAVE_ITEM( S3C_EPLL_CON_K),
        SAVE_ITEM( S3C_CLK_SRC0 ),
        SAVE_ITEM( S3C_CLK_DIV0),
        SAVE_ITEM( S3C_CLK_DIV1),
        SAVE_ITEM( S3C_CLK_DIV2),
        SAVE_ITEM( S3C_CLK_OUT  ),
        SAVE_ITEM( S3C_CLK_GATE_HCLK0),
        SAVE_ITEM( S3C_CLK_GATE_PCLK),
        SAVE_ITEM( S3C_CLK_GATE_SCLK0),
        SAVE_ITEM( S3C_CLK_GATE_MEM0),
        SAVE_ITEM( S3C_CLK_DIV3 ),
        SAVE_ITEM( S3C_CLK_GATE_HCLK1),
        SAVE_ITEM( S3C_CLK_GATE_SCLK1),
        SAVE_ITEM( S3C_AHB_CON0     ),
        SAVE_ITEM( S3C_CLK_SRC1    ),
};
static struct sleep_save gpio_save[] = {
        SAVE_ITEM(S5P64XX_GPACON),
        SAVE_ITEM(S5P64XX_GPADAT),
        SAVE_ITEM(S5P64XX_GPAPUD),
        SAVE_ITEM(S5P64XX_GPACONSLP),
        SAVE_ITEM(S5P64XX_GPAPUDSLP),
        SAVE_ITEM(S5P64XX_GPBCON),
        SAVE_ITEM(S5P64XX_GPBDAT),
        SAVE_ITEM(S5P64XX_GPBPUD),
        SAVE_ITEM(S5P64XX_GPBCONSLP),
        SAVE_ITEM(S5P64XX_GPBPUDSLP),
        SAVE_ITEM(S5P64XX_GPCCON),
        SAVE_ITEM(S5P64XX_GPCDAT),
        SAVE_ITEM(S5P64XX_GPCPUD),
        SAVE_ITEM(S5P64XX_GPCCONSLP),
        SAVE_ITEM(S5P64XX_GPCPUDSLP),
        SAVE_ITEM(S5P64XX_GPDCON),
        SAVE_ITEM(S5P64XX_GPDDAT),
        SAVE_ITEM(S5P64XX_GPDPUD),
        SAVE_ITEM(S5P64XX_GPDCONSLP),
        SAVE_ITEM(S5P64XX_GPDPUDSLP),
        SAVE_ITEM(S5P64XX_GPFCON),
        SAVE_ITEM(S5P64XX_GPFDAT),
        SAVE_ITEM(S5P64XX_GPFPUD),
        SAVE_ITEM(S5P64XX_GPFCONSLP),
        SAVE_ITEM(S5P64XX_GPFPUDSLP),
        SAVE_ITEM(S5P64XX_GPGCON0),//LN to verify the GPIO mapping
        SAVE_ITEM(S5P64XX_GPGCON1),//LN to verify the GPIO mapping
        SAVE_ITEM(S5P64XX_GPGDAT),
        SAVE_ITEM(S5P64XX_GPGPUD),
        SAVE_ITEM(S5P64XX_GPGCONSLP),
        SAVE_ITEM(S5P64XX_GPGPUDSLP),
        SAVE_ITEM(S5P64XX_GPHCON0),
        SAVE_ITEM(S5P64XX_GPHCON1),
        SAVE_ITEM(S5P64XX_GPHDAT),
        SAVE_ITEM(S5P64XX_GPHPUD),
        SAVE_ITEM(S5P64XX_GPHCONSLP),
        SAVE_ITEM(S5P64XX_GPHPUDSLP),
        SAVE_ITEM(S5P64XX_GPICON),
        SAVE_ITEM(S5P64XX_GPIDAT),
        SAVE_ITEM(S5P64XX_GPIPUD),
        SAVE_ITEM(S5P64XX_GPICONSLP),
        SAVE_ITEM(S5P64XX_GPIPUDSLP),
        SAVE_ITEM(S5P64XX_GPJCON),
        SAVE_ITEM(S5P64XX_GPJDAT),
        SAVE_ITEM(S5P64XX_GPJPUD),
        SAVE_ITEM(S5P64XX_GPJCONSLP),
        SAVE_ITEM(S5P64XX_GPJPUDSLP),
        SAVE_ITEM(S5P64XX_GPKCON),
        SAVE_ITEM(S5P64XX_GPKDAT),
        SAVE_ITEM(S5P64XX_GPKPUD),
        SAVE_ITEM(S5P64XX_GPKCONSLP),
        SAVE_ITEM(S5P64XX_GPKPUDSLP),
	SAVE_ITEM(S5P64XX_GPNCON),
        SAVE_ITEM(S5P64XX_GPNDAT),
        SAVE_ITEM(S5P64XX_GPNPUD),
        SAVE_ITEM(S5P64XX_GPPCON),
        SAVE_ITEM(S5P64XX_GPPDAT),
        SAVE_ITEM(S5P64XX_GPPPUD),
        SAVE_ITEM(S5P64XX_GPPCONSLP),
        SAVE_ITEM(S5P64XX_GPPPUDSLP),
        SAVE_ITEM(S5P64XX_GPQCON),
        SAVE_ITEM(S5P64XX_GPQDAT),
        SAVE_ITEM(S5P64XX_GPQPUD),
        SAVE_ITEM(S5P64XX_GPQCONSLP),
        SAVE_ITEM(S5P64XX_GPQPUDSLP),

        /* One_board special item */
        SAVE_ITEM(S5P64XX_GPRCON0),
        SAVE_ITEM(S5P64XX_GPRCON1), 
        SAVE_ITEM(S5P64XX_GPRDAT),
        SAVE_ITEM(S5P64XX_GPRPUD),
        SAVE_ITEM(S5P64XX_GPRCONSLP),
        SAVE_ITEM(S5P64XX_GPRPUDSLP),

        SAVE_ITEM(S5P64XX_GPSCON),
        SAVE_ITEM(S5P64XX_GPSDAT),
        SAVE_ITEM(S5P64XX_GPSPUD),
        SAVE_ITEM(S5P64XX_GPSCONSLP),
        SAVE_ITEM(S5P64XX_GPSPUDSLP),
};

static struct sleep_save irq_save[] = {
        SAVE_ITEM(S5P64XX_VIC0INTSELECT),
        SAVE_ITEM(S5P64XX_VIC1INTSELECT),
        SAVE_ITEM(S5P64XX_VIC0INTENABLE),
        SAVE_ITEM(S5P64XX_VIC1INTENABLE),
        SAVE_ITEM(S5P64XX_VIC0SOFTINT),
        SAVE_ITEM(S5P64XX_VIC1SOFTINT),

        SAVE_ITEM(S5P6450_EINT0CON0),
        SAVE_ITEM(S5P6450_EINT0FLTCON0),
        SAVE_ITEM(S5P6450_EINT0FLTCON1),
        SAVE_ITEM(S5P6450_EINT0MASK),

        SAVE_ITEM(S5P6450_EINT12CON),
        SAVE_ITEM(S5P6450_EINT56CON),
        SAVE_ITEM(S5P6450_EINT8CON),

        SAVE_ITEM(S5P6450_EINT12FLTCON),
        SAVE_ITEM(S5P6450_EINT56FLTCON),
        SAVE_ITEM(S5P6450_EINT8FLTCON),

        SAVE_ITEM(S5P6450_EINT12MASK),
        SAVE_ITEM(S5P6450_EINT56MASK),
        SAVE_ITEM(S5P6450_EINT8MASK),
};

static struct sleep_save sromc_save[] = {
        SAVE_ITEM(S5P64XX_SROM_BW),
        SAVE_ITEM(S5P64XX_SROM_BC0),
        //SAVE_ITEM(S5P64XX_SROM_BC1),
       // SAVE_ITEM(S5P64XX_SROM_BC2),
       // SAVE_ITEM(S5P64XX_SROM_BC3),
       // SAVE_ITEM(S5P64XX_SROM_BC4),
       // SAVE_ITEM(S5P64XX_SROM_BC5),
};
#if 0
static struct sleep_save sromc_save[] = {
	SAVE_ITEM(S5P_SROM_BW),
	SAVE_ITEM(S5P_SROM_BC0),
	SAVE_ITEM(S5P_SROM_BC1),
	SAVE_ITEM(S5P_SROM_BC2),
	SAVE_ITEM(S5P_SROM_BC3),
	SAVE_ITEM(S5P_SROM_BC4),
	SAVE_ITEM(S5P_SROM_BC5),
};
#endif
        /* Special register */
static struct sleep_save misc_save[] = {
        SAVE_ITEM(S3C_AHB_CON0),

        SAVE_ITEM(S5P64XX_SPC_BASE),
        SAVE_ITEM(S5P64XX_SPC1_BASE),
	SAVE_ITEM(S5P6450_MEM0CONSLP0),
        SAVE_ITEM(S5P6450_MEM0CONSLP1),
        SAVE_ITEM(S5P64XX_LCDSRCON),
        SAVE_ITEM(S5P6450_MEM0DRVCON),
        SAVE_ITEM(S5P6450_MEM1DRVCON),

        SAVE_ITEM(S5P_OTHERS),
};




/* NAND control registers */
#define PM_NFCONF             (S3C_VA_NAND + 0x00)        
#define PM_NFCONT             (S3C_VA_NAND + 0x04)        

static struct sleep_save nand_save[] = {
        SAVE_ITEM(PM_NFCONF),
        SAVE_ITEM(PM_NFCONT),
};



/* s3c_pm_check_resume_pin
 *
 * check to see if the pin is configured correctly for sleep mode, and
 * make any necessary adjustments if it is not
*/

static void s3c_pm_check_resume_pin(unsigned int pin, unsigned int irqoffs)
{

}

/* s3c_pm_configure_extint
 *
 * configure all external interrupt pins
*/

void s3c_pm_configure_extint(void)
{

	/* for each of the external interrupts (EINT0..EINT15) we
	 * need to check wether it is an external interrupt source,
	 * and then configure it as an input if it is not
	*/
	__raw_writel(s3c_irqwake_eintmask, S5P_EINT_WAKEUP_MASK);
}


void s3c_pm_restore_core(void)
{
	s3c_pm_do_restore_core(core_save, ARRAY_SIZE(core_save));
	s3c_pm_do_restore(gpio_save, ARRAY_SIZE(gpio_save));
	s3c_pm_do_restore(irq_save, ARRAY_SIZE(irq_save));
        s3c_pm_do_restore(misc_save, ARRAY_SIZE(misc_save));
	s3c_pm_do_restore(sromc_save, ARRAY_SIZE(sromc_save));
	s3c_pm_do_restore(nand_save, ARRAY_SIZE(nand_save));
}

void s3c_pm_save_core(void)
{
	s3c_pm_do_save(nand_save, ARRAY_SIZE(nand_save));
	s3c_pm_do_save(sromc_save, ARRAY_SIZE(sromc_save));
	s3c_pm_do_save(gpio_save, ARRAY_SIZE(gpio_save));
	s3c_pm_do_save(irq_save, ARRAY_SIZE(irq_save));
        s3c_pm_do_save(misc_save, ARRAY_SIZE(misc_save));
	s3c_pm_do_save(core_save, ARRAY_SIZE(core_save));
}

