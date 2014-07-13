#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/list.h>
#include <linux/timer.h>
#include <linux/delay.h>
#include <linux/init.h>
#include <linux/serial_core.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/vgpio.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/clk.h>
#include <linux/pwm_backlight.h>
#include <linux/spi/spi.h>

#include <asm/mach/arch.h>
#include <asm/mach/map.h>
#include <asm/mach/irq.h>

#include <mach/hardware.h>
#include <mach/map.h>
#include <mach/regs-mem.h>

#include <asm/irq.h>
#include <asm/mach-types.h>

#include <plat/regs-serial.h>
#include <plat/iic.h>

#include <plat/fb.h>

#include <plat/regs-otg.h>
#include <plat/s5p6450.h>
#include <plat/clock.h>
#include <mach/regs-clock.h>
#include <plat/devs.h>
#include <plat/cpu.h>
#include <plat/ts.h>
#include <plat/mendoza.h>
#include <plat/fdt.h>
#include <plat/mendoza-lcd.h>
#include <plat/mendoza-lcd-data.h>

#include <plat/adc.h>
#include <plat/pm.h>
#include <plat/pll.h>
#include <plat/regs-rtc.h>
#include <plat/spi.h>
#include <plat/media.h>
#include <mach/gpio.h>
#include <mach/regs-gpio.h>
#include <plat/gpio-cfg.h>
#include <plat/gpio-bank-c.h>
#include <plat/gpio-bank-n.h>
#include <plat/gpio-bank-f.h>
#include <plat/mshci.h>

#include <linux/regulator/machine.h>
#include <linux/mfd/s5m8751/core.h>
#include <linux/mfd/s5m8751/pdata.h>

#ifdef CONFIG_USB_SUPPORT
void usb_host_phy_init(void)
{
	struct clk *otg_clk;

	otg_clk = clk_get(NULL, "usbotg");
	clk_enable(otg_clk);

	if (readl(S5P_OTHERS) & (0x1<<16))
		return;

	__raw_writel(__raw_readl(S5P_OTHERS)
		|(0x1<<16), S5P_OTHERS);
	
	__raw_writel(__raw_readl(S5P_OTHERS)
		|(0x0<<17), S5P_OTHERS);

	__raw_writel((__raw_readl(S3C_USBOTG_PHYPWR)
		&~(0x1<<7)&~(0x1<<6))|(0x1<<8)|(0x1<<5), S3C_USBOTG_PHYPWR);
	__raw_writel((__raw_readl(S3C_USBOTG_PHYCLK)
		&~(0x1<<7))|(0x1<<0), S3C_USBOTG_PHYCLK);
	__raw_writel((__raw_readl(S3C_USBOTG_RSTCON))
		|(0x1<<4)|(0x1<<3), S3C_USBOTG_RSTCON);
	__raw_writel(__raw_readl(S3C_USBOTG_RSTCON)
		&~(0x1<<4)&~(0x1<<3), S3C_USBOTG_RSTCON);
}
EXPORT_SYMBOL(usb_host_phy_init);

void usb_host_phy_off(void)
{
	__raw_writel(__raw_readl(S3C_USBOTG_PHYPWR)
		|(0x1<<7)|(0x1<<6), S3C_USBOTG_PHYPWR);
	__raw_writel(__raw_readl(S5P_OTHERS)
		&~(1<<16), S5P_OTHERS);
}
EXPORT_SYMBOL(usb_host_phy_off);

/* Initializes OTG Phy. */
void otg_phy_init(void) 
{
	writel(readl(S5P_OTHERS)&~S5P_OTHERS_USB_SIG_MASK, S5P_OTHERS);
	writel(readl(S3C_USBOTG_PHYPWR) & ~0x1F, S3C_USBOTG_PHYPWR);		/* Power up */

	if (machine_is_valdez()) {
		writel(0x21, S3C_USBOTG_PHYCLK);
	} else if (machine_is_taranto()) {
		writel(0x61, S3C_USBOTG_PHYCLK);
	} else {
		printk ("WARNING: Assuming clock source for USB OTG PHY configuration is XXTI (TARANTO configuration)\n");
		writel(0x61, S3C_USBOTG_PHYCLK);
	}
	writel(0x1, S3C_USBOTG_RSTCON);

	udelay(50);
	writel(0x0, S3C_USBOTG_RSTCON);
	udelay(50);
}
EXPORT_SYMBOL(otg_phy_init);

