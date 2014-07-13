#define _GNU_SOURCE
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/usbdevice_fs.h>
#include <linux/usb/ch9.h>
#include <err.h>
#include <unistd.h>

#include <glib.h>

/* Bits copied from <kernel>/drivers/usb/misc/ehset.c */

#define USB_PORT_FEAT_TEST      21
#define USB_PORT_FEAT_SUSPEND	2
#define USB_CTRL_GET_TIMEOUT	5000

#define USB_RT_HUB              (USB_TYPE_CLASS | USB_RECIP_OTHER)
#define USB_RT_DESC		(USB_DIR_IN | USB_TYPE_STANDARD | USB_RECIP_DEVICE)

struct usbdev_elem {
	int bus;
	int port;
	int dev;
	int prnt;
	int root;
};

enum tests {
	TEST_SE0,
	TEST_J,
	TEST_K,
	TEST_PACKET,
	TEST_FORCE_ENABLE,
	HS_HOST_PORT_SUSPEND_RESUME,
	SINGLE_STEP_GET_DEVICE_DESCRIPTOR,
	SINGLE_STEP_FEATURE,
	TEST_MAX,
};

static struct usbdevfs_ctrltransfer simple_ctrl_req = {
	.bRequestType = USB_RT_HUB,
	.bRequest = USB_REQ_SET_FEATURE,
	.wValue = USB_PORT_FEAT_TEST,
	.timeout = 1000,
};

static struct usbdevfs_ctrltransfer port_susp_ctrl_req = {
	.bRequestType = USB_RT_HUB,
	.wValue = USB_PORT_FEAT_SUSPEND,
	.timeout = 1000,
};

static char desc_buf[USB_DT_DEVICE_SIZE];
static struct usbdevfs_ctrltransfer single_step_desc_ctrl_req = {
	.bRequestType = USB_RT_DESC,
	.bRequest = USB_REQ_GET_DESCRIPTOR,
	.wValue = USB_DT_DEVICE,
	.wLength = USB_DT_DEVICE_SIZE,
	.timeout = USB_CTRL_GET_TIMEOUT,
	.data = desc_buf,
};

static struct usbdevfs_ctrltransfer single_step_set_feat_ctrl_req = {
	.bRequestType = USB_RT_HUB,
	.bRequest = USB_REQ_SET_FEATURE,
	.wValue = USB_PORT_FEAT_TEST,
	.wIndex = (6 << 8)|1,
	.timeout = 60 * 1000,
};

#define TEST_PRIV(n, c, t)			\
	[n] = { .ctrl = c,			\
		.test_index = t }

#define TEST_PRIV_S(n, t)	TEST_PRIV(n, &simple_ctrl_req, t)

static struct test_priv_struct {
	struct usbdevfs_ctrltransfer *ctrl;
	int test_index;
}
test_priv[TEST_MAX] = {
	TEST_PRIV_S(TEST_SE0, 1),
	TEST_PRIV_S(TEST_J, 2),
	TEST_PRIV_S(TEST_K, 3),
	TEST_PRIV_S(TEST_PACKET, 4),
	TEST_PRIV_S(TEST_FORCE_ENABLE, 5),
	TEST_PRIV(HS_HOST_PORT_SUSPEND_RESUME, &port_susp_ctrl_req, 0),
	TEST_PRIV(SINGLE_STEP_GET_DEVICE_DESCRIPTOR, &single_step_desc_ctrl_req, 0),
	TEST_PRIV(SINGLE_STEP_FEATURE, &single_step_set_feat_ctrl_req, 0),
};

static int simple_test(struct usbdev_elem *, GHashTable *, struct test_priv_struct *);
static int port_susp_test(struct usbdev_elem *, GHashTable *, struct test_priv_struct *);
static int single_step_desc_test(struct usbdev_elem *, GHashTable *, struct test_priv_struct *);
static int single_step_set_feat_test(struct usbdev_elem *, GHashTable *, struct test_priv_struct *);

