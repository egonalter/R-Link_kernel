/* b/drivers/usb/gadget/f_adc.c
 *
 * Copyright (c) 2009 TomTom BV <http://www.tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
 */

#include <linux/err.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/timer.h>
#include <linux/usb/audio.h>
#include <linux/usb/composite.h>
#include <sound/core.h>
#include <sound/pcm.h>

#ifdef CONFIG_USB_ANDROID_VDC
#include <linux/usb/android_composite.h>
#endif

#include <sound/soc.h>
#include "gadget_chips.h"
#include "f_adc.h"

#define DMA_ADDR_INVALID (~(dma_addr_t)0)

#undef DEBUG
#undef INFO
#define ERROR

/* define BUGME to enable run-time invariant checks */
#define BUGME

#define PFX				"usb-fadc: "

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

/* BINTERVAL:

	The interval between packets is given by:

	2 ^ (bInterval - 1) * time-unit

	Where time-unit is 1 millisecond on a full speed bus, and 1/8
	millisecond on  a high speed bus.
				bInterval value
	time interval	high speed	full speed
	------------------------------------------
	1 millisec	4		1
	2 millisec	5		2
	4 millisec	6		3
	8 millisec	7		4

	For 44KHz stereo 16 bit PCM sound, 4 millisecond is the
	longest period that will 'fit', otherwise the packets
	need to be larger than the allowed maximum.
*/

#define FADC_DEFAULT_CHANNELS_PLAYBACK		2
#define FADC_DEFAULT_SAMPLERATE_PLAYBACK		44100
#define FADC_DEFAULT_FULL_SPEED_BINTERVAL_PLAYBACK	1
#define FADC_DEFAULT_HIGH_SPEED_BINTERVAL_PLAYBACK	\
	(FADC_DEFAULT_FULL_SPEED_BINTERVAL_PLAYBACK + 3)
#define FADC_DEFAULT_INTERVAL_PLAYBACK		\
	( 1 << (FADC_DEFAULT_FULL_SPEED_BINTERVAL_PLAYBACK - 1))
#define FADC_DEFAULT_CHANNELS_CAPTURE			1

#define FADC_DEFAULT_SAMPLERATE_CAPTURE		16000
#define FADC_DEFAULT_FULL_SPEED_BINTERVAL_CAPTURE	1
#define FADC_DEFAULT_HIGH_SPEED_BINTERVAL_CAPTURE	\
	(FADC_DEFAULT_FULL_SPEED_BINTERVAL_CAPTURE + 3)
#define FADC_DEFAULT_INTERVAL_CAPTURE			\
	( 1 << (FADC_DEFAULT_FULL_SPEED_BINTERVAL_CAPTURE - 1))

#define FADC_MAX_ISO_PACKET_SIZE			1023

/* The USB packet transfers need to support the ALSA data rate as close as
   possible. ALSA produces data in 'periods', with a data rate that equals:

	channels * sample_width * sample_rate

  For the to-host device, this amount of data needs to be transported in a given
  number of packet transactions. The size of these packets may vary (within the
  limits imposed by the endpoint), but the host will ensure enough timeslots to
  transfer the data, and will poll the isochronous endpoint at regular
  intervals.

  On a full speed bus, the amount of isochronous packets that may be send is
  limited to at most 1 per frame (1ms), on a high speed bus it is limited to at
  most 3 per microframe.

  If the maximum amount of transactions is used for a full speed bus (1000), the
  same amount of transactions can be used on a high speed bus. The maximum
  packet sizes do not really matter, (1023 vs. 1024).

  As each isochronous packet should contain an integral number of 'audio frames'
  (= samples for all channels), it is unlikely that the amount of data to be
  transferred in these 1000 transactions corresponds to an exact integral number
  of packets.

  To accommodate the excess audio frames that are build up over a series of
  isochronous packets, an alignment packet will be send after a certain amount
  of nominal packets.

  A precomputed table is used to determine: the size of a nominal packet, the
  size of an alignment packet, the number of packets in an alignment sequence.
  The last packet of an alignment sequence is an alignment packet.

  The table lists the settings for a mono-source. The actual nominal and
  alignment packet size need to be multiplied by the number of channels. It is
  important that the following always holds:

  channels * alignment_size <= FADC_MAX_ISO_PACKET_SIZE

  That will ensure that the packet never exceeds the full speed limit of 1023
  bytes. The high speed limit is 1024 bytes, but it does not make sense to make
  use of that difference.

  For the capture device, the data comes in as packets whose length is
  determined by the host.
 */

static struct fadc_packet_setting {
	unsigned int channels;
	unsigned int samplerate;
	unsigned int alsa_rate;
	unsigned int nominal_size;
	unsigned int alignment_size;
	unsigned int align_length;
} fadc_packet_settings[] = {
	{ 1, 8000, SNDRV_PCM_RATE_8000, 16, 16, 0},
	{ 1, 11025, SNDRV_PCM_RATE_11025, 22, 24, 40},
	{ 1, 16000, SNDRV_PCM_RATE_16000, 32, 32, 0},
	{ 1, 22050, SNDRV_PCM_RATE_22050, 44, 46, 20},
	{ 1, 32000, SNDRV_PCM_RATE_32000, 64, 64, 0},
	{ 1, 44100, SNDRV_PCM_RATE_44100, 88, 90, 10},
	{ 1, 48000, SNDRV_PCM_RATE_48000, 96, 96, 0}, 
	{ 0},
};

struct fadc_volume_setting {
	long int gain;
	unsigned long int shift;
	long int db;
};

#define FADC_IDENT_GAIN				32768
#define FADC_IDENT_SHIFT			15
#define FADC_TIMER_TICKS			(HZ / 10)
#define FADC_ALSA_DRIVER_NAME			"snd_usb_audio"
#define FADC_HELPBUF_SIZE			1024
#define FADC_PLAYBACK_RINGBUFFER_SIZE		(48*1024)
#define FADC_CAPTURE_RINGBUFFER_SIZE		(16*1024)
#define FADC_PREALLOC_SIZE			\
	(FADC_PLAYBACK_RINGBUFFER_SIZE + FADC_CAPTURE_RINGBUFFER_SIZE)

enum fadc_dev_state {
	FADC_STATE_VOID,
	FADC_STATE_IDLE,
	FADC_STATE_CONFIGURED,
	FADC_STATE_SELECTED,
	FADC_STATE_PREPARED,	// received prepare trigger from ALSA
	FADC_STATE_READY,	// configured && prepared
	FADC_STATE_STREAMING,	// ready && start trigger && no stop trigger yet
	FADC_STATE_VIRTUAL,	// prepared && start trigger (no USB connection)
};

struct fadc_dev_pcm {
	unsigned int ring_size;
	unsigned int period_size;
	unsigned int period_pos;
	unsigned int ring_pos;
};

static struct snd_card *fadc_card = 0;

static struct fadc_dev {
	char *name;
	int alt;
	struct fadc_dev_pcm pcm;
	struct snd_pcm_substream *substream;
	enum fadc_dev_state state;

	struct usb_request *request;
	unsigned char *databuf;
	struct usb_ep *iso;
	struct usb_endpoint_descriptor *iso_desc;
	struct usb_endpoint_descriptor *fs_iso_desc;
	struct usb_endpoint_descriptor *hs_iso_desc;

	struct fadc_packet_setting settings;
	struct fadc_volume_setting volume;

	struct timer_list timer;

	unsigned int packet_index;

	wait_queue_head_t queue;
	int active;
} *fadc_playback_dev = 0, *fadc_capture_dev = 0;

static inline void
fadc_set_dev_state(struct fadc_dev *dev, enum fadc_dev_state state)
{
	if (!dev)
		return;

	if (state != dev->state) {
		info("switching %s from %d to %d\n",
		     dev->name, dev->state, state);
		dev->state = state;
	}
}

#define FADC_STRING_IDX		0
#define FADC_PLAYBACK_STRING_IDX	1
#define FADC_CAPTURE_STRING_IDX	2

static char fadc_name[] = "USB Audio";
static char fadc_func_name[] = "faudio";

/* the following names are used in USB descriptors, 'capture' and 'playback'
   should be understood in 'host context' */
static char fadc_playback_name[] = "USB Audio Capture (Device-To-Host)";
static char fadc_capture_name[] = "USB Audio Playback (Host-To-Device)";

/* ########################################################################## */
/* #################### USB specific part of the driver ##################### */
/* ########################################################################## */

#define FADC_ALTERNATE_IDLE	0x00
#define FADC_ALTERNATE_ACTIVE	0x01

static struct usb_string fadc_strings_dev[] = {
	[FADC_STRING_IDX].s = fadc_name,
	[FADC_PLAYBACK_STRING_IDX].s = fadc_playback_name,
	[FADC_CAPTURE_STRING_IDX].s = fadc_capture_name,
	{},
};

static struct usb_gadget_strings fadc_english_strings = {
	.language = 0x0409,
	.strings = fadc_strings_dev,
};

static struct usb_gadget_strings *fadc_strings[] = {
	&fadc_english_strings,
	0,
};

static struct usb_interface_assoc_descriptor ac_intf_assoc_desc = {
	.bLength = sizeof ac_intf_assoc_desc,
	.bDescriptorType = USB_DT_INTERFACE_ASSOCIATION,
	.bFirstInterface = 0x00,
	.bInterfaceCount = 0x03,
	.bFunctionClass = AUDIO,
	.bFunctionSubClass = FUNCTION_SUBCLASS_UNDEFINED,
	.bFunctionProtocol = 0x00,
};

static struct usb_interface_descriptor audio_control_interface_desc = {
	.bLength = sizeof audio_control_interface_desc,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 0x00,
	.bAlternateSetting = FADC_ALTERNATE_IDLE,
	.bNumEndpoints = 0x00,
	.bInterfaceClass = AUDIO,
	.bInterfaceSubClass = AUDIOCONTROL,
	.bInterfaceProtocol = 0,
};

static struct input_terminal_descriptor_10 playback_input_terminal_desc10 = {
	.bLength = sizeof playback_input_terminal_desc10,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = INPUT_TERMINAL,
	.bTerminalID = 0x01,
	.wTerminalType = __constant_cpu_to_le16(0x0201),
	.bAssocTerminal = 0x02,
	.bNrChannels = FADC_DEFAULT_CHANNELS_PLAYBACK,
	.wChannelConfig = 0x0000,
};

static struct output_terminal_descriptor_10 playback_output_terminal_desc10 = {
	.bLength = sizeof playback_output_terminal_desc10,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = OUTPUT_TERMINAL,
	.bTerminalID = 0x02,
	.wTerminalType = __constant_cpu_to_le16(0x0101),
	.bAssocTerminal = 0x01,
	.bSourceID = 0x01,
};

