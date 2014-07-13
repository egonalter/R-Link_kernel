/*****************************************************************************
 * Copyright 2003 - 2010 Broadcom Corporation.  All rights reserved.
 *
 * Unless you and Broadcom execute a separate written software license
 * agreement governing use of this software, this software is licensed to you
 * under the terms of the GNU General Public License version 2, available at
 * http://www.broadcom.com/licenses/GPLv2.php (the "GPL").
 *
 * Notwithstanding the above, under no circumstances may you combine this
 * software in any way with any other Broadcom software provided under a
* license other than the GPL, without Broadcom's express prior written
 * consent.
 *****************************************************************************/

/** BCM59040 PMU Charger Driver
 *****************************************************************************
 *
 *     This implements the BCM59040 PMU Charger driver layer. This layer
 *     implements support for the BCM59040 chargers. It handles all the low-
 *     level handling of charger related interrupts from the BCM59040 and
 *     requests from the Linux power supply class driver for the PMU device
 *     framework. The objective of this driver is to provide a level of
 *     abstraction from the charger hardware mapping, capabilities and
 *     configuration. As such this layer should provide an API layer
 *     that the more general power supply driver interfaces to. This layer
 *     needs to provide functions that allow for:
 *
 *         -#.Determine number of chargers
 *             - Capabilities of each charger
 *             - Registration of charger events on a per charger basis
 *             - ???
 *         -#. Control operating state of charger
 *         -#. Obtain current status of charger
 *
 ****************************************************************************/

/* ---- Include Files ---------------------------------------------------- */

#include <linux/version.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/io.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/sysctl.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/poll.h>
#include <linux/interrupt.h>

#include <linux/pmu_device.h>

#include <asm/arch/hardware.h>
#include <asm/arch/bcm4760_reg.h>
#include <asm/arch/pmu_device_bcm59040.h>

#include <asm/plat-bcm/pmu_device_bcm.h>

#include <linux/broadcom/pmu_bcm59040.h>
#include <plat/usbmode.h>

/* Include external interfaces */

#include <asm/arch/bcm59040_irqs.h>

//#include <linux/pmu/bcm59040_core.h> // not sure if this is needed yet

/* Include internal interfaces */


/*
 * Structure and macro definitions.
 */
#define BCM59040_CHARGER_DRIVER             "bcm59040_charger"
#define BCM59040_CHARGER_MOD_DESCRIPTION	"BCM59040 Charger Driver"
#define BCM59040_CHARGER_MOD_VERSION		1.0

#undef BCM59040_DEVNAME
#define BCM59040_DEVNAME				BCM59040_CHARGER_DRIVER
#define PFX								BCM59040_DEVNAME ": "

/* Define this to disable wakeup from suspend on charder insertion/removal
 * When enabled it would only work if USB_STAT lines are connected */
#define	BCM59040_CHARGER_NO_USB_WAKEUP

/* Debug logging */
#ifdef DEBUG
#undef DEBUG
#endif

#define DEBUG 1

#define DBG_ERROR	0x01
#define DBG_INFO	0x02
#define DBG_TRACE	0x04
#define DBG_TRACE2	0x08
#define DBG_DATA	0x10
#define DBG_DATA2	0x20

#ifdef DEBUG
 //#define DBG_DEFAULT_LEVEL	(DBG_ERROR | DBG_INFO | DBG_TRACE)
 #define DBG_DEFAULT_LEVEL	(DBG_ERROR)

 static int gLevel = DBG_DEFAULT_LEVEL;
 #define PMU_DEBUG(level,x) {if (level & gLevel) printk x;}
#else
 #define PMU_DEBUG(level,x)
#endif

/*
 * Local function declarations.
 */
static int bcm59040_id_insert(struct pmu_client *pclient);

/* OTG related data structure */
typedef enum {
    OTG_BOOST_STAT,        // otg boost regulator status
    OTG_BLOCK_STAT         // otg block status
} OTG_Stat_t;

typedef enum
{
   BCM_Charging_Host_Port,
   BCM_Dedicated_Charger,
   BCM_Standard_Host_Port
} bcm59040_usb_charger_type_t;

/* OTG related data structure */
typedef enum {
    OTG_A_DEV,          // otg A device
    OTG_B_DEV_LEGACY,    // otg B legacy device or no cable
    OTG_RID_A,          // otg RID A (has ACA)
    OTG_RID_B,          // otg RID B (has ACA)
    OTG_RID_C,          // otg RID C (has ACA)
    OTG_B_DEV_FLOAT1,     // otg B device (has ACA)
	OTG_B_DEV_FLOAT3,     // otg B device (has ACA)
} BCM_PMU_OTG_Role_t;

/** struct bcm59040_charger_config_struct - Driver data for
 *  BCM59040 Charger
 * @dev:                Pointer to associated device.
 * @work:               Work generated from ISR.
 * @workqueue:          Work queue for foreground ISR processing
 *                      of Charger interrupts.
 */
typedef struct bcm59040_charger_config_struct
{
    int				hw_otg_ctrl;			// need to initialize this;
    int				otg_block_enabled;
    int				otg_boost_enabled;

    int				in_maintchrg_mode;
    int				isWallChargerPresent;
    int				isUSBChargerPresent;
    bcm59040_usb_charger_type_t	charger_type;

    struct pmu_client		*pclient;
    struct work_struct		work;
    struct workqueue_struct	*workqueue;
} bcm59040_charger_config_t;

