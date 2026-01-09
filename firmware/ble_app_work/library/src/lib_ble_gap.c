/**
  ******************************************************************************************
  * @file    lib_ble_gap.c
  * @author  k.tashiro
  * @version 1.0
  * @date    2020/09/23
  * @brief   BLE Gap Setting
  ******************************************************************************************
    Revision:
    Ver            Date             Revised by        Explanation
  ******************************************************************************************
  * 1.0            2020/09/23       k.tashiro         create new
  ******************************************************************************************
*/

/* Includes --------------------------------------------------------------*/
#include "ble_definition.h"

#include "lib_common.h"
#include "lib_fifo.h"
#include "lib_timer.h"
#include "ble_manager.h"
#include "mode_manager.h"
#include "lib_ram_retain.h"
#include "definition.h"
#include "flash_operation.h"
#include "lib_trace_log.h"

/* Definition ------------------------------------------------------------*/
#define BLE_MTU_SIZE NRF_SDH_BLE_GATT_MAX_MTU_SIZE

/* Private variables -----------------------------------------------------*/
ble_uuid_t g_adv_uuids[] = {{BLE_UUID_NUS_SERVICE, BLE_UUID_TYPE_VENDOR_BEGIN}};
BLE_ADVERTISING_DEF(gAdvertising);

volatile CON_PARAM gGetConParam;
volatile CON_PARAM gSetConParam;
/**< Handle of the current connection. */
volatile uint16_t m_conn_handle = BLE_CONN_HANDLE_INVALID;

volatile bool g_tx_send_flg = false;
#ifdef TEST_NOTIFY_ERROR
extern ble_gatts_char_handles_t gFw_res_id;
#endif
#ifdef TEST_MODE_RESULT_ERROR
extern ble_gatts_char_handles_t gMode_result_id;
#endif

/* 2020.12.08 RAW Mode時にのみACC FIFO INITをPush ++ */
volatile static bool g_raw_exec_flag = false;
/* 2020.12.08 RAW Mode時にのみACC FIFO INITをPush -- */

volatile static bool g_clear_exec_flag = false;
volatile static uint16_t g_tx_comp_counter = 0;

volatile static bool g_force_disconnect = false;

/* 2022.06.03 Add RSSI通知 ++ */
volatile static int16_t g_notify_rssi = 0;
/* 2022.06.03 Add RSSI通知 -- */

/* Private function prototypes -------------------------------------------*/
/**
 * @brief BLE Set Connect Hanble
 * @param cnt_handle_value connect handle
 * @retval None
 */
static void set_ble_cnt_handle(uint16_t cnt_handle_value);

/**
 * @brief Set BLE Send Flag
 * @param pFlag TX Send flag
 * @retval None
 */
static void set_ble_send_flg(bool Flag);

/**
 * @brief BLE Event Handler
 * @param p_ble_evt BLE Event
 * @param p_context context
 * @retval None
 */
