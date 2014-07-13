/*
 * Copyright (C) 2009 TomTom BV <http://www.tomtom.com/>
 * Author: Kees Jongenburger <kees.jongenburger@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/platform_device.h>

#include <plat/tt_setup_handler.h>

#ifdef CONFIG_TOMTOM_FDT
#include <plat/fdt.h>
#endif

#include <linux/slab.h>

/*TODO convert to something cleaner like a klist */
struct tt_setup_handler {
	char *identifier;
	tt_setup_handler handler;
};

struct tt_setup_handler *tt_setup_handlers = NULL;
static int tt_setup_handlers_entries = 0;

int tt_setup_register_handler(tt_setup_handler handler, char *identifier)
{

	struct tt_setup_handler *tmp;

	pr_info("tt_setup_register_handler: Registering %s\n", identifier);
	tt_setup_handlers_entries += 1;
	tmp =
	    (struct tt_setup_handler *)krealloc(tt_setup_handlers,
						tt_setup_handlers_entries *
						sizeof(struct tt_setup_handler),
						GFP_ATOMIC);
	if (tmp == NULL) {
		tt_setup_handlers_entries -= 1;
		return -2;
	}

	tt_setup_handlers = tmp;
	tt_setup_handlers[tt_setup_handlers_entries - 1].identifier =
	    identifier;
	tt_setup_handlers[tt_setup_handlers_entries - 1].handler = handler;

	return 0;		/* success */
}

#ifdef CONFIG_TOMTOM_FDT
/**
 * tt_setup is called from postcore_initcall callback below
 **/
int tt_setup(void)
{
	fdt_node_iterator_t iter;
	int i;
	unsigned long node_offset;
	int fdt_entry_used;

	node_offset = fdt_find_node("/devices");
	if (!node_offset) {
		printk(KERN_WARNING
		       "TT_SETUP:FDT %s failed to find /devices node "
		       "in the FDT\n", __FUNCTION__);
		return -ENOENT;
	}

	/* Make the iter point to the /devices node */
	fdt_iterate_nodes(node_offset, &iter);
	/* Make the iter point to the first child */
	if (fdt_first_child_node(&iter) != 1) {
		printk(KERN_WARNING
		       "TT_SETUP:FDT %s failed to find /devices node "
		       "children in the FDT\n", __FUNCTION__);
		return 0;	/* This is not really an error */
	}

	/* iterate over the children */
	do {
		fdt_entry_used = 0;
		for (i = 0; i < tt_setup_handlers_entries; i++) {
			if (!strcmp(tt_setup_handlers[i].identifier,
			    iter.name)) {
				printk(KERN_INFO "TT_SETUP:FDT %s probing %s\n",
				       __FUNCTION__,
				       tt_setup_handlers[i].identifier);
				fdt_entry_used = 1;
				if (tt_setup_handlers[i].
				    handler(tt_setup_handlers[i].identifier)) {
					printk(KERN_WARNING
					       "TT_SETUP:FTD %s probed for "
					       "entry %s but that returned a "
					       "non 0 value \n",
					       __FUNCTION__, iter.name);
				}
			}
		}
		if (fdt_entry_used == 0) {
			printk(KERN_INFO
			       "TT_SETUP:FTS %s entry %s in the FDT did not "
			       "result in probing of a function \n",
			       __FUNCTION__, iter.name);
		}
	} while (fdt_next_sibling_node(&iter));

	kfree(tt_setup_handlers);
	tt_setup_handlers = NULL;

	return 0;
};
#else
int tt_setup(void)
{
	int i;

	printk(KERN_WARNING " FDT %s support is not in the kernel, "
	       "probing all the registered devices \n", __FUNCTION__);

	for (i = 0; i < tt_setup_handlers_entries; i++) {
		if (tt_setup_handlers[i].
		    handler(tt_setup_handlers[i].identifier)) {
			printk(KERN_WARNING "TT_SETUP: %s probed for "
		       "entry %s but that returned a non 0 value \n",
		       __FUNCTION__, tt_setup_handlers[i].identifier);
		}
	}
	kfree(tt_setup_handlers);

	return 0;
};
#endif /* else CONFIG_TOMTOM_FDT */

/* ---------------------- platform-driver --------------------- */

static int ttsetup_probe(struct platform_device *pdev)
{
	return tt_setup();
}

static int ttsetup_remove(struct platform_device *pdev)
{
	return 0;
}

struct platform_driver tomtom_ttsetup_driver = {
	.probe		= ttsetup_probe,
	.remove		= ttsetup_remove,
	.driver		= {
		.owner	= THIS_MODULE,
		.name	= "ttsetup",
	},
};

static int __init ttsetup_postcore_init(void) {
	return platform_driver_register(&tomtom_ttsetup_driver);
}
late_initcall(ttsetup_postcore_init);

