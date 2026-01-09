/**
  ******************************************************************************************
  * Reference source: lib_adc.c
  ******************************************************************************************
*/

/* Includes --------------------------------------------------------------*/
#include "tu_lib_common.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_drv_saadc.h"
#include "nrfx_saadc.h"
#include "tu_state_control.h"
#include "tu_lib_fifo.h"

#define MAX_GET_BAT_VALUE		10

/* Definition ------------------------------------------------------------*/
#define SAMPLES_IN_BUFFER	1

#define ANALOG_BH_BAT	0

/* Private variables ------------------------------------------------------*/
volatile static nrf_saadc_value_t tu_g_adc_buffer[2][SAMPLES_IN_BUFFER];

static ret_code_t TU_analog_startup_init( void );
static ret_code_t TU_analog_startup_uninit( void );
static ret_code_t TU_analog_startup_get_val( int32_t *adc_value );



/**
 * @brief Get ADC value directly
 * @param None
 * @retval None
 */
int32_t TU_AnalogBattGetDirectly( void )
{
	ret_code_t err_code = 0;
	int32_t bat_volt;
	
	err_code = TU_analog_startup_init();
	if ( err_code == NRF_SUCCESS )
	{
		err_code = TU_analog_startup_get_val( &bat_volt );
	}
	
	TU_analog_startup_uninit();
	
	return bat_volt;
}

/**
 * @brief 起動時の電圧チェック初期化
 * @param None
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 Failed
 */
static ret_code_t TU_analog_startup_init( void )
{
	ret_code_t err_code;
	nrf_drv_saadc_config_t config_saadc = NRFX_SAADC_DEFAULT_CONFIG;
	nrf_saadc_channel_config_t channel_config = NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE( NRF_SAADC_INPUT_AIN3 );

	/* SAADC Initialize and event handler setup */
	err_code = nrf_drv_saadc_init( &config_saadc, NULL );
//	LIB_ERR_CHECK( err_code, ADC_INIT, __LINE__ );
	/* Channel Config */
	err_code = nrf_drv_saadc_channel_init( ANALOG_BH_BAT, &channel_config );
//	LIB_ERR_CHECK( err_code, ADC_INIT, __LINE__ );
	
	return err_code;
}

/**
 * @brief 起動時の電圧チェック終了処理
 * @param None
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 Failed
 */
static ret_code_t TU_analog_startup_uninit( void )
{
	ret_code_t err_code;

	err_code = nrf_drv_saadc_channel_uninit( ANALOG_BH_BAT );
	if ( err_code != NRF_SUCCESS )
	{
		return err_code;
	}
	nrf_drv_saadc_uninit();
	
	return err_code;
}

/**
 * @brief 起動時の電圧チェック 電圧取得
 * @param None
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 Failed
 */
static ret_code_t TU_analog_startup_get_val( int32_t *adc_value )
{
	ret_code_t err_code;
	int32_t average_value = 0;
	int32_t calc_battery_val;
	int16_t temp_value = 0;
	uint16_t idx;
	
	/* 10回取得した平均値から電圧値を求める */
	for ( idx = 0; idx < MAX_GET_BAT_VALUE; idx++ )
	{
		/* Dataを取得 */
        err_code = nrfx_saadc_sample_convert( ANALOG_BH_BAT, &temp_value );
		if ( err_code != NRF_SUCCESS )
		{
			return err_code;
		}
		average_value += temp_value;
	}
	average_value /= MAX_GET_BAT_VALUE;
	
	calc_battery_val = average_value * 1000 / 1024;
	calc_battery_val = calc_battery_val * 36 / 10;
	
	if ( adc_value != NULL )
	{
		*adc_value = calc_battery_val;
	}

	return NRF_SUCCESS;
}

