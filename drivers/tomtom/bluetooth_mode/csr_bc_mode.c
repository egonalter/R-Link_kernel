/*
 * CSR BlueCore mode driver. 
 * This driver takes care of specifying 
 *
 * Author: Mark Vels <mark.vels@tomtom.com>
 * Copyright (c) 2008 TomTom International B.V.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
 * NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#include <linux/pm.h>
#include <linux/kernel.h>
#include <linux/ctype.h>

#include <linux/rtc.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/platform_device.h>
#include <plat/csr_bc_mode.h>
#include <linux/rfkill.h> 

#include <asm/irq.h>
#include <asm/gpio.h>
//#include <asm/arch/map.h>
#include <linux/apm-emulation.h>

#define PBPWB_DEBUG
#ifdef PBPWB_DEBUG
#define PBP_DEBUG(fmt, args...) printk( "CSR_BC: " fmt, ##args )
#define PBP_ERR(fmt, args...) \
	printk(KERN_ERR "CSR_BC: ##ERROR## :" fmt, ##args )
#else
#define PBP_DEBUG(fmt, args...)
#define PBP_ERR(fmt, args...)
#endif

enum {
	BTMODE_UART_BCSP = 0,
	BTMODE_BCSP,
	BTMODE_USB_16MHZ,
	BTMODE_USB_26MHZ,
	BTMODE_3WIRE_UART,
	BTMODE_H4DS,
	BTMODE_UART_H4,
	BTMODE_UNDEFINED
};

static int csrbc_rfkill_set_power(void *data, bool blocked)
{
	struct csr_bc_info *info = (struct csr_bc_info*) data;
	
	if (blocked) {
		/* Power off the Bluetooth chip */
		info->reset(1);
	}
	else {
		/* Power on the Bluetooth chip */
		info->reset(0);
	}
	return 0;
}

static const struct rfkill_ops csrbc_rfkill_ops = {
	.set_block = csrbc_rfkill_set_power,
};

static void do_reset(struct csr_bc_info *info)
{
	BUG_ON(info == NULL);

	mdelay(10);
	info->reset(1);
	mdelay(10); /* wait at least 5ms according to BC4ROM datasheet */
	info->reset(0);
}


/**
 * Set the mode to the CSR BC Bluetooth chip. 
 * 
 * The mode argument can have the following values:
 *
 * 	PIO[0]	PIO[1]	pio[4]		Hex	Description
 * -----------------------------------------------------------------------------
 *	 0	  0	  0		  0	UART,BCSP
 *	 0	  0	  1		  1	BCSP ( 2 stop bits, no parity )
 *	 0	  1	  0		  2	USB @ 16 MHz
 *	 0	  1	  1		  3	USB @ 26MHz
 *	 1	  0	  0		  4	Three-wire UART
 *	 1	  0	  1		  5	H4DS
 *	 1	  1	  0		  6	UART(H4)
 *	 1	  1	  1		  7	UNDEFINED
 * -----------------------------------------------------------------------------
*/

static int  csrbc_mode_set_mode( struct device *dev, int mode )
{
	int realmode;
	struct csr_bc_info *info = (struct csr_bc_info*) dev_get_drvdata(dev);
	BUG_ON(info == NULL);

	if(mode < BTMODE_UART_BCSP || mode >=BTMODE_UNDEFINED )
		return -EINVAL;

	/* Approach: We first look if we can actually realise the requested mode, based
	 * on which pins are actually connected to gpio and which might be hardwired.
	 * Then, if the mode is found to be valid, we will write the pins that are appropriate.
	*/

	/* Step 1: feasible? */
	realmode = 0;
	if(info->get_pio0){
		realmode |= ((info->get_pio0()) << 2);
	}
	if(info->get_pio1){
		realmode |= ((info->get_pio1()) << 1);
	}
	if(info->get_pio4){
		realmode |= ((info->get_pio4()) << 0);
	}

	/* Step 2: Do the check */
	if(realmode != mode ){
		PBP_ERR("The current hardware platform cannot support this mode, not all needed pins are connected!");
		return -EINVAL;
	}

	/* Step 3: Now write the values to the actual connected gpio pins */
	if(info->set_pio0){
		info->set_pio0((mode&(1<<2))?1:0);
	}
	if(info->set_pio1){
		info->set_pio1((mode&(1<<1))?1:0);
	}
	if(info->set_pio4){
		info->set_pio4((mode&(1<<0))?1:0);
	}

	/* issue BT reset */
	info->do_reset(info);

	info->csrbc_mode = mode;
	return 0;
}



