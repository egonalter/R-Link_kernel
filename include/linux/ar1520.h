/*
* Copyright (c) 2011 Atheros Communications Inc.
*
* Permission to use, copy, modify, and/or distribute this software for any
* purpose with or without fee is hereby granted, provided that the above
* copyright notice and this permission notice appear in all copies.
*
* THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
* WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
* ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
* WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
* ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
* OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/


#ifndef LINUX_AR1520_H
#define LINUX_AR1520_H

struct ar1520_platform_data {
	unsigned gps_gpio_rts;
	unsigned gps_gpio_wakeup;
	unsigned gps_gpio_reset;
};

struct ar1520_ioctl {
	unsigned value;
};

#define AR1520_DEV              "ar1520"
#define AR1520_IOCTL_MAGIC      0xc2
#define AR1520_IOCTL_MAXNR      5
#define AR1520_IOCTL_RESET      _IO(AR1520_IOCTL_MAGIC, 1)
#define AR1520_IOCTL_WAKEUP     _IO(AR1520_IOCTL_MAGIC, 2)
#define AR1520_IOCTL_GET_RTS    \
	_IOWR(AR1520_IOCTL_MAGIC, 3, struct ar1520_ioctl)
#define AR1520_IOCTL_ENABLE_IRQ \
	_IOW(AR1520_IOCTL_MAGIC, 4, struct ar1520_ioctl)
#define AR1520_IOCTL_DEBUG_LEVEL \
	_IOW(AR1520_IOCTL_MAGIC, 5, struct ar1520_ioctl)
#endif
