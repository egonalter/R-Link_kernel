/*
 * Copyright (C) 2008 TomTom BV <http://www.tomtom.com/>
 * Author: Jeroen Taverne <jeroen.taverne@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
*/

#include <linux/device.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/i2c.h>
#include <linux/cdev.h>

#include <asm/arch/pwm.h>
#include <linux/si4711.h>

#include <linux/platform_device.h>

#define PFX "SI4711: "
#define PK_DBG(x...) printk(KERN_DEBUG PFX x)
#define PK_ERR(x...) printk(KERN_ERR PFX x)

struct si4711_tuneparam {
	unsigned short tunefreq;
	unsigned short rfdbuv;
	unsigned char antcap;
	unsigned char rnl;
};

struct si4711_property {
	const unsigned short property;
	unsigned short value;
};

struct si4711dev {
	struct cdev cdev;
	struct i2c_client *client;
} dev;

#ifdef CONFIG_PM
static int si4711_i2c_suspend(struct i2c_client *c, pm_message_t state);
static int si4711_i2c_resume(struct i2c_client *c);
#else
#define si4711_i2c_suspend	(NULL)
#define si4711_i2c_resume 	(NULL)
#endif /* CONFIG_PM     */
static int si4711_powersuspend(struct i2c_client *client);
static int si4711_powerresume(struct i2c_client *client);
static int si4711_i2c_probe(struct i2c_client *client);
static int si4711_i2c_remove(struct i2c_client *client);

static struct i2c_driver si4711_driver = {
	.id = I2C_DRIVERID_SI4711,
	.probe = si4711_i2c_probe,
	.remove = si4711_i2c_remove,
	.suspend = si4711_i2c_suspend,
	.resume = si4711_i2c_resume,
	.driver = {
		   .name = SI4711_DEVNAME,
		   },
};

/* Big endian short representation of frequency */
static enum fmtrx_state gFMState = fmtrx_off;

static struct si4711_priv *si4711_priv_copy;

static struct si4711_tuneparam suspend_tuneparam;
/* Note that Property 0x0103 is not in the list. This is because it's a digital property */
/* that seems to cause problems if get_property is used on it while in analog mode. */
static struct si4711_property suspend_property[] =
{	{0x0001, 0x0000},
	{0x0002, 0x0000},
	{0x0101, 0x0000}, 
	{0x0201, 0x0000},
	{0x0202, 0x0000}, 
	{0x2100, 0x0000},
	{0x2101, 0x0000}, 
	{0x2102, 0x0000},
	{0x2103, 0x0000}, 
	{0x2104, 0x0000},
	{0x2105, 0x0000}, 
	{0x2106, 0x0000},
	{0x2107, 0x0000}, 
	{0x2200, 0x0000},
	{0x2201, 0x0000}, 
	{0x2202, 0x0000},
	{0x2203, 0x0000}, 
	{0x2204, 0x0000},
	{0x2300, 0x0000}, 
	{0x2301, 0x0000},
	{0x2302, 0x0000}, 
	{0x2303, 0x0000},
	{0x2304, 0x0000}, 
	{0xFFFF, 0xFFFF}
};

static void si4711_enable_reset(void)
{
	IO_Activate(&(si4711_priv_copy->reset_pin));
}

static void si4711_disable_reset(void)
{
	IO_Deactivate(&(si4711_priv_copy->reset_pin));
}

static void si4711_pulse_reset(void)
{
	IO_Deactivate(&(si4711_priv_copy->tx_int_pin));
	si4711_enable_reset();
	udelay(500);
	si4711_disable_reset();
	udelay(500);
	IO_SetInput(&(si4711_priv_copy->tx_int_pin));
}

static int clock_enabled = 0;
static struct pwm_dev * pwm;

static void si4711_enable_clock(void)
{
	if (!clock_enabled) {
		pwm = sirfsoc_pwm_acquire(si4711_priv_copy->pwm_channel, 32768,NULL);	/* Frequency setting is ignored by PWM controller because RTC crystal is selected for channel 4 */
		pwm->interfaces->set_duty_period(pwm, 50);
		pwm->interfaces->start(pwm);
		clock_enabled = 1;
	}
}