/*
 * Local variables.
 */

static uint8_t bcm59040_charger_int_mask = 0x0;
static uint8_t bcm59040_int9_mask = 0x0;
static bcm59040_charger_config_t	*local_charger_config = NULL;

/* charging current mapping table */
static u32 bcm59040_charging_curr_to_mA_map[] =
{
   0,  /* 00000 */
   10, /* 00001 */
   20,
   30,
   40,
   50,
   60,
   70,
   80,
   90,
   100,
   110,
   120,
   130,
   140,
   150,
   0,  /*10000*/
   100,
   200,
   300,
   400,
   500,
   600,
   700,
   800,
   900,
   1000,
   1100,
   1200,
   1300,
   1400,
   1500, /*11111*/
};

const char * usb_chargers[]	= {"USB-NONE", "USB-CLA", "USB-IDLE", "USB-DEV"};
const char * usb_states[]	= {"INIT", "IDLE", "DEV-DET", "DEV-WAIT", "DEV", "HOST-DET", "HOST", "CLA"};

/*
 * Function definitions.
 */

/** bcm59040_charger_config - bcm59040_charger_config
 *
 * Initialize some charger related registers.
 *
 * @pclient:		Pointer to PMU Client structure.
 *
 * Returns 0 if successful otherwise standard Linux error codes.
 *
 */
static int bcm59040_charger_config(struct pmu_client *pclient)
{
	int				rc;
	uint8_t				data;
	bcm59040_charger_config_t	*charger_config;
	bcm59040_client_platform_data_t *pdata;
	struct bcm59040_charger_defaults *defaults;

	charger_config = pmu_get_drvdata(pclient);

//	charger_config->in_maintchrg_mode = 0;
	charger_config->isUSBChargerPresent = 0;
	charger_config->isWallChargerPresent = 0;

	if (pclient->dev.platform_data == NULL) {
		PMU_DEBUG(DBG_ERROR, (KERN_ERR "Error, platform data not initialized\n"));
		return -1;
	}

	pdata = (bcm59040_client_platform_data_t*)(pclient->dev.platform_data);
	if (pdata->defaults == NULL) {
		PMU_DEBUG(DBG_ERROR, (KERN_ERR "Error, default not initialized for charger\n"));
		return -1;
	}

	defaults = (struct bcm59040_charger_defaults *) pdata->defaults;
	
	/* configure MBCCTRLx registers for initializing some non-OTPable fields */
	data = defaults->mbcctrl1;
	if((rc = pmu_bus_seqwrite(pclient, BCM59040_REG_MBCCTRL1, &data, 1)) < 0) {
		printk (KERN_ERR PFX "error writing BCM59040_REG_MBCCTRL1 register.\n");
		return rc;
	}

	data = defaults->mbcctrl3 | BCM59040_MBCCTRL3_MAINGCHRG;
	if((rc = pmu_bus_seqwrite(pclient, BCM59040_REG_MBCCTRL3, &data, 1)) < 0) {
		printk (KERN_ERR PFX "error writing BCM59040_REG_MBCCTRL3 register.\n");
		return rc;
	}

	data = defaults->mbcctrl6;
	if((rc = pmu_bus_seqwrite(pclient, BCM59040_REG_MBCCTRL6, &data, 1)) < 0) {
		printk (KERN_ERR PFX "error writing BCM59040_REG_MBCCTRL6 register.\n");
		return rc;
	}

	data = defaults->mbcctrl7;
	if((rc = pmu_bus_seqwrite(pclient, BCM59040_REG_MBCCTRL7, &data, 1)) < 0) {
		printk(KERN_ERR PFX "error writing BCM59040_REG_MBCCTRL7 register.\n");
		return rc;
	}

	/* Ensure we don't modify the adapter priority behaviour, otherwise the PMU can cut power. 
	   Keep ideal diode in normal mode, currently system can (only) boot with >100mA*/
	/* keep the defaults  for now.. (they're set in bcm59040-pnd.c)
	if ((rc = pmu_bus_seqread(pclient, BCM59040_REG_MBCCTRL8, &data, 1)) < 0) {
		PMU_DEBUG(DBG_ERROR, (KERN_ERR PFX "error reading BCM59040_REG_MBCCTRL8 register.\n"));
		return rc;
	}
	data = (defaults->mbcctrl8 | 0xE8);
	if((rc = pmu_bus_seqwrite(pclient, BCM59040_REG_MBCCTRL8, &data, 1)) < 0) {
		printk(KERN_ERR PFX "error writing BCM59040_REG_MBCCTRL8 register.\n");
		return rc;
	}*/

	if((rc = pmu_bus_seqread(pclient, BCM59040_REG_MBCCTRL9, &data, 1)) < 0) {
		printk (KERN_ERR PFX "error reading BCM59040_REG_MBCCTRL9 register.\n");
		return rc;
	}

	data = (data & BCM59040_MBCCTRL9_CHGTYP_MASK) | defaults->mbcctrl9;
	if((rc = pmu_bus_seqwrite(pclient, BCM59040_REG_MBCCTRL9, &data, 1)) < 0) {
		printk (KERN_ERR PFX "error writing BCM59040_REG_MBCCTRL9 register.\n");
		return rc;
	}

	data = defaults->mbcctrl10;
	if((rc = pmu_bus_seqwrite(pclient, BCM59040_REG_MBCCTRL10, &data, 1)) < 0) {
		printk (KERN_ERR PFX "error writing BCM59040_REG_MBCCTRL10 register.\n");
		return rc;
	}

	if((rc = pmu_bus_seqread(pclient, BCM59040_REG_CMPCTRL3, &data, 1)) < 0) {
		printk (KERN_ERR PFX "error reading BCM59040_REG_CMPCTRL3 register.\n");
		return rc;
	}

	/* set Maintenance charge comparator hysteresis to 100mV, keep other OTP bits*/
	data &= ~BCM59040_CMPCTRL3_MBMC_HYS;
	if((rc = pmu_bus_seqwrite(pclient, BCM59040_REG_CMPCTRL3, &data, 1)) < 0) {
		printk (KERN_ERR PFX "error writing BCM59040_REG_CMPCTRL3 register.\n");
		return rc;
	}

	/* configure NTCCTRLx registers for initializing some non-OTPable fields */
	data = defaults->ntcctrl1;
	if((rc = pmu_bus_seqwrite(pclient, BCM59040_REG_NTCCTRL1, &data, 1)) < 0) {
		printk (KERN_ERR PFX "error writing BCM59040_REG_NTCCTRL1 register.\n");
		return rc;
	}

	return 0;
}

