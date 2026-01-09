/**
  ******************************************************************************************
  * @file    mode_manager.c
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

/* Includes --------------------------------------------------------------*/
#include "nrf_fstorage.h"
#include "ble_gatts.h"
#include "lib_common.h"
#include "state_control.h"
#include "ble_manager.h"
#include "ble_definition.h"
#include "mode_manager.h"
#include "lateral_wakeup.h"
#include "lib_ram_retain.h"
#include "lib_fifo.h"
#include "lib_timer.h"
#include "lib_bat.h"
#include "lib_ex_rtc.h"
#include "algo_acc.h"
#include "Walk_Algo.h"
#include "daily_log.h"
#include "definition.h"
#include "nrf_fstorage.h"
#include "nrf_fstorage_sd.h"
#include "flash_operation.h"
#include "lib_trace_log.h"
#include "nrf_delay.h"
#include "lib_angle_flash.h"

#include "time.h"

/* Definition ------------------------------------------------------------*/
#define OFFSET_P_TO_P (int32_t)300

#define LOG_LEVEL_MODE_MGR 5
//#define LOG_LEVEL_MODE_MGR LOG_DEBUG
//#define LOG_LEVEL_MODE_MGR LOG_INFO

#define RAW_MODE_MASK			(0x03)		/* 2020.10.28 Add RawMode Mask */
#define RAW_BLE_RETRY			650			/* 2020.11.02 Add BLE Retry Count */
#define RAW_BLE_WAIT_INTERVAL	100			/* 2020.11.02 Add BLE Retry Interval */

#define EVENT_ID_NONE			0			/* 2022.01.26 Add Event ID None */

//#define TEST_FIFO_COUNT_NOTIFY			/* 2020.12.08 Add Test FIFO Count Notify */
//#define TEST_BLE_POWER_CHANGE				/* 2020.12.09 Add Test BLE Power Change */

/* 2022.06.07 Add RSSI取得対応 ++ */
#define BLE_FORCE_DISCONNECT_COUNT		3
/* 2022.06.07 Add RSSI取得対応 -- */

/* Private variables -----------------------------------------------------*/
//paring check flag
volatile bool gPairingPinCodeCheck = false;
//access pin code flag
volatile bool gAccessPinCodeFlag = false;

volatile POP_ST gpCurrOPmode;
//algo sid
volatile uint16_t gAlgoSid;
// Temperature sid
volatile uint16_t g_temp_sid;
void* gpResult;
//algorithm daiy mode result.
volatile DAILYCOUNT gDailyRes;
//algorithm ult mode result.  tap stamp, side agility, teleportation. start reaction.
volatile ALTCOUNT gUltCountRes;
//mode sky jump result
volatile SKYJUMPCOUNT gUltSkyJumpRes;
//no walk timer counter
volatile uint16_t gNo_walk_sec = 0;
volatile uint8_t gDebugDailyAccDisplayON_OFF = 0;

/*global var*/
volatile int16_t gAccXoffset = 0;
volatile int16_t gAccYoffset = 0;
volatile int16_t gAccZoffset = 0;

/*acc calibration global var*/

static int16_t gXmin = 32767;
static int16_t gXmax = -32768;
static int16_t gYmin = 32767;
static int16_t gYmax = -32768;
static int16_t gZmin = 32767;
static int16_t gZmax = -32768;

volatile uint8_t gOffset_valid = 0;
volatile int32_t gXOffset = DEFAULT_SENSOR_OFFSET;
volatile int32_t gYOffset = DEFAULT_SENSOR_OFFSET;
volatile int32_t gZOffset = DEFAULT_SENSOR_OFFSET;
static int32_t gXsum = 0;
static int32_t gYsum = 0;
static int32_t gZsum = 0;
static int16_t gCalibCount = 0;

volatile uint8_t gRawData[RAW_DATA_SIZE];
volatile RAW_DATA gRawBox;
/* 2020.10.28 Add Raw Mode追加 ++ */
volatile static uint16_t g_current_raw_mode = RAW_MODE;
/* 2020.10.28 Add Raw Mode追加 -- */

/* 2020.12.08 Add BLE再送処理 ++ */
volatile static bool g_notify_failed = false;
volatile static uint16_t g_fifo_clear_sid = 0;
/* 2020.12.08 Add BLE再送処理 -- */

/* 2022.01.26 Add RawData送信中のRTC処理 ++ */
volatile static EVT_ID g_rtc_int_id = EVENT_ID_NONE;
/* 2022.01.26 Add RawData送信中のRTC処理 -- */

/* 2022.03.17 Add 送信SIDの初期化処理追加 ++ */
volatile static uint16_t g_send_sid_base = 0;
volatile static bool g_send_sid_init = false;
/* 2022.03.17 Add 送信SIDの初期化処理追加 -- */

/* 2022.05.13 Add Guest Mode Flag追加 ++ */
volatile static bool g_guest_mode = false;
/* 2022.05.13 Add Guest Mode Flag追加 -- */

/* 2022.05.19 Add 角度調整情報 ++ */
static ACC_ANGLE g_angle_info = {0};
static uint8_t g_angle_adjust_state = ANGLE_ADJUST_DISABLE;
/* 2022.05.19 Add 角度調整情報 -- */

/* 2022.06.07 Add RSSI取得対応 ++ */
static uint16_t g_force_disconnect_count = 0;
/* 2022.06.07 Add RSSI取得対応 -- */

/*2022.08.09 random index*/
static int g_random_id = 0;

/*2022.08.09 random index*/

/* Private function prototypes -------------------------------------------*/
static uint32_t operation_mode_change(PEVT_ST pEvent);
static uint32_t get_battery_value_daily(PEVT_ST pEvent);
static uint32_t get_rtc_time_daily(PEVT_ST pEvent);
static uint32_t recieve_ult_start_trigger(PEVT_ST pEvent);
static uint32_t recieve_ult_trriger(PEVT_ST pEvent);
static uint32_t reply_ack(PEVT_ST pEvent);
static uint32_t start_trigger_timeout_func(PEVT_ST pEvent);
static uint32_t end_trigger_timeout_func(PEVT_ST pEvent);
static uint32_t erase_log_one(PEVT_ST pEvent);
static void init_offset_cal_global_var(void);
static void clear_tmp_acc_offset(void);
static uint8_t bcc_create(uint8_t *str, uint32_t len);	//raw data bcc
/* 2022.01.26 Add RawData送信中のRTC処理 ++ */
static uint32_t raw_data_sending_rtc_proc( PEVT_ST pEvent );
/* 2022.01.26 Add RawData送信中のRTC処理 -- */
/* 2020.12.23 Add RSSI取得テスト ++ */
static uint32_t get_rssi_notify( PEVT_ST pEvent );
/* 2020.12.23 Add RSSI取得テスト -- */


/*
 * Daily ModeとUltimate Modeで実行する関数定義と使用する情報をまとめたテーブル
 * フォーマット
 *  { 
 *    BLE_RTC_INT,					BLE_CMD_INIT,				BLE_CMD_PLAYER_INFO_SET,	BLE_CMD_TIME_INFO_SET,			BLE_CMD_ACCEPT_PINCODE_CHECK,	BLE_CMD_PINCODE_CHECK,	BLE_CMD_PINCODE_ERASE,		BLE_CMD_READ_LOG,
 *    BLE_CMD_READ_LOG_ONE,			BLE_CMD_GET_TIME,			BLE_CMD_GET_BAT,			BLE_ADC_READ,					BLE_CMD_ERASE_LOG_ONE,			BLE_CMD_MODE_CHANGE,	BLE_CMD_ULT_START_TRIG, 	BLE_CMD_ULT_END_TRIG,
 *    BLE_ACK_TO,					BLE_ULT_START_TRIG_TO,		BLE_ULT_STOP_TRIG_TO,		BLE_FLASH_DATA_FORMAT_CREATE,	BLE_FORCE_SLEEP,				BLE_CMD_ULT_TRIG,		BLE_FLASH_OP_STATUS_CHECK,	BLE_INIT_CMD_START,
 *    BLE_FLASH_DATA_WRITE_CMPL,	BLE_FLASH_DATA_ERASE_CMPL,	BLE_GET_RSSI
 *  },
 *  ult_move_time, first_sid, RESULTDATA,
 *  max_trigger_count, running, timeout,
 *  currTrigger, trigger_count, currOP_id, residual_count
 */

OP_ST side_op = {
	{
		RepushEvent,	NonAction,					NonAction,					NonAction,				NonAction,	NonAction,				NonAction,					RepushEvent,
		RepushEvent,	RepushEvent,				RepushEvent,				operation_mode_change,	NonAction,	NonAction,				recieve_ult_start_trigger,	NonAction,
		reply_ack,		start_trigger_timeout_func,	end_trigger_timeout_func,	NonAction,				NonAction,	recieve_ult_trriger,	NonAction,					NonAction,
		NonAction,		NonAction,					NonAction
	},
	0, 0, {0},
	MAX_TRIGGER_COUNT_SIDE_AGI_MODE, 0, END_TRIGGER_TO_SIDE_AGI_MODE,
	0, 0, SIDE_AGI_MODE, false
};

OP_ST telep_op = {
	{
		RepushEvent,	NonAction,					NonAction,					NonAction,				NonAction,	NonAction,				NonAction,					RepushEvent,
		RepushEvent,	RepushEvent,				RepushEvent,				operation_mode_change,	NonAction,	NonAction,				recieve_ult_start_trigger,	NonAction,
		reply_ack,		start_trigger_timeout_func,	end_trigger_timeout_func,	NonAction,				NonAction,	recieve_ult_trriger,	NonAction,					NonAction,
		NonAction,		NonAction,					NonAction
	},
	0, 0, {0},
	MAX_TRIGGER_COUNT_TELEP_MODE, 0, END_TRIGGER_TO_TELEP_MODE,
	0, 0, TELEP_MODE, false
};

OP_ST skyjump_op = {
	{
		RepushEvent,	NonAction,					NonAction,					NonAction,				NonAction,	NonAction,				NonAction,					RepushEvent,
		RepushEvent,	RepushEvent,				RepushEvent,				operation_mode_change,	NonAction,	NonAction,				recieve_ult_start_trigger,	NonAction,
		reply_ack,		start_trigger_timeout_func,	end_trigger_timeout_func,	NonAction,				NonAction,	recieve_ult_trriger,	NonAction,					NonAction,
		NonAction,		NonAction,					NonAction
	},
	0, 0, {0},
	MAX_TRIGGER_COUNT_SKY_JUMP_MODE, 0, END_TRIGGER_TO_SKY_JUMP_MODE,
	0, 0, SKY_JUMP_MODE, false
};

OP_ST startReac_op = {
	{
		RepushEvent,	NonAction,					NonAction,					NonAction,				NonAction,	NonAction,				NonAction,					RepushEvent,
		RepushEvent,	RepushEvent,				RepushEvent,				operation_mode_change,	NonAction,	NonAction,				recieve_ult_start_trigger,	NonAction,
		reply_ack,		start_trigger_timeout_func,	end_trigger_timeout_func,	NonAction,				NonAction,	recieve_ult_trriger,	NonAction,					NonAction,
		NonAction,		NonAction,					NonAction
	},
	0, 0, {0},
	MAX_TRIGGER_COUNT_START_REAC_MODE, 0, END_TRIGGER_TO_START_REAC_MODE,
	0, 0, START_REAC_MODE, false
};

OP_ST tap_stp_op = {
	{
		RepushEvent,	NonAction,					NonAction,					NonAction,				NonAction,	NonAction,				NonAction,					RepushEvent, 
		RepushEvent,	RepushEvent,				RepushEvent,				operation_mode_change,	NonAction,	NonAction,				recieve_ult_start_trigger,	NonAction,
		reply_ack,		start_trigger_timeout_func,	end_trigger_timeout_func,	NonAction,				NonAction,	recieve_ult_trriger,	NonAction,					NonAction,
		NonAction,		NonAction,					NonAction
	},
	0, 0, {0},
	MAX_TRIGGER_COUNT_TAP_STP_MODE, 0, END_TRIGGER_TO_TAP_STP_MODE,
	0, 0, TAP_STP_MODE, false
};

OP_ST daily_op = {
	{
		FlashOpIdCheck,		FlashOpIdCheck,			FlashOpIdCheck,				FlashOpIdCheck,				FlashOpIdCheck,		FlashOpIdCheck,			FlashOpIdCheck,		FlashOpIdCheck,
		FlashOpIdCheck,		get_rtc_time_daily,		get_battery_value_daily,	ReadAuthorizeBatteryInfo,	erase_log_one,		operation_mode_change,	NonAction,			NonAction,
		NonAction,			NonAction,				NonAction,					NonAction,					NonAction,			NonAction,				FlashWrapperfunc,	FlashWrapperfunc,
		FlashWrapperfunc,	FlashWrapperfunc,		get_rssi_notify
	},
	0, 0, {0},
	0, 0, 0,
	0, 0, DAILY_MODE, false
};

OP_ST res_one_op = {
	{
		NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,
		NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,
		NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,
		NonAction,	NonAction,	NonAction
	},
	0, 0, {0},
	0, 0, 0,
	0, 0, RES_ONE, false
};

OP_ST res_two_op = {
	{
		NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,
		NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,
		NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,
		NonAction,	NonAction,	NonAction
	},
	0, 0, {0},
	0, 0, 0,
	0, 0, RES_TWO, false
};

OP_ST res_three_op = {
	{
		NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,
		NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,
		NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,
		NonAction,	NonAction,	NonAction
	},
	0, 0, {0},
	0, 0, 0,
	0, 0, RES_THREE, false
};

OP_ST res_four_op = {
	{
		NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,
		NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,
		NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,
		NonAction,	NonAction,	NonAction
	},
	0, 0, {0},
	0, 0, 0,
	0, 0, RES_FOUR, false
};

//add calib_meas mode
OP_ST calib_meas_op = {
	{
		RepushEvent,	NonAction,					NonAction,					NonAction,				NonAction,	NonAction,				NonAction,					RepushEvent,
		RepushEvent,	RepushEvent,				NonAction,					operation_mode_change,	NonAction,	NonAction,				recieve_ult_start_trigger,	NonAction,
		reply_ack,		start_trigger_timeout_func,	end_trigger_timeout_func,	NonAction,				NonAction,	recieve_ult_trriger,	NonAction,					NonAction,
		NonAction,		NonAction,					NonAction
	},
	0, 0, {0},
	0, 0, 0,
	0, 0, CALIB_MEAS_MODE, false
};


OP_ST calib_exe_op = {
	{
		NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,
		NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,
		NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,	NonAction,
		NonAction,	NonAction,	NonAction
	},
	0, 0, {0},
	0, 0, 0,
	0, 0, CALIB_EXE_MODE, false
};

OP_ST raw_op = {
	{
		raw_data_sending_rtc_proc,	NonAction,					NonAction,					NonAction,				NonAction,	NonAction,				NonAction,					RepushEvent,
		RepushEvent,				RepushEvent,				RepushEvent,				operation_mode_change,	NonAction,	NonAction,				recieve_ult_start_trigger,	NonAction,
		reply_ack,					start_trigger_timeout_func,	end_trigger_timeout_func,	NonAction,				NonAction,	recieve_ult_trriger,	NonAction,					NonAction,
		NonAction,					NonAction,					NonAction
	},
	0, 0, {0},
	0, 0, END_TRIGGER_TO_RAW_MODE,
	0, 0, RAW_MODE, false
};

