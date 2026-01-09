/**
  ******************************************************************************************
  * @file    lib_icm42607.c
  * @author  k.tashiro
  * @version 1.0
  * @date    2022/03/04
  * @brief   ICM42607 Control ACC/Gyro
  ******************************************************************************************
    Revision:
    Ver            Date             Revised by        Explanation
  ******************************************************************************************
  * 1.0            2022/03/04       k.tashiro         create new
  ******************************************************************************************
*/

/* Includes --------------------------------------------------------------*/
#include <math.h>

#include "nrf_gpio.h"
#include "nrfx_gpiote.h"

#include "lib_icm42607.h"
#include "lib_debug_uart.h"
#include "lib_fifo.h"
#include "ble_definition.h"
#include "mode_manager.h"

/* Definition ------------------------------------------------------------*/
#define FIFO_OUT_SIZE	(ACC_GYRO_FIFO_PACKET_SIZE)		/* FIFO Output Size */

#define INT_LATCH_ENABLE			(1)			/* Interrupt Latch Enable */
#define INT_LATCH_DISABLE			(0)			/* Interrupt Latch Disable */

#define ACC_GYRO_DATA_COMPLETE		(1)			/* FIFOへの格納条件 */
#define ACC_GYRO_DATA_NOT_COMPLETE	(2)			/* FIFOへの格納条件 */

#define DEF_GYRO_CONSTANT			(1000)		/* mdpsから変換するための定数 */
#define DEF_TEMP_CALC_HIGH_POS		(8)			/* Temp Highを計算すためのPosition */
#define DEF_TEMP_CALC_LOW_MASK		(0x00FF)	/* Temp LowのMask */
#define DEF_TEMP_HIGH_MIDIAN		(25)		/* Temp中央値 */

#define WAIT_SW_RESET_INTERVAL		(500)		/* Software Reset Wait time (500us) */
#define WAIT_SW_RESET_RETRY			(3)			/* Software Reset Wait Retry */

#define WAIT_FIFO_FLUSH_INTERVAL	(2)			/* FIFO Flush Wait Interval(2us) */
#define WAIT_FIFO_FLUSH_RETRY		(3)			/* FIFO Flush Wait Retry */

#define WAIT_MODE_CHANGE_INTERVAL	(100)		/* Mode Chenget Wait time (100ms) */
#define MREG_WAIT_APPLY_TIME		(1000)		/* レジスタ反映待ち(10us) */

#define WAIT_RECONFIG_MODE_CHANGE	(100)		/* Mode Chenget Wait time (100ms) */

#define IDLE_WAIT_TIME				(10)		/* IDLE Wait Interval (10ms) */

#define WAIT_REG_SETUP_TIME			(500)		/* Register Wait Time */
#define WAIT_OTP_RELOAD_TIME		(300)		/* OTP Reload Wait Time */

/* 2022.03.24 Test GPIO Output 割り込み時間計測テスト ++ */
#define GPIO_UART_RX_PIN			NRF_GPIO_PIN_MAP(0, 29)
/* 2022.03.24 Test GPIO Output 割り込み時間計測テスト -- */

/* WakeUpのOFS_USR設定計算用種別
 *  WakeUp時に加速度へオフセットかけ他の軸からの割り込みが入りづらくなるようにする
 */
#define X_AXIS_OFFSET				0			/* Offset X-Axis */
#define Y_AXIS_OFFSET				1			/* Offset Y-Axis */
#define Z_AXIS_OFFSET				2			/* Offset Z-Axis */
/* MREGへ書き込んだ後には、0x00を書き込まなければいけない */
#define BLK_SEL_DEFAULT_ADDR		(0x00)		/* BLK_SEL_W and BLK_SEL_Rのデフォルト値 */

/* Private variables -----------------------------------------------------*/
static volatile uint8_t g_acc_gyro_mode = MODE_ACC_ONLY;

static volatile ACC_GYRO_DATA_INFO g_acc_gyro_data = {0};
static volatile ACC_GYRO_DATA_INFO g_temp_data = {0};

static volatile uint32_t g_store_count = 0;
static volatile uint32_t g_temp_store_count = 0;

/* 2020.12.07 Add データ取得時にSIDをつけるように修正 ++ */
static volatile uint16_t g_send_sid = 0;
/* 2020.12.07 Add データ取得時にSIDをつけるように修正 -- */

/* 2020.12.08 Add FIFO Countを設定する ++ */
static volatile uint16_t g_fifo_count = 0;
/* 2020.12.08 Add FIFO Countを設定する -- */


/* Struct ----------------------------------------------------------------*/
typedef nrfx_err_t ( *init_func )( void );
typedef struct _acc_gyro_func_table
{
	uint8_t		number;		/* Function番号 */
	init_func	func;		/* 関数ポインタ */
} ACC_GYRO_FUNC_TABLE;

typedef void ( *calc_func )( ACC_GYRO_DATA_INFO *calc_data );

/* Private Function ------------------------------------------------------*/
/**
 * @brief ACC/Gyro FIFO Non Read Data Number
 * @param fifo_count FIFO Number
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 Failed
 */
static nrfx_err_t get_acc_gyro_fifo_count( uint16_t *fifo_count );

/**
 * @brief ACC/Gyro FIFO Read Data
 * @param acc_gyro_info ACC/Gyro Store Data
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 Failed
 */
static uint32_t reader_acc_gyro_data( ACC_GYRO_DATA_INFO *acc_gyro_info );

/**
 * @brief ACC/Gyro Sensor Setup Error Log
 * @param reg_addr Register Address
 * @param set_val Set Value
 * @param line Execution Line
 * @retval None
 */
__STATIC_INLINE void register_setup_error_log( uint8_t reg_addr, uint8_t set_val, uint32_t line );

/**
 * @brief ICM42607 MREG Write Processing
 * @param blk_sel MREG1(0x00) or MREG2(0x28) or MREG3(0x58)
 * @param reg_addr Register Address
 * @param val Register Write Value
 * @retval NRF_SUCCESS Success
 * @retval ACC_GYRO_SETEUP_ERROR Setup Error
 */
static nrfx_err_t setup_mreg_write( uint8_t blk_sel, uint8_t reg_addr, uint8_t value );

/**
 * @brief ICM42607 MREG Read Processing
 * @param blk_sel MREG1(0x00) or MREG2(0x28) or MREG3(0x58)
 * @param reg_addr Register Address
 * @param val Register Write Value
 * @retval NRF_SUCCESS Success
 * @retval ACC_GYRO_SETEUP_ERROR Setup Error
 */
static nrfx_err_t setup_mreg_read( uint8_t blk_sel, uint8_t reg_addr, uint8_t *value );

/**
 * @brief ACC/Gyro Set Mode
 * @param mode Mode setting
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 Failed
 */
static uint32_t set_acc_gyro_mode( uint8_t mode )
{
	volatile uint32_t err_code;
	uint8_t  acc_gyro_mode_session;
	
	err_code = sd_nvic_critical_region_enter( &acc_gyro_mode_session );
	if(err_code == NRF_SUCCESS)
	{
		/* Bufferも初期化しておく */
		memset( (void *)&g_acc_gyro_data, 0, sizeof( g_acc_gyro_data ) );
		memset( (void *)&g_temp_data, 0, sizeof( g_temp_data ) );
		g_store_count = 0;
		g_temp_store_count = 0;
		g_acc_gyro_mode = mode;
		acc_gyro_mode_session = 0;
		err_code = sd_nvic_critical_region_exit( acc_gyro_mode_session );
		if(err_code != NRF_SUCCESS)
		{
			TRACE_LOG( TR_ACC_SET_MODE_FAILED, 0 );
			DEBUG_LOG( LOG_ERROR, "!!! Set ACC/Gyro Mode critical Exit !!!" );
		}
	}
	else
	{
		TRACE_LOG( TR_ACC_SET_MODE_FAILED, 1 );
		DEBUG_LOG( LOG_ERROR, "!!! Set ACC/Gyro Mode critical Enter !!!" );
	}
	return err_code;
}

/**
 * @brief ACC/Gyro Get Mode
 * @param mode current mode
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 Failed
 */
static uint32_t get_acc_gyro_mode( uint8_t *mode )
{
	volatile uint32_t err_code;
	uint8_t  acc_gyro_mode_session;
	
	err_code = sd_nvic_critical_region_enter( &acc_gyro_mode_session );
	if(err_code == NRF_SUCCESS)
	{
		*mode = g_acc_gyro_mode;
		acc_gyro_mode_session = 0;
		err_code = sd_nvic_critical_region_exit( acc_gyro_mode_session );
		if(err_code != NRF_SUCCESS)
		{
			TRACE_LOG( TR_ACC_GET_MODE_FAILED, 0 );
			DEBUG_LOG( LOG_ERROR, "!!! Get ACC/Gyro Mode critical Exit !!!" );
		}
	}
	else
	{
		TRACE_LOG( TR_ACC_GET_MODE_FAILED, 1 );
		DEBUG_LOG( LOG_ERROR, "!!! Get ACC/Gyro Mode critical Enter !!!" );
	}
	
	return err_code;
}

/**
 * @brief ACC/Gyro INT1 Handler
 * @param pin Input PIN Number
 * @param action Input PIN action
 * @retval None
 */
static void acc_gyro_int1_event_handler( nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action )
{
	volatile EVT_ST event_info = {0};
	volatile nrfx_err_t err_code;
	volatile ACC_GYRO_DATA_INFO acc_gyro_data_info = {0};
	volatile uint32_t fifo_err_code = NRF_SUCCESS;
	uint32_t fifo_err;
	uint16_t fifo_count = 0;
	bool uart_enable = false;
	
	uart_enable = GetUartOutputStatus();
	if ( uart_enable == false )
	{
		LibUartEnable();
	}

	/* 2020.12.09 Add Clear FIFO ++ */
	AccGyroValidateClearFifo();
	/* 2020.12.09 Add Clear FIFO -- */
	
	/* SPI Function Initialize */
	err_code = SpiInit();
	if ( err_code == NRF_SUCCESS )
	{
#if 0
		/* 2022.03.24 Test GPIO Output 割り込み時間計測テスト ++ */
		nrf_gpio_pin_set( GPIO_UART_RX_PIN );
		/* 2022.03.24 Test GPIO Output 割り込み時間計測テスト -- */
#endif
		/* 現在 FIFOに積まれているデータ数を取得 */
		err_code = get_acc_gyro_fifo_count( &fifo_count );
		if ( err_code == NRF_SUCCESS )
		{
			for ( uint16_t i = 0; i < fifo_count; i++ )
			{
				/* ACC/Gyro DataをFIFOから取得 */
				err_code = reader_acc_gyro_data( (void *)&acc_gyro_data_info );
				if ( err_code == ACC_GYRO_DATA_COMPLETE )
				{
					/* 2020.12.09 SIDをここで付加 ++ */
					acc_gyro_data_info.sid = AccGyroIncSid();
					/* 2020.12.09 SIDをここで付加 -- */
					/* 2020.12.08 Add 現在のFIFO Countを計測するように修正 ++ */
					AccGyroIncFifoCount();
					/* 2020.12.08 Add 現在のFIFO Countを計測するように修正 -- */
					/* データの準備が完了した際にFIFOに積む */
					fifo_err = AccGyroPushFifo( (void *)&acc_gyro_data_info );
					if ( fifo_err == NRF_ERROR_NO_MEM )
					{
						/* 2020.12.08 Add Buffer Fullの場合FIFO Countをへらす ++ */
						fifo_err_code = fifo_err;
						AccGyroSubFifoCount();
						/* 2020.12.08 Add Buffer Fullの場合FIFO Countをへらす -- */
					}
					memset( (void *)&acc_gyro_data_info, 0, sizeof( acc_gyro_data_info ) );
				}
				else if ( err_code != ACC_GYRO_DATA_NOT_COMPLETE )
				{
					TRACE_LOG( TR_ACC_READ_INT_ERROR, 0 );
				}
			}
#if 0
			/* 2022.03.24 Test GPIO Output 割り込み時間計測テスト ++ */
			nrf_gpio_pin_clear( GPIO_UART_RX_PIN );
			/* 2022.03.24 Test GPIO Output 割り込み時間計測テスト -- */
#endif
		}
		else
		{
			TRACE_LOG( TR_ACC_READ_INT_ERROR, 1 );
		}
		/* 終了処理 */
		SpiUninit();
	}
	else
	{
		/* 初期化エラー時にはTraceLogを保存 */
		TRACE_LOG( TR_ACC_READ_INT_INIT_ERROR, 0 );
		DEBUG_LOG( LOG_ERROR, "!!! SPI Init Error !!!" );
		SetBleErrReadCmd( (uint8_t)UTC_BLE_SPI_INIT_ERROR );
	}
	
	/* 2020.12.08 Add FIFOがエラーになった場合にログ出力を追加 ++ */
	if ( fifo_err_code == NRF_ERROR_NO_MEM )
	{
		DEBUG_LOG( LOG_DEBUG, "Acc/Gyro PushFifo No MEM" );
	}
	/* 2020.12.08 Add FIFOがエラーになった場合にログ出力を追加 -- */
	
	event_info.evt_id = EVT_ACC_FIFO_INT;
	fifo_err = PushFifo( (void *)&event_info );
	DEBUG_EVT_FIFO_LOG( fifo_err, EVT_ACC_FIFO_INT );
	/* 2020.12.08 Add 割り込みを無効にするのではなくLatch Clearのみに修正 ++ */
	nrf_gpio_pin_latch_clear( ACC_INT1_PIN );
	/* 2020.12.08 Add 割り込みを無効にするのではなくLatch Clearのみに修正 -- */

	if ( uart_enable == false )
	{
		LibUartDisable();
	}
}

