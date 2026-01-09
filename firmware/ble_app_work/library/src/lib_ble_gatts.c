/**
  ******************************************************************************************
  * @file    lib_ble_gatts.c
  * @author  k.tashiro
  * @version 1.0
  * @date    2020/09/23
  * @brief   BLE GATT Setting
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
#include "state_control.h"
#include "lib_fifo.h"
#include "lib_timer.h"
#include "mode_manager.h"
#include "lib_ram_retain.h"
#include "ble_manager.h"
#include "definition.h"
#include "lib_trace_log.h"

/* variables -------------------------------------------------------------*/
/*debug*/
extern uint8_t gDaily_display;

/* Private variables -----------------------------------------------------*/
/**< Context for the Queued Write module.*/
NRF_BLE_QWR_DEF(m_shose_qwr);
/**< GATT module instance. */
NRF_BLE_GATT_DEF(m_shose_gatt);

static uint16_t gSample_ble;
static uint16_t gMode_ble;
static uint16_t gDaily_ble;
static uint16_t gRespose_ble;
static uint16_t gBatt_ble;
static uint16_t gPairing_ble;
/*add calib*/
static uint16_t gCalib_ble;
/* 2022.03.22 Add Test Command ++ */
static uint16_t g_test_command_ble;
/* 2022.03.22 Add Test Command -- */
/* 2022.05.16 Add 角度調整 ++ */
static uint16_t g_angle_adjust_ble;
/* 2022.05.16 Add 角度調整 -- */
/* 2020.12.23 Add RSSI取得テスト ++ */
static uint16_t g_rssi_notify_ble;
/* 2020.12.23 Add RSSI取得テスト -- */
/* 2022.06.09 Add 分解能のCharacteristic追加 ++ */
static uint16_t g_resolution_ble;
/* 2022.06.09 Add 分解能のCharacteristic追加 -- */

/* shoes sensor BLE service and characteristic UUID list */
volatile ble_gatts_char_handles_t gShoes_id;
volatile ble_gatts_char_handles_t gMode_change_id;
volatile ble_gatts_char_handles_t gMode_trigger_id;
volatile ble_gatts_char_handles_t gMode_result_id;
volatile ble_gatts_char_handles_t gDaily_id_set_id;
volatile ble_gatts_char_handles_t gDaily_log_id;
volatile ble_gatts_char_handles_t gFw_res_id;
volatile ble_gatts_char_handles_t gFw_ver_id;
volatile ble_gatts_char_handles_t gRead_err_id;
volatile ble_gatts_char_handles_t gGet_time_id;
volatile ble_gatts_char_handles_t gBattery_mVol_id;
volatile ble_gatts_char_handles_t gPairing_code_id;
volatile ble_gatts_char_handles_t gPairing_match_id;
volatile ble_gatts_char_handles_t gPairing_clear_id;
volatile ble_gatts_char_handles_t gCalib_id;

/*test raw data send*/
volatile ble_gatts_char_handles_t gRawData_id;
/* 2022.03.22 Add Test Command ++ */
volatile ble_gatts_char_handles_t g_test_cmd_id;
/* 2022.03.22 Add Test Command -- */
/* 2022.05.16 Add 角度調整 ++ */
volatile ble_gatts_char_handles_t g_angle_adjust_id;
/* 2022.05.16 Add 角度調整 -- */

/* 2020.12.23 Add RSSI取得テスト ++ */
volatile ble_gatts_char_handles_t g_rssi_notify_id;
/* 2020.12.23 Add RSSI取得テスト -- */
/* 2022.06.09 Add 分解能 ++ */
volatile ble_gatts_char_handles_t g_resolution_id;
/* 2022.06.09 Add 分解能 ++ */

uint8_t gDammyRaw[RAW_DATA_SIZE] = {0};

ble_gatts_char_handles_t static *const g_gatts_char_handle_table[GATT_CHAR_HANDLE_END_ID] = 
{
	[SHOES_ID]			= (ble_gatts_char_handles_t*)&gShoes_id, 
	[MODE_CHANGE_ID]	= (ble_gatts_char_handles_t*)&gMode_change_id, 
	[MODE_TRIGGER_ID]	= (ble_gatts_char_handles_t*)&gMode_trigger_id, 
	[MODE_RESULT_ID]	= (ble_gatts_char_handles_t*)&gMode_result_id,
	[DAILY_SET_ID]		= (ble_gatts_char_handles_t*)&gDaily_id_set_id,
	[DAILY_LOG_ID]		= (ble_gatts_char_handles_t*)&gDaily_log_id,
	[FW_RES_ID]			= (ble_gatts_char_handles_t*)&gFw_res_id,
	[FW_VER_ID]			= (ble_gatts_char_handles_t*)&gFw_ver_id,
	[READ_ERR_ID]		= (ble_gatts_char_handles_t*)&gRead_err_id,
	[GET_TIME_ID]		= (ble_gatts_char_handles_t*)&gGet_time_id,
	[BAT_VOL_ID]		= (ble_gatts_char_handles_t*)&gBattery_mVol_id,
	[PAIRING_CODE_ID]	= (ble_gatts_char_handles_t*)&gPairing_code_id,
	[PAIRING_MATCH_ID]	= (ble_gatts_char_handles_t*)&gPairing_match_id,
	[PAIRING_CLEAR_ID]	= (ble_gatts_char_handles_t*)&gPairing_clear_id,
	[CALIB_ID]			= (ble_gatts_char_handles_t*)&gCalib_id,
	[RAW_DATA_ID]		= (ble_gatts_char_handles_t*)&gRawData_id,
	/* 2022.03.22 Add Test Command ++ */
	[TEST_CMD_ID]		= (ble_gatts_char_handles_t*)&g_test_cmd_id,
	/* 2022.03.22 Add Test Command -- */
	/* 2022.05.16 Add 角度調整 ++ */
	[ANGLE_ADJUST_ID]	= (ble_gatts_char_handles_t*)&g_angle_adjust_id,
	/* 2022.05.16 Add 角度調整 -- */
	[RSSI_NOTIFY_ID]	= (ble_gatts_char_handles_t*)&g_rssi_notify_id,		/* 2022.06.03 Add RSSI取得 */
	[RESOLUTION_ID]		= (ble_gatts_char_handles_t*)&g_resolution_id		/* 2022.06.09 Add 分解能取得 */
};

/* Private variables -----------------------------------------------------*/


/**
 * @brief BLE Qwr Error Handler
 * @param nrf_error error code
 * @retval None
 */
static void nrf_qwr_error_handler(uint32_t nrf_error)
{
    APP_ERROR_HANDLER(nrf_error);
}

/**
 * @brief Set BLE characteristic default
 * @param char_md デフォルトに設定するble_gatts_char_md_t構造体へのポインタ
 * @retval None
 */
static void default_char_set(ble_gatts_char_md_t *char_md)
{
	char_md->p_char_user_desc  = NULL;
	char_md->p_char_pf         = NULL;
	char_md->p_user_desc_md    = NULL;
	char_md->p_cccd_md         = NULL;
	char_md->p_sccd_md         = NULL;
}

/**
 * @brief Set BLE attribute default
 * @param attr_md デフォルトに設定するble_gatts_attr_md_t構造体へのポインタ
 * @retval None
 */
static void default_attr_set(ble_gatts_attr_md_t *attr_md)
{
	attr_md->rd_auth = 0;
	attr_md->wr_auth = 0;
	attr_md->vlen    = 0;
}

/**
 * @brief Set BLE attribute and characteristic default
 * @param attr_char_value デフォルトに設定するble_gatts_attr_t構造体へのポインタ
 * @retval None
 */
static void default_attr_char_set(ble_gatts_attr_t *attr_char_value)
{
	attr_char_value->p_uuid    = NULL;
	attr_char_value->p_attr_md = NULL;
	attr_char_value->init_len  = 0;
	attr_char_value->init_offs = 0;
	attr_char_value->max_len   = 0;
	attr_char_value->p_value   = NULL;
}

/**
 * @brief BLE GATTS initialize.
 * @param None
 * @retval None
 */
static void gatt_init(void)
{
	ret_code_t err_code;
	
	err_code = nrf_ble_gatt_init(&m_shose_gatt, NULL);
	LIB_ERR_CHECK(err_code,GATT_INIT_ERROR, __LINE__);
	/* 2022.03.17 Add MTU Size Change ++ */
	err_code = nrf_ble_gatt_att_mtu_periph_set( &m_shose_gatt,  NRF_SDH_BLE_GATT_MAX_MTU_SIZE );
	LIB_ERR_CHECK(err_code,GATT_INIT_ERROR, __LINE__);
	/* 2022.03.17 Add MTU Size Change -- */
}

