obj-y		:= eeprom.o mendoza-lcd.o sd_pwr_en.o
obj-y		+= panel/
obj-m		:=
obj-n		:=
obj-$(CONFIG_TOMTOM_GPS)			+= gps.o
obj-$(CONFIG_TOMTOM_GPRS)			+= gprs_faro.o
obj-$(CONFIG_TOMTOM_BLUETOOTH_MODE)		+= bluetooth.o
obj-$(CONFIG_TOMTOM_POWER_BUTTON)		+= power_button.o
obj-$(CONFIG_TOMTOM_FLIPFLOP)			+= flipflop.o
obj-$(CONFIG_TOMTOM_TOUCHSCREEN_CYTTSP)		+= ts-cyttsp.o
ifdef CONFIG_TOMTOM_TILT_TS0001_L
obj-y						+= tilt-ts0001-l.o
endif
obj-$(CONFIG_SND_TLV320ADC3101)	        		+= tlv320adc3101_pdata.o
obj-$(CONFIG_MFD_S5M8751_I2C)			+= s5m8751_audio.o
obj-$(CONFIG_TOMTOM_BCM4330_WIFI)			+= bcm4330_wifi.o
obj-$(CONFIG_TOMTOM_BCM2070)			+= bcm2070.o
obj-$(CONFIG_TOMTOM_USB_MODE_LITE)		+= usb_mode_lite.o