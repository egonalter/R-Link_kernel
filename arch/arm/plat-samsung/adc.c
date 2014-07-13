/* arch/arm/plat-samsung/adc.c
 *
 * Copyright (c) 2008 Simtec Electronics
 *	http://armlinux.simtec.co.uk/
 *	Ben Dooks <ben@simtec.co.uk>, <ben-linux@fluff.org>
 *
 * Samsung ADC device core
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License.
*/

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/err.h>
#include <linux/clk.h>
#include <linux/interrupt.h>
#include <linux/io.h>

#include <plat/regs-adc.h>
#include <plat/adc.h>

/* This driver is designed to control the usage of the ADC block between
 * the touchscreen and any other drivers that may need to use it, such as
 * the hwmon driver.
 *
 * Priority will be given to the touchscreen driver, but as this itself is
 * rate limited it should not starve other requests which are processed in
 * order that they are received.
 *
 * Each user registers to get a client block which uniquely identifies it
 * and stores information such as the necessary functions to callback when
 * action is required.
 */

wait_queue_head_t wake;

enum s3c_cpu_type {
	TYPE_S3C24XX,
	TYPE_S3C64XX
};

struct s3c_adc_client {
	struct platform_device	*pdev;
	struct list_head	 pend;
	wait_queue_head_t	*wait;

	int			 nr_samples;
	int			 result;
	unsigned char		 is_ts;
	unsigned char		 channel;

	void	(*select_cb)(struct s3c_adc_client *c, unsigned selected);
	void	(*convert_cb)(struct s3c_adc_client *c,
			      unsigned val1, unsigned val2,
			      int samples_left);
};

struct adc_device {
	struct platform_device	*pdev;
	struct platform_device	*owner;
	struct clk		*clk;
	struct s3c_adc_client	*cur;
	struct s3c_adc_client	*ts_pend;
	void __iomem		*regs;
	unsigned int 		 data_mask;
	unsigned int		 prescale;

	int			 irq;
};

static struct adc_device *adc_dev;
static DEFINE_SPINLOCK(adc_lock);
static LIST_HEAD(adc_pending);
static int adc_suspended;

#define adc_dbg(_adc, msg...) dev_dbg(&(_adc)->pdev->dev, msg)

int s3c_adc_set_ts_control (struct s3c_adc_client *client, unsigned int port, unsigned int value)
{
	struct adc_device *adc = adc_dev;

	if (!client->is_ts)
		return -EINVAL;	

	if (port != S3C_ADCTSC && port != S3C_ADCCLRWK)
		return -EINVAL;

	writel (value, adc->regs + port);

	return 0;
}
EXPORT_SYMBOL_GPL(s3c_adc_set_ts_control);

unsigned int s3c_adc_get_ts_control (struct s3c_adc_client *client, unsigned int port)
{
	struct adc_device *adc = adc_dev;

	if (client == NULL)
		return -EINVAL;

	if (!client->is_ts)
		return -EINVAL;

	return (readl (adc->regs + port));
}
EXPORT_SYMBOL_GPL(s3c_adc_get_ts_control);

static inline void s3c_adc_convert(struct adc_device *adc, struct s3c_adc_client *client)
{
	unsigned con; 
	
	client->select_cb(client,SELECTED);
	
	con = readl(adc->regs + S3C_ADCCON);
	con |= S3C_ADCCON_ENABLE_START;
	writel(con, adc->regs + S3C_ADCCON);
}

static inline void s3c_adc_select(struct adc_device *adc,
				  struct s3c_adc_client *client)
{
	unsigned con = readl(adc->regs + S3C_ADCCON);
	
	con &= ~S3C_ADCCON_MUXMASK;
	con &= ~S3C_ADCCON_STDBM;
	con &= ~S3C_ADCCON_STARTMASK;

	if (!client->is_ts)
		con |= S3C_ADCCON_SELMUX(client->channel);

	writel(con, adc->regs + S3C_ADCCON);
	writel(client->channel, adc->regs + S3C_ADCMUX);
}

static void s3c_adc_dbgshow(struct adc_device *adc)
{
	adc_dbg(adc, "CON=%08x, TSC=%08x, DLY=%08x\n",
		readl(adc->regs + S3C_ADCCON),
		readl(adc->regs + S3C_ADCTSC),
		readl(adc->regs + S3C_ADCDLY));
}

static void s3c_adc_try(struct adc_device *adc)
{
	struct s3c_adc_client *next = adc->ts_pend;
	unsigned long flags;
	
	/* If suspend(ed/ing), don't let any client bug us,
	 * it just confuses the hell out the ADC... */
	if (unlikely(adc_suspended))
		return;
	
	spin_lock_irqsave(&adc_lock, flags);
	
	if (!next && !list_empty(&adc_pending)) {
		next = list_first_entry(&adc_pending,
					struct s3c_adc_client, pend);
		list_del(&next->pend);
	} else
		adc->ts_pend = NULL;

	spin_unlock_irqrestore(&adc_lock, flags);
	
	if (next) {
		adc_dbg(adc, "new client is %p\n", next);
		adc->cur = next;
		s3c_adc_select(adc, next);
		s3c_adc_convert(adc, next);
		s3c_adc_dbgshow(adc);
	}
}