/**
 * @brief setup ACC/Gyro FIFO Mode
 * @param value FIFO Mode
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 Failed
 */
static nrfx_err_t setup_acc_gyro_fifo_mode( uint8_t value )
{
	volatile nrfx_err_t ret;
	volatile uint8_t read_data = 0;
	
	ret = SpiIORead( ICM42607_FIFO_CONFIG1, (uint8_t *)&read_data, sizeof( read_data ) );
	SPI_ERR_CHECK( ret, __LINE__ );
	if ( ret != NRF_SUCCESS )
	{
		return ret;
	}
	
	/* FIFO Bypassのみ設定変更 */
	read_data &= ~FIFO_CONFIG1_BYPASS_MASK;
	read_data |= value;
	
	ret = SpiIOWrite( ICM42607_FIFO_CONFIG1, (uint8_t *)&read_data, sizeof( read_data ) );
	SPI_ERR_CHECK( ret, __LINE__ );

	return ret;
}

/**
 * @brief setup ACC/Gyro Device Interrupt
 * @param reg_addr INTX_CTRL Register Address
 * @param value Set INT Value
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 Failed
 */
static nrfx_err_t setup_acc_gyro_interrupt( uint8_t reg_addr, uint8_t value )
{
	volatile nrfx_err_t ret;
	volatile uint8_t read_data = 0;
	
	ret = SpiIORead( reg_addr, (uint8_t *)&read_data, sizeof( read_data ) );
	SPI_ERR_CHECK( ret, __LINE__ );
	if ( ret != NRF_SUCCESS )
	{
		return ret;
	}
	
	/* 割り込みレジスタは一度すべての割り込みを無効にした後に有効にするもののみ設定する */
	read_data &= (uint8_t)~ACC_GYRO_INT_MASK;
	read_data |= value;
	
	ret = SpiIOWrite( reg_addr, (uint8_t *)&read_data, sizeof( read_data ) );
	SPI_ERR_CHECK( ret, __LINE__ );
	
	return ret;
}

/**
 * @brief setup ACC/Gyro Weke up Threshold
 * @param address Set Register Address
 * @param theshold wake up threshold
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 Failed
 */
static nrfx_err_t setup_acc_gyro_wakeup_ths( uint8_t address, uint8_t theshold )
{
	volatile nrfx_err_t ret = 0;
	volatile uint8_t read_data = 0;

	/* MREG1 Bankを指定し、データ読み出し */
	ret = setup_mreg_read( ICM42607_MREG1, address, (void *)&read_data );
	SPI_ERR_CHECK( ret, __LINE__ );
	if ( ret != NRF_SUCCESS )
	{
		return ret;
	}
	
	read_data &= ~WOM_THRESHOLD_MASK;
	read_data |= theshold;
	
	/* TMST_CONFIG1への書き込み */
	ret = setup_mreg_write( ICM42607_MREG1, address, read_data );
	SPI_ERR_CHECK( ret, __LINE__ );
	
	return ret;
}

/**
 * @brief setup FIFO watermark
 * @remark FIFO_CTRL1[7:0]とFIFO_CTRL2[9:8]を設定する
 * @param wtm watermark
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 Failed
 */
static nrfx_err_t setup_acc_gyro_fifo_wtm( uint16_t wtm )
{
	volatile nrfx_err_t ret = 0;
	volatile uint8_t set_val = 0;
	volatile uint8_t read_data = 0;

	/* FIFO_CONFIG2への設定 */
	set_val = (uint8_t)( wtm & ACC_GYRO_FIFO_CONFIG2_WTM_MASK );

	ret = SpiIOWrite( ICM42607_FIFO_CONFIG2, (uint8_t *)&set_val, sizeof( set_val ) );
	SPI_ERR_CHECK( ret, __LINE__ );
	if ( ret != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_FIFO_CONFIG2, set_val, __LINE__ );
		return ret;
	}
	
	/* 指定されたwatermarkを確認し、設定があるようであればFIFO_CTRL2へ設定する */
	set_val = (uint8_t)( ( wtm & ACC_GYRO_FIFO_CONFIG3_WTM_MASK ) >> 8 );
	if ( set_val != 0 )
	{
		ret = SpiIOWrite( ICM42607_FIFO_CONFIG3, (uint8_t *)&set_val, sizeof( set_val ) );
		SPI_ERR_CHECK( ret, __LINE__ );
		if ( ret != NRF_SUCCESS )
		{
			register_setup_error_log( ICM42607_FIFO_CONFIG3, set_val, __LINE__ );
		}
	}
	return ret;
}

/**
 * @brief setup software reset
 * @param Noen
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 Failed
 */
static nrfx_err_t setup_acc_gyro_software_reset( void )
{
	volatile nrfx_err_t ret = 0;
	volatile uint8_t sw_reset = ACC_GYRO_SW_RESET_VAL;

	ret = SpiIOWrite( ICM42607_SIGNAL_PATH_RESET, (uint8_t *)&sw_reset, sizeof( sw_reset ) );
	SPI_ERR_CHECK( ret, __LINE__ );
	
	return ret;
}

/**
 * @brief setup ACC/Gyro INT1/INT2 GPIO Pins
 * @param None
 * @retval None
 */
static void setup_acc_gyro_gpio_pin_init( void )
{
	nrfx_gpiote_in_config_t gpio_config;
	uint16_t i;
	const ACC_GYRO_GPIO_PIN_INFO gpio_pin_info[] = {
		{	ACC_INT1_PIN,	NRF_GPIO_PIN_NOPULL,	NRF_GPIOTE_POLARITY_LOTOHI,	acc_gyro_int1_event_handler	},
	};
	
	for ( i = 0; i < sizeof( gpio_pin_info ) / sizeof( ACC_GYRO_GPIO_PIN_INFO ); i++ )
	{
		nrf_gpio_cfg_input( gpio_pin_info[i].gpio_pin, gpio_pin_info[i].pull_config );
		nrf_gpio_cfg_sense_set( gpio_pin_info[i].gpio_pin, NRF_GPIO_PIN_NOSENSE );

		/* latchクリア */
        nrf_gpio_pin_latch_clear( gpio_pin_info[i].gpio_pin );
		/* GPIO設定 */
		gpio_config.pull		= gpio_pin_info[i].pull_config;
		gpio_config.hi_accuracy	= 0;
		gpio_config.is_watcher	= 0;
		gpio_config.sense		= gpio_pin_info[i].sense_config;
		/* 一度、Uninit */
		nrfx_gpiote_in_uninit( gpio_pin_info[i].gpio_pin );
		
		nrfx_gpiote_in_init(gpio_pin_info[i].gpio_pin, &gpio_config, gpio_pin_info[i].handler );
	}
}

/**
 * @brief setup ACC/Gyro INT1/INT2 GPIO Pins WakeUp Setting
 * @param None
 * @retval None
 */
static void setup_acc_gyro_gpio_pin_wakeup( void )
{
	uint16_t i;
	const ACC_GYRO_GPIO_WAKEUP_PIN_INFO gpio_pin_info[] = {
		{	ACC_INT1_PIN,	NRF_GPIO_PIN_NOPULL,	NRF_GPIO_PIN_SENSE_HIGH	},
	};

	if ( NRF_GPIO->DETECTMODE != 1 )
	{
		/* latch enale */
		NRF_GPIO->DETECTMODE = 1;
	}

	/* 起動設定 */
	for ( i = 0; i < sizeof( gpio_pin_info ) / sizeof( ACC_GYRO_GPIO_WAKEUP_PIN_INFO ); i++ )
	{
		nrf_gpio_cfg_input( gpio_pin_info[i].gpio_pin, gpio_pin_info[i].pull_config );
		nrf_gpio_cfg_sense_set( gpio_pin_info[i].gpio_pin, gpio_pin_info[i].sense_config );
		nrf_gpio_pin_latch_clear( gpio_pin_info[i].gpio_pin );
	}
}

/**
 * @brief ACC/Gyro ODR Setting
 * @param address Setting ODR Register Address
 * @param odr ODR Setting
 * @param fss full-scale selection
 * @retval NRF_SUCCESS Success
 * @retval ACC_GYRO_SETEUP_ERROR Setup Error
 */
static nrfx_err_t setup_acc_gyro_sensor_odr_fss( uint8_t address, uint8_t odr, uint8_t fss )
{
	volatile nrfx_err_t ret;
	volatile uint8_t read_data = 0;

	ret = SpiIORead( address, (uint8_t *)&read_data, sizeof( read_data ) );
	SPI_ERR_CHECK( ret, __LINE__ );
	if ( ret != NRF_SUCCESS )
	{
		return ret;
	}

	read_data &= (uint8_t)~(ACC_GYRO_ODR_MASK | ACC_GYRO_FSS_MASK);
	read_data |= odr | ( fss << ACC_GYRO_FSS_POS );
	
	ret = SpiIOWrite( address, (uint8_t *)&read_data, sizeof( read_data ) );
	SPI_ERR_CHECK( ret, __LINE__ );
	
	return ret;
}

/**
 * @brief ACC/Gyro Power Management Setting
 * @param acc_mode ACC Mode Setting
 * @param gyro_mode Gyro Mode Setting
 * @retval NRF_SUCCESS Success
 * @retval ACC_GYRO_SETEUP_ERROR Setup Error
 */
static nrfx_err_t setup_acc_gyro_pwr_mgmt0( uint8_t acc_mode, uint8_t gyro_mode )
{
	volatile nrfx_err_t ret;
	volatile uint8_t read_data = 0;

	ret = SpiIORead( ICM42607_PWR_MGMT0, (uint8_t *)&read_data, sizeof( read_data ) );
	SPI_ERR_CHECK( ret, __LINE__ );
	if ( ret != NRF_SUCCESS )
	{
		return ret;
	}

	read_data &= (uint8_t)~ACC_GYRO_MODE_MASK;
	read_data |= ( gyro_mode << PWR_MGMT_GYRO_MODE_POS ) | acc_mode;
	
	ret = SpiIOWrite( ICM42607_PWR_MGMT0, (uint8_t *)&read_data, sizeof( read_data ) );
	SPI_ERR_CHECK( ret, __LINE__ );
	if ( ret != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_PWR_MGMT0, read_data, __LINE__ );
	}
	
	return ret;
}

