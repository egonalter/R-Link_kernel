/*
 * dsasig.c - dsa signature checking
 *
 * 2009-10-19 Ard Biesheuvel <ard.biesheuvel@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/crypto/mpi.h>
#include <linux/crypto/dsasig.h>
#include <asm/errno.h>

#define L	(sizeof(P0)*8)
#define N	(sizeof(Q0)*8)

#include "dsa_pubkey.h"

/*
 * DSA signature verification algorithm:
 * =====================================
 *
 * Let   (p,q,g,y)     = public key
 *       d = H(m)      = digest of message
 *       (r,s)         = signature
 *
 * then calculate   w  = 1/s mod q
 *                  u1 = (d * w) mod q
 *                  u2 = (r * w) mod q
 *                  v  = ((g^u1 * y^u2) mod p) mod q
 * 
 * Signature is valid if v == r
 */

int dsa_verify_hash( unsigned char *hash, unsigned char *sig, int keyidx )
{
	mpi_limb_t	D [ SHA1_DIGEST_SIZE / BYTES_PER_MPI_LIMB ],
			R [   N/BITS_PER_MPI_LIMB ],
			S [   N/BITS_PER_MPI_LIMB ],
			W [   N/BITS_PER_MPI_LIMB ],
			U1[ 2*N/BITS_PER_MPI_LIMB+1 ],
			U2[ 2*N/BITS_PER_MPI_LIMB+1 ],
			V [ 2*L/BITS_PER_MPI_LIMB+1 ];

	struct _mpi
		d  = { .alloced = SHA1_DIGEST_SIZE / BYTES_PER_MPI_LIMB, .d = D },
		r  = { .alloced =   N/BITS_PER_MPI_LIMB,   .d = R },
		s  = { .alloced =   N/BITS_PER_MPI_LIMB,   .d = S },
		w  = { .alloced =   N/BITS_PER_MPI_LIMB,   .d = W },
		u1 = { .alloced = 2*N/BITS_PER_MPI_LIMB+1, .d = U1 },
		u2 = { .alloced = 2*N/BITS_PER_MPI_LIMB+1, .d = U2 },
		v  = { .alloced = 2*L/BITS_PER_MPI_LIMB+1, .d = V };

	MPI exp[] = { &u1, &u2, NULL };

	if (keyidx >= dsa_key_count)
		return -ENOENT;

	mpi_set_buffer( &d, hash,	SHA1_DIGEST_SIZE, 0);
	mpi_set_buffer( &r, &sig[2],	sig[1]/8, 0);

	sig += sig[1]/8+2;

	mpi_set_buffer( &s, &sig[2],	sig[1]/8, 0);

	/* make sure 0 < r < q and 0 < s < q */
	if (!mpi_cmp_ui(&r, 0) || !mpi_cmp_ui(&s, 0)
			|| mpi_cmp(&r, (MPI)&q[keyidx]) >= 0 || mpi_cmp(&s, (MPI)&q[keyidx]) >= 0)
		return -EFAULT;

	if (0 > mpi_invm(&w, &s, &q[keyidx]))			return -ENOMEM;
	if (0 > mpi_mulm(&u1, &d, &w, &q[keyidx]))		return -ENOMEM;
	if (0 > mpi_mulm(&u2, &r, &w, &q[keyidx]))		return -ENOMEM;
	if (0 > mpi_mulpowm(&v, base[keyidx], exp, &p[keyidx]))	return -ENOMEM;
	if (0 > mpi_fdiv_r(&v, &v, &q[keyidx]))			return -ENOMEM;

	return !!mpi_cmp(&v, &r);
}