int s3c_adc_start(struct s3c_adc_client *client,
		  unsigned int channel, int nr_samples)
{
	struct adc_device *adc = adc_dev;
	unsigned long flags;

	/* If suspend(ed/ing), don't let any client bug us,
	 * it just confuses the hell out the ADC... */
	if (unlikely(adc_suspended))
		return -EAGAIN;
	
	if (!adc) {
		printk(KERN_ERR "%s: failed to find adc\n", __func__);
		return -EINVAL;
	}

	if (client->is_ts && adc->ts_pend)
		return -EAGAIN;

	spin_lock_irqsave(&adc_lock, flags);

	client->channel = channel;
	client->nr_samples = nr_samples;

	if (client->is_ts)
		adc->ts_pend = client;

	else {
		/* Don't allow a client to be added twice,
		 * this is just a recipe for disaster... */
		struct list_head *lh;
		int listed = 0;

		list_for_each(lh, &adc_pending) {
			if (list_entry(lh, struct s3c_adc_client, pend) == client) {
				listed = 1;
				break;
			}
		}

		if (!listed)
			list_add_tail(&client->pend, &adc_pending);
	}

	spin_unlock_irqrestore(&adc_lock, flags);
	
	if (!adc->cur)
		s3c_adc_try(adc);
	
	return 0;
}
EXPORT_SYMBOL_GPL(s3c_adc_start);

static void s3c_convert_done(struct s3c_adc_client *client,
			     unsigned v, unsigned u, int left)
{
	client->result = v;
	wake_up(client->wait);
}

int s3c_adc_read(struct s3c_adc_client *client, unsigned int ch)
{
	int ret;

	client->convert_cb = s3c_convert_done;
	client->wait = &wake;
	client->result = -1;

	ret = s3c_adc_start(client, ch, 1);
	if (ret < 0)
		goto err;

	ret = wait_event_timeout(wake, client->result >= 0, HZ / 2);
	if (client->result < 0) {
		ret = -ETIMEDOUT;
		goto err;
	}

	client->convert_cb = NULL;
	return client->result;

err:
	return ret;
}
EXPORT_SYMBOL_GPL(s3c_adc_read);

static void s3c_adc_default_select(struct s3c_adc_client *client,
				   unsigned select)
{
}

struct s3c_adc_client *s3c_adc_register(struct platform_device *pdev,
					void (*select)(struct s3c_adc_client *client,
						       unsigned int selected),
					void (*conv)(struct s3c_adc_client *client,
						     unsigned d0, unsigned d1,
						     int samples_left),
					unsigned int is_ts)
{
	struct s3c_adc_client *client;

	WARN_ON(!pdev);

	if (!select)
		select = s3c_adc_default_select;

	if (!pdev)
		return ERR_PTR(-EINVAL);

	client = kzalloc(sizeof(struct s3c_adc_client), GFP_KERNEL);
	if (!client) {
		dev_err(&pdev->dev, "no memory for adc client\n");
		return ERR_PTR(-ENOMEM);
	}

	client->pdev = pdev;
	client->is_ts = is_ts;
	client->select_cb = select;
	client->convert_cb = conv;

	return client;
}
EXPORT_SYMBOL_GPL(s3c_adc_register);

void s3c_adc_release(struct s3c_adc_client *client)
{
	/* We should really check that nothing is in progress. */
	if (adc_dev->cur == client)
		adc_dev->cur = NULL;
	if (adc_dev->ts_pend == client)
		adc_dev->ts_pend = NULL;
	else {
		struct list_head *p, *n;
		struct s3c_adc_client *tmp;

		list_for_each_safe(p, n, &adc_pending) {
			tmp = list_entry(p, struct s3c_adc_client, pend);
			if (tmp == client)
				list_del(&tmp->pend);
		}
	}

	if (adc_dev->cur == NULL)
		s3c_adc_try(adc_dev);
	kfree(client);
}
EXPORT_SYMBOL_GPL(s3c_adc_release);

static irqreturn_t s3c_adc_irq(int irq, void *pw)
{
	struct adc_device *adc = pw;
	struct s3c_adc_client *client = adc->cur;
	unsigned long flags;
	unsigned data0, data1;

	if (!client) {
		return IRQ_HANDLED;
	}

	data0 = readl(adc->regs + S3C_ADCDAT0);
	data1 = readl(adc->regs + S3C_ADCDAT1);
	
	local_irq_save (flags);

	if (client->is_ts)
		(client->convert_cb)(client, data0, data1, client->nr_samples);
	else
		(client->convert_cb)(client, data0 & adc->data_mask, data1 & adc->data_mask,  client->nr_samples);

	__raw_writel(0x0, adc->regs+S3C_ADCCLRINT);
	if ( client->nr_samples <= 0) printk("WARNING ADC samples<=0\n");
	if (--client->nr_samples > 0) {
		/* fire another conversion for this */
		s3c_adc_convert(adc, client);
	} else {
		adc->cur = NULL;

		(client->select_cb)(client,FINISHED);
	
		s3c_adc_try(adc);
	}

	local_irq_restore(flags);

	return IRQ_HANDLED;
}

