/*
 * signedloop.c - loop driver plugin with signature checking
 *
 * 2009-10-08 Ard Biesheuvel <ard.biesheuvel@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

/*
 * Theory of operation
 * ===================
 *
 * The signedloop driver aims to improve platform security by checking 
 * each block in a block device against its precalculated hash stored
 * at the end of the image. 
 *
 * The signedloop file layout:
 *     +---------------------+-----------------+--------+---+
 *     |    data blocks      | level 0 hashes  | l1h  .n|sig|
 *     +---------------------+-----------------+--------+---+
 *
 * The first section of the file contains the actual, unencrypted data,
 * padded to be a multiple of 4k.
 *
 * The second section contains SHA1 hashes for all 4k blocks in the data
 * section, putting up to 204 hashes in each level 0 block. This means the
 * last 16 bytes of each level 0 page are unused and should be zero. This
 * section is also padded to be a multiple of 4k.
 * 
 * The third section contains a sequence of hashes of the level 0 hashes,
 * laid out in a similar manner. However, the last 4 bytes of the last 
 * level 1 block contain the number of data blocks in section 1.
 *
 * The last section contains the DSA signature of the level 1 hashes,
 * including the number of data blocks in the last page. The signature
 * itself must be less than 1 page.
 *
 * At init time, the driver finds the number of data blocks by looking
 * at the last 4 bytes of the last full page, calculates the number of 
 * l0 and l1 blocks and loads the full l1 section into memory.
 * The l1 hashes are verified using a DSA public key hardcoded in the kernel
 * and the signature which is located at the end of the file. If the signature
 * is invalid, the init is aborted.
 *
 * When a block is requested from the loop device, the level 0 block
 * containing the hash of that data block is retrieved and checked against
 * its cached l1 hash. If correct, the data block itself is checked in turn.
 * After a page has been verified, a tag is stored in the page private field
 * indicating its validity , and so it will not be checked again before it is
 * paged out. If a hash check fails, the transfer is aborted and an error is 
 * returned.
 *
 * In addition, post_mount() and post_remount() security handlers are installed
 * that force the nodev, nosuid and noexec options on any mounted file system
 * that does not reside on a loop device that uses this driver.
 */
 
#include <linux/module.h>
#include <linux/loop.h>
#include <linux/file.h>
#include <linux/crypto.h>
#include <linux/err.h>
#include <linux/scatterlist.h>
#include <linux/mount.h>
#include <linux/security.h>

#ifdef CONFIG_CRYPTO_DSASIG
#include <linux/crypto/dsasig.h>
#endif

#define LO_CRYPT_SIGNEDLOOP	19
#define SHA1_DIGEST_SIZE	20

#ifdef CONFIG_BLK_DEV_SIGNEDLOOP_RW
#warning CONFIG_BLK_DEV_SIGNEDLOOP_RW enabled -- sig and hash checking not functional on this kernel
#endif

struct signedloop_data {
	struct crypto_hash	*tfm;

	unsigned short		digest_size;
	unsigned short		dg_per_page;

	unsigned long		data_pages;
	unsigned long		l0_pages;

	char 			*l1_digests;
};