static void si4711_disable_clock(void)
{
	if (clock_enabled) {
		pwm->interfaces->stop(pwm);
		sirfsoc_pwm_release(pwm);
		clock_enabled = 0;
	}
}

static void si4711_inithw(void)
{
	si4711_pulse_reset();
	si4711_enable_clock();
}

static void si4711_sleephw(void)
{
	si4711_disable_clock();
	si4711_enable_reset();
}

static int si4711_save_properties(struct i2c_client *client,
				  struct si4711_property *properties)
{
	int count = 0;
	unsigned char getprop[4] = { 0x13, 0x00, 0x00, 0x00 };
	unsigned char recvbuf[4];
	int rc;
	int retry = 5;

	/* Get all entries in the array. */
	while (properties[count].property != 0xFFFF) {
		getprop[2] =
		    ((unsigned char)((properties[count].property & 0xFF00) >>
				     8));
		getprop[3] =
		    ((unsigned char)(properties[count].property & 0x00FF));

		retry = 5;
		do {
			retry--;
			rc = i2c_master_send(client, getprop, sizeof(getprop));
			if (rc < 0)
				return rc;
			mdelay(1);

			rc = i2c_master_recv(client, recvbuf, sizeof(recvbuf));
			if (rc < 0)
				return rc;
			mdelay(1);
		}
		while (((recvbuf[0] & 0xC0) != 0x80) && (retry >= 0));

		if ((retry < 0) && ((recvbuf[0] & 0xC0) != 0x80))
			return -EINVAL;

		properties[count].value =
		    (((unsigned short int)recvbuf[2]) << 8) |
		    ((unsigned short int)recvbuf[3]);
		count += 1;
	}

	/* Done. */
	return 0;
}

static int si4711_restore_properties(struct i2c_client *client,
				     struct si4711_property *properties)
{
	int count = 0;
	unsigned char setprop[6] = { 0x12, 0x00, 0x00, 0x00, 0x00, 0x00 };
	int rc;

	/* Set all entries in the array. */
	while (properties[count].property != 0xFFFF) {
		setprop[2] = ((unsigned char)((properties[count].property & 0xFF00) >> 8));
		setprop[3] = ((unsigned char)(properties[count].property & 0x00FF));
		setprop[4] = ((unsigned char)((properties[count].value & 0xFF00) >> 8));
		setprop[5] = ((unsigned char)(properties[count].value & 0x00FF));

		rc = i2c_master_send(client, setprop, sizeof(setprop));
		if (rc < 0)
			return rc;

		mdelay(50);

		count += 1;
	}

	/* Done. */
	return 0;
}

static int si4711_save_tuneparam(struct i2c_client *client,
				 struct si4711_tuneparam *tuneparam)
{
	int rc;
	unsigned char recvbuf[8];
	unsigned char txtunestatus[2] = { 0x33, 0x00 };
	int retry = 5;

	/* Get the frequency params. */
	do {
		retry--;
		rc = i2c_master_send(client, txtunestatus,
				     sizeof(txtunestatus));
		if (rc < 0)
			return rc;

		rc = i2c_master_recv(client, recvbuf, sizeof(recvbuf));
		if (rc < 0)
			return rc;
	}
	while (((recvbuf[0] & 0xC0) != 0x80) && (retry >= 0));

	if ((retry < 0) && ((recvbuf[0] & 0xC0) != 0x80))
		return -EINVAL;

	/* Save them. */
	tuneparam->tunefreq =
	    (((unsigned short)recvbuf[2]) << 8) | ((unsigned short)recvbuf[3]);
	tuneparam->rfdbuv =
	    (((unsigned short)recvbuf[4]) << 8) | ((unsigned short)recvbuf[5]);
	tuneparam->antcap = recvbuf[6];
	tuneparam->rnl = recvbuf[7];
	return 0;
}

static int si4711_restore_tuneparam(struct i2c_client *client,
				    struct si4711_tuneparam *tuneparam)
{
	int rc;
	unsigned char txtunefreq[4] = { 0x33, 0x00, 0x00, 0x00 };
	unsigned char txtunepwr[5] = { 0x31, 0x00, 0x00, 0x00, 0x00 };

