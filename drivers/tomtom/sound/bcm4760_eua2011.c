#include <linux/init.h>
#include <linux/module.h>

#include <sound/pcm.h>
#include <sound/pcm_params.h>
#include <sound/soc.h>
#include <sound/soc-dapm.h>

#include "asm/arch/hardware.h"
#include <asm/io.h>

#include <linux/broadcom/halaudio.h>

#include <plat/scenarii.h>
#include <plat/tt_setup_handler.h>

#include "../../../sound/soc/codecs/bcm476x_codec.h"

#define DUMMY_CALLBACK(name)static int dummy_##name(struct tt_scenario_context *c) \
				{							\
					/* Do nothing */				\
					return 0;					\
				}

#define BCM4760_MINGAIN (0)
#define BCM4760_MAXGAIN (42)
#ifdef PREAMP_ENABLED
	#define PREAMPGAIN (15)
#else
	#define PREAMPGAIN (0)
#endif
#define OUTPUT_ATT_5V_BOOSTER (8) /* ~1.5W (-2.00dB) normal max output level */
#define OUTPUT_ATT_NO_5V_BOOSTER (17) /* ~1.0W (-3.00dB) normal max output level */
#define OUTPUT_HF_ATT_5V_BOOSTER (3) /* ~1.5W  (-0.75dB) normal max output level without distortion */
#define OUTPUT_HF_ATT_NO_5V_BOOSTER (9) /* ~1.0W  (-2.25dB) normal max output level without distortion */
#define VOLUME_MUTE (90)

#include <plat/fdt.h>
static int bcm4760_set_volume_db(struct tt_scenario_context *c)
{
	struct snd_kcontrol	*kcontrol = (struct snd_kcontrol *) c->data;
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	int output_gain = (0x1ff - (120 << 2));
	int VCFGR_gain = 0x00;

	if (c->tt_volume_dB < VOLUME_MUTE) {
		switch(c->tt_scenario) {
		default:
		case TT_SCENARIO_AUDIO_INIT:
		case TT_SCENARIO_AUDIO_OFF:
			output_gain = (0x1ff - (120 << 2));
			break;
		case TT_SCENARIO_HF_LINEOUT:
		case TT_SCENARIO_HF_SPEAKER:
			/* 0 db, fixed line out level, SW volume level control by HF lib */
			if (c->has_5V_booster == 0) {
				output_gain = (0x01ff - OUTPUT_HF_ATT_NO_5V_BOOSTER);
			} else {
				output_gain = (0x01ff - OUTPUT_HF_ATT_5V_BOOSTER);
			}
			break;
		case TT_SCENARIO_SPEAKER:
			if (c->has_5V_booster == 0) {
				output_gain = ((0x1ff - OUTPUT_ATT_NO_5V_BOOSTER) - (c->tt_volume_dB << 2));
			} else {
				output_gain = ((0x1ff - OUTPUT_ATT_5V_BOOSTER) - (c->tt_volume_dB << 2));
			}
			c->tt_scenario_speaker_volume = output_gain;
			break;
		}
	}

	/* ensure voice channel is always at same level as audio channel */
	snd_soc_update_bits(codec, REG_VCFGR_ADDR, 0xffff, VCFGR_gain);

	/* ensure mixer level is always set to best performance */
	snd_soc_update_bits(codec, REG_MIXER_GAIN_CHSEL_ADDR, 0xffff, 0);	
	snd_soc_update_bits(codec, REG_MIXER_GAIN_ADJUST_ADDR, 0xffff, 0x2000);	
	snd_soc_update_bits(codec, REG_MIXER_GAIN_CHSEL_ADDR, 0xffff, 1);	
	snd_soc_update_bits(codec, REG_MIXER_GAIN_ADJUST_ADDR, 0xffff, 0x2000);	

 	/* set attenuation level for audio left and right channels and voice channel */
	snd_soc_update_bits(codec, REG_ALSLOPGAIN_ADDR, 0x07ff, output_gain);	
	snd_soc_update_bits(codec, REG_ARSLOPGAIN_ADDR, 0x07ff, output_gain);	
	snd_soc_update_bits(codec, REG_VSLOPGAIN_ADDR, 0x07ff, output_gain);	

	return 1;
}

