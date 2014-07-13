/* linux/arch/arm/plat-s5phrtime/hr-time.c
 *
 * Copyright (c) 2010 Samsung Electronics Co., Ltd.
 * 		http://www.samsung.com
 *
 * S5PC11X (and compatible)  HRT support
 * PWM 2/4 is used for this feature
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/clk.h>
#include <linux/err.h>
#include <linux/clocksource.h>
#include <linux/clockchips.h>
#include <linux/io.h>

#include <asm/system.h>
#include <mach/hardware.h>
#include <asm/leds.h>
#include <asm/irq.h>
#include <asm/mach/irq.h>
#include <asm/mach/time.h>
#include <asm/mach-types.h>
#include <mach/map.h>
#include <plat/regs-timer.h>
#include <mach/regs-irq.h>
#include <mach/tick.h>

#include <plat/clock.h>
#include <plat/cpu.h>

static unsigned long long time_stamp;
static unsigned long long s5p_sched_timer_overflows;
static unsigned long long old_overflows;
static cycle_t last_ticks;




/* Sched timer interrupt is not processed right after
 * timer counter expired
 */
static unsigned int pending_irq;


/* sched_timer_running 
 * 0 : sched timer stopped or not initialized
 * 1 : sched timer started
 */
static unsigned int sched_timer_running;

static void s5phrtime_ticktimer_setup(void);

static int tick_timer_mode;     /* 0: oneshot, 1: autoreload */

static inline void s5phrtime_tick_set_autoreset(void)
{
	unsigned long tcon;

	tcon  = __raw_readl(S3C2410_TCON);
	tcon |= (1<<22);
	__raw_writel(tcon, S3C2410_TCON);
}

static inline void s5phrtime_tick_remove_autoreset(void)
{
	unsigned long tcon;

	tcon  = __raw_readl(S3C2410_TCON);
	tcon &= ~(1<<22);
	__raw_writel(tcon, S3C2410_TCON);
}

static void s5phrtime_tick_timer_start(unsigned long load_val,
					int autoreset)
{
	unsigned long tcon;
	unsigned long tcnt;
	unsigned long tcfg1;
	unsigned long tcfg0;
	unsigned long tcstat;
	unsigned long tcfg1_shift = S3C2410_TCFG1_SHIFT(4);

	tcon  = __raw_readl(S3C2410_TCON);
	tcfg1 = __raw_readl(S3C2410_TCFG1);
	tcfg0 = __raw_readl(S3C2410_TCFG0);

	tcstat = __raw_readl(S3C64XX_TINT_CSTAT);
	tcstat |=  0x10;
	__raw_writel(tcstat, S3C64XX_TINT_CSTAT);

	tcnt = load_val;
	tcnt--;
	__raw_writel(tcnt, S3C2410_TCNTB(4));

	tcfg1 &= ~(S3C64XX_TCFG1_MUX_MASK << tcfg1_shift);
	tcfg1 |= (S3C64XX_TCFG1_MUX_DIV1 << tcfg1_shift);

	tcfg0 &= ~S3C2410_TCFG_PRESCALER1_MASK;
	tcfg0 |= (0) << S3C2410_TCFG_PRESCALER1_SHIFT;

	__raw_writel(tcfg1, S3C2410_TCFG1);
	__raw_writel(tcfg0, S3C2410_TCFG0);

	tcon &= ~(7<<20);

	tcon |= S3C2410_TCON_T4MANUALUPD;

	if (autoreset)
		tcon |= S3C2410_TCON_T4RELOAD;

	__raw_writel(tcon, S3C2410_TCON);


	/* start the timer running */
	tcon |= S3C2410_TCON_T4START;
	tcon &= ~S3C2410_TCON_T4MANUALUPD;
	__raw_writel(tcon, S3C2410_TCON);

}

static inline void s5phrtime_tick_timer_stop(void)
{
	unsigned long tcon;

	tcon  = __raw_readl(S3C2410_TCON);
	tcon &= ~(1<<20);
	__raw_writel(tcon, S3C2410_TCON);
}

static void s5phrtime_sched_timer_start(unsigned long load_val,
					int autoreset)
{
	unsigned long tcon;
	unsigned long tcnt;
	unsigned long tcfg1;
	unsigned long tcfg0;
	unsigned long tcstat;

	tcstat = __raw_readl(S3C64XX_TINT_CSTAT);
	tcstat |=  0x04;
	__raw_writel(tcstat, S3C64XX_TINT_CSTAT);

	tcon  = __raw_readl(S3C2410_TCON);
	tcfg1 = __raw_readl(S3C2410_TCFG1);
	tcfg0 = __raw_readl(S3C2410_TCFG0);

	tcnt = load_val;
	tcnt--;
	__raw_writel(tcnt, S3C2410_TCNTB(2));
	__raw_writel(tcnt, S3C2410_TCMPB(2));

	tcon &= ~(0x0b<<12);

	if (autoreset)
		tcon |= S3C2410_TCON_T2RELOAD;

	tcon |= S3C2410_TCON_T2MANUALUPD;

	__raw_writel(tcon, S3C2410_TCON);

	/* start the timer running */
	tcon |= S3C2410_TCON_T2START;
	tcon &= ~S3C2410_TCON_T2MANUALUPD;
	__raw_writel(tcon, S3C2410_TCON);

	sched_timer_running = 1;
}

/*
 * ---------------------------------------------------------------------------
 * PWM timer 4 ... count down to zero, interrupt, reload
 * ---------------------------------------------------------------------------
 */
static int s5phrtime_tick_set_next_event(unsigned long cycles,
				   struct clock_event_device *evt)
{
	//printk("%d\n", cycles); 
        if  (cycles == 0)       /* Should be larger than 0 */
                cycles = 1;

	s5phrtime_tick_timer_start(cycles, 0);
	return 0;
}

static void s5phrtime_tick_set_mode(enum clock_event_mode mode,
			      struct clock_event_device *evt)
{
	switch (mode) {
	case CLOCK_EVT_MODE_PERIODIC:
		//s5phrtime_tick_set_autoreset();
		tick_timer_mode = 1;
		break;
	case CLOCK_EVT_MODE_ONESHOT:
		s5phrtime_tick_timer_stop();
		tick_timer_mode = 0;
		//s5phrtime_tick_remove_autoreset();
		break;
	case CLOCK_EVT_MODE_UNUSED:
	case CLOCK_EVT_MODE_SHUTDOWN:
		/* Sched timer stopped */
		sched_timer_running = 0;

/* Reset sched_clock variables after sleep/wakeup */
		last_ticks = 0;
		s5p_sched_timer_overflows = 0;
		old_overflows = 0;
		pending_irq = 0;

		break;
	case CLOCK_EVT_MODE_RESUME:
		s5phrtime_ticktimer_setup();
		s5phrtime_sched_timer_start(~0, 1);
		break;
	}
}

static struct clock_event_device clockevent_tick_timer = {
	.name		= "pwm_timer4",
	.features       = CLOCK_EVT_FEAT_PERIODIC | CLOCK_EVT_FEAT_ONESHOT,
	.shift		= 32,
	.set_next_event	= s5phrtime_tick_set_next_event,
	.set_mode	= s5phrtime_tick_set_mode,
};

irqreturn_t s5phrtime_tick_timer_interrupt(int irq, void *dev_id)
{
	struct clock_event_device *evt = &clockevent_tick_timer;

       /* In case of oneshot mode */
        if (tick_timer_mode == 0)
                s5phrtime_tick_timer_stop();

	evt->event_handler(evt);

	return IRQ_HANDLED;
}

static struct irqaction s5phrtime_tick_timer_irq = {
	.name		= "pwm_timer4",
	.flags		= IRQF_DISABLED | IRQF_TIMER | IRQF_IRQPOLL,
	.handler	= s5phrtime_tick_timer_interrupt,
};

static void __init s5phrtime_init_dynamic_tick_timer(unsigned long rate)
{
	tick_timer_mode = 1;

        s5phrtime_tick_timer_stop();

	s5phrtime_tick_timer_start((rate / HZ) - 1, 1);

	clockevent_tick_timer.mult = div_sc(rate, NSEC_PER_SEC,
					    clockevent_tick_timer.shift);
	clockevent_tick_timer.max_delta_ns =
		clockevent_delta2ns(-1, &clockevent_tick_timer);
	clockevent_tick_timer.min_delta_ns =
		clockevent_delta2ns(1, &clockevent_tick_timer);

	clockevent_tick_timer.cpumask = cpumask_of(0);
	clockevents_register_device(&clockevent_tick_timer);


        printk(KERN_INFO "mult[%ld]\n", clockevent_tick_timer.mult);
        printk(KERN_INFO "max_delta_ns[%ld]\n", clockevent_tick_timer.max_delta_ns);
        printk(KERN_INFO "min_delta_ns[%ld]\n", clockevent_tick_timer.min_delta_ns);
        printk(KERN_INFO "rate[%ld]\n", rate);
        printk(KERN_INFO "HZ[%d]\n", HZ);

}


/*
 * ---------------------------------------------------------------------------
 * PWM timer 2 ... free running 32-bit clock source and scheduler clock
 * ---------------------------------------------------------------------------
 */

