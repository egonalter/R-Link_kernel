/*
 * Arm specific backtracing code for oprofile
 *
 * Copyright 2005 Openedhand Ltd.
 *
 * Author: Richard Purdie <rpurdie@openedhand.com>
 *
 * Based on i386 oprofile backtrace code by John Levon, David Smith
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/oprofile.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <asm/ptrace.h>
#include <asm/stacktrace.h>

static int report_trace(struct stackframe *frame, void *d)
{
	unsigned int *depth = d;

	if (*depth) {
		oprofile_add_trace(frame->pc);
		(*depth)--;
	}

	return *depth == 0;
}

static int user_valid_lr(struct mm_struct *mm, unsigned long lr)
{
	struct vm_area_struct *vma;
	unsigned long bl;

	/* If not THUMB mode, must be ARM mode */
	if (!(lr & 1) && (lr & 3))
		return 0;

	vma = find_vma(mm, lr);
	/* Test whether the address is valid and executable */
	if (!vma ||
	  vma->vm_start >= lr ||
	  !(vma->vm_flags & VM_EXEC) ||
	  !access_ok(VERIFY_READ, (lr & ~1) - sizeof(bl), sizeof(bl)))
		return 0;

	/* Test whether the instruction before *lr is a branch */
	if (__copy_from_user_inatomic(&bl,
	      (void*)((lr & ~1) - sizeof(bl)), sizeof(bl)))
		return 0;

	return
	  ((lr & 1) && ( /* THUMB mode */
	    (bl & 0xD000F800) == 0xD000F000 ||
	    (bl & 0xD001F800) == 0xC000F000 ||
	    (bl & 0xFF800000) == 0x47800000)) ||
	  (!(lr & 1) && ( /* ARM mode */
	    (bl & 0x0F000000) == 0x0B000000 ||
	    (bl & 0xFE000000) == 0xFA000000 ||
	    (bl & 0x0FF000F0) == 0x01200030));
}

void arm_backtrace(struct pt_regs * const regs, unsigned int depth)
{
	struct mm_struct *mm;
	unsigned long vmlow, sp, lr, spbuf[0x40];
	unsigned int limit = 0x40, spbuflen = 0, spbufidx = 0;

	if (!user_mode(regs)) {
		struct stackframe frame;
		frame.fp = regs->ARM_fp;
		frame.sp = regs->ARM_sp;
		frame.lr = regs->ARM_lr;
		frame.pc = regs->ARM_pc;
		walk_stackframe(&frame, report_trace, &depth);
		return;
	}

	mm = current->mm;
	if (!mm || !mm->mmap)
		return;

	vmlow = mm->mmap->vm_start;
	sp = regs->ARM_sp;
	lr = regs->ARM_lr;

	for (; limit && depth; --limit) {
		if (lr >= vmlow && /* sanity check */
		    user_valid_lr(mm, lr)) {
			oprofile_add_trace(lr);
			limit = 0x40;
			--depth;
		}
		if (spbufidx * sizeof(spbuf[0]) >= spbuflen) {
			if (!access_ok(VERIFY_READ, sp, sizeof(spbuf)))
				break;

			spbufidx = 0;
			spbuflen = sizeof(spbuf) - __copy_from_user_inatomic(
			  spbuf, (void*)sp, sizeof(spbuf));
			if (!spbuflen)
				break;
			sp += spbuflen;
		}
		lr = spbuf[spbufidx++];
	}
}
