/*
 * drivers/usb/core/otg_whitelist.h
 *
 * Copyright (C) 2004 Texas Instruments
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

/*
 * This OTG Whitelist is the OTG "Targeted Peripheral List".  It should
 * mostly use of USB_DEVICE() or USB_DEVICE_VER() entries..
 *
 * YOU _SHOULD_ CHANGE THIS LIST TO MATCH YOUR PRODUCT AND ITS TESTING!
 */

#ifdef CONFIG_USB_OTG_WHITELIST
/* THIS TABLE NEEDS TO BE DEFINED IN THE BOARD FILE */
extern struct usb_device_id usbhost_whitelist_table[];
#else
static struct usb_device_id usbhost_whitelist_table [] = {

/* hubs are optional in OTG, but very handy ... */
{ USB_DEVICE_INFO(USB_CLASS_HUB, 0, 0), },
{ USB_DEVICE_INFO(USB_CLASS_HUB, 0, 1), },

#ifdef	CONFIG_USB_PRINTER		/* ignoring nonstatic linkage! */
/* FIXME actually, printers are NOT supposed to use device classes;
 * they're supposed to use interface classes...
 */
{ USB_DEVICE_INFO(7, 1, 1) },
{ USB_DEVICE_INFO(7, 1, 2) },
{ USB_DEVICE_INFO(7, 1, 3) },
#endif

#ifdef	CONFIG_USB_NET_CDCETHER
/* Linux-USB CDC Ethernet gadget */
{ USB_DEVICE(0x0525, 0xa4a1), },
/* Linux-USB CDC Ethernet + RNDIS gadget */
{ USB_DEVICE(0x0525, 0xa4a2), },
#endif

#if	defined(CONFIG_USB_TEST) || defined(CONFIG_USB_TEST_MODULE)
/* gadget zero, for testing */
{ USB_DEVICE(0x0525, 0xa4a0), },
#endif

{ }	/* Terminating entry */
};
#endif

static bool match_intf(struct usb_device *dev, struct usb_device_id *id)
{
	struct usb_interface_cache *intf_cache;
	struct usb_host_interface *intf;
	struct usb_host_config *config;
	int i,j;
	bool found = 0;
	
	for (i = 0; i < dev->descriptor.bNumConfigurations && !found; i++) {
		config = &dev->config[i];
		for (j = 0; j < config->desc.bNumInterfaces && !found; j++) {
			intf_cache = config->intf_cache[j];
			if (intf_cache != NULL) {		
				intf = &intf_cache->altsetting[0];				
				if ((id->match_flags & USB_DEVICE_ID_MATCH_INT_CLASS) &&
				    (id->bInterfaceClass != intf->desc.bInterfaceClass))
					continue;

				if ((id->match_flags & USB_DEVICE_ID_MATCH_INT_SUBCLASS) &&
				    (id->bInterfaceSubClass != intf->desc.bInterfaceSubClass))
					continue;

				if ((id->match_flags & USB_DEVICE_ID_MATCH_INT_PROTOCOL) &&
				    (id->bInterfaceProtocol != intf->desc.bInterfaceProtocol))
					continue;
					
				found = 1;
			}
		}
	}
	return found;
}

static int is_targeted(struct usb_device *dev)
{
	struct usb_device_id *id = usbhost_whitelist_table;

	/* HNP test device is _never_ targeted (see OTG spec 6.6.6) */
	if (dev->bus->otg_port &&
	    (le16_to_cpu(dev->descriptor.idVendor) == 0x1a0a &&
	     le16_to_cpu(dev->descriptor.idProduct) == 0xbadd))
		return 0;

	/* NOTE: can't use usb_match_id() since interface caches
	 * aren't set up yet. this is cut/paste from that code.
	 */
	for (id = usbhost_whitelist_table; id->match_flags; id++) {
		if ((id->match_flags & USB_DEVICE_ID_MATCH_VENDOR) &&
		    id->idVendor != le16_to_cpu(dev->descriptor.idVendor))
			continue;

		if ((id->match_flags & USB_DEVICE_ID_MATCH_PRODUCT) &&
		    id->idProduct != le16_to_cpu(dev->descriptor.idProduct))
			continue;

		/* No need to test id->bcdDevice_lo != 0, since 0 is never
		   greater than any unsigned number. */
		if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_LO) &&
		    (id->bcdDevice_lo > le16_to_cpu(dev->descriptor.bcdDevice)))
			continue;

		if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_HI) &&
		    (id->bcdDevice_hi < le16_to_cpu(dev->descriptor.bcdDevice)))
			continue;

		if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_CLASS) &&
		    (id->bDeviceClass != dev->descriptor.bDeviceClass))
			continue;

		if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_SUBCLASS) &&
		    (id->bDeviceSubClass != dev->descriptor.bDeviceSubClass))
			continue;

		if ((id->match_flags & USB_DEVICE_ID_MATCH_DEV_PROTOCOL) &&
		    (id->bDeviceProtocol != dev->descriptor.bDeviceProtocol))
			continue;

		if ((id->match_flags & USB_DEVICE_ID_MATCH_INT_INFO) &&
		    !(match_intf(dev, id)))
		    	continue;

		return 1;
	}

	/* add other match criteria here ... */

#ifdef	CONFIG_USB_OTG_WHITELIST
	/* OTG MESSAGE: report errors here, customize to match your product */
	dev_err(&dev->dev, "device v%04x p%04x is not supported\n",
		le16_to_cpu(dev->descriptor.idVendor),
		le16_to_cpu(dev->descriptor.idProduct));
	return 0;
#else
	return 1;
#endif
}

