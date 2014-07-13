/* linux/arch/arm/mach-s5p6450/include/mach/regs-gpio.h
 *
 * Copyright (c) 2009 Samsung Electronics Co., Ltd.
 *		http://www.samsung.com/
 *
 * S5P6450 - GPIO register definitions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_ARCH_REGS_GPIO_H
#define __ASM_ARCH_REGS_GPIO_H __FILE__

#include <plat/gpio-bank-a.h>
#include <plat/gpio-bank-b.h>
#include <plat/gpio-bank-c.h>
#include <plat/gpio-bank-d.h>
#include <plat/gpio-bank-f.h>
#include <plat/gpio-bank-g.h>
#include <plat/gpio-bank-h.h>
#include <plat/gpio-bank-i.h>
#include <plat/gpio-bank-j.h>
#include <plat/gpio-bank-k.h>
#include <plat/gpio-bank-n.h>
#include <plat/gpio-bank-p.h>
#include <plat/gpio-bank-r.h>
#include <plat/gpio-bank-s.h>
#include <plat/gpio-bank-q.h>

#include <mach/map.h>

/* Base addresses for each of the banks */
#define S5P64XX_GPA_BASE                (S5P_VA_GPIO + 0x0000)
#define S5P64XX_GPB_BASE                (S5P_VA_GPIO + 0x0020)
#define S5P64XX_GPC_BASE                (S5P_VA_GPIO + 0x0040)
#define S5P64XX_GPD_BASE                (S5P_VA_GPIO + 0x0060)

#define S5P64XX_GPF_BASE                (S5P_VA_GPIO + 0x00A0)
#define S5P64XX_GPG_BASE                (S5P_VA_GPIO + 0x00C0)
#define S5P64XX_GPH_BASE                (S5P_VA_GPIO + 0x00E0)
#define S5P64XX_GPI_BASE                (S5P_VA_GPIO + 0x0100)
#define S5P64XX_GPJ_BASE                (S5P_VA_GPIO + 0x0120)
#define S5P64XX_GPK_BASE                (S5P_VA_GPIO + 0x0140)

#define S5P64XX_GPN_BASE                (S5P_VA_GPIO + 0x0830)
#define S5P64XX_GPP_BASE                (S5P_VA_GPIO + 0x0160)
#define S5P64XX_GPQ_BASE                (S5P_VA_GPIO + 0x0180)

#define S5P64XX_GPR_BASE                (S5P_VA_GPIO + 0x0290)
#define S5P64XX_GPS_BASE                (S5P_VA_GPIO + 0x0300) 

#define S5P64XX_SPC_BASE        	(S5P_VA_GPIO + 0x01A0)
#define S5P64XX_SPC1_BASE       	(S5P_VA_GPIO + 0x02B0)
#define S5P64XX_LCDSRCON	       	(S5P_VA_GPIO + 0x02B8)

#define S5P6450_MEM0CONSLP0             (S5P_VA_GPIO + 0x1C0)
#define S5P6450_MEM0CONSLP1             (S5P_VA_GPIO + 0x1C4)

#define S5P6450_MEM0DRVCON              (S5P_VA_GPIO + 0x1D0)
#define S5P6450_MEM1DRVCON              (S5P_VA_GPIO + 0x1D4)

#define S5P64XX_EINT0CON0               (S5P_VA_GPIO + 0x900)
#define S5P64XX_EINT0FLTCON0            (S5P_VA_GPIO + 0x910)
#define S5P64XX_EINT0FLTCON1            (S5P_VA_GPIO + 0x914)
#define S5P64XX_EINT0MASK               (S5P_VA_GPIO + 0x920)
#define S5P64XX_EINT0PEND               (S5P_VA_GPIO + 0x924)



#define S5P6450_GPA_BASE		(S5P_VA_GPIO + 0x0000)
#define S5P6450_GPB_BASE		(S5P_VA_GPIO + 0x0020)
#define S5P6450_GPC_BASE		(S5P_VA_GPIO + 0x0040)
#define S5P6450_GPD_BASE                (S5P_VA_GPIO + 0x0060)

#define S5P6450_GPF_BASE		(S5P_VA_GPIO + 0x00A0)
#define S5P6450_GPG_BASE		(S5P_VA_GPIO + 0x00C0)
#define S5P6450_GPH_BASE		(S5P_VA_GPIO + 0x00E0)
#define S5P6450_GPI_BASE		(S5P_VA_GPIO + 0x0100)
#define S5P6450_GPJ_BASE		(S5P_VA_GPIO + 0x0120)
#define S5P6450_GPK_BASE                (S5P_VA_GPIO + 0x0140)

#define S5P6450_GPN_BASE		(S5P_VA_GPIO + 0x0830)
#define S5P6450_GPP_BASE		(S5P_VA_GPIO + 0x0160)
#define S5P6450_GPQ_BASE                (S5P_VA_GPIO + 0x0180)

#define S5P6450_GPR_BASE		(S5P_VA_GPIO + 0x0290)
#define S5P6450_GPS_BASE                (S5P_VA_GPIO + 0x0300)

