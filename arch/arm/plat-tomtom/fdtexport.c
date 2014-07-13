/* 
 *  Copyright (C) 2009 TomTom BV <http://www.tomtom.com/>
 *
 *  Export the device-tree buffer to user-land through a char device.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/string.h>
#include <linux/limits.h>
#include <linux/fs.h>
#include <linux/ioport.h>
#include <linux/mm.h>
#include <linux/bootmem.h>
#include <linux/crypto/dsasig.h>
#include <plat/fdt.h>
#include <plat/factorydata.h>
#include <asm/uaccess.h>

/* Defines */
#define PFX "fdtexport:"
#define PK_DBG PK_DBG_FUNC

#define FDT_DEV_MAJOR	177
#define FDT_DEV_MINOR		0
#define FDTRAW_DEV_MINOR	1
#define FDT_DEV_NAME	"fdtexport"
#define FDTRAW_DEV_NAME	"fdtraw"

static char banner[] = KERN_INFO "fdtexport driver, (c) 2009 TomTom BV\n";
static struct class *fdt_class;

static size_t fdt_size;
static size_t fdtraw_size;

static int fdt_open(struct inode *inode, struct file *filp)
{
	switch (iminor(inode)) {
	case FDT_DEV_MINOR:
		filp->private_data = &fdt_size;
		break;
	case FDTRAW_DEV_MINOR:
		filp->private_data = &fdtraw_size;
		break;	
	default:
		return -ENODEV;
	}
		
	return 0;
}

static int fdt_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t fdt_read(struct file *filp, char __user *buf,
		size_t count, loff_t *f_pos)
{
	ssize_t retval = 0;
	size_t size = *((size_t*)filp->private_data);
	void *from = fdt_buffer_info.address;
	if (*f_pos < size) {
		if (count > size - *f_pos) {
			count = size - *f_pos;
		}

		from += *f_pos;
		
		if (!copy_to_user(buf, from, count)) {
			*f_pos += count;
			retval = count;
		} else {		
			retval = -EFAULT;
		}
	} else {
		retval = 0;
	}

	return retval;
}

loff_t fdt_llseek(struct file *filp, loff_t off, int whence)
{
	loff_t newpos;
	size_t size = *((size_t*)filp->private_data);

	switch(whence) {
	  case SEEK_SET:
		newpos = off;
		break;
	  case SEEK_CUR:
		newpos = filp->f_pos + off; 
		break;
	  case SEEK_END:
		newpos = size + off;
		break;
	  default:
		newpos = -EINVAL;
		goto out;
	}
	if (newpos < 0) {
		newpos = -EINVAL;
		goto out;
	}
	filp->f_pos = newpos;
out:
	return newpos;
}

static int fdt_vm_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	return VM_FAULT_SIGBUS;
}

struct vm_operations_struct fdt_vm_ops = {	
	.fault		= fdt_vm_fault,
};

static int fdt_mmap(struct file *filp, struct vm_area_struct *vma)
{
	size_t size = *((size_t*)filp->private_data);
	unsigned long offset	= vma->vm_pgoff << PAGE_SHIFT;
	unsigned long physaddr	= virt_to_phys(fdt_buffer_info.address) + offset;
	unsigned long pfn		= physaddr >> PAGE_SHIFT;
	unsigned long psize		= size - offset;

	if (vma->vm_flags & VM_WRITE) {
		printk(KERN_ERR PFX	" fdt_mmap: no write permission !!\n");
		return -EPERM;
	}

	if (remap_pfn_range(vma, vma->vm_start, pfn, psize, vma->vm_page_prot)) {
		printk(KERN_ERR PFX	" fdt_mmap: pfn_range failed !!\n");
		return -EAGAIN;
	}

	vma->vm_ops				=  &fdt_vm_ops;
	vma->vm_flags			|= VM_RESERVED;
	vma->vm_private_data	=  filp->private_data;

	return 0;
}


struct file_operations fdt_fops = {
	.owner		= THIS_MODULE,
	.read		= fdt_read,
	.open		= fdt_open,
	.llseek		= fdt_llseek,
	.mmap		= fdt_mmap,
	.release	= fdt_release,
};

