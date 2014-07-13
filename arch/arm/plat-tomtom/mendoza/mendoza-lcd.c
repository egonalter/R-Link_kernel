#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/list.h>
#include <linux/reboot.h>
#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <plat/devs.h>
#include <plat/fdt.h>
#include <plat/mendoza.h>
#include <plat/mendoza-lcd.h>
#include <plat/fb.h>


static LIST_HEAD(lcd_list);

static struct notifier_block mendoza_fb_reboot_block;

static struct s3c_platform_fb mendoza_fb_data __initdata = {
	.hw_ver	= 0x50,
	.clk_name = "lcd",
	.nr_wins = 3,
	.default_win = 0,
	.swap = FB_SWAP_WORD | FB_SWAP_HWORD,
};


void __init mendoza_lcd_lookup(void)
{
	struct lcd_paneldev *tmp;
	const char *screen;

	screen = fdt_get_string ("/features", "tft", "LMS430WQV");
	
	list_for_each_entry(tmp, &lcd_list, list) {
		if (!strcmp(tmp->panel->lcm_name, screen)) {	
			mendoza_fb_data.reset_lcd = tmp->panel->reset_lcd;
			mendoza_fb_data.lcd = tmp->panel->lcm_info;
			mendoza_fb_data.cfg_gpio = tmp->panel->gpio_setup;
			mendoza_fb_data.reset_lcd_onoff = tmp->panel->reset_power_onoff;	
			if (tmp->panel->shutdown) {
				mendoza_fb_reboot_block.notifier_call = tmp->panel->shutdown;
				register_reboot_notifier(&mendoza_fb_reboot_block);
			}
			break;
		}
	}
	
	if (screen == NULL) {
		printk("LCD: There is no default panel defined\n");
		return;
	}

	s3cfb_set_platdata(&mendoza_fb_data);/* see arch/arm/plat-sp5/dev-fb.c */
}

void __init mendoza_lcd_register (struct lcd_paneldev *dev)
{
	list_add_tail(&dev->list, &lcd_list);
}

void mendoza_lcd_unregister (struct lcd_paneldev *dev)
{
	list_del(&dev->list);
}