/**
 * @brief Add BLE Characteristic
 * @param 
 * @retval None
 * @retval None
 */
static uint32_t ble_characteristic_add(
	BLE_CHAR_PARAM *char_param,
	uint8_t *dummy_data,
	uint16_t service_id,
	ble_gatts_char_handles_t *handler_id )
{
	ble_gatts_char_md_t char_md;
	ble_gatts_attr_md_t cccd_md;
	ble_gatts_attr_md_t attr_md;
	ble_gatts_attr_t attr_char_value;
	ble_uuid128_t base_uuid = {0};
	ble_uuid_t char_uuid;
	uint32_t ret;
	uint8_t type;
	
	/* UUID取得 */
	memcpy( base_uuid.uuid128, char_param->uuid, 16 );
	ret = sd_ble_uuid_vs_add( &base_uuid, &type );
	LIB_ERR_CHECK( ret, GATT_UUID_ADD_ERROR, __LINE__ );
	char_uuid.type = type;
	char_uuid.uuid = ( base_uuid.uuid128[13] << 8 ) + base_uuid.uuid128[12];

	/* CCCD設定(Client Characteristic Configuration Descriptor) */
	memset( &cccd_md, 0, sizeof( cccd_md ) );
	/* Read Permissionを設定 */
	BLE_GAP_CONN_SEC_MODE_SET_OPEN( &cccd_md.read_perm );
	BLE_GAP_CONN_SEC_MODE_SET_OPEN( &cccd_md.write_perm );
	cccd_md.vloc = BLE_GATTS_VLOC_STACK;
	
	/* */
	memset( &char_md, 0, sizeof( char_md ) );
	default_char_set( &char_md );
	char_md.char_props.read				= char_param->read_enable;
	char_md.char_props.write			= char_param->write_enable;
	char_md.char_props.write_wo_resp	= char_param->write_wo_resp;
	char_md.char_props.notify			= char_param->notify_enable;
	char_md.p_cccd_md					= &cccd_md;
	
	/* */
	memset( &attr_md, 0, sizeof( attr_md ) );
	BLE_GAP_CONN_SEC_MODE_SET_OPEN( &attr_md.read_perm );
	BLE_GAP_CONN_SEC_MODE_SET_OPEN( &attr_md.write_perm );
	default_attr_set( &attr_md );
	attr_md.vloc	= BLE_GATTS_VLOC_STACK;
	attr_md.rd_auth	= char_param->read_auth;

	/* */
	memset( &attr_char_value, 0, sizeof( attr_char_value ) );
	default_attr_char_set( &attr_char_value );
	attr_char_value.p_uuid		= &char_uuid;
	attr_char_value.p_attr_md	= &attr_md;
	attr_char_value.init_len	= char_param->init_len;
	attr_char_value.max_len		= char_param->max_len;
	attr_char_value.p_value		= dummy_data;

	ret = sd_ble_gatts_characteristic_add( service_id, &char_md, &attr_char_value, (void*)handler_id );
	return ret;
}


/**
 * @brief BLE shoes sensor service DataInfo Characteristic add
 * @param serviceId Service ID
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 Error
 */
static uint32_t data_set_char_add(uint16_t serviceId)
{
	BLE_CHAR_PARAM ble_cahr_info;
	ble_uuid128_t base_uuid = {DATA_SET_UUID};
	ret_code_t err_code;
	uint8_t dummyData[DATA_INFO_SIZE];

	/* 2020.10.01 Add characteristic 追加処理を共通化 */
	/* BLE Characteristic Information Initialize */
	memset( &ble_cahr_info, 0, sizeof( BLE_CHAR_PARAM ) );
	ble_cahr_info.read_enable		= 1;
	ble_cahr_info.write_enable		= 1;
	ble_cahr_info.write_wo_resp		= 1;
	ble_cahr_info.init_len			= DATA_INFO_SIZE;
	ble_cahr_info.max_len			= DATA_INFO_SIZE;
	memcpy( ble_cahr_info.uuid, base_uuid.uuid128, BLE_UUID_SIZE );
	
	/* Characteristic Add */
	err_code = ble_characteristic_add( &ble_cahr_info, dummyData, serviceId, (void *)&gShoes_id );
	LIB_ERR_CHECK(err_code, GATT_UUID_ADD_ERROR, __LINE__);

	return err_code;
}

/**
 * @brief BLE shoes sensor service init
 * @param None
 * @retval None
 */
static void shoes_services_init(void)
{
	ret_code_t err_code;
	nrf_ble_qwr_init_t qwr_init = {0};
	uint8_t type;
	ble_uuid128_t base_uuid = {DATA_INFO_UUID};
	ble_uuid_t ble_uuid2;
	uint32_t data_set_err;
	
	// Initialize Queued Write Module.
	qwr_init.error_handler = nrf_qwr_error_handler;

	err_code = nrf_ble_qwr_init(&m_shose_qwr, &qwr_init);
	LIB_ERR_CHECK(err_code,GATT_SHOES_QW, __LINE__);

	err_code = sd_ble_uuid_vs_add(&base_uuid, &type);
	LIB_ERR_CHECK(err_code, GATT_SHOES_UUID, __LINE__);

	ble_uuid2.type = type;
	ble_uuid2.uuid = DATA_INFO_UUID2;

	err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid2, &gSample_ble);
	LIB_ERR_CHECK(err_code, GATT_SHOES_SERVICE, __LINE__);

	data_set_err = data_set_char_add(gSample_ble);
	LIB_ERR_CHECK(data_set_err, GATT_CHAR_SET_ERROR, __LINE__);
		
	if(data_set_err == NRF_SUCCESS)
	{
		TRACE_LOG(TR_BLE_PLAYER_INOF_SERVICE_CREATE_CMPL,0);
	}
}

/**
 * @brief BLE Daily - Ultimate Service. Characteristic Set.
 * @param serviceId Service ID
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 Error
 */
static uint32_t mode_char_add(uint16_t serviceId)
{
	BLE_CHAR_PARAM ble_cahr_info;
	ble_uuid128_t base_uuid = {MODE_CHANGE_UUID};
	ble_uuid128_t trigger_uuid = {MODE_TRIGGER_UUID};
	ble_uuid128_t ult_res_uuid = {ULT_RES_UUID};
	ble_uuid128_t raw_data_uuid = {MODE_RAW_DATA};		/*raw data uuid*/
	ret_code_t err_code;
	uint8_t dummyModeChange[MODE_CHANGE_SIZE];
	uint8_t dummyModeTrriger = 0;
	uint8_t dummyUltResult[ULTIMET_RES_SIZE];

//////Mode Change
	/* 2020.10.01 Add characteristic 追加処理を共通化 */
	/* BLE Characteristic Information Initialize */
	memset( &ble_cahr_info, 0, sizeof( BLE_CHAR_PARAM ) );
	ble_cahr_info.read_enable		= 1;
	ble_cahr_info.write_enable		= 1;
	ble_cahr_info.write_wo_resp		= 1;
	ble_cahr_info.read_auth			= 1;
	ble_cahr_info.init_len			= MODE_CHANGE_SIZE;
	ble_cahr_info.max_len			= MODE_CHANGE_SIZE;
	memcpy( ble_cahr_info.uuid, base_uuid.uuid128, BLE_UUID_SIZE );

	/* Dummy Data Initialize */
	memset( dummyModeChange, 0x00, sizeof( dummyModeChange ) );
	/* Characteristic Add */
	err_code = ble_characteristic_add( &ble_cahr_info, dummyModeChange, serviceId, (void *)&gMode_change_id );
	LIB_ERR_CHECK(err_code, GATT_UUID_ADD_ERROR, __LINE__);

//////Mode Trigger
	/* 2020.10.01 Add characteristic 追加処理を共通化 */
	/* BLE Characteristic Information Initialize */
	memset( &ble_cahr_info, 0, sizeof( BLE_CHAR_PARAM ) );
	ble_cahr_info.read_enable		= 1;
	ble_cahr_info.write_enable		= 1;
	ble_cahr_info.write_wo_resp		= 1;
	ble_cahr_info.init_len			= MODE_TRIGGER_SIZE;
	ble_cahr_info.max_len			= MODE_TRIGGER_SIZE;
	memcpy( ble_cahr_info.uuid, trigger_uuid.uuid128, BLE_UUID_SIZE );

	/* Characteristic Add */
	err_code = ble_characteristic_add( &ble_cahr_info, &dummyModeTrriger, serviceId, (void *)&gMode_trigger_id );
	LIB_ERR_CHECK(err_code, GATT_UUID_ADD_ERROR, __LINE__);
	
//////Ultimete Results
	/* 2020.10.01 Add characteristic 追加処理を共通化 */
	/* BLE Characteristic Information Initialize */
	memset( &ble_cahr_info, 0, sizeof( BLE_CHAR_PARAM ) );
	ble_cahr_info.read_enable		= 1;
	ble_cahr_info.notify_enable		= 1;
	ble_cahr_info.init_len			= ULTIMET_RES_SIZE;
	ble_cahr_info.max_len			= ULTIMET_RES_SIZE;
	memcpy( ble_cahr_info.uuid, ult_res_uuid.uuid128, BLE_UUID_SIZE );

	/* Dummy Data Initialize */
	memset( dummyUltResult, 0x00, sizeof( dummyUltResult ) );
	/* Characteristic Add */
	err_code = ble_characteristic_add( &ble_cahr_info, dummyUltResult, serviceId, (void *)&gMode_result_id );
	LIB_ERR_CHECK(err_code, GATT_UUID_ADD_ERROR, __LINE__);
	
////Rawdata result
	/* 2020.10.01 Add characteristic 追加処理を共通化 */
	/* BLE Characteristic Information Initialize */
	memset( &ble_cahr_info, 0, sizeof( BLE_CHAR_PARAM ) );
	ble_cahr_info.read_enable		= 1;
	ble_cahr_info.notify_enable		= 1;
	ble_cahr_info.init_len			= RAW_DATA_SIZE;
	ble_cahr_info.max_len			= RAW_DATA_SIZE;
	memcpy( ble_cahr_info.uuid, raw_data_uuid.uuid128, BLE_UUID_SIZE );

	/* Characteristic Add */
	err_code = ble_characteristic_add( &ble_cahr_info, gDammyRaw, serviceId, (void *)&gRawData_id );
	LIB_ERR_CHECK(err_code, GATT_UUID_ADD_ERROR, __LINE__);
	
	return err_code;
}