/* Mode Manager Mode List
 * フォーマット
 *  DAILY_MODE,		TAP_STP_MODE,	RES_ONE,			START_REAC_MODE,
 *  RES_TWO,		SKY_JUMP_MODE,	RES_THREE,			TELEP_MODE
 *  SIDE_AGI_MODE,	RES_FOUR, 		CALIB_MEAS_MODE,	CALIB_EXE_MODE,
 *  RAW_MODE
 */
volatile const POP_ST gListCurrOPmode[RAW_MODE + 1] = {
	(POP_ST)&daily_op,		(POP_ST)&tap_stp_op,	(POP_ST)&res_one_op,	(POP_ST)&startReac_op,
	(POP_ST)&res_two_op,	(POP_ST)&skyjump_op,	(POP_ST)&res_three_op,	(POP_ST)&telep_op,
	(POP_ST)&side_op,		(POP_ST)&res_four_op,	(POP_ST)&calib_meas_op,	(POP_ST)&calib_exe_op,
	(POP_ST)&raw_op
};

/**
 * @brief BLE cmd function. operation mode
 * @param pEvent Event Information
 * @retval 0 Success
 */
static uint32_t ble_mode_func(PEVT_ST pEvent)
{
	uint8_t eventId;
	uint8_t bleEvtId;
	
	eventId = pEvent->evt_id;
	bleEvtId = (eventId + BLE_CMD_OFFSET) & (~BLE_CMD_MASK);
	
	DEBUG_LOG( LOG_INFO, "ble event id: 0x%x, %d", bleEvtId, bleEvtId );

	if(eventId == EVT_BLE_CMD_INIT)
	{
		DEBUG_LOG(LOG_DEBUG,"mode func init mode id 0x%x, flash flag 0x%x",pEvent->DATA.dataInfo.mode, pEvent->DATA.dataInfo.flash_flg);
	}
	
	if(eventId == EVT_BLE_CMD_ACCEPT_PINCODE_CHECK)
	{
		DEBUG_LOG(LOG_DEBUG,"accept pin code check non pair ble mode is 0x%x",gpCurrOPmode->currOP_id);
	}
	
	if(pEvent->evt_id == EVT_FLASH_OP_STATUS_CHECK)
	{
		DEBUG_LOG(LOG_DEBUG,"ble mode func flash op statuc check. ble evt id 0x%x",bleEvtId);
	}
	
	//Current Operation function start.
	gpCurrOPmode->exefunc[bleEvtId](pEvent);
	
	return 0;
}

/**
 * @brief BLE acces pin code check flag
 * @param ble_evt_cmd BLE Command Event
 * @param pEvent Event Information
 * @retval None
 */
static void access_pincode_flag_check(BLE_CMD_EVENT ble_evt_cmd, PEVT_ST pEvent)
{
	if(ble_evt_cmd == BLE_CMD_ACCEPT_PINCODE_CHECK)
	{
		DEBUG_LOG(LOG_DEBUG,"accept pin");
		gAccessPinCodeFlag = true;
	}
	else if( ( ble_evt_cmd == BLE_CMD_PINCODE_CHECK ) || ( ble_evt_cmd == BLE_CMD_PINCODE_ERASE ) ||
			 ( ble_evt_cmd == BLE_FLASH_OP_STATUS_CHECK ) || ( ble_evt_cmd == BLE_INIT_CMD_START ) ||
			 ( ble_evt_cmd == BLE_FLASH_DATA_WRITE_CMPL ) || ( ble_evt_cmd == BLE_FLASH_DATA_ERASE_CMPL )||
			 ( ( ble_evt_cmd == BLE_CMD_MODE_CHANGE ) && ( ( pEvent->DATA.modeSet.mode == CALIB_MEAS_MODE ) ||
			 ( pEvent->DATA.modeSet.mode == CALIB_EXE_MODE) ) ) )
	{
		//if not calibration mode, tmp offset value is reset. 
		DEBUG_LOG(LOG_DEBUG,"nop 0x%x",ble_evt_cmd);
#if 0
		if((ble_evt_cmd == BLE_FLASH_OP_STATUS_CHECK) || ((ble_evt_cmd == BLE_CMD_MODE_CHANGE) && ((pEvent->DATA.modeSet.mode == CALIB_MEAS_MODE)||(pEvent->DATA.modeSet.mode == CALIB_EXE_MODE))))
		{
			DEBUG_LOG(LOG_DEBUG,"clear1");
			__NOP();
		}
		else
		{
			DEBUG_LOG(LOG_DEBUG,"clear2");
			clear_tmp_acc_offset();
		}
#endif
		if((ble_evt_cmd == BLE_CMD_PINCODE_CHECK) || (ble_evt_cmd == BLE_CMD_PINCODE_ERASE))
		{
			clear_tmp_acc_offset();
		}
		__NOP();
	}
	else
	{
		//futre change
		DEBUG_LOG(LOG_INFO,"acc flag false 0x%x",ble_evt_cmd);
		//if not calibration mode, tmp offset value is reset. 
		if(ble_evt_cmd != BLE_RTC_INT)
		{
			clear_tmp_acc_offset();
		}
		gAccessPinCodeFlag = false;
	}
	DEBUG_LOG(LOG_DEBUG,"access_pincode_flag_end");
}

/**
 * @brief access pincode flag get
 * @param pPincodeCheckFlag pin code check flag
 * @retval None
 */
static void pin_code_check_flag_get(bool *pPincodeCheckFlag)
{
	uint32_t err;
	uint8_t critical_pin;
	
	err = sd_nvic_critical_region_enter(&critical_pin);
	if(err == NRF_SUCCESS)
	{
		*pPincodeCheckFlag = gPairingPinCodeCheck;
		critical_pin = 0;
		err = sd_nvic_critical_region_exit(critical_pin);
		if(err != NRF_SUCCESS)
		{
			//nothing to do
		}
	}
}

/**
 * @brief operation mode change.
 * @param pEvent Event Information
 * @retval 0 Success
 */
//
static uint32_t operation_mode_change(PEVT_ST pEvent)
{
	OP_MODE mode;
	uint8_t data;
	uint8_t op_id;
	uint8_t reconfig_sensor_mode = 0;
	EVT_ST  event;
	uint32_t fifo_err;
	EVT_ST  sleepEvent;
	SENSOR_FIFO_DATA_INFO sensor_fifo_data = {0};

	DEBUG_LOG(LOG_DEBUG, "before operation mode 0x%x",gpCurrOPmode->currOP_id);
	//Get operation mode from BLE_CMD_EVENT
	mode = (OP_MODE)pEvent->DATA.modeSet.mode;
	
	/* 動作しないモードに以下を追加 
	 *  - START_REAC_MODE
	 *  - SKY_JUMP_MODE
	 *  - TELEP_MODE
	 *  - SIDE_AGI_MODE
	 */
	if( ( (mode >= TAP_STP_MODE) && (mode <= RES_FOUR) ) || (RAW_MODE_BOTH < mode) )
	{
		DEBUG_LOG( LOG_ERROR,"Invalid operation mode : %x", mode );
		mode = DAILY_MODE;
	}
	
	if((mode != CALIB_MEAS_MODE) && (mode != CALIB_EXE_MODE))
	{
		/* 2020.10.28 Add RAW Mode追加 ++ */
		if ( ( mode >= RAW_MODE ) && ( mode <= RAW_MODE_BOTH ) )
		{
			/* RAW Modeを保持 */
			g_current_raw_mode = mode;
			/* Raw Modeはどのモードの場合も同様のモードにする */
			mode = RAW_MODE;
			/* 2020.12.08 Add 送信データの初期化を追加 ++ */
			g_notify_failed = false;
			memset( (void *)&gRawData, 0, sizeof( gRawData ) );
			/* 2020.12.08 Add 送信データの初期化を追加 -- */
			/* 2020.12.08 Add BLE Tx Complete時の処理を有効に設定 ++ */
			SetBleRawExec( true );
			/* 2020.12.08 Add BLE Tx Complete時の処理を有効に設定 -- */
#ifdef TEST_BLE_POWER_CHANGE
			/* 2020.12.08 Add RAW Mode開始時に8dbmへ設定する ++ */
			BlePowerChenge( TX_POWER_DBM_8 );
			/* 2020.12.08 Add RAW Mode開始時に8dbmへ設定する -- */
#endif
			/* 2020.12.09 Add Tx Complete Countを初期化する ++ */
			SetTxCompCount( 0 );
			/* 2020.12.09 Add Tx Complete Countを初期化する -- */
		}
		/* 2020.10.28 Add RAW Mode追加 -- */
		//change current operation mode.
		/* 2020.10.28 Modify POP_ST_TESTではなくPOP_STを使用する用に修正 ++ */
		gpCurrOPmode = gListCurrOPmode[mode];
		/* 2020.10.28 Modify POP_ST_TESTではなくPOP_STを使用する用に修正 -- */
		//Algo data clear
		AlgoDataReset();
		/* 2020.10.29 Add RAW Mode時は、Mode遷移と同時にTimeout時間が指定されるためここでtimeoutを更新しておく++ */
		if (  ( mode == RAW_MODE ) && ( pEvent->DATA.modeSet.mode_timeout != 0 ) )
		{
			/* 2020.11.20 Add Timeout値が最大値を超えていた場合デフォルトに設定する ++ */
			if ( pEvent->DATA.modeSet.mode_timeout > MODE_CHANGE_TIMEOUT_MAX_VAL )
			{
				DEBUG_LOG( LOG_ERROR, "Over Timeout Max Value(%d s). Change to default value(20s)", pEvent->DATA.modeSet.mode_timeout );
				pEvent->DATA.modeSet.mode_timeout = END_TRIGGER_TO_RAW_MODE;
			}
			/* 2020.11.20 Add Timeout値が最大値を超えていた場合デフォルトに設定する -- */
			
			/* 2020.12.08 Add SIDクリアを受け取る ++ */
			if ( pEvent->DATA.modeSet.clear_sid != 0 )
			{
				g_fifo_clear_sid = pEvent->DATA.modeSet.clear_sid;
			}
			else
			{
				/* クリアなし */
				g_fifo_clear_sid = 0;
			}
			/* 2020.12.08 Add SIDクリアを受け取る -- */

			/* RAW Mode Timeoutを設定 */
			gpCurrOPmode->timeout = pEvent->DATA.modeSet.mode_timeout;
		}
		/* 2020.10.29 Add RAW Mode時は、Mode遷移と同時にTimeout時間が指定されるためここでtimeoutを更新しておく-- */
		DEBUG_LOG( LOG_INFO, "change mode 0x%x timeout=%d clear sid=%d",gpCurrOPmode->currOP_id, gpCurrOPmode->timeout, g_fifo_clear_sid );
	}
	
	if(mode == CALIB_MEAS_MODE)
	{
		/*Reset global var*/
		init_offset_cal_global_var();
		/*algo Data reset*/
		AlgoDataReset();
		StopFlashTimer();
		/*flash operation check*/
		GetFlashOperatoinMode(&op_id);
		if(op_id != FLASH_OP_IDLE)
		{
			/*force sleep*/
			sleepEvent.evt_id = EVT_FORCE_SLEEP;
			fifo_err = PushFifo(&sleepEvent);
			DEBUG_EVT_FIFO_LOG(fifo_err,sleepEvent.evt_id);
			
			fifo_err = PushFifo(pEvent);
			DEBUG_EVT_FIFO_LOG(fifo_err, pEvent->evt_id);
			return 0;
		}
		
		if((gAccessPinCodeFlag == true) && (gPairingPinCodeCheck == true))
		{
			/*timer start*/
			StartUltStartTimer();
			/*notify ack set*/
			data = BLE_NOTIFY_SIG;
			BleNotifySignals(&data, (uint16_t)sizeof(data));
			RestartForceDisconTimer();
			/* 2020.10.28 Modify POP_ST_TESTではなくPOP_STを使用する用に修正 ++ */
			gpCurrOPmode = gListCurrOPmode[mode];
			/* 2020.10.28 Modify POP_ST_TESTではなくPOP_STを使用する用に修正 -- */
		}
		else
		{
			/*return daily mode*/
			ForceChangeDailyMode();
			gAccessPinCodeFlag = false;
		}
	}
	else if(mode == CALIB_EXE_MODE)
	{
		AlgoDataReset();
		StopFlashTimer();
		/*flash operation check*/
		GetFlashOperatoinMode(&op_id);
		if(op_id != FLASH_OP_IDLE)
		{
			/*force sleep*/
			sleepEvent.evt_id = EVT_FORCE_SLEEP;
			fifo_err = PushFifo(&sleepEvent);
			DEBUG_EVT_FIFO_LOG(fifo_err,sleepEvent.evt_id);
			
			fifo_err = PushFifo(pEvent);
			DEBUG_EVT_FIFO_LOG(fifo_err, pEvent->evt_id);
			return 0;
		}
		
		if((gPairingPinCodeCheck == true) && (gAccessPinCodeFlag == true))
		{
			/*check sig data*/
			if(((uint16_t)(gXOffset >> 16) == CHECK_DATA) && ((uint16_t)(gYOffset >> 16) == CHECK_DATA) && ((uint16_t)(gZOffset >> 16) == CHECK_DATA))
			{
				data = BLE_NOTIFY_SIG;
				/*notify ack*/
				BleNotifySignals(&data, (uint16_t)sizeof(data));
				RestartForceDisconTimer();
				/*set offset value and reset flg for power retain ram*/
				RamSaveOffset(CHECK_DATA, gXOffset, gYOffset, gZOffset);
				/*Reboot(force disconnect event (reboot))*/
				/* 2020.10.28 Modify POP_ST_TESTではなくPOP_STを使用する用に修正 ++ */
				gpCurrOPmode = gListCurrOPmode[mode];
				/* 2020.10.28 Modify POP_ST_TESTではなくPOP_STを使用する用に修正 -- */
				event.evt_id = EVT_FORCED_TO;
				fifo_err = PushFifo(&event);
				DEBUG_EVT_FIFO_LOG(fifo_err, event.evt_id);
			}
			else
			{
				/*set defualt value*/
				RamSaveOffset(DEFAULT_SENSOR_OFFSET_SIG, DEFAULT_SENSOR_OFFSET, DEFAULT_SENSOR_OFFSET, DEFAULT_SENSOR_OFFSET);
				ForceChangeDailyMode();
			}
		}
		else
		{
			/*change daily mode*/
			ForceChangeDailyMode();
		}
		gAccessPinCodeFlag = false;
	}
	else if(mode == RAW_MODE)
	{
		/*return ack notify raw mode test*/
		gpCurrOPmode->running = false;
		AlgoDataReset();
		data = BLE_NOTIFY_SIG;
		BleNotifySignals(&data, (uint16_t)sizeof(data));
		RestartForceDisconTimer();
		//StartAckTimer();
		
		/* 2020.10.28 Add RAW_MODEがACC Only以外はLSM6DOSWの再設定を実行 ++ */
		reconfig_sensor_mode = (uint8_t)g_current_raw_mode & RAW_MODE_MASK;
		DEBUG_LOG( LOG_INFO, "ICM42607 Mode : %d", reconfig_sensor_mode );
		/* 2020.12.08 Add FIFO Count Sub ++ */
		sensor_fifo_data.fifo_count = AccGyroGetFifoCount();
		/* Clear対象のSIDに達していたら現在のFIFO Countを取得してFIFOにPush ++ */
		FifoClearPushFifo( &sensor_fifo_data );
		AccGyroValidateClearFifo();
		/* 2020.12.08 Add FIFO Count Sub -- */
		AccGyroReconfig( reconfig_sensor_mode );
		/* 2020.10.28 Add RAW_MODEがACC Only以外はLSM6DOSWの再設定を実行 -- */
		/* 2020.12.23 Add RSSI取得テスト ++ */
		StopBleRSSIReport();
		/* 2020.12.23 Add RSSI取得テスト -- */
	}
	else if((mode != DAILY_MODE) && (mode != CALIB_MEAS_MODE) && (mode != CALIB_EXE_MODE))
	{
		/* ult mode ack timer start. */
		
		DEBUG_LOG(LOG_DEBUG, "ult ack notify timer start 0x%x",gpCurrOPmode->currOP_id);
		if(mode == SKY_JUMP_MODE)
		{
			memset((SKYJUMPCOUNT*)&gUltSkyJumpRes,0x00,sizeof(gUltSkyJumpRes));
			gpResult = (void*)&gUltSkyJumpRes;
		}
		else
		{
			memset((ALTCOUNT*)&gUltCountRes,0x00,sizeof(gUltCountRes));
			gpResult = (void*)&gUltCountRes;
		}
		StartAckTimer();
	}
	else
	{
		memset((DAILYCOUNT*)&gDailyRes,0x00,sizeof(gDailyRes));
		gpResult = (void*)&gDailyRes;
		SetCurrentMode((uint8_t)gpCurrOPmode->currOP_id);
		
		data = BLE_NOTIFY_SIG;
		BleNotifySignals(&data, (uint16_t)sizeof(data));
		//the five hour timeout restart.
		RestartForceDisconTimer();
	}
	
	return 0;
}

