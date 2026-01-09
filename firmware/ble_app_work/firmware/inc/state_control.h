/**
  ******************************************************************************************
  * @file    state_control.h
  * @author  k.tashiro
  * @version 1.0
  * @date    2020/09/15
  * @brief   State COntrol
  ******************************************************************************************
    Revision:
    Ver            Date             Revised by        Explanation
  ******************************************************************************************
  * 1.0            2020/09/15       k.tashiro         create new
  ******************************************************************************************
*/

#ifndef STATE_CONTROL_H_
#define STATE_CONTROL_H_

/* Includes --------------------------------------------------------------*/
#include "lib_common.h"

#ifdef __cplusplus
extern "C"{
#endif

/* Definition ------------------------------------------------------------*/
#define MAX_STATE_NUM			((uint8_t)ST_WAIT_DISCNT + 1)
#define MAX_EVT_NUM				((uint8_t)EVT_END_POINT)
#define TABLE_MAX_EVT_NUM		(EVT_BLE_CMD_PINCODE_ERASE + 1)
#define CHANGE_STATE_EVT		EVT_BLE_CMD_READ_LOG
/* Event command mask. EVT ID -> OP_mode event ID */
#define BLE_CMD_MASK			0x20
/* ble cmd mask offset */
#define BLE_CMD_OFFSET			(BLE_CMD_MASK  - EVT_RTC_INT)

/* Enum ------------------------------------------------------------------*/
/*Statement enum*/
typedef enum
{
	ST_INIT                   = 0x00,		//0x00 Init
	ST_ADV,									//0x01 ADV
	ST_PRE_DEEP_SLEEP, 						//0x02 Pre-DeepSleep
	ST_DEEP_SLEEP,							//0x03 DeepSleep
	ST_CNT_PARAM_UPDATE, 					//0x04 ConnectParam-Update
	ST_CNT_PARAM_UPDATE_ERR, 				//0x05 ConnectParam-Update Error
	ST_CNT,                					//0x06 Connect
	ST_FORCE_DISCNT,						//0x07 Force DIsconnect
	ST_CNT_PARAM_UPDATE_SL1,				//0x08 Conect Param update slave latency 1
	ST_CNT_SLAVE_LATENCY_ONE,				//0x09 Connect slave latency 1
	ST_PRE_CNT,								//0x0a Pre-Connect
	ST_WAIT_DISCNT							//0x0b Wait-Disconnect
}SHOES_STATE;

/*Event enum*/
typedef enum
{
	EVT_INIT_CMPL 						  = 0x00,
	EVT_UPDATE_PARAM,									//0x01
	EVT_UPDATING_PARAM,									//0x02
	EVT_ADV_TO,											//0x03
	EVT_NO_WALK_TO,										//0x04
	EVT_CNT_CMPL,										//0x05
	EVT_CNT_PARAM_UPDATE_CMPL,							//0x06
	EVT_CNT_PARAM_UPDATE_ERR,							//0x07
	EVT_DISCNT_SUCCESS,			 						//0x08
	EVT_FORCED_TO,					 					//0x09
	EVT_DISCNT_API_CMPL,		 						//0x0A
	EVT_UPDATE_TO,					 					//0x0B
	EVT_UPDATE_ERR_TO,									//0x0C
	EVT_FLASH_CNT_PARAM_CHANGE,							//0x0D				//change state
	EVT_FLASH_CNT_PARAM_RETURN,							//0x0E				//change state

	EVT_NOTIFY_RETRY,									//0x0F				//Not change state. retry notify event. state cnt, state sl1 process
	EVT_ACC_FIFO_INT,									//0x10				//Not change state
	
	/*below list event is offset add */
	EVT_RTC_INT,										//0x11				//change state + BLE_CMD_OFFSET
	EVT_BLE_CMD_INIT,									//0x12				//change state 
	EVT_BLE_CMD_PLAYER_INFO_SET,						//0x13				//change state 
	EVT_BLE_CMD_TIME_INFO_SET,							//0x14				//change state 
	EVT_BLE_CMD_ACCEPT_PINCODE_CHECK,					//0x15				//change state 
	EVT_BLE_CMD_PINCODE_CHECK,							//0x16				//change state 
	EVT_BLE_CMD_PINCODE_ERASE,							//0x17				//change state 
	
	EVT_BLE_CMD_READ_LOG,								//0x18				//Not change state and operation dependent
	EVT_BLE_CMD_READ_LOG_ONE,							//0x19				//Not change state and operation dependent
	/***************************************/
	//table reject
	/*BLE CMD ~ mode dependent*/	
	EVT_BLE_CMD_GET_TIME,								//0x1a				//Not change state and operation dependent
	EVT_BLE_CMD_GET_BAT,				      			//0x1b				//Not change state and operation dependent
	/* 2020.10.30 Modify EVT_ADC_READ -> EVT_COMPARATOR_TIMEOUT */
	/* 2022.01.15 Modify EVT_COMPARATOR_TIMEOUT -> EVT_ADC_READ */
	EVT_ADC_READ,										//0x1c				//Not change state and operaiton dependent
	EVT_BLE_CMD_ERASE_LOG_ONE,							//0x1d				//Not change state and operation dependent
	EVT_BLE_CMD_MODE_CHANGE,							//0x1e				//Not change state and operation dependent
	EVT_BLE_CMD_ULT_START_TRIG,							//0x1f				//Not change state and operation dependent
	EVT_BLE_CMD_ULT_END_TRIG,							//0x20				//Not change state and operation dependent
	
	EVT_BLE_ACK_TO,										//0x21				//Not change state
	EVT_ULT_START_TRIG_TO,								//0x22				//Not change state
	EVT_ULT_STOP_TRIG_TO,								//0x23				//Not change state
	EVT_FLASH_DATA_FORMAT_CREATE,						//0x24				//Not change state

	EVT_FORCE_SLEEP,									//0x25
	EVT_BLE_CMD_ULT_TRIG,								//0x26				//Not change state
	
	//flash operation event
	EVT_FLASH_OP_STATUS_CHECK,							//0x27
	EVT_INIT_CMD_START,									//0x28
	EVT_FLASH_DATA_WRITE_CMPL,							//0x29				//Not change state
	EVT_FLASH_DATA_ERASE_CMPL,							//0x2a				//Not change state

	/* 2020.12.23 Add RSSI取得テスト ++ */
	EVT_RSSI_NOTIFY,
	/* 2020.12.23 Add RSSI取得テスト -- */
	
	EVT_X_AXIS_ADV,										//0x2b
	
	/* 2022.05.13 Add Guest Mode ++ */
	EVT_BLE_CMD_GUEST_MODE,								//0x2c
	/* 2022.05.13 Add Guest Mode -- */
	
	/* 2022.05.16 Add 角度調整 ++ */
	EVT_BLE_CMD_ANGLE_ADJUST,							//0x2d
	/* 2022.05.16 Add 角度調整 -- */
	
	EVT_END_POINT										//0x2b -> 0x2c -> 0x2d -> 0x2e
}STATE_EVENT;

/* Struct ----------------------------------------------------------------*/
typedef struct _data_info	//
{
	uint8_t mode;
	uint8_t dateTime[12];
	uint8_t flash_flg;
	uint8_t age;
	uint8_t sex;
	uint8_t weight;
} DATA_INFO, *PDATA_INFO;

typedef struct _daily_ult_set
{
	uint8_t mode;
	uint16_t mode_timeout;
	uint16_t clear_sid;
}MODE_SET, *PMODE_SET;

typedef struct _mode_trigger
{
	uint8_t trigger;
}MODE_TRIGGER, *PMODE_TRIGGER;

typedef struct _daly_ult_result//
{
	union _ult_result
	{
		uint8_t count[10];
		int16_t time[5];
	}ULT_RESULT;
	uint8_t mode;
} MODE_RESULT, *PMODE_RESULT;

typedef struct _daily_id//
{
	uint16_t       sid;
	uint8_t       mode;
}DAILY_ID, *PDAILY_ID;

typedef struct _daily_log_data//
{
	uint8_t		dateTime[8];
	uint16_t	walk_count;
	uint16_t	run_count;
	uint16_t	dash_count;
	uint16_t	last_sid;
}DAILY_LOG, *PDAILY_LOG;

typedef struct _response_fw //
{
	uint8_t  res_fw;
}RES_FW, *PRES_FW;

typedef struct _fw_version //
{
	uint8_t  fw_ver[3];
}FW_VER, *PFW_VER;

typedef struct _err_code //
{
	uint8_t err_code;
}ERR_CODE, *PERR_CODE;

typedef struct _time_info//
{
	uint8_t  dateTime[12];
}TIME_INFO, *PTIME_INFO;

typedef struct _bat_info //
{
	uint16_t batValue;
	uint16_t getCount;
}BAT_INFO, *PBAT_INFO;

typedef struct _pin_code //
{
	uint8_t  pinCode[16];
}PIN_CODE, *PPIN_CODE;

typedef struct _pin_match_res //
{
	uint8_t pin_match_result;
}PIN_MATCH_RES, *PPIN_MATCH_RES;

typedef struct _pin_code_clear //
{
	uint8_t pinCodeClear;
}PIN_CODE_CLEAR, *PPIN_CODE_CLEAR;

/* 2020.11.17 Add X-Axis SRC追加++ */
typedef struct _x_axis_int_src
{
	uint8_t x_int_src;
	uint8_t x_int_data;
} X_AXIS_INT_SRC, *PX_AXIS_INT_SRC; 
/* 2020.11.17 Add X-Axis SRC追加++ */

/* 2022.05.16 Add 角度調整 ++ */
typedef struct _angle_adjust_info
{
	uint8_t state;
	int16_t x_angle;
	int16_t y_angle;
} ANGLE_ADJUST_INFO, *PANGLE_ADJUST_INFO;
/* 2022.05.16 Add 角度調整 -- */

/* 2020.12.23 Add RSSI取得テスト ++ */
typedef struct _ble_rssi_info
{
	int16_t rssi;
} BLE_RSSI_INFO, *PBLE_RSSI_INFO;
/* 2020.12.23 Add RSSI取得テスト -- */

// event_id + data
typedef struct _ev_id
{
	union _data
	{
		DATA_INFO			dataInfo;
		MODE_SET			modeSet;
		MODE_TRIGGER		modeTrigger;
		MODE_RESULT			modeResult;
		DAILY_ID			dailyId;
		DAILY_LOG			dailyLog;
		RES_FW				response;
		FW_VER				fwVersion;
		ERR_CODE			errCode;
		TIME_INFO			timeInfo;
		BAT_INFO			bat;
		PIN_CODE			pinCode;
		PIN_MATCH_RES		pinMatchRes;
		PIN_CODE_CLEAR		pincodeClear;
		X_AXIS_INT_SRC		x_axis_info;		/* 2020.11.17 Add X-Axis割り込みsource追加 */
		ANGLE_ADJUST_INFO	angle_adjust_info;	/* 2022.05.16 Add 角度調整 */
		BLE_RSSI_INFO		notify_rssi_info;	/* 2020.12.23 Add RSSI取得テスト */
	}DATA;
	EVT_ID	evt_id;
	uint8_t dailyLogSendSt;
}EVT_ST, *PEVT_ST;

// statement_id + event_function
typedef struct _statement_struct
{
	uint32_t	(*exefunc[MAX_EVT_NUM])(PEVT_ST);
	ST_ID		st_id;
}STATE_ST, *PSTATE_ST;

/**
 * @brief Change State
 * @param pCurrState Current State Information
 * @param pCurrEvt Current Event Information
 * @retval None
 */
//void shoes_changeState(STATE_ST *pCurrState, PEVT_ST pCurrEvt);
void ShoesChangeState(STATE_ST *pCurrState, PEVT_ST pCurrEvt);

/**
 * @brief Initialize State
 * @param pCurrState Current State Information
 * @retval None
 */
//void initState(STATE_ST *pCurrState);
void InitState(STATE_ST *pCurrState);

/**
 * @brief Not Action Process
 * @param pEvent Event Information
 * @retval 0 Success
 */
//uint32_t utc_notAction(PEVT_ST pEvent);
uint32_t NonAction(PEVT_ST pEvent);

/**
 * @brief Force Sleep Process
 * @param pEvent Event Information
 * @retval 0 Success
 */
//uint32_t utc_force_sleep(PEVT_ST pEvent);
uint32_t ForceSleep(PEVT_ST pEvent);

/**
 * @brief Initialize State No Advertise
 * @param pCurrState Current State Information
 * @retval None
 */
//void initState_noAdv(STATE_ST *pCurrState);
void InitStateNoAdv(STATE_ST *pCurrState);

#ifdef __cplusplus
}
#endif
#endif
