/* b/drivers/usb/gadget/f_adc.h
 * 
 * Copyright (c) 2009 TomTom BV <http://www.tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 */
#ifndef __F_ADC_H__
#define __F_ADC_H__

#include <linux/usb.h>
#include <linux/usb/gadget.h>
#include <linux/usb/composite.h>

#undef INFO
#undef DEBUG
#undef ERROR
#undef info
#undef debug
#undef err

struct fadc_device_parameters {
	unsigned long int channels;
	unsigned long int samplerate;
};

struct fadc_function {
	struct usb_function func;
	struct fadc_device_parameters playback;
	struct fadc_device_parameters capture;
};

extern struct fadc_function fadc_function_driver;


/* the definitions below should be included in include/linux/usb/audio.h */
#ifndef __LINUX_USB_AUDIO_EXTENSION_H
#define __LINUX_USB_AUDIO_EXTENSION_H

#define AUDIO						0x01

#define INTERFACE_SUBCLASS_UNDEFINED			0x00
#define AUDIOCONTROL					0x01
#define AUDIOSTREAMING					0x02
#define MIDISTREAMING					0x03

#define INTERFACE_PROTOCOL_UNDEFINED			0x00
#define IP_VERSION_02_00				0x20

#define FUNCTION_PROTOCOL_UNDEFINED			0x00
#define AF_VERSION_02_00				IP_VERSION_02_00

#define FUNCTION_SUBCLASS_UNDEFINED			0x00
#define DESKTOP_SPEAKER					0x01
#define HOME_THEATER					0x02
#define MICROPHONE					0x03
#define HEADSET						0x04
#define TELEPHONE					0x05
#define CONVERTER					0x06
#define VOICESOUND_RECORDER				0x07
#define IO_BOX						0x08
#define MUSICAL_INSTRUMENT				0x09
#define PRO_AUDIO					0x0A
#define AUDIOVIDEO					0x0B
#define CONTROL_PANEL					0x0C
#define OTHER						0xFF

#define CS_UNDEFINED					0x20
#define CS_DEVICE					0x21
#define CS_CONFIGURATION				0x22
#define CS_STRING					0x23
#define CS_INTERFACE					0x24
#define CS_ENDPOINT					0x25

#define AC_DESCRIPTOR_UNDEFINED				0x00
#define HEADER						0x01
#define INPUT_TERMINAL					0x02
#define OUTPUT_TERMINAL					0x03
#define MIXER_UNIT					0x04
#define SELECTOR_UNIT					0x05
#define FEATURE_UNIT					0x06
#define EFFECT_UNIT					0x07
#define PROCESSING_UNIT					0x08
#define EXTENSION_UNIT					0x09
#define CLOCK_SOURCE					0x0A
#define CLOCK_SELECTOR					0x0B
#define CLOCK_MULTIPLIER				0x0C
#define SAMPLE_RATE_CONVERTER				0x0D

#define AS_DESCRIPTOR_UNDEFINED				0x00
#define AS_GENERAL					0x01
#define FORMAT_TYPE					0x02
#define ENCODER						0x03
#define DECODER						0x04

#define EFFECT_UNDEFINED				0x00
#define PARAM_EQ_SECTION_EFFECT				0x01
#define REVERBERATION_EFFECT				0x02
#define MOD_DELAY_EFFECT				0x03
#define DYN_RANGE_COMP_EFFECT				0x04

#define PROCESS_UNDEFINED				0x00
#define UPDOWNMIX_PROCESS				0x01
#define DOLBY_PROLOGIC_PROCESS				0x02
#define STEREO_EXTENDER_PROCESS				0x03

#define DESCRIPTOR_UNDEFINED				0x00
#define EP_GENERAL					0x01

#define REQUEST_CODE_UNDEFINED				0x00
#define CUR						0x01
#define RANGE						0x02
#define MEM						0x03

#define ENCODER_UNDEFINED				0x00
#define OTHER_ENCODER					0x01
#define MPEG_ENCODER					0x02
#define AC3_ENCODER					0x03
#define WMA_ENCODER					0x04
#define DTS_ENCODER					0x05

