#include <linux/can_micro.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/vgpio.h>
#include <linux/irq.h>
#include <linux/platform_device.h>
#include <linux/spi/spi.h>
#include <plat/offenburg.h>
#include <plat/mcspi.h>

static struct omap2_mcspi_device_config can_micro_spi_config = {
        .turbo_mode     = 0,
        .single_channel = 1,
};

struct spi_board_info can_micro_spi_bdinfo __initdata = {
	.modalias		= "spi_slave_buffered",
	.bus_num		= 1,
	.chip_select		= 0,
	.max_speed_hz		= 1500000,
	.controller_data	= &can_micro_spi_config,
};

static struct can_micro_dev strasbourg_can_micro = {
	.gpio_pin_can_resetin	= TT_VGPIO_CAN_RST_MON,
	.gpio_pin_can_resetout	= TT_VGPIO_CAN_RST,
	.gpio_pin_can_bt_mode	= TT_VGPIO_CAN_BT_MD,
	.gpio_pin_can_sync   	= TT_VGPIO_CAN_SYNC,
	.gpio_pin_can_reserv    = -1,
	.gpio_pin_mcu_pwr_on    = -1,
	.gpio_pin_pwr_5v_on_mcu = -1,
};

static struct resource can_micro_resources[] = {
	[0] = {
		.name  = "CAN IRQ",
		.start = 0, /* Fill in later from gpio_to_irq */
		.flags = IORESOURCE_IRQ | IORESOURCE_IRQ_LOWEDGE,
	},
};

static struct platform_device strasbourg_can_micro_pdev = {
	.name		   = "tomtomgo-canmicro",
	.id		   = 0,
	.dev.platform_data = &strasbourg_can_micro,
	.resource	   = can_micro_resources,
	.num_resources	   = ARRAY_SIZE(can_micro_resources),
};

static int __init offenburg_can_micro_init_device(void)
{
	int ret;

#define GPIO_INPUT 0
#define GPIO_OUTPUT 1
#define setup_gpio(p, n, d, v) __setup_gpio(strasbourg_can_micro.gpio_pin_ ## p, n, d, v)
	int __setup_gpio(unsigned pin, char *name, unsigned dir, unsigned val) {
		int r = gpio_request(pin, name);

		if (r) {
			printk(KERN_ERR "CAN micro: Can't request %s\n", name);
			return r;
		}
		if (dir == GPIO_INPUT)
			r = gpio_direction_input(pin);
		else
			r = gpio_direction_output(pin, val);

		return r;
	}

	ret  = setup_gpio(can_resetin,  "CAN_RESETIN",  GPIO_INPUT,  0);
	ret |= setup_gpio(can_resetout, "CAN_RESETOUT", GPIO_OUTPUT, 0);
	ret |= setup_gpio(can_bt_mode,  "CAN_BT_MODE",  GPIO_OUTPUT, 0);
	ret |= setup_gpio(can_sync,     "CAN_SYNC",     GPIO_OUTPUT, 1);

	if (ret)
		return ret;

	ret = spi_register_board_info(&can_micro_spi_bdinfo, 1);
	if (ret) {
		printk(KERN_ERR "Failed to register SPI board info for CAN micro\n");
		return ret;
	}

	ret = gpio_to_irq(strasbourg_can_micro.gpio_pin_can_resetin);
	if (ret < 0) {
		printk(KERN_ERR "Couldn't get irq number for CAN micro\n");
		/* IRQ won't be used ... */
	}
	strasbourg_can_micro_pdev.resource[0].start = ret;
	return platform_device_register(&strasbourg_can_micro_pdev);
}

arch_initcall(offenburg_can_micro_init_device);

