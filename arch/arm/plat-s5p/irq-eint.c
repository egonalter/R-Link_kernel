/* arch/arm/plat-s5p64xx/irq-eint.c
 *
 * Copyright 2008 Openmoko, Inc.
 * Copyright 2008 Simtec Electronics
 *      Ben Dooks <ben@simtec.co.uk>
 *      http://armlinux.simtec.co.uk/
 *
 * S5P64XX - Interrupt handling for IRQ_EINT(x)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/io.h>

#include <asm/hardware/vic.h>

#include <plat/regs-irqtype.h>

#include <mach/map.h>
#include <plat/cpu.h>

#include <mach/gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/gpio-bank-n.h>


/* GPIO is 0x7F008xxx, */
#define S5P64XX_GPIOREG(x)	(S5P64XX_VA_GPIO + (x))

#define S5P64XX_EINT0CON0	S5P64XX_GPIOREG(0x900)
#define S5P64XX_EINT0FLTCON0	S5P64XX_GPIOREG(0x910)
#define S5P64XX_EINT0FLTCON1	S5P64XX_GPIOREG(0x914)

#define S5P64XX_EINT0MASK	S5P64XX_GPIOREG(0x920)
#define S5P64XX_EINT0PEND	S5P64XX_GPIOREG(0x924)


#define eint_offset(irq)	((irq) - IRQ_EINT(0))
#define eint_irq_to_bit(irq)	(1 << eint_offset(irq))

static inline void s3c_irq_eint_mask(unsigned int irq)
{
	u32 mask;

	mask = __raw_readl(S5P64XX_EINT0MASK);
	mask |= eint_irq_to_bit(irq);
	__raw_writel(mask, S5P64XX_EINT0MASK);
}

static void s3c_irq_eint_unmask(unsigned int irq)
{
	u32 mask;

	mask = __raw_readl(S5P64XX_EINT0MASK);
	mask &= ~(eint_irq_to_bit(irq));
	__raw_writel(mask, S5P64XX_EINT0MASK);
}

static inline void s3c_irq_eint_ack(unsigned int irq)
{
	__raw_writel(eint_irq_to_bit(irq), S5P64XX_EINT0PEND);
}

static void s3c_irq_eint_maskack(unsigned int irq)
{
	/* compiler should in-line these */
	s3c_irq_eint_mask(irq);
	s3c_irq_eint_ack(irq);
}

static int s3c_irq_eint_set_type(unsigned int irq, unsigned int type)
{
	int offs = eint_offset(irq);
	int shift;
	u32 ctrl, mask;
	u32 newvalue = 0;
	void __iomem *reg;

	if (offs > 15)
		return -EINVAL;
	else
		reg = S5P64XX_EINT0CON0;

	switch (type) {
	case IRQ_TYPE_NONE:
		printk(KERN_WARNING "No edge setting!\n");
		break;

	case IRQ_TYPE_EDGE_RISING:
		newvalue = S3C2410_EXTINT_RISEEDGE;
		break;

	case IRQ_TYPE_EDGE_FALLING:
		newvalue = S3C2410_EXTINT_FALLEDGE;
		break;

	case IRQ_TYPE_EDGE_BOTH:
		newvalue = S3C2410_EXTINT_BOTHEDGE;
		break;

	case IRQ_TYPE_LEVEL_LOW:
		newvalue = S3C2410_EXTINT_LOWLEV;
		break;

	case IRQ_TYPE_LEVEL_HIGH:
		newvalue = S3C2410_EXTINT_HILEV;
		break;

	default:
		printk(KERN_ERR "No such irq type %d", type);
		return -1;
	}

	shift = (offs / 2) * 4;
	mask = 0x7 << shift;

	ctrl = __raw_readl(reg);
	ctrl &= ~mask;
	ctrl |= newvalue << shift;
	__raw_writel(ctrl, reg);

	s3c_gpio_cfgpin(S5P64XX_GPN(offs), 0x2 << (offs * 2));

	return 0;
}

static struct irq_chip s3c_irq_eint = {
	.name		= "s3c-eint",
	.mask		= s3c_irq_eint_mask,
	.unmask		= s3c_irq_eint_unmask,
	.mask_ack	= s3c_irq_eint_maskack,
	.ack		= s3c_irq_eint_ack,
	.set_type	= s3c_irq_eint_set_type,
};