#define DECODER_UNDEFINED				0x00
#define OTHER_DECODER					0x01
#define MPEG_DECODER					0x02
#define AC3_DECODER					0x03
#define WMA_DECODER					0x04
#define DTS_DECODER					0x05

#define CS_CONTROL_UNDEFINED				0x00
#define CS_SAM_FREQ_CONTROL				0x01
#define CS_CLOCK_VALID_CONTROL				0x02

#define CX_CONTROL_UNDEFINED				0x00
#define CX_CLOCK_SELECTOR_CONTROL			0x01

#define CM_CONTROL_UNDEFINED				0x00
#define CM_NUMERATOR_CONTROL				0x01
#define CM_DENOMINATOR_CONTROL				0x02

#define TE_CONTROL_UNDEFINED				0x00
#define TE_COPY_PROTECT_CONTROL				0x01
#define TE_CONNECTOR_CONTROL				0x02
#define TE_OVERLOAD_CONTROL				0x03
#define TE_CLUSTER_CONTROL				0x04
#define TE_UNDERFLOW_CONTROL				0x05
#define TE_OVERFLOW_CONTROL				0x06
#define TE_LATENCY_CONTROL				0x07

#define MU_CONTROL_UNDEFINED				0x00
#define MU_MIXER_CONTROL				0x01
#define MU_CLUSTER_CONTROL				0x02
#define MU_UNDERFLOW_CONTROL				0x03
#define MU_OVERFLOW_CONTROL				0x04
#define MU_LATENCY_CONTROL				0x05

#define SU_CONTROL_UNDEFINED				0x00
#define SU_SELECTOR_CONTROL				0x01
#define SU_LATENCY_CONTROL				0x02

#define FU_CONTROL_UNDEFINED				0x00
#define FU_MUTE_CONTROL					0x01
#define FU_VOLUME_CONTROL				0x02
#define FU_BASS_CONTROL					0x03
#define FU_MID_CONTROL					0x04
#define FU_TREBLE_CONTROL				0x05
#define FU_GRAPHIC_EQUALIZER_CONTROL			0x06
#define FU_AUTOMATIC_GAIN_CONTROL			0x07
#define FU_DELAY_CONTROL				0x08
#define FU_BASS_BOOST_CONTROL				0x09
#define FU_LOUDNESS_CONTROL				0x0A
#define FU_INPUT_GAIN_CONTROL				0x0B
#define FU_INPUT_GAIN_PAD_CONTROL			0x0C
#define FU_PHASE_INVERTER_CONTROL			0x0D
#define FU_UNDERFLOW_CONTROL				0x0E
#define FU_OVERFLOW_CONTROL				0x0F
#define FU_LATENCY_CONTROL				0x10

#define PE_CONTROL_UNDEFINED				0x00
#define PE_ENABLE_CONTROL				0x01
#define PE_CENTERFREQ_CONTROL				0x02
#define PE_QFACTOR_CONTROL				0x03
#define PE_GAIN_CONTROL					0x04
#define PE_UNDERFLOW_CONTROL				0x05
#define PE_OVERFLOW_CONTROL				0x06
#define PE_LATENCY_CONTROL				0x07

#define RV_CONTROL_UNDEFINED				0x00
#define RV_ENABLE_CONTROL				0x01
#define RV_TYPE_CONTROL					0x02
#define RV_LEVEL_CONTROL				0x03
#define RV_TIME_CONTROL					0x04
#define RV_FEEDBACK_CONTROL				0x05
#define RV_PREDELAY_CONTROL				0x06
#define RV_DENSITY_CONTROL				0x07
#define RV_HIFREQ_ROLLOFF_CONTROLL			0x08
#define RV_UNDERFLOW_CONTROL				0x09
#define RV_OVERFLOW_CONTROL				0x0A
#define RV_LATENCY_CONTROL				0x0B

