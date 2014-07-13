#
# Makefile for the linux kernel.
#

# Common support
# !!!!Don't remove the obj-y line!!!!
obj-y					+=
obj-$(CONFIG_STRICT_DEVMEM)		+= mem.o
obj-$(CONFIG_TOMTOM_GADC)		+= gadc_generic.o
obj-$(CONFIG_TOMTOM_FDT)		+= libfdt.o fdtexport.o
obj-$(CONFIG_TOMTOM_TT_SETUP)		+= tt_setup_handler.o
obj-$(CONFIG_PLAT_TOMTOM)		+= tt_brick.o
obj-$(CONFIG_TOMTOM_TT_CRYPTO)		+= tt_crypto.o
obj-m :=
obj-n :=
obj-  :=

# Structure that are to be reused , not necessarily fully initialized
obj-$(CONFIG_TOMTOM_TT_SETUP)		+= tt_setup_dev.o

