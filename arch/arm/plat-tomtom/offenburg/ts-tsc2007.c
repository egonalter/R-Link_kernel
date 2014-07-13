#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/vgpio.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/regulator/consumer.h>
#include <linux/i2c/tsc2007.h>
#include <asm/mach-types.h>

#include <plat/offenburg.h>

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

struct tsc2007_platform_data tsc2007_info = {
	.model			= 2007,
	.x_plate_ohms		= 1250 /* min = 700, max = 1800 */,
	.y_plate_ohms		= 350,
	.max_rt			= 3500,
	.get_pendown_state	= ts_get_pendown_state,
	.init_platform_hw	= ts_init,
};

struct i2c_board_info tsc2007_i2c_board_info = {
	I2C_BOARD_INFO("tsc2007", 0x48),
	.platform_data	= &tsc2007_info,
};

static int __init offenburg_tp_init_device(void)
{
	struct i2c_adapter	*adapter;

	/* Reset the chip in the case of the tsc2017 */
	if (machine_is_strasbourg_a2()) {
		gpio_request(TT_VGPIO_TP_RESET, "tsc2017 reset");	/* Not checking the return value, the vgpios will have crashed before this call if it's not available. */
		gpio_direction_output(TT_VGPIO_TP_RESET, 0);
		mdelay(1);
		gpio_set_value(TT_VGPIO_TP_RESET, 1);
		mdelay(1);
		gpio_set_value(TT_VGPIO_TP_RESET, 0);
	}

	tsc2007_i2c_board_info.irq	= gpio_to_irq(TT_VGPIO_TP_IRQ);

#if 0
	TT_LINE_CONFIG(TT_VGPIO_I2C2_POWER);
#endif

	/* Add a new device on the bus. */
	adapter	= i2c_get_adapter(2);

	if (IS_ERR(adapter)) {
		printk("Could not get adapter: %ld\n", PTR_ERR(adapter));
		return PTR_ERR(adapter);
	}

	return IS_ERR(i2c_new_device(adapter, &tsc2007_i2c_board_info));
}

late_initcall(offenburg_tp_init_device);
