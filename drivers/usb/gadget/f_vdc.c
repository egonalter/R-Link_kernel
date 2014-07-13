/* b/drivers/usb/gadget/f_vdc.c
 * 
 * Copyright (c) 2009 TomTom BV <http://www.tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 */
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/platform_device.h>

#include <linux/usb/composite.h>
#include <linux/delay.h>
#include <linux/fb.h>

#include <linux/workqueue.h>
#include <linux/interrupt.h>

/* includes for sysfs */
#include <linux/cdev.h>
#include <linux/device.h>

#include <linux/mm.h>
#include <linux/dma-mapping.h>

/* Define this to enable the RLE variant of RGBP. This must use frame based descriptors
   which are *not* supported in Windows XP, and will cause the connected video device
   to fail to start (code 10). */
#define AVH_USE_RGBP_RLE

#ifdef CONFIG_USB_ANDROID_VDC
#include <linux/usb/android_composite.h>
#endif

#include "f_vdc.h"

#undef INFO
#undef DEBUG
#define ERROR

/* define BUGME to enable run-time invariant checks */
#define BUGME

#define PFX				"usb-fvdc: "

#ifdef INFO
#define info(fmt,args...) 		printk(KERN_INFO PFX fmt, ##args)
#else
#define info(fmt,args...) 		do {} while(0)
#endif

#ifdef DEBUG
#define debug(fmt,args...) 		printk(KERN_INFO PFX fmt, ##args)
#else
#define debug(fmt,args...) 		do {} while(0)
#endif

#ifdef ERROR
#define err(fmt,args...) 		printk(KERN_INFO PFX fmt, ##args)
#else
#define err(fmt,args...) 		do {} while(0)
#endif

#ifdef BUGME
#define BUGME_ON(cond)			do { if (cond) BUG(); } while(0)
#else
#define BUGME_ON(cond)			do { } while(0)
#endif

#define FVDC_NAME 			"fvdc"
#define FVDC_KWORK_NAME			"kvdc"

#define FVDC_URB_SIZE			(PAGE_SIZE)
#define FVDC_PAYLOAD_HEADER_SIZE	(8)
#define FVDC_PAYLOAD_FOOTER		(4)
#define FVDC_MAX_PAYLOAD_SIZE		(FVDC_URB_SIZE - FVDC_PAYLOAD_HEADER_SIZE - FVDC_PAYLOAD_FOOTER)

#ifdef AVH_USE_RGBP_RLE
#define FVDC_NR_FORMATS			(3)
#else
#define FVDC_NR_FORMATS			(2)
#endif
#define FVDC_DEFAULT_FB_INDEX		(-1)
#define FVDC_DEFAULT_WIDTH		(480)
#define FVDC_DEFAULT_HEIGHT		(272)

static struct usb_interface_assoc_descriptor vc_interface_association_desc = {
	.bLength = sizeof vc_interface_association_desc,
	.bDescriptorType = USB_DT_INTERFACE_ASSOCIATION,
	.bFirstInterface = 0,
	.bInterfaceCount = 2,
	.bFunctionClass = CC_VIDEO,
	.bFunctionSubClass = SC_VIDEO_INTERFACE_COLLECTION,
	.bFunctionProtocol = PC_PROTOCOL_UNDEFINED,
};

/* 
 * Video Control Interface Section
 */

static struct usb_interface_descriptor video_control_interface_desc = {
	.bLength = sizeof video_control_interface_desc,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 0,
	.bAlternateSetting = 0,
	.bNumEndpoints = 0,
	.bInterfaceClass = CC_VIDEO,
	.bInterfaceSubClass = SC_VIDEOCONTROL,
	.bInterfaceProtocol = PC_PROTOCOL_UNDEFINED,
};

/* We only have an Intput Terminal and an Output Terminal. No selector/processing/extension units, etc. */
static struct usb_input_terminal_descriptor camera_terminal_desc = {
	.bLength = sizeof camera_terminal_desc,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = VC_INPUT_TERMINAL,
	.bTerminalID = 1,
	.wTerminalType = __constant_cpu_to_le16(ITT_CAMERA),
	.bAssocTerminal = 3,
	.iTerminal = 0,
	.wObjectiveFocalLengthMin = 0,
	.wObjectiveFocalLengthMax = 0,
	.wOcularFocalLength = 0,
	.bControlSize = 2,
	.bmControls0 = 0,
	.bmControls1 = 0,
};

static struct usb_vc_output_terminal_descriptor output_terminal_desc = {
	.bLength = sizeof output_terminal_desc,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = VC_OUTPUT_TERMINAL,
	.bTerminalID = 3,
	.wTerminalType = __constant_cpu_to_le16(TT_STREAMING),
	.bAssocTerminal = 1,
	.bSourceID = 1,
	.iTerminal = 0,
};

static struct usb_cs_vc_interface_descriptor vc_video_control_interface_desc = {
	.bLength = sizeof vc_video_control_interface_desc,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = VC_HEADER,
	.bcdUVC = __constant_cpu_to_le16(0x0110),
	.wTotalLength =
	    __constant_cpu_to_le16(sizeof(vc_video_control_interface_desc) +
				   sizeof(camera_terminal_desc) +
				   sizeof(output_terminal_desc)),
	.dwClockFrequency = __constant_cpu_to_le32(0x0000BB80),
	.bInCollection = 1,
	.baInterfaceNr = 1,
};

/* 
 * Video Stream Interface Section
 */

static struct usb_interface_descriptor video_stream_interface_desc = {
	.bLength = sizeof video_stream_interface_desc,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 1,
	.bAlternateSetting = 0,
	.bNumEndpoints = 1,
	.bInterfaceClass = CC_VIDEO,
	.bInterfaceSubClass = SC_VIDEOSTREAMING,
	.bInterfaceProtocol = PC_PROTOCOL_UNDEFINED,
};

/* the intervals below are in units of 100ns */
#define VC_FRAME_INTERVAL_DEFAULT		2000000	/* 5 fps */
#define VC_FRAME_INTERVAL_MIN			500000	/* 20 fps */
#define VC_FRAME_INTERVAL_MAX			1000000000	/* 0.01 fps */
#define VC_FRAME_INTERVAL_STEP			(VC_FRAME_INTERVAL_MIN)

#define VC_YUYV_FORMAT_INDEX			1
#define VC_RGBP_FORMAT_INDEX			2
#define VC_RGBP_RLE_FORMAT_INDEX		3

#define VC_YUYV_FORMAT_BPP			16
#define VC_RGBP_RLE_FORMAT_BPP			16
#define VC_RGBP_FORMAT_BPP			16
#define VC_MAX_FORMAT_BPP			16
#define VC_DEFAULT_BPP				16
#define VC_DELAY_DEFAULT			100
#define VC_CLOCK_FREQUENCY_DEFAULT		48000

#define VC_EOF_MASK				2

/* YUYV Format and Frame Descriptors */

static struct usb_uncompressed_format_descriptor yuyv_format_desc = {
	.bLength = sizeof yuyv_format_desc,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = VS_FORMAT_UNCOMPRESSED,
	.bFormatIndex = VC_YUYV_FORMAT_INDEX,
	.bNumFrameDescriptors = 1,
	.guidFormat = {'Y', 'U', 'Y', '2', 0x00, 0x00, 0x10, 0x00,
		       0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71}
	,
	.bBitsPerPixel = VC_YUYV_FORMAT_BPP,
	.bDefaultFrameIndex = 1,
	.bAspectRatioX = 0,
	.bAspectRatioY = 0,
	.bmInterlaceflags = 0,
	.bCopyProtect = 0,
};

struct usb_uncompressed_frame_descriptor yuyv_frame_desc = {
	.bLength = sizeof yuyv_frame_desc,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = VS_FRAME_UNCOMPRESSED,
	.bFrameIndex = 1,
	.bmCapabilities = 0,
	.wWidth = __constant_cpu_to_le16(FVDC_DEFAULT_WIDTH),
	.wHeight = __constant_cpu_to_le16(FVDC_DEFAULT_HEIGHT),
	.dwMinBitRate =
	    __constant_cpu_to_le32(FVDC_DEFAULT_WIDTH *
				   FVDC_DEFAULT_HEIGHT * VC_YUYV_FORMAT_BPP /
				   100),
	.dwMaxBitRate =
	    __constant_cpu_to_le32(FVDC_DEFAULT_WIDTH *
				   FVDC_DEFAULT_HEIGHT * VC_YUYV_FORMAT_BPP *
				   20),
	.dwMaxVideoFrameBufferSize = __constant_cpu_to_le32(0),
	.dwDefaultFrameInterval =
	    __constant_cpu_to_le32(VC_FRAME_INTERVAL_DEFAULT),
	.bFrameIntervalType = 0,
	.dwMinFrameInterval = __constant_cpu_to_le32(VC_FRAME_INTERVAL_MIN),
	.dwMaxFrameInterval = __constant_cpu_to_le32(VC_FRAME_INTERVAL_MAX),
	.dwFrameIntervalStep = __constant_cpu_to_le32(VC_FRAME_INTERVAL_MIN),
};

