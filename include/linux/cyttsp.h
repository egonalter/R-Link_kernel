/* include/linux/cyttsp.h
 *
 * Control driver for  Cypress TTSP IC
 *
 * Copyright (C) 2009 TomTom BV <http://www.tomtom.com/>
 * Authors: Vincent Dejouy <vincent.dejouy@tomtom.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef INCLUDE_LINUX_CYTTSP_H
#define INCLUDE_LINUX_CYTTSP_H

typedef enum {
	OPERATING_MODE = 0,
	SYS_INFO_MODE,
	RAW_COUNTS_TEST_MODE = 4,
	DIFFERENCE_COUNTS_TEST_MODE,
	IDAC_SETTINGS_TEST_MODE,
	BASELINE_TEST_MODE,
} CYTTSP_MODE;

typedef enum {
	TTSP_REVISION_UNKNOWN = 0,
	TTSP_REVISION_C,
	TTSP_REVISION_J_OR_HIGHER
} TTSP_REVISION;

enum {
	LOG_DISABLED = 0,
	LOG_ERROR,
	LOG_DEBUG,
	LOG_TUNING,
	LOG_ALL,
};

#define XMIN								0                     
#define XMAX								800
#define YMIN								0
#define YMAX								480                 

#define PRESSUREMIN							0                     
#define PRESSUREMAX							1                     
#define GESTUREMIN							0                     
#define GESTUREMAX							0x10                  
#define FINGERSMIN							0                     
#define FINGERSMAX							2

#define HST_MODE							0                     
#define HST_MODE_DATA_READ_TOGGLE_SHIFT		7
#define HST_MODE_DATA_READ_TOGGLE			(1 << HST_MODE_DATA_READ_TOGGLE_SHIFT)
#define HST_MODE_DEVICE_MODE_SHIFT			4                     
#define HST_MODE_DEVICE_MODE				(7 << HST_MODE_DEVICE_MODE_SHIFT)
#define HST_MODE_LOW_POWER					(1 << 2)
#define HST_MODE_DEEP_SLEEP					(1 << 1)
#define HST_MODE_SOFT_RESET					(1 << 0)


#define TT_MODE								1
#define TT_MODE_BOOTLOADER_FLAG				(1 << 4)
#define TT_MODE_BUFFER_INVALID_SHIFT		5
#define TT_MODE_BUFFER_INVALID				(1 << TT_MODE_BUFFER_INVALID_SHIFT)
#define TT_MODE_NEW_DATA_COUNTER			(3 << 6)

#define TT_STAT								2
#define TT_STAT_OBJECT_DETECTED				(7 << 4)
#define TT_STAT_NUMBER_OF_TOUCHES			(0x0F << 0)

#define GEST_CNT							0x0E
#define GEST_CNT_GESTURE_COUNT				(0x7F << 0)

#define BL_FILE								0

#define BL_STATUS							1
#define BL_STATUS_BUSY						(1 << 7) // Only >= RevJ
#define BL_STATUS_BOOTLOADER_RUN			(1 << 4)
#define BL_STATUS_WATCHDOG_RESET			(1 << 1) // Only >= RevJ
#define BL_STATUS_CHECKSUM_VALID			(1 << 0)

#define BL_ERROR							2
#define BL_ERROR_INVALID_CMD				(1 << 7)
#define BL_ERROR_INVALID_SEC_KEY			(1 << 6)
#define BL_ERROR_BOOTLOADING				(1 << 5)
#define BL_ERROR_CMD_CHECKSUM_ERR			(1 << 4)
#define BL_ERROR_FLASH_PROT_ERR				(1 << 3)
#define BL_ERROR_FLASH_CHECKSUM_ERR			(1 << 2)
#define BL_ERROR_IMAGE_VERIFY_ERR			(1 << 1) // Only <= RevC

/*Warning: Number of touches is a field in a register of the TS                 
  But it means:                                                                 
  0       No touch detected                                             
  1       One touch detected                                            
  2       Two touches detected with ghosting                            
  3       Two touches detected with no ghosting                         
  4 to 15 Reserved                                                      
*/                                                                            

#define MAXNUMTOUCHES						3                     
#define ROWS								11                    
#define COLUMNS								19                    

#define MAX_CHAR_PER_LINE       			200
#define MAX_BYTE_PER_LINE       			(MAX_CHAR_PER_LINE / 2)

#define MAX_LENGTH_I2C_FRAME 				16