	/* Set the frequency params. */
	txtunefreq[2] = ((unsigned char)((tuneparam->tunefreq & 0xFF00) >> 8));
	txtunefreq[3] = ((unsigned char)(tuneparam->tunefreq & 0x00FF));
	txtunepwr[2] = ((unsigned char)((tuneparam->rfdbuv & 0xFF00) >> 8));
	txtunepwr[3] = ((unsigned char)(tuneparam->rfdbuv & 0x00FF));
	txtunepwr[4] = 0;	/* auto */

	rc = i2c_master_send(client, txtunefreq, sizeof(txtunefreq));
	if (rc < 0)
		return rc;

	PK_DBG("%s writing txtunepwr %02x %02x %02x %02x %02x\n", __func__,
	       txtunepwr[0], txtunepwr[1], txtunepwr[2], txtunepwr[3],
	       txtunepwr[4]);
	rc = i2c_master_send(client, txtunepwr, sizeof(txtunepwr));
	if (rc < 0)
		return rc;

	/* Done. */
	return 0;
}

static int si4711_powerdown(struct i2c_client *c)
{
	int rc = -ENOTTY;
	unsigned char pwrdown[] = { 0x11 };
	unsigned char settunepwr[] = { 0x31, 0x00, 0x00, 0x00, 0x00 };

	PK_DBG("%s writing txtunepwr %02x %02x %02x %02x %02x\n", __func__,
	       settunepwr[0], settunepwr[1], settunepwr[2], settunepwr[3],
	       settunepwr[4]);
	rc = i2c_master_send(c, settunepwr, sizeof(settunepwr));
	if (rc > 0)
		rc = 0;		/* no error if amount of bytes is returned */
	mdelay(10);

	rc = i2c_master_send(c, pwrdown, sizeof(pwrdown));
	if (rc > 0)
		rc = 0;		/* no error if amount if bytes is returned */
	mdelay(10);

	si4711_sleephw();

	gFMState = fmtrx_off;

	return rc;
}

static int si4711_settunepwr(struct i2c_client *c, unsigned char power)
{
	int rc;
	unsigned char settunepwr[] = { 0x31, 0x00, 0x00, power, 0x00 };

	PK_DBG("%s writing txtunepwr %02x %02x %02x %02x %02x\n", __func__,
	       settunepwr[0], settunepwr[1], settunepwr[2], settunepwr[3],
	       settunepwr[4]);
	rc = i2c_master_send(c, settunepwr, sizeof(settunepwr));
	mdelay(50);

	return rc;
}

#if 0