static struct usb_color_matching_descriptor yuyv_color_matching_desc = {
	.bLength = sizeof yuyv_color_matching_desc,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = VS_COLORFORMAT,
	.bColorPrimaries = 1,
	.bTransferCharacteristics = 1,
	.bMatrixCoefficients = 4,
};

/* RGBP Format and Frame Descriptors */

#ifdef AVH_USE_RGBP_RLE
static struct usb_frame_based_format_descriptor rgbp_format_desc = {
	.bLength = sizeof rgbp_format_desc,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = VS_FORMAT_FRAME_BASED,
	.bFormatIndex = VC_RGBP_FORMAT_INDEX,
	.bNumFrameDescriptors = 1,
	.guidFormat =
	    {'R', 'G', 'B', 'P', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xAA,
	     0x00, 0x38, 0x9B, 0x71}
	,
	.bBitsPerPixel = VC_RGBP_FORMAT_BPP,
	.bDefaultFrameIndex = 1,
	.bAspectRatioX = 0,
	.bAspectRatioY = 0,
	.bmInterlaceflags = 0,
	.bCopyProtect = 1,
	.bVariableSize = 0,
};

struct usb_frame_based_frame_descriptor rgbp_frame_desc = {
	.bLength = sizeof rgbp_frame_desc,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = VS_FRAME_FRAME_BASED,
	.bFrameIndex = 1,
	.bmCapabilities = 0,
	.wWidth = __constant_cpu_to_le16(FVDC_DEFAULT_WIDTH),
	.wHeight = __constant_cpu_to_le16(FVDC_DEFAULT_HEIGHT),
	.dwMinBitRate =
	    __constant_cpu_to_le32(FVDC_DEFAULT_WIDTH *
				   FVDC_DEFAULT_HEIGHT *
				   VC_RGBP_RLE_FORMAT_BPP / 100),
	.dwMaxBitRate =
	    __constant_cpu_to_le32(FVDC_DEFAULT_WIDTH *
				   FVDC_DEFAULT_HEIGHT *
				   VC_RGBP_RLE_FORMAT_BPP * 20),
	.dwDefaultFrameInterval =
	    __constant_cpu_to_le32(VC_FRAME_INTERVAL_DEFAULT),
	.bFrameIntervalType = 0,
	.dwMinFrameInterval = __constant_cpu_to_le32(VC_FRAME_INTERVAL_MIN),
	.dwMaxFrameInterval = __constant_cpu_to_le32(VC_FRAME_INTERVAL_MAX),
	.dwFrameIntervalStep = __constant_cpu_to_le32(VC_FRAME_INTERVAL_MIN),
	.dwBytesPerLine =
	    __constant_cpu_to_le32(FVDC_DEFAULT_WIDTH *
				   VC_RGBP_RLE_FORMAT_BPP / 8),
};

#else
static struct usb_uncompressed_format_descriptor rgbp_format_desc = {
	.bLength = sizeof rgbp_format_desc,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = VS_FORMAT_UNCOMPRESSED,
	.bFormatIndex = VC_RGBP_FORMAT_INDEX,
	.bNumFrameDescriptors = 1,
	.guidFormat = {'R', 'G', 'B', 'P', 0x00, 0x00, 0x10, 0x00,
		       0x80, 0x00, 0x00, 0xAA, 0x00, 0x38, 0x9B, 0x71}
	,
	.bBitsPerPixel = VC_RGBP_FORMAT_BPP,
	.bDefaultFrameIndex = 1,
	.bAspectRatioX = 0,
	.bAspectRatioY = 0,
	.bmInterlaceflags = 0,
	.bCopyProtect = 0,
};

struct usb_uncompressed_frame_descriptor rgbp_frame_desc = {
	.bLength = sizeof rgbp_frame_desc,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = VS_FRAME_UNCOMPRESSED,
	.bFrameIndex = 1,
	.bmCapabilities = 0,
	.wWidth = __constant_cpu_to_le16(FVDC_DEFAULT_WIDTH),
	.wHeight = __constant_cpu_to_le16(FVDC_DEFAULT_HEIGHT),
	.dwMinBitRate =
	    __constant_cpu_to_le32(FVDC_DEFAULT_WIDTH *
				   FVDC_DEFAULT_HEIGHT * VC_RGBP_FORMAT_BPP /
				   100),
	.dwMaxBitRate =
	    __constant_cpu_to_le32(FVDC_DEFAULT_WIDTH *
				   FVDC_DEFAULT_HEIGHT * VC_RGBP_FORMAT_BPP *
				   20),
	.dwMaxVideoFrameBufferSize = __constant_cpu_to_le32(0),
	.dwDefaultFrameInterval =
	    __constant_cpu_to_le32(VC_FRAME_INTERVAL_DEFAULT),
	.bFrameIntervalType = 0,
	.dwMinFrameInterval = __constant_cpu_to_le32(VC_FRAME_INTERVAL_MIN),
	.dwMaxFrameInterval = __constant_cpu_to_le32(VC_FRAME_INTERVAL_MAX),
	.dwFrameIntervalStep = __constant_cpu_to_le32(VC_FRAME_INTERVAL_MIN),
};
#endif

static struct usb_color_matching_descriptor rgbp_color_matching_desc = {
	.bLength = sizeof rgbp_color_matching_desc,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = VS_COLORFORMAT,
	.bColorPrimaries = 1,
	.bTransferCharacteristics = 1,
	.bMatrixCoefficients = 4,
};

#ifdef AVH_USE_RGBP_RLE
/* RGBp (-RLE) Format and Frame Descriptors */

static struct usb_frame_based_format_descriptor rgbp_rle_format_desc = {
	.bLength = sizeof rgbp_rle_format_desc,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = VS_FORMAT_FRAME_BASED,
	.bFormatIndex = VC_RGBP_RLE_FORMAT_INDEX,
	.bNumFrameDescriptors = 1,
	.guidFormat =
	    {'R', 'G', 'B', 'p', 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0xAA,
	     0x00, 0x38, 0x9B, 0x71}
	,
	.bBitsPerPixel = VC_RGBP_RLE_FORMAT_BPP,
	.bDefaultFrameIndex = 1,
	.bAspectRatioX = 0,
	.bAspectRatioY = 0,
	.bmInterlaceflags = 0,
	.bCopyProtect = 1,
	.bVariableSize = 1,
};

struct usb_frame_based_frame_descriptor rgbp_rle_frame_desc = {
	.bLength = sizeof rgbp_rle_frame_desc,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = VS_FRAME_FRAME_BASED,
	.bFrameIndex = 1,
	.bmCapabilities = 0,
	.wWidth = __constant_cpu_to_le16(FVDC_DEFAULT_WIDTH),
	.wHeight = __constant_cpu_to_le16(FVDC_DEFAULT_HEIGHT),
	.dwMinBitRate =
	    __constant_cpu_to_le32(FVDC_DEFAULT_WIDTH *
				   FVDC_DEFAULT_HEIGHT *
				   VC_RGBP_RLE_FORMAT_BPP / 100),
	.dwMaxBitRate =
	    __constant_cpu_to_le32(FVDC_DEFAULT_WIDTH *
				   FVDC_DEFAULT_HEIGHT *
				   VC_RGBP_RLE_FORMAT_BPP * 20),
	.dwDefaultFrameInterval =
	    __constant_cpu_to_le32(VC_FRAME_INTERVAL_DEFAULT),
	.bFrameIntervalType = 0,
	.dwMinFrameInterval = __constant_cpu_to_le32(VC_FRAME_INTERVAL_MIN),
	.dwMaxFrameInterval = __constant_cpu_to_le32(VC_FRAME_INTERVAL_MAX),
	.dwFrameIntervalStep = __constant_cpu_to_le32(VC_FRAME_INTERVAL_MIN),
	.dwBytesPerLine = __constant_cpu_to_le32(0),
};

static struct usb_color_matching_descriptor rgbp_rle_color_matching_desc = {
	.bLength = sizeof rgbp_rle_color_matching_desc,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = VS_COLORFORMAT,
	.bColorPrimaries = 1,
	.bTransferCharacteristics = 1,
	.bMatrixCoefficients = 4,
};

#endif

static struct usb_cs_vs_interface_input_header_descriptor
    vc_video_stream_interface_desc = {
	.bLength = 13 + FVDC_NR_FORMATS,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubType = VC_HEADER,
	.bNumFormats = FVDC_NR_FORMATS,
	.wTotalLength = __constant_cpu_to_le16(13 + FVDC_NR_FORMATS +
					       sizeof(yuyv_format_desc) +
					       sizeof(yuyv_frame_desc) +
					       sizeof(yuyv_color_matching_desc)
					       + sizeof(rgbp_format_desc) +
					       sizeof(rgbp_frame_desc) +
					       sizeof(rgbp_color_matching_desc)
#ifdef AVH_USE_RGBP_RLE
					       +
					       sizeof(rgbp_rle_format_desc) +
					       sizeof(rgbp_rle_frame_desc) +
					       sizeof
					       (rgbp_rle_color_matching_desc)
#endif
	    ),
	.bEndpointAddress = USB_DIR_IN,
	.bmInfo = 0,
	.bTerminalLink = 3,
	.bStillCaptureMethod = 0,
	.bTriggerSupport = 0,
	.bTriggerUsage = 0,
	.bControlSize = 1,
	.bmaControls0 = 0,
	.bmaControls1 = 0,
	.bmaControls2 = 0,
	.bmaControls3 = 0,
	.bmaControls4 = 0,
	.bmaControls5 = 0,
	.bmaControls6 = 0,
	.bmaControls7 = 0,
};

