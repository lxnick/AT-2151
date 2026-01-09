/**
  ******************************************************************************************
  * @file    ble_definition.h
  * @author  k.tashiro
  * @version 1.0
  * @date    2020/09/16
  * @brief   BLE Definition (old name : exBEL.h)
  ******************************************************************************************
    Revision:
    Ver            Date             Revised by        Explanation
  ******************************************************************************************
  * 1.0            2020/09/16       k.tashiro         create new
  ******************************************************************************************
*/


#ifndef BLE_DEFINITION_H_
#define BLE_DEFINITION_H_

/* Includes --------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "ble.h"
#include "ble_err.h"
#include "ble_hci.h"
#include "ble_srv_common.h"
#include "ble_advdata.h"
#include "ble_conn_params.h"
#include "ble_gatts.h"
#include "ble_advertising.h"
#include "ble_nus.h"
#include "nrf.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_qwr.h"
#include "nrf.h"
#include "nrf_drv_twi.h"
#include "ble_conn_params.h"
#include "ble_bas.h"
#include "state_control.h"
#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "app_timer.h"

/* Definition ------------------------------------------------------------*/
#define BLE_UUID_SIZE			16
/* 2020.12.23 Add RSSI取得テスト ++ */
#define BLE_RSSI_THS_VALUE			2
#define BLE_RSSI_COUNT_THS_VALUE	5
/* 2020.12.23 Add RSSI取得テスト -- */

/* Enum ------------------------------------------------------------------*/
//Ble radio Tx power enum
typedef enum
{
	TX_POWER_DBM_m40 = -40,
	TX_POWER_DBM_m20 = -20,
	TX_POWER_DBM_m16 = -16,
	TX_POWER_DBM_m12 = -12,
	TX_POWER_DBM_m8  =  -8,
	TX_POWER_DBM_m4  =  -4,
	TX_POWER_DBM_0   =   0,
	TX_POWER_DBM_2   =   2,
	TX_POWER_DBM_3   =   3,
	TX_POWER_DBM_4   =   4,
	TX_POWER_DBM_5   =   5,
	TX_POWER_DBM_6   =   6,
	TX_POWER_DBM_7   =   7,
	TX_POWER_DBM_8   =   8
}RADIO_TX_POWER;

typedef enum
{
	SHOES_ID  = 0x00,
	MODE_CHANGE_ID,
	MODE_TRIGGER_ID,
	MODE_RESULT_ID,
	DAILY_SET_ID,
	DAILY_LOG_ID,
	FW_RES_ID,
	FW_VER_ID,
	READ_ERR_ID,
	GET_TIME_ID,
	BAT_VOL_ID,
	PAIRING_CODE_ID,
	PAIRING_MATCH_ID,
	PAIRING_CLEAR_ID,
	
	/*TEST UUID*/
	CALIB_ID,
	
	/*raw data id*/
	RAW_DATA_ID,
	
	/* 2022.03.22 Add Test Command ++ */
	TEST_CMD_ID,
	/* 2022.03.22 Add Test Command -- */

	/* 2022.05.16 Add 角度調整 ++ */
	ANGLE_ADJUST_ID,
	/* 2022.05.16 Add 角度調整 -- */

	/* 2020.12.23 Add RSSI取得テスト ++ */
	RSSI_NOTIFY_ID,
	/* 2020.12.23 Add RSSI取得テスト -- */

	/* 2022.06.09 Add 分解能 ++ */
	RESOLUTION_ID,
	/* 2022.06.09 Add 分解能 -- */

	GATT_CHAR_HANDLE_END_ID
}GATTS_CHAR_HANDLE_ID;

/* Struct ----------------------------------------------------------------*/
typedef struct _con_param_data
{
	uint16_t max_con_param;
	uint16_t min_con_param;
	uint16_t slave_param;
	uint16_t sup_time;
} CON_PARAM, *PCON_PARAM;

typedef struct _ble_characteristic_param
{
	uint16_t init_len;				/**< Initial attribute value length in bytes. */
	uint16_t max_len;				/**< Maximum attribute value length in bytes, see @ref BLE_GATTS_ATTR_LENS_MAX for maximum values. */
	uint8_t uuid[BLE_UUID_SIZE];	/**< BLE UUID */
	uint8_t read_auth;				/**< Read authorization and value will be requested from the application on every read operation. */
	uint8_t read_enable;			/**< Reading the value permitted. */
	uint8_t write_enable;			/**< Writing the value with Write Request permitted. */
	uint8_t write_wo_resp;			/**< Writing the value with Write Command permitted. */
	uint8_t notify_enable;			/**< Notification of the value permitted. */
} BLE_CHAR_PARAM;

/* Function prototypes(ble_gap) -------------------------------------------*/
/**
 * @brief Get Connect Parameter
 * @param p_con_param CON_PARAM構造体を格納する変数へのポインタ
 * @retval None
 */
void GetConParam( CON_PARAM *p_con_param );

/**
 * @brief Setup Connect Parameter
 * @param setcntparam CON_PARAM構造体へのポインタ変数
 * @retval None
 */
void SetSettingConParam(CON_PARAM *setcntparam);

/**
 * @brief Get Setting Connect Parameter
 * @param None
 * @retval CON_PARAM
 */
CON_PARAM GetSetConParam(void);

/**
 * @brief BLE advertising start
 * @param None
 * @retval None
 */
//void advertising_start(void);
void BleAdvertisingStart(void);

/**
 * @brief BLE Gap Setting
 * @param None
 * @retval None
 */
//void gap_set(void);
void BleGapSetting(void);

/**
 * @brief BLE hvx Error Process
 * @param err_code Error Code
 * @param pNotify_param Notify Parameter
 * @retval None
 */
