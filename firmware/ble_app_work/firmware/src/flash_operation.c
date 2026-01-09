/**
  ******************************************************************************************
  * @file    flash_operation.c
  * @author  k.tashiro
  * @version 1.0
  * @date    2020/09/17
  * @brief   Flash Function Operation
  ******************************************************************************************
    Revision:
    Ver            Date             Revised by        Explanation
  ******************************************************************************************
  * 1.0            2020/09/17       k.tashiro         create new
  ******************************************************************************************
*/

/* Includes --------------------------------------------------------------*/
#include "flash_operation.h"

/* Definition ------------------------------------------------------------*/
#define EX_FLASH_OP_FILE_ID 0x0000

#define NOTIFY_SHARED
#define EVENT_NO_MODIFY		(0xFF)		/* 通知するイベントを変更しないときに指定する */

/* variables -------------------------------------------------------------*/
extern nrf_fstorage_t ex_fstorage;
extern nrf_fstorage_t ex_fstorage2;
extern nrf_fstorage_t player_pincode_fstorage;
extern uint8_t gFlash_daily_log[PAGE_SIZE];

/* Private variables -----------------------------------------------------*/
volatile uint8_t gPin_player_page[PLAYER_AND_PAIRING_PAGE_SIZE];
static PFLASH_OP gpFlashCurrOp;

/* Private function prototypes -------------------------------------------*/
//flash ble init cmd
static uint32_t init_procees_state_check(PEVT_ST pEvent);
static uint32_t init_player_flash_flag_erase(PEVT_ST pEvent);
static uint32_t init_wait_write(PEVT_ST pEvent);
static uint32_t init_wait_erase(PEVT_ST pEvent);
//flash ble player init cmd
static uint32_t init_player_procees_state_check(PEVT_ST pEvent);
static uint32_t player_set_flash_erase(PEVT_ST pEvent);
static uint32_t player_set_flash_uninit(PEVT_ST pEvent);
static uint32_t init_player_flash_write(PEVT_ST pEvent);
//flash ble time init cmd
static uint32_t init_time_set_process_state_check(PEVT_ST pEvent);
static uint32_t time_set_daily_log_erase(PEVT_ST pEvent);
static uint32_t time_set_daily_log_write(PEVT_ST pEvent);
static uint32_t time_set_flash_uninit(PEVT_ST pEvent);
//flash ble access pin code
static uint32_t init_access_pincode_reply_notify(PEVT_ST pEvent);

//flash ble pin code check
static uint32_t init_pincode_check_state(PEVT_ST pEvent);
static uint32_t flash_pincode_check_erase(PEVT_ST pEvent);
static uint32_t flash_pincode_check_write(PEVT_ST pEvent);
static uint32_t flash_pincode_check_uninit(PEVT_ST pEvent);

//flash ble pin code erase
static uint32_t pin_code_erase_state_check(PEVT_ST pEvent);
static uint32_t flash_pincode_erase_player_rewrite(PEVT_ST pEvent);
static uint32_t flash_pincode_erase_uninit(PEVT_ST pEvent);

//RTC code erase
static uint32_t rtc_int_state_check(PEVT_ST pEvent);
static uint32_t rtc_int_flash_uninit(PEVT_ST pEvent);

/* Other Utility Function */
static void clear_flash_sector(nrf_fstorage_t *p_fstorage, uint32_t start_addr);
static void player_info_erase(nrf_fstorage_t *p_fstorage, nrf_fstorage_api_t *p_fs_api, uint32_t *start_addr, PEVT_ST pEvent);
static void daily_log_write(nrf_fstorage_t *p_fstorage, uint32_t start_addr);
static void daily_log_erase_prepare(uint16_t *sid, uint8_t *sector_bit, uint32_t *start_addr, bool *checkTime, DATE_TIME *rtcdateTime);
static void daily_log_erase(nrf_fstorage_t *p_fstorage, uint32_t start_addr, uint16_t sid, DATE_TIME *rtcdateTime);
static bool compare_time(DATE_TIME *firstData, DATE_TIME *secondData);
static uint32_t sex_check(uint8_t sexData);
static uint32_t date_check(uint8_t *pDateTime);

/**
 * @brief Notify Message Push
 * @param event_id 通知するイベントID
 * @param src_event コピーするイベントがある場合指定する.ない場合NULLを指定
 * @retval None
 */
static void notify_push_message( uint8_t event_id, EVT_ST *src_event );

//FW version 9.00 debug
//static void debug_send_updateEvent(void);

/*
 * Flash操作時の状態により読み出す関数定義と使用する情報をまとめたテーブル
 * フォーマット
 *  { OP_STATUS_CHECK,	INIT_CMD_START,	FLASH_DATA_WRITE_CMPL,	FLASH_DATA_ERASE_CMPL },
 *  op_id, seq_id, start_addr, p_fstorage, op(Operation State), DATE_TIME
 */

//0x00,st_check 0x01 op start 0x02 wait write cmpl 0x03 wait erase cmpl
FLASH_OP_REG  flash_init_cmd = {
	{	init_procees_state_check,			init_player_flash_flag_erase,	init_wait_write,			init_wait_erase				},
	FLASH_INIT_CMD, 0, 0, NULL,
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0},
};

FLASH_OP_REG  flash_player_cmd = {
	{	init_player_procees_state_check, 	player_set_flash_erase,			player_set_flash_uninit,	init_player_flash_write		},
	FLASH_SET_PLAYER, 0, 0, NULL,
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0},
};

FLASH_OP_REG  flash_time_cmd = {
	{	init_time_set_process_state_check,	time_set_daily_log_erase,		time_set_flash_uninit,		time_set_daily_log_write	},
	FLASH_SET_TIME, 0, 0, NULL,
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0},
};

FLASH_OP_REG flash_pin_code_Access = {
	{	init_access_pincode_reply_notify,	NonAction,						NonAction,					NonAction					},
	FLASH_ACCESS_PINCODE, 0, 0, NULL,
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0},
};

FLASH_OP_REG flash_pin_code_check = {
	{	init_pincode_check_state,			flash_pincode_check_erase,		flash_pincode_check_uninit,	flash_pincode_check_write	},
	FLASH_CHECK_PINCODE, 0, 0, NULL,
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0},
};

FLASH_OP_REG flash_pin_code_erase = {
	{	pin_code_erase_state_check,			flash_pincode_check_erase,		flash_pincode_erase_uninit,	flash_pincode_erase_player_rewrite	},
	FLASH_ERASE_PINCODE, 0, 0, NULL,
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0},
};

FLASH_OP_REG flash_idle = {
	{	NonAction,							NonAction,						NonAction,					NonAction					},
	FLASH_OP_IDLE, 0, 0, NULL,
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0},
};

FLASH_OP_REG flash_rtc_init = {
	{	rtc_int_state_check,				time_set_daily_log_erase,		rtc_int_flash_uninit,		time_set_daily_log_write	},
	FLASH_RTC_INT, 0, 0, NULL,
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0},
};

FLASH_OP_REG flash_read_log = {
	{	SendDailyLog,						NonAction,						NonAction,					NonAction					},
	FLASH_READ_DAILY, 0, 0, NULL,
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0},
};

FLASH_OP_REG flash_read_log_one = {
	{	SendSingleDailyLog,					NonAction,						NonAction,					NonAction					},
	FLASH_READ_DAILY_ONE, 0, 0, NULL,
	{0,0,0,0,0,0,0,0},
	{0,0,0,0,0,0,0},
};

/* Flash Operation ID 
 * フォーマット
 *  FLASH_RTC_INT,			FLASH_INIT_CMD,			FLASH_SET_PLAYER,		FLASH_SET_TIME,
 *  FLASH_ACCESS_PINCODE,	FLASH_CHECK_PINCODE,	FLASH_ERASE_PINCODE,	FLASH_READ_DAILY
 *  FLASH_READ_DAILY_ONE,	FLASH_OP_IDLE
 */
volatile const PFLASH_OP_REG gpFlashOpList[FLASH_OP_ID_END_POINT] = {
	(PFLASH_OP_REG)&flash_rtc_init,			(PFLASH_OP_REG)&flash_init_cmd,			(PFLASH_OP_REG)&flash_player_cmd,		(PFLASH_OP_REG)&flash_time_cmd,
	(PFLASH_OP_REG)&flash_pin_code_Access,	(PFLASH_OP_REG)&flash_pin_code_check,	(PFLASH_OP_REG)&flash_pin_code_erase,	(PFLASH_OP_REG)&flash_read_log,
	(PFLASH_OP_REG)&flash_read_log_one,		(PFLASH_OP_REG)&flash_idle
};

/**
 * @brief Flash Operation Initialize
 * @param None
 * @retval None
 */
void FlashOpInit(void)
{
	gpFlashCurrOp = (PFLASH_OP)gpFlashOpList[FLASH_OP_IDLE];
}

/**
 * @brief Flash Operation Entry Point(State Controlから読み出され処理を実行する)
 * @param pEvent Event Information
 * @retval 0 Success
 */
uint32_t FlashWrapperfunc(PEVT_ST pEvent)
{
	uint8_t flash_eventId;
	
	flash_eventId = (pEvent->evt_id + FLASH_OP_MASK_OFFSET) & (~FLASH_OP_MASK);
	DEBUG_LOG(LOG_INFO,"flash wrapper func. flash op id 0x%x, evt 0x%x",gpFlashCurrOp->op_id,pEvent->evt_id);
	
	gpFlashCurrOp->exefunc[flash_eventId](pEvent);
	
	return 0;
}

/**
 * @brief main state cnt, event flash operation all event.
 * @param pEvent Event Information
 * @retval 0 Success
 */