/** bcm59040_config_charger_fc_option - Control fast charge options.
 *
 * Change the FC option settings, and configure the FC option accordingly.
 * For SHP charger, we need to do so in order to go FC2 charge.
 * For non-SHP charger, even though we can charge to FC1, we
 * prefer to use FC2 as well.  Currently used only by
 * bcm59040_charger_start().
 *
 * @pclient:		Pointer to PMU Client structure.
 *
 * Returns 0 if successful otherwise standard Linux error codes.
 *
 */
static int bcm59040_config_charger_fc_option(struct pmu_client *pclient)
{
	int			rc;
	uint8_t		data;

	if((rc = pmu_bus_seqread(pclient, BCM59040_REG_MBCCTRL3, &data, 1)) < 0) {
		PMU_DEBUG(DBG_ERROR, (KERN_ERR PFX "error reading BCM59040_REG_MBCCTRL3 register.\n"));
		return rc;
   }

	if(!(data & BCM59040_MBCCTRL3_VUBGR_FC2_EN)) {
		data = ((unsigned char)data | BCM59040_MBCCTRL3_VUBGR_FC2_EN);
		pmu_bus_seqwrite(pclient, BCM59040_REG_MBCCTRL3, &data, 1);
	}

	return 0;
}

/** bcm59040_charger_start - Start charging process.
 *
 * @pclient:		Pointer to PMU Client structure.
 * @chargerID:		Selects USB or wall charger.
 *
 * Returns 0 if successful otherwise standard Linux error codes.
 *
 */
static int bcm59040_charger_start(struct pmu_client *pclient, int chargerID)
{
	int	rc;
	uint8_t	data;

	PMU_DEBUG(DBG_INFO, ("Charger type: %d\n", chargerID));
	if ((chargerID != BCM59040_CHARGER_MAIN) &&
		(chargerID != BCM59040_CHARGER_USB)) {
		PMU_DEBUG(DBG_ERROR, (KERN_ERR PFX "Bad chargerID %d\n", chargerID));
		return -EINVAL;
	}

	/* read ENV for charger presence */
	if((rc = pmu_bus_seqread(pclient, BCM59040_REG_ENV1, &data, 1)) < 0) {
			PMU_DEBUG(DBG_ERROR, (KERN_ERR PFX "error reading BCM59040_REG_ENV1 register.\n"));
			return rc;
	}

	if((chargerID == BCM59040_CHARGER_MAIN) &&
	   (data & BCM59040_ENV1_CGPD)) {
		/* set MBCHOSTEN bit and maintenance enable bit */
		if((rc = pmu_bus_seqread(pclient, BCM59040_REG_MBCCTRL3, &data, 1)) < 0) {
			PMU_DEBUG(DBG_ERROR, (KERN_ERR PFX "error reading BCM59040_REG_MBCCTRL3 register.\n"));
			return rc;
		}

		data |= BCM59040_MBCCTRL3_MBCHOSTEN;
		if((rc = pmu_bus_seqwrite(pclient, BCM59040_REG_MBCCTRL3, &data, 1)) < 0) {
			PMU_DEBUG(DBG_ERROR, (KERN_ERR PFX "error writing BCM59040_REG_MBCCTRL3 register.\n"));
			return rc;
		}

		/* set VSRHOSTEN bit */
		if((rc = pmu_bus_seqread(pclient, BCM59040_REG_MBCCTRL10, &data, 1)) < 0) {
			PMU_DEBUG(DBG_ERROR, (KERN_ERR PFX "error reading BCM59040_REG_MBCCTRL10 register.\n"));
			return rc;
		}

		data |= BCM59040_MBCCTRL10_VSRHOSTEN ;
		if((rc = pmu_bus_seqwrite(pclient, BCM59040_REG_MBCCTRL10, &data, 1)) < 0) {
			PMU_DEBUG(DBG_ERROR, (KERN_ERR PFX "error writing BCM59040_REG_MBCCTRL10 register.\n"));
			return rc;
		}

		/* Not setting the charging current here. USBMode deals with this. */
	} else if ((chargerID == BCM59040_CHARGER_USB) && (data & BCM59040_ENV1_UBPD)) {

		/* set MBCHOSTEN bit and maintenance enable bit */
		if((rc = pmu_bus_seqread(pclient, BCM59040_REG_MBCCTRL3, &data, 1)) < 0) {
			PMU_DEBUG(DBG_ERROR, (KERN_ERR PFX "error reading BCM59040_REG_MBCCTRL3 register.\n"));
			return rc;
		}

		data |= BCM59040_MBCCTRL3_MBCHOSTEN;
		if((rc = pmu_bus_seqwrite(pclient, BCM59040_REG_MBCCTRL3, &data, 1)) < 0) {
			PMU_DEBUG(DBG_ERROR, (KERN_ERR PFX "error writing BCM59040_REG_MBCCTRL3 register.\n"));
			return rc;
		}

		/* need to change FC options in mbcctrl3 regardless of USB charger is SHP or not */
		bcm59040_config_charger_fc_option(pclient);

		/* Not setting the charging current here. USBMode deals with this. */
	}

	return rc;
}

