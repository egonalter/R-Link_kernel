#include <linux/version.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/proc_fs.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/pmu_device.h>
#include <linux/broadcom/pmu_bcm59040.h>
#include <asm/arch/pmu_device_bcm59040.h>
#include <asm/arch/bcm4760_reg.h>
#include <asm/io.h>
#include <linux/broadcom/bcm59040_usbstat.h>

#include <asm/arch/hardware.h>
#include <plat/tt_vbusmon.h>

typedef enum {
	USBSTAT_CHARGER_INIT	= 0,
	USBSTAT_CHARGER_CHGDET_RECEIVED,
	USBSTAT_CHARGER_CHGDET_GONE
} usbstat_hack_stat_t;

typedef struct bcm59040_usbstat_drvdata {
	struct pmu_client	*pclient;
	struct work_struct	work;
	struct workqueue_struct	*workqueue;
	usbstat_hack_stat_t	stat;
} bcm59040_usbstat_drvdata_t;

static int host_allowed_enabled = 0;

typedef struct vbus_drvdata {
	int			(*vbusdetect_handler)(struct device *, int);
	atomic_t		configured;
	struct pmu_client	*pclient;
} vbus_drvdata_t;

static vbus_drvdata_t bcm59040_vbus_drvdata;
static int config_vbusdetect(int (*hndlr)(struct device *, int));
static void cleanup_vbusdetect(void);
static int poll_vbusdetect(void);
static void vbus_suspend(void);
static void vbus_resume(void);

static struct vbus_pdata pdata = 
{
  .name = "pmu-usbstat",
	.config_vbusdetect = config_vbusdetect,
	.cleanup_vbusdetect = cleanup_vbusdetect,
	.poll_vbusdetect = poll_vbusdetect,
	.do_suspend = vbus_suspend,
	.do_resume = vbus_resume,
};

static struct resource vbus_resources[] =
{
	{
		.name		=	"base-address",
		.start		=	IO_ADDRESS( USB_R_GOTGCTL_MEMADDR ),
		.end		=	IO_ADDRESS( USB_R_DTXFSTS12_MEMADDR ) + 4,
		.flags		=	IORESOURCE_MEM,
		.parent		=	NULL,
		.child		=	NULL,
		.sibling	=	NULL,
	},
};

struct platform_device irvine_device_usbstat_vbus = 
{
	.name			= "tomtom-vbus",
	.id			= 0,
	.num_resources		= ARRAY_SIZE( vbus_resources ),
	.resource		= vbus_resources,
	.dev.platform_data	= &pdata,
};

static void update_vbus(int vbus)
{
	vbus_drvdata_t			*vb_drv_data = (vbus_drvdata_t *) dev_get_drvdata(&irvine_device_usbstat_vbus.dev);

	if (vb_drv_data->vbusdetect_handler) {
		vb_drv_data->vbusdetect_handler(&irvine_device_usbstat_vbus.dev, vbus);
	}
}