uint32_t FlashOpIdCheck(PEVT_ST pEvent)
{
#if !defined(NOTIFY_SHARED)
	uint32_t fifo_err;
#endif
	uint8_t  flash_stateId;
#if !defined(NOTIFY_SHARED)
	EVT_ST   event;
	EVT_ST   sleepEvent;
	EVT_ST   replyRtcEvent;
#endif

	DEBUG_LOG(LOG_DEBUG,"Flash operaton id check enter");
	if(gpFlashCurrOp->op_id == FLASH_OP_IDLE)
	{
		flash_stateId = (pEvent->evt_id - 0x01) & (~0x10);
		DEBUG_LOG(LOG_DEBUG,"Enter flash op evt 0x%x, state id 0x%x",pEvent->evt_id, flash_stateId);
		
		//change init cmd state.
		gpFlashCurrOp = (PFLASH_OP)gpFlashOpList[flash_stateId];
		//set sequense id is 0 set(start)
		gpFlashCurrOp->seq_id = 0;
		//op status zero clear
		memset(&(gpFlashCurrOp->op),0x00,sizeof(gpFlashCurrOp->op));
		
		/* 2020.09.18 Add Notify処理を共通化 */
#if defined(NOTIFY_SHARED)
		notify_push_message( EVT_FLASH_OP_STATUS_CHECK, pEvent );
#else
		memcpy(&event,pEvent,sizeof(event));
		event.evt_id = EVT_FLASH_OP_STATUS_CHECK;
		//opration id cd
		fifo_err = PushFifo(&event);
		DEBUG_EVT_FIFO_LOG(fifo_err,event.evt_id);
#endif
		//trace log
		TRACE_LOG(TR_FLASH_OPERATION_MODE_START,(uint16_t)gpFlashCurrOp->op_id);
		
	}
	else if((gpFlashCurrOp->op_id == FLASH_INIT_CMD) && (gpFlashCurrOp->seq_id == 0))
	{
		if(2 <= pEvent->DATA.dataInfo.flash_flg)
		{
			DEBUG_LOG(LOG_INFO,"Flash_op_id_check func INIT_CMD flash flag 0x%x",pEvent->DATA.dataInfo.flash_flg);
		/* 2020.09.18 Add Notify処理を共通化 */
#if defined(NOTIFY_SHARED)
			notify_push_message( EVT_FLASH_OP_STATUS_CHECK, pEvent );
#else
			memcpy(&event,pEvent,sizeof(event));
			event.evt_id = EVT_FLASH_OP_STATUS_CHECK;
			fifo_err = PushFifo(&event);	
			DEBUG_EVT_FIFO_LOG(fifo_err,event.evt_id);
#endif
		}
	}
	else
	{
		DEBUG_LOG(LOG_INFO,"operation not change. flash op is 0x%x", gpFlashCurrOp->op_id);
	}
	
	if(gpFlashCurrOp->op_id != FLASH_OP_IDLE)
	{
		if((gpFlashCurrOp->op_id != FLASH_RTC_INT) && (pEvent->evt_id == EVT_RTC_INT))
		{
			DEBUG_LOG(LOG_INFO,"flash_op_id_check. repush fifo RTC int");
		/* 2020.09.18 Add Notify処理を共通化 */
#if defined(NOTIFY_SHARED)
			notify_push_message( EVT_FORCE_SLEEP, NULL );
#else
			sleepEvent.evt_id = EVT_FORCE_SLEEP;
			fifo_err = PushFifo(&sleepEvent);
			DEBUG_EVT_FIFO_LOG(fifo_err,sleepEvent.evt_id);
#endif
		/* 2020.09.18 Add Notify処理を共通化 */
#if defined(NOTIFY_SHARED)
			notify_push_message( EVENT_NO_MODIFY, pEvent );
#else
			memcpy(&replyRtcEvent,pEvent,sizeof(replyRtcEvent));
			fifo_err = PushFifo(&replyRtcEvent);
			DEBUG_EVT_FIFO_LOG(fifo_err,replyRtcEvent.evt_id);
#endif
		}
		/* 2022.06.22 Add Read LogイベントをRepush ++ */
		if ( ( ( gpFlashCurrOp->op_id != FLASH_READ_DAILY ) && ( pEvent->evt_id == EVT_BLE_CMD_READ_LOG ) ) ||
			 ( ( gpFlashCurrOp->op_id != FLASH_READ_DAILY_ONE ) && ( pEvent->evt_id == EVT_BLE_CMD_READ_LOG_ONE ) ) )
		{
			notify_push_message( EVT_FORCE_SLEEP, NULL );
			/* 到来したイベントRepush */
			notify_push_message( EVENT_NO_MODIFY, pEvent );
		}
		/* 2022.06.22 Add Read LogイベントをRepush -- */
	}

	return 0;
}

/**
 * @brief Flash Operation Force Initialize
 * @param None
 * @retval None
 */
void FlashOpForceInit(void)
{
	uint32_t err;
	uint8_t critical_flash;
	
	err = sd_nvic_critical_region_enter(&critical_flash);
	if(err == NRF_SUCCESS)
	{
		memset(&(gpFlashCurrOp->op), 0x00, sizeof(gpFlashCurrOp->op));
		gpFlashCurrOp->seq_id = 0;
		gpFlashCurrOp = (PFLASH_OP)gpFlashOpList[FLASH_OP_IDLE];
		critical_flash = 0;
		sd_nvic_critical_region_exit(critical_flash);
		if(err != NRF_SUCCESS)
		{
			//nothing to do.
		}
	}
}

/**
 * @brief main state cnt, event flash operation slave latency zero check
 * @param pEvent Event Information
 * @retval 0 Success
 */
uint32_t FlashOpSlaveLatencyZeroCheck(PEVT_ST pEvent)
{
	uint8_t notify_sig;
	uint16_t gatt_value_handle;
	uint16_t tracelogId = TR_DUMMY_ID;
	
	if(gpFlashCurrOp->seq_id == INIT_CMD_WAIT_SLAVE_LATENCY_ZERO)
	{
		gpFlashCurrOp->op[INIT_CMD_CMPL_SLAVE_LATENCY_ZERO] = 1;
		gpFlashCurrOp->seq_id = INIT_CMD_CMPL_SLAVE_LATENCY_ZERO;
		if((gpFlashCurrOp->op_id == FLASH_RTC_INT) || (gpFlashCurrOp->op_id == FLASH_OP_IDLE) || (gpFlashCurrOp->op_id == FLASH_ERASE_PINCODE))
		{
			__NOP();
		}
		else if(gpFlashCurrOp->op_id == FLASH_CHECK_PINCODE)
		{
			notify_sig = BLE_PINCODE_MATCH_SUCCESS;
			GetGattsCharHandleValueID(&gatt_value_handle,PAIRING_MATCH_ID);
			SetBleNotifyCmd(gatt_value_handle, &notify_sig, (uint16_t)sizeof(notify_sig));
		}
		else
		{
			if(gpFlashCurrOp->op_id != FLASH_INIT_CMD)
			{
				notify_sig = BLE_NOTIFY_SIG;
				BleNotifySignals(&notify_sig, (uint16_t)sizeof(notify_sig));
				
				if(gpFlashCurrOp->op_id == FLASH_SET_PLAYER)
				{
					tracelogId = TR_BLE_SET_PLAYER_INFO_CMPL;
					TRACE_LOG(tracelogId,0);
				}
				else if(gpFlashCurrOp->op_id == FLASH_SET_TIME)
				{
					tracelogId = TR_BLE_SET_TIME_CMPL;
					TRACE_LOG(tracelogId,0);
				}
			}
			else if((gpFlashCurrOp->op_id == FLASH_INIT_CMD) && (gpFlashCurrOp->op[INIT_CMD_WAIT_ER_FLASH] == 0))
			{
				notify_sig = BLE_NOTIFY_SIG;
				BleNotifySignals(&notify_sig, (uint16_t)sizeof(notify_sig));
				tracelogId = TR_BLE_INIT_CMD_CMPL;
				TRACE_LOG(tracelogId,0);
			}
		}			
		FlashOpForceInit();
	}
	//FLAHS_CHECK_PINCODE, first connection param up paramter. up notify
	else if(gpFlashCurrOp->op_id == FLASH_CHECK_PINCODE)
	{
		if(gpFlashCurrOp->op[INIT_CMD_PINCODE_CHECK_MATCH] == 1)
		{
			notify_sig = BLE_PINCODE_MATCH_SUCCESS;
			GetGattsCharHandleValueID(&gatt_value_handle,PAIRING_MATCH_ID);
			SetBleNotifyCmd(gatt_value_handle, &notify_sig, sizeof(notify_sig));
			FlashOpForceInit();
			DEBUG_LOG(LOG_INFO,"pincode match");
		}
		else if(gpFlashCurrOp->op[INIT_CMD_PINCODE_CHECK_NOMATCH] == 1)
		{
			notify_sig = BLE_PINCODE_MATCH_ERROR;
			GetGattsCharHandleValueID(&gatt_value_handle,PAIRING_MATCH_ID);
			SetBleNotifyCmd(gatt_value_handle, &notify_sig, sizeof(notify_sig));
			FlashOpForceInit();
			DEBUG_LOG(LOG_INFO,"pincode not match");
		}
		else if(gpFlashCurrOp->op[INIT_CMD_PINCODE_ACCESS_NG] == 1)
		{
			notify_sig = BLE_NOTIFY_SIG_NG;
			GetGattsCharHandleValueID(&gatt_value_handle,FW_RES_ID);
			SetBleNotifyCmd(gatt_value_handle, &notify_sig, sizeof(notify_sig));
			FlashOpForceInit();
			DEBUG_LOG(LOG_INFO,"pincode access NG");
		}
		else
		{
			//nothing to do
		}
	}
	
	return 0;
}

/**
 * @brief Flash Timeout event handler
 * @param None
 * @retval None
 */
void FlashTimeoutEvtHandler(void)
{
#if !defined(NOTIFY_SHARED)
	EVT_ST event;
	uint32_t fifo_err;
#endif
	uint16_t gatt_value_handle;
	bool uart_output_enable;
	uint8_t notify_sig;
	
	uart_output_enable = GetUartOutputStatus();
	if(uart_output_enable == false)
	{
		LibUartEnable();
	}
	
	//flash instance force uninit
	if(gpFlashCurrOp->p_fstorage != NULL)
	{
		nrf_fstorage_uninit(gpFlashCurrOp->p_fstorage, NULL);
	}
		
	if((gpFlashCurrOp->op[INIT_CMD_WAIT_SLAVE_LATENCY_ONE] == 1) || (gpFlashCurrOp->op[INIT_CMD_CMPL_SLAVE_LATENCY_ONE] == 1) || (gpFlashCurrOp->op[INIT_CMD_WAIT_SLAVE_LATENCY_ZERO] == 1))
	{
		//future change
		DEBUG_LOG(LOG_INFO,"param change timeout evt handler 2");
		/* 2020.09.18 Add Notify処理を共通化 */
#if defined(NOTIFY_SHARED)
		notify_push_message( EVT_FLASH_CNT_PARAM_RETURN, NULL );
#else
		event.evt_id = EVT_FLASH_CNT_PARAM_RETURN;
		fifo_err = PushFifo(&event);
		DEBUG_EVT_FIFO_LOG(fifo_err,event.evt_id);
#endif
	}
	//other, flash Operation mode force init
	else
	{
		//future change
		DEBUG_LOG(LOG_INFO,"flash op init timeout evt handler 3");
	}
	
	RestartForceDisconTimer();
	
	if((gpFlashCurrOp->op_id == FLASH_INIT_CMD) || (gpFlashCurrOp->op_id == FLASH_SET_PLAYER) || (gpFlashCurrOp->op_id == FLASH_SET_TIME))
	{
		SetBleErrReadCmd((uint8_t)UTC_BLE_INIT_CMD_ERROR);
	}
	else if(gpFlashCurrOp->op_id == FLASH_CHECK_PINCODE)
	{
		SetBleErrReadCmd((uint8_t)UTC_BLE_PIN_CODE_ERROR);
		notify_sig = BLE_PINCODE_MATCH_ERROR;
		
		GetGattsCharHandleValueID(&gatt_value_handle,PAIRING_MATCH_ID);
		SetBleNotifyCmd(gatt_value_handle, &notify_sig, (uint16_t)sizeof(notify_sig));
	}
	else if(gpFlashCurrOp->op_id == FLASH_ERASE_PINCODE)
	{
		SetBleErrReadCmd((uint8_t)UTC_BLE_PIN_CODE_ERROR);
	}

	FlashOpForceInit();

	if(uart_output_enable == false)
	{
		LibUartDisable();
	}
}

/****************************************************************************************
 * flash ble init cmd
 ****************************************************************************************/

/**
 * @brief state cnt, event EVT_FLASH_OP_STATUS_CHECK. flash state FLASH_INIT_CMD
 * @param pEvent Event Information
 * @retval 0 Success
 */