static int s3c_adc_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct s3c_adc_mach_info *cfg = dev->platform_data;
	struct adc_device *adc;
	struct resource *regs;
	int ret;
	unsigned tmp;

	adc = kzalloc(sizeof(struct adc_device), GFP_KERNEL);
	if (adc == NULL) {
		dev_err(dev, "failed to allocate adc_device\n");
		return -ENOMEM;
	}

	adc->pdev = pdev;
	adc->prescale = S3C_ADCCON_PRSCVL(cfg->presc);

	if(cfg->resolution == 12)
		adc->data_mask = 0xFFF;
	else
		adc->data_mask = 0x3FF;

	adc->irq = platform_get_irq(pdev, 0);
	if (adc->irq <= 0) {
		dev_err(dev, "failed to get adc irq\n");
		ret = -ENOENT;
		goto err_alloc;
	}

	ret = request_irq(adc->irq, s3c_adc_irq, 0, dev_name(dev), adc);

	if (ret < 0) {
		dev_err(dev, "failed to attach adc irq\n");
		goto err_alloc;
	}

	adc->clk = clk_get(dev, "adc");
	if (IS_ERR(adc->clk)) {
		dev_err(dev, "failed to get adc clock\n");
		ret = PTR_ERR(adc->clk);
		goto err_irq;
	}

	regs = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!regs) {
		dev_err(dev, "failed to find registers\n");
		ret = -ENXIO;
		goto err_clk;
	}

	adc->regs = ioremap(regs->start, resource_size(regs));
	if (!adc->regs) {
		dev_err(dev, "failed to map registers\n");
		ret = -ENXIO;
		goto err_clk;
	}

	clk_enable(adc->clk);

	tmp = adc->prescale | S3C_ADCCON_PRSCEN;

	/* Enable 12-bit ADC resolution */
	if(cfg->resolution == 12)
		tmp |= S3C64XX_ADCCON_RESSEL;

	writel(tmp, adc->regs + S3C_ADCCON);
	writel(cfg->delay, adc->regs + S3C_ADCDLY);
	dev_info(dev, "attached adc driver\n");

	platform_set_drvdata(pdev, adc);
	adc_dev = adc;

	init_waitqueue_head(&wake);
	
	return 0;

 err_clk:
	clk_put(adc->clk);

 err_irq:
	free_irq(adc->irq, adc);

 err_alloc:
	kfree(adc);
	return ret;
}

static int __devexit s3c_adc_remove(struct platform_device *pdev)
{
	struct adc_device *adc = platform_get_drvdata(pdev);

	iounmap(adc->regs);
	free_irq(adc->irq, adc);
	clk_disable(adc->clk);
	clk_put(adc->clk);
	kfree(adc);

	return 0;
}

#ifdef CONFIG_PM
static int s3c_adc_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct adc_device *adc = platform_get_drvdata(pdev);
	u32 con = 0;

	adc_suspended = 1;
	
	/* set ADC in standby mode */
	con = readl(adc->regs + S3C_ADCCON);
	con |= S3C_ADCCON_STDBM;
	writel(con, adc->regs + S3C_ADCCON);
	
	adc->cur = NULL;
	adc->ts_pend = NULL;

	disable_irq(adc->irq);
	clk_disable(adc->clk);

	return 0;
}

static int s3c_adc_resume(struct platform_device *pdev)
{
	struct adc_device *adc = platform_get_drvdata(pdev);
	struct device *dev = &pdev->dev;
	struct s3c_adc_mach_info *cfg = dev->platform_data;
	u32 con = 0;
	
	clk_enable(adc->clk);
	
	/* set ADC prescaler & resolution */
	con |= adc->prescale| S3C_ADCCON_PRSCEN | S3C_ADCCON_RESSEL_12BIT;
	/* disable stanby mode */
	con &= ~S3C_ADCCON_STDBM;
	writel(con, adc->regs + S3C_ADCCON);
	writel(cfg->delay, adc->regs + S3C_ADCDLY);

	enable_irq(adc->irq);
	adc_suspended = 0;
	
	return 0;
}

#else
#define s3c_adc_suspend NULL
#define s3c_adc_resume NULL
#endif


static struct platform_driver s3c_adc_driver = {
	.driver		= {
		.name	= "s3c-adc",
		.owner	= THIS_MODULE,
	},
	.probe		= s3c_adc_probe,
	.remove		= __devexit_p(s3c_adc_remove),
	.suspend	= s3c_adc_suspend,
	.resume		= s3c_adc_resume,
};

static int __init adc_init(void)
{
	int ret;

	ret = platform_driver_register(&s3c_adc_driver);
	if (ret)
		printk(KERN_ERR "%s: failed to add adc driver\n", __func__);

	return ret;
}

arch_initcall(adc_init);