//void ble_hvx_err_process(ret_code_t err_code,ble_gatts_hvx_params_t *pNotify_param);
void BleHvxErrProcess(ret_code_t err_code,ble_gatts_hvx_params_t *pNotify_param);

/**
 * @brief BLE Retry Notify
 * @param pEvent Event Information
 * @retval 0 Success
 */
//uint32_t retryNotify(PEVT_ST pEvent);
uint32_t RetryNotify(PEVT_ST pEvent);

/**
 * @brief BLE Authoraize Error Process
 * @param err_code Error Code
 * @retval None
 */
//void ble_authorize_err_process(ret_code_t err_code);
void BleAuthorizeErrProcess(ret_code_t err_code);

/**
 * @brief BLE Value Error Process
 * @param err_code Error Code
 * @retval None
 */
//void ble_set_value_err_process(ret_code_t err_code);
void BleSetValueErrProcess(ret_code_t err_code);

/**
 * @brief Get BLE Connect Handle
 * @param pCnt_handle_value Connect Handle
 * @retval None
 */
//void getBLE_Cnt_handle(uint16_t *pCnt_handle_value);
void GetBleCntHandle(uint16_t *pCnt_handle_value);

/**
 * @brief Get BLE Send Flag
 * @param pFlag TX Send flag
 * @retval None
 */
//void getBLE_SendFlg(bool *pFlag);
void GetBleSendFlg(bool *pFlag);

/* Function prototypes(ble_gatts) -----------------------------------------*/
/**
 * @brief BLE GATT Setting
 * @param pValue_handle characteristic handle
 * @param gattId characteristic handle id
 * @retval None
 */
//void getGattsCharHandle_ValueID(uint16_t *pValue_handle, GATTS_CHAR_HANDLE_ID gattId);
void GetGattsCharHandleValueID(uint16_t *pValue_handle, GATTS_CHAR_HANDLE_ID gattId);

/**
 * @brief BLE GATT Setting
 * @param None
 * @retval None
 */
//void gatts_set(void);
void BleGattsSet(void);

/**
 * @brief BLE Write command BLE event. init, mode change, ult trigger.
 * @param handle_id handle id
 * @param len 受信したデータ長
 * @param p_data 受信データ
 * @retval None
 */
//void select_evt_write(uint16_t hadle_id, uint16_t len, uint8_t *p_data);
void SelectEvtWrite(uint16_t handle_id, uint16_t len, uint8_t *p_data);

/**
 * @brief BLE Read command BLE event
 * @param handle_id handle id
 * @retval None
 */
//void select_evt_read(uint16_t hadle_id);
void SelectEvtRead(uint16_t handle_id);

/**
 * @brief BLE Notify Signals
 * @param pSig signal
 * @param size signal size
 * @retval None
 */
//void notify_signals(uint8_t *pSig, uint16_t size);
void BleNotifySignals(uint8_t *pSig, uint16_t size);

/**
 * @brief Set BLE Error Read Command
 * @param func_err_code Function Error Code
 * @retval None
 */
//void setBLE_err_read_cmd(uint8_t func_err_code);
void SetBleErrReadCmd(uint8_t func_err_code);

/**
 * @brief Set BLE Notify Command
 * @param uuid_value_handle UUID Handle
 * @param pSignal Signal
 * @param size Signal Size
 * @retval None
 */
//void setBLE_notify_cmd(uint16_t uuid_value_handle, uint8_t *pSignal, uint16_t size);
void SetBleNotifyCmd(uint16_t uuid_value_handle, uint8_t *pSignal, uint16_t size);

/**
 * @brief set acc offset value for ble read
 * @param None
 * @retval None
 */
//void setCalib_Offset_Value_Read_Set(void);
void SetCalibOffsetValueReadSet(void);

/**
 * @brief Set BLE Raw Data Notify
 * @param enable Set RawMode Exec/Non-Exec
 * @retval None
 */
void SetBleRawExec( bool enable );

/**
 * @brief Get BLE Raw Data Notify
 * @param enable Get RawMode Exec/Non-Exec
 * @retval None
 */
void GetBleRawExec( bool *enable );

/**
 * @brief BLE Tx power change
 * @param tx_power tx power
 * @retval None
 */
void BlePowerChenge( int8_t tx_power );

/**
 * @brief BLE Inc Tx Complete Count
 * @param tx_power tx power
 * @retval None
 */
void IncTxCompCount( void );

/**
 * @brief BLE Tx power change
 * @param tx_power tx power
 * @retval None
 */
void SetTxCompCount( uint16_t tx_comp_count );

/**
 * @brief Update MTU Size
 * @param None
 * @retval None
 */
void UpdateMtuSize( uint16_t mtu_size );

/**
 * @brief BLE Set Force Disconnect
 * @param None
 * @retval None
 */
void SetForceDisconnect( void );

/**
 * @brief BLE Get Force Disconnect
 * @param None
 * @retval None
 */
void GetForceDisconnect( bool *force_flag );

/* 2020.12.23 Add RSSI取得テスト ++ */
/**
 * @brief Start BLE RSSI
 * @param None
 * @retval None
 */
void StartBleRSSIReport( void );

/**
 * @brief Stop BLE RSSI
 * @param None
 * @retval None
 */
void StopBleRSSIReport( void );

/**
 * @brief Set RSSI Value
 * @param value RSSI Value
 * @retval None
 */
void SetRssiValue( int16_t value );

/**
 * @brief Get RSSI Value
 * @param value RSSI Value
 * @retval None
 */
void GetRssiValue( int16_t *value );

/* 2020.12.23 Add RSSI取得テスト -- */

#endif