/**
 * @brief ACC/Gyro Power Management IDLE Control
 * @param acc_mode ACC Mode Setting
 * @param gyro_mode Gyro Mode Setting
 * @retval NRF_SUCCESS Success
 * @retval ACC_GYRO_SETEUP_ERROR Setup Error
 */
static nrfx_err_t setup_acc_gyro_idle_ctrl( uint8_t enable )
{
	volatile nrfx_err_t ret;
	volatile uint8_t read_data = 0;

	ret = SpiIORead( ICM42607_PWR_MGMT0, (uint8_t *)&read_data, sizeof( read_data ) );
	SPI_ERR_CHECK( ret, __LINE__ );
	if ( ret != NRF_SUCCESS )
	{
		return ret;
	}

	read_data &= ~ACC_GYRO_IDLE_MASK;
	read_data |= enable;
	
	ret = SpiIOWrite( ICM42607_PWR_MGMT0, (uint8_t *)&read_data, sizeof( read_data ) );
	SPI_ERR_CHECK( ret, __LINE__ );
	if ( ret != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_PWR_MGMT0, read_data, __LINE__ );
	}
	
	return ret;
}

/**
 * @brief ACC/Gyro FIFO Format Setting
 * @param None
 * @retval NRF_SUCCESS Success
 * @retval ACC_GYRO_SETEUP_ERROR Setup Error
 */
static nrfx_err_t setup_acc_gyro_fifo_format( void )
{
	volatile nrfx_err_t ret;
	volatile uint8_t read_data = 0;
	
	ret = SpiIORead( ICM42607_INTF_CONFIG0, (uint8_t *)&read_data, sizeof( read_data ) );
	SPI_ERR_CHECK( ret, __LINE__ );
	if ( ret != NRF_SUCCESS )
	{
		return ret;
	}

	read_data &= (uint8_t)~ACC_GYRO_FIFO_FORMAT_MASK;
	read_data |= ACC_GYRO_FIFO_FORMAT_VAL | ACC_GYRO_FIFO_COUNT_FORMAT;
	
	ret = SpiIOWrite( ICM42607_INTF_CONFIG0, (uint8_t *)&read_data, sizeof( read_data ) );
	SPI_ERR_CHECK( ret, __LINE__ );
	
	return ret;
}

/**
 * @brief ICM42607 MREG Write Processing
 * @param blk_sel MREG1(0x00) or MREG2(0x28) or MREG3(0x58)
 * @param reg_addr Register Address
 * @param val Register Write Value
 * @retval NRF_SUCCESS Success
 * @retval ACC_GYRO_SETEUP_ERROR Setup Error
 */
static nrfx_err_t setup_mreg_write( uint8_t blk_sel, uint8_t reg_addr, uint8_t value )
{
	volatile nrfx_err_t ret;
	volatile uint8_t reset_val = BLK_SEL_DEFAULT_ADDR;
	
	/* BLK_SEL_Wレジスタへバンクを指定 */
	ret = SpiIOWrite( ICM42607_BLK_SEL_W, (uint8_t *)&blk_sel, sizeof( blk_sel ) );
	SPI_ERR_CHECK( ret, __LINE__ );
	if ( ret != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_BLK_SEL_W, blk_sel, __LINE__ );
		return ret;
	}
	/* MADDR_Wレジスタへ書き込みを実行するレジスタアドレスを指定 */
	ret = SpiIOWrite( ICM42607_MADDR_W, (uint8_t *)&reg_addr, sizeof( reg_addr ) );
	SPI_ERR_CHECK( ret, __LINE__ );
	if ( ret != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_MADDR_W, reg_addr, __LINE__ );
		return ret;
	}
	
	/* M_Wレジスタへ書き込みを実行 */
	ret = SpiIOWrite( ICM42607_M_W, (uint8_t *)&value, sizeof( value ) );
	SPI_ERR_CHECK( ret, __LINE__ );
	if ( ret != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_M_W, value, __LINE__ );
		return ret;
	}
	/* Write完了待ち(10us) */
	nrf_delay_us( MREG_WAIT_APPLY_TIME );
	
	/* 書き込み完了後は、BLK_SEL_Wを0x00に戻す */
	ret = SpiIOWrite( ICM42607_BLK_SEL_W, (uint8_t *)&reset_val, sizeof( reset_val ) );
	SPI_ERR_CHECK( ret, __LINE__ );
	if ( ret != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_BLK_SEL_W, reset_val, __LINE__ );
	}
	
	return ret;
}

/**
 * @brief ICM42607 MREG Read Processing
 * @param blk_sel MREG1(0x00) or MREG2(0x28) or MREG3(0x58)
 * @param reg_addr Register Address
 * @param val Register Write Value
 * @retval NRF_SUCCESS Success
 * @retval ACC_GYRO_SETEUP_ERROR Setup Error
 */
static nrfx_err_t setup_mreg_read( uint8_t blk_sel, uint8_t reg_addr, uint8_t *value )
{
	volatile nrfx_err_t ret;
	volatile uint8_t read_data = 0;
	volatile uint8_t reset_val = BLK_SEL_DEFAULT_ADDR;
	
	/* BLK_SEL_Rレジスタへバンクを指定 */
	ret = SpiIOWrite( ICM42607_BLK_SEL_R, (uint8_t *)&blk_sel, sizeof( blk_sel ) );
	SPI_ERR_CHECK( ret, __LINE__ );
	if ( ret != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_BLK_SEL_R, blk_sel, __LINE__ );
		return ret;
	}
	/* MADDR_Rレジスタへ書き込みを実行するレジスタアドレスを指定 */
	ret = SpiIOWrite( ICM42607_MADDR_R, (uint8_t *)&reg_addr, sizeof( reg_addr ) );
	SPI_ERR_CHECK( ret, __LINE__ );
	if ( ret != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_MADDR_R, reg_addr, __LINE__ );
		return ret;
	}
	/* Write完了待ち(10us) */
	nrf_delay_us( MREG_WAIT_APPLY_TIME );

	/* M_Rレジスタへ書き込みを実行 */
	ret = SpiIORead( ICM42607_M_R, (uint8_t *)&read_data, sizeof( read_data ) );
	SPI_ERR_CHECK( ret, __LINE__ );
	if ( ret != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_M_R, read_data, __LINE__ );
		return ret;
	}
	/* Write完了待ち(10us) */
	nrf_delay_us( MREG_WAIT_APPLY_TIME );
	
	if ( value != NULL )
	{
		/* 読み出したデータを格納 */
		*value = read_data;
	}
	
	/* 書き込み完了後は、BLK_SEL_Rを0x00に戻す */
	ret = SpiIOWrite( ICM42607_BLK_SEL_R, (uint8_t *)&reset_val, sizeof( reset_val ) );
	SPI_ERR_CHECK( ret, __LINE__ );
	if ( ret != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_BLK_SEL_R, reset_val, __LINE__ );
	}
	
	return ret;
}

/**
 * @brief ACC/Gyro Timestamp config
 * @param value TMST_RESとTMST_ENの設定値
 * @retval NRF_SUCCESS Success
 * @retval ACC_GYRO_SETEUP_ERROR Setup Error
 */
static nrfx_err_t setup_tmst_config( uint8_t value )
{
	volatile nrfx_err_t ret;
	volatile uint8_t read_data = 0;
	
	/* MREG1 Bankを指定しTMST_CONFIG1からデータ読み出し */
	ret = setup_mreg_read( ICM42607_MREG1, ICM42607_MREG1_TMST_CONFIG1, (void *)&read_data );
	SPI_ERR_CHECK( ret, __LINE__ );
	if ( ret != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_MREG1_TMST_CONFIG1, read_data, __LINE__ );
		return ret;
	}
	
	read_data &= ~TMST_CONFIG1_MASK;
	read_data |= value;
	
	/* TMST_CONFIG1への書き込み */
	ret = setup_mreg_write( ICM42607_MREG1, ICM42607_MREG1_TMST_CONFIG1, read_data );
	SPI_ERR_CHECK( ret, __LINE__ );
	if ( ret != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_MREG1_TMST_CONFIG1, read_data, __LINE__ );
		return ret;
	}
	
	return ret;
}

/**
 * @brief ACC/Gyro FIFO Configuration
 * @param fifo_tmst_fsync_en Timestamp FSYNC Setting
 * @param fifo_acc_en FIFO_ACCEL_EN Setting
 * @param fifo_gyro_en FIFO_GYRO_EN Setting
 * @retval NRF_SUCCESS Success
 * @retval ACC_GYRO_SETEUP_ERROR Setup Error
 */
static nrfx_err_t setup_fifo_config( uint8_t fifo_tmst_fsync_en, uint8_t fifo_acc_en, uint8_t fifo_gyro_en )
{
	volatile nrfx_err_t ret;
	volatile uint8_t read_data = 0;
	
	/* MREG1 Bankを指定しTMST_CONFIG1からデータ読み出し */
	ret = setup_mreg_read( ICM42607_MREG1, ICM42607_MREG1_FIFO_CONFIG5, (void *)&read_data );
	SPI_ERR_CHECK( ret, __LINE__ );
	if ( ret != NRF_SUCCESS )
	{
		return ret;
	}
	
	read_data &= ~FIFO_CONFIG5_MASK;
	read_data |= ( ( fifo_tmst_fsync_en << FIFO_CONFIG5_TMST_FSYNC_POS ) | ( fifo_gyro_en << FIFO_CONFIG5_GYRO_EN_POS ) | fifo_acc_en | 0x20 );
	
	/* TMST_CONFIG1への書き込み */
	ret = setup_mreg_write( ICM42607_MREG1, ICM42607_MREG1_FIFO_CONFIG5, read_data );
	SPI_ERR_CHECK( ret, __LINE__ );
	
	return ret;
}

/**
 * @brief ACC/Gyro Interrupt PIN Configuration
 * @param None
 * @retval NRF_SUCCESS Success
 * @retval ACC_GYRO_SETEUP_ERROR Setup Error
 */
static nrfx_err_t setup_acc_gyro_int_pin( void )
{
	volatile nrfx_err_t ret;
	volatile uint8_t write_data = 0;

	/* INT1/INT2 Setting Latched Mode and Active high */
	write_data = INT_CONFIG_VALUE;
	
	ret = SpiIOWrite( ICM42607_INT_CONFIG, (uint8_t *)&write_data, sizeof( write_data ) );
	SPI_ERR_CHECK( ret, __LINE__ );
	if ( ret != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_INT_CONFIG, write_data, __LINE__ );
	}
	
	return ret;
}

/**
 * @brief setup software reset
 * @param Noen
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 Failed
 */
static nrfx_err_t setup_acc_gyro_fifo_flush( void )
{
	volatile nrfx_err_t ret = 0;
	volatile uint8_t fifo_flush = ACC_GYRO_FIFO_FLUSH;
	volatile uint8_t read_data = 0;
	uint8_t i;

	ret = SpiIOWrite( ICM42607_SIGNAL_PATH_RESET, (uint8_t *)&fifo_flush, sizeof( fifo_flush ) );
	SPI_ERR_CHECK( ret, __LINE__ );
	if ( ret != NRF_SUCCESS )
	{
		return ACC_GYRO_SETEUP_ERROR;
	}
	
	/* Software Reset Wait */
	for ( i = 0; i < WAIT_FIFO_FLUSH_RETRY; i++ )
	{
		/* 2us待ち */
		nrf_delay_us( WAIT_FIFO_FLUSH_INTERVAL );
		ret = SpiIORead( ICM42607_SIGNAL_PATH_RESET, (uint8_t *)&read_data, sizeof( read_data ) );
		SPI_ERR_CHECK( ret, __LINE__ );
		if ( ret == NRF_SUCCESS )
		{
			if ( ( read_data & ACC_GYRO_FIFO_FLUSH_MASK ) == 0 )
			{
				break;
			}
		}
		else
		{
			break;
		}
	}
	
	if ( i == WAIT_FIFO_FLUSH_RETRY )
	{
		DEBUG_LOG( LOG_ERROR, "!!! ICM42607 FIFO Flush Clear Error !!!" );
		ret = ACC_GYRO_SETEUP_ERROR;
	}
	
	return ret;
}

