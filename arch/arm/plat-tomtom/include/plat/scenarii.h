/* include/asm-arm/plat-tomtom/scenari.h
 *
 * Header for the ALSA controls that are required by Nashville.
 *
 * Copyright (C) 2009 TomTom BV <http://www.tomtom.com/>
 * Author: Guillaume Ballet <Guillaume.Ballet@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/* TT scenarios */
enum TT_SCENARIO {
	TT_SCENARIO_NONE          = -1,
	TT_SCENARIO_AUDIO_INIT    = 0,
	TT_SCENARIO_AUDIO_OFF     = 1,
	TT_SCENARIO_ASR           = 2,
	TT_SCENARIO_VR            = 3,
	TT_SCENARIO_NOISE         = 4,
	TT_SCENARIO_iPod          = 5,
	TT_SCENARIO_SPEAKER       = 6,
	TT_SCENARIO_LINEOUT       = 7,
	TT_SCENARIO_FM            = 8,
	TT_SCENARIO_iPod_SPEAKER  = 9,
	TT_SCENARIO_iPod_LINEOUT  = 10,
	TT_SCENARIO_iPod_FM       = 11,
	TT_SCENARIO_HF_SPEAKER    = 12,
	TT_SCENARIO_HF_LINEOUT    = 13,
	TT_SCENARIO_HF_FM         = 14,
};

/* TT scenario modes */
enum TT_MODE {
	TT_MODE_INIT  = 0,
	TT_MODE_OFF   = 1,
	TT_MODE_SPP   = 2,
	TT_MODE_TTS   = 3,
	TT_MODE_MP3   = 4,
	TT_MODE_iPod  = 5,
	TT_MODE_HF    = 6,
	TT_MODE_ASR   = 7,
	TT_MODE_VR    = 8,
	TT_MODE_POI   = 9,
};

//#define INCLUDE_TEST_KCONTROLS

struct tt_scenario_context;

typedef int (*tt_scenario_callback_t)(struct tt_scenario_context *);

struct tt_scenario_ops {
	tt_scenario_callback_t	set_volume,
				set_volume_db,
				set_scenario,
				set_scenario_mode,
				#ifdef INCLUDE_TEST_KCONTROLS
				set_gain,
				#endif
				set_mic_gain,
				get_hpdetect_irq,
				hpdetect_callback;
};

struct tt_scenario_context {
	int			tt_volume;
	int			tt_volume_dB;
	int			tt_scenario;
	int			tt_current_scenario;
	int			tt_scenario_mode;
	int			tt_mic_gain;
	int			tt_scenario_speaker_volume;
	int			tt_scenario_iPod_speaker_volume;
	#ifdef INCLUDE_TEST_KCONTROLS
	int			tt_test_gain_type;
	int			tt_test_gain_value;
	#endif
	int			dapm_enabled;
	int			has_5V_booster;

	int			tt_hpdetect;
	struct snd_ctl_elem_id	*tt_hpdetect_elem_id;

	struct snd_soc_codec	*codec;
	struct snd_kcontrol	**kcontrols;
	struct tt_scenario_ops	*ops;

	struct list_head	list;
};
