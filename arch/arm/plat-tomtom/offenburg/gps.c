#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <plat/gps.h>
#include <plat/tt_setup_handler.h>
#include <linux/gpio.h>
#include <linux/vgpio.h>
#include <plat/gps.h>
#include <plat/offenburg.h>

static void offenburg_gps_reset(void)
{
	gpio_set_value(TT_VGPIO_GPS_RESET, 1);
	msleep(50);
	gpio_set_value(TT_VGPIO_GPS_RESET, 0);
}

/* On Santiago the gps chip is always powered and needs a reset from the daemon to get into a
   known state upon startup, so to be sort of compatible with rfkill we also do a reset. */
static void offenburg_gps_set_power(int power)
{	
	if (power) {
		gpio_set_value(TT_VGPIO_GPS_TCXO_ON, 1);
		gpio_set_value(TT_VGPIO_GPS_POWER, 1);
		msleep(10);
		offenburg_gps_reset();
	}
	else {
		gpio_set_value(TT_VGPIO_GPS_POWER, 0);
		gpio_set_value(TT_VGPIO_GPS_TCXO_ON, 0);
	}
}

static struct generic_gps_info offenburg_gps_info = {
	.name		= "offenburg",
	.gps_set_power	= offenburg_gps_set_power,
};

static struct platform_device offenburg_gps_pdev =
{
	.name		= "tomtom-gps",
	.id		= -1,
	.dev = {
		.platform_data	= &offenburg_gps_info,
	},
};

static __init int offenburg_gps_register(void)
{
	return gpio_request(TT_VGPIO_GPS_RESET, "GPS RESET")     ||
	       gpio_direction_output(TT_VGPIO_GPS_RESET, 0)      ||
	       gpio_request(TT_VGPIO_GPS_TCXO_ON, "GPS_TCXO_ON") ||
	       gpio_direction_output(TT_VGPIO_GPS_TCXO_ON, 1)    ||
	       gpio_request(TT_VGPIO_GPS_POWER, "GPS POWER")     ||
	       gpio_direction_output(TT_VGPIO_GPS_POWER, 1)      ||
	       gpio_request(TT_VGPIO_GPS_ANT_SHORT, "GPS_ANT_SHORT") ||
	       gpio_direction_input(TT_VGPIO_GPS_ANT_SHORT)      ||
	       gpio_export(TT_VGPIO_GPS_ANT_SHORT, 0)            ||
	       platform_device_register(&offenburg_gps_pdev)     ||
	       gpio_export_link(&offenburg_gps_pdev.dev, "gps_ant_short", 
	                        TT_VGPIO_GPS_ANT_SHORT);
}

arch_initcall(offenburg_gps_register);