static struct input_terminal_descriptor_10 capture_input_terminal_desc10 = {
	.bLength = sizeof capture_input_terminal_desc10,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = INPUT_TERMINAL,
	.bTerminalID = 0x04,
	.wTerminalType = __constant_cpu_to_le16(0x0101),
	.bAssocTerminal = 0x03,
	.bNrChannels = FADC_DEFAULT_CHANNELS_CAPTURE,
	.wChannelConfig = 0x0000,
};

static struct output_terminal_descriptor_10 capture_output_terminal_desc10 = {
	.bLength = sizeof capture_output_terminal_desc10,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = OUTPUT_TERMINAL,
	.bTerminalID = 0x03,
	.wTerminalType = __constant_cpu_to_le16(0x0301),
	.bAssocTerminal = 0x04,
	.bSourceID = 0x04,
};


//DECLARE_USB_AC_HEADER_DESCRIPTOR(2);
//static struct usb_ac_header_descriptor_2 ac_header_desc2 = {
DECLARE_UAC_AC_HEADER_DESCRIPTOR(2);
static struct uac_ac_header_descriptor_2 ac_header_desc2 = {
	.bLength = sizeof ac_header_desc2,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = HEADER,
	.bcdADC = __constant_cpu_to_le16(0x0100),
	.wTotalLength = __constant_cpu_to_le16(sizeof ac_header_desc2 +
					       sizeof
					       playback_input_terminal_desc10 +
					       sizeof
					       playback_output_terminal_desc10 +
					       sizeof
					       capture_input_terminal_desc10 +
					       sizeof
					       capture_output_terminal_desc10),
	.bInCollection = 0x02,
	.baInterfaceNr[0] = 0x01,
	.baInterfaceNr[1] = 0x02,
};

static struct standard_as_interface_descriptor pb_audio_stream_intf_alt0_desc = {
	.bLength = sizeof pb_audio_stream_intf_alt0_desc,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 0x01,
	.bAlternateSetting = FADC_ALTERNATE_IDLE,
	.bNumEndpoints = 0x00,
	.bInterfaceClass = AUDIO,
	.bInterfaceSubClass = AUDIOSTREAMING,
	.bInterfaceProtocol = 0,
};

static struct standard_as_interface_descriptor pb_audio_stream_intf_alt1_desc = {
	.bLength = sizeof pb_audio_stream_intf_alt1_desc,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 0x01,
	.bAlternateSetting = FADC_ALTERNATE_ACTIVE,
	.bNumEndpoints = 0x01,
	.bInterfaceClass = AUDIO,
	.bInterfaceSubClass = AUDIOSTREAMING,
	.bInterfaceProtocol = 0,
};

static struct cs_as_interface_descriptor_10 pb_cs_audio_stream_intf_desc10 = {
	.bLength = sizeof pb_cs_audio_stream_intf_desc10,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = AS_GENERAL,
	.bTerminalLink = 0x02,
	.bDelay = 0xFF,
	.wFormatTag = __constant_cpu_to_le16(0x0001),	/* PCM */
};

static struct format_type_descriptor_disc pb_format_type_desc = {
/* this struct is defined to hold 4 tSamFreq's, we use it holding just one,
   so do not use 'sizeof' to determine bLength */
	.bLength = 0x0B,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = FORMAT_TYPE,
	.bFormatType = FORMAT_TYPE_I,
	.bSubFrameSize = 0x02,
	.bBitResolution = 0x10,
};

static struct standard_as_isochronous_audio_data_endpoint_descriptor_10
 pb_as_hs_iso_audio_data_ep_desc = {
	.bLength = sizeof pb_as_hs_iso_audio_data_ep_desc,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = USB_DIR_IN,
	.bmAttributes = 0x0D,
	.wMaxPacketSize = __constant_cpu_to_le16(0x400),
	.bInterval = FADC_DEFAULT_HIGH_SPEED_BINTERVAL_PLAYBACK,
	.bRefresh = 0x00,
	.bSyncAddress = 0x00,
};

static struct standard_as_isochronous_audio_data_endpoint_descriptor_10
 pb_as_fs_iso_audio_data_ep_desc = {
	.bLength = sizeof pb_as_fs_iso_audio_data_ep_desc,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = USB_DIR_IN,
	.bmAttributes = 0x0D,
	.wMaxPacketSize = __constant_cpu_to_le16(0x3FF),
	.bInterval = FADC_DEFAULT_FULL_SPEED_BINTERVAL_PLAYBACK,
	.bRefresh = 0x00,
	.bSyncAddress = 0x00,
};

static struct cs_as_isochronous_audio_data_endpoint_descriptor_10
 pb_cs_as_iso_audio_data_ep_desc10 = {
	.bLength = sizeof pb_cs_as_iso_audio_data_ep_desc10,
	.bDescriptorType = USB_DT_CS_ENDPOINT,
	.bDescriptorSubtype = EP_GENERAL,
	.bmAttributes = 0x00,
	.bLockDelayUnits = 0x00,
	.wLockDelay = 0x0000,
};

static struct standard_as_interface_descriptor cp_audio_stream_intf_alt0_desc = {
	.bLength = sizeof cp_audio_stream_intf_alt0_desc,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 0x02,
	.bAlternateSetting = FADC_ALTERNATE_IDLE,
	.bNumEndpoints = 0x00,
	.bInterfaceClass = AUDIO,
	.bInterfaceSubClass = AUDIOSTREAMING,
	.bInterfaceProtocol = 0,
};

static struct standard_as_interface_descriptor
 cp_audio_stream_intf_alt1_desc = {
	.bLength = sizeof cp_audio_stream_intf_alt1_desc,
	.bDescriptorType = USB_DT_INTERFACE,
	.bInterfaceNumber = 0x02,
	.bAlternateSetting = FADC_ALTERNATE_ACTIVE,
	.bNumEndpoints = 0x01,
	.bInterfaceClass = AUDIO,
	.bInterfaceSubClass = AUDIOSTREAMING,
	.bInterfaceProtocol = 0,
};

static struct cs_as_interface_descriptor_10 cp_cs_audio_stream_intf_desc10 = {
	.bLength = sizeof cp_cs_audio_stream_intf_desc10,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = AS_GENERAL,
	.bTerminalLink = 0x04,
	.bDelay = 0xFF,
	.wFormatTag = __constant_cpu_to_le16(0x0001),	/* PCM */
};

static struct format_type_descriptor_disc cp_format_type_desc = {
/* this struct is defined to hold 4 tSamFreq's, we use it holding just one,
   so do not use 'sizeof' to determine bLength */
	.bLength = 0x0B,
	.bDescriptorType = USB_DT_CS_INTERFACE,
	.bDescriptorSubtype = FORMAT_TYPE,
	.bFormatType = FORMAT_TYPE_I,
	.bSubFrameSize = 0x02,
	.bBitResolution = 0x10,
};

/* bInterval:

	For full/high speed isochronous endpoints 'bInterval' determines the
	time interval between to transactions.
	The time is expressed in units of frames (full speed) or microframe 
	(high speed) and according to the formula: 2^(bInterval-1).

	bInterval should be between 1 and 16 */

static struct standard_as_isochronous_audio_data_endpoint_descriptor_10
 cp_as_hs_iso_audio_data_ep_desc = {
	.bLength = sizeof cp_as_hs_iso_audio_data_ep_desc,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = USB_DIR_OUT,
	.bmAttributes = 0x0D,
	.wMaxPacketSize = __constant_cpu_to_le16(0x400),
	.bInterval = FADC_DEFAULT_HIGH_SPEED_BINTERVAL_CAPTURE,
	.bRefresh = 0x00,
	.bSyncAddress = 0x00,
};

static struct standard_as_isochronous_audio_data_endpoint_descriptor_10
 cp_as_fs_iso_audio_data_ep_desc = {
	.bLength = sizeof cp_as_fs_iso_audio_data_ep_desc,
	.bDescriptorType = USB_DT_ENDPOINT,
	.bEndpointAddress = USB_DIR_OUT,
	.bmAttributes = 0x0D,
	.wMaxPacketSize = __constant_cpu_to_le16(0x3FF),
	.bInterval = FADC_DEFAULT_FULL_SPEED_BINTERVAL_CAPTURE,
	.bRefresh = 0x00,
	.bSyncAddress = 0x00,
};

static struct cs_as_isochronous_audio_data_endpoint_descriptor_10
 cp_cs_as_iso_audio_data_ep_desc10 = {
	.bLength = sizeof cp_cs_as_iso_audio_data_ep_desc10,
	.bDescriptorType = USB_DT_CS_ENDPOINT,
	.bDescriptorSubtype = EP_GENERAL,
	.bmAttributes = 0x00,
	.bLockDelayUnits = 0x00,
	.wLockDelay = 0x0000,
};

static struct usb_descriptor_header *fadc_fs_descriptors[] = {
	(struct usb_descriptor_header *)&ac_intf_assoc_desc,
	(struct usb_descriptor_header *)&audio_control_interface_desc,
	(struct usb_descriptor_header *)&ac_header_desc2,
	(struct usb_descriptor_header *)&playback_input_terminal_desc10,
	(struct usb_descriptor_header *)&playback_output_terminal_desc10,
	(struct usb_descriptor_header *)&capture_input_terminal_desc10,
	(struct usb_descriptor_header *)&capture_output_terminal_desc10,
	(struct usb_descriptor_header *)&pb_audio_stream_intf_alt0_desc,
	(struct usb_descriptor_header *)&pb_audio_stream_intf_alt1_desc,
	(struct usb_descriptor_header *)&pb_cs_audio_stream_intf_desc10,
	(struct usb_descriptor_header *)&pb_format_type_desc,
	(struct usb_descriptor_header *)&pb_as_fs_iso_audio_data_ep_desc,
	(struct usb_descriptor_header *)&pb_cs_as_iso_audio_data_ep_desc10,
	(struct usb_descriptor_header *)&cp_audio_stream_intf_alt0_desc,
	(struct usb_descriptor_header *)&cp_audio_stream_intf_alt1_desc,
	(struct usb_descriptor_header *)&cp_cs_audio_stream_intf_desc10,
	(struct usb_descriptor_header *)&cp_format_type_desc,
	(struct usb_descriptor_header *)&cp_as_fs_iso_audio_data_ep_desc,
	(struct usb_descriptor_header *)&cp_cs_as_iso_audio_data_ep_desc10,
	0,
};

