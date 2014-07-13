/*
 * tt_crypto.h - tt specific crypto using crypto context passed by the bootloader
 *
 * 2010-03-09 Ard Biesheuvel <ard.biesheuvel@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifdef __KERNEL__
#include <asm/ioctl.h>
#else
#include <sys/ioctl.h>
#endif

struct ttcrypto_ss_crypt {
	unsigned int	size;		/* size of data */
	unsigned char	data[0];	/* data to decrypt (in place) */
};

struct ttcrypto_ss_hmac {
	unsigned int	size;		/* size of data */
	unsigned char	hmac[20];	/* resulting hmac */
	unsigned char	data[0];	/* data to hash */
};

#define	TTCRYPTO_SS_DECRYPT	_IOW('C',1,struct ttcrypto_ss_crypt*)
#define	TTCRYPTO_SS_DECRYPT_CBC	_IOW('C',3,struct ttcrypto_ss_crypt*)
#define	TTCRYPTO_SS_ENCRYPT_CBC	_IOW('C',4,struct ttcrypto_ss_crypt*)
#define	TTCRYPTO_SS_HMAC	_IOW('C',2,struct ttcrypto_ss_hmac*)

