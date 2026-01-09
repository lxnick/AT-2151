/**
  ******************************************************************************************
  * @file    ble_manager.h
  * @author  k.tashiro
  * @version 1.0
  * @date    2020/09/24
  * @brief   BLE Control Manager (old : ble_mng.h)
  ******************************************************************************************
    Revision:
    Ver            Date             Revised by        Explanation
  ******************************************************************************************
  * 1.0            2020/09/24       k.tashiro         create new
  ******************************************************************************************
*/

#ifndef BLE_MANAGER_H_
#define BLE_MANAGER_H_

/* Includes --------------------------------------------------------------*/
#include <stdint.h>
#include "lib_common.h"
#include "state_control.h"
#include "AccAngle.h"

#ifdef __cplusplus
extern "C"{
#endif

/* Function prototypes ---------------------------------------------------*/
/**
 * @brief Display Firmware Version
 * @param None
 * @retval None
 */
void DisplayFwVersion(void);

/**
 * @brief param update retry count clear
 * @param None
 * @retval None
 */
void ParamUpdateRetryCountClear(void);

/**
 * @brief BLE advertising start.
 * @param pEvent Event Information
 * @retval 0 Success
 */
//uint32_t utc_advertisng_start(PEVT_ST pEvent);
uint32_t BleAdvertisngStart(PEVT_ST pEvent);

/**
 * @brief EVT_CNT_PARAM_UPDATE_ERR function
 * @param pEvent Event Information
 * @retval 0 Success
 */
//uint32_t updateErrorFailed(PEVT_ST pEvent);
uint32_t UpdateErrorFailed(PEVT_ST pEvent);

//uint32_t updateParamCheck(PEVT_ST pEvent);
uint32_t UpdateParamCheck(PEVT_ST pEvent);

/**
 * @brief BLE force disconnect function.
 *        state force_disconnect, evt EVT_FORCE_DISOCON_TO, CNT_UPDATA_ERROR_TO
 * @param pEvent Event Information
 * @retval 0 Success
 */
//uint32_t forceDisconnect(PEVT_ST pEvent);
uint32_t BleForceDisconnect(PEVT_ST pEvent);

/**
 * @brief BLE slave latency 1 change
 * @param pEvent Event Information
 * @retval 0 Success
 */
//uint32_t utc_changeCntParamSlaveLetencyOne(PEVT_ST pEvent);
uint32_t ChangeCntParamSlaveLetencyOne(PEVT_ST pEvent);

/**
 * @brief Event Reply
 * @param pEvent Event Information
 * @retval 0 Success
 */
//uint32_t utc_repush_event(PEVT_ST pEvent);
uint32_t RepushEvent(PEVT_ST pEvent);

/**
 * @brief Setup BLE GATT/GAP
 * @param None
 * @retval None
 */
//void utc_ble_gatts_gap_SetUp(void);
void BleGattsGapSetup(void);

/**
 * @brief BLE Slave Latency Zero Change
 * @param pEvent Event Information
 * @retval 0 Success
 */
//uint32_t utc_slave_latencyZero_change(PEVT_ST pEvent);
uint32_t BleSlaveLatencyZeroChange(PEVT_ST pEvent);

/**
 * @brief BLE Slave Latency One Change
 * @param pEvent Event Information
 * @retval 0 Success
 */
//uint32_t utc_slave_latencyOne_change(PEVT_ST pEvent);
uint32_t BleSlaveLatencyOneChange(PEVT_ST pEvent);

/**
 * @brief Set Current Mode
 * @param mode set mode
 * @retval None
 */
//void setCurrentMode(uint8_t mode);
void SetCurrentMode(uint8_t mode);

/**
 * @brief Clear send connect update retry count
 * @param None
 * @retval None
 */
//void clearSendCntUpdateRetryCount(void);
void ClearSendCntUpdateRetryCount(void);

/**
 * @brief Raw data Notify signal
 * @param pSignal signal
 * @param size signal size
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 Failed
 */
//ret_code_t raw_res_setBLE_notify_cmd(uint8_t *pSignal, uint16_t size);
ret_code_t RawResSetBleNotifyCmd(uint8_t *pSignal, uint16_t size);

/* 2022.05.19 Add 角度調整対応 ++ */
/**
 * @brief Setup Angle Adjustment Data
 * @param acc_angle 角度補正データ
 * @param state 現在の状態
 * @retval None
 */
void SetupAccAngleData( ACC_ANGLE *angle_info, uint8_t state );
/* 2022.05.19 Add 角度調整対応 -- */

/* 2022.06.09 Add 分解能追加 ++ */
/**
 * @brief Setup Resolution Data
 * @param None
 * @retval None
 */
void SetupResolutionData( void );
/* 2022.06.09 Add 分解能追加 -- */

uint32_t UpdateParamCheckGuest(PEVT_ST pEvent);

#ifdef __cplusplus
}
#endif

#endif
