#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>

#include <plat/scenarii.h>

#include <sound/control.h>
#include <sound/soc.h>
#include "../../../sound/soc/codecs/twl4030.h"

#define DUMMY_CALLBACK(name)  static int dummy_##name(struct tt_scenario_context *c) \
                              { \
                                  printk(KERN_ERR "Callback %s not implemented in %s\n", #name, __FILE__); \
                                  return 0; \
                              }

/* Default functions - Replace them as needed */
DUMMY_CALLBACK(set_scenario_mode)
DUMMY_CALLBACK(set_volume)
DUMMY_CALLBACK(set_volume_dB)
#ifdef INCLUDE_TEST_KCONTROLS
DUMMY_CALLBACK(set_gain)
#endif

#define TWL3040_REG(name, mask, value)    snd_soc_update_bits(codec, TWL4030_REG_##name, mask, value)

#define TWL4030_REG_ALL_BITS_ZERO (0x00)

//CODEC_MODE register 0x00000001
#define TWL4030_REG_CODEC_MASK (0x02)
#define TWL4030_REG_CODEC_POWER_ENABLE (0x02)

//ANAMICL register 0x00000005, ANAMICR register 0x00000006
#define TWL4030_REG_ANAMIC_MASK (0x1F)
#define TWL4030_REG_ANAMIC_AMP_ENABLE (0x10)
#define TWL4030_REG_ANAMIC_AUX_ENABLE (0x04)
#define TWL4030_REG_ANAMIC_MAINMIC_ENABLE (0x01)

//AVADC_CTL register 0x00000007
#define TWL4030_REG_AVADC_CTl_MASK (0x0A)
#define TWL4030_REG_AVADC_LEFT_ENABLE (0x08)
#define TWL4030_REG_AVADC_RIGHT_ENABLE (0x02)

//HS_SEL register 0x00000022
#define TWL4030_REG_HS_SEL_MASK (0x36)
#define TWL4030_REG_HSOR_AR2_ENABLE (0x20)
#define TWL4030_REG_HSOL_AL2_ENABLE (0x04)

//HS_GAIN_SET register 0x00000023
#define TWL4030_REG_HS_GAIN_SET_MASK (0x0F)
#define TWL4030_REG_HS_GAIN_SET_HS_GAIN_0_DB (0x02)
#define TWL4030_REG_HS_GAIN_SET_HSR_GAIN_0_DB (TWL4030_REG_HS_GAIN_SET_HS_GAIN_0_DB << 2)
#define TWL4030_REG_HS_GAIN_SET_HSL_GAIN_0_DB (TWL4030_REG_HS_GAIN_SET_HS_GAIN_0_DB)

#define TWL4030_REG_ARX2PGA_MINIMAL_DIGITAL_GAIN (-63)

static inline void twl4030_disable_codec(struct snd_soc_codec *codec)
{
	snd_soc_update_bits(codec, TWL4030_REG_CODEC_MODE, TWL4030_REG_CODEC_MASK, TWL4030_REG_ALL_BITS_ZERO);
}

static inline void twl4030_enable_codec(struct snd_soc_codec *codec)
{
	snd_soc_update_bits(codec, TWL4030_REG_CODEC_MODE, TWL4030_REG_CODEC_MASK, TWL4030_REG_CODEC_POWER_ENABLE);
}

static inline void twl4030_disable_headset_out(struct snd_soc_codec *codec)
{
	snd_soc_update_bits(codec, TWL4030_REG_HS_SEL, TWL4030_REG_HS_SEL_MASK, TWL4030_REG_ALL_BITS_ZERO);
	snd_soc_update_bits(codec, TWL4030_REG_HS_GAIN_SET, TWL4030_REG_HS_GAIN_SET_MASK, TWL4030_REG_ALL_BITS_ZERO);
}

static inline void twl4030_enable_headset_out(struct snd_soc_codec *codec)
{
	snd_soc_update_bits(codec, TWL4030_REG_HS_SEL, TWL4030_REG_HS_SEL_MASK, TWL4030_REG_HSOR_AR2_ENABLE | TWL4030_REG_HSOL_AL2_ENABLE);
	snd_soc_update_bits(codec, TWL4030_REG_HS_GAIN_SET, TWL4030_REG_HS_GAIN_SET_MASK, TWL4030_REG_HS_GAIN_SET_HSR_GAIN_0_DB | TWL4030_REG_HS_GAIN_SET_HSL_GAIN_0_DB);
}

static inline void twl4030_disable_aux_in(struct snd_soc_codec *codec)
{
	snd_soc_update_bits(codec, TWL4030_REG_ANAMICL, TWL4030_REG_ANAMIC_MASK, TWL4030_REG_ALL_BITS_ZERO);
	snd_soc_update_bits(codec, TWL4030_REG_ANAMICR, TWL4030_REG_ANAMIC_MASK, TWL4030_REG_ALL_BITS_ZERO);
	snd_soc_update_bits(codec, TWL4030_REG_AVADC_CTL, TWL4030_REG_AVADC_CTl_MASK, TWL4030_REG_ALL_BITS_ZERO);
}

static inline void twl4030_enable_aux_in(struct snd_soc_codec *codec)
{
	snd_soc_update_bits(codec, TWL4030_REG_ANAMICL, TWL4030_REG_ANAMIC_MASK, TWL4030_REG_ANAMIC_AMP_ENABLE | TWL4030_REG_ANAMIC_AUX_ENABLE);
	snd_soc_update_bits(codec, TWL4030_REG_ANAMICR, TWL4030_REG_ANAMIC_MASK, TWL4030_REG_ANAMIC_AMP_ENABLE | TWL4030_REG_ANAMIC_AUX_ENABLE);
	snd_soc_update_bits(codec, TWL4030_REG_AVADC_CTL, TWL4030_REG_AVADC_CTl_MASK, TWL4030_REG_AVADC_LEFT_ENABLE | TWL4030_REG_AVADC_RIGHT_ENABLE);
}

static inline void twl4030_set_analog_output_gain(struct snd_soc_codec * codec, int gainIndB, int enableFlag)
{
	//gainIndB must be in range +12dB to -24dB, even values only
	//PMIC has ARX[RL][12]_APGA_CTL register for analog output gain control (-24dB to +12dB in 2dB steps)
	//  see product manual, tables 14-38 through 14-41, page 741
	unsigned char gain = 6 - gainIndB / 2;
	unsigned char mute = enableFlag ? 0x3 : 0x2;
	unsigned char value = (gain << 3) | mute;

	snd_soc_update_bits(codec, TWL4030_REG_ARXL2_APGA_CTL, 0xff, value);
	snd_soc_update_bits(codec, TWL4030_REG_ARXR2_APGA_CTL, 0xff, value);
}

static inline void twl4030_set_digital_output_gain(struct snd_soc_codec * codec, int gainIndB)
{
	//gainIndB must be in range +12dB to -63dB
	//PMIC has ARX[RL][12]PGA register for digital output gain control (-62dB to +12dB in 1dB steps)
	//  see product manual, tables 14-29 through 14-32, page 731)
	unsigned char coarse_gain = gainIndB > 0 ? 1 + (gainIndB - 1) / 6 : 0;
	unsigned char fine_gain = 0x3f + (gainIndB - coarse_gain * 6);
	unsigned char value = (coarse_gain << 6) | fine_gain;

	snd_soc_update_bits(codec, TWL4030_REG_ARXL2PGA, 0xff, value);
	snd_soc_update_bits(codec, TWL4030_REG_ARXR2PGA, 0xff, value);
}

static inline void twl4030_set_analog_microphone_gain(struct snd_soc_codec * codec, int gainIndB)
{
	//gainIndB must be in range 0dB to +30dB
	//PMIC has ANAMIC_GAIN register for analog microphone gain control (0db to +30dB in 6dB steps)
	//  see product manual, table 14-81, page 773)
	unsigned char gain = clamp(gainIndB, 0, 30) / 6;
	snd_soc_update_bits(codec, TWL4030_REG_ANAMIC_GAIN, 0x3f, (gain << 3) | gain);
}

static void twl4030_complete_reset(struct snd_soc_codec *codec)
{
	TWL3040_REG(OPTION, 0xff, 0xc3); // filter: c: output (RX) filter enabl. 3: input (TX) filter enable
	TWL3040_REG(MICBIAS_CTL, 0xff, 0x00);  // all bias disabled
	// [CORAUD-1066] Switching the Left microphone Amplifier on/off introduces a ramp in the mic in signal so we keep it always on
	TWL3040_REG(ANAMICL, 0xff, 0x11);  // 11=left mic ON
	TWL3040_REG(ANAMICR, 0xff, 0x00);  // right mic OFF
	// [CORAUD-1066] Switching the ADC on/off introduces a ramp in the mic in signal so we keep it always on
	TWL3040_REG(AVADC_CTL, 0xff, 0x08);  // 08=ADCleft pwrON                 (Record)
	TWL3040_REG(ATXL1PGA, 0xff, 0x00); // 00=0dB after init; make no gain for default mic
	TWL3040_REG(ATXR1PGA, 0xff, 0x00); // 00=0dB after init; make no gain for default mic
	TWL3040_REG(AUDIO_IF, 0xff, 0x01); // Audio settings
	TWL3040_REG(VOICE_IF, 0xff, 0x00); // 0= reset off
	TWL3040_REG(ARXR1PGA, 0xff, 0x00); // coarse: 0dB, fine: Mute
	TWL3040_REG(ARXL1PGA, 0xff, 0x00); // coarse: 0dB, fine: Mute
	TWL3040_REG(ARXR2PGA, 0xff, 0x00);
	TWL3040_REG(ARXL2PGA, 0xff, 0x00);
	TWL3040_REG(VRXPGA, 0xff, 0x00); // 0=Mute
	TWL3040_REG(VSTPGA, 0xff, 0x00); // mute (not used in option 1)
	TWL3040_REG(VRX2ARXPGA, 0xff, 0x00); // mute (not used in option 1)
	TWL3040_REG(AVDAC_CTL, 0xff, 0x0c);  // ADAC1 off, ADAC2 on, VDAC off
	TWL3040_REG(ARX2VTXPGA, 0xff, 0x00); // mute (not used in option 1)
	TWL3040_REG(ARXL1_APGA_CTL, 0xff, 0x00); // 0=12dB,power-down
	TWL3040_REG(ARXR1_APGA_CTL, 0xff, 0x00); // 0=12dB,power-down
	TWL3040_REG(ARXL2_APGA_CTL, 0xff, 0x00);
	TWL3040_REG(ARXR2_APGA_CTL, 0xff, 0x00);
	TWL3040_REG(ATX2ARXPGA, 0xff, 0x00); // loop back R=mute, L=mute
	TWL3040_REG(BT_IF, 0xff, 0x00);  // off(not used)
	TWL3040_REG(BTPGA, 0xff, 0x00);  // 0=BT RX=-30dB, TX=-15dB; reg is NA in opt1 mode
	TWL3040_REG(BTSTPGA, 0xff, 0x00);  // mute(default)
	TWL3040_REG(EAR_CTL, 0xff, 0x00);  // 0=off, must be off
	TWL3040_REG(HS_SEL, 0xff, 0x00); // must be 0, disabled
	TWL3040_REG(HS_GAIN_SET, 0xff, 0x00);  // must be 0, power-down
	TWL3040_REG(HS_POPN_SET, 0xff, 0x00);  // must be 0, Headset not used
	TWL3040_REG(PREDL_CTL, 0xff, 0x00);
	TWL3040_REG(PREDR_CTL, 0xff, 0x00);
	TWL3040_REG(PRECKL_CTL, 0xff, 0x00); // all disabled(default)
	TWL3040_REG(PRECKR_CTL, 0xff, 0x00); // all disabled(default)
	TWL3040_REG(HFL_CTL, 0xff, 0x00);  // all disabled(default)
	TWL3040_REG(HFR_CTL, 0xff, 0x00);  // all disabled(default)
	TWL3040_REG(ALC_CTL, 0xff, 0x00);  // 0=ALC disabled, ALC_WAIT=0
	TWL3040_REG(ALC_SET1, 0xff, 0x00); // (default)
	TWL3040_REG(ALC_SET2, 0xff, 0x00); // (default)
	TWL3040_REG(BOOST_CTL, 0xff, 0x00);  // No effect(default)
	TWL3040_REG(SOFTVOL_CTL, 0xff, 0x00);  // Bypass(default)
	TWL3040_REG(DTMF_FREQSEL, 0xff, 0x13); // 13=default(not used, why change)
	TWL3040_REG(DTMF_TONEXT1H, 0xff, 0x00);  // (default) not used
	TWL3040_REG(DTMF_TONEXT1L, 0xff, 0x00);  // (default) not used
	TWL3040_REG(DTMF_TONEXT2H, 0xff, 0x00);  // (default) not used
	TWL3040_REG(DTMF_TONEXT2L, 0xff, 0x00);  // (default) not used
	TWL3040_REG(DTMF_TONOFF, 0xff, 0x79);  // 79=default(not used, why change)
	TWL3040_REG(DTMF_WANONOFF, 0xff, 0x11);  // 11=default(not used, why change)
	TWL3040_REG(I2S_RX_SCRAMBLE_H, 0xff, 0x00);  // (default) not used
	TWL3040_REG(I2S_RX_SCRAMBLE_M, 0xff, 0x00);  // (default) not used
	TWL3040_REG(I2S_RX_SCRAMBLE_L, 0xff, 0x00);  // (default) not used
	TWL3040_REG(APLL_CTL, 0xff, 0x16); // enabled 26MHz
	TWL3040_REG(DTMF_CTL, 0xff, 0x00); // (default) not used
	TWL3040_REG(DTMF_PGA_CTL2, 0xff, 0x44);  // 44=default(not used, why change)
	TWL3040_REG(DTMF_PGA_CTL1, 0xff, 0x69);  // 69=default(not used, why change)
	TWL3040_REG(MISC_SET_1, 0xff, 0x02); // 2=SMOOTH_ANAVOL_EN
	TWL3040_REG(PCMBTMUX, 0xff, 0x00); // (default) not used
	TWL3040_REG(RX_PATH_SEL, 0xff, 0x00);  // default
	TWL3040_REG(VDL_APGA_CTL, 0xff, 0x00); // 0=disable all 12dB
	TWL3040_REG(VIBRA_CTL, 0xff, 0x00);  // (default) not used
	TWL3040_REG(VIBRA_SET, 0xff, 0x00);  // (default) not used
	TWL3040_REG(ANAMIC_GAIN, 0xff, 0x00);  // 0=R=0dB, L=0dB; make no gain for default mic
	TWL3040_REG(MISC_SET_2, 0xff, 0x00); // 0=all high-pass filters in application mode
}

static int twl4030_santiago_set_scenario(struct tt_scenario_context *c)
{
	struct snd_soc_codec *codec = c->codec;

	switch(c->tt_scenario) {
	default:
		printk("Defaulting to OFF scenario for unimplemented scenario %d", c->tt_scenario);
	case TT_SCENARIO_AUDIO_INIT:
	case TT_SCENARIO_AUDIO_OFF:
		twl4030_disable_aux_in(codec);
		twl4030_disable_headset_out(codec);
		twl4030_disable_codec(codec);
		break;
	case TT_SCENARIO_HF_SPEAKER:
	case TT_SCENARIO_HF_LINEOUT:
		twl4030_enable_aux_in(codec);
		twl4030_enable_headset_out(codec);
		twl4030_enable_codec(codec);
		break;
	case TT_SCENARIO_ASR:
		twl4030_enable_aux_in(codec);
		twl4030_disable_headset_out(codec);
		twl4030_enable_codec(codec);
		break;
	case TT_SCENARIO_SPEAKER:
	case TT_SCENARIO_LINEOUT:
		twl4030_disable_aux_in(codec);
		twl4030_enable_headset_out(codec);
		twl4030_enable_codec(codec);
		break;
	}

	return 0;
}

static int twl4030_monopoli_set_scenario(struct tt_scenario_context *c)
{
	struct snd_soc_codec *codec = c->codec;

	switch(c->tt_scenario) {
	default:
		printk("Defaulting to OFF scenario for unimplemented scenario %d", c->tt_scenario);
	case TT_SCENARIO_AUDIO_INIT:
	case TT_SCENARIO_AUDIO_OFF:
		twl4030_disable_aux_in(codec);
		twl4030_disable_headset_out(codec);
		twl4030_disable_codec(codec);
		break;
	case TT_SCENARIO_HF_SPEAKER:
	case TT_SCENARIO_HF_LINEOUT:
		twl4030_enable_aux_in(codec);
		twl4030_enable_headset_out(codec);
		twl4030_enable_codec(codec);
		break;
	case TT_SCENARIO_ASR:
		twl4030_enable_aux_in(codec);
		twl4030_disable_headset_out(codec);
		twl4030_enable_codec(codec);
		break;
	case TT_SCENARIO_SPEAKER:
	case TT_SCENARIO_LINEOUT:
		twl4030_disable_aux_in(codec);
		twl4030_enable_headset_out(codec);
		twl4030_enable_codec(codec);
		break;
	}

	return 0;
}

static int twl4030_strasbourg_set_scenario(struct tt_scenario_context *c)
{
	struct snd_soc_codec *codec = c->codec;

	

	switch(c->tt_scenario) {
	default:
		printk("Defaulting to OFF scenario for unimplemented scenario %d", c->tt_scenario);
	case TT_SCENARIO_AUDIO_INIT:
		twl4030_complete_reset(codec);
	case TT_SCENARIO_AUDIO_OFF:
		/* set mic gain back to 0dB */
		TWL3040_REG(ATXL1PGA, 0xff, 0x00); 
		TWL3040_REG(ATXR1PGA, 0xff, 0x00);
		#define DEFAULT_MIC_GAIN (0)
		TWL3040_REG(ANAMIC_GAIN, 0xff, (((DEFAULT_MIC_GAIN / 6) << 3) | (DEFAULT_MIC_GAIN / 6)));
		break;
	case TT_SCENARIO_ASR:
		TWL3040_REG(ATXL1PGA, 0xff, 0x00); // c=0dB; make gain for default mic
		TWL3040_REG(ATXR1PGA, 0xff, 0x00); // c=0dB; make gain for default mic
		
		#define ASR_MIC_GAIN (6)
		TWL3040_REG(ANAMIC_GAIN, 0xff, (((ASR_MIC_GAIN / 6) << 3) | (ASR_MIC_GAIN / 6)));
		break;
	case TT_SCENARIO_HF_SPEAKER:
	case TT_SCENARIO_HF_LINEOUT:		
		TWL3040_REG(ATXL1PGA, 0xff, 0x00); // c=0dB; make gain for default mic
		TWL3040_REG(ATXR1PGA, 0xff, 0x00); // c=0dB; make gain for default mic
		TWL3040_REG(ANAMIC_GAIN, 0xff, 0x00);  // 2d=R=0dB, L=0dB; make gain for default mic
	case TT_SCENARIO_SPEAKER:
	case TT_SCENARIO_LINEOUT:
		TWL3040_REG(ARXR2PGA, 0xff, 0x3e); // 3e= coarse: 0dB, fine: -1dB;  for 600mVrms out
		TWL3040_REG(ARXL2PGA, 0xff, 0x3e); // 3e= coarse: 0dB, fine: -1dB;  for 600mVrms out
		TWL3040_REG(ARXL2_APGA_CTL, 0xff, 0x2b); // 2b=2dB, on; +2dB for 600mVrms out
		TWL3040_REG(ARXR2_APGA_CTL, 0xff, 0x2b); // 2b=2dB, on; +2dB for 600mVrms out
		TWL3040_REG(PREDL_CTL, 0xff, 0x24);  // a4 = mux=Audio L2, 0dB
		TWL3040_REG(PREDR_CTL, 0xff, 0x24);  // a4 = mux=Audio R2, 0dB
		break;
	}

	return 0;
}

/* user of this function will pass in a the gain as a
   positive value which should be interpreted as negative */
static int twl4030_set_volume_dB(struct tt_scenario_context *c)
{
	struct snd_soc_codec *codec = c->codec;

	int enable = 1;
	int volume_dB = -1 * c->tt_volume_dB;
	switch(c->tt_scenario) {
	default:
		printk("Disabling audio output for unimplemented scenario %d", c->tt_scenario);
	case TT_SCENARIO_AUDIO_INIT:
	case TT_SCENARIO_AUDIO_OFF:
		volume_dB = TWL4030_REG_ARX2PGA_MINIMAL_DIGITAL_GAIN;
		enable = 0;
		break;
	case TT_SCENARIO_ASR:
		volume_dB = TWL4030_REG_ARX2PGA_MINIMAL_DIGITAL_GAIN;
		enable = 0;
		break;
	case TT_SCENARIO_HF_SPEAKER:
	case TT_SCENARIO_SPEAKER:
		enable = (volume_dB > TWL4030_REG_ARX2PGA_MINIMAL_DIGITAL_GAIN);
		break;
	case TT_SCENARIO_HF_LINEOUT:
	case TT_SCENARIO_LINEOUT:
		//lineout -> always pass through with 0 gain
		volume_dB = 0;
		break;
	}
	twl4030_set_analog_output_gain(codec, 0, enable);
	twl4030_set_digital_output_gain(codec, volume_dB);
	c->tt_volume_dB = -1 * volume_dB;
	return 0;
}

static int twl4030_set_mic_gain(struct tt_scenario_context *c)
{
	struct snd_soc_codec *codec = c->codec;

	switch(c->tt_scenario) {
	default:
	case TT_SCENARIO_AUDIO_INIT:
	case TT_SCENARIO_AUDIO_OFF:
		break;
	case TT_SCENARIO_ASR:
	case TT_SCENARIO_HF_SPEAKER:
	case TT_SCENARIO_HF_LINEOUT:
		twl4030_set_analog_microphone_gain(codec, c->tt_mic_gain);
		break;
	}
	return 0;
}

struct tt_scenario_ops twl4030_santiago_scenario_ops = {
	.set_volume_db		= twl4030_set_volume_dB,
	.set_volume		= dummy_set_volume,
	.set_scenario		= twl4030_santiago_set_scenario,
	.set_scenario_mode	= dummy_set_scenario_mode,
	.set_mic_gain		= twl4030_set_mic_gain,
	.hpdetect_callback	= NULL,
	.get_hpdetect_irq	= NULL,
#ifdef INCLUDE_TEST_KCONTROLS
	.set_gain		= dummy_set_gain,
#endif
};
EXPORT_SYMBOL(twl4030_santiago_scenario_ops);

struct tt_scenario_ops twl4030_monopoli_scenario_ops = {
	.set_volume_db		= twl4030_set_volume_dB,
	.set_volume		= dummy_set_volume,
	.set_scenario		= twl4030_monopoli_set_scenario,
	.set_scenario_mode	= dummy_set_scenario_mode,
	.set_mic_gain		= twl4030_set_mic_gain,
	.hpdetect_callback	= NULL,
	.get_hpdetect_irq	= NULL,
#ifdef INCLUDE_TEST_KCONTROLS
	.set_gain		= dummy_set_gain,
#endif
};
EXPORT_SYMBOL(twl4030_monopoli_scenario_ops);

struct tt_scenario_ops twl4030_strasbourg_scenario_ops = {
	.set_volume_db		= dummy_set_volume_dB,
	.set_volume		= dummy_set_volume,
	.set_scenario		= twl4030_strasbourg_set_scenario,
	.set_scenario_mode	= dummy_set_scenario_mode,
	.set_mic_gain		= twl4030_set_mic_gain,
	.hpdetect_callback	= NULL,
	.get_hpdetect_irq	= NULL,
#ifdef INCLUDE_TEST_KCONTROLS
	.set_gain		= dummy_set_gain,
#endif
};
EXPORT_SYMBOL(twl4030_strasbourg_scenario_ops);
