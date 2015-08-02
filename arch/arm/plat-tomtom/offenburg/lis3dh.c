#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/input/lis3dh.h>

static struct lis3dh_acc_platform_data lis3dh_acc_pdata = {
	.poll_interval		= 10,
	.min_interval		= 1,
	.avg_samples		= 20,
	.g_range		= LIS3DH_ACC_G_2G,
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

static struct i2c_board_info __initdata lis3dh_i2c_boardinfo[] = {
	{
		I2C_BOARD_INFO(LIS3DH_ACC_DEV_NAME, LIS3DH_ACC_I2C_SAD_L),
		.platform_data = &lis3dh_acc_pdata,
	},
};

static int __init lis3dh_init(void)
{
	struct i2c_adapter	*adapter;

	/* Insert the i2c device */
	adapter	= i2c_get_adapter(3);

	if (IS_ERR(adapter)) {
		printk("Error getting adapter: %ld\n", PTR_ERR(adapter));
		return PTR_ERR(adapter);
	}

	return IS_ERR(i2c_new_device(adapter, &lis3dh_i2c_boardinfo));
}

late_initcall(lis3dh_init);