static void si4711_test(struct i2c_client *c)
{
	static unsigned int TX_freq = 10100; 

	const unsigned char power_up_cmd[]={0x01, 0x02, 0x50};
	const unsigned char set_tune_power_cmd[]={0x31, 0x0, 0x0, 0x78,0 };

	const unsigned char gpio_ien_cmd[]=			{0x12, 0x00, 0x00, 0x01, 0x00, 0x00};
	const unsigned char gpio_cfg_cmd[]=			{0x12, 0x00, 0x00, 0x02, 0x00, 0x00};
	const unsigned char rclk_freq_cmd[]=			{0x12, 0x00, 0x02, 0x01, 0x80, 0x00};
	const unsigned char rclk_prescale_cmd[]=		{0x12, 0x00, 0x02, 0x02, 0x00, 0x01};
	const unsigned char tx_component_cmd[]=			{0x12, 0x00, 0x21, 0x00, 0x00, 0x03};
	const unsigned char tx_audio_deviation_cmd[] =		{0x12, 0x00, 0x21, 0x01, 0x1a, 0xc2};
	const unsigned char tx_line_inout_level_cmd[]=		{0x12, 0x00, 0x21, 0x02, 0x02, 0xa8};
	const unsigned char tx_line_input_level_cmd[]=		{0x12, 0x00, 0x21, 0x04, 0x32, 0x7C};
	const unsigned char tx_input_mute_cmd[]=		{0x12, 0x00, 0x21, 0x05, 0x00, 0x00};
	const unsigned char tx_preemphasis_cmd[]=		{0x12, 0x00, 0x21, 0x06, 0x00, 0x00};
	const unsigned char tx_accomp_en_cmd[]=			{0x12, 0x00, 0x22, 0x00, 0x00, 0x00};
	const unsigned char tx_accomp_thrd_cmd[]=		{0x12, 0x00, 0x22, 0x01, 0xFF, 0x60};
	const unsigned char tx_accomp_attch_time_cmd[]=		{0x12, 0x00, 0x22, 0x02, 0x00, 0x00};
	const unsigned char tx_accomp_release_time_cmd[]=	{0x12, 0x00, 0x22, 0x03, 0x00, 0x01};
	const unsigned char tx_accomp_gain_cmd[]=		{0x12, 0x00, 0x22, 0x04, 0x00, 0x3C};

	unsigned char tx_tune_power_cmd[]={0x31, 0x00, 0x00, 0, 0};
	const unsigned char tx_pwr_off[]={0x11};

	unsigned char tune_freq_cmd[]={0x30, 0x00,0, 0};
	static unsigned char TX_power = 120;  //88~120 dBuV
	static unsigned char ANT_TUNE_CAP = 0;  //1~191   ,0 for auto

	i2c_master_send(c, power_up_cmd, sizeof(power_up_cmd));
	mdelay(100);
	i2c_master_send(c, set_tune_power_cmd, sizeof(set_tune_power_cmd));
	mdelay(100);
	i2c_master_send(c, gpio_ien_cmd, sizeof(gpio_ien_cmd));
	mdelay(100);
	i2c_master_send(c, gpio_cfg_cmd, sizeof(gpio_cfg_cmd));
	mdelay(100);
	i2c_master_send(c, rclk_freq_cmd, sizeof(rclk_freq_cmd));
	mdelay(100);
	i2c_master_send(c, rclk_prescale_cmd, sizeof(rclk_prescale_cmd));
	mdelay(100);
	i2c_master_send(c, tx_component_cmd, sizeof(tx_component_cmd));
	mdelay(100);
	i2c_master_send(c, tx_audio_deviation_cmd, sizeof(tx_audio_deviation_cmd));
	mdelay(100);
	i2c_master_send(c, tx_line_inout_level_cmd, sizeof(tx_line_inout_level_cmd));
	mdelay(100);
	i2c_master_send(c, tx_line_input_level_cmd, sizeof(tx_line_input_level_cmd));
	mdelay(100);
	i2c_master_send(c, tx_input_mute_cmd, sizeof(tx_input_mute_cmd));
	mdelay(100);
	i2c_master_send(c, tx_preemphasis_cmd, sizeof(tx_preemphasis_cmd));
	mdelay(100);
	i2c_master_send(c, tx_accomp_en_cmd, sizeof(tx_accomp_en_cmd));
	mdelay(100);
	i2c_master_send(c, tx_accomp_thrd_cmd, sizeof(tx_accomp_thrd_cmd));
	mdelay(100);
	i2c_master_send(c, tx_accomp_attch_time_cmd, sizeof(tx_accomp_attch_time_cmd));
	mdelay(100);
	i2c_master_send(c, tx_accomp_release_time_cmd, sizeof(tx_accomp_release_time_cmd));
	mdelay(100);
	i2c_master_send(c, tx_accomp_gain_cmd, sizeof(tx_accomp_gain_cmd));
	mdelay(100);

	tune_freq_cmd[2]=(TX_freq>>8&0xFF);
	tune_freq_cmd[3]=(TX_freq&0xFF);
  
	i2c_master_send(c, tune_freq_cmd, sizeof(tune_freq_cmd));
	mdelay(100);

	tx_tune_power_cmd[3] = TX_power&0xFF;
	tx_tune_power_cmd[4] = ANT_TUNE_CAP&0xFF;
  
	i2c_master_send(c, tx_tune_power_cmd, sizeof(tx_tune_power_cmd));
	mdelay(100);

	printk("****************************** FM should transmit now\n");

	while(1);
}

