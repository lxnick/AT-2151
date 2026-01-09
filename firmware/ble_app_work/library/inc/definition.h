/**
  ******************************************************************************************
  * @file    definition.h
  * @author  k.tashiro
  * @version 1.0
  * @date    2020/09/09
  * @brief   Firmware and Library descripit difinition
  ******************************************************************************************
    Revision:
    Ver            Date             Revised by        Explanation
  ******************************************************************************************
  * 1.0            2020/09/09       k.tashiro         create new
  ******************************************************************************************
*/

#ifndef DEFINITION_H_
#define DEFINITION_H_

#ifdef __cplusplus
extern "C"{
#endif

/* 2020.10.30 Add Debug用定義 ++ */
//#define TEST_RAW_MODE_DATA_OLD
/* 2020.10.30 Add Debug用定義 -- */
//#define TEST_ADV_10S_NO_WALK_3S

#define CHECK_VALID					true
#define CHECK_INVALID				false

/* Unused GPIO Pin Define */
#define UNUSED_GPIO_24				(24)
/* GPIOE Detect Mode ON */
#define GPIO_DETECTMODE_LATCH_ON	(1)

/* Retain RAM Section Index and Setting */
#define RETAIN_RAM_INDEX						(7)
#define RETAIN_RAM_RETAIN_KEEP_ON_SYSTEM_OFF	(POWER_RAM_POWER_S1POWER_On << POWER_RAM_POWER_S1POWER_Pos)
#define RETAIN_RAM_RETAIN_ON					(POWER_RAM_POWERSET_S1RETENTION_On << POWER_RAM_POWERSET_S1RETENTION_Pos)
#define RETAIN_RAM_INITIAL_VALUE				0xfffc
#define RETAIN_RAM_POWER_SET_VALUE				(RETAIN_RAM_RETAIN_KEEP_ON_SYSTEM_OFF | RETAIN_RAM_RETAIN_ON)

/* FIFO Settting */
#define EVT_FIFO_SIZE					(40)		/* Event FIFO Number */
#define ACC_GYRO_FIFO_SIZE    			(40)		/* ACC/Gyro Sensor FIFO Number */
#define SYSTEM_OFF_ENTERY_RETRY_LIMIT   (10)		/* Standby Entry Retry Limit Count */
#define BATTERY_VOLT_FIFO_SIZE			(60)		/* Get Battery Voltage from Comparator Count*/

/* Mode Manager Setting */
#define ULT_DATA_SIZE					(10)
#define ULT_END_TRIGGER 				(255)
#define DAILY_ID_CONTINUE				(1)
#define DAILY_ID_SINGLE					(2)
#define MALE   							(0)				/* player data */
#define FEMALE  						(1)				/* player data */
//#define DAILY_DATA_RAMSAVE_ADDRESS		(0x20005ECC)	/* Player Data and Daily Data Ram Save Address */
/* Retention RAM領域に512byteを割り当てる
 *  ただし、0x20007E00からTraceLogが領域(198byte)を使用しているためその分領域(CC:204byte分)を確保して定義している
 */
#define DAILY_DATA_RAMSAVE_ADDRESS		(0x2000FECC)	/* Player Data and Daily Data Ram Save Address (512 byte:0x200)*/

/* BLE Command */
#define BLE_CMD_INIT_MODE					(0)			/* BLE Command Initialize */
#define BLE_CMD_TIME_INFO_SET_MODE			(1)			/* BLE Command Time Information Set Mode */
#define BLE_CMD_PLAYER_INFO_SET_MODE		(2)			/* BLE Command Player Information Set Mode */
#define BLE_CMD_ACCEPT_PINCODE_CHECK_MODE	(3)			/* BLE Command Accept pin code check */
/* 2022.05.13 Add ゲストモード追加 ++ */
#define BLE_CMD_GUEST_MODE					(4)			/* BLE Command Guest Mode */
/* 2022.05.13 Add ゲストモード追加 -- */

/* Retention RAM Default Setting */
#define RAM_SIG								"RAM_SAVE_SIGN"
#define RMA_SIG_SIZE						(13)
#define DEFAULT_WALK						(0)
#define DEFAULT_RUN							(0)
#define DEFAULT_DASH						(0)
#define DEFAULT_SID							(0)
#define DEFAULT_YEAR						(0)
#define DEFAULT_MONTH						(1)
#define DEFAULT_DAY							(1)
#define DEFAULT_HOUR						(0)
#define DEFAULT_MIN							(0)
#define DEFAULT_SEC							(0)
#define DEFAULT_AGE							(6)
#define DEFAULT_WEIGHT						(20)
#define DEFAULT_SEX							MALE
#define DEFAULT_SENSOR_OFFSET_SIG			(0xffff)
#define DEFAULT_SENSOR_OFFSET				(0xffffffff)
#define DEFAULT_WEEKDAYS					(0x06)			/* 2022.01.11 Add Day-of-the-week default設定追加 */

/* No Walk Time */
#ifdef TEST_ADV_10S_NO_WALK_3S
	#define NO_WALK_TIMEOUT					(3)			/* Unit[sec] */
#else
	#define NO_WALK_TIMEOUT					(10)		/* Unit[sec] */
#endif

#define NO_COUNT_WALK						(0)

/* SkyJump calculat Unit Setting */
#define START_REACTION_TIME_UNIT_10MS		(10)		/* 10ms */
#define NOTIFY_RESOURCE_NON					(1)
#define NOTIFY_RESOURCE_EXIST				(0)

/* Daily Mode Setting */
#define FLASH_DATA_MAX 						(256)		/* 1セクターに書き込むデータの最大数 */
//#define FLASH_DATA_C						(128)
#define FLASH_DATA_CENTER					(128)		/* データの中央値 */
#define SEND_NO_DATA_STATE					(0)
#define SEND_DATA_STATE						(1)
#define SEND_FINISH_STATE					(2)

/* app_timer Setting
 *  timer Timeout value. unit [ms]
 */
//#define FORCE_TIMEOUT_MS					(18000000)	/* 強制切断までのタイムアウト時間(5時間[18000s]) */
/* 2022.07.06 Modify 強制切断までのタイムアウト時間を変更 */
//#define FORCE_TIMEOUT_MS					(3600000)	/* 強制切断までのタイムアウト時間(1時間[3600s]) */

/* 2022.08.26 Modify 強制切断までのタイムアウト時間を変更 */
#define FORCE_TIMEOUT_MS					(900000)	/* 強制切断までのタイムアウト時間(1時間[900s])*/

#define PARAM_UPDATE_TIMEOUT_MS    			(3000)		/* BLE Parameter Update Timeout 3s */
#define PARAM_UPDATE_ERROR_TIMEOUT_MS		(30000)		/* BLE Parameter Update Error Timeout 30s */
#define WAIT_DISCON_CALLBACK_TIMEOUT_MS		(30000)		/* 切断待ち時間 30s */
#define ULT_ACK_TIMEOUT_MS					(2000)		/* Unlimitiv ACK Timeout 2s */
#define ULT_START_TIMEOUT_MS				(120000)	/* Unlimitiv Start Timeout 120s */
#define TIMER_UNIT_MS						(1000)		/* 1s Timeout */
#define BAT_TIMEOUT_MS						(1000)		/* 2022.01.15 Add Get Battery Timeout 1s */
#define INIT_TIMEOUT_MS						(5000)		/* Initialize Timeout Value 5s */
/* 2022.06.03 Add RSSI通知時間 ++ */
#define RSSI_NOTIFY_MS						(5000)		/* RSSI Notify Timer Value 5s */
//#define RSSI_NOTIFY_MS						(100)		/* RSSI Notify Timer Value 100ms(Test用) */
/* 2022.06.03 Add RSSI通知時間 -- */
/* 2020.11.20 Add Mode Change時のTimeout時間の最大値 ++ */
/* 2022.01.15 Modify Mode Change時のTimeout時間の最大値を1805 -> 10805に変更 */
#define MODE_CHANGE_TIMEOUT_MAX_VAL			10805
/* 2020.11.20 Add Mode Change時のTimeout時間の最大値 -- */

/* BLE Device and GATT Setting */
//#define DEVICE_NAME 						"UNLIMITIV_SHOES03"		/* Device name, unlimitve shoes */
/*2022 08 26 device nameを変更*/
#define DEVICE_NAME 						"DIGICALIZED_01"		/* Device name, unlimitve shoes */
#define COMPANY_ID							0x0108					/* 0x8000 is dummy data. chicony id 0x0801(0x0108 no little endian) */
#define DATA_INFO_SIZE						17						/* Data Information */
#define MODE_CHANGE_SIZE					5						/* Mode change */
#define MODE_TRIGGER_SIZE					1						/* Mode Trigger */
#define ULTIMET_RES_SIZE 					11						/* Ultimet Result */
#define DAILY_ID_SIZE						3						/* Daily ID */
#define DAILY_LOG_SIZE						16						/* Daily Log */
#define FW_RES_SIZE							1						/* Firmware Response */
#define FW_VERSION_SIZE						3						/* Firmware Version */
#define READ_ERR_SIZE						1						/* Read Error */
#define GET_TIME_SIZE 						12						/* Get Time */
#define BATT_LEVEL_SIZE 		 			2						/* Battery Level */
#define CALIB_SIZE							9						/* calibration datasize. satate + xOffset(2byte) + yOffset(2byte) + zOffset(2byte) */
#define PAIRING_CODE_SIZE 					16						/* Pairing Code */
#define PAIRING_MATCH_SIZE 	 				1						/* Pairing Matching */
#define PAIRING_CLEAR_SIZE					1						/* Pairing Clear */
#define WALK_COUNT_DATA_SIZE				8						/* Walk Count Data */
#define BEACON_INFO_SIZE					16						/* Beacon Information(UniqueID) */
/* 2022.03.22 Add Test Command ++ */
#define TEST_CMD_SIZE						1						/* Test Command */
/* 2022.03.22 Add Test Command -- */
/* 2022.05.16 Add 角度調整 ++ */
#define ANGLE_ADJUST_SIZE					5
/* 2022.05.16 Add 角度調整 -- */
/* 2022.06.09 Add 分解能 ++ */
#define RESOLUTION_SIZE						16						/* Resolution Size */
/* 2022.06.09 Add 分解能 -- */

#define RAW_DATA_TS							2						/* Timestamp Raw Data Size */
#define RAW_DATA_HEADER						1						/* Header Raw Data Size */
#define RAW_DATA_SID						2						/* SID Raw Data Size */
#define RAW_DATA_ACC						6						/* ACC(X,Y,Z) Raw Data Size */
#define RAW_DATA_GYRO						6						/* Gyro(X,Y,Z) Raw Data Size */
#define RAW_DATA_TEMP						2						/* Temperature Raw Data Size */
//#define RAW_DATA_ACC_RESOLUTION			2						/* ACC Resolution Raw Data Size */
//#define RAW_DATA_GYRO_RESOLUTION			2						/* Gyro Resolution Raw Data Size */
/* 2022.06.13 Modify 分解能を読み出せるようにしたためサイズを0に変更 ++ */
#define RAW_DATA_ACC_RESOLUTION				0						/* ACC Resolution Raw Data Size */
#define RAW_DATA_GYRO_RESOLUTION			0						/* Gyro Resolution Raw Data Size */
/* 2022.06.13 Modify 分解能を読み出せるようにしたためサイズを0に変更 -- */
#define RAW_DATA_CHECK_SUM					1						/* Checksum Raw Data Size */
/* Raw Data Size */
#define RAW_DATA_SIZE						RAW_DATA_TS + RAW_DATA_HEADER + RAW_DATA_SID + RAW_DATA_ACC + RAW_DATA_GYRO + RAW_DATA_TEMP + RAW_DATA_ACC_RESOLUTION + RAW_DATA_GYRO_RESOLUTION + RAW_DATA_CHECK_SUM
#define RAW_DATA_NUM						2						/* Raw Data Num */

/* 2020.12.23 Add RSSI取得テスト ++ */
#define RSSI_VALUE_SIZE						2
/* 2020.12.23 Add RSSI取得テスト -- */

#define BLE_CMD_INIT_FLASH_FLAG_ZERO		0
#define BLE_CMD_INIT_FLASH_FLAG_ONE			1
#define BLE_CMD_INIT_FLASH_FLAG_TWO			2
#define BLE_CMD_INIT_FLASH_FLAG_THREE		3
#define BLE_CMD_INIT_FLASH_FLAG_FOUR		4

#define APP_BLE_OBSERVER_PRIO				3			/**< Application's BLE observer priority. You shouldn't need to modify this value. */
#define APP_BLE_CONN_CFG_TAG				1			/**< A tag identifying the SoftDevice BLE configuration. */

#define APP_ADV_INTERVAL					(32)		/**< The advertising interval (in units of 0.625 ms; this value corresponds to 40 ms). */

#ifdef TEST_ADV_10S_NO_WALK_3S
	#define APP_ADV_DURATION				(1000)
#else
	//#define APP_ADV_DURATION				(uint32_t)(6000/1.25)
	#define APP_ADV_DURATION				6000		/* 2020.11.25 Modify unitsが10msになっているため1.25msでの除算を削除 */
#endif

//#define MIN_CONN_INTERVAL					MSEC_TO_UNITS(45, UNIT_1_25_MS)		/**< Minimum acceptable connection interval (0.5 seconds). */
//#define MAX_CONN_INTERVAL					MSEC_TO_UNITS(75, UNIT_1_25_MS)		/**< Maximum acceptable connection interval (1 second). */
/* change connection interval. max 75->45, min 45->15 */
/* 2022.05.25 Modify コネクションインターバルを[max:75 min:45]から[max:45 min:15]に修正  */
#define MIN_CONN_INTERVAL					MSEC_TO_UNITS(15, UNIT_1_25_MS)		/**< Minimum acceptable connection interval (0.5 seconds). */
#define MAX_CONN_INTERVAL					MSEC_TO_UNITS(45, UNIT_1_25_MS)		/**< Maximum acceptable connection interval (1 second). */

#define MIN_CONN_INTERVAL_ONE					MSEC_TO_UNITS(20, UNIT_1_25_MS)		/**< Minimum acceptable connection interval (0.5 seconds). */
#define MAX_CONN_INTERVAL_ONE					MSEC_TO_UNITS(50, UNIT_1_25_MS)		/**< Maximum acceptable connection interval (1 second). */

#define MIN_CONN_INTERVAL_TWO					MSEC_TO_UNITS(25, UNIT_1_25_MS)		/**< Minimum acceptable connection interval (0.5 seconds). */
#define MAX_CONN_INTERVAL_TWO					MSEC_TO_UNITS(55, UNIT_1_25_MS)		/**< Maximum acceptable connection interval (1 second). */

#define MIN_CONN_INTERVAL_THREE					MSEC_TO_UNITS(30, UNIT_1_25_MS)		/**< Minimum acceptable connection interval (0.5 seconds). */
#define MAX_CONN_INTERVAL_THREE					MSEC_TO_UNITS(60, UNIT_1_25_MS)		/**< Maximum acceptable connection interval (1 second). */

#define MIN_CONN_INTERVAL_FOUR					MSEC_TO_UNITS(35, UNIT_1_25_MS)		/**< Minimum acceptable connection interval (0.5 seconds). */
#define MAX_CONN_INTERVAL_FOUR					MSEC_TO_UNITS(65, UNIT_1_25_MS)		/**< Maximum acceptable connection interval (1 second). */

#define MIN_CONN_INTERVAL_FIVE					MSEC_TO_UNITS(40, UNIT_1_25_MS)		/**< Minimum acceptable connection interval (0.5 seconds). */
#define MAX_CONN_INTERVAL_FIVE					MSEC_TO_UNITS(70, UNIT_1_25_MS)		/**< Maximum acceptable connection interval (1 second). */

#define SLAVE_LATENCY						(0)									/**< Slave latency. */
#define FLASH_SLAVE_LATENCY					(1)
#define CONN_SUP_TIMEOUT					MSEC_TO_UNITS(2500, UNIT_10_MS)		/**< Connection supervisory time-out (2.5 seconds). */
//#define FIRST_CONN_PARAMS_UPDATE_DELAY  APP_TIMER_TICKS(FIRST_CONN_PARAMS_UPDATE_DELAY_10MS) /**< Time from initiating event (connect or start of notification) to first time sd_ble_gap_conn_param_update is called (15 seconds). */
//#define NEXT_CONN_PARAMS_UPDATE_DELAY   APP_TIMER_TICKS(NEXT_CONN_PARAMS_UPDATE_DELAY_10MS)                   /**< Time between each call to sd_ble_gap_conn_param_update after the first call (5 seconds). */                                  

/* BLE Manager */
#define PARAM_UPDATE_RETRY_LIMIT			(200)

/* BLE Radio Power */
#define TX_POWER_VALUE						(TX_POWER_DBM_4)		/* BLE radio Tx power set : +4dBm*/

/* WatchDog Timer Interval */
#define WDT_TO_TIME							(5000)		/* unit [ms]. wdt timeout value */
/***************************/

/* Flash Area Setting */
#define PIN_CODE_SIZE						(16)//[byte]
#define PIN_CODE_ERASE_PERMISSION			(1)
#define PAGE_SIZE							(2052)
#define SIGN_DATA_ONE						(0xAA)
#define SIGN_DATA_TWO						(0x55)
#define CHECK_DATA							((SIGN_DATA_ONE << 8) | (SIGN_DATA_TWO))  //0xAA55
#define CHECK_DATA_SIZE						(2)
/* 2022.05.17 Modify DAILY_ADDR1: 0x4E000 -> 0x4F000, DAILY_ADDR2: 0x4F000 -> 0x50000 */
#define DAILY_ADDR1							(0x4F000)			/* Daily Log 1 Flash Address */
#define DAILY_ADDR2							(0x50000)			/* Daily Log 2 Flash Address */
#define PLAYER_AND_PAIRING_ADDR				(0x4D000)			/* Player Data Flash Address */
#define TRACE_ADDR							(0x4C000)			/* Trace Log Flash Address */
#define TEST_PAGE_SIZE 						(0x0fff)
#define PLAYER_DATA_SIZE					(20)
#define TRACE_SIZE							(256)
#define CHECK_1ST_DATA						(0xFF000000)
#define CHECK_2ND_DATA						(0x00FF0000)
#define CHECK_3RD_DATA						(0x0000FF00)
#define CHECK_4TH_DATA						(0x000000FF)
#define CHECK_1ST_SHORAT_DATA				(0xFFFF0000)
#define CHECK_2nd_SHORAT_DATA				(0x0000FFFF)

#define PAIRING_DATA_SIZE 					(20)		/* include check data and dummy for align prog unit 4 byte */
#define FLASH_PARING_START_POS				(2)			/* pariring code flash start position */
#define PAIRING_CHECK_DATA_POS				(0)
#define FLASH_PAIRING_WRITE_POS				PAIRING_CHECK_DATA_POS 

#define PALYER_DATA_SIZE					(8)			/* include check data and dummy for align prog unit 4 byte */
#define FLASH_PLAYER_START_POS				(20)		/* player info flast start position include check data */
#define PLAYER_START_POS					FLASH_PLAYER_START_POS + 2
#define PLAYER_CHECK_DATA_POS				FLASH_PLAYER_START_POS
#define FLASH_PLAYER_WRITE_POS				PLAYER_CHECK_DATA_POS
#define PLAYER_AND_PAIRING_PAGE_SIZE		FLASH_PLAYER_START_POS + PALYER_DATA_SIZE
#define MAX_FLASH_SID						65535

#define MAX_PAGE_DATA						(128)
#define PAGE_FARST_SID_ADD					(16)
#define PAGE_FINISH_SID_ADD					(2048)
#define LOG_WRITE							(1)
#define TRACE_DAMMY							(0)

#define PIN_MATCH							(0)
#define PIN_SUCCESS							(1)
#define PIN_NOT_MATCH						(2)
#define PIN_NOT_EXIST						(3)
#define PIN_INTERVAL_ERR					(4)
#define ADC_READ_ERR						(5)
#define MAX_BLE_SID							(65535)
#define TRACE_MAX_DATA						(64)
/*****************************************/

#define YEAR_HIGH							(0)
#define YEAR_LOW							(1)
#define MONTH_HIGH							(2)
#define MONTH_LOW							(3)
#define DAY_HIGH							(4)
#define DAY_LOW								(5)
#define HOUR_HIGH							(6)
#define HOUR_LOW							(7)
#define MIN_HIGH							(8)
#define MIN_LOW								(9)
#define SEC_HIGH							(10)
#define SEC_LOW								(11)

/* Data Information */
#define DATA_INFO_UUID  					{ 0xb8, 0xe0, 0x44, 0xfd, 0x11, 0x3e, 0x03, 0x91, 0x1e, 0x40, 0xc4, 0xc2, 0x11, 0x2e, 0x82, 0x63 }
#define DATA_SET_UUID 						{ 0xe4, 0x48, 0x92, 0xcc, 0x6a, 0x64, 0xee, 0xbe, 0x71, 0x4f, 0x47, 0x53, 0x12, 0x70, 0xa5, 0x43 }
#define DATA_INFO_UUID2						(0x2e11)
#define DATA_SET_UUID2						(0x7012)

/* Daily - Ultimate mode */
#define MODE_INFO_UUID						{ 0x25, 0xf7, 0xad, 0xc1, 0xb1, 0x55, 0x7f, 0xb8, 0x78, 0x47, 0x4c, 0xcf, 0x62, 0x6c, 0xc0, 0x0a }
#define MODE_CHANGE_UUID					{ 0x2e, 0x96, 0x71, 0xcc, 0x2a, 0x1e, 0x2b, 0xaa, 0xc5, 0x40, 0xcd, 0x57, 0x6c, 0x3a, 0x7a, 0x19 }
#define MODE_TRIGGER_UUID					{ 0x19, 0xa3, 0xd0, 0x83, 0xfd, 0x34, 0x4f, 0x99, 0x36, 0x41, 0x83, 0x2f, 0x30, 0x03, 0x4f, 0xf9 }
#define ULT_RES_UUID						{ 0x1d, 0x2e, 0x8c, 0x04, 0x5a, 0x74, 0x6b, 0x97, 0x3d, 0x43, 0x61, 0x36, 0xc6, 0xc0, 0x10, 0x2b }
#define MODE_INFO_UUID2						(0x6c62)
#define MODE_CHANGE_UUID2					(0x3a6c)
#define MODE_TRIGGER_UUID2					(0x0330)
#define ULT_RES_UUID2						(0xc0c6)
/* Raw data result */
#define MODE_RAW_DATA						{ 0xf3, 0x6f, 0xd9, 0xe1, 0xab, 0x76, 0xa6, 0xa0, 0xdc, 0x44, 0x1b, 0x02, 0xe8, 0xd9, 0x29, 0xe4 }
#define MODE_RAW_DATA_UUID2					(0xd9e8)

/* Daily Log */
#define DAILY_MODE_UUID						{ 0x12, 0x78, 0x93, 0x20, 0xef, 0x4e, 0xac, 0xbe, 0x47, 0x46, 0xea, 0xc1, 0xd4, 0x58, 0x6d, 0x93 }
#define DAILY_ID_SET_UUID					{ 0xa8, 0xf4, 0xd6, 0xf3, 0xa6, 0x9d, 0x4e, 0xb4, 0x11, 0x44, 0xe2, 0xc8, 0x32, 0x6b, 0x10, 0xa4 }
#define DAILY_LOG_UUID						{ 0xdf, 0xec, 0x05, 0x36, 0x8e, 0xe1, 0x78, 0x9a, 0x3b, 0x4e, 0x2b, 0x62, 0xbc, 0x9c, 0x81, 0xcd }
#define DAILY_MODE_UUID2					(0x58d4)
#define DAILY_ID_SET_UUID2					(0x6b32)
#define DAILY_LOG_UUID2						(0x9cbc)

/* Other Response */
#define RESPONSE_UUID						{ 0x4a, 0x3f, 0xe3, 0xe6, 0x34, 0x93, 0xc9, 0x92, 0xcc, 0x4b, 0x12, 0x6b, 0xd7, 0xb2, 0x90, 0xfb }
#define FW_RES_UUID							{ 0xe7, 0xc0, 0xca, 0x73, 0xd2, 0xed, 0x08, 0x82, 0x1e, 0x4c, 0x88, 0x7b, 0xbc, 0x2b, 0xbe, 0xec }
#define FW_VER_UUID							{ 0xf5, 0xb4, 0x5b, 0xd9, 0xf3, 0xa4, 0xa6, 0x98, 0x7a, 0x40, 0x6c, 0xaf, 0xde, 0x6a, 0x82, 0x37 }
#define READ_ERR_UUID						{ 0xe8, 0x87, 0x67, 0x13, 0xec, 0x5d, 0xc3, 0xa5, 0xd5, 0x4d, 0xae, 0x99, 0x5c, 0x2c, 0x79, 0x83 }
#define GET_TIME_UUID						{ 0xb0, 0xc6, 0xbe, 0x5e, 0x14, 0x9d, 0xb0, 0xa6, 0x61, 0x4c, 0xb0, 0x64, 0x9c, 0xee, 0x48, 0xe1 }
#define RESPONSE_UUID2						(0xb2d7)
#define FW_RES_UUID2						(0x2bbc)
#define FW_VER_UUID2						(0x6ade)
#define READ_ERR_UUID2						(0x2c5c)
#define GET_TIME_UUID2						(0xee9c)

/*Battery Read*/
#define BATTERY_UUID						{ 0xd1, 0x79, 0x20, 0x3f, 0x52, 0x6c, 0x42, 0xb0, 0x19, 0x40, 0xa2, 0x40, 0xdd, 0x4c, 0x4b, 0x1a }
#define BATTERY_VOL_UUID					{ 0x6d, 0xf7, 0xc4, 0x7b, 0xe8, 0x8b, 0x65, 0x9a, 0xef, 0x47, 0xf4, 0x52, 0x37, 0x64, 0x9b, 0x5b }
#define BATTERY_UUID2						(0x4cdd)
#define BATTERY_VOL_UUID2					(0x6437)

/*calibration*/
#define CALIB_UUID							{ 0x95, 0x43, 0xef, 0xa2, 0xf1, 0xd6, 0x03, 0xb0, 0xa0, 0x42, 0x87, 0x69, 0xf7, 0x20, 0x18, 0xe3 }
#define CALIB_VAL_UUID						{ 0x32, 0x5f, 0x9e, 0x5f, 0xf4, 0x04, 0x84, 0xa3, 0xd9, 0x43, 0xe6, 0xcb, 0x00, 0x17, 0xe5, 0x68 }
#define CALIB_UUID2							(0x20f7)
#define CALIB_VAL_UUID2						(0x1700)

/*Pairing*/
#define PAIR_INFO_UUID						{ 0xc2, 0x80, 0xae, 0x32, 0xff, 0x85, 0xf9, 0xbf, 0x21, 0x40, 0x4b, 0x64, 0x6b, 0xfc, 0x1b, 0x59 }
#define PAIR_CODE_UUID						{ 0x7c, 0x7f, 0x37, 0x23, 0xb4, 0x0f, 0x16, 0xa4, 0xcf, 0x4f, 0x17, 0x21, 0xa3, 0xaa, 0x5a, 0xfa }
#define PAIR_MATCH_UUID 					{ 0xe2, 0xc7, 0xf6, 0xd6, 0x11, 0x56, 0xb0, 0x95, 0x17, 0x48, 0x7d, 0xde, 0x81, 0x37, 0x7a, 0x0f }
#define PAIR_CLEAR_UUID						{ 0x44, 0x6c, 0x61, 0x90, 0x96, 0x8f, 0x69, 0xa6, 0x5a, 0x4b, 0xf7, 0xf6, 0xc0, 0x08, 0x00, 0x0b }
#define PAIR_INFO_UUID2 					(0xfc6b)
#define PAIR_CODE_UUID2						(0xaaa3)
#define PAIR_MATCH_UUID2 					(0x3781)
#define PAIR_CLEAR_UUID2					(0x08c0)

/* 2022.03.22 Add Test Command ++ */
#define TEST_CMD_SERVICE_UUID				{ 0xa5, 0xb2, 0xf7, 0xe3, 0xbf, 0x98, 0xb1, 0xd8, 0x3a, 0x73, 0x1f, 0x8d, 0x99, 0x4e, 0x7e, 0x43 }
#define TEST_CMD_UUID						{ 0x21, 0xc3, 0xab, 0x5d, 0x4d, 0x54, 0xb7, 0xab, 0xe3, 0x40, 0x5d, 0x45, 0x17, 0x90, 0xd3, 0x76 }
/* 2022.03.22 Add Test Command -- */

/* 2022.05.16 Add 角度調整 ++ */
#define ANGLE_ADJUSTMENT_SERVICE_UUID		{ 0x2c, 0x17, 0xff, 0x45, 0xb4, 0x1d, 0xf2, 0x36, 0x86, 0x8c, 0xb7, 0xb8, 0x56, 0x22, 0x9d, 0xf0 }
#define ANGLE_UUID							{ 0x56, 0xc5, 0x8d, 0xc0, 0xea, 0x5b, 0x65, 0x0f, 0x1e, 0x62, 0x5a, 0xbf, 0x83, 0x83, 0xcd, 0x31 }
/* 2022.05.16 Add 角度調整 -- */

/* 2020.12.23 Add RSSI取得テスト ++ */
#define RSSI_NOTIFY_SERVICE_UUID			{ 0x6e, 0x79, 0xa9, 0xcd, 0x11, 0x5d, 0xd3, 0x49, 0xfc, 0x81, 0xb6, 0x74, 0xf7, 0x8c, 0x23, 0x98 }
#define RSSI_NOTIFY_UUID					{ 0xc9, 0xf5, 0xee, 0xa5, 0xe1, 0xc8, 0x98, 0x14, 0xbe, 0xa2, 0xaa, 0x48, 0xb4, 0xb0, 0x1c, 0x3b }
/* 2020.12.23 Add RSSI取得テスト -- */

/* 2022.06.09 Add 分解能 ++ */
#define RESOLUTION_SERVICE_UUID				{ 0x26, 0xb4, 0x16, 0xab, 0xd7, 0x8d, 0xef, 0x9f, 0xe7, 0xb6, 0x1a, 0x18, 0xc1, 0xec, 0xe1, 0x87 }
#define RESOLUTION_UUID						{ 0xb2, 0xbd, 0xde, 0x3a, 0x89, 0x31, 0x38, 0xfa, 0x25, 0x61, 0xf0, 0x55, 0x34, 0x3e, 0x3b, 0xc2 }
/* 2022.06.09 Add 分解能 -- */

/*mode ult*/
#define END_TRIGGER_TO_TAP_STP_MODE			(15)
#define END_TRIGGER_TO_START_REAC_MODE		(10)
#define END_TRIGGER_TO_SKY_JUMP_MODE		(30)

/* Test Task No 7,8 err modify. */
#define END_TRIGGER_TO_TELEP_MODE			(20)
#define END_TRIGGER_TO_SIDE_AGI_MODE		(20)
#define END_TRIGGER_TO_RAW_MODE				(20)		/* Timeout時間を変更 30s -> 20s */

#define MAX_TRIGGER_COUNT_TAP_STP_MODE		(10)
#define MAX_TRIGGER_COUNT_START_REAC_MODE	(0)
#define MAX_TRIGGER_COUNT_SKY_JUMP_MODE		(5)
#define MAX_TRIGGER_COUNT_TELEP_MODE		(0)
#define MAX_TRIGGER_COUNT_SIDE_AGI_MODE		(0)

/* Read daily log */
#define DAILY_LOG_RETRY_COUNT				(30)

/* 2022.05.17 Add 角度補正値保持アドレス ++ */
#define ANGLE_ADJUST_ADDR					(0x0004E000)
/* 2022.05.17 Add 角度補正値保持アドレス -- */

/* 2022.06.06 Add ADV発行時間帯対応 ++ */
#define ADV_EXEC_TIME_RANGE_START			0
#define ADV_EXEC_TIME_RANGE_END				6
/* 2022.06.06 Add ADV発行時間帯対応 -- */

#ifdef __cplusplus
}
#endif

#endif