#ifdef CONFIG_USB_GADGET_DUALSPEED
static struct usb_descriptor_header *fadc_hs_descriptors[] = {
	(struct usb_descriptor_header *)&ac_intf_assoc_desc,
	(struct usb_descriptor_header *)&audio_control_interface_desc,
	(struct usb_descriptor_header *)&ac_header_desc2,
	(struct usb_descriptor_header *)&playback_input_terminal_desc10,
	(struct usb_descriptor_header *)&playback_output_terminal_desc10,
	(struct usb_descriptor_header *)&capture_input_terminal_desc10,
	(struct usb_descriptor_header *)&capture_output_terminal_desc10,
	(struct usb_descriptor_header *)&pb_audio_stream_intf_alt0_desc,
	(struct usb_descriptor_header *)&pb_audio_stream_intf_alt1_desc,
	(struct usb_descriptor_header *)&pb_cs_audio_stream_intf_desc10,
	(struct usb_descriptor_header *)&pb_format_type_desc,
	(struct usb_descriptor_header *)&pb_as_hs_iso_audio_data_ep_desc,
	(struct usb_descriptor_header *)&pb_cs_as_iso_audio_data_ep_desc10,
	(struct usb_descriptor_header *)&cp_audio_stream_intf_alt0_desc,
	(struct usb_descriptor_header *)&cp_audio_stream_intf_alt1_desc,
	(struct usb_descriptor_header *)&cp_cs_audio_stream_intf_desc10,
	(struct usb_descriptor_header *)&cp_format_type_desc,
	(struct usb_descriptor_header *)&cp_as_hs_iso_audio_data_ep_desc,
	(struct usb_descriptor_header *)&cp_cs_as_iso_audio_data_ep_desc10,
	0,
};
#endif

static void fadc_playback_complete(struct usb_ep *ep, struct usb_request *rq);
static void fadc_capture_complete(struct usb_ep *ep, struct usb_request *rq);
static int fadc_dev_set_alt(struct usb_function *f,
				struct fadc_dev* dev,
				unsigned alt);

/* Alsa mixer controls */

/* note: the volume in this driver uses a generalized scale, from -90 dB
	 (attenuation), 0 dB (no change to PCM data) to 90 dB (gain).

	 playback volume control in ALSA user space is a non-negative value,
	 with higher values signalling a higher attenuation.

	 capture volume control in ALSA user space is a non-negative value,
	 with higher values signalling a higher gain. */

/* key to this table is a db value -127 < db <= 0

   gain = gains[-db % 6)
   shift = 15 + floor(-db/6);

   gain represents the fixed-point multiplier, while
   shift represents the fixed-point scaling factor
*/
long int fadc_attenuate_gains[] = {
	32768,			/*  0dB */
	29205,			/* -1dB */
	26029,			/* -2dB */
	23198,			/* -3dB */
	20675,			/* -4dB */
	18427,			/* -5dB */
};

/* key to this table is a db value 127 <= db <= 0

   gain = gains[db % 6]
   shift = 15 - floor(db/6);
*/
long int fadc_boost_gains[] = {
	32768,			/*  0dB */
	36766,			/*  1dB */
	41252,			/*  2dB */
	46286,			/*  3dB */
	51934,			/*  4dB */
	58271,			/*  5dB */
};

static void fadc_set_volume(struct fadc_volume_setting *volume, int db)
{
	if (!volume)
		return;

	if (db < -90)
		db = -90;
	if (db > 90)
		db = 90;

	if (db < 0) {
		/* attenuate */
		volume->gain = fadc_attenuate_gains[(-db) % 6];
		volume->shift = 15 + (-db / 6);
		volume->db = db;
	} else {
		/* gain */
		volume->gain = fadc_boost_gains[db % 6];
		volume->shift = 15 - (db / 6);
		volume->db = db;
	}
}

static void fadc_boost(struct fadc_volume_setting *boost,
			 short int *samples, unsigned long int count)
{
	long int gain;
	unsigned long int shift;

	BUGME_ON(!boost);

	gain = boost->gain;
	shift = boost->shift;

	if ((gain == FADC_IDENT_GAIN) && (shift == FADC_IDENT_SHIFT))
		return;

	while (count) {
		*samples = (gain * (*samples)) >> shift;
		samples++;
		count--;
	}
}

#ifdef USB_AVH_FUNCTION_ADC_MIXER_CONTROL

#define FADC_PLAYBACK_VOLUME_CONTROL_NAME	"TT Playback Volume dB"
#define FADC_CAPTURE_VOLUME_CONTROL_NAME	"TT Capture Volume dB"

static int fadc_get_playback_volume_db(struct snd_kcontrol *kcontrol,
					 struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = -fadc_playback_dev->volume.db;
	return 0;
}

static int fadc_set_playback_volume_db(struct snd_kcontrol *kcontrol,
					 struct snd_ctl_elem_value *ucontrol)
{
	fadc_playback_dev->volume.db = -ucontrol->value.integer.value[0];
	fadc_set_volume(&(fadc_playback_dev->volume),
			  fadc_playback_dev->volume.db);

	info("set playback volume %ld\n", fadc_playback_dev->volume.db);
	return 1;
}

static int fadc_get_capture_volume_db(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	ucontrol->value.integer.value[0] = fadc_capture_dev->volume.db;
	return 0;
}

static int fadc_set_capture_volume_db(struct snd_kcontrol *kcontrol,
					struct snd_ctl_elem_value *ucontrol)
{
	fadc_capture_dev->volume.db = ucontrol->value.integer.value[0];
	fadc_set_volume(&(fadc_capture_dev->volume),
			  fadc_capture_dev->volume.db);

	info("set capture volume %ld\n", fadc_capture_dev->volume.db);
	return 1;
}

static const struct snd_kcontrol_new fadc_controls[] = {
	SOC_SINGLE_EXT(FADC_PLAYBACK_VOLUME_CONTROL_NAME, 0, 0, 0x7f, 0,
		       fadc_get_playback_volume_db,
		       fadc_set_playback_volume_db),
	SOC_SINGLE_EXT(FADC_CAPTURE_VOLUME_CONTROL_NAME, 0, 0, 0x7f, 0,
		       fadc_get_capture_volume_db,
		       fadc_set_capture_volume_db)
};

static int fadc_register_controls(struct snd_card *card)
{
	int idx;
	int err;
	struct snd_kcontrol *control;

	BUGME_ON(!card);

	for (idx = 0; idx < ARRAY_SIZE(fadc_controls); idx++) {
		control = snd_ctl_new1(&fadc_controls[idx], 0);

		if (!control) {
			err("can't create sound control\n");
			return -ENOENT;
		}

		err = snd_ctl_add(card, control);

		if (err) {
			err("can't add control to sound card\n");
			return err;
		}
	}

	return 0;
}
#else
static int fadc_register_controls(struct snd_card *card)
{
	err("alsa controls are not supported\n");
	return 0;
}
#endif

static int fadc_setup(struct usb_function *f,
			const struct usb_ctrlrequest *ctrl)
{
	info("setup (nop)\n");
	return -EOPNOTSUPP;
}

static int __devexit fadc_alsa_exit(void)
{
	info("alsa exit\n");

	if (fadc_card) {
		snd_card_disconnect(fadc_card);
		snd_card_free(fadc_card);
		fadc_card = 0;
	}

	return 0;
}

static void fadc_dev_disable(struct fadc_dev *dev)
{
	BUGME_ON(!dev);

	fadc_dev_set_alt(&fadc_function_driver.func, dev, FADC_ALTERNATE_IDLE);
}

static void fadc_disable(struct usb_function *f)
{
	info("disable\n");

	fadc_dev_disable(fadc_playback_dev);
	fadc_dev_disable(fadc_capture_dev);
}

static void fadc_suspend(struct usb_function *f)
{
	info("suspend (nop)\n");
	fadc_disable(f);
}

static void fadc_resume(struct usb_function *f)
{
	info("resume (nop)\n");
}

static int fadc_start_transfer(struct fadc_dev *dev)
{
	int ret = 0;
	unsigned long flags;

	local_irq_save(flags);

	switch (dev->state) {
	case FADC_STATE_PREPARED:
		fadc_set_dev_state(dev, FADC_STATE_VIRTUAL);

		del_timer_sync(&dev->timer);

		dev->timer.expires = jiffies + FADC_TIMER_TICKS;
		dev->timer.data = jiffies;	/* to track how much time elapsed */

		add_timer(&dev->timer);
		break;

	case FADC_STATE_READY:
		fadc_set_dev_state(dev, FADC_STATE_STREAMING);

		if (dev->active) {
			break;
		}

		dev->request->status = 0;
		dev->request->length = dev->iso->maxpacket;
		dev->request->actual = 0;
		dev->request->buf = dev->databuf;
		dev->request->dma = DMA_ADDR_INVALID;
		dev->packet_index = 1;
		dev->active = 1;
		dev->request->complete(dev->iso, dev->request);
		break;

	default:
		err("start transfer while not prepared\n");
		ret = -EPIPE;
		break;
	}

	local_irq_restore(flags);

	return ret;
}

static int fadc_stop_transfer(struct fadc_dev *dev)
{
	switch (dev->state) {
	case FADC_STATE_READY:
	case FADC_STATE_STREAMING:
	case FADC_STATE_SELECTED:
		fadc_set_dev_state(dev, FADC_STATE_SELECTED);
		break;

	case FADC_STATE_VIRTUAL:
		del_timer_sync(&dev->timer);
		/* fall through intentional */
	case FADC_STATE_PREPARED:
	case FADC_STATE_CONFIGURED:
		fadc_set_dev_state(dev, FADC_STATE_CONFIGURED);
		break;

	default:
		err("unexpected alsa stop (state=%d)\n", dev->state);
		break;
	}

	return 0;
}