#endif

static int si4711_powerup(struct i2c_client *c)
{
	int rc = -ENOTTY;
	unsigned char pwrup[] = { 0x01, 0xc2, 0x50 };
	unsigned char get_rev[] = { 0x10 };
	unsigned char setfreq[] = { 0x30, 0x00, 0x27, 0x56 };
	unsigned char getstate[] = { 0x33, 0x01 };
	unsigned char buf[9];
	struct si4711_property startup_property[] =
	{	
		{0x2100, 0x0003},
		{0x2101, 6825},
		{0x2102, 675},
		{0x2104, ((0x3 << 12) | (600 << 0))},
		{0x2301, -15},
		{0x2300, 0x0001}, 
		{0x2302, 5},
		{0x2204, 20},
/* Undefine this if you want the Audio Dynamic Range Control feature default enabled. */
#if 1
		{2200, 0x0003},
#endif
		{0x0001, 0x0000},
		{0x0002, 0x0000},
		{0x0201, 0x8000},
		{0x0202, 0x0001},
		{0x2105, 0x0000},
		{0x2106, 0x0000},
		{0x2200, 0x0000},
		{0x2201, 0xff60},
		{0x2202, 0x0000},
		{0x2203, 0x0001},
		{0xFFFF, 0xFFFF}
	};

	PK_DBG("%s power up set freq=[%x][%x]\n", __func__, setfreq[2],
	       setfreq[3]);

	si4711_inithw();

	rc = i2c_master_send(c, pwrup, sizeof(pwrup));
	mdelay(100);
	rc = i2c_master_recv(c, buf, 1);
	if (rc != 1 || buf[0] != 0x80){
		PK_ERR("power up i2c command failed\n");
	}

	rc = i2c_master_send(c, get_rev, sizeof(get_rev));
	mdelay(200);
	rc = i2c_master_recv(c, buf, 9);
	if (rc != 9 || buf[0] != 0x80){
		PK_ERR("get revision command failed\n");
	}
	if (buf[1] != 0x0D){
		PK_ERR("Failed to read the part number expected 0x0D but got 0x%02u!!\n",buf[1]);
	}

	rc = si4711_settunepwr(c, 115);

	rc = i2c_master_recv(c, buf, 1);
	if (rc != 1 || buf[0] != 0x80){
		PK_ERR("tune power i2c command failed\n");
	}

	rc = i2c_master_send(c, setfreq, sizeof(setfreq));
	mdelay(100);
	rc = i2c_master_recv(c, buf, 1);
	if (rc != 1 || buf[0] != 0x80){
		PK_ERR("set freq i2c command failed\n");
	}

	rc = i2c_master_send(c, getstate, sizeof(getstate));
	mdelay(100);
	rc = i2c_master_recv(c, buf, 1);
	if (rc != 1 || buf[0] != 0x80){
		PK_ERR("getstate failed freq i2c command failed\n");
	}

	rc = si4711_restore_properties(c, startup_property);

	gFMState = fmtrx_on;
	return rc;
}

static int si4711_powersuspend(struct i2c_client *client)
{
	int rc = 0;

	if (gFMState == fmtrx_on) {
		rc = si4711_save_properties(client, suspend_property);
		if (rc < 0)
			return rc;

		rc = si4711_save_tuneparam(client, &suspend_tuneparam);
		if (rc < 0)
			return rc;

		rc = si4711_powerdown(client);

		gFMState = fmtrx_suspend;
	}
	return rc;
}

static int si4711_powerresume(struct i2c_client *client)
{
	int rc = 0;

	if (gFMState == fmtrx_suspend) {
		rc = si4711_powerup(client);
		if (rc < 0)
			return rc;

		rc = si4711_restore_tuneparam(client, &suspend_tuneparam);
		if (rc < 0)
			return rc;

		rc = si4711_restore_properties(client, suspend_property);
	}
	return rc;
}

