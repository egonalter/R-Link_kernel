/*
 * s5m8752-core.c  --  S5M8752 Advanced PMIC with AUDIO Codec
 *
 * Copyright 2010 Samsung Electronics.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 */

#include <linux/irq.h>
#include <linux/mfd/core.h>
#include <linux/slab.h>
#include <linux/io.h>
#include <linux/interrupt.h>

#include <mach/gpio.h>
#include <mach/regs-gpio.h>

#include <linux/mfd/s5m8752/core.h>
#include <linux/mfd/s5m8752/pdata.h>

static DEFINE_MUTEX(io_mutex);

unsigned int s5m8752_dvs2a, s5m8752_dvs2b;
unsigned int s5m8752_dvs3a, s5m8752_dvs3b;

/******************************************************************************
 * This function gets the value of the masked bit.
 ******************************************************************************/
int s5m8752_get_bit(struct s5m8752 *s5m8752, uint8_t reg, int bit)
{
	uint8_t reg_val;
	int ret = 0;

	ret = s5m8752_reg_read(s5m8752, reg, &reg_val);
	if (ret)
		return ret;

	reg_val &= (1 << bit);
	reg_val = reg_val >> bit;

	return reg_val;
}
EXPORT_SYMBOL_GPL(s5m8752_get_bit);

/******************************************************************************
 * This function clears the masked bit to "0".
 ******************************************************************************/
int s5m8752_clear_bits(struct s5m8752 *s5m8752, uint8_t reg, uint8_t mask)
{
	uint8_t reg_val;
	int ret = 0;

	ret = s5m8752_reg_read(s5m8752, reg, &reg_val);
	if (ret)
		return ret;

	reg_val &= ~mask;
	ret = s5m8752_reg_write(s5m8752, reg, reg_val);

	return ret;
}
EXPORT_SYMBOL_GPL(s5m8752_clear_bits);

/******************************************************************************
 * This function sets the masked bit to "1".
 ******************************************************************************/
int s5m8752_set_bits(struct s5m8752 *s5m8752, uint8_t reg, uint8_t mask)
{
	uint8_t reg_val;
	int ret = 0;

	ret = s5m8752_reg_read(s5m8752, reg, &reg_val);
	if (ret)
		return ret;

	reg_val |= mask;
	ret = s5m8752_reg_write(s5m8752, reg, reg_val);

	return ret;
}
EXPORT_SYMBOL_GPL(s5m8752_set_bits);

/******************************************************************************
 * This function calls S5M8752 i2c byte read function.
 ******************************************************************************/
int s5m8752_reg_read(struct s5m8752 *s5m8752, uint8_t reg, uint8_t *val)
{
	int ret = 0;

	mutex_lock(&io_mutex);
	ret = s5m8752->read_dev(s5m8752, reg, val);
	if (ret < 0)
		dev_err(s5m8752->dev, "failed reading from 0x%02x\n", reg);

	mutex_unlock(&io_mutex);
	return ret;
}
EXPORT_SYMBOL_GPL(s5m8752_reg_read);

/******************************************************************************
 * This function calls S5M8752 i2c byte write function.
 ******************************************************************************/
int s5m8752_reg_write(struct s5m8752 *s5m8752, uint8_t reg, uint8_t val)
{
	int ret = 0;

	mutex_lock(&io_mutex);
	ret = s5m8752->write_dev(s5m8752, reg, val);
	if (ret < 0)
		dev_err(s5m8752->dev, "failed writing 0x%02x\n", reg);

	mutex_unlock(&io_mutex);
	return ret;
}
EXPORT_SYMBOL_GPL(s5m8752_reg_write);

/******************************************************************************
 * This function calls S5M8752 i2c block read function.
 ******************************************************************************/
int s5m8752_block_read(struct s5m8752 *s5m8752, uint8_t reg, int len,
							uint8_t *val)
{
	int ret = 0;

	mutex_lock(&io_mutex);
	ret = s5m8752->read_block_dev(s5m8752, reg, len, val);
	if (ret < 0)
		dev_err(s5m8752->dev, "failed reading from 0x%02x\n", reg);

	mutex_unlock(&io_mutex);
	return ret;
}
EXPORT_SYMBOL_GPL(s5m8752_block_read);

