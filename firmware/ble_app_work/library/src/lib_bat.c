/**
  ******************************************************************************************
  * @file    lib_bat.c
  * @author  k.tashiro
  * @version 1.0
  * @date    2020/10/29
  * @brief   Battery Control
  ******************************************************************************************
    Revision:
    Ver            Date             Revised by        Explanation
  ******************************************************************************************
  * 1.0            2020/10/29       k.tashiro         create new
  ******************************************************************************************
*/

/* Includes --------------------------------------------------------------*/
#include "lib_common.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "ble_gap.h"
#include "ble_gatts.h"
#include "ble_definition.h"
#include "state_control.h"
#include "lib_fifo.h"
#include "lib_bat.h"
#include "lib_adc.h"
#include "lib_timer.h"
#include "lib_trace_log.h"

/* Definition ------------------------------------------------------------*/
/* 2020.11.16 Add テスト用定義 ++ */
//#define TEST_COMP_AUTHRIZE_ERROR
/* 2020.11.16 Add テスト用定義 -- */

/* Private variables ------------------------------------------------------*/
/* バッテリー値は10回の取得の平均値を取得する */
static volatile int32_t g_battery_total_val = 0;
/* バッテリー値取得のリトライ回数 */
static volatile uint16_t g_battery_retry_count = 0;
/* バッテリー値取得中のエラーカウント */
static volatile uint16_t g_battery_error_val_count = 0;

/**
 * @brief Add Battry Voltage Value
 * @param bat_val 取得したADC値
 * @retval None
 */
static void add_battery_total_val( int32_t bat_val )
{
	uint32_t err_code;
	uint8_t critical_sec_add_bat;

	err_code = sd_nvic_critical_region_enter( &critical_sec_add_bat );
	if( err_code == NRF_SUCCESS )
	{
		/* バッテリー値計算 */
		g_battery_total_val += bat_val;
		critical_sec_add_bat = 0;
		err_code = sd_nvic_critical_region_exit( critical_sec_add_bat );
		if( err_code != NRF_SUCCESS ) {}
	}
}

/**
 * @brief Get Battry Voltage Value
 * @param bat_val ADC値を格納する変数へのポインタ
 * @retval None
 */
static void get_battery_total_val( int32_t *bat_val )
{
	uint32_t err_code;
	uint8_t critical_sec_get_bat;

	if ( bat_val != NULL )
	{
		err_code = sd_nvic_critical_region_enter( &critical_sec_get_bat );
		if( err_code == NRF_SUCCESS )
		{
			/* ADC値取得 */
			*bat_val = g_battery_total_val;
			critical_sec_get_bat = 0;
			err_code = sd_nvic_critical_region_exit( critical_sec_get_bat );
			if( err_code != NRF_SUCCESS ) {}
		}
	}
}

/**
 * @brief Clear Battry Voltage Value
 * @param None
 * @retval None
 */
static void clear_battery_total_val( void )
{
	uint32_t err_code;
	uint8_t critical_sec_clear_bat;

	err_code = sd_nvic_critical_region_enter( &critical_sec_clear_bat );
	if( err_code == NRF_SUCCESS )
	{
		/* ADC値クリア */
		g_battery_total_val = 0;
		critical_sec_clear_bat = 0;
		err_code = sd_nvic_critical_region_exit( critical_sec_clear_bat );
		if( err_code != NRF_SUCCESS ) {}
	}
}

/**
 * @brief Increment Battry Retry Count
 * @param None
 * @retval None
 */
static void inc_battery_retry_count( void )
{
	uint32_t err_code;
	uint8_t critical_sec_inc_retry;

	err_code = sd_nvic_critical_region_enter( &critical_sec_inc_retry );
	if( err_code == NRF_SUCCESS )
	{
		/* リトライカウントインクリメント */
		g_battery_retry_count++;
		critical_sec_inc_retry = 0;
		err_code = sd_nvic_critical_region_exit( critical_sec_inc_retry );
		if( err_code != NRF_SUCCESS ) {}
	}
}

/**
 * @brief Clear Battry Retry Count
 * @param None
 * @retval None
 */
static void clear_battery_retry_count( void )
{
	uint32_t err_code;
	uint8_t critical_sec_clear_retry;

	err_code = sd_nvic_critical_region_enter( &critical_sec_clear_retry );
	if( err_code == NRF_SUCCESS )
	{
		/* リトライカウントクリア */
		g_battery_retry_count = 0;
		critical_sec_clear_retry = 0;
		err_code = sd_nvic_critical_region_exit( critical_sec_clear_retry );
		if( err_code != NRF_SUCCESS ) {}
	}
}

/**
 * @brief Get Battry Retry Count
 * @param None
 * @retval None
 */