/* s3c_irq_demux_eint
 *
 * This function demuxes the IRQ from the group0 external interrupts,
 * from IRQ_EINT(0) to IRQ_EINT(15). It is designed to be inlined into
 * the specific handlers s3c_irq_demux_eintX_Y.
 */
static inline void s3c_irq_demux_eint(unsigned int start, unsigned int end)
{
	u32 status = __raw_readl(S5P64XX_EINT0PEND);
	u32 mask = __raw_readl(S5P64XX_EINT0MASK);
	unsigned int irq;

	status &= ~mask;
	status >>= start;
	status &= (1 << (end - start + 1)) - 1;

	for (irq = IRQ_EINT(start); irq <= IRQ_EINT(end); irq++) {
		if (status & 1)
			generic_handle_irq(irq);

		status >>= 1;
	}
}

static void s3c_irq_demux_eint0_3(unsigned int irq, struct irq_desc *desc)
{
	s3c_irq_demux_eint(0, 3);
}

static void s3c_irq_demux_eint4_11(unsigned int irq, struct irq_desc *desc)
{
	s3c_irq_demux_eint(4, 11);
}

static void s3c_irq_demux_eint12_15(unsigned int irq, struct irq_desc *desc)
{
	s3c_irq_demux_eint(12, 15);
}

int __init s5p64xx_init_irq_eint(void)
{
	int irq;

	for (irq = IRQ_EINT(0); irq <= IRQ_EINT(15); irq++) {
		set_irq_chip(irq, &s3c_irq_eint);
		set_irq_handler(irq, handle_level_irq);
		set_irq_flags(irq, IRQF_VALID);
	}

	set_irq_chained_handler(IRQ_EINT0_3, s3c_irq_demux_eint0_3);
	set_irq_chained_handler(IRQ_EINT4_11, s3c_irq_demux_eint4_11);
	set_irq_chained_handler(IRQ_EINT12_15, s3c_irq_demux_eint12_15);

	return 0;
}

arch_initcall(s5p64xx_init_irq_eint);


#ifdef GPIO_INTR_DEBUG

#include <plat/gpio-core.h>
#include <plat/gpio-cfg.h>
#include <plat/gpio-cfg-helpers.h>

#define gpio_print  printk
#else
#define gpio_print(...)
#endif

#define S5P6450_GPIOINT_CONBASE		(S5P_VA_GPIO + 0x200)
#define S5P6450_GPIOINT_FLTBASE		(S5P_VA_GPIO + 0x220)
#define S5P6450_GPIOINT_MASKBASE	(S5P_VA_GPIO + 0x240)
#define S5P6450_GPIOINT_PENDBASE	(S5P_VA_GPIO + 0x260)
#define S5P6450_GPIOINT_SERVICE		(S5P_VA_GPIO + 0x284)
#define S5P6450_GPIOINT_SERVICEPEND	(S5P_VA_GPIO + 0x288)


#define S3C_IRQ_GPIOINT_BASE   IRQ_EINT_GROUP_BASE
#define S3C_IRQ_GPIOINT_END     (IRQ_EINT_GROUP8_BASE + IRQ_EINT_GROUP8_NR)
#define S3C_IRQ_GPIOINT_NUMS     ( S3C_IRQ_GPIOINT_END - S3C_IRQ_GPIOINT_BASE )

struct s3c_gpioint_cfg {
	unsigned int irq_start;	
	unsigned int gpio_start;	
	unsigned int irq_nums;	
	unsigned int reg_offset;	
	unsigned int bit_offset;	
};

struct s3c_gpioint_mux {
	unsigned int irq_base;	
	unsigned int offset;	
	unsigned int mask;	
};

#define gpioint_irq_to_bit(irq,cfg)	(1 << ((irq - cfg->irq_start) + cfg->bit_offset))
#define  S3C_GPIOINT_MAXCFGS		6