/**
 * @brief ACC/Gyro Interrupt WOM_CONFIG
 * @param value Wake on Motion Value
 * @retval NRF_SUCCESS Success
 * @retval ACC_GYRO_SETEUP_ERROR Setup Error
 */
static nrfx_err_t setup_acc_gyro_wom_config( uint8_t value )
{
	volatile nrfx_err_t ret;
	volatile uint8_t read_data = 0;

	ret = SpiIORead( ICM42607_WOM_CONFIG, (uint8_t *)&read_data, sizeof( read_data ) );
	SPI_ERR_CHECK( ret, __LINE__ );
	if ( ret != NRF_SUCCESS )
	{
		return ret;
	}

	read_data &= (uint8_t)~WOM_CONFIG_MASK;
	read_data |= value;
	
	ret = SpiIOWrite( ICM42607_WOM_CONFIG, (uint8_t *)&read_data, sizeof( read_data ) );
	SPI_ERR_CHECK( ret, __LINE__ );
	if ( ret != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_WOM_CONFIG, read_data, __LINE__ );
	}
	
	return ret;
}

/**
 * @brief ACC/Gyro ACCEL Configarution
 * @param value ACCEL Config Value
 * @retval NRF_SUCCESS Success
 * @retval ACC_GYRO_SETEUP_ERROR Setup Error
 */
static nrfx_err_t setup_acc_config( uint8_t value )
{
	volatile nrfx_err_t ret;
	volatile uint8_t read_data = 0;

	ret = SpiIORead( ICM42607_ACC_CONFIG1, (uint8_t *)&read_data, sizeof( read_data ) );
	SPI_ERR_CHECK( ret, __LINE__ );
	if ( ret != NRF_SUCCESS )
	{
		return ret;
	}

	read_data &= (uint8_t)~ACCEL_CONFIOG_MASK;
	read_data |= value;
	
	ret = SpiIOWrite( ICM42607_ACC_CONFIG1, (uint8_t *)&read_data, sizeof( read_data ) );
	SPI_ERR_CHECK( ret, __LINE__ );
	if ( ret != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_ACC_CONFIG1, read_data, __LINE__ );
	}
	
	return ret;
}

/**
 * @brief ACC/Gyro OTP Configarution
 * @param value OTP Config Value
 * @retval NRF_SUCCESS Success
 * @retval ACC_GYRO_SETEUP_ERROR Setup Error
 */
static nrfx_err_t setup_otp_config( uint8_t value )
{
	volatile nrfx_err_t ret;
	volatile uint8_t read_data = 0;
	
	/* MREG1 Bankを指定しOTP_CONFIGからデータ読み出し */
	ret = setup_mreg_read( ICM42607_MREG1, ICM42607_MREG1_OTP_CONFIG, (void *)&read_data );
	SPI_ERR_CHECK( ret, __LINE__ );
	if ( ret != NRF_SUCCESS )
	{
		return ret;
	}
	
	read_data &= ~OTP_CONFIG_MASK;
	read_data |= value;
	
	/* OTP_CONFIGへの書き込み */
	ret = setup_mreg_write( ICM42607_MREG1, ICM42607_MREG1_OTP_CONFIG, read_data );
	SPI_ERR_CHECK( ret, __LINE__ );

	return ret;
}

/**
 * @brief ACC/Gyro OTP Configarution 7
 * @param value OPT_CTRL7 Config Value
 * @retval NRF_SUCCESS Success
 * @retval ACC_GYRO_SETEUP_ERROR Setup Error
 */
static nrfx_err_t setup_otp_ctrl7( uint8_t value )
{
	volatile nrfx_err_t ret;
	volatile uint8_t read_data = 0;
	
	/* MREG2 Bankを指定しOTP_CTRL7からデータ読み出し */
	ret = setup_mreg_read( ICM42607_MREG2, ICM42607_MREG2_OTP_CTRL7, (void *)&read_data );
	SPI_ERR_CHECK( ret, __LINE__ );
	if ( ret != NRF_SUCCESS )
	{
		return ret;
	}
	
	read_data &= ~OTP_CTRL7_MASK;
	read_data |= value;
	
	/* OTP_CTRL7への書き込み */
	ret = setup_mreg_write( ICM42607_MREG2, ICM42607_MREG2_OTP_CTRL7, read_data );
	SPI_ERR_CHECK( ret, __LINE__ );

	return ret;
}


/*******************************************************************************
 * Acc/Gyro Setting Functions
 *******************************************************************************/

/**
 * @brief ACC/Gyro FIFO Read Data
 * @param acc_gyro_info ACC/Gyro Store Data
 * @retval ACC_GYRO_DATA_COMPLETE FIFO Ready
 * @retval ACC_GYRO_DATA_NOT_COMPLETE FIFO Not Ready
 * @retval 上記以外 Error
 */
static uint32_t reader_acc_gyro_data( ACC_GYRO_DATA_INFO *acc_gyro_info )
{
	volatile nrfx_err_t err_code;
	volatile uint8_t fifo_data[FIFO_OUT_SIZE] = {0};
	volatile uint8_t current_mode;
	
	/* Read Tags + FIFO_OUT_DATA (Tag 1byte + Data 6byte) */
	err_code = SpiIORead( ICM42607_FIFO_DATA, (uint8_t *)&fifo_data[0], sizeof( fifo_data ) );
	SPI_ERR_CHECK( err_code, __LINE__ );
	if ( ( err_code == NRF_SUCCESS ) && ( fifo_data[0] != HEADER_FIFO_EMPTY ) )
	{
		acc_gyro_info->acc_x_data	= ( fifo_data[2] << 8 ) | fifo_data[1];
		acc_gyro_info->acc_y_data	= ( fifo_data[4] << 8 ) | fifo_data[3];
		acc_gyro_info->acc_z_data	= ( fifo_data[6] << 8 ) | fifo_data[5];
		acc_gyro_info->gyro_x_data	= ( fifo_data[8] << 8 ) | fifo_data[7];
		acc_gyro_info->gyro_y_data	= ( fifo_data[10] << 8 ) | fifo_data[9];
		acc_gyro_info->gyro_z_data	= ( fifo_data[12] << 8 ) | fifo_data[11];
		/* 2022.03.30 Modify TimestampだけLittle Endianではなくbig endianになっているため修正 */
		acc_gyro_info->timestamp	= ( fifo_data[15] << 8 ) | fifo_data[14];
		acc_gyro_info->temperature	= fifo_data[13];
		acc_gyro_info->header		= fifo_data[0];
		/* 現在のモードを取得 */
		get_acc_gyro_mode( (void *)&current_mode );
		if ( ( current_mode == MODE_GYRO_TEMP ) || ( current_mode == MODE_BOTH ) )
		{
			/* ACC Only以外の際にGyroデータを確認する */
			if ( ( acc_gyro_info->gyro_x_data != GYRO_INVALID_DATA ) &&
				 ( acc_gyro_info->gyro_y_data != GYRO_INVALID_DATA ) &&
				 ( acc_gyro_info->gyro_z_data != GYRO_INVALID_DATA ) )
			{
				/* GYROデータが正常のデータの場合 */
				err_code = ACC_GYRO_DATA_COMPLETE;
			}
			/* Gyroモードの場合、ACCに無効な値が入っているため0を代入 */
			if ( current_mode == MODE_GYRO_TEMP )
			{
				acc_gyro_info->acc_x_data = 0;
				acc_gyro_info->acc_y_data = 0;
				acc_gyro_info->acc_z_data = 0;
			}
		}
		else
		{
			/* ACC Onlyの場合は、Gyro Dataを無効な値に設定する */
			acc_gyro_info->gyro_x_data = 0;
			acc_gyro_info->gyro_y_data = 0;
			acc_gyro_info->gyro_z_data = 0;
			/* ACC Only */
			err_code = ACC_GYRO_DATA_COMPLETE;
		}
	}
	
	return err_code;
}

/**
 * @brief ACC/Gyro FIFO Non Read Data Number
 * @param fifo_count FIFO Number
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 Failed
 */
static nrfx_err_t get_acc_gyro_fifo_count( uint16_t *fifo_count )
{
	volatile nrfx_err_t err_code = 0;
	volatile uint8_t read_data[2] = {0};
	volatile uint16_t temp_data = 0;
	volatile uint8_t int_status = 0;

	/* INT Clear */
	err_code = SpiIORead( ICM42607_INT_STATUS, (uint8_t *)&int_status, sizeof( int_status ) );
	SPI_ERR_CHECK( err_code, __LINE__ );
	
	/* Read FIFO Count-L */
	err_code = SpiIORead( ICM42607_FIFO_COUNTH, (uint8_t *)&read_data, sizeof( read_data ) );
	SPI_ERR_CHECK( err_code, __LINE__ );
	if ( err_code == NRF_SUCCESS )
	{
		//DEBUG_LOG( LOG_INFO, "read_data[0]=%d, read_data[1]=%d", read_data[0], read_data[1] );
		temp_data = read_data[0] | read_data[1] << 8;
		/* Packet Sizeからデータ個数を算出 */
		*fifo_count = temp_data / ACC_GYRO_FIFO_PACKET_SIZE;
	}

	return err_code;
}

/**
 * @brief ACC/Gyro Read Device ID
 * @param None
 * @retval NRF_SUCCESS Success
 * @retval ACC_GYRO_SETEUP_ERROR Setup Error
 */
static nrfx_err_t acc_gyro_read_device_id( void )
{
	nrfx_err_t err_code;
	uint8_t device_id = 0;

	/* Device ID 読み出し */
	err_code = SpiIORead( ICM42607_WHO_AM_I, &device_id, sizeof( device_id ) );
	if ( err_code != NRF_SUCCESS )
	{
		return ACC_GYRO_SETEUP_ERROR;
	}
	
	if ( device_id != ICM42607_DEVICE_ID )
	{
		err_code = ACC_GYRO_SETEUP_ERROR;
		DEBUG_LOG( LOG_ERROR, "Acc/Gyro Device ID Failed : 0x%x", device_id );
	}
	
	return err_code;

}

/**
 * @brief ACC/Gyro Power Down Mode Change
 * @param None
 * @retval NRF_SUCCESS Success
 * @retval ACC_GYRO_SETEUP_ERROR Setup Error
 */
static nrfx_err_t acc_gyro_power_down_mode( void )
{
	nrfx_err_t err_code;

	/* ACC/Gyro ODR Power Down */
	err_code = setup_acc_gyro_pwr_mgmt0( PWR_ACC_TURN_OFF, PWR_GYRO_TURN_OFF );
	if ( err_code == NRF_SUCCESS )
	{
		/* モード変更時には100ms待ち実行 */
		nrf_delay_ms( WAIT_MODE_CHANGE_INTERVAL );
	}
	else
	{
		DEBUG_LOG( LOG_INFO, "!!! ICM42607 Power Down Failed !!!" );
		err_code = ACC_GYRO_SETEUP_ERROR;
	}
	
	return err_code;
}

/**
 * @brief ACC/Gyro Sensor Software Reset
 * @param None
 * @retval NRF_SUCCESS Success
 * @retval ACC_GYRO_SETEUP_ERROR Setup Error
 */