/**
 * @brief Increment Walk Timeout
 * @param None
 * @retval None
 */
static void walk_timeout_inc(void)
{
	uint32_t err;
	uint8_t  critical_walk;
	
	err = sd_nvic_critical_region_enter(&critical_walk);
	if(err == NRF_SUCCESS)
	{	
		gNo_walk_sec++;
		critical_walk = 0;
		err = sd_nvic_critical_region_exit(critical_walk);
		if(err != NRF_SUCCESS)
		{
			//nothing to do.
		}		
	}
	DEBUG_LOG(LOG_DEBUG,"no walk count sec %u", gNo_walk_sec);
}

/**
 * @brief reply ack to smart phone function. state cnt, evt EVT_BLE_ACK_TO
 * @param pEvent Event Information
 * @retval 0 Success
 */
static uint32_t reply_ack(PEVT_ST pEvent)
{
	uint8_t  ackdata;
	
	ackdata = BLE_NOTIFY_SIG;
	BleNotifySignals(&ackdata, (uint16_t)sizeof(ackdata));	
	StartUltStartTimer();
	
	return 0;
}

/**
 * @brief ult start trigger timeout function
 * @param pEvent Event Information
 * @retval 0 Success
 */
static uint32_t start_trigger_timeout_func(PEVT_ST pEvent)
{
	DEBUG_LOG(LOG_INFO,"start_trigger_timeout");
	StopUltStartTimer();				//start trigger timer stop
	SetBleErrReadCmd(UTC_BLE_ULT_TIME_ERROR);
	ForceChangeDailyMode();
	RestartForceDisconTimer();	 //force disconnect timer restart.
	
	return 0;
}

/**
 * @brief ult end trigger timeout function
 * @param pEvent Event Information
 * @retval 0 Success
 */
static uint32_t end_trigger_timeout_func(PEVT_ST pEvent)
{
	DEBUG_LOG(LOG_INFO,"end_trigger_timeout");
	
	//end trigger timer stop
	StopUltEndTimer();
	SetBleErrReadCmd(UTC_BLE_ULT_TIME_ERROR);
	DEBUG_LOG(LOG_DEBUG,"ult end timeout. max trigger %u",gpCurrOPmode->max_trigger_count);
	//algo data reset
	ForceChangeDailyMode();
	
	//force disconnect timer restart.
	RestartForceDisconTimer();
	
	return 0;
}

/**
 * @brief erase daily log function
 * @param pEvent Event Information
 * @retval 0 Success
 */
static uint32_t erase_log_one(PEVT_ST pEvent)
{
	uint8_t flashFlag;
	uint32_t fifo_err;
	EVT_ST      event;
	
	flashFlag = pEvent->DATA.dataInfo.flash_flg;
	
	DEBUG_LOG(LOG_DEBUG,"Erase daily log enter");
	FlashDailyLogPageClear(flashFlag);
	//ram save walk information clear.
	RamSaveRegionWalkInfoClear();
	RamSidClear();
	//push event return slave latency event  180128
	event.evt_id = EVT_FLASH_CNT_PARAM_RETURN;
	fifo_err = PushFifo(&event);
	DEBUG_EVT_FIFO_LOG(fifo_err,event.evt_id);
	
	return 0;
}

/**
 * @brief Get battery value
 * @param pEvent Event Information
 * @retval 0 Success
 */
static uint32_t get_battery_value_daily(PEVT_ST pEvent)
{
	UNUSED_PARAMETER(*pEvent);
	// Get battery value
	GetBatteryInfo();

	return 0;
}

/**
 * @brief RTC Time Change Binary Data
 * @param pResult 変換した結果
 * @param pDateTime RTCから取得した時刻
 * @param Len pResultの長さ
 * @retval None
 */
static void time_data_format_change(uint8_t *pResult, DATE_TIME *pDateTime, uint8_t Len)
{
	if( 12 < Len )
	{
		return;
	}

	// year
	*(pResult + YEAR_HIGH)  = (uint8_t)pDateTime->year  /  10;
	*(pResult + YEAR_LOW)   = (uint8_t)pDateTime->year  %  10;
	//month
	*(pResult + MONTH_HIGH) = (uint8_t)pDateTime->month /  10;
	*(pResult + MONTH_LOW)  = (uint8_t)pDateTime->month %  10;
	//day
	*(pResult + DAY_HIGH)   = (uint8_t)pDateTime->day   /  10;
	*(pResult + DAY_LOW)    = (uint8_t)pDateTime->day   %  10;
	//hour
	*(pResult + HOUR_HIGH)  = (uint8_t)pDateTime->hour  /  10;
	*(pResult + HOUR_LOW)   = (uint8_t)pDateTime->hour  %  10;
	//min
	*(pResult + MIN_HIGH)   = (uint8_t)pDateTime->min   /  10;
	*(pResult + MIN_LOW)    = (uint8_t)pDateTime->min   %  10;
	//sec
	*(pResult + SEC_HIGH)   = (uint8_t)pDateTime->sec   /  10;
	*(pResult + SEC_LOW)    = (uint8_t)pDateTime->sec   %  10;
}

/**
 * @brief Get RTC Time from Daily Mode
 * @param pEvent Event Information
 * @retval 0 Success
 */
static uint32_t get_rtc_time_daily(PEVT_ST pEvent)
{
	DATE_TIME dateTime;
	uint8_t sendTime[12];
	ret_code_t err_code;
	ret_code_t time_err_code;
	ble_gatts_rw_authorize_reply_params_t  reply_timedata;
	uint16_t cnt_handle = BLE_CONN_HANDLE_INVALID;

	memset(sendTime,0x00,sizeof(sendTime));
	
	time_err_code = ExRtcGetDateTime(&dateTime);
	if(time_err_code == NRF_SUCCESS)
	{
		reply_timedata.params.read.update = 1;
	}
	else
	{
		reply_timedata.params.read.update = 0;
	}
	time_data_format_change(sendTime,&dateTime, sizeof(sendTime));
	reply_timedata.params.read.p_data		= (uint8_t*)&sendTime;
	reply_timedata.type						= BLE_GATTS_AUTHORIZE_TYPE_READ ;
	reply_timedata.params.read.gatt_status	= BLE_GATT_STATUS_SUCCESS;
	reply_timedata.params.read.len			= sizeof(sendTime);
	reply_timedata.params.read.offset		= 0;
	GetBleCntHandle(&cnt_handle);
	err_code = sd_ble_gatts_rw_authorize_reply(cnt_handle, &reply_timedata);
	
#ifdef	TEST_AUTHRIZE_ERROR
	err_code = NRF_ERROR_BUSY;
#endif
	if(err_code != NRF_SUCCESS)
	{
		SetBleErrReadCmd(UTC_BLE_AUTHORIZE_ERROR);
	}
	BleAuthorizeErrProcess(err_code);
	
	RestartForceDisconTimer();
	DEBUG_LOG(LOG_INFO, "Get RTC time, Y %u, M %u, D %u, hh %u, mm %u, ss %u",dateTime.year, dateTime.month,dateTime.day,dateTime.hour,dateTime.min,dateTime.sec);
	return 0;
}

/**
 * @brief Get Algorithm SID
 * @param None
 * @retval gAlgoSid Algorithm sid
 */
static uint16_t algo_get_sid(void)
{
	return gAlgoSid;
}

/**
 * @brief recieve ult start trigger
 * @param pEvent Event Information
 * @retval 0 Success
 */
static uint32_t recieve_ult_start_trigger(PEVT_ST pEvent)
{
	if(gpCurrOPmode->currOP_id == SKY_JUMP_MODE)
	{
		gpCurrOPmode->trigger_count = -1;
	}
	else
	{
		gpCurrOPmode->trigger_count = 0;
	}
	//currmode
	gpCurrOPmode->currTrigger = 0;
	gpCurrOPmode->residual_count = false;
	//result data clear
	memset(&(gpCurrOPmode->RESULTDATA), 0x00, sizeof(gpCurrOPmode->RESULTDATA));
	gpCurrOPmode->first_sid     = true;
	//curr sid get for skyjump and start reaction
	gpCurrOPmode->ult_move_time = algo_get_sid();
	gpCurrOPmode->running = true;
	DEBUG_LOG(LOG_DEBUG,"ult start trig. mode 0x%x, run state %d",gpCurrOPmode->currOP_id,gpCurrOPmode->running);
	DEBUG_LOG(LOG_DEBUG,"ult start trig. max trigger %u",gpCurrOPmode->max_trigger_count);
	
	/* 2022.07.19 Add Raw Data送信中には強制切断タイマーを停止する ++ */
	StopForceDisconTimer();
	/* 2022.07.19 Add Raw Data送信中には強制切断タイマーを停止する -- */
	
	return 0;
}

/**
 * @brief recieve ult trigger
 * @param pEvent Event Information
 * @retval 0 Success
 */
static uint32_t recieve_ult_trriger(PEVT_ST pEvent)
{
	uint32_t fifo_err;
	uint16_t gatts_value_handle;
	uint8_t mes_trigger;
	uint8_t resultdata[ULTIMET_RES_SIZE];
	uint8_t mode;
	uint8_t data;
	EVT_ST sleepEvent;
	EVT_ST replyEvent;
	static uint8_t rCount = 0;

	//curr trigger send
	gpCurrOPmode->currTrigger = pEvent->DATA.modeTrigger.trigger;
	if(pEvent->DATA.modeTrigger.trigger == ULT_END_TRIGGER)
	{
		//FW 9.00 version
		DEBUG_LOG(LOG_INFO,"Recieve trig func. count %u. max trigger %u. END Trigger",(gpCurrOPmode->trigger_count + 1),gpCurrOPmode->max_trigger_count);
		if(((gpCurrOPmode->trigger_count + 1) < gpCurrOPmode->max_trigger_count) || (gpCurrOPmode->running == false))
		{
			//Test Task No 6 err modify.
			DEBUG_LOG(LOG_ERROR,"Recieve trig Seq err. END Trigger");
			SetBleErrReadCmd(UTC_BLE_ULT_SEAQUENCE_ERROR);
		}

		mode = gpCurrOPmode->currOP_id;
		if((mode == TAP_STP_MODE) || (mode == SIDE_AGI_MODE) || (mode == TELEP_MODE))
		{
			if(gpCurrOPmode->residual_count == false)
			{
				sleepEvent.evt_id = EVT_FORCE_SLEEP;
				fifo_err = PushFifo(&sleepEvent);
				DEBUG_EVT_FIFO_LOG(fifo_err,sleepEvent.evt_id);
				
				memcpy(&replyEvent, pEvent, sizeof(replyEvent));
				fifo_err = PushFifo(&replyEvent);
				DEBUG_EVT_FIFO_LOG(fifo_err, replyEvent.evt_id);
				return 0;
			}
		}
		/*raw mode end trig*/
		if(mode == RAW_MODE)
		{
			//gpCurrOPmode->running = false;
			data = BLE_NOTIFY_SIG;
			//BleNotifySignals(&data, (uint16_t)sizeof(data));
			ret_code_t ret = RawResSetBleNotifyCmd(&data,sizeof(data));
			if(ret != NRF_SUCCESS)
			{
				/*retry event*/
				rCount++;
				if(rCount < 10)
				{
					DEBUG_LOG(LOG_ERROR,"raw mode end trig retry evt");
					sleepEvent.evt_id = EVT_FORCE_SLEEP;
					fifo_err = PushFifo(&sleepEvent);
					DEBUG_EVT_FIFO_LOG(fifo_err,sleepEvent.evt_id);
				
					memcpy(&replyEvent, pEvent, sizeof(replyEvent));
					fifo_err = PushFifo(&replyEvent);
					DEBUG_EVT_FIFO_LOG(fifo_err, replyEvent.evt_id);
					return 0;
				}
				else
				{
					DEBUG_LOG(LOG_ERROR,"raw mode end trig retry err");
					rCount = 0;
				}
			}
			else
			{
				DEBUG_LOG(LOG_ERROR,"raw mode end trig");
				/* 2020.10.28 Add LSM6DOSWをACC Onlyに戻す ++ */
				AccGyroReconfig( MODE_ACC_ONLY_LP );
				/* 2020.10.28 Add LSM6DOSWをACC Onlyに戻す -- */
			}
		}
		else
		{
			resultdata[0] = gpCurrOPmode->currOP_id;
			memcpy(&resultdata[1], &(gpCurrOPmode->RESULTDATA), sizeof(gpCurrOPmode->RESULTDATA));
		
			GetGattsCharHandleValueID(&gatts_value_handle,MODE_RESULT_ID);
			SetBleNotifyCmd(gatts_value_handle, resultdata,sizeof(resultdata));
			gpCurrOPmode->running = false;
		}
		DEBUG_LOG(LOG_DEBUG,"ult mode change daily mode. run %d",gpCurrOPmode->running);
		ForceChangeDailyMode();//change current operation mode.
		rCount = 0;
		//End timer stop
		StopUltEndTimer();
		//force discon timer start
		StartForceDisconTimer();
	}
	else
	{ 
		gpCurrOPmode->residual_count = false;
		//trigger count and change of save box
		gpCurrOPmode->trigger_count++;
		DEBUG_LOG(LOG_INFO,"trigger count %u",gpCurrOPmode->trigger_count);
		if(gpCurrOPmode->currOP_id == SKY_JUMP_MODE)
		{
			mes_trigger = pEvent->DATA.modeTrigger.trigger - 1;
		}
		/*raw mode */
		else if(mode == RAW_MODE)
		{
			/*run start*/
			if(pEvent->DATA.modeTrigger.trigger == 0)
			{
				/*start trigger. meas start*/
				gpCurrOPmode->running = true;
			}
			return 0;
		}
		else
		{
			mes_trigger = pEvent->DATA.modeTrigger.trigger;
		}

		DEBUG_LOG(LOG_DEBUG,"tri count %u, mes tri count %u",gpCurrOPmode->trigger_count,mes_trigger);
		if(gpCurrOPmode->trigger_count < mes_trigger)
		{	
			DEBUG_LOG(LOG_ERROR,"Recieve trig func. ULT Trigger");
			SetBleErrReadCmd(UTC_BLE_ULT_SEAQUENCE_ERROR);
		}
		
		gpCurrOPmode->first_sid     = true;
		gpCurrOPmode->ult_move_time = algo_get_sid();
	}
	return 0;
}