static void get_battery_retry_count( uint16_t *retry_count )
{
	uint32_t err_code;
	uint8_t critical_sec_get_retry;

	if ( retry_count != NULL )
	{
		err_code = sd_nvic_critical_region_enter( &critical_sec_get_retry );
		if( err_code == NRF_SUCCESS )
		{
			/* リトライカウントクリア */
			*retry_count = g_battery_retry_count;
			critical_sec_get_retry = 0;
			err_code = sd_nvic_critical_region_exit( critical_sec_get_retry );
			if( err_code != NRF_SUCCESS ) {}
		}
	}
}

/**
 * @brief Increment Battry Error Count
 * @param None
 * @retval None
 */
static void inc_battery_error_count( void )
{
	uint32_t err_code;
	uint8_t critical_sec_inc_error;

	err_code = sd_nvic_critical_region_enter( &critical_sec_inc_error );
	if( err_code == NRF_SUCCESS )
	{
		/* リトライカウントインクリメント */
		g_battery_retry_count++;
		critical_sec_inc_error = 0;
		err_code = sd_nvic_critical_region_exit( critical_sec_inc_error );
		if( err_code != NRF_SUCCESS ) {}
	}
}

/**
 * @brief Clear Battry Error Count
 * @param None
 * @retval None
 */
static void clear_battery_error_count( void )
{
	uint32_t err_code;
	uint8_t critical_sec_clear_error;

	err_code = sd_nvic_critical_region_enter( &critical_sec_clear_error );
	if( err_code == NRF_SUCCESS )
	{
		/* リトライカウントクリア */
		g_battery_retry_count = 0;
		critical_sec_clear_error = 0;
		err_code = sd_nvic_critical_region_exit( critical_sec_clear_error );
		if( err_code != NRF_SUCCESS ) {}
	}
}

/**
 * @brief Get Battry Error Count
 * @param None
 * @retval None
 */
static void get_battery_error_count( uint16_t *error_count )
{
	uint32_t err_code;
	uint8_t critical_sec_get_error;

	if ( error_count != NULL )
	{
		err_code = sd_nvic_critical_region_enter( &critical_sec_get_error );
		if( err_code == NRF_SUCCESS )
		{
			/* リトライカウントクリア */
			*error_count = g_battery_retry_count;
			critical_sec_get_error = 0;
			err_code = sd_nvic_critical_region_exit( critical_sec_get_error );
			if( err_code != NRF_SUCCESS ) {}
		}
	}
}

/**
 * @brief Clear Battery Count Information
 * @param None
 * @retval None
 */
static void clear_battery_info( void )
{
	/* Total Value Clear */
	clear_battery_total_val();
	/* Error Counr Clear */
	clear_battery_error_count();
	/* Retry Count Clear */
	clear_battery_retry_count();
}

/**
 * @brief Battry Voltage BLE Read Authorise
 * @param bat_volt 通知するバッテリー電圧
 * @retval None
 */
static void read_bat_authorise( uint16_t bat_volt )
{
	ble_gatts_rw_authorize_reply_params_t notify_bat_data;
	uint32_t err_code;
	uint16_t cnt_handle = BLE_CONN_HANDLE_INVALID;
	
	notify_bat_data.type					= BLE_GATTS_AUTHORIZE_TYPE_READ;
	notify_bat_data.params.read.gatt_status	= BLE_GATT_STATUS_SUCCESS;
	notify_bat_data.params.read.p_data		= (uint8_t*)&bat_volt;
	notify_bat_data.params.read.update		= 1;
	notify_bat_data.params.read.len			= sizeof(bat_volt);
	notify_bat_data.params.read.offset		= 0;
	GetBleCntHandle( &cnt_handle );
	err_code = sd_ble_gatts_rw_authorize_reply( cnt_handle, &notify_bat_data );
#ifdef TEST_COMP_AUTHRIZE_ERROR
	err_code = NRF_ERROR_BUSY;
#endif
	if(err_code != NRF_SUCCESS)
	{
		SetBleErrReadCmd(UTC_BLE_AUTHORIZE_ERROR);
	}
	BleAuthorizeErrProcess(err_code);
}

/**
 * @brief Get Battery Info from SAADC
 * @param None
 * @retval NRF_SUCCESS Success
 */
uint32_t GetBatteryInfo(void)
{
	/* Stop Force Disconnect Timer */
	StopForceDisconTimer();
	/* Start Battery Timer : 1s */
	StartBatTimer( BAT_TIMEOUT_MS );
	/* Start ADC */
	StartADC();
	
	return NRF_SUCCESS;
}

/**
 * @brief Get Battery Info from SAADC
 * @param None
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 Success
 */