/******************************************************************************
 * This function calls S5M8752 i2c block write function.
 ******************************************************************************/
int s5m8752_block_write(struct s5m8752 *s5m8752, uint8_t reg, int len,
							uint8_t *val)
{
	int ret = 0;

	mutex_lock(&io_mutex);
	ret = s5m8752->write_block_dev(s5m8752, reg, len, val);
	if (ret < 0)
		dev_err(s5m8752->dev, "failed writings to 0x%02x\n", reg);

	mutex_unlock(&io_mutex);
	return ret;
}
EXPORT_SYMBOL_GPL(s5m8752_block_write);

static struct resource s5m8752_sdc1_resource[] = {
	{
		.start = S5M8752_SDC1_SET,
		.end = S5M8752_SDC1_VOL,
		.flags = IORESOURCE_IO,
	},
};

static struct resource s5m8752_sdc2_resource[] = {
	{
		.start = S5M8752_SDC2_SET,
		.end = S5M8752_SDC2_DVS1,
		.flags = IORESOURCE_IO,
	},
};

static struct resource s5m8752_sdc3_resource[] = {
	{
		.start = S5M8752_SDC3_SET,
		.end = S5M8752_SDC3_DVS1,
		.flags = IORESOURCE_IO,
	},
};

static struct resource s5m8752_sdc4_resource[] = {
	{
		.start = S5M8752_SDC4_SET,
		.end = S5M8752_SDC4_VOL,
		.flags = IORESOURCE_IO,
	},
};

static struct resource s5m8752_ldo_ka_resource[] = {
	{
		.start = S5M8752_LDO_KA_SET,
		.end = S5M8752_LDO_KA_SET,
		.flags = IORESOURCE_IO,
	},
};

static struct resource s5m8752_ldo1_resource[] = {
	{
		.start = S5M8752_LDO1_SET,
		.end = S5M8752_LDO1_SET,
		.flags = IORESOURCE_IO,
	},
};

static struct resource s5m8752_ldo2_resource[] = {
	{
		.start = S5M8752_LDO2_SET,
		.end = S5M8752_LDO2_SET,
		.flags = IORESOURCE_IO,
	},
};

static struct resource s5m8752_ldo3_resource[] = {
	{
		.start = S5M8752_LDO3_SET,
		.end = S5M8752_LDO3_SET,
		.flags = IORESOURCE_IO,
	},
};

static struct resource s5m8752_ldo4_resource[] = {
	{
		.start = S5M8752_LDO4_SET,
		.end = S5M8752_LDO4_SET,
		.flags = IORESOURCE_IO,
	},
};

static struct resource s5m8752_ldo5_resource[] = {
	{
		.start = S5M8752_LDO5_SET,
		.end = S5M8752_LDO5_SET,
		.flags = IORESOURCE_IO,
	},
};

static struct resource s5m8752_ldo6_resource[] = {
	{
		.start = S5M8752_LDO6_SET,
		.end = S5M8752_LDO6_SET,
		.flags = IORESOURCE_IO,
	},
};

static struct resource s5m8752_ldo7_resource[] = {
	{
		.start = S5M8752_LDO7_SET,
		.end = S5M8752_LDO7_SET,
		.flags = IORESOURCE_IO,
	},
};

