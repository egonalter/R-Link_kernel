#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#define OMAP_REGISTER(_name,_addr)	\
	{				\
		.name	= #_name,	\
		.addr	= _addr,	\
	}
static struct omap_register {
	char		*name;
	uint32_t	addr;
} omap_registers[] = {
	OMAP_REGISTER(CM_FCLKEN_SGX,		0xfa004b00),
	OMAP_REGISTER(CM_ICLKEN_SGX,		0xfa004b10),
	OMAP_REGISTER(CM_IDLEST_SGX,		0xfa004b20),

	OMAP_REGISTER(CM_FCLKEN_DSS,		0xfa004e00),
	OMAP_REGISTER(CM_ICLKEN_DSS,		0xfa004e10),
	OMAP_REGISTER(CM_IDLEST_DSS,		0xfa004e20),
	OMAP_REGISTER(CM_AUTOIDLE_DSS,		0xfa004e30),
	OMAP_REGISTER(CM_CLKSEL_DSS,		0xfa004e40),
	OMAP_REGISTER(CM_SLEEPDEP_DSS,		0xfa004e44),
	OMAP_REGISTER(CM_CLKSTCTRL_DSS,		0xfa004e48),
	OMAP_REGISTER(CM_CLKSTST_DSS,		0xfa004e4c),

	OMAP_REGISTER(INTCPS_ITR0,		0xfa200080),
	OMAP_REGISTER(INTCPS_ITR1,		0xfa2000a0),
	OMAP_REGISTER(INTCPS_ITR2,		0xfa2000c0),
	OMAP_REGISTER(INTCPS_MIR0,		0xfa200084),
	OMAP_REGISTER(INTCPS_MIR1,		0xfa2000a4),
	OMAP_REGISTER(INTCPS_MIR2,		0xfa2000c4),
	OMAP_REGISTER(INTCPS_PENDING_IRQ0,	0xfa200098),
	OMAP_REGISTER(INTCPS_PENDING_IRQ1,	0xfa2000b8),
	OMAP_REGISTER(INTCPS_PENDING_IRQ2,	0xfa2000d8),
};

static int init_line_debug_files(void);

static int omap_registers_proc_show(struct seq_file *m, void *v)
{
 int			i;
 struct omap_register	*reg;

	for (i=0; i<ARRAY_SIZE(omap_registers); i++) {
		reg	= &omap_registers[i];
		seq_printf(m, "%s\t%#x\n", reg->name, ((uint32_t *)reg->addr)[0]);
	}

	return 0;
}

static int omap_registers_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, omap_registers_proc_show, NULL);
}

static const struct file_operations omap_registers_proc_operations = {
	.open		= omap_registers_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release_private,
};

static int __init init_line_debug_files(void)
{
 struct proc_dir_entry *ret;

	ret	= proc_create("omap_registers", 0444, NULL, &omap_registers_proc_operations);	/* XXX sort by name to reduce the amount of data in each file */
	return IS_ERR(ret);
}
arch_initcall(init_line_debug_files);