static int signedloop_init(struct loop_device *lo, const struct loop_info64 *info)
{
	struct signedloop_data *sl;
	loff_t i, l1_idx;
	pgoff_t l1_pages;
	struct hash_desc desc;
	struct scatterlist *sg;
	struct page *pg;
	unsigned char *p, hash_result[SHA1_DIGEST_SIZE];
	int rc = -ENOMEM;

	if ( !(lo->lo_flags & LO_FLAGS_READ_ONLY)) {
		printk( KERN_ERR "signedloop: read/write mounts not supported\n");
		return -EINVAL;
	}

	if ( !(sl = kzalloc(sizeof(*sl), GFP_KERNEL)))
		return -ENOMEM;

	desc.tfm = sl->tfm = crypto_alloc_hash("sha1", 0, CRYPTO_ALG_ASYNC);
	desc.flags = 0;

	if (IS_ERR(sl->tfm)) {
		kfree(sl);
		return -ENOMEM;
	}

	sl->digest_size = crypto_hash_digestsize(sl->tfm);
	sl->dg_per_page = PAGE_SIZE / sl->digest_size;

	/* grab the size of the data area from the last full page of the image */
	l1_idx = i_size_read(lo->lo_backing_file->f_mapping->host) / PAGE_SIZE -1;
	pg = read_mapping_page(lo->lo_backing_file->f_mapping, l1_idx, NULL );

	if (!pg) {
		printk( KERN_ERR "signedloop: failed to determine size of data area\n");
		goto err;
	}

	p = kmap( pg );

	sl->data_pages	= be32_to_cpu( *(unsigned long*)(p + PAGE_SIZE - sizeof(unsigned long)));

	if (!sl->data_pages) {
		printk( KERN_ERR "signedloop: failed to determine size of data area\n");
		rc = -EINVAL;
		goto err;
	}

	sl->l0_pages	= (sl->data_pages + sl->dg_per_page - 1) / sl->dg_per_page;
	l1_pages	= (sl->l0_pages + sl->dg_per_page - 1) / sl->dg_per_page;

	printk( KERN_INFO "signedloop: using %lu data pages, %lu L0 pages and %lu L1 pages\n", 
		sl->data_pages, sl->l0_pages, l1_pages);

	if (S_ISBLK(lo->lo_backing_file->f_mapping->host->i_mode))
	{
		/* the page containing the number of blocks is not the last l1 page */
		kunmap( pg );
		page_cache_release( pg );

		l1_idx = sl->data_pages + sl->l0_pages + l1_pages -1;

		if ( !(pg = read_mapping_page(lo->lo_backing_file->f_mapping, l1_idx, NULL ))) {
			printk( KERN_ERR "signedloop: failed to map signature area\n");
			goto err;
		}
		p = kmap( pg );
	}

	/* set up a scatterlist pointing to the l1 digests */
	if ( !(sg = kmalloc( l1_pages * sizeof(*sg), GFP_KERNEL ))) {
		kunmap( pg );
		page_cache_release( pg );
		goto err;
	}

	sg_init_table( sg, l1_pages );

	if ( !(sl->l1_digests = kmalloc( PAGE_SIZE * l1_pages, GFP_KERNEL )))
		goto err_sg;

	for ( i = l1_pages-1 ; i >= 0; i-- ) {
		char *l1p = sl->l1_digests + i * PAGE_SIZE;

		memcpy( l1p, p, PAGE_SIZE );
		sg_set_buf( &sg[ i ], l1p, PAGE_SIZE );

		kunmap( pg );
		page_cache_release( pg );

		if (!i)
			break;

		if ( !(pg = read_mapping_page( lo->lo_backing_file->f_mapping, l1_idx-l1_pages+i, NULL )))
			goto err_sg;

		p = kmap( pg );
	}

	if (crypto_hash_digest(&desc, sg, l1_pages * PAGE_SIZE, hash_result))
		goto err_sg;

#ifdef CONFIG_CRYPTO_DSASIG
	/* find the signature at the beginning of the last page */
	if ( !(pg = read_mapping_page( lo->lo_backing_file->f_mapping, l1_idx+1, NULL )))
		goto err_sg;

	p = kmap( pg );

	/* make sure the rootfs loop is signed with key 0, other loops use the other key */
	if (MINOR(lo->lo_device->bd_dev) == 0) {
		if (dsa_verify_hash(hash_result, p, ROOTFS_KEY)) {
			printk( KERN_ERR "signedloop: rootfs signature check failed!\n");
#ifndef CONFIG_BLK_DEV_SIGNEDLOOP_RW
			rc = -EIO;
			goto err_pg;
#endif
		}
	} else {
		if (dsa_verify_hash(hash_result, p, LOOPFS_PROD_KEY)
			&& dsa_verify_hash(hash_result, p, LOOPFS_DEV_KEY)) {
			printk( KERN_ERR "signedloop: loopfs signature check failed!\n");
#ifndef CONFIG_BLK_DEV_SIGNEDLOOP_RW
			rc = -EIO;
			goto err_pg;
#endif
		}
	}

	kunmap( pg );
	page_cache_release( pg );
#endif

	lo->key_data = sl;

	set_capacity( lo->lo_disk, sl->data_pages << (PAGE_SHIFT - 9));
	bd_set_size( lo->lo_device, sl->data_pages << PAGE_SHIFT );

	kfree(sg);

	printk( KERN_INFO "signedloop: activated hash checking on device %s\n", lo->lo_disk->disk_name);

	return 0;

err_pg:
	kunmap( pg );
	page_cache_release( pg );
err_sg:
	kfree(sg);
err:
	crypto_free_hash(sl->tfm);
	kfree(sl);
	return rc;
}

