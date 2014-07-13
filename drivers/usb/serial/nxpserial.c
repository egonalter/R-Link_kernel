/*
 * NXP serial USB driver
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License version
 *	2 as published by the Free Software Foundation.
 *
 */

#include <linux/tty.h>
#include <linux/tty_flip.h>
#include <linux/usb.h>
#include <linux/usb/serial.h>

#define DRIVER_AUTHOR "jwvb"
#define DRIVER_DESC "NXP USB serial driver"

static int debug;

static struct usb_device_id id_table[] = {
	{USB_DEVICE(0x1fc9, 0x0109)},	/* CRD2010 */
  {USB_DEVICE(0x03eb, 0x6119)},	/* HERO test board */
	{ }				/* Terminating entry */
};
MODULE_DEVICE_TABLE(usb, id_table);

static struct usb_driver nxpdriver = {
	.name			= "nxpserial",
	.probe			= usb_serial_probe,
	.disconnect		= usb_serial_disconnect,
	.id_table		= id_table,
	.suspend		= usb_serial_suspend,
	.resume			= usb_serial_resume,
	.supports_autosuspend	= true,
};


static int nxpprobe(struct usb_serial *serial, const struct usb_device_id *id)
{
	unsigned short vendorId = le16_to_cpu(id->idVendor);
	unsigned short productId = le16_to_cpu(id->idProduct);
	struct usb_host_interface *iface_desc = serial->interface->cur_altsetting;
	int ifnum = iface_desc->desc.bInterfaceNumber;
	int retval = -ENODEV;
	int num_bulk_out = 0;
	int num_bulk_in = 0;
	int i;

	for (i = 0; i < iface_desc->desc.bNumEndpoints; ++i) {
		struct usb_endpoint_descriptor *endpoint = &iface_desc->endpoint[i].desc;
		
		if (usb_endpoint_is_bulk_out(endpoint))
			++num_bulk_out;
		else if ((usb_endpoint_is_bulk_in(endpoint)))
			++num_bulk_in;
	}

	
	if (num_bulk_out == 0 || num_bulk_in == 0) {
	  dev_err(&serial->dev->dev, "skipping interface %d on %04X:%04X because it has no bulk out or bulk in\n", ifnum, vendorId, productId);
	  return -ENODEV;
	}

	retval = usb_set_interface(serial->dev, ifnum, 0);
	if (retval < 0) {
		dev_err(&serial->dev->dev, "Could not set interface %d on %04X:%04X, error %d\n", ifnum, vendorId, productId, retval);
		return -ENODEV;
	}
	return retval;
}


static void init_nxp_termios(struct tty_struct *tty) {
	int i = 0;
	struct ktermios *termios = tty->termios;
	*termios = tty_std_termios;

	termios->c_cflag = B115200;
	termios->c_cflag &= ~PARENB;  
	termios->c_cflag &= ~CSTOPB;
	termios->c_cflag &= ~CSIZE;
	termios->c_cflag |= CS8;

	// no flow control
	termios->c_cflag &= ~CRTSCTS;
	termios->c_cflag |= CREAD | CLOCAL;  // turn on READ & ignore ctrl lines
	termios->c_iflag &= ~(IXON | IXOFF | IXANY); // turn off s/w flow ctrl
	termios->c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG); // make raw
	termios->c_oflag &= ~OPOST; // make raw

	for (i = 0; i < NCCS; i++) {
		termios->c_cc[i] = 0;
	}
  
	termios->c_ispeed = B115200;
	termios->c_ospeed = B115200;
}


static struct usb_serial_driver nxpdevice = {
	.driver = {
		.owner     = THIS_MODULE,
		.name      = "nxpserial",
	},
	.id_table            = id_table,
	.num_ports           = 1,
  .init_termios        = init_nxp_termios,
	.probe               = nxpprobe,
};

static int __init nxpinit(void)
{
	int retval;

	retval = usb_serial_register(&nxpdevice);
	if (retval)
		return retval;

	retval = usb_register(&nxpdriver);
	if (retval) {
		usb_serial_deregister(&nxpdevice);
		return retval;
	}

	return 0;
}

static void __exit nxpexit(void)
{
	usb_deregister(&nxpdriver);
	usb_serial_deregister(&nxpdevice);
}

module_init(nxpinit);
module_exit(nxpexit);

MODULE_AUTHOR(DRIVER_AUTHOR);
MODULE_DESCRIPTION(DRIVER_DESC);
MODULE_LICENSE("GPL v2");

module_param(debug, bool, S_IRUGO | S_IWUSR);
MODULE_PARM_DESC(debug, "Debug enabled or not");