static int fadc_dev_set_alt(struct usb_function *f,
			      struct fadc_dev *dev,
				unsigned alt)
{
	struct usb_composite_dev *cdev = f->config->cdev;

	BUGME_ON(!f);
	BUGME_ON(!cdev);
	BUGME_ON(!dev);

	info("setting device alternate from %d to %d\n", dev->alt, alt);

	if(alt == FADC_ALTERNATE_IDLE) {
		switch (dev->state) {
		case FADC_STATE_READY:
			fadc_set_dev_state(dev, FADC_STATE_PREPARED);
			break;

		case FADC_STATE_SELECTED:
			fadc_set_dev_state(dev, FADC_STATE_CONFIGURED);
			break;

		case FADC_STATE_STREAMING:
			fadc_set_dev_state(dev, FADC_STATE_PREPARED);
			fadc_start_transfer(dev);
			break;

		default:
			break;
		}
		usb_ep_disable(dev->iso);
		dev->alt = alt;
	}

	if(alt == FADC_ALTERNATE_ACTIVE) {
		int result;

		if (dev->state == FADC_STATE_STREAMING) {
			fadc_set_dev_state(dev, FADC_STATE_PREPARED);
			usb_ep_disable(dev->iso);
		}

		dev->iso_desc = ep_choose(cdev->gadget, dev->hs_iso_desc,
					  dev->fs_iso_desc);
	
		result = usb_ep_enable(dev->iso, dev->iso_desc);

		if (result) {
			err("failed to enable isochronous endpoint\n");
			return result;
		}

		dev->alt = alt;

		switch (dev->state) {
		case FADC_STATE_CONFIGURED:
			fadc_set_dev_state(dev, FADC_STATE_SELECTED);
			break;

		case FADC_STATE_PREPARED:
			fadc_set_dev_state(dev, FADC_STATE_READY);
			break;

		case FADC_STATE_VIRTUAL:
			fadc_stop_transfer(dev);
			fadc_set_dev_state(dev, FADC_STATE_READY);
			fadc_start_transfer(dev);
			break;

		default:
			break;
		}
	}

	return 0;
}

static int fadc_set_alt(struct usb_function *f, unsigned intf, unsigned alt)
{
	info("select alternate setting %d for interface %d\n", alt, intf);

	if (intf == pb_audio_stream_intf_alt0_desc.bInterfaceNumber) {
		return fadc_dev_set_alt(f, fadc_playback_dev, alt);
	}

	if (intf == cp_audio_stream_intf_alt0_desc.bInterfaceNumber) {
		return fadc_dev_set_alt(f, fadc_capture_dev, alt);
	}

	return 0;
}

static int fadc_dev_get_alt(struct fadc_dev *dev)
{
	BUGME_ON(!dev);
	return dev->alt;
}

static int fadc_get_alt(struct usb_function *f, unsigned intf)
{
	info("get alternate setting for interface %d\n", intf);

	if (intf == pb_audio_stream_intf_alt0_desc.bInterfaceNumber) {
		return fadc_dev_get_alt(fadc_playback_dev);
	}

	if (intf == cp_audio_stream_intf_alt0_desc.bInterfaceNumber) {
		return fadc_dev_get_alt(fadc_capture_dev);
	}

	return -EOPNOTSUPP;
}

/* ########################################################################## */
/* ######################### Interface to userspace ######################### */
/* ########################################################################## */

static unsigned int fadc_playback_samplerate[] = { 0 };

static struct snd_pcm_hw_constraint_list playback_constraint_rates = {
	.count = ARRAY_SIZE(fadc_playback_samplerate),
	.list = fadc_playback_samplerate,
	.mask = 0,
};

static unsigned int fadc_capture_samplerate[] = { 0 };

static struct snd_pcm_hw_constraint_list capture_constraint_rates = {
	.count = ARRAY_SIZE(fadc_capture_samplerate),
	.list = fadc_capture_samplerate,
	.mask = 0,
};

#define USE_PERIODS_MIN 	2
#define USE_PERIODS_MAX 	1024

static void fadc_playback_timer(unsigned long past)
{
	struct fadc_dev *dev = fadc_playback_dev;

	BUGME_ON(!dev);

	switch (dev->state) {
	case FADC_STATE_READY:
	case FADC_STATE_STREAMING:
		fadc_set_dev_state(dev, FADC_STATE_READY);
		fadc_start_transfer(dev);
		break;

	case FADC_STATE_VIRTUAL:
		{
			unsigned long now;
			unsigned long ticks;
			unsigned long silenz;
			unsigned long appl_pos;
			unsigned long bytes_to_send;
			struct snd_pcm_runtime *runtime;

			now = jiffies;
			ticks = now - past;

			/* now compute the amount of data that would have
			   been played over USB should there have been
			   a connection */

			silenz =
			    (dev->settings.channels * dev->settings.samplerate *
			     ticks) / HZ;

			/* passing 0 as the buffer pointer causes silence to be
			   written */

			BUGME_ON(!dev->substream);
			BUGME_ON(!dev->substream->runtime);

			runtime = dev->substream->runtime;
			appl_pos =
			    frames_to_bytes(runtime,
					    runtime->control->appl_ptr) %
			    dev->pcm.ring_size;

			if (appl_pos < dev->pcm.ring_pos) {
				bytes_to_send = dev->pcm.ring_size -
				    (dev->pcm.ring_pos - appl_pos);
			} else if (appl_pos > dev->pcm.ring_pos) {
				bytes_to_send = appl_pos - dev->pcm.ring_pos;
			} else {
				bytes_to_send = dev->pcm.ring_size;
			}

			if (silenz > bytes_to_send)
				silenz = bytes_to_send;

		dev->pcm.ring_pos = (dev->pcm.ring_pos + silenz);
		if (dev->pcm.ring_pos >= dev->pcm.ring_size)
			dev->pcm.ring_pos -= dev->pcm.ring_size;

			dev->pcm.period_pos += silenz;

			/* The period_pos is used to check if we have effectively ignored
			   an 'alsa period' worth of data from the ringbuffer. When that
			   happens a signal is sent so that userspace can put new audio
			   data in the ringbuffer. */
			if (dev->pcm.period_pos >= dev->pcm.period_size) {
				dev->pcm.period_pos %= dev->pcm.period_size;
				snd_pcm_period_elapsed(dev->substream);
			}

			dev->timer.expires = now + FADC_TIMER_TICKS;
			dev->timer.data = now;
			add_timer(&dev->timer);
			break;
		}

	default:
		break;
	}
}

/* This function is the callback that is called by the USB gadget driver when a
   request queued by the to_host driver has completed. This may be due to a
   disconnect, endpoint disable or another anomaly, in which case the status of
   the request will indicate an error.

   In case no error occurred, this function will do the following:

   - determine whether with the sending of the data included in this completed
     request, an ALSA 'period' worth of data has been transmitted. If so, this
     must be signalled to ALSA.

   - determine the next packet to be send, and queue it for transmission (the
     HOST will come and try to get this data within the next millisecond).
*/

static void fadc_playback_complete(struct usb_ep *ep, struct usb_request *req)
{
	int status = req->status;
	unsigned long int appl_pos;
	unsigned long int bytes_to_send;
	struct snd_pcm_runtime *runtime;
	struct fadc_dev *dev = (struct fadc_dev *)req->context;

	BUGME_ON(!dev);
	BUGME_ON(dev->request != req);
	BUGME_ON(dev->iso != ep);

	if (dev->state != FADC_STATE_STREAMING) {
		//info("to-host urb completion in non-streaming state\n");
		dev->active = 0;
		wake_up(&dev->queue);
		return;
	}

	if ((!dev->substream) || (!dev->substream->runtime)) {
		err("no alsa administration in to-host device (panic)\n");
		dev->active = 0;
		wake_up(&dev->queue);
		return;
	}

	runtime = dev->substream->runtime;
	appl_pos = frames_to_bytes(runtime, runtime->control->appl_ptr) %
	    dev->pcm.ring_size;

	if (status) {
		err("error in urb (%d), stopping to-host stream\n", status);
		fadc_set_dev_state(dev, FADC_STATE_SELECTED);
		snd_pcm_stop(dev->substream, SNDRV_PCM_STATE_SUSPENDED);
		dev->pcm.ring_pos = appl_pos;
		snd_pcm_period_elapsed(dev->substream);
		dev->active = 0;
		wake_up(&dev->queue);
		return;
	}

	/* The period_pos is used to check if we have send an 'alsa period'
	   worth of data from the ringbuffer. When that happens a signal is sent
	   so that userspace can put new audio data in the ringbuffer. */
	if (dev->pcm.period_pos >= dev->pcm.period_size) {
		dev->pcm.period_pos %= dev->pcm.period_size;
		snd_pcm_period_elapsed(dev->substream);
	}

	if (appl_pos <= dev->pcm.ring_pos) {
		/* appl. data is split between the end of the ringbuffer
		   and the beginning of the ringbuffer */

		bytes_to_send = dev->pcm.ring_size -
		    (dev->pcm.ring_pos - appl_pos);
	} else if (appl_pos > dev->pcm.ring_pos) {
		/* appl. data is linear between the ring_pos and appl_pos. */

		bytes_to_send = appl_pos - dev->pcm.ring_pos;
	}

	if (bytes_to_send == 0) {
		err("out of data, stopping to-host stream\n");
		fadc_set_dev_state(dev, FADC_STATE_PREPARED);
		snd_pcm_stop(dev->substream, SNDRV_PCM_STATE_SUSPENDED);
		dev->pcm.ring_pos = appl_pos;
		snd_pcm_period_elapsed(dev->substream);
		dev->active = 0;
		wake_up(&dev->queue);
		return;
	}

	/* Accumulator check. We can only send whole bytes. For more information
	   see the comments in the fadc_alsa_prepare function */
	if (dev->packet_index == dev->settings.align_length) {
		dev->packet_index = 1;

		req->length = dev->settings.alignment_size;
	} else {
		req->length = dev->settings.nominal_size;

		/* note: in case a 1000 packets of nominal size in 1 second
		   precisely generates the framerate desired, there is no need
		   for alignment packets. captureIn that case, the alignment packet 
		   size will be equal to the nominal packet size and it does not
		   matter to distinguish between the two cases.

		   In that particular case, the packet_index counter simply
		   races to its maximum value and wraps around to start counting
		   again. While this counting does not do anything useful in
		   that case, adding a check to prevent this counting is useless
		   in other situations, and it does not hurt, really. */

		dev->packet_index++;
	}

	/* bytes_to_send now holds the number of bytes that the usb audio stream
	   is lagging behind the application pointer */

	if (bytes_to_send <= req->length) {
		/* if the remaining data fits in the packet, then this will be
		   the last packet */
		req->length = bytes_to_send;
	} else {
		/* there are more bytes to send than the request can do (nominal
		   or alignment size) */
		bytes_to_send = req->length;
	}

	/* bytes_to_send now holds the actual number of bytes to be transmitted
	   for audio */

	if (dev->pcm.ring_pos + bytes_to_send < dev->pcm.ring_size) {
		// the packet is not split
		req->buf = runtime->dma_area + dev->pcm.ring_pos;

		dev->pcm.ring_pos += bytes_to_send;
		dev->pcm.period_pos += bytes_to_send;
	} else if (dev->pcm.ring_pos + bytes_to_send == dev->pcm.ring_size) {
		/* the packet is not split, but this packet contains exactly
		   all data from ring_pos to the end of the ring buffer
		   so ring_pos must be reset */
		req->buf = runtime->dma_area + dev->pcm.ring_pos;

		dev->pcm.ring_pos = 0;
		dev->pcm.period_pos += bytes_to_send;
	} else {
		// the packet is split
		unsigned int part_size = dev->pcm.ring_size - dev->pcm.ring_pos;

		memcpy(dev->databuf,
		       runtime->dma_area + dev->pcm.ring_pos, part_size);

		dev->pcm.ring_pos = bytes_to_send - part_size;

		memcpy(dev->databuf + part_size,
		       runtime->dma_area, dev->pcm.ring_pos);

		req->buf = dev->databuf;

		dev->pcm.period_pos += bytes_to_send;
	}


	if (bytes_to_send != 0) {
		int result;

		/* volume adjustment */
		fadc_boost(&dev->volume, (unsigned short *)req->buf,
			     bytes_to_send / 2);

		result = usb_ep_queue(dev->iso, dev->request, GFP_ATOMIC);		
		if (result) {
			err("error queuing iso packet (%d)\n", result);
			dev->pcm.ring_pos = appl_pos;
			fadc_set_dev_state(dev, FADC_STATE_PREPARED);
			snd_pcm_period_elapsed(dev->substream);
			dev->active = 0;
			wake_up(&dev->queue);
		}
	} else {
		err("empty packet?\n");
		dev->active = 0;
		wake_up(&dev->queue);
	}

	return;
}