/**
 * @brief Daily - Ultimate service init
 * @param None
 * @retval None
 */
static void mode_services_init(void)
{
	ret_code_t err_code;
	uint8_t type;
	ble_uuid128_t base_uuid = {MODE_INFO_UUID};
	ble_uuid_t ble_uuid2;
	uint32_t data_set_err;

	/////
	err_code = sd_ble_uuid_vs_add(&base_uuid, &type);
	LIB_ERR_CHECK(err_code, GATT_MODE_UUID, __LINE__);

	ble_uuid2.type = type;
	ble_uuid2.uuid = MODE_INFO_UUID2;

	err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid2, &gMode_ble);
	LIB_ERR_CHECK(err_code, GATT_MODE_SERVICE, __LINE__);
	data_set_err =  mode_char_add(gMode_ble);
	LIB_ERR_CHECK(data_set_err, GATT_CHAR_SET_ERROR, __LINE__);
	
	if(data_set_err == NRF_SUCCESS)
	{
		TRACE_LOG(TR_BLE_MODE_SERVICE_CREATE_CMPL,0);
	}
}

/**
 * @brief BLE Daily Log Service. Characteristic Set.
 * @param serviceId Service ID
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 Error
 */
static uint32_t daily_char_add(uint16_t serviceId)
{
	BLE_CHAR_PARAM ble_cahr_info;
	ret_code_t err_code;
	ble_uuid128_t daily_id_uuid = {DAILY_ID_SET_UUID};
	ble_uuid128_t daily_log_uuid = {DAILY_LOG_UUID};
	uint8_t dummyDailyID[DAILY_ID_SIZE];
	uint8_t dummyDailyLog[DAILY_LOG_SIZE];
	
//////Daily ID set
	/* 2020.10.01 Add characteristic 追加処理を共通化 */
	/* BLE Characteristic Information Initialize */
	memset( &ble_cahr_info, 0, sizeof( BLE_CHAR_PARAM ) );
	ble_cahr_info.read_enable		= 1;
	ble_cahr_info.write_enable		= 1;
	ble_cahr_info.write_wo_resp		= 1;
	ble_cahr_info.init_len			= DAILY_ID_SIZE;
	ble_cahr_info.max_len			= DAILY_ID_SIZE;
	memcpy( ble_cahr_info.uuid, daily_id_uuid.uuid128, BLE_UUID_SIZE );

	/* Dummy Data Initialize */
	memset( dummyDailyID, 0x00, sizeof( dummyDailyID ) );
	/* Characteristic Add */
	err_code = ble_characteristic_add( &ble_cahr_info, dummyDailyID, serviceId, (void *)&gDaily_id_set_id );
	LIB_ERR_CHECK(err_code, GATT_UUID_ADD_ERROR, __LINE__);

//////Daily Log
	/* 2020.10.01 Add characteristic 追加処理を共通化 */
	/* BLE Characteristic Information Initialize */
	memset( &ble_cahr_info, 0, sizeof( BLE_CHAR_PARAM ) );
	ble_cahr_info.read_enable		= 1;
	ble_cahr_info.notify_enable		= 1;
	ble_cahr_info.init_len			= DAILY_LOG_SIZE;
	ble_cahr_info.max_len			= DAILY_LOG_SIZE;
	memcpy( ble_cahr_info.uuid, daily_log_uuid.uuid128, BLE_UUID_SIZE );

	/* Dummy Data Initialize */
	memset( dummyDailyLog, 0x00, sizeof( dummyDailyLog ) );
	/* Characteristic Add */
	err_code = ble_characteristic_add( &ble_cahr_info, dummyDailyLog, serviceId, (void *)&gDaily_log_id );
	LIB_ERR_CHECK(err_code, GATT_UUID_ADD_ERROR, __LINE__);

	return err_code;
}

/**
 * @brief BLE Daily Log service init
 * @param None
 * @retval None
 */
static void daily_services_init(void)
{
	ret_code_t err_code;
	uint32_t data_set_err;
	uint8_t  type;
	ble_uuid128_t base_uuid = {DAILY_MODE_UUID};
	ble_uuid_t ble_uuid2;

	err_code = sd_ble_uuid_vs_add(&base_uuid, &type);
	LIB_ERR_CHECK(err_code, GATT_DAILY_UUID, __LINE__);

	ble_uuid2.type = type;
	ble_uuid2.uuid = DAILY_MODE_UUID2;

	err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid2, &gDaily_ble);
	LIB_ERR_CHECK(err_code, GATT_DAILY_SERVICE, __LINE__);

	/* Daily Log Characteristic 追加 */
	data_set_err = daily_char_add(gDaily_ble);
	LIB_ERR_CHECK(data_set_err, GATT_CHAR_SET_ERROR, __LINE__);
	if(data_set_err == NRF_SUCCESS)
	{
		TRACE_LOG(TR_BLE_DAILY_SERVICE_CREATE_CMPL,0);
	}
}

/**
 * @brief BLE Other response service. Characteristic Set
 * @param serviceId Service ID
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 Error
 */
