# bcm4329
DHDCFLAGS = -DLINUX -DBCMDRIVER -DBCMDONGLEHOST -DDHDTHREAD -DBCMWPA2          \
	-DUNRELEASEDCHIP -DDHD_GPL -DDHD_SCHED -DBDC -DTOE -DDHD_BCMEVENTS     \
	-DSHOW_EVENTS -DSDIO_ISR_THREAD -DBCMSDIO -DDHD_GPL -DBCMLXSDMMC       \
	-DBCMPLATFORM_BUS -DEMBEDDED_PLATFORM -DOEM_ANDROID  -DCONFIG_MACH_MAHIMAHI                  \
	-DARP_OFFLOAD_SUPPORT -DPKT_FILTER_SUPPORT -DCUSTOMER_HW2              \
	-Wall -Wstrict-prototypes                                              \
	-Idrivers/net/wireless/bcm4330 -Idrivers/net/wireless/bcm4330/include

DHDOFILES = dhd_linux.o linux_osl.o bcmutils.o dhd_common.o dhd_custom_gpio.o       \
	siutils.o sbutils.o aiutils.o hndpmu.o dhd_linux_sched.o bcmwifi.o wl_iw.o  \
	dhd_cdc.o dhd_sdio.o bcmsdh_sdmmc.o bcmsdh.o bcmsdh_linux.o bcmevent.o dhd_bta.o               \
	bcmsdh_sdmmc_linux.o

obj-$(CONFIG_BCM4330) += bcm4330.o
bcm4330-objs += $(DHDOFILES)
EXTRA_CFLAGS = $(DHDCFLAGS)
EXTRA_LDFLAGS += --strip-debug
