obj-y	:= lcm_init.o backlight.o eeprom.o
obj-y	+= panel-samsung_lms700kf19.o panel-lg_ld050wv1sp01.o
obj-y	+= panel-samsung_lms500hf04.o panel-cmo_g070y2l01.o
obj-y	+= panel-samsung_lms501kf03.o
obj-$(CONFIG_TOMTOM_BATTERY_BQ27x00)		+= bq27x00.o
obj-$(CONFIG_TOMTOM_GPS)			+= gps.o
obj-$(CONFIG_TOMTOM_GPRS)			+= gprs_faro.o
obj-$(CONFIG_TOMTOM_BLUETOOTH_MODE)		+= bluetooth.o
obj-$(CONFIG_TOMTOM_DEBUG)			+= debug.o
ifdef CONFIG_TOMTOM_FLIPFLOP
obj-y						+= flipflop.o
endif
obj-$(CONFIG_TOMTOM_POWER_BUTTON)		+= power_button.o
obj-$(CONFIG_TOUCHSCREEN_TSC2007)		+= ts-tsc2007.o
ifdef CONFIG_TOUCHSCREEN_ATMEL_MXT
obj-y						+= ts-atmel_mxt_ts.o
endif
obj-$(CONFIG_TOUCHSCREEN_HX8519)		+= ts-hx8519.o
obj-$(CONFIG_TOMTOM_TOUCHSCREEN_CYTTSP)		+= ts-cyttsp.o
obj-$(CONFIG_TOMTOM_CAN_MICRO)			+= can_micro.o
ifdef CONFIG_TOMTOM_TILT_TS0001_L
obj-y						+= tilt-ts0001-l.o
endif
ifdef CONFIG_TOMTOM_USB_PSUPPLY
obj-y						+= usb_psupply.o
endif
obj-$(CONFIG_TOMTOM_PMIC_STRASBOURG)		+= pmic.o
obj-$(CONFIG_TOMTOM_IDPINMON)			+= idpinmon.o
ifdef CONFIG_TOMTOM_PPS_DRIVER
obj-y						+= pps.o
endif
obj-$(CONFIG_TOMTOM_BCM4329)			+= bcm4329.o
ifdef CONFIG_TOMTOM_LIS3DH
obj-y						+= lis3dh.o
endif
ifdef CONFIG_TOMTOM_L3GD20
obj-y						+= l3gd20.o
endif
ifdef CONFIG_TOMTOM_EWTS98
obj-y						+= ewts98.o
endif
ifdef CONFIG_TOMTOM_LM26
obj-y						+= lm26.o
endif
ifdef CONFIG_VIDEO_OMAP3_OUT
obj-y						+= vout.o
endif
obj-$(CONFIG_MACH_STRASBOURG_A2)		+= apple_auth.o
obj-$(CONFIG_MACH_STRASBOURG_A2)		+= micpwr.o
obj-$(CONFIG_MACH_STRASBOURG_XENIA)		+= apple_auth.o
obj-$(CONFIG_MACH_STRASBOURG_XENIA)		+= micpwr.o
