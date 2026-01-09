/**
  ******************************************************************************************
  * @file    lib_timer.h
  * @author  k.tashiro
  * @version 1.0
  * @date    2020/09/15
  * @brief   APP_Timer Contorl
  ******************************************************************************************
    Revision:
    Ver            Date             Revised by        Explanation
  ******************************************************************************************
  * 1.0            2020/09/15       k.tashiro         create new
  ******************************************************************************************
*/

/* Includes --------------------------------------------------------------*/
#include "nrf_nvic.h"
#include "app_timer.h"
#include "lib_common.h"
#include "state_control.h"
#include "mode_manager.h"
#include "flash_operation.h"
#include "lib_ram_retain.h"
#include "lib_timer.h"
#include "lib_fifo.h"
#include "definition.h"
#include "lib_trace_log.h"

/* Definition ------------------------------------------------------------*/
#define UPDATE_RETRY_COUNT  3

//Timeout value. change unit Real Time Counter Value(ms -> app timer count value).
#define FORCE_TIMEOUT  					APP_TIMER_TICKS(FORCE_TIMEOUT_MS)
#define PARAM_UPDATE_TIMEOUT			APP_TIMER_TICKS(PARAM_UPDATE_TIMEOUT_MS)
#define PARAM_UPDATE_ERROR_TIMEOUT		APP_TIMER_TICKS(PARAM_UPDATE_ERROR_TIMEOUT_MS)
#define WAIT_DISCON_CALLBACK_TIMEOUT	APP_TIMER_TICKS(WAIT_DISCON_CALLBACK_TIMEOUT_MS)
#define ULT_ACK_TIMEOUT					APP_TIMER_TICKS(ULT_ACK_TIMEOUT_MS)
#define ULT_START_TIMEOUT				APP_TIMER_TICKS(ULT_START_TIMEOUT_MS)

#define INIT_TIMEOUT					APP_TIMER_TICKS(INIT_TIMEOUT_MS)

/* 2022.06.03 Add RSSI通知 ++ */
#define RSSI_NOTIFY_TIME				APP_TIMER_TICKS( RSSI_NOTIFY_MS )
/* 2022.06.03 Add RSSI通知 -- */

#define APP_TIMER_ERR_CHECK( err, event )                                                     \
	do {                                                                                      \
		if ( err != NRF_SUCCESS )                                                             \
		{                                                                                     \
			/*trace log*/                                                                     \
			TRACE_LOG( TR_TIMER_ERROR,(uint16_t)event );                                      \
			DEBUG_LOG( LOG_ERROR,"timer err. err code 0x%x. timer event 0x%x", err, event );  \
			sd_nvic_SystemReset();                                                            \
		}                                                                                     \
	} while( 0 )                                                                              \

/* Private variables -----------------------------------------------------*/
volatile uint8_t gCntUpdateCount = 0;

/*timer instance*/
APP_TIMER_DEF(gForcedDiscon_Timer);					//5h time out, force disconnect
APP_TIMER_DEF(gParamUpdate_Timer);					//ParamUpdate_Timer
APP_TIMER_DEF(gParamUpdateError_Timer);				//ParamUpdateError_Timer
APP_TIMER_DEF(gWaitDisconCallback_Timer);			//Disconnect call back timer.
APP_TIMER_DEF(gUltAck_Timer);						//ultimate mode, Ack Timer.
APP_TIMER_DEF(gUltStart_Tiemr);						//ultimate mode, start trigger timer instance
APP_TIMER_DEF(gUltEnd_Timer);						//ultimate mode, end trigger timer instance

APP_TIMER_DEF(gBat_Timer);
APP_TIMER_DEF(g_flash_timer);

/* 2022.06.03 Add RSSI通知 ++ */
APP_TIMER_DEF( g_rssi_notify );
/* 2022.06.03 Add RSSI通知 -- */


/* Private function prototypes -------------------------------------------*/
/**
 * @brief Force Disconnect Timeout event handler
 * @param None
 * @retval None
 */
static void force_discon_timeout_evt_handler(void);

/**
 * @brief Parameter Update Timeout event handler
 * @param None
 * @retval None
 */
static void param_update_timeout_evt_handler(void);

/**
 * @brief Parameter Update Error Timeout event handler
 * @param None
 * @retval None
 */
static void param_update_error_timeout_evt_handler(void);

/**
 * @brief Ult mode ACK Timeout event handler
 * @param None
 * @retval None
 */
static void ult_ack_timeout_evt_handler(void);

/**
 * @brief Wait Disconnect Callback Timeout event handler
 * @param None
 * @retval None
 */
static void wait_discon_callback_timeout_evt_handler(void);

/**
 * @brief Ult Start Triger Timeout event handler
 * @param None
 * @retval None
 */
static void start_trig_timeout_evt_handler(void);

/**
 * @brief Ult End Triger Timeout event handler
 * @param None
 * @retval None
 */
static void end_trig_timeout_evt_handler(void);

/**
 * @brief Battery Timeout event handler
 * @param None
 * @retval None
 */
static void bat_timeout_evt_handler(void);

/**
 * @brief Update Retry Count increment
 * @param None
 * @retval None
 */
static void update_retry_count_inc(void);

/**
 * @brief Update Retry Count Clear
 * @param None
 * @retval None
 */
static void get_update_retry_count(uint8_t *pRecCount);

/* 2022.06.03 Add RSSI通知 ++ */
/**
 * @brief RSSI Notify Timer event handler
 * @param None
 * @retval None
 */
static void rssi_notify_timer_evt_handler(void);
/* 2022.06.03 Add RSSI通知 -- */


/**
 * @brief app_timer Initialize
 * @param None
 * @retval None
 */
void LibTimersInit(void)
{
	ret_code_t err_code;
	// Initialize app timer module(RTC1).
	err_code	= app_timer_init();				
	DEBUG_LOG(LOG_DEBUG,"timer init err_code 0x%x",err_code);
	LIB_ERR_CHECK(err_code, APP_TIMER_INIT, __LINE__);
	
	// Force Diisconnect Timer
	err_code = app_timer_create(&gForcedDiscon_Timer, APP_TIMER_MODE_SINGLE_SHOT, (app_timer_timeout_handler_t)force_discon_timeout_evt_handler);
	LIB_ERR_CHECK(err_code, APP_TIMER_CREATE, __LINE__);
	// ParamUpdate timer
	err_code = app_timer_create(&gParamUpdate_Timer, APP_TIMER_MODE_SINGLE_SHOT, (app_timer_timeout_handler_t)param_update_timeout_evt_handler);
	LIB_ERR_CHECK(err_code, APP_TIMER_CREATE, __LINE__);
	//ParamUpdate Error timer
	err_code = app_timer_create(&gParamUpdateError_Timer, APP_TIMER_MODE_SINGLE_SHOT, (app_timer_timeout_handler_t)param_update_error_timeout_evt_handler);
	LIB_ERR_CHECK(err_code, APP_TIMER_CREATE, __LINE__);
	// Ult mode, return Ack timer
	err_code = app_timer_create(&gUltAck_Timer, APP_TIMER_MODE_SINGLE_SHOT, (app_timer_timeout_handler_t)ult_ack_timeout_evt_handler);
	LIB_ERR_CHECK(err_code, APP_TIMER_CREATE, __LINE__);
	//disconnect callback timer
	err_code = app_timer_create(&gWaitDisconCallback_Timer, APP_TIMER_MODE_SINGLE_SHOT, (app_timer_timeout_handler_t)wait_discon_callback_timeout_evt_handler);
	LIB_ERR_CHECK(err_code, APP_TIMER_CREATE, __LINE__);
	//Ultimate start force finish timer
	err_code = app_timer_create(&gUltStart_Tiemr, APP_TIMER_MODE_SINGLE_SHOT, (app_timer_timeout_handler_t)start_trig_timeout_evt_handler);
	LIB_ERR_CHECK(err_code, APP_TIMER_CREATE, __LINE__);
	//Ultimate End force finish timer
	err_code = app_timer_create(&gUltEnd_Timer, APP_TIMER_MODE_SINGLE_SHOT, (app_timer_timeout_handler_t)end_trig_timeout_evt_handler);
	LIB_ERR_CHECK(err_code, APP_TIMER_CREATE, __LINE__);
	
	err_code = app_timer_create(&gBat_Timer, APP_TIMER_MODE_SINGLE_SHOT, (app_timer_timeout_handler_t)bat_timeout_evt_handler);
	LIB_ERR_CHECK(err_code, APP_TIMER_CREATE, __LINE__);
	
	err_code = app_timer_create(&g_flash_timer, APP_TIMER_MODE_SINGLE_SHOT, (app_timer_timeout_handler_t)FlashTimeoutEvtHandler);
	LIB_ERR_CHECK(err_code, APP_TIMER_CREATE, __LINE__);
	
	/* 2022.06.03 Add RSSI通知 ++ */
	err_code = app_timer_create( &g_rssi_notify, APP_TIMER_MODE_REPEATED, (app_timer_timeout_handler_t)rssi_notify_timer_evt_handler );
	LIB_ERR_CHECK(err_code, APP_TIMER_CREATE, __LINE__);
	/* 2022.06.03 Add RSSI通知 -- */
}

/**
 * @brief Force Disconnect Timeout event handler[5 hour timeout. timeout handler]
 * @param None
 * @retval None
 */
static void force_discon_timeout_evt_handler(void)
{
	EVT_ST event;
	uint32_t fifo_err;
	bool uart_output_enable;
	
	uart_output_enable = GetUartOutputStatus();
	if(uart_output_enable == false)
	{
		LibUartEnable();
	}
	
	event.evt_id = EVT_FORCED_TO;
	fifo_err = PushFifo(&event);
	DEBUG_EVT_FIFO_LOG(fifo_err,event.evt_id);
	
	DEBUG_LOG(LOG_INFO,"discon timeout");
	TRACE_LOG(TR_FORCR_DISCONNECT_GENERATE,0);
	if(uart_output_enable == false)
	{
		LibUartDisable();
	}
	
}

/**
 * @brief Parameter Update Timeout event handler
 * @param None
 * @retval None
 */
static void param_update_timeout_evt_handler(void)
{
	EVT_ST event;
	uint32_t fifo_err;
	uint8_t retrycount;
	bool uart_output_enable;
	
	uart_output_enable = GetUartOutputStatus();
	if(uart_output_enable == false)
	{
		LibUartEnable();
	}
	
	get_update_retry_count(&retrycount);
	
	//New add 20180827
	if(retrycount < UPDATE_RETRY_COUNT)
	{
		DEBUG_LOG(LOG_INFO,"cnt update time out retry");
		event.evt_id = EVT_UPDATING_PARAM;
		update_retry_count_inc();
		RestartParamUpdateTimer();
	}
	else
	{
		event.evt_id = EVT_UPDATE_TO;	
	}
	fifo_err = PushFifo(&event);
	DEBUG_EVT_FIFO_LOG(fifo_err,event.evt_id);
	
	if(uart_output_enable == false)
	{
		LibUartDisable();
	}
}

/**
 * @brief Parameter Update Error Timeout event handler
 * @param None
 * @retval None
 */
static void param_update_error_timeout_evt_handler(void)
{
	EVT_ST event;
	uint32_t fifo_err;
	bool uart_output_enable;
	
	uart_output_enable = GetUartOutputStatus();
	if(uart_output_enable == false)
	{
		LibUartEnable();
	}
	
	event.evt_id = EVT_UPDATE_ERR_TO;
	fifo_err = PushFifo(&event);
	DEBUG_EVT_FIFO_LOG(fifo_err,event.evt_id);
	
	if(uart_output_enable == false)
	{
		LibUartDisable();
	}
}

/*Ult Ack timeout. timeout handler*/
static void ult_ack_timeout_evt_handler(void)
{
	EVT_ST 				event;
	uint32_t   fifo_err;
	bool 			uart_output_enable;
	
	uart_output_enable = GetUartOutputStatus();
	if(uart_output_enable == false)
	{
		LibUartEnable();
	}
	
	event.evt_id = EVT_BLE_ACK_TO;
	fifo_err = PushFifo(&event);
	DEBUG_EVT_FIFO_LOG(fifo_err, event.evt_id);
	
	if(uart_output_enable == false)
	{
		LibUartDisable();
	}
	
}

/**
 * @brief Wait Disconnect Callback Timeout event handler
 * @remark Action soft reset
 * @param None
 * @retval None
 */
static void wait_discon_callback_timeout_evt_handler(void)
{
	bool 			uart_output_enable;
	
	uart_output_enable = GetUartOutputStatus();
	if(uart_output_enable == false)
	{
		LibUartEnable();
	}
	DEBUG_LOG(LOG_INFO,"wait disconnect timer timeout");
	TRACE_LOG(TR_WAIT_DISONNECT_STATE_RESET,0);
	sd_nvic_SystemReset();						//soft reset.
	
	if(uart_output_enable == false)
	{
		LibUartDisable();
	}
	
}

/**
 * @brief Ult Start Triger Timeout event handler
 * @param None
 * @retval None
 */
static void start_trig_timeout_evt_handler(void)
{
	EVT_ST				event;
	uint32_t   fifo_err;
	bool 			uart_output_enable;
	
	uart_output_enable = GetUartOutputStatus();
	if(uart_output_enable == false)
	{
		LibUartEnable();
	}
	
	event.evt_id = EVT_ULT_START_TRIG_TO;
	fifo_err = PushFifo(&event);
	DEBUG_EVT_FIFO_LOG(fifo_err, event.evt_id);
	
	if(uart_output_enable == false)
	{
		LibUartDisable();
	}
}

/**
 * @brief Ult End Triger Timeout event handler
 * @param None
 * @retval None
 */
static void end_trig_timeout_evt_handler(void)
{
	EVT_ST event;
	uint32_t fifo_err;
	bool uart_output_enable;
	
	uart_output_enable = GetUartOutputStatus();
	if(uart_output_enable == false)
	{
		LibUartEnable();
	}
	
	//end trigger timeout event generate.
	event.evt_id = EVT_ULT_STOP_TRIG_TO;
	fifo_err = PushFifo(&event);
	DEBUG_EVT_FIFO_LOG(fifo_err, event.evt_id);
	if(uart_output_enable == false)
	{
		LibUartDisable();
	}	
}

/**
 * @brief Update Retry Count Increment
 * @param None
 * @retval None
 */
static void update_retry_count_inc(void)
{
	uint32_t err;
	uint8_t critical;
	
	err = sd_nvic_critical_region_enter(&critical);
	if(err == NRF_SUCCESS)
	{
		gCntUpdateCount++;
		critical = 0;
		err = sd_nvic_critical_region_exit(critical);
		if(err != NRF_SUCCESS)
		{
			//nothing to do.
		}
	}
}

/**
 * @brief Start Force Disconnect Timer (5 hour)
 * @param None
 * @retval None
 */
void StartForceDisconTimer(void)
{
	ret_code_t err_code;
	
	err_code = app_timer_start(gForcedDiscon_Timer,FORCE_TIMEOUT, NULL);
	APP_TIMER_ERR_CHECK(err_code, FORCE_DISCON_START_ERROR);
	DEBUG_LOG(LOG_INFO,"force timer start");
}

/**
 * @brief Stop Force Disconnect Timer
 * @param None
 * @retval None
 */
void StopForceDisconTimer(void)
{
	ret_code_t err_code;
	
	err_code = app_timer_stop(gForcedDiscon_Timer);
	APP_TIMER_ERR_CHECK(err_code, FORCE_DISCON_STOP_ERROR);
}

/**
 * @brief Restart Force Disconnect Timer
 * @param None
 * @retval None
 */
void RestartForceDisconTimer(void)
{
	StopForceDisconTimer();
	StartForceDisconTimer();
}

/**
 * @brief End trigger time out start
 * @param None
 * @retval None
 */
void StartUltEndTimer(void)
{
	uint32_t endTime;
	ret_code_t err_code;
	uint16_t timeout = 0;
	
	GetModeTimeoutValue(&timeout);
	
	//ultimate mode end trigger timeout set.
	endTime = APP_TIMER_TICKS(timeout * TIMER_UNIT_MS);
	//timer start.
	err_code = app_timer_start(gUltEnd_Timer, endTime, NULL);
	APP_TIMER_ERR_CHECK(err_code, ULT_END_TIMER_START_ERROR);
}

/**
 * @brief End trigger time out stop
 * @param None
 * @retval None
 */
void StopUltEndTimer(void)
{
	ret_code_t err_code;
	// timer start.
	err_code = app_timer_stop(gUltEnd_Timer);
	APP_TIMER_ERR_CHECK(err_code, ULT_END_TIMER_STOP_ERROR);
}

/**
 * @brief Start Ult Timer
 * @param None
 * @retval None
 */
void StartUltStartTimer(void)
{						
	ret_code_t err_code;
	// timer start.
	err_code = app_timer_start(gUltStart_Tiemr, ULT_START_TIMEOUT, NULL);
	APP_TIMER_ERR_CHECK(err_code, ULT_START_TIMER_START_ERROR);
}

/**
 * @brief Stop Ult Timer
 * @param None
 * @retval None
 */
void StopUltStartTimer(void)
{
	ret_code_t err_code;
	
	// timer start.
	err_code = app_timer_stop(gUltStart_Tiemr);
	APP_TIMER_ERR_CHECK(err_code, ULT_START_TIMER_STOP_ERROR);
}

/**
 * @brief Ble connection param Update Timer start
 * @param None
 * @retval None
 */
void StartParamUpdateTimer(void)
{
	ret_code_t  err_code;
	DEBUG_LOG(LOG_INFO,"param update timer start");
	err_code = app_timer_start(gParamUpdate_Timer, PARAM_UPDATE_TIMEOUT,NULL);
	APP_TIMER_ERR_CHECK(err_code, PARAM_UPDATE_TIMER_START_ERROR);
}

/**
 * @brief Ble connection param Update Timer stop
 * @param None
 * @retval None
 */
void StopParamUpdateTimer(void)
{
	ret_code_t err_code;
	DEBUG_LOG(LOG_INFO,"param update timer stop");
	err_code = app_timer_stop(gParamUpdate_Timer);
	APP_TIMER_ERR_CHECK(err_code, PARAM_UPDATE_TIMER_STOP_ERROR);
}

/**
 * @brief BLE Connection param Update Timer Restart
 * @param None
 * @retval None
 */
void RestartParamUpdateTimer(void)
{
	StopParamUpdateTimer();
	StartParamUpdateTimer();
}

/**
 * @brief Ble connection param Update error timer start
 * @param None
 * @retval None
 */
void StartParamUpdateErrorTimer(void)
{
	ret_code_t err_code;
	DEBUG_LOG(LOG_INFO,"param update err timer start");
	err_code = app_timer_start(gParamUpdateError_Timer, PARAM_UPDATE_ERROR_TIMEOUT, NULL);
	APP_TIMER_ERR_CHECK(err_code, PARAM_UPDATE_ERROR_TIMER_START_ERROR);
}

/**
 * @brief Ble connection param Update error timer end
 * @param None
 * @retval None
 */
void StopParamUpdateErrorTimer(void)
{
	ret_code_t err_code;
	
	err_code = app_timer_stop(gParamUpdateError_Timer);
	APP_TIMER_ERR_CHECK(err_code, PARAM_UPDATE_ERROR_TIMER_STOP_ERROR);
}

/**
 * @brief Ult mode. smartphone trrigger wait ack timer start
 * @param None
 * @retval None
 */
void StartAckTimer(void)
{
	ret_code_t err_code;
	
	err_code = app_timer_start(gUltAck_Timer, ULT_ACK_TIMEOUT, NULL);
	APP_TIMER_ERR_CHECK(err_code, ULT_ACK_TIMER_START_ERROR);

}

/**
 * @brief Ult mode. smartphone trrigger wait ack timer stop
 * @param None
 * @retval None
 */
void StopAckTimer(void)
{
	ret_code_t err_code;
	
	err_code = app_timer_stop(gUltAck_Timer);
	APP_TIMER_ERR_CHECK(err_code, ULT_ACK_TIMER_STOP_ERROR);
}

/**
 * @brief app_timer All Stop
 * @param None
 * @retval None
 */
void TimerAllStop(void)
{
	ret_code_t err_code;
	
	err_code = app_timer_stop_all();
	APP_TIMER_ERR_CHECK(err_code, ALL_TIMER_STOP_ERROR);
}

/**
 * @brief Start Wait Disconnect Callback Timer
 * @param None
 * @retval None
 */
void StartWaitDisconCallbackTimer(void)
{
	ret_code_t err_code;
	
	err_code = app_timer_start(gWaitDisconCallback_Timer, WAIT_DISCON_CALLBACK_TIMEOUT, NULL);
	APP_TIMER_ERR_CHECK(err_code, WAIT_DICON_TIMER_START_ERROR);
}

/**
 * @brief Stop Wait Disconnect Callback Timer
 * @param None
 * @retval None
 */
void StopWaitDisconCallbackTimer(void)
{
	ret_code_t err_code;
	
	err_code = app_timer_stop(gWaitDisconCallback_Timer);
	APP_TIMER_ERR_CHECK(err_code, WAIT_DICON_TIMER_STOP_ERROR);
}

/**
 * @brief Start Flash Timer
 * @param None
 * @retval None
 */
void StartFlashTimer(void)
{
	ret_code_t err_code;
	
	err_code = app_timer_start(g_flash_timer, INIT_TIMEOUT, NULL);
	APP_TIMER_ERR_CHECK(err_code, INIT_TIMER_START_ERROR);
	DEBUG_LOG(LOG_INFO,"Init timer start");
}

/**
 * @brief Stop Flash Timer
 * @param None
 * @retval None
 */
void StopFlashTimer(void)
{
	ret_code_t err_code;
	
	err_code = app_timer_stop(g_flash_timer);
	APP_TIMER_ERR_CHECK(err_code, INIT_TIMER_STOP_ERROR);
	DEBUG_LOG(LOG_DEBUG, "init timer stop");
}

/**
 * @brief Start BatTimer
 * @param None
 * @retval None
 */
void StartBatTimer( uint32_t timeout )
{
	ret_code_t err_code;

	err_code = app_timer_start(gBat_Timer, APP_TIMER_TICKS(timeout), NULL);
	APP_TIMER_ERR_CHECK(err_code, BAT_TIMER_START_ERROR);
}

/**
 * @brief Stop BatTimer
 * @param None
 * @retval None
 */
void StopBatTimer(void)
{
	ret_code_t err_code;
	
	err_code = app_timer_stop(gBat_Timer);
	APP_TIMER_ERR_CHECK(err_code, BAT_TIMER_STOP_ERROR);
}

/**
 * @brief Battery Timeout event handler
 * @param None
 * @retval None
 */
static void bat_timeout_evt_handler(void)
{
	bool uart_output_enable;

	uart_output_enable = GetUartOutputStatus();
	if(uart_output_enable == false)
	{
		LibUartEnable();
	}

	/* タイムアウト時のADC取得設定処理 */
	ReadAuthorizeBatteryInfoTimeout();

	if(uart_output_enable == false)
	{
		LibUartDisable();
	}
}

/**
 * @brief Get Update Retry Count
 * @param pRecCount Update リトライカウントを格納する変数へのポインタ
 * @retval None
 */
void UpdateRetryCountClear(void)
{
	uint32_t err;
	uint8_t critical;
	
	err = sd_nvic_critical_region_enter(&critical);
	if(err == NRF_SUCCESS)
	{
		gCntUpdateCount = 0;
		DEBUG_LOG(LOG_DEBUG,"clear param update retry timeout count %u",gCntUpdateCount);
		critical = 0;
		err = sd_nvic_critical_region_exit(critical);
		if(err != NRF_SUCCESS)
		{
			//nothing to do.
		}
	}
}

/**
 * @brief Update Retry Count Clear
 * @param None
 * @retval None
 */
static void get_update_retry_count(uint8_t *pRecCount)
{
	uint32_t err;
	uint8_t critical;
	
	err = sd_nvic_critical_region_enter(&critical);
	if(err == NRF_SUCCESS)
	{
		*pRecCount = gCntUpdateCount;
		critical = 0;
		err = sd_nvic_critical_region_exit(critical);
		if(err != NRF_SUCCESS)
		{
			//nothing to do.
		}
	}
}

/* 2022.06.03 Add RSSI通知 ++ */
/**
 * @brief RSSI Notify Timer event handler
 * @param None
 * @retval None
 */
static void rssi_notify_timer_evt_handler(void)
{
	EVT_ST event;
	uint32_t fifo_err;
	bool uart_output_enable;
	
	uart_output_enable = GetUartOutputStatus();
	if(uart_output_enable == false)
	{
		LibUartEnable();
	}
	
	//end trigger timeout event generate.
	event.evt_id = EVT_RSSI_NOTIFY;
	fifo_err = PushFifo(&event);
	DEBUG_EVT_FIFO_LOG(fifo_err, event.evt_id);
	if(uart_output_enable == false)
	{
		LibUartDisable();
	}
}

/**
 * @brief Start Rssi Notify Timer
 * @param None
 * @retval None
 */
void StartRssiNotifyTimer(void)
{
	ret_code_t err_code;
	
	err_code = app_timer_start( g_rssi_notify, RSSI_NOTIFY_TIME, NULL );
	APP_TIMER_ERR_CHECK( err_code, RSSI_NOTIFY_TIMER_START_ERROR );
	DEBUG_LOG( LOG_INFO, "!!! RSSI Notify Timer Start !!!" );
}

/**
 * @brief Stop Rssi Notify Timer
 * @param None
 * @retval None
 */
void StopRssiNotifyTimer(void)
{
	ret_code_t err_code;
	
	err_code = app_timer_stop( g_rssi_notify );
	APP_TIMER_ERR_CHECK(err_code, RSSI_NOTIFY_TIMER_STOP_ERROR);
	DEBUG_LOG( LOG_INFO, "!!! RSSI Notify Timer Stop !!!" );
}

/* 2022.06.03 Add RSSI通知 -- */