#define MD_CONTROL_UNDEFINED				0x00
#define MD_ENABLE_CONTROL				0x01
#define MD_BALANCE_CONTROL				0x02
#define MD_RATE_CONTROL					0x03
#define MD_DEPTH_CONTROL				0x04
#define MD_TIME_CONTROL					0x05
#define MD_FEEDBACK_CONTROL				0x06
#define MD_UNDERFLOW_CONTROL				0x07
#define MD_OVERFLOW_CONTROL				0x08
#define MD_LATENCY_CONTROL				0x09

#define DR_CONTROL_UNDEFINED				0x00
#define DR_ENABLE_CONTROL				0x01
#define DR_COMPRESSION_RATE_CONTROL			0x02
#define DR_MAXAMPL_CONTROL				0x03
#define DR_THRESHOLD_CONTROL				0x04
#define DR_ATTACK_TIME_CONTROL				0x05
#define DR_RELEASE_TIME_CONTROL				0x06
#define DR_UNDERFLOW_CONTROL				0x07
#define DR_OVERFLOW_CONTROL				0x08
#define DR_LATENCY_CONTROL				0x09

#define UD_CONTROL_UNDEFINED				0x00
#define UD_ENABLE_CONTROL				0x01
#define UD_MODE_SELECT_CONTROL				0x02
#define UD_CLUSTER_CONTROL				0x03
#define UD_UNDERFLOW_CONTROL				0x04
#define UD_OVERFLOW_CONTROL				0x05
#define UD_LATENCY_CONTROL				0x06

#define DP_CONTROL_UNDEFINED				0x00
#define DP_ENABLE_CONTROL				0x01
#define DP_MODE_SELECT_CONTROL				0x02
#define DP_CLUSTER_CONTROL				0x03
#define DP_UNDERFLOW_CONTROL				0x04
#define DP_OVERFLOW_CONTROL				0x05
#define DP_LATENCY_CONTROL				0x06

#define ST_EXT_CONTROL_UNDEFINED			0x00
#define ST_EXT_ENABLE_CONTROL				0x01
#define ST_EXT_WIDTH_CONTROL				0x02
#define ST_EXT_UNDERFLOW_CONTROL			0x03
#define ST_EXT_OVERFLOW_CONTROL				0x04
#define ST_EXT_LATENCY_CONTROL				0x05

#define XU_CONTROL_UNDEFINED				0x00
#define XU_ENABLE_CONTROL				0x01
#define XU_CLUSTER_CONTROL				0x02
#define XU_UNDERFLOW_CONTROL				0x03
#define XU_OVERFLOW_CONTROL				0x04
#define XU_LATENCY_CONTROL				0x05

#define AS_CONTROL_UNDEFINED				0x00
#define AS_ACT_ALT_SETTING_CONTROL			0x01
#define AS_VAL_ALT_SETTINGS_CONTROL			0x02
#define AS_AUDIO_DATA_FORMAT_CONTROL			0x03

#define EN_CONTROL_UNDEFINED				0x00
#define EN_BIT_RATE_CONTROL				0x01
#define EN_QUALITY_CONTROL				0x02
#define EN_VBR_CONTROL					0x03
#define EN_TYPE_CONTROL					0x04
#define EN_UNDERFLOW_CONTROL				0x05
#define EN_OVERFLOW_CONTROL				0x06
#define EN_ENCODER_ERROR_CONTROL			0x07
#define EN_PARAM1_CONTROL				0x08
#define EN_PARAM2_CONTROL				0x09
#define EN_PARAM3_CONTROL				0x0A
#define EN_PARAM4_CONTROL				0x0B
#define EN_PARAM5_CONTROL				0x0C
#define EN_PARAM6_CONTROL				0x0D
#define EN_PARAM7_CONTROL				0x0E
#define EN_PARAM8_CONTROL				0x0F