#define CHECKSUM_SEED_CMD					0xFF
#define INITIATE_BOOTLOAD_CMD    			0x38
#define WRITE_BLOCK_CMD						0x39
#define VERIFY_BLOCK_CMD					0x3A
#define TERMINATE_BOOTLOAD_CMD				0x3B
#define LAUNCH_APPLICATION_CMD				0xA5

enum {
	STATE_INITIAL = 0,
	STATE_ENTER_BOOTLOADER,
	STATE_WRITE_BLOCK,
	STATE_EXIT_BOOTLOADER,
};

#define ERROR_INVALID_FIRMWARE_FORMAT					10
#define ERROR_FIRMWARE_DOES_NOT_START_WITH_ENTER_BL_CMD	20
#define ERROR_INVALID_CMD_SEQUENCE						30
#define ERROR_UNKNOWN_STATE								40
#define ERROR_UNKNOWN_CMD								50
#define ERROR_TS_REBOOTED								60
#define ERROR_I2C_ACCESS								70
#define ERROR_UNKNOWN_MODE								80

#define SIZE_PREFIX_BOOTLOADER_CMD						2
#define SIZE_BOOTLOADER_CMD								2
#define SIZE_BOOTLOADER_KEY								8

#define MAX_RETRIES_BL_ACCESS							20   //timeout=20*100=2000ms

#define DELAY_AFTER_RESET								100  //in ms 
#define DELAY_BETWEEN_WRITES							200 //in usecs        
#define DELAY_SWITCH_MODES								1000 //in msecs
#define DELAY_LAUNCH_APPLICATION						1000 //in msecs       
#define DELAY_RECALIBRATION								1000 //in msecs
#define DELAY_AFTER_UPDATE								3000 //in ms
#define DELAY_BETWEEN_CMD								10 /* mS */
#define DELAY_BETWEEN_READS								10 /* mS */
#define DELAY_AFTER_WRITE								50   //in ms
#define DELAY_AFTER_ERASE								300  //in ms

struct bootloader_mode_revc
{                                                                             
	unsigned char bl_file;                                               
	unsigned char bl_status;                                                
	unsigned char bl_error;                       
	unsigned short blver;                        
} __attribute__((__packed__));                       

struct bootloader_mode
{                                                                             
	unsigned char bl_file;                                               
	unsigned char bl_status;                                                
	unsigned char bl_error;                       
	unsigned short blver;                        
	unsigned short bld_blver;                        
	unsigned short ttspver;
	unsigned short appid;
	unsigned short appver;
	unsigned char cid_0;
	unsigned char cid_1;
	unsigned char cid_2;
} __attribute__((__packed__));                       

struct operation_mode
{                                                                             
	unsigned char hst_mode;                                               
	unsigned char tt_mode;                                                
	unsigned char tt_stat;                       
	unsigned short touch1_x;
	unsigned short touch1_y;
	unsigned char touch1_z;
	unsigned char touch12_id;                                            
	unsigned short touch2_x;
	unsigned short touch2_y;
	unsigned char touch2_z;
	unsigned char gest_cnt;                        
	unsigned char gest_id;                                              
} __attribute__((__packed__));                       

/*
 * Structure used for older devices
 * depending on the contents of BL_VER (0x0101)
 * the revc structure is used
 */

struct sysinfo_mode_revc
{
	unsigned char hst_mode;
	unsigned char bist_comm;
	unsigned char bist_stat;
	unsigned char unused1;
	unsigned char unused2;
	unsigned char unused3;
	unsigned char unused4;
	unsigned char uid_0;
	unsigned char uid_1;
	unsigned char uid_2;
	unsigned char uid_3;
	unsigned char uid_4;
	unsigned char uid_5;
	unsigned char uid_6;
	unsigned char uid_7;
	unsigned short bl_ver;
	unsigned short tts_ver;
	unsigned short app_id;
	unsigned short app_ver;
	unsigned char unused5;
	unsigned char unused6;
	unsigned char unused7;
	unsigned char unused8;
	unsigned char cid_0;	// x resolution msb
	unsigned char cid_1;	// x resolution lsb
	unsigned char cid_2;	// y resolution msb
	unsigned char cid_3;	// y resolution lsb
	unsigned char cid_4;	// number of gestures
	unsigned char act_intrvl;
	unsigned char tch_tmout;
	unsigned char lp_intrvl;
} __attribute__((__packed__));              

