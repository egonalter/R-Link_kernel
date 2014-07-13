/*
 * tt_crypto.c - tt specific crypto using crypto context passed by the bootloader
 *
 * 2010-03-08 Ard Biesheuvel <ard.biesheuvel@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/io.h>
#include <linux/scatterlist.h>
#include <linux/crypto/ttcrypto_ctx.h>
#include <linux/crypto/tt_crypto.h>
#include <plat/factorydata.h>
#include <asm/uaccess.h>
#include <crypto/aes.h>
#include <crypto/hash.h>

#define TTCRYPTO_DEV_MAJOR	178
#define TTCRYPTO_DEV_NAME	"ttcrypto"

extern struct factorydata_buffer_info_t fdt_buffer_info;

static struct ttcrypto_ctx ctx;
static struct class *ttcrypto_class;

static int ttcrypto_hmac_sha1(char *key, size_t klen,	// key and key length
      char *data_in, size_t dlen,			// data in and length
      char *hash_res, size_t hlen);			// hash buffer and length

static int ttcrypto_ss_decrypt(struct ttcrypto_ss_crypt *in)
{
	struct blkcipher_desc desc = { .flags = CRYPTO_TFM_REQ_MAY_SLEEP };
	struct scatterlist sg;
	unsigned int s;
	unsigned char *buf;
	int rc = 0;

	if ( IS_ERR(desc.tfm = crypto_alloc_blkcipher("ecb(aes)", 0, CRYPTO_ALG_ASYNC))) {
		printk(KERN_ERR "ttcrypto: required aes cipher not available\n");
		return -ENOENT;
	}

	/* use the leading 128 bits of the shared secret as AES key */
	if (0 > crypto_blkcipher_setkey(desc.tfm, ctx.shared_secret, AES_KEYSIZE_128)) {
		printk(KERN_ERR "ttcrypto: failed to set aes key\n");
		rc = -ENOENT;
		goto freecipher;
	}
	
	if (copy_from_user((void*)&s, (void*)&in->size, sizeof(s))) {
		rc = -EINVAL;
		goto freecipher;
	}

	if ( !(buf = kmalloc(s, GFP_KERNEL))) {
		rc = -ENOMEM;
		goto freecipher;
	}
	
	if (copy_from_user(buf, (void*)&in->data, s)) {
		rc = -ENOMEM;
		goto freemem;
	}

	/* set up a scatterlist containing the whole data area */
	sg_init_one(&sg, buf, s);

	if (0 != (rc = crypto_blkcipher_decrypt(&desc, &sg, &sg, s)))
		goto freemem;

	if (copy_to_user((void*)&in->data, buf, s))
		rc = -ENOMEM;

freemem:
	kfree(buf);
freecipher:
	crypto_free_blkcipher(desc.tfm);
	return rc;
}

static int ttcrypto_ss_decrypt_cbc(struct ttcrypto_ss_crypt *in)
{
	struct blkcipher_desc desc = { .flags = CRYPTO_TFM_REQ_MAY_SLEEP };
	struct scatterlist sg;
	unsigned int s;
	unsigned char *buf;
	int rc = 0;

	if ( IS_ERR(desc.tfm = crypto_alloc_blkcipher("cbc(aes)", 0, CRYPTO_ALG_ASYNC))) {
		printk(KERN_ERR "ttcrypto: required aes cipher not available\n");
		return -ENOENT;
	}

	/* use the leading 128 bits of the shared secret as AES key */
	if (0 > crypto_blkcipher_setkey(desc.tfm, ctx.shared_secret, AES_KEYSIZE_128)) {
		printk(KERN_ERR "ttcrypto: failed to set aes key\n");
		rc = -ENOENT;
		goto freecipher;
	}
	
	if (copy_from_user((void*)&s, (void*)&in->size, sizeof(s))) {
		rc = -EINVAL;
		goto freecipher;
	}

	if ( !(buf = kmalloc(s, GFP_KERNEL))) {
		rc = -ENOMEM;
		goto freecipher;
	}
	
	if (copy_from_user(buf, (void*)&in->data, s)) {
		rc = -ENOMEM;
		goto freemem;
	}

	/* set up a scatterlist containing the whole data area */
	sg_init_one(&sg, buf, s);

	if (0 != (rc = crypto_blkcipher_decrypt(&desc, &sg, &sg, s)))
		goto freemem;

	if (copy_to_user((void*)&in->data, buf, s))
		rc = -ENOMEM;

freemem:
	kfree(buf);
freecipher:
	crypto_free_blkcipher(desc.tfm);
	return rc;
}

