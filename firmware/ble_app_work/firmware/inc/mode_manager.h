/**
  ******************************************************************************************
  * @file    mode_manager.h
  * @author  k.tashiro
  * @version 1.0
  * @date    2020/09/24
  * @brief   Mode Manager (Daily Mode and Ultimate Modeを管理する)
  ******************************************************************************************
    Revision:
    Ver            Date             Revised by        Explanation
  ******************************************************************************************
  * 1.0            2020/09/24       k.tashiro         create new
  ******************************************************************************************
*/

#ifndef MODE_MANAGER_H_
#define MODE_MANAGER_H_

/* Includes --------------------------------------------------------------*/
#include "stdint.h"
#include "nrf_ble_gatt.h"
#include "state_control.h"
#include "lib_ex_rtc.h"
#include "lib_common.h"
#include "daily_log.h"
#include "walk_algo.h"
#include "definition.h"
#include "AccAngle.h"

#ifdef __cplusplus
extern "C"{
#endif

/* Enum ------------------------------------------------------------------*/
//shoes sensor operation mode enum
typedef enum
{
	DAILY_MODE			= 0x00,
	TAP_STP_MODE		= 0x01,
	RES_ONE				= 0x02,		//Not use
	START_REAC_MODE		= 0x03,
	RES_TWO				= 0x04,		//Not use
	SKY_JUMP_MODE		= 0x05,
	RES_THREE			= 0x06,		//Not use
	TELEP_MODE			= 0x07,
	SIDE_AGI_MODE		= 0x08,
	RES_FOUR			= 0x09,
	CALIB_MEAS_MODE		= 0x0a,		//add acc calib meas mode
	CALIB_EXE_MODE		= 0x0b,		//add acc calib exe mode
	RAW_MODE			= 0x0c,		//add acc raw data mode
	RAW_MODE_GYRO_TEMP	= 0x0d,		/* 2020.10.28 Add Raw Data(Gyro/Temp Mode) */
	RAW_MODE_BOTH		= 0x0e,		/* 2020.10.28 Add Raw Data(ACC/Gyro/Temp Mode) */
}OP_MODE;

typedef enum
{
	//0x00 ~ 0x08                 flash op id check
	BLE_RTC_INT                   = 0x00,
	BLE_CMD_INIT,                 			//0x01
	BLE_CMD_PLAYER_INFO_SET,				//0x02
	BLE_CMD_TIME_INFO_SET,					//0x03
	BLE_CMD_ACCEPT_PINCODE_CHECK,			//0x04
	BLE_CMD_PINCODE_CHECK,					//0x05
	BLE_CMD_PINCODE_ERASE,					//0x06
	BLE_CMD_READ_LOG,						//0x07
	BLE_CMD_READ_LOG_ONE,					//0x08
	
	BLE_CMD_GET_TIME,						//0x09
	BLE_CMD_GET_BAT,						//0x0a
	BLE_ADC_READ,							//0x0b
	BLE_CMD_ERASE_LOG_ONE,					//0x0c
	BLE_CMD_MODE_CHANGE,					//0x0d
	BLE_CMD_ULT_START_TRIG,					//0x0e
	BLE_CMD_ULT_END_TRIG,					//0x0f
	BLE_ACK_TO,								//0x10
	BLE_ULT_START_TRIG_TO,					//0x11
	BLE_ULT_STOP_TRIG_TO,					//0x12
	BLE_FLASH_DATA_FORMAT_CREATE,			//0x13		Not change state
	BLE_FORCE_SLEEP,						//0x14
	BLE_CMD_ULT_TRIG,						//0x15
	BLE_FLASH_OP_STATUS_CHECK,				//0x16
	BLE_INIT_CMD_START,						//0x17	
	BLE_FLASH_DATA_WRITE_CMPL,				//0x18		Not change state
	BLE_FLASH_DATA_ERASE_CMPL,				//0x19		Not change state
	/* 2020.12.23 Add RSSI取得テスト ++ */
	BLE_GET_RSSI,
	/* 2020.12.23 Add RSSI取得テスト -- */
	
	BLE_CMD_END_POINT
}BLE_CMD_EVENT;

/* Struct ----------------------------------------------------------------*/
typedef struct _daily_box
{
	uint8_t Data[8];
	int16_t walk;
	int16_t  run;
	int16_t dash;
	int16_t sid;
}DAILY_BOX,*PDAILY_BOX;

/*calib acc sensor value*/
typedef struct _calib_ack
{
	uint16_t err_code;
	int16_t x_offset;
	int16_t y_offset;
	int16_t z_offset;
}CALIB_ACK,*PCALIBACK;

/* Raw data send box */
typedef struct _raw_data
{
	uint16_t	sid;				/* SID */
	int16_t		acc_x_data;			/* ACC X-Axis Data and Temp Data */
	int16_t		acc_y_data;			/* ACC Y-Axis Data */
	int16_t		acc_z_data;			/* ACC X-Axis Data */
	int16_t		gyro_x_data;		/* Gyro X-Axis Data */
	int16_t		gyro_y_data;		/* Gyro Y-Axis Data */
	int16_t		gyro_z_data;		/* Gyro Z-Axis Data */
	int16_t		temperature;		/* Temperature */
} RAW_DATA, *PRAW_DATA;

/*******************/
//Operation mode struct
typedef struct op_mode_st
{
	uint32_t	(*const exefunc[BLE_CMD_END_POINT])(EVT_ST *evt);
	uint16_t	ult_move_time;
	uint16_t	first_sid;
	union __resultdata{
		uint8_t		count[ULT_DATA_SIZE];
		int16_t		time[ULT_DATA_SIZE / 2];
		RAW_DATA	raw_acc;
	} RESULTDATA;
	const uint8_t	max_trigger_count;
	uint8_t			running;
	uint16_t		timeout;
	uint8_t			currTrigger;
	int8_t			trigger_count;
	const OP_MODE	currOP_id;
	bool			residual_count;
}OP_ST, *POP_ST;

/* Function prototypes ---------------------------------------------------*/
/**
 * @brief Initialize Mode Manager Operation
 * @param None
 * @retval None
 */
void InitOpModeMgr( void );

/**
 * @brief Get Mode Timeout Value
 * @param pTimeout Timeout Value
 * @retval None
 */
void GetModeTimeoutValue(uint16_t *pTimeout);

/**
 * @brief Force Change Daily Mode
 * @param None
 * @retval None
 */
//void force_change_dailyMode(void);
void ForceChangeDailyMode(void);

/**
 * @brief BLE Wrapper Function
 * @param pEvent Event Information
 * @retval None
 */
//uint32_t utc_bleWrapperfunc(PEVT_ST pEvent);
uint32_t BleWrapperFunc(PEVT_ST pEvent);

/**
 * @brief pairing pincode check flag clear.
 * @param None
 * @retval None
 */
//void pinCodeCheckflagClear(void);
void PinCodeCheckFlagClear(void);

/**
 * @brief pairing pincode check flag set.
 * @param None
 * @retval None
 */
//void pinCodeCheckflagSet(void);
void PinCodeCheckFlagSet(void);

/**
 * @brief Get access pin code flag
 * @param None
 * @retval true 
 * @retval false
 */
//bool accessPinCodeflagGet(void);
bool AccessPinCodeFlagGet(void);

/**
 * @brief RTC interrupt, daily log write. 
 * @param pEvent Event Information
 * @retval 0 Success
 */
//uint32_t utc_intRTC_NON_BLE(PEVT_ST pEvent);
uint32_t IntRtcNonBle(PEVT_ST pEvent);

/**
 * @brief Binary Data Change RTC Time
 * @param pDateTime RTCから取得した時刻
 * @param pResult 変換した結果
 * @retval None
 */
//void timeDataFormatReverse(DATE_TIME *pDateTime, uint8_t *pResult);
void TimeDataFormatReverse(DATE_TIME *pDateTime, uint8_t *pResult);
	
/**
 * @brief Run Algorithm
 * @param pEvent Event Information
 * @retval 0 Success
 */
uint32_t RunAlgo(PEVT_ST pEvent);

/**
 * @brief Algorithm Data Reset
 * @param None
 * @retval None
 */
//void algoDataReset(void);
void AlgoDataReset(void);

/**
 * @brief statement, other than connect, evt, EVT_FIFO_INT,
 *        Run Algorithm Pre Deep Sleep
 * @param pEvent Event Information
 * @retval 0 Success
 */
//uint32_t RunAlgo_PreDeepSleep(PEVT_ST pEvent);
uint32_t RunAlgoPreDeepSleep(PEVT_ST pEvent);

/**
 * @brief Run Algorithm ADV
 * @param pEvent Event Information
 * @retval 0 Success
 */
//uint32_t RunAlgo_ADV(PEVT_ST pEvent);
uint32_t RunAlgoAdv(PEVT_ST pEvent);

/**
 * @brief Clear Walk Timeout
 * @param None
 * @retval None
 */
//void walkTimeOutClear(void);
void WalkTimeOutClear(void);

/**
 * @brief set daily log function
 * @param dateTime Date TIme
 * @retval 0 Success
 */
//uint32_t set_daily_log(DATE_TIME *dateTime);
uint32_t SetDailyLog(DATE_TIME *dateTime);

/**
 * @brief Get Operation Mode
 * @param pOpMode Operation Mode Code
 * @retval None
 */
//void getOpMode(OP_MODE *pOpMode);
void GetOpMode(OP_MODE *pOpMode);

/**
 * @brief Pre Connect Parameter Update Reject
 * @param pEvent Event Information
 * @retval 0 Success
 */
//uint32_t utc_pre_cnt_paramUpdate_reject(PEVT_ST pEvent);
uint32_t PreCntParamUpdateReject(PEVT_ST pEvent);

/**
 * @brief set global var to offset value
 * @param xoffset X軸オフセット
 * @param yoffset Y軸オフセット
 * @param zoffset Z軸オフセット
 * @retval None
 */
void SetAccOffset(int16_t xoffset, int16_t yoffset, int16_t zoffset);

/**
 * @brief set acc offset valid
 * @param valid offset valid
 * @retval None
 */
void SetAccOffsetValid(uint8_t valid);

/**
 * @brief Get global var to offset value
 * @param xoffset X軸オフセット
 * @param yoffset Y軸オフセット
 * @param zoffset Z軸オフセット
 * @retval None
 */
void GetAccOffset(uint8_t *valid, int16_t *xoffset, int16_t *yoffset, int16_t *zoffset);

/**
 * @brief Y Axis ADV
 * @param pEvent Event Information
 * @retval 0 Success
 */
uint32_t YAxisAdv(PEVT_ST pEvent);

/**
 * @brief Get Sensor Data FIFO Clear SID
 * @param None
 * @retval sid Clear SID
 */
uint16_t GetClearSid( void );

/**
 * @brief ADV Timeout時のADV判定処理リセット
 * @param pEvent Event Information
 * @retval 0 Success
 */
uint32_t AdvTimeoutClearInt(PEVT_ST pEvent);

/**
 * @brief Guest Modeの状態を取得
 * @param None
 * @retval ture Guest Mode ON
 * @retval false Guest Mode OFF
 */
bool GetGuestModeState( void );

/**
 * @brief Guest Modeの状態を取得
 * @param guest_mode_state Guest Modeの状態
 * @retval None
 */
void SetGuestModeState( bool guest_mode_state );

/**
 * @brief 角度調整情報設定
 * @param angle_info 角度調整情報
 * @retval None
 */
void SetAngleAdjustInfo( ACC_ANGLE *angle_info );

/**
 * @brief 角度調整情報取得
 * @param angle_info 角度調整情報
 * @retval None
 */
void GetAngleAdjustInfo( ACC_ANGLE *angle_info );

/**
 * @brief 角度調整状態変更
 * @param state 角度調整状態
 * @retval None
 */
void ChangeAngleAdjustState( uint8_t state );

/**
 * @brief 角度調整状態取得
 * @param state 角度調整状態
 * @retval None
 */
void GetAngleAdjustState( uint8_t *state );

/**
 * @brief RTC Interruptを処理するとどうかを検証し決定する
 * @param pEvent Event Information
 * @retval 0 Success
 */
uint32_t ValidateRtcInterrupt( PEVT_ST pEvent );

/*2022.08.09 kono test random id*/
int random_index(void);
void SetRndId(int id);
int GetRndId(void);

#ifdef __cplusplus
}
#endif

#endif