#define S5P64XX_SPC_BASE        (S5P_VA_GPIO + 0x01A0)
#define S5P64XX_SPC1_BASE       (S5P_VA_GPIO + 0x02B0)
#define S5P6450_EINT0CON0		(S5P_VA_GPIO + 0x900)
#define S5P6450_EINT0FLTCON0		(S5P_VA_GPIO + 0x910)
#define S5P6450_EINT0FLTCON1		(S5P_VA_GPIO + 0x914)
#define S5P6450_EINT0MASK		(S5P_VA_GPIO + 0x920)
#define S5P6450_EINT0PEND		(S5P_VA_GPIO + 0x924)

#define S5P6450_SPCONSLP		(S5P_VA_GPIO + 0x880)
#define S5P6450_SLPEN			(S5P_VA_GPIO + 0x930)
#define S5P6450_PSHOLD_CON		(S5P_VA_GPIO + 0x934)

        
#define S5P6450_GPA_BASE                (S5P_VA_GPIO + 0x0000)
#define S5P6450_GPB_BASE                (S5P_VA_GPIO + 0x0020)
#define S5P6450_GPC_BASE                (S5P_VA_GPIO + 0x0040)
#define S5P6450_GPD_BASE                (S5P_VA_GPIO + 0x0060)
#define S5P6450_GPF_BASE                (S5P_VA_GPIO + 0x00A0)
#define S5P6450_GPG_BASE                (S5P_VA_GPIO + 0x00C0)
#define S5P6450_GPH_BASE                (S5P_VA_GPIO + 0x00E0)
#define S5P6450_GPI_BASE                (S5P_VA_GPIO + 0x0100)
#define S5P6450_GPJ_BASE                (S5P_VA_GPIO + 0x0120)
#define S5P6450_GPK_BASE                (S5P_VA_GPIO + 0x0140)

#define S5P6450_GPN_BASE                (S5P_VA_GPIO + 0x0830)
#define S5P6450_GPP_BASE                (S5P_VA_GPIO + 0x0160)

#define S5P6450_GPQ_BASE                (S5P_VA_GPIO + 0x0180)

#define S5P6450_GPR_BASE                (S5P_VA_GPIO + 0x0290)
#define S5P6450_GPS_BASE                (S5P_VA_GPIO + 0x0300)

#define S5P6450_SPC_BASE        	(S5P_VA_GPIO + 0x01A0)
#define S5P6450_SPC1_BASE       	(S5P_VA_GPIO + 0x02B0)
#define S5P6450_EINT0CON0               (S5P_VA_GPIO + 0x900)
#define S5P6450_EINT0FLTCON0            (S5P_VA_GPIO + 0x910)
#define S5P6450_EINT0FLTCON1            (S5P_VA_GPIO + 0x914)
#define S5P6450_EINT0MASK               (S5P_VA_GPIO + 0x920)
#define S5P6450_EINT0PEND               (S5P_VA_GPIO + 0x924)

#define S5P6450_EINT12CON               (S5P_VA_GPIO + 0x200)
#define S5P6450_EINT56CON               (S5P_VA_GPIO + 0x208)
#define S5P6450_EINT8CON                (S5P_VA_GPIO + 0x20C)


#define S5P6450_EINT12FLTCON             (S5P_VA_GPIO + 0x220)
#define S5P6450_EINT56FLTCON             (S5P_VA_GPIO + 0x228)
#define S5P6450_EINT8FLTCON              (S5P_VA_GPIO + 0x22C)


#define S5P6450_EINT12MASK               (S5P_VA_GPIO + 0x240)
#define S5P6450_EINT56MASK               (S5P_VA_GPIO + 0x248)
#define S5P6450_EINT8MASK                (S5P_VA_GPIO + 0x24C)


#define S5P6450_EINT12PEND               (S5P_VA_GPIO + 0x260)
#define S5P6450_EINT56PEND               (S5P_VA_GPIO + 0x268)
#define S5P6450_EINT8PEND                (S5P_VA_GPIO + 0x26C)


#define S5P6450_PRIORITY                 (S5P_VA_GPIO + 0x280)
#define S5P6450_SERVICE                  (S5P_VA_GPIO + 0x284)
#define S5P6450_SERVICEPEND              (S5P_VA_GPIO + 0x288)




/* for LCD */
#define S5P6450_SPCON_LCD_SEL_RGB	(1 << 0)
#define S5P6450_SPCON_LCD_SEL_MASK	(3 << 0)

/* These set of macros are not really useful for the
 * GPF/GPI/GPJ/GPN/GPP,
 * useful for others set of GPIO's (4 bit)
 */
#define S5P6450_GPIO_CONMASK(__gpio)	(0xf << ((__gpio) * 4))
#define S5P6450_GPIO_INPUT(__gpio)	(0x0 << ((__gpio) * 4))
#define S5P6450_GPIO_OUTPUT(__gpio)	(0x1 << ((__gpio) * 4))

/* Use these macros for GPF/GPI/GPJ/GPN/GPP set of GPIO (2 bit)
 * */
#define S5P6450_GPIO2_CONMASK(__gpio)	(0x3 << ((__gpio) * 2))
#define S5P6450_GPIO2_INPUT(__gpio)	(0x0 << ((__gpio) * 2))
#define S5P6450_GPIO2_OUTPUT(__gpio)	(0x1 << ((__gpio) * 2))

#endif /* __ASM_ARCH_REGS_GPIO_H */
