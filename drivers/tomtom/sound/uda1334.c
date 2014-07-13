#include <linux/kernel.h>
#include <linux/module.h>

#include <plat/scenarii.h>

#define DUMMY_CALLBACK(name)  static int dummy_##name(struct tt_scenario_context *c) \
                              { \
                                  printk(KERN_ERR "Callback %s not implemented in %s\n", #name, __FILE__); \
                                  return 0; \
                              }

DUMMY_CALLBACK(set_scenario_mode)
DUMMY_CALLBACK(set_volume)
DUMMY_CALLBACK(set_mic_gain)
DUMMY_CALLBACK(set_volume_dB)
DUMMY_CALLBACK(set_scenario)
#ifdef INCLUDE_TEST_KCONTROLS
DUMMY_CALLBACK(set_gain)
#endif

struct tt_scenario_ops uda1334_scenario_ops = {
	.set_volume_db		= dummy_set_volume_dB,
	.set_volume		= dummy_set_volume,
	.set_scenario		= dummy_set_scenario,
	.set_scenario_mode	= dummy_set_scenario_mode,
	.set_mic_gain		= dummy_set_mic_gain,
	.hpdetect_callback	= NULL,
	.get_hpdetect_irq	= NULL,
#ifdef INCLUDE_TEST_KCONTROLS
	.set_gain		= dummy_set_gain,
#endif
};
EXPORT_SYMBOL(uda1334_scenario_ops);
