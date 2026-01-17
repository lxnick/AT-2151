/**
  ******************************************************************************************
  * @file    lib_adc.c
  * @author  k.tashiro
  * @version 1.0
  * @date    2022/01/15
  * @brief   SAADC Control
  ******************************************************************************************
    Revision:
    Ver            Date             Revised by        Explanation
  ******************************************************************************************
  * 1.0            2022/01/15       k.tashiro         create new
  ******************************************************************************************
*/

/* Includes --------------------------------------------------------------*/
//#include "lib_common.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_drv_saadc.h"
#include "nrfx_saadc.h"
//#include "state_control.h"
//#include "lib_fifo.h"
//#include "lib_bat.h"
#include "lib_adc.h"
//#include "lib_timer.h"
//#include "lib_trace_log.h"

#include "SEGGER_RTT.h"

/* Definition ------------------------------------------------------------*/
#define USE_BAT_FIFIO		0	// Do oneshot adc only
#define BAT_ADC_COUNT		10

#define SAMPLES_IN_BUFFER	1

#define ANALOG_BH_BAT	0

/* Private variables ------------------------------------------------------*/
#if USE_BAT_FIFIO
volatile nrf_saadc_value_t g_adc_buffer[2][SAMPLES_IN_BUFFER];
#endif

static ret_code_t analog_startup_init( void );
static ret_code_t analog_startup_uninit( void );
static ret_code_t analog_startup_get_val( int32_t *adc_value );

volatile int32_t bat_level_mv = 0;	

/**
 * @brief SAADC Event Handler
 * @param pAdcEvent SAADC Event Information
 * @retval None
 */
 #if USE_BAT_FIFIO
static void saadc_evt_handler( nrf_drv_saadc_evt_t const * pAdcEvent )
{
	uint32_t err_code;
	EVT_ST event;
	bool uart_output_enable;

	uart_output_enable = GetUartOutputStatus();
	if( uart_output_enable == false )
	{
		LibUartEnable();
	}

	if(pAdcEvent->type == NRF_DRV_SAADC_EVT_DONE)
	{
		/* adc value fifo input */
		event.DATA.bat.batValue = pAdcEvent->data.done.p_buffer[0];
		DEBUG_LOG( LOG_DEBUG, "adc value %d", event.DATA.bat.batValue );
	}
	else
	{
		/* エラーの場合ダミーデータを設定 */
		event.DATA.bat.batValue = 0;
	}
	/* Event Push */
	event.evt_id = EVT_ADC_READ;
	err_code = PushFifo(&event);
	DEBUG_EVT_FIFO_LOG(err_code, event.evt_id);
	AnalogUninit();

	if( uart_output_enable == false )
	{
		LibUartDisable();
	}
}

/**
 * @brief SAADC Sampling Start
 * @param None
 * @retval None
 */
void StartADC( void )
{
	ret_code_t err_code;

	DEBUG_LOG( LOG_INFO, "!!! Start ADC !!!" );
	
	/* SAADC Init */
	AnalogInitialize();
	/* Sampling Start */
	err_code = nrf_drv_saadc_sample();
	LIB_ERR_CHECK( err_code, ADC_START, __LINE__ );
}

/**
 * @brief SAADC Initialize
 * @param None
 * @retval None
 */
void AnalogInitialize( void )
{
	ret_code_t err_code;
	nrf_drv_saadc_config_t config_saadc = NRFX_SAADC_DEFAULT_CONFIG;
	nrf_saadc_channel_config_t channel_config = NRF_DRV_SAADC_DEFAULT_CHANNEL_CONFIG_SE( NRF_SAADC_INPUT_AIN3 );
	
	/* Acquisition time : 3us */
	channel_config.acq_time = NRF_SAADC_ACQTIME_3US;
	/* Resolution : 10bit */
	config_saadc.resolution = NRF_SAADC_RESOLUTION_10BIT;

	/* SAADC Initialize and event handler setup */
	err_code = nrf_drv_saadc_init( &config_saadc, saadc_evt_handler );
	LIB_ERR_CHECK( err_code, ADC_INIT, __LINE__ );
	/* Channel Config */
	err_code = nrf_drv_saadc_channel_init( NULL, &channel_config );
	LIB_ERR_CHECK( err_code, ADC_INIT, __LINE__ );
	/* ADC Sampling Buffer Setup */
	err_code = nrf_drv_saadc_buffer_convert( (nrf_saadc_value_t*)g_adc_buffer[0], SAMPLES_IN_BUFFER );
	LIB_ERR_CHECK( err_code, ADC_INIT, __LINE__ );
}

/**
 * @brief SAADC Uninitialize
 * @param None
 * @retval None
 */
void AnalogUninit( void )
{
	/* SAADC 終了 */
	nrf_drv_saadc_uninit();
	/* GPIOをNon Connet状態に設定 */
	nrf_gpio_cfg( NRF_SAADC_INPUT_AIN3,
				  NRF_GPIO_PIN_DIR_INPUT,
				  NRF_GPIO_PIN_INPUT_DISCONNECT,
				  NRF_GPIO_PIN_NOPULL,
				  NRF_GPIO_PIN_D0S1,
				  NRF_GPIO_PIN_NOSENSE);
}
#endif

/**
 * @brief 起動時の電圧チェック
 * @param None
 * @retval None
 */
ret_code_t AnalogStartUpCheck( void )
{
	ret_code_t err_code = 0;
	int32_t bat_volt = 0;

	err_code = analog_startup_init();
	if ( err_code == NRF_SUCCESS )
	{
		err_code = analog_startup_get_val( &bat_volt );
	}
	
	if ( err_code == NRF_SUCCESS )
	{
//		DEBUG_LOG( LOG_INFO, "bat: %d", bat_volt );
		/* 電圧値判定処理 */
		if ( bat_volt <= THRESHOLD_BAT_VOLT )
		{
			err_code = BATVOLT_NG;
		}
		else
		{
			bat_level_mv = bat_volt;
			err_code = BATVOLT_OK;
		}
	}
	
	analog_startup_uninit();
	
	return err_code;
}

/**
 * @brief 起動時の電圧チェック初期化
 * @param None
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 Failed
 */
static ret_code_t analog_startup_init( void )
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
static ret_code_t analog_startup_uninit( void )
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
static ret_code_t analog_startup_get_val( int32_t *adc_value )
{
	ret_code_t err_code;
	int32_t average_value = 0;
	int32_t calc_battery_val;
	int16_t temp_value = 0;
	uint16_t idx;
	
	/* 10回取得した平均値から電圧値を求める */
	for ( idx = 0; idx < BAT_ADC_COUNT; idx++ )
	{
		/* Dataを取得 */
        err_code = nrfx_saadc_sample_convert( ANALOG_BH_BAT, &temp_value );
		if ( err_code != NRF_SUCCESS )
		{
			return err_code;
		}
		average_value += temp_value;
	}
	average_value /= BAT_ADC_COUNT;
	
	calc_battery_val = average_value * 1000 / 1024;
	calc_battery_val = calc_battery_val * 36 / 10;
	
	if ( adc_value != NULL )
	{
		*adc_value = calc_battery_val;
	}

	return NRF_SUCCESS;
}