#define MIC_GAIN_RETRY (3)
static void bcm4760_mic_gain(int mic_gain)
{
	unsigned long gain1_addr_1, gain1_addr_2;
	unsigned long gain2_addr_1, gain2_addr_2;
	unsigned long gain1_ctrl_1, gain1_ctrl_2;
	unsigned long gain2_ctrl_1, gain2_ctrl_2;
	int cur_gain1, cur_gain2;
	int i = 0;

	mic_gain -= PREAMPGAIN;
	if (mic_gain < BCM4760_MINGAIN) {
		mic_gain = BCM4760_MINGAIN;
	} else if (mic_gain > BCM4760_MAXGAIN) {
		mic_gain = BCM4760_MAXGAIN;
	}

	gain1_addr_1 = IO_ADDRESS(CMU_R_ADM_RXAUDIO_CTRL1_MEMADDR); // 0xB0204
	gain1_addr_2 = IO_ADDRESS(CMU_R_ADM_RXAUDIO_CTRL2_MEMADDR); // 0xB0208
	gain2_addr_1 = IO_ADDRESS(CMU_R_ADM_RXAUDIO_CTRL4_MEMADDR); // 0xB0220
	gain2_addr_2 = IO_ADDRESS(CMU_R_ADM_RXAUDIO_CTRL5_MEMADDR); // 0xB0224
	for (i=0; i<=MIC_GAIN_RETRY; i++) {
		/* verify gains are set to requested value */
		gain1_ctrl_1 = readl(gain1_addr_1);
		gain1_ctrl_2 = readl(gain1_addr_2);
		cur_gain1 = ((gain1_ctrl_1 >> 30) & 3) | ((gain1_ctrl_2 & 0x0f) << 2);
		gain2_ctrl_1 = readl(gain2_addr_1);
		gain2_ctrl_2 = readl(gain2_addr_2);
		cur_gain2 = ((gain2_ctrl_1 >> 30) & 3) | ((gain2_ctrl_2 & 0x0f) << 2);

		if ((cur_gain1 == mic_gain) && (cur_gain1 == mic_gain)) { 
			if (i > 1) {
				printk(KERN_ERR "Error, needed %d attempts to set MIC gain %d\n", i, mic_gain);
			}
			break;
		} else if (i == MIC_GAIN_RETRY) {
			printk(KERN_ERR "Error, failed %d times to set MIC gain %d now running with MIC1 = %d MIC2 = %d\n", MIC_GAIN_RETRY, mic_gain, cur_gain1, cur_gain2);
			break;
		} else if (i > 0) {
			printk(KERN_ERR "Error, attempt %d to set MIC gain %d (MIC1 = %d MIC2 = %d)\n", i+1, mic_gain, cur_gain1, cur_gain2);
		}

		/* set gain for MIC1 and MIC2 */
		gain1_ctrl_1 &= 0x3fffffff;
		gain1_ctrl_1 |= ((mic_gain & 3) << 30);
		gain1_ctrl_2 &= 0xfffffff0;
		gain1_ctrl_2 |= ((mic_gain & 0x3c) >> 2);
		writel(gain1_ctrl_1, gain1_addr_1); 
		writel(gain1_ctrl_2, gain1_addr_2); 
		gain2_ctrl_1 &= 0x3fffffff;
		gain2_ctrl_1 |= ((mic_gain & 3) << 30);
		gain2_ctrl_2 &= 0xfffffff0;
		gain2_ctrl_2 |= ((mic_gain & 0x3c) >> 2);
		writel(gain2_ctrl_1, gain2_addr_1); 
		writel(gain2_ctrl_2, gain2_addr_2); 
	}
}

static int bcm4760_set_mic_gain(struct tt_scenario_context *c)
{
	bcm4760_mic_gain(c->tt_mic_gain);
	return 1;
}

#ifdef INCLUDE_TEST_KCONTROLS
static int bcm4760_set_gain(struct tt_scenario_context *c)
{
	struct snd_kcontrol	*kcontrol = (struct snd_kcontrol *) c->data;
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);

	unsigned long gain_addr_1, gain_addr_2;
	unsigned long gain_ctrl_1, gain_ctrl_2;
	int cur_gain;

	switch (c->tt_test_gain_type) {
		case 1:
			snd_soc_update_bits(codec, REG_ALSLOPGAIN_ADDR, 0x07ff, c->tt_test_gain_value);	
			snd_soc_update_bits(codec, REG_ARSLOPGAIN_ADDR, 0x07ff, c->tt_test_gain_value);	
			snd_soc_update_bits(codec, REG_VSLOPGAIN_ADDR, 0x07ff, c->tt_test_gain_value);
			break;
		case 2:
			snd_soc_update_bits(codec, REG_MIXER_GAIN_CHSEL_ADDR, 0xffff, 0);	
			snd_soc_update_bits(codec, REG_MIXER_GAIN_ADJUST_ADDR, 0xffff, c->tt_test_gain_value);	
			snd_soc_update_bits(codec, REG_MIXER_GAIN_CHSEL_ADDR, 0xffff, 1);	
			snd_soc_update_bits(codec, REG_MIXER_GAIN_ADJUST_ADDR, 0xffff, c->tt_test_gain_value);	
			break;
		case 3:
			snd_soc_update_bits(codec, REG_VCFGR_ADDR, 0xffff, c->tt_test_gain_value);	
			break;
		case 21:
			gain_addr_1 = IO_ADDRESS(CMU_R_ADM_RXAUDIO_CTRL1_MEMADDR); // 0xB0204
			gain_addr_2 = IO_ADDRESS(CMU_R_ADM_RXAUDIO_CTRL2_MEMADDR); // 0xB0208
			gain_ctrl_1 = readl(gain_addr_1);
			gain_ctrl_2 = readl(gain_addr_2);
			cur_gain = ((gain_ctrl_1 >> 30) & 3) | ((gain_ctrl_2 & 0x0f) << 2);
			if (cur_gain != c->tt_test_gain_value) {
				gain_ctrl_1 &= 0x3fffffff;
				gain_ctrl_1 |= ((c->tt_test_gain_value & 3) << 30);
				gain_ctrl_2 &= 0xfffffff0;
				gain_ctrl_2 |= ((c->tt_test_gain_value & 0x3c) >> 2);
				writel(gain_ctrl_1, gain_addr_1); 
				writel(gain_ctrl_2,gain_addr_2); 
			}
			break;
		case 22:
			gain_addr_1 = IO_ADDRESS(CMU_R_ADM_RXAUDIO_CTRL4_MEMADDR); // 0xB0220
			gain_addr_2 = IO_ADDRESS(CMU_R_ADM_RXAUDIO_CTRL5_MEMADDR); // 0xB0224
			gain_ctrl_1 = readl(gain_addr_1);
			gain_ctrl_2 = readl(gain_addr_2);
			cur_gain = ((gain_ctrl_1 >> 30) & 3) | ((gain_ctrl_2 & 0x0f) << 2);
			if (cur_gain != c->tt_test_gain_value) {
				gain_ctrl_1 &= 0x3fffffff;
				gain_ctrl_1 |= ((c->tt_test_gain_value & 3) << 30);
				gain_ctrl_2 &= 0xfffffff0;
				gain_ctrl_2 |= ((c->tt_test_gain_value & 0x3c) >> 2);
				writel(gain_ctrl_1, gain_addr_1); 
				writel(gain_ctrl_2,gain_addr_2); 
			}
			break;
		default:
			printk("unknown gain type\n");
			break;
	}

	return 1;
}
#endif

