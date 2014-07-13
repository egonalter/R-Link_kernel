#include <linux/platform_device.h>
#include <plat/tt_mem.h>

static int allowed = 0;

int devmem_is_allowed(unsigned long pagenr)
{
	return allowed;
}

static void devmem_allow (void)
{
	allowed = 1;
}

static void devmem_disallow (void)
{
	allowed = 0;
}

static struct ttmem_info mem_info = {
	.allow 		= devmem_allow,
	.disallow	= devmem_disallow
};

static struct platform_device tt_device_mem =
{
	.name	= "ttmem",
	.id	= -1,
	.dev = {
		.platform_data = ((void *)&mem_info),
	}
};


static __init int devmem_init(void)
{
	return  platform_device_register (&tt_device_mem);
}

arch_initcall(devmem_init);