#define MD_CONTROL_UNDEFINED				0x00
#define MD_DUAL_CHANNEL_CONTROL				0x01
#define MD_SECOND_STEREO_CONTROL			0x02
#define MD_MULTILANGUAL_CONTROL				0x03
#define MD_DYN_RANGE_CONTROL				0x04
#define MD_SCALING_CONTROL				0x05
#define MD_HILO_SCALING_CONTROL				0x06
#define MD_UNDERFLOW_CONTROL				0x07
#define MD_OVERFLOW_CONTROL				0x08
#define MD_DECODER_ERROR_CONTROL			0x09

#define AD_CONTROL_UNDEFINED				0x00
#define AD_MODE_CONTROL					0x01
#define AD_DYN_RANGE_CONTROL				0x02
#define AD_SCALING_CONTROL				0x03
#define AD_HILO_SCALING_CONTROL				0x04
#define AD_UNDERFLOW_CONTROL				0x05
#define AD_OVERFLOW_CONTROL				0x06
#define AD_DECODER_ERROR_CONTROL			0x07

#define WD_CONTROL_UNDEFINED				0x00
#define WD_UNDERFLOW_CONTROL				0x01
#define WD_OVERFLOW_CONTROL				0x02
#define WD_DECODER_ERROR_CONTROL			0x03

#define DD_CONTROL_UNDEFINED				0x00
#define DD_UNDERFLOW_CONTROL				0x01
#define DD_OVERFLOW_CONTROL				0x02
#define DD_DECODER_ERROR_CONTROL			0x03

#define EP_CONTROL_UNDEFINED				0x00
#define EP_PITCH_CONTROL				0x01
#define EP_DATA_OVERRUN_CONTROL				0x02
#define EP_DATA_UNDERRUN_CONTROL			0x03

#define FORMAT_TYPE_UNDEFINED				0x00
#define FORMAT_TYPE_I					0x01
#define FORMAT_TYPE_II					0x02
#define FORMAT_TYPE_III					0x03
#define FORMAT_TYPE_IV					0x04
#define EXT_FORMAT_TYPE_I				0x81
#define EXT_FORMAT_TYPE_II				0x82
#define EXT_FORMAT_TYPE_III				0x83

struct audio_channel_cluster_descriptor {
	__u8 bNrChannels;
	__u8 bmChannelConfig0;
	__u8 bmChannelConfig1;
	__u8 bmChannelConfig2;
	__u8 bmChannelConfig3;
	__u8 iChannelNames;
} __attribute__ ((packed));

struct dolby_prologic_cluster_descriptor {
	__u8 bNrChannels;
	__u8 bmChannelConfig0;
	__u8 bmChannelConfig1;
	__u8 bmChannelConfig2;
	__u8 bmChannelConfig3;
	__u8 iChannelNames;
} __attribute__ ((packed));

struct standard_ac_interface_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bInterfaceNumer;
	__u8 bAlternateSetting;
	__u8 bNumEndpoints;
	__u8 bInterfaceClass;
	__u8 bInterfaceSubClass;
	__u8 bInterfaceProtocol;
	__u8 iInterface;
} __attribute__ ((packed));

struct cs_ac_interface_descriptor_10 {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u16 bcdADC;
	__u16 wTotalLength;
	__u8 bInCollection;
	__u8 baInterfaceNr;
} __attribute__ ((packed));

struct cs_ac_interface_descriptor_20 {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u16 bcdADC;
	__u8 bCategory;
	__u16 wTotalLength;
	__u8 bmControls;
} __attribute__ ((packed));

struct clock_source_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bClockID;
	__u8 bmAttributes;
	__u8 bmControls;
	__u8 bAssocTerminal;
	__u8 iClockSource;
} __attribute__ ((packed));

struct clock_selector_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bClockID;
	__u8 bNrInPins;
	__u8 baCSourceID1; /* Add baCSourceID(p) */
	__u8 bmControls;
	__u8 iClockSelector;
} __attribute__ ((packed));

struct clock_multiplier_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bClockID;
	__u8 bCSourceID;
	__u8 bmControls;
	__u8 iClockMultiplier;
} __attribute__ ((packed));

struct input_terminal_descriptor_10 {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bTerminalID;
	__u16 wTerminalType;
	__u8 bAssocTerminal;
	__u8 bNrChannels;
	__u16 wChannelConfig;
	__u8 iChannelNames;
	__u8 iTerminal;
} __attribute__ ((packed));