static struct mfd_cell regulator_devs[] = {
	{
		.name = "s5m8752-regulator",
		.id = 0,
		.num_resources = ARRAY_SIZE(s5m8752_sdc1_resource),
		.resources = s5m8752_sdc1_resource,
	},
	{
		.name = "s5m8752-regulator",
		.id = 1,
		.num_resources = ARRAY_SIZE(s5m8752_sdc2_resource),
		.resources = s5m8752_sdc2_resource,
	},
	{
		.name = "s5m8752-regulator",
		.id = 2,
		.num_resources = ARRAY_SIZE(s5m8752_sdc3_resource),
		.resources = s5m8752_sdc3_resource,
	},
	{
		.name = "s5m8752-regulator",
		.id = 3,
		.num_resources = ARRAY_SIZE(s5m8752_sdc4_resource),
		.resources = s5m8752_sdc4_resource,
	},
	{
		.name = "s5m8752-regulator",
		.id = 4,
		.num_resources = ARRAY_SIZE(s5m8752_ldo_ka_resource),
		.resources = s5m8752_ldo_ka_resource,
	},
	{
		.name = "s5m8752-regulator",
		.id = 5,
		.num_resources = ARRAY_SIZE(s5m8752_ldo1_resource),
		.resources = s5m8752_ldo1_resource,
	},
	{
		.name = "s5m8752-regulator",
		.id = 6,
		.num_resources = ARRAY_SIZE(s5m8752_ldo2_resource),
		.resources = s5m8752_ldo2_resource,
	},
	{
		.name = "s5m8752-regulator",
		.id = 7,
		.num_resources = ARRAY_SIZE(s5m8752_ldo3_resource),
		.resources = s5m8752_ldo3_resource,
	},
	{
		.name = "s5m8752-regulator",
		.id = 8,
		.num_resources = ARRAY_SIZE(s5m8752_ldo4_resource),
		.resources = s5m8752_ldo4_resource,
	},
	{
		.name = "s5m8752-regulator",
		.id = 9,
		.num_resources = ARRAY_SIZE(s5m8752_ldo5_resource),
		.resources = s5m8752_ldo5_resource,
	},
	{
		.name = "s5m8752-regulator",
		.id = 10,
		.num_resources = ARRAY_SIZE(s5m8752_ldo6_resource),
		.resources = s5m8752_ldo6_resource,
	},
	{
		.name = "s5m8752-regulator",
		.id = 11,
		.num_resources = ARRAY_SIZE(s5m8752_ldo7_resource),
		.resources = s5m8752_ldo7_resource,
	},

};

static struct mfd_cell backlight_devs[] = {
	{
		.name = "s5m8752-backlight",
		.id = -1,
	},
};

static struct resource power_supply_resources[] = {
	{
		.name = "s5m8752-power",
		.start = S5M8752_IRQB_EVENT2,
		.end = S5M8752_IRQB_MASK2,
		.flags = IORESOURCE_IO,
	},
};

static struct mfd_cell power_devs[] = {
	{
		.name = "s5m8752-power",
		.num_resources = 1,
		.resources = &power_supply_resources[0],
		.id = -1,
	},
};

static struct mfd_cell audio_devs[] = {
	{
		.name = "s5m8752-codec",
		.id = -1,
	}
};

struct s5m8752_irq_data {
	int reg;
	int mask_reg;
	int enable;
	int offs;
	int flags;
};

static struct s5m8752_irq_data s5m8752_irqs[] = {
#if 1
	[S5M8752_IRQ_PK1B] = {
		.reg		= S5M8752_IRQB_EVENT1,
		.mask_reg	= S5M8752_IRQB_MASK1,
		.offs		= 1 << 2,
	},
	[S5M8752_IRQ_PK2B] = {
		.reg		= S5M8752_IRQB_EVENT1,
		.mask_reg	= S5M8752_IRQB_MASK1,
		.offs		= 1 << 1,
	},
	[S5M8752_IRQ_PKY3] = {
		.reg		= S5M8752_IRQB_EVENT1,
		.mask_reg	= S5M8752_IRQB_MASK1,
		.offs		= 1 << 0,
	},
#endif
	[S5M8752_IRQ_VCHG_DET] = {
		.reg		= S5M8752_IRQB_EVENT2,
		.mask_reg	= S5M8752_IRQB_MASK2,
		.offs		= 1 << 3,
	},
	[S5M8752_IRQ_VCHG_REM] = {
		.reg		= S5M8752_IRQB_EVENT2,
		.mask_reg	= S5M8752_IRQB_MASK2,
		.offs		= 1 << 2,
	},
	[S5M8752_IRQ_CHG_TOUT] = {
		.reg		= S5M8752_IRQB_EVENT2,
		.mask_reg	= S5M8752_IRQB_MASK2,
		.offs		= 1 << 1,
	},
	[S5M8752_IRQ_CHG_EOC] = {
		.reg		= S5M8752_IRQB_EVENT2,
		.mask_reg	= S5M8752_IRQB_MASK2,
		.offs		= 1 << 0,
	},
};