/* OTG PHY Power Off */
void otg_phy_off(void) 
{
	writel(readl(S3C_USBOTG_PHYPWR)|(0x1F<<1), S3C_USBOTG_PHYPWR);
	writel(readl(S5P_OTHERS)&~S5P_OTHERS_USB_SIG_MASK, S5P_OTHERS);
//	gpio_free(S5P64XX_GPN(1));
}
EXPORT_SYMBOL(otg_phy_off);

#endif

#if defined(CONFIG_RTC_DRV_S3C)
/* RTC common Function for samsung APs*/
unsigned int s3c_rtc_set_bit_byte(void __iomem *base, uint offset, uint val)
{
	writeb(val, base + offset);

	return 0;
}

unsigned int s3c_rtc_read_alarm_status(void __iomem *base)
{
	return 1;
}

void s3c_rtc_set_pie(void __iomem *base, uint to)
{
	unsigned int tmp;

	tmp = readw(base + S3C2410_RTCCON) & ~S3C_RTCCON_TICEN;

	if (to)
		tmp |= S3C_RTCCON_TICEN;

	writew(tmp, base + S3C2410_RTCCON);
}

void s3c_rtc_set_freq_regs(void __iomem *base, uint freq, uint s3c_freq)
{
	unsigned int tmp;

	tmp = readw(base + S3C2410_RTCCON) & (S3C_RTCCON_TICEN | S3C2410_RTCCON_RTCEN );
	writew(tmp, base + S3C2410_RTCCON);
	s3c_freq = freq;
	tmp = (32768 / freq)-1;
	writel(tmp, base + S3C2410_TICNT);
}

void s3c_rtc_enable_set(struct platform_device *pdev,void __iomem *base, int en)
{
	unsigned int tmp;

	if (!en) {
		tmp = readw(base + S3C2410_RTCCON);
		writew(tmp & ~ (S3C2410_RTCCON_RTCEN | S3C_RTCCON_TICEN), base + S3C2410_RTCCON);
	} else {
		/* re-enable the device, and check it is ok */
		if ((readw(base+S3C2410_RTCCON) & S3C2410_RTCCON_RTCEN) == 0){
			dev_info(&pdev->dev, "rtc disabled, re-enabling\n");

			tmp = readw(base + S3C2410_RTCCON);
			writew(tmp|S3C2410_RTCCON_RTCEN, base+S3C2410_RTCCON);
		}

		if ((readw(base + S3C2410_RTCCON) & S3C2410_RTCCON_CNTSEL)){
			dev_info(&pdev->dev, "removing RTCCON_CNTSEL\n");

			tmp = readw(base + S3C2410_RTCCON);
			writew(tmp& ~S3C2410_RTCCON_CNTSEL, base+S3C2410_RTCCON);
		}

		if ((readw(base + S3C2410_RTCCON) & S3C2410_RTCCON_CLKRST)){
			dev_info(&pdev->dev, "removing RTCCON_CLKRST\n");

			tmp = readw(base + S3C2410_RTCCON);
			writew(tmp & ~S3C2410_RTCCON_CLKRST, base+S3C2410_RTCCON);
		}
	}
}
#endif