/* 
 * Video Endpoint Section
 */

static struct usb_endpoint_descriptor
 fs_bulk_in_ep_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = USB_DIR_IN,
	.bmAttributes = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize = __constant_cpu_to_le16(64),
	.bInterval = 0,
};

static struct usb_endpoint_descriptor
 hs_bulk_in_ep_desc = {
	.bLength = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = USB_DIR_IN,
	.bmAttributes = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize = __constant_cpu_to_le16(512),
	.bInterval = 0,
};

static struct usb_video_probe_commit_controls fvdc_default_controls = {
	.bmHint = __constant_cpu_to_le16(0),
	.bFormatIndex = 1,
	.bFrameIndex = 1,
	.dwFrameInterval = __constant_cpu_to_le32(VC_FRAME_INTERVAL_DEFAULT),
	.wDelay = __constant_cpu_to_le16(VC_DELAY_DEFAULT),
	.dwMaxVideoFrameSize =
	    __constant_cpu_to_le32(FVDC_DEFAULT_WIDTH *
				   FVDC_DEFAULT_HEIGHT * VC_DEFAULT_BPP / 8),
	.dwMaxPayloadTransferSize = __constant_cpu_to_le32(FVDC_URB_SIZE),
	.dwClockFrequency = __constant_cpu_to_le32(VC_CLOCK_FREQUENCY_DEFAULT),
	.bmFramingInfo = 3,
	.bPreferedVersion = 1,
	.bMinVersion = 1,
	.bMaxVersion = 1,
};

static struct usb_descriptor_header *fvdc_fs_function[] = {
	(struct usb_descriptor_header *)&vc_interface_association_desc,
	(struct usb_descriptor_header *)&video_control_interface_desc,
	(struct usb_descriptor_header *)&vc_video_control_interface_desc,
	(struct usb_descriptor_header *)&camera_terminal_desc,
	(struct usb_descriptor_header *)&output_terminal_desc,
	(struct usb_descriptor_header *)&video_stream_interface_desc,
	(struct usb_descriptor_header *)&vc_video_stream_interface_desc,
	(struct usb_descriptor_header *)&yuyv_format_desc,
	(struct usb_descriptor_header *)&yuyv_frame_desc,
	(struct usb_descriptor_header *)&yuyv_color_matching_desc,
	(struct usb_descriptor_header *)&rgbp_format_desc,
	(struct usb_descriptor_header *)&rgbp_frame_desc,
	(struct usb_descriptor_header *)&rgbp_color_matching_desc,
#ifdef AVH_USE_RGBP_RLE
	(struct usb_descriptor_header *)&rgbp_rle_format_desc,
	(struct usb_descriptor_header *)&rgbp_rle_frame_desc,
	(struct usb_descriptor_header *)&rgbp_rle_color_matching_desc,
#endif
	(struct usb_descriptor_header *)&fs_bulk_in_ep_desc,
	0
};

#ifdef CONFIG_USB_GADGET_DUALSPEED
static struct usb_descriptor_header *fvdc_hs_function[] = {
	(struct usb_descriptor_header *)&vc_interface_association_desc,
	(struct usb_descriptor_header *)&video_control_interface_desc,
	(struct usb_descriptor_header *)&vc_video_control_interface_desc,
	(struct usb_descriptor_header *)&camera_terminal_desc,
	(struct usb_descriptor_header *)&output_terminal_desc,
	(struct usb_descriptor_header *)&video_stream_interface_desc,
	(struct usb_descriptor_header *)&vc_video_stream_interface_desc,
	(struct usb_descriptor_header *)&yuyv_format_desc,
	(struct usb_descriptor_header *)&yuyv_frame_desc,
	(struct usb_descriptor_header *)&yuyv_color_matching_desc,
	(struct usb_descriptor_header *)&rgbp_format_desc,
	(struct usb_descriptor_header *)&rgbp_frame_desc,
	(struct usb_descriptor_header *)&rgbp_color_matching_desc,
#ifdef AVH_USE_RGBP_RLE
	(struct usb_descriptor_header *)&rgbp_rle_format_desc,
	(struct usb_descriptor_header *)&rgbp_rle_frame_desc,
	(struct usb_descriptor_header *)&rgbp_rle_color_matching_desc,
#endif
	(struct usb_descriptor_header *)&hs_bulk_in_ep_desc,
	0
};
#endif /* CONFIG_USB_GADGET_DUALSPEED */

#define STRING_VIDEO_IDX		0
#define STRING_VIDEO_CONTROL_IDX	1
#define STRING_VIDEO_STREAM_IDX		2

static struct usb_string fvdc_strings_dev[] = {
	[STRING_VIDEO_IDX].s = "USB Video (Bulk)",
	[STRING_VIDEO_CONTROL_IDX].s = "USB Video Control",
	[STRING_VIDEO_STREAM_IDX].s = "USB Video Stream",
	{}
};

static struct usb_gadget_strings fvdc_stringtab_dev = {
	.language = 0x0409,
	.strings = fvdc_strings_dev,
};

static struct usb_gadget_strings *fvdc_strings[] = {
	&fvdc_stringtab_dev,
	0,
};

#define VC_PROBECOMMIT_REQUEST_LENGTH	34
#define VC_PROBECOMMIT_REQUEST_LENGTH_MIN 26

struct device fvdc_device;

static int fvdc_free_urbs(void);
static void fvdc_urb_complete(struct usb_ep *ep, struct usb_request *req);
static void fvdc_urb_final_complete(struct usb_ep *ep,
				      struct usb_request *req);

static int fvdc_stream_frame(void *dma, void *cpu, int size);
static void fvdc_halt(void);

static struct usb_video_probe_commit_controls fvdc_commit_controls;
static struct usb_video_probe_commit_controls fvdc_probe_controls;

enum fvdc_dev_state {
	FVDC_STATE_VOID,
	FVDC_STATE_IDLE,
	FVDC_STATE_CONFIGURED,
	FVDC_STATE_SELECTED,
	FVDC_STATE_STREAMABLE,
	FVDC_STATE_STREAMING
};

struct fvdc_frame {
	void *dma;
	void *cpu;
	unsigned long size;
};

static struct fvdc_dev {
	int alt;
	struct fb_info *attached_fb;
	int (*pan_display) (struct fb_var_screeninfo * var,
			    struct fb_info * info);

	int width;
	int height;

	struct usb_ep *bulk_in;
	struct usb_endpoint_descriptor *bulk_in_ep_desc;

	struct usb_request 		*probecommit_req;

	struct usb_request **payload_urb;
	int payload_urbs;

	enum fvdc_dev_state state;

	struct fvdc_frame pending;

	struct work_struct pending_frame_work;
	struct work_struct stream_frame_work;

	struct workqueue_struct *work_queue;
} *fvdc_dev = 0;

static inline void fvdc_set_dev_state(struct fvdc_dev *dev,
					enum fvdc_dev_state state)
{
	if (!dev)
		return;

	if (state != dev->state) {
		info("switching from %d to %d\n", dev->state, state);
		dev->state = state;
	}
}


/* ------------------------------------------------------------------------- */

static unsigned char ycomp(unsigned char r, unsigned char g, unsigned char b)
{
	unsigned long result;

	result = (((2171 * (r)) + (2122 * g) + (822 * b) + 16896) >> 10);

	return (unsigned char)result;
}

static unsigned char vcomp(unsigned char r, unsigned char g, unsigned char b)
{
	unsigned long result;

	result = (((3701 * (r)) - (1521 * g) - (592 * b) + 131584) >> 10);

	return (unsigned char)result;
}

static unsigned char ucomp(unsigned char r, unsigned char g, unsigned char b)
{
	unsigned long result;

	result = (((-1250 * (r)) - (1198 * g) + (3685 * b) + 131584) >> 10);

	return (unsigned char)result;
}

static int fvdc_encode_payload_yuy2(unsigned short *src, int nr_of_pixels)
{
	int pixels_done = 0;
	unsigned short int *dest = src;
	unsigned char red, green, blue;
	unsigned long int y0, y1, u0, u1, v0, v1, u, v;

	while (pixels_done < nr_of_pixels) {
		pixels_done += 2;

		blue = (*src) & 0x1f;
		*src >>= 5;

		green = (*src) & 0x3f;
		*src >>= 6;

		red = *src;

		src++;

		y0 = ycomp(red, green, blue);
		u0 = ucomp(red, green, blue);
		v0 = vcomp(red, green, blue);

		blue = (*src) & 0x1f;
		*src >>= 5;

		green = (*src) & 0x3f;
		*src >>= 6;

		red = *src;

		src++;

		y1 = ycomp(red, green, blue);
		u1 = ucomp(red, green, blue);
		v1 = vcomp(red, green, blue);

		u = (u0 + u1) >> 1;
		v = (v0 + v1) >> 1;

		*dest++ = (y0) | (u << 8);
		*dest++ = (y1) | (v << 8);
	}
	return nr_of_pixels * 2;
}