static void fadc_capture_transfer(struct snd_pcm_runtime *runtime,
				    unsigned long amount, void *buf)
{
	struct fadc_dev *dev = fadc_capture_dev;

	BUGME_ON(!dev);
	BUGME_ON(!runtime);
/*
	BUGME_ON(!dev->substream);
	BUGME_ON(dev->substream->runtime != runtime);
*/
	if (dev->pcm.ring_pos + amount < dev->pcm.ring_size) {
		// the space to be written to is not split
		if (buf) {
			memcpy(runtime->dma_area + dev->pcm.ring_pos,
			       dev->databuf, amount);
		} else {
			memset(runtime->dma_area + dev->pcm.ring_pos,
			       0, amount);
		}
		dev->pcm.ring_pos += amount;
		dev->pcm.period_pos += amount;
	} else if (dev->pcm.ring_pos + amount == dev->pcm.ring_size) {
		// the space to be written to is not split
		if (buf) {
			memcpy(runtime->dma_area + dev->pcm.ring_pos,
			       dev->databuf, amount);
		} else {
			memset(runtime->dma_area + dev->pcm.ring_pos,
			       0, amount);
		}
		dev->pcm.ring_pos = 0;
		dev->pcm.period_pos += amount;
	} else {
		// the packet is split
		unsigned int part_size = dev->pcm.ring_size - dev->pcm.ring_pos;
		if (buf) {
			memcpy(runtime->dma_area + dev->pcm.ring_pos,
			       dev->databuf, part_size);
		} else {
			memset(runtime->dma_area + dev->pcm.ring_pos,
			       0, part_size);
		}

		dev->pcm.ring_pos = amount - part_size;

		if (buf) {
			memcpy(runtime->dma_area,
			       dev->databuf + part_size, dev->pcm.ring_pos);
		} else {
			memset(runtime->dma_area, 0, dev->pcm.ring_pos);
		}

		dev->pcm.period_pos += amount;
	}

	/* The period_pos is used to check if we have send an 'alsa period'
	   worth of data from the ringbuffer. When that happens a signal is sent
	   so that userspace can get new audio data from the ringbuffer. */

	if (dev->pcm.period_pos >= dev->pcm.period_size) {
		dev->pcm.period_pos %= dev->pcm.period_size;
		snd_pcm_period_elapsed(dev->substream);
	}
}

static void fadc_capture_timer(unsigned long past)
{
	struct fadc_dev *dev = fadc_capture_dev;

	BUGME_ON(!dev);

	switch (dev->state) {
	case FADC_STATE_READY:
	case FADC_STATE_STREAMING:
		fadc_set_dev_state(dev, FADC_STATE_READY);
		fadc_start_transfer(dev);
		break;

	case FADC_STATE_VIRTUAL:
		{
			/* unsigned arithmetic will take care of wrap around
			   should that ever occur */
			unsigned long now;
			unsigned long ticks;
			unsigned long silenz;

			now = jiffies;
			ticks = now - past;

			/* now compute the amount of data that would have
			   been captured over USB should there have been
			   a connection */

			silenz =
			    (dev->settings.channels * dev->settings.samplerate *
			     ticks) / HZ;

			BUGME_ON(!dev->substream);
			BUGME_ON(!dev->substream->runtime);

			/* passing 0 as the buffer pointer causes silence to be
			   written */
			fadc_capture_transfer(dev->substream->runtime,
						silenz, 0);

			dev->timer.expires = now + FADC_TIMER_TICKS;
			dev->timer.data = now;
			add_timer(&dev->timer);
			break;
		}

	default:
		break;
	}
}

static void fadc_capture_complete(struct usb_ep *ep, struct usb_request *req)
{
	int status = req->status;
	int appl_pos;
	int bytes_available;
	struct snd_pcm_runtime *runtime;
	struct fadc_dev *dev = (struct fadc_dev *)req->context;

	BUGME_ON(!dev);
	BUGME_ON(dev->request != req);
	BUGME_ON(dev->iso != ep);

	if (dev->state == FADC_STATE_VIRTUAL) {
		/* no USB connection, but data needs to be produced
		   at steady pace */
		err("CAPTURE REQUEST COMPLETE WHILE VIRTUAL\n");
		dev->active = 0;
		wake_up(&dev->queue);
		return;
	}

	if (dev->state != FADC_STATE_STREAMING) {
		debug("capture urb completion in non-streaming state\n");
		dev->active = 0;
		wake_up(&dev->queue);
		return;
	}

	runtime = dev->substream->runtime;
	appl_pos = frames_to_bytes(runtime, runtime->control->appl_ptr) %
	    dev->pcm.ring_size;

	if (status) {
		err("error in urb (%d), stopping capture stream\n", status);
		fadc_set_dev_state(dev, FADC_STATE_PREPARED);
		snd_pcm_stop(dev->substream, SNDRV_PCM_STATE_SUSPENDED);
		dev->pcm.ring_pos = appl_pos;
		snd_pcm_period_elapsed(dev->substream);
		dev->active = 0;
		wake_up(&dev->queue);
		return;
	}

	dev->packet_index++;

	if (req->actual % (dev->settings.channels * 2)) {
		err("received a packet that contains a split audio sample\n");
		err("channels = %d\n", dev->settings.channels);
		err("packet->actual = %d\n", req->actual);
	}

	/* volume adjustment */
	fadc_boost(&dev->volume, (unsigned short *)req->buf, req->actual / 2);

	/* copy data from the received USB request buffer to the ringbuffer */

	if (appl_pos < dev->pcm.ring_pos) {
		/* available space in the ringbuffer is split between the end of
		   the ringbuffer and the beginning of the ringbuffer */

		bytes_available = dev->pcm.ring_size -
		    (dev->pcm.ring_pos - appl_pos);
	} else if (appl_pos > dev->pcm.ring_pos) {
		/* available space in the ringbuffer is linear between the
		   ring_pos and appl_pos.   */

		bytes_available = appl_pos - dev->pcm.ring_pos;
	} else {		/* the ring is empty (if the state is STREAMING) */

		bytes_available = dev->pcm.ring_size;
	}

	if (bytes_available <= req->actual) {
		/* the ring will be filled completely; this is not desired
		   normally, the application should be fast enough to keep up with
		   the USB stream filling the ringbuffer; now, we start dropping 
		   packets until there is room enough again. */
		bytes_available = 0;
	} else {
		/* there is more space in the ring than can be filled with data
		   from this packet  */
		bytes_available = req->actual;
	}

	/* bytes_available now holds the actual number of bytes to be copied to
	   the ringbuffer */

	fadc_capture_transfer(runtime, bytes_available, dev->databuf);

	req->status = 0;
	req->length = ep->maxpacket;
	req->buf = dev->databuf;
	req->dma = DMA_ADDR_INVALID;
	req->actual = 0;


	status = usb_ep_queue(ep, req, GFP_ATOMIC);
	if (status) {
		err("error queuing iso packet (%d)\n", status);
		dev->pcm.ring_pos = appl_pos;
		snd_pcm_period_elapsed(dev->substream);

		/* TODO: check whether READY is the correct state. This depends
		   on the reason the queue operation failed. One way to
		   determine what to do might be to check the 'alt-setting'
		   of the device at this point. */

		fadc_set_dev_state(dev, FADC_STATE_READY);

		dev->active = 0;
		wake_up(&dev->queue);
	}
}

static int fadc_alsa_trigger(struct snd_pcm_substream *substream, int cmd)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct fadc_dev *dev = (struct fadc_dev *)runtime->private_data;

	BUGME_ON(!dev);

	switch (cmd) {
	case SNDRV_PCM_TRIGGER_START:
		info("alsa start\n");

		return fadc_start_transfer(dev);

	case SNDRV_PCM_TRIGGER_STOP:
		info("alsa stop\n");

		return fadc_stop_transfer(dev);

	default:
		err("unknown command (%d)\n", cmd);
		return -EINVAL;
	}

	return 0;
}

/* Calculate the amount of data we have to send each request. */
static int fadc_alsa_prepare(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct fadc_dev *dev = (struct fadc_dev *)runtime->private_data;

	BUGME_ON(!dev);

	info("prepare\n");

	if (wait_event_interruptible(dev->queue, !dev->active) != 0) {
		return -ERESTARTSYS;
	}

	if (dev->state == FADC_STATE_STREAMING) {
		err("state is FADC_STATE_STREAMING\n");
		return -EINVAL;
	}

	if (runtime->rate != dev->settings.samplerate) {
		err("sample rate %d is not allowed (%d)\n",
		    runtime->rate, dev->settings.samplerate);
		return -EINVAL;
	}

	if (runtime->channels != dev->settings.channels) {
		err("number of channels %d is not allowed (%d)\n",
		    runtime->channels, dev->settings.channels);
		return -EINVAL;
	}

	if (runtime->format != SNDRV_PCM_FORMAT_S16_LE) {
		err("sample format %d is not allowed (%d)\n",
		    runtime->format, SNDRV_PCM_FORMAT_S16_LE);
		return -EINVAL;
	}

	dev->pcm.ring_size = snd_pcm_lib_buffer_bytes(substream);
	dev->pcm.ring_pos = 0;
	dev->pcm.period_size = snd_pcm_lib_period_bytes(substream);
	dev->pcm.period_pos = 0;

	switch (dev->state) {
	case FADC_STATE_SELECTED:
		fadc_set_dev_state(dev, FADC_STATE_READY);
		break;

	case FADC_STATE_CONFIGURED:
		fadc_set_dev_state(dev, FADC_STATE_PREPARED);
		break;

	default:
		break;
	}

	return 0;
}

