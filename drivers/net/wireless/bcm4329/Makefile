# bcm4329
DHDCFLAGS = -DLINUX -DBCMDRIVER -DBCMDONGLEHOST -DDHDTHREAD -DBCMWPA2          \
	-DUNRELEASEDCHIP -DDHD_GPL -DDHD_SCHED -DBDC -DTOE -DDHD_BCMEVENTS     \
	-DSHOW_EVENTS -DSDIO_ISR_THREAD -DBCMSDIO -DDHD_GPL -DBCMLXSDMMC       \
	-DBCMPLATFORM_BUS -DEMBEDDED_PLATFORM -DOEM_ANDROID                    \
	-DARP_OFFLOAD_SUPPORT -DPKT_FILTER_SUPPORT -DCUSTOMER_HW2              \
	-Wall -Wstrict-prototypes                                              \
	-Idrivers/net/wireless/bcm4329 -Idrivers/net/wireless/bcm4329/include

DHDOFILES = dhd_linux.o linux_osl.o bcmutils.o dhd_common.o dhd_custom_gpio.o       \
	siutils.o sbutils.o aiutils.o hndpmu.o dhd_linux_sched.o bcmwifi.o wl_iw.o  \
	dhd_cdc.o dhd_sdio.o bcmsdh_sdmmc.o bcmsdh.o bcmsdh_linux.o                 \
	bcmsdh_sdmmc_linux.o

obj-$(CONFIG_BCM4329) += bcm4329.o
bcm4329-objs += $(DHDOFILES)
EXTRA_CFLAGS = $(DHDCFLAGS)
EXTRA_LDFLAGS += --strip-debug