/**
 * @brief Calibration Calculate Value
 * @param acc_data ACC Data Information
 * @retval None
 * 2020.10.26 Modify acc_dataを新フォーマットへ対応とポインタ化
 */
static void calib_calculate_value(ACC_GYRO_DATA_INFO *acc_data)
{
	uint16_t   notify_handle;
	int32_t    checkdata = CHECK_DATA;
	uint8_t    notify_data[CALIB_SIZE];
	int32_t    diff_p = 0;
	int32_t    diff_n = 0;
	
	int16_t    Xave16 = 0;
	int16_t    Yave16 = 0;
	int16_t    Zave16 = 0;
	int16_t    tmp = 0; 
	
	memset(notify_data,0x00,sizeof(notify_data));
	
	/* 2020.10.26 Add Tagをチェックし、ACCデータ以外は何もしない */
	//if ( acc_data->tag_data != TAG_ACC_NC )
	if ( acc_data->header != 0x01 )
	{
		//DEBUG_LOG( LOG_ERROR,"Non ACC Data tag=0x%x", acc_data->tag_data );
		DEBUG_LOG( LOG_ERROR,"Non ACC Data tag=0x%x", acc_data->header );
		return ;
	}
	
	/* 2020.10.26 Modify ACC/Gyro用のフォーマットに変数を変更 ++ */
	DEBUG_LOG(LOG_ERROR,"Calib x %d, y %d, z %d count %d",acc_data->acc_x_data,acc_data->acc_y_data,acc_data->acc_z_data, gCalibCount);
#if 1
	/*x data min,max serach*/
	if(gXmax < acc_data->acc_x_data)
	{
		gXmax = acc_data->acc_x_data;
	}
	
	if(acc_data->acc_x_data < gXmin)
	{
		gXmin = acc_data->acc_x_data;
	}
	
	/*y data min, max search*/
	if(gYmax < acc_data->acc_y_data)
	{
		gYmax = acc_data->acc_y_data;
	}
	
	if(acc_data->acc_y_data < gYmin)
	{
		gYmin = acc_data->acc_y_data;
	}
	
	/*z data min, max serch*/
	if(gZmax < acc_data->acc_z_data)
	{
		gZmax = acc_data->acc_z_data;
	}
	
	if(acc_data->acc_z_data < gZmin)
	{
		gZmin = acc_data->acc_z_data;
	}
#endif
	gCalibCount++;
	gXsum += acc_data->acc_x_data;
	gYsum += acc_data->acc_y_data;
	gZsum += acc_data->acc_z_data;
	/* 2020.10.26 Modify ACC/Gyro用のフォーマットに変数を変更 -- */
	
	/*offset value caluculation. average X, Y, Z axis value 50 point*/
	if(49 < gCalibCount)
	{

		/*average calculation(offset value caluclation)*/
		gXsum = gXsum / gCalibCount;
		gYsum = gYsum / gCalibCount;
		gZsum = gZsum / gCalibCount;
		
		/**/
		Xave16 = (int16_t)gXsum;
		Yave16 = (int16_t)gYsum;
		Zave16 = (int16_t)gZsum;
		
		/*normal case*/
		if((((int32_t)gXmax - (int32_t)gXmin) <= OFFSET_P_TO_P) && (((int32_t)gYmax - (int32_t)gYmin) <= OFFSET_P_TO_P) && (((int32_t)gZmax - (int32_t)gZmin) <= OFFSET_P_TO_P))
		{
			/*set ble notify signal data*/
			//memset((CALIB_ACK*)&calib_ack,0x00,sizeof(calib_ack));
			//calib_ack.err_code = (uint16_t)1;
			notify_data[0] = 1;
			memcpy(&notify_data[1], &Xave16, sizeof(Xave16));
			notify_data[3] = 1;
			memcpy(&notify_data[4], &Yave16, sizeof(Yave16));
			notify_data[6] = 1;
			memcpy(&notify_data[7], &Zave16, sizeof(Zave16));
			DEBUG_LOG(LOG_INFO,"Calib end: x_off %d 0x%x, y_off %d 0x%x, z_off %d 0x%x,",Xave16,Xave16,Yave16,Yave16,Zave16,Zave16);
			/*z axis offset value reject gravitiy components 1000[mg]*/
			Zave16 -= 1000;
			/*set offset value and signature to global var*/
			gXOffset = ((checkdata << 16) | ((int32_t)Xave16 & 0x0000ffff));
			gYOffset = ((checkdata << 16) | ((int32_t)Yave16 & 0x0000ffff));
			gZOffset = ((checkdata << 16) | ((int32_t)Zave16 & 0x0000ffff));
			
		}
		/*err case*/
		else
		{
			gXOffset = DEFAULT_SENSOR_OFFSET;
			gYOffset = DEFAULT_SENSOR_OFFSET;
			gZOffset = DEFAULT_SENSOR_OFFSET;
			/*x axis err value check*/
			if(OFFSET_P_TO_P < ((int32_t)gXmax - (int32_t)gXmin))
			{
				notify_data[0] = 0;
				diff_p = (int32_t)gXmax - (int32_t)Xave16;
				diff_n = (int32_t)Xave16 - (int32_t)gXmin;
				
				if(diff_p < 0)
				{
					diff_p = diff_p * -1;
				}
				
				if(diff_n < 0)
				{
					diff_n = diff_n * -1;
				}
				
				if(diff_p < diff_n)
				{
					tmp = gXmin;
				}
				else
				{
					tmp = gXmax;
				}
				DEBUG_LOG(LOG_DEBUG,"x err. calib end x %d",tmp);
				memcpy(&notify_data[1], &tmp, sizeof(tmp));
			}
			else
			{
				notify_data[0] = 1;
				memcpy(&notify_data[1], &Xave16, sizeof(Xave16));
			}
			/*y axis err value check*/
			if(OFFSET_P_TO_P < (gYmax - gYmin))
			{
				notify_data[3] = 0;
				diff_p = (int32_t)gYmax - (int32_t)Yave16;
				diff_n = (int32_t)Yave16 - (int32_t)gYmin;
				
				if(diff_p < 0)
				{
					diff_p = diff_p * -1;
				}
				
				if(diff_n < 0)
				{
					diff_n = diff_n * -1;
				}
				
				if(diff_p < diff_n)
				{
					tmp = gYmin;
				}
				else
				{
					tmp = gYmax;
				}
				DEBUG_LOG(LOG_DEBUG,"y err. calib end y %d",tmp);
				memcpy(&notify_data[4], &tmp, sizeof(tmp));
			}
			else
			{
				notify_data[3] = 1;
				memcpy(&notify_data[4], &Yave16, sizeof(Yave16));
			}
			/*z axis err value check*/
			if(OFFSET_P_TO_P < (gZmax - gZmin))
			{
				notify_data[6] = 0;
				diff_p = (int32_t)gZmax - (int32_t)Zave16;
				diff_n = (int32_t)Zave16 - (int32_t)gZmin;
				
				if(diff_p < 0)
				{
					diff_p = diff_p * -1;
				}
				
				if(diff_n < 0)
				{
					diff_n = diff_n * -1;
				}
				
				if(diff_p < diff_n)
				{
					tmp = gZmin;
				}
				else
				{
					tmp = gZmax;
				}
				DEBUG_LOG(LOG_DEBUG,"z err. calib end z %d",tmp);
				memcpy(&notify_data[7], &tmp, sizeof(tmp));
			}
			else
			{
				notify_data[6] = 1;
				memcpy(&notify_data[7], &Zave16, sizeof(Zave16));
			}
			/*debug display*/
			DEBUG_LOG(LOG_ERROR,"Calib end: err. x_sig %d, x_value %d, y_sig %d, y_value %d, z_sig %d, z_value %d ",notify_data[0],(int16_t)(((int16_t)notify_data[2] << 8)|(int16_t)notify_data[1]), notify_data[3],(int16_t)(((int16_t)notify_data[5] << 8)|(int16_t)notify_data[4]), notify_data[6],(int16_t)(((int16_t)notify_data[8] << 8)|(int16_t)notify_data[7]));
			/*trace log*/
			TRACE_LOG(TR_OFFSET_P_TO_P_ERROR, (uint16_t)((notify_data[0] << 2) | (notify_data[3]) << 1 | notify_data[6]));
		}
		
		/*Notify up*/
		GetGattsCharHandleValueID(&notify_handle, CALIB_ID);
		SetBleNotifyCmd(notify_handle,(uint8_t*)&notify_data,sizeof(notify_data));
		
		/*init global var*/
		init_offset_cal_global_var();
		
		/*daily mode change*/
		ForceChangeDailyMode();
		
		/*access calib flag clear*/
		gAccessPinCodeFlag = false;
		
		/*timer stop*/
		StopUltStartTimer();
	}
}

/**
 * @brief Initialize Offset Calc Global value
 * @param None
 * @retval None
 */
static void init_offset_cal_global_var(void)
{
	gXmin = 32767;
	gXmax = -32768;
	gYmin = 32767;
	gYmax = -32768;
	gZmin = 32767;
	gZmax = -32768;
	gXsum = 0;
	gYsum = 0;
	gZsum = 0;
	gCalibCount = 0;
}

/* 2022.01.26 Add RawData送信中はRTCの割り込みが発生しても処理を行わないように修正 ++ */
/**
 * @brief Raw Mode Sending RTC Processing
 * @param pEvent Event Information
 * @retval 0 Success
 */
static uint32_t raw_data_sending_rtc_proc( PEVT_ST pEvent )
{
	DEBUG_LOG( LOG_INFO, "!!! RTC Interrupt Pass Through !!!" );
	/* イベントIDのみを保持しておく */
	g_rtc_int_id = pEvent->evt_id;
	
	/* RTC Interrupt を無効に設定 */
	ExRtcDisableInterrupt();
	
	return 0;
}
/* 2022.01.26 Add RawData送信中はRTCの割り込みが発生しても処理を行わないように修正 -- */

/* 2022.05.13 Add Guest Mode 追加 ++ */
/**
 * @brief Guest Mode Setting And Notify
 * @param None
 * @retval None
 */
static void guest_mode_notify( void )
{
	uint8_t notify_signal;
	
	DEBUG_LOG( LOG_INFO, "!!! Enter Guest Mode !!!" );
	
	/* Guest Mode ON */
	SetGuestModeState( true );
	/* BLE Response */
	notify_signal = BLE_NOTIFY_SIG;
	BleNotifySignals( &notify_signal, (uint16_t)sizeof( notify_signal ) );
}
/* 2022.05.13 Add Guest Mode 追加 -- */

/* 2022.05.19 Add 角度調整モード追加 ++ */
/**
 * @brief Guest Mode Setting And Notify
 * @param None
 * @retval None
 */
static void angle_adjust_exec( PEVT_ST pEvent )
{
	uint8_t notify_signal;

	DEBUG_LOG( LOG_INFO, "Angle Adjust: %d", pEvent->DATA.angle_adjust_info.state );
	/* 状態変更 */
	ChangeAngleAdjustState( pEvent->DATA.angle_adjust_info.state );
	if ( pEvent->DATA.angle_adjust_info.state != ANGLE_ADJUST_START )
	{
		/* 開始以外の場合ここでレスポンスを返す */
		/* BLE Response */
		notify_signal = BLE_NOTIFY_SIG;
		BleNotifySignals( &notify_signal, (uint16_t)sizeof( notify_signal ) );
	}
	/* 2022.06.22 Add 角度調整の開始だった場合バッファをクリア ++ */
	else
	{
		acc_angle_reset();
	}
	/* 2022.06.22 Add 角度調整の開始だった場合バッファをクリア -- */
}
/* 2022.05.19 Add 角度調整モード追加 -- */


/**
 * @brief Initialize Mode Manager Operation
 * @param None
 * @retval None
 */
void InitOpModeMgr( void )
{
	//initialize current op mdoe (Daily Mode)
	/* 2020.10.28 Modify POP_ST_TESTではなくPOP_STを使用する用に修正 ++ */
	gpCurrOPmode = gListCurrOPmode[DAILY_MODE];
	/* 2020.10.28 Modify POP_ST_TESTではなくPOP_STを使用する用に修正 -- */
	gpResult = (DAILYCOUNT*)&gDailyRes;
}

/**
 * @brief Get Mode Timeout Value
 * @param pTimeout Timeout Value
 * @retval None
 */
void GetModeTimeoutValue(uint16_t *pTimeout)
{
	uint32_t err;
	uint8_t critical_force;
	
	err = sd_nvic_critical_region_enter(&critical_force);
	if(err == NRF_SUCCESS)
	{
		*pTimeout = gpCurrOPmode->timeout;
		critical_force = 0;
		err = sd_nvic_critical_region_exit(critical_force);
		if(err != NRF_SUCCESS)
		{
				//nothing to do
		}
	}
}

/**
 * @brief Force Change Daily Mode
 * @param None
 * @retval None
 */
