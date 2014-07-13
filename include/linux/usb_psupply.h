#ifndef __USB_PSUPPLY_H__
#define __USB_PSUPPLY_H__

#define DRIVER_NAME "usb-psupply"
#define PFX DRIVER_NAME ": "

#define USBPS_DBG(fmt, args...) \
	printk(KERN_DEBUG PFX fmt "\n", ##args)

#define USBPS_INFO(fmt, args...) \
        printk(KERN_INFO PFX fmt "\n", ##args)

#define USBPS_ERR(fmt, args...) \
        printk(KERN_ERR PFX fmt "\n", ##args)

struct usb_psupply_pdata
{
	int gpio_vbus;
	int gpio_fast_charge_en;
	int gpio_chrg_det;

	int (*is_accessory_charger_plugged)(void);
};

#endif
