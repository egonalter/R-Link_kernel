#ifndef _INCLUDE_LINUX_VGPIO_H
#define _INCLUDE_LINUX_VGPIO_H

struct vgpio_pin {
	unsigned int	vpin; // Probably useless in most cases.
	unsigned int	tpin;
	unsigned int	valid:1;
	unsigned int	inverted:1;
	unsigned int	notcon:1;
};

#define _VGPIO_DEF_PIN(b,v,t,i,n)	[(v) - (b)] = { \
							.vpin		= (v), \
							.tpin		= (t), \
							.inverted	= (i), \
							.notcon		= (n), \
							.valid		= 1, \
							}

#define VGPIO_DEF_PIN(b,v,t)	_VGPIO_DEF_PIN(b,v,t,0,0)
#define VGPIO_DEF_INVPIN(b,v,t)	_VGPIO_DEF_PIN(b,v,t,1,0)
#define VGPIO_DEF_NCPIN(b,v)	_VGPIO_DEF_PIN(b,v,0,0,1)

struct platform_device;

struct vgpio_platform_data {
	unsigned int		gpio_base;
	unsigned int		gpio_number;
	struct vgpio_pin	*pins;
	void			*context;
	int 			(*setup)(struct platform_device *, void *);
	int			(*teardown)(struct platform_device *, void *);
};

extern int vgpio_to_gpio(unsigned gpio);

#endif