void ForceChangeDailyMode(void)
{
	EVT_ST evt = {0};
	uint32_t err;
	uint8_t critical_force;
	
	if(gpCurrOPmode->currOP_id != DAILY_MODE)
	{
		err = sd_nvic_critical_region_enter(&critical_force);
		if(err == NRF_SUCCESS)
		{
			DEBUG_LOG(LOG_INFO,"force change daily mode. run %d",gpCurrOPmode->running);
			gpCurrOPmode->running = false;
			AlgoDataReset();
			/* 2020.10.29 Add Raw Modeの場合にはTimeout値をDefaultへ戻す ++ */
			if ( gpCurrOPmode->currOP_id == RAW_MODE )
			{
				gpCurrOPmode->timeout = END_TRIGGER_TO_RAW_MODE;
#ifdef TEST_BLE_POWER_CHANGE
				/* 2020.12.08 Add RAW Mode終了時に4dbmに戻す ++ */
				BlePowerChenge( TX_POWER_DBM_4 );
				/* 2020.12.08 Add RAW Mode終了時に4dbmに戻す -- */
#endif
			}
			/* 2020.10.29 Add Raw Modeの場合にはTimeout値をDefaultへ戻す -- */
			/* Daily Mode へ戻す */
			/* 2020.10.28 Modify POP_ST_TESTではなくPOP_STを使用する用に修正 ++ */
			gpCurrOPmode = gListCurrOPmode[DAILY_MODE];
			/* 2020.10.28 Modify POP_ST_TESTではなくPOP_STを使用する用に修正 -- */
			SetCurrentMode((uint8_t)DAILY_MODE);
			/*rawdata counter reset*/
			/* 2020.10.28 Add LSM6DOSWをACC Only Low Powerに戻す ++ */
			AccGyroReconfig( MODE_ACC_ONLY_LP );
			/* 2020.10.28 Add LSM6DOSWをACC Only Low Powerに戻す -- */
			/* 2020.10.28 Add Raw Mode Stateを初期化 ++ */
			g_current_raw_mode = RAW_MODE;
			/* 2020.10.28 Add Raw Mode Stateを初期化 -- */
			/* 2020.12.08 Add BLE Tx Complete時の処理を有効に設定 ++ */
			SetBleRawExec( false );
			/* 2020.12.08 Add BLE Tx Complete時の処理を有効に設定 -- */
			/* 2022.01.26 Add RTC Interruptが発生していた場合は、再度、イベントを積む ++ */
			if ( g_rtc_int_id == EVT_RTC_INT )
			{
				/* RTCの割り込みを有効化 */
				ExRtcEnableInterrupt();
				/* イベントを発行する */
				evt.evt_id = EVT_RTC_INT;
				err = PushFifo(&evt);
				DEBUG_EVT_FIFO_LOG( err, evt.evt_id );
			}
			g_rtc_int_id = EVENT_ID_NONE;
			/* 2022.01.26 Add RTC Interruptが発生していた場合は、再度、イベントを積む -- */
			/* 2022.03.17 Add 送信SID初期化 ++ */
			g_send_sid_base = 0;
			g_send_sid_init = false;
			/* 2022.03.17 Add 送信SID初期化 -- */

			/* 2020.12.23 Add RSSI取得テスト ++ */
			g_force_disconnect_count = 0;
			StartBleRSSIReport();
			/* 2020.12.23 Add RSSI取得テスト -- */

			/* 2022.07.19 Add Raw Data送信中に停止していた強制切断タイマーを再開 ++ */
			StartForceDisconTimer();
			/* 2022.07.19 Add Raw Data送信中に停止していた強制切断タイマーを再開 -- */

			critical_force = 0;
			err = sd_nvic_critical_region_exit(critical_force);
			if(err != NRF_SUCCESS)
			{
				//nothing to do
			}
		}
	}
}

/**
 * @brief BLE Wrapper Function
 * @param pEvent Event Information
 * @retval None
 */
uint32_t BleWrapperFunc(PEVT_ST pEvent)
{
	ret_code_t err_code;
	uint8_t eventId;
	ble_gatts_rw_authorize_reply_params_t reply_dummy;
	uint16_t bat_dummy;
	uint16_t cnt_handle = BLE_CONN_HANDLE_INVALID;
	bool pairing_enable = false;
	/* 2022.05.13 Add Guest Mode追加 ++ */
	bool guest_mode_enable = false;
	/* 2022.05.13 Add Guest Mode追加 -- */
	
	eventId = pEvent->evt_id;
#ifdef PAIRING_MODE
	pin_code_check_flag_get(&pairing_enable);
	
	//pairing OK ?
	if(pairing_enable == true)
	{
		DEBUG_LOG(LOG_DEBUG,"Cmpl pairing");
		DEBUG_LOG(LOG_DEBUG,"a w");
		DEBUG_LOG(LOG_DEBUG,"wrapper func event 0x%x, ble event 0x%x",eventId, (eventId + BLE_CMD_OFFSET) & (~BLE_CMD_MASK));
		DEBUG_LOG(LOG_DEBUG,"b w");
		access_pincode_flag_check((BLE_CMD_EVENT)((eventId + BLE_CMD_OFFSET) & (~BLE_CMD_MASK)), pEvent);
		/* 2022.05.19 Add 角度調整 ++ */
		if ( pEvent->evt_id == EVT_BLE_CMD_ANGLE_ADJUST )
		{
			/* 角度調整実行 */
			angle_adjust_exec( pEvent );
		}
		else
		{
			ble_mode_func(pEvent);
		}
		/* 2022.05.19 Add 角度調整 -- */
	}
	else
	{
		DEBUG_LOG(LOG_DEBUG,"NOT pairing, 0x%x",pEvent->evt_id);
		if ( ( pEvent->evt_id == EVT_RTC_INT ) || ( pEvent->evt_id == EVT_BLE_CMD_ACCEPT_PINCODE_CHECK ) || ( pEvent->evt_id == EVT_BLE_CMD_PINCODE_CHECK ) ||
			 ( pEvent->evt_id == EVT_BLE_CMD_PINCODE_ERASE ) || ( pEvent->evt_id == EVT_FLASH_OP_STATUS_CHECK ) || ( pEvent->evt_id == EVT_INIT_CMD_START )||
			 ( pEvent->evt_id == EVT_FLASH_DATA_WRITE_CMPL ) || ( pEvent->evt_id == EVT_FLASH_DATA_ERASE_CMPL ) )
		{
			DEBUG_LOG(LOG_DEBUG,"check pairing code");
			DEBUG_LOG(LOG_INFO,"not paring event 0x%x, ble event 0x%x",eventId,(eventId + BLE_CMD_OFFSET) & (~BLE_CMD_MASK));
			access_pincode_flag_check((BLE_CMD_EVENT)((eventId + BLE_CMD_OFFSET) & (~BLE_CMD_MASK)), pEvent);
			ble_mode_func(pEvent);
		}
		else if(pEvent->evt_id == EVT_BLE_CMD_GET_BAT || pEvent->evt_id == EVT_BLE_CMD_GET_TIME)
		{
			access_pincode_flag_check((BLE_CMD_EVENT)((eventId + BLE_CMD_OFFSET) & (~BLE_CMD_MASK)), pEvent);
			if(pEvent->evt_id == EVT_BLE_CMD_GET_TIME)
			{
				reply_dummy.params.read.p_data	= pEvent->DATA.dataInfo.dateTime;
				reply_dummy.params.read.len		= sizeof(pEvent->DATA.dataInfo.dateTime);
			}
			else
			{
				bat_dummy						= 0x0000;
				reply_dummy.params.read.p_data	= (uint8_t*)&bat_dummy;
				reply_dummy.params.read.len		= sizeof(bat_dummy);
			}
			reply_dummy.type					= BLE_GATTS_AUTHORIZE_TYPE_READ;	//dummy_data
			reply_dummy.params.read.gatt_status	= BLE_GATT_STATUS_SUCCESS;
			reply_dummy.params.read.update 		= 1;
			reply_dummy.params.read.offset 		= 0;
			GetBleCntHandle(&cnt_handle);
			err_code = sd_ble_gatts_rw_authorize_reply(cnt_handle,&reply_dummy);
			BleAuthorizeErrProcess(err_code);
			
			SetBleErrReadCmd(UTC_BLE_AUTHORIZE_ERROR);
		}
		/* 2022.05.13 Add Guest Mode追加 ++ */
		else if ( pEvent->evt_id == EVT_BLE_CMD_GUEST_MODE )
		{
			access_pincode_flag_check((BLE_CMD_EVENT)((eventId + BLE_CMD_OFFSET) & (~BLE_CMD_MASK)), pEvent);
			/* ゲストモード有効化 */
			guest_mode_notify();
			
			/* kono test 220809 */
			EVT_ST evt; 
			evt.evt_id = EVT_UPDATING_PARAM;//EVT_UPDATE_PARAM;
			
			int random_id = random_index();
			SetRndId(random_id);
			//state change.
			uint32_t fifo_err = PushFifo(&evt);
			DEBUG_EVT_FIFO_LOG(fifo_err, evt.evt_id);
			/* kono test 220809 */
		}
		/* 2022.05.13 Add Guest Mode追加 -- */
		/* 2022.05.13 Add 角度調整モード追加 ++ */
		else if ( pEvent->evt_id == EVT_BLE_CMD_ANGLE_ADJUST )
		{
			/* 角度調整実行 */
			angle_adjust_exec( pEvent );
		}
		/* 2022.05.13 Add 角度調整モード追加 -- */
		/* 2022.06.13 Add RSSI通知 ++ */
		else if ( pEvent->evt_id == EVT_RSSI_NOTIFY )
		{
			/* pin codeのチェックなく通知する */
			ble_mode_func(pEvent);
		}
		/* 2022.06.13 Add RSSI通知 -- */
		else
		{
			access_pincode_flag_check((BLE_CMD_EVENT)((eventId + BLE_CMD_OFFSET) & (~BLE_CMD_MASK)), pEvent);
			/* 2022.05.13 Add Guest Mode追加 ++ */
			guest_mode_enable = GetGuestModeState();
			if ( guest_mode_enable == true )
			{
				DEBUG_LOG( LOG_INFO, "Start Guest Mode: 0x%x", pEvent->evt_id );
				/* ゲストモードが有効の場合、Mode Change,Mode Triggerは実行させる */
				if ( ( pEvent->evt_id == EVT_BLE_CMD_MODE_CHANGE ) || ( pEvent->evt_id == EVT_BLE_CMD_ULT_START_TRIG ) ||
					 ( pEvent->evt_id == EVT_BLE_CMD_ULT_END_TRIG ) || ( pEvent->evt_id == EVT_BLE_CMD_ULT_TRIG ) ||
					 ( pEvent->evt_id == EVT_ULT_START_TRIG_TO ) || ( pEvent->evt_id == EVT_ULT_STOP_TRIG_TO ) )
				{
					/* 実行 */
					ble_mode_func(pEvent);
				}
			}
			else
			{
				if ( pEvent->evt_id == EVT_BLE_CMD_MODE_CHANGE )
				{
					SetCurrentMode((uint8_t)DAILY_MODE);
				}
				DEBUG_LOG(LOG_INFO,"not pairing other event 0x%x",pEvent->evt_id);
			}
			/* 2022.05.13 Add Guest Mode追加 -- */
		}
		DEBUG_LOG(LOG_DEBUG,"wrapper event 0x%x,",pEvent->evt_id);
	}
	
#else
	access_pincode_flag_check((BLE_CMD_EVENT)((eventId + BLE_CMD_OFFSET) & (~BLE_CMD_MASK)), pEvent);
	ble_mode_func(pEvent);
#endif
	return 0;
}

/**
 * @brief pairing pincode check flag clear.
 * @param None
 * @retval None
 */
void PinCodeCheckFlagClear(void)
{
	uint32_t err;
	uint8_t critical_pin;
	
	err = sd_nvic_critical_region_enter(&critical_pin);
	if(err == NRF_SUCCESS)
	{
		gPairingPinCodeCheck = false;
		critical_pin = 0;
		err = sd_nvic_critical_region_exit(critical_pin);
		if(err != NRF_SUCCESS)
		{
			//nothing to do
		}
	}
}

/**
 * @brief pairing pincode check flag set.
 * @param None
 * @retval None
 */
void PinCodeCheckFlagSet(void)
{
	uint32_t err;
	uint8_t critical_pin;
	
	err = sd_nvic_critical_region_enter(&critical_pin);
	if(err == NRF_SUCCESS)
	{
		gPairingPinCodeCheck = true;
		critical_pin = 0;
		err = sd_nvic_critical_region_exit(critical_pin);
		if(err != NRF_SUCCESS)
		{
			//nothing to do
		}
	}
}

/**
 * @brief Get access pin code flag
 * @param None
 * @retval true 
 * @retval false
 */
bool AccessPinCodeFlagGet(void)
{
	return gAccessPinCodeFlag;
}

/**
 * @brief RTC interrupt, daily log write. 
 * @param pEvent Event Information
 * @retval 0 Success
 */
uint32_t IntRtcNonBle(PEVT_ST pEvent)
{
	DATE_TIME ram_dateTime;
	DATE_TIME rtc_dateTime;
	ret_code_t rtc_err;
	bool       bPastRet;
	DEBUG_LOG(LOG_INFO,"Non BLE Write daily log with RTC INT");
	
	memset(&ram_dateTime,0x00,sizeof(ram_dateTime));
	memset(&rtc_dateTime,0x00,sizeof(rtc_dateTime));
	
	rtc_err = ExRtcGetDateTime(&rtc_dateTime);
	if(rtc_err == NRF_SUCCESS)
	{
		// No!! check rtc time, compare ram save region.
		bPastRet = CheckCurrTime(&rtc_dateTime);
		if(bPastRet == CHECK_VALID)
		{
			DEBUG_LOG(LOG_INFO,"Non BLE Write daily log with RTC INT");
			GetRamTime(&ram_dateTime);
			SetDailyLog(&ram_dateTime);
		}	
		InitRamDateSet(&rtc_dateTime);
	}
	else
	{
		//add modify one hour time set
		TRACE_LOG(TR_RTC_CNT_INT_SET_TIME_ERROR, 0);
		GetRamTime(&ram_dateTime);	//add 180904
		WriteRamOneHourProgressTime(&ram_dateTime);
	}
	
	/* 2022.06.13 Add RTC割込を有効化 ++ */
	ExRtcEnableInterrupt();
	/* 2022.06.13 Add RTC割込を有効化 -- */
	
	return 0;
}

/**
 * @brief Binary Data Change RTC Time
 * @param pDateTime RTCから取得した時刻
 * @param pResult 変換した結果
 * @retval None
 */
void TimeDataFormatReverse(DATE_TIME *pDateTime, uint8_t *pResult)
{
	pDateTime->year  = *(pResult + YEAR_HIGH)  * 10  + *(pResult + YEAR_LOW);
	pDateTime->month = *(pResult + MONTH_HIGH) * 10  + *(pResult + MONTH_LOW);
	pDateTime->day   = *(pResult + DAY_HIGH)   * 10  + *(pResult + DAY_LOW);
	pDateTime->hour  = *(pResult + HOUR_HIGH)  * 10  + *(pResult + HOUR_LOW);
	pDateTime->min   = *(pResult + MIN_HIGH)   * 10  + *(pResult + MIN_LOW);
	pDateTime->sec   = *(pResult + SEC_HIGH)   * 10  + *(pResult + SEC_LOW);
}

/**
 * @brief Run Algorithm
 * @param pEvent Event Information
 * @retval 0 Success
 */