static uint32_t respose_char_add(uint16_t serviceId)
{
	BLE_CHAR_PARAM ble_cahr_info;
	ret_code_t err_code;
	ble_uuid128_t fw_res_uuid = {FW_RES_UUID};
	ble_uuid128_t fw_ver_uuid = {FW_VER_UUID};
	ble_uuid128_t read_err_uuid = {READ_ERR_UUID};
	ble_uuid128_t get_time_uuid = {GET_TIME_UUID};
	uint8_t dummyRes[FW_RES_SIZE];
	uint8_t dummyFW[FW_VERSION_SIZE];
	uint8_t dummyReadError[READ_ERR_SIZE];
	uint8_t dummyGetTime[GET_TIME_SIZE];

//////FW Response
	/* 2020.10.01 Add characteristic 追加処理を共通化 */
	/* BLE Characteristic Information Initialize */
	memset( &ble_cahr_info, 0, sizeof( BLE_CHAR_PARAM ) );
	ble_cahr_info.read_enable		= 1;
	ble_cahr_info.notify_enable		= 1;
	ble_cahr_info.init_len			= FW_RES_SIZE;
	ble_cahr_info.max_len			= FW_RES_SIZE;
	memcpy( ble_cahr_info.uuid, fw_res_uuid.uuid128, BLE_UUID_SIZE );

	/* Dummy Data Initialize */
	memset( dummyRes, 0x00, sizeof( dummyRes ) );
	/* Characteristic Add */
	err_code = ble_characteristic_add( &ble_cahr_info, dummyRes, serviceId, (void *)&gFw_res_id );
	LIB_ERR_CHECK(err_code, GATT_UUID_ADD_ERROR, __LINE__);

//////FW Version
	/* 2020.10.01 Add characteristic 追加処理を共通化 */
	/* BLE Characteristic Information Initialize */
	memset( &ble_cahr_info, 0, sizeof( BLE_CHAR_PARAM ) );
	ble_cahr_info.read_enable		= 1;
	ble_cahr_info.write_enable		= 1;
	ble_cahr_info.read_auth			= 1;
	ble_cahr_info.init_len			= FW_VERSION_SIZE;
	ble_cahr_info.max_len			= FW_VERSION_SIZE;
	memcpy( ble_cahr_info.uuid, fw_ver_uuid.uuid128, BLE_UUID_SIZE );

	/* Dummy Data Initialize */
	memset( dummyFW, 0x00, sizeof( dummyFW ) );
	/* Characteristic Add */
	err_code = ble_characteristic_add( &ble_cahr_info, dummyFW, serviceId, (void *)&gFw_ver_id );
	LIB_ERR_CHECK(err_code, GATT_UUID_ADD_ERROR, __LINE__);
	
//////Read Error
	/* 2020.10.01 Add characteristic 追加処理を共通化 */
	/* BLE Characteristic Information Initialize */
	memset( &ble_cahr_info, 0, sizeof( BLE_CHAR_PARAM ) );
	ble_cahr_info.read_enable		= 1;
	ble_cahr_info.write_enable		= 1;
	ble_cahr_info.write_wo_resp		= 1;
	ble_cahr_info.read_auth			= 1;
	ble_cahr_info.init_len			= READ_ERR_SIZE;
	ble_cahr_info.max_len			= READ_ERR_SIZE;
	memcpy( ble_cahr_info.uuid, read_err_uuid.uuid128, BLE_UUID_SIZE );

	/* Dummy Data Initialize */
	memset( dummyReadError, 0x00, sizeof( dummyReadError ) );
	/* Characteristic Add */
	err_code = ble_characteristic_add( &ble_cahr_info, dummyReadError, serviceId, (void *)&gRead_err_id );
	LIB_ERR_CHECK(err_code, GATT_UUID_ADD_ERROR, __LINE__);
	
//////Get Time
	/* 2020.10.01 Add characteristic 追加処理を共通化 */
	/* BLE Characteristic Information Initialize */
	memset( &ble_cahr_info, 0, sizeof( BLE_CHAR_PARAM ) );
	ble_cahr_info.read_enable		= 1;
	ble_cahr_info.read_auth			= 1;
	ble_cahr_info.init_len			= GET_TIME_SIZE;
	ble_cahr_info.max_len			= GET_TIME_SIZE;
	memcpy( ble_cahr_info.uuid, get_time_uuid.uuid128, BLE_UUID_SIZE );

	/* Dummy Data Initialize */
	memset( dummyGetTime, 0x00, sizeof( dummyGetTime ) );
	/* Characteristic Add */
	err_code = ble_characteristic_add( &ble_cahr_info, dummyGetTime, serviceId, (void *)&gGet_time_id );
	LIB_ERR_CHECK(err_code, GATT_UUID_ADD_ERROR, __LINE__);
	
	return err_code;
}

/**
 * @brief BLE Other response service init
 * @param None
 * @retval None
 */
static void response_services_init(void)
{
	ret_code_t err_code;
	uint32_t data_set_err;
	uint8_t type;
	ble_uuid128_t base_uuid = {RESPONSE_UUID};
	ble_uuid_t ble_uuid2;

	err_code = sd_ble_uuid_vs_add(&base_uuid, &type);
	LIB_ERR_CHECK(err_code, GATT_RES_UUID, __LINE__);

	ble_uuid2.type = type;
	ble_uuid2.uuid = RESPONSE_UUID2;

	err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid2, &gRespose_ble);
	LIB_ERR_CHECK(err_code, GATT_RES_SERVICE, __LINE__);

	data_set_err = respose_char_add(gRespose_ble);
	LIB_ERR_CHECK(data_set_err, GATT_CHAR_SET_ERROR, __LINE__);

	if(data_set_err == NRF_SUCCESS)
	{
		TRACE_LOG(TR_BLE_RESPONSE_SERVICE_CREATE_CMPL,0);
	}
}

/**
 * @brief BLE Battery Service. Characteristic Set
 * @param serviceId Service ID
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 Error
 */
static uint32_t batt_char_add(uint16_t serviceId)
{
	BLE_CHAR_PARAM ble_cahr_info;
	ret_code_t err_code;
	ble_uuid128_t batt_uuid = {BATTERY_VOL_UUID};
	uint8_t dummyBatData[BATT_LEVEL_SIZE];
	
//////Battery Voltage
	/* 2020.10.01 Add characteristic 追加処理を共通化 */
	/* BLE Characteristic Information Initialize */
	memset( &ble_cahr_info, 0, sizeof( BLE_CHAR_PARAM ) );
	ble_cahr_info.read_enable		= 1;
	ble_cahr_info.read_auth			= 1;
	ble_cahr_info.init_len			= BATT_LEVEL_SIZE;
	ble_cahr_info.max_len			= BATT_LEVEL_SIZE;
	memcpy( ble_cahr_info.uuid, batt_uuid.uuid128, BLE_UUID_SIZE );

	/* Dummy Data Initialize */
	memset( dummyBatData, 0x00, sizeof( dummyBatData ) );
	/* Characteristic Add */
	err_code = ble_characteristic_add( &ble_cahr_info, dummyBatData, serviceId, (void *)&gBattery_mVol_id );
	LIB_ERR_CHECK(err_code, GATT_UUID_ADD_ERROR, __LINE__);

	return err_code;
}

/**
 * @brief BLE Battery service init
 * @param None
 * @retval None
 */
static void batt_services_init(void)
{
	ret_code_t err_code;
	uint32_t data_set_err;
	uint8_t type;
	ble_uuid128_t base_uuid = {BATTERY_UUID};
	ble_uuid_t ble_uuid2;

	/////
	err_code = sd_ble_uuid_vs_add(&base_uuid, &type);
	LIB_ERR_CHECK(err_code, GATT_BAT_SERVICE, __LINE__);
		
	ble_uuid2.type = type;
	ble_uuid2.uuid = BATTERY_UUID2;

	err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid2, &gBatt_ble);
	LIB_ERR_CHECK(err_code, GATT_BAT_UUID, __LINE__);

	data_set_err = batt_char_add(gBatt_ble);
	LIB_ERR_CHECK(data_set_err, GATT_CHAR_SET_ERROR, __LINE__);
	
	if(data_set_err == NRF_SUCCESS)
	{
		TRACE_LOG(TR_BLE_BATTERY_SERVICE_CREATE_CMPL,0);
	}
}

/**
 * @brief BLE Pairing Service. Characteristic Set.
 * @param serviceId Service ID
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 Error
 */