#define TEST_MODE(n, f)				\
	[n] = {	.name = #n,			\
		.func = f,			\
		.priv = &test_priv[n] }

#define TEST_MODE_S(n)	TEST_MODE(n, simple_test)

typedef int (*test_func)(struct usbdev_elem *, GHashTable *, struct test_priv_struct *);
static struct {
	char *name;
	test_func func;
	struct test_priv_struct *priv;
}
test_modes[TEST_MAX] = {
	TEST_MODE_S(TEST_SE0),
	TEST_MODE_S(TEST_J),
	TEST_MODE_S(TEST_K),
	TEST_MODE_S(TEST_PACKET),
	TEST_MODE_S(TEST_FORCE_ENABLE),
	TEST_MODE(HS_HOST_PORT_SUSPEND_RESUME, port_susp_test),
	TEST_MODE(SINGLE_STEP_GET_DEVICE_DESCRIPTOR, single_step_desc_test),
	TEST_MODE(SINGLE_STEP_FEATURE, single_step_set_feat_test),
};

int open_dev(GHashTable *t, struct usbdev_elem *e)
{
	int fd;
	char *s;

	assert(e != NULL);
	if (-1 == asprintf(&s, "/proc/bus/usb/%3.3u/%3.3u", (unsigned) e->bus, (unsigned) e->dev))
		errx(EXIT_FAILURE, "Couldn't asprintf");

	fd = open(s,O_RDWR);
	if (fd < 0) {
		err(EXIT_FAILURE, "open of %s failed", s); 
	}
	free(s);

	return fd;
}

int open_hub(GHashTable *t, struct usbdev_elem *e)
{
	struct usbdev_elem *p;

	assert(e != NULL);
	assert(t != NULL);

	p = e->root == e->dev ? e : g_hash_table_lookup(t, &e->prnt);
	assert(p != NULL);

	printf("Opening bus %d dev %d\n", p->bus, p->dev);
	return open_dev(t, p);
}

int open_root(GHashTable *t, struct usbdev_elem *e)
{
	struct usbdev_elem *r;

	assert(e != NULL);
	assert(t != NULL);

	r = g_hash_table_lookup(t, &e->root);
	assert(r != NULL);

	return open_dev(t, r);
}

static int simple_test(struct usbdev_elem *e, GHashTable *t, struct test_priv_struct *priv)
{
	int test = priv->test_index, ret, fd;

	assert(e != NULL);
	assert(t != NULL);

	printf("dev = %d, test = %d, port = %d\n", e->dev, test, e->port);
	fd = open_hub(t, e);
	priv->ctrl->wIndex = (test << 8)|(e->port+1);
	ret = ioctl(fd, USBDEVFS_CONTROL, priv->ctrl);
	close(fd);

	return ret;
}

static int port_susp_test(struct usbdev_elem *e, GHashTable *t, struct test_priv_struct *priv)
{
	int ret, fd;

	fd = open_hub(t, e);

	priv->ctrl->wIndex = e->port;

	sleep(15);
	priv->ctrl->bRequest = USB_REQ_SET_FEATURE;
	ret = ioctl(fd, USBDEVFS_CONTROL, priv->ctrl);
	if (ret)
		return ret;

	sleep(15);
	priv->ctrl->bRequest = USB_REQ_CLEAR_FEATURE;
	return ioctl(fd, USBDEVFS_CONTROL, priv->ctrl);
}

/* It seems that most USB devices fail when doing this; they can't deal with a device descriptor request
 * at any old time. However, the correct transactions are made compliant with the EHSET test spec, as
 * seen on the bus analyzer
 */
static int single_step_desc_test(struct usbdev_elem *e, GHashTable *t, struct test_priv_struct *priv)
{
	int fd;

	fd = open_dev(t, e);

	sleep(15);
	return ioctl(fd, USBDEVFS_CONTROL, priv->ctrl);
}