static const uint16_t	charge_current[]=
{
	0,	10,	20,	30,	40,	50,	60,	70,
	80,	90,	100,	110,	120,	130,	140,	150,
	0,	100,	200,	300,	400,	500,	600,	700,
	800,	900,	1000,	1100,	1200,	1300,	1400,	1500
};
 
static u8 bcm59040_get_max_charging_current( u32 max_current )
{
	u8	index;
	u8	retval=0;

	for( index=0; index < (sizeof( charge_current )/sizeof( charge_current[0] )); index++ )
	{
		if( (charge_current[index] < max_current) && (charge_current[index] > charge_current[retval]) )
			retval=index;
	}
	return retval;
}


/** bcm59040_set_charging_mode - Set charging mode.
 *
 * This function returns 0 if OK else error code.
 * It's called from bcm59040_usbmode.c whenever 
 * the mode changes.
 *
 * 
 * @charger:	Current charger connecterd ie CLA or USB.
 */
int bcm59040_set_charging_mode(struct pmu_client *pclient, bcm59040_usb_charger_t charger)
{
	int ret = 0;

	unsigned char	charger_mode		= 0;
	unsigned char	charging_current	= 0;
	unsigned char	usb_i_limits		= 0;
	u32		max_charge_current	= 0;
	bcm59040_client_platform_data_t		*pdata=NULL;
	struct bcm59040_charger_defaults	*defaults=NULL;

	pdata=(bcm59040_client_platform_data_t *) pclient->dev.platform_data;
	if( pdata != NULL )
		defaults=(struct bcm59040_charger_defaults *) pdata->defaults;

	if( (pdata == NULL) || (defaults == NULL) )
	{
		printk( KERN_ERR "Charger platform_data not found !\n" );
		return -ENODEV;
	}

	if( local_charger_config == NULL )
	{
		printk( KERN_ERR "Warning! Charger not active, yet code still called!\n" );
		return -ENODEV;
	}

	/* Determine what the maximum is we can charge at. */
	max_charge_current=bcm59040_get_max_charging_current( defaults->battery_capacity/2 );

	switch(charger)
	{
		case BCM59040_USB_CLA:
		  charger_mode	=	BCM59040_MBCCTRL3_MBCHOSTEN
				|	BCM59040_MBCCTRL3_VUBGR_FC2_EN
				|	BCM59040_MBCCTRL3_MAINGCHRG
				|	BCM59040_MBCCTRL3_USB0_FC_OPTION;

		  charging_current	=	(BCM59040_MBCCTRL6_FC_CC_1000MA < max_charge_current ? BCM59040_MBCCTRL6_FC_CC_1000MA : max_charge_current);

		  usb_i_limits	=	BCM59040_MBCCTRL9_USB_ILIMIT_900MA
				|	BCM59040_MBCCTRL9_WALL_ILIMIT_900MA
				|	BCM59040_MBCCTRL9_USB_VSR_EN
				|	BCM59040_MBCCTRL9_CHGTYP_SHP;

		  break;
		case BCM59040_USB_IDLE:
		case BCM59040_USB_NONE:
		case BCM59040_USB_DEV:
		  charger_mode	=	BCM59040_MBCCTRL3_MBCHOSTEN
				|	BCM59040_MBCCTRL3_VUBGR_FC2_EN
				|	BCM59040_MBCCTRL3_MAINGCHRG
				|	BCM59040_MBCCTRL3_USB0_FC_OPTION;

		  charging_current	=	(BCM59040_MBCCTRL6_FC_CC_500MA < max_charge_current ? BCM59040_MBCCTRL6_FC_CC_500MA : max_charge_current);

		  usb_i_limits =	BCM59040_MBCCTRL9_USB_ILIMIT_500MA
				|	BCM59040_MBCCTRL9_WALL_ILIMIT_500MA
				|	BCM59040_MBCCTRL9_USB_VSR_EN
				|	BCM59040_MBCCTRL9_CHGTYP_SHP;

		  break;

		default:
		  pr_err("bcm59040_set_charging_mode "
					"[%d: UNKNOWN CHARGER_TYPE]", charger);
		  break;
	}

	if(local_charger_config->pclient == NULL){
		printk(KERN_ERR "ERROR no pclient!\n");
		return -ENODEV;
	}	

	ret=pmu_bus_seqwrite(local_charger_config->pclient,BCM59040_REG_MBCCTRL3,&charger_mode, 1);
	if (ret < 0) {
		printk("bcm59040_set_charging_mode: "
				"error writing BCM59040_REG_MBCCTRL3 register.\n");
		return ret;
	}

	ret=pmu_bus_seqwrite(local_charger_config->pclient,BCM59040_REG_MBCCTRL6,&charging_current, 1);
	if (ret < 0) {
		printk("bcm59040_set_charging_mode: "
				"error writing BCM59040_REG_MBCCTRL6 register.\n");
		return ret;
	}

	ret=pmu_bus_seqwrite(local_charger_config->pclient,BCM59040_REG_MBCCTRL9,&usb_i_limits, 1);
	if (ret < 0) {
		printk("bcm59040_set_charging_mode: "
				"error writing BCM59040_REG_MBCCTRL9 register.\n");
		return ret;
	}

	return ret; /*if the write succeeds ret = n bytes written*/
}