/* Return hardware pointer in ALSA ring buffer. */
static snd_pcm_uframes_t
fadc_alsa_pointer(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	struct fadc_dev *dev = (struct fadc_dev *)runtime->private_data;

	BUGME_ON(!dev);

	switch (dev->state) {
	case FADC_STATE_VIRTUAL:
	case FADC_STATE_STREAMING:
		return bytes_to_frames(runtime, dev->pcm.ring_pos);

	default:
		return bytes_to_frames(runtime,
				       frames_to_bytes(runtime,
						       runtime->control->
						       appl_ptr) %
				       dev->pcm.ring_size);
	}
}

static struct snd_pcm_hardware fadc_alsa_playback = { 0 };
static struct snd_pcm_hardware fadc_alsa_capture = { 0 };

/* This function will free the allocated ring buffer
 * Non atomic function. */
static int fadc_alsa_hw_params(struct snd_pcm_substream *substream,
				 struct snd_pcm_hw_params *hw_params)
{
	info("going to allocate ring buffer of %d bytes\n",
	     params_buffer_bytes(hw_params));
	return snd_pcm_lib_malloc_pages(substream,
					params_buffer_bytes(hw_params));
}

/* This function will free the allocated/reserved memory pool */
static int fadc_alsa_hw_free(struct snd_pcm_substream *substream)
{
	info("free ring buffer\n");
	return snd_pcm_lib_free_pages(substream);
}

static int fadc_alsa_playback_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	int err = 0;

	debug("alsa to-host open\n");

	BUGME_ON(!fadc_playback_dev);

	if (runtime->private_data != 0)
		return -EBUSY;

	/* Make sure the device can only be opened with the configured sample
	   rate Hz */
	err = snd_pcm_hw_constraint_list(runtime, 0, SNDRV_PCM_HW_PARAM_RATE,
					 &playback_constraint_rates);
	if (err) {
		info("audio rate constraint check returned %d\n", err);
		return err;
	}

	fadc_playback_dev->substream = substream;
	runtime->private_data = fadc_playback_dev;
	runtime->hw = fadc_alsa_playback;

	return err;
}

static int fadc_alsa_capture_open(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;
	int err = 0;

	info("alsa capture open\n");

	BUGME_ON(!fadc_capture_dev);

	if (runtime->private_data != 0) {
		return -EBUSY;
	}

	fadc_capture_dev->substream = substream;
	runtime->private_data = fadc_capture_dev;
	runtime->hw = fadc_alsa_capture;

	/* Make sure the device can only be opened with the configured sample
	   rate Hz */
	err = snd_pcm_hw_constraint_list(runtime, 0, SNDRV_PCM_HW_PARAM_RATE,
					 &capture_constraint_rates);
	info("audio rate constraint check returned %d\n", err);

	return err;
}

static int fadc_alsa_close(struct snd_pcm_substream *substream)
{
	struct snd_pcm_runtime *runtime = substream->runtime;

	info("alsa close\n");

	((struct fadc_dev *)runtime->private_data)->substream = 0;
	runtime->private_data = 0;

	return 0;
}

static struct snd_pcm_ops audio_alsa_playback_ops = {
	.open = fadc_alsa_playback_open,
	.close = fadc_alsa_close,
	.ioctl = snd_pcm_lib_ioctl,
	.hw_params = fadc_alsa_hw_params,
	.hw_free = fadc_alsa_hw_free,
	.prepare = fadc_alsa_prepare,
	.trigger = fadc_alsa_trigger,
	.pointer = fadc_alsa_pointer,
};

static struct snd_pcm_ops audio_alsa_capture_ops = {
	.open = fadc_alsa_capture_open,
	.close = fadc_alsa_close,
	.ioctl = snd_pcm_lib_ioctl,
	.hw_params = fadc_alsa_hw_params,
	.hw_free = fadc_alsa_hw_free,
	.prepare = fadc_alsa_prepare,
	.trigger = fadc_alsa_trigger,
	.pointer = fadc_alsa_pointer,
};

/* init/exit - module insertion/removal */

static int __init fadc_alsa_init(void)
{
	struct snd_pcm *pcm;
	int err;

	info("alsa init\n");

	BUGME_ON(fadc_card);
	BUGME_ON(!fadc_playback_dev);
	BUGME_ON(!fadc_capture_dev);


 	err = snd_card_create(-1, fadc_name, THIS_MODULE, 0, &fadc_card);
 	if (err) {
//	fadc_card = snd_card_new(-1, fadc_name, THIS_MODULE, 0);
//	if (fadc_card == 0) {
		err("snd_card_new failed\n");
		goto fail;
	}

	strlcpy(fadc_card->driver, fadc_name, sizeof(fadc_card->driver));
	strlcpy(fadc_card->shortname, "usb_audio",
		sizeof(fadc_card->shortname));

	snprintf(fadc_card->longname, sizeof(fadc_card->longname),
		 "%s%i", fadc_name, 1);

	if ((err = snd_pcm_new(fadc_card, fadc_name, 0, 1, 1, &pcm)) < 0) {
		err("snd_pcm_new failed (%d)\n", err);
		goto fail;
	}

	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_PLAYBACK,
			&audio_alsa_playback_ops);
	snd_pcm_set_ops(pcm, SNDRV_PCM_STREAM_CAPTURE, &audio_alsa_capture_ops);

	pcm->info_flags = 0;
	strcpy(pcm->name, fadc_name);

	if ((err = snd_pcm_lib_preallocate_pages_for_all(pcm,
							 SNDRV_DMA_TYPE_CONTINUOUS,
							 snd_dma_continuous_data
							 (GFP_KERNEL),
							 FADC_PREALLOC_SIZE,
							 FADC_PREALLOC_SIZE))
	    < 0) {
		err("snd_pcm_lib_preallocate_pages_for_all failed (%d)\n", err);
		goto fail;
	}

	fadc_playback_dev->name = "playback";
	fadc_capture_dev->name = "capture";

	/* Register the soundcard */

	err = snd_card_register(fadc_card);

	if (err) {
		err("register soundcard failed (%d)\n", err);
		goto fail;
	}

	fadc_register_controls(fadc_card);

	return 0;

      fail:
	fadc_alsa_exit();
	return err;
}

static __init void fadc_compute_playback_streaming_parameters(void)
{
	int index;
	struct fadc_packet_setting *setting;
	struct fadc_dev *dev = fadc_playback_dev;
	struct snd_pcm_hardware *pcm_hw = &fadc_alsa_playback;

	BUGME_ON(!fadc_playback_dev);

	info("lookup to_host parameters for %d channels, 16 bit samples, "
	     "%d samples/second\n",
	     dev->settings.channels, dev->settings.samplerate);

	fadc_playback_samplerate[0] = dev->settings.samplerate;

	for (index = 0; index < ARRAY_SIZE(fadc_packet_settings); index++) {
		if (fadc_packet_settings[index].samplerate ==
		    dev->settings.samplerate)
			break;
	}

	if (index == ARRAY_SIZE(fadc_packet_settings)) {
		err("confused about the playback settings\n");
		return;
	}

	setting = &fadc_packet_settings[index];

	dev->settings.alsa_rate = setting->alsa_rate;
	dev->settings.nominal_size = setting->nominal_size *
	    dev->settings.channels * FADC_DEFAULT_INTERVAL_PLAYBACK;

	dev->settings.alignment_size = setting->alignment_size *
	    dev->settings.channels * FADC_DEFAULT_INTERVAL_PLAYBACK;

	dev->settings.align_length = setting->align_length;

	info("%d nominal packets (%d bytes each) for each alignment packet of %d bytes\n", dev->settings.align_length, dev->settings.nominal_size, dev->settings.alignment_size);

	pcm_hw->info = (SNDRV_PCM_INFO_INTERLEAVED |
			SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID);
	pcm_hw->formats = (SNDRV_PCM_FMTBIT_S16_LE);
	pcm_hw->rates = dev->settings.alsa_rate;
	pcm_hw->rate_min = dev->settings.samplerate;
	pcm_hw->rate_max = dev->settings.samplerate;
	pcm_hw->channels_min = dev->settings.channels;
	pcm_hw->channels_max = dev->settings.channels;
	pcm_hw->buffer_bytes_max = FADC_PLAYBACK_RINGBUFFER_SIZE;
	pcm_hw->period_bytes_min = 64;
	pcm_hw->period_bytes_max = FADC_PLAYBACK_RINGBUFFER_SIZE;
	pcm_hw->periods_min = USE_PERIODS_MIN;
	pcm_hw->periods_max = USE_PERIODS_MAX;
	pcm_hw->fifo_size = 0;

	/* fix the (static) descriptor values ... */

	/* ... in the format type descriptor ... */
	pb_format_type_desc.bNrChannels = dev->settings.channels;
	pb_format_type_desc.bSamFreqType = 0x01;
	pb_format_type_desc.tSamFreq00 = dev->settings.samplerate & 0xff;
	pb_format_type_desc.tSamFreq01 = (dev->settings.samplerate >> 8) & 0xff;
	pb_format_type_desc.tSamFreq02 =
	    (dev->settings.samplerate >> 16) & 0xff;

	/* ... in the input terminal descriptor ... */
	playback_input_terminal_desc10.bNrChannels = dev->settings.channels;

	/* ... and in the endpoint descriptor */
	pb_as_fs_iso_audio_data_ep_desc.wMaxPacketSize =
	    __cpu_to_le16(dev->settings.alignment_size);
	pb_as_hs_iso_audio_data_ep_desc.wMaxPacketSize =
	    __cpu_to_le16(dev->settings.alignment_size);
}

