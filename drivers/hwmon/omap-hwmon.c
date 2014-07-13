/*
 * drivers/hwmon/omap-hwmon.c
 *
 * Copyright (C) 2011 Domenico Andreoli <cavokz@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/err.h>
#include <linux/platform_device.h>
#include <linux/hwmon.h>
#include <linux/hwmon-sysfs.h>
#include <linux/clk.h>

#include <plat/control.h>


#define debugk(fmt, ...)   { if (debug) printk(KERN_DEBUG fmt, ##__VA_ARGS__); }
#define dbg(fmt, ...)      debugk("%s - " fmt, __FUNCTION__, ##__VA_ARGS__)

#define MAX_WAIT_MS (3)

static unsigned int debug = 0;
module_param(debug, uint, S_IRUGO);
MODULE_PARM_DESC(debug, "Debug level");

struct omap_hwmon_data
{
	struct device *hwmon_dev;
	struct mutex update_lock;
	struct clk *ts_fck;

	/* sensor values */
	int temp;
};

#define ADC2TEMP_ENTRY(adc, from, to)      [adc] = ((from) + (((to) - (from)) / 2))
static const signed char reg2temp[128] = {
	ADC2TEMP_ENTRY(0 ... 12, -40, -40),
	ADC2TEMP_ENTRY(13, -40, -38),
	ADC2TEMP_ENTRY(14, -38, -35),
	ADC2TEMP_ENTRY(15, -35, -34),
	ADC2TEMP_ENTRY(16, -34, -32),
	ADC2TEMP_ENTRY(17, -32, -30),
	ADC2TEMP_ENTRY(18, -30, -28),
	ADC2TEMP_ENTRY(19, -28, -26),
	ADC2TEMP_ENTRY(20, -26, -24),
	ADC2TEMP_ENTRY(21, -24, -22),
	ADC2TEMP_ENTRY(22, -22, -20),
	ADC2TEMP_ENTRY(23, -20, -18.5),
	ADC2TEMP_ENTRY(24, -18.5, -17),
	ADC2TEMP_ENTRY(25, -17, -15),
	ADC2TEMP_ENTRY(26, -15, -13.5),
	ADC2TEMP_ENTRY(27, -13.5, -12),
	ADC2TEMP_ENTRY(28, -12, -10),
	ADC2TEMP_ENTRY(29, -10, -8),
	ADC2TEMP_ENTRY(30, -8, -6.5),
	ADC2TEMP_ENTRY(31, -6.5, -5),
	ADC2TEMP_ENTRY(32, -5, -3.5),
	ADC2TEMP_ENTRY(33, -3.5, -1.5),
	ADC2TEMP_ENTRY(34, -1.5, 0),
	ADC2TEMP_ENTRY(35, 0, 2),
	ADC2TEMP_ENTRY(36, 2, 3.5),
	ADC2TEMP_ENTRY(37, 3.5, 5),
	ADC2TEMP_ENTRY(38, 5, 6.5),
	ADC2TEMP_ENTRY(39, 6.5, 8.5),
	ADC2TEMP_ENTRY(40, 8.5, 10),
	ADC2TEMP_ENTRY(41, 10, 12),
	ADC2TEMP_ENTRY(42, 12, 13.5),
	ADC2TEMP_ENTRY(43, 13.5, 15),
	ADC2TEMP_ENTRY(44, 15, 17),
	ADC2TEMP_ENTRY(45, 17, 19),
	ADC2TEMP_ENTRY(46, 19, 21),
	ADC2TEMP_ENTRY(47, 21, 23),
	ADC2TEMP_ENTRY(48, 23, 25),
	ADC2TEMP_ENTRY(49, 25, 27),
	ADC2TEMP_ENTRY(50, 27, 28.5),
	ADC2TEMP_ENTRY(51, 28.5, 30),
	ADC2TEMP_ENTRY(52, 30, 32),
	ADC2TEMP_ENTRY(53, 32, 33.5),
	ADC2TEMP_ENTRY(54, 33.5, 35),
	ADC2TEMP_ENTRY(55, 35, 37),
	ADC2TEMP_ENTRY(56, 37, 38.5),
	ADC2TEMP_ENTRY(57, 38.5, 40),
	ADC2TEMP_ENTRY(58, 40, 42),
	ADC2TEMP_ENTRY(59, 42, 43.5),
	ADC2TEMP_ENTRY(60, 43.5, 45),
	ADC2TEMP_ENTRY(61, 45, 47),
	ADC2TEMP_ENTRY(62, 47, 48.5),
	ADC2TEMP_ENTRY(63, 48.5, 50),
	ADC2TEMP_ENTRY(64, 50, 52),
	ADC2TEMP_ENTRY(65, 52, 53.5),
	ADC2TEMP_ENTRY(66, 53.5, 55),
	ADC2TEMP_ENTRY(67, 55, 57),
	ADC2TEMP_ENTRY(68, 57, 58.5),
	ADC2TEMP_ENTRY(69, 58.5, 60),
	ADC2TEMP_ENTRY(70, 60, 62),
	ADC2TEMP_ENTRY(71, 62, 64),
	ADC2TEMP_ENTRY(72, 64, 66),
	ADC2TEMP_ENTRY(73, 66, 68),
	ADC2TEMP_ENTRY(74, 68, 70),
	ADC2TEMP_ENTRY(75, 70, 71.5),
	ADC2TEMP_ENTRY(76, 71.5, 73.5),
	ADC2TEMP_ENTRY(77, 73.5, 75),
	ADC2TEMP_ENTRY(78, 75, 77),
	ADC2TEMP_ENTRY(79, 77, 78.5),
	ADC2TEMP_ENTRY(80, 78.5, 80),
	ADC2TEMP_ENTRY(81, 80, 82),
	ADC2TEMP_ENTRY(82, 82, 83.5),
	ADC2TEMP_ENTRY(83, 83.5, 85),
	ADC2TEMP_ENTRY(84, 85, 87),
	ADC2TEMP_ENTRY(85, 87, 88.5),
	ADC2TEMP_ENTRY(86, 88.5, 90),
	ADC2TEMP_ENTRY(87, 90, 92),
	ADC2TEMP_ENTRY(88, 92, 93.5),
	ADC2TEMP_ENTRY(89, 93.5, 95),
	ADC2TEMP_ENTRY(90, 95, 97),
	ADC2TEMP_ENTRY(91, 97, 98.5),
	ADC2TEMP_ENTRY(92, 98.5, 100),
	ADC2TEMP_ENTRY(93, 100, 102),
	ADC2TEMP_ENTRY(94, 102, 103.5),
	ADC2TEMP_ENTRY(95, 103.5, 105),
	ADC2TEMP_ENTRY(96, 105, 107),
	ADC2TEMP_ENTRY(97, 107, 109),
	ADC2TEMP_ENTRY(98, 109, 111),
	ADC2TEMP_ENTRY(99, 111, 113),
	ADC2TEMP_ENTRY(100, 113, 115),
	ADC2TEMP_ENTRY(101, 115, 117),
	ADC2TEMP_ENTRY(102, 117, 118.5),
	ADC2TEMP_ENTRY(103, 118.5, 120),
	ADC2TEMP_ENTRY(104, 120, 122),
	ADC2TEMP_ENTRY(105, 122, 123.5),
	ADC2TEMP_ENTRY(106, 123.5, 125),
	ADC2TEMP_ENTRY(107 ... 127, 125, 125),
};
#undef ADC2TEMP_ENTRY