static int signedloop_release(struct loop_device *lo)
{
	struct signedloop_data *sl = lo->key_data;

	/* do not allow this transfer to be disabled if we are not closing the device */
	if  (lo->lo_state != Lo_rundown)
		return -EPERM;

	if (sl->tfm)
		crypto_free_hash(sl->tfm);

	if (sl->l1_digests)
		kfree(sl->l1_digests);

	kfree(sl);
	lo->key_data = NULL;

	return 0;
}

/* time invariant memcmp() function */
static int secure_memcmp(unsigned char *lh, unsigned char *rh, unsigned int size)
{
	int rc = 0;

#ifndef CONFIG_BLK_DEV_SIGNEDLOOP_RW
	while (size--)
		rc += !!(*lh++ ^ *rh++);
#endif

	return rc;
}

static int signedloop_check_hash(struct loop_device *lo, struct page *p )
{
	unsigned long tag = (unsigned long)p->mapping << 16 ^ p->index;
	struct signedloop_data *sl = lo->key_data;
	struct hash_desc desc = { .tfm = sl->tfm, .flags = 0 };
	struct scatterlist sg;
	unsigned char *kmh = NULL, *hash, hash_result[SHA1_DIGEST_SIZE];
	struct page *hash_page = NULL;
	int rc = 0;

	/* have we checked this page already? */
	if (page_private(p) == tag)
		return 0;

	/* are we hashing a data page? */
	if ( p->index < sl->data_pages )
	{
		/* find the page containing the hash */
		pgoff_t hash_page_idx = sl->data_pages + p->index / sl->dg_per_page;

		hash_page = read_mapping_page(
			lo->lo_backing_file->f_mapping, hash_page_idx, NULL );

		if (signedloop_check_hash( lo, hash_page ))
			return -EINVAL;

		kmh = kmap( hash_page );
		hash = kmh + sl->digest_size * (p->index % sl->dg_per_page);
	} 
	/* are we hashing a level 0 hash? */
	else if ( p->index < sl->data_pages+sl->l0_pages )
	{
		unsigned long idx = p->index - sl->data_pages;
		hash = &sl->l1_digests[
			PAGE_SIZE*(idx / sl->dg_per_page) + sl->digest_size*(idx % sl->dg_per_page)];
	}
	else
	{
		/* level 1 hashes have been signature verified at init time */
		printk( KERN_ERR "signedloop: page index out of range (%lu)\n", p->index );
		return -EINVAL;
	}
	
	sg_init_table(&sg, 1);
	sg_set_page(&sg, p, PAGE_SIZE, 0);

	if (crypto_hash_digest(&desc, &sg, PAGE_SIZE, hash_result)
			|| secure_memcmp(hash, hash_result, sl->digest_size))
		rc = -EINVAL;
	else
		/* block devices use the page private field for buffer heads */
		if (!PagePrivate(p))
			set_page_private(p, tag);

	if (kmh) {
		kunmap( hash_page );
		page_cache_release( hash_page );
	}

	return rc;
}

static int signedloop_transfer(struct loop_device *lo, int cmd, struct page *raw_page,
	unsigned raw_off, struct page *loop_page, unsigned loop_off, int size, sector_t real_block)
{
	char *raw_buf, *loop_buf;

	if (cmd != READ)
		return -EINVAL;

	if (lo->key_data && signedloop_check_hash(lo, raw_page ))
	{
		printk( KERN_ERR "Hash check failed on read of sector %lu on device %s\n",
			real_block, lo->lo_disk->disk_name );

		// TODO reset device if hash check fails
		return -EINVAL;
	}

	raw_buf = kmap(raw_page) + raw_off;
	loop_buf = kmap(loop_page) + loop_off;

	memcpy(loop_buf, raw_buf, size); // TODO accelerate?

	kunmap(loop_page);
	kunmap(raw_page);

	cond_resched();
	return 0;
}

