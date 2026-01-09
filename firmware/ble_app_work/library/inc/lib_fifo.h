/**
  ******************************************************************************************
  * @file    lib_fifo.h
  * @author  k.tashiro
  * @version 1.0
  * @date    2020/09/15
  * @brief   FIFO Contorl (Message AtomicFIFO)
  ******************************************************************************************
    Revision:
    Ver            Date             Revised by        Explanation
  ******************************************************************************************
  * 1.0            2020/09/15       k.tashiro         create new
  ******************************************************************************************
*/

#ifndef LIB_FIFO_H_
#define LIB_FIFO_H_

/* Includes --------------------------------------------------------------*/
#include "lib_common.h"
#include "state_control.h"
#include "lib_icm42607.h"
#include "ble_gatts.h"
#include "lib_bat.h"

/* Definition -------------------------------------------------------------*/
#define DEBUG_EVT_FIFO_LOG DebugEvtFifoLog

/* Function prototypes ----------------------------------------------------*/
/**
 * @brief FIFO create
 * @param None
 * @retval None
 */
void FifoCreate(void);

/**
 * @brief Push Fifo Data(Event用)
 * @param pEvent Event Intofmation Data
 * @retval None
 */
uint32_t PushFifo(EVT_ST *pEvent);

/**
 * @brief POP Fifo Data(Event用)
 * @param pEvent Event Intofmation Data
 * @retval NRF_SUCCESS Success
 * @retval NRF_ERROR_NULL Error
 */
uint32_t PopFifo(PEVT_ST pEvent);

/**
 * @brief Push Fifo Data(ACC/Gyro用)
 * @param p_data ACC/Gyro Data
 * @retval NRF_SUCCESS Success
 * @retval NRF_ERROR_NO_MEM Error
 */
uint32_t AccGyroPushFifo(ACC_GYRO_DATA_INFO *p_data);

/**
 * @brief POP Fifo Data(ACC/Gyro用)
 * @param p_data ACC/Gyro Data
 * @retval NRF_SUCCESS Success
 * @retval NRF_ERROR_NULL Error
 */
uint32_t AccGyroPopFifo(ACC_GYRO_DATA_INFO *p_data);

/**
 * @brief POP Fifo Data(Notifiy用)
 * @param pNotify_param BLE Notify parameter
 * @retval NRF_SUCCESS Success
 * @retval NRF_ERROR_NO_MEM Error
 */
uint32_t NotifyPopFifo(ble_gatts_hvx_params_t *pNotify_param);

/**
 * @brief Push Fifo Data(FIFO Clear)
 * @param pData FIFO Count Info
 * @retval NRF_SUCCESS Success
 * @retval NRF_ERROR_NO_MEM Error
 */
uint32_t FifoClearPushFifo( SENSOR_FIFO_DATA_INFO *p_data );

/**
 * @brief Pop Fifo Data(FIFO Clear)
 * @param pData FIFO Count Info
 * @retval NRF_SUCCESS Success
 * @retval NRF_ERROR_NO_MEM Error
 */
uint32_t FifoClearPopFifo( SENSOR_FIFO_DATA_INFO *p_data );

/**
 * @brief Clear Notify FIFO(Notifiy用)
 * @param None
 * @retval None
 */
void ClearNotifyFifo(void);

/**
 * @brief FIFO Event Debug Log
 * @param err_code Error Code
 * @param event Event ID
 * @retval None
 */
void DebugEvtFifoLog(uint32_t err_code, uint8_t event);

#endif