static int ttcrypto_ss_encrypt_cbc(struct ttcrypto_ss_crypt *in)
{
	struct blkcipher_desc desc = { .flags = CRYPTO_TFM_REQ_MAY_SLEEP };
	struct scatterlist sg;
	unsigned int s;
	unsigned char *buf;
	int rc = 0;

	if ( IS_ERR(desc.tfm = crypto_alloc_blkcipher("cbc(aes)", 0, CRYPTO_ALG_ASYNC))) {
		printk(KERN_ERR "ttcrypto: required aes cipher not available\n");
		return -ENOENT;
	}

	/* use the leading 128 bits of the shared secret as AES key */
	if (0 > crypto_blkcipher_setkey(desc.tfm, ctx.shared_secret, AES_KEYSIZE_128)) {
		printk(KERN_ERR "ttcrypto: failed to set aes key\n");
		rc = -ENOENT;
		goto freecipher;
	}
	
	if (copy_from_user((void*)&s, (void*)&in->size, sizeof(s))) {
		rc = -EINVAL;
		goto freecipher;
	}

	if ( !(buf = kmalloc(s, GFP_KERNEL))) {
		rc = -ENOMEM;
		goto freecipher;
	}
	
	if (copy_from_user(buf, (void*)&in->data, s)) {
		rc = -ENOMEM;
		goto freemem;
	}

	/* set up a scatterlist containing the whole data area */
	sg_init_one(&sg, buf, s);

	if (0 != (rc = crypto_blkcipher_encrypt(&desc, &sg, &sg, s)))
		goto freemem;

	if (copy_to_user((void*)&in->data, buf, s))
		rc = -ENOMEM;

freemem:
	kfree(buf);
freecipher:
	crypto_free_blkcipher(desc.tfm);
	return rc;
}

static int ttcrypto_ss_hmac(struct ttcrypto_ss_hmac *in)
{
	int rc = 0;
	unsigned int s;
	unsigned char *buf;
	char hash_buffer[20];

	if (copy_from_user((void*)&s, (void*)&in->size, sizeof(s))) {
		rc = -EINVAL;
		goto exitfunction;
	}

	if (s < sizeof(hash_buffer)) {
		// Too short for any use, too small buffer to transport back the hash
		rc = -EINVAL;
		goto exitfunction;
	}

	if ( !(buf = kmalloc(s, GFP_KERNEL))) {
		rc = -ENOMEM;
		goto exitfunction;
	}

	if (copy_from_user(buf, (void*)&in->data, s)) {
		rc = -ENOMEM;
		goto freemem;
	}

	ttcrypto_hmac_sha1((char*)&ctx.shared_secret, 16,
			   buf, s,
			   hash_buffer, sizeof(hash_buffer));

	s = sizeof(hash_buffer);
	if (copy_to_user((void*)&in->hmac, hash_buffer, s)) 
		rc = -ENOMEM;

freemem:
	kfree(buf);
exitfunction:
	return rc;
}

static int ttcrypto_ioctl(struct inode *i, struct file *fp, unsigned int cmd, unsigned long arg)
{
	if (!ctx.version)
		return -ENOENT;

	switch (cmd) {
		default:			return -EINVAL;
		case TTCRYPTO_SS_DECRYPT:	return ttcrypto_ss_decrypt((void*)arg);
		case TTCRYPTO_SS_ENCRYPT_CBC:	return ttcrypto_ss_encrypt_cbc((void*)arg);
		case TTCRYPTO_SS_DECRYPT_CBC:	return ttcrypto_ss_decrypt_cbc((void*)arg);
		case TTCRYPTO_SS_HMAC:		return ttcrypto_ss_hmac((void*)arg);
	}
	return 0;
}

static int ttcrypto_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int ttcrypto_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static struct file_operations ttcrypto_fops = {
	.owner		= THIS_MODULE,
	.ioctl		= ttcrypto_ioctl,
	.open		= ttcrypto_open,
	.release	= ttcrypto_release
};

static int __init tomtom_crypto_init(void)
{
	struct device * ttcrypto_dev;
	struct ttcrypto_ctx *c;
	int rc;

	c = (struct ttcrypto_ctx *)
		((unsigned long)(fdt_buffer_info.address + fdt_buffer_info.size) & ~TTCRYPTO_ALIGN_MASK);

	/* look for the correct magic */
	if (c->magic != TTCRYPTO_CTX_MAGIC) {
		printk (KERN_ERR "ttcrypto: incorrect magic, no crypto context found\n");
		return -1;
	}

	ttcrypto_class = class_create(THIS_MODULE, "ttcrypto");
	if (IS_ERR(ttcrypto_class)) {
		rc = PTR_ERR(ttcrypto_class);
		printk(KERN_ERR "Unable to create ttcrypto class (%d)\n", rc);
		goto class_create_err;
	}

	rc = register_chrdev(TTCRYPTO_DEV_MAJOR, TTCRYPTO_DEV_NAME, &ttcrypto_fops);
	if (rc < 0) {
		printk (KERN_ERR "ttcrypto: can't create char device: %d\n", rc);
		goto dev_reg_err;	
	}

	printk(KERN_WARNING "ttcrypto: found version %d tt_crypto context\n", c->version);

	memcpy(&ctx, c, sizeof(struct ttcrypto_ctx));
	memset(c, 0, sizeof(struct ttcrypto_ctx));

	ttcrypto_dev = device_create(ttcrypto_class, NULL, MKDEV(TTCRYPTO_DEV_MAJOR, 0),
                                                                NULL, "%s", TTCRYPTO_DEV_NAME);
	if (IS_ERR(ttcrypto_dev)) {
		rc = PTR_ERR(ttcrypto_dev);
		printk(KERN_ERR "Unable to create device with major=%d, minor=%d (%d)\n",
			TTCRYPTO_DEV_MAJOR, 0, rc);
		goto dev_create_err;
	}
	return 0;

dev_create_err:
	unregister_chrdev(TTCRYPTO_DEV_MAJOR, TTCRYPTO_DEV_NAME);
dev_reg_err:
	class_destroy(ttcrypto_class);
class_create_err:
        return rc;
}

