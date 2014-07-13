#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <plat/gps.h>
#include <plat/tt_setup_handler.h>
#include <linux/gpio.h>
#include <linux/vgpio.h>
#include <plat/gps.h>
#include <plat/mendoza.h>

static void mendoza_gps_reset(void)
{
	gpio_set_value(TT_VGPIO_GPS_RESET, 1);
	msleep(50);
	gpio_set_value(TT_VGPIO_GPS_RESET, 0);
}

/* On Santiago the gps chip is always powered and needs a reset from the daemon to get into a
   known state upon startup, so to be sort of compatible with rfkill we also do a reset. */
static void mendoza_gps_set_power(int power)
{	
	if (power) {
		gpio_set_value(TT_VGPIO_GPS_TCXO_ON, 1);
		gpio_set_value(TT_VGPIO_GPS_POWER, 1);
		msleep(10);
		mendoza_gps_reset();
	}
	else {
		gpio_set_value(TT_VGPIO_GPS_POWER, 0);
		gpio_set_value(TT_VGPIO_GPS_TCXO_ON, 0);
	}
}

static struct generic_gps_info mendoza_gps_info = {
	.name		= "mendoza",
	.gps_set_power	= mendoza_gps_set_power,
};

static struct platform_device mendoza_gps_pdev =
{
	.name		= "tomtom-gps",
	.id		= -1,
	.dev = {
		.platform_data	= &mendoza_gps_info,
	},
};

static __init int mendoza_gps_register(void)
{
	return gpio_request(TT_VGPIO_GPS_RESET, "GPS RESET")     ||
	       gpio_direction_output(TT_VGPIO_GPS_RESET, 0)      ||
	       gpio_request(TT_VGPIO_GPS_TCXO_ON, "GPS_TCXO_ON") ||
	       gpio_direction_output(TT_VGPIO_GPS_TCXO_ON, 1)    ||
	       gpio_request(TT_VGPIO_GPS_POWER, "GPS POWER")     ||
	       gpio_direction_output(TT_VGPIO_GPS_POWER, 1)      ||
	       platform_device_register(&mendoza_gps_pdev);
}

arch_initcall(mendoza_gps_register);