/** bcm59040_get_charging_current - Return charging current.
 *
 * This function returns the current value of the charging current.
 *
 * @pclient:		Pointer to PMU Client structure.
 *
 * Returns charging current in mA.
 *
 */
static int bcm59040_get_charging_current(struct pmu_client *pclient)
{
	int								rc;
	u32								curr_index;
	uint8_t							data;
	bcm59040_charger_config_t	    *charger_config;

	pclient = bcm_get_pclient_from_client_type(pclient, BCMPMU_CHARGER);

	charger_config = pmu_get_drvdata(pclient);

	if((rc = pmu_bus_seqread(pclient, BCM59040_REG_MBCCTRL6, &data, 1)) < 0) {
		PMU_DEBUG(DBG_ERROR, (KERN_ERR PFX "error reading BCM59040_REG_MBCCTRL6 register.\n"));
		return rc;
	}

   curr_index = BCM59040_MBCCTRL6_FC_CC_MASK & data;

   if(curr_index <= BCM59040_MBCCTRL6_FC_CC_1500MA) {
       return bcm59040_charging_curr_to_mA_map[curr_index];
   } else {
       PMU_DEBUG(DBG_ERROR, (KERN_ERR PFX "bcm59040_get_charging_current: error: current exceeds legal range\n"));
       return -EINVAL;
   }
} /* bcm59040_get_charging_current */
EXPORT_SYMBOL_GPL(bcm59040_get_charging_current);

/** bcm59040_otg_config - Configure OTG based on pin state.
 *
 * Notes: If hardware control is selected, PMU automatically cause
 *         GPIO1 pin be selected as OTGSHD pin, and GPIO2 pin as vbus_pulse pin
 *         Both GPIO 1 & 2 are input pins regardless of GPIO_DIR values
 *
 * @pclient:		Pointer to PMU Client structure.
 *
 * Returns 0 if successful otherwise standard Linux error codes.
 *
 */
static int bcm59040_otg_config( struct pmu_client *pclient )
{
	int							rc;
	uint8_t						data;
	bcm59040_charger_config_t	*charger_config;

	charger_config = pmu_get_drvdata(pclient);
	if(!charger_config) {
		return -ENOMEM;
	}

	/* configure otg related registers */
	if((rc = pmu_bus_seqread(pclient, BCM59040_REG_OTGCTRL1, &data, 1)) < 0) {
		PMU_DEBUG(DBG_ERROR, (KERN_ERR PFX "error reading BCM59040_REG_OTGCTRL1 register.\n"));
		return rc;
	}
	data = ((data & 0x83) | BCM59040_PMU_REG_OTGCTRL1_VAL); //For ACA support, configure bit7:2, keep bit1:0 and 8, 

	if((rc = pmu_bus_seqwrite(pclient, BCM59040_REG_OTGCTRL1,
			&data, 1)) < 0) {
		PMU_DEBUG(DBG_ERROR, (KERN_ERR PFX "error writing BCM59040_REG_OTGCTRL1 register.\n"));
		return rc;
	}

	data = BCM59040_PMU_REG_OTGCTRL2_VAL & 0xEF; //SW control OTG

	/* Disable VA_SESS_EN. USB_STAT driver will take care of this */
	data&=0xBF;
	if((rc = pmu_bus_seqwrite(pclient, BCM59040_REG_OTGCTRL2,
			&data, 1)) < 0) {
		PMU_DEBUG(DBG_ERROR, (KERN_ERR PFX "error writing BCM59040_REG_OTGCTRL2 register.\n"));
		return rc;
	}
	
	if(BCM59040_PMU_REG_OTGCTRL2_VAL & BCM59040_OTGCTRL2_HWVSSW) {
		charger_config->hw_otg_ctrl = 1;
	} else {
		charger_config->hw_otg_ctrl = 0;
	}

	charger_config->otg_block_enabled = 1; // init to true. TODO: make sure Host's GPIO1 pin is set to hi on board
	charger_config->otg_boost_enabled = 0; // TODO: once Host mode is supported, if ID = ground, this should be 1

	return 0;
}