static int fvdc_encode_payload_rgbp(unsigned short *src, int nr_of_pixels)
{
	unsigned short int *dest = src;
	unsigned short int *orig = src;
	unsigned short int compare = *src++;
	unsigned long int runlength = 1;

	while (nr_of_pixels > 0) {
		if ((runlength < nr_of_pixels) && (compare == *src)) {
			runlength++;
			src++;
			if (runlength == 0xffff) {
				goto force_run;
			}
		} else {
		      force_run:
			// runlength >= nr_of_pixels || compare != *frame
			nr_of_pixels -= runlength;

			if (runlength > 2) {
				*dest++ = compare & 0xffdf;	// lsb bit green := 0 => runlength encoded color
				*dest++ = runlength;
			} else {
				*dest++ = compare | 0x0020;	// lsb bit green := 1 => normal color
				if (runlength == 2) {
					*dest++ = compare | 0x0020;	// lsb bit green := 1 => normal color
				}
			}

			compare = *src++;
			runlength = 1;
		}
	}

	return (dest - orig) * sizeof(*dest);
}

static int fvdc_encode_payload(int format, unsigned char *data, int nr_of_bytes)
{
	switch (format) {
	case VC_YUYV_FORMAT_INDEX:
		return fvdc_encode_payload_yuy2((unsigned short *)data,
						  nr_of_bytes / 2);

	case VC_RGBP_FORMAT_INDEX:
		/* data should already be RGB565 */
		return nr_of_bytes;

	case VC_RGBP_RLE_FORMAT_INDEX:
		return fvdc_encode_payload_rgbp((unsigned short *)data,
						  nr_of_bytes / 2);

	default:
		return -EPROTOTYPE;
	}
}

/* ------------------------------------------------------------------------- */

static inline int fvdc_remember_frame(void *dma, void *cpu, int size)
{
	unsigned long int flags;
	int ret = 0;
	int work = 0;

	local_irq_save(flags);
	{
		if (fvdc_dev->pending.size == 0) work = 1;
	
		fvdc_dev->pending.size = size;
		fvdc_dev->pending.dma = dma;
		fvdc_dev->pending.cpu = cpu;
	}
	local_irq_restore(flags);
	
	if (work) {
		ret = queue_work(fvdc_dev->work_queue,
				   &fvdc_dev->pending_frame_work);
	}

	return ret;
}

/* not to be called from atomic context, this function may block */
static int fvdc_stream_frame(void *dma, void *cpu, int size)
{
	int ret;

	debug("stream_frame\n");

	BUGME_ON(size < 0);
	BUGME_ON((!dma) && (!cpu));

	if (size == 0)
		return 0;

	if (!fvdc_dev)
		return -ENODEV;

	switch (fvdc_dev->state) {
	case FVDC_STATE_STREAMING:
		/* a request for streaming a frame while busy with sending a previous frame
		   this particular request will be remembered as a pending request, but it
		   will 'overwrite' any already pending request */

		ret = fvdc_remember_frame(dma, cpu, size);
		break;

	case FVDC_STATE_STREAMABLE:
		{
			/* we're currently not streaming; kick the work thread after copying all the
			   data to the video urb buffers */

			int index = 0;
			int offset = 0;
			void __iomem *dma_remapped = 0;

			if (dma) {
				dma_remapped = ioremap_cached((unsigned long)dma, size);
				cpu = (void*) dma_remapped;
			}

			/* switching the state to STREAMING prevents further frames to be accepted for
			   streaming (subsequent frames will be made pending; but only the last of the
			   pending frames actually will be streamed */

			fvdc_set_dev_state(fvdc_dev,
					     FVDC_STATE_STREAMING);

			while ((size > 0) && (fvdc_dev)
			       && (fvdc_dev->state ==
				   FVDC_STATE_STREAMING)) {
				struct usb_request *req =
				    fvdc_dev->payload_urb[index];
				int length =
				    (size <
				     FVDC_MAX_PAYLOAD_SIZE) ? (size)
				    : (FVDC_MAX_PAYLOAD_SIZE);

				size -= length;

				memcpy(req->buf + FVDC_PAYLOAD_HEADER_SIZE,
					cpu + offset, 
					length);

				offset += length;

				req->length =
				    length + FVDC_PAYLOAD_HEADER_SIZE;

				index++;

				if (likely(size > 0)) {
					req->complete = fvdc_urb_complete;
				} else {
					req->complete = fvdc_urb_final_complete;
				}
			}

			if (dma_remapped) {
				iounmap(dma_remapped);
			}

			info("waking up to stream a frame!\n");
			info("filled %d buffers\n", index);

			/* the data to be streamed is now safe in the payload 
			   buffers, the encoding and preparing the buffers for 
			   transmission is done by the video kernel thread */

			ret = queue_work(fvdc_dev->work_queue,
				       &fvdc_dev->stream_frame_work);
			break;
		}

	default:
		info("host's down\n");
		ret = -EHOSTDOWN;
		break;
	}

	return ret;
}

/* -------------------------------------------------------------------------
   worker functions
   ------------------------------------------------------------------------- */

static void fvdc_pending_frame_work(struct work_struct *data)
{
	unsigned long int flags;
	void *dma = 0;
	void *cpu = 0;
	int size = 0;

	local_irq_save(flags);
	{
		if (fvdc_dev->pending.size > 0) {
			dma = fvdc_dev->pending.dma;
			cpu = fvdc_dev->pending.cpu;
			size = fvdc_dev->pending.size;
	
			fvdc_dev->pending.size = 0;
			fvdc_dev->pending.cpu = 0;
			fvdc_dev->pending.dma = 0;
		}
	}
	local_irq_restore(flags);

	if (size > 0) {
		fvdc_stream_frame(dma, cpu, size);
	}

	return;
}

static void fvdc_stream_frame_work(struct work_struct *data)
{
#ifdef INFO
	static unsigned long int fc = 0;
#endif
	static int fid = 0;
	int index = 0;
	int eof = 0;
	int ret = 0;
	int format;
	unsigned long int flags;

	if (fvdc_dev->state < FVDC_STATE_STREAMING) {
		err("state < STREAMING (%d)", fvdc_dev->state);
		goto early_exit;
	}

	format = fvdc_commit_controls.bFormatIndex;

	do {
		struct usb_request *req = fvdc_dev->payload_urb[index++];
		unsigned char *buf = req->buf;

		info("encoding %ld.%d\n", fc, index);

		/* encoding payload data may change the size of the payload */
		req->length = fvdc_encode_payload(
					format,
					buf + FVDC_PAYLOAD_HEADER_SIZE,
					req->length - 
					FVDC_PAYLOAD_HEADER_SIZE
				) + FVDC_PAYLOAD_HEADER_SIZE;

		req->status = 0;
		req->zero = 1;

		/* if the completion function of the urb is fvdc_urb_final_complete, this
		   is the last payload of the frame sequence; and the end-of-frame bit
		   can be set; this also terminates this encoding/queuing loop. */
		if (req->complete == fvdc_urb_final_complete)
			eof = VC_EOF_MASK;

		BUGME_ON((eof == 0) && (index == fvdc_dev->payload_urbs));

		memset(buf, 0, FVDC_PAYLOAD_HEADER_SIZE);

		buf[0] = 8;
		buf[1] = 0x80 | 0x08 | fid | eof;

#ifdef INFO
		buf[2] = index & 0xff;
		buf[3] = (index >> 8) & 0xff;

		buf[4] = fc & 0xff;
		buf[5] = (fc >> 8) & 0xff;
		buf[6] = (fc >> 16) & 0xff;
		buf[7] = (fc >> 24) & 0xff;
#endif

		/* check whether the state of the device is still valid */
		local_irq_save(flags);

		if ((!fvdc_dev)
		    || (fvdc_dev->state != FVDC_STATE_STREAMING)) {
			err("video device in bad shape! aborting frame!\n");
			goto abort_locked;
		}

		if (req->length < 0) {
			err("error while encoding payload, aborting frame\n");
			goto abort_locked;
		}

		/* In case the underlying USB device controller driver
		 * is using DMA to transfer data from the main memory buffer
		 * pointed to by 'req->buf/req->dma', it must make sure the
		 * data is flushed out to main memory before enabling this
		 * dma transfer. The usual method to ensure this is by 
		 * calling dma_map_single() on the passed virtual address.
		 */

		if ((ret = usb_ep_queue(fvdc_dev->bulk_in, req, GFP_ATOMIC))) {
			/* error queuing urb */
			err("error queueing urb (%d)\n", ret);
			err("   dma  = %08x\n", (unsigned int)req->dma);
			err("   len  = %d\n", req->length);
			err("   buf  = %08x\n", (unsigned int)req->buf);
			err("   cmpl = %08x\n", (unsigned int)req->complete);
			err("   ctx = %08x\n", (unsigned int)req->context);
			err("   sts = %d\n", req->status);
			err("   act = %d\n", req->actual);
			err("   zero = %d\n", req->zero);
			err("aborting frame\n");

			goto abort_locked;
		}
		local_irq_restore(flags);
	} while (eof == 0);

	/* when successful in queuing all payloads of a frame, toggle the frame identifier 
	   note: this does not happen when the frame transfer was interrupted by
	   some low-level error
	 */
	fid = !fid;

#ifdef INFO
	fc++;
#endif

      early_exit:
	debug("exit normal\n");
	return;

      abort_locked:
	debug("aborting\n");
	if ((fvdc_dev) && (fvdc_dev->state == FVDC_STATE_STREAMING))
		fvdc_set_dev_state(fvdc_dev, FVDC_STATE_STREAMABLE);
	local_irq_restore(flags);
	return;
}

