/*
 * ttcrypto_ctx.h - format to share crypto context between uboot and kernel
 *
 * 2010-03-08 Ard Biesheuvel <ard.biesheuvel@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#define TTCRYPTO_CTX_MAGIC	(0xAB1E5BAD)
#define TTCRYPTO_CTX_VERSION	(1)

/* we put the ttcrypto context on the next page boundary */
#define TTCRYPTO_ALIGN_MASK	((1 << 12)-1)
#define TTCRYPTO_ALIGN(a)	(((unsigned long)(a) + TTCRYPTO_ALIGN_MASK) & ~TTCRYPTO_ALIGN_MASK)

struct ttcrypto_ctx {
	unsigned int	magic;
	unsigned int	version;
	unsigned char	shared_secret[20];
};

#define TTCRYPTO_INIT(ctx,addr,size) \
	do {									\
		(ctx) = TTCRYPTO_ALIGN((addr)+(size));				\
		(size) = TTCRYPTO_ALIGN(size) + sizeof(struct ttcrypto_ctx);	\
		ctx->magic = TTCRYPTO_CTX_MAGIC;				\
		ctx->version = TTCRYPTO_CTX_VERSION;				\
	} while (0)

