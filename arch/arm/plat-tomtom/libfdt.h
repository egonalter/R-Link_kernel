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

#ifndef LIBFDT_H
#define LIBFDT_H

/* took from include/asm-powerpc */
#define _ALIGN_UP(addr,size)	(((addr)+((size)-1))&(~((size)-1)))
#define _ALIGN_DOWN(addr,size)	((addr)&(~((size)-1)))

/* align addr on a size boundary - adjust address up if needed */
#define _ALIGN(addr,size)     _ALIGN_UP(addr,size)

/* Definitions used by the flattened device tree */
#define OF_DT_HEADER            0xd00dfeed      /* marker */
#define OF_DT_BEGIN_NODE        0x1     /* Start of node, full name */
#define OF_DT_END_NODE          0x2     /* End node */
#define OF_DT_PROP              0x3     /* Property: name off, size, content */
#define OF_DT_NOP               0x4     /* nop */
#define OF_DT_END               0x9

#define OF_DT_VERSION           0x10

/* Note: all the following fields are big-endian according to the
   flattened-device-tree format. */
struct boot_param_header {
	u32 magic;              /* magic word OF_DT_HEADER */
	u32 totalsize;          /* total size of DT block */
	u32 off_dt_struct;      /* offset to structure */
	u32 off_dt_strings;     /* offset to strings */
	u32 off_mem_rsvmap;     /* offset to memory reserve map */
	u32 version;            /* format version */
	u32 last_comp_version;  /* last compatible version */
	/* version 2 fields below */
	u32 boot_cpuid_phys;    /* Physical CPU id we're booting on */
	/* version 3 fields below */
	u32 dt_strings_size;    /* size of the DT strings block */
};

#endif

