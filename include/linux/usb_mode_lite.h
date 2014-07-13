#ifndef __USB_MODE_LITE_H__
#define __USB_MODE_LITE_H__

#define OTG_HOST_MODE	0
#define OTG_DEVICE_MODE	1

extern void usb_mode_lite_handle_usb_reset(void);

struct usb_mode_lite_platform_data
{
	int gpio_vbus;
	int (*set_charger_type)(int);
	int (*is_aca_plugged)(void);
	int (*switch_otg_mode)(int);
};

#endif