static int bcm59040_usbstat_update_otg_mode( bcm59040_usbstat_drvdata_t *drvdata )
{
	struct pmu_client				*pclient = drvdata->pclient;
	struct bcm59040_client_platform_data_struct	*bus_pdata = (struct bcm59040_client_platform_data_struct *) pclient->dev.platform_data;
	struct bcm59040_usbstat_pdata			*pdata = (struct bcm59040_usbstat_pdata *) bus_pdata->extra;
	uint32_t					mode;
	uint32_t					hprt;
	uint32_t					state;
	uint8_t						env4;
	unsigned long					flags;

	/* Check the state of VBUS. */
	if( pmu_bus_seqread( pclient, BCM59040_REG_ENV4, &env4, 1 ) < 0 )
		return -ENXIO;

	/* make access atomic; there's no spinlock or such that protects the CMU registers */
	/* difficult to properly lock against dwc_otg driver; on single-CPU this is good enough */
	local_irq_save(flags);

	mode = readl( pdata->bcm4760_usb_base + USB_R_GUSBCFG_SEL ) & ~(USB_F_FORCEDEVMODE_MASK | USB_F_FORCEHSTMODE_MASK);
	hprt = readl( pdata->bcm4760_usb_base + USB_R_HPRT_SEL ) & ~USB_F_PRTPWR_MASK;
	state = readl( pdata->bcm4760_cmu_base + CMU_R_MODE_CTL_SEL ) & ~CMU_F_CMU_USB_SW_CTRL_MASK;

	/* We're always overriding. Set state to default: No VBUS, no mode. */
	state |= CMU_F_CMU_SW_VBC_EN_MASK;

	/* Check the state of the more interesting bits. */
	if( env4 & BCM59040_ENV4_VBUS_VALID )
	{
		/* VBUS is valid. Set the bit. */
		state |= CMU_F_CMU_SW_VBUSVALID_MASK;

		/* Check the state of the ID pin. */
		switch( env4 & BCM59040_ENV4_ID_IN_MASK )
		{
			case BCM59040_ENV4_A_DEV :
			case BCM59040_ENV4_RID_A :
			{
				/* Host mode. */
				if (host_allowed_enabled) {
					dev_info(&pclient->dev, "Enabling host mode\n");
					state |= CMU_F_CMU_SW_AVALID_MASK|CMU_F_CMU_SW_CHRGVBUS_MASK|CMU_F_CMU_SW_DRVVBUS_MASK;
					mode |= USB_F_FORCEHSTMODE_MASK;
					hprt |= USB_F_PRTPWR_MASK;
				}
				else {
					dev_info(&pclient->dev, "Host mode not allowed\n");
					state &= ~CMU_F_CMU_SW_VBUSVALID_MASK;
				}
				break;
			}

			case BCM59040_ENV4_B_DEV_LEGACY :
			case BCM59040_ENV4_RID_B :
			case BCM59040_ENV4_RID_C :
			case BCM59040_ENV4_B_DEV_FLOAT :
			{
				/* Ignore switch to device mode if the charger detect interrupt has not arrived. */
				/* The order is important according to Broadcom.				 */
				if (drvdata->stat != USBSTAT_CHARGER_CHGDET_GONE) {
					dev_info(&pclient->dev, "Enabling device mode\n");
					/* Device mode. */
					state |= CMU_F_CMU_SW_BVALID_MASK;
					mode |= USB_F_FORCEDEVMODE_MASK;
				} else {
					/* Return without updating the registers */
					/* We should be called again when the CHGDET interrupt is ready */
					return 0;
				}
				break;
			}

			default :
			{
				/* Unsupported. */
				dev_warn(&pclient->dev, "Unsupported ID 0x%08x.\n", env4 & BCM59040_ENV4_ID_IN_MASK);
				state &= ~CMU_F_CMU_SW_VBUSVALID_MASK;
				break;
			}
		}
	}
	else
	{
		/* Create disconnect event. */
		dev_info(&pclient->dev, "Disconnecting...\n");
		state |= CMU_F_CMU_SW_SESSEND_MASK;
	}

	/* Write back the correct state. Ensure the CMU registers are unlocked when we do this. */
	writel( 0xBCBC4760, pdata->bcm4760_cmu_base + CMU_R_TRIGGER_UNLOCK_SEL );
	writel( state, pdata->bcm4760_cmu_base + CMU_R_MODE_CTL_SEL );
	writel( 0, pdata->bcm4760_cmu_base + CMU_R_TRIGGER_UNLOCK_SEL );
	writel( mode, pdata->bcm4760_usb_base + USB_R_GUSBCFG_SEL );
	writel( hprt, pdata->bcm4760_usb_base + USB_R_HPRT_SEL );

	local_irq_restore(flags);

	/* update_vbus outside the irqs disabled section */
	update_vbus((env4 & BCM59040_ENV4_VBUS_VALID) ? 1 : 0);

	return 0;
}

