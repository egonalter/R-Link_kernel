#ifndef __TOMTOM_GPIO_CACHE_H__
#define __TOMTOM_GPIO_CACHE_H__

struct gpio_cache {
	unsigned nr;
	void *state;
};

#define CONFIG_GPIO_CACHE

#ifdef CONFIG_GPIO_CACHE

int gpio_cache_alloc(struct gpio_cache *gpio, unsigned int numgpio);
void gpio_cache_free(struct gpio_cache *gpio, unsigned int numgpio);

int gpio_cache_set_out(struct gpio_cache *gpio, unsigned int numgpio, int val);
void gpio_cache_restore(struct gpio_cache *gpio, unsigned int numgpio);

#else

static inline int gpio_cache_alloc(struct gpio_cache *gpio, unsigned int numgpio)
{
	return 0;
}

static inline void gpio_cache_free(struct gpio_cache *gpio, unsigned int numgpio)
{
}

static inline int gpio_cache_set_out(struct gpio_cache *gpio, unsigned int numgpio, int val)
{
	return 0;
}

static inline void gpio_cache_restore(struct gpio_cache *gpio, unsigned int numgpio)
{
}

#endif /* CONFIG_GPIO_CACHE */

#endif