uint32_t RunAlgo(PEVT_ST pEvent)
{
	UNUSED_PARAMETER(*pEvent);
	
	ACC_GYRO_DATA_INFO acc_gyro_data = {0};
	uint8_t i;
	int16_t retainNo;
	int8_t algo_ret;
	ALTCOUNT ultCountRes;
	SKYJUMPCOUNT ultTimeRes;
	DAILYCOUNT walkCount;
	uint8_t walk_result[WALK_COUNT_DATA_SIZE];
	ble_gatts_hvx_params_t notify_data;
	/*test raw data*/
	uint32_t err_code;
	uint16_t cnt_handle = BLE_CONN_HANDLE_INVALID;
	uint16_t notify_size;
	uint16_t id_handle;
	static bool display_log = false;
	/* 2022.05.19 Add 角度調整 ++ */
	ACC_ANGLE acc_angle_info = {0};
	ACC_RESULT acc_angle_result = {0};
	uint8_t notify_signal;
	/* 2022.05.19 Add 角度調整 -- */
#ifdef TEST_FIFO_COUNT_NOTIFY
	uint16_t fifo_index = 0;
#endif
	/* 2020.12.08 Add */

#if LOG_LEVEL_MODE_MGR <= LOG_LEVEL		/* 2020.10.26 Add LOG_LEVELで出力するかどうかを決定する */
	uint8_t buffer[64] = {0};
#endif
	/* 2020.12.09 Add FIFO Countが0以上の場合FIFOをクリアする ++ */
	AccGyroValidateClearFifo();
	/* 2020.12.09 Add FIFO Countが0以上の場合FIFOをクリアする -- */
	
	/* 2020.12.08 Add 再送処理修正 ++ */
	if ( ( g_notify_failed == true ) && ( gpCurrOPmode->currOP_id == RAW_MODE ) )
	{
		/*Notify up*/
		/*retry proccess nashi*/
		GetBleCntHandle( &cnt_handle );
		/*notify up*/
		GetGattsCharHandleValueID( &id_handle, RAW_DATA_ID );
		
		notify_size = sizeof( gRawData );
		notify_data.handle	= id_handle;
		notify_data.type	= BLE_GATT_HVX_NOTIFICATION;
		notify_data.offset	= BLE_NOTIFY_OFFSET;
		notify_data.p_len	= &notify_size;
		notify_data.p_data	= (uint8_t *)&gRawData;
		
		err_code = sd_ble_gatts_hvx( cnt_handle, &notify_data );
		if( ( err_code == NRF_ERROR_BUSY ) || ( err_code == NRF_ERROR_RESOURCES ) )
		{
			/* 失敗した場合はすぐに抜ける */
			return 0;
		}
		g_notify_failed = false;
		memset( (void *)&gRawData, 0, sizeof( gRawData ) );
	}
	/* 2020.12.08 Add 再送処理修正 -- */
	
	gpResult = &walk_result;
	for(i = 0; (NRF_ERROR_NULL != AccGyroPopFifo(&acc_gyro_data)); i++)
	{
		if(gpCurrOPmode->currOP_id == CALIB_MEAS_MODE)
		{
			/*calibration process add*/
			DEBUG_LOG(LOG_DEBUG,"aaaa");
			calib_calculate_value(&acc_gyro_data);
		}
		else if(gpCurrOPmode->currOP_id == CALIB_EXE_MODE)
		{
			/*Do not nothing*/
		}
		/* RAW Mode時にはここに来る */
		else if(gpCurrOPmode->currOP_id == RAW_MODE)
		{
			if(gpCurrOPmode->running == false)
			{
				if ( display_log == false )
				{
					DEBUG_LOG( LOG_INFO, "!!! Start Trigger Not yet !!!" );
					display_log = true;
				}
				/* 2020.12.08 Add FIFO Count Sub ++ */
				AccGyroSubFifoCount();
				/* 2020.12.08 Add FIFO Count Sub -- */
				/* 2020.12.09 Add SIDクリア追加 ++ */
				g_send_sid_base = 0;
				g_send_sid_init = false;
				/* 2020.12.09 Add SIDクリア追加 -- */
				continue;
			}
			/* 初期のSIDを決定 */
			if ( ( g_send_sid_init == false ) && ( g_send_sid_base == 0 ) )
			{
				/* 2022.03.17 Add 送信SID処理を変更 ++ */
				if ( acc_gyro_data.sid != 0 )
				{
					/* SIDが0以外の場合に更新 */
					g_send_sid_base = acc_gyro_data.sid;
				}
				/* 一度、ここに入った場合はフラグを立てて入らないようにする */
				g_send_sid_init = true;
				/* 2022.03.17 Add 送信SID処理を変更 -- */
			}
			/* 2020.12.08 Add BLE再送処理追加 ++ */
			g_notify_failed = false;
			/* 2020.12.08 Add BLE再送処理追加 -- */
			display_log = false;
			/* 2020.10.28 Modify RAW Data送信処理を修正 ++ */
			/* Timestamp */
			memcpy( (void *)&gRawData[0], &acc_gyro_data.timestamp, sizeof( uint16_t ) );
			/* Header */
			gRawData[2] = AccGyroGetSendHeader( acc_gyro_data.header );
			/* SIDを格納 */
			/* 2020.12.09 Modify SIDはSensorからのデータ格納時に付加したものを使用する用に修正 ++ */
			gRawBox.sid = acc_gyro_data.sid - g_send_sid_base;
			/* 2020.12.09 Modify SIDはSensorからのデータ格納時に付加したものを使用する用に修正 -- */
			gAlgoSid++;
			
			gRawBox.acc_x_data		= acc_gyro_data.acc_x_data;			/* ACC X-Axis */
			gRawBox.acc_y_data		= acc_gyro_data.acc_y_data;			/* ACC Y-Axis */
			gRawBox.acc_z_data		= acc_gyro_data.acc_z_data;			/* ACC Z-Axis */
			gRawBox.gyro_x_data		= acc_gyro_data.gyro_x_data;		/* Gyro X-Axis */
			gRawBox.gyro_y_data		= acc_gyro_data.gyro_y_data;		/* Gyro Y-Axis */
			gRawBox.gyro_z_data		= acc_gyro_data.gyro_z_data;		/* Gyro Z-Axis */
			gRawBox.temperature		= acc_gyro_data.temperature;		/* 温度 */
			
#ifdef TEST_FIFO_COUNT_NOTIFY
			/* Test FIFO Count Set */
			fifo_index = AccGyroSubFifoCount();
			gRawBox.acc_z_data = fifo_index;
#else
			/* 2020.12.08 Add FIFO Count Sub ++ */
			AccGyroSubFifoCount();
			/* 2020.12.08 Add FIFO Count Sub -- */
#endif

			/* 送信データ作成
			 *  "-4"をしているのはHeader(1)とCheckSum(1)、Timestamp(2)を省くデータをコピーするため
			 */
			memcpy( (void *)&gRawData[3], (void *)&gRawBox, sizeof( gRawData ) - 4 );
			/*bcc create*/
			gRawData[RAW_DATA_SIZE - 1] = bcc_create( (void *)&gRawData[0], sizeof( gRawData ) - 1 );;
			/* 2020.10.28 Modify RAW Data送信処理を修正 -- */
#if 0
			sprintf( (char *)buffer, "%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\r\n",
				gRawData[0], gRawData[1], gRawData[2], gRawData[3], gRawData[4], gRawData[5],
				gRawData[6], gRawData[7], gRawData[8], gRawData[9], gRawData[10], gRawData[11],
				gRawData[12], gRawData[13], gRawData[14], gRawData[15], gRawData[16], gRawData[17],
				gRawData[18], gRawData[19] );
			DEBUG_LOG_DIRECT( (char *)buffer, strlen( (char *)buffer ) );
#endif
			/*Notify up*/
			/*retry proccess nashi*/
			GetBleCntHandle( &cnt_handle );
			/*notify up*/
			GetGattsCharHandleValueID( &id_handle, RAW_DATA_ID );
			
			notify_size = sizeof( gRawData );
			notify_data.handle	= id_handle;
			notify_data.type	= BLE_GATT_HVX_NOTIFICATION;
			notify_data.offset	= BLE_NOTIFY_OFFSET;
			notify_data.p_len	= &notify_size;
			notify_data.p_data	= (uint8_t *)&gRawData;

			/* 2020.12.08 Add 再送処理修正 ++ */
			err_code = sd_ble_gatts_hvx( cnt_handle, &notify_data );
			if( ( err_code == NRF_ERROR_BUSY ) || ( err_code == NRF_ERROR_RESOURCES ) )
			{
				/* 送信がエラーの場合、gRawDataをクリアせずに抜ける */
				g_notify_failed = true;
				break;
			}
			/* 2020.12.08 Add 再送処理修正 -- */
			memset( (void *)&gRawData, 0, sizeof( gRawData ) );
		}
		else
		{
			/* 2022.05.19 Add 角度調整 ++ */
			if ( g_angle_adjust_state == ANGLE_ADJUST_START )
			{
				/* 角度補正実行 */
				err_code = clac_acc_angle( acc_gyro_data.acc_x_data, acc_gyro_data.acc_y_data, acc_gyro_data.acc_z_data, &acc_angle_info );
				if ( err_code == NRF_SUCCESS )
				{
					if ( acc_angle_info.cmpl == true )
					{
						/* 補正データ保存 */
						SetAngleAdjustInfo( &acc_angle_info );
#if 1
						uint8_t buffer[128] = {0};
						sprintf( (char *)buffer, "roll: %f, pitch: %f, yaw: %f", acc_angle_info.roll, acc_angle_info.pitch, acc_angle_info.yaw );
						DEBUG_LOG( LOG_INFO, "%s", buffer );
#endif
						/* Disableに変更 */
						ChangeAngleAdjustState( ANGLE_ADJUST_STOP );
						
						/* BLE Response */
						notify_signal = BLE_NOTIFY_SIG;
						BleNotifySignals( &notify_signal, (uint16_t)sizeof( notify_signal ) );
					}
				}
			}
			/* 2022.05.19 Add 角度調整 -- */
			
			/* 2022.03.18 Add 計算処理追加(RAW Data送信中は計算を使用しないため) ++ */
			AccGyroCalcData( &acc_gyro_data );
			/* 2022.03.18 Add 計算処理追加(RAW Data送信中は計算を使用しないため) -- */
			/* 2020.12.08 Add FIFO Count Sub ++ */
			AccGyroSubFifoCount();
			/* 2020.12.08 Add FIFO Count Sub -- */
			/* 2020.10.26 Add ACCデータ以外のデータが到来した場合無視する ++ */
			if ( ( acc_gyro_data.header != HEADER_ACC_DATA ) && ( acc_gyro_data.header != HEADER_ACC_GYRO_DATA) &&
				 ( acc_gyro_data.header != HEADER_ACC_LP_DATA ) )
			{
				/* ここに入る場合、Raw Mode以外でGyroやTempのデータが有効になっている可能性がある */
				DEBUG_LOG( LOG_ERROR, "Non ACC Data Header=0x%x", acc_gyro_data.header );
				continue;
			}
			/* 2020.10.26 Add ACCデータ以外のデータが到来した場合無視する -- */

			/* 2022.05.19 Add 角度調整 ++ */
			if ( g_angle_adjust_state == ANGLE_ADJUST_ENABLE )
			{
				calc_acc_rot( acc_gyro_data.acc_x_data, acc_gyro_data.acc_y_data, acc_gyro_data.acc_z_data, &acc_angle_result );
				acc_gyro_data.acc_x_data = acc_angle_result.x;
				acc_gyro_data.acc_y_data = acc_angle_result.y;
				acc_gyro_data.acc_z_data = acc_angle_result.z;
				memset( &acc_angle_result, 0, sizeof( acc_angle_result ) );
			}
			/* 2022.05.19 Add 角度調整 -- */

#if LOG_LEVEL_MODE_MGR <= LOG_LEVEL		/* 2020.10.26 Add LOG_LEVELで出力するかどうかを決定する */
			sprintf( (char *)buffer, "acc data %d, x %d, y %d, z %d\r\n", i, (acc_gyro_data.acc_x_data - gAccXoffset),(acc_gyro_data.acc_y_data - gAccYoffset),(acc_gyro_data.acc_z_data - gAccZoffset) );
			DEBUG_LOG_DIRECT( (char *)buffer, strlen( (char *)buffer ) );
			//DEBUG_LOG(LOG_DEBUG,"acc data %u, x %d, y %d, z %d", i, (acc_gyro_data.acc_gyro_x_data - gAccXoffset),(acc_gyro_data.acc_gyro_y_data - gAccYoffset),(acc_gyro_data.acc_gyro_z_data - gAccZoffset));
#endif
			/* 2020.11.26 Add ACCデータのX,Y Axisが反転しているため-1をかけてアルゴリズムへ渡すように修正 ++ */
			algo_ret = GetWalkResult((acc_gyro_data.acc_x_data - gAccXoffset) * -1, (acc_gyro_data.acc_z_data - gAccZoffset), gAlgoSid, gpCurrOPmode->currOP_id, &retainNo, gpResult);
			/* 2020.11.26 Add ACCデータのX,Y Axisが反転しているため-1をかけてアルゴリズムへ渡すように修正 -- */
			gAlgoSid++;
			if(1 == algo_ret)
			{
				if(gpCurrOPmode->currOP_id == DAILY_MODE)
				{
					memcpy(&walkCount,gpResult,sizeof(walkCount));
					DEBUG_LOG(LOG_DEBUG,"cnt walk %u, run %u, dash %u",walkCount.walk,walkCount.run,walkCount.dash);
					if((walkCount.walk + walkCount.run + walkCount.dash) != NO_COUNT_WALK)
					{
						WalkRamSet(gpResult);								//ram set walk result
					}
				}
			
				if((gpCurrOPmode->currOP_id == TAP_STP_MODE) || (gpCurrOPmode->currOP_id == TELEP_MODE) || (gpCurrOPmode->currOP_id == SIDE_AGI_MODE))
				{
					memcpy(&ultCountRes, gpResult, sizeof(ultCountRes));
					if((2 < gpCurrOPmode->currTrigger) && (gpCurrOPmode->currOP_id == TAP_STP_MODE) && (gpCurrOPmode->residual_count == false))
					{
						DEBUG_LOG(LOG_DEBUG,"3.tap count plus, max count %u, trigg count %d, curr trigg count %u, result count %u",gpCurrOPmode->max_trigger_count, gpCurrOPmode->trigger_count,gpCurrOPmode->currTrigger,gpCurrOPmode->RESULTDATA.count[gpCurrOPmode->trigger_count - 1]);
						gpCurrOPmode->RESULTDATA.count[gpCurrOPmode->trigger_count - 1] += ultCountRes.alt;
						DEBUG_LOG(LOG_DEBUG,"4.tap count plus, max count %u, tap currCount %u, result count %u",gpCurrOPmode->max_trigger_count, ultCountRes.alt,gpCurrOPmode->RESULTDATA.count[gpCurrOPmode->trigger_count - 1]);
						gpCurrOPmode->residual_count = true;
					}
					else
					{
						DEBUG_LOG(LOG_DEBUG,"0.tap count plus, max count %u, trigg count %d, curr trigg count %u, result count %u",gpCurrOPmode->max_trigger_count, gpCurrOPmode->trigger_count,gpCurrOPmode->currTrigger,gpCurrOPmode->RESULTDATA.count[gpCurrOPmode->trigger_count]);
						if((gpCurrOPmode->currOP_id == TAP_STP_MODE)&&(MAX_TRIGGER_COUNT_TAP_STP_MODE <= gpCurrOPmode->trigger_count))
						{
							DEBUG_LOG(LOG_DEBUG,"a");
						
							gpCurrOPmode->RESULTDATA.count[MAX_TRIGGER_COUNT_TAP_STP_MODE - 1] += ultCountRes.alt;
							DEBUG_LOG(LOG_DEBUG,"1.tap count plus, max count %u, tap currCount %u, result count %u",gpCurrOPmode->max_trigger_count, ultCountRes.alt,gpCurrOPmode->RESULTDATA.count[MAX_TRIGGER_COUNT_TAP_STP_MODE - 1]);
						}
						else
						{
							DEBUG_LOG(LOG_DEBUG,"b");
							gpCurrOPmode->RESULTDATA.count[gpCurrOPmode->trigger_count] += ultCountRes.alt;
							DEBUG_LOG(LOG_DEBUG,"2.tap count plus, max count %u, tap currCount %u, result count %u",gpCurrOPmode->max_trigger_count, ultCountRes.alt,gpCurrOPmode->RESULTDATA.count[gpCurrOPmode->trigger_count]);
						}
					}
				
					//end trigger residual count increments
					//case of tap stamp
					if(gpCurrOPmode->currOP_id == TAP_STP_MODE && ((gpCurrOPmode->currTrigger == 10) || (gpCurrOPmode->currTrigger == ULT_END_TRIGGER)))
					{
						gpCurrOPmode->residual_count = true;
					}
				
					//case of side and telepo
					if((gpCurrOPmode->currOP_id == TELEP_MODE || gpCurrOPmode->currOP_id == SIDE_AGI_MODE) && (gpCurrOPmode->currTrigger == ULT_END_TRIGGER))
					{
						gpCurrOPmode->residual_count = true;
					}
				}
		
				if(gpCurrOPmode->currOP_id == SKY_JUMP_MODE)
				{
					memcpy(&ultTimeRes, gpResult, sizeof(ultTimeRes));
					if(gpCurrOPmode->first_sid == true && (0 < ultTimeRes.alt))
					{
						DEBUG_LOG(LOG_INFO,"sky jump time %u, start sid %u", ultTimeRes.alt, ultTimeRes.sid);
						gpCurrOPmode->RESULTDATA.time[gpCurrOPmode->trigger_count] = ultTimeRes.alt;
						gpCurrOPmode->first_sid = false;
						DEBUG_LOG(LOG_INFO,"ult times %u",gpCurrOPmode->RESULTDATA.time[gpCurrOPmode->trigger_count]);
					}
				}
			
				if(gpCurrOPmode->currOP_id == START_REAC_MODE)
				{
					memcpy(&ultCountRes, gpResult, sizeof(ultCountRes));
					DEBUG_LOG(LOG_INFO,"start sid %u, end sid %u",gpCurrOPmode->ult_move_time,ultCountRes.alt);
					if(gpCurrOPmode->first_sid == true && (0 < ultCountRes.alt))
					{
						gpCurrOPmode->RESULTDATA.time[gpCurrOPmode->trigger_count] = ((int16_t)ultCountRes.alt - (int16_t)gpCurrOPmode->ult_move_time) * START_REACTION_TIME_UNIT_10MS;		//sky jmp time cluculation [ms]
						DEBUG_LOG(LOG_INFO,"start reaction time %d", gpCurrOPmode->RESULTDATA.time[gpCurrOPmode->trigger_count]);
						gpCurrOPmode->first_sid = false;
					}
				}
			}
		}
	}
	/* 2020.10.26 Add ACC/Gyro Interrupt Enable ++ */
	//AccGyroEnableGpioInt( ACC_INT1_PIN );
	/* 2020.10.26 Add ACC/Gyro Interrupt Enable -- */

	return 0;
}

