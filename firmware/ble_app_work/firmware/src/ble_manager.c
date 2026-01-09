/**
  ******************************************************************************************
  * @file    ble_manager.c
  * @author  k.tashiro
  * @version 1.0
  * @date    2020/09/24
  * @brief   BLE Control Manager (old : ble_mng.c)
  ******************************************************************************************
    Revision:
    Ver            Date             Revised by        Explanation
  ******************************************************************************************
  * 1.0            2020/09/24       k.tashiro         create new
  ******************************************************************************************
*/

/* Includes --------------------------------------------------------------*/
#include "ble.h"
#include "lib_common.h"
#include "state_control.h"
#include "mode_manager.h"
#include "lib_ram_retain.h"
#include "ble_definition.h"
#include "ble_manager.h"
#include "lib_timer.h"
#include "lib_fifo.h"
#include "algo_acc.h"
#include "definition.h"
#include "flash_operation.h"
#include "lib_trace_log.h"
#include "AccAngle.h"

/* Private variables -----------------------------------------------------*/
volatile ble_gap_conn_params_t m_conn_params;
volatile uint8_t gParamRetryCount = 0;		//param update retry count

volatile uint8_t gCntCountOne = 0;
volatile uint8_t gCntCountZero = 0;

volatile uint8_t gFlashParamEvent;
extern ble_gatts_char_handles_t gFw_res_id;
extern ble_gatts_char_handles_t gMode_result_id;
//FW version. 20180806 0.01->0.02
//FW version. 20180824 0.07->0.08
//FW version. 20180824 0.08->0.09 *apllication test version number
//FW version. 20180829 0.09->0.10 *apllication test version number
//FW version. 20180829 0.10->0.11 *apllication test version number
//FW version. 20100907 1.00       *fix release version
//FW version. 20181022 1.10       *adition calibration function version
//FW version. 20181022 1.10       *adition calibration function version user edition(uintec debugging version). when release, return 1.10
//FW version. 20190401 1.11       *adition get raw data mode for experiment.
//FW version. 20200402 1.12       *change advertise duration time 30s->60s
//FW version. 20200805 2.00       *unlimitv ver2 number-> dead
//FW version. 20200806 1.13       *change device name. "UNLIMITIV_SHOES01" -> "UNLIMITIV_SHOES02"
//FW version. 20200806 1.14       *gyro dummy data send
//FW version. 20200811 1.15       *Addtion Acc sensor wake up axis. ADV public axis changed X. 
//FW version. 20201020 2.00       *Version 2.0 Unlimitiv Ver2 Step2
//FW version. 20220117 3.00       *Version 3.0 Unlimitiv Ver3 change device name. "UNLIMITIV_SHOES02" -> "UNLIMITIV_SHOES03"
//FW version. 20220325 4.00       *Version 4.0 Unlimitiv Ver3 Change Sensor LSM6DSOW -> ICM42607
//FW version. 20220407 4.01       *Version 4.01 Unlimitiv Ver3 Change DCDC Power Save
//FW version. 20220418 9.99       *Version 9.99 ADV判定アルゴリズムを調整
//FW version. 20220422 4.10       *Version 4.10 ADV判定アルゴリズムを正式に採用し、V7基板でのversionを4.1.0に変更
//FW version. 20220425 4.11       *Version 4.11 Gyroの分解能計算に誤りがあったため修正
//FW version. 20220425 4.12       *Version 4.11 connection interval変更. max 75-45ms, min 45->15ms
//FW version. 20220530 4.13       *Version 4.13 自動接続、ゲストモード、角度調整に対応
//FW version. 20220615 4.14       *Version 4.14 RSSI追加、自動接続時のADV発行時間帯を制限
//FW version. 20220622 4.15       *Version 4.15 自動接続時、Get Daily Logイベントが消失する場合を解決
//FW version. 20220722 4.16       *Version 4.16 RawMode中はに強制切断タイマを停止する RTC異常対応 RSSI値をint8 -> int16に修正
//FW version. 20220722 4.17       *Version 4.17 Guest mode接続時、cnt intervalをランダムに選択
//FW version. 20220826 4.18       *Version 4.18 強制切断1hour->15min. ios scan resp対応, device name対応

static const uint8_t gfw_version[] = {4,1,8};

/* 2022.05.19 Add 角度調整 ++ */
static ANGLE_ADJUST_INFO g_angle_adjust_info = {0};
/* 2022.05.19 Add 角度調整 -- */

/* Private function prototypes -------------------------------------------*/
/**
 * @brief connection param update error check
 * @param None
 * @retval true Success
 * @retval false Failed
 */
static bool cnt_param_check(void);

/**
 * @brief Flash Operation Connect Parameter Check
 * @param pEvent Event Information
 * @retval true Success
 * @retval false Failed
 */
static bool check_flash_op_cnt_pamam(void);

/**
 * @brief Set Firmware Version to BLE
 * @param None
 * @retval None
 */
static void set_fw_version_to_ble(void);

/**
 * @brief param update retry count increment
 * @param None
 * @retval None
 */
static void param_update_retry_count_inc(void)
{
	uint8_t critrical_retry;
	uint32_t err;
	
	err = sd_nvic_critical_region_enter(&critrical_retry);
	if(err == NRF_SUCCESS)
	{	
		gParamRetryCount++;
		critrical_retry = 0;
		err = sd_nvic_critical_region_exit(critrical_retry);
		if(err != NRF_SUCCESS)
		{
			//nothing to do
		}
	}		
}