static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
	ble_evt_t const *p_ble_context;
	volatile ret_code_t err_code;
	volatile uint32_t fifo_err;
	EVT_ST evt;
	bool uart_output_enable;
	uint16_t cnt_handle;
	static uint8_t updatecount = 0;
	/* 2020.12.08 Add RAW Mode時にのみACC FIFO INITをPush ++ */
	bool raw_exec = false;
	/* 2020.12.08 Add RAW Mode時にのみACC FIFO INITをPush -- */
	/* 2020.12.09 Add FIFO Clear ++ */
	SENSOR_FIFO_DATA_INFO sensor_fifo_data = {0};
	volatile uint16_t clear_sid = 0;
	/* 2020.12.09 Add FIFO Clear -- */
	/* 2022.07.22 Add RSSI値のint16対応 ++ */
	int16_t rssi_value = 0;
	/* 2022.07.22 Add RSSI値のint16対応 -- */

	uart_output_enable = GetUartOutputStatus();
	if(uart_output_enable == false)
	{
		LibUartEnable();
	}
	
	err_code = 0;
	
	switch (p_ble_evt->header.evt_id)
	{
	/* Connected */
	case BLE_GAP_EVT_CONNECTED:
		DEBUG_LOG(LOG_INFO,"CONNECT");
		set_ble_cnt_handle(p_ble_evt->evt.gap_evt.conn_handle);
		//err_code = nrf_ble_qwr_conn_handle_assign(&m_shose_qwr, m_conn_handle);
		// param update event generate.
		evt.evt_id = EVT_UPDATE_PARAM;
		//state change.
		fifo_err = PushFifo(&evt);
		DEBUG_EVT_FIFO_LOG(fifo_err, evt.evt_id);
		TRACE_LOG(TR_CONNECT_CMPL,0);

		ParamUpdateRetryCountClear();
		UpdateRetryCountClear();
		ClearSendCntUpdateRetryCount();
		/* 2020.12.23 Add RSSI取得テスト ++ */
		StartBleRSSIReport();
		/* 2020.12.23 Add RSSI取得テスト -- */
		////////////////////////////////////
		updatecount = 0;
		/*force discon timer start. 5 hour*/
		StartForceDisconTimer();
		break;

	/* Connect Parameter Update */
	case BLE_GAP_EVT_CONN_PARAM_UPDATE:
		updatecount++;
		DEBUG_LOG(LOG_INFO,"update count %u",updatecount);
		evt.evt_id = EVT_UPDATING_PARAM;
		/* Connection Parameter Update */
		gGetConParam.max_con_param = p_ble_evt->evt.gap_evt.params.conn_param_update.conn_params.max_conn_interval;
		gGetConParam.min_con_param = p_ble_evt->evt.gap_evt.params.conn_param_update.conn_params.min_conn_interval;
		gGetConParam.slave_param   = p_ble_evt->evt.gap_evt.params.conn_param_update.conn_params.slave_latency;
		gGetConParam.sup_time      = p_ble_evt->evt.gap_evt.params.conn_param_update.conn_params.conn_sup_timeout;
		DEBUG_LOG( LOG_INFO,"max %u, min %u",gGetConParam.max_con_param,gGetConParam.min_con_param );
		fifo_err = PushFifo(&evt);
		DEBUG_EVT_FIFO_LOG(fifo_err,evt.evt_id);

		/*update monitor timer start*/
		StopParamUpdateTimer();
		StartParamUpdateTimer();
		break;

	/* ADV_TIMEOUT */
	case BLE_GAP_EVT_ADV_SET_TERMINATED:
		evt.evt_id = EVT_ADV_TO;
		fifo_err = PushFifo(&evt);
		DEBUG_EVT_FIFO_LOG(fifo_err,evt.evt_id);
		/*trace log*/
		TRACE_LOG(TR_ADV_TIMEOUT,0);
		break;

	/* BLE disconnect event. */
	case BLE_GAP_EVT_DISCONNECTED:
		DEBUG_LOG(LOG_INFO,"DISCONNECT");
		set_ble_cnt_handle(BLE_CONN_HANDLE_INVALID);
		// all timer stop
		TimerAllStop();
		WalkTimeOutClear();
		ForceChangeDailyMode();
		FlashOpForceInit();
		//New add pincode flag clear.
		PinCodeCheckFlagClear();
		ClearNotifyFifo();
		
		//20180911 modify///////////////////
		ParamUpdateRetryCountClear();
		ClearSendCntUpdateRetryCount();
		////////////////////////////////////
		UpdateRetryCountClear();

		/* 2022.05.13 Add Guest Modeを無効化 ++ */
		SetGuestModeState( false );
		/* 2022.05.13 Add Guest Modeを無効化 -- */

		/*add acc calib, if offset flag on, system reset exec*/
		AccOffsetCheckSystemReset();
		
		evt.evt_id = EVT_DISCNT_SUCCESS;
		fifo_err = PushFifo(&evt);
		DEBUG_EVT_FIFO_LOG(fifo_err,evt.evt_id);
		TRACE_LOG(TR_DISCONNECT_CMPL,0);
		break;

	case BLE_GAP_EVT_SEC_PARAMS_REQUEST:
		// Pairing not supported
		GetBleCntHandle(&cnt_handle);
		err_code = sd_ble_gap_sec_params_reply(cnt_handle,
			BLE_GAP_SEC_STATUS_PAIRING_NOT_SUPP,
			NULL,
			NULL);
		LIB_ERR_CHECK(err_code,GAP_SECS_PARAME_ERROR, __LINE__);
		break;

	// read authorlize
	case BLE_GATTS_EVT_RW_AUTHORIZE_REQUEST:
		p_ble_context = p_ble_evt;
		SelectEvtRead(p_ble_context->evt.gatts_evt.params.authorize_request.request.read.handle);
		break;

	case BLE_GAP_EVT_PHY_UPDATE_REQUEST:
		{
			ble_gap_phys_t const phys =
			{
				.rx_phys = BLE_GAP_PHY_AUTO,
				.tx_phys = BLE_GAP_PHY_AUTO,
			};
			err_code = sd_ble_gap_phy_update(p_ble_evt->evt.gap_evt.conn_handle, &phys);
			LIB_ERR_CHECK(err_code, GAP_EVT_PHY_UPDATE_REQ_ERROR, __LINE__);
		}
		break;

	case BLE_GATTS_EVT_SYS_ATTR_MISSING:
		// No system attributes have been stored.
		GetBleCntHandle(&cnt_handle);
		err_code = sd_ble_gatts_sys_attr_set(cnt_handle, NULL, 0, 0);
		LIB_ERR_CHECK(err_code, GAP_EVT_PHY_UPDATE_REQ_ERROR, __LINE__);
		break;

	case BLE_GATTC_EVT_TIMEOUT:
		// Disconnect on GATT Client timeout event.
		err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gattc_evt.conn_handle,
										BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
		LIB_ERR_CHECK(err_code,GAP_BLE_DISCONNECT_ERROR, __LINE__);
		break;

	case BLE_GATTS_EVT_TIMEOUT:
		// Disconnect on GATT Server timeout event.
		err_code = sd_ble_gap_disconnect(p_ble_evt->evt.gatts_evt.conn_handle,
			BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
		LIB_ERR_CHECK(err_code, GAP_BLE_DISCONNECT_ERROR, __LINE__);
		break;

	// gatt event Write. ble recieve event in.
	case BLE_GATTS_EVT_WRITE:
		p_ble_context = p_context;
		SelectEvtWrite(
			p_ble_evt->evt.gatts_evt.params.write.handle,
			p_ble_evt->evt.gatts_evt.params.write.len,
			(uint8_t *)p_ble_evt->evt.gatts_evt.params.write.data);
		break;

	case BLE_GATTS_EVT_HVC:
		set_ble_send_flg(true);
		break;
				
	case BLE_GATTS_EVT_HVN_TX_COMPLETE:
		/* 2020.12.08 RAW Mode時にのみACC FIFO INITをPush ++ */
		GetBleRawExec( &raw_exec );
		if ( raw_exec == true )
		{
			clear_sid = GetClearSid();
			/* Clear SIDを取得し確認する */
			IncTxCompCount();
			if ( ( clear_sid != 0 ) && ( g_tx_comp_counter > clear_sid ) && ( g_clear_exec_flag == false ) )
			{
				g_clear_exec_flag = true;
				sensor_fifo_data.fifo_count = AccGyroGetFifoCount();
				/* Clear対象のSIDに達していたら現在のFIFO Countを取得してFIFOにPush ++ */
				FifoClearPushFifo( &sensor_fifo_data );
			}
			
			evt.evt_id = EVT_ACC_FIFO_INT;
			fifo_err = PushFifo( (void *)&evt );
			DEBUG_EVT_FIFO_LOG( fifo_err, EVT_ACC_FIFO_INT );
		}
		/* 2020.12.08 RAW Mode時にのみACC FIFO INITをPush -- */
		set_ble_send_flg(true);
		break;
	/* 2022.03.18 Add MTUサイズを変更++ */
	case BLE_GAP_EVT_DATA_LENGTH_UPDATE:
		if ( p_ble_evt->evt.gap_evt.params.data_length_update.effective_params.max_tx_octets != BLE_MTU_SIZE )
		{
			DEBUG_LOG( LOG_INFO, "RequestMTU=%d", p_ble_evt->evt.gap_evt.params.data_length_update.effective_params.max_tx_octets );
			/* MTUサイズ変更 */
			UpdateMtuSize( BLE_MTU_SIZE );
		}
		break;
	/* 2022.03.18 Add MTUサイズを変更-- */

	/* 2020.12.23 Add RSSI取得テスト ++ */
	case BLE_GAP_EVT_RSSI_CHANGED:
		/* ここは取得した値の更新のみにしてタイマーを使用してRSSI Notifyを発行する */
		/* int8 -> int16へ変換 */
		rssi_value = (int16_t)p_ble_evt->evt.gap_evt.params.rssi_changed.rssi;
		SetRssiValue( rssi_value );
		break;
	/* 2020.12.23 Add RSSI取得テスト -- */
	
	default:
		DEBUG_LOG(LOG_DEBUG,"defalut : 0x%x",p_ble_evt->header.evt_id);
		// No implementation needed.
		break;
    }
		
	if(uart_output_enable == false)
	{
		LibUartDisable();
	}
}