static uint32_t pairing_char_add(uint16_t serviceId)
{
	BLE_CHAR_PARAM ble_cahr_info;
	ret_code_t err_code;
	ble_uuid128_t pairing_code_uuid = {PAIR_CODE_UUID};
	ble_uuid128_t pairing_uuid = {PAIR_MATCH_UUID};
	ble_uuid128_t paritg_clear_uuid = {PAIR_CLEAR_UUID};
	uint8_t dummyValue[PAIRING_CODE_SIZE];
	uint8_t dummyValue_pinclear[PAIRING_CLEAR_SIZE];
	uint8_t  dummyValue_pin_match[PAIRING_MATCH_SIZE];

//////Pairing Code
	/* 2020.10.01 Add characteristic 追加処理を共通化 */
	/* BLE Characteristic Information Initialize */
	memset( &ble_cahr_info, 0, sizeof( BLE_CHAR_PARAM ) );
	ble_cahr_info.read_enable		= 1;
	ble_cahr_info.read_auth			= 1;
	ble_cahr_info.write_enable		= 1;
	ble_cahr_info.write_wo_resp		= 1;
	ble_cahr_info.init_len			= PAIRING_CODE_SIZE;
	ble_cahr_info.max_len			= PAIRING_CODE_SIZE;
	memcpy( ble_cahr_info.uuid, pairing_code_uuid.uuid128, BLE_UUID_SIZE );

	/* Dummy Data Initialize */
	memset( dummyValue, 0x00, sizeof( dummyValue ) );
	/* Characteristic Add */
	err_code = ble_characteristic_add( &ble_cahr_info, dummyValue, serviceId, (void *)&gPairing_code_id );
	LIB_ERR_CHECK(err_code, GATT_UUID_ADD_ERROR, __LINE__);

//////Pairing Matching
	/* 2020.10.01 Add characteristic 追加処理を共通化 */
	/* BLE Characteristic Information Initialize */
	memset( &ble_cahr_info, 0, sizeof( BLE_CHAR_PARAM ) );
	ble_cahr_info.read_enable		= 1;
	ble_cahr_info.notify_enable		= 1;
	ble_cahr_info.init_len			= PAIRING_MATCH_SIZE;
	ble_cahr_info.max_len			= PAIRING_MATCH_SIZE;
	memcpy( ble_cahr_info.uuid, pairing_uuid.uuid128, BLE_UUID_SIZE );

	/* Dummy Data Initialize */
	memset( dummyValue_pin_match, 0x00, sizeof( dummyValue_pin_match ) );
	/* Characteristic Add */
	err_code = ble_characteristic_add( &ble_cahr_info, dummyValue_pin_match, serviceId, (void *)&gPairing_match_id );
	LIB_ERR_CHECK(err_code, GATT_UUID_ADD_ERROR, __LINE__);

//////Pairing Clear
	/* 2020.10.01 Add characteristic 追加処理を共通化 */
	/* BLE Characteristic Information Initialize */
	memset( &ble_cahr_info, 0, sizeof( BLE_CHAR_PARAM ) );
	ble_cahr_info.read_enable		= 1;
	ble_cahr_info.read_auth			= 1;
	ble_cahr_info.write_enable		= 1;
	ble_cahr_info.write_wo_resp		= 1;
	ble_cahr_info.init_len			= PAIRING_CLEAR_SIZE;
	ble_cahr_info.max_len			= PAIRING_CLEAR_SIZE;
	memcpy( ble_cahr_info.uuid, paritg_clear_uuid.uuid128, BLE_UUID_SIZE );

	/* Dummy Data Initialize */
	memset( dummyValue_pinclear, 0x00, sizeof( dummyValue_pinclear ) );
	/* Characteristic Add */
	err_code = ble_characteristic_add( &ble_cahr_info, dummyValue_pinclear, serviceId, (void *)&gPairing_clear_id );
	LIB_ERR_CHECK(err_code, GATT_UUID_ADD_ERROR, __LINE__);

	return err_code;
}

/**
 * @brief BLE Pairing service init
 * @param None
 * @retval None
 */
static void pairing_services_init(void)
{
	ret_code_t err_code;
	uint32_t data_set_err;
	uint8_t type;
	ble_uuid128_t base_uuid = {PAIR_INFO_UUID};
	ble_uuid_t ble_uuid2;

	/////
	err_code = sd_ble_uuid_vs_add(&base_uuid, &type);
	LIB_ERR_CHECK(err_code,GATT_PAIR_UUID, __LINE__);

	ble_uuid2.type = type;
	ble_uuid2.uuid = PAIR_INFO_UUID2;

	err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid2, &gPairing_ble);
	LIB_ERR_CHECK(err_code,GATT_PAIR_SERVICE, __LINE__);

	data_set_err = pairing_char_add(gPairing_ble);
	LIB_ERR_CHECK(data_set_err, GATT_CHAR_SET_ERROR, __LINE__);

	if(data_set_err == NRF_SUCCESS)
	{
		TRACE_LOG(TR_BLE_PAIRING_CODE_SERVICE_CREATE_CMPL,0);
	}
}

/**
 * @brief BLE Pairing Service. Characteristic Set.
 * @param serviceId Service ID
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 Error
 */
static uint32_t calib_char_add(uint16_t serviceId)
{
	BLE_CHAR_PARAM ble_cahr_info;
	ret_code_t err_code;
	ble_uuid128_t calib_uuid = {CALIB_VAL_UUID};
	uint8_t dummyCalibData[CALIB_SIZE];
	
//////calib service
	/* BLE Characteristic Information Initialize */
	memset( &ble_cahr_info, 0, sizeof( BLE_CHAR_PARAM ) );
	ble_cahr_info.read_enable		= 1;
	ble_cahr_info.read_auth			= 1;
	ble_cahr_info.notify_enable		= 1;
	ble_cahr_info.init_len			= CALIB_SIZE;
	ble_cahr_info.max_len			= CALIB_SIZE;
	memcpy( ble_cahr_info.uuid, calib_uuid.uuid128, BLE_UUID_SIZE );

	/* Dummy Data Initialize */
	memset( dummyCalibData, 0x00, sizeof( dummyCalibData ) );
	/* Characteristic Add */
	err_code = ble_characteristic_add( &ble_cahr_info, dummyCalibData, serviceId, (void *)&gCalib_id );
	LIB_ERR_CHECK(err_code, GATT_UUID_ADD_ERROR, __LINE__);
	
	return err_code;
}

/**
 * @brief BLE calibration service init
 * @param None
 * @retval None
 */
static void calib_services_init(void)
{
	ret_code_t err_code;
	uint32_t data_set_err;
	uint8_t  type;
	ble_uuid128_t base_uuid = {CALIB_UUID};
	ble_uuid_t ble_uuid2;

	/////
	err_code = sd_ble_uuid_vs_add(&base_uuid, &type);
	LIB_ERR_CHECK(err_code, GATT_BAT_SERVICE, __LINE__);

	ble_uuid2.type = type;
	ble_uuid2.uuid = CALIB_UUID2;

	err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid2, &gCalib_ble);
	LIB_ERR_CHECK(err_code, GATT_BAT_UUID, __LINE__);

	data_set_err = calib_char_add(gCalib_ble);
	LIB_ERR_CHECK(data_set_err, GATT_CHAR_SET_ERROR, __LINE__);

	if(data_set_err == NRF_SUCCESS)
	{
		TRACE_LOG(TR_BLE_BATTERY_SERVICE_CREATE_CMPL,0);
	}
}

/**
 * @brief BLE Pairing Service. Characteristic Set.
 * @param serviceId Service ID
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 Error
 */
static uint32_t test_command_char_add(uint16_t serviceId)
{
	BLE_CHAR_PARAM ble_cahr_info;
	ret_code_t err_code;
	ble_uuid128_t calib_uuid = {TEST_CMD_UUID};
	uint8_t dummy_test_cmd[TEST_CMD_SIZE];
	
//////calib service
	/* BLE Characteristic Information Initialize */
	memset( &ble_cahr_info, 0, sizeof( BLE_CHAR_PARAM ) );
	ble_cahr_info.write_enable		= 1;
	ble_cahr_info.write_wo_resp		= 1;
	ble_cahr_info.init_len			= TEST_CMD_SIZE;
	ble_cahr_info.max_len			= TEST_CMD_SIZE;
	memcpy( ble_cahr_info.uuid, calib_uuid.uuid128, BLE_UUID_SIZE );

	/* Dummy Data Initialize */
	memset( dummy_test_cmd, 0x00, sizeof( dummy_test_cmd ) );
	/* Characteristic Add */
	err_code = ble_characteristic_add( &ble_cahr_info, dummy_test_cmd, serviceId, (void *)&g_test_cmd_id );
	LIB_ERR_CHECK(err_code, GATT_UUID_ADD_ERROR, __LINE__);
	
	return err_code;
}

/**
 * @brief BLE calibration service init
 * @param None
 * @retval None
 */
static void test_command_services_init(void)
{
	ret_code_t err_code;
	uint32_t data_set_err;
	uint8_t  type;
	ble_uuid128_t base_uuid = {TEST_CMD_SERVICE_UUID};
	ble_uuid_t ble_uuid2;

	/////
	err_code = sd_ble_uuid_vs_add( &base_uuid, &type );
	LIB_ERR_CHECK(err_code, GATT_BAT_SERVICE, __LINE__);

	ble_uuid2.type = type;
	ble_uuid2.uuid = ( base_uuid.uuid128[13] << 8 ) + base_uuid.uuid128[12];

	err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid2, &g_test_command_ble);
	LIB_ERR_CHECK(err_code, GATT_BAT_UUID, __LINE__);

	data_set_err = test_command_char_add( g_test_command_ble );
	LIB_ERR_CHECK(data_set_err, GATT_CHAR_SET_ERROR, __LINE__);

	if(data_set_err == NRF_SUCCESS)
	{
		TRACE_LOG(TR_BLE_BATTERY_SERVICE_CREATE_CMPL,0);
	}
}

/**
 * @brief BLE Angle Adjustment Service. Characteristic Set.
 * @param serviceId Service ID
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 Error
 */