static __init void fadc_compute_capture_streaming_parameters(void)
{
	int index;
	struct fadc_packet_setting *setting;
	struct fadc_dev *dev = fadc_capture_dev;
	struct snd_pcm_hardware *pcm_hw = &fadc_alsa_capture;

	BUGME_ON(!fadc_capture_dev);

	info("lookup capture parameters for %d channels, 16 bit samples, "
	     "%d samples/second\n", dev->settings.channels,
	     dev->settings.samplerate);

	fadc_capture_samplerate[0] = fadc_capture_dev->settings.samplerate;

	for (index = 0; index < ARRAY_SIZE(fadc_packet_settings); index++) {
		if (fadc_packet_settings[index].samplerate ==
		    dev->settings.samplerate)
			break;
	}

	if (index == ARRAY_SIZE(fadc_packet_settings)) {
		err("confused about the playback settings\n");
		return;
	}

	setting = &fadc_packet_settings[index];

	dev->settings.alsa_rate = setting->alsa_rate;

	/* these settings are not required for from_host (host-to-device) as
	   the host determines the packet sizes however, we use them here to
	   compute the maximum time interval between packets */

	dev->settings.nominal_size = setting->nominal_size *
	    dev->settings.channels * FADC_DEFAULT_INTERVAL_CAPTURE;

	dev->settings.alignment_size = setting->alignment_size *
	    dev->settings.channels * FADC_DEFAULT_INTERVAL_CAPTURE;

	dev->settings.align_length = setting->align_length;

	pcm_hw->info = (SNDRV_PCM_INFO_INTERLEAVED |
			SNDRV_PCM_INFO_MMAP | SNDRV_PCM_INFO_MMAP_VALID);
	pcm_hw->formats = (SNDRV_PCM_FMTBIT_S16_LE);
	pcm_hw->rates = dev->settings.alsa_rate;
	pcm_hw->rate_min = dev->settings.samplerate;
	pcm_hw->rate_max = dev->settings.samplerate;
	pcm_hw->channels_min = dev->settings.channels;
	pcm_hw->channels_max = dev->settings.channels;
	pcm_hw->buffer_bytes_max = FADC_CAPTURE_RINGBUFFER_SIZE;
	pcm_hw->period_bytes_min = 64;
	pcm_hw->period_bytes_max = FADC_CAPTURE_RINGBUFFER_SIZE;
	pcm_hw->periods_min = USE_PERIODS_MIN;
	pcm_hw->periods_max = USE_PERIODS_MAX;
	pcm_hw->fifo_size = 0;

	/* fix the (static) descriptor values ... */

	/* ... in the format type descriptor ... */
	cp_format_type_desc.bNrChannels = dev->settings.channels;
	cp_format_type_desc.bSamFreqType = 0x01;
	cp_format_type_desc.tSamFreq00 = dev->settings.samplerate & 0xff;
	cp_format_type_desc.tSamFreq01 = (dev->settings.samplerate >> 8) & 0xff;
	cp_format_type_desc.tSamFreq02 =
	    (dev->settings.samplerate >> 16) & 0xff;

	/* ... in the input terminal descriptor ... */
	capture_input_terminal_desc10.bNrChannels = dev->settings.channels;

	/* ... and in the endpoint descriptor */
	{
		unsigned int max_packet_size = dev->settings.alignment_size;
		unsigned int ep_max_packet_size =
		    __le16_to_cpu(cp_as_hs_iso_audio_data_ep_desc.
				  wMaxPacketSize);

		/* the alignment size is OUR estimate of the maximum packet
		   size, the host may differ if it uses a different algorithm,
		   we compensate with 12.5% allowance */
		/* ROLK: temporary fix

		   max_packet_size += (max_packet_size >> 3);

		 */
		if (ep_max_packet_size > max_packet_size) {
			cp_as_fs_iso_audio_data_ep_desc.wMaxPacketSize =
			    __cpu_to_le16(max_packet_size);
			cp_as_hs_iso_audio_data_ep_desc.wMaxPacketSize =
			    __cpu_to_le16(max_packet_size);
		}
	}
}

static __init void fadc_compute_streaming_parameters(void)
{
	fadc_compute_playback_streaming_parameters();
	fadc_compute_capture_streaming_parameters();
}

int __init fadc_validate_parameters(struct fadc_device_parameters *p,
				      int interval)
{
	int index;

	BUGME_ON(!p);

	for (index = 0; index < ARRAY_SIZE(fadc_packet_settings); index++) {
		if (fadc_packet_settings[index].samplerate != p->samplerate)
			continue;

		if (fadc_packet_settings[index].alignment_size * interval *
		    p->channels <= FADC_MAX_ISO_PACKET_SIZE)
			return 0;
	}

	info("module parameters are not valid; cannot determine");
	info(" USB parameters\nsupported samplerates:\n");

	for (index = 0; index < ARRAY_SIZE(fadc_packet_settings); index++) {
		info("   %d\n", fadc_packet_settings[index].samplerate);
	}

	return -EINVAL;
}

/* sysfs support */

static int fadc_get_state_string(char *buf, struct fadc_dev *dev)
{
	int ret = 0;

	if (dev == NULL)
		return 0;
	switch (dev->state) {
	case FADC_STATE_VOID:
		ret = sprintf(buf, "VOID\n");
		break;
	case FADC_STATE_IDLE:
		ret = sprintf(buf, "IDLE\n");
		break;
	case FADC_STATE_CONFIGURED:
		ret = sprintf(buf, "CONFIGURED\n");
		break;
	case FADC_STATE_SELECTED:
		ret = sprintf(buf, "SELECTED\n");
		break;
	case FADC_STATE_PREPARED:
		ret = sprintf(buf, "PREPARED\n");
		break;
	case FADC_STATE_READY:
		ret = sprintf(buf, "READY\n");
		break;
	case FADC_STATE_STREAMING:
		ret = sprintf(buf, "STREAMING\n");
		break;
	case FADC_STATE_VIRTUAL:
		ret = sprintf(buf, "VIRTUAL\n");
		break;
	default:
		ret = sprintf(buf, "UNKNOWN\n");
		break;
	}
	return ret;
}

static ssize_t fadc_read_playback_state(struct device *dev,
					  struct device_attribute *attr,
					  char *buf)
{
	return fadc_get_state_string(buf, fadc_playback_dev);
}

static ssize_t fadc_read_capture_state(struct device *dev,
					 struct device_attribute *attr,
					 char *buf)
{
	return fadc_get_state_string(buf, fadc_capture_dev);
}

static DEVICE_ATTR(playback_state, S_IRUGO, fadc_read_playback_state, NULL);
static DEVICE_ATTR(capture_state, S_IRUGO, fadc_read_capture_state, NULL);

static void fadc_release(struct device *dev)
{
	info("%s - not implemented\n", __FUNCTION__);
}

struct device fadc_device = {
	.init_name = "faudio",
	.release = fadc_release
};

static int fadc_init_attr(void)
{
	int res = -ENODEV;

	res = device_register(&fadc_device);

	/* Sysfs attributes. */
	if (res >= 0) {
		if (device_create_file(&fadc_device, &dev_attr_playback_state)
		    || device_create_file(&fadc_device,
					  &dev_attr_capture_state)) {
			return -ENODEV;
		}
	} else {
		return res;
	}

	return 0;
}

static void fadc_destroy_attr(void)
{
	device_remove_file(&fadc_device, &dev_attr_playback_state);
	device_remove_file(&fadc_device, &dev_attr_capture_state);
	device_unregister(&fadc_device);
}

static int __init fadc_bind(struct usb_configuration *c,
			      struct usb_function *f)
{
	struct usb_composite_dev *cdev = c->cdev;
	int status;

	info("bind\n");

	BUGME_ON(fadc_card);

	/* Allocate fadc_dev structure to keep track of our administration */
	fadc_playback_dev = kzalloc(sizeof(struct fadc_dev), GFP_KERNEL);
	if (!fadc_playback_dev) {
		err("failed to allocate playback fadc_dev\n");
		return -ENOMEM;
	}

	/* Allocate fadc_dev structure to keep track of our administration */
	fadc_capture_dev = kzalloc(sizeof(struct fadc_dev), GFP_KERNEL);
	if (!fadc_capture_dev) {
		err("failed to allocate capture fadc_dev\n");
		return -ENOMEM;
	}

	fadc_set_volume(&fadc_playback_dev->volume, 0);
	fadc_set_volume(&fadc_capture_dev->volume, 0);

	/* Initialise timer structures */
	init_timer(&fadc_playback_dev->timer);
	fadc_playback_dev->timer.function = fadc_playback_timer;

	init_timer(&fadc_capture_dev->timer);
	fadc_capture_dev->timer.function = fadc_capture_timer;

	/* Allocate buffer, used in fadc_playback_complete. */
	fadc_playback_dev->databuf = kmalloc(FADC_HELPBUF_SIZE, GFP_KERNEL);
	if (!fadc_playback_dev->databuf) {
		err("not enough memory to allocate playback packet buffer\n");
		return -ENOMEM;
	}

	/* Allocate buffer, used in fadc_capture_complete. */
	fadc_capture_dev->databuf = kmalloc(FADC_HELPBUF_SIZE, GFP_KERNEL);
	if (!fadc_capture_dev->databuf) {
		err("not enough memory to allocate capture packet buffer\n");
		return -ENOMEM;
	}

	if (fadc_validate_parameters(&fadc_function_driver.playback,
				       FADC_DEFAULT_INTERVAL_PLAYBACK) == 0) {
		fadc_playback_dev->settings.channels =
		    fadc_function_driver.playback.channels;
		fadc_playback_dev->settings.samplerate =
		    fadc_function_driver.playback.samplerate;
	} else {
		fadc_playback_dev->settings.channels =
		    FADC_DEFAULT_CHANNELS_PLAYBACK;
		fadc_playback_dev->settings.samplerate =
		    FADC_DEFAULT_SAMPLERATE_PLAYBACK;
	}

	if (fadc_validate_parameters(&fadc_function_driver.capture,
				       FADC_DEFAULT_INTERVAL_CAPTURE) == 0) {
		fadc_capture_dev->settings.channels =
		    fadc_function_driver.capture.channels;
		fadc_capture_dev->settings.samplerate =
		    fadc_function_driver.capture.samplerate;
	} else {
		fadc_capture_dev->settings.channels =
		    FADC_DEFAULT_CHANNELS_CAPTURE;
		fadc_capture_dev->settings.samplerate =
		    FADC_DEFAULT_SAMPLERATE_CAPTURE;
	}

	fadc_compute_streaming_parameters();

	fadc_playback_dev->alt = 0;
	fadc_capture_dev->alt = 0;

	init_waitqueue_head(&fadc_playback_dev->queue);
	init_waitqueue_head(&fadc_capture_dev->queue);

	fadc_playback_dev->active = 0;
	fadc_capture_dev->active = 0;

	fadc_alsa_init();

	fadc_set_dev_state(fadc_playback_dev, FADC_STATE_IDLE);
	fadc_set_dev_state(fadc_capture_dev, FADC_STATE_IDLE);

	/* Reserve an interface number for the audio control interface */
	status = usb_interface_id(c, f);
	if (status < 0) {
		err("usb_interface_id for audio control failed (%d)\n", status);
		goto fail;
	}
	audio_control_interface_desc.bInterfaceNumber = status;
	ac_intf_assoc_desc.bFirstInterface = status;