/**
 * @brief Get param update retry count
 * @param None
 * @retval None
 */
static void get_param_update_retry_count(uint8_t *pRetryCount)
{
	uint32_t err;
	uint8_t  critrical_retry;

	err = sd_nvic_critical_region_enter(&critrical_retry);
	if(err == NRF_SUCCESS)
	{		
		*pRetryCount = gParamRetryCount;
		critrical_retry = 0;
		err = sd_nvic_critical_region_exit(critrical_retry);
		if(err != NRF_SUCCESS)
		{
			//nothing to do
		}
	}
}

/**
 * @brief connection param update error check
 * @param None
 * @retval true Success
 * @retval false Failed
 */
static bool cnt_param_check(void)
{
	volatile bool bret;
	volatile CON_PARAM currConparam;
	
	bret = true;
	//connection parameter get
	GetConParam( (void *)&currConparam );

	if((uint16_t)MAX_CONN_INTERVAL < currConparam.max_con_param)
	{
		if(currConparam.max_con_param < (uint16_t)MIN_CONN_INTERVAL)
		{
			bret = false;
		}
	}
	
	if((uint16_t)MAX_CONN_INTERVAL < currConparam.min_con_param)
	{
		if(currConparam.min_con_param < (uint16_t)MIN_CONN_INTERVAL)
		{
			bret = false;
		}
	}

	if(currConparam.slave_param != (uint16_t)SLAVE_LATENCY)
	{
		bret = false;
	}
	
	if(currConparam.sup_time != (uint16_t)CONN_SUP_TIMEOUT)
	{
		bret = false;
	}
	
//debug print.
	if(bret == true){
		DEBUG_LOG(LOG_INFO,"cnt para check success max %u, min %u, slave %u, sup time %u",currConparam.max_con_param, currConparam.min_con_param,currConparam.slave_param,currConparam.sup_time);
	}else
	{
		DEBUG_LOG(LOG_DEBUG,"err max %u, min %u, slave %u, sup time %u",currConparam.max_con_param, currConparam.min_con_param,currConparam.slave_param,currConparam.sup_time);
	}

	return bret;
}

/**
 * @brief Flash Operation Connect Parameter Check
 * @param pEvent Event Information
 * @retval true Success
 * @retval false Failed
 */
static bool check_flash_op_cnt_pamam(void)
{
	volatile bool bret;
	volatile CON_PARAM currConparam;
	
	bret = true;
	//connection parameter check
	GetConParam( (void *)&currConparam );

	if((uint16_t)MAX_CONN_INTERVAL < currConparam.max_con_param)
	{
		if(currConparam.max_con_param < (uint16_t)MIN_CONN_INTERVAL)
		{
			bret = false;
		}
	}
	
	if((uint16_t)MAX_CONN_INTERVAL < currConparam.min_con_param)
	{
		if(currConparam.min_con_param < (uint16_t)MIN_CONN_INTERVAL)
		{
			bret = false;
		}
	}
	
	if(currConparam.slave_param != (uint16_t)FLASH_SLAVE_LATENCY)
	{
		bret = false;
	}
	
	if(currConparam.sup_time != (uint16_t)CONN_SUP_TIMEOUT)
	{
		bret = false;
	}
	
//debug print.
	if(bret == true)
	{
		DEBUG_LOG(LOG_INFO,"flash cnt para check success  success max %u, min %u, slave %u, sup time %u",currConparam.max_con_param, currConparam.min_con_param,currConparam.slave_param, currConparam.sup_time);
	}
	else
	{
		DEBUG_LOG(LOG_DEBUG,"flash err max %u, min %u, slave %u, sup time %u",currConparam.max_con_param, currConparam.min_con_param,currConparam.slave_param, currConparam.sup_time);
	}

	return bret;
}

/**
 * @brief Set Firmware Version to BLE
 * @param None
 * @retval None
 */
static void set_fw_version_to_ble(void)
{
	ret_code_t err;
	ble_gatts_value_t fw_value;
	uint16_t cnt_handle = BLE_CONN_HANDLE_INVALID;
	uint16_t gatt_value_handle;
	
	GetGattsCharHandleValueID(&gatt_value_handle, FW_VER_ID);
	fw_value.offset = BLE_SET_VALUE_OFFSET;
	fw_value.p_value = (uint8_t*)&gfw_version[0];
	fw_value.len    = sizeof(gfw_version);
	GetBleCntHandle(&cnt_handle);
	err = sd_ble_gatts_value_set(cnt_handle, gatt_value_handle, &fw_value);
	BleSetValueErrProcess(err);
}

/**
 * @brief Get first send param update retry count
 * @param pRetryCount retry count
 * @retval None
 */
static void get_param_update_retry_count_one(uint8_t *pRetryCount)
{
	uint32_t err;
	uint8_t  critrical_retry;

	err = sd_nvic_critical_region_enter(&critrical_retry);
	if(err == NRF_SUCCESS)
	{		
		*pRetryCount = gCntCountOne;
		critrical_retry = 0;
		err = sd_nvic_critical_region_exit(critrical_retry);
		if(err != NRF_SUCCESS)
		{
			//nothing to do
		}
	}
}

/**
 * @brief increment parameter update retry count one
 * @param None
 * @retval None
 */