static uint32_t init_procees_state_check(PEVT_ST pEvent)
{
#if !defined(NOTIFY_SHARED)
	uint32_t fifo_err;
#endif
	uint32_t sexInvalid; 
	uint32_t dateInvalid;
	ret_code_t ret_code;
	DATE_TIME rtcdateTime;
	DATE_TIME recTime;
	Daily_t daily_data;
	uint8_t notify_sig;
	EVT_ST event;
#if !defined(NOTIFY_SHARED)
	EVT_ST replyEvent;
#endif

	DEBUG_LOG(LOG_INFO,"INIT CMD start mode 0x%x, flash flag 0x%x", pEvent->DATA.dataInfo.mode, pEvent->DATA.dataInfo.flash_flg);
	
	if(2 <= pEvent->DATA.dataInfo.flash_flg)
	{
		gpFlashCurrOp->op[INIT_CMD_WAIT_ER_FLASH] = pEvent->DATA.dataInfo.flash_flg;
		if(3 <= pEvent->DATA.dataInfo.flash_flg)
		{
			DEBUG_LOG(LOG_DEBUG,"3 <= flash flag 0x%x",pEvent->DATA.dataInfo.flash_flg);
			if(pEvent->DATA.dataInfo.flash_flg == 3)
			{
				gpFlashCurrOp->op[INIT_CMD_WAIT_SLAVE_LATENCY_ZERO] = 1;
				gpFlashCurrOp->seq_id = INIT_CMD_WAIT_SLAVE_LATENCY_ZERO;
				memset(&event,0x00,sizeof(event));
				event.evt_id = EVT_BLE_CMD_INIT;
				event.DATA.dataInfo.flash_flg = 4;
				gpFlashCurrOp->seq_id = 0;
				/* 2020.09.18 Add Notify処理を共通化 */
#if defined(NOTIFY_SHARED)
				notify_push_message( EVENT_NO_MODIFY, &event );
#else
				//20180817 add new. PushFifo EVT_BLE_CMD_INIT.flash flag == 3 event push fifo
				fifo_err = PushFifo(&event);
				DEBUG_EVT_FIFO_LOG(fifo_err,event.evt_id);
#endif
			}
			else
			{
				gpFlashCurrOp->op[INIT_CMD_WAIT_SLAVE_LATENCY_ZERO] = 1;
				gpFlashCurrOp->seq_id = INIT_CMD_WAIT_SLAVE_LATENCY_ZERO;
				/* 2020.09.18 Add Notify処理を共通化 */
#if defined(NOTIFY_SHARED)
				notify_push_message( EVT_FLASH_CNT_PARAM_RETURN, NULL );
#else
				event.evt_id = EVT_FLASH_CNT_PARAM_RETURN;
				fifo_err = PushFifo(&event);
				DEBUG_EVT_FIFO_LOG(fifo_err,event.evt_id);
#endif
			}
			return 0;
		}
		
		/* 2020.09.18 Add Notify処理を共通化 */
#if defined(NOTIFY_SHARED)
		notify_push_message( EVT_INIT_CMD_START, pEvent );
#else
		memcpy(&event,pEvent,sizeof(event));
		event.evt_id = EVT_INIT_CMD_START;
		fifo_err = PushFifo(&event);
		DEBUG_EVT_FIFO_LOG(fifo_err,event.evt_id);
#endif
		return 0;
	}
	else
	{
		//player state check
		StartFlashTimer();
		sexInvalid = sex_check(pEvent->DATA.dataInfo.sex);

		//Time date check
		dateInvalid = date_check(pEvent->DATA.dataInfo.dateTime);

		if((dateInvalid != UTC_SUCCESS) || (sexInvalid != UTC_SUCCESS))
		{
			SetBleErrReadCmd((uint8_t)UTC_BLE_INIT_CMD_ERROR);
			notify_sig = BLE_NOTIFY_SIG_NG;
			BleNotifySignals(&notify_sig, (uint16_t)sizeof(notify_sig));
			FlashOpForceInit();
			StopFlashTimer();
			return 0;
		}
		
		//player information set
		gpFlashCurrOp->op[INIT_CMD_WAIT_ER_FLASH]  			= pEvent->DATA.dataInfo.flash_flg;
		gpFlashCurrOp->op[INIT_CMD_WAIT_ER_PLAYER] 			= 1;
		gpFlashCurrOp->op[INIT_CMD_WAIT_WR_PLAYER] 			= 1;
		gpFlashCurrOp->op[INIT_CMD_WAIT_SLAVE_LATENCY_ONE]	= 1;
		
		ret_code = ExRtcGetDateTime(&rtcdateTime);
		if(ret_code == NRF_SUCCESS)
		{
			TimeDataFormatReverse(&recTime, pEvent->DATA.dataInfo.dateTime);
			//if flash flag clear, do not operation daily log. Only rtc time set.
			if(gpFlashCurrOp->op[INIT_CMD_WAIT_ER_FLASH] == 0)
			{
				if(compare_time(&rtcdateTime, &recTime))
				{
					GetRamDailyLog(&daily_data);
					/*flash flag check. (*(p + INIT_CMD_WAIT_ER_FLASH)). if flash flag on,*(p + INIT_CMD_WAIT_ER_DAILY_LOG) and *(p + INIT_CMD_WAIT_WR_DAILY_LOG) = 0 */
					if((daily_data.walk + daily_data.run + daily_data.dash) != 0)
					{
						DEBUG_LOG(LOG_INFO,"init cmd non flash flag set daily log");
						gpFlashCurrOp->op[INIT_CMD_WAIT_ER_DAILY_LOG] = 1;
						gpFlashCurrOp->op[INIT_CMD_WAIT_WR_DAILY_LOG] = 1;
						memcpy(&(gpFlashCurrOp->time),&recTime,sizeof(recTime));
					}
					else
					{
						InitRamDateSet(&recTime);
					}
				}
			}
			ExRtcSetDateTime(&recTime);
		}
		else
		{
			DEBUG_LOG(LOG_ERROR,"init cmd set time err.");
			TRACE_LOG(TR_RTC_CNT_INIT_SET_TIME_ERROR,0);
			SetBleErrReadCmd(UTC_BLE_TIME_UPDATE_ERROR);
		}
	}

	//flash change slave latency.
	gpFlashCurrOp->seq_id = INIT_CMD_WAIT_SLAVE_LATENCY_ONE;
	gpFlashCurrOp->op[INIT_CMD_WAIT_SLAVE_LATENCY_ONE] = 1;
	/* 2020.09.18 Add Notify処理を共通化 */
#if defined(NOTIFY_SHARED)
		notify_push_message( EVT_FLASH_CNT_PARAM_CHANGE, NULL );
#else
	event.evt_id = EVT_FLASH_CNT_PARAM_CHANGE;
	fifo_err = PushFifo(&event);
	DEBUG_EVT_FIFO_LOG(fifo_err,event.evt_id);
#endif
	
	/* 2020.09.18 Add Notify処理を共通化 */
#if defined(NOTIFY_SHARED)
		notify_push_message( EVT_INIT_CMD_START, pEvent );
#else
	//re push event init process start cmd.
	memcpy(&replyEvent,pEvent,sizeof(replyEvent));
	replyEvent.evt_id = EVT_INIT_CMD_START;
	fifo_err = PushFifo(&replyEvent);
	DEBUG_EVT_FIFO_LOG(fifo_err,replyEvent.evt_id);
#endif
	
	return 0;
}

/**
 * @brief Initialize Flash flag Erase
 * @param pEvent Event Information
 * @retval 0 Success
 */
static uint32_t init_player_flash_flag_erase(PEVT_ST pEvent)
{
	ret_code_t rc;
	nrf_fstorage_api_t *p_fs_api;

	p_fs_api = &nrf_fstorage_sd;

	if(gpFlashCurrOp->op[INIT_CMD_WAIT_ER_PLAYER] == 1)
	{
		StartFlashTimer();
		gpFlashCurrOp->seq_id = INIT_CMD_CMPL_SLAVE_LATENCY_ONE;
		gpFlashCurrOp->op[INIT_CMD_CMPL_SLAVE_LATENCY_ONE] = 1;
		gpFlashCurrOp->start_addr = PLAYER_AND_PAIRING_ADDR;
		gpFlashCurrOp->p_fstorage = &player_pincode_fstorage;
		rc = nrf_fstorage_init(gpFlashCurrOp->p_fstorage, p_fs_api, NULL);
		ERR_CHECK_FLASH(rc,(FLASH_INIT_ERROR|EX_FLASH_OP_FILE_ID),__LINE__);
		
//debug test
#ifdef TEST_INIT_TO_ERROR
		return 0;
#endif
		gpFlashCurrOp->seq_id = INIT_CMD_WAIT_ER_PLAYER;
		player_info_erase(gpFlashCurrOp->p_fstorage, p_fs_api, &(gpFlashCurrOp->start_addr), pEvent);
	}
	else if(gpFlashCurrOp->op[INIT_CMD_WAIT_ER_FLASH] == 2)
	{
		DEBUG_LOG(LOG_INFO, "init cmd secter 2 clear");
		gpFlashCurrOp->start_addr = DAILY_ADDR2;
		gpFlashCurrOp->p_fstorage = &ex_fstorage2;
		rc = nrf_fstorage_init(gpFlashCurrOp->p_fstorage, p_fs_api, NULL);
		ERR_CHECK_FLASH(rc,(FLASH_INIT_ERROR|EX_FLASH_OP_FILE_ID),__LINE__);
		//sequence id set.
		gpFlashCurrOp->seq_id = INIT_CMD_WAIT_ER_FLASH;
		clear_flash_sector(gpFlashCurrOp->p_fstorage, gpFlashCurrOp->start_addr);
	}
	return 0;
}

/**
 * @brief Initialize Wait Write
 * @param pEvent Event Information
 * @retval 0 Success
 */
