#ifndef __PMIC_H__
#define __PMIC_H__
#ifdef __cplusplus
extern "C" {
#endif
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
#ifdef __cplusplus
}
#endif
#endif /*__PMIC_H__*/