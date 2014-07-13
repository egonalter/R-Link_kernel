#ifndef __LINUX_I2C_HX8519_H
#define __LINUX_I2C_HX8519_H

/* linux/i2c/hx8519.h */

struct hx8519_platform_data {
	u16	model;
	u16	x_plate_ohms;

	int	(*get_pendown_state)(void);
	void	(*clear_penirq)(void);		/* If needed, clear 2nd level
						   interrupt source */
	int	(*init_platform_hw)(void);
	void	(*exit_platform_hw)(void);
};

#endif
