
#include <asm/atomic.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/gpio.h>
#include <linux/kref.h>

#include <plat/gpio_cache.h>

/* mach-omap2 */
u16 omap_mux_get_gpio(int gpio);
void omap_mux_set_gpio(u16 val, int gpio);

#ifdef CONFIG_GPIO_CACHE

struct gpio_state {
	struct list_head head;
	struct kref kref;
	unsigned int nr;
	atomic_t in_use;
	u16 val;
	u16 mode;
};

static LIST_HEAD(cached);
static DEFINE_MUTEX(cached_lock);

/* this assumes cached_lock is held by the caller */
static struct gpio_state *gpio_lookup(int nr)
{
	struct gpio_state *state;

	list_for_each_entry(state, &cached, head)
		if (state->nr == nr)
			return state;

	return NULL;
}

/* this assumes cached_lock is held by the caller */
static int gpio_cache_register(struct gpio_cache *gpio)
{
	struct gpio_state *state;

	state = gpio_lookup(gpio->nr);
	if (state) {
		kref_get(&state->kref);
		gpio->state = state;
		return 0;
	}

	state = kzalloc(sizeof(*state), GFP_KERNEL);
	if (!state)
		return -ENOMEM;

	kref_init(&state->kref);
	state->nr = gpio->nr;
	atomic_set(&state->in_use, 0);
	gpio->state = state;

	list_add(&state->head, &cached);
	return 0;
}

static void gpio_cache_release(struct kref *kref)
{
	struct gpio_state *state = container_of(kref, struct gpio_state, kref);
	list_del(&state->head);
	kfree(state);
}

/* this assumes cached_lock is held by the caller */
static int gpio_cache_unregister(struct gpio_cache *gpio)
{
	struct gpio_state *state = gpio->state;

	if (state)
		kref_put(&state->kref, gpio_cache_release);

	gpio->state = NULL;
	return 0;
}

int gpio_cache_alloc(struct gpio_cache *gpio, unsigned int numgpio)
{
	int i, err;

	if (!gpio || !numgpio)
		return 0;

	mutex_lock(&cached_lock);

	err = 0;
	for (i = 0; !err && i < numgpio; i++, gpio++)
		err = gpio_cache_register(gpio);

	mutex_unlock(&cached_lock);
	return err;
}

void gpio_cache_free(struct gpio_cache *gpio, unsigned int numgpio)
{
	int i;

	if (!gpio || !numgpio)
		return;

	mutex_lock(&cached_lock);

	for (i = 0; i < numgpio; i++, gpio++)
		gpio_cache_unregister(gpio);

	mutex_unlock(&cached_lock);
}

int gpio_cache_set_out(struct gpio_cache *gpio, unsigned int numgpio, int val)
{
	struct gpio_state *state;
	u16 mode = 4; /* gpio mode, output */
	int i;

	for (i = 0; i < numgpio; i++, gpio++) {
		state = gpio->state;
		WARN_ON(!state || state->nr != gpio->nr);

		if (atomic_add_return(1, &state->in_use) > 1) {
			atomic_dec(&state->in_use);
			printk(KERN_WARNING "skipping in use state nr. %d...\n", gpio->nr);
			continue;
		}

		state->mode = omap_mux_get_gpio(gpio->nr);
		omap_mux_set_gpio(gpio->nr, mode);

		state->val = gpio_get_value(gpio->nr);
		gpio_set_value(gpio->nr, val);
	}

	return 0;
}

void gpio_cache_restore(struct gpio_cache *gpio, unsigned int numgpio)
{
	struct gpio_state *state;
	int i;

	gpio += numgpio - 1;
	for (i = numgpio; i > 0; i--, gpio--) {
		state = gpio->state;
		WARN_ON(!state || state->nr != gpio->nr);

		if (atomic_sub_return(1, &state->in_use) < 0) {
			atomic_inc(&state->in_use);
			printk(KERN_WARNING "skipping restore nr. %d\n", gpio->nr);
			continue;
		}

		gpio_set_value(gpio->nr, state->val);
		omap_mux_set_gpio(state->mode, gpio->nr);
	}
}

#endif /* CONFIG_GPIO_CACHE */
