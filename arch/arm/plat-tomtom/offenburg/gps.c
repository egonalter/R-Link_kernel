#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <plat/gps.h>
#include <plat/tt_setup_handler.h>
#include <linux/gpio.h>
#include <linux/vgpio.h>
#include <plat/gpio_cache.h>
#include <plat/gps.h>
#include <plat/offenburg.h>

/* set LOW in this order and restore in the reverse */
static struct gpio_cache cached_stress[] = {
	{ 167, }, /* GPS_CORE_PWR_EN */
	{ 160, 150, }, /* nGPS_RESET */
	{ 60, }, /* nBT_RST */
	{ 108, }, /* nGPS_ANT_SHORT */
	{ 42, }, /* GPS_1PPS */
	{ 142, }, /* BT_Rx */
	{ 143, }, /* BT_Tx */
	{ 148, }, /* GPS_RX_SOC_TX */
	{ 149, }, /* GPS_CTS_SOC_RTS */
	{ 150, }, /* GPS_RTS_SOC_CTS */
	{ 151, }, /* GPS_TX_SOC_RX */
	{ 158, }, /* BT_PCM_IN */
	{ 159, }, /* BT_PCM_OUT */
	{ 161, }, /* BT_PCM_SYNC */
	{ 162, }, /* BT_PCM_CLK */
	{ 164, }, /* GPS_HOST_REQ */
};

static void offenburg_gps_reset(void)
{
	gpio_set_value(TT_VGPIO_GPS_RESET, 1);
	msleep(50);
	gpio_set_value(TT_VGPIO_GPS_RESET, 0);
	msleep(150);
}

/* On Santiago the gps chip is always powered and needs a reset from the daemon to get into a
   known state upon startup, so to be sort of compatible with rfkill we also do a reset. */
static void offenburg_gps_set_power(int power)
{	
	if (power) {
		gpio_set_value(TT_VGPIO_GPS_POWER, 1);
		msleep(5);
		offenburg_gps_reset();
		gpio_set_value(TT_VGPIO_GPS_TCXO_ON, 1);
	}
	else {
		gpio_set_value(TT_VGPIO_GPS_RESET, 1);
		gpio_set_value(TT_VGPIO_GPS_POWER, 0);
		gpio_set_value(TT_VGPIO_GPS_TCXO_ON, 0);
	}
}

/*
 * set LOW in this order and restore in the reverse, they need to be
 * low when 1V8_AUX3 is automatically turned on by the pmic on resume
 */
static struct gpio_cache cached[] = {
	{ 108, }, /* nGPS_ANT_SHORT */
	{ 42, }, /* GPS_1PPS */
	{ 148, }, /* GPS_RX_SOC_TX */
	{ 149, }, /* GPS_CTS_SOC_RTS */
	{ 150, }, /* GPS_RTS_SOC_CTS */
	{ 151, }, /* GPS_TX_SOC_RX */
	{ 164, }, /* GPS_HOST_REQ */
};

static void offenburg_gps_suspend(void)
{
	printk(KERN_DEBUG "%s - setting data lines to low...\n", __func__);

	gpio_set_value(TT_VGPIO_GPS_RESET, 1);
	gpio_set_value(TT_VGPIO_GPS_POWER, 0);

	WARN_ON(gpio_cache_set_out(cached, ARRAY_SIZE(cached), 0));
}

static void offenburg_gps_resume(void)
{
	printk(KERN_DEBUG "%s - restoring data lines...\n", __func__);

	gpio_set_value(TT_VGPIO_GPS_RESET, 1);
	gpio_set_value(TT_VGPIO_GPS_POWER, 0);

	gpio_cache_restore(cached, ARRAY_SIZE(cached));
}

static int offenburg_gps_coldboot_start(struct device *dev, struct regulator *reg)
{
	int err;

	//dev_dbg(dev, "starting coldboot reset...\n");
	printk(KERN_ERR "starting coldboot reset...\n");

	err = gpio_cache_set_out(cached_stress, ARRAY_SIZE(cached_stress), 0);
	if (!err)
		/* switch off the GPS chip */
		regulator_disable(reg);

	return err;
}

static int offenburg_gps_coldboot_finish(struct device *dev, struct regulator *reg)
{
	printk(KERN_ERR "finishing coldboot reset...\n");

	/* switch on the GPS chip */
	regulator_enable(reg);

	gpio_cache_restore(cached_stress, ARRAY_SIZE(cached_stress));
	return 0;
}

static struct generic_gps_info offenburg_gps_info = {
	.name		= "offenburg",
	.gps_set_power	= offenburg_gps_set_power,
	.suspend	= offenburg_gps_suspend,
	.resume		= offenburg_gps_resume,
	.coldboot_start = offenburg_gps_coldboot_start,
	.coldboot_finish = offenburg_gps_coldboot_finish,
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
	if (gpio_cache_alloc(cached_stress, ARRAY_SIZE(cached_stress)))
		return -ENOMEM;
	if (gpio_cache_alloc(cached, ARRAY_SIZE(cached)))
		return -ENOMEM;

        return gpio_request(TT_VGPIO_GPS_POWER, "GPS POWER")     ||
               gpio_direction_output(TT_VGPIO_GPS_POWER, 0)      ||
	       gpio_request(TT_VGPIO_GPS_RESET, "GPS RESET")     ||
	       gpio_direction_output(TT_VGPIO_GPS_RESET, 1)      ||
	       gpio_request(TT_VGPIO_GPS_TCXO_ON, "GPS_TCXO_ON") ||
	       gpio_direction_output(TT_VGPIO_GPS_TCXO_ON, 1)    ||
	       gpio_request(TT_VGPIO_GPS_ANT_SHORT, "GPS_ANT_SHORT") ||
	       gpio_direction_input(TT_VGPIO_GPS_ANT_SHORT)      ||
	       gpio_export(TT_VGPIO_GPS_ANT_SHORT, 0)            ||
	       platform_device_register(&offenburg_gps_pdev)     ||
	       gpio_export_link(&offenburg_gps_pdev.dev, "gps_ant_short", 
	                        TT_VGPIO_GPS_ANT_SHORT);
}

arch_initcall(offenburg_gps_register);