static int bcm4760_set_scenario(struct tt_scenario_context *c)
{
	/* set audio routing scenarion and resore the output gain */
	struct snd_kcontrol	*kcontrol = (struct snd_kcontrol *) c->data;
	struct snd_soc_codec *codec = snd_kcontrol_chip(kcontrol);
	int output_gain = (0x1ff - (120 << 2));
	int VCFGR_gain = 0x00;

	switch(c->tt_scenario) {
	default:
	case TT_SCENARIO_AUDIO_INIT:
		/* go to off state */
	case TT_SCENARIO_AUDIO_OFF:
		bcm4760_mic_gain(0);
		break;
	case TT_SCENARIO_ASR:
		break;
	case TT_SCENARIO_HF_LINEOUT:
	case TT_SCENARIO_HF_SPEAKER:
		if (c->has_5V_booster == 0) {
			output_gain = (0x01ff - OUTPUT_HF_ATT_NO_5V_BOOSTER); /* 0 db, fixed line out level */
		} else {
			output_gain = (0x01ff - OUTPUT_HF_ATT_5V_BOOSTER); /* 0 db, fixed line out level */
		}
		break;
	case TT_SCENARIO_SPEAKER:
		output_gain = c->tt_scenario_speaker_volume;
		break;
	}

	/* ensure voice channel is always at same level as audio channel */
	snd_soc_update_bits(codec, REG_VCFGR_ADDR, 0xffff, VCFGR_gain);

	/* ensure mixer level is always set to best performance */
	snd_soc_update_bits(codec, REG_MIXER_GAIN_CHSEL_ADDR, 0xffff, 0);	
	snd_soc_update_bits(codec, REG_MIXER_GAIN_ADJUST_ADDR, 0xffff, 0x2000);	
	snd_soc_update_bits(codec, REG_MIXER_GAIN_CHSEL_ADDR, 0xffff, 1);	
	snd_soc_update_bits(codec, REG_MIXER_GAIN_ADJUST_ADDR, 0xffff, 0x2000);	

 	/* set attenuation level for audio left and right channels and voice channel */
	snd_soc_update_bits(codec, REG_ALSLOPGAIN_ADDR, 0x07ff, output_gain);	
	snd_soc_update_bits(codec, REG_ARSLOPGAIN_ADDR, 0x07ff, output_gain);	
	snd_soc_update_bits(codec, REG_VSLOPGAIN_ADDR, 0x07ff, output_gain);	

	return 0;
}

DUMMY_CALLBACK(set_volume)		/* Afaik, set_volume is not really used */
DUMMY_CALLBACK(set_scenario_mode)	/* implementing dapm-like functions. */

static struct tt_scenario_ops bcm4760_scenario_ops = 
{
	.set_volume		= dummy_set_volume,
	.set_volume_db		= bcm4760_set_volume_db,
	.set_scenario		= bcm4760_set_scenario,
	.set_scenario_mode	= dummy_set_scenario_mode,
	.set_mic_gain		= bcm4760_set_mic_gain,
#ifdef INCLUDE_TEST_KCONTROLS
	.set_gain		= bcm4760_set_gain,
#endif
};

static int __init tt_nashville_eua2011_setup(char *str)
{
	tt_scenario_register(&bcm4760_scenario_ops);

	printk (KERN_INFO "TomTom EUA2011 ALSA Nashville scenario controls, (C) 2009 TomTom BV\n");

	return 0;
}

TT_SETUP_CB(tt_nashville_eua2011_setup, "tomtom-bcm-amplifier-eua2011");