static nrfx_err_t acc_gyro_softreset( void )
{
	nrfx_err_t err_code;
	uint16_t i;
	uint8_t read_data = 0;
	
	/* Software Reset実行 */
	err_code = setup_acc_gyro_software_reset();
	if ( err_code != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_SIGNAL_PATH_RESET, ACC_GYRO_SW_RESET_VAL, __LINE__ );
		return ACC_GYRO_SETEUP_ERROR;
	}
	
	/* Software Reset Wait */
	for ( i = 0; i < WAIT_SW_RESET_RETRY; i++ )
	{
		/* 500us待ち */
		nrf_delay_us( WAIT_SW_RESET_INTERVAL );
		err_code = SpiIORead( ICM42607_INT_STATUS, &read_data, sizeof( read_data ) );
		SPI_ERR_CHECK( err_code, __LINE__ );
		if ( err_code == NRF_SUCCESS )
		{
			if ( ( read_data & ACC_GYRO_SW_RESET_MASK ) == 0 )
			{
				break;
			}
		}
		else
		{
			break;
		}
	}
	
	if ( i == WAIT_SW_RESET_RETRY )
	{
		DEBUG_LOG( LOG_INFO, "!!! ICM42607 Software Reset Failed !!!" );
		err_code = ACC_GYRO_SETEUP_ERROR;
	}
	
	return err_code;
}

/**
 * @brief ACC/Gyro OTP Reload
 * @param None
 * @retval NRF_SUCCESS Success
 * @retval ACC_GYRO_SETEUP_ERROR Setup Error
 */
static nrfx_err_t acc_gyro_otp_reload( void )
{
	nrfx_err_t err_code;
	
	/* IDLE状態へ移行させる */
	err_code = setup_acc_gyro_idle_ctrl( PWR_MGMT_RC_ON );
	SPI_ERR_CHECK( err_code, __LINE__ );
	if ( err_code != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_PWR_MGMT0, PWR_MGMT_RC_ON, __LINE__ );
		return ACC_GYRO_SETEUP_ERROR;
	}
	
	/* 10ms wait */
	nrf_delay_ms( IDLE_WAIT_TIME );

	/* OTP_CONFIG */
	err_code = setup_otp_config( OTP_CONFIG_VALUE );
	if ( err_code != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_MREG1_OTP_CONFIG, OTP_CONFIG_VALUE, __LINE__ );
		return ACC_GYRO_SETEUP_ERROR;
	}
	
	/* OTP_CTRL7 Setting */
	err_code = setup_otp_ctrl7( OTP_CTRL7_SETTING_VAL );
	if ( err_code != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_MREG2_OTP_CTRL7, OTP_CTRL7_SETTING_VAL, __LINE__ );
		return ACC_GYRO_SETEUP_ERROR;
	}
	
	/* Sleep 300us */
	nrf_delay_us( WAIT_OTP_RELOAD_TIME );

	/* OTP_CTRL7 Reload */
	err_code = setup_otp_ctrl7( OTP_CTRL7_RESET_VAL );
	if ( err_code != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_MREG2_OTP_CTRL7, OTP_CTRL7_RESET_VAL, __LINE__ );
		return ACC_GYRO_SETEUP_ERROR;
	}

	/* Sleep 300us */
	nrf_delay_us( 280 );

	return err_code;
}

/**
 * @brief ACC/Gyro ODR and Full-scale Setting
 * @param None
 * @retval NRF_SUCCESS Success
 * @retval ACC_GYRO_SETEUP_ERROR Setup Error
 */
static nrfx_err_t acc_gyro_odr_fsr( void )
{
	nrfx_err_t err_code;

	/* IDLE状態へ移行させる */
	err_code = setup_acc_gyro_idle_ctrl( PWR_MGMT_RC_ON );
	SPI_ERR_CHECK( err_code, __LINE__ );
	if ( err_code != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_PWR_MGMT0, PWR_MGMT_RC_ON, __LINE__ );
		return ACC_GYRO_SETEUP_ERROR;
	}
	
	/* 10ms wait */
	nrf_delay_ms( IDLE_WAIT_TIME );

	/* 割り込みPIN設定 */
	err_code = setup_acc_gyro_int_pin();
	if ( err_code != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_INT_CONFIG, INT_CONFIG_VALUE, __LINE__ );
		return ACC_GYRO_SETEUP_ERROR;
	}

	/* Gyro ODR/full-scale Setting */
	err_code = setup_acc_gyro_sensor_odr_fss( ICM42607_GYRO_CONFIG0, GYRO_ODR_100HZ, GYRO_FSS_2000DPS );
	if ( err_code != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_GYRO_CONFIG0, GYRO_ODR_100HZ | GYRO_FSS_2000DPS, __LINE__ );
		return ACC_GYRO_SETEUP_ERROR;
	}

	/* ACC ODR/full-scale Setting */
	err_code = setup_acc_gyro_sensor_odr_fss( ICM42607_ACC_CONFIG0, ACC_ODR_100HZ, ACC_FS_SEL_16G );
	if ( err_code != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_ACC_CONFIG0, ACC_ODR_100HZ | ACC_FS_SEL_16G, __LINE__ );
		return ACC_GYRO_SETEUP_ERROR;
	}

	/* ACCEL_CONFIG 2x average and Low pass filter bypassed */
	err_code = setup_acc_config( ACCEL_CONFIOG_VAL );
	if ( err_code != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_ACC_CONFIG1, ACCEL_CONFIOG_VAL, __LINE__ );
		return ACC_GYRO_SETEUP_ERROR;
	}
	
	return err_code;

}

/**
 * @brief ACC/Gyro FIFO Config
 * @param None
 * @retval NRF_SUCCESS Success
 * @retval ACC_GYRO_SETEUP_ERROR Setup Error
 */
static nrfx_err_t acc_gyro_fifo_config( void )
{
	nrfx_err_t err_code;

	/* FIFO Report Format Setting */
	err_code = setup_acc_gyro_fifo_format();
	if ( err_code != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_INTF_CONFIG0, ACC_GYRO_FIFO_FORMAT_VAL, __LINE__ );
		return ACC_GYRO_SETEUP_ERROR;
	}
	/* FIFO watermark Setting Watermark Count
	 *  起動時はACCのみ動作させるためWaterMarkは設定値の1/2を設定する
	 */
	err_code = setup_acc_gyro_fifo_wtm( ACC_GYRO_WTM_COUNT );
	if ( err_code != NRF_SUCCESS )
	{
		return ACC_GYRO_SETEUP_ERROR;
	}

	/* Timestamp Configuration */
	err_code = setup_tmst_config( TMST_CONFIG1_RESOLUTION | TIMESTAMP_REG_ENABLE );
	if ( err_code != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_MREG1_TMST_CONFIG1, TMST_CONFIG1_RESOLUTION | TMST_CONFIG1_ENABLE, __LINE__ );
		return ACC_GYRO_SETEUP_ERROR;
	}
	
	/* FIFO Configuration */
	err_code = setup_fifo_config( FIFO_TMST_FSYNC_DISABLE, FIFO_ACC_ENABLE, FIFO_GYRO_ENABLE );
	if ( err_code != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_MREG1_FIFO_CONFIG5, FIFO_ACC_ENABLE | FIFO_GYRO_ENABLE, __LINE__ );
		return ACC_GYRO_SETEUP_ERROR;
	}

	/* FIFO Mode Setting */
	err_code = setup_acc_gyro_fifo_mode( FIFO_BYPASS );
	if ( err_code != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_FIFO_CONFIG1, FIFO_NOT_BYPASS, __LINE__ );
		return ACC_GYRO_SETEUP_ERROR;
	}
	err_code = setup_acc_gyro_fifo_mode( FIFO_NOT_BYPASS | MODE_STREAM );
	if ( err_code != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_FIFO_CONFIG1, FIFO_NOT_BYPASS, __LINE__ );
		return ACC_GYRO_SETEUP_ERROR;
	}

	/* FIFO Flush */
	err_code = setup_acc_gyro_fifo_flush();
	if ( err_code != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_SIGNAL_PATH_RESET, ACC_GYRO_FIFO_FLUSH, __LINE__ );
		return ACC_GYRO_SETEUP_ERROR;
	}

	return err_code;
}

/**
 * @brief ACC/Gyro INT1/INT2 Interrupt Setting
 * @param None
 * @retval NRF_SUCCESS Success
 * @retval ACC_GYRO_SETEUP_ERROR Setup Error
 */
static nrfx_err_t acc_gyro_interrupt_ctrl_set( void )
{
	nrfx_err_t err_code;

	/* 割り込み設定(INT_SOURCE0) */
	err_code = setup_acc_gyro_interrupt( ICM42607_INT_SOURCE0, FIFO_THS_INT1_EN | FIFO_FULL_INT1_EN );
	if ( err_code != NRF_SUCCESS )
	{
		return ACC_GYRO_SETEUP_ERROR;
	}
	/* IDLE状態をOFF */
	err_code = setup_acc_gyro_idle_ctrl( PWR_MGMT_RC_OFF );
	SPI_ERR_CHECK( err_code, __LINE__ );
	if ( err_code != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_PWR_MGMT0, PWR_MGMT_RC_OFF, __LINE__ );
		return ACC_GYRO_SETEUP_ERROR;
	}
	/* Setup INT1 GPIO Interrupt */
	setup_acc_gyro_gpio_pin_init();
	
	return err_code;
}

/**
 * @brief ACC/Gyro INT1 Wake On Motion Threshold Setting
 * @param None
 * @retval NRF_SUCCESS Success
 * @retval ACC_GYRO_SETEUP_ERROR Setup Error
 */
static nrfx_err_t acc_gyro_wom_setting( void )
{
	nrfx_err_t err_code;
	
	/* IDLE状態へ移行させる */
	err_code = setup_acc_gyro_idle_ctrl( PWR_MGMT_RC_ON );
	SPI_ERR_CHECK( err_code, __LINE__ );
	if ( err_code != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_PWR_MGMT0, PWR_MGMT_RC_ON, __LINE__ );
		return ACC_GYRO_SETEUP_ERROR;
	}

	/* ACCEL_WOM_Y_THR : 1000mg (255/256 = 0.99609g (996.1mg)) */
	err_code = setup_acc_gyro_wakeup_ths( ICM42607_MREG1_ACCEL_WOM_X_THR, WOM_X_AXIS_THR );
	if ( err_code != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_MREG1_ACCEL_WOM_Y_THR, WOM_Y_AXIS_THR, __LINE__ );
		return ACC_GYRO_SETEUP_ERROR;
	}

	err_code = setup_acc_gyro_wakeup_ths( ICM42607_MREG1_ACCEL_WOM_Y_THR, WOM_Y_AXIS_THR );
	if ( err_code != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_MREG1_ACCEL_WOM_Y_THR, WOM_Y_AXIS_THR, __LINE__ );
		return ACC_GYRO_SETEUP_ERROR;
	}
	/* 2022.04.19 Add Z-Axis 閾値追加 ++ */
	err_code = setup_acc_gyro_wakeup_ths( ICM42607_MREG1_ACCEL_WOM_Z_THR, WOM_Z_AXIS_THR );
	if ( err_code != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_MREG1_ACCEL_WOM_Y_THR, WOM_Y_AXIS_THR, __LINE__ );
		return ACC_GYRO_SETEUP_ERROR;
	}
	/* 2022.04.19 Add Z-Axis 閾値追加 -- */
	
	/* IDLE状態をOFF */
	err_code = setup_acc_gyro_idle_ctrl( PWR_MGMT_RC_OFF );
	SPI_ERR_CHECK( err_code, __LINE__ );
	if ( err_code != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_PWR_MGMT0, PWR_MGMT_RC_OFF, __LINE__ );
		return ACC_GYRO_SETEUP_ERROR;
	}
	
	/* WOM_CONFIG wake on motion enable */
	err_code = setup_acc_gyro_wom_config( WOM_MODE_DIFFERENTIAL );
	SPI_ERR_CHECK( err_code, __LINE__ );
	if ( err_code != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_WOM_CONFIG, WOM_MODE_DIFFERENTIAL, __LINE__ );
		return ACC_GYRO_SETEUP_ERROR;
	}
	err_code = setup_acc_gyro_wom_config( WOM_MODE_ENABLE );
	SPI_ERR_CHECK( err_code, __LINE__ );
	if ( err_code != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_WOM_CONFIG, WOM_MODE_ENABLE, __LINE__ );
		return ACC_GYRO_SETEUP_ERROR;
	}

	return err_code;
}

