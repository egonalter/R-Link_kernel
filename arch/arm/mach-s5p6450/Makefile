# arch/arm/mach-s5p6450/Makefile
#
# Copyright (c) 2009 Samsung Electronics Co., Ltd.
# 		http://www.samsung.com/
#
# Licensed under GPLv2

obj-y				:=
obj-m				:=
obj-n				:=
obj-				:=

# Core support for S5P6450 system

obj-$(CONFIG_CPU_S5P6450)		+= cpu.o init.o clock.o gpio.o dma.o

#CONFIG PM 
obj-$(CONFIG_PM)    			+= pm.o

obj-$(CONFIG_CPU_S5P6450)       	+= setup-sdhci.o setup-mshci.o setup-mshci-gpio.o
# machine support

obj-$(CONFIG_MACH_SMDK6450)		+= mach-smdk6450.o
obj-$(CONFIG_MACH_S5P6450_TOMTOM)	+= mach-valdez.o mach-taranto.o
obj-$(CONFIG_MACH_SMDK6450)     	+= button-smdk6450.o 
obj-$(CONFIG_MACH_SMDK6450)     	+= setup-fb.o setup-misc.o
obj-$(CONFIG_SPI)			+= spi.o
obj-$(CONFIG_VIDEO_FIMC)		+= setup-fimc0.o
obj-$(CONFIG_MACH_SMDK6450)     	+= setup-i2c0.o

obj-$(CONFIG_MACH_S5P6450_TOMTOM)	+= setup-i2c0.o setup-misc.o

obj-$(CONFIG_S5P_SETUP_I2C1)		+= setup-i2c1.o


obj-$(CONFIG_CPU_FREQ)          	+= s5p6450-cpufreq.o

# ltc3714 voltage regulator
obj-$(CONFIG_S5P64XX_LTC3714)		+=  ltc3714.o
