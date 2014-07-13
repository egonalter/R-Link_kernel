/*
 * Hibernation support specific for ARM
 *
 * Derived from work on ARM hibernation support by:
 *
 * Ubuntu project, hibernation support for mach-dove
 * Copyright (C) 2010 Nokia Corporation (Hiroshi Doyu)
 * Copyright (C) 2010 Texas Instruments, Inc. (Teerth Reddy et al.)
 *	https://lkml.org/lkml/2010/6/18/4
 *	https://lists.linux-foundation.org/pipermail/linux-pm/2010-June/027422.html
 *	https://patchwork.kernel.org/patch/96442/
 *
 * Copyright (C) 2006 Rafael J. Wysocki <rjw@sisk.pl>
 *
 * License terms: GNU General Public License (GPL) version 2
 */

#include <linux/mm.h>
#include <linux/suspend.h>
#include <asm/tlbflush.h>
#include <asm/cacheflush.h>

extern const void __nosave_begin, __nosave_end;

int pfn_is_nosave(unsigned long pfn)
{
	unsigned long nosave_begin_pfn = __pa_symbol(&__nosave_begin) >> PAGE_SHIFT;
	unsigned long nosave_end_pfn = PAGE_ALIGN(__pa_symbol(&__nosave_end)) >> PAGE_SHIFT;

	return (pfn >= nosave_begin_pfn) && (pfn < nosave_end_pfn);
}

void notrace save_processor_state(void)
{
	WARN_ON(num_online_cpus() != 1);
	flush_thread();
	local_fiq_disable();
}

void notrace restore_processor_state(void)
{
	local_fiq_enable();
}

u8 __swsusp_resume_stk[PAGE_SIZE/2] __nosavedata;
static int __swsusp_arch_resume_ret;

static void notrace __swsusp_arch_reset(void)
{
	extern void cpu_resume(void);
	void (*phys_reset)(unsigned long) =
		(void (*)(unsigned long))virt_to_phys(cpu_reset);

	flush_tlb_all();
	flush_cache_all();

	identity_mapping_add(swapper_pg_dir, 0, TASK_SIZE);
	flush_tlb_all();
	flush_cache_all();

	cpu_proc_fin();
	flush_tlb_all();
	flush_cache_all();

	phys_reset(virt_to_phys(cpu_resume));
}

static int __swsusp_arch_resume_finish(void)
{
	cpu_init();
	identity_mapping_del(swapper_pg_dir, 0, TASK_SIZE);
	flush_tlb_all();
	flush_cache_all();

	return __swsusp_arch_resume_ret;
}

/*
 * Snapshot kernel memory and reset the system.
 * After resume, the hibernation snapshot is written out.
 */
static void notrace __swsusp_arch_save_image(void)
{
	extern int swsusp_save(void);

	flush_cache_all();
	__swsusp_arch_resume_ret = swsusp_save();

	cpu_switch_mm(swapper_pg_dir, &init_mm);
	__swsusp_arch_reset();
}

/*
 * The framework loads the hibernation image into a linked list anchored
 * at restore_pblist, for swsusp_arch_resume() to copy back to the proper
 * destinations.
 *
 * To make this work if resume is triggered from initramfs, the
 * pagetables need to be switched to allow writes to kernel mem.
 */
static void notrace __swsusp_arch_restore_image(void)
{
	extern struct pbe *restore_pblist;
	struct pbe *pbe;

	cpu_switch_mm(swapper_pg_dir, &init_mm);

	for (pbe = restore_pblist; pbe; pbe = pbe->next)
		copy_page(pbe->orig_address, pbe->address);

	/*
	 * Done - reset and resume from the hibernation image.
	 */
	__swsusp_arch_resume_ret = 0;	/* success at this point */
	__swsusp_arch_reset();
}

/*
 * Save the current CPU state before suspend / poweroff.
 */
int notrace swsusp_arch_suspend(void)
{
	extern void cpu_suspend(unsigned long, unsigned long, unsigned long, void *);

	cpu_suspend(0, virt_to_phys(0), 0, __swsusp_arch_save_image);

	/*
	 * After resume, execution restarts here. Clean up and return.
	 */
	return __swsusp_arch_resume_finish();
}

/*
 * Resume from the hibernation image.
 * Due to the kernel heap / data restore, stack contents change underneath
 * and that would make function calls impossible; switch to a temporary
 * stack within the nosave region to avoid that problem.
 */
int __naked swsusp_arch_resume(void)
{
	cpu_init();	/* get a clean PSR */

	/*
	 * when switch_stack() becomes available, should use that
	 */
	__asm__ __volatile__("mov	sp, %0\n\t"
		: : "r"(__swsusp_resume_stk + sizeof(__swsusp_resume_stk))
		: "memory", "cc");

	__swsusp_arch_restore_image();

	return 0;
}