/**
 * @brief ACC/Gyro Reconfig Gyro/Temp Mode Change
 * @param wtm watermark
 * @param acc_odr ACC FIFO ODR
 * @param gyro_odr Gyro ODR
 * @param temp_odr Temp ODR
 * @retval NRF_SUCCESS Success
 * @retval ACC_GYRO_SETEUP_ERROR Setup Error
 */
static nrfx_err_t acc_gyro_exec_mode_set( void )
{
	nrfx_err_t err_code;
	
	err_code = setup_acc_gyro_pwr_mgmt0( PWR_ACC_LP_MODE, PWR_GYRO_TURN_OFF );
	if ( err_code != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_PWR_MGMT0, PWR_ACC_LP_MODE | PWR_GYRO_TURN_OFF, __LINE__ );
	}
	else
	{
		/* モード変更時には100ms待ち実行 */
		nrf_delay_ms( WAIT_MODE_CHANGE_INTERVAL );
	}
	
	return err_code;
}

/**
 * @brief ACC/Gyro PowerOff FIFO Mode
 * @param None
 * @retval NRF_SUCCESS Success
 * @retval ACC_GYRO_SETEUP_ERROR Setup Error
 */
static nrfx_err_t acc_gyro_poweroff_fifo_config( void )
{
	nrfx_err_t err_code;

	/* FIFO Disable bypassモードに変更 */
	err_code = setup_acc_gyro_fifo_mode( FIFO_BYPASS );
	if ( err_code != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_FIFO_CONFIG1, FIFO_BYPASS, __LINE__ );
		return ACC_GYRO_SETEUP_ERROR;
	}
	err_code = setup_acc_gyro_fifo_mode( FIFO_NOT_BYPASS | MODE_STREAM );
	if ( err_code != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_FIFO_CONFIG1, FIFO_BYPASS, __LINE__ );
		return ACC_GYRO_SETEUP_ERROR;
	}
	/* FIFO Flush */
	err_code = setup_acc_gyro_fifo_flush();
	if ( err_code != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_SIGNAL_PATH_RESET, ACC_GYRO_FIFO_FLUSH, __LINE__ );
		return ACC_GYRO_SETEUP_ERROR;
	}
	
	return err_code;

}

/**
 * @brief ACC/Gyro PowerOff Wakeup Setting
 * @param None
 * @retval NRF_SUCCESS Success
 * @retval ACC_GYRO_SETEUP_ERROR Setup Error
 */
static nrfx_err_t acc_gyro_poweroff_wakeup_set( void )
{
	nrfx_err_t err_code = 0;
	
	/* IDLE状態へ移行させる */
	err_code = setup_acc_gyro_idle_ctrl( PWR_MGMT_RC_ON );
	SPI_ERR_CHECK( err_code, __LINE__ );
	if ( err_code != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_PWR_MGMT0, PWR_MGMT_RC_ON, __LINE__ );
		return ACC_GYRO_SETEUP_ERROR;
	}
	
	/* FIFO Configuration */
	err_code = setup_fifo_config( FIFO_TMST_FSYNC_DISABLE, FIFO_ACC_DISABLE, FIFO_GYRO_DISABLE );
	if ( err_code != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_MREG1_FIFO_CONFIG5, FIFO_ACC_ENABLE | FIFO_GYRO_ENABLE, __LINE__ );
		return ACC_GYRO_SETEUP_ERROR;
	}

	/* IDLE状態をOFF */
	err_code = setup_acc_gyro_idle_ctrl( PWR_MGMT_RC_OFF );
	SPI_ERR_CHECK( err_code, __LINE__ );
	if ( err_code != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_PWR_MGMT0, PWR_MGMT_RC_OFF, __LINE__ );
		return ACC_GYRO_SETEUP_ERROR;
	}

	err_code = setup_acc_gyro_fifo_mode( FIFO_BYPASS );
	if ( err_code != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_FIFO_CONFIG1, FIFO_BYPASS, __LINE__ );
		return ACC_GYRO_SETEUP_ERROR;
	}
	/* 割り込みPIN設定 */
	err_code = setup_acc_gyro_int_pin();
	if ( err_code != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_INT_CONFIG, INT_CONFIG_VALUE, __LINE__ );
		return ACC_GYRO_SETEUP_ERROR;
	}

	/* 割り込み設定(INT_SOURCE1) Wake On Motion X or Y */
	err_code = setup_acc_gyro_interrupt( ICM42607_INT_SOURCE1, INT1_WOM_X_EN | INT1_WOM_Y_EN );
	if ( err_code != NRF_SUCCESS )
	{
		return ACC_GYRO_SETEUP_ERROR;
	}
	/* WOM Threshold Setting */
	err_code = acc_gyro_wom_setting();
	if ( err_code != NRF_SUCCESS )
	{
		return ACC_GYRO_SETEUP_ERROR;
	}

	/* ODR Setting */
	/* ACC ODR/full-scale Setting 12.5Hz 16g */
	err_code = setup_acc_gyro_sensor_odr_fss( ICM42607_ACC_CONFIG0, ACC_ODR_12_5HZ, ACC_FS_SEL_16G );
	if ( err_code != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_ACC_CONFIG0, ACC_ODR_12_5HZ | ACC_FS_SEL_16G, __LINE__ );
		return ACC_GYRO_SETEUP_ERROR;
	}

	/* ACCEL_CONFIG 2x average and 180Hz */
	err_code = setup_acc_config( ACCEL_CONFIOG_VAL );
	if ( err_code != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_ACC_CONFIG1, ACCEL_CONFIOG_VAL, __LINE__ );
		return ACC_GYRO_SETEUP_ERROR;
	}

	/* Low Power Mode Start */
	err_code = acc_gyro_exec_mode_set();
	if ( err_code != NRF_SUCCESS )
	{
		return ACC_GYRO_SETEUP_ERROR;
	}
	
	return err_code;
}

/**
 * @brief ACC/Gyro Reconfig Interrupt Setting
 * @param value Interrupt Setting Value
 * @retval NRF_SUCCESS Success
 * @retval ACC_GYRO_SETEUP_ERROR Setup Error
 */
static nrfx_err_t acc_gyro_reconfig_interrupt( uint8_t value )
{
	nrfx_err_t err_code = 0;
	volatile uint8_t int_status = 0;

	/* 割り込みを設定 */
	err_code = setup_acc_gyro_interrupt( ICM42607_INT_SOURCE0, value );
	if ( err_code != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_INT_SOURCE1, INT1_NONE, __LINE__ );
		return err_code;
	}
	
	/* 割り込みクリア */
	err_code = SpiIORead( ICM42607_INT_STATUS, (uint8_t *)&int_status, sizeof( int_status ) );
	SPI_ERR_CHECK( err_code, __LINE__ );

	return err_code;
}

/**
 * @brief ACC/Gyro Reconfig Gyro/Temp Mode Change
 * @param acc_mode ACC Mode
 * @param gyro_mode Gyro Mode
 * @retval NRF_SUCCESS Success
 * @retval ACC_GYRO_SETEUP_ERROR Setup Error
 */
static nrfx_err_t reconfig_operation_mode( uint8_t acc_mode, uint8_t gyro_mode )
{
	nrfx_err_t err_code;

	/* ODR Power Down */
	err_code = acc_gyro_power_down_mode();
	if ( err_code != NRF_SUCCESS )
	{
		return err_code;
	}
	/* IDLE状態へ移行させる */
	err_code = setup_acc_gyro_idle_ctrl( PWR_MGMT_RC_ON );
	SPI_ERR_CHECK( err_code, __LINE__ );
	if ( err_code != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_PWR_MGMT0, PWR_MGMT_RC_ON, __LINE__ );
		return ACC_GYRO_SETEUP_ERROR;
	}
	
	/* 10ms wait */
	nrf_delay_ms( IDLE_WAIT_TIME );
	
	/* 割り込みを一度無効に設定 */
	err_code = acc_gyro_reconfig_interrupt( INT1_NONE );
	if ( err_code != NRF_SUCCESS )
	{
		return err_code;
	}

	/* 再度、割り込みを有効に設定 */
	err_code = acc_gyro_reconfig_interrupt( FIFO_THS_INT1_EN | FIFO_FULL_INT1_EN );
	if ( err_code != NRF_SUCCESS )
	{
		return err_code;
	}
	
	/* FIFO Power Down */
	err_code = acc_gyro_poweroff_fifo_config();
	if ( err_code != NRF_SUCCESS )
	{
		return err_code;
	}
	
	/* IDLE状態をOFF */
	err_code = setup_acc_gyro_idle_ctrl( PWR_MGMT_RC_OFF );
	SPI_ERR_CHECK( err_code, __LINE__ );
	if ( err_code != NRF_SUCCESS )
	{
		register_setup_error_log( ICM42607_PWR_MGMT0, PWR_MGMT_RC_OFF, __LINE__ );
		return ACC_GYRO_SETEUP_ERROR;
	}

	/* ACC/Gyro Mode On */
	err_code = setup_acc_gyro_pwr_mgmt0( acc_mode, gyro_mode );
	if ( err_code == NRF_SUCCESS )
	{
		/* Reconfig時は、100ms待ち */
		nrf_delay_ms( WAIT_RECONFIG_MODE_CHANGE );
	}
	else
	{
		register_setup_error_log( ICM42607_PWR_MGMT0, acc_mode | gyro_mode, __LINE__ );
		err_code = ACC_GYRO_SETEUP_ERROR;
	}
	
	return err_code;
}

/**
 * @brief ACC/Gyro Sensor ACC Data Calc
 * @param calc_data ACC/Gyro Sensor Data
 * @retval None
 */
static void acc_calculation( ACC_GYRO_DATA_INFO *calc_data )
{
	float acc_data = 0.0;
	/* 加速度をSensitivity Scale Factorで割ってmgに変換する */
	acc_data = (float)( calc_data->acc_x_data / ACC_FS_16G ) * ACC_MG_UNIT;
	calc_data->acc_x_data = (int16_t)acc_data;
	acc_data = (float)( calc_data->acc_y_data / ACC_FS_16G ) * ACC_MG_UNIT;
	calc_data->acc_y_data = (int16_t)acc_data;
	acc_data = (float)( calc_data->acc_z_data / ACC_FS_16G ) * ACC_MG_UNIT;
	calc_data->acc_z_data = (int16_t)acc_data;
}

/**
 * @brief ACC/Gyro Sensor Gyro Data Calc
 * @param calc_data ACC/Gyro Sensor Data
 * @retval None
 */
static void gyro_calculation( ACC_GYRO_DATA_INFO *calc_data )
{
	float result = 0.0;

	result = ((int32_t)calc_data->gyro_x_data) * GYRO_FS_2000DPS;
	calc_data->gyro_x_data = (int16_t)(result / DEF_GYRO_CONSTANT);
	result = ((int32_t)calc_data->gyro_y_data) * GYRO_FS_2000DPS;
	calc_data->gyro_y_data = (int16_t)(result / DEF_GYRO_CONSTANT);
	result = ((int32_t)calc_data->gyro_z_data) * GYRO_FS_2000DPS;
	calc_data->gyro_z_data = (int16_t)(result / DEF_GYRO_CONSTANT);
}

/**
 * @brief ACC/Gyro Sensor ACC/Gyro Data Calc
 * @param calc_data ACC/Gyro Sensor Data
 * @retval None
 */
static void acc_gyro_calculation( ACC_GYRO_DATA_INFO *calc_data )
{
	/* ACCデータ計算 */
	acc_calculation( calc_data );
	/* Gyroデータ計算 */
	gyro_calculation( calc_data );
}

/**
 * @brief ACC/Gyro Sensor Setup Error Log
 * @param reg_addr Register Address
 * @param set_val Set Value
 * @param line Execution Line
 * @retval None
 */
__STATIC_INLINE void register_setup_error_log( uint8_t reg_addr, uint8_t set_val, uint32_t line )
{
	DEBUG_LOG( LOG_ERROR, "register=0x%x val=0x%x line=%d", reg_addr, set_val, line );
}