	/* NICE_TO: to support arbitrary number of streams, it is required to
	   reserve an interface number for each stream, and to dynamically add
	   the required descriptors to 'fadc_fs_descriptors' and
	   'fadc_hs_descriptors' */

	/* Reserve an interface number for the audio playback stream interface */
	status = usb_interface_id(c, f);
	if (status < 0)
		goto fail;

	pb_audio_stream_intf_alt0_desc.bInterfaceNumber = status;
	pb_audio_stream_intf_alt1_desc.bInterfaceNumber = status;
	ac_header_desc2.baInterfaceNr[0] = status;

	/* Reserve an interface number for the audio capture stream interface */
	status = usb_interface_id(c, f);
	if (status < 0)
		goto fail;

	cp_audio_stream_intf_alt0_desc.bInterfaceNumber = status;
	cp_audio_stream_intf_alt1_desc.bInterfaceNumber = status;
	ac_header_desc2.baInterfaceNr[1] = status;

	/* Reserve strings from the composite framework */

	if (fadc_strings_dev[FADC_STRING_IDX].id == 0) {
		status = usb_string_id(c->cdev);
		if (status < 0)
			goto fail;

		fadc_strings_dev[FADC_STRING_IDX].id = status;
		ac_intf_assoc_desc.iFunction = status;
		audio_control_interface_desc.iInterface = status;
	}

	if (fadc_strings_dev[FADC_PLAYBACK_STRING_IDX].id == 0) {
		status = usb_string_id(c->cdev);
		if (status < 0)
			goto fail;

		fadc_strings_dev[FADC_PLAYBACK_STRING_IDX].id = status;
		playback_input_terminal_desc10.iChannelNames = status;
		playback_input_terminal_desc10.iTerminal = status;
		playback_output_terminal_desc10.iTerminal = status;
		pb_audio_stream_intf_alt0_desc.iInterface = status;
		pb_audio_stream_intf_alt1_desc.iInterface = status;
	}

	if (fadc_strings_dev[FADC_CAPTURE_STRING_IDX].id == 0) {
		status = usb_string_id(c->cdev);
		if (status < 0)
			goto fail;

		fadc_strings_dev[FADC_CAPTURE_STRING_IDX].id = status;
		capture_input_terminal_desc10.iChannelNames = status;
		capture_input_terminal_desc10.iTerminal = status;
		capture_output_terminal_desc10.iTerminal = status;
		cp_audio_stream_intf_alt0_desc.iInterface = status;
		cp_audio_stream_intf_alt1_desc.iInterface = status;
	}

	f->descriptors = fadc_fs_descriptors;

	/* usb to_host driver endpoint */

	fadc_playback_dev->iso = usb_ep_autoconfig(cdev->gadget,
						     (struct
						      usb_endpoint_descriptor *)
						     &pb_as_fs_iso_audio_data_ep_desc);

	if (!fadc_playback_dev->iso) {
		err("usb_ep_autoconfig for audio playback streaming endpoint "
		    "failed for full-speed\n");
		goto fail;
	}

	info("playback got endpoint %s\n", fadc_playback_dev->iso->name);
	
	fadc_playback_dev->iso->driver_data = fadc_playback_dev;


	fadc_capture_dev->iso = usb_ep_autoconfig(cdev->gadget,
						    (struct
						     usb_endpoint_descriptor *)
						    &cp_as_fs_iso_audio_data_ep_desc);
	if (!fadc_capture_dev->iso) {
		err("usb_ep_autoconfig for audio capture streaming endpoint "
		    "failed for full-speed\n");
		goto fail;
	}

	info("capture got endpoint %s\n", fadc_capture_dev->iso->name);


	fadc_capture_dev->iso->driver_data = fadc_capture_dev;


	/* Set local endpoint administration */
	fadc_playback_dev->fs_iso_desc =
	    (struct usb_endpoint_descriptor *)&pb_as_fs_iso_audio_data_ep_desc;
	fadc_capture_dev->fs_iso_desc =
	    (struct usb_endpoint_descriptor *)&cp_as_fs_iso_audio_data_ep_desc;

	/* Allocate USB request for audio data transport */
	fadc_playback_dev->request =
	    usb_ep_alloc_request(fadc_playback_dev->iso, GFP_KERNEL);

	if (!fadc_playback_dev->request) {
		err("usb request allocation failed\n");
		goto fail;
	}

	info("playback request = %p (list = %p)\n", fadc_playback_dev->request, &(fadc_playback_dev->request->list));

	fadc_playback_dev->request->context = fadc_playback_dev;
	fadc_playback_dev->request->complete = fadc_playback_complete;

	fadc_capture_dev->request =
	    usb_ep_alloc_request(fadc_capture_dev->iso, GFP_KERNEL);

	if (!fadc_capture_dev->request) {
		err("usb request allocation failed\n");
		goto fail;
	}

	info("capture request = %p (list = %p)\n", fadc_capture_dev->request, &(fadc_capture_dev->request->list));

	fadc_capture_dev->request->context = fadc_capture_dev;
	fadc_capture_dev->request->complete = fadc_capture_complete;

	/* Runtime check if compile time CONFIG_USB_GADGET_DUALSPEED was set. */
	if (gadget_is_dualspeed(c->cdev->gadget)) {
		info("high speed usb - assign audio descriptors accordingly\n");

		pb_as_hs_iso_audio_data_ep_desc.bEndpointAddress =
		    pb_as_fs_iso_audio_data_ep_desc.bEndpointAddress;
		cp_as_hs_iso_audio_data_ep_desc.bEndpointAddress =
		    cp_as_fs_iso_audio_data_ep_desc.bEndpointAddress;

		f->hs_descriptors = fadc_hs_descriptors;

		fadc_playback_dev->hs_iso_desc =
		    (struct usb_endpoint_descriptor *)
		    &pb_as_hs_iso_audio_data_ep_desc;

		fadc_capture_dev->hs_iso_desc =
		    (struct usb_endpoint_descriptor *)
		    &cp_as_hs_iso_audio_data_ep_desc;
	}

	fadc_set_dev_state(fadc_playback_dev, FADC_STATE_CONFIGURED);
	fadc_set_dev_state(fadc_capture_dev, FADC_STATE_CONFIGURED);

	fadc_init_attr();

	return 0;

      fail:
	/* cleanup of allocated resources (memory, descriptors, ...) is done in
	   the unbind call, called by the composite framework because this
	   function bind call fails */

	err("can't bind\n");
	return -ENODEV;
}

static void fadc_destroy_device(struct fadc_dev *dev)
{
	if (!dev)
		return;

	fadc_set_dev_state(dev, FADC_STATE_IDLE);

	if (dev->iso) {
		usb_ep_disable(dev->iso);
	}

	if (dev->request) {
		usb_ep_free_request(dev->iso, dev->request);
		dev->request = 0;
	}

	dev->fs_iso_desc = 0;
	dev->hs_iso_desc = 0;

	if (dev->iso) {
		dev->iso->driver_data = 0;
		dev->iso = 0;
	}

	del_timer_sync(&dev->timer);

	if (dev->databuf) {
		kfree(dev->databuf);
		dev->databuf = 0;
	}

	fadc_set_dev_state(dev, FADC_STATE_VOID);

	if (dev) {
		kfree(dev);
	}
}

static void fadc_unbind(struct usb_configuration *c, struct usb_function *f)
{
	info("unbind\n");

	BUGME_ON(in_interrupt());

	fadc_destroy_attr();

	fadc_alsa_exit();

	fadc_destroy_device(fadc_playback_dev);
	fadc_playback_dev = 0;

	fadc_destroy_device(fadc_capture_dev);
	fadc_capture_dev = 0;

#if 0
	/* this is required if the descriptors have been copied from static
	   initialisation data using cusb_copy_descriptors (in which case
	   ->fs_iso_desc and ->hs_iso_desc have been set to the appropriate
	   copied descriptors using usb_find_endpoint() */

	if (f->descriptors) {
		usb_free_descriptors(f->descriptors);
		f->descriptors = 0;
	}

	if (f->hs_descriptors) {
		usb_free_descriptors(f->hs_descriptors);
		f->hs_descriptors = 0;
	}
#endif
}

/* ------------------------------------------------------------------------- */
/*                         external interface                                */
/* ------------------------------------------------------------------------- */

struct fadc_function fadc_function_driver = {
	.func = {
		 .name = fadc_func_name,
		 .strings = fadc_strings,
		 .bind = fadc_bind,
		 .unbind = fadc_unbind,
		 .setup = fadc_setup,
		 .disable = fadc_disable,
		 .set_alt = fadc_set_alt,
		 .get_alt = fadc_get_alt,
		 .suspend = fadc_suspend,
		 .resume = fadc_resume,
		 },
	.playback = {
		     .channels = FADC_DEFAULT_CHANNELS_PLAYBACK,
		     .samplerate = FADC_DEFAULT_SAMPLERATE_PLAYBACK,
		     },
	.capture = {
		    .channels = FADC_DEFAULT_CHANNELS_CAPTURE,
		    .samplerate = FADC_DEFAULT_SAMPLERATE_CAPTURE,
		    },
};

#ifdef CONFIG_USB_ANDROID_VDC
module_param_named(adc_playback_channels, fadc_function_driver.playback.channels, ulong, S_IRUGO);
MODULE_PARM_DESC(adc_playback_channels, "(audio) playback / number of channels");

module_param_named(adc_playback_samplerate, fadc_function_driver.playback.samplerate, ulong, S_IRUGO);
MODULE_PARM_DESC(adc_playback_samplerate, "(audio) playback / samplerate");

module_param_named(adc_capture_channels, fadc_function_driver.capture.channels, ulong, S_IRUGO);
MODULE_PARM_DESC(adc_capture_channels, "(audio) capture / number of channels");

module_param_named(adc_capture_samplerate, fadc_function_driver.capture.samplerate, ulong, S_IRUGO);
MODULE_PARM_DESC(adc_capture_samplerate, "(audio) capture / samplerate");

static int fadc_bind_config(struct usb_configuration *c)
{
	int ret = 0;
	printk(KERN_ERR "%s\n", __FUNCTION__);

	ret = usb_add_function(c, &fadc_function_driver.func);
	if(ret) {
		info("adding audio function failed\n");
	}
	printk(KERN_ERR "%s: returning %d\n", __FUNCTION__, ret);
	return ret;
}

static struct android_usb_function fadc_function = {
	.name = "fadc",
	.bind_config = fadc_bind_config,
};

static int __init fadc_init(void)
{
	printk(KERN_INFO "fadc init\n");
	android_register_function(&fadc_function);
	return 0;
}
late_initcall(fadc_init);
#endif