static inline struct s5m8752_irq_data *irq_to_s5m8752(struct s5m8752 *s5m8752,
							int irq)
{
	return &s5m8752_irqs[irq - s5m8752->irq_base];
}

/******************************************************************************
 * This function is interrupt service routine of S5M8752.
 ******************************************************************************/
static irqreturn_t s5m8752_irq(int irq, void *data)
{
	struct s5m8752 *s5m8752 = data;
	struct s5m8752_irq_data *irq_data;

	int read_reg = -1;
	uint8_t value;
	int i;
	
	
	for (i = 0; i < ARRAY_SIZE(s5m8752_irqs); i++) {
		irq_data = &s5m8752_irqs[i];
		if (read_reg != irq_data->reg) {
			read_reg = irq_data->reg;
			s5m8752_reg_read(s5m8752, irq_data->reg, &value);
		}
		if (value & irq_data->enable)
			handle_nested_irq(s5m8752->irq_base + i);
	}

/* S5M8752 event must be clear after the interrupt occurs */
	s5m8752_reg_write(s5m8752, S5M8752_IRQB_EVENT1, 0x00);
	s5m8752_reg_write(s5m8752, S5M8752_IRQB_EVENT2, 0x00);

	return IRQ_HANDLED;
}

/******************************************************************************
 * This function can be lock each of S5M8752 irqs.
 ******************************************************************************/
static void s5m8752_irq_lock(unsigned int irq)
{
	struct s5m8752 *s5m8752 = get_irq_chip_data(irq);
	mutex_lock(&s5m8752->irq_lock);
}

/******************************************************************************
 * This function can be unlock each of S5M8752 irqs.
 ******************************************************************************/
static void s5m8752_irq_sync_unlock(unsigned int irq)
{
	struct s5m8752 *s5m8752 = get_irq_chip_data(irq);
	struct s5m8752_irq_data *irq_data;
	static uint8_t cache_pwr = 0xFF;
	static uint8_t cache_chg = 0xFF;
	static uint8_t irq_pwr, irq_chg;
	int i;

	irq_pwr = cache_pwr;
	irq_chg = cache_chg;
	for (i = 0; i < ARRAY_SIZE(s5m8752_irqs); i++) {
		irq_data = &s5m8752_irqs[i];
		switch (irq_data->mask_reg) {
		case S5M8752_IRQB_MASK1:
			irq_pwr &= ~irq_data->enable;
			break;
		case S5M8752_IRQB_MASK2:
			irq_chg &= ~irq_data->enable;
			break;
		default:
			dev_err(s5m8752->dev, "Unknown IRQ\n");
			break;
		}
	}

	if (cache_pwr != irq_pwr) {
		cache_pwr = irq_pwr;
		s5m8752_reg_write(s5m8752, S5M8752_IRQB_MASK1, irq_pwr);
	}

	if (cache_chg != irq_chg) {
		cache_chg = irq_chg;
		s5m8752_reg_write(s5m8752, S5M8752_IRQB_MASK2, irq_chg);
	}
	mutex_unlock(&s5m8752->irq_lock);
}

/******************************************************************************
 * This function can enable each of S5M8752 irqs.
 ******************************************************************************/
static void s5m8752_irq_enable(unsigned int irq)
{
	struct s5m8752 *s5m8752 = get_irq_chip_data(irq);
	s5m8752_irqs[irq - s5m8752->irq_base].enable =
		s5m8752_irqs[irq - s5m8752->irq_base].offs;
}

/******************************************************************************
 * This function can disable each of S5M8752 irqs.
 ******************************************************************************/
static void s5m8752_irq_disable(unsigned int irq)
{
	struct s5m8752 *s5m8752 = get_irq_chip_data(irq);
	s5m8752_irqs[irq - s5m8752->irq_base].enable = 0;
}