/**
 * @brief Algorithm Data Reset
 * @param None
 * @retval None
 */
void AlgoDataReset(void)
{
	gAlgoSid = 0;
	g_temp_sid = 0;
	StorageReset();
}

/**
 * @brief statement, other than connect, evt, EVT_FIFO_INT,
 *        Run Algorithm Pre Deep Sleep
 * @param pEvent Event Information
 * @retval 0 Success
 */
uint32_t RunAlgoPreDeepSleep(PEVT_ST pEvent)
{
	int16_t retainNo;
	uint32_t err_fifo;
	int8_t algo_ret;
	uint8_t i;
	ACC_GYRO_DATA_INFO acc_gyro_data = {0};
	DAILYCOUNT result;
	EVT_ST evt;
	/* 2022.03.18 Add ADV判定処理追加 ++ */
	uint32_t lateral_detect = LATERARL_ACC_NO_DETECT;
	/* 2022.03.18 Add ADV判定処理追加 -- */
	
	/* 2022.05.19 Add 角度調整 ++ */
	ACC_RESULT acc_angle_result = {0};
	/* 2022.05.19 Add 角度調整 -- */
#if LOG_LEVEL_MODE_MGR <= LOG_LEVEL		/* 2020.10.26 Add LOG_LEVELで出力するかどうかを決定する */
	uint8_t buffer[64] = {0};
#endif

	for(i = 0; (NRF_ERROR_NULL != AccGyroPopFifo(&acc_gyro_data)); i++)
	{
		/* 2020.11.11 Add 計算処理追加 ++ */
		AccGyroCalcData( &acc_gyro_data );
		/* 2020.11.11 Add 計算処理追加 -- */
		/* 2020.12.08 Add FIFO Count Sub ++ */
		AccGyroSubFifoCount();
		/* 2020.12.08 Add FIFO Count Sub -- */

		/* 2020.10.26 Add ACCデータ以外のデータが到来した場合無視する ++ */
		if ( ( acc_gyro_data.header != HEADER_ACC_DATA ) && ( acc_gyro_data.header != HEADER_ACC_GYRO_DATA) &&
			 ( acc_gyro_data.header != HEADER_ACC_LP_DATA ) )
		{
			/* ここに入る場合、Raw Mode以外でGyroやTempのデータが有効になっている可能性がある */
			DEBUG_LOG( LOG_INFO,"Non ACC Data Header=0x%x", acc_gyro_data.header );
			continue;
		}
		/* 2020.10.26 Add ACCデータ以外のデータが到来した場合無視する -- */

		/* 2022.05.19 Add 角度調整 ++ */
		if ( g_angle_adjust_state == ANGLE_ADJUST_ENABLE )
		{
			calc_acc_rot( acc_gyro_data.acc_x_data, acc_gyro_data.acc_y_data, acc_gyro_data.acc_z_data, &acc_angle_result );
			acc_gyro_data.acc_x_data = acc_angle_result.x;
			acc_gyro_data.acc_y_data = acc_angle_result.y;
			acc_gyro_data.acc_z_data = acc_angle_result.z;
			memset( &acc_angle_result, 0, sizeof( acc_angle_result ) );
		}
		/* 2022.05.19 Add 角度調整 -- */
		
		/* 2022.03.18 Add ADV判定処理追加 ++ */
		lateral_detect = LateralAxisWakeup(gAlgoSid, acc_gyro_data.acc_y_data );
		if ( lateral_detect == LATERARL_ACC_DETECT )
		{
			/* 検出した際には、ADVイベントを発行する */
			evt.evt_id = EVT_INIT_CMPL;
			err_fifo = PushFifo(&evt);
			DEBUG_EVT_FIFO_LOG(err_fifo,evt.evt_id);
		}
		/* 2022.03.18 Add ADV判定処理追加 -- */

#if LOG_LEVEL_MODE_MGR <= LOG_LEVEL		/* 2020.10.26 Add LOG_LEVELで出力するかどうかを決定する */
		sprintf( (char *)buffer, "acc data %u, x %d, y %d, z %d, %u\r\n", i, (acc_gyro_data.acc_x_data - gAccXoffset),(acc_gyro_data.acc_y_data - gAccYoffset),(acc_gyro_data.acc_z_data - gAccZoffset), gAlgoSid );
		DEBUG_LOG_DIRECT( (char *)buffer, strlen( (char *)buffer ) );
		//DEBUG_LOG(LOG_DEBUG,"acc data %u, x %d, y %d, z %d, %u ", i, (acc_gyro_data.acc_gyro_x_data - gAccXoffset),(acc_gyro_data.acc_gyro_y_data - gAccYoffset),(acc_gyro_data.acc_gyro_z_data - gAccZoffset), gAlgoSid);
#endif
		/* 2020.11.26 Add ACCデータのX,Y Axisが反転しているため-1をかけてアルゴリズムへ渡すように修正 ++ */
		algo_ret = GetWalkResult((acc_gyro_data.acc_x_data - gAccXoffset) * -1, (acc_gyro_data.acc_z_data - gAccZoffset), gAlgoSid, gpCurrOPmode->currOP_id, &retainNo, &result);
		/* 2020.11.26 Add ACCデータのX,Y Axisが反転しているため-1をかけてアルゴリズムへ渡すように修正 -- */
		gAlgoSid++;
		/*no walk count*/
		if(algo_ret == 1)
		{
			DEBUG_LOG(LOG_DEBUG,"w %u, r %u, d %u", result.walk, result.run, result.dash);
			if((result.walk + result.run + result.dash) == NO_COUNT_WALK)
			{
				walk_timeout_inc();
			}
			else
			{
				WalkRamSet(&result);
				WalkTimeOutClear();
			}
		}
		if(NO_WALK_TIMEOUT < gNo_walk_sec)
		{
			evt.evt_id = EVT_NO_WALK_TO;
			err_fifo = PushFifo(&evt);
			DEBUG_EVT_FIFO_LOG(err_fifo,evt.evt_id);
		}
	}
	
	/* 2020.10.26 Add ACC/Gyro Interrupt Enable ++ */
	//AccGyroEnableGpioInt( ACC_INT1_PIN );
	/* 2020.10.26 Add ACC/Gyro Interrupt Enable -- */
	
	return 0;
}

/**
 * @brief Run Algorithm ADV
 * @param pEvent Event Information
 * @retval 0 Success
 */
uint32_t RunAlgoAdv(PEVT_ST pEvent)
{
	int16_t retainNo;
	int8_t algo_ret;
	uint8_t i;
	ACC_GYRO_DATA_INFO acc_gyro_data = {0};
	DAILYCOUNT result;
	/* 2022.05.19 Add 角度調整 ++ */
	ACC_RESULT acc_angle_result = {0};
	/* 2022.05.19 Add 角度調整 -- */
#if LOG_LEVEL_MODE_MGR <= LOG_LEVEL		/* 2020.10.26 Add LOG_LEVELで出力するかどうかを決定する */
	uint8_t buffer[64] = {0};
	static uint16_t time_proc = 0;
	volatile uint16_t time_result = 0;
	volatile uint16_t time_tmep_result = 0;
#endif

	for(i = 0; (NRF_ERROR_NULL != AccGyroPopFifo(&acc_gyro_data)); i++)
	{
		/* 2020.11.11 Add 計算処理追加 ++ */
		AccGyroCalcData( &acc_gyro_data );
		/* 2020.11.11 Add 計算処理追加 -- */

		/* 2020.12.08 Add FIFO Count Sub ++ */
		AccGyroSubFifoCount();
		/* 2020.12.08 Add FIFO Count Sub -- */
		
		/* 2020.10.26 Add ACCデータ以外のデータが到来した場合無視する ++ */
		if ( ( acc_gyro_data.header != HEADER_ACC_DATA ) && ( acc_gyro_data.header != HEADER_ACC_GYRO_DATA) &&
			 ( acc_gyro_data.header != HEADER_ACC_LP_DATA ) )
		{
			/* ここに入る場合、Raw Mode以外でGyroやTempのデータが有効になっている可能性がある */
			DEBUG_LOG( LOG_ERROR,"Non ACC Data Header=0x%x", acc_gyro_data.header );
			continue;
		}
		/* 2020.10.26 Add ACCデータ以外のデータが到来した場合無視する -- */
		/* 2022.05.19 Add 角度調整 ++ */
		if ( g_angle_adjust_state == ANGLE_ADJUST_ENABLE )
		{
			//DEBUG_LOG(LOG_INFO,"0:%d,%d,%d",acc_gyro_data.acc_x_data,acc_gyro_data.acc_y_data,acc_gyro_data.acc_z_data);
			calc_acc_rot( acc_gyro_data.acc_x_data, acc_gyro_data.acc_y_data, acc_gyro_data.acc_z_data, &acc_angle_result );
			acc_gyro_data.acc_x_data = acc_angle_result.x;
			acc_gyro_data.acc_y_data = acc_angle_result.y;
			acc_gyro_data.acc_z_data = acc_angle_result.z;
			//DEBUG_LOG(LOG_INFO,"1:%d,%d,%d",acc_gyro_data.acc_x_data,acc_gyro_data.acc_y_data,acc_gyro_data.acc_z_data);
			memset( &acc_angle_result, 0, sizeof( acc_angle_result ) );
		}
		/* 2022.05.19 Add 角度調整 -- */
		
#if LOG_LEVEL_MODE_MGR <= LOG_LEVEL		/* 2020.10.26 Add LOG_LEVELで出力するかどうかを決定する */
		time_tmep_result = acc_gyro_data.timestamp - time_proc;
		time_result = time_tmep_result * 16;
		time_proc = acc_gyro_data.timestamp;
		sprintf( (char *)buffer, "acc data %u, tm %x, x %d, y %d, z %d, %u\r\n", i, acc_gyro_data.timestamp, (acc_gyro_data.acc_x_data - gAccXoffset), (acc_gyro_data.acc_y_data - gAccYoffset),(acc_gyro_data.acc_z_data - gAccZoffset), gAlgoSid );
		//sprintf( (char *)buffer, "time: %d timestamp: %d, %d\r\n", time_result, acc_gyro_data.timestamp, time_tmep_result );
		DEBUG_LOG_DIRECT( (char *)buffer, strlen( (char *)buffer ) );
		//DEBUG_LOG(LOG_DEBUG,"acc data %u, x %d, y %d, z %d, %u ", i, (acc_gyro_data.acc_gyro_x_data - gAccXoffset),(acc_gyro_data.acc_gyro_y_data - gAccYoffset),(acc_gyro_data.acc_gyro_z_data - gAccZoffset), gAlgoSid);
#endif
		/* 2020.11.26 Add ACCデータのX,Y Axisが反転しているため-1をかけてアルゴリズムへ渡すように修正 ++ */
		algo_ret = GetWalkResult((acc_gyro_data.acc_x_data - gAccXoffset) * -1, (acc_gyro_data.acc_z_data - gAccZoffset), gAlgoSid, gpCurrOPmode->currOP_id, &retainNo, &result);
		/* 2020.11.26 Add ACCデータのX,Y Axisが反転しているため-1をかけてアルゴリズムへ渡すように修正 -- */
		gAlgoSid++;
		/*no walk count*/
		if(algo_ret == 1)
		{
			DEBUG_LOG(LOG_DEBUG,"w %u, r %u, d %u", result.walk, result.run, result.dash);
			if((result.walk + result.run + result.dash) != NO_COUNT_WALK)
			{
				WalkRamSet(&result);
			}
		}
	}
	
	/* 2020.10.26 Add ACC/Gyro Interrupt Enable ++ */
	//AccGyroEnableGpioInt( ACC_INT1_PIN );
	/* 2020.10.26 Add ACC/Gyro Interrupt Enable -- */
	
	return 0;
}

/**
 * @brief Clear Walk Timeout
 * @param None
 * @retval None
 */
void WalkTimeOutClear(void)
{
	uint32_t err;
	uint8_t  critical_walk;
	
	err = sd_nvic_critical_region_enter(&critical_walk);
	if(err == NRF_SUCCESS)
	{
		gNo_walk_sec = 0;
		critical_walk = 0;
		err = sd_nvic_critical_region_exit(critical_walk);
		if(err != NRF_SUCCESS)
		{
			//nothing to do.
		}
	}
	DEBUG_LOG(LOG_DEBUG,"walk count clear %u", gNo_walk_sec);
}

/**
 * @brief set daily log function
 * @param dateTime Date TIme
 * @retval 0 Success
 */
