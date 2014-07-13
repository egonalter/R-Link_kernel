/*****************************************************************************
* Place text here
*****************************************************************************/
#ifndef __MENDOZA_LCD_H__
#define __MENDOZA_LCD_H__

struct lcm_desc {
	char			*lcm_name;
	struct s3cfb_lcd	*lcm_info;
	int			(*gpio_setup)(struct platform_device *pdev);
	int			(*reset_lcd)(struct platform_device *pdev);
	int			(*shutdown)(struct notifier_block *block, unsigned long code, void *null);
	int			(*reset_power_onoff)(struct platform_device *pdev, int power);
};

struct lcd_paneldev {
	struct list_head	list;
	struct lcm_desc		*panel;
};

extern void mendoza_lcd_register(struct lcd_paneldev *dev);
extern void mendoza_lcd_unregister(struct lcd_paneldev *dev);
extern void mendoza_lcd_lookup (void);

#endif /* __MENDOZA_LCD_H__ */