static uint32_t init_wait_write(PEVT_ST pEvent)
{
	ret_code_t rc;
	nrf_fstorage_api_t *p_fs_api;
#if !defined(NOTIFY_SHARED)
	uint32_t fifo_err;
	EVT_ST event;
#endif
	uint16_t sid;
	bool checkTime;
	
	p_fs_api = &nrf_fstorage_sd;
	
	//player flash uninit
	if(gpFlashCurrOp->seq_id == INIT_CMD_WAIT_WR_PLAYER)
	{
		DEBUG_LOG(LOG_INFO,"init cmd player info flash uninit");
		rc = nrf_fstorage_uninit(gpFlashCurrOp->p_fstorage, NULL);
		ERR_CHECK_FLASH(rc,(FLASH_UNINIT_ERROR|EX_FLASH_OP_FILE_ID),__LINE__);
		
		if(gpFlashCurrOp->op[INIT_CMD_WAIT_ER_DAILY_LOG] == 1)
		{
			daily_log_erase_prepare(&sid, &(gpFlashCurrOp->op[INIT_CMD_WAIT_ER_DAILY_LOG]), &(gpFlashCurrOp->start_addr), &checkTime, &(gpFlashCurrOp->time));
			if(checkTime == true)
			{
				if((gpFlashCurrOp->op[INIT_CMD_WAIT_ER_DAILY_LOG] & (1 << 1)) != 0)
				{
					gpFlashCurrOp->p_fstorage = &ex_fstorage2;
				}
				else
				{
					gpFlashCurrOp->p_fstorage = &ex_fstorage;
				}
				DEBUG_LOG(LOG_INFO,"init cmd daily log erase init for daily log write");
				rc = nrf_fstorage_init(gpFlashCurrOp->p_fstorage, p_fs_api, NULL);
				ERR_CHECK_FLASH(rc, (FLASH_INIT_ERROR|EX_FLASH_OP_FILE_ID),__LINE__);
				daily_log_erase(gpFlashCurrOp->p_fstorage, gpFlashCurrOp->start_addr, sid, &(gpFlashCurrOp->time));
				gpFlashCurrOp->seq_id = INIT_CMD_WAIT_ER_DAILY_LOG;
			}
			else
			{
				gpFlashCurrOp->seq_id = INIT_CMD_WAIT_WR_DAILY_LOG;
				/* 2020.09.18 Add Notify処理を共通化 */
#if defined(NOTIFY_SHARED)
				notify_push_message( EVT_FLASH_DATA_WRITE_CMPL, NULL );
#else
				event.evt_id = EVT_FLASH_DATA_WRITE_CMPL;
				fifo_err = PushFifo(&event);
				DEBUG_EVT_FIFO_LOG(fifo_err,event.evt_id);
#endif
			}
		}
		//No daily log write and flash flag non
		else if(gpFlashCurrOp->op[INIT_CMD_WAIT_ER_FLASH] == 0)
		{
			gpFlashCurrOp->seq_id = INIT_CMD_WAIT_SLAVE_LATENCY_ZERO;
			gpFlashCurrOp->op[INIT_CMD_WAIT_SLAVE_LATENCY_ZERO] = 1;
			/* 2020.09.18 Add Notify処理を共通化 */
#if defined(NOTIFY_SHARED)
			notify_push_message( EVT_FLASH_CNT_PARAM_RETURN, NULL );
#else
			event.evt_id = EVT_FLASH_CNT_PARAM_RETURN;
			fifo_err = PushFifo(&event);
			DEBUG_EVT_FIFO_LOG(fifo_err,event.evt_id);
#endif
		}
		else if(gpFlashCurrOp->op[INIT_CMD_WAIT_ER_FLASH] == 1)
		{
			DEBUG_LOG(LOG_INFO,"Flash sector 1 clear flash init 1");
			gpFlashCurrOp->start_addr = DAILY_ADDR1;
			gpFlashCurrOp->p_fstorage = &ex_fstorage;
			rc = nrf_fstorage_init(gpFlashCurrOp->p_fstorage, p_fs_api, NULL);
			ERR_CHECK_FLASH(rc,(FLASH_INIT_ERROR|EX_FLASH_OP_FILE_ID),__LINE__);
			clear_flash_sector(gpFlashCurrOp->p_fstorage, gpFlashCurrOp->start_addr);
			gpFlashCurrOp->seq_id = INIT_CMD_WAIT_ER_FLASH;			
		}
	}
	else if(gpFlashCurrOp->seq_id == INIT_CMD_WAIT_WR_DAILY_LOG)
	{
		rc = nrf_fstorage_uninit(gpFlashCurrOp->p_fstorage, NULL);
		ERR_CHECK_FLASH(rc,(FLASH_UNINIT_ERROR|EX_FLASH_OP_FILE_ID),__LINE__);
		//daily log write and flash flag on
		if(gpFlashCurrOp->op[INIT_CMD_WAIT_ER_FLASH] == 1)
		{
			DEBUG_LOG(LOG_INFO,"Flash sector 1 clear flash init 2");
			gpFlashCurrOp->start_addr = DAILY_ADDR1;
			gpFlashCurrOp->p_fstorage = &ex_fstorage;
			rc = nrf_fstorage_init(gpFlashCurrOp->p_fstorage, p_fs_api, NULL);
			ERR_CHECK_FLASH(rc,(FLASH_INIT_ERROR|EX_FLASH_OP_FILE_ID),__LINE__);
			clear_flash_sector(gpFlashCurrOp->p_fstorage, gpFlashCurrOp->start_addr);
			gpFlashCurrOp->seq_id = INIT_CMD_WAIT_ER_FLASH;		
		}
		//condition: daily log write finish and flash flag no
		else
		{
			gpFlashCurrOp->seq_id = INIT_CMD_WAIT_SLAVE_LATENCY_ZERO;
			gpFlashCurrOp->op[INIT_CMD_WAIT_SLAVE_LATENCY_ZERO] = 1;
			/* 2020.09.18 Add Notify処理を共通化 */
#if defined(NOTIFY_SHARED)
			notify_push_message( EVT_FLASH_CNT_PARAM_RETURN, NULL );
#else
			event.evt_id = EVT_FLASH_CNT_PARAM_RETURN;
			fifo_err = PushFifo(&event);
			DEBUG_EVT_FIFO_LOG(fifo_err,event.evt_id);
#endif
		}
	}
	
	return 0;
}

/**
 * @brief Initialize cmd erase cmpl event wait 
 * @param pEvent Event Information
 * @retval 0 Success
 */
static uint32_t init_wait_erase(PEVT_ST pEvent)
{
	ret_code_t rc;
	uint8_t notify_signal;
#if !defined(NOTIFY_SHARED)
	uint32_t fifo_err;
#endif
	EVT_ST event;
	
	notify_signal = BLE_NOTIFY_SIG;
	
	if(gpFlashCurrOp->seq_id == INIT_CMD_WAIT_ER_PLAYER)
	{
		DEBUG_LOG(LOG_INFO,"init cmd player info write");
		gpFlashCurrOp->seq_id = INIT_CMD_WAIT_WR_PLAYER;
		rc = nrf_fstorage_write(gpFlashCurrOp->p_fstorage, gpFlashCurrOp->start_addr, (uint8_t*)&gPin_player_page, sizeof(gPin_player_page), NULL);
		ERR_CHECK_FLASH(rc,(FLASH_WRITE_ERROR|EX_FLASH_OP_FILE_ID),__LINE__);
	}
	else if(gpFlashCurrOp->seq_id == INIT_CMD_WAIT_ER_DAILY_LOG)
	{
		//write event generate.
		gpFlashCurrOp->seq_id = INIT_CMD_WAIT_WR_DAILY_LOG;
		daily_log_write(gpFlashCurrOp->p_fstorage, gpFlashCurrOp->start_addr);
	}
	else if(gpFlashCurrOp->seq_id == INIT_CMD_WAIT_ER_FLASH)
	{
		if(gpFlashCurrOp->op[INIT_CMD_WAIT_ER_FLASH] == 1)
		{
			rc = nrf_fstorage_uninit(gpFlashCurrOp->p_fstorage,NULL);
			ERR_CHECK_FLASH(rc,(FLASH_UNINIT_ERROR|EX_FLASH_OP_FILE_ID),__LINE__);
			memset(&(gpFlashCurrOp->op), 0x00, sizeof(gpFlashCurrOp->op));
			gpFlashCurrOp->seq_id = 0;
			
			//reply notify signal
			DEBUG_LOG(LOG_INFO,"flash flag 1. up notify signal");
			BleNotifySignals(&notify_signal,(uint16_t)sizeof(notify_signal));
			RestartForceDisconTimer();
			//20180817 add new. PushFifo EVT_BLE_CMD_INIT.flash flag == 2 event push fifo
			memset(&event,0x00,sizeof(event));
			event.evt_id = EVT_BLE_CMD_INIT;
			event.DATA.dataInfo.flash_flg = 2;
			/* 2020.09.18 Add Notify処理を共通化 */
#if defined(NOTIFY_SHARED)
			notify_push_message( EVENT_NO_MODIFY, &event );
#else
			fifo_err = PushFifo(&event);
			DEBUG_EVT_FIFO_LOG(fifo_err,event.evt_id);
#endif
		}
		else if(gpFlashCurrOp->op[INIT_CMD_WAIT_ER_FLASH] == 2)
		{
			rc = nrf_fstorage_uninit(gpFlashCurrOp->p_fstorage,NULL);
			ERR_CHECK_FLASH(rc,(FLASH_UNINIT_ERROR|EX_FLASH_OP_FILE_ID),__LINE__);
			DEBUG_LOG(LOG_INFO, "init cmd secter 2 clear end");
			gpFlashCurrOp->seq_id = 0;
			
			memset(&event, 0x00, sizeof(event));
			event.evt_id = EVT_BLE_CMD_INIT;
			event.DATA.dataInfo.flash_flg = 3;
			/* 2020.09.18 Add Notify処理を共通化 */
#if defined(NOTIFY_SHARED)
			notify_push_message( EVENT_NO_MODIFY, &event );
#else
			fifo_err = PushFifo(&event);
			DEBUG_EVT_FIFO_LOG(fifo_err,event.evt_id);
#endif
		}
	}
	return 0;
}

/****************************************************************************************
 * flash ble player init cmd
 ****************************************************************************************/

/**
 * @brief player info set state check.
 *        state cnt, event EVT_FLASH_OP_STATUS_CHECK. flash state FLASH_SET_PLAYER.
 * @param pEvent Event Information
 * @retval 0 Success
 */
static uint32_t init_player_procees_state_check(PEVT_ST pEvent)
{
	uint32_t sexInvalid;
	uint8_t  notify_sig;
#if !defined(NOTIFY_SHARED)
	uint32_t fifo_err;
	EVT_ST   event;
#endif
	//player state check
	StartFlashTimer();

	sexInvalid = sex_check(pEvent->DATA.dataInfo.sex);
	if(sexInvalid != UTC_SUCCESS)
	{
		SetBleErrReadCmd((uint8_t)UTC_BLE_INIT_CMD_ERROR);
		notify_sig = BLE_NOTIFY_SIG_NG;
		BleNotifySignals(&notify_sig, (uint16_t)sizeof(notify_sig));
		FlashOpForceInit();
		StopFlashTimer();
		RestartForceDisconTimer();
		return 0;
	}
	
	DEBUG_LOG(LOG_INFO,"player info set cmd");
	gpFlashCurrOp->op[INIT_CMD_WAIT_ER_PLAYER]			= 1;
	gpFlashCurrOp->op[INIT_CMD_WAIT_WR_PLAYER]			= 1;
	gpFlashCurrOp->op[INIT_CMD_WAIT_SLAVE_LATENCY_ONE]	= 1;
	
	gpFlashCurrOp->seq_id = INIT_CMD_WAIT_ER_PLAYER;
	
	//flash change slave latency.
	/* 2020.09.18 Add Notify処理を共通化 */
#if defined(NOTIFY_SHARED)
	notify_push_message( EVT_FLASH_CNT_PARAM_CHANGE, NULL );
#else
	event.evt_id = EVT_FLASH_CNT_PARAM_CHANGE;
	fifo_err = PushFifo(&event);
	DEBUG_EVT_FIFO_LOG(fifo_err,event.evt_id);
#endif

	//re push event init process start cmd.
	/* 2020.09.18 Add Notify処理を共通化 */
#if defined(NOTIFY_SHARED)
	notify_push_message( EVT_INIT_CMD_START, pEvent );
#else
	pEvent->evt_id = EVT_INIT_CMD_START;
	fifo_err = PushFifo(pEvent);
	DEBUG_EVT_FIFO_LOG(fifo_err,pEvent->evt_id);
#endif
	return 0;
}

/**
 * @brief main state cnt,
 *        event EVT_FLASH_OP_STATUS_CHECK, flash state FLASH_SET_PLAYER;
 * @param pEvent Event Information
 * @retval 0 Success
 */
