/*
 * tt_brick.c - check hardcoded version# against revocation count
 *
 * 2010-02-09 Ard Biesheuvel <ard.biesheuvel@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/kernel.h>
#include <linux/init.h>

/*
 * Increment this number after plugging an exploitable hole in the kernel, and pass this number
 * on the kernel command line from U-Boot like 'brick=N'
 */

#define BRICK_INDEX	2

static int __init parse_brick_count(char *arg)
{
	int brick_count;

	get_option(&arg, &brick_count);

	if (brick_count > BRICK_INDEX)
		panic("Refusing to boot revoked kernel! (brick_count == %d, brick_index == %d)\n",
			brick_count, BRICK_INDEX);

	return 0;
}

early_param("brick", parse_brick_count);

