/* Platform data for low dc vcc detector */
struct detect_low_dc_platform_data {

	void (*mach_release)(struct detect_low_dc_platform_data*);

	int (*config_gpio)(void);
	int (*get_gpio_value)(void);

	void (*suspend)(void);
	void (*resume)(void);
};
