#include  <linux/io.h>
#include  <mach/regs-gpio.h>
#include "pmic.h"
#include <plat/gpio-bank-r.h>

/*---------------------------------------------------------------------------------------------------------------*/
#if 0
#define GPRCON1 *(volatile u32 *)(0xE0308294)
#define GPRDAT *(volatile u32 *)(0xE0308298)
#define GPRPUD *(volatile u32 *)(0xE030829C)
#define IIC_ESCL_Hi GPRDAT |= (0x1<<9)
#define IIC_ESCL_Lo GPRDAT &= ~(0x1<<9)
#define IIC_ESDA_Hi GPRDAT |= (0x1<<10)
#define IIC_ESDA_Lo GPRDAT &= ~(0x1<<10)
#define IIC_ESCL_INP GPRCON1 &= ~(0xf<<8)
#define IIC_ESCL_OUTP GPRCON1 = (GPRCON1 & ~(0xf<<8))|(0x1<<8)
#define IIC_ESDA_INP GPRCON1 &= ~(0xf<<12)
#define IIC_ESDA_OUTP GPRCON1 = (GPRCON1 & ~(0xf<<12))|(0x1<<12)
#define DELAY 100
#define S5M8751_ADDR 0x68 // 8751 Chip ID is 0xD0
#endif
/*---------------------------------------------------------------------------------------------------------------*/


static void Delay(void)
{
static int i;
for (i = 0; i < DELAY; i++);
}

static void SCLH_SDAH(void)
{
static int tmp;
tmp = __raw_readl(S5P64XX_GPRDAT);
tmp |= (0x3 << 9);
__raw_writel(tmp, S5P64XX_GPRDAT);
Delay();
}

static void SCLH_SDAL(void)
{
static int tmp;
tmp = __raw_readl(S5P64XX_GPRDAT);
tmp &= ~(0x3 << 9);
tmp |= (0x1 << 9);
__raw_writel(tmp, S5P64XX_GPRDAT);
Delay();
}

static void SCLL_SDAH(void)
{
static int tmp;
tmp = __raw_readl(S5P64XX_GPRDAT);
tmp &= ~(0x3 << 9);
tmp |= (0x1 << 10);
__raw_writel(tmp, S5P64XX_GPRDAT);
Delay();
}

static void SCLL_SDAL(void)
{
static int tmp;
tmp = __raw_readl(S5P64XX_GPRDAT);
tmp &= ~(0x3 << 9);
__raw_writel(tmp, S5P64XX_GPRDAT);
Delay();
}

static void IIC_ELow(void)
{
SCLL_SDAL();
SCLH_SDAL();
SCLH_SDAL();
SCLL_SDAL();
}
static void IIC_EHigh(void)
{
SCLL_SDAH();
SCLH_SDAH();
SCLH_SDAH();
SCLL_SDAH();
}

static void IIC_EStart(void)
{
SCLH_SDAH();
SCLH_SDAL();
Delay();
SCLL_SDAL();
}

static void IIC_EEnd(void)
{
SCLL_SDAL();
SCLH_SDAL();
Delay();
SCLH_SDAH();
}

static void IIC_EAck(void)
{
static int ack;
static int tmp;
tmp = __raw_readl(S5P64XX_GPRCON1);
tmp &= ~(0xf << 12);
__raw_writel(tmp, S5P64XX_GPRCON1);
tmp = __raw_readl(S5P64XX_GPRDAT);
tmp &= ~(0x1 << 9);
__raw_writel(tmp, S5P64XX_GPRDAT);
Delay();
tmp = __raw_readl(S5P64XX_GPRDAT);
tmp |= (0x1 << 9);
__raw_writel(tmp, S5P64XX_GPRDAT);
Delay();
ack = __raw_readl(S5P64XX_GPRDAT);
tmp = __raw_readl(S5P64XX_GPRDAT);
tmp |= (0x1 << 9);
__raw_writel(tmp, S5P64XX_GPRDAT);
Delay();
tmp = __raw_readl(S5P64XX_GPRDAT);
tmp |= (0x1 << 9);
__raw_writel(tmp, S5P64XX_GPRDAT);
Delay();
tmp = __raw_readl(S5P64XX_GPRCON1);
tmp &= ~(0xf << 12);
tmp |= (0x1 << 12);
__raw_writel(tmp, S5P64XX_GPRCON1);
ack = (ack >> 10) & 0x1;
while (ack != 0);
SCLL_SDAL();
}

void IIC_ESetport(void)
{
static int tmp;
tmp = __raw_readl(S5P64XX_GPRPUD);
tmp &= ~(0xf << 8);
__raw_writel(tmp, S5P64XX_GPRPUD);
SCLH_SDAL();
tmp = __raw_readl(S5P64XX_GPRCON1);
tmp &= ~(0xff << 8);
tmp |= ((0x1 << 8) | (0x1 << 12));
__raw_writel(tmp, S5P64XX_GPRCON1);
Delay();
}

void IIC_EWrite(u8 ChipId, u8 IicAddr, u8 IicData)
{
static int i;
IIC_EStart();
/****************** write chip id *******************/
for (i = 7; i > 0; i--) {
if ((ChipId >> (i-1)) & 0x0001)
IIC_EHigh();
else
IIC_ELow();
}
IIC_ELow();
IIC_EAck();
/***************** write reg. addr. *****************/
for (i = 8; i > 0; i--) {
if ((IicAddr >> (i-1)) & 0x0001)
IIC_EHigh();
else
IIC_ELow();
}

IIC_EAck();
/***************** write reg. data. *****************/
for (i = 8; i > 0; i--) {
if ((IicData >> (i-1)) & 0x0001)
IIC_EHigh();
else
IIC_ELow();
}
IIC_EAck();
IIC_EEnd();
}

void IIC_GPIO_Set(void)
{
static int tmp;
tmp = __raw_readl(S5P64XX_GPRCON1);
tmp &= ~(0xff << 8);
tmp |= ((0x6 << 8) | (0x6 << 12));
__raw_writel(tmp, S5P64XX_GPRCON1);
}



#if 0


===========================================================================
s3c_pm_save_gpios();
s3c_pm_save_uarts();
s3c_pm_save_core();
IIC_ESetport();
IIC_EWrite(S5M8751_ADDR, 0x11, 0x3C); // BUCK1 1.8V PFM
/* set the irq configuration for wake */
s3c_pm_configure_extint();

S3c_pm_check_restore();
IIC_EWrite(S5M8751_ADDR, 0x10, 0x85); // BUCK1 1.925V PFM
IIC_EWrite(S5M8751_ADDR, 0x11, 0x7C); // BUCK1 1.925V PWM
IIC_EWrite(S5M8751_ADDR, 0x10, 0x80); // BUCK1 1.8V PWM
IIC_GPIO_Set();
/* ok, let's return from sleep */


===========================================================================

#endif