static ssize_t csrbc_mode_mode_show ( struct device *dev, struct device_attribute *attr, char *buffer )
{
	struct csr_bc_info *info = (struct csr_bc_info*) dev_get_drvdata(dev);
        BUG_ON(info == NULL);

	snprintf( buffer, PAGE_SIZE, "%i", info->csrbc_mode );
        return strlen( buffer );
}

static ssize_t csrbc_mode_mode_store(struct device *dev, struct device_attribute *attr, const char *buffer, size_t size )
{
	int mode;
	char t;
	int ret;

	/* 
	 * Ensure the data written is valid. We expect any argument to be in the range of
	 *  0x00 to 0xff BUT WITHOUT THE '0x'prepended. There may be a trailing newline character.
	 */
	if((size > 2) || (size == 2 && (buffer[1] !='\n')) )
		return -EINVAL;
	t = toupper(buffer[0]);
	if(t>='0' && t<='9') 
		mode = t-'0';
	else if (t>='A' && t<='F')
		mode = t-'A' + 10;
	else
		return -EINVAL;
	ret = csrbc_mode_set_mode( dev, mode);
        return (ret? ret: size);
}

static ssize_t csrbc_mode_reset_store( struct device *dev, struct device_attribute *attr, const char *buffer, size_t size )
{
	struct csr_bc_info *info = (struct csr_bc_info*) dev_get_drvdata(dev);

	info->do_reset(info);
	return size;
}

static struct device_attribute csrbc_mode_sysfs_attributes[]=
{
	{{"mode", THIS_MODULE, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP}, csrbc_mode_mode_show, csrbc_mode_mode_store},
	{{"reset", THIS_MODULE, S_IWUSR | S_IWGRP}, NULL, csrbc_mode_reset_store},
};
#define SIZE_CSRBC_SYSFS_ATTR	(sizeof(csrbc_mode_sysfs_attributes)/sizeof( csrbc_mode_sysfs_attributes[0]))

static int __init csrbc_mode_probe(struct platform_device *pdev )
{
	int ret = 0;
	struct csr_bc_info* private;
	struct csr_bc_info* pdata;
	int index;
	
	BUG_ON(pdev == NULL);
	BUG_ON(pdev->dev.platform_data == NULL);

	pdata = (struct csr_bc_info*) pdev->dev.platform_data;

	PBP_DEBUG( "%s: Probing CSR BlueCore mode driver.\n", pdev->name );

	private = (struct csr_bc_info*) kmalloc( sizeof(struct csr_bc_info), GFP_KERNEL );
	if(!private){
		PBP_ERR("Unable to obtain memory for internal info structures!" );
		return -ENOMEM;
	}

	/* Initialize structure */
	memcpy(private, pdata, sizeof(struct csr_bc_info) );

	/* Fill in default reset function if not provided */
	if (!private->do_reset)
		private->do_reset = do_reset;

	if(private->request_gpio()) {
		ret = -EBUSY;
		goto gpio_request_failed;
	}

	private->config_gpio();
	/* Do not reset the bluetooth chip here, user space is reseting the chip */ 
/*	private->do_reset(private);*/

	private->csrbc_mode = BTMODE_UNDEFINED;

	/* Register each 'file' in sysfs seperately. */
	for( index=0; index < SIZE_CSRBC_SYSFS_ATTR; index++ ){
			ret = device_create_file( &pdev->dev, &csrbc_mode_sysfs_attributes[index]);
	if(ret){
		for( index--; index >=0; index-- )
			device_remove_file( &pdev->dev, &csrbc_mode_sysfs_attributes[index] );
		goto sysfs_register_failed;
		}
	}

	/* Register as rfkill device */
	private->rfk_data = rfkill_alloc("CSR BlueCore", &pdev->dev,
					RFKILL_TYPE_BLUETOOTH, &csrbc_rfkill_ops, (void *)private);

	if (unlikely(!private->rfk_data)) {
		ret = -ENOMEM;
		goto sysfs_register_failed;
	}

	ret = rfkill_register(private->rfk_data);
	if (unlikely(ret)) {
		goto rfkill_register_failed;
	}
	dev_set_drvdata(&pdev->dev, private);

	return 0;

rfkill_register_failed:
		rfkill_destroy(private->rfk_data);
sysfs_register_failed:
		private->free_gpio();
gpio_request_failed:
		kfree(private);
		private = NULL;
		return ret;
}