struct s3c_gpioint_cfg s3c_gpioint_info[S3C_GPIOINT_MAXCFGS] = 
	{
		{
			.reg_offset  = 0x0,
			.bit_offset  = 0,
			.irq_start   = IRQ_EINT_GROUP1_BASE,
			.gpio_start  = S5P6450_GPIO_A_START, 
			.irq_nums    = 8,
		},
		{
			.reg_offset  = 0x0,
			.bit_offset  = 8,
			.irq_start   = IRQ_EINT_GROUP1_BASE + 8,
			.gpio_start  = S5P6450_GPIO_B_START, 
			.irq_nums    = 7,
		},
		{
			.reg_offset  = 0x0,
			.bit_offset  = 16,
			.irq_start   = IRQ_EINT_GROUP2_BASE ,
			.gpio_start  = S5P6450_GPIO_C_START, 
			.irq_nums    = IRQ_EINT_GROUP2_NR ,
		},
		{
			.reg_offset  = 0x8,
			.bit_offset  = 0,
			.irq_start   = IRQ_EINT_GROUP5_BASE ,
			.gpio_start  = S5P6450_GPIO_G_START, 
			.irq_nums    = IRQ_EINT_GROUP5_NR ,
		},
		{
			.reg_offset  = 0x8,
			.bit_offset  = 16,
			.irq_start   = IRQ_EINT_GROUP6_BASE ,
			.gpio_start  = S5P6450_GPIO_H_START, 
			.irq_nums    = IRQ_EINT_GROUP6_NR ,
		},
		{
			.reg_offset  = 0xc,
			.bit_offset  = 16,
			.irq_start   = IRQ_EINT_GROUP8_BASE ,
			.gpio_start  = S5P6450_GPIO_P_START, 
			.irq_nums    = IRQ_EINT_GROUP8_NR ,
		},
	};


unsigned int s3c_gpioint_groups[] = {
		 0,
		IRQ_EINT_GROUP1_BASE,
		IRQ_EINT_GROUP2_BASE,
		0,
		0,
		IRQ_EINT_GROUP5_BASE,
		IRQ_EINT_GROUP6_BASE,
		0,
		IRQ_EINT_GROUP8_BASE,
};

static struct s3c_gpioint_cfg *s3c_gpioint_cfgs[S3C_IRQ_GPIOINT_NUMS];

static struct s3c_gpioint_cfg* s3c_irq_gpioint_tocfg(unsigned int irq)
{
	unsigned int i;	
	struct s3c_gpioint_cfg *cfg;
	
	for(i = 0; i < S3C_GPIOINT_MAXCFGS; i++) {
		cfg = &s3c_gpioint_info[i];

		if(irq < (cfg->irq_start + cfg->irq_nums))
			return cfg;
	}
		 
		
	return NULL;

}

static inline struct s3c_gpioint_cfg* s3c_irq_gpioint_getcfg(unsigned int irq)
{
	return s3c_gpioint_cfgs[irq - S3C_IRQ_GPIOINT_BASE];

}

static inline void s3c_irq_gpioint_mask(unsigned int irq)
{
	u32 mask;
	void __iomem *reg;
	struct s3c_gpioint_cfg *cfg;

	cfg = s3c_irq_gpioint_getcfg(irq);

        reg = S5P6450_GPIOINT_MASKBASE + cfg->reg_offset;


	mask = __raw_readl(reg);
	mask |= gpioint_irq_to_bit(irq,cfg);
	__raw_writel(mask, reg);
	gpio_print("mask irq %d, mask  %x, off %d,reg %x\n",irq,mask,irq - cfg->irq_start,reg);
	gpio_print("bit.... %x\n",gpioint_irq_to_bit(irq,cfg));
}

static void s3c_irq_gpioint_unmask(unsigned int irq)
{
	u32 mask;
	void __iomem *reg;
	struct s3c_gpioint_cfg *cfg;

	if(irq == 138)
		dump_stack();

	cfg = s3c_irq_gpioint_getcfg(irq);
        reg = S5P6450_GPIOINT_MASKBASE + cfg->reg_offset;

	mask = __raw_readl(reg);
	mask &= ~(gpioint_irq_to_bit(irq,cfg));
	__raw_writel(mask, reg);

	gpio_print("unmask irq %d, mask  %x, off %d,reg %x\n",irq,mask,irq - cfg->irq_start,reg);
	gpio_print("bit.... %x\n",gpioint_irq_to_bit(irq,cfg));
}

static inline void s3c_irq_gpioint_ack(unsigned int irq)
{
	struct s3c_gpioint_cfg *cfg;
	void __iomem *reg;

	cfg = s3c_irq_gpioint_getcfg(irq);
        reg = S5P6450_GPIOINT_PENDBASE + cfg->reg_offset;

	__raw_writel(gpioint_irq_to_bit(irq,cfg),reg);

	gpio_print("ack..:bit.... %x\n",gpioint_irq_to_bit(irq,cfg));
}

