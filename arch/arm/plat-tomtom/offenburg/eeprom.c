#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/i2c/at24.h>
#include <linux/gpio.h>
#include <linux/vgpio.h>

#include <plat/offenburg.h>

#ifdef CONFIG_MACH_SANTIAGO
#define EEPROM_I2C_BUS	2
#elif defined CONFIG_MACH_MONOPOLI
#define EEPROM_I2C_BUS	2 /* TODO: change to 1 */
#else
#define EEPROM_I2C_BUS	3
#endif

static struct at24_platform_data board_eeprom = {
	.byte_len	= 8192,
	.page_size	= 32,
	.flags		= AT24_FLAG_IRUGO | AT24_FLAG_ADDR16,
};

struct i2c_board_info eeprom_i2c_board_info = {
	I2C_BOARD_INFO("at24", 0x50),
	.platform_data	= &board_eeprom,
};

static int eeprom_request_gpio(void)
{
	int err = 0;

#ifdef CONFIG_TOMTOM_PND
	/* Only enabled on PNDs */
	if (gpio_is_valid(TT_VGPIO_EEPROM_WP)) {
		if ((err = gpio_request(TT_VGPIO_EEPROM_WP, "TT_VGPIO_EEPROM_WP"))) {
			printk("%s: Can't request TT_VGPIO_EEPROM_WP GPIO\n", __func__);
			return err;
		}
	}

	gpio_direction_output(TT_VGPIO_EEPROM_WP, 0);
#endif

	return err;
}

static int __init eeprom_power(void)
{
	struct i2c_adapter	*adapter;

	eeprom_request_gpio();

	/* Insert the i2c device */
	adapter	= i2c_get_adapter(EEPROM_I2C_BUS /* XXX */);

	if (IS_ERR(adapter)) {
		printk("Error getting the i2c%d adapter: %ld\n", EEPROM_I2C_BUS, PTR_ERR(adapter));
		return PTR_ERR(adapter);
	}

	return IS_ERR(i2c_new_device(adapter, &eeprom_i2c_board_info));
}

late_initcall(eeprom_power);