static int csrbc_mode_remove(struct platform_device *pdev)
{
	int	index;
	struct csr_bc_info *info = (struct csr_bc_info*) dev_get_drvdata(&pdev->dev);
	BUG_ON(info == NULL);

	PBP_DEBUG( "%s: Removing CSR BlueCore mode driver.\n", pdev->name );

	/* Deregister rfkill driver */
	rfkill_unregister(info->rfk_data);
	rfkill_destroy(info->rfk_data);

	/* Deregister each 'file' in sysfs seperately. */
	for( index=0; index < SIZE_CSRBC_SYSFS_ATTR; index++ ){
		device_remove_file( &pdev->dev, &csrbc_mode_sysfs_attributes[index] );
	}

	if (info->free_gpio)
		info->free_gpio();

	kfree(info);
	dev_set_drvdata(&pdev->dev, NULL);
	return 0;
}


#ifdef CONFIG_PM
static int csrbc_mode_suspend(struct platform_device *pdev, pm_message_t msg)
{
	struct csr_bc_info *info = (struct csr_bc_info*) dev_get_drvdata(&pdev->dev);
	BUG_ON(info == NULL);

	if (info->suspend)
		info->suspend();

	return 0;
}
 
static int csrbc_mode_resume(struct platform_device *pdev)
{
	struct csr_bc_info *info = (struct csr_bc_info*) dev_get_drvdata(&pdev->dev);
	BUG_ON(info == NULL);

	if (info->resume)
		info->resume();

	return 0;
}
#else
#define csrbc_mode_suspend	NULL
#define csrbc_mode_resume	NULL
#endif

static struct platform_driver csrbc_mode_driver = {
	.driver		= {
		.name	= "csr-bc",
		.owner	= THIS_MODULE,
	},

	.probe		= csrbc_mode_probe,
	.remove		= csrbc_mode_remove,
	.suspend	= csrbc_mode_suspend,
	.resume		= csrbc_mode_resume,
};

// END: TODO

static int __init csrbc_mode_init_module(void)
{
	int ret = 0;

	ret = platform_driver_register( &csrbc_mode_driver );
	if (ret) {
		printk( KERN_ERR "%s: Failed to register CSR BlueCore mode driver!: %d\n", csrbc_mode_driver.driver.name,ret );
	}

	return ret;
}

static void csrbc_mode_exit_module(void)
{
	platform_driver_unregister(&csrbc_mode_driver);
}
module_init(csrbc_mode_init_module);
module_exit(csrbc_mode_exit_module);

MODULE_DESCRIPTION( "TomTom CSR BlueCore mode driver, (C) 2008 TomTom BV" );
MODULE_LICENSE("GPL");

