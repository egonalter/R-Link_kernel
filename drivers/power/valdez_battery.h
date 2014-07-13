#ifndef __VALDEZ_BATTERY_H__
#define __VALDEZ_BATTERY_H__

void battery_calc_init(struct platform_device *pdev);
void battery_calc_add_adc(unsigned int adc, unsigned int charging,
	                  unsigned int *voltage, unsigned int *capacity);

#endif /* __VALDEZ_BATTERY_H__ */