/** bcm59040_update_otg_stat - Allow host to update OTG status.
 *
 * For host to set OTG status when using hardware based OTG control
 *
 * @pclient:		Pointer to PMU Client structure.
 * @OTG_Stat:		New OTG status.
 * @enable:			Enable boost regulator.
 *
 * Returns 0 if successful otherwise standard Linux error codes.
 *
 */
static int bcm59040_update_otg_stat(struct pmu_client *pclient, OTG_Stat_t OTG_Stat, int enable)
{
	bcm59040_charger_config_t	*charger_config;

	charger_config = pmu_get_drvdata(pclient);
	if(!charger_config) {
		return -ENOMEM;
	}

	switch (OTG_Stat) {
		case OTG_BOOST_STAT:
			if (enable)
				charger_config->otg_boost_enabled = 1;
			else
				charger_config->otg_boost_enabled = 0;
			PMU_DEBUG(DBG_INFO, (KERN_INFO PFX "bcm59040_update_otg_stat: boost regulator = %d\n", enable ));
			break;

		case OTG_BLOCK_STAT:
			if (enable)
				charger_config->otg_block_enabled = 1;
			else
				charger_config->otg_block_enabled = 0;
			PMU_DEBUG(DBG_INFO, (KERN_INFO PFX "bcm59040_update_otg_stat: otg block = %d\n", enable ));
			break;
	}

	return 0;
}

/** bcm59040_id_insert - USB insertion detected.
 *
 * ISR for ID insertion detection, occurring when ID_OUT changes from hi to lo
 *
 * @pclient:		Pointer to PMU Client structure.
 *
 * Returns 0 if successful otherwise standard Linux error codes.
 *
 */
static int bcm59040_id_insert(struct pmu_client *pclient)
{
	int	rc;
	uint8_t	data;
	bcm59040_charger_config_t	*charger_config;

	PMU_DEBUG(DBG_INFO, (KERN_INFO PFX "bcm59040_id_insert\n"));

	charger_config = pmu_get_drvdata(pclient);
	if(!charger_config) {
		return -ENOMEM;
	}

	if((rc = pmu_bus_seqread(pclient, BCM59040_REG_ENV4, &data, 1)) < 0) {
		PMU_DEBUG(DBG_ERROR, (KERN_ERR PFX "error reading BCM59040_REG_ENV4 register.\n"));
		return rc;
	}

	/* For 59035 C0 and later, USB charger automatically disabled if vbus is turned on */
	bcm59040_update_otg_stat(pclient,
							 OTG_BOOST_STAT,
							 (data & BCM59040_ENV4_OFFVBUSb)? 1:0 );
	bcm59040_update_otg_stat(pclient,
							 OTG_BLOCK_STAT,
							 (data & BCM59040_ENV4_OTGSHDNb)? 1:0 );

	return rc;
}

/** bcm59040_charger_work - Work function for BCM59040 Charger
 *  IRQ
 * @work:   Pointer to work structure.
 *
 */

static void bcm59040_charger_work(struct work_struct *work)
{
	int				rc;
	uint8_t				data;
	bcm59040_charger_config_t	*charger_config;
	struct pmu_client		*pclient;

	PMU_DEBUG(DBG_INFO, (KERN_INFO PFX "Entering bcm59040_charger_work:\n"));

	charger_config = container_of(work, bcm59040_charger_config_t, work);
	pclient = charger_config->pclient;

	if((rc = pmu_bus_seqread(pclient, BCM59040_REG_INT2, &data, 1 )) < 0) {
		PMU_DEBUG(DBG_ERROR, (KERN_ERR PFX "error reading BCM59040_REG_VINT2 register.\n"));
	}
	else {

		PMU_DEBUG(DBG_TRACE, (KERN_INFO PFX "Read %d bytes (%02X) from virtual int 2 status register.\n", rc, data));

		if(data & ~bcm59040_charger_int_mask & 0x01) {
			// wall charger inserted
			PMU_DEBUG(DBG_INFO,(KERN_INFO PFX "Main charger inserted\n"));

			// bcm59040_charger_start(pclient, BCM59040_CHARGER_MAIN);
			charger_config->isWallChargerPresent = 1;
		}

		if(data & ~bcm59040_charger_int_mask & 0x02) {
			// wall removal
			PMU_DEBUG(DBG_INFO,(KERN_INFO PFX "Main charger removed\n"));

			charger_config->isWallChargerPresent = 0;
		}

		if(data & ~bcm59040_charger_int_mask & 0x10) {
			// USB insert
			charger_config->isUSBChargerPresent = 1;
			PMU_DEBUG(DBG_INFO,(KERN_INFO PFX "USB charger inserted\n"));
		}

		if(data & ~bcm59040_charger_int_mask & 0x20) {
			// USB removal
			PMU_DEBUG(DBG_INFO,(KERN_INFO PFX "USB charger removed\n"));

			charger_config->isUSBChargerPresent = 0;
		}
	}

	if((rc = pmu_bus_seqread(pclient, BCM59040_REG_INT9, &data, 1)) < 0) {
		PMU_DEBUG(DBG_ERROR, (KERN_ERR PFX "error reading BCM59040_REG_VINT9 register.\n"));
	}
	else {

		PMU_DEBUG(DBG_TRACE, (KERN_INFO PFX "Read %d bytes (%02X) from virtual int 9 status register.\n", rc, data));

		if((data & ~bcm59040_int9_mask) & 0x80) {
			// ID change
			bcm59040_id_insert(pclient);

			//Inform upper layer the current OTG role, if the role is RID_A or RID_GND, the OTG device is A device, otherwise, B device
		}
	}

	/* clear the interrupt */
	data = 1 << BCM59040_VINT1_CHARGER_BIT;
	pmu_bus_seqwrite(pclient, BCM59040_REG_VINT1, &data, 1 );
}