static void inc_param_update_retry_count_one(void)
{
	uint32_t err;
	uint8_t  critrical_retry;

	err = sd_nvic_critical_region_enter(&critrical_retry);
	if(err == NRF_SUCCESS)
	{		
		gCntCountOne++;
		critrical_retry = 0;
		err = sd_nvic_critical_region_exit(critrical_retry);
		if(err != NRF_SUCCESS)
		{
			//nothing to do
		}
	}
}

/**
 * @brief Clear parameter update retry count one
 * @param None
 * @retval None
 */
static void clear_param_update_retry_count_one(void)
{
	uint32_t err;
	uint8_t  critrical_retry;

	err = sd_nvic_critical_region_enter(&critrical_retry);
	if(err == NRF_SUCCESS)
	{		
		gCntCountOne = 0;
		DEBUG_LOG(LOG_DEBUG,"send update sl one retry count %u",gCntCountOne);
		critrical_retry = 0;
		err = sd_nvic_critical_region_exit(critrical_retry);
		if(err != NRF_SUCCESS)
		{
			//nothing to do
		}
	}
}

/**
 * @brief Get param update retry count Zero
 * @param pRetryCount Retry Count
 * @retval None
 */
static void get_param_update_retry_count_zero(uint8_t *pRetryCount)
{
	uint32_t err;
	uint8_t  critrical_retry;

	err = sd_nvic_critical_region_enter(&critrical_retry);
	if(err == NRF_SUCCESS)
	{		
		*pRetryCount = gCntCountZero;
		critrical_retry = 0;
		err = sd_nvic_critical_region_exit(critrical_retry);
		if(err != NRF_SUCCESS)
		{
			//nothing to do
		}
	}
}

/**
 * @brief increment param update retry count Zero
 * @param None
 * @retval None
 */
static void inc_param_update_retry_count_zero(void)
{
	uint32_t err;
	uint8_t  critrical_retry;

	err = sd_nvic_critical_region_enter(&critrical_retry);
	if(err == NRF_SUCCESS)
	{		
		gCntCountZero++;
		critrical_retry = 0;
		err = sd_nvic_critical_region_exit(critrical_retry);
		if(err != NRF_SUCCESS)
		{
			//nothing to do
		}
	}
}

/**
 * @brief Clear param update retry count Zero
 * @param None
 * @retval None
 */
static void clear_param_update_retry_count_zero(void)
{
	uint32_t err;
	uint8_t  critrical_retry;

	err = sd_nvic_critical_region_enter(&critrical_retry);
	if(err == NRF_SUCCESS)
	{		
		gCntCountZero = 0;
		DEBUG_LOG(LOG_DEBUG,"send update sl zero retry count %u",gCntCountZero);
		critrical_retry = 0;
		err = sd_nvic_critical_region_exit(critrical_retry);
		if(err != NRF_SUCCESS)
		{
			//nothing to do
		}
	}
}

/**
 * @brief Display Firmware Version
 * @param None
 * @retval None
 */
void DisplayFwVersion(void)
{
	DEBUG_LOG(LOG_INFO,"FW version %u.%u%u",gfw_version[0],gfw_version[1],gfw_version[2]);	
}

/**
 * @brief param update retry count clear
 * @param None
 * @retval None
 */
void ParamUpdateRetryCountClear(void)
{
	uint8_t critrical_retry;
	uint32_t err;
	
	err = sd_nvic_critical_region_enter(&critrical_retry);
	if(err == NRF_SUCCESS)
	{
		gParamRetryCount = 0;
		DEBUG_LOG(LOG_DEBUG,"param retry count %u",gParamRetryCount);
		critrical_retry = 0;
		err = sd_nvic_critical_region_exit(critrical_retry);
		if(err != NRF_SUCCESS)
		{
			//nothing to do
		}
	}
}

/**
 * @brief BLE advertising start.
 * @param pEvent Event Information
 * @retval 0 Success
 */
uint32_t BleAdvertisngStart(PEVT_ST pEvent)
{
	bool force_disconnect;
	ret_code_t err_code;
	EVT_ST event;
	
	UNUSED_PARAMETER(*pEvent);
	/* 2022.05.19 Add 角度補正値保存 ++ */
	SaveAngleAdjust();
	/* 2022.05.19 Add 角度補正値保存 -- */
	
	/* 2022.03.24 Add 強制切断対応 ++ */
	GetForceDisconnect( &force_disconnect );
	if ( force_disconnect )
	{
		event.evt_id = EVT_NO_WALK_TO;
		err_code = PushFifo(&event);
		DEBUG_EVT_FIFO_LOG(err_code,event.evt_id);
	}
	/* 2022.03.24 Add 強制切断対応 -- */
	else
	{
		//no walk timer clear.
		WalkTimeOutClear();
		//New add flag clear.
		PinCodeCheckFlagClear();
		//advertising start.
		BleAdvertisingStart();
	}
	return 0;
}

/**
 * @brief EVT_CNT_PARAM_UPDATE_ERR function
 * @param pEvent Event Information
 * @retval 0 Success
 */
uint32_t UpdateErrorFailed(PEVT_ST pEvent)
{
	UNUSED_PARAMETER(*pEvent);
	
	//update monitor timer stop
	StopParamUpdateTimer();
	//update err process monitor tiemr start
	StartParamUpdateErrorTimer();
	
	return 0;
}