static struct irq_chip s5m8752_irq_chip = {
	.name		= "s5m8752",
	.bus_lock	= s5m8752_irq_lock,
	.bus_sync_unlock = s5m8752_irq_sync_unlock,
	.enable		= s5m8752_irq_enable,
	.disable	= s5m8752_irq_disable,
};

/******************************************************************************
 * This is S5M8752 irq initial function.
 ******************************************************************************/
static int s5m8752_irq_init(struct s5m8752 *s5m8752, int irq,
				struct s5m8752_pdata *pdata)
{
	unsigned long flags = IRQF_TRIGGER_FALLING | IRQF_ONESHOT;
	struct irq_desc *desc;
	int i, ret, __irq;

	if (!pdata || !pdata->irq_base) {
		dev_warn(s5m8752->dev, "No interrupt support on IRQ base\n");
		return -EINVAL;
	}

	/* Clear all the irqs */
	s5m8752_reg_write(s5m8752, S5M8752_IRQB_EVENT1, 0x00);
	s5m8752_reg_write(s5m8752, S5M8752_IRQB_EVENT2, 0x00);

	/* Mask all the irqs */
	s5m8752_reg_write(s5m8752, S5M8752_IRQB_MASK1, 0x00);
	s5m8752_reg_write(s5m8752, S5M8752_IRQB_MASK2, 0x00);

	mutex_init(&s5m8752->irq_lock);
	s5m8752->core_irq = irq;
	s5m8752->irq_base = pdata->irq_base;
	desc = irq_to_desc(s5m8752->core_irq);

	for (i = 0; i < ARRAY_SIZE(s5m8752_irqs); i++) {
		__irq = i + s5m8752->irq_base;
		set_irq_chip_data(__irq, s5m8752);
		set_irq_chip_and_handler(__irq, &s5m8752_irq_chip,
					 handle_edge_irq);
		set_irq_nested_thread(__irq, 1);
#ifdef CONFIG_ARM
		set_irq_flags(__irq, IRQF_VALID);
#else
		set_irq_noprobe(__irq);
#endif
	}
	if (!irq) {
		dev_warn(s5m8752->dev, "No interrupt support on core IRQ\n");
		return -EINVAL;
	}

	ret = request_threaded_irq(irq, NULL, s5m8752_irq, flags,
				"s5m8752", s5m8752);

	if (ret) {
		dev_err(s5m8752->dev, "Failed to request core IRQ: %d\n", ret);
		s5m8752->core_irq = 0;
	}

	return 0;
}

static void s5m8752_irq_exit(struct s5m8752 *s5m8752)
{
	if (s5m8752->core_irq)
		free_irq(s5m8752->core_irq, s5m8752);
}

/******************************************************************************
 * This function set PSDHOLD pin to '0'.
 * If PSHOLD is '0', S5M8752 turns off.
 ******************************************************************************/
static ssize_t s5m8752_store_pshold(struct device *dev,
	       struct device_attribute *attr, const char *buf, size_t count)
{
	int rc;
	int reg;
	unsigned long value;

	rc = strict_strtoul(buf, 0, &value);
	if (rc)
		return rc;

	rc = -ENXIO;

	if (!value) {
		reg = __raw_readl(S5P6450_PSHOLD_CON);
		reg &= ~(0x1 << 1);
		__raw_writel(reg, S5P6450_PSHOLD_CON);
		rc = count;
	} else
		rc = -EINVAL;

	return rc;
}

/******************************************************************************
 * This function reads the status of PSHOLD pin.
 ******************************************************************************/
static ssize_t s5m8752_show_pshold(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int reg, value;

	reg = __raw_readl(S5P6450_PSHOLD_CON);
	value = (reg & 0x2) >> 1;

	return sprintf(buf, "%d\n", value);
}

static DEVICE_ATTR(pshold, 0644, s5m8752_show_pshold, s5m8752_store_pshold);

/******************************************************************************
 * This function controls the SLEEPB pin to be enable or disable.
 ******************************************************************************/
