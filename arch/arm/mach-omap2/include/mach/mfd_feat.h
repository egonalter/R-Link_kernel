/*
 * Copyright (C) 2012 TomTom International BV
 * Written by Domenico Andreoli <domenico.andreoli@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef TOMTOM_MFD_FEAT_H
#define TOMTOM_MFD_FEAT_H

#include <linux/module.h>

/*
 * Helper struct to hold a TomTom MFD board specific feature in form
 * of a pointer.  Specific features are selected in base of the mach and
 * rev fields.
 */
struct mfd_feat {
	unsigned int mach;
	unsigned int rev;
	void *priv;
};

/* Helper macros to init arrays of MFD features */
#define MFD_FEAT_INIT(_mach, _rev, _feat)   { \
	.mach = _mach, \
	.rev = _rev, \
	.priv = _feat, \
}

/*
 * The default entry is mandatory, it grants that mfd_lookup() is not
 * an infinite loop. Entries after the default are ignored so normally
 * it is last.  It also provides a means to return a possibly meaningful
 * value in case no MFD cadidate is found.
 */
#define MFD_DEFAULT(_feat)        MFD_FEAT_INIT(0, 0, _feat)

/* Lookup functions to find the first matching MFD feature */
const struct mfd_feat * __init mfd_lookup(const struct mfd_feat *feat);

static inline void * __init mfd_feature(const struct mfd_feat *feat)
{
	/* mfd_lookup always returns a non-NULL pointer */
	return mfd_lookup(feat)->priv;
}

/* Ideally this should go as mfd_feature and mfd_lookup are fully deployed */
unsigned int get_mfd_rev(void);

struct platform_device;
void mfd_set_pdata(struct platform_device *pdev, const struct mfd_feat *feat);

#endif /* TOMTOM_MFD_FEAT_H */