static void bcm59040_usbstat_work(struct work_struct *work)
{
	bcm59040_usbstat_drvdata_t	*drvdata = container_of(work, bcm59040_usbstat_drvdata_t, work);

	bcm59040_usbstat_update_otg_mode(drvdata);
}

static int bcm59040_usbstat_disconnect( struct pmu_client *pclient )
{
	struct bcm59040_client_platform_data_struct	*bus_pdata = (struct bcm59040_client_platform_data_struct *) pclient->dev.platform_data;
	struct bcm59040_usbstat_pdata			*pdata = (struct bcm59040_usbstat_pdata *) bus_pdata->extra;
	uint32_t					state;
	unsigned long					flags;

	dev_info(&pclient->dev, "Forcing disconnect\n");

	/* make it atomic */
	local_irq_save(flags);

	state=readl( pdata->bcm4760_cmu_base + CMU_R_MODE_CTL_SEL ) & ~CMU_F_CMU_USB_SW_CTRL_MASK;
	state |= CMU_F_CMU_SW_VBC_EN_MASK;

	// Create disconnect event.
	state |= CMU_F_CMU_SW_SESSEND_MASK;

	/* Write back the correct state. Ensure the CMU registers are unlocked when we do this. */
	writel( 0xBCBC4760, pdata->bcm4760_cmu_base + CMU_R_TRIGGER_UNLOCK_SEL );
	writel( state, pdata->bcm4760_cmu_base + CMU_R_MODE_CTL_SEL );
	writel( 0, pdata->bcm4760_cmu_base + CMU_R_TRIGGER_UNLOCK_SEL );

	local_irq_restore(flags);

	return 0;
}

static irqreturn_t bcm59040_usbstat_irqhandler( int irq, void *dev_id )
{

	bcm59040_usbstat_drvdata_t	*drvdata = (bcm59040_usbstat_drvdata_t*) dev_id;
	struct pmu_client		*pclient = drvdata->pclient;
	uint8_t				int2, env1;

#if 0
	WARN_ON(!drvdata);
	WARN_ON(!drvdata->workqueue);
	queue_work(drvdata->workqueue, &drvdata->work);
#else
	pmu_bus_seqread( pclient, BCM59040_REG_INT2, &int2, 1 );
	pmu_bus_seqread( pclient, BCM59040_REG_ENV1, &env1, 1 );
	if ((int2 & BCM59040_INT2_CHGDET) || (env1 & BCM59040_ENV1_UBPD))
		drvdata->stat	= USBSTAT_CHARGER_CHGDET_RECEIVED;
	else
		drvdata->stat	= USBSTAT_CHARGER_CHGDET_GONE;

	/* clear the interrupt */
	int2 = 1 << BCM59040_VINT2_USB_BIT;
	pmu_bus_seqwrite(pclient, BCM59040_REG_VINT2, &int2, 1 );

	bcm59040_usbstat_update_otg_mode(drvdata);
#endif

	return IRQ_HANDLED;
}

static ssize_t usbstat_sysfs_read_host_allowed(struct device *dev, struct device_attribute *attr,
		                                       char *buf)
{
	snprintf (buf, PAGE_SIZE, "%d\n", host_allowed_enabled );

	return strlen(buf)+1;
}

