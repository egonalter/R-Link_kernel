#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/gpio.h>
#include <linux/vgpio.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/interrupt.h>
#include <linux/regulator/consumer.h>
#include <linux/i2c/atmel_mxt_ts.h>
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

static struct mxt_platform_data mxt_info = {
	.irqflags = IRQF_TRIGGER_LOW | IRQF_ONESHOT,
};

static struct i2c_board_info mxt_i2c_board_info = {
	I2C_BOARD_INFO("atmel_mxt_ts", 0x4a),
	.platform_data	= &mxt_info,
};

static const char *supported_screens[] = {
	"DD070NA_A336",
	"DD070NA_A336_1",
	"DD070NA_A336_2",
};

/* these screens use the atmxt TP */
static int __init offenburg_atmxt_lcm(void)
{
	int i;
	const char *screen = fdt_get_string ("/features", "tft", DEFAULT_LCM);

	for (i = 0; i < ARRAY_SIZE(supported_screens); i++) {
		if (strcmp(screen, supported_screens[i]) == 0) {
			mxt_info.screen_name = supported_screens[i];
			return 1;
		}
	}

	return 0;
}

static int __init offenburg_tp_init_device(void)
{
	struct i2c_adapter	*adapter;

	/* init atmxt only if required for the configured screen */
	if (!offenburg_atmxt_lcm())
		return 0;

	/* Reset the chip in the case of the tsc2017 */
	if (machine_is_strasbourg_a2()) {
		gpio_request(TT_VGPIO_TP_RESET, "mxt_ts reset");	/* Not checking the return value, the vgpios will have crashed before this call if it's not available. */
		gpio_direction_output(TT_VGPIO_TP_RESET, 0);
	}

	mxt_i2c_board_info.irq	= gpio_to_irq(TT_VGPIO_TP_IRQ);

	/* Add a new device on the bus. */
	adapter	= i2c_get_adapter(2);

	if (IS_ERR(adapter)) {
		printk("Could not get adapter: %ld\n", PTR_ERR(adapter));
		return PTR_ERR(adapter);
	}

	return IS_ERR(i2c_new_device(adapter, &mxt_i2c_board_info));
}

late_initcall(offenburg_tp_init_device);