/**
 * @brief EVT_UPDATING_PARAM, state, param update, param update sl = 0 return cnt
 * @param pEvent Event Information
 * @retval 0 Success
 */
uint32_t UpdateParamCheck(PEVT_ST pEvent)
{
	ret_code_t err_code;
	EVT_ST event;
	uint8_t retrycount;
	bool bParamCheck;
	uint16_t cnt_handle = BLE_CONN_HANDLE_INVALID;
	uint8_t flash_op;
	EVT_ST sleepevent;
	ble_gap_conn_params_t con_para;
	
	con_para.slave_latency     = SLAVE_LATENCY;
	con_para.min_conn_interval = MIN_CONN_INTERVAL;
	con_para.max_conn_interval = MAX_CONN_INTERVAL;
	con_para.conn_sup_timeout  = CONN_SUP_TIMEOUT;
	
	DEBUG_LOG(LOG_INFO, "conncetion param update slave latecy zero enter");
	//update param check.
	bParamCheck = cnt_param_check();
	
#ifdef TEST_CNT_UPDATE_RETRY_ERROR
	//FW version 9.00
	bParamCheck = false;
#endif

	if(true == bParamCheck)
	{
		DEBUG_LOG(LOG_INFO, "conncetion param update success");
		// retry count
		ParamUpdateRetryCountClear();
		UpdateRetryCountClear();
		//connection update cmpl. state change connect
		event.evt_id = EVT_CNT_PARAM_UPDATE_CMPL;
		err_code = PushFifo(&event);
		DEBUG_EVT_FIFO_LOG(err_code,event.evt_id);
		StopParamUpdateTimer();
		
		//rtc int not
		GetFlashOperatoinMode(&flash_op);
		if(flash_op != FLASH_RTC_INT)
		{
			RestartForceDisconTimer();
		}
	}
	else
	{
		//retrycount check
		get_param_update_retry_count(&retrycount);
		if(retrycount < PARAM_UPDATE_RETRY_LIMIT)
		{
			DEBUG_LOG(LOG_INFO, "conncetion param updatting slave latecy zero");
			GetBleCntHandle(&cnt_handle);
			//request connection update param.
			err_code = sd_ble_gap_conn_param_update(cnt_handle, &con_para);
			
			//debug test
#ifdef TEST_CNT_UPDATE_RETRY_ERROR
			err_code = NRF_ERROR_BUSY;
#endif
			if(err_code != NRF_SUCCESS)
			{
				DEBUG_LOG(LOG_ERROR, "change SL zero sd_ble_gap_conn_param_update exe err. 0x%x",err_code);
				if((err_code == NRF_ERROR_INVALID_STATE) || (err_code == NRF_ERROR_BUSY))
				{
					memcpy(&event,pEvent,sizeof(event));
					sleepevent.evt_id = EVT_FORCE_SLEEP;
					err_code = PushFifo(&sleepevent);
					DEBUG_EVT_FIFO_LOG(err_code, sleepevent.evt_id);
					err_code = PushFifo(&event);
					DEBUG_EVT_FIFO_LOG(err_code, event.evt_id);
					TRACE_LOG(TR_CONNECTION_PARAM_UPDATE_API_FAILED,(uint16_t)m_conn_params.slave_latency);
				}
			}
			else
			{
				DEBUG_LOG(LOG_INFO,"change SL 0 sd_ble_gap_conn_param_update exe success");
			}
			//retry count increment
			param_update_retry_count_inc();
			//update parameter timer restart
			RestartParamUpdateTimer();
		}
		else
		{
			DEBUG_LOG(LOG_ERROR,"conncetion param update, retry count over %u",retrycount);
			SetBleErrReadCmd(UTC_BLE_CON_INTERVAL_ERROR);
			//change state paramUpdate err state.
			event.evt_id = EVT_CNT_PARAM_UPDATE_ERR;
			err_code = PushFifo(&event);
			DEBUG_EVT_FIFO_LOG(err_code,event.evt_id);
			StopParamUpdateTimer();
			TRACE_LOG(TR_CONNECTION_PARAM_UPDATE_FAILED_SLAVE_LATENCY_ZERO, 0);
		}
	}
	return 0;
}

/**
 * @brief BLE force disconnect function.
 *        state force_disconnect, evt EVT_FORCE_DISOCON_TO, CNT_UPDATA_ERROR_TO
 * @param pEvent Event Information
 * @retval 0 Success
 */
uint32_t BleForceDisconnect(PEVT_ST pEvent)
{
	ret_code_t err_code;
	EVT_ST event;
	uint16_t cnt_handle = BLE_CONN_HANDLE_INVALID;
	
	DEBUG_LOG(LOG_DEBUG,"force disconnect event in");
	GetBleCntHandle(&cnt_handle);
	
	err_code = sd_ble_gap_disconnect(cnt_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE);
	if(err_code != NRF_SUCCESS)
	{
		/*trace log*/
		DEBUG_LOG(LOG_ERROR,"force disconnect api exe failed");
		//disconnect error, soft reset.
		sd_nvic_SystemReset();
	}
	else
	{
		//state is wait disconnect change
		event.evt_id = EVT_DISCNT_API_CMPL;
		err_code = PushFifo(&event);
		DEBUG_EVT_FIFO_LOG(err_code,event.evt_id);
		//disconnect callback monitor timer start
		StartWaitDisconCallbackTimer();
	}
	return 0;
}