static ssize_t usbstat_sysfs_write_host_allowed(struct device *dev, struct device_attribute *attr,
		                                        const char *buf, size_t n)
{
	ssize_t ret = n;
	int updated = 0;

	if (!strncmp(buf, "0\n", PAGE_SIZE)) {
		if (host_allowed_enabled == 1) {
			dev_info(dev, "host_allowed: 1 -> 0!\n" );
			updated = 1;
			host_allowed_enabled = 0;
		}
	} else if (!strncmp(buf, "1\n", PAGE_SIZE)) {
		if (host_allowed_enabled == 0) {
			dev_info(dev, "host_allowed: 0 -> 1!\n" );
			updated = 1;
			host_allowed_enabled = 1;
		}
	} else {
		ret = -EINVAL;
	}

	if (updated)
	{
		uint8_t env4;
		int	is_host = 0;

		if( pmu_bus_seqread( pmu_verify_client(dev), BCM59040_REG_ENV4, &env4, 1 ) < 0 ) {
			/* don't return an error from this; */
			/* safe value, although it may do a disconnect that is not actually needed */
			is_host = 1;
		}
		else if( env4 & BCM59040_ENV4_VBUS_VALID ) {
			/* Check the state of the ID pin. */
			switch( env4 & BCM59040_ENV4_ID_IN_MASK )
			{
				case BCM59040_ENV4_A_DEV :
				case BCM59040_ENV4_RID_A :
					is_host = 1;
					break;
			}
		}

		dev_info(dev, "Update USB mode!\n" );
		if (is_host) {
			bcm59040_usbstat_disconnect( pmu_verify_client(dev) );
		}
		bcm59040_usbstat_update_otg_mode( (bcm59040_usbstat_drvdata_t *) pmu_get_drvdata( pmu_verify_client(dev) ) );
	}

	return ret;
}

static DEVICE_ATTR(host_allowed, S_IRUGO | S_IWUSR, usbstat_sysfs_read_host_allowed, usbstat_sysfs_write_host_allowed);

static int bcm59040_usbstat_probe(struct pmu_client *pclient, const struct pmu_device_id *id)
{
	struct bcm59040_client_platform_data_struct	*bus_pdata=(struct bcm59040_client_platform_data_struct *) pclient->dev.platform_data;
	struct bcm59040_usbstat_pdata			*pdata=(struct bcm59040_usbstat_pdata *) bus_pdata->extra;	
	uint8_t						otgctrl[2];
	const int					otgctrl2_idin_delay[]={0, 20000, 50000, 100000};
	const int					otgctrl2_vbusdb_delay[]={0, 300, 1000, 2000};
	int						delay_usec=0;
	bcm59040_usbstat_drvdata_t			*drvdata;
	vbus_drvdata_t					*vb_drv_data = &bcm59040_vbus_drvdata;

	/* Enable the PMU OTG block. */
	if( pmu_bus_seqread( pclient, BCM59040_REG_OTGCTRL1, otgctrl, 2 ) < 0 )
	{
		dev_err(&pclient->dev, "Error reading OTGCTRL registers for USBSTAT!\n" );
		return -ENODEV;
	}

	otgctrl[0]|=0x80;
	otgctrl[1]|=0x40;

	if( pmu_bus_seqwrite( pclient, BCM59040_REG_OTGCTRL1, otgctrl, 2 ) < 0 )
	{
		dev_err(&pclient->dev, "Error setting OTGCTRL registers for USBSTAT!\n" );
		return -ENODEV;
	}

	/* Set the correct state. As the conversion to get the exact mode can take a little while, delay here. */ 
	/* The OTGCTRL2 register specifies how much this delay is. We want to make sure we don't transition */
	/* to an intermediary weird state. */
	delay_usec=otgctrl2_idin_delay[(otgctrl[1] & 0x0C) >> 2];
	delay_usec+=otgctrl2_vbusdb_delay[(otgctrl[1] & 0x03) >> 0];
	if( delay_usec != 0 )
		udelay( delay_usec );

	if((drvdata = kzalloc(sizeof(*drvdata), GFP_KERNEL)) == NULL) {
		dev_err(&pclient->dev, "Out of memory allocating bcm59040 usb driver data\n");
		return -ENOMEM;
	}

	drvdata->workqueue	= create_singlethread_workqueue("bcm59040_usbstat");
	drvdata->pclient	= pclient;
	drvdata->stat		= USBSTAT_CHARGER_INIT;
	INIT_WORK(&drvdata->work, &bcm59040_usbstat_work);

	pmu_set_drvdata(pclient, drvdata);

	/* Vbusmon device driver registration. Moved here in a desesperate attempt at dodging concurrency issues */
	vb_drv_data->pclient	= pclient;
	atomic_set(&vb_drv_data->configured, 0);
	dev_set_drvdata(&irvine_device_usbstat_vbus.dev, vb_drv_data);
	platform_device_register(&irvine_device_usbstat_vbus);

	/* Now that we delayed, switch to the right state. */
	if( bcm59040_usbstat_update_otg_mode( drvdata ) < 0 )
	{
		dev_err(&pclient->dev, "Error setting OTG Mode from probe!\n" );
		pmu_set_drvdata(pclient, NULL);
		destroy_workqueue(drvdata->workqueue);
		kfree(drvdata);
		return -ENODEV;
	}
	
	/* Register an IRQ handler on the specified IRQ. */
	if( request_irq( pdata->bcm59040_usb_irq, bcm59040_usbstat_irqhandler, 0, "bcm59040_usbstat", drvdata ) < 0 )
	{
		dev_err(&pclient->dev, "Can't register BCM59040 USB IRQ!\n" );
		pmu_set_drvdata(pclient, NULL);
		destroy_workqueue(drvdata->workqueue);
		kfree(drvdata);
		return -ENODEV;
	}
	
	if( device_create_file(&pclient->dev, &dev_attr_host_allowed))
	{
		dev_err(&pclient->dev, " unable to create sysfs file\n");
		return -ENODEV;
	}

	// I don't like to do this here, but it is very practical...
	vbusmon_set_device(&irvine_device_usbstat_vbus.dev);

	return 0;
}