/**
 * @brief BLE advertising initialze
 * @param None
 * @retval None
 */

static void advertising_init(void)
{
	ret_code_t err_code;
	ble_advertising_init_t init;
	//20180810 add company address and company ID
	ble_advdata_manuf_data_t manuf_specific_data;
	ble_gap_addr_t add_t;
	/* 2022.06.28 Add SCAN_RESPの内容追加 ++ */
	ble_uuid_t daily_uuid[1];
	/* 2022.06.28 Add SCAN_RESPの内容追加 -- */
	
	/////
	sd_ble_gap_addr_get(&add_t);
	
	memset(&init, 0, sizeof(init));

	init.advdata.name_type          = BLE_ADVDATA_FULL_NAME;
	init.advdata.include_appearance = true;
	init.advdata.flags              = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;

	/* 2022.08.17 Modify iOSではFlagを0x02に設定しないとスキャンできないためADVの設定を変更 ++ */
	//init.srdata.uuids_complete.uuid_cnt = sizeof(g_adv_uuids) / sizeof(g_adv_uuids[0]);
	//init.srdata.uuids_complete.p_uuids  = g_adv_uuids;
	daily_uuid[0].uuid = DAILY_MODE_UUID2;
	daily_uuid[0].type = BLE_UUID_TYPE_BLE;
	init.advdata.uuids_more_available.uuid_cnt = 1;
	init.advdata.uuids_more_available.p_uuids  = daily_uuid;
	/* 2022.08.17 Modify iOSではFlagを0x02に設定しないとスキャンできないためADVの設定を変更 -- */

	init.config.ble_adv_fast_enabled  = true;
	init.config.ble_adv_fast_interval = APP_ADV_INTERVAL;
	init.config.ble_adv_fast_timeout  = APP_ADV_DURATION;

	init.config.ble_adv_on_disconnect_disabled = true;
		
	//20180810 add company address and company ID
	manuf_specific_data.company_identifier = COMPANY_ID; //future change. after setting number. dummy number
	/* 2022.06.28 Delete SCAN_RSPの内容をデバイス名に修正したため削除 ++ */
	manuf_specific_data.data.size = BLE_GAP_ADDR_LEN;
	//little endian
	manuf_specific_data.data.p_data = add_t.addr;
	/* 2022.06.28 Delete SCAN_RSPの内容をデバイス名に修正したため削除 -- */
	
	init.srdata.p_manuf_specific_data = &manuf_specific_data; //
	////////////////////////////////////////////////////////////
	init.evt_handler = NULL;

    err_code = ble_advertising_init(&gAdvertising, &init);
	LIB_ERR_CHECK(err_code, ADV_INIT_ERROR, __LINE__);
	TRACE_LOG(TR_ADV_INIT_CMPL,0);
	ble_advertising_conn_cfg_tag_set(&gAdvertising, APP_BLE_CONN_CFG_TAG);
}
#if 0
static void advertising_init(void)
{
	ret_code_t err_code;
	ble_advertising_init_t init;
	//20180810 add company address and company ID
	ble_advdata_manuf_data_t manuf_specific_data;
	ble_gap_addr_t add_t;
	
	/////
	sd_ble_gap_addr_get(&add_t);
	
	memset(&init, 0, sizeof(init));

	init.advdata.name_type          = BLE_ADVDATA_FULL_NAME;
	init.advdata.include_appearance = true;
	init.advdata.flags              = BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE;

	init.srdata.uuids_complete.uuid_cnt = sizeof(g_adv_uuids) / sizeof(g_adv_uuids[0]);
	init.srdata.uuids_complete.p_uuids  = g_adv_uuids;

	init.config.ble_adv_fast_enabled  = true;
	init.config.ble_adv_fast_interval = APP_ADV_INTERVAL;
	init.config.ble_adv_fast_timeout  = APP_ADV_DURATION;

	init.config.ble_adv_on_disconnect_disabled = true;
		
	//20180810 add company address and company ID
	manuf_specific_data.company_identifier = COMPANY_ID; //future change. after setting number. dummy number
	manuf_specific_data.data.size = BLE_GAP_ADDR_LEN;
	//little endian
	manuf_specific_data.data.p_data = add_t.addr;
	init.srdata.p_manuf_specific_data = &manuf_specific_data; //
	////////////////////////////////////////////////////////////
	init.evt_handler = NULL;

    err_code = ble_advertising_init(&gAdvertising, &init);
	LIB_ERR_CHECK(err_code, ADV_INIT_ERROR, __LINE__);
	TRACE_LOG(TR_ADV_INIT_CMPL,0);
	ble_advertising_conn_cfg_tag_set(&gAdvertising, APP_BLE_CONN_CFG_TAG);
}
#endif
/**
 * @brief GAP Parameter Initialize
 * @param None
 * @retval None
 */