module_init(tomtom_crypto_init);


/* Code from http://www.ee.cooper.edu/~stefan/doku.php?id=hmac_sha1_linux
   Now part of ttcrypto.
   Found using Google search, under GPLv2 license. 
*/

/*
 * This code is part of the TUBA kernel trust agent cryptkbd
 *
 * Copyright (c) 2009 Deian Stefan
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * CREDIT:
 * the tcrypt testing module was very helpful in writing this
 * the module is credited to
 * Copyright (c) 2002 James Morris <jmorris@intercode.com.au>
 * Copyright (c) 2002 Jean-Francois Dive <jef@linuxbe.org>
 * Copyright (c) 2007 Nokia Siemens Networks
 *
 */

struct ttcrypto_hmac_sha1_result
{
	struct completion completion;
	int err;
};

static void ttcrypto_hmac_sha1_complete(struct crypto_async_request *req, int err)
{
	struct ttcrypto_hmac_sha1_result *r = req->data;

	if (err == -EINPROGRESS)
		return;

	r->err = err;
	complete(&r->completion);
}

static int ttcrypto_hmac_sha1(char *key, size_t klen,	// key and key length
      char *data_in, size_t dlen,			// data in and length
      char *hash_res, size_t hlen)			// hash buffer and length
{
	int rc = 0;
	struct crypto_ahash *tfm;
	struct scatterlist sg;
	struct ahash_request *req;
	struct ttcrypto_hmac_sha1_result tresult;
	void *hash_buf;

	init_completion(&tresult.completion);
	tfm = crypto_alloc_ahash("hmac(sha1)", 0, 0);
	
	if (IS_ERR(tfm)) {
		printk(KERN_ERR "ttcrypto_hmac_sha1: crypto_alloc_ahash failed.\n");
		rc = PTR_ERR(tfm);
		goto err_tfm;
	}
	
	if ( !(req = ahash_request_alloc(tfm,GFP_KERNEL)))
	{
		printk(KERN_ERR "ttcrypto_hmac_sha1: failed to allocate request for hmac(sha1)\n");
		rc = -ENOMEM;
		goto err_req;
	}
	
	if (crypto_ahash_digestsize(tfm)>hlen) {
		printk(KERN_ERR "ttcrypto_hmac_sha1: tfm size > result buffer.\n");
		rc = -EINVAL;
		goto err_req;
	}
	
	ahash_request_set_callback(req, CRYPTO_TFM_REQ_MAY_BACKLOG,
				   ttcrypto_hmac_sha1_complete, &tresult);

	memset(hash_res, 0, hlen);
	if ( !(hash_buf = kzalloc(dlen, GFP_KERNEL)))
	{
		printk(KERN_ERR "ttcrypto_hmac_sha1: failed to kzalloc hash_buf");
		rc = -ENOMEM;
		goto err_hash_buf;
	}

	memcpy(hash_buf, data_in, dlen);
	sg_init_one(&sg, hash_buf, dlen);

	crypto_ahash_clear_flags(tfm, 0);

	if ((rc = crypto_ahash_setkey(tfm, key, klen)))
	{
		printk(KERN_ERR "ttcrypto_hmac_sha1: crypto_ahash_setkey failed\n");
		goto err_setkey;
	}

	ahash_request_set_crypt(req,&sg,hash_res,dlen);
	rc = crypto_ahash_digest(req);

	switch (rc)
	{
	case 0:
		break;

	case -EINPROGRESS:
	case -EBUSY:
		rc = wait_for_completion_interruptible(&tresult.completion);

		if (!rc && !(rc = tresult.err))
		{
			INIT_COMPLETION(tresult.completion);
			break;
		} else {
			printk(KERN_ERR "ttcrypto_hmac_sha1: wait_for_completeion_interruptible failed\n");
			goto out;
		}
	default:
		goto out;
	}

#ifdef CRYPTKBD_DEBUG
	printk(KERN_DEBUG "ttcrypto_hmac_sha1: result=\n");
	dump_hex(hash_res, hlen);
#endif

out:
err_setkey:
	kfree(hash_buf);
err_hash_buf:
	ahash_request_free(req);
err_req:
	crypto_free_ahash(tfm);
err_tfm:
	return rc;
}
