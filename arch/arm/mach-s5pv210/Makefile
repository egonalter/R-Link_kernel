# arch/arm/mach-s5pv210/Makefile
#
# Copyright (c) 2010 Samsung Electronics Co., Ltd.
# 		http://www.samsung.com/
#
# Licensed under GPLv2

obj-y				:=
obj-m				:=
obj-n				:=
obj-				:=

# Core support for S5PV210 system

obj-$(CONFIG_CPU_S5PV210)	+= cpu.o init.o clock.o gpio.o dma.o pd.o irq-eint-group.o irq-eint.o

# machine support

obj-$(CONFIG_MACH_SMDKV210)	+= mach-smdkv210.o
obj-$(CONFIG_MACH_SMDKC110)	+= mach-smdkc110.o

obj-$(CONFIG_S5PV210_PM)	+= pm.o
obj-$(CONFIG_S5PV210_SETUP_SDHCI)       += setup-sdhci.o

# Device setup - ADCTS
obj-$(CONFIG_S5PV210_ADCTS)		+= adcts.o

obj-$(CONFIG_S5PV210_SETUP_FB_S3C)	+= setup-fb.o
obj-$(CONFIG_S5PV210_SETUP_FIMC0)	+= setup-fimc0.o
obj-$(CONFIG_S5PV210_SETUP_FIMC1)	+= setup-fimc1.o
obj-$(CONFIG_S5PV210_SETUP_FIMC2)	+= setup-fimc2.o
obj-$(CONFIG_S5PV210_SETUP_CSIS)	+= setup-csis.o

obj-$(CONFIG_CPU_FREQ)         += s5pv210-cpufreq.o
obj-$(CONFIG_CPU_IDLE)		+= cpuidle.o idle2.o
