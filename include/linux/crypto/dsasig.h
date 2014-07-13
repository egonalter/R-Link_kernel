/*
 * dsasig.h - dsa signature checking
 *
 * 2009-10-19 Ard Biesheuvel <ard.biesheuvel@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef SHA1_DIGEST_SIZE
#define SHA1_DIGEST_SIZE	20
#endif

#define DSASIG_SIZE		44

int dsa_verify_hash( unsigned char *hash, unsigned char *sig, int keyidx );

#define ROOTFS_KEY	0
#define KERNEL_PROD_KEY 1
#define LOOPFS_PROD_KEY	2
#define LOOPFS_DEV_KEY	3