static int si4711_SetFreq(struct i2c_client *c, unsigned int u32Freq)
{
	int rc = 0;
	unsigned char setfreq[] = { 0x30, 0x00, 0x27, 0xa6 };

	setfreq[2] = (unsigned char)(((u32Freq / 10000) & 0xff00) >> 8);
	setfreq[3] = (unsigned char)((u32Freq / 10000) & 0x00ff);

	if (fmtrx_on == gFMState) {
		rc = i2c_master_send(c, setfreq, sizeof(setfreq));
	}

	return rc;
}

#ifdef CONFIG_PM
static int si4711_i2c_suspend(struct i2c_client *c, pm_message_t state)
{
	int rc = 0;

	if (!rc)
		rc = si4711_powersuspend(c);
	if (rc) {
		PK_DBG("%s error while doing powersuspend: rc = %d\n", __func__,
		       rc);
	}

	/* Never return an error here. It will block the kernel from going into suspend further
	 * and the FM transmitter is not important enough to block that!
	 */
	//return rc;
	return 0;
}

static int si4711_i2c_resume(struct i2c_client *c)
{
	int rc = 0;

	if (!rc)
		rc = si4711_powerresume(c);
	if (rc) {
		PK_DBG("%s error while doing powerresume: rc = %d\n", __func__,
		       rc);
	}

	return 0;
}
#endif /* CONFIG_PM     */

static int si4711_open(struct inode *nodep, struct file *filep)
{
	struct si4711dev *dev =
	    container_of(nodep->i_cdev, struct si4711dev, cdev);

	filep->private_data = dev;	/* for easy reference */

	return 0;
}

static int si4711_release(struct inode *nodep, struct file *filep)
{
	if (filep && filep->private_data) {
		filep->private_data = NULL;
	}
	return 0;
}

static int si4711_ioctl(struct inode *nodep, struct file *filep,
			unsigned int cmd, unsigned long arg)
{
	struct si4711dev *dev = filep ? filep->private_data : NULL;
	struct si4711_property dynrange_property_on[] =
	    { {0x2201, 0xFFD8}, {0x2202, 0x0000}, {0x2203, 4}, {0x2200, 0x0001},
	{0xFFFF, 0xFFFF}
	};
	struct si4711_property dynrange_property_off[] =
	    { {0x2200, 0x0000}, {0xFFFF, 0xFFFF} };
	struct si4711_property stereo_property_on[] =
	    { {0x2100, 0x0003}, {0xFFFF} };
	struct si4711_property stereo_property_off[] =
	    { {0x2100, 0x0000}, {0xFFFF} };
	struct si4711_property silent_mode_property_on[] =
	    { {0x2300, 0x0001}, {0xFFFF} };
	struct si4711_property silent_mode_property_off[] =
	    { {0x2300, 0x0000}, {0xFFFF} };
	struct si4711_property adrc_property_on[] =
	    { {0x2200, 0x0003}, {0xFFFF} };
	struct si4711_property adrc_property_off[] =
	    { {0x2200, 0x0002}, {0xFFFF} };

	int rc = -ENOTTY;

	PK_DBG("ioctl cmd=0x%x\n", cmd);
	if (_IOC_TYPE(cmd) != FMTRANSMITTER_DRIVER_MAGIC) {
		PK_ERR("Wrong IOC type! Failing command");
		return -ENOTTY;
	}

	switch (cmd) {
	case IOW_SET_FM_FREQUENCY:
		{
			unsigned int u32Freq = (unsigned int)arg;
			PK_DBG("ioctl cmd=set frequency %d\n",
			       (unsigned int)arg);
			rc = si4711_SetFreq(dev->client, u32Freq);
		}
		break;
	case IOW_ENABLE:
		{
			unsigned char bEnable = (unsigned char)arg;
			PK_DBG("ioctl cmd=%s\n",
			       ((unsigned int)arg == 0) ? "disable" : "enable");
			if (bEnable) {
				rc = si4711_powerresume(dev->client);
			} else {
				rc = si4711_powersuspend(dev->client);
			}
		}
		break;

	case IOW_FMTRX_SET_MONO_STEREO:
		{
			unsigned char bEnable = (unsigned char)arg;
			if (bEnable)
				rc = si4711_restore_properties(dev->client,
							       stereo_property_on);
			else
				rc = si4711_restore_properties(dev->client,
							       stereo_property_off);
		}
		break;

	case IOW_FMTRX_TADRC_ENABLE:
		{
			unsigned char bEnable = (unsigned char)arg;
			if (bEnable)
				rc = si4711_restore_properties(dev->client,
							       dynrange_property_on);
			else
				rc = si4711_restore_properties(dev->client,
							       dynrange_property_off);
		}
		break;

	case IOW_FMTRX_SILENT_MODE_ENABLE:
		{
			unsigned char bEnable = (unsigned char)arg;
			if (bEnable)
				rc = si4711_restore_properties(dev->client,
							       silent_mode_property_on);
			else
				rc = si4711_restore_properties(dev->client,
							       silent_mode_property_off);
		}
		break;
	case IOR_FM_GET_STATE:
		/* Save the state. */
		*((int *)arg) = gFMState;
		break;

	case IOW_FMTRX_SET_ADRC:
		{
			/* Enable the ADRC. */
			unsigned char bEnable = (unsigned char)arg;
			if (bEnable)
				rc = si4711_restore_properties(dev->client,
							       adrc_property_on);
			else
				rc = si4711_restore_properties(dev->client,
							       adrc_property_off);
		}
		break;

	case IOW_FMTRX_SET_POWER:
		{
			/* Set power mode. */
			unsigned char power = (unsigned char)arg;
			if ((power >= 88) || (power <= 120)) {
				rc = si4711_settunepwr(dev->client, power);
				PK_DBG("ioctl cmd=powermode=%d %s\n", power,
				       (rc < 0) ? "FAILED" : "OK");
			}
		}
		break;

#ifdef SI4711_SW_TEST
	case IOW_SUSPEND:
		rc = si4711_powersuspend(dev->client);
		break;
	case IOW_RESUME:
		rc = si4711_powerresume(dev->client);
		break;
#endif /*  SI4711_SW_TEST       */
	default:
		break;
	}
	return rc;
}