irqreturn_t s5phrtime_mpu_timer2_interrupt(int irq, void *dev_id)
{
	//
	//s5p_sched_timer_overflows++;
	//
	
        if (unlikely(pending_irq))
                pending_irq = 0;
        else
                s5p_sched_timer_overflows++;

	return IRQ_HANDLED;
}

struct irqaction s5phrtime_timer2_irq = {
	.name		= "pwm_timer2",
	.flags		= IRQF_DISABLED ,
	.handler	= s5phrtime_mpu_timer2_interrupt,
};


static cycle_t s5phrtime_sched_timer_read(struct clocksource *cs)
{
	return (cycle_t) ~__raw_readl(S3C_TIMERREG(0x2c));
}

struct clocksource clocksource_s5p = {
	.name		= "clock_source_timer2",
	.rating		= 300,
	.read		= s5phrtime_sched_timer_read,
	.mask		= CLOCKSOURCE_MASK(32),
	.shift		= 20,
	.flags		= CLOCK_SOURCE_IS_CONTINUOUS ,
};

/*
 * Returns current time from boot in nsecs. It's OK for this to wrap
 * around for now, as it's just a relative time stamp.
 */

unsigned long long sched_clock(void)
{
        unsigned long irq_flags;
        cycle_t ticks, elapsed_ticks = 0;
        unsigned long long increment = 0;
        unsigned int overflow_cnt = 0;

        local_irq_save(irq_flags);

        if (likely(sched_timer_running)) {
                overflow_cnt = (s5p_sched_timer_overflows - old_overflows);
                ticks = s5phrtime_sched_timer_read(NULL);

                if (overflow_cnt) {
                        increment =
                                clocksource_cyc2ns((overflow_cnt - 1),
                                clocksource_s5p.mult,clocksource_s5p.shift);
                } else {
                        if (unlikely(last_ticks > ticks)) {
                                pending_irq = 1;
                                s5p_sched_timer_overflows++;
                        }
                }
                elapsed_ticks = (ticks - last_ticks) & clocksource_s5p.mask;

                time_stamp += clocksource_cyc2ns(elapsed_ticks,
                        clocksource_s5p.mult,clocksource_s5p.shift);

                old_overflows = s5p_sched_timer_overflows;
                last_ticks = ticks;
        }
        local_irq_restore(irq_flags);

        return time_stamp;
}



static void s5phrtime_init_clocksource(unsigned long rate)
{
	static char err[] __initdata = KERN_ERR
			"%s: can't register clocksource!\n";

	clocksource_s5p.mult
		= clocksource_khz2mult(rate/1000, clocksource_s5p.shift);

	s5phrtime_sched_timer_start(~0, 1);

	if (clocksource_register(&clocksource_s5p))
		printk(err, clocksource_s5p.name);
}

/*
 * ---------------------------------------------------------------------------
 *  Tick Timer initialization
 * ---------------------------------------------------------------------------
 */
static void s5phrtime_dynamic_timer_setup(void)
{
	struct clk	*ck_ref = clk_get(NULL, "timers");
	unsigned long	rate;

	if (IS_ERR(ck_ref))
		panic("failed to get clock for system timer");

	rate = clk_get_rate(ck_ref);
	clk_put(ck_ref);

	s5phrtime_init_dynamic_tick_timer(rate);
	s5phrtime_init_clocksource(rate);

}


static void s5phrtime_ticktimer_setup(void)
{
	struct clk	*ck_ref = clk_get(NULL, "timers");
	unsigned long	rate;

	if (IS_ERR(ck_ref))
		panic("failed to get clock for system timer");

	rate = clk_get_rate(ck_ref);
	clk_put(ck_ref);

	/* Restart tick timer */
	s5phrtime_tick_timer_start((rate / HZ) - 1, 1);

#if 0
	/* Reset sched_clock variables after sleep/wakeup */
	last_ticks = 0;
	s5p_sched_timer_overflows = 0;
	old_overflows = 0;

	/* Restart sched timer */
	s5phrtime_sched_timer_start(~0, 1);
#endif
}


static void s5phrtime_dynamic_timer_init(void)
{
	/* Initialize variables before starting each timers */
	last_ticks = 0;
	s5p_sched_timer_overflows = 0;
	old_overflows = 0;
	time_stamp = 0;
	sched_timer_running = 0;
	pending_irq = 0;

	s5phrtime_dynamic_timer_setup();
	setup_irq(IRQ_TIMER2, &s5phrtime_timer2_irq);
	setup_irq(IRQ_TIMER4, &s5phrtime_tick_timer_irq);
}


struct sys_timer s5p_systimer = {
	.init		= s5phrtime_dynamic_timer_init,
};