static uint32_t player_set_flash_erase(PEVT_ST pEvent)
{
	ret_code_t rc;
	nrf_fstorage_api_t *p_fs_api;
	
	/* Timer Start */
	StartFlashTimer();
	
	gpFlashCurrOp->op[INIT_CMD_CMPL_SLAVE_LATENCY_ONE] = 1;
	
	p_fs_api = &nrf_fstorage_sd;
	gpFlashCurrOp->start_addr = PLAYER_AND_PAIRING_ADDR;
	gpFlashCurrOp->p_fstorage = &player_pincode_fstorage;
	
	rc = nrf_fstorage_init(gpFlashCurrOp->p_fstorage, p_fs_api, NULL);
	ERR_CHECK_FLASH(rc,(FLASH_INIT_ERROR|EX_FLASH_OP_FILE_ID),__LINE__);

	rc = nrf_fstorage_read(gpFlashCurrOp->p_fstorage, gpFlashCurrOp->start_addr, (uint8_t*)&gPin_player_page, sizeof(gPin_player_page));
	ERR_CHECK_FLASH(rc,(FLASH_READ_ERROR|EX_FLASH_OP_FILE_ID),__LINE__);

	gPin_player_page[PLAYER_CHECK_DATA_POS]     = SIGN_DATA_ONE;
	gPin_player_page[PLAYER_CHECK_DATA_POS + 1] = SIGN_DATA_TWO;
	gPin_player_page[PLAYER_CHECK_DATA_POS + 2] = pEvent->DATA.dataInfo.age;
	gPin_player_page[PLAYER_CHECK_DATA_POS + 3] = pEvent->DATA.dataInfo.sex;
	gPin_player_page[PLAYER_CHECK_DATA_POS + 4] = pEvent->DATA.dataInfo.weight;
	//algorithm player infomation set
	SetRamPlayerInfo(pEvent->DATA.dataInfo.age,pEvent->DATA.dataInfo.sex,pEvent->DATA.dataInfo.weight);
	gpFlashCurrOp->seq_id = INIT_CMD_WAIT_ER_PLAYER;
#ifdef TEST_PLAYER_SET_TO_ERROR
	return 0;
#endif
	rc = nrf_fstorage_erase(gpFlashCurrOp->p_fstorage, gpFlashCurrOp->start_addr, 1, NULL);
	ERR_CHECK_FLASH(rc,(FLASH_ERASE_ERROR|EX_FLASH_OP_FILE_ID),__LINE__);

	return 0;
}

/**
 * @brief Player Set Flash uninit
 * @param pEvent Event Information
 * @retval 0 Success
 */
static uint32_t player_set_flash_uninit(PEVT_ST pEvent)
{
	ret_code_t rc;
#if !defined(NOTIFY_SHARED)
	uint32_t fifo_err;
	EVT_ST   event;
#endif

	DEBUG_LOG(LOG_INFO,"player info flash uninit");
	
	rc = nrf_fstorage_uninit(gpFlashCurrOp->p_fstorage,NULL);
#ifdef TEST_FLASH_WEITE_ERROR
	rc = NRF_ERROR_INVALID_PARAM;
	DEBUG_LOG( LOG_INFO, "!!! Flash Weite Error Test !!!" );
#endif
	ERR_CHECK_FLASH(rc,(FLASH_UNINIT_ERROR|EX_FLASH_OP_FILE_ID),__LINE__);
	
	//change slave latency 0
	gpFlashCurrOp->op[INIT_CMD_WAIT_SLAVE_LATENCY_ZERO] = 1;
	gpFlashCurrOp->seq_id = INIT_CMD_WAIT_SLAVE_LATENCY_ZERO;
	/* 2020.09.18 Add Notify処理を共通化 */
#if defined(NOTIFY_SHARED)
	notify_push_message( EVT_FLASH_CNT_PARAM_RETURN, NULL );
#else
	event.evt_id = EVT_FLASH_CNT_PARAM_RETURN;
	fifo_err = PushFifo(&event);
	DEBUG_EVT_FIFO_LOG(fifo_err,event.evt_id);
#endif
	
	return 0;
}

/**
 * @brief flash state  FLASH_INIT_CMD, EVENT ERASE_CMPL
 *        Player Flash Write
 * @param pEvent Event Information
 * @retval 0 Success
 */
static uint32_t init_player_flash_write(PEVT_ST pEvent)
{
	ret_code_t rc;
	gpFlashCurrOp->seq_id = INIT_CMD_WAIT_WR_PLAYER;
	rc = nrf_fstorage_write(gpFlashCurrOp->p_fstorage, gpFlashCurrOp->start_addr, (uint8_t*)&gPin_player_page, sizeof(gPin_player_page), NULL);
	ERR_CHECK_FLASH(rc,(FLASH_WRITE_ERROR|EX_FLASH_OP_FILE_ID),__LINE__);

	return 0;
}

/****************************************************************************************
 * flash ble time init cmd
 ****************************************************************************************/

/**
 * @brief Time information state check
 *        state cnt, event EVT_FLASH_OP_STATUS_CHECK. flash state FLASH_SET_TIME
 * @param pEvent Event Information
 * @retval 0 Success
 */
static uint32_t init_time_set_process_state_check(PEVT_ST pEvent)
{
#if !defined(NOTIFY_SHARED)
	uint32_t fifo_err;
#endif
	uint32_t dateInvalid;
#if !defined(NOTIFY_SHARED)
	EVT_ST   event;
	EVT_ST   replyEvent;
#endif
	ret_code_t ret_code;
	DATE_TIME rtcdateTime;
	DATE_TIME recTime;
	Daily_t   daily_data;   
	uint8_t   notify_signal;
	
	notify_signal = BLE_NOTIFY_SIG;
	
	StartFlashTimer();
	DEBUG_LOG(LOG_INFO,"time set cmd");
	
	dateInvalid = date_check(pEvent->DATA.dataInfo.dateTime);
	if(dateInvalid != UTC_SUCCESS)
	{
		SetBleErrReadCmd((uint8_t)UTC_BLE_INIT_CMD_ERROR);
		notify_signal = BLE_NOTIFY_SIG_NG;
		BleNotifySignals(&notify_signal, (uint16_t)sizeof(notify_signal));
		FlashOpForceInit();
		StopFlashTimer();
		RestartForceDisconTimer();
		return 0;
	}
	
	ret_code = ExRtcGetDateTime(&rtcdateTime);
	
	if(ret_code == NRF_SUCCESS)
	{
		TimeDataFormatReverse(&recTime, pEvent->DATA.dataInfo.dateTime);
		if(compare_time(&rtcdateTime, &recTime))
		{
			GetRamDailyLog(&daily_data);
			if((daily_data.walk + daily_data.run + daily_data.dash) != 0)
			{
				gpFlashCurrOp->op[INIT_CMD_WAIT_ER_DAILY_LOG] 			= 1;
				gpFlashCurrOp->op[INIT_CMD_WAIT_WR_DAILY_LOG] 			= 1;
				gpFlashCurrOp->op[INIT_CMD_WAIT_SLAVE_LATENCY_ONE]  = 1;
				memcpy(&(gpFlashCurrOp->time),&recTime,sizeof(recTime));
				DEBUG_LOG(LOG_INFO,"SET TIME and Write Flash start");
				
				//flash change slave latency.
				/* 2020.09.18 Add Notify処理を共通化 */
#if defined(NOTIFY_SHARED)
				notify_push_message( EVT_FLASH_CNT_PARAM_CHANGE, NULL );
#else
				event.evt_id = EVT_FLASH_CNT_PARAM_CHANGE;
				fifo_err = PushFifo(&event);
				DEBUG_EVT_FIFO_LOG(fifo_err,event.evt_id);
#endif
				
				//re push event init process start cmd.
				/* 2020.09.18 Add Notify処理を共通化 */
#if defined(NOTIFY_SHARED)
				notify_push_message( EVT_INIT_CMD_START, pEvent );
#else
				memcpy(&replyEvent,pEvent,sizeof(replyEvent));
				replyEvent.evt_id = EVT_INIT_CMD_START;
				fifo_err = PushFifo(&replyEvent);
				DEBUG_EVT_FIFO_LOG(fifo_err,replyEvent.evt_id);
#endif
			}
			else
			{
				DEBUG_LOG(LOG_DEBUG,"SET TIME flash op Idle 1");
				InitRamDateSet(&recTime);
				FlashOpForceInit();
				BleNotifySignals(&notify_signal,(uint16_t)sizeof(notify_signal));
				StopFlashTimer();
				RestartForceDisconTimer();
			}
		}
		//Not change time
		else
		{
			DEBUG_LOG(LOG_DEBUG,"SET TIME flash op Idle 2");
			FlashOpForceInit();
			BleNotifySignals(&notify_signal,(uint16_t)sizeof(notify_signal));
			StopFlashTimer();
			RestartForceDisconTimer();
		}
		ExRtcSetDateTime(&recTime);
	}
	else
	{
		DEBUG_LOG(LOG_ERROR,"init set time error");
		TRACE_LOG(TR_RTC_CNT_SET_TIME_ERROR,0);
		SetBleErrReadCmd(UTC_BLE_TIME_UPDATE_ERROR);
		FlashOpForceInit();
		BleNotifySignals(&notify_signal,(uint16_t)sizeof(notify_signal));
		StopFlashTimer();
		RestartForceDisconTimer();
	}

	return 0;
}

/**
 * @brief State FLASH_SET_TIME Event. EVT_INIT_CMD_START,
 *        (State FLASH_INIT_CMD, Event INIT_CMD_DAILY_SET) flash operation state,
 * @param pEvent Event Information
 * @retval 0 Success
 */
static uint32_t time_set_daily_log_erase(PEVT_ST pEvent)
{
	Daily_t daily_data;
	uint8_t  sid;
	nrf_fstorage_api_t *p_fs_api;
	ret_code_t rc;
	uint8_t overCount;
	
	p_fs_api = &nrf_fstorage_sd;
	
	if(gpFlashCurrOp->op_id != FLASH_RTC_INT)
	{
		StartFlashTimer();
	}
	GetRamDailyLog(&daily_data);
	overCount = GetSidOverCount();
	
	DEBUG_LOG(LOG_INFO,"SET TIME flash init and erase");
	
	sid = daily_data.sid;
	sid = sid % FLASH_DATA_MAX;
	if(sid < FLASH_DATA_CENTER)
	{
		gpFlashCurrOp->start_addr = DAILY_ADDR1;
		gpFlashCurrOp->p_fstorage = &ex_fstorage;
	}
	else
	{
		gpFlashCurrOp->start_addr =  DAILY_ADDR2;
		gpFlashCurrOp->p_fstorage = &ex_fstorage2;
	}
	sid = sid % FLASH_DATA_CENTER;
	
	rc = nrf_fstorage_init(gpFlashCurrOp->p_fstorage, p_fs_api, NULL);
	ERR_CHECK_FLASH(rc,(FLASH_INIT_ERROR|EX_FLASH_OP_FILE_ID),__LINE__);
	
	rc = nrf_fstorage_read(gpFlashCurrOp->p_fstorage, gpFlashCurrOp->start_addr, (uint8_t*)&gFlash_daily_log, PAGE_SIZE);
	ERR_CHECK_FLASH(rc,(FLASH_READ_ERROR|EX_FLASH_OP_FILE_ID),__LINE__);
	
	gFlash_daily_log[0] = SIGN_DATA_ONE;
	gFlash_daily_log[1] = SIGN_DATA_TWO;
	gFlash_daily_log[2] = overCount;
	memcpy((uint8_t*)&gFlash_daily_log[(sid * 16) + 4],&daily_data,sizeof(Daily_t));
	
	gpFlashCurrOp->seq_id = INIT_CMD_WAIT_ER_DAILY_LOG;
	
	RamSaveSidIncrement();
	SetWriteSid();
	RamSaveRegionWalkInfoClear();
	InitRamDateSet(&(gpFlashCurrOp->time));

#ifdef TEST_TIME_SET_TO_ERROR
	return 0;
#endif
	rc = nrf_fstorage_erase(gpFlashCurrOp->p_fstorage, gpFlashCurrOp->start_addr, 1, NULL);
	ERR_CHECK_FLASH(rc,(FLASH_ERASE_ERROR|EX_FLASH_OP_FILE_ID),__LINE__); 

	return 0;
}