/** bcm59040_charger_isr - Interrupt service routine for BCM59040
 *  Charger IRQ vector number
 * @dev_id: Data structure associated with this interrupt event.
 *
 * This ISR handles events for the Charger in the BCM59040. The
 * primary event we're interested in is the
 */
static irqreturn_t bcm59040_charger_isr(int irq, void *dev_id)
{
	bcm59040_charger_config_t	*charger_config = dev_id;

	queue_work(charger_config->workqueue, &charger_config->work);

	return IRQ_HANDLED;
}

static void bcm59040_usbmode_state_listener( USB_STATE previous_usb_state, USB_STATE current_usb_state, void *arg )
{
	bcm59040_usb_charger_t	charger;
	struct pmu_client	*pclient=(struct pmu_client *) arg;
	int			ret;

	switch(current_usb_state)
	{
		case USB_STATE_DEVICE_DETECT:
			charger = BCM59040_USB_DEV;
			break;

		case USB_STATE_DEVICE_WAIT:
			charger = BCM59040_USB_DEV;
			break;

		case USB_STATE_DEVICE:
			charger = BCM59040_USB_DEV;
			break;

		case USB_STATE_IDLE:
			charger = BCM59040_USB_IDLE;
			break;

		case USB_STATE_CLA:
			charger = BCM59040_USB_CLA;
			break;

		case USB_STATE_HOST:
			charger = BCM59040_USB_CLA;
			break;

		default:
		{
			/* Not supported. Put in charger DEV for security. */
			printk(KERN_WARNING PFX "-> UNSUPPORTED STATE [%d]\n", 
					current_usb_state);
			charger = BCM59040_USB_DEV;
			break;
		}
	}

	ret = bcm59040_set_charging_mode(pclient, charger);

	if (ret > 0) {
		printk(KERN_INFO PFX "%s => %s\n", 
				usb_states[current_usb_state],
				usb_chargers[charger]);
	} else {
		printk(KERN_ERR PFX "%s -> %s FAILURE [return code:%d]\n", 
				usb_states[current_usb_state],
				usb_chargers[charger],
				ret);
	}
}

/** bcm59040_charger_probe - BCM59040 charger probe
 *
 * Probe to determine if BCM59040 is really present.
 *
 * @pclient:		Pointer to PMU Client structure.
 * @id:				id table.
 *
 * Returns 0 if successful otherwise standard Linux error codes.
 *
 */

static int bcm59040_charger_probe(struct pmu_client *pclient, const struct pmu_device_id *id)
{
	int				rc=0;
	uint8_t				data;
	bcm59040_charger_config_t	*charger_config;
	USB_STATE			current_state;

	if((charger_config = kzalloc(sizeof(*charger_config), GFP_KERNEL)) == NULL) {
		PMU_DEBUG(DBG_ERROR, (KERN_ERR PFX "Out of memory allocating charger_data\n"));
		return -ENOMEM;
	}

	charger_config->workqueue = create_workqueue(BCM59040_CHARGER_DRIVER);
	charger_config->pclient = pclient;

	INIT_WORK(&charger_config->work, &bcm59040_charger_work);

	pmu_set_drvdata(pclient, charger_config);
	local_charger_config = charger_config;

	bcm59040_charger_config(pclient);
	bcm59040_otg_config(pclient);

	rc = request_irq(BCM59040_IRQ_CHARGER,
					  bcm59040_charger_isr,
					  0,
					  "bcm59040_charger",
					  charger_config);
	if(rc < 0) {
		PMU_DEBUG(DBG_ERROR, (KERN_ERR PFX "bcm59040_charger_probe failed to attach charger interrupt, rc = %d\n", rc) );
		local_charger_config = NULL;
		kfree(charger_config);
		return rc;
	}

	if((rc = pmu_bus_seqread(pclient, BCM59040_REG_ENV1, &data, 1)) < 0) {
		PMU_DEBUG(DBG_ERROR, (KERN_ERR PFX "error reading BCM59040_REG_ENV1 register.\n"));
		goto out_err;
	}

	if(data & (1<<1)) {
		PMU_DEBUG(DBG_TRACE, (KERN_INFO PFX "wall charger detected\n"));
		charger_config->isWallChargerPresent = 1;
		bcm59040_charger_start(pclient, BCM59040_CHARGER_MAIN);
	}
	if(data & (1<<2)) {
		PMU_DEBUG(DBG_TRACE, (KERN_INFO PFX "USB charger detected\n"));
		charger_config->isUSBChargerPresent = 1;
		bcm59040_charger_start(pclient, BCM59040_CHARGER_USB);
	}

	/* Register the handler. */
	printk(PFX "Registering BCM59040 USBMode State Listener\n" );

	bcm59040_set_charging_mode(pclient, BCM59040_USB_DEV);
	if( add_usb_state_change_listener( bcm59040_usbmode_state_listener, pclient, &current_state ) != 0 )
	{
		PMU_DEBUG(DBG_ERROR, (KERN_ERR PFX "WARNING! USB State change listener for BCM59040 failed registration!\n" ));
		rc = -ENODEV;
		goto out_err;
	}

	/* Make sure we're in the right state. */
	bcm59040_usbmode_state_listener( USB_STATE_INITIAL, current_state, pclient );
	return 0;


out_err:
	free_irq(BCM59040_IRQ_CHARGER, charger_config);
	kfree(charger_config);
	local_charger_config = NULL;
	pmu_set_drvdata(pclient, NULL);
	return rc;
}