static struct loop_func_table signedloop_funcs = {
	.owner		= THIS_MODULE,
	.number		= LO_CRYPT_SIGNEDLOOP,
	.init		= signedloop_init,
	.release	= signedloop_release,
	.transfer	= signedloop_transfer,
};

#ifdef CONFIG_SECURITY

/* automagically activate this driver for each loop mount */
static int signedloop_sb_mount(char *dev_name, struct path *path, char *type,
	unsigned long flags, void *data)
{
	/* try to find the blockdevice related to this mount */
	struct block_device *bdev = open_bdev_exclusive( dev_name, MS_RDONLY, NULL );
	int rc = 0;

	if (IS_ERR(bdev))
		return 0;

	if (MAJOR(bdev->bd_dev) == LOOP_MAJOR && !(flags & MS_NOEXEC))
	{
		struct loop_device *lo = bdev->bd_disk->private_data;

#ifndef CONFIG_BLK_DEV_SIGNEDLOOP_RW
		lo->lo_flags |= LO_FLAGS_READ_ONLY;
		set_device_ro(bdev, 1);
#endif

		if (lo->transfer != signedloop_transfer)
		{
			if (0 == (rc = signedloop_init( lo, NULL )))
				lo->transfer = signedloop_transfer;
		}
	}

	close_bdev_exclusive(bdev, MS_RDONLY);
	return rc;
}

/* force the MS_NOEXEC|MS_NOSUID|MS_NODEV flags on all non-loop mounts */
static void signedloop_verify_mntflags(struct vfsmount *mnt)
{
	/* try to find the blockdevice related to this mount */
	struct block_device *bdev = lookup_bdev( mnt->mnt_devname );

	if (IS_ERR(bdev)) { /* bdev is a special fs like /proc or /sys */
		if (mnt->mnt_parent != mnt) // not root
			mnt->mnt_flags |= MNT_NOEXEC|MNT_NOSUID;
	} else {
		if (MAJOR(bdev->bd_dev) != LOOP_MAJOR
			|| ((struct loop_device*)bdev->bd_disk->private_data)->transfer != signedloop_transfer)
			mnt->mnt_flags |= MNT_NOEXEC|MNT_NOSUID|MNT_NODEV;

		bdput(bdev);
	}
}

static void signedloop_sb_post_addmount(struct vfsmount *mnt, struct path *mountpoint)
{
	signedloop_verify_mntflags(mnt);
}

static void signedloop_sb_post_remount(struct vfsmount *mnt, unsigned long flags, void *data)
{
	signedloop_verify_mntflags(mnt);
}

static int signedloop_ptrace(struct task_struct* t)
{
	return -EPERM;
}

static struct security_operations signedloop_sec_ops = {
	.ptrace_traceme		= signedloop_ptrace,

	.sb_mount		= signedloop_sb_mount,
	.sb_post_addmount	= signedloop_sb_post_addmount,
	.sb_post_remount	= signedloop_sb_post_remount
};

#endif

static int __init signedloop_mod_init(void)
{
	int rc;

	if (0 > (rc = loop_register_transfer(&signedloop_funcs))) {
		printk(KERN_ERR "signedloop: loop_register_transfer() failed (%d)\n", rc);
		return rc;
	}

#ifndef CONFIG_TOMTOM_DEBUG
	if (0 > (rc = register_security( &signedloop_sec_ops ))) {
		printk(KERN_ERR "signedloop: register_security() failed (%d)\n", rc);
		loop_unregister_transfer(LO_CRYPT_SIGNEDLOOP);
	}
#endif
	return rc;
}

static void __exit signedloop_mod_exit(void)
{
	if (loop_unregister_transfer(LO_CRYPT_SIGNEDLOOP))
		printk(KERN_ERR "signedloop: loop_unregister_transfer() failed\n");
}

module_init(signedloop_mod_init);
module_exit(signedloop_mod_exit);

MODULE_LICENSE("GPL");