/**
 * @brief Time Set Flash Uninit
 * @param pEvent Event Information
 * @retval 0 Success
 */
static uint32_t time_set_flash_uninit(PEVT_ST pEvent)
{
	ret_code_t rc;
#if !defined(NOTIFY_SHARED)
	uint32_t fifo_err;
	EVT_ST event;
#endif

	rc = nrf_fstorage_uninit(gpFlashCurrOp->p_fstorage,NULL);
	ERR_CHECK_FLASH(rc,(FLASH_UNINIT_ERROR|EX_FLASH_OP_FILE_ID),__LINE__);
	
	//change slave latency 0
	gpFlashCurrOp->seq_id = INIT_CMD_WAIT_SLAVE_LATENCY_ZERO;
	gpFlashCurrOp->op[INIT_CMD_WAIT_SLAVE_LATENCY_ZERO] = 1;
	/* 2020.09.18 Add Notify処理を共通化 */
#if defined(NOTIFY_SHARED)
	notify_push_message( EVT_FLASH_CNT_PARAM_RETURN, NULL );
#else
	event.evt_id = EVT_FLASH_CNT_PARAM_RETURN;
	fifo_err = PushFifo(&event);
	DEBUG_EVT_FIFO_LOG(fifo_err,event.evt_id);
#endif
	
	return 0;
}

/**
 * @brief event EVT_FLASH_CMPL_ERASE. state State FLASH_SET_TIME Event
 * @param pEvent Event Information
 * @retval 0 Success
 */
static uint32_t time_set_daily_log_write(PEVT_ST pEvent)
{
	ret_code_t rc;

	DEBUG_LOG(LOG_INFO,"SET TIME daily log flash write");
	gpFlashCurrOp->seq_id = INIT_CMD_WAIT_WR_DAILY_LOG;
	rc = nrf_fstorage_write(gpFlashCurrOp->p_fstorage, gpFlashCurrOp->start_addr, (uint8_t*)&gFlash_daily_log, PAGE_SIZE, NULL);
	ERR_CHECK_FLASH(rc,(FLASH_WRITE_ERROR|EX_FLASH_OP_FILE_ID),__LINE__);
	
	return 0;
}

/**
 * @brief state cnt, event EVT_FLASH_OP_STATUS_CHECK. flash state FLASH_SET_PLAYER
 *        pincode access check reply ack
 * @param pEvent Event Information
 * @retval 0 Success
 */
static uint32_t init_access_pincode_reply_notify(PEVT_ST pEvent)
{
	uint8_t notifyAck;
#ifdef INIT_3_ACK_TIMER_ON
	StartFlashTimer();
#endif
	notifyAck = BLE_NOTIFY_SIG;
	BleNotifySignals(&notifyAck,(uint16_t)sizeof(notifyAck));
	DEBUG_LOG(LOG_INFO, "pin Ack");	
	FlashOpForceInit();
	//forceDiscon timer restart
	RestartForceDisconTimer();
				
	return 0;
}

/****************************************************************************************
 * flash ble pin code check
 ****************************************************************************************/

/**
 * @brief pin code check 
 *        state cnt, event EVT_FLASH_OP_STATUS_CHECK. flash state FLASH_CHECK_PINCODE.
 * @param pEvent Event Information
 * @retval 0 Success
 */
static uint32_t init_pincode_check_state(PEVT_ST pEvent)
{
#if !defined(NOTIFY_SHARED)
	uint32_t fifo_err;
#endif
	uint16_t notify_handle;
	uint8_t pinCode[PIN_CODE_SIZE];
	int8_t err_code;
#if !defined(NOTIFY_SHARED)
	EVT_ST event;
	EVT_ST replyEvent;
#endif
	uint8_t notify_sig;
	bool accessPin;
	
	err_code = PIN_NOT_MATCH;
	GetGattsCharHandleValueID(&notify_handle,PAIRING_MATCH_ID);
	//notify_handle = gPairing_match_id.value_handle;

	StopFlashTimer();
	DEBUG_LOG(LOG_INFO,"Pairing code check function enter");
	accessPin = AccessPinCodeFlagGet();	
	if(accessPin == true)
	{	
		StartFlashTimer();
		memcpy(&pinCode,&pEvent->DATA.pinCode.pinCode,sizeof(pinCode));
		err_code = CheckPairing(pinCode);
		if(err_code == PIN_MATCH)
		{
			DEBUG_LOG(LOG_INFO,"Pairing code is matching");
			//paring flag set
			PinCodeCheckFlagSet();
			gpFlashCurrOp->op[INIT_CMD_PINCODE_CHECK_MATCH] = 1;

		}
		else if(err_code == PIN_NOT_MATCH)
		{
			DEBUG_LOG(LOG_INFO,"Pairing code is not match");
			gpFlashCurrOp->op[INIT_CMD_PINCODE_CHECK_NOMATCH] = 1;
		}
		else
		{
			//future change.
			//pin code not exist.
			DEBUG_LOG(LOG_INFO,"Pairing code is not Exsit");
		}
		
		if(PIN_NOT_EXIST == err_code)
		{
			gpFlashCurrOp->op[INIT_CMD_WAIT_ER_PIN_CODE] 		= 1;
			gpFlashCurrOp->op[INIT_CMD_WAIT_WR_PIN_CODE] 		= 1;
			gpFlashCurrOp->op[INIT_CMD_WAIT_SLAVE_LATENCY_ZERO]	= 1;
			gpFlashCurrOp->op[INIT_CMD_WAIT_SLAVE_LATENCY_ONE]	= 1;
		
			ParamUpdateRetryCountClear();
			memset((uint8_t*)&gPin_player_page, 0xff, sizeof(gPin_player_page));
			memcpy((uint8_t*)&gPin_player_page[2], pEvent->DATA.pinCode.pinCode, sizeof(pEvent->DATA.pinCode.pinCode));
		
			/* 2020.09.18 Add Notify処理を共通化 */
#if defined(NOTIFY_SHARED)
			notify_push_message( EVT_FLASH_CNT_PARAM_CHANGE, NULL );
			DEBUG_LOG(LOG_INFO,"Pairing code check function change sl param 0x%x",EVT_FLASH_CNT_PARAM_CHANGE);
#else
			event.evt_id = EVT_FLASH_CNT_PARAM_CHANGE;
			DEBUG_LOG(LOG_INFO,"Pairing code check function change sl param 0x%x",event.evt_id);
			fifo_err = PushFifo(&event);
			DEBUG_EVT_FIFO_LOG(fifo_err,event.evt_id);
#endif
		
			/* 2020.09.18 Add Notify処理を共通化 */
#if defined(NOTIFY_SHARED)
			notify_push_message( EVT_INIT_CMD_START, pEvent );
#else
			memcpy(&replyEvent,pEvent,sizeof(replyEvent));
			replyEvent.evt_id = EVT_INIT_CMD_START;
			fifo_err = PushFifo(&replyEvent);		
			DEBUG_EVT_FIFO_LOG(fifo_err,replyEvent.evt_id);
#endif
		}
		else
		{
			/* 2020.09.18 Add Notify処理を共通化 */
#if defined(NOTIFY_SHARED)
			notify_push_message( EVT_FLASH_CNT_PARAM_RETURN, NULL );
#else
			event.evt_id = EVT_FLASH_CNT_PARAM_RETURN;
			fifo_err = PushFifo(&event);
			DEBUG_EVT_FIFO_LOG(fifo_err,event.evt_id);
#endif
			StopFlashTimer();
		}
	}
	else
	{
		DEBUG_LOG(LOG_INFO,"Pairing code check access NG");
		gpFlashCurrOp->op[INIT_CMD_PINCODE_ACCESS_NG] = 1;
		
		//20180903 
#if 1
		/* 2020.09.18 Add Notify処理を共通化 */
#if defined(NOTIFY_SHARED)
		notify_push_message( EVT_FLASH_CNT_PARAM_RETURN, NULL );
#else
		event.evt_id = EVT_FLASH_CNT_PARAM_RETURN;
		fifo_err = PushFifo(&event);
		DEBUG_EVT_FIFO_LOG(fifo_err,event.evt_id);
#endif

#endif
		notify_sig    = BLE_PINCODE_MATCH_ERROR;
		GetGattsCharHandleValueID(&notify_handle,PAIRING_MATCH_ID);
		SetBleNotifyCmd(notify_handle, &notify_sig ,sizeof(notify_sig));
		StopFlashTimer();
		FlashOpForceInit();

	}
	RestartForceDisconTimer();
	return 0;
}

/**
 * @brief pin code check 
 *        flash pin code check erase
 * @param pEvent Event Information
 * @retval 0 Success
 */
static uint32_t flash_pincode_check_erase(PEVT_ST pEvent)
{
	ret_code_t rc;
	nrf_fstorage_api_t *p_fs_api;
	uint16_t checkdata;
	uint8_t tmpPage[PLAYER_AND_PAIRING_PAGE_SIZE];
	
	checkdata = CHECK_DATA;
	StartFlashTimer();
	//future change.
	DEBUG_LOG(LOG_INFO,"pincode write. pairing erase");
	
	p_fs_api   = &nrf_fstorage_sd;
	gpFlashCurrOp->p_fstorage = &player_pincode_fstorage;
	gpFlashCurrOp->start_addr = PLAYER_AND_PAIRING_ADDR;
	
	rc = nrf_fstorage_init(gpFlashCurrOp->p_fstorage, p_fs_api, NULL);
	ERR_CHECK_FLASH(rc,(FLASH_INIT_ERROR|EX_FLASH_OP_FILE_ID),__LINE__);
	
	//player info save save
	rc = nrf_fstorage_read(gpFlashCurrOp->p_fstorage, gpFlashCurrOp->start_addr, (uint8_t*)&tmpPage, sizeof(tmpPage));
	ERR_CHECK_FLASH(rc,(FLASH_READ_ERROR|EX_FLASH_OP_FILE_ID),__LINE__);
	
	memcpy((uint8_t*)&gPin_player_page[FLASH_PLAYER_START_POS],&tmpPage[FLASH_PLAYER_START_POS],PALYER_DATA_SIZE);
	memcpy((uint8_t*)&gPin_player_page[0], &checkdata, sizeof(checkdata));
	/* Erase the DATA_STORAGE_PAGE before write operation */
	rc = nrf_fstorage_erase(gpFlashCurrOp->p_fstorage, gpFlashCurrOp->start_addr, 1, NULL);
	ERR_CHECK_FLASH(rc,(FLASH_ERASE_ERROR|EX_FLASH_OP_FILE_ID),__LINE__);
	
	return 0;
}

/**
 * @brief pin code check 
 *        Flash pin code check write
 * @param pEvent Event Information
 * @retval 0 Success
 */
static uint32_t flash_pincode_check_write(PEVT_ST pEvent)
{
	ret_code_t rc;
	
	DEBUG_LOG(LOG_INFO,"enter pairing write");
	
#ifdef TEST_PINCODE_WRITE_TO_ERROR
	return 0;
#endif
	
	rc = nrf_fstorage_write(gpFlashCurrOp->p_fstorage, gpFlashCurrOp->start_addr, (uint8_t*)&gPin_player_page, sizeof(gPin_player_page), NULL);
	ERR_CHECK_FLASH(rc, (FLASH_WRITE_ERROR|EX_FLASH_OP_FILE_ID),__LINE__);
	
	return 0;
}

