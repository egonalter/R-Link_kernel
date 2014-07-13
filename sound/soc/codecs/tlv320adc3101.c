/*
 * ALSA SoC TLV320ADC3101 tlv_adc driver
 *
 * Author:      Vladimir Barinov, <vbarinov@embeddedalley.com>
 * Copyright:   (C) 2007 MontaVista Software, Inc., <source@mvista.com>
 *
 * Based on sound/soc/tlv_adcs/wm8753.c by Liam Girdwood
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Notes:
 *
 */

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/pm.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <sound/core.h>
#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>
#include <sound/tlv.h>
#include <sound/initval.h>
#include <asm/div64.h>
//#include <linux/gpio.h>


#include "tlv320adc3101.h"
#include <plat/tlv320adc3101_pdata.h>
#define ADC3101_VERSION "0.0"

struct i2c_client *tlv_adc;


int tlv320adc3101_reg_read(uint8_t reg, uint8_t *val)
{
     int ret;

        ret = i2c_smbus_read_byte_data(tlv_adc, reg);

        if (ret < 0){
         	printk("failed reading at 0x%02x\n", reg);
                return ret;
        }
        *val = (uint8_t)ret;
        return 0;
}
EXPORT_SYMBOL_GPL(tlv320adc3101_reg_read);

int tlv320adc3101_reg_write(struct i2c_client *i2c,uint8_t reg, uint8_t val)
{
	 int ret;
        ret = i2c_smbus_write_byte_data(tlv_adc, reg, val);
        if (ret < 0) {
                printk("failed writing 0x%02x err=%d\n",reg,ret);
                return ret;
        }
        return 0;
}
EXPORT_SYMBOL_GPL(tlv320adc3101_reg_write);

static int  tlv320adc3101_hw_params(struct snd_pcm_substream *substream,
                                struct snd_pcm_hw_params *params,
                                struct snd_soc_dai *dai)
{
return 0 ;
}
static int tlv320adc3101_mute(struct snd_soc_dai *dai, int mute)
{
	return 0;
}

tlv320adc3101_set_dai_clkdiv(struct snd_soc_dai *dai, int div_id, int div)
{
	return 0;
}
tlv320adc3101_set_dai_fmt(struct snd_soc_dai *dai, unsigned int fmt)
{
	return 0;
}

static struct snd_soc_dai_ops tlv320adc3101_dai_ops_capture = {
        .hw_params      = tlv320adc3101_hw_params,
        .digital_mute = tlv320adc3101_mute,
        .set_clkdiv = tlv320adc3101_set_dai_clkdiv,
        .set_fmt = tlv320adc3101_set_dai_fmt,
};

#define ADC3101_FORMATS (SNDRV_PCM_FMTBIT_S16_LE | SNDRV_PCM_FMTBIT_S20_3LE | \
                         SNDRV_PCM_FMTBIT_S24_3LE | SNDRV_PCM_FMTBIT_S32_LE)

struct snd_soc_dai tlv320adc3101_dai = {
        .name = "tlv320adc3101",
        .id = 1,
        .capture = {
                .stream_name = "Capture",
                .channels_min = 1,
                .channels_max = 2,
                .rates = SNDRV_PCM_RATE_8000_96000,
                .formats = ADC3101_FORMATS,
        },
        .ops = &tlv320adc3101_dai_ops_capture,
        
};
EXPORT_SYMBOL_GPL(tlv320adc3101_dai);