/* ------------------------------------------------------------------------- */

static void fvdc_abort(void)
{
	info("video streaming abort\n");

	BUGME_ON(!fvdc_dev);

	if (fvdc_dev->state == FVDC_STATE_STREAMING) {
		/* this aborts the ongoing streaming operation */
		fvdc_set_dev_state(fvdc_dev, FVDC_STATE_STREAMABLE);

		usb_ep_disable(fvdc_dev->bulk_in);

		mdelay(1);

		usb_ep_enable(fvdc_dev->bulk_in, fvdc_dev->bulk_in_ep_desc);

		mdelay(1);
	}
}

/* ------------------------------------------------------------------------- */

static int fvdc_pan_display(struct fb_var_screeninfo *var,
			      struct fb_info *info)
{
	/* this must call the actual pan-display function, and then stream the panned frame
	   if STREAMABLE, or queue the frame as pending if STREAMING */
	int result = 0;
	dma_addr_t dma_start = 0;
	void *cpu_start = 0;
	int length;
	int offset;

	/* this is called from the attached framebuffer driver, and should not block with
	   the exception for the 'pan_display' function (that is the original function
	   that was replaced in the fb_info stucture by this function) */

	BUGME_ON(!fvdc_dev);

	if (fvdc_dev->pan_display) {
		result = fvdc_dev->pan_display(var, info);
	}

	if (result)
		return result;

	BUGME_ON(!var);
	BUGME_ON(!info);

	if (var->xoffset != 0) {
		err("cannot handle xoffset in pan display hook\n");
		return -EINVAL;
	}

	offset = var->yoffset * info->fix.line_length;
	length = info->var.yres * info->fix.line_length;
	if (info->fix.smem_start) {
		dma_start = info->fix.smem_start + offset;
	}
	if (info->screen_base) {
		cpu_start = info->screen_base + offset;
	}

	fvdc_stream_frame((void *)dma_start, cpu_start, length);

	debug("hook: sending frame @ dma=%p (=%p kernel), cpu=%p, length=%d\n",
	      (void *)dma_start, phys_to_virt(dma_start), cpu_start, length);

	return 0;
}

static int fvdc_force_frame(struct fvdc_dev *dev)
{
	int offset;
	struct fb_info *info;

	BUGME_ON(!dev);

	info = dev->attached_fb;

	if (!info)
		return -ENOENT;

	if (info->var.xoffset) {
		info("can't stream frame with horizontal offset\n");
		return -EINVAL;
	}

	offset = info->var.yoffset * info->fix.line_length;

	return fvdc_remember_frame((void *)((unsigned long int)info->fix.smem_start + offset),
				(void *)((unsigned long int)info->screen_base + offset),
				info->var.yres * info->fix.line_length);
}

static int fvdc_detach_framebuffer(void)
{
	BUGME_ON(!fvdc_dev);

	/* remove hook pan display function */
	if (fvdc_dev->attached_fb) {
		if (fvdc_dev->attached_fb->fbops->fb_pan_display ==
		    fvdc_pan_display)
			fvdc_dev->attached_fb->fbops->fb_pan_display =
			    fvdc_dev->pan_display;
	}
	fvdc_dev->pan_display = 0;
	fvdc_dev->attached_fb = 0;

	return 0;
}

static int fvdc_attach_framebuffer(int index)
{
	int result = 0;
	struct fb_info *fb_info;

	info("attempt to attach to framebuffer %d\n", index);
	info("(%d framebuffer devices registered)\n", num_registered_fb);

	BUGME_ON(!fvdc_dev);
	BUGME_ON(index < 0);

	if (index >= num_registered_fb) {
		result = -EINVAL;
		goto fail;
	}

	fb_info = registered_fb[index];

	if (!fb_info) {
		err("frame buffer device has no info structure !?\n");
		result = -EINVAL;
		goto fail;
	}

	if (fvdc_dev->state == FVDC_STATE_IDLE) {
		/* while the width & height have not been announced
		   (idle state) its ok to change these */
		fvdc_dev->width = fb_info->var.xres;
		fvdc_dev->height = fb_info->var.yres;
	}

	if ((fb_info->var.xres != fvdc_dev->width) ||
	    (fb_info->var.yres != fvdc_dev->height)) {
		/* if these do not match, its not allowed to attach
		   because we can't change the usb announced resolution
		   on-the-fly */
		err("cannot change width / height (not in IDLE mode)\n");
		result = -EINVAL;
		goto fail;
	}

	fvdc_detach_framebuffer();

	fvdc_dev->attached_fb = fb_info;
	fvdc_dev->pan_display = fb_info->fbops->fb_pan_display;
	fb_info->fbops->fb_pan_display = fvdc_pan_display;

      fail:
	return result;
}

struct fb_info* fvdc_attached_framebuffer(void)
{
	return (fvdc_dev)?(fvdc_dev->attached_fb):(0);
}

/* ------------------------------------------------------------------------- 
       functions that are safe to call from atomic (non-blocking) context
   ------------------------------------------------------------------------- */

static void fvdc_probe_complete(struct usb_ep *ep,
					   struct usb_request *req)
{
	int set_format_index;
	struct usb_video_probe_commit_controls *set_cur =
	    (struct usb_video_probe_commit_controls *)req->buf;

	BUGME_ON(!set_cur);
	BUGME_ON(!fvdc_dev);

	if (fvdc_dev->state < FVDC_STATE_SELECTED) {
		err("probe request while not configured\n");
		return;
	}

	if (req->length < VC_PROBECOMMIT_REQUEST_LENGTH_MIN)
		return;

	if (req->status) {
		err("probe request failed (status=%d)\n", req->status);
		return;
	}

	/* SET_CUR on PROBE control is only allowed to change the
	   format index of the probe control */

	set_format_index = set_cur->bFormatIndex;

	info("probing format %d\n", set_format_index);

	switch (set_format_index) {
	case VC_YUYV_FORMAT_INDEX:
	case VC_RGBP_FORMAT_INDEX:
#ifdef AVH_USE_RGBP_RLE
	case VC_RGBP_RLE_FORMAT_INDEX:
#endif
		break;

	default:
		err("host probes unknown format index (%d)\n",
		    set_format_index);
		return;
	}

	/* all checks done, change the index in the probe control */

	fvdc_probe_controls.bFormatIndex = set_format_index;
	return;
}

static void fvdc_commit_complete(struct usb_ep *ep,
					    struct usb_request *req)
{
	int set_format_index;
	struct usb_video_probe_commit_controls *set_cur =
	    (struct usb_video_probe_commit_controls *)req->buf;

	BUGME_ON(!set_cur);
	BUGME_ON(!fvdc_dev);

	if (fvdc_dev->state < FVDC_STATE_SELECTED) {
		err("commit request while not configured\n");
		return;
	}

	if (req->length < VC_PROBECOMMIT_REQUEST_LENGTH_MIN)
		return;

	if (req->status) {
		err("commit request failed (status=%d)\n", req->status);
		return;
	}

	set_format_index = set_cur->bFormatIndex;

	info("committing format %d\n", set_format_index);

	switch (set_format_index) {
	case VC_YUYV_FORMAT_INDEX:
	case VC_RGBP_FORMAT_INDEX:
#ifdef AVH_USE_RGBP_RLE
	case VC_RGBP_RLE_FORMAT_INDEX:
#endif
		break;

	default:
		err("host commits unknown format index (%d)\n",
		    set_format_index);
		return;
	}

	/* committing format choice */

	if (fvdc_commit_controls.bFormatIndex != set_format_index) {
		fvdc_commit_controls.bFormatIndex = set_format_index;
	}

	if (fvdc_dev->state == FVDC_STATE_SELECTED) {
		fvdc_set_dev_state(fvdc_dev, FVDC_STATE_STREAMABLE);
	}

	if (fvdc_dev->state == FVDC_STATE_STREAMABLE) {
		fvdc_force_frame(fvdc_dev);
	}

	return;
}


/* this is the callback function that is called by the controller driver
   for all queued requests; it is called in interrupt context but in some cases also
   in process context (if the endpoint is disabled, or the request is explicitly dequeued) */

