// This file contains the bit definitions as of Si470x firmware 16
#ifndef _BIT_FIELDS_H_
#define _BIT_FIELDS_H_

// Register 00h
#define SI47REG_DEVIDEID    0x00
#define SI47_PN             0xF000
#define SI47_MFGID          0x0FFF

// Register 01h
#define SI47REG_CHIPID      0x01
#define SI47_REV            0xFC00
#define SI47_DEV            0x03C0
#define SI47_FIRMWARE       0x003F

// Register 02h
#define SI47REG_POWERCFG    0x02
#define SI47_DSMUTE         0x8000
#define SI47_DMUTE          0x4000
#define SI47_MONO           0x2000
#define SI47_RDSM           0x0800
#define SI47_SKMODE         0x0400
#define SI47_SEEKUP         0x0200
#define SI47_SEEK           0x0100
#define SI47_DISABLE        0x0040
#define SI47_ENABLE         0x0001

// Register 03h
#define SI47REG_CHANNEL     0x03
#define SI47_TUNE           0x8000
#define SI47_CHAN           0x03FF

// Register 04h
#define SI47REG_SYSCONFIG1  0x04
#define SI47_RDSIEN         0x8000
#define SI47_STCIEN         0x4000
#define SI47_RDS            0x1000
#define SI47_DE             0x0800
#define SI47_AGCD           0x0400

#define SI47_BLNDADJ        0x00C0
#define SI47_BLNDADJ_DN12DB 0x00C0
#define SI47_BLNDADJ_DN6DB  0x0080
#define SI47_BLNDADJ_UP6DB  0x0040
#define SI47_BLNDADJ_DEF    0x0000

#define SI47_GPIO3          0x0030
#define SI47_GPIO3_HI       0x0030
#define SI47_GPIO3_LO       0x0020
#define SI47_GPIO3_ST       0x0010
#define SI47_GPIO3_HIZ      0x0000

#define SI47_GPIO2          0x000C
#define SI47_GPIO2_HI       0x000C
#define SI47_GPIO2_LO       0x0008
#define SI47_GPIO2_INT      0x0004
#define SI47_GPIO2_HIZ      0x0000

#define SI47_GPIO1          0x0003
#define SI47_GPIO1_HI       0x0003
#define SI47_GPIO1_LO       0x0002
#define SI47_GPIO1_HIZ      0x0000

// Register 05h
#define SI47REG_SYSCONFIG2  0x05
#define SI47_SEEKTH         0xFF00

#define SI47_BAND           0x00C0
#define SI47_BAND_JAPAN     0x0080
#define SI47_BAND_JAPANW    0x0040
#define SI47_BAND_USA_EUR   0x0000

#define SI47_SPACE          0x0030
#define SI47_SPACE_50KHZ    0x0020
#define SI47_SPACE_100KHZ   0x0010
#define SI47_SPACE_200KHZ   0x0000

#define SI47_VOLUME         0x000F

// Register 06h
#define SI47REG_SYSCONFIG3  0x06
#define SI47_SMUTER         0xC000
#define SI47_SMUTER_SLOWEST 0xC000
#define SI47_SMUTER_SLOW    0x8000
#define SI47_SMUTER_FAST    0x4000
#define SI47_SMUTER_FASTEST 0x0000

#define SI47_SMUTEA         0x3000
#define SI47_SMUTEA_10DB    0x3000
#define SI47_SMUTEA_12DB    0x2000
#define SI47_SMUTEA_14DB    0x1000
#define SI47_SMUTEA_16DB    0x0000

#define SI47_RDSPRF         0x0200
#define SI47_VOLEXT         0x0100
#define SI47_SKSNR          0x00F0
#define SI47_SKCNT          0x000F

// Register 07h
#define SI47REG_TEST1       0x07
#define SI47_XOSCEN         0x8000
#define SI47_AHIZEN         0x4000

// Register 08h
#define SI47REG_TEST2       0x08
// Register 09h
#define SI47REG_BOOTCONFIG  0x09

// Register 0Ah
#define SI47REG_STATUSRSSI  0x0A
#define SI47_RDSR           0x8000
#define SI47_STC            0x4000
#define SI47_SFBL           0x2000
#define SI47_AFCRL          0x1000
#define SI47_RDSS           0x0800
#define SI47_BLERA          0x0600
#define SI47_ST             0x0100
#define SI47_RSSI           0x00FF

// Register 0Bh
#define SI47REG_READCHAN    0x0B
#define SI47_BLERB          0xC000
#define SI47_BLERC          0x3000
#define SI47_BLERD          0x0C00
#define SI47_READCHAN       0x03FF

// Register 0Ch
#define SI47REG_RDSA        0x0C
#define SI47_RDSA           0xFFFF

// Register 0Dh
#define SI47REG_RDSB        0x0D
#define SI47_RDSB           0xFFFF

// Register 0Eh
#define SI47REG_RDSC        0x0E
#define SI47_RDSC           0xFFFF

// Register 0Fh
#define SI47REG_RDSD        0x0F
#define SI47_RDSD           0xFFFF

#endif
