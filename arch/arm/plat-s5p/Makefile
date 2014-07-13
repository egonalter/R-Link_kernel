# arch/arm/plat-s5p/Makefile
#
# Copyright (c) 2009 Samsung Electronics Co., Ltd.
# 		http://www.samsung.com/
#
# Licensed under GPLv2

obj-y				:=
obj-m				:=
obj-n				:= dummy.o
obj-				:=

# Core files

obj-y				+= dev-uart.o
obj-y				+= cpu.o
obj-y				+= clock.o
obj-y				+= irq.o
obj-y                           += devs.o
obj-y				+= dev-fb.o
obj-y				+= bootmem.o
#obj-y				+= dev-fimc0.o

ifdef CONFIG_S5P_HIGH_RES_TIMERS
obj-y                           += hr-time.o
endif
obj-$(CONFIG_PM)		+= pm.o
obj-$(CONFIG_PM)		+= irq-pm.o
obj-y				+= irq-eint.o
obj-$(CONFIG_PM)		+= sleep.o
# Device files:
obj-$(CONFIG_S5P_DMA_PL330)	+= dma-pl330.o

obj-$(CONFIG_SND_S3C24XX_SOC)	+= dev-audio.o

obj-y			        += dev-ts.o