static void fvdc_urb_complete(struct usb_ep *ep,
					 struct usb_request *req)
{
	int status = req->status;

	info("urb complete (status = %d, nr = %d)\n", status,
	     (int)req->context);

	BUGME_ON(!fvdc_dev);

	if (fvdc_dev->state <= FVDC_STATE_STREAMABLE) {
		info("urb completion while not streamable/streaming\n");
		return;
	}

	switch (status) {
	case 0:
		debug("request done (%p)\n", req);
		break;

	case -ECONNABORTED:
		err("hardware forced ep reset\n");
		break;

	case -ESHUTDOWN:
		err("disconnect from host?\n");
		break;

	case -ECONNRESET:
		err("request was dequeued?\n");
		break;

	case -EOVERFLOW:
		err("request caused internal overflow?\n");
		break;

	default:
		err("unknown error %d\n", status);
		break;
	}

	info("request done (%p)\n", req);

	return;
}

/* this is the callback function that is called by the controller driver
   for all queued requests; it is called in interrupt context but in some cases also
   in process context (if the endpoint is disabled, or the request is explicitly dequeued) */

static void fvdc_urb_final_complete(struct usb_ep *ep,
					       struct usb_request *req)
{
	int status = req->status;

	info("urb final complete (status = %d, nr = %d)\n", status,
	     (int)req->context);

	BUGME_ON(!fvdc_dev);

	if (fvdc_dev->state <= FVDC_STATE_STREAMABLE) {
		info("urb completion while not streaming\n");
		return;
	}

	switch (status) {
	case 0:
		debug("request done (%p)\n", req);
		if (fvdc_dev->state == FVDC_STATE_STREAMING) {
			fvdc_set_dev_state(fvdc_dev,
					     FVDC_STATE_STREAMABLE);
		}
#if 0
		if ((fvdc_dev->state >= FVDC_STATE_STREAMABLE) &&
		    (fvdc_dev->pending.size > 0)) {
			queue_work(fvdc_dev->work_queue,
				   &fvdc_dev->pending_frame_work);
		}
#endif
		break;

	case -ECONNABORTED:
		err("hardware forced ep reset\n");
		break;

	case -ESHUTDOWN:
		err("disconnect from host?\n");
		break;

	case -ECONNRESET:
		err("request was dequeued?\n");
		fvdc_set_dev_state(fvdc_dev, FVDC_STATE_STREAMABLE);
		/* clear this error, as this is recoverable from the device side */
		status = 0;
		/* fall through intentional */

	default:
		fvdc_halt();
		break;
	}

	return;
}

/* this is called from composite.c, when it defers an interface specific setup request
   to this function driver. It should only be called on interrupt context. */

static int fvdc_setup(struct usb_function *f,
				 const struct usb_ctrlrequest *ctrl)
{
	struct usb_composite_dev *cdev = f->config->cdev;
	struct usb_request *req = cdev->req;
	int value = -EOPNOTSUPP;
	u16 w_value = le16_to_cpu(ctrl->wValue);
	u16 w_length = le16_to_cpu(ctrl->wLength);

	/* We only support the videostreaming probe and commit
	 * control selectors. Anything else is stalled. */

	BUGME_ON(!fvdc_dev);
	BUGME_ON(!fvdc_dev->probecommit_req);
	BUGME_ON(fvdc_dev->state < FVDC_STATE_CONFIGURED);

	if (ctrl->wIndex != video_stream_interface_desc.bInterfaceNumber)
		return value;

	switch (w_value >> 8) {
	case VS_PROBE_CONTROL:
		info("probe control - %d\n", ctrl->bRequest);
		switch (ctrl->bRequest) {
		case GET_CUR:
			value =
			    min(w_length, (u16) sizeof(fvdc_probe_controls));
			memcpy(req->buf, &fvdc_probe_controls, value);
			break;

		case GET_MIN:
			value =
			    min(w_length, (u16) sizeof(fvdc_probe_controls));
			memcpy(req->buf, &fvdc_probe_controls, value);
			break;

		case GET_MAX:
			value =
			    min(w_length, (u16) sizeof(fvdc_probe_controls));
			memcpy(req->buf, &fvdc_probe_controls, value);
			break;

		case GET_DEF:
			value =
			    min(w_length,
				(u16) sizeof(fvdc_default_controls));
			memcpy(req->buf, &fvdc_default_controls, value);
			break;

		case GET_LEN:
			if ((w_length == 1) || (w_length == 2)) {
				((unsigned char *)req->buf)[0] =
				    VC_PROBECOMMIT_REQUEST_LENGTH;
				((unsigned char *)req->buf)[1] = 0;
				value = w_length;
			} else
				value = 0;
			break;

		case GET_INFO:
			if (w_length == 1) {
				((unsigned char *)req->buf)[0] = 0x03;
				value = 1;
			} else
				value = 0;
			break;

		case SET_CUR:
			value = min(w_length, (u16) VC_PROBECOMMIT_REQUEST_LENGTH);
			fvdc_dev->probecommit_req->length = value;
			fvdc_dev->probecommit_req->zero = value < w_length;
			fvdc_dev->probecommit_req->context = 0;
			fvdc_dev->probecommit_req->complete = fvdc_probe_complete;
			value = usb_ep_queue(cdev->gadget->ep0, fvdc_dev->probecommit_req, GFP_ATOMIC);
			return value;

		default:
			err("unsupported probe request (0x%x)\n", ctrl->bRequest);
			return value;
		}
		break;

	case VS_COMMIT_CONTROL:
		info("commit control - %d\n", ctrl->bRequest);
		switch (ctrl->bRequest) {
		case GET_CUR:
			value =
			    min(w_length, (u16) sizeof(fvdc_commit_controls));
			memcpy(req->buf, &fvdc_commit_controls, value);
			break;

		case GET_LEN:
			if ((w_length == 1) || (w_length == 2)) {
				((unsigned char *)req->buf)[0] =
				    VC_PROBECOMMIT_REQUEST_LENGTH;
				((unsigned char *)req->buf)[1] = 0;
				value = w_length;
			} else
				value = 0;
			break;

		case GET_INFO:
			if (w_length == 1) {
				((unsigned char *)req->buf)[0] = 0x03;
				value = 1;
			} else
				value = 0;
			break;

		case SET_CUR:
			value = min(w_length, (u16) VC_PROBECOMMIT_REQUEST_LENGTH);
			fvdc_dev->probecommit_req->length = value;
			fvdc_dev->probecommit_req->zero = value < w_length;
			fvdc_dev->probecommit_req->context = 0;
			fvdc_dev->probecommit_req->complete = fvdc_commit_complete;
			value = usb_ep_queue(cdev->gadget->ep0, fvdc_dev->probecommit_req, GFP_ATOMIC);
			return value;

		default:
			err("unsupported commit request (0x%x)\n", ctrl->bRequest);
			return value;
		}
		break;
	default:
		err("received unsupported control selector (0x%x)\n", w_value >> 8);
		return value;
	}

	if (value >= 0) {
		value = min((u16)value, w_length);
		req->length = value;
		req->zero = value < ctrl->wLength;
		value = usb_ep_queue(cdev->gadget->ep0, req, GFP_ATOMIC);
	}

	return value;
}

/* variant of 'disable' without function pointer argument */
static void fvdc_halt(void) {
	BUGME_ON(!fvdc_dev);

	if (fvdc_dev->state <= FVDC_STATE_CONFIGURED)
		return;

	fvdc_abort();

	fvdc_set_dev_state(fvdc_dev, FVDC_STATE_CONFIGURED);

	if (fvdc_dev->bulk_in) {
		info("disabling bulk endpoint...\n");
		usb_ep_disable(fvdc_dev->bulk_in);
	}
}

/* called from composite.c, handling a set configuration request, on interrupt context */
static void fvdc_disable(struct usb_function *f)
{
	info("disable\n");

	BUGME_ON(!fvdc_dev);

	fvdc_halt();
}

static int fvdc_set_alt(struct usb_function *f, unsigned intf,
				   unsigned alt)
{
	struct usb_composite_dev *cdev = f->config->cdev;

	BUGME_ON(!fvdc_dev);
	BUGME_ON(fvdc_dev->state < FVDC_STATE_CONFIGURED);

	info("setting alternate setting (intf=%d,alt=%d,current=%d)\n", intf,
	     alt, fvdc_dev->alt);

	if ((fvdc_dev->state < FVDC_STATE_SELECTED) &&
	    (intf == video_stream_interface_desc.bInterfaceNumber)) {
		int ret;

		fvdc_set_dev_state(fvdc_dev, FVDC_STATE_SELECTED);

		info("init bulk-in (intf: %d alt: %d)\n", intf, alt);

		fvdc_dev->bulk_in_ep_desc =
		    ep_choose(cdev->gadget, &hs_bulk_in_ep_desc,
			      &fs_bulk_in_ep_desc);

		info("bulk_in_ep_desc is %p\n", fvdc_dev->bulk_in_ep_desc);

		ret =
		    usb_ep_enable(fvdc_dev->bulk_in,
				  fvdc_dev->bulk_in_ep_desc);

		fvdc_dev->alt = alt;

		if (ret) {
			err("failed to enable bulk-in for video\n");
			return ret;
		}
	}

	return 0;
}