void __init fdt_reserve_bootmem(void)
{
	int ret;

	if (!fdt_buffer_info.address || !fdt_buffer_info.size) {
		printk(KERN_WARNING "factory data: invalid address (%p) or size (%d)\n",
		                    fdt_buffer_info.address, fdt_buffer_info.size);

		fdt_buffer_info.address = NULL;
		fdt_buffer_info.size = 0;
		return;
	}

	ret = reserve_bootmem((unsigned long) fdt_buffer_info.address,
	                      fdt_buffer_info.size, BOOTMEM_EXCLUSIVE);

	if (ret == 0) {
		printk("factory data: bootmem reserved successful\n");
		fdt_buffer_info.address = phys_to_virt((unsigned long) fdt_buffer_info.address);
	} else {
		printk("factory data: bootmem reservation FAILED [error: %d]\n", ret);
		fdt_buffer_info.address = NULL;
		fdt_buffer_info.size = 0;
	}
}

int __init fdt_export_init_module(void)
{
	int rc = 0;
	struct device * fdtexport_dev;

	printk(banner);

	fdt_class = class_create(THIS_MODULE, "fdt");
	if (IS_ERR(fdt_class)) {
		rc = PTR_ERR(fdt_class);
		printk(KERN_ERR "Unable to create fdt class (%d)\n", rc);
		goto class_create_err;
	}

	if (fdt_buffer_info.address == NULL) {
		printk(KERN_ERR "fdt buffer memory resource is NULL\n");
		rc = -EINVAL;
		goto fdt_buffer_err;
	}
	fdt_size = fdt_totalsize();
	fdtraw_size = fdt_size + DSASIG_SIZE;
	if (fdtraw_size > fdt_buffer_info.size)
		fdtraw_size = fdt_buffer_info.size;

	rc = register_chrdev(FDT_DEV_MAJOR, FDT_DEV_NAME, &fdt_fops);
	if (rc < 0) {
		printk(KERN_ERR "fdt can't create char device: %d\n", rc);
		goto dev_reg_err;
	}

	fdtexport_dev = device_create(fdt_class, NULL, MKDEV(FDT_DEV_MAJOR, FDT_DEV_MINOR), 
								NULL, "%s", FDT_DEV_NAME);
	if (IS_ERR(fdtexport_dev)) {
		rc = PTR_ERR(fdtexport_dev);
		printk(KERN_ERR "Unable to create device with major=%d, minor=%d (%d)\n", 
			FDT_DEV_MAJOR, FDT_DEV_MINOR, rc);
		goto fdt_dev_create_err;
	}
	
	fdtexport_dev = device_create(fdt_class, NULL, MKDEV(FDT_DEV_MAJOR, FDTRAW_DEV_MINOR), 
							NULL, "%s", FDTRAW_DEV_NAME);
	if (IS_ERR(fdtexport_dev)) {
		rc = PTR_ERR(fdtexport_dev);
		printk(KERN_ERR "Unable to create device with major=%d, minor=%d (%d)\n", 
			FDT_DEV_MAJOR, FDTRAW_DEV_MINOR, rc);
		goto fdtraw_dev_create_err;
	}
	return 0;

fdtraw_dev_create_err:
	device_destroy(fdt_class, MKDEV(FDT_DEV_MAJOR, FDT_DEV_MINOR));
fdt_dev_create_err:
	unregister_chrdev(FDT_DEV_MAJOR, FDT_DEV_NAME);
dev_reg_err:
fdt_buffer_err:
	class_destroy(fdt_class);
class_create_err:
	return rc;
}

void __exit fdt_export_exit_module(void)
{
	device_destroy(fdt_class, MKDEV(FDT_DEV_MAJOR, FDTRAW_DEV_MINOR));
	device_destroy(fdt_class, MKDEV(FDT_DEV_MAJOR, FDT_DEV_MINOR));
	unregister_chrdev(FDT_DEV_MAJOR, FDT_DEV_NAME);
	class_destroy(fdt_class);
}

module_init(fdt_export_init_module);
module_exit(fdt_export_exit_module);

MODULE_DESCRIPTION("fdt_export");
MODULE_LICENSE("GPL");