static int bcm59040_usbstat_remove(struct pmu_client *pclient)
{
	struct bcm59040_client_platform_data_struct	*bus_pdata=(struct bcm59040_client_platform_data_struct *) pclient->dev.platform_data;
	struct bcm59040_usbstat_pdata			*pdata=(struct bcm59040_usbstat_pdata *) bus_pdata->extra;	

	free_irq( pdata->bcm59040_usb_irq, pclient );
	device_remove_file(&pclient->dev, &dev_attr_host_allowed);
	return 0;
}

struct pmu_device_id bcm59040_usbstat_idtable[] = {
	{ "usbstat" },
	{ }
};

#ifdef CONFIG_PM
static uint8_t	bcm59040_otgctrl[2]={0,0};

static int bcm59040_usbstat_suspend( struct pmu_client *pclient, pm_message_t state )
{
	uint8_t	otgctrl_mod[2];	

	/* The USBStat lines cause 1.6v to appear on 3V2_Main during suspend. Therefore disable the OTG block so this doesn't happen. */
	if( pmu_bus_seqread( pclient, BCM59040_REG_OTGCTRL1, bcm59040_otgctrl, sizeof( otgctrl_mod ) ) < 0 )
		dev_err(&pclient->dev, "WARNING! Could not save PMU OTGCTRL1/2 registers!\n" );

	/* Ensure the OTG block and the VaSessvld comparator are off. */
	memcpy( otgctrl_mod, bcm59040_otgctrl, sizeof( otgctrl_mod ) );
	otgctrl_mod[1]&=0xBF;
	otgctrl_mod[0]&=0x7F;

	if( pmu_bus_seqwrite( pclient, BCM59040_REG_OTGCTRL1, otgctrl_mod, sizeof( otgctrl_mod ) ) < 0 )
		dev_err(&pclient->dev, "WARNING! Could not turn off OTG Block/VaSessvld comparator!\n" );

	/* Disable host_allowed. Let userspace tell when host mode is allowed again */
	host_allowed_enabled = 0;

	return 0;
}