/**
 * @brief BLE slave latency 1 change
 * @param pEvent Event Information
 * @retval 0 Success
 */
uint32_t ChangeCntParamSlaveLetencyOne(PEVT_ST pEvent)
{
	volatile ret_code_t err_code;
	uint8_t retrycount;
	bool bret;
	EVT_ST event;
	ble_gap_conn_params_t con_para;
	uint32_t fifo_err_code;
	uint16_t cnt_handle = BLE_CONN_HANDLE_INVALID;
	uint8_t flash_op;
	EVT_ST sleepevent;
	
	con_para.slave_latency     = FLASH_SLAVE_LATENCY;
	con_para.min_conn_interval = MIN_CONN_INTERVAL;
	con_para.max_conn_interval = MAX_CONN_INTERVAL;
	con_para.conn_sup_timeout  = CONN_SUP_TIMEOUT;
	
	/* Check Flash Operation */
	bret = check_flash_op_cnt_pamam();
	
	DEBUG_LOG(LOG_INFO,"Operation flash change cnt paramters start");
	if(bret == true)
	{
		UpdateRetryCountClear();
		ParamUpdateRetryCountClear();
		
		DEBUG_LOG(LOG_INFO,"conection param update success. change SL 1");
		//connection update cmpl. state change connect
		event.evt_id = EVT_CNT_PARAM_UPDATE_CMPL;
		fifo_err_code = PushFifo(&event);
		DEBUG_EVT_FIFO_LOG(fifo_err_code,event.evt_id);
		StopParamUpdateTimer();
		
		//rtc int not
		GetFlashOperatoinMode(&flash_op);
		if(flash_op != FLASH_RTC_INT)
		{
			RestartForceDisconTimer();
		}
	}
	else
	{
		//retry count check.
		get_param_update_retry_count(&retrycount);
		if(retrycount < PARAM_UPDATE_RETRY_LIMIT)
		{
			GetBleCntHandle(&cnt_handle);
			//request connection update param.
			err_code = sd_ble_gap_conn_param_update(cnt_handle, &con_para);
			if(err_code != NRF_SUCCESS)
			{
				DEBUG_LOG(LOG_ERROR,"change SL 1 sd_ble_gap_conn_param_update exe err. 0x%x",err_code);
				TRACE_LOG(TR_CONNECTION_PARAM_UPDATE_API_FAILED,(uint16_t)err_code);
				if((err_code == NRF_ERROR_INVALID_STATE) || (err_code == NRF_ERROR_BUSY))
				{
					memcpy(&event,pEvent,sizeof(event));
					sleepevent.evt_id = EVT_FORCE_SLEEP;
					fifo_err_code = PushFifo(&sleepevent);
					DEBUG_EVT_FIFO_LOG(fifo_err_code, sleepevent.evt_id);
				
					fifo_err_code = PushFifo(&event);
					DEBUG_EVT_FIFO_LOG(fifo_err_code, event.evt_id);
				}
			}
			else
			{
				DEBUG_LOG(LOG_DEBUG,"change SL 1 sd_ble_gap_conn_param_update exe success");
			}
			//update parameter timer restart
			RestartParamUpdateTimer();
			param_update_retry_count_inc();
		}
		else
		{
			DEBUG_LOG(LOG_ERROR,"change SL 1 connection param update count over. %u", retrycount);
			SetBleErrReadCmd(UTC_BLE_CON_INTERVAL_ERROR);
			//change state paramUpdate err state.
			event.evt_id = EVT_CNT_PARAM_UPDATE_ERR;
			fifo_err_code = PushFifo(&event);
			DEBUG_EVT_FIFO_LOG(fifo_err_code, event.evt_id);
			StopParamUpdateTimer();
			TRACE_LOG(TR_CONNECTION_PARAM_UPDATE_FAILED_SLAVE_LATENCY_ONE, 0);
		}
	}
	return 0;
}

/**
 * @brief Event Reply
 * @param pEvent Event Information
 * @retval 0 Success
 */
uint32_t RepushEvent(PEVT_ST pEvent)
{
	uint32_t fifo_err;
	EVT_ST   event;
	
	//future change
	DEBUG_LOG(LOG_DEBUG,"Sleep");
	event.evt_id = EVT_FORCE_SLEEP;
	fifo_err = PushFifo(&event);
	DEBUG_EVT_FIFO_LOG(fifo_err,event.evt_id);
	//future change
	DEBUG_LOG(LOG_DEBUG,"reply evt 0x%x",pEvent->evt_id);
	fifo_err = PushFifo(pEvent);
	DEBUG_EVT_FIFO_LOG(fifo_err,pEvent->evt_id);
	
	return 0;
}

/**
 * @brief Setup BLE GATT/GAP
 * @param None
 * @retval None
 */
void BleGattsGapSetup(void)
{
	BleGattsSet();
	BleGapSetting();
	set_fw_version_to_ble();
	SetCurrentMode((uint8_t)DAILY_MODE);
	SetBleErrReadCmd((uint8_t)UTC_BLE_SUCCESS);
}

/**
 * @brief Set Current Mode
 * @param mode set mode
 * @retval None
 */