static int fvdc_get_alt(struct usb_function *f, unsigned intf)
{
	BUGME_ON(!fvdc_dev);
	info("getting alternate setting (=%d)", fvdc_dev->alt);
	return fvdc_dev->alt;
}

static void fvdc_suspend(struct usb_function *f)
{
	info("suspend (nop)\n");
	fvdc_halt();
}

static void fvdc_resume(struct usb_function *f)
{
	info("resume (nop)\n");
}

/* ------------------------------------------------------------------------- */

static int /* __init_or_exit */ fvdc_free_urbs(void)
{
	int index;

	BUGME_ON(!fvdc_dev);
	BUGME_ON(fvdc_dev->state != FVDC_STATE_IDLE);

	for (index = 0; index < fvdc_dev->payload_urbs; index++) {
		struct usb_request *req = fvdc_dev->payload_urb[index];
		fvdc_dev->payload_urb[index] = 0;
		if (req) {
			if (req->buf) {
				ClearPageReserved(virt_to_page
						  ((void *)req->buf));

				free_page((unsigned long)req->buf);

				req->dma = 0;
				req->buf = 0;
			}

			usb_ep_free_request(fvdc_dev->bulk_in, req);
		}
	}

	kfree(fvdc_dev->payload_urb);
	fvdc_dev->payload_urb = 0;
	fvdc_dev->payload_urbs = 0;

	return 0;
}

/* ------------------------------------------------------------------------- */

static int __init fvdc_request_urbs(void)
{
	int ret;
	int index;

	BUGME_ON(!fvdc_dev);
	BUGME_ON(fvdc_dev->state != FVDC_STATE_IDLE);

	info("attached fb %p\n", fvdc_dev->attached_fb);

	info("allocating URBs for frame size %dx%d @ %d bpp\n",
	     fvdc_dev->attached_fb->var.xres,
	     fvdc_dev->attached_fb->var.yres,
	     fvdc_dev->attached_fb->var.bits_per_pixel);

	fvdc_dev->payload_urbs = (FVDC_MAX_PAYLOAD_SIZE - 1 +
				    ((fvdc_dev->attached_fb->var.xres *
				      fvdc_dev->attached_fb->var.yres *
				      fvdc_dev->attached_fb->var.
				      bits_per_pixel) >> 3))
	    / FVDC_MAX_PAYLOAD_SIZE;

	fvdc_dev->payload_urb = kmalloc(fvdc_dev->payload_urbs *
					  sizeof(struct usb_request *),
					  GFP_KERNEL);

	if (!fvdc_dev->payload_urb) {
		err("failed to allocate URB pointers\n");
		return -ENOMEM;
	}

	info("allocating %d video urbs (each of %ld bytes)\n",
	     fvdc_dev->payload_urbs, FVDC_URB_SIZE);

	BUGME_ON(FVDC_URB_SIZE > PAGE_SIZE);

	for (index = 0; index < fvdc_dev->payload_urbs; index++) {
		struct page *page;
		struct usb_request *req =
		    usb_ep_alloc_request(fvdc_dev->bulk_in, GFP_KERNEL);

		if (!req) {
			err("failed to allocate usb request on bulk-in endpoint\n");
			ret = -ENOMEM;
			goto failure;
		}

		req->length = 0;
		req->complete = fvdc_urb_complete;
		req->context = (void *)index;

		page = alloc_page(GFP_KERNEL);

		if (!page) {
			req->buf = 0;
			req->dma = 0;
			usb_ep_free_request(fvdc_dev->bulk_in, req);

			ret = -ENOMEM;
			goto failure;
		}

		SetPageReserved(page);

		req->buf = page_address(page);
		req->dma = virt_to_dma(&fvdc_device, req->buf);

		info("allocated a page buffer (cpu=%p/dma=%08x)\n", req->buf,
		     req->dma);

		fvdc_dev->payload_urb[index] = req;
	}

	return 0;

      failure:
	fvdc_free_urbs();
	return ret;
}

/* init/exit - module insertion/removal */

static void __init fvdc_prepare_descriptors(void)
{
	int width;
	int height;
	int size;
	BUGME_ON(!fvdc_dev);

	width = fvdc_dev->width;
	height = fvdc_dev->height;
	size = width * height;

	fvdc_default_controls.dwMaxVideoFrameSize =
	    __cpu_to_le32(size * VC_MAX_FORMAT_BPP / 8);

	yuyv_frame_desc.wWidth = __cpu_to_le16(width);
	yuyv_frame_desc.wHeight = __cpu_to_le16(height);
	yuyv_frame_desc.dwMinBitRate =
	    __cpu_to_le32(size * VC_YUYV_FORMAT_BPP / 100);
	yuyv_frame_desc.dwMaxBitRate =
	    __cpu_to_le32(size * VC_YUYV_FORMAT_BPP * 100);
	yuyv_frame_desc.dwMaxVideoFrameBufferSize =
	    __cpu_to_le32(size * VC_YUYV_FORMAT_BPP / 8);

	rgbp_frame_desc.wWidth = __cpu_to_le16(width);
	rgbp_frame_desc.wHeight = __cpu_to_le16(height);
	rgbp_frame_desc.dwMinBitRate =
	    __cpu_to_le32(size * VC_RGBP_FORMAT_BPP / 100);
	rgbp_frame_desc.dwMaxBitRate =
	    __cpu_to_le32(size * VC_RGBP_FORMAT_BPP * 100);
#ifndef AVH_USE_RGBP_RLE
	rgbp_frame_desc.dwMaxVideoFrameBufferSize =
	    __cpu_to_le32(size * VC_RGBP_FORMAT_BPP / 8);
#else
	/* this dwBytesPerLine only exists in the frame based descriptor variant of RGBP */
	rgbp_frame_desc.dwBytesPerLine =
	    __cpu_to_le32(width * VC_RGBP_FORMAT_BPP / 8);

	rgbp_rle_frame_desc.wWidth = __cpu_to_le16(width);
	rgbp_rle_frame_desc.wHeight = __cpu_to_le16(height);
	rgbp_rle_frame_desc.dwMinBitRate =
	    __cpu_to_le32(size * VC_RGBP_RLE_FORMAT_BPP / 100);
	rgbp_rle_frame_desc.dwMaxBitRate =
	    __cpu_to_le32(size * VC_RGBP_RLE_FORMAT_BPP * 100);
	rgbp_rle_frame_desc.dwBytesPerLine = __cpu_to_le32(0);
#endif
}

/* sysfs support */

static ssize_t fvdc_read_resolution(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "%dx%d\n", fvdc_dev->width, fvdc_dev->height);
}

static ssize_t fvdc_read_format(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	int ret = 0;
	switch (fvdc_probe_controls.bFormatIndex) {
	case VC_YUYV_FORMAT_INDEX:
		ret = sprintf(buf, "YUYV\n");
		break;
	case VC_RGBP_FORMAT_INDEX:
		ret = sprintf(buf, "RGBP\n");
		break;
	case VC_RGBP_RLE_FORMAT_INDEX:
		ret = sprintf(buf, "RGBP_RLE\n");
		break;
	default:
		ret = sprintf(buf, "UNKNOWN\n");
		break;
	}
	return ret;
}

static ssize_t fvdc_read_state(struct device *dev,
				 struct device_attribute *attr, char *buf)
{

	int ret = 0;
	switch (fvdc_dev->state) {
	case FVDC_STATE_VOID:
		ret = sprintf(buf, "VOID\n");
		break;
	case FVDC_STATE_IDLE:
		ret = sprintf(buf, "IDLE\n");
		break;
	case FVDC_STATE_CONFIGURED:
		ret = sprintf(buf, "CONFIGURED\n");
		break;
	case FVDC_STATE_SELECTED:
		ret = sprintf(buf, "SELECTED\n");
		break;
	case FVDC_STATE_STREAMABLE:
		ret = sprintf(buf, "STREAMABLE\n");
		break;
	case FVDC_STATE_STREAMING:
		ret = sprintf(buf, "STREAMING\n");
		break;
	default:
		ret = sprintf(buf, "UNKNOWN\n");
		break;
	}

	return ret;
}

static ssize_t fvdc_read_bpp(struct device *dev,
			       struct device_attribute *attr, char *buf)
{
	if (fvdc_dev->attached_fb == NULL)
		return 0;
	else
		return sprintf(buf, "%d\n",
			       fvdc_dev->attached_fb->var.bits_per_pixel);
}

static DEVICE_ATTR(resolution, S_IRUGO, fvdc_read_resolution, NULL);
static DEVICE_ATTR(format, S_IRUGO, fvdc_read_format, NULL);
static DEVICE_ATTR(bpp, S_IRUGO, fvdc_read_bpp, NULL);
static DEVICE_ATTR(state, S_IRUGO, fvdc_read_state, NULL);

static void fvdc_release(struct device *dev)
{
	info("%s - not implemented\n", __FUNCTION__);
}

struct device fvdc_device = {
	.init_name = "fvideo",
	.release = fvdc_release
};

