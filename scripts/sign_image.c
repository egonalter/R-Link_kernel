/*
 * sign_image - Image signature generation and checking tool
 *
 * 2009-10-28 Ard Biesheuvel <ard.biesheuvel@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <openssl/pem.h>
#include <openssl/dsa.h>
#include <openssl/err.h>
#include <openssl/objects.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>
#include <openssl/bn.h>

/* for htonl() */
#include <arpa/inet.h>

static int check;
static int rebuild;
static int verbose;

static int calculate_signature( unsigned char *digest, unsigned char *sig, DSA *dsa )
{
	DSA_SIG *bn_sig;
	int r = 0;

	if (!check) {
		if ( !(bn_sig = DSA_do_sign( digest, 20, dsa ))) {
			if (verbose)
				fprintf( stderr, "DSA_sign(): %s\n", ERR_error_string( ERR_get_error(), NULL));
			fprintf( stderr, "Verification failed!\n");
			r = -1;
			goto bail;
		} else {
			*(unsigned short*)sig = htons( 8*BN_num_bytes( bn_sig->r ));
			sig += 2;
			sig += BN_bn2bin( bn_sig->r, sig );

			*(unsigned short*)sig = htons( 8*BN_num_bytes( bn_sig->s ));
			sig += 2;
			sig += BN_bn2bin( bn_sig->s, sig );
		}
	} else {
		bn_sig = DSA_SIG_new();

		unsigned short s = ntohs( *(unsigned short*) sig ) / 8;
		sig += 2;

		if ( !(bn_sig->r = BN_bin2bn( sig, s, NULL ))) {
			fprintf(stderr, "BN_bin2bn() failed!\n");
			r = -1;
			goto bail;
		}

		sig += s;

		s = ntohs( *(unsigned short*) sig ) / 8;
		sig += 2;

		if ( !(bn_sig->s = BN_bin2bn( sig, s, NULL ))) {
			fprintf(stderr, "BN_bin2bn() failed!\n");
			r = -1;
			goto bail;
		}

		if (1 != DSA_do_verify( digest, 20, bn_sig, dsa )) {
			fprintf( stderr, "Verification failed!\n");
			if (verbose)
				fprintf( stderr, "*** DSA_verify(): %s\n", ERR_error_string( ERR_get_error(), NULL));
			r = -1;
			goto bail;
		}
	}

bail:
	DSA_SIG_free(bn_sig);

	return r;
}

static void usage(char *self)
{
	fprintf(stderr, "usage: %s [-c|-r] [-h <key>] -k <key_file> <image_file>\n", self);
	fprintf(stderr, "\t-c:\tcheck the signature\n");
	fprintf(stderr, "\t-r:\trecalculate the signature\n");
	fprintf(stderr, "\t-h:\tsign HMAC hash with <key> (16 hex bytes) instead of plain SHA1 hash\n");
	fprintf(stderr, "\t-v:\tverbose output\n");
	exit(1);
}

static int unhexify(char *in)
{
	char *out;
	int i;

	for (i = 0, out = in; *in; in += 2, out++, i++)
		*out = (in[0] <= '9' ? in[0] - '0' : 10+(in[0] <= 'F' ? in[0] - 'A' : in[0] - 'a')) << 4
		     | (in[1] <= '9' ? in[1] - '0' : 10+(in[1] <= 'F' ? in[1] - 'A' : in[1] - 'a'));
	*out = 0;

	return i;
}

int main(int argc, char **argv)
{
	struct stat st;
	int opt, fd, hmac_keylen = 0;
	char *key_file = NULL, *loop_file = NULL, *hmac_key = NULL;
	FILE *key;
	DSA *dsa;
	unsigned char hash[20], sig[44];
	void *p;
	int i, sig_size = 44;

	while ( -1 != (opt = getopt(argc, argv, "ch:k:rv"))) {
		switch (opt) {
		case 'c':	check = 1;					break;
		case 'h':	hmac_keylen = unhexify(hmac_key = optarg);	break;
		case 'k':	key_file = optarg;				break;
		case 'r':	rebuild = 1;					break;
		case 'v':	verbose = 1;					break;
		default:	usage(argv[0]);
		}
	}

	if (!key_file || optind >= argc)
		usage(argv[0]);

	loop_file = argv[optind];

	ERR_load_crypto_strings();
	OpenSSL_add_all_algorithms();

	/* open the key file */
	if (!strcmp(key_file, "-")) {
		key = stdin;
	}
	else if ( !(key = fopen( key_file, "r" ))) {
		fprintf(stderr, "fopen() failed on key file %s: ", key_file );
		goto err;
	}

	if ( !(dsa = PEM_read_DSAPrivateKey( key, NULL, NULL, NULL ))
		&& (fseek(key, 0, SEEK_SET) || !(dsa = PEM_read_DSA_PUBKEY( key, NULL, NULL, NULL )))) {
		fprintf(stderr, "PEM_read_DSAPrivateKey() failed on key file %s\n", key_file );
		fprintf(stderr, "%s\n", ERR_error_string( ERR_get_error(), NULL));
		return 1;
	}

	fclose(key);


	if (stat( loop_file, &st)) {
		fprintf(stderr, "stat() failed on file %s: ", loop_file );
		goto err;
	}
	
	if ( 0 > (fd = open( loop_file, O_RDWR ))) {
		fprintf(stderr, "open() failed on file %s: ", loop_file );
		goto err;
	}

	p = mmap( NULL, st.st_size, PROT_READ|PROT_WRITE, MAP_SHARED|MAP_NONBLOCK, fd, 0 );

	if (p == MAP_FAILED )
		return 1;

	if (check || rebuild) {
		if ( -1 == lseek(fd, st.st_size - sig_size, SEEK_SET)) {
			fprintf(stderr, "lseek() failed on file %s: ", loop_file );
			goto err;
		}
		if (!hmac_key)
			SHA1(p, st.st_size - sig_size, hash);
		else {
			unsigned int dummy;
			int i;

			HMAC( EVP_sha1(), hmac_key, hmac_keylen, p, st.st_size - sig_size, hash, &dummy );
			for (i = 0; i < 20; i++)
				printf("%02x:", hash[i]);
			printf("\n");

		}
	} else {

		if ( -1 == lseek(fd, st.st_size, SEEK_SET)) {
			fprintf(stderr, "lseek() failed on file %s: ", loop_file );
			goto err;
		}
		if (!hmac_key)
			SHA1(p, st.st_size, hash);
		else {
			unsigned int dummy;
			HMAC( EVP_sha1(), hmac_key, hmac_keylen, p, st.st_size, hash, &dummy );
		}
	}

	if (verbose) {
		printf("HASH:");
		for (i = 0; i < 20; i++)
			printf(" %02x", hash[i]);
		printf("\n");
	}

	if (check) {
		if ( sig_size != read(fd, sig, sig_size)) {
			fprintf(stderr, "read() signature failed on file %s: ", loop_file );
			goto err;
		}
	}
		
	if (calculate_signature( hash, sig, dsa ) < 0)
		goto err;

	if (!check) {
		if ( -1 == write(fd, sig, sig_size)) {
			fprintf(stderr, "write() signature failed on file %s: ", loop_file );
			goto err;
		}
	}

	if (0 > munmap(p, st.st_size)) {
		fprintf(stderr, "munmap() failed on file %s: ", loop_file );
		goto err;
	}

	close( fd );

	return 0;

err:
	errx(EXIT_FAILURE, "%s failed: file [%s] key [%s]", check? "Checking" : "Signing", loop_file, key_file);
}