static int reg_to_temp(u32 reg)
{
	int temp = reg2temp[reg & 0x7f];
	dbg("reg: %u, temp: %d\n", reg & 0x7f, temp);
	return temp;
}

static inline bool wait_for_eocz(u32 level)
{
	unsigned long timeout;
	u32 temp_sensor_reg;

	level = !!level * OMAP343X_TEMP_EOCZ;

	timeout = jiffies + msecs_to_jiffies(MAX_WAIT_MS);
	do {
		temp_sensor_reg = omap_ctrl_readl(OMAP343X_CONTROL_TEMP_SENSOR);
		if ((temp_sensor_reg & OMAP343X_TEMP_EOCZ) == level)
			break;
		
		cpu_relax();
	} while (!time_after(jiffies, timeout));

	return ((temp_sensor_reg & OMAP343X_TEMP_EOCZ) == level);
}

static int omap_hwmon_stop_adc_conversion(struct omap_hwmon_data *data)
{
	int ret = 0;

	clk_enable(data->ts_fck);
	
	/* stop any conversion */
	dbg("stopping any conversion\n");
	omap_ctrl_writel(0, OMAP343X_CONTROL_TEMP_SENSOR);
	if (!wait_for_eocz(0)) {
		ret = -EBUSY;
		goto exit;
	}
exit:
	clk_disable(data->ts_fck);
	return ret;
}

static int omap_hwmon_do_adc_conversion(struct omap_hwmon_data *data, u32* val)
{
	int ret = 0;

	clk_enable(data->ts_fck);
	
	/* Wait for EOCZ to be idle */
	if (!wait_for_eocz(0)) {
		dbg("Timed out waiting for EOCZ to be idle");
		ret = -EBUSY;
		goto exit;
	}
	
	/* transit to high starts the conversion */
	dbg("starting single conversion\n");
	omap_ctrl_writel(OMAP343X_TEMP_SOC, OMAP343X_CONTROL_TEMP_SENSOR);	
	if (!wait_for_eocz(1)) {
		dbg("Timed out waiting for EOCZ to be high");
		ret = -EBUSY;
		goto exit;
	}

	/* ack conversion start */
	omap_ctrl_writel(0, OMAP343X_CONTROL_TEMP_SENSOR);	
	if (!wait_for_eocz(0)) {
		dbg("Timed out waiting for EOCZ to be low");
		ret = -EBUSY;
		goto exit;
	}
	*val = omap_ctrl_readl(OMAP343X_CONTROL_TEMP_SENSOR) & OMAP343X_TEMP_MASK;
exit:
	clk_disable(data->ts_fck);
	return ret;
}

