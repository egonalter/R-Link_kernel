/*
 * Copyright (C) 2009 TomTom BV <http://www.tomtom.com/>
 *
 * This code is heavily sourced from the powerpc libtlv.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/module.h>
#include <linux/byteorder/generic.h>
#include <linux/ioport.h>
#include <linux/platform_device.h>

#include <asm/errno.h>
#include <plat/fdt.h>
#include <plat/factorydata.h>

#include "libfdt.h"

#define fdt32_to_cpu(x)		be32_to_cpu(x)
#define fdt64_to_cpu(x)		be64_to_cpu(x)

#define PNODE_TO_OFFSET(pnode)	(pnode & 0x00FFFFFF)
#define PNODE_TO_DEPTH(pnode)	(((pnode & 0xFF000000) >> 24) - 1)
#define PNODE(offset, depth)	(((depth + 1) & 0xFF) << 24 | offset)

/* ---------------------- library -------------------------- */

static inline char *find_flat_dt_string(u32 offset)
{
	struct boot_param_header *initial_boot_params = fdt_buffer_info.address;
	return ((char *)initial_boot_params) +
		fdt32_to_cpu(initial_boot_params->off_dt_strings) + offset;
}

void fdt_iterate_nodes(unsigned long p, fdt_node_iterator_t *i)
{
	memset(i, '\0', sizeof(*i));
	if (p == 0) {
		unsigned long offset;
		struct boot_param_header *initial_boot_params = fdt_buffer_info.address;
		offset = fdt32_to_cpu(initial_boot_params->off_dt_struct);
		p = PNODE(offset, -1);
	}
	i->p = p;
	i->depth = PNODE_TO_DEPTH(p);
}
EXPORT_SYMBOL(fdt_iterate_nodes);

static int fdt_next_node(fdt_node_iterator_t *i)
{
	struct boot_param_header *initial_boot_params = fdt_buffer_info.address;
	unsigned long base = (unsigned long)initial_boot_params;
	unsigned long offset = PNODE_TO_OFFSET(i->p);
	int depth = PNODE_TO_DEPTH(i->p);
	int rc = 0;
	u32 fdt_version = fdt32_to_cpu(initial_boot_params->version);
	u32 fdt_magic = fdt32_to_cpu(initial_boot_params->magic);

	if (fdt_magic != OF_DT_HEADER)
		return 0;	/* Sanity check */

	do {
		u32 tag = fdt32_to_cpu(*((u32 *)(base + offset)));
		char *pathp;
		
		offset += 4;
		if (tag == OF_DT_END_NODE) {
			depth --;
			continue;
		}
		if (tag == OF_DT_NOP)
			continue;
		if (tag == OF_DT_END) {
			rc = 0;
			break;
		}
		if (tag == OF_DT_PROP) {
			u32 sz = fdt32_to_cpu(*((u32 *)(base + offset)));
			offset += 8;
			if (fdt_version < 0x10)
				offset = ALIGN(offset, sz >= 8 ? 8 : 4);
			offset += sz;
			offset = _ALIGN(offset, 4);
			continue;
		}
		if (tag != OF_DT_BEGIN_NODE) {
			printk(KERN_WARNING "Invalid tag %x scanning flattened"
			       " device tree !\n", tag);
			rc = 0;
			break;
		}
		depth++;
		pathp = (char *)(base + offset);
		offset = _ALIGN(offset + strlen(pathp) + 1, 4);
		if ((*pathp) == '/') {
			char *lp, *np;
			for (lp = NULL, np = pathp; *np; np++)
				if ((*np) == '/')
					lp = np+1;
			if (lp != NULL)
				pathp = lp;
		}
		i->name = pathp;
		rc = 1;
		break;
	} while(1);

	i->p = PNODE(offset, depth);
	i->depth = depth;
	return rc;
}

int fdt_next_sibling_node(fdt_node_iterator_t *i)
{
	int depth0 = i->depth;
	while (fdt_next_node(i)) {
		if (i->depth == depth0)
			return 1;
		else if (i->depth < depth0)
			return 0;
	}
	return 0;
}
EXPORT_SYMBOL(fdt_next_sibling_node);

int fdt_first_child_node(fdt_node_iterator_t *i)
{
	int depth0 = i->depth;
	if (fdt_next_node(i)) {
		if (i->depth == depth0 + 1)
			return 1;
	}
	return 0;
}
EXPORT_SYMBOL(fdt_first_child_node);

void fdt_iterate_properties(unsigned long p, fdt_property_iterator_t *i)
{
	memset(i, '\0', sizeof(*i));
	i->p = p;
}
EXPORT_SYMBOL(fdt_iterate_properties);