static int single_step_set_feat_test(struct usbdev_elem *e, GHashTable *t, struct test_priv_struct *priv)
{
	int fd;

	fd = open_root(t, e);

	/* GetDescriptor's SETUP request -> 15secs delay -> IN & STATUS
	 * Issue request to ehci root hub driver with portnum = 1
	 */
	return ioctl(fd, USBDEVFS_CONTROL, priv->ctrl);
}

static void usage()
{
	int i;

	fprintf(stderr, "Usage: port_test <bus> <dev> <test>\n");
	fprintf(stderr, "Where test is one of:\n");
	for(i=0; i<TEST_MAX; i++)
		if (test_modes[i].func != NULL)
			fprintf(stderr, "  %s\n", test_modes[i].name);
}

int add_element(GHashTable *t, FILE *f, int bus)
{
	struct usbdev_elem *e = malloc(sizeof(struct usbdev_elem)), *p;
	bzero(e, sizeof(struct usbdev_elem));
		
	while (!feof(f)) {
		int r;
		char *s = NULL;

		if (ferror(f))
			errx(EXIT_FAILURE, "File error");

		if (-1 == asprintf(&s, "T: Bus=%2.2u Lev= %%*2u Prnt= %%2u Port= %%2u Cnt= %%*2u Dev#= %%3u Spd= %%*3u MxCh= %%*2u\n", bus))
			errx(EXIT_FAILURE, "Couldn't asprintf");

		e->bus = bus;
		r = fscanf(f, s, &e->prnt, &e->port, &e->dev);
		free(s);

		if (r == 3) {
			g_hash_table_insert(t, &e->dev, e);
			p = g_hash_table_lookup(t, &e->prnt);
			e->root = p == NULL ? e->dev : p->root;
			//printf("bus = %u, prnt = %u, port = %u, dev = %u, root = %u\n",
			//       e->bus, e->prnt, e->port, e->dev, e->root);
			return 0;
		} else
			while (!feof(f) && !ferror(f) && fgetc(f) != '\n');
	}

	return -1;
}

int main(int argc, char *argv[])
{
	int ret;
	int test = 0;
	int bus, dev;
	FILE *f;
	GHashTable *gash;
	struct usbdev_elem *e;

	if (argc != 4) {
		usage();
		exit(EXIT_FAILURE);
	}

	errno = 0;
	bus = strtoul(argv[1], NULL, 10);
	if (errno != 0)
		err(EXIT_FAILURE, "Invalid bus '%s'", argv[1]);
	printf("set bus to %d\n", bus);

	errno = 0;
	dev = strtoul(argv[2], NULL, 10);
	if (errno != 0)
		err(EXIT_FAILURE, "Invalid dev '%s'", argv[2]);
	printf("set dev to %d\n", dev);

	while (test<TEST_MAX && strcasecmp(argv[3], test_modes[test].name))
		test++;
	if (test == TEST_MAX)
		errx(EXIT_FAILURE, "Invalid test '%s'", argv[3]);
	printf("set test to %s\n", test_modes[test].name);

	f = fopen("/proc/bus/usb/devices", "r");
	if (f == NULL)
		err(EXIT_FAILURE, "Couldn't open file");

	gash = g_hash_table_new(g_int_hash, g_int_equal);
	while (0 == add_element(gash, f, bus));

	e = g_hash_table_lookup(gash, &dev);
	if (e == NULL)
		errx(EXIT_FAILURE, "Device %d not found!", dev);
	else
		printf("Device: %d Parent: %d Root: %d\n", e->dev, e->prnt, e->root);

	ret = test_modes[test].func(e, gash, test_modes[test].priv);

	printf("Test return status:%d\n",ret);
	if (ret<0) {
		err(EXIT_FAILURE, "ioctl failed");
	}

	exit(EXIT_SUCCESS);
}