static void s3c_irq_gpioint_maskack(unsigned int irq)
{
	/* compiler should in-line these */
	s3c_irq_gpioint_mask(irq);
	s3c_irq_gpioint_ack(irq);
}

static int s3c_irq_gpioint_set_type(unsigned int irq, unsigned int type)
{
	int offs,pin;
	int shift;
	u32 ctrl, mask;
	u32 newvalue = 0;
	struct s3c_gpioint_cfg *cfg;
	void __iomem *reg;

	if( (irq > S3C_IRQ_GPIOINT_END) || (irq < S3C_IRQ_GPIOINT_BASE))
		__WARN_printf("GPIO Interrupt number is not valid \n");


	cfg = s3c_irq_gpioint_getcfg(irq);

        switch (type) {
        case IRQ_TYPE_NONE:
		printk(KERN_WARNING "No edge setting!\n");
		break;

	case IRQ_TYPE_EDGE_RISING:
		newvalue = S3C2410_EXTINT_RISEEDGE;
		break;

	case IRQ_TYPE_EDGE_FALLING:
		newvalue = S3C2410_EXTINT_FALLEDGE;
		break;

	case IRQ_TYPE_EDGE_BOTH:
		newvalue = S3C2410_EXTINT_BOTHEDGE;
		break;

        case IRQ_TYPE_LEVEL_LOW:
		newvalue = S3C2410_EXTINT_LOWLEV;
		break;

        case IRQ_TYPE_LEVEL_HIGH:
		newvalue = S3C2410_EXTINT_HILEV;
                break;

	default:
		printk(KERN_ERR "No such irq type %d", type);
                return -1;
        }

	pin = irq - cfg->irq_start;
	offs =  pin/4;

	shift = (offs & 0x7) * 4  + cfg->bit_offset;
	mask = 0x7 << shift;

        reg = S5P6450_GPIOINT_CONBASE + cfg->reg_offset;
	
	if (irq >= IRQ_EINT_GROUP8_BASE) {
		s3c_gpio_cfgpin(cfg->gpio_start + pin, 0x3  << (pin * 2));
	} else {
		s3c_gpio_cfgpin(cfg->gpio_start + pin, (0x7  <<(pin * 4)));
	}

	ctrl = __raw_readl(reg);
	ctrl &= ~mask;
	ctrl |= newvalue << shift;
	__raw_writel(ctrl, reg);

	gpio_print("type irq %d, ctrl  %x, off %d,reg %x \n", irq,ctrl,offs,reg);

#ifdef GPIO_INTR_DEBUG
	struct s3c_gpio_chip *chip = s3c_gpiolib_getchip( cfg->gpio_start + offs);
	gpio_print(" pin = %d gpio_reg : %x\n",pin, *((unsigned int *)chip->base));
#endif

	return 0;
}

static void s3c_irq_demux_gpioint(unsigned int gpioirq, struct irq_desc *desc)
{
	u32 service  = __raw_readl(S5P6450_GPIOINT_SERVICE);
	u32 group,index = 0 ;
	unsigned int irq;
	
	group = (service >> 4 )	& 0xf;
	index  = service & 0xf;

	irq = s3c_gpioint_groups[group] + index;
	
	gpio_print("demux irq %d, index %d, group  %d\n",irq,index,group);
 
	generic_handle_irq(irq);
}

static struct irq_chip s3c_irq_gpioint = {
	.name		= "s3c-gpioint",
	.mask		= s3c_irq_gpioint_mask,
	.unmask		= s3c_irq_gpioint_unmask,
	.mask_ack	= s3c_irq_gpioint_maskack,
	.ack		= s3c_irq_gpioint_ack,
	.set_type	= s3c_irq_gpioint_set_type,
};

int __init s5p6450_init_irq_gpioint(void)
{
	int irq;
	struct s3c_gpioint_cfg *cfg;

	for (irq =  IRQ_EINT_GROUP_BASE; irq <= S3C_IRQ_GPIOINT_END; irq++) {
		cfg = s3c_irq_gpioint_tocfg(irq);
		s3c_gpioint_cfgs[irq - S3C_IRQ_GPIOINT_BASE] = cfg;
		set_irq_chip(irq, &s3c_irq_gpioint);
		set_irq_handler(irq, handle_level_irq);
		set_irq_flags(irq, IRQF_VALID);
	}

	set_irq_chained_handler( IRQ_EINT_GROUPS, s3c_irq_demux_gpioint);
	return 0;
}

arch_initcall(s5p6450_init_irq_gpioint);