static unsigned int si4711_poll(struct file *filep,
				struct poll_table_struct *pq)
{
	// FM transmitter can neither be written nor read, 
	// the poll() routine should always indicate 
	// that it is neither possible to read nor to write to the driver.
	PK_DBG("%s\n", __func__);
	return 0;
}

static struct file_operations si4711_fops = {
	.owner = THIS_MODULE,
	.open = si4711_open,
	.release = si4711_release,
	.ioctl = si4711_ioctl,
	.poll = si4711_poll,
};


static int si4711_i2c_probe(struct i2c_client *client)
{
	int rc = 0;

	PK_DBG("probe\n");
	memset(&dev, 0, sizeof(dev));
	i2c_set_clientdata(client, &dev);

	/* Get platform data */
	si4711_priv_copy = (struct si4711_priv *)client->dev.platform_data;

	/* Now setup our character device to the fm */
	cdev_init(&dev.cdev, &si4711_fops);
	dev.cdev.owner = THIS_MODULE;
	dev.cdev.ops = &si4711_fops;
	if ((rc = cdev_add(&dev.cdev, si4711_priv_copy->device_nr, 1)))
		goto no_cdev;

	PK_DBG("%s: %p\n", __func__, client);

	rc = si4711_powerup(client);
	if (rc < 0)
		goto no_cdev;

	dev.client = client;

	return si4711_powersuspend(client);

      no_cdev:
	PK_ERR("%s: ERROR %d!\n", __func__, rc);

	return rc;
}

static int si4711_i2c_remove(struct i2c_client *client)
{
	si4711_sleephw();
	return 0;
}

static int __init si4711_init(void)
{
	return i2c_add_driver(&si4711_driver);
}

static void __exit si4711_exit(void)
{
	i2c_del_driver(&si4711_driver);
}

MODULE_AUTHOR("Jeroen Taverne <jeroen.taverne@tomtom.com>");
MODULE_DESCRIPTION("SI4711 FM Transmitter driver");
MODULE_LICENSE("GPL");

module_init(si4711_init);
module_exit(si4711_exit);
