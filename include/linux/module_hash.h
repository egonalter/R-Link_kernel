/*
 * module_hash.h - check loaded modules against a hardcoded list of hashes
 *
 * 2009-10-20 Ard Biesheuvel <ard.biesheuvel@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifdef CONFIG_MODULE_HASHES

#define MAX_MODULE_SIZE		(4 * 1024 * 1024)

long check_module_hash(char *mod, unsigned int len);

#else
#define check_module_hash(a,b)	(0)
#endif