void SetCurrentMode(uint8_t mode)
{
	ret_code_t err;
	uint8_t mode_internal = mode;
	ble_gatts_value_t mode_value;
	uint16_t cnt_handle = BLE_CONN_HANDLE_INVALID;
	uint16_t mode_change_value_id;
	
	mode_value.offset = BLE_SET_VALUE_OFFSET;
	mode_value.p_value = &mode_internal;
	mode_value.len    = 1;
	GetBleCntHandle(&cnt_handle);
	
	GetGattsCharHandleValueID(&mode_change_value_id,MODE_CHANGE_ID);
	err = sd_ble_gatts_value_set(cnt_handle, mode_change_value_id, &mode_value);
	BleSetValueErrProcess(err);
}

/**
 * @brief Clear send connect update retry count
 * @param None
 * @retval None
 */
void ClearSendCntUpdateRetryCount(void)
{
	clear_param_update_retry_count_zero();
	clear_param_update_retry_count_one();
}

/**
 * @brief BLE Slave Latency Zero Change
 * @param pEvent Event Information
 * @retval 0 Success
 */
uint32_t BleSlaveLatencyZeroChange(PEVT_ST pEvent)
{
	uint32_t fifo_err;
	volatile ret_code_t err_code;
	ble_gap_conn_params_t con_para;
	uint8_t flash_op;
	uint16_t cnt_handle = BLE_CONN_HANDLE_INVALID;
	uint8_t retrycount = 0;
	EVT_ST sleepevent;
	EVT_ST replyevent;
	
	
	con_para.slave_latency     = SLAVE_LATENCY;
	con_para.min_conn_interval = MIN_CONN_INTERVAL;
	con_para.max_conn_interval = MAX_CONN_INTERVAL;
	con_para.conn_sup_timeout  = CONN_SUP_TIMEOUT;
	

	
	GetFlashOperatoinMode(&flash_op);
	if(flash_op != FLASH_RTC_INT)
	{
		StopFlashTimer();
	}
	
	StopParamUpdateTimer();
	StartParamUpdateTimer();
	
	GetBleCntHandle(&cnt_handle);
	err_code = sd_ble_gap_conn_param_update(cnt_handle, &con_para);	
	if(err_code != NRF_SUCCESS)
	{
		inc_param_update_retry_count_zero();
		get_param_update_retry_count_zero(&retrycount);
		DEBUG_LOG(LOG_ERROR,"BleSlaveLatencyZeroChange err 0x%x. retry count %u",err_code, retrycount);
		TRACE_LOG(TR_CONNECTION_PARAM_UPDATE_API_FAILED,(uint16_t)err_code);
		if(retrycount < PARAM_UPDATE_RETRY_LIMIT)
		{
			if((err_code == NRF_ERROR_INVALID_STATE) || (err_code == NRF_ERROR_BUSY))
			{
				sleepevent.evt_id = EVT_FORCE_SLEEP;
				memcpy(&replyevent,pEvent,sizeof(replyevent));
				fifo_err = PushFifo(&sleepevent);
				DEBUG_EVT_FIFO_LOG(fifo_err,sleepevent.evt_id);
				fifo_err = PushFifo(&replyevent);
				DEBUG_EVT_FIFO_LOG(fifo_err,replyevent.evt_id);
				
			}			
			else
			{
				DEBUG_LOG(LOG_INFO,"BleSlaveLatencyZeroChange invalid param err");
			}
		}
		else
		{
			clear_param_update_retry_count_zero();
			StopParamUpdateTimer();
			replyevent.evt_id = EVT_CNT_PARAM_UPDATE_ERR;
			fifo_err = PushFifo(&replyevent);
			DEBUG_EVT_FIFO_LOG(fifo_err,replyevent.evt_id);
			TRACE_LOG(TR_CONNECTION_PARAM_UPDATE_FAILED_SLAVE_LATENCY_ZERO, 0);
		}
	}
	else
	{
		clear_param_update_retry_count_zero();
		DEBUG_LOG(LOG_INFO,"slave latency zero change start. return init cmd");
	}
	
	return 0;
}

/**
 * @brief BLE Slave Latency One Change
 * @param pEvent Event Information
 * @retval 0 Success
 */
uint32_t BleSlaveLatencyOneChange(PEVT_ST pEvent)
{
	uint32_t fifo_err;
	volatile ret_code_t err_code;
	ble_gap_conn_params_t con_para;
	uint8_t flash_op;
	uint16_t cnt_handle = BLE_CONN_HANDLE_INVALID;
	uint8_t retrycount = 0;
	EVT_ST sleepevent;
	EVT_ST replyevent;

	con_para.slave_latency     = FLASH_SLAVE_LATENCY;
	con_para.min_conn_interval = MIN_CONN_INTERVAL;
	con_para.max_conn_interval = MAX_CONN_INTERVAL;
	con_para.conn_sup_timeout  = CONN_SUP_TIMEOUT;
	
	GetFlashOperatoinMode(&flash_op);
	if(flash_op != FLASH_RTC_INT)
	{
		StopFlashTimer();
	}
	StopParamUpdateTimer();
	StartParamUpdateTimer();
	
	GetBleCntHandle(&cnt_handle);
	
	err_code = sd_ble_gap_conn_param_update(cnt_handle, &con_para);
	if(err_code != NRF_SUCCESS)
	{
		inc_param_update_retry_count_one();
		get_param_update_retry_count_one(&retrycount);
		DEBUG_LOG(LOG_ERROR,"utc_slave_latency_one_change err 0x%x. rtrycount %u",err_code,retrycount);
		TRACE_LOG(TR_CONNECTION_PARAM_UPDATE_API_FAILED,(uint16_t)err_code);
		if(retrycount < PARAM_UPDATE_RETRY_LIMIT)
		{
			if((err_code == NRF_ERROR_INVALID_STATE) || (err_code == NRF_ERROR_BUSY))
			{
				sleepevent.evt_id = EVT_FORCE_SLEEP;
				memcpy(&replyevent,pEvent,sizeof(replyevent));
				fifo_err = PushFifo(&sleepevent);
				DEBUG_EVT_FIFO_LOG(fifo_err,sleepevent.evt_id);
				fifo_err = PushFifo(&replyevent);
				DEBUG_EVT_FIFO_LOG(fifo_err,replyevent.evt_id);
			}
		}
		else
		{
			StopParamUpdateTimer();
			clear_param_update_retry_count_one();
			replyevent.evt_id = EVT_CNT_PARAM_UPDATE_ERR;
			fifo_err = PushFifo(&replyevent);
			DEBUG_EVT_FIFO_LOG(fifo_err,replyevent.evt_id);
			TRACE_LOG(TR_CONNECTION_PARAM_UPDATE_FAILED_SLAVE_LATENCY_ONE, 0);
		}
	}
	else
	{
		clear_param_update_retry_count_one();
		DEBUG_LOG(LOG_INFO,"slave latency one change start. return init cmd");
	}	
	
	return 0;
}

