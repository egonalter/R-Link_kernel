#ifndef __APPLE_AUTH__
#define __APPLE_AUTH__

#include <linux/platform_device.h>

struct apple_auth_platform_data {
	int (* init)(struct platform_device *pdev);
	int (* resume)(struct platform_device *pdev);
	int (* suspend)(struct platform_device *pdev, pm_message_t state);
	void (* set_mode)(void);
};

#endif /* __APPLE_AUTH__ */
