/*
 * module_hash.c - check loaded modules against a hardcoded list of hashes
 *
 * 2009-10-20 Ard Biesheuvel <ard.biesheuvel@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/crypto.h>
#include <linux/scatterlist.h>
#include <linux/err.h>
#include <linux/module_hash.h>

#include "modhashes.h"

static int const modcount = sizeof(modhashes)/20;

static int hashcmp(char const *l, char const *r)
{
	int i;

	for (i = 0; i < 20; i++)
		if (l[i]-r[i])
			return (l[i]-r[i]) < 0 ? -1 : 1;
	return 0;
}

long check_module_hash(char *mod, unsigned int len)
{
	/* caller holds module_mutex, so no concurrency */
	static struct hash_desc desc;
	static struct scatterlist sg[ MAX_MODULE_SIZE >> PAGE_SHIFT ];

	int i, m, n, pages = (len + PAGE_SIZE) >> PAGE_SHIFT;
	unsigned char sha1_result[20];

	/* allocate on first invocation */
	if (!desc.tfm)
		desc.tfm = crypto_alloc_hash("sha1", 0, CRYPTO_ALG_ASYNC);

	if (IS_ERR(desc.tfm))
		return PTR_ERR(desc.tfm);

	sg_init_table( sg, pages );

	for (i = 0; i < pages; i++)
		sg_set_buf( &sg[i], &mod[PAGE_SIZE * i], PAGE_SIZE );

	crypto_hash_digest(&desc, sg, len, sha1_result);

	/* use the first nibble to decide where to start searching, and take single steps from there */
	for (n = 0, i = ((sha1_result[0] >> 4) * modcount) >> 4; i >= 0 && i < modcount; i += (n = m)) 
	{
		if (!(m = hashcmp(sha1_result, modhashes[i])))
			return 0;

		/* bail if we are about to change direction */
		if (n && (m < 0)^(n < 0))
			break;
	}
	return -EPERM;
}