static int bcm59040_usbstat_resume( struct pmu_client *pclient )
{
	bcm59040_usbstat_drvdata_t	*drvdata = (bcm59040_usbstat_drvdata_t*) pmu_get_drvdata(pclient);

	/* Restore the OTGBlock/VaSessvld comparator state. */
	if( pmu_bus_seqwrite( pclient, BCM59040_REG_OTGCTRL1, bcm59040_otgctrl, sizeof( bcm59040_otgctrl ) ) < 0 )
		dev_err(&pclient->dev, "WARNING! Could restore OTG Block/VaSessvld comparator!\n" );

	/* Ensure all IRQs are cleared. */
	return bcm59040_usbstat_update_otg_mode( drvdata );
}

#else
#define bcm59040_usbstat_suspend	NULL
#define bcm59040_usbstat_resume		NULL
#endif

static void vbus_suspend(void)
{
	pr_info("vbus: suspending\n");
}

static void vbus_resume(void)
{
	pr_info("vbus: resuming\n");
}

/**
 * Configure the vbus-detect pin and register it as an interrupt
 **/
static int config_vbusdetect(int (*hndlr)(struct device *, int))
{
	vbus_drvdata_t		*drvdata;

	drvdata	= (vbus_drvdata_t *) dev_get_drvdata(&irvine_device_usbstat_vbus.dev);

	if (atomic_read(&drvdata->configured)) {
		printk(KERN_ERR "%s: vbus-detect is already configured\n",__FILE__);
		return 1;
	}

	drvdata->vbusdetect_handler	= hndlr;

	/* We are configured now, unset this if we fail later */
	atomic_set(&drvdata->configured, 1);

	return 0;
}

/**
 * Unregister the vbus pin's interrupt
 **/
static void cleanup_vbusdetect(void)
{
	vbus_drvdata_t	*drvdata;

	drvdata	= (vbus_drvdata_t *) dev_get_drvdata(&irvine_device_usbstat_vbus.dev);

	if (!atomic_read(&drvdata->configured)) {
		printk(KERN_ERR "%s: vbus-detect is not configured\n",__FILE__);
		return;
	}

  drvdata->vbusdetect_handler = NULL;
	atomic_set(&drvdata->configured, 0);
}

/**
 * Poll the vbus pin and return its status
 * Return codes:
 *  0             - vbus is not detected
 *  anything else - vbus is detected
 **/
static int poll_vbusdetect(void)
{
 vbus_drvdata_t		*drvdata;
 struct pmu_client	*pclient;
 int			err;
 uint8_t		env4;

	drvdata	= (vbus_drvdata_t *) dev_get_drvdata(&irvine_device_usbstat_vbus.dev);
	pclient	= drvdata->pclient;

	err	= pmu_bus_seqread(pclient, BCM59040_REG_ENV4, &env4, 1);
	if (err < 0)
	  return err;
	else
	  return (env4 & BCM59040_ENV4_VBUS_VALID) ? 1 : 0;
}

struct pmu_driver bcm59040_usbstat_driver =
{
	.driver =
	{
		.name	= "bcm59040_usbstat",
	},
	.probe		= bcm59040_usbstat_probe,
	.remove		= bcm59040_usbstat_remove,
	.suspend	= bcm59040_usbstat_suspend,
	.resume		= bcm59040_usbstat_resume,
	.id_table	= bcm59040_usbstat_idtable,
};

static int __init bcm59040_usbstat_init(void)
{
	int ret = pmu_register_driver( &bcm59040_usbstat_driver );
	if (ret)
	  return ret;

	return 0;
}

static void __exit bcm59040_usbstat_exit(void)
{
	pmu_unregister_driver( &bcm59040_usbstat_driver );
}

device_initcall(bcm59040_usbstat_init);
module_exit(bcm59040_usbstat_exit);

MODULE_DESCRIPTION( DRIVER_DESC_LONG );
MODULE_AUTHOR( "Rogier Stam (rogier.stam@tomtom.com)" );
MODULE_LICENSE("GPL");
