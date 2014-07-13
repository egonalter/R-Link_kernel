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

#ifndef FDT_H
#define FDT_H

/*
 * fdt node iterator structure. Usage example:
 *
 *     fdt_node_iterator_t i;
 *     fdt_iterate_nodes(0, &i);
 *     while (fdt_next_node(&i)) {
 *         do_something(i.p, i.name);
 *     }
 *
 */
typedef struct fdt_node_iterator {
	unsigned long p;		/* opaque offset */
	int depth;			/* depth, 0 is root */
	const char *name;		/* name of property or node */
} fdt_node_iterator_t;

/*
 * fdt property iterator structure. Usage example:
 *
 *     unsigned long nodep = fdt_find_node("/some/node");
 *     fdt_property_iterator_t j;
 *     fdt_iterate_properties(nodep, &j);
 *     while (fdt_next_property(&j)) {
 *         do_something(j.name, j.pchar_value);
 *     }
 */
typedef struct fdt_property_iterator {
	unsigned long p;		/* opaque offset */
	const char *name;		/* name of property */
	const char *pchar_value;	/* pointer to string value */
	unsigned long ulong_value;	/* pointer to integer value */
	int size;			/* size of returned value */
} fdt_property_iterator_t;

/* 
 * Start iterating all the nodes, starting at offset p.
 * Parameters:  p - starting opaque offset, 0 for root
 *              i - iterator to initialize and use.
 */
void fdt_iterate_nodes(unsigned long p, fdt_node_iterator_t *i);

/*
 * Go and fetch to next sibling node. Return 1 if it was available.
 * Return: 0 - no new node to read
 *         1 - a new node can be read in the iterator:
 *             i.name is the name of the current node only (w/o parents)
 *             i.depth is the node depth (0 means root)
 *             i.p is the opaque offset, can be used as an input
 *                 parameter to fdt_iterate_properties() / fdt_get_node_xxx().
 */
int fdt_next_sibling_node(fdt_node_iterator_t *i);

/*
 * Go and fetch the first child of this node. Return 1 if it was available.
 */
int fdt_first_child_node(fdt_node_iterator_t *i);

/*
 * Start iterating all properties of a node.
 * Parameters: p - opaque offset for node, needed.
 *                 For convenience users can safely use 0,
 *                 no single property will be returned.
 *                 (0 is returned by fdt_find_node when no node is found)
 *             i - iterator to initialize and use.
 */
void fdt_iterate_properties(unsigned long p, fdt_property_iterator_t *i);

/*
 * Go and fetch to next property. Return if it was available.
 * Return: 0 - no new property to read
 *         1 - a new property can be read in the iterator:
 *             i.name is the name of the property
 *             i.value.[pchar|ulong] is a pointer to the content of property
 *             (pchar: zero-terminated string or buffer,
 *              ulong: unsigned long)
 *             i.size is the size of the returned property value.
 */
int fdt_next_property(fdt_property_iterator_t *i);

/*
 * Get a string property from a given node offset.
 * Parameters: p - node offset as returned by fdt_find_node()
 *                 or the value of fdt_node_iterator_t.p.
 *             pname - property name, such as "family-name"
 *             defvalue - default value to return if property not found.
 */
const char *fdt_get_node_string(unsigned long p, const char *pname,
		const char *defvalue);

/*
 * Get an unsigned long property from a given node offset.
 * See fdt_get_node_string() for usage.
 */
unsigned long fdt_get_node_ulong(unsigned long p, const char *pname,
		unsigned long defvalue);

/*
 * Get a string property from a given node name.
 * Parameters: vpath - node absolute vpath such as "/path/to/node"
 *                     (SHOULD start with a '/')
 *             pname - property name, such as "family-name"
 *             defvalue - default value to return if property not found.
 */
const char *fdt_get_string(const char *vpath, const char *pname,
		const char *defvalue);

/*
 * Get a unsigned long property.
 * See fdt_get_string() for usage.
 */
unsigned long fdt_get_ulong(const char *vpath, const char *pname,
		unsigned long defvalue);

/*
 * Find a node opaque offset.
 * Parameters: vpath - node name, like "/path/to/node"
 * Return: >0 - node found, opaque offset
           =0 - node not found
 */
unsigned long fdt_find_node(const char *vpath);

/*
 * Check fdt binary blob header magic.
 * Return: 1 if the header is correct,
 *         0 if the header is not correct.
 */
int fdt_check_header(void);

/*
 * Initialize factory data memory
 */
void fdt_reserve_bootmem(void);

/*
 * Get the total size of the FDT
 */
size_t fdt_totalsize(void);

extern struct platform_device tomtom_device_libfdt;
extern struct platform_device tomtom_device_ttsetup;

#endif