static uint32_t angle_adjust_char_add(uint16_t serviceId)
{
	BLE_CHAR_PARAM ble_cahr_info;
	ret_code_t err_code;
	ble_uuid128_t angle_uuid = {ANGLE_UUID};
	uint8_t dummy_angle_cmd[ANGLE_ADJUST_SIZE];
	
//////calib service
	/* BLE Characteristic Information Initialize */
	memset( &ble_cahr_info, 0, sizeof( BLE_CHAR_PARAM ) );
	ble_cahr_info.write_enable		= 1;
	ble_cahr_info.write_wo_resp		= 1;
	ble_cahr_info.read_enable		= 1;
	ble_cahr_info.notify_enable		= 1;
	ble_cahr_info.init_len			= ANGLE_ADJUST_SIZE;
	ble_cahr_info.max_len			= ANGLE_ADJUST_SIZE;
	memcpy( ble_cahr_info.uuid, angle_uuid.uuid128, BLE_UUID_SIZE );

	/* Dummy Data Initialize */
	memset( dummy_angle_cmd, 0x00, sizeof( dummy_angle_cmd ) );
	/* Characteristic Add */
	err_code = ble_characteristic_add( &ble_cahr_info, dummy_angle_cmd, serviceId, (void *)&g_angle_adjust_id );
	LIB_ERR_CHECK(err_code, GATT_UUID_ADD_ERROR, __LINE__);
	
	return err_code;
}

/**
 * @brief BLE calibration service init
 * @param None
 * @retval None
 */
static void angle_adjust_services_init(void)
{
	ret_code_t err_code;
	uint32_t data_set_err;
	uint8_t  type;
	ble_uuid128_t base_uuid = {ANGLE_ADJUSTMENT_SERVICE_UUID};
	ble_uuid_t ble_uuid2;

	/////
	err_code = sd_ble_uuid_vs_add( &base_uuid, &type );
	LIB_ERR_CHECK(err_code, GATT_BAT_SERVICE, __LINE__);

	ble_uuid2.type = type;
	ble_uuid2.uuid = ( base_uuid.uuid128[13] << 8 ) + base_uuid.uuid128[12];

	err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid2, &g_angle_adjust_ble);
	LIB_ERR_CHECK(err_code, GATT_BAT_UUID, __LINE__);

	data_set_err = angle_adjust_char_add( g_angle_adjust_ble );
	LIB_ERR_CHECK(data_set_err, GATT_CHAR_SET_ERROR, __LINE__);

	if(data_set_err == NRF_SUCCESS)
	{
		TRACE_LOG(TR_BLE_BATTERY_SERVICE_CREATE_CMPL,0);
	}
}

/* 2020.12.23 Add RSSI取得テスト ++ */
/**
 * @brief BLE RSSI Notify Service. Characteristic Set.
 * @param serviceId Service ID
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 Error
 */
static uint32_t rssi_notify_char_add( uint16_t serviceId )
{
	BLE_CHAR_PARAM ble_cahr_info;
	ret_code_t err_code;
	ble_uuid128_t get_rssi_uuid = {RSSI_NOTIFY_UUID};
	uint8_t dummyValue[RSSI_VALUE_SIZE];

	/* BLE Characteristic Information Initialize */
	memset( &ble_cahr_info, 0, sizeof( BLE_CHAR_PARAM ) );
	ble_cahr_info.read_enable		= 1;
	ble_cahr_info.notify_enable		= 1;
	ble_cahr_info.init_len			= RSSI_VALUE_SIZE;
	ble_cahr_info.max_len			= RSSI_VALUE_SIZE;
	memcpy( ble_cahr_info.uuid, get_rssi_uuid.uuid128, BLE_UUID_SIZE );

	/* Dummy Data Initialize */
	memset( dummyValue, 0x00, sizeof( dummyValue ) );
	/* Characteristic Add */
	err_code = ble_characteristic_add( &ble_cahr_info, dummyValue, serviceId, (void *)&g_rssi_notify_id );
	LIB_ERR_CHECK( err_code, GATT_UUID_ADD_ERROR, __LINE__ );

	return err_code;
}

/**
 * @brief BLE RSSI Notify service init
 * @param None
 * @retval None
 */
static void rssi_notify_services_init( void )
{
	ret_code_t err_code;
	uint32_t data_set_err;
	uint8_t type;
	ble_uuid128_t base_uuid = {RSSI_NOTIFY_SERVICE_UUID};
	ble_uuid_t ble_uuid2;

	err_code = sd_ble_uuid_vs_add( &base_uuid, &type );
	LIB_ERR_CHECK( err_code, GATT_PAIR_UUID, __LINE__ );

	ble_uuid2.type = type;
	ble_uuid2.uuid = ( base_uuid.uuid128[13] << 8 ) + base_uuid.uuid128[12];

	err_code = sd_ble_gatts_service_add( BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid2, &g_rssi_notify_ble );
	LIB_ERR_CHECK( err_code, GATT_PAIR_SERVICE, __LINE__ );

	data_set_err = rssi_notify_char_add( g_rssi_notify_ble );
	LIB_ERR_CHECK(data_set_err, GATT_CHAR_SET_ERROR, __LINE__);

	if(data_set_err == NRF_SUCCESS)
	{
		TRACE_LOG( TR_BLE_RSSI_NOTIFY_COMP, 0 );
	}
}
/* 2020.12.23 Add RSSI取得テスト -- */

/* 2022.06.09 Add 分解能のCharacteristic追加 ++ */
/**
 * @brief BLE Angle Resolution Service. Characteristic Set.
 * @param serviceId Service ID
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 Error
 */
static uint32_t resolution_char_add(uint16_t serviceId)
{
	BLE_CHAR_PARAM ble_cahr_info;
	ret_code_t err_code;
	ble_uuid128_t resolution_uuid = {RESOLUTION_UUID};
	uint8_t dummy_resolution_cmd[RESOLUTION_SIZE];
	
//////calib service
	/* BLE Characteristic Information Initialize */
	memset( &ble_cahr_info, 0, sizeof( BLE_CHAR_PARAM ) );
	ble_cahr_info.read_enable		= 1;
	ble_cahr_info.notify_enable		= 1;
	ble_cahr_info.init_len			= RESOLUTION_SIZE;
	ble_cahr_info.max_len			= RESOLUTION_SIZE;
	memcpy( ble_cahr_info.uuid, resolution_uuid.uuid128, BLE_UUID_SIZE );

	/* Dummy Data Initialize */
	memset( dummy_resolution_cmd, 0x00, sizeof( dummy_resolution_cmd ) );
	/* Characteristic Add */
	err_code = ble_characteristic_add( &ble_cahr_info, dummy_resolution_cmd, serviceId, (void *)&g_resolution_id );
	LIB_ERR_CHECK(err_code, GATT_UUID_ADD_ERROR, __LINE__);
	
	return err_code;
}

/**
 * @brief BLE Resolution service init
 * @param None
 * @retval None
 */
static void resolution_services_init(void)
{
	ret_code_t err_code;
	uint32_t data_set_err;
	uint8_t  type;
	ble_uuid128_t base_uuid = {RESOLUTION_SERVICE_UUID};
	ble_uuid_t ble_uuid2;

	/////
	err_code = sd_ble_uuid_vs_add( &base_uuid, &type );
	LIB_ERR_CHECK(err_code, GATT_BAT_SERVICE, __LINE__);

	ble_uuid2.type = type;
	ble_uuid2.uuid = ( base_uuid.uuid128[13] << 8 ) + base_uuid.uuid128[12];

	err_code = sd_ble_gatts_service_add( BLE_GATTS_SRVC_TYPE_PRIMARY, &ble_uuid2, &g_resolution_ble );
	LIB_ERR_CHECK(err_code, GATT_BAT_UUID, __LINE__);

	data_set_err = resolution_char_add( g_resolution_ble );
	LIB_ERR_CHECK(data_set_err, GATT_CHAR_SET_ERROR, __LINE__);

	if(data_set_err == NRF_SUCCESS)
	{
		TRACE_LOG(TR_BLE_BATTERY_SERVICE_CREATE_CMPL,0);
	}
}
/* 2022.06.09 Add 分解能のCharacteristic追加 -- */

/**
 * @brief BLE GATT Setting
 * @param pValue_handle characteristic handle
 * @param gattId characteristic handle id
 * @retval None
 */
void GetGattsCharHandleValueID(uint16_t *pValue_handle, GATTS_CHAR_HANDLE_ID gattId)
{
	ble_gatts_char_handles_t *pGattCharHandle;
	
	pGattCharHandle = g_gatts_char_handle_table[gattId];
	*pValue_handle = pGattCharHandle->value_handle;
}

/**
 * @brief BLE GATT Setting
 * @param None
 * @retval None
 */
