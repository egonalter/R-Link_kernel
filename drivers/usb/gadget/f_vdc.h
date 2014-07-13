/* b/drivers/usb/gadget/f_vdc.h
 * 
 * Copyright (c) 2009 TomTom BV <http://www.tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 */
#ifndef __F_VDC_H__
#define __F_VDC_H__

#include <linux/usb.h>
#include <linux/usb/gadget.h>
#include <linux/usb/composite.h>

#undef INFO
#undef DEBUG
#undef ERROR
#undef info
#undef debug
#undef err

struct fvdc_function {
	struct usb_function func;
	int fb_index;
};

struct fb_info;

extern struct fvdc_function fvdc_function_driver;
extern struct fb_info* fvdc_attached_framebuffer(void);

/* the definitions below should be included in include/linux/usb/video.h */

#ifndef __LINUX_USB_VIDEO_EXTENSION_H
#define __LINUX_USB_VIDEO_EXTENSION_H

#define SET_CUR				0x01
#define GET_CUR 			0x81
#define GET_MIN 			0x82
#define GET_MAX 			0x83
#define GET_RES 			0x84
#define GET_LEN 			0x85
#define GET_INFO 			0x86
#define GET_DEF 			0x87

#define CC_VIDEO			0x0E

#define VC_DESCRIPTOR_UNDEFINED		0x00
#define VC_HEADER			0x01
#define VC_INPUT_TERMINAL		0x02
#define VC_OUTPUT_TERMINAL		0x03
#define VC_SELECTOR_UNIT		0x04
#define VC_PROCESSING_UNIT		0x05
#define VC_EXTENSION_UNIT		0x06

#define SC_UNDEFINED			0x00
#define SC_VIDEOCONTROL			0x01
#define SC_VIDEOSTREAMING		0x02
#define SC_VIDEO_INTERFACE_COLLECTION	0x03

#define PC_PROTOCOL_UNDEFINED		0x00

#define TT_VENDOR_SPECIFIC		0x0100
#define TT_STREAMING			0x0101

#define ITT_VENDOR_SPECIFIC		0x0200
#define ITT_CAMERA			0x0201
#define ITT_MEDIA_TRANSPORT_INPUT	0x0202

#define SUP_GET_VAL_REQ			0
#define SUP_SET_VAL_REQ			1
#define DISABLED_DUE_TO_AUTO_MODE	2
#define AUTOUPDATE_CONTROL		3
#define ASYNC_CONTROL			4

#define VC_INTERFACE_DIGIT		0
#define VS_INTERFACE_DIGIT		1
#define VS_PROBE_CONTROL		0x01
#define VS_COMMIT_CONTROL		0x02

#define VS_UNDEFINED			0x00
#define VS_INPUT_HEADER			0x01
#define VS_OUTPUT_HEADER		0x02
#define VS_STILL_IMAGE_FRAME		0x03
#define VS_FORMAT_UNCOMPRESSED		0x04
#define VS_FRAME_UNCOMPRESSED		0x05
#define VS_FORMAT_MJPEG			0x06
#define VS_FRAME_MJPEG			0x07
#define VS_FORMAT_MPEG2TS		0x0A
#define VS_FORMAT_DV			0x0C
#define VS_COLORFORMAT			0x0D
#define VS_FORMAT_FRAME_BASED		0x10
#define VS_FRAME_FRAME_BASED		0x11
#define VS_FORMAT_STREAM_BASED		0x12

#define VC_GUID_LENGTH			16

struct usb_cs_vc_interface_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubType;
	__le16 bcdUVC;
	__le16 wTotalLength;
	__le32 dwClockFrequency;
	__u8 bInCollection;
	__u8 baInterfaceNr;
} __attribute__ ((packed));

struct usb_input_terminal_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubType;
	__u8 bTerminalID;
	__le16 wTerminalType;
	__u8 bAssocTerminal;
	__u8 iTerminal;
	__le16 wObjectiveFocalLengthMin;
	__le16 wObjectiveFocalLengthMax;
	__le16 wOcularFocalLength;
	__u8 bControlSize;
	__u8 bmControls0;
	__u8 bmControls1;
} __attribute__ ((packed));

struct usb_vc_output_terminal_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubType;
	__u8 bTerminalID;
	__le16 wTerminalType;
	__u8 bAssocTerminal;
	__u8 bSourceID;
	__u8 iTerminal;
} __attribute__ ((packed));

struct usb_vc_class_specific_interrupt_endpoint_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubType;
	__le16 wMaxTransferSize;
} __attribute__ ((packed));

struct usb_cs_vs_interface_input_header_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubType;
	__u8 bNumFormats;
	__le16 wTotalLength;
	__u8 bEndpointAddress;
	__u8 bmInfo;
	__u8 bTerminalLink;
	__u8 bStillCaptureMethod;
	__u8 bTriggerSupport;
	__u8 bTriggerUsage;
	__u8 bControlSize;
	__u8 bmaControls0;
	__u8 bmaControls1;
	__u8 bmaControls2;
	__u8 bmaControls3;
	__u8 bmaControls4;
	__u8 bmaControls5;
	__u8 bmaControls6;
	__u8 bmaControls7;
} __attribute__ ((packed));