struct sysinfo_mode
{
	unsigned char hst_mode;
	unsigned char mfg_stat;
	unsigned char mfg_cmd;
	unsigned char cid_0;
	unsigned char cid_1;
	unsigned char cid_2;
	unsigned char unused1;
	unsigned char uid_0;
	unsigned char uid_1;
	unsigned char uid_2;
	unsigned char uid_3;
	unsigned char uid_4;
	unsigned char uid_5;
	unsigned char uid_6;
	unsigned char uid_7;
	unsigned short bl_ver;
	unsigned short tts_ver;
	unsigned short app_id;
	unsigned short app_ver;
	unsigned char unused2;
	unsigned char unused3;
	unsigned char unused4;
	unsigned char unused5;
	unsigned char unused6;
	unsigned char unused7;
	unsigned char scn_typ;
	unsigned char act_intrvl;
	unsigned char tch_tmout;
	unsigned char lp_intrvl;
} __attribute__((__packed__));              

struct test_mode_revc
{
	unsigned char hst_mode;                                               
	unsigned char tt_mode;                                                
	unsigned char tt_stat;                                                
	unsigned short touch1_x;                                         
	unsigned short touch1_y;                                         
	unsigned char sns[ROWS*COLUMNS];                                     
} __attribute__((__packed__));

struct test_mode
{
	unsigned char hst_mode;                                               
	unsigned char tt_mode;                                                
	unsigned char tt_stat;                                                
	unsigned short touch1_x;                                         
	unsigned short touch1_y;                                         
	unsigned char sns[ROWS*COLUMNS];                                     
} __attribute__((__packed__));

struct cyttsp_pdata
{
	void (*reset_ts)(void);
	unsigned char key[SIZE_BOOTLOADER_KEY];

	int xmax; /* This field can be changed depending on LCM type (dirty hack for ld050wv1sp01 LCM) */
};

struct cyttsp_drv_data                                                            
{                                                                             
	struct mutex				lock;                                
	struct i2c_client			*client;
	TTSP_REVISION				ttsp_revision;
	CYTTSP_MODE					current_mode;

	union {
		struct bootloader_mode_revc	revc;
		struct bootloader_mode		revj;
	} bl_mode;

	union {
		struct sysinfo_mode_revc	revc;
		struct sysinfo_mode			revj;
	} si_mode;
};

#define SIZE_REVC_SYSINFO_MODE	(sizeof(struct sysinfo_mode_revc) / sizeof(char))
#define SIZE_SYSINFO_MODE		(sizeof(struct sysinfo_mode) / sizeof(char))
#define SIZE_BL_MODE			(sizeof(struct bootloader_mode) / sizeof(char))
#define SIZE_TEST_MODE			(sizeof(struct test_mode) / sizeof(char))
#define SIZE_OPERATION_MODE		(sizeof(struct operation_mode) / sizeof(char))

#define REVC_BLVER				0x1000

#define CYTTSP_DEVNAME			"cyttsp"

extern void cyttsp_dump_rawdata(char * msg, char * ptr, int len);
extern void cyttsp_set_loglevel(int new_loglevel);
extern void cyttsp_restore_loglevel(void);
extern int cyttsp_start_ts_app(struct cyttsp_drv_data *drv_data, void *key, int len);
extern int cyttsp_read_systeminfo(struct cyttsp_drv_data *drv_data);
extern int cyttsp_read_bootloader(struct cyttsp_drv_data *drv_data);
extern TTSP_REVISION cyttsp_get_ttsp_revision(struct cyttsp_drv_data *drv_data);
extern int cyttsp_set_offset_register(struct cyttsp_drv_data *drv_data, char offset);
extern int cyttsp_read_register(struct cyttsp_drv_data *drv_data, char reg, char *value);
extern int cyttsp_write_register(struct cyttsp_drv_data *drv_data, char reg, char value);
extern int cyttsp_switch_to_other_mode(struct cyttsp_drv_data *drv_data, char device_mode);
extern void cyttsp_recalibrate(struct cyttsp_drv_data *drv_data);
extern void cyttsp_printk(int level, const char *fmt, ...);
extern int cyttsp_i2c_recv(struct i2c_client * client, char *buf, int count);
extern int cyttsp_i2c_send(struct i2c_client * client, char *buf, int count);
extern int cyttsp_read_data(struct cyttsp_drv_data *drv_data, char reg, char* buf, int len);

#define CYTTSP_DBG(fmt, args...)		cyttsp_printk(LOG_DEBUG, fmt, ##args)
#define CYTTSP_ERR(fmt, args...)		cyttsp_printk(LOG_ERROR, fmt, ##args)
#define CYTTSP_TUN(fmt, args...)		cyttsp_printk(LOG_TUNING, fmt, ##args)
#define CYTTSP_ALL(fmt, args...)		cyttsp_printk(LOG_ALL, fmt, ##args)

#endif /* INCLUDE_LINUX_CYTTSP_H */

