#include <linux/i2c.h>
#include <linux/err.h>
#include <linux/input/i3g4250d.h>

static struct i3g4250d_platform_data i3g4250d_pdata = {
	.poll_interval		= 20,
	.min_interval		= 2,
	.fs_range		= I3G4250D_FS_245DPS,
	.avg_samples		= 10,
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

static struct i2c_board_info __initdata i3g4250d_i2c_boardinfo[] = {
	{
		I2C_BOARD_INFO(I3G4250D_DEV_NAME, I3G4250D_I2C_SAD_L),
		.platform_data = &i3g4250d_pdata,
	},
};

static int __init i3g4250d_init(void)
{
	struct i2c_adapter	*adapter;

	/* Insert the i2c device */
	adapter	= i2c_get_adapter(3);

	if (IS_ERR(adapter)) {
		printk("Error getting adapter: %ld\n", PTR_ERR(adapter));
		return PTR_ERR(adapter);
	}

	return IS_ERR(i2c_new_device(adapter, &i3g4250d_i2c_boardinfo));
}

late_initcall(i3g4250d_init);
