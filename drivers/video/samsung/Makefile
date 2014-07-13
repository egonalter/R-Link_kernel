#
# Makefile for the s3c framebuffer driver
#

ifeq ($(CONFIG_FB_S3C),y)
obj-y				+= s3cfb.o
obj-$(CONFIG_ARCH_S5P6440)	+= s3cfb_fimd5x.o
obj-$(CONFIG_ARCH_S5P6450)      += s3cfb_fimd5x.o
obj-$(CONFIG_ARCH_S5PV210)	+= s3cfb_fimd6x.o

obj-$(CONFIG_FB_S3C_LTE480WV)	+= s3cfb_lte480wv.o
obj-$(CONFIG_FB_S3C_LMS500)	+= s3cfb_lms500.o
obj-$(CONFIG_FB_S3C_HT101HD1)	+= s3cfb_ht101hd1.o
endif

ifeq ($(CONFIG_FB_S3C_DEBUG),y)
EXTRA_CFLAGS += -DDEBUG
endif
