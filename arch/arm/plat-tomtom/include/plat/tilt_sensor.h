/* Platform data for Tilt sensor*/
struct tilt_sensor_platform_data {

	int (*tilt_power_set) (int);
	int (*tilt_power_get) (void);
	int (*tilt_value) (void);
	int (*config_gpio) (void);

	int irq;
};
