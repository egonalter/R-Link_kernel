#ifndef _MENDOZA_S5M8751_H
#define _MENDOZA_S5M8751_H

extern struct platform_device mendoza_s5m8751_bl;
extern struct platform_device mendoza_s5m8751_codec;
extern struct platform_device mendoza_s5m8751_pmic;

typedef struct
{
        void (*amp_pwr_en)(int);

        void (*suspend)(void);
        void (*resume)(void);
        void (*config_gpio) (void);
        int (*request_gpio)  (void);
        void (*free_gpio) (void);
} s5m8751_platform_t;

#ifndef CONFIG_MFD_S5M8751_I2C
#define mendoza_s5m8751_i2c_init(x) do {} while(0)
#define mendoza_s5m8751_pmic_setup(x) do {} while(0)
#else
int mendoza_s5m8751_i2c_init(int);
void mendoza_s5m8751_pmic_setup(int);
#endif

#endif