static int tlv320adc3101_probe(struct i2c_client *i2c,
                                        const struct i2c_device_id *id)
{
	tlv320adc3101_platform_t	* pdata = 
			(tlv320adc3101_platform_t *)i2c->dev.platform_data;	
				
	int ret,val=0,error;
	int reg_num = 0;
	tlv_adc = i2c;
	
	if (NULL != pdata) {
		//codec_private_data->pdata = pdata;
	} else {
		printk(KERN_ERR "ADC3101: No platform data!\n");
		return -EINVAL;
	}

	/* Switch on both codec and amplifier */
	if ((ret = pdata->request_gpio())) {
		printk(KERN_ERR "ADC3101: Cannot request GPIO\n");
		return ret;
	}
	pdata->config_gpio();
	pdata->mic_reset(1);
	udelay(100);
	pdata->mic_reset(0);
	  
  tlv320adc3101_dai.dev = &i2c->dev;
  
	ret = snd_soc_register_dai(&tlv320adc3101_dai);
	if (ret != 0) { 
                printk("TLV320ADC: Failed to register DAI: %d\n", ret);
        }
        
        tlv320adc3101_reg_write(tlv_adc, ADC3101_PAGE_SELECT, PAGE0_SELECT);
        tlv320adc3101_reg_write(tlv_adc, ADC3101_RESET, SOFT_RESET);


        tlv320adc3101_reg_write(tlv_adc, ADC3101_PAGE_SELECT, PAGE0_SELECT);

        tlv320adc3101_reg_write(tlv_adc, ADC3101_ADC_NADC_VAL, 0x81);
        tlv320adc3101_reg_write(tlv_adc, ADC3101_ADC_MADC_VAL, 0x82);
        tlv320adc3101_reg_write(tlv_adc, ADC3101_ADC_INT_CONT2, 0x00);
        tlv320adc3101_reg_write(tlv_adc, ADC3101_ADC_INST_SET, 0x01);

        /* Select Page 1 */
        tlv320adc3101_reg_write(tlv_adc, ADC3101_PAGE_SELECT, PAGE1_SELECT);

        tlv320adc3101_reg_write(tlv_adc, ADC3101_MICBIAS_CONTROL, 0x50);

        tlv320adc3101_reg_write(tlv_adc, ADC3101_LADC_INPUT_LPGA1, 0xcf);
        tlv320adc3101_reg_write(tlv_adc, ADC3101_RADC_INPUT_RPGA1, 0xcf);

        tlv320adc3101_reg_write(tlv_adc, ADC3101_LPGA_SETTING, 0x50);
        tlv320adc3101_reg_write(tlv_adc, ADC3101_RPGA_SETTING, 0x50);

        tlv320adc3101_reg_write(tlv_adc, ADC3101_PAGE_SELECT, PAGE0_SELECT);

        tlv320adc3101_reg_write(tlv_adc, ADC3101_PAGE_SELECT, PAGE0_SELECT);
        tlv320adc3101_reg_write(tlv_adc, ADC3101_ADC_DIGITAL, 0xC2);

        tlv320adc3101_reg_write(tlv_adc, ADC3101_ADC_INT_CONT1, 0x00);

        tlv320adc3101_reg_write(tlv_adc, ADC3101_ADC_VOL_CNT, 0x00);

	return 0;
}

static int tlv320adc3101_remove(struct i2c_client *i2c)
{
	tlv320adc3101_platform_t	* pdata = 
			(tlv320adc3101_platform_t *)i2c->dev.platform_data;	
			
			
			pdata->free_gpio();
			
        return 0;
}

#if 0
int tlv320adc3101_init_tx(void *)
{
	return 0;
}
EXPORT_SYMBOL_GPL(tlv320adc3101_dai);
#endif
static const struct i2c_device_id tlv320adc3101_i2c_id[] = {
        { TLV320ADC3101_DEVNAME, 0 },
        { }
};
MODULE_DEVICE_TABLE(i2c, tlv320adc3101_i2c_id);




static struct i2c_driver tlv320adc3101_driver = {
        .driver = {
                .name   = TLV320ADC3101_DEVNAME,
                .owner  = THIS_MODULE,
        },
        .probe          = tlv320adc3101_probe,
        .remove         = tlv320adc3101_remove,
        .id_table       = tlv320adc3101_i2c_id,
};

static int __init tlv320adc3101_init(void)
{
	
	      printk("tlv320adc3101_init\n");
	      
        return i2c_add_driver(&tlv320adc3101_driver);
}
module_init(tlv320adc3101_init);

static void __exit tlv320adc3101_exit(void)
{
        i2c_del_driver(&tlv320adc3101_driver);
}
module_exit(tlv320adc3101_exit)
/* power down chip */



MODULE_AUTHOR("Jongbin Won <jongbin.won@samsung.com>");
MODULE_DESCRIPTION("ASoC TLV320ADC3101 tlv_adc driver");
MODULE_LICENSE("GPL");
