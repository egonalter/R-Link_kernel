/* si4703_attr.c
 *
 * Helper functions for the FM receiver.
 *
 * Copyright (C) 2008 TomTom BV <http://www.tomtom.com/>
 * Authors: Guillaume Ballet <Guillaume.Ballet@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/delay.h>

#include "si4703.h"
#include "fmreceiver.h"
#include "si470x_bit_fields.h"

#define	PK_ERR	YELL
#define PK_X	YELL

extern int	si470x_reg_write(u8);
extern int	si470x_reg_read(u8, u16 *);
extern void	si4703_fifo_purge(struct si4703dev *);
extern int 	si4703_fifo_put(struct si4703dev *, unsigned char *, int);
extern void si470x_reg_dump(u16 *lreg, int startreg, int endreg);

/* XXX - needed as long as things have not been moved here */
extern volatile u8 WaitSTCInterrupt;

int	si470x_tune(struct si4703dev *, __u16);
u16	freqToChan(struct si4703dev *, u16 freq);
u16	si470x_get_current_frequency(struct si4703dev *);
u8	si470x_get_current_rssi(struct si4703dev *);
u16	chanToFreq(struct si4703dev *, u16);


int
si470x_get_band(__u16 *band)
{
#if 0
	si470x_reg_read(NR_OF_REGISTERS, );
#endif

	return 0;
}

int
si470x_set_band(struct si4703dev	*gdev,
		__u16			band)
{
 u16 spacing=0;
 u16 deemphasis=0;

	ENTER();

	switch (band) {
		case 0U:
			/* Europe, 87.5 - 108.0 MHz, 100 kHz grid */
			band = 0x0000;
			spacing = 0x0010;
			deemphasis = 0x0800;
			break;
		case 1U:
			/* USA, 87.5 - 108.0 MHz, 200 kHz grid */
			band = 0x0000;
			spacing = 0x0000;
			deemphasis = 0x0000;
			break;
		case 2U:
			/* Japan, 76.0 - 90.0 MHz, 100 kHz grid */
			band = 0x0080;
			spacing = 0x0010;
			deemphasis = 0x0800;
			break;
		default:
			EXIT(-1);
	}

	gdev->shadow_regs.sysconfig2 = (gdev->shadow_regs.sysconfig2 & ~0x00FF) | band | spacing | 0x000F;
	gdev->shadow_regs.sysconfig1 = (gdev->shadow_regs.sysconfig1 & ~0x0800) | deemphasis;

	if (si470x_reg_write(4) != 0)
		EXIT(-1);

	/* Update the cached value */
	gdev->band	= band;

	EXIT(0);
}

int
si470x_set_frequency(struct si4703dev	*gdev,
			__u16		freq)
{
 struct fmr_rds_type	rdsoutdata;

	si4703_fifo_purge(gdev);
  
	if (si470x_tune(gdev, freq) != 0)
		EXIT(-1);
	else {
		rdsoutdata.sf_bandlimit = (unsigned char) ((gdev->shadow_regs.statusrssi & SI47_SFBL) >> 13);
		rdsoutdata.rssi = (unsigned char) si470x_get_current_rssi(gdev);
		rdsoutdata.freq = (unsigned short int) si470x_get_current_frequency(gdev);

		rdsoutdata.blocka = (unsigned short int) 0x00FF;	/* XXX guba : not sure why they bother doing this yet. */
		rdsoutdata.blockb = (unsigned short int) 0x0000;
		rdsoutdata.blockc = (unsigned short int) 0x0000;
		rdsoutdata.blockd = (unsigned short int) 0x0000;

		//XXX - fix it YELL("si470x_set_frequency: fifo_put f(%d) rssi(%d)", rdsoutdata.freq, rdsoutdata.rssi);
		si4703_fifo_put(gdev, (u8 *) &rdsoutdata, sizeof(struct fmr_rds_type));
		wake_up_interruptible(&gdev->wq);
	}

	/* Update the cached value */
	gdev->freq	= freq;

	EXIT(0);
}