/**
 * @brief pin code check 
 *        Flash pin code check uninit
 * @param pEvent Event Information
 * @retval 0 Success
 */
static uint32_t flash_pincode_check_uninit(PEVT_ST pEvent)
{
#if !defined(NOTIFY_SHARED)
	uint32_t fifo_err;
#endif
	ret_code_t rc;
#if !defined(NOTIFY_SHARED)
	EVT_ST event;
#endif

	PinCodeCheckFlagSet();
	
	rc = nrf_fstorage_uninit(gpFlashCurrOp->p_fstorage,NULL);
	ERR_CHECK_FLASH(rc,(FLASH_UNINIT_ERROR|EX_FLASH_OP_FILE_ID),__LINE__);
	
	gpFlashCurrOp->seq_id = INIT_CMD_WAIT_SLAVE_LATENCY_ZERO;
	
	/* 2020.09.18 Add Notify処理を共通化 */
#if defined(NOTIFY_SHARED)
	notify_push_message( EVT_FLASH_CNT_PARAM_RETURN, NULL );
#else
	event.evt_id = EVT_FLASH_CNT_PARAM_RETURN;
	fifo_err = PushFifo(&event);
	DEBUG_EVT_FIFO_LOG(fifo_err,event.evt_id);
#endif
	
	return 0;
}

/****************************************************************************************
 * flash ble pin code erase
 ****************************************************************************************/

/**
 * @brief pin_code erase
 *        main state cnt, event EVT_FLASH_OP_STATUS_CHECK. flash state FLASH_ERASE_PINCODE.
 * @param pEvent Event Information
 * @retval 0 Success
 */
static uint32_t pin_code_erase_state_check(PEVT_ST pEvent)
{
#if !defined(NOTIFY_SHARED)
	uint32_t fifo_err;
	EVT_ST event;
	EVT_ST replyEvent;
#endif
	bool accessPin = false;
	
	StopFlashTimer();
	accessPin = AccessPinCodeFlagGet();	
	if(accessPin == true)
	{
		StartFlashTimer();
		if(pEvent->DATA.pincodeClear.pinCodeClear == 1)
		{
			DEBUG_LOG(LOG_INFO,"pincode erase flag OK");
			gpFlashCurrOp->op[INIT_CMD_WAIT_ER_PIN_CODE] = 1;
			gpFlashCurrOp->op[INIT_CMD_WAIT_WR_PIN_CODE] = 1;
			gpFlashCurrOp->op[INIT_CMD_WAIT_SLAVE_LATENCY_ZERO] = 1;
			gpFlashCurrOp->op[INIT_CMD_WAIT_SLAVE_LATENCY_ONE]  = 1;
			ParamUpdateRetryCountClear();
			/* 2020.09.18 Add Notify処理を共通化 */
#if defined(NOTIFY_SHARED)
			notify_push_message( EVT_FLASH_CNT_PARAM_CHANGE, NULL );
#else
			event.evt_id = EVT_FLASH_CNT_PARAM_CHANGE;
			fifo_err = PushFifo(&event);
			DEBUG_EVT_FIFO_LOG(fifo_err,event.evt_id);
#endif
	
			/* 2020.09.18 Add Notify処理を共通化 */
#if defined(NOTIFY_SHARED)
			notify_push_message( EVT_INIT_CMD_START, pEvent );
#else
			memcpy(&replyEvent,pEvent,sizeof(replyEvent));
			replyEvent.evt_id = EVT_INIT_CMD_START;
			fifo_err = PushFifo(&replyEvent);
			DEBUG_EVT_FIFO_LOG(fifo_err,replyEvent.evt_id);
#endif
		}
		else
		{
			DEBUG_LOG(LOG_ERROR,"Erase flag NG");
			StopFlashTimer();
			FlashOpForceInit();
		}
	}
	else
	{
		DEBUG_LOG(LOG_ERROR,"Erase access pin NG");
		StopFlashTimer();
		FlashOpForceInit();
	}
	
	RestartForceDisconTimer();
	
	return 0;
}

/**
 * @brief pin_code erase
 *        Flash pin code erase player rewrite
 * @param pEvent Event Information
 * @retval 0 Success
 */
static uint32_t flash_pincode_erase_player_rewrite(PEVT_ST pEvent)
{
	ret_code_t rc;
	
	DEBUG_LOG(LOG_INFO,"flash rewrite player into pairing erase");
#ifdef TEST_PINCODE_CLEAR_TO_ERROR 
	return 0;
#endif	
	rc = nrf_fstorage_write(gpFlashCurrOp->p_fstorage, (gpFlashCurrOp->start_addr + FLASH_PLAYER_WRITE_POS), (uint8_t*)&gPin_player_page[PLAYER_CHECK_DATA_POS], PALYER_DATA_SIZE, NULL);
	ERR_CHECK_FLASH(rc, (FLASH_WRITE_ERROR|EX_FLASH_OP_FILE_ID),__LINE__);
	
	return 0;
}

/**
 * @brief pin_code erase
 *        Flash pin code erase uninit
 * @param pEvent Event Information
 * @retval 0 Success
 */
static uint32_t flash_pincode_erase_uninit(PEVT_ST pEvent)
{
#if !defined(NOTIFY_SHARED)
	uint32_t fifo_err;
#endif
	ret_code_t rc;
#if !defined(NOTIFY_SHARED)
	EVT_ST event;
#endif
	//future change
	DEBUG_LOG(LOG_INFO,"erase pin code flash uninit");
	
	PinCodeCheckFlagClear();
	
	rc = nrf_fstorage_uninit(gpFlashCurrOp->p_fstorage,NULL);
	ERR_CHECK_FLASH(rc,(FLASH_UNINIT_ERROR|EX_FLASH_OP_FILE_ID),__LINE__);
	
	/* 2020.09.18 Add Notify処理を共通化 */
#if defined(NOTIFY_SHARED)
	notify_push_message( EVT_FLASH_CNT_PARAM_RETURN, NULL );
#else
	event.evt_id = EVT_FLASH_CNT_PARAM_RETURN;
	fifo_err = PushFifo(&event);
	DEBUG_EVT_FIFO_LOG(fifo_err,event.evt_id);
#endif
	
	gpFlashCurrOp->seq_id = INIT_CMD_WAIT_SLAVE_LATENCY_ZERO;
	
	return 0;
}

/****************************************************************************************
 * RTC code erase
 ****************************************************************************************/

/**
 * @brief RTC code erase
 *        RTC Interrupt Initialzie State Check
 * @param pEvent Event Information
 * @retval 0 Success
 */
static uint32_t rtc_int_state_check(PEVT_ST pEvent)
{
#if !defined(NOTIFY_SHARED)
	EVT_ST event;
	EVT_ST replyEvent;
	uint32_t fifo_err;
#endif
	Daily_t walk_data;
	DATE_TIME ram_dateTime;
	DATE_TIME rtc_dateTime;
	ret_code_t rtc_err;
	bool bPastRet;
	
	DEBUG_LOG(LOG_INFO,"cnt state. Write daily log with RTC INT");
	memset(&ram_dateTime,0x00,sizeof(ram_dateTime));
	memset(&rtc_dateTime,0x00,sizeof(rtc_dateTime));
	GetRamTime(&ram_dateTime);
	GetRamDailyLog(&walk_data);
	
	rtc_err = ExRtcGetDateTime(&rtc_dateTime);
	if(rtc_err == NRF_SUCCESS)
	{
		// No!! check rtc time, compare ram save region.
		bPastRet = CheckCurrTime(&rtc_dateTime);
		if(bPastRet == CHECK_VALID)
		{
			memcpy(&(gpFlashCurrOp->time),&rtc_dateTime,sizeof(rtc_dateTime));
			if((uint16_t)0 < (walk_data.dash + walk_data.run + walk_data.walk))
			{
				DEBUG_LOG(LOG_INFO,"cnt state. RTC int walk count w, %u, r %u, d %u",walk_data.walk, walk_data.run, walk_data.dash);
				
				gpFlashCurrOp->op[INIT_CMD_WAIT_ER_DAILY_LOG]		= 1;
				gpFlashCurrOp->op[INIT_CMD_WAIT_WR_DAILY_LOG]		= 1;
				gpFlashCurrOp->op[INIT_CMD_WAIT_SLAVE_LATENCY_ZERO]	= 1;
				gpFlashCurrOp->op[INIT_CMD_WAIT_SLAVE_LATENCY_ONE]	= 1;
				/* 2020.09.18 Add Notify処理を共通化 */
#if defined(NOTIFY_SHARED)
				notify_push_message( EVT_FLASH_CNT_PARAM_CHANGE, NULL );
#else
				event.evt_id = EVT_FLASH_CNT_PARAM_CHANGE;
				fifo_err = PushFifo(&event);
				DEBUG_EVT_FIFO_LOG(fifo_err,event.evt_id);
#endif
				
				/* 2020.09.18 Add Notify処理を共通化 */
#if defined(NOTIFY_SHARED)
				notify_push_message( EVT_INIT_CMD_START, pEvent );
#else
				memcpy(&replyEvent,pEvent,sizeof(replyEvent));
				replyEvent.evt_id = EVT_INIT_CMD_START;
				fifo_err = PushFifo(&replyEvent);
				DEBUG_EVT_FIFO_LOG(fifo_err,replyEvent.evt_id);
#endif
			}
			else
			{
				InitRamDateSet(&rtc_dateTime);
				FlashOpForceInit();
			}
		}
		else
		{
			FlashOpForceInit();
		}
	}
	else
	{
		TRACE_LOG(TR_RTC_CNT_INT_SET_TIME_ERROR,0);
		WriteRamOneHourProgressTime(&ram_dateTime);
		FlashOpForceInit();
	}
	
	/* 2022.01.26 Add RTC割り込みを有効化 ++ */
	ExRtcEnableInterrupt();
	/* 2022.01.26 Add RTC割り込みを有効化 -- */
	
	return 0;
}

/**
 * @brief RTC code erase
 *        RTC Interrupt UnInitialzie Flash
 * @param pEvent Event Information
 * @retval 0 Success
 */
static uint32_t rtc_int_flash_uninit(PEVT_ST pEvent)
{
	ret_code_t rc;
#if !defined(NOTIFY_SHARED)
	uint32_t fifo_err;
	EVT_ST event;
#endif

	rc = nrf_fstorage_uninit(gpFlashCurrOp->p_fstorage,NULL);
	ERR_CHECK_FLASH(rc,(FLASH_UNINIT_ERROR|EX_FLASH_OP_FILE_ID),__LINE__);

	//change slave latency 0
	gpFlashCurrOp->seq_id = INIT_CMD_WAIT_SLAVE_LATENCY_ZERO;
	/* 2020.09.18 Add Notify処理を共通化 */
#if defined(NOTIFY_SHARED)
	notify_push_message( EVT_FLASH_CNT_PARAM_RETURN, NULL );
#else
	event.evt_id = EVT_FLASH_CNT_PARAM_RETURN;
	fifo_err = PushFifo(&event);
	DEBUG_EVT_FIFO_LOG(fifo_err,event.evt_id);
#endif
	
	return 0;
}

/**
 * @brief clear Flash Sector
 * @param p_fstorage fstorage instance
 * @param start_addr Eraseを開始するアドレス
 * @retval None
 */
static void clear_flash_sector(nrf_fstorage_t *p_fstorage, uint32_t start_addr)
{
	ret_code_t rc;
	DATE_TIME  rtcdateTime;

	rc = ExRtcGetDateTime(&rtcdateTime);
	if(rc == NRF_SUCCESS)
	{
		RamSaveRegionWalkInfoClear();
		RamSidClear();
		VolFlashClear();
		InitRamDateSet(&rtcdateTime);
		if(start_addr == DAILY_ADDR2)
		{
			DEBUG_LOG(LOG_DEBUG,"daily 2 sector p_fst addr 0x%x",p_fstorage->start_addr);
		}
		rc = nrf_fstorage_erase(p_fstorage, start_addr, 1, NULL);
		ERR_CHECK_FLASH(rc,(FLASH_ERASE_ERROR|EX_FLASH_OP_FILE_ID),__LINE__);
	}
	else
	{
		LIB_ERR_CHECK(rc,I2C_READ,__LINE__);
	}
}

/**
 * @brief clear Flash Sector
 * @remark 一度FlashからPlayer情報を読み出しRetention RAM上に書き込んでからErace処理を実行する
 * @param p_fstorage fstorage instance
 * @param p_fs_api fsorage softdevice api instance
 * @param *start_addr Read and Erace Start Address
 * @param pEvent Event Information
 * @retval None
 */
static void player_info_erase(nrf_fstorage_t *p_fstorage, nrf_fstorage_api_t *p_fs_api, uint32_t *start_addr, PEVT_ST pEvent) 
{
	ret_code_t rc;
	
	*start_addr = PLAYER_AND_PAIRING_ADDR;
	rc = nrf_fstorage_read(p_fstorage, *start_addr, (uint8_t*)&gPin_player_page, sizeof(gPin_player_page));
	ERR_CHECK_FLASH(rc,(FLASH_READ_ERROR|EX_FLASH_OP_FILE_ID),__LINE__);
	
	gPin_player_page[PLAYER_CHECK_DATA_POS]     = SIGN_DATA_ONE;
	gPin_player_page[PLAYER_CHECK_DATA_POS + 1] = SIGN_DATA_TWO;
	gPin_player_page[PLAYER_CHECK_DATA_POS + 2] = pEvent->DATA.dataInfo.age;
	gPin_player_page[PLAYER_CHECK_DATA_POS + 3] = pEvent->DATA.dataInfo.sex;
	gPin_player_page[PLAYER_CHECK_DATA_POS + 4] = pEvent->DATA.dataInfo.weight;
	//algorithm player infomation set
	SetRamPlayerInfo(pEvent->DATA.dataInfo.age,pEvent->DATA.dataInfo.sex,pEvent->DATA.dataInfo.weight);
	DEBUG_LOG(LOG_INFO,"init cmd player info erase");
	rc = nrf_fstorage_erase(p_fstorage, *start_addr, 1, NULL);
	ERR_CHECK_FLASH(rc,(FLASH_ERASE_ERROR|EX_FLASH_OP_FILE_ID),__LINE__);
}

/**
 * @brief Daily Log Write
 * @param p_fstorage fstorage instance
 * @param start_addr Write Start Address
 * @retval None
 */
static void daily_log_write(nrf_fstorage_t *p_fstorage, uint32_t start_addr)
{
	ret_code_t rc;
	
	rc = nrf_fstorage_write(p_fstorage, start_addr, (uint8_t*)&gFlash_daily_log, PAGE_SIZE,NULL);
	ERR_CHECK_FLASH(rc,(FLASH_WRITE_ERROR|EX_FLASH_OP_FILE_ID),__LINE__);	
}

/**
 * @brief Daily Log Erace Prepare
 * @param sid SID
 * @param sector_bit Flash Sector Bit
 * @param start_addr Daily Log Save Address
 * @param checkTime check time
 * @param rtcdateTime date time information
 * @retval None
 */
static void daily_log_erase_prepare(uint16_t *sid, uint8_t *sector_bit, uint32_t *start_addr, bool *checkTime, DATE_TIME *rtcdateTime)
{
	ret_code_t rc;
	Daily_t   daily_data;
	
	rc = ExRtcGetDateTime(rtcdateTime);
	if(rc == NRF_SUCCESS)
	{
		*checkTime = CheckCurrTime(rtcdateTime);
		if(*checkTime)
		{
			GetRamDailyLog(&daily_data);

			*sid = daily_data.sid;
			*sid = *sid % FLASH_DATA_MAX;
			if(*sid < FLASH_DATA_CENTER)
			{
				*start_addr = DAILY_ADDR1;
				*sector_bit |= (0 << 1);
			}
			else
			{
				*start_addr = DAILY_ADDR2;
				*sector_bit |= (1 << 1);
			}
			*sid = *sid % FLASH_DATA_CENTER;
		}
	}
	else
	{
		LIB_ERR_CHECK(rc,I2C_READ,__LINE__);
	}
}

/**
 * @brief Daily Log Erace
 * @param p_fstorage fstorage instance
 * @param start_addr Daily Log Save Address
 * @param sid SID
 * @param rtcdateTime date time information
 * @retval None
 */
static void daily_log_erase(nrf_fstorage_t *p_fstorage, uint32_t start_addr, uint16_t sid, DATE_TIME *rtcdateTime)
{
	ret_code_t rc;
	Daily_t   daily_data;
	uint8_t overCount;

	GetRamDailyLog(&daily_data);
	overCount = GetSidOverCount();

	rc = nrf_fstorage_read(p_fstorage, start_addr, (uint8_t*)&gFlash_daily_log, PAGE_SIZE);
	ERR_CHECK_FLASH(rc,(FLASH_READ_ERROR|EX_FLASH_OP_FILE_ID),__LINE__);
	
	gFlash_daily_log[0] = SIGN_DATA_ONE;
	gFlash_daily_log[1] = SIGN_DATA_TWO;
	gFlash_daily_log[2] = overCount;
	memcpy((uint8_t*)&gFlash_daily_log[(sid * 16) + 4],&daily_data,sizeof(Daily_t));
	RamSaveSidIncrement();
	SetWriteSid();
	RamSaveRegionWalkInfoClear();
	InitRamDateSet(rtcdateTime);
	rc = nrf_fstorage_erase(p_fstorage, start_addr, 1, NULL);
	ERR_CHECK_FLASH(rc,(FLASH_ERASE_ERROR|EX_FLASH_OP_FILE_ID),__LINE__);
}

/**
 * @brief Compare Date Information
 * @param firstData first date time information
 * @param secondData second date time information
 * @retval true No Match
 * @retval false Match
 */
static bool compare_time(DATE_TIME *firstData, DATE_TIME *secondData)
{
	
	if(firstData->year != secondData->year)
	{
		return true;
	}
	
	if(firstData->month != secondData->month)
	{
		return true;
	}
	
	if(firstData->day != secondData->day)
	{
		return true;
	}
	
	if(firstData->hour != secondData->hour)
	{
		return true;
	}
	
	return false;
}

/**
 * @brief Sex Check
 * @param sexData sex data
 * @retval UTC_SUCCESS male or female 
 * @retval UTC_ERROR male or female 以外
 */
static uint32_t sex_check(uint8_t sexData)
{
	uint32_t err_code;
	
	err_code = UTC_SUCCESS;
	
	if((sexData != MALE) && (sexData != FEMALE))
	{
		err_code = UTC_ERROR;
	}
	return err_code;
}

/**
 * @brief Date Check
 * @param pDateTime UNIX Timeを指定する
 * @retval UTC_SUCCESS Success
 * @retval UTC_ERROR Error
 */
static uint32_t date_check(uint8_t *pDateTime)
{
	uint32_t   ret;
	uint16_t  year;
	uint8_t      i;
	uint8_t  month;
	uint8_t    day;
	uint8_t   hour;
	uint8_t    min;
	uint8_t    sec;

	ret = UTC_SUCCESS;
	
	year  = *(pDateTime + YEAR_HIGH)  * 10  +  *(pDateTime + YEAR_LOW);
	month = *(pDateTime + MONTH_HIGH) * 10  +  *(pDateTime + MONTH_LOW);
	day   = *(pDateTime + DAY_HIGH)   * 10  +  *(pDateTime + DAY_LOW);
	hour  = *(pDateTime + HOUR_HIGH)  * 10  +  *(pDateTime + HOUR_LOW);
	min   = *(pDateTime + MIN_HIGH)   * 10  +  *(pDateTime + MIN_LOW);
	sec   = *(pDateTime + SEC_HIGH)   * 10  +  *(pDateTime + SEC_LOW);
	
	for(i = 0; i < SEC_LOW; i++)
	{
		if( 9 < *(pDateTime + i) )
		{
			ret = UTC_ERROR;
		}
	}
	
	if((month < JAN)|| (DEC < month))
	{
		ret = UTC_ERROR;
	}
	
	if(day <= 0)
	{
		ret = UTC_ERROR;
	}
	
	if(month == FEB)
	{
		if(year % 4 == 0)
		{
			if(29 < day)
			{
				ret = UTC_ERROR;
			}
		}
		else
		{
			if(28 < day)
			{
				ret = UTC_ERROR;
			}
		}
	}
	else if((month == APR) || (month == JUN) || (month == SEP) || (month == NOV))
	{
		if(30 < day)
		{
			ret = UTC_ERROR;
		}
	}
	else
	{
		if(31 < day)
		{
			ret = UTC_ERROR;
		}
	}
	
	if(24 < hour)
	{
		ret = UTC_ERROR;
	}
	if(59 < min)
	{
		ret = UTC_ERROR;
	}
	if(59 < sec)
	{
		ret = UTC_ERROR;
	}
	return ret;
}

/**
 * @brief Get Flash Operation Mode
 * @param p_op_mode 現在のoperation modeを返す
 * @retval None
 */
void GetFlashOperatoinMode(uint8_t *p_op_mode)
{
	*p_op_mode = gpFlashCurrOp->op_id;
}

/**
 * @brief Notify Message Push
 * @param event_id 通知するイベントID
 * @param src_event コピーするイベントがある場合指定する.ない場合NULLを指定
 * @retval None
 */
static void notify_push_message( uint8_t event_id, EVT_ST *src_event )
{
	EVT_ST notify_event = {0};
	uint32_t fifo_err;
	
	if ( src_event != NULL )
	{
		/* コピーするイベント情報がある場合、元のイベント情報をコピした後にMessage Pushする */
		memcpy( &notify_event, src_event, sizeof( notify_event ) );
	}
	/* イベントIDの変更がない場合コピー元のイベントIDをそのまま使用する */
	if ( event_id != EVENT_NO_MODIFY )
	{
		/* イベントIDを設定 */
		notify_event.evt_id = event_id;
	}
	
	/* メッセージPush */
	fifo_err = PushFifo( &notify_event );
	DEBUG_EVT_FIFO_LOG( fifo_err, notify_event.evt_id );
}


/*
//FW version 9.00 debug
static void debug_send_updateEvent(void)
{
	EVT_ST testEvent;
	uint32_t err_fifo;
	testEvent.evt_id = EVT_UPDATING_PARAM;
	err_fifo = PushFifo(&testEvent);
	DEBUG_EVT_FIFO_LOG(err_fifo,testEvent.evt_id);
	DEBUG_LOG(LOG_INFO,"debug test send updating event"); 	
	
}
*/