uint32_t SetDailyLog(DATE_TIME *dateTime)
{
	uint16_t sid;
	Daily_t daily_data;
	uint8_t overCount;
	uint8_t id_case;
	int8_t err;
	
	DEBUG_LOG(LOG_DEBUG,"Enter write daily log");

	GetRamDailyLog(&daily_data);
	DEBUG_LOG(LOG_DEBUG,"set daily log w %u, r %u, d %u",daily_data.walk, daily_data.run, daily_data.dash);
	
	overCount = GetSidOverCount();
	sid = daily_data.sid;
	sid = sid % FLASH_DATA_MAX;

	if(sid < FLASH_DATA_CENTER)
	{
		id_case = 0;
	}
	else
	{
		id_case = 1;
	}
	sid = sid % FLASH_DATA_CENTER;
		
	DEBUG_LOG(LOG_DEBUG,"write daily log enter. sid %u, id_case %u, over count %u",sid, id_case, overCount);
	if((daily_data.walk + daily_data.run + daily_data.dash) <= 0)
	{
		DEBUG_LOG(LOG_INFO,"walk count zero. No write flash");
		__NOP();
	}
	else
	{
		err = FlashWrite(sid, &daily_data, id_case, overCount);
		if(err != 1)
		{
			//trace log
			TRACE_LOG(TR_SET_DAILY_LOG_WRITE_ERROR,0);
			DEBUG_LOG(LOG_ERROR,"daily log flash write Error");
		}
		else
		{
			DEBUG_LOG(LOG_INFO,"daily log flash write success");
			RamSaveSidIncrement();
			SetWriteSid();
			RamSaveRegionWalkInfoClear();
		}
	}
	return 0;
}

/**
 * @brief Get Operation Mode
 * @param pOpMode Operation Mode Code
 * @retval None
 */
void GetOpMode(OP_MODE *pOpMode)
{
	uint32_t err;
	uint8_t op_critical;
	
	err = sd_nvic_critical_region_enter(&op_critical);
	if(err == NRF_SUCCESS)
	{
		*pOpMode = gpCurrOPmode->currOP_id;
		op_critical = 0;
		err = sd_nvic_critical_region_exit(op_critical);
		if(err != NRF_SUCCESS)
		{
			//nothing to do.
		}
	}
}

/**
 * @brief Pre Connect Parameter Update Reject
 * @param pEvent Event Information
 * @retval 0 Success
 */
uint32_t PreCntParamUpdateReject(PEVT_ST pEvent)
{
	StopParamUpdateTimer();
	
	return 0;
}

/**
 * @brief set global var to offset value
 * @param xoffset X軸オフセット
 * @param yoffset Y軸オフセット
 * @param zoffset Z軸オフセット
 * @retval None
 */
void SetAccOffset(int16_t xoffset, int16_t yoffset, int16_t zoffset)
{
	gAccXoffset = xoffset;
	gAccYoffset = yoffset;
	gAccZoffset = zoffset;
	
	//change future
	DEBUG_LOG(LOG_INFO,"Set alog offset gAccXoff %d, gAccYoff %d, gAccZoff %d",gAccXoffset,gAccYoffset,gAccZoffset);
}

/**
 * @brief set acc offset valid
 * @param valid offset valid
 * @retval None
 */
void SetAccOffsetValid(uint8_t valid)
{
	gOffset_valid = valid;
}

/**
 * @brief Get global var to offset value
 * @param xoffset X軸オフセット
 * @param yoffset Y軸オフセット
 * @param zoffset Z軸オフセット
 * @retval None
 */
void GetAccOffset(uint8_t *valid, int16_t *xoffset, int16_t *yoffset, int16_t *zoffset)
{
	*valid   = gOffset_valid;
	*xoffset = gAccXoffset;
	*yoffset = gAccYoffset;
	*zoffset = gAccZoffset;
}

/**
 * @brief Clear Acc Offset Value
 * @param None
 * @retval None
 */
static void clear_tmp_acc_offset(void)
{
	//gOffset_valid = 0;
	gXOffset = DEFAULT_SENSOR_OFFSET;
	gYOffset = DEFAULT_SENSOR_OFFSET;
	gZOffset = DEFAULT_SENSOR_OFFSET;	
}

/**
 * @brief BCC Create
 * @param str 計算するデータ
 * @param len 計算するデータ長
 * @retval BCC_Val XORデータ
 */
static uint8_t bcc_create(uint8_t *str, uint32_t len)
{
	uint8_t BCC_Val = 0;

	while(len--)
	{
		BCC_Val ^= *str;
		str++;
	}
	return BCC_Val;
}

/**
 * @brief Y Axis ADV
 * @param pEvent Event Information
 * @retval 0 Success
 */
uint32_t YAxisAdv(PEVT_ST pEvent)
{
	return 0;
}

/**
 * @brief Get Sensor Data FIFO Clear SID
 * @param None
 * @retval sid Clear SID
 */
uint16_t GetClearSid( void )
{
	uint32_t err;
	uint8_t clear_sid_sec;
	uint16_t sid;
	
	err = sd_nvic_critical_region_enter(&clear_sid_sec);
	if(err == NRF_SUCCESS)
	{
		sid = g_fifo_clear_sid;
		clear_sid_sec = 0;
		err = sd_nvic_critical_region_exit(clear_sid_sec);
		if(err != NRF_SUCCESS)
		{
			//nothing to do.
		}
	}
	return sid;
}

/**
 * @brief ADV Timeout時のADV判定処理リセット
 * @param pEvent Event Information
 * @retval 0 Success
 */
uint32_t AdvTimeoutClearInt(PEVT_ST pEvent)
{
	/* ADV発行フラグを初期化 */
	ClearAdvStatus();
	
	return 0;
}

/**
 * @brief Guest Modeの状態を取得
 * @param None
 * @retval ture Guest Mode ON
 * @retval false Guest Mode OFF
 */
bool GetGuestModeState( void )
{
	uint32_t err_code;
	bool guest_mode = false;
	uint8_t critical_sec_get_guest_mode;
	
	err_code = sd_nvic_critical_region_enter( &critical_sec_get_guest_mode );
	if( err_code == NRF_SUCCESS )
	{
		guest_mode = g_guest_mode;
		critical_sec_get_guest_mode = 0;
		err_code = sd_nvic_critical_region_exit( critical_sec_get_guest_mode );
		if(err_code != NRF_SUCCESS)
		{
			//nothing to do.
		}
	}
	
	return guest_mode;
}

/**
 * @brief Guest Modeの状態を取得
 * @param guest_mode_state Guest Modeの状態
 * @retval None
 */
void SetGuestModeState( bool guest_mode_state )
{
	uint32_t err_code;
	uint8_t critical_sec_set_guest_mode;
	
	err_code = sd_nvic_critical_region_enter( &critical_sec_set_guest_mode );
	if( err_code == NRF_SUCCESS )
	{
		g_guest_mode = guest_mode_state;
		critical_sec_set_guest_mode = 0;
		err_code = sd_nvic_critical_region_exit( critical_sec_set_guest_mode );
		if(err_code != NRF_SUCCESS)
		{
			//nothing to do.
		}
	}
}

/**
 * @brief 角度調整情報設定
 * @param angle_info 角度調整情報
 * @retval None
 */
void SetAngleAdjustInfo( ACC_ANGLE *angle_info )
{
	uint32_t err_code;
	uint8_t critical_sec_set_angle_adjust;
	
	err_code = sd_nvic_critical_region_enter( &critical_sec_set_angle_adjust );
	if( err_code == NRF_SUCCESS )
	{
		g_angle_info = *angle_info;
		critical_sec_set_angle_adjust = 0;
		err_code = sd_nvic_critical_region_exit( critical_sec_set_angle_adjust );
		if(err_code != NRF_SUCCESS)
		{
			//nothing to do.
		}
	}
}

/**
 * @brief 角度調整情報取得
 * @param angle_info 角度調整情報
 * @retval None
 */
void GetAngleAdjustInfo( ACC_ANGLE *angle_info )
{
	uint32_t err_code;
	uint8_t critical_sec_set_angle_adjust;
	
	err_code = sd_nvic_critical_region_enter( &critical_sec_set_angle_adjust );
	if( err_code == NRF_SUCCESS )
	{
		*angle_info = g_angle_info;
		critical_sec_set_angle_adjust = 0;
		err_code = sd_nvic_critical_region_exit( critical_sec_set_angle_adjust );
		if(err_code != NRF_SUCCESS)
		{
			//nothing to do.
		}
	}
}

/**
 * @brief 角度調整状態変更
 * @param state 角度調整状態
 * @retval None
 */
void ChangeAngleAdjustState( uint8_t state )
{
	ACC_ANGLE acc_angle_info = {0};
	
	/* 現在の角度補正値取得 */
	GetAngleAdjustInfo( &acc_angle_info );
	/* ステータスを変更 */
	g_angle_adjust_state = state;
	/* BLEのReadに設定 */
	SetupAccAngleData( &acc_angle_info, state );
}

/**
 * @brief 角度調整状態取得
 * @param state 角度調整状態
 * @retval None
 */
void GetAngleAdjustState( uint8_t *state )
{
	if ( state != NULL )
	{
		/* ステータスを変更 */
		*state = g_angle_adjust_state;
	}
}

/* 2020.12.23 Add RSSI取得テスト ++ */
/**
 * @brief Get RSSI Notify
 * @param pEvent Event Information
 * @retval 0 Success
 */
static uint32_t get_rssi_notify( PEVT_ST pEvent )
{
	uint32_t err_code;
	uint16_t cnt_handle = BLE_CONN_HANDLE_INVALID;
	uint16_t notify_size = 2;
	uint16_t id_handle;
	int16_t rssi = 0;
	ble_gatts_hvx_params_t notify_data;

	GetRssiValue( &rssi );

	/*retry proccess nashi*/
	GetBleCntHandle( &cnt_handle );
	/* notify */
	GetGattsCharHandleValueID( &id_handle, RSSI_NOTIFY_ID );
	
	notify_data.handle	= id_handle;
	notify_data.type	= BLE_GATT_HVX_NOTIFICATION;
	notify_data.offset	= BLE_NOTIFY_OFFSET;
	notify_data.p_len	= &notify_size;
	notify_data.p_data	= (uint8_t *)&rssi;
	err_code = sd_ble_gatts_hvx( cnt_handle, &notify_data );
	if ( ( err_code == NRF_ERROR_TIMEOUT ) || ( err_code == NRF_ERROR_RESOURCES ) )
	{
		g_force_disconnect_count++;
		/* 3回失敗した際に強制切断を実行(5秒 x 3 = 15秒) */
		if ( g_force_disconnect_count >= BLE_FORCE_DISCONNECT_COUNT )
		{
			DEBUG_LOG( LOG_ERROR, "!!! RSSI Error Force Disconnect !!!" );
			g_force_disconnect_count = 0;
			/* 強制切断 */
			GetBleCntHandle( &cnt_handle );
			sd_ble_gap_disconnect( cnt_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE );
		}
	}
	else
	{
		/* 正常に戻ったらカウントをクリア */
		g_force_disconnect_count = 0;
	}

	DEBUG_LOG( LOG_INFO, "RSSI=%d err_code=0x%x", rssi, err_code );
	
	return 0;
}
/* 2020.12.23 Add RSSI取得テスト -- */

/* 2022.07.22 Add Guestモード中にRTC割り込みがあった場合の処理を追加 ++ */
/**
 * @brief RTC Interruptを処理するとどうかを検証し決定する
 * @param pEvent Event Information
 * @retval 0 Success
 */
uint32_t ValidateRtcInterrupt( PEVT_ST pEvent )
{
	uint8_t eventId;
	bool guest_mode_enable = false;
	
	if ( pEvent != NULL )
	{
		eventId = pEvent->evt_id;

		/* ゲストモード有効かどうかを確認 */
		guest_mode_enable = GetGuestModeState();
		if ( guest_mode_enable == true )
		{
			DEBUG_LOG( LOG_INFO, "Enable Guest Mode: 0x%x", pEvent->evt_id );
			/* ゲストモードが有効でモードがRawModeならイベントをペンディング */
			if ( gpCurrOPmode->currOP_id == RAW_MODE )
			{
				DEBUG_LOG( LOG_INFO, "Pending Interrupt RTC Event" );
				raw_data_sending_rtc_proc( pEvent );
			}
			else
			{
				/* IDをチェックし、ble_mode_funcを実行 */
				if ( pEvent->evt_id == EVT_RTC_INT )
				{
					DEBUG_LOG( LOG_INFO, "!!! Exec RTC Event !!!" );
					/* ここはRTC_INTしか来ないはず */
					access_pincode_flag_check((BLE_CMD_EVENT)((eventId + BLE_CMD_OFFSET) & (~BLE_CMD_MASK)), pEvent);
					ble_mode_func( pEvent );
				}
			}
		}
		else
		{
			/* ゲストモードが無効ならイベントをRepushEvent */
			RepushEvent( pEvent );
		}
	}
	
	return 0;
}
/* 2022.07.22 Add Guestモード中にRTC割り込みがあった場合の処理を追加 -- */

/*kono test random id 220809*/
int random_index(void)
{
	
	uint32_t err_code;
	DATE_TIME date_info;
	uint32_t seed = 0;
	static bool first_seed = false;
	if(first_seed == false)
	{
		err_code = ExRtcGetDateTime(&date_info);
		if(err_code == NRF_SUCCESS)
		{
			DEBUG_LOG(LOG_INFO,"random seed set");
			seed =   (((uint32_t)date_info.day)<<24) |(((uint32_t)date_info.hour)<<16)|(((uint32_t)date_info.min) << 8)| (uint32_t)date_info.sec;
		}
		srand(seed);
		first_seed = true;
  }
	
	int num = rand()%6;
	
	if( 5 < num)
	{
		num = 5;
	}
	else if(num < 0)
	{
		num = 0;
	}
	
	DEBUG_LOG(LOG_INFO,"random id = %d",num);
	
	return num;
	
}

void SetRndId(int id)
{
	g_random_id = id;
}

int GetRndId(void)
{
	return g_random_id;
}

/**
 * @brief Pre Connect Parameter Update Reject
 * @param pEvent Event Information
 * @retval 0 Success
 */


/*mode ble cmd operation*/
/**************************************************************************************************/
//					  [OP_mode]  daily,   tap_stamp,   startRection,   skyjump,     telepo,      sideAgility
//[event(CMD)]
//RTC_INT                 OK         resend        resend       resend        resend        resend
//INIT                    OK           NG            NG           NG            NG            NG
//GET_TIME                get        resend        resend       resend        resend        resend
//GET_BAT                 get        resend        resend       resend        resend        resend
//ERASE_LOG_ONE           OK           NG            NG           NG            NG            NG
//ACCEPT_PINCODE_CHECK    OK           NG            NG           NG            NG            NG
//MODE_CHANGE             OK           OK            OK           OK            OK            OK
//PINCODE_CHECK           OK           NG            NG           NG            NG            NG
//PINCODE_ERASE           OK           NG            NG           NG            NG            NG
//PLAYER_INFO_SET         OK           NG            NG           NG            NG            NG
//READ_LOG                OK           NG            NG           NG            NG            NG
//ULT_START_TRIG          NG           OK            OK           OK            OK            OK
//ULT_END_TRIG            NG           OK            OK           OK            OK            OK
//ULT_TRIG                NG           OK            OK           OK            OK            OK


//OK: execute
//NG: return
//resend: PushFifo same cmd(EVT)
/**************************************************************************************************/