void s3c_setup_uart_cfg_gpio(unsigned char port)
{
		switch(port)
		{
		case 0: 
			s3c_gpio_cfgpin(S5P64XX_GPA(0), S3C_GPIO_SFN(2));
			s3c_gpio_setpull(S5P64XX_GPA(0), S3C_GPIO_PULL_NONE);
			s3c_gpio_cfgpin(S5P64XX_GPA(1), S3C_GPIO_SFN(2));
			s3c_gpio_setpull(S5P64XX_GPA(1), S3C_GPIO_PULL_NONE);
			s3c_gpio_cfgpin(S5P64XX_GPA(2), S3C_GPIO_SFN(2));
			s3c_gpio_setpull(S5P64XX_GPA(2), S3C_GPIO_PULL_NONE);
			s3c_gpio_cfgpin(S5P64XX_GPA(3), S3C_GPIO_SFN(2));
			s3c_gpio_setpull(S5P64XX_GPA(3), S3C_GPIO_PULL_NONE);
			break;
		case 1:
			s3c_gpio_cfgpin(S5P64XX_GPA(4), S3C_GPIO_SFN(2));
			s3c_gpio_setpull(S5P64XX_GPA(4), S3C_GPIO_PULL_NONE);
			s3c_gpio_cfgpin(S5P64XX_GPA(5), S3C_GPIO_SFN(2));
			s3c_gpio_setpull(S5P64XX_GPA(5), S3C_GPIO_PULL_NONE);
			s3c_gpio_cfgpin(S5P64XX_GPB(5), S3C_GPIO_SFN(3));
			s3c_gpio_setpull(S5P64XX_GPB(5), S3C_GPIO_PULL_NONE);
			s3c_gpio_cfgpin(S5P64XX_GPB(6), S3C_GPIO_SFN(3));
			s3c_gpio_setpull(S5P64XX_GPB(6), S3C_GPIO_PULL_NONE);
			break;
		case 2:
			s3c_gpio_cfgpin(S5P64XX_GPB(0), S3C_GPIO_SFN(2));
			s3c_gpio_setpull(S5P64XX_GPB(0), S3C_GPIO_PULL_NONE);
			s3c_gpio_cfgpin(S5P64XX_GPB(1), S3C_GPIO_SFN(2));
			s3c_gpio_setpull(S5P64XX_GPB(1), S3C_GPIO_PULL_NONE);
			break;
		case 3:
			s3c_gpio_cfgpin(S5P64XX_GPB(2), S3C_GPIO_SFN(2));
			s3c_gpio_setpull(S5P64XX_GPB(2), S3C_GPIO_PULL_NONE);
			s3c_gpio_cfgpin(S5P64XX_GPB(3), S3C_GPIO_SFN(2));
			s3c_gpio_setpull(S5P64XX_GPB(3), S3C_GPIO_PULL_NONE);
			break;
		case 4:
			s3c_gpio_cfgpin(S5P64XX_GPD(0), S3C_GPIO_SFN(2));
			s3c_gpio_setpull(S5P64XX_GPD(0), S3C_GPIO_PULL_NONE);
			s3c_gpio_cfgpin(S5P64XX_GPD(1), S3C_GPIO_SFN(2));
			s3c_gpio_setpull(S5P64XX_GPD(1), S3C_GPIO_PULL_NONE);
			s3c_gpio_cfgpin(S5P64XX_GPD(4), S3C_GPIO_SFN(2));
			s3c_gpio_setpull(S5P64XX_GPD(4), S3C_GPIO_PULL_NONE);
			s3c_gpio_cfgpin(S5P64XX_GPD(5), S3C_GPIO_SFN(2));
			s3c_gpio_setpull(S5P64XX_GPD(5), S3C_GPIO_PULL_NONE);
			break;
		case 5:
			s3c_gpio_cfgpin(S5P64XX_GPD(2), S3C_GPIO_SFN(2));
			s3c_gpio_setpull(S5P64XX_GPD(2), S3C_GPIO_PULL_NONE);
			s3c_gpio_cfgpin(S5P64XX_GPD(3), S3C_GPIO_SFN(2));
			s3c_gpio_setpull(S5P64XX_GPD(3), S3C_GPIO_PULL_NONE);
			s3c_gpio_cfgpin(S5P64XX_GPD(6), S3C_GPIO_SFN(3));
			s3c_gpio_setpull(S5P64XX_GPD(6), S3C_GPIO_PULL_NONE);
			s3c_gpio_cfgpin(S5P64XX_GPD(7), S3C_GPIO_SFN(3));
			s3c_gpio_setpull(S5P64XX_GPD(7), S3C_GPIO_PULL_NONE);
			break;

		default:
			break;
	}
}
EXPORT_SYMBOL(s3c_setup_uart_cfg_gpio);