static void gap_params_init(void)
{
	volatile uint32_t ble_err;
	ble_gap_conn_params_t gap_conn_params;
	ble_gap_conn_sec_mode_t sec_mode;

	BLE_GAP_CONN_SEC_MODE_SET_OPEN(&sec_mode);

	ble_err = sd_ble_gap_device_name_set(&sec_mode,(const uint8_t *)DEVICE_NAME,strlen(DEVICE_NAME));
	LIB_ERR_CHECK(ble_err, GAP_PARAM_INIT_ERROR, __LINE__);
	if(ble_err == NRF_SUCCESS)
	{
		TRACE_LOG(TR_BLE_DEVICE_NAME_SET_CMPL,0);
	}
	
	memset(&gap_conn_params, 0, sizeof(gap_conn_params));
	gap_conn_params.min_conn_interval = MIN_CONN_INTERVAL;
	gap_conn_params.max_conn_interval = MAX_CONN_INTERVAL;
	gap_conn_params.slave_latency     = SLAVE_LATENCY;
	gap_conn_params.conn_sup_timeout  = CONN_SUP_TIMEOUT;
	
	//appearance_set. add 20180817
	ble_err = sd_ble_gap_appearance_set(BLE_APPEARANCE_RUNNING_WALKING_SENSOR_IN_SHOE);
	LIB_ERR_CHECK(ble_err, GAP_PARAM_INIT_ERROR, __LINE__);
	
	ble_err = sd_ble_gap_ppcp_set(&gap_conn_params);
	LIB_ERR_CHECK(ble_err, GAP_PARAM_INIT_ERROR, __LINE__);
}

/**
 * @brief BLE Tx power init
 * @param None
 * @retval None
 */
static void ble_power_init(void)
{
	ret_code_t err_code;

	/* BLE Tx Powerを +4dBmに変更する @@@@@ */
	err_code = sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_ADV, gAdvertising.adv_handle, TX_POWER_VALUE);
	LIB_ERR_CHECK(err_code, GAP_TX_POWER_SET_ERROR, __LINE__);
}

/**
 * @brief BLE Set Connect Hanble
 * @param cnt_handle_value connect handle
 * @retval None
 */