struct input_terminal_descriptor_20 {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bTerminalID;
	__u16 wTerminalType;
	__u8 bAssocTerminal;
	__u8 bCSourceID;
	__u8 bNrChannels;
	__u8 bmChannelConfig0;
	__u8 bmChannelConfig1;
	__u8 bmChannelConfig2;
	__u8 bmChannelConfig3;
	__u8 iChannelNames;
	__u16 bmControls;
	__u8 iTerminal;
} __attribute__ ((packed));

struct output_terminal_descriptor_10 {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bTerminalID;
	__u16 wTerminalType;
	__u8 bAssocTerminal;
	__u8 bSourceID;
	__u8 iTerminal;
} __attribute__ ((packed));

struct output_terminal_descriptor_20 {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bTerminalID;
	__u16 wTerminalType;
	__u8 bAssocTerminal;
	__u8 bSourceID;
	__u8 bCSourceID;
	__u16 bmControls;
	__u8 iTerminal;
} __attribute__ ((packed));

struct mixer_unit_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bUnitID;
	__u8 bNrInPins;
	__u8 baSourceID1;	/* Add baSourceID(p) */
	__u8 bNrChannels;
	__u8 bmChannelConfig0;
	__u8 bmChannelConfig1;
	__u8 bmChannelConfig2;
	__u8 bmChannelConfig3;
	__u8 iChannelNames;
	/* Add bmMixerControls */
	__u8 bmControls;
	__u8 iMixer;
} __attribute__ ((packed));

struct selector_unit_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bUnitID;
	__u8 bNrInPins;
	__u8 baSourceID1;	/* Add baSourceID(p) */
	__u8 bmControls;
	__u8 iSelector;
} __attribute__ ((packed));

struct feature_unit_descriptor_10 {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bUnitID;
	__u8 bSourceID;
	__u8 bControlSize;
	__u8 bmaControls0;
	__u8 iFeature;
} __attribute__ ((packed));

struct feature_unit_descriptor_20 {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bUnitID;
	__u8 bSourceID;
	__u8 bmaControls0;	/* Add bmaControls(1) bmaControls(ch) */
	__u8 iFeature;
} __attribute__ ((packed));

struct sampling_rate_converter_unit_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bUnitID;
	__u8 bSourceID;
	__u8 bCSourceInID;
	__u8 bCSourceOutID;
	__u8 iSRC;
} __attribute__ ((packed));

struct extension_unit_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bUnitID;
	__u16 wExtensionCode;
	__u8 bNrInPins;
	__u8 baSourceID1;	/* Add baSourceID(p) */
	__u8 bNrChannels;
	__u8 bmChannelConfig0;
	__u8 bmChannelConfig1;
	__u8 bmChannelConfig2;
	__u8 bmChannelConfig3;
	__u8 iChannelNames;
	__u8 bmControls;
	__u8 iExtension;
} __attribute__ ((packed));

struct standard_ac_interrupt_endpoint_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bEndpointAddress;
	__u8 bmAttributes;
	__u16 wMaxPacketSize;
	__u8 bInterval;
} __attribute__ ((packed));

struct standard_as_interface_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bInterfaceNumber;
	__u8 bAlternateSetting;
	__u8 bNumEndpoints;
	__u8 bInterfaceClass;
	__u8 bInterfaceSubClass;
	__u8 bInterfaceProtocol;
	__u8 iInterface;
} __attribute__ ((packed));

struct cs_as_interface_descriptor_10 {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bTerminalLink;
	__u8 bDelay;
	__u16 wFormatTag;
} __attribute__ ((packed));

struct cs_as_interface_descriptor_20 {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bTerminalLink;
	__u8 bmControls;
	__u8 bFormatType;
	__u8 bmFormats0;
	__u8 bmFormats1;
	__u8 bmFormats2;
	__u8 bmFormats3;
	__u8 bNrChannels;
	__u8 bmChannelConfig0;
	__u8 bmChannelConfig1;
	__u8 bmChannelConfig2;
	__u8 bmChannelConfig3;
	__u8 iChannelNames;
} __attribute__ ((packed));

