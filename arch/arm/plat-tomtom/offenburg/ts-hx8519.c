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

struct hx8519_platform_data hx8519_info = {
	.model			= 8519,
	.x_plate_ohms		= 180 /*937*/,
	.get_pendown_state	= ts_get_pendown_state,
	.init_platform_hw	= ts_init,
};

struct i2c_board_info hx8519_i2c_board_info = {
	I2C_BOARD_INFO("hx8519", 0x4b),
	.platform_data	= &hx8519_info,
};

static int __init offenburg_tp_init_device(void)
{
	struct i2c_adapter	*adapter;
	/* Reset the chip */
	if (machine_is_strasbourg_a2()) 
{
		gpio_request(TT_VGPIO_TP_RESET, "tsc2017 reset");	/* Not checking the return value, the vgpios will have crashed before this call if it's not available. */
		gpio_direction_output(TT_VGPIO_TP_RESET, 0);
		mdelay(1);
		gpio_set_value(TT_VGPIO_TP_RESET, 1);
		mdelay(1);
		gpio_set_value(TT_VGPIO_TP_RESET, 0);
	}

	hx8519_i2c_board_info.irq	= gpio_to_irq(TT_VGPIO_TP_IRQ);

#if 0
	TT_LINE_CONFIG(TT_VGPIO_I2C2_POWER);
#endif

	/* Add a new device on the bus. */
	adapter	= i2c_get_adapter(2);

	if (IS_ERR(adapter)) {
		printk("Could not get adapter: %ld\n", PTR_ERR(adapter));
		return PTR_ERR(adapter);
	}

	return IS_ERR(i2c_new_device(adapter, &hx8519_i2c_board_info));
}

late_initcall(offenburg_tp_init_device);