#define preg_shadow	(&gdev->shadow_regs)
#define si470x_shadow	((u16 *) &gdev->shadow_regs)

int
si470x_tune(struct si4703dev	*gdev,
		__u16		freq)
{
  int ret = 0;
  u16 channel= 0;
  long timeout=0;
  int retry = 0;

	ENTER();
	// XXX fix it - YELL("freq=%d", freq);

  gdev->rds_bad		= 0;
  gdev->rds_good	= 0;
#ifdef RDSQ_LOGGING
  gdev->prev_rds_good	= 0;
  gdev->rdsq		= 0;
#endif
  channel = freqToChan(gdev, freq);

  // Write channel number to register 0x03 and set tune bit
  gdev->shadow_regs.channel = channel | SI47_TUNE;
#ifdef LOGGING
  YELL("si470x_tune: ");
  si470x_reg_dump(si470x_shadow, 0x00, 0x0F);
#endif

  WaitSTCInterrupt = 1;
	YELL("si470x_tune: set TUNE bit");
	if (si470x_reg_write(2) != 0)
		EXIT(-1);

  // Wait for stc bit to be set
  timeout = wait_event_interruptible_timeout(gdev->wq, (WaitSTCInterrupt == 0), TUNE_TIMEOUT * HZ);

  if (timeout == 0)
  {
    PK_ERR("ERROR STC interrupt missed\n");
#ifdef LOGGING
    //timeout, read back all registers
    if (si470x_reg_read(NR_OF_REGISTERS, si470x_shadow) != 0)
    {
      PK_ERR("ERROR @ %d, si470x_reg_read\n", __LINE__);
      return -1;
    }
    PK_X("si470x_tune: ");
    si470x_reg_dump(si470x_shadow, 0x00, 0x0F);
#endif
    ret = -1;
  }
  else
  {
    PK_X("si470x_tune: STC interrupt after %d s\n", (int)(TUNE_TIMEOUT - (timeout/HZ)));
  }
  
  // Write address 0x03 to clear tune bit
  gdev->shadow_regs.channel &= ~SI47_TUNE;
  PK_X("si470x_tune: clear TUNE bit, retry(%d)\n", retry);
  if (si470x_reg_write(2) != 0)
  {
    PK_ERR("ERROR @ %d, si470x_reg_write\n", __LINE__);
    return -1;
  }

  // Wait for stc bit to be cleared.  This step is very important. If it
  // is ignored and another seek or tune occurs too quickly, the tuner
  // will not set the STC bit properly.
  retry=0;
  if (si470x_reg_read(2, si470x_shadow) != 0)
  {
    PK_ERR("ERROR @ %d, si470x_reg_read\n", __LINE__);
    return -1;
  }
  while (((gdev->shadow_regs.statusrssi & SI47_STC) != 0) && (retry < 100))
  {
      if (si470x_reg_read(2, si470x_shadow) != 0)
      {
        PK_ERR("ERROR @ %d, si470x_reg_read\n", __LINE__);
        return -1;
      }
      msleep(5);
      retry++;
  }

  PK_X("si470x_tune return retry(%d) f(%d) rssi(%d) afcrl(%d)\n", 
         retry, 
         si470x_get_current_frequency(gdev), 
         (u8)(gdev->shadow_regs.statusrssi & SI47_RSSI), 
         (u8)(gdev->shadow_regs.statusrssi & SI47_AFCRL));

  return ret;
}

int
si470x_set_volume(struct si4703dev	*gdev,
			u16		volume)
{
	ENTER();

	volume &= 0x000F;
	preg_shadow->sysconfig2 = (preg_shadow->sysconfig2 & ~SI47_VOLUME) | volume;

	if (si470x_reg_write(4))
		EXIT(-1);

	/* Upon success, update the cached value. */
	gdev->volume	= volume;

	EXIT(0);
}

//-----------------------------------------------------------------------------
// Get current frequency fromn shadow registers
//-----------------------------------------------------------------------------
u16
si470x_get_current_frequency(struct si4703dev	*gdev)
{
	return (chanToFreq(gdev, preg_shadow->readchan & SI47_READCHAN));
}