void BleGattsSet(void)
{
	gatt_init();
	shoes_services_init();
	mode_services_init();
	daily_services_init();
	response_services_init();
	batt_services_init();
	pairing_services_init();
	
	//calib service
	calib_services_init();
	/* 2020.10.21 Add Beacon Info Service */
	//beacon_service_init();
	/* 2022.03.22 Add SystemStandy追加 ++ */
	test_command_services_init();
	/* 2022.03.22 Add SystemStandy追加 -- */
	/* 2022.05.16 Add 角度調整追加 ++ */
	angle_adjust_services_init();
	/* 2022.05.16 Add 角度調整追加 -- */
	/* 2020.12.23 Add RSSI取得テスト ++ */
	rssi_notify_services_init();
	/* 2020.12.23 Add RSSI取得テスト -- */
	/* 2022.06.09 Add 分解能読み出し追加 ++ */
	resolution_services_init();
	/* 2022.06.09 Add 分解能読み出し追加 -- */
}

/**
 * @brief BLE Write command BLE event. init, mode change, ult trigger.
 * @param handle_id handle id
 * @param len 受信したデータ長
 * @param p_data 受信データ
 * @retval None
 */
void SelectEvtWrite(uint16_t handle_id, uint16_t len, uint8_t *p_data)
{
	ble_gatts_value_t value_param;
	EVT_ST event;
	uint32_t err_fifo;
	uint8_t notify_signal;
	OP_MODE currOP_Id = DAILY_MODE;
	/* 2022.03.22 Add Test Command ++ */
	uint16_t cnt_handle = BLE_CONN_HANDLE_INVALID;
	/* 2022.03.22 Add Test Command -- */
	
	memset(&value_param, 0x00, sizeof(value_param));
	memset(&event, 0x00, sizeof(event));
	
	//init trigger
	if(gShoes_id.value_handle == handle_id)
	{
		memcpy(&event.DATA.dataInfo.mode, p_data, len);
		if(event.DATA.dataInfo.mode == BLE_CMD_ACCEPT_PINCODE_CHECK_MODE)
		{
			event.evt_id = EVT_BLE_CMD_ACCEPT_PINCODE_CHECK;
			err_fifo = PushFifo(&event);
			DEBUG_EVT_FIFO_LOG(err_fifo,event.evt_id);
		}
		else if(event.DATA.dataInfo.mode == BLE_CMD_PLAYER_INFO_SET_MODE)
		{
			event.evt_id = EVT_BLE_CMD_PLAYER_INFO_SET;
			DEBUG_LOG(LOG_DEBUG,"play info event id 0x%x", event.evt_id);
			event.DATA.dataInfo.age    = p_data[1];
			event.DATA.dataInfo.sex    = p_data[2];
			event.DATA.dataInfo.weight = p_data[3];

			err_fifo = PushFifo(&event);
			DEBUG_EVT_FIFO_LOG(err_fifo,event.evt_id);
			
			DEBUG_LOG(LOG_DEBUG,"evt  age %u, sex %u, weight %u", event.DATA.dataInfo.age,event.DATA.dataInfo.sex,event.DATA.dataInfo.weight);
			DEBUG_LOG(LOG_DEBUG,"play info age %u, sex %u, weight %u", p_data[1],p_data[2],p_data[3]);
		}
		else if(event.DATA.dataInfo.mode == BLE_CMD_TIME_INFO_SET_MODE)
		{
			event.evt_id = EVT_BLE_CMD_TIME_INFO_SET;
			err_fifo = PushFifo(&event);
			DEBUG_EVT_FIFO_LOG(err_fifo,event.evt_id);
		}
		else if(event.DATA.dataInfo.mode == BLE_CMD_INIT_MODE)
		{
			if(2 <= event.DATA.dataInfo.flash_flg)
			{
				notify_signal = BLE_NOTIFY_SIG;
				BleNotifySignals(&notify_signal,(uint16_t)sizeof(notify_signal));
			}
			else
			{
				event.evt_id = EVT_BLE_CMD_INIT;
				err_fifo = PushFifo(&event);
				DEBUG_EVT_FIFO_LOG(err_fifo,event.evt_id);
			}
		}
		/* 2022.05.12 Add ゲストモード追加 ++ */
		else if ( event.DATA.dataInfo.mode == BLE_CMD_GUEST_MODE )
		{
			/* Guest Mode Event */
			event.evt_id = EVT_BLE_CMD_GUEST_MODE;
			err_fifo = PushFifo( &event );
			DEBUG_EVT_FIFO_LOG(err_fifo,event.evt_id);
		}
		/* 2022.05.12 Add ゲストモード追加 ++ */
		else
		{
			//nothing to do.
		}
		DEBUG_LOG(LOG_DEBUG,"evt write hadle init mode 0x%x, flash flag 0x%x",event.DATA.dataInfo.mode,event.DATA.dataInfo.flash_flg);
		DEBUG_LOG(LOG_DEBUG,"event id 0x%x", event.evt_id);
	}
	//mode change trigger
	else if (gMode_change_id.value_handle == handle_id)
	{
		event.evt_id = EVT_BLE_CMD_MODE_CHANGE;
		//memcpy(&event.DATA.modeSet.mode,p_data,len);
		/* 2020.11.20 Modify モード設定時のTimeou値取得に誤りがあったため修正 ++ */
		memcpy( &event.DATA.modeSet.mode, &p_data[0], sizeof( uint8_t ) );
		memcpy( &event.DATA.modeSet.mode_timeout, &p_data[1], sizeof( uint16_t ) );
		memcpy( &event.DATA.modeSet.clear_sid, &p_data[3], sizeof( uint16_t ) );
		/* 2020.11.20 Modify モード設定時のTimeou値取得に誤りがあったため修正 -- */
		err_fifo = PushFifo(&event);
		DEBUG_EVT_FIFO_LOG(err_fifo,event.evt_id);
	}
	else if (gMode_trigger_id.value_handle == handle_id)
	{
		memcpy(&event.DATA.modeTrigger.trigger,p_data,len);
		if(event.DATA.modeTrigger.trigger == 0)
		{
			event.evt_id = EVT_BLE_CMD_ULT_START_TRIG;
			GetOpMode(&currOP_Id);
			//if((currOP_Id != DAILY_MODE) && (currOP_Id != RES_ONE) && (currOP_Id != RES_TWO) && (currOP_Id != RES_THREE))
			/*raw mode add*/
			if((currOP_Id != DAILY_MODE) && (currOP_Id != RES_ONE) && (currOP_Id != RES_TWO) && (currOP_Id != RES_THREE) /*&& (currOP_Id != RAW_MODE)*/)
			{
				StopUltStartTimer();
				StartUltEndTimer();
			}
		}
		else
		{
			event.evt_id = EVT_BLE_CMD_ULT_TRIG;
			//FW 9.00 version debug
			if(event.DATA.modeTrigger.trigger == ULT_END_TRIGGER)
			{
				/*raw mode*/
				DEBUG_LOG(LOG_DEBUG,"ble write event. recieve END Trigger");
			}
		}
		err_fifo = PushFifo(&event);
		DEBUG_EVT_FIFO_LOG(err_fifo,event.evt_id);
	}
	//daily log. read mode and position write.
	else if (gDaily_id_set_id.value_handle == handle_id)
	{
		event.DATA.dailyId.mode = p_data[0];
		event.DATA.dailyId.sid =(((uint16_t)p_data[2] << 8) | (uint16_t)p_data[1]);

		event.dailyLogSendSt = 1;
		if(event.DATA.dailyId.mode == DAILY_ID_CONTINUE)
		{
			event.dailyLogSendSt = SEND_DATA_STATE;
			event.evt_id = EVT_BLE_CMD_READ_LOG;
		}
		if(event.DATA.dailyId.mode == DAILY_ID_SINGLE)
		{
			event.evt_id = EVT_BLE_CMD_READ_LOG_ONE;
		}
		err_fifo = PushFifo(&event);
		DEBUG_EVT_FIFO_LOG(err_fifo,event.evt_id);
	}
	//paring code write
	else if (gPairing_code_id.value_handle == handle_id)
	{
		event.evt_id = EVT_BLE_CMD_PINCODE_CHECK;
		memcpy(&event.DATA.pinCode.pinCode, p_data, len);
#ifdef FORCE_PAIRING
		//memset(&event.DATA.pinCode.pinCode,0x01,len);
#endif
		err_fifo = PushFifo(&event);
		DEBUG_EVT_FIFO_LOG(err_fifo,event.evt_id);
	 }
	//pairing code erase.
	else if (gPairing_clear_id.value_handle == handle_id)
	{
		event.evt_id = EVT_BLE_CMD_PINCODE_ERASE;
		memcpy(&event.DATA.pincodeClear.pinCodeClear, p_data, len);
		err_fifo = PushFifo(&event);
		DEBUG_EVT_FIFO_LOG(err_fifo,event.evt_id);
	}
	else if(gRead_err_id.value_handle == handle_id)
	{
		SetBleErrReadCmd((uint8_t)UTC_BLE_SUCCESS);
	}
	/* 2022.03.22 Add Test Command ++ */
	else if (g_test_cmd_id.value_handle == handle_id)
	{
		GetBleCntHandle(&cnt_handle);
		sd_ble_gap_disconnect( cnt_handle, BLE_HCI_CONN_INTERVAL_UNACCEPTABLE );
		/* スタンバイフラグON */
		SetForceDisconnect();
		//DEBUG_LOG(LOG_INFO,"standby write");
	}
	/* 2022.03.22 Add Test Command ++ */
	/* 2022.05.16 Add 角度調整 ++ */
	else if ( g_angle_adjust_id.value_handle == handle_id )
	{
		DEBUG_LOG( LOG_INFO, "len=%d", len );
		/* モード取得 */
		memcpy( &event.DATA.angle_adjust_info, p_data, sizeof( ANGLE_ADJUST_INFO ) );
		/* 角度調整コマンド */
		event.evt_id = EVT_BLE_CMD_ANGLE_ADJUST;
		err_fifo = PushFifo( &event );
		DEBUG_EVT_FIFO_LOG(err_fifo,event.evt_id);
	}
	/* 2022.05.16 Add 角度調整 -- */
	else
	{
		/*do nothing*/
	}
	// Restart force disconnect timer
	RestartForceDisconTimer();
}