int fdt_next_property(fdt_property_iterator_t *i)
{
	struct boot_param_header *initial_boot_params = fdt_buffer_info.address;
	u32 fdt_version = fdt32_to_cpu(initial_boot_params->version);
	u32 fdt_magic = fdt32_to_cpu(initial_boot_params->magic);
	unsigned long base = (unsigned long)initial_boot_params;
	unsigned long offset = PNODE_TO_OFFSET(i->p);
	int depth = PNODE_TO_DEPTH(i->p);

	if (fdt_magic != OF_DT_HEADER)
		return 0;	/* Sanity check */

	if (i->p == 0)
		return 0;

	do {
		u32 tag = fdt32_to_cpu(*((u32 *)(base + offset)));
		u32 sz, noff;
		const char *nstr;

		offset += 4;
		if (tag == OF_DT_NOP)
			continue;
		if (tag != OF_DT_PROP)
			return 0;

		sz = fdt32_to_cpu(*((u32 *)(base + offset)));
		noff = fdt32_to_cpu(*((u32 *)(base + offset + 4)));
		offset += 8;
		if (fdt_version < 0x10)
			offset = _ALIGN(offset, sz >= 8 ? 8 : 4);

		nstr = find_flat_dt_string(noff);
		if (nstr == NULL) {
			printk(KERN_WARNING "Can't find property index"
			       " name !\n");
			return 0;
		}
		i->size = sz;
		i->pchar_value = (void *)(base + offset);
		i->ulong_value = fdt32_to_cpu(
				*((unsigned long*)(base + offset)));
		i->name = nstr;
		offset += sz;
		offset = _ALIGN(offset, 4);
		i->p = PNODE(offset, depth);
		return 1;
	} while(1);

	return 0;
}
EXPORT_SYMBOL(fdt_next_property);

static void *fdt_get_node_property(unsigned long p, const char *pname)
{
	fdt_property_iterator_t i;
	fdt_iterate_properties(p, &i);
	while (fdt_next_property(&i)) {
		if (strcmp(i.name, pname) == 0)
			return (void*)i.pchar_value;
	}
	return NULL;
}

const char *fdt_get_node_string(unsigned long p, const char *pname,
		const char *defvalue)
{
	void *ret = fdt_get_node_property(p, pname);
	if (ret == NULL)
		return defvalue;
	return (const char*)ret;
}
EXPORT_SYMBOL(fdt_get_node_string);

unsigned long fdt_get_node_ulong(unsigned long p, const char *pname,
		unsigned long defvalue)
{
	void *ret = fdt_get_node_property(p, pname);
	if (ret == NULL)
		return defvalue;
	return fdt32_to_cpu(*((unsigned long*)ret));
}
EXPORT_SYMBOL(fdt_get_node_ulong);

/*
 * Compare two strings, stops at a '/' OR '\0' in pname
 * Return 1 if strings are equals
 */
static int node_match(const char *pname, const char *name)
{
	while (*pname && *pname != '/' && *name)
	{
		if (*pname != *name)
			return 0;
		name++;
		pname++;
	}
	return ((*pname == '\0' || *pname == '/') && *name == '\0');
}

unsigned long fdt_find_node(const char *vpath)
{
	int depth = 0;
	fdt_node_iterator_t i;
	const char *p = vpath;

	fdt_iterate_nodes(0, &i);
	while (fdt_next_node(&i)) {
		if (i.depth < depth)
			return 0;
		if ((i.depth == depth) && node_match(p, i.name)) {
			while (*p != '/' && *p)
				p++;
			if (*p == '\0')
				return i.p;
			p++;
			depth++;
	}	}
	return 0;
}
EXPORT_SYMBOL(fdt_find_node);

static void *fdt_get_property(const char *vpath, const char *pname)
{
	unsigned long p = fdt_find_node(vpath);
	if (p)
		return fdt_get_node_property(p, pname);
	return NULL;
}

const char *fdt_get_string(const char *vpath, const char *pname,
		const char *defvalue)
{
	void *ret = fdt_get_property(vpath, pname);
	if (ret == NULL)
		return defvalue;
	return (const char *)ret;
}
EXPORT_SYMBOL(fdt_get_string);

unsigned long fdt_get_ulong(const char *vpath, const char *pname,
		unsigned long defvalue)
{
	void *ret = fdt_get_property(vpath, pname);
	if (ret == NULL)
		return defvalue;
	return fdt32_to_cpu(*((unsigned long*)ret));
}
EXPORT_SYMBOL(fdt_get_ulong);

int fdt_check_magic(void)
{
	struct boot_param_header *initial_boot_params = fdt_buffer_info.address;
	u32 fdt_magic = fdt32_to_cpu(initial_boot_params->magic);
	return (fdt_magic == OF_DT_HEADER);
}
EXPORT_SYMBOL(fdt_check_magic);

size_t fdt_totalsize(void)
{
	struct boot_param_header *initial_boot_params = fdt_buffer_info.address;
	return fdt32_to_cpu(initial_boot_params->totalsize);
}
EXPORT_SYMBOL(fdt_totalsize);
