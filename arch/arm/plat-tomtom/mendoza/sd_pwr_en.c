#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/vgpio.h>
#include <linux/io.h>
#include <linux/interrupt.h>
#include <plat/mendoza.h>


static int sd_pwr_en_probe(struct platform_device *pdev)
{
	return gpio_request(TT_VGPIO_SD_PWR_EN, "sd_pwr_en") ||
	       gpio_direction_output(TT_VGPIO_SD_PWR_EN, 1);
}

#ifdef CONFIG_PM

static int sd_pwr_en_suspend(struct platform_device *pdev, pm_message_t state)
{
	return gpio_direction_output(TT_VGPIO_SD_PWR_EN, 0);
}

/* Do an early resume because of HWEG-366 */ 
static int sd_pwr_en_resume(struct device *dev)
{
	return gpio_direction_output(TT_VGPIO_SD_PWR_EN, 1);
}

static const struct dev_pm_ops sd_dev_pm_ops = {
	.resume_noirq	= sd_pwr_en_resume,
};

#define SD_DEV_PM_OPS (&sd_dev_pm_ops)

#else

static int sd_pwr_en_suspend(struct platform_device *pdev, pm_message_t state)
{
	return gpio_direction_output(TT_VGPIO_SD_PWR_EN, 1);
}

#define SD_DEV_PM_OPS NULL

#endif /*CONFIG_PM*/

static struct platform_driver sd_pwr_en_driver = {
	.probe   = sd_pwr_en_probe,
	.suspend = sd_pwr_en_suspend,
	.driver  = {
		.owner	= THIS_MODULE,
		.name	= "sd_pwr_en",
		.pm	= SD_DEV_PM_OPS,
	},
};

static int __init sd_pwr_en_init(void)
{
	int ret;

	ret = platform_driver_register(&sd_pwr_en_driver);
	if (ret)
		printk(KERN_ERR "Error creating the SD_PWR_EN platform driver\n");

	return ret;
}

arch_initcall(sd_pwr_en_init);