struct encoder_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bEncoderID;
	__u8 bEncoder;
	__u8 bmControls0;
	__u8 bmControls1;
	__u8 bmControls2;
	__u8 bmControls3;
	__u8 iParam1;
	__u8 iParam2;
	__u8 iParam3;
	__u8 iParam4;
	__u8 iParam5;
	__u8 iParam6;
	__u8 iParam7;
	__u8 iParam8;
	__u8 iEncoder;
} __attribute__ ((packed));

struct mpeg_decoder_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bDecoderID;
	__u8 bDecoder;
	__u16 bmMPEGCapabilities;
	__u8 bmMPEGFeatures;
	__u8 bmControls;
	__u8 iDecoder;
} __attribute__ ((packed));

struct ac3_dcoder_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bDecoderID;
	__u8 bDecoder;
	__u8 bmBSID0;
	__u8 bmBSID1;
	__u8 bmBSID2;
	__u8 bmBSID3;
	__u8 bmAC3Features;
	__u8 bmControls;
	__u8 iDecoder;
} __attribute__ ((packed));

struct wma_decoder_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bDecoderID;
	__u8 bDecoder;
	__u16 bmWMAProfile;
	__u8 bmControls;
	__u8 iDecoder;
} __attribute__  ((packed));

struct dts_decoder_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bDecoderID;
	__u8 bDecoder;
	__u8 bmCapabilities;
	__u8 bmControls;
	__u8 iDecoder;
} __attribute__ ((packed));

struct standard_as_isochronous_audio_data_endpoint_descriptor_10 {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bEndpointAddress;
	__u8 bmAttributes;
	__u16 wMaxPacketSize;
	__u8 bInterval;
	__u8 bRefresh;
	__u8 bSyncAddress;
} __attribute__ ((packed));

struct cs_as_isochronous_audio_data_endpoint_descriptor_10 {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bmAttributes;
	__u8 bLockDelayUnits;
	__u16 wLockDelay;
} __attribute__ ((packed));

struct cs_as_isochronous_audio_data_endpoint_descriptor_20 {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bmAttributes;
	__u8 bmControls;
	__u8 bLockDelayUnits;
	__u16 wLockDelay;
} __attribute__ ((packed));

struct standard_as_isochronous_feedback_endpoint_descriptor {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bEndpointAddress;
	__u8 bmAttributes;
	__u16 wMaxPacketSize;
	__u8 bInterval;
} __attribute__ ((packed));

struct format_type_descriptor_cont {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bFormatType;
	__u8 bNrChannels;
	__u8 bSubFrameSize;
	__u8 bBitResolution;
	__u8 bSamFreqType;
	__u8 tLowerSamFreq00;
	__u8 tLowerSamFreq01;
	__u8 tLowerSamFreq02;
	__u8 tUpperSamFreq00;
	__u8 tUpperSamFreq01;
	__u8 tUpperSamFreq02;
} __attribute__ ((packed));

struct format_type_descriptor_disc {
	__u8 bLength;
	__u8 bDescriptorType;
	__u8 bDescriptorSubtype;
	__u8 bFormatType;
	__u8 bNrChannels;
	__u8 bSubFrameSize;
	__u8 bBitResolution;
	__u8 bSamFreqType;
	__u8 tSamFreq00;
	__u8 tSamFreq01;
	__u8 tSamFreq02;
	__u8 tSamFreq10;
	__u8 tSamFreq11;
	__u8 tSamFreq12;
	__u8 tSamFreq20;
	__u8 tSamFreq21;
	__u8 tSamFreq22;
	__u8 tSamFreq30;
	__u8 tSamFreq31;
	__u8 tSamFreq32;
} __attribute__ ((packed));

#endif	// __LINUX_USB_AUDIO_EXTENSION_H

#endif	//__F_AUDIO_H__