static void set_ble_cnt_handle(uint16_t cnt_handle_value)
{
	uint32_t err;
	uint8_t  cnt_hdl_critical;
	
	err = sd_nvic_critical_region_enter(&cnt_hdl_critical);
	if(err == NRF_SUCCESS)
	{
		m_conn_handle = cnt_handle_value;
		cnt_hdl_critical = 0;
		err = sd_nvic_critical_region_exit(cnt_hdl_critical);
		if(err != NRF_SUCCESS)
		{
			//nothing to do
		}
	}
}

/**
 * @brief Set BLE Send Flag
 * @param pFlag TX Send flag
 * @retval None
 */
static void set_ble_send_flg(bool Flag)
{
	uint32_t err;
	uint8_t  cnt_hdl_critical;
	
	err = sd_nvic_critical_region_enter(&cnt_hdl_critical);
	if(err == NRF_SUCCESS)
	{
		g_tx_send_flg = Flag;
		cnt_hdl_critical = 0;
		err = sd_nvic_critical_region_exit(cnt_hdl_critical);
		if(err != NRF_SUCCESS)
		{
			//nothing to do
		}
	}
}

/**
 * @brief Get Setting Connect Parameter
 * @param None
 * @retval CON_PARAM
 */
CON_PARAM GetSetConParam(void)
{
	return gSetConParam;
}

/**
 * @brief Setup Connect Parameter
 * @param setcntparam CON_PARAM構造体へのポインタ変数
 * @retval None
 */
void SetSettingConParam(CON_PARAM *setcntparam)
{
	gSetConParam.max_con_param = setcntparam->max_con_param;
	gSetConParam.min_con_param = setcntparam->min_con_param;
	gSetConParam.slave_param   = setcntparam->slave_param;
	gSetConParam.sup_time      = setcntparam->sup_time;
}

/**
 * @brief Get Connect Parameter
 * @param p_con_param CON_PARAM構造体を格納する変数へのポインタ
 * @retval None
 */
void GetConParam( CON_PARAM *p_con_param )
{
	if ( p_con_param != NULL )
	{
		*p_con_param = gGetConParam;
	}
}

/**
 * @brief BLE advertising start
 * @param None
 * @retval None
 */
void BleAdvertisingStart(void)
{
	ret_code_t err_code;

//debug test mac address random
#ifdef TEST_RANDOM_MAC
	ble_gap_addr_t data_t;
	
	data_t.addr_id_peer = 0;
	data_t.addr_type = BLE_GAP_ADDR_TYPE_PUBLIC;
	for(int8_t i = 0; i < 6; i++)
	{
		data_t.addr[i] = rand() % 256;
	}
	sd_ble_gap_addr_set(&data_t);
#endif
	err_code = ble_advertising_start(&gAdvertising, BLE_ADV_MODE_FAST);
	if(err_code != NRF_SUCCESS)
	{
		TRACE_LOG(TR_ADV_FAILED, 0);
		DEBUG_LOG(LOG_ERROR,"ADV Start failed. err code 0x%x",err_code);
		// case of adv start err, system reset.
		sd_nvic_SystemReset();
	}
	else
	{
		TRACE_LOG(TR_ADV_START, 0);
		DEBUG_LOG(LOG_ERROR,"ADV_START");
	}
}

/**
 * @brief BLE Gap Setting
 * @param None
 * @retval None
 */
void BleGapSetting(void)
{
	uint32_t err_code;
	ble_opt_t opt = {0};

	//event handler reg.
	NRF_SDH_BLE_OBSERVER(m_ble_observer, APP_BLE_OBSERVER_PRIO, ble_evt_handler, NULL);
	gap_params_init();
	advertising_init();
	ble_power_init();

	/* 2020.12.09 Add  Connection Event Length Extensionを有効に設定 ++ */
	opt.common_opt.conn_evt_ext.enable = 1;
	err_code = sd_ble_opt_set(BLE_COMMON_OPT_CONN_EVT_EXT, &opt);
	LIB_ERR_CHECK(err_code, GAP_COMMON_OPT_CONN_EVT_EXT, __LINE__);
	/* 2020.12.09 Add  Connection Event Length Extensionを有効に設定 -- */
}

/**
 * @brief BLE hvx Error Process
 * @param err_code Error Code
 * @param pNotify_param Notify Parameter
 * @retval None
 */