struct usb_uncompressed_format_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bFormatIndex;
	__u8 bNumFrameDescriptors;
	__u8 guidFormat[VC_GUID_LENGTH];
	__u8 bBitsPerPixel;
	__u8 bDefaultFrameIndex;
	__u8 bAspectRatioX;
	__u8 bAspectRatioY;
	__u8 bmInterlaceflags;
	__u8 bCopyProtect;
} __attribute__ ((packed));

struct usb_uncompressed_frame_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubType;
	__u8 bFrameIndex;
	__u8 bmCapabilities;
	__le16 wWidth;
	__le16 wHeight;
	__le32 dwMinBitRate;
	__le32 dwMaxBitRate;
	__le32 dwMaxVideoFrameBufferSize;
	__le32 dwDefaultFrameInterval;
	__u8 bFrameIntervalType;
	__le32 dwMinFrameInterval;
	__le32 dwMaxFrameInterval;
	__le32 dwFrameIntervalStep;
} __attribute__ ((packed));

struct usb_frame_based_format_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bFormatIndex;
	__u8 bNumFrameDescriptors;
	__u8 guidFormat[VC_GUID_LENGTH];
	__u8 bBitsPerPixel;
	__u8 bDefaultFrameIndex;
	__u8 bAspectRatioX;
	__u8 bAspectRatioY;
	__u8 bmInterlaceflags;
	__u8 bCopyProtect;
	__u8 bVariableSize;
} __attribute__ ((packed));

struct usb_frame_based_frame_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubType;
	__u8 bFrameIndex;
	__u8 bmCapabilities;
	__le16 wWidth;
	__le16 wHeight;
	__le32 dwMinBitRate;
	__le32 dwMaxBitRate;
	__le32 dwDefaultFrameInterval;
	__u8 bFrameIntervalType;
	__le32 dwBytesPerLine;
	__le32 dwMinFrameInterval;
	__le32 dwMaxFrameInterval;
	__le32 dwFrameIntervalStep;
} __attribute__ ((packed));

struct usb_mjpeg_format_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bFormatIndex;
	__u8 bNumFrameDescriptors;
	__u8 bmFlags;
	__u8 bDefaultFrameIndex;
	__u8 bAspectRatioX;
	__u8 bAspectRatioY;
	__u8 bmInterlaceflags;
	__u8 bCopyProtect;
} __attribute__ ((packed));

struct usb_mjpeg_frame_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubType;
	__u8 bFrameIndex;
	__u8 bmCapabilities;
	__le16 wWidth;
	__le16 wHeight;
	__le32 dwMinBitRate;
	__le32 dwMaxBitRate;
	__le32 dwMaxVideoFrameBufferSize;
	__le32 dwDefaultFrameInterval;
	__u8 bFrameIntervalType;
	__le32 dwMinFrameInterval;
	__le32 dwMaxFrameInterval;
	__le32 dwFrameIntervalStep;
} __attribute__ ((packed));

struct usb_color_matching_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bColorPrimaries;
	__u8 bTransferCharacteristics;
	__u8 bMatrixCoefficients;
} __attribute__ ((packed));

struct usb_video_probe_commit_controls {
	__le16 bmHint;
	__u8 bFormatIndex;
	__u8 bFrameIndex;
	__le32 dwFrameInterval;
	__le16 wKeyFrameRate;
	__le16 wPFrameRate;
	__le16 wCompQuality;
	__le16 wCompWindowSize;
	__le16 wDelay;
	__le32 dwMaxVideoFrameSize;
	__le32 dwMaxPayloadTransferSize;
	__le32 dwClockFrequency;
	__u8 bmFramingInfo;
	__u8 bPreferedVersion;
	__u8 bMinVersion;
	__u8 bMaxVersion;
} __attribute__ ((packed));

struct usb_vc_processing_unit_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubType;
	__u8 bUnitID;
	__u8 bSourceID;
	__le16 wMaxMultiplier;
	__u8 bControlSize;
	__le16 bmControls;
	__u8 iProcessing;
//      __u8 bmVideoStandards;
} __attribute__ ((packed));

struct usb_vc_extension_unit_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubType;
	__u8 bUnitID;
	__u8 guidExtensionCode0;
	__u8 guidExtensionCode1;
	__u8 guidExtensionCode2;
	__u8 guidExtensionCode3;
	__u8 guidExtensionCode4;
	__u8 guidExtensionCode5;
	__u8 guidExtensionCode6;
	__u8 guidExtensionCode7;
	__u8 guidExtensionCode8;
	__u8 guidExtensionCode9;
	__u8 guidExtensionCode10;
	__u8 guidExtensionCode11;
	__u8 guidExtensionCode12;
	__u8 guidExtensionCode13;
	__u8 guidExtensionCode14;
	__u8 guidExtensionCode15;
	__u8 bNumControls;
	__u8 bNrInPins;
	__u8 baSourceID;
	__u8 bControlSize;
	__u8 bmControls0;
	__u8 iExtension;
} __attribute__ ((packed));

struct video_control {
	unsigned long long cur;
	unsigned long long min;
	unsigned long long max;
	unsigned long long res;
	unsigned long long def;
	__u8 len;
	__u8 info;
} __attribute__ ((packed));

struct video_entity {
	__u8 ControlCount;
	struct video_control VideoControls[];
} __attribute__ ((packed));

#endif //__LINUX_USB_VIDEO_EXTENSION_H

#endif //__F_VIDEO_H__