static int fvdc_init_attr(void)
{
	int res = -ENODEV;

	res = device_register(&fvdc_device);

	/* Sysfs attributes. */
	if (res >= 0) {
		if (device_create_file(&fvdc_device, &dev_attr_resolution) ||
		    device_create_file(&fvdc_device, &dev_attr_format) ||
		    device_create_file(&fvdc_device, &dev_attr_bpp) ||
		    device_create_file(&fvdc_device, &dev_attr_state)) {
			return -ENODEV;
		}
	} else {
		return res;
	}

	return 0;
}

static void fvdc_destroy_attr(void)
{
	device_remove_file(&fvdc_device, &dev_attr_resolution);
	device_remove_file(&fvdc_device, &dev_attr_format);
	device_remove_file(&fvdc_device, &dev_attr_bpp);
	device_remove_file(&fvdc_device, &dev_attr_state);
	device_unregister(&fvdc_device);
}

static int __init fvdc_bind(struct usb_configuration *c,
				       struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	struct usb_ep *ep = 0;
	int status;

	info("bind\n");
	BUGME_ON(fvdc_dev);

	fvdc_dev = kzalloc(sizeof *fvdc_dev, GFP_KERNEL);
	if (!fvdc_dev)
		return -ENOMEM;

	/* Prepare work structure */
	INIT_WORK(&fvdc_dev->stream_frame_work, fvdc_stream_frame_work);
	INIT_WORK(&fvdc_dev->pending_frame_work, fvdc_pending_frame_work);

	/* Create kernel work queue and worker thread */
	fvdc_dev->work_queue = create_workqueue(FVDC_KWORK_NAME);

	if (!fvdc_dev->work_queue)
		return -ESRCH;

	fvdc_set_dev_state(fvdc_dev, FVDC_STATE_IDLE);

	/* local things ready now */

	status = fvdc_attach_framebuffer(fvdc_function_driver.fb_index);

	if (status) {
		err("cannot attach to designated frame buffer device (%d)\n",
		    fvdc_function_driver.fb_index);
		return status;
	}

	fvdc_prepare_descriptors();

	status = usb_interface_id(c, f);
	if (status < 0) {
		err("allocate control interface failed\n");
		goto fail;
	}

	video_control_interface_desc.bInterfaceNumber = status;
	vc_interface_association_desc.bFirstInterface = status;
	info("allocated control interface number %d\n", status);

	status = usb_interface_id(c, f);
	if (status < 0) {
		err("allocate stream interface failed\n");
		goto fail;
	}
	video_stream_interface_desc.bInterfaceNumber = status;
	vc_video_control_interface_desc.baInterfaceNr = status;
	info("allocated stream interface number %d\n", status);

	if (fvdc_strings_dev[STRING_VIDEO_IDX].id == 0) {
		status = usb_string_id(c->cdev);
		if (status < 0) {
			err("allocating video string failed\n");
			goto fail;
		}

		fvdc_strings_dev[STRING_VIDEO_IDX].id = status;
		vc_interface_association_desc.iFunction = status;
	}

	if (fvdc_strings_dev[STRING_VIDEO_CONTROL_IDX].id == 0) {
		status = usb_string_id(c->cdev);
		if (status < 0) {
			err("allocating video control string failed\n");
			goto fail;
		}

		fvdc_strings_dev[STRING_VIDEO_CONTROL_IDX].id = status;
		video_control_interface_desc.iInterface = status;
	}

	if (fvdc_strings_dev[STRING_VIDEO_STREAM_IDX].id == 0) {
		status = usb_string_id(c->cdev);
		if (status < 0) {
			err("allocating video control string failed\n");
			goto fail;
		}

		fvdc_strings_dev[STRING_VIDEO_STREAM_IDX].id = status;
		video_stream_interface_desc.iInterface = status;
	}

	ep = usb_ep_autoconfig(cdev->gadget, &fs_bulk_in_ep_desc);
	if (!ep) {
		err("allocation of bulk-in endpont failed\n");
		goto fail;
	}
	info("bulk-in endpoint ep %s allocated\n", ep->name);

	/* copy the dynamically assigned endpoint address */
	vc_video_stream_interface_desc.bEndpointAddress =
	    fs_bulk_in_ep_desc.bEndpointAddress;

	f->descriptors = fvdc_fs_function;
	fvdc_dev->bulk_in = ep;

	fvdc_dev->probecommit_req = usb_ep_alloc_request(cdev->gadget->ep0, GFP_KERNEL);
	if (!fvdc_dev->probecommit_req) {
		err("allocation of control request failed\n");
		goto fail;
	}
	fvdc_dev->probecommit_req->buf = kmalloc(VC_PROBECOMMIT_REQUEST_LENGTH, GFP_KERNEL);
	if (!fvdc_dev->probecommit_req->buf) {
		err("probe/commit memory allocation failed\n"); 
		goto fail;
	}
	fvdc_dev->probecommit_req->complete = 0;

#ifdef CONFIG_USB_GADGET_DUALSPEED
	if (gadget_is_dualspeed(c->cdev->gadget)) {
		hs_bulk_in_ep_desc.bEndpointAddress =
		    fs_bulk_in_ep_desc.bEndpointAddress;
		f->hs_descriptors = fvdc_hs_function;
	}
#endif

	memcpy(&fvdc_commit_controls, &fvdc_default_controls,
	       sizeof(fvdc_commit_controls));
	memcpy(&fvdc_probe_controls, &fvdc_default_controls,
	       sizeof(fvdc_probe_controls));

	if (fvdc_request_urbs()) {
		err("cannot claim necessary payload buffers\n");
		goto fail;
	}

	ep->driver_data = fvdc_dev;

	fvdc_set_dev_state(fvdc_dev, FVDC_STATE_CONFIGURED);

	fvdc_init_attr();
	return 0;

      fail:
	/* cleanup of allocated resources (memory, descriptors, ...) is done in the function 
	   unbind and config_unbind calls, these are called by the composite framework because 
	   this function bind call fails */

	err("can't bind\n");
	return -ENODEV;
}

/* 
 * Called to release structures/memory/resources still held by the USB function.
 * It should not be called from interrupt context!
 */
static void fvdc_unbind(struct usb_configuration *c, struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	BUGME_ON(in_interrupt());
	BUGME_ON(!fvdc_dev);
	BUGME_ON(fvdc_dev->state < FVDC_STATE_CONFIGURED);

	info("unbinding video function\n");

	fvdc_destroy_attr();

	fvdc_set_dev_state(fvdc_dev, FVDC_STATE_IDLE);

	fvdc_detach_framebuffer();

	flush_workqueue(fvdc_dev->work_queue);
	destroy_workqueue(fvdc_dev->work_queue);

	if (fvdc_dev->bulk_in) {
		err("disabling bulk endpoint\n");
		usb_ep_disable(fvdc_dev->bulk_in);
	}

	fvdc_free_urbs();

	if(fvdc_dev->probecommit_req) {
		if(fvdc_dev->probecommit_req->buf) kfree(fvdc_dev->probecommit_req->buf);

		usb_ep_free_request(cdev->gadget->ep0, fvdc_dev->probecommit_req);

		fvdc_dev->probecommit_req = 0;
	}

	if (fvdc_dev->bulk_in) {
		fvdc_dev->bulk_in->driver_data = 0;
	}

	fvdc_dev->bulk_in = 0;

	fvdc_set_dev_state(fvdc_dev, FVDC_STATE_VOID);

	kfree(fvdc_dev);
	fvdc_dev = 0;
}

/* ------------------------------------------------------------------------- */
/*                         external interface                                */
/* ------------------------------------------------------------------------- */

struct fvdc_function fvdc_function_driver = {
	.func = {
		 .name = FVDC_NAME,
		 .strings = fvdc_strings,
		 .bind = fvdc_bind,
		 .unbind = fvdc_unbind,
		 .setup = fvdc_setup,
		 .disable = fvdc_disable,
		 .set_alt = fvdc_set_alt,
		 .get_alt = fvdc_get_alt,
		 .suspend = fvdc_suspend,
		 .resume = fvdc_resume,
		 },
	.fb_index = 0,
};

#ifdef CONFIG_USB_ANDROID_VDC
module_param_named(fb_index, fvdc_function_driver.fb_index, int, S_IRUGO);
MODULE_PARM_DESC(fb_index, "(video) framebuffer to attach to");

static int fvdc_bind_config(struct usb_configuration *c)
{
	int ret = 0;
	printk(KERN_ERR "%s\n", __FUNCTION__);

	ret = usb_add_function(c, &fvdc_function_driver.func);
	if(ret) {
		info("adding video function failed\n");
	}
	printk(KERN_ERR "%s: returning %d\n", __FUNCTION__, ret);
	return ret;
}

static struct android_usb_function fvdc_function = {
	.name = "fvdc",
	.bind_config = fvdc_bind_config,
};

static int __init fvdc_init(void)
{
	printk(KERN_INFO "fvdc init\n");
	android_register_function(&fvdc_function);
	return 0;
}
late_initcall(fvdc_init);
#endif