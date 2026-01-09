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

#ifndef LIB_TIMER_H_
#define LIB_TIMER_H_

/* Includes --------------------------------------------------------------*/
#include "app_timer.h"
#include "lib_common.h"

#ifdef __cplusplus
extern "C"{
#endif

/* Enum ------------------------------------------------------------------*/
typedef enum
{
	FORCE_DISCON_START_ERROR = 0x00,
	FORCE_DISCON_STOP_ERROR,
	ULT_START_TIMER_START_ERROR,
	ULT_START_TIMER_STOP_ERROR,
	ULT_END_TIMER_START_ERROR,
	ULT_END_TIMER_STOP_ERROR,
	PARAM_UPDATE_TIMER_START_ERROR,
	PARAM_UPDATE_TIMER_STOP_ERROR,
	PARAM_UPDATE_ERROR_TIMER_START_ERROR,
	PARAM_UPDATE_ERROR_TIMER_STOP_ERROR,
	ULT_ACK_TIMER_START_ERROR,
	ULT_ACK_TIMER_STOP_ERROR,
	ALL_TIMER_STOP_ERROR,
	WAIT_DICON_TIMER_START_ERROR,
	WAIT_DICON_TIMER_STOP_ERROR,
	BAT_TIMER_START_ERROR,
	BAT_TIMER_STOP_ERROR,
	INIT_TIMER_START_ERROR,
	INIT_TIMER_STOP_ERROR,
	ACK_PIN_TIMER_START_ERROR,
	ACK_PIN_TIMER_STOP_ERROR,
	/* 2022.06.03 Add RSSI通知 ++ */
	RSSI_NOTIFY_TIMER_START_ERROR,
	RSSI_NOTIFY_TIMER_STOP_ERROR
	/* 2022.06.03 Add RSSI通知 -- */
} TIEMR_ERROR_EVENT;

/* Function prototypes ----------------------------------------------------*/
/**
 * @brief app_timer Initialize
 * @param None
 * @retval None
 */
void LibTimersInit(void);
//void utc_app_timers_init_create(void);

/**
 * @brief Start Force Disconnect Timer
 * @param None
 * @retval None
 */
//void utc_app_timer_forceDisconTimer_Start(void);
void StartForceDisconTimer(void);

/**
 * @brief Stop Force Disconnect Timer
 * @param None
 * @retval None
 */
//void utc_app_timer_forceDisconTimer_Stop(void);
void StopForceDisconTimer(void);

/**
 * @brief Restart Force Disconnect Timer
 * @param None
 * @retval None
 */
//void utc_app_timer_forceDisconTimer_restart(void);
void RestartForceDisconTimer(void);

/**
 * @brief Ble connection param Update Timer start
 * @param None
 * @retval None
 */
//void utc_app_timer_paramUpdateTimer_Start(void);
void StartParamUpdateTimer(void);

/**
 * @brief Ble connection param Update Timer stop
 * @param None
 * @retval None
 */
//void utc_app_timer_paramUpdateTimer_Stop(void);
void StopParamUpdateTimer(void);

/**
 * @brief BLE Connection param Update Timer Restart
 * @param None
 * @retval None
 */
//void utc_app_timer_paramUpdateTimer_restart(void);
void RestartParamUpdateTimer(void);
	
/**
 * @brief Ble connection param Update error timer start
 * @param None
 * @retval None
 */
//void utc_app_timer_paramUpdateErrorTimer_Start(void);
void StartParamUpdateErrorTimer(void);

/**
 * @brief Ble connection param Update error timer end
 * @param None
 * @retval None
 */
//void utc_app_timer_paramUpdateErrorTimer_Stop(void);
void StopParamUpdateErrorTimer(void);

/**
 * @brief Ult mode. smartphone trrigger wait ack timer start
 * @param None
 * @retval None
 */
//void utc_app_timer_AckTImer_Start(void);
void StartAckTimer(void);

/**
 * @brief Ult mode. smartphone trrigger wait ack timer stop
 * @param None
 * @retval None
 */
//void utc_app_timer_AckTImer_Stop(void);
void StopAckTimer(void);

/**
 * @brief app_timer All Stop
 * @param None
 * @retval None
 */
//void utc_app_timer_all_stop(void);
void TimerAllStop(void);

/**
 * @brief Start Wait Disconnect Callback Timer
 * @param None
 * @retval None
 */
//void utc_app_timer_WaitDisconCallbackTimer_Start(void);
void StartWaitDisconCallbackTimer(void);

/**
 * @brief Stop Wait Disconnect Callback Timer
 * @param None
 * @retval None
 */
//void utc_app_timer_WaitDisconCallbackTimer_Stop(void);
void StopWaitDisconCallbackTimer(void);


/**
 * @brief Start Ult Timer
 * @param None
 * @retval None
 */
//void utc_app_timer_UltStartTimer_Start(void);
void StartUltStartTimer(void);

/**
 * @brief Stop Ult Timer
 * @param None
 * @retval None
 */
//void utc_app_timer_UltStartTimer_Stop(void);
void StopUltStartTimer(void);

/**
 * @brief End trigger time out start
 * @param None
 * @retval None
 */
//void utc_app_timer_UltEndTimer_Start(void);
void StartUltEndTimer(void);

/**
 * @brief End trigger time out stop
 * @param None
 * @retval None
 */
//void utc_app_timer_UltEndTimer_Stop(void);
void StopUltEndTimer(void);

/**
 * @brief Start BatTimer
 * @param timeout Timeout Value
 * @retval None
 */
void StartBatTimer( uint32_t timeout );

/**
 * @brief Stop BatTimer
 * @param None
 * @retval None
 */
//void utc_app_timer_BatTimer_Stop(void);
void StopBatTimer(void);


/**
 * @brief Start Flash Timer
 * @param None
 * @retval None
 */
//void utc_app_timer_InitTimer_Start(void);
void StartFlashTimer(void);

/**
 * @brief Stop Flash Timer
 * @param None
 * @retval None
 */
//void utc_app_timer_InitTimer_Stop(void);
void StopFlashTimer(void);

/**
 * @brief Get Update Retry Count
 * @param pRecCount Update リトライカウントを格納する変数へのポインタ
 * @retval None
 */
void GetUpdateRetryCount(uint8_t *pRecCount);

/**
 * @brief Update Retry Count Clear
 * @param None
 * @retval None
 */
void UpdateRetryCountClear(void);

/* 2022.06.03 Add RSSI通知 ++ */
/**
 * @brief Start Rssi Notify Timer
 * @param None
 * @retval None
 */
void StartRssiNotifyTimer(void);

/**
 * @brief Stop Rssi Notify Timer
 * @param None
 * @retval None
 */
void StopRssiNotifyTimer(void);
/* 2022.06.03 Add RSSI通知 -- */


#ifdef __cplusplus
}
#endif

#endif