/**
 * @brief ACC/Gyro Sensor Registor Reset
 * @param None
 * @retval UTC_SUCCESS Success
 * @retval UTC_SPI_ERROR Error
 */
uint32_t AccGyroRegReset( void )
{
	nrfx_err_t err_code;
	uint32_t ret = UTC_SUCCESS;
	uint16_t i;
	const ACC_GYRO_FUNC_TABLE acc_gyro_reg_table[] = {
		{	0,	&acc_gyro_read_device_id		},			/* WHO_AM_I(0Fh)レジスタ読み出し */
		{	1,	&acc_gyro_softreset				},			/* Software Reset */
		{	2,	&acc_gyro_otp_reload			},			/* OTP Reload */
	};
	
	/* SPI Function Initialize */
	err_code = SpiInit();
	if ( err_code == NRF_SUCCESS )
	{
		for ( i = 0; i < sizeof( acc_gyro_reg_table ) / sizeof( ACC_GYRO_FUNC_TABLE ); i++ )
		{
			if ( err_code == NRF_SUCCESS )
			{
				/* Registor Reset関数呼び出し */
				err_code = acc_gyro_reg_table[i].func();
			}
			else
			{
				/* エラー時 */
				ret = UTC_SPI_ERROR;
				break;
			}
		}
		/* 終了処理 */
		SpiUninit();
	}
	else
	{
		/* SPI Initialize Error */
		ret = UTC_SPI_ERROR;
	}
	
	if ( ret != UTC_SUCCESS )
	{
		DEBUG_LOG( LOG_ERROR, "!!! Acc/Gyro Reg Reset Error !!!" );
		TRACE_LOG( TR_ACC_RESET_FAIL, 0 );
		SetBleErrReadCmd( (uint8_t)UTC_BLE_SPI_INIT_ERROR );
	}
	else
	{
		/* 起動時のモードはACC Only Low Power */
		set_acc_gyro_mode( MODE_ACC_ONLY_LP );
		DEBUG_LOG( LOG_INFO, "!!! Acc/Gyro Reg Reset Complete !!!" );
		TRACE_LOG( TR_ACC_RESET_CMPL, 0 );
#if 0
		/* 2022.03.24 Test GPIO Output 割り込み時間計測テスト ++ */
		nrf_gpio_cfg_output( GPIO_UART_RX_PIN );
		nrf_gpio_pin_clear( GPIO_UART_RX_PIN );
		/* 2022.03.24 Test GPIO Output 割り込み時間計測テスト -- */
#endif
	}
	
	return ret;
}

/**
 * @brief ACC/Gyro Sensor Initialize
 * @param previous_err 一つ前の処理結果
 * @retval UTC_SUCCESS Success
 * @retval UTC_SPI_ERROR Error
 */
uint32_t AccGyroInitialize( uint32_t previous_err )
{
	nrfx_err_t err_code;
	uint32_t ret = UTC_SUCCESS;
	uint16_t i;
	const ACC_GYRO_FUNC_TABLE acc_gyro_init_table[] = {
		{	2,	&acc_gyro_odr_fsr				},			/* ACC/Gyro ODR/full-scale setting(GYRO_CONFIG0,ACCEL_CONFIG0) */
		{	0,	&acc_gyro_fifo_config			},			/* FIFO Mode Config */
		{	1,	&acc_gyro_interrupt_ctrl_set	},			/* INT1/INT2 Setting */
		{	3,	&acc_gyro_exec_mode_set			},			/* Acc/Gyro動作モード設定 */
	};
	
	if ( previous_err != UTC_SUCCESS )
	{
		return UTC_THROUGH_ERROR;
	}
	
	/* 2020.12.08 Add Clear FIFO Count ++ */
	AccGyroClearFifoCount();
	/* 2020.12.08 Add Clear FIFO Count -- */
	
	/* SPI Function Initialize */
	err_code = SpiInit();
	if ( err_code == NRF_SUCCESS )
	{
		for ( i = 0; i < sizeof( acc_gyro_init_table ) / sizeof( ACC_GYRO_FUNC_TABLE ); i++ )
		{
			if ( err_code == NRF_SUCCESS )
			{
				nrf_delay_us( 500 );
				/* Registor Reset関数呼び出し */
				err_code = acc_gyro_init_table[i].func();
			}
			else
			{
				/* エラー時 */
				ret = UTC_SPI_ERROR;
				break;
			}
		}
		/* 終了処理 */
		SpiUninit();
		if ( ret == UTC_SUCCESS )
		{
			/* 割り込み有効化 */
			nrfx_gpiote_in_event_enable( ACC_INT1_PIN, true );
			nrfx_gpiote_in_event_enable( ACC_INT2_PIN, true );
		}
	}
	else
	{
		/* SPI Initialize Error */
		ret = UTC_SPI_ERROR;
	}
	
	if ( ret != UTC_SUCCESS )
	{
		DEBUG_LOG( LOG_ERROR, "!!! Acc/Gyro Initialize Error !!!" );
		TRACE_LOG( TR_ACC_INIT_FAIL, 0 );
		SetBleErrReadCmd( (uint8_t)UTC_BLE_SPI_INIT_ERROR );
	}
	else
	{
		DEBUG_LOG( LOG_INFO, "!!! Acc/Gyro Initialize Complete !!!" );
		TRACE_LOG( TR_ACC_INIT_CMPL, 0 );
	}
	return ret;
}

/**
 * @brief ACC/Gyro Sensor Wake Up Setting
 * @param previous_err 一つ前の処理結果
 * @retval UTC_SUCCESS Success
 * @retval UTC_SPI_ERROR Error
 */
uint32_t AccGyroWakeUpSetting( uint32_t previous_err )
{
	nrfx_err_t err_code;
	uint32_t ret = UTC_SUCCESS;
	uint16_t i;
	const ACC_GYRO_FUNC_TABLE acc_gyro_wakeup_table[] = {
		{	0,	&acc_gyro_power_down_mode		},			/* ACC/Gyro Power Down Mode設定(PWR_MGMT0) */
		{	1,	&acc_gyro_softreset				},			/* Software Reset */
		{	2,	&acc_gyro_poweroff_wakeup_set	},			/* Wake Up Setting */
	};
	
	if ( previous_err != UTC_SUCCESS )
	{
		return UTC_THROUGH_ERROR;
	}

	/* WakeUp PIN Setting */
	setup_acc_gyro_gpio_pin_wakeup();

	/* SPI Function Initialize */
	err_code = SpiInit();
	if ( err_code == NRF_SUCCESS )
	{
		for ( i = 0; i < sizeof( acc_gyro_wakeup_table ) / sizeof( ACC_GYRO_FUNC_TABLE ); i++ )
		{
			if ( err_code == NRF_SUCCESS )
			{
				/* Registor Reset関数呼び出し */
				err_code = acc_gyro_wakeup_table[i].func();
				/* レジスタ反映待ち 500us */
				nrf_delay_us( WAIT_REG_SETUP_TIME );
			}
			else
			{
				/* エラー時 */
				ret = UTC_SPI_ERROR;
				break;
			}
		}
		/* 終了処理 */
		SpiUninit();
	}
	else
	{
		/* SPI Initialize Error */
		ret = UTC_SPI_ERROR;
	}
	
	if ( ret != UTC_SUCCESS )
	{
		DEBUG_LOG( LOG_ERROR, "!!! Acc/Gyro WakeUp Setting Error !!!" );
		TRACE_LOG( TR_ACC_3G_INIT_CMPL, 0 );
		SetBleErrReadCmd( (uint8_t)UTC_BLE_SPI_INIT_ERROR );
	}
	else
	{
		DEBUG_LOG( LOG_INFO, "!!! Acc/Gyro WakeUp Setting Complete !!!" );
		TRACE_LOG( TR_ACC_3G_INIT_FAIL, 0 );
	}
	return ret;
}

/**
 * @brief ACC/Gyro Sensor Read WakeUp Interrupt Source
 * @param x_int X-Axis Wake Up
 * @param y_int Y-Axis Wake Up
 * @param z_int Z-Axis Wake Up
 * @retval UTC_SUCCESS Success
 * @retval UTC_SPI_ERROR Error
 */
uint32_t AccGyroReadIntSrc( uint8_t *x_int, uint8_t *y_int, uint8_t *z_int )
{
	nrfx_err_t err_code;
	uint32_t ret = UTC_SUCCESS;
	uint8_t wakeup_src_val = 0;
	
	/* SPI Function Initialize */
	err_code = SpiInit();
	if ( err_code == NRF_SUCCESS )
	{
		/* Device ID 読み出し */
		err_code = acc_gyro_read_device_id();
		if ( err_code == NRF_SUCCESS )
		{
			/* INT2 PinによるWakeUpかどうか確認 */
			/* INT2 Interrupt チェック */
			ret = SpiIORead( ICM42607_INT_STATUS2, (uint8_t *)&wakeup_src_val, sizeof( wakeup_src_val ) );
			SPI_ERR_CHECK( ret, __LINE__ );
			if ( ret != NRF_SUCCESS )
			{
				SpiUninit();
				return ret;
			}
			
			*y_int = NON_WAKEUP;
			*x_int = NON_WAKEUP;
			/* 2022.04.19 Add Z-Axis追加 ++ */
			*z_int = NON_WAKEUP;
			/* 2022.04.19 Add Z-Axis追加 -- */
			/* WakeUp SRCの確認 X WOM チェック */
			if ( wakeup_src_val & ACC_GYRO_WOM_X_INT )
			{
				/* X-Axis WakeUp */
				*x_int = WAKEUP_X_AXIS;
			}
			/* WakeUp SRCの確認 Y WOM チェック */
			if ( wakeup_src_val & ACC_GYRO_WOM_Y_INT )
			{
				/* Y-Axis WakeUp */
				*y_int = WAKEUP_Y_AXIS;
			}
			/* 2022.04.19 Add Z-Axis追加 ++ */
			/* WakeUp SRCの確認 Z WOM チェック */
			if ( wakeup_src_val & ACC_GYRO_WOM_Z_INT )
			{
				/* Y-Axis WakeUp */
				*z_int = WAKEUP_Z_AXIS;
			}
			/* 2022.04.19 Add Z-Axis追加 -- */
		}
		/* 上記処理がエラーの場合 */
		if ( err_code != NRF_SUCCESS )
		{
			ret = ACC_GYRO_SETEUP_ERROR;
		}
		
		/* 終了処理 */
		SpiUninit();
	}
	else
	{
		/* SPI Initialize Error */
		ret = UTC_SPI_ERROR;
	}
	
	if ( ret != UTC_SUCCESS )
	{
		DEBUG_LOG( LOG_ERROR, "!!! Acc/Gyro Read Interrupt Src Error !!!" );
		SetBleErrReadCmd( (uint8_t)UTC_BLE_SPI_INIT_ERROR );
	}
	else
	{
		DEBUG_LOG( LOG_INFO, "Acc/Gyro INT WakeUp SRC=0x%x", wakeup_src_val );
	}
		
	return ret;
}

/**
 * @brief ACC/Gyro Sensor Reconfig Execution Setting
 * @param mode_number Mode id number
 * @retval UTC_SUCCESS Success
 * @retval UTC_SPI_ERROR Error
 */