static ssize_t show_temp(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct omap_hwmon_data *data = dev_get_drvdata(dev);
	u32 raw_temp = 0;
	
	mutex_lock(&data->update_lock);
	if (omap_hwmon_do_adc_conversion (data, &raw_temp) == 0) {
		data->temp = reg_to_temp(raw_temp);
	} else {
		/* For unknown reason EOCZ-to-high/low-timeout is happening
		 * occasionaly -> try to recover
		 */
		omap_ctrl_writel(0, OMAP343X_CONTROL_TEMP_SENSOR);
	}
	mutex_unlock(&data->update_lock);

	return sprintf(buf, "%d\n", 1000*data->temp);
}

static struct sensor_device_attribute omap_hwmon_sensors[] = {
	SENSOR_ATTR(temp1_input, S_IRUGO, show_temp, NULL, 0),
};

static void omap_hwmon_device_remove_files(struct device *dev)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(omap_hwmon_sensors); i++)
		device_remove_file(dev, &omap_hwmon_sensors[i].dev_attr);
}

static int __devinit omap_hwmon_init_bgapts(struct platform_device *pdev)
{
	int ret = 0;
	u32 bgapts;
	struct device *dev = &pdev->dev;
	struct omap_hwmon_data *data = platform_get_drvdata(pdev);
	
	data->ts_fck = clk_get(dev, "ts_fck");
	if (!data->ts_fck) {
		printk (KERN_ERR "cannot obtain ts_fck clock\n");
		ret = -EPERM;
		goto exit;
	}
	
	bgapts = omap_ctrl_readl(OMAP343X_CONTROL_WKUP_BGAPTS);
	bgapts &= ~(OMAP343X_BGAPTS_BGROFF | OMAP343X_BGAPTS_TMPSOFF);
	omap_ctrl_writel(bgapts, OMAP343X_CONTROL_WKUP_BGAPTS);
exit:
	return ret;
}

static void __devinit omap_hwmon_exit_bgapts(struct platform_device *pdev)
{
	struct omap_hwmon_data *data = platform_get_drvdata(pdev);
	clk_put(data->ts_fck);
}

static int __devinit omap_hwmon_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct omap_hwmon_data *data;
	int ret, i;

	data = kzalloc(sizeof(*data), GFP_KERNEL);
	if (!data) {
		ret = -ENOMEM;
		goto exit;
	}

	mutex_init(&data->update_lock);
	platform_set_drvdata(pdev, data);
	ret = omap_hwmon_init_bgapts(pdev);
	if (ret < 0)
		goto err_init_bgapts;

	data->temp = -1;
	for (i = 0; i < ARRAY_SIZE(omap_hwmon_sensors); i++) {
		ret = device_create_file(dev, &omap_hwmon_sensors[i].dev_attr);
		if (ret < 0)
			goto err_device_create;
	}

	data->hwmon_dev = hwmon_device_register(dev);
	if (IS_ERR(data->hwmon_dev)) {
		ret = PTR_ERR(data->hwmon_dev);
		goto err_hwmon;
	}
	
	return 0;
err_hwmon:
err_device_create:
	omap_hwmon_device_remove_files(dev);
	omap_hwmon_exit_bgapts(pdev);
err_init_bgapts:
	platform_set_drvdata(pdev, NULL);
	kfree(data);
exit: 
	return ret;
}

static int __devexit omap_hwmon_remove(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct omap_hwmon_data *data = platform_get_drvdata(pdev);

	hwmon_device_unregister(data->hwmon_dev);
	omap_hwmon_device_remove_files(dev);
	omap_hwmon_stop_adc_conversion(data);
	omap_hwmon_exit_bgapts(pdev);
	platform_set_drvdata(pdev, NULL);
	kfree(data);
	return 0;
}

static struct platform_driver omap_hwmon_driver = {
	.driver = {
		.name = "omap-hwmon",
		.owner = THIS_MODULE,
	},
	.probe = omap_hwmon_probe,
	.remove = __devexit_p(omap_hwmon_remove),
};

static int __init omap_hwmon_init(void)
{
	printk(KERN_INFO "OMAP hwmon driver\n");
	return platform_driver_register(&omap_hwmon_driver);
}

static void __exit omap_hwmon_exit(void)
{
	platform_driver_unregister(&omap_hwmon_driver);
}

module_init(omap_hwmon_init);
module_exit(omap_hwmon_exit);

MODULE_AUTHOR("Domenico Andreoli <cavokz@gmail.com>");
MODULE_DESCRIPTION("TI OMAP SoC hwmon driver");
MODULE_LICENSE("GPL");
