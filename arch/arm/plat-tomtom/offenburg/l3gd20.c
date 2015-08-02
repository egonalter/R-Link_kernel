#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/input/l3gd20.h>

static struct l3gd20_gyr_platform_data l3gd20_gyr_pdata = {
	.poll_interval		= 10,
	.min_interval		= 1,
	.fs_range		= L3GD20_GYR_FS_250DPS,
	.avg_samples		= 20,
	.axis_map_x		= 0,
	.axis_map_y		= 1,
	.axis_map_z		= 2,
	.negate_x		= 0,
	.negate_y		= 0,
	.negate_z		= 0,
	.init			= NULL,
	.exit			= NULL,
	.power_on		= NULL,
	.power_off		= NULL,
	.gpio_int1		= -EINVAL,
	.gpio_int2		= -EINVAL,
};

static struct i2c_board_info __initdata l3gd20_i2c_boardinfo[] = {
	{
		I2C_BOARD_INFO(L3GD20_GYR_DEV_NAME, L3GD20_GYR_I2C_SAD_L),
		.platform_data = &l3gd20_gyr_pdata,
	},
};

static int __init l3gd20_init(void)
{
	struct i2c_adapter	*adapter;

	/* Insert the i2c device */
	adapter	= i2c_get_adapter(3);

	if (IS_ERR(adapter)) {
		printk("Error getting adapter: %ld\n", PTR_ERR(adapter));
		return PTR_ERR(adapter);
	}

	return IS_ERR(i2c_new_device(adapter, &l3gd20_i2c_boardinfo));
}

late_initcall(l3gd20_init);
