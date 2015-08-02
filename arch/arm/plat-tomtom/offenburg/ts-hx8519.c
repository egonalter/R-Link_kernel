#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/vgpio.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/regulator/consumer.h>
#include <linux/i2c/hx8519.h>
#include <asm/mach-types.h>

#include <plat/offenburg.h>
#include <plat/fdt.h>

static int ts_init(void)
{
	/* Request IRQ */
	return gpio_request(TT_VGPIO_TP_IRQ, "TP IRQ") ||
	       gpio_direction_input(TT_VGPIO_TP_IRQ);
}

static int ts_get_pendown_state(void)
{
	return !!gpio_get_value(TT_VGPIO_TP_IRQ);
}

static struct hx8519_platform_data hx8519_info = {
	.model			= 8519,
	.x_plate_ohms		= 180 /*937*/,
	.get_pendown_state	= ts_get_pendown_state,
	.init_platform_hw	= ts_init,
};

static struct i2c_board_info hx8519_i2c_board_info = {
	I2C_BOARD_INFO("hx8519", 0x4b),
	.platform_data	= &hx8519_info,
};

/* these screens use the hx8519 TP */
static int __init offenburg_hx8519_lcm(void)
{
	int i;
	const char *screen = fdt_get_string ("/features", "tft", DEFAULT_LCM);

	const char *supported_screens[] = {
		"DD070NA_H8519",
	};

	for (i = 0; i < ARRAY_SIZE(supported_screens); i++)
		if (strcmp(screen, supported_screens[i]) == 0)
			return 1;

	return 0;
}

static int __init offenburg_tp_init_device(void)
{
	struct i2c_adapter	*adapter;

	/* init hx8519 only if required for the configured screen */
	if (!offenburg_hx8519_lcm())
		return 0;

	/* Reset the chip */
	if (machine_is_strasbourg_a2()) {
		gpio_request(TT_VGPIO_TP_RESET, "hx8526 reset");	/* Not checking the return value, the vgpios will have crashed before this call if it's not available. */
		gpio_direction_output(TT_VGPIO_TP_RESET, 0);
		mdelay(10);
		gpio_set_value(TT_VGPIO_TP_RESET, 1);
		mdelay(1);
		gpio_set_value(TT_VGPIO_TP_RESET, 0);
		mdelay(15);
	}

	hx8519_i2c_board_info.irq	= gpio_to_irq(TT_VGPIO_TP_IRQ);

	/* Add a new device on the bus. */
	adapter	= i2c_get_adapter(2);

	if (IS_ERR(adapter)) {
		printk("Could not get adapter: %ld\n", PTR_ERR(adapter));
		return PTR_ERR(adapter);
	}

	return IS_ERR(i2c_new_device(adapter, &hx8519_i2c_board_info));
}

late_initcall(offenburg_tp_init_device);