//-----------------------------------------------------------------------------
// Get current rssi from shadow registers
//-----------------------------------------------------------------------------
u8
si470x_get_current_rssi(struct si4703dev	*gdev)
{
	si470x_reg_read(NR_OF_REGISTERS, si470x_shadow);

	if ((preg_shadow->statusrssi & SI47_AFCRL) == 1) {
		YELL("si470x_get_current_rssi: AFCRL=1 invalid channel f(%d)\n", si470x_get_current_frequency(gdev));
		preg_shadow->statusrssi &= ~SI47_RSSI;
	}

	return ((u8)(preg_shadow->statusrssi & SI47_RSSI));
}

//-----------------------------------------------------------------------------
// Converts from a channel number to a frequency value
//
// Inputs:
//  Channel number using current channel spacing
//
// Output:
//  Frequency in 10kHz steps
//-----------------------------------------------------------------------------
u16 chanToFreq(struct si4703dev	*gdev,
		u16		channel)
{
 u8 channelSpace, delta;
 u16 bottomOfBand;

	channelSpace = (preg_shadow->sysconfig2 & SI47_SPACE) >> 4;

	// Frequency calculation parameters retreival
	if((preg_shadow->sysconfig2 & SI47_BAND) == SI47_BAND_USA_EUR) {
		bottomOfBand = 8750;
	} else {
		bottomOfBand = 7600;
	}

	switch (channelSpace) {
		case 0x0:
			delta = 20;
			break;
		case 0x1:
			delta = 10;
			break;
		case 0x2:
			delta = 5;
			break;
		default:
			//ERROR
			delta = 0;
			break;
	}

	return (bottomOfBand + channel * delta);
}

u16 freqToChan(struct si4703dev	*gdev,
		u16		freq)
{
  u8 channelSpace, delta;
  u16 bottomOfBand;

	channelSpace = (preg_shadow->sysconfig2 & SI47_SPACE) >> 4;

	if((preg_shadow->sysconfig2 & SI47_BAND) == SI47_BAND_USA_EUR) {
		bottomOfBand = 8750;
	} else {
		bottomOfBand = 7600;
	}

	switch (channelSpace) {
		case 0x0:
			delta = 20;
			break;
		case 0x1:
			delta = 10;
			break;
		case 0x2:
			delta = 5;
			break;
		default:
			//ERROR
			delta = 1;
			break;
	}

	return ((freq - bottomOfBand) / delta);
}

//-----------------------------------------------------------------------------
// Configures the device for normal RDS operation
//-----------------------------------------------------------------------------
/* XXX - make it static / inline */
int
si470x_enable_rds(struct si4703dev	*gdev)
{
 int	err = 0;

	ENTER();

#ifdef RDSQ_LOGGING
	gdev->rds_timestamp_ms	= jiffies_to_msecs(jiffies);
#endif

	preg_shadow->sysconfig1 |= SI47_RDSIEN; /* set RDSI enable */
	preg_shadow->sysconfig1 |= SI47_RDS;    /* set RDS enable  */

	if (si470x_reg_write(3) != 0)
		err	= -1;

	gdev->rds_enabled	= 1;

	EXIT(0);
}

int
si470x_disable_rds(struct si4703dev	*gdev)
{
	ENTER();

	preg_shadow->sysconfig1 &= ~SI47_RDSIEN;  /* unset RDSI enable */
	preg_shadow->sysconfig1 &= ~SI47_RDS;     /* unset RDS enable  */

	if (si470x_reg_write(3) != 0)
		EXIT(-1);

	si4703_fifo_purge(gdev);

	gdev->rds_enabled	= 0;

	EXIT(0);
}

int
si470x_set_rds_enabled(struct si4703dev	*gdev,
			__u16		enable)
{
	if (enable)
		return si470x_enable_rds(gdev);
	else
		return si470x_disable_rds(gdev);
}

