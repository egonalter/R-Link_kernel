# drivers/barcelona/usbmode/Makefile
#
# Makefile for the TomTom usbmode kernel module
# This driver handles switching from and to different 
# usb modes
#
# Copyright (C) 2007,2008 TomTom BV <http://www.tomtom.com/>
# Author: Kees Jongenburger <kees.jongenburger@tomtom.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.

obj-$(CONFIG_TOMTOM_USBMODE)			+= usbmode.o
obj-$(CONFIG_TOMTOM_USBMODE_S3C64XX)		+= s3c64xx_usbmode.o
obj-$(CONFIG_TOMTOM_USBMODE_SYNAP_LL_LIGHT)	+= synap_ll_usbmode_light.o
obj-$(CONFIG_TOMTOM_USBMODE_DUMMY)		+= dummy_usbmode.o
# EOF
