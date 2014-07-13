/*
 * pubkey2c.c - export a dsa public key for inclusion in the kernel
 *
 * 2009-10-19 Ard Biesheuvel <ard.biesheuvel@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include <openssl/pem.h>

static void print_mpi( FILE *fd, int idx, char const *name, BIGNUM *num)
{
	int i, n = BN_num_bytes(num);
	unsigned char *buf = malloc(n);

	BN_bn2bin( num, buf );

	fprintf(fd, "static mpi_limb_t const %s%d[] = {", name, idx);
	for (i = 0; i < n; i += 4) {
		if ( !(i % 32))
			fprintf(fd, "\n\t");
		fprintf(fd, "0x%08x, ", ntohl(*(unsigned long*)&buf[n-i-4]));
	}
	fprintf(fd, "};\n");

	free(buf);
}

static void print_struct( FILE *fd, char const *name, char const *mpiname, int count )
{
	int i;

	fprintf(fd,"%s[] = {\n", name);
	for (i = 0; i < count; i++)
		fprintf(fd, "\t{ .alloced = sizeof(%s%d)/BYTES_PER_MPI_LIMB, .nlimbs = sizeof(%s%d)/BYTES_PER_MPI_LIMB,"
			" .nbits = 8*sizeof(%s%d), .sign = 0, .flags = 0, .d = (mpi_limb_t*)%s%d },\n",
			mpiname, i, mpiname, i, mpiname, i, mpiname, i);

	fprintf(fd,"}, \n");
}

static void print_structs( FILE *fd, int count )
{
	int i; 
	fprintf(fd, "static struct _mpi const\n");

	print_struct(fd, "p", "P", count); 
	print_struct(fd, "q", "Q", count); 
	print_struct(fd, "g", "G", count); 
	print_struct(fd, "y", "Y", count); 

	fprintf(fd, "*const base[][3] = {");
	for (i = 0; i < count; i++)
		fprintf(fd, "{ &g[%d], &y[%d], NULL},", i, i);
	fprintf(fd,"}; \n");
}

int main(int argc, char **argv)
{
	DSA *dsa;
	FILE *fp;
	int i;

	if (argc < 2)
		return 1;

	for (i = 0; i < argc-1; i++) { 
		if ( !(fp = fopen( argv[i+1], "r")))
			return 1;

		if ( !(dsa = PEM_read_DSA_PUBKEY(fp, NULL, NULL, NULL))) {
			fprintf(stderr, "Could not read public key from file\n");
			return 1;
		}

		print_mpi( stdout, i, "P", dsa->p );
		print_mpi( stdout, i, "Q", dsa->q );
		print_mpi( stdout, i, "G", dsa->g );
		print_mpi( stdout, i, "Y", dsa->pub_key );

		fclose( fp );
	}
	print_structs( stdout, argc-1 );
	fprintf(stdout, "static int const dsa_key_count = %d;\n", argc-1);

	return 0;
}