/**
 * @brief Raw data Notify signal
 * @param pSignal signal
 * @param size signal size
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 Failed
 */
ret_code_t RawResSetBleNotifyCmd(uint8_t *pSignal, uint16_t size)
{
	ret_code_t err_code;
	ble_gatts_hvx_params_t notify_param;
	uint16_t dataSize;
	uint16_t cnt_handle = BLE_CONN_HANDLE_INVALID;
	uint16_t notify_id;
	
	dataSize = size;
	GetGattsCharHandleValueID(&notify_id,FW_RES_ID);
	notify_param.handle = notify_id;
	notify_param.offset = BLE_NOTIFY_OFFSET;
	notify_param.type   = BLE_GATT_HVX_NOTIFICATION;
	notify_param.p_data = pSignal;
	notify_param.p_len  = &dataSize;
	
	//DEBUG_LOG(LOG_INFO,"ble notfy up. handler 0x%x. size %u",notify_id, size);
	GetBleCntHandle(&cnt_handle);
	err_code = sd_ble_gatts_hvx(cnt_handle,&notify_param);

	return err_code;

	//BleHvxErrProcess(err_code, &notify_param);
}

/* 2022.05.19 Add 角度調整対応 ++ */
/**
 * @brief Setup Angle Adjustment Data
 * @param acc_angle 角度補正データ
 * @retval None
 */
void SetupAccAngleData( ACC_ANGLE *angle_info, uint8_t state )
{
	ret_code_t err;
	ACC_ANGLE angle_deg = {0};
	ble_gatts_value_t angle_adjust_data = {0};
	uint16_t cnt_handle = BLE_CONN_HANDLE_INVALID;
	uint16_t gatt_value_handle;
	
	/* rad -> degに変換 */
	angle_rad2deg( angle_info, &angle_deg );
	
	/* 読み取れるデータへ変換 */
	if ( state != ANGLE_ADJUST_ENABLE )
	{
		/* 現在のステータスがEnable以外の場合はDisableにする */
		state = ANGLE_ADJUST_DISABLE;
	}
	g_angle_adjust_info.state = state;
	g_angle_adjust_info.x_angle = (int16_t)angle_deg.roll * 100.0;
	g_angle_adjust_info.y_angle = (int16_t)angle_deg.pitch * 100.0;
	
	GetGattsCharHandleValueID( &gatt_value_handle, ANGLE_ADJUST_ID );
	angle_adjust_data.offset	= BLE_SET_VALUE_OFFSET;
	angle_adjust_data.p_value	= (uint8_t *)&g_angle_adjust_info;
	angle_adjust_data.len		= ANGLE_ADJUST_SIZE;
	GetBleCntHandle(&cnt_handle);
	err = sd_ble_gatts_value_set( cnt_handle, gatt_value_handle, &angle_adjust_data );
	BleSetValueErrProcess(err);
}
/* 2022.05.19 Add 角度調整対応 -- */

/* 2022.06.09 Add 分解能追加 ++ */
/**
 * @brief Setup Resolution Data
 * @param None
 * @retval None
 */
void SetupResolutionData( void )
{
	ret_code_t err;
	ACC_GYRO_RESOLUTION resolution = {0};
	ble_gatts_value_t resolution_data = {0};
	uint16_t cnt_handle = BLE_CONN_HANDLE_INVALID;
	uint16_t gatt_value_handle;
	float gyro_res = 0.0;
	
	resolution.acc_resolution	= (uint16_t)ACC_FS_16G;
	gyro_res = GYRO_FS_2000DPS * GYRO_UNIT;
	resolution.gyro_resolution	= (uint16_t)gyro_res;
	
	GetGattsCharHandleValueID( &gatt_value_handle, RESOLUTION_ID );
	resolution_data.offset	= BLE_SET_VALUE_OFFSET;
	resolution_data.p_value	= (uint8_t *)&resolution;
	resolution_data.len		= RESOLUTION_SIZE;
	GetBleCntHandle(&cnt_handle);
	err = sd_ble_gatts_value_set( cnt_handle, gatt_value_handle, &resolution_data );
	BleSetValueErrProcess(err);
}
/* 2022.06.09 Add 分解能追加 -- */