/** @brief BCM59040 charger remove
 *
 * Need to remove BCM59040 charger driver.
 *
 * @client:	Pointer to client structure for this device.
 *
 * Returns 0 if successful otherwise standard Linux error codes.
 *
 */

static int bcm59040_charger_remove(struct pmu_client *pclient)
{
	bcm59040_charger_config_t			*charger_config;

	charger_config = pmu_get_drvdata(pclient);
	if(charger_config)
	{
		free_irq(BCM59040_IRQ_CHARGER, charger_config);
		local_charger_config = NULL;
		kfree(charger_config);
	}

	if( remove_usb_state_change_listener( bcm59040_usbmode_state_listener, pclient ) != 0 )
		printk(PFX "WARNING! USB State change listener for BCM59040 failed unregistration!\n" );
	return 0;
}

#ifdef CONFIG_PM

/** bcm59040_charger_suspend - Suspend charger functioning.
 *
 * Charger suspend handler that is called when sysem is entering
 * a suspend state.
 *
 * @pclient:	Pointer to client structure for this device.
 * @state:		PM state being entered.
 *
 * Returns 0 if successful otherwise standard Linux error codes.
 *
 * @todo	Need to determine what suspend does for this driver still.
 *
 */
static int bcm59040_charger_suspend(struct pmu_client *pclient, pm_message_t state)
{
	PMU_DEBUG(DBG_TRACE, (KERN_INFO PFX "Suspend processed for %s.\n",
						  pclient->name));

#ifdef BCM59040_CHARGER_NO_USB_WAKEUP
	disable_irq(BCM59040_IRQ_CHARGER);
#endif

	return 0;
} /* bcm59040_charger_suspend */

/** bcm59040_charger_resume - Resume charger functioning.
 *
 * Charger resume handler that is called when sysem is exiting
 * a suspend state.
 *
 * @client:	Pointer to client structure for this device.
 *
 * Returns 0 if successful otherwise standard Linux error codes.
 *
 * @todo	Need to determine what suspend does for this driver still.
 *
 */
static int bcm59040_charger_resume(struct pmu_client *pclient)
{
	PMU_DEBUG(DBG_TRACE, (KERN_INFO PFX "Suspend processed for %s.\n",
						  pclient->name ));

#ifdef BCM59040_CHARGER_NO_USB_WAKEUP
	enable_irq(BCM59040_IRQ_CHARGER);
#endif

	return 0;
} /* bcm59040_charger_resume */

#endif

struct pmu_device_id bcm59040_charger_idtable[] = {
	{ "charger" },
	{ }
};

/** @brief BCM59040 charger driver registration data
 *
 */

struct pmu_driver bcm59040_charger_driver =
{
	.driver	= {
		.name	= BCM59040_CHARGER_DRIVER,
		.owner	= THIS_MODULE,
	},
	.probe		= &bcm59040_charger_probe,
	.remove		= &bcm59040_charger_remove,
#ifdef CONFIG_PM
	.suspend	= &bcm59040_charger_suspend,
	.resume		= &bcm59040_charger_resume,
#endif
	.id_table	= bcm59040_charger_idtable,
};

/** @brief BCM59040 charger module Initialization
 *
 * Registers the BCM59040 charger driver with the PMU device
 * framework.
 *
 * @return 0 if successful otherwise standard Linux error codes.
 *
 */

static int __init bcm59040_charger_init(void)
{
	int ret;

	ret = pmu_register_driver(&bcm59040_charger_driver);
	if (ret < 0)
	{
		printk (KERN_ERR PFX "Couldn't register PMU charger\n");
		return ret;
	}
	return 0;
}

/** @brief BCM59040 charger module Exit
 *
 * Deregisters the BCM59040 charger driver with the PMU device
 * framework.
 *
 * @return 0 if successful otherwise standard Linux error codes.
 *
 */

static void __exit bcm59040_charger_exit(void)
{
	pmu_unregister_driver( &bcm59040_charger_driver );
	return;
}


device_initcall(bcm59040_charger_init);
module_exit(bcm59040_charger_exit);

MODULE_DESCRIPTION(BCM59040_CHARGER_MOD_DESCRIPTION);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Broadcom Corporation");
MODULE_VERSION(BCM59040_CHARGER_MOD_VERSION);