void BleHvxErrProcess(ret_code_t err_code,ble_gatts_hvx_params_t *pNotify_param)
{	
	ret_code_t ble_err;
	uint16_t cnt_handle = BLE_CONN_HANDLE_INVALID;
	uint16_t gatts_value_handle;
	
	if(err_code != NRF_SUCCESS)
	{
		//trace log
		TRACE_LOG(TR_NOTIFY_ERROR,(uint16_t)err_code);
		GetGattsCharHandleValueID(&gatts_value_handle,FW_RES_ID);
		if(pNotify_param->handle == gatts_value_handle)
		{
			SetBleErrReadCmd(UTC_BLE_FW_RESPONSE_ERROR);
		}
		GetGattsCharHandleValueID(&gatts_value_handle,MODE_RESULT_ID);
		if(pNotify_param->handle == gatts_value_handle)
		{
			SetBleErrReadCmd(UTC_BLE_ULT_SEND_RESULT_ERROR);
		}
		DEBUG_LOG(LOG_ERROR,"notify failed. err code 0x%x", err_code);
	}
	else
	{
		DEBUG_LOG(LOG_DEBUG,"notify success");
	}
	
	switch(err_code)
	{
	case NRF_SUCCESS:
		break;
	case NRF_ERROR_INVALID_STATE:
	case NRF_ERROR_INVALID_ADDR:
	case NRF_ERROR_INVALID_PARAM:
	case BLE_ERROR_INVALID_ATTR_HANDLE:
	case BLE_ERROR_GATTS_INVALID_ATTR_TYPE:
	case NRF_ERROR_NOT_FOUND:
	case NRF_ERROR_FORBIDDEN:
	case NRF_ERROR_DATA_SIZE:
	case BLE_ERROR_GATTS_SYS_ATTR_MISSING:
		//up list is force system reset.
		sd_nvic_SystemReset();
		break;
	case NRF_ERROR_RESOURCES:
		
		break;
	case NRF_ERROR_TIMEOUT:
		//force change daily mode
		ForceChangeDailyMode();
		FlashOpForceInit();
		GetBleCntHandle(&cnt_handle);
		// force disconnect
		ble_err = sd_ble_gap_disconnect(cnt_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
		if(ble_err != NRF_SUCCESS)
		{
			/*trace log*/
			//fail disconnect. soft reset
			sd_nvic_SystemReset();
		}
		break;
	case NRF_ERROR_BUSY:
		//retry nottify
		break;
	default:
		break;
	}
}

/**
 * @brief BLE Retry Notify
 * @param pEvent Event Information
 * @retval 0 Success
 */
uint32_t RetryNotify(PEVT_ST pEvent)
{
	ble_gatts_hvx_params_t *pNotify_param;
	ret_code_t ret;
	uint8_t i;
	uint16_t cnt_handle = BLE_CONN_HANDLE_INVALID;
	UNUSED_PARAMETER(*pEvent);
	
	DEBUG_LOG(LOG_DEBUG, "retry notify process");
	
	for(i = 0; (NRF_ERROR_NULL != NotifyPopFifo(pNotify_param)); i++)
	{
		if(pNotify_param != NULL)
		{
			GetBleCntHandle(&cnt_handle);
			ret = sd_ble_gatts_hvx(cnt_handle, pNotify_param);
			if(ret != NRF_SUCCESS)
			{
				DEBUG_LOG(LOG_ERROR,"retry notify failed. force system reset. 0x%x",ret);
				//retry failed. force system reset.
				sd_nvic_SystemReset();
			}
		}
	}
	return 0;
}

/**
 * @brief BLE Authoraize Error Process
 * @param err_code Error Code
 * @retval None
 */
void BleAuthorizeErrProcess(ret_code_t err_code)
{
	ret_code_t  err;
	uint16_t                cnt_handle = BLE_CONN_HANDLE_INVALID;
	
	if(err_code != NRF_SUCCESS)
	{
		TRACE_LOG(TR_AUTHRIZE_ERROR,(uint16_t)err_code);
		DEBUG_LOG(LOG_ERROR,"ble authorize err 0x%x", err_code);
	}
	
	switch(err_code)
	{
	case NRF_SUCCESS:
		break;
	case BLE_ERROR_INVALID_CONN_HANDLE:
	case NRF_ERROR_INVALID_ADDR:
	case NRF_ERROR_INVALID_STATE:
	case NRF_ERROR_INVALID_PARAM:
		break;
	case NRF_ERROR_TIMEOUT:
		GetBleCntHandle(&cnt_handle);
		err = sd_ble_gap_disconnect(cnt_handle, BLE_HCI_REMOTE_USER_TERMINATED_CONNECTION);
		if(err != NRF_SUCCESS)
		{
			/*trace log*/
			sd_nvic_SystemReset();
		}
		break;
	case NRF_ERROR_BUSY:
		break;
	default:
		break;
	}
}

/**
 * @brief BLE Value Error Process
 * @param err_code Error Code
 * @retval None
 */
void BleSetValueErrProcess(ret_code_t err_code)
{
	switch(err_code)
	{
		case NRF_SUCCESS:
			break;
		case NRF_ERROR_INVALID_ADDR:
		case NRF_ERROR_INVALID_PARAM:
		case NRF_ERROR_NOT_FOUND:
		case NRF_ERROR_FORBIDDEN:
		case NRF_ERROR_DATA_SIZE:
		case BLE_ERROR_INVALID_CONN_HANDLE:
			DEBUG_LOG(LOG_ERROR,"ble gatts set value err 0x%x",err_code);
			sd_nvic_SystemReset();
			break;
		default:
			break;
	}
}

/**
 * @brief Get BLE Connect Handle
 * @param pCnt_handle_value Connect Handle
 * @retval None
 */
void GetBleCntHandle(uint16_t *pCnt_handle_value)
{
	uint32_t err;
	uint8_t  cnt_hdl_critical;
	
	err = sd_nvic_critical_region_enter(&cnt_hdl_critical);
	if(err == NRF_SUCCESS)
	{
		*pCnt_handle_value = m_conn_handle;
		cnt_hdl_critical = 0;
		err = sd_nvic_critical_region_exit(cnt_hdl_critical);
		if(err != NRF_SUCCESS)
		{
			//nothing to do
		}
	}
}

/**
 * @brief Get BLE Send Flag
 * @param pFlag TX Send flag
 * @retval None
 */
void GetBleSendFlg(bool *pFlag)
{
	uint32_t err;
	uint8_t  cnt_hdl_critical;
	
	err = sd_nvic_critical_region_enter(&cnt_hdl_critical);
	if(err == NRF_SUCCESS)
	{
		*pFlag = g_tx_send_flg;
		cnt_hdl_critical = 0;
		err = sd_nvic_critical_region_exit(cnt_hdl_critical);
		if(err != NRF_SUCCESS)
		{
			//nothing to do
		}
	}
}

/**
 * @brief Set BLE Error Read Command
 * @param func_err_code Function Error Code
 * @retval None
 */
void SetBleErrReadCmd(uint8_t func_err_code)
{
	ret_code_t err;
	volatile ble_gatts_value_t err_value;
	volatile uint8_t read_err_code;
	uint16_t cnt_handle = BLE_CONN_HANDLE_INVALID;
	uint16_t read_err_value_id;
	
	read_err_code     = func_err_code;
	err_value.offset  = BLE_SET_VALUE_OFFSET;
	err_value.p_value = (void*)&read_err_code;
	err_value.len     = sizeof(read_err_code);
	
	DEBUG_LOG(LOG_INFO,"utc ble err read value set. err 0x%x. size %u",func_err_code,err_value.len);
	GetBleCntHandle(&cnt_handle);
	GetGattsCharHandleValueID(&read_err_value_id,READ_ERR_ID);
	err = sd_ble_gatts_value_set(cnt_handle, read_err_value_id,(void*)&err_value);
	BleSetValueErrProcess(err);
}

/**
 * @brief Set BLE Notify Command
 * @param uuid_value_handle UUID Handle
 * @param pSignal Signal
 * @param size Signal Size
 * @retval None
 */
void SetBleNotifyCmd(uint16_t uuid_value_handle, uint8_t *pSignal, uint16_t size)
{
	ret_code_t err_code;
	ble_gatts_hvx_params_t notify_param;
	uint16_t dataSize;
	uint16_t cnt_handle = BLE_CONN_HANDLE_INVALID;
	
	dataSize = size;
	
	notify_param.handle = uuid_value_handle;
	notify_param.offset = BLE_NOTIFY_OFFSET;
	notify_param.type   = BLE_GATT_HVX_NOTIFICATION;
	notify_param.p_data = pSignal;
	notify_param.p_len  = &dataSize;
	
	DEBUG_LOG(LOG_INFO,"ble notfy up. handler 0x%x. size %u",uuid_value_handle, size);
	GetBleCntHandle(&cnt_handle);
	err_code = sd_ble_gatts_hvx(cnt_handle,&notify_param);

//debug test
#ifdef TEST_NOTIFY_ERROR
	if(uuid_value_handle == gFw_res_id.value_handle)
	{
		err_code = NRF_ERROR_BUSY;
	}
#endif
//debug test
#ifdef TEST_MODE_RESULT_ERROR
	if(uuid_value_handle == gMode_result_id.value_handle)
	{
		err_code = NRF_ERROR_BUSY;
	}
#endif

	BleHvxErrProcess(err_code, &notify_param);
}

/**
 * @brief Set BLE Raw Data Notify
 * @param enable Set RawMode Exec/Non-Exec
 * @retval None
 */
void SetBleRawExec( bool enable )
{
	uint32_t err;
	uint8_t  exec_flag_critical;
	
	err = sd_nvic_critical_region_enter(&exec_flag_critical);
	if(err == NRF_SUCCESS)
	{
		g_raw_exec_flag = enable;
		/* 2020.12.09 Add Clear Flag初期化を追加 ++ */
		g_clear_exec_flag = false;
		/* 2020.12.09 Add Clear Flag初期化を追加 -- */
		exec_flag_critical = 0;
		err = sd_nvic_critical_region_exit(exec_flag_critical);
		if(err != NRF_SUCCESS)
		{
			//nothing to do
		}
	}
}

/**
 * @brief Get BLE Raw Data Notify
 * @param enable Get RawMode Exec/Non-Exec
 * @retval None
 */
void GetBleRawExec( bool *enable )
{
	uint32_t err;
	uint8_t  exec_flag_critical;
	
	err = sd_nvic_critical_region_enter(&exec_flag_critical);
	if(err == NRF_SUCCESS)
	{
		*enable = g_raw_exec_flag;
		exec_flag_critical = 0;
		err = sd_nvic_critical_region_exit(exec_flag_critical);
		if(err != NRF_SUCCESS)
		{
			//nothing to do
		}
	}
}

/**
 * @brief BLE Tx power change
 * @param tx_power tx power
 * @retval None
 */
void BlePowerChenge( int8_t tx_power )
{
	ret_code_t err_code;
	uint16_t connect_handle;

	GetBleCntHandle( &connect_handle );
	err_code = sd_ble_gap_tx_power_set(BLE_GAP_TX_POWER_ROLE_CONN, connect_handle, tx_power);
	LIB_ERR_CHECK(err_code, GAP_TX_POWER_SET_ERROR, __LINE__);
}

/**
 * @brief BLE Inc Tx Complete Count
 * @param tx_power tx power
 * @retval None
 */
void IncTxCompCount( void )
{
	uint32_t err;
	uint8_t  tx_comp_critical;
	
	err = sd_nvic_critical_region_enter(&tx_comp_critical);
	if(err == NRF_SUCCESS)
	{
		g_tx_comp_counter++;
		tx_comp_critical = 0;
		err = sd_nvic_critical_region_exit(tx_comp_critical);
		if(err != NRF_SUCCESS)
		{
			//nothing to do
		}
	}
}

/**
 * @brief BLE Set Tx Complete Count
 * @param tx_power tx power
 * @retval None
 */
void SetTxCompCount( uint16_t tx_comp_count )
{
	uint32_t err;
	uint8_t  tx_comp_critical;
	
	err = sd_nvic_critical_region_enter(&tx_comp_critical);
	if(err == NRF_SUCCESS)
	{
		g_tx_comp_counter = tx_comp_count;
		tx_comp_critical = 0;
		err = sd_nvic_critical_region_exit(tx_comp_critical);
		if(err != NRF_SUCCESS)
		{
			//nothing to do
		}
	}
}

/**
 * @brief BLE Set Force Disconnect
 * @param None
 * @retval None
 */
void SetForceDisconnect( void )
{
	uint32_t err;
	uint8_t force_critical;
	
	err = sd_nvic_critical_region_enter(&force_critical);
	if(err == NRF_SUCCESS)
	{
		g_force_disconnect = ( g_force_disconnect == false ) ? true : false;
		force_critical = 0;
		err = sd_nvic_critical_region_exit(force_critical);
		if(err != NRF_SUCCESS)
		{
			//nothing to do
		}
	}
}

/**
 * @brief BLE Get Force Disconnect
 * @param None
 * @retval None
 */
void GetForceDisconnect( bool *force_flag )
{
	uint32_t err;
	uint8_t force_critical;
	
	err = sd_nvic_critical_region_enter(&force_critical);
	if(err == NRF_SUCCESS)
	{
		*force_flag = g_force_disconnect;
		force_critical = 0;
		err = sd_nvic_critical_region_exit(force_critical);
		if(err != NRF_SUCCESS)
		{
			//nothing to do
		}
	}
}

/* 2020.12.23 Add RSSI取得テスト ++ */
/**
 * @brief Start BLE RSSI
 * @param None
 * @retval None
 */
void StartBleRSSIReport( void )
{
	uint32_t err_code;
	uint16_t connect_hanble;

	GetBleCntHandle( &connect_hanble );
	err_code = sd_ble_gap_rssi_start( connect_hanble, BLE_RSSI_THS_VALUE, BLE_RSSI_COUNT_THS_VALUE );
	if ( err_code != NRF_SUCCESS )
	{
		DEBUG_LOG( LOG_INFO, "Start RSSI Error : %d", err_code );
	}
	else
	{
		/* 2022.06.03 Add RSSI Notify Timer Start ++ */
		StartRssiNotifyTimer();
		/* 2022.06.03 Add RSSI Notify Timer Start -- */
	}
}

/**
 * @brief Stop BLE RSSI
 * @param None
 * @retval None
 */
void StopBleRSSIReport( void )
{
	uint32_t err_code;
	uint16_t connect_hanble;

	GetBleCntHandle( &connect_hanble );
	err_code = sd_ble_gap_rssi_stop( connect_hanble );
	if ( err_code != NRF_SUCCESS )
	{
		DEBUG_LOG( LOG_INFO, "Stop RSSI Error : %d", err_code );
	}
	/* 2022.06.03 Add RSSI Notify Timer Stop ++ */
	StopRssiNotifyTimer();
	/* 2022.06.03 Add RSSI Notify Timer Stop -- */
}

/**
 * @brief Set RSSI Value
 * @param value RSSI Value
 * @retval None
 */
void SetRssiValue( int16_t value )
{
	uint32_t err;
	uint8_t rssi_set_critical;
	
	err = sd_nvic_critical_region_enter( &rssi_set_critical );
	if(err == NRF_SUCCESS)
	{
		g_notify_rssi = value;
		rssi_set_critical = 0;
		err = sd_nvic_critical_region_exit( rssi_set_critical );
		if(err != NRF_SUCCESS)
		{
			//nothing to do
		}
	}
}

/**
 * @brief Get RSSI Value
 * @param value RSSI Value
 * @retval None
 */
void GetRssiValue( int16_t *value )
{
	uint32_t err;
	uint8_t rssi_get_critical;
	
	err = sd_nvic_critical_region_enter( &rssi_get_critical );
	if(err == NRF_SUCCESS)
	{
		if ( value != NULL )
		{
			*value = g_notify_rssi;
		}
		rssi_get_critical = 0;
		err = sd_nvic_critical_region_exit( rssi_get_critical );
		if(err != NRF_SUCCESS)
		{
			//nothing to do
		}
	}
}

/* 2020.12.23 Add RSSI取得テスト -- */