uint32_t UpdateParamCheckGuest(PEVT_ST pEvent)
{
	ret_code_t err_code;
	EVT_ST event;
	uint8_t retrycount;
	bool bParamCheck;
	uint16_t cnt_handle = BLE_CONN_HANDLE_INVALID;
	uint8_t flash_op;
	EVT_ST sleepevent;
	ble_gap_conn_params_t con_para;
	
	static uint16_t RANDOM_MIN[]={MIN_CONN_INTERVAL,MIN_CONN_INTERVAL_ONE,MIN_CONN_INTERVAL_TWO,MIN_CONN_INTERVAL_THREE,MIN_CONN_INTERVAL_FOUR,MIN_CONN_INTERVAL_FIVE};
	static uint16_t RANDOM_MAX[]={MAX_CONN_INTERVAL,MAX_CONN_INTERVAL_ONE,MAX_CONN_INTERVAL_TWO,MAX_CONN_INTERVAL_THREE,MAX_CONN_INTERVAL_FOUR,MAX_CONN_INTERVAL_FIVE};
	
	int rand_id = GetRndId();
	
	con_para.slave_latency     = SLAVE_LATENCY;
	con_para.min_conn_interval = RANDOM_MIN[rand_id];//MIN_CONN_INTERVAL;
	con_para.max_conn_interval = RANDOM_MAX[rand_id];//MAX_CONN_INTERVAL;
	con_para.conn_sup_timeout  = CONN_SUP_TIMEOUT;
	
	
	bool isGuest = GetGuestModeState();
	
	if(isGuest == true)
	{
	DEBUG_LOG(LOG_INFO,"guest set max cnt interval = %d, min cnt interval = %d",con_para.max_conn_interval,con_para.min_conn_interval);
	DEBUG_LOG(LOG_INFO, "guest conncetion param update slave latecy zero enter");
	//update param check.
	bParamCheck = cnt_param_check();
	
#ifdef TEST_CNT_UPDATE_RETRY_ERROR
	//FW version 9.00
	bParamCheck = false;
#endif

		if(true == bParamCheck)
		{
			DEBUG_LOG(LOG_INFO, "guest conncetion param update success");
			// retry count
			ParamUpdateRetryCountClear();
			UpdateRetryCountClear();
			//connection update cmpl. state change connect
			event.evt_id = EVT_CNT_PARAM_UPDATE_CMPL;
			err_code = PushFifo(&event);
			DEBUG_EVT_FIFO_LOG(err_code,event.evt_id);
			StopParamUpdateTimer();
			
			//rtc int not
			GetFlashOperatoinMode(&flash_op);
			if(flash_op != FLASH_RTC_INT)
			{
				RestartForceDisconTimer();
			}
		}
		else
		{
			//retrycount check
			get_param_update_retry_count(&retrycount);
			if(retrycount < PARAM_UPDATE_RETRY_LIMIT)
			{
				DEBUG_LOG(LOG_INFO, "guest conncetion param updatting slave latecy zero");
				GetBleCntHandle(&cnt_handle);
				//request connection update param.
				err_code = sd_ble_gap_conn_param_update(cnt_handle, &con_para);
				
				//debug test
	#ifdef TEST_CNT_UPDATE_RETRY_ERROR
				err_code = NRF_ERROR_BUSY;
	#endif
				if(err_code != NRF_SUCCESS)
				{
					DEBUG_LOG(LOG_ERROR, "guest change SL zero sd_ble_gap_conn_param_update exe err. 0x%x",err_code);
					if((err_code == NRF_ERROR_INVALID_STATE) || (err_code == NRF_ERROR_BUSY))
					{
						memcpy(&event,pEvent,sizeof(event));
						sleepevent.evt_id = EVT_FORCE_SLEEP;
						err_code = PushFifo(&sleepevent);
						DEBUG_EVT_FIFO_LOG(err_code, sleepevent.evt_id);
						err_code = PushFifo(&event);
						DEBUG_EVT_FIFO_LOG(err_code, event.evt_id);
						TRACE_LOG(TR_CONNECTION_PARAM_UPDATE_API_FAILED,(uint16_t)m_conn_params.slave_latency);
					}
				}
				else
				{
					DEBUG_LOG(LOG_INFO,"guest change SL 0 sd_ble_gap_conn_param_update exe success");
				}
				//retry count increment
				param_update_retry_count_inc();
				//update parameter timer restart
				RestartParamUpdateTimer();
			}
			else
			{
				DEBUG_LOG(LOG_ERROR,"guest conncetion param update, retry count over %u",retrycount);
				SetBleErrReadCmd(UTC_BLE_CON_INTERVAL_ERROR);
				//change state paramUpdate err state.
				event.evt_id = EVT_CNT_PARAM_UPDATE_ERR;
				err_code = PushFifo(&event);
				DEBUG_EVT_FIFO_LOG(err_code,event.evt_id);
				StopParamUpdateTimer();
				TRACE_LOG(TR_CONNECTION_PARAM_UPDATE_FAILED_SLAVE_LATENCY_ZERO, 0);
			}
		}
	}
	else
	{
		DEBUG_LOG(LOG_INFO,"no guest mode");
		StopParamUpdateTimer();
	}		
	
	return 0;
}