uint32_t ReadAuthorizeBatteryInfo( PEVT_ST pEvent )
{
	int32_t calc_battery_val;
	int32_t adc_val;
	int32_t battery_total_val = 0;
	uint16_t retry_count = 0;
	uint16_t bat_err_count = 0;
	uint16_t div_calc_num = 0;

#ifdef TEST_BATTERY_TO_ERROR
	/* バッテリー情報取得エラーテスト */
	static uint8_t test_battery_err_count = 0;
	if ( 5 < test_battery_err_count )
	{
		test_battery_err_count = 0;
		return 0;
	}
	else
	{
		test_battery_err_count++;
	}
#endif
	adc_val = pEvent->DATA.bat.batValue;
	/* ADC値からVoltage[mV]へ変換 */
	calc_battery_val = adc_val * 1000 / 1024;
	calc_battery_val = calc_battery_val * 36 / 10;
	
	/* Retry Countを取得 */
	get_battery_retry_count( &retry_count );
	DEBUG_LOG( LOG_INFO, "Battery: %d mV, Count: %d", calc_battery_val, retry_count );
	
	if ( retry_count < MAX_GET_BAT_VALUE )
	{
		/* 取得回数が10回より小さいの場合 */
		if ( calc_battery_val != 0 )
		{
			/* バッテリー電圧が取得できている場合は足し込む */
			add_battery_total_val( calc_battery_val );
		}
		else
		{
			/* 取得できていない場合は、エラーカウントを増やす */
			inc_battery_error_count();
		}
		/* リトライカウント処理 */
		inc_battery_retry_count();
		get_battery_retry_count( &retry_count );
		if ( MAX_GET_BAT_VALUE <= retry_count )
		{
			/* リトライカウントが10回以上の場合 */
			get_battery_total_val( &battery_total_val );
			get_battery_error_count( &bat_err_count );
			/* リトライカウントとエラーカウントを計算し成功した分だけの平均値を算出する */
			if ( ( retry_count - bat_err_count ) == 0 )
			{
				div_calc_num = MAX_GET_BAT_VALUE;
			}
			else
			{
				div_calc_num = retry_count - bat_err_count;
			}
			/* 平均値を計算 */
			calc_battery_val = (int16_t)( battery_total_val / ( (int32_t)div_calc_num ) );
			DEBUG_LOG( LOG_INFO, "Battery: %d mV, Ave: %d", calc_battery_val, div_calc_num );
			/* Battery Voltage通知 */
			read_bat_authorise( calc_battery_val );
			/* Clear Battery Counter Information */
			clear_battery_info();
			/* Stop Battery Timeout Timer */
			StopBatTimer();
			/* Restart Force Disconnect Timer */
			RestartForceDisconTimer();
		}
		else
		{
			/* Start ADC */
			StartADC();
		}
	}
	else
	{
		/* リトライカウントが10回以上の場合 */
		calc_battery_val = 0;
		/* Battery Voltage通知 */
		read_bat_authorise( calc_battery_val );
		/* Clear Battery Counter Information */
		clear_battery_info();
		/* Trace Log */
		TRACE_LOG( TR_GET_BAT_ERROR, 0 );
		SetBleErrReadCmd( (uint8_t)UTC_BLE_GET_BAT_ERROR );
		/* Stop Battery Timeout Timer */
		StopBatTimer();
		/* Restart Force Disconnect Timer */
		RestartForceDisconTimer();
	}
	
	return 0;
}

/**
 * @brief ADCの処理がタイムアウトした際のバッテリー情報読み出し処理
 * @param None
 * @retval None
 */
void ReadAuthorizeBatteryInfoTimeout( void )
{
	int32_t bat_total_val = 0;
	int16_t current_bat_val = 0;
	uint16_t current_count = 0;
	uint16_t current_error_count = 0;
	uint16_t div_calc_num = 0;
	
	/* SAADC 終了処理 */
	AnalogUninit();
	
	/* 現在値を取得 */
	get_battery_total_val( &bat_total_val );
	get_battery_retry_count( &current_count );
	get_battery_error_count( &current_error_count );
	/* 平均値計算 */
	if ( ( current_count - current_error_count ) == 0 )
	{
		div_calc_num = MAX_GET_BAT_VALUE;
	}
	else
	{
		div_calc_num = current_count - current_error_count;
	}
	current_bat_val = (int16_t)( bat_total_val / ( (int32_t)div_calc_num ) );
	/* Battery Voltage通知 */
	read_bat_authorise( current_bat_val );
	/* Clear Battery Counter Information */
	clear_battery_info();
	/* Trace Log */
	TRACE_LOG( TR_GET_BAT_ERROR, 1 );
	SetBleErrReadCmd( (uint8_t)UTC_BLE_GET_BAT_ERROR );
	/* Restart Force Disconnect Timer */
	RestartForceDisconTimer();
	
	DEBUG_LOG( LOG_ERROR, "!!! Battery Timeout Event !!!");
	DEBUG_LOG( LOG_DEBUG, "TO bat ave %d, divNum %u", current_bat_val, div_calc_num );
}