uint32_t AccGyroReconfig( uint8_t mode_number )
{
	nrfx_err_t err_code;
	uint32_t ret = UTC_SUCCESS;
	uint16_t i;
	const struct _operaton_mode_info
	{
		uint8_t mode_id;
		uint8_t acc_mode;
		uint8_t gyro_mode;
	} op_mode_info_table[] = {
		{	MODE_ACC_ONLY,		PWR_ACC_LN_MODE,	PWR_GYRO_TURN_OFF	},	/* ACC Only Mode */
		{	MODE_ACC_ONLY_LP,	PWR_ACC_LP_MODE,	PWR_GYRO_TURN_OFF	},	/* ACC Only Low Power Mode */
		{	MODE_GYRO_TEMP,		PWR_ACC_TURN_OFF,	PWR_GYRO_LN_MODE	},	/* Gyro/Temp Mode */
		{	MODE_BOTH,			PWR_ACC_LN_MODE,	PWR_GYRO_LN_MODE	},	/* ACC/Gyro/Temp Mode */
	};
	
	/* 割り込みを無効に設定 */
	AccGyroDisableGpioInt( ACC_INT1_PIN );
	
	/* SPI Function Initialize */
	err_code = SpiInit();
	if ( err_code == NRF_SUCCESS )
	{
		for ( i = 0; i < sizeof( op_mode_info_table ) / sizeof( struct _operaton_mode_info ); i++ )
		{
			if ( mode_number == op_mode_info_table[i].mode_id )
			{
				/* モードを設定する */
				set_acc_gyro_mode( mode_number );
				/* 各モードの設定値に再設定を実行する */
				err_code = reconfig_operation_mode( op_mode_info_table[i].acc_mode, op_mode_info_table[i].gyro_mode );
				if ( err_code != NRF_SUCCESS )
				{
					/* エラー時 */
					ret = UTC_SPI_ERROR;
				}
				break;
			}
		}
		/* 終了処理 */
		SpiUninit();
	}
	else
	{
		/* SPI Initialize Error */
		ret = UTC_SPI_ERROR;
	}
	
	if ( ret != UTC_SUCCESS )
	{
		DEBUG_LOG( LOG_ERROR, "!!! ACC/Gyro Reconfig Error !!!" );
		TRACE_LOG( TR_ACC_RESET_FAIL, 0 );
		SetBleErrReadCmd( (uint8_t)UTC_BLE_SPI_INIT_ERROR );
	}
	else
	{
		/* 2020.12.08 Add SIDをクリア ++ */
		AccGyroSetSid( 0 );
		/* 2020.12.08 Add SIDをクリア -- */
		DEBUG_LOG( LOG_INFO, "!!! ACC/Gyro Reconfig Complete Mode[%d]!!!", mode_number );
		TRACE_LOG( TR_ACC_RESET_CMPL, 0 );
	}
	/* 再度、割り込みを有効に設定 */
	AccGyroEnableGpioInt( ACC_INT1_PIN );

	return ret;
}

/**
 * @brief ACC/Gyro Sensor Data Calc
 * @param acc_gyro_info ACC/Gyro Sensor Data
 * @retval UTC_SUCCESS Success
 * @retval UTC_SPI_ERROR Error
 */
void AccGyroCalcData( ACC_GYRO_DATA_INFO *acc_gyro_info )
{
	uint8_t idx = 0;
	volatile uint8_t current_mode;
	/* 計算用の配列 */
	const struct _calc_data_func_table
	{
		calc_func func;
	} calcop_mode_info_table[] = { &acc_calculation, &gyro_calculation, &acc_gyro_calculation, &acc_calculation };

	/* ACC/Gyro/Tempデータを計算する */
	get_acc_gyro_mode( (void *)&current_mode );
	idx = current_mode;
	calcop_mode_info_table[idx].func( acc_gyro_info );
	/* ここで通知するデータのHeaderを設定する */
	if ( ( acc_gyro_info->header & HEADER_ACC_GYRO ) != 0 )
	{
		acc_gyro_info->header = current_mode + 1;
	}
	return ;
}

/**
 * @brief ACC/Gyro Sensor INT GPIO Enable Interrupt
 * @param pin Interrupt Enable PIN Number
 * @retval None
 */
void AccGyroEnableGpioInt( uint32_t pin )
{
	/* Latch Clear */
	nrf_gpio_pin_latch_clear( pin );
	/* Interrupt Enable */
	nrfx_gpiote_in_event_enable( pin, true );
}

/**
 * @brief ACC/Gyro Sensor INT GPIO Disable Interrupt
 * @param pin Interrupt Enable PIN Number
 * @retval None
 */
void AccGyroDisableGpioInt( uint32_t pin )
{
	/* 割り込みを無効にしてLatchをクリアする */
	nrfx_gpiote_in_event_disable( pin );
	/* Latch Clear */
	nrf_gpio_pin_latch_clear( pin );
}

/**
 * @brief ACC/Gyro SID Clear
 * @param pin Interrupt Enable PIN Number
 * @retval None
 */
void AccGyroSetSid( uint16_t sid )
{
	volatile uint32_t err_code;
	uint8_t  acc_gyro_sid_session;
	
	err_code = sd_nvic_critical_region_enter( &acc_gyro_sid_session );
	if(err_code == NRF_SUCCESS)
	{
		g_send_sid = sid;
		acc_gyro_sid_session = 0;
		err_code = sd_nvic_critical_region_exit( acc_gyro_sid_session );
		if(err_code != NRF_SUCCESS)
		{
			DEBUG_LOG( LOG_ERROR, "!!! Set ACC/Gyro SID Clear critical Exit !!!" );
		}
	}
	else
	{
		DEBUG_LOG( LOG_ERROR, "!!! Set ACC/Gyro SID Clear critical Enter !!!" );
	}
}

/**
 * @brief ACC/Gyro Inc SID
 * @param None
 * @retval sid 現在のSIDを返す
 */
uint16_t AccGyroIncSid( void )
{
	volatile uint32_t err_code;
	volatile uint16_t sid;
	uint8_t  acc_gyro_sid_session;
	
	err_code = sd_nvic_critical_region_enter( &acc_gyro_sid_session );
	if(err_code == NRF_SUCCESS)
	{
		sid = g_send_sid;
		g_send_sid++;
		acc_gyro_sid_session = 0;
		err_code = sd_nvic_critical_region_exit( acc_gyro_sid_session );
		if(err_code != NRF_SUCCESS)
		{
			DEBUG_LOG( LOG_ERROR, "!!! Get ACC/Gyro Inc critical Exit !!!" );
		}
	}
	else
	{
		DEBUG_LOG( LOG_ERROR, "!!! Get ACC/Gyro Inc critical Enter !!!" );
	}
	
	return sid;
}

/**
 * @brief ACC/Gyro Clear FIFO Count
 * @param None
 * @retval None
 */
void AccGyroClearFifoCount( void )
{
	volatile uint32_t err_code;
	uint8_t  acc_gyro_fifo_count_session;
	
	err_code = sd_nvic_critical_region_enter( &acc_gyro_fifo_count_session );
	if(err_code == NRF_SUCCESS)
	{
		g_fifo_count = 0;
		acc_gyro_fifo_count_session = 0;
		err_code = sd_nvic_critical_region_exit( acc_gyro_fifo_count_session );
		if(err_code != NRF_SUCCESS)
		{
			DEBUG_LOG( LOG_ERROR, "!!! Set ACC/Gyro FIFO Count Clear Exit !!!" );
		}
	}
	else
	{
		DEBUG_LOG( LOG_ERROR, "!!! Set ACC/Gyro FIFO Count Clear Enter !!!" );
	}
}

/**
 * @brief ACC/Gyro Increment FIFO Count
 * @param None
 * @retval None
 */
void AccGyroIncFifoCount( void )
{
	volatile uint32_t err_code;
	uint8_t  acc_gyro_fifo_count_session;
	
	err_code = sd_nvic_critical_region_enter( &acc_gyro_fifo_count_session );
	if(err_code == NRF_SUCCESS)
	{
		if ( g_fifo_count < 40 )
		{
			g_fifo_count++;
		}
		acc_gyro_fifo_count_session = 0;
		err_code = sd_nvic_critical_region_exit( acc_gyro_fifo_count_session );
		if(err_code != NRF_SUCCESS)
		{
			DEBUG_LOG( LOG_ERROR, "!!! Get ACC/Gyro FIFO Count critical Exit !!!" );
		}
	}
	else
	{
		DEBUG_LOG( LOG_ERROR, "!!! Get ACC/Gyro FIFO Count critical Enter !!!" );
	}
}

/**
 * @brief ACC/Gyro Sub FIFO Count
 * @param None
 * @retval fifo_count FIFO Count
 */
uint16_t AccGyroSubFifoCount( void )
{
	volatile uint32_t err_code;
	volatile uint16_t fifo_count = 0;
	uint8_t  acc_gyro_fifo_count_session;
	
	err_code = sd_nvic_critical_region_enter( &acc_gyro_fifo_count_session );
	if(err_code == NRF_SUCCESS)
	{
		if ( g_fifo_count > 0 )
		{
			g_fifo_count--;
			fifo_count = g_fifo_count;
		}
		acc_gyro_fifo_count_session = 0;
		err_code = sd_nvic_critical_region_exit( acc_gyro_fifo_count_session );
		if(err_code != NRF_SUCCESS)
		{
			DEBUG_LOG( LOG_ERROR, "!!! Get ACC/Gyro FIFO Count critical Exit !!!" );
		}
	}
	else
	{
		DEBUG_LOG( LOG_ERROR, "!!! Get ACC/Gyro FIFO Count critical Enter !!!" );
	}
	
	return fifo_count;
}

/**
 * @brief ACC/Gyro Get FIFO Count
 * @param None
 * @retval fifo_count FIFO Count
 */
uint16_t AccGyroGetFifoCount( void )
{
	volatile uint32_t err_code;
	volatile uint16_t fifo_count = 0;
	uint8_t  acc_gyro_fifo_count_session;
	
	err_code = sd_nvic_critical_region_enter( &acc_gyro_fifo_count_session );
	if(err_code == NRF_SUCCESS)
	{
		fifo_count = g_fifo_count;
		acc_gyro_fifo_count_session = 0;
		err_code = sd_nvic_critical_region_exit( acc_gyro_fifo_count_session );
		if(err_code != NRF_SUCCESS)
		{
			DEBUG_LOG( LOG_ERROR, "!!! Get ACC/Gyro FIFO Count critical Exit !!!" );
		}
	}
	else
	{
		DEBUG_LOG( LOG_ERROR, "!!! Get ACC/Gyro FIFO Count critical Enter !!!" );
	}
	
	return fifo_count;
}

/**
 * @brief validate and clear Acc/Gyro FIFO
 * @param None
 * @retval None
 */
void AccGyroValidateClearFifo( void )
{
	ACC_GYRO_DATA_INFO acc_gyro_data = {0};
	SENSOR_FIFO_DATA_INFO fifo_data_info = {0};
	volatile uint16_t set_sid = 0;
	uint32_t ret;
	
	ret = FifoClearPopFifo( &fifo_data_info );
	if ( ret == NRF_SUCCESS )
	{
		/* 一度、INT1の割り込みを無効に設定する */
		AccGyroDisableGpioInt( ACC_INT1_PIN );
		
		DEBUG_LOG( LOG_DEBUG, "Clear FIFO Count=%d", fifo_data_info.fifo_count );
		for ( uint16_t i = 0; i < fifo_data_info.fifo_count; i++ )
		{
			/* FIFOからデータ取得してクリアする */
			ret = AccGyroPopFifo(&acc_gyro_data);
			if ( ret == NRF_SUCCESS )
			{
				if ( i == 0 )
				{
					set_sid = acc_gyro_data.sid;
					DEBUG_LOG( LOG_DEBUG, "set_sid=%d", set_sid );
				}
				AccGyroSubFifoCount();
			}
		}
		
		if ( set_sid != 0 )
		{
			/* SIDを更新 */
			AccGyroSetSid( set_sid );
		}
		
		/* 有効にしてから抜ける */
		AccGyroEnableGpioInt( ACC_INT1_PIN );
	}
}

/**
 * @brief Get Send Header
 * @param tag_data ACC/Gyro FIFO Tag Data
 * @retval header Send Header Data
 */
uint8_t AccGyroGetSendHeader( uint8_t tag_data )
{
	volatile uint8_t current_mode;
	uint8_t header = 0;

	/* ACC/Gyro/Tempデータを計算する */
	get_acc_gyro_mode( (void *)&current_mode );
	/* ここで通知するデータのHeaderを設定する */
	if ( ( tag_data & HEADER_ACC_GYRO ) != 0 )
	{
		 header = current_mode + 1;
	}
	return header;
}