static int s5m8752_sleepb_set(struct s5m8752 *s5m8752, int status)
{
	if (status)
		s5m8752_set_bits(s5m8752, S5M8752_ONOFF1,
					S5M8752_SLEEPB_PIN_ENABLE);
	else
		s5m8752_clear_bits(s5m8752, S5M8752_ONOFF1,
					S5M8752_SLEEPB_PIN_ENABLE);
	return 0;
}

/******************************************************************************
 * This function set S5M8752 dvs gpio pin for DVFS operation.
 ******************************************************************************/
static int s5m8752_dvs_set(struct s5m8752 *s5m8752, struct s5m8752_pdata *pdata)
{
	s5m8752_dvs2a = pdata->dvs->gpio_dvs2a;
	s5m8752_dvs2b = pdata->dvs->gpio_dvs2b;
	s5m8752_dvs3a = pdata->dvs->gpio_dvs3a;
	s5m8752_dvs3b = pdata->dvs->gpio_dvs3b;

	return 0;
}

/******************************************************************************
 * S5M8752 device driver is being loaded.
 ******************************************************************************/
int s5m8752_device_init(struct s5m8752 *s5m8752, int irq,
						struct s5m8752_pdata *pdata)
{
	int ret = 0;

	if (irq)
		ret = s5m8752_irq_init(s5m8752, irq, pdata);

	if (ret < 0)
		goto err;

	dev_set_drvdata(s5m8752->dev, s5m8752);
	s5m8752_sleepb_set(s5m8752, S5M8752_SLEEPB_ENABLE);

	if (pdata && pdata->gpio_pshold)
		ret = device_create_file(s5m8752->dev, &dev_attr_pshold);

	if (ret < 0) {
		dev_err(s5m8752->dev,
				"Failed to add s5m8752 sysfs: %d\n", ret);
		goto err;
	}

	/* Regulator driver is added.. */
	if (pdata && pdata->regulator) {
		ret = mfd_add_devices(s5m8752->dev, 0, regulator_devs,
					ARRAY_SIZE(regulator_devs), NULL,
					0);
		if (ret < 0)
			dev_err(s5m8752->dev,
				"Failed to add regulator driver: %d\n", ret);

		s5m8752_dvs_set(s5m8752, pdata);
	}

	/* Backlight driver is added.. */
	if (pdata && pdata->backlight) {
		ret = mfd_add_devices(s5m8752->dev, -1, backlight_devs,
					ARRAY_SIZE(backlight_devs), NULL,
					0);
		if (ret < 0)
			dev_err(s5m8752->dev,
				"Failed to add backlight driver: %d\n",	ret);
	}

	/* Battery charger driver is added.. */
	if (pdata && pdata->power) {
		ret = mfd_add_devices(s5m8752->dev, -1, power_devs,
					ARRAY_SIZE(power_devs), NULL,
					0);
		if (ret < 0)
			dev_err(s5m8752->dev,
				"Failed to add power driver: %d\n", ret);
	}

	/* Audio Codec driver is added.. */
	if (pdata && pdata->audio) {
		ret = mfd_add_devices(s5m8752->dev, -1, audio_devs,
					ARRAY_SIZE(audio_devs), NULL,
					0);
		if (ret < 0)
			dev_err(s5m8752->dev,
				"Failed to add audio driver: %d\n", ret);
	}

	return 0;

err:
	return ret;
}
EXPORT_SYMBOL_GPL(s5m8752_device_init);

/******************************************************************************
 * S5M8752 device driver is being unloaded.
 ******************************************************************************/
void s5m8752_device_exit(struct s5m8752 *s5m8752)
{
	dev_info(s5m8752->dev, "Unloaded S5M8752 device driver\n");

	mfd_remove_devices(s5m8752->dev);
	if (s5m8752->irq_base)
		s5m8752_irq_exit(s5m8752);

	kfree(s5m8752);
}
EXPORT_SYMBOL_GPL(s5m8752_device_exit);

/* Module Information */
MODULE_DESCRIPTION("S5M8752 Multi-function IC");
MODULE_AUTHOR("Jongbin, Won <jongbin.won@samsung.com>");
MODULE_LICENSE("GPL");