/**
 * @brief BLE Read command BLE event
 * @param handle_id handle id
 * @retval None
 */
void SelectEvtRead(uint16_t handle_id)
{
	ble_gatts_value_t value_param;
	EVT_ST event;
	uint32_t fifo_err;
	ble_gatts_rw_authorize_reply_params_t reply_dummy;
	ret_code_t err_code;
	uint16_t cnt_handle = BLE_CONN_HANDLE_INVALID;
	int16_t x_offset = 0;
	int16_t y_offset = 0;
	int16_t z_offset = 0;
	uint8_t valid = 0;
	uint8_t reply_offset_data[CALIB_SIZE];

	memset(&value_param, 0x00, sizeof(value_param));
	memset(&event, 0x00, sizeof(event));
	
	if(gGet_time_id.value_handle == handle_id)
	{
		
#ifdef DIALY_ACC_LOG_ON
		/*debug*/
		gDaily_display = ~gDaily_display;
#endif
		event.evt_id = EVT_BLE_CMD_GET_TIME;
		fifo_err = PushFifo(&event);
		DEBUG_EVT_FIFO_LOG(fifo_err,event.evt_id);
	}
	else if(gBattery_mVol_id.value_handle == handle_id)
	{
		event.evt_id = EVT_BLE_CMD_GET_BAT;
		fifo_err = PushFifo(&event);
		DEBUG_EVT_FIFO_LOG(fifo_err,event.evt_id);
	}
#if 0
	else if(gCalib_id.value_handle == handle_id)
	{
		GetBleCntHandle(&cnt_handle);
		reply_dummy.type				= BLE_GATTS_AUTHORIZE_TYPE_READ;
		reply_dummy.params.read.update	= 1;
		reply_dummy.params.read.offset	= 0;
		
		GetAccOffset(&valid,&x_offset,&y_offset,&z_offset);
		reply_offset_data[0] = valid;
		memcpy(&reply_offset_data[1],&x_offset,sizeof(x_offset));
		reply_offset_data[3] = valid;
		memcpy(&reply_offset_data[4],&y_offset,sizeof(y_offset));
		
		z_offset += 1000;
		reply_offset_data[6] = valid;
		memcpy(&reply_offset_data[7],&z_offset,sizeof(z_offset));
		reply_dummy.params.read.p_data = (uint8_t*)reply_offset_data;
		reply_dummy.params.read.len = sizeof(reply_offset_data);
		
		err_code = sd_ble_gatts_rw_authorize_reply(cnt_handle, &reply_dummy);
		if(err_code != NRF_SUCCESS)
		{
			SetBleErrReadCmd(UTC_BLE_AUTHORIZE_ERROR);
		}
		BleAuthorizeErrProcess(err_code);
	}
#endif
	else
	{
		GetBleCntHandle(&cnt_handle);
		reply_dummy.type 					= BLE_GATTS_AUTHORIZE_TYPE_READ;	//dummy_data
		reply_dummy.params.read.gatt_status	= BLE_GATT_STATUS_SUCCESS;
		reply_dummy.params.read.update 		= 0;
		
		if(gCalib_id.value_handle == handle_id)
		{
			reply_dummy.params.read.update 			= 1;
			GetAccOffset(&valid,&x_offset,&y_offset,&z_offset);
			reply_offset_data[0] = valid;
			memcpy(&reply_offset_data[1],&x_offset,sizeof(x_offset));
			reply_offset_data[3] = valid;
			memcpy(&reply_offset_data[4],&y_offset,sizeof(y_offset));
		
			z_offset += 1000;
			reply_offset_data[6] = valid;
			memcpy(&reply_offset_data[7],&z_offset,sizeof(z_offset));
			reply_dummy.params.read.p_data = (uint8_t*)reply_offset_data;
			reply_dummy.params.read.len = sizeof(reply_offset_data);
		}
		
		reply_dummy.params.read.offset = 0;
		err_code = sd_ble_gatts_rw_authorize_reply(cnt_handle,&reply_dummy);
		if(err_code != NRF_SUCCESS)
		{
			SetBleErrReadCmd(UTC_BLE_AUTHORIZE_ERROR);
		}
		BleAuthorizeErrProcess(err_code);
		/*Do not nothing*/
	}
	RestartForceDisconTimer();
}

/**
 * @brief BLE Notify Signals
 * @param pSig signal
 * @param size signal size
 * @retval None
 */
void BleNotifySignals(uint8_t *pSig, uint16_t size)
{
	SetBleNotifyCmd(gFw_res_id.value_handle,pSig,size);
}

/**
 * @brief set acc offset value for ble read
 * @param None
 * @retval None
 */
void SetCalibOffsetValueReadSet(void)
{
	ret_code_t err;
	ble_gatts_value_t offset_value;
	uint16_t cnt_handle = BLE_CONN_HANDLE_INVALID;
	uint16_t value_id;
	int16_t acc_x_offset;
	int16_t acc_y_offset;
	int16_t acc_z_offset;
	uint8_t valid;
	uint8_t st_set_data[CALIB_SIZE];
	
	memset(st_set_data,0x00,sizeof(st_set_data));
	/*get offset_value with acc_value*/
	GetAccOffset(&valid, &acc_x_offset, &acc_y_offset, &acc_z_offset);
	
	if(valid == 1)
	{
		st_set_data[0] = 1;
		memcpy(&st_set_data[1],&acc_x_offset,sizeof(acc_x_offset));
		st_set_data[3] = 1;
		memcpy(&st_set_data[4],&acc_y_offset,sizeof(acc_y_offset));
		st_set_data[6] = 1;
		/*Z axis offset value correct Raw Data*/
		acc_z_offset = acc_z_offset + 1000;
		memcpy(&st_set_data[7],&acc_z_offset,sizeof(acc_z_offset));
	}
	offset_value.offset  = BLE_SET_VALUE_OFFSET;
	offset_value.p_value = (uint8_t*)&st_set_data;
	offset_value.len     = sizeof(st_set_data);
	
	GetBleCntHandle(&cnt_handle);
	GetGattsCharHandleValueID(&value_id, CALIB_ID);
	//err = sd_ble_gatts_value_set(cnt_handle, value_id,(void*)&offset_value);
	err = sd_ble_gatts_value_set(cnt_handle, value_id, &offset_value);
	/*change future*/
	DEBUG_LOG(LOG_INFO,"utc ble offset read value set.valid %u, x value %d, y value %d, z value %d",valid, acc_x_offset, acc_y_offset, acc_z_offset);
	BleSetValueErrProcess(err);
}

/**
 * @brief Update MTU Size
 * @param None
 * @retval None
 */
void UpdateMtuSize( uint16_t mtu_size )
{
	ret_code_t err_code;

	err_code = nrf_ble_gatt_att_mtu_periph_set( &m_shose_gatt, mtu_size );
	LIB_ERR_CHECK(err_code,GATT_MTU_SIZE_UPD_ERROR, __LINE__);
}

