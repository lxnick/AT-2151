/**
  ******************************************************************************************
  * @file    lib_ex_rtc.c
  * @author  k.tashiro
  * @version 1.0
  * @date    2020/09/10
  * @brief   I2C External RTC Control
  ******************************************************************************************
    Revision:
    Ver            Date             Revised by        Explanation
  ******************************************************************************************
  * 1.0            2020/09/10       k.tashiro         create new
  ******************************************************************************************
*/

/* Includes --------------------------------------------------------------*/
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_gpio.h"
#include "nrfx_gpiote.h"
#include "sdk_errors.h"
#include "app_error.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "lib_ex_rtc.h"
#include "state_control.h"
#include "lib_fifo.h"
#include "ble_definition.h"
#include "lib_trace_log.h"

/* Definition ------------------------------------------------------------*/
#define WAIT_EVENT_INTERVAL		(100000)		/* I2Cの応答待ち時間 */

#define I2C_DRV_FREQ_390K		(0x06200000)	/* Errata No.219対応 */

/* private variables -----------------------------------------------------*/
const nrf_drv_twi_t gRtc_twi = NRF_DRV_TWI_INSTANCE(1);		/* SPIMとTWIMはレジスタを共有しているためSPI0とTWI1にする */
const nrf_drv_twi_config_t twi_config = {
	.scl				= I2C_CLK_PIN,		/* PIN: P0.14 SCL */
	.sda				= I2C_SDA_PIN,		/* PIN: P0.15 SDA */
	.frequency			= (nrf_drv_twi_frequency_t)NRF_DRV_TWI_FREQ_100K,	/* 2022.01.11 Modify 390KHz -> 100KHz */
	.interrupt_priority	= APP_IRQ_PRIORITY_HIGH,
	.clear_bus_init		= false
};

static volatile bool g_event = false;
static volatile uint16_t g_res_state = 0;

/* 2022.05.16 Add 自動接続対応 ++ */
static volatile bool g_rtc_wakeup = false;
/* 2022.05.16 Add 自動接続対応 -- */

/* private prototypes ----------------------------------------------------*/
/**
 * @brief Read/Write Callback function
 * @param p_event event information
 * @param p_context context
 * @retval None
 */
static void twi_event_handler(nrf_drv_twi_evt_t const * p_event, void * p_context);

/**
 * @brief wait interrupt event
 * @param none
 * @retval true Interrupt Event receive
 * @retval false Interrupt Event non-receive
 */
static bool wait_event( void );

/**
* @brief I2C Error Check(Save Trace Log and Set ble read error)
 * @param err error code
 * @param line execution line
 * @retval NRF_SUCCESS Success
 */
static void i2c_err_check(nrfx_err_t err, uint16_t line)
{
	if(err != NRF_SUCCESS)
	{
		/*trace log*/
		TRACE_LOG(TR_I2C_ERROR,line);
		SetBleErrReadCmd(UTC_BLE_I2C_ERROR);
		DEBUG_LOG( LOG_ERROR,"I2C err. err_code 0x%x, line %d",err, line );
	}
}

/**
 * @brief Initialize I2C
 * @param None
 * @retval NRF_SUCCESS Success
 */
static ret_code_t i2c_init(void)
{
	ret_code_t err_code;

	/* debug test */
#ifdef 	TEST_I2C_INIT_ERROR
	ret_code_t tmp_err_code;
	tmp_err_code = NRF_ERROR_BUSY;
	SetBleErrReadCmd(UTC_BLE_I2C_INIT_ERROR);
#endif
	err_code = nrf_drv_twi_init(&gRtc_twi, &twi_config, twi_event_handler, NULL);
	if(err_code != NRF_SUCCESS)
	{
		TRACE_LOG(TR_I2C_INIT_ERROR, 0);
		SetBleErrReadCmd(UTC_BLE_I2C_INIT_ERROR);
	}
	nrf_drv_twi_enable(&gRtc_twi);
	
	return err_code;
}

/**
 * @brief Uninitialize I2C
 * @param None
 * @retval None
 */
static void i2c_uninit(void)
{
	nrf_drv_twi_disable(&gRtc_twi);
	nrf_drv_twi_uninit(&gRtc_twi);
}

/**
 * @brief I2C Write
 * @param slave_addr Slave Address
 * @param reg_addr Register Address
 * @param ptxdata 送信データ
 * @param size 送信データサイズ
 * @retval NRF_SUCCESS Success
 */
static ret_code_t i2c_write(uint8_t slave_addr, uint8_t reg_addr, uint8_t *ptxdata, uint8_t size)
{
	volatile ret_code_t err_code;
	volatile uint8_t txbuf[I2C_BUFFER_SIZE];
	bool event = false;
	
	err_code = NRF_SUCCESS;
	
	if((sizeof(txbuf) - 1) < size)
	{
		err_code = NRF_ERROR_INVALID_ADDR;
		return err_code;
	}
	
	memset((uint8_t*)&txbuf[0],0x00, sizeof(txbuf));
	txbuf[0] = reg_addr;
	
	if(ptxdata != NULL)
	{
		g_event = false;
		memcpy((uint8_t*)&txbuf[1], ptxdata, size);
		err_code = nrf_drv_twi_tx(&gRtc_twi, slave_addr, (uint8_t*)&txbuf[0], (size + 1), false);
		if ( err_code == NRF_SUCCESS )
		{
			/* ACK待ち */
			event = wait_event();
			if ( event == false )
			{
				return NRFX_ERROR_BUSY;
			}
			switch ( g_res_state )
			{
			case 1:	err_code = NRFX_ERROR_DRV_TWI_ERR_ANACK;	break;
			case 2: err_code = NRFX_ERROR_DRV_TWI_ERR_DNACK;	break;
			}
		}
	}
	return err_code;
}

/**
 * @brief I2C Read
 * @param slave_addr Slave Address
 * @param reg_addr Register Address
 * @param prxdata 受信データ
 * @param size 受信データサイズ
 * @retval NRF_SUCCESS Success
 */
static ret_code_t i2c_read(uint8_t slave_addr, uint8_t reg_addr, uint8_t *prxdata, uint8_t size)
{
	volatile ret_code_t err_code;
	volatile uint8_t buf;
	uint16_t i;
	bool event = false;
	
	err_code = NRF_ERROR_INVALID_ADDR;
	
	if((uint8_t)(I2C_BUFFER_SIZE - 1) < size)
	{
		return err_code;
	}
	
	if(prxdata != NULL)
	{
		g_event = false;
		g_res_state = 0;

		buf = reg_addr;
		
		/* 2022.01.26 Add Read Retry ++ */
		for ( i = 0; i < 10; i++ )
		{
			err_code = nrf_drv_twi_tx(&gRtc_twi, slave_addr, (uint8_t*)&buf, sizeof(buf), true);
			if(err_code == NRF_SUCCESS)
			{
				/* ACK待ち */
				event = wait_event();
				if ( event == false )
				{
					return NRFX_ERROR_BUSY;
				}
				switch ( g_res_state )
				{
				case 1:	err_code = NRFX_ERROR_DRV_TWI_ERR_ANACK;	break;
				case 2: err_code = NRFX_ERROR_DRV_TWI_ERR_DNACK;	break;
				}

				if ( err_code == NRF_SUCCESS )
				{
					g_event = false;
					g_res_state = 0;
					/* Read */
					err_code = nrf_drv_twi_rx(&gRtc_twi, slave_addr, prxdata, size);
					if ( err_code == NRF_SUCCESS )
					{
						/* ACK待ち */
						event = wait_event();
						if ( event == false )
						{
							return NRFX_ERROR_BUSY;
						}
						switch ( g_res_state )
						{
						case 1:	err_code = NRFX_ERROR_DRV_TWI_ERR_ANACK;	break;
						case 2: err_code = NRFX_ERROR_DRV_TWI_ERR_DNACK;	break;
						}
					}
				}
			}
			if ( err_code != NRF_ERROR_BUSY )
			{
				break;
			}
			nrf_delay_us(1);
			/* Busyの場合のみ1us待ってから再度実行する */
		}
	}
	return err_code;
}

/**
 * @brief Read/Write Callback function
 * @param p_event event information
 * @param p_context context
 * @retval None
 */
static void twi_event_handler(nrf_drv_twi_evt_t const * p_event, void * p_context)
{
	switch ( p_event->type )
	{
	case NRF_DRV_TWI_EVT_DONE:
		g_res_state = 0;
		/* chack xfer type */
		switch ( p_event->xfer_desc.type )
		{
		case NRF_DRV_TWI_XFER_TX:
			g_event = true;
			break;
		case NRF_DRV_TWI_XFER_TXTX:
			g_event = true;
			break;
		case NRF_DRV_TWI_XFER_RX:
			g_event = true;
			break;
		case NRF_DRV_TWI_XFER_TXRX:
			g_event = true;
			break;
		default: break;
		}
		break;
	case NRF_DRV_TWI_EVT_ADDRESS_NACK:
		g_event = true;
        g_res_state = 1;
		break;
	case NRF_DRV_TWI_EVT_DATA_NACK:
		g_event = true;
        g_res_state = 2;
		break;
	default:
		break;
	}
}

/**
 * @brief wait interrupt event
 * @param none
 * @retval true Interrupt Event receive
 * @retval false Interrupt Event non-receive
 */
static bool wait_event( void )
{
	uint32_t i;
	bool event_state = false;
	
	for ( i = 0; i < WAIT_EVENT_INTERVAL; i++ )
	{
		nrf_delay_us(1);
		if ( g_event == true )
		{
			event_state = true;
			break;
		}
	}
	return event_state;
}
#if 0
static void register_read_test( void )
{
	volatile ret_code_t err_code;
	volatile uint8_t ctrlreg1 = 0;
	volatile uint8_t ctrlreg2 = 0;
	DATE_TIME read_data;

	err_code = i2c_read( RTC_SLAVE_ADD, RTC_CTRL_REG1, (uint8_t*)&ctrlreg1, sizeof( ctrlreg1 ) );
	err_code = i2c_read( RTC_SLAVE_ADD, RTC_CTRL_REG2, (uint8_t*)&ctrlreg2, sizeof( ctrlreg2 ) );
	err_code = i2c_read( RTC_SLAVE_ADD, RTC_SECONDS, (uint8_t*)&read_data, sizeof( read_data ) );
	DEBUG_LOG( LOG_INFO, "[0x%x:0x%x] %x.%x.%x %x:%x:%x [%x]", ctrlreg1, ctrlreg2, read_data.year, read_data.month, read_data.day, read_data.hour, read_data.min, read_data.sec, read_data.week );
}
#endif

/**
 * @brief External RTC Alarm flag Clear
 * @param None
 * @retval None
 */
static void ex_rtc_alarm_flag_clear(void)
{
	volatile ret_code_t err_code;
	volatile uint8_t buf;

	err_code = i2c_read( RTC_SLAVE_ADD, RTC_CTRL_REG2, (uint8_t*)&buf, sizeof( buf ) );
	i2c_err_check(err_code,__LINE__);
	buf = buf & ~RTC_ALARM_FLAG_OFF_MASK;

	err_code = i2c_write( RTC_SLAVE_ADD, RTC_CTRL_REG2, (uint8_t*)&buf, sizeof( buf ) );
	i2c_err_check(err_code,__LINE__);
}

/**
 * @brief BCD convert. Binary to Dec convert
 * @param data Binary Data
 * @retval None
 */
static void btod_convert(uint8_t *data)
{
	uint8_t ten_place;
	uint8_t one_place;
	
	ten_place = *data >> 4;
	one_place = *data & 0x0f;
	
	*data = (ten_place * 10) + one_place;
	
}

/**
 * @brief date, time BCD converter. Binary -> Dec
 * @param pdatetime date time
 * @retval None
 */
static void date_btod_convert(DATE_TIME *pdatetime)
{
	//sec convert.
	pdatetime->sec &= EX_RTC_SEC_GET_MASK ;
	btod_convert(&pdatetime->sec);
	
	//min convert.
	pdatetime->min &= EX_RTC_MIN_GET_MASK;
	btod_convert(&pdatetime->min);
	
	//hour
	pdatetime->hour &= EX_RTC_HOUR_GET_MASK;
	btod_convert(&pdatetime->hour);
	
	//day
	pdatetime->day &= EX_RTC_DAY_GET_MASK;
	btod_convert(&pdatetime->day);
	
	//weekend
	//Not change Binary -> Dec
	pdatetime->week &= 0x07;
	
	//month
	pdatetime->month &= EX_RTC_MONTH_GET_MASK;
	btod_convert(&pdatetime->month);
	
	//year
	pdatetime->year &= EX_RTC_YEAR_GET_MASK;
	btod_convert(&pdatetime->year);
}

/**
 * @brief BCD convert. Dec to Binary convert
 * @param data Dec Data
 * @retval None
 */
static void dtob_convert(uint8_t *data)
{
	uint8_t ten_place;
	uint8_t one_place;
	
	ten_place = *data / (uint8_t)10;
	ten_place = ten_place << 4;
	
	one_place = *data % (uint8_t)10;
	
	*data = ten_place | one_place;
}

/**
 * @brief BCD convert. Dec to Binary convert
 * @param pdatetime date time
 * @retval None
 */
static void date_dtob_convert(DATE_TIME *pdatetime)
{
	dtob_convert(&pdatetime->sec);		//sec
	dtob_convert(&pdatetime->min);		//min
	dtob_convert(&pdatetime->hour);		//hour
	dtob_convert(&pdatetime->day);		//day
	dtob_convert(&pdatetime->month);	//month
	dtob_convert(&pdatetime->year);		//yert
}

/**
 * @brief External RTC get oscillator stop data
 * @param None
 * @retval ret Register SecondsのOS値
 */
static uint8_t ex_rtc_os_flag_check(void)
{
	uint8_t ret;
	volatile ret_code_t err_code;
	volatile uint8_t rxbuf;
	
	err_code = i2c_read( RTC_SLAVE_ADD, RTC_CTRL_REG2, (uint8_t*)&rxbuf, sizeof( rxbuf ) );
	i2c_err_check(err_code,__LINE__);
	
	ret = rxbuf & OSC_FLAG_MASK;
	
	return ret;
}

/**
 * @brief External RTC get oscillator stop data
 * @param None
 * @retval ret Register SecondsのOS値
 */
static uint8_t ex_rtc_os_flag_clear(void)
{
	volatile ret_code_t err_code;
	volatile uint8_t buf;
	volatile uint8_t rtccheck;
	volatile uint8_t ret;
	
	err_code = i2c_read( RTC_SLAVE_ADD, RTC_CTRL_REG2, (uint8_t*)&buf, sizeof( buf ) );
	i2c_err_check(err_code,__LINE__);
	
	/* 読みだしたデータを反転して書き込む */
	/* Control Register2を0クリアする */
	buf = buf & ~OSC_CLEAR_MASK;
	
	err_code = i2c_write( RTC_SLAVE_ADD, RTC_CTRL_REG2, (uint8_t*)&buf, sizeof( buf ) );
	i2c_err_check(err_code,__LINE__);

	/* Register Seconds から読みだしたOSの値 */
	ret =  ex_rtc_os_flag_check();

	return ret;
}

/**
 * @brief External RTC Set Alert
 * @param None
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 Error
 */
static ret_code_t ex_rtc_alert_set( uint8_t reg )
{
	volatile ret_code_t err_code;
	volatile uint8_t reg_data = 0;
	volatile uint8_t ctrlreg1 = 0;
	
	/* 割り込み設定 */
	reg_data = reg;
	err_code = i2c_write( RTC_SLAVE_ADD, RTC_CTRL_REG1, (uint8_t *)&reg_data, sizeof( reg_data ) );
	i2c_err_check(err_code,__LINE__);
#if 0
	err_code = i2c_read( RTC_SLAVE_ADD, RTC_CTRL_REG1, (uint8_t*)&ctrlreg1, sizeof( ctrlreg1 ) );
	DEBUG_LOG( LOG_INFO, "ctrl reg1: 0x%x", ctrlreg1 );
#endif
	return err_code;
}

/**
 * @brief External RTC INT Function
 * @param pin Pin Number
 * @param action Interrupt Action
 * @retval  None
 */
static void rtc_int_evt_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
	/*ex rtc Interrupt event handler not use UART*/
	EVT_ST evt;
	uint32_t err_fifo;
	ret_code_t err_code;
	bool uart_output_enable;

	uart_output_enable = GetUartOutputStatus();
	if(uart_output_enable == false)
	{
		LibUartEnable();
	}
	//event push fifo
	evt.evt_id = EVT_RTC_INT;
	err_fifo = PushFifo(&evt);
	DEBUG_EVT_FIFO_LOG(err_fifo, evt.evt_id);

	err_code = i2c_init();
	if(err_code == NRF_SUCCESS)
	{
		// Alarm flag clear
		ex_rtc_alarm_flag_clear();
		/* 2022.01.26 Add GPIOの割り込みを無効に設定 ++ */
		nrfx_gpiote_in_event_disable( RTC_INT_PIN );
		/* 2022.01.26 Add GPIOの割り込みを無効に設定 -- */
		nrf_gpio_pin_latch_clear(RTC_INT_PIN);
		i2c_uninit();
	}
	else
	{
		TRACE_LOG(TR_I2C_EVT_INT_INIT_ERROR, 0);
		SetBleErrReadCmd(UTC_BLE_I2C_INIT_ERROR);
		DEBUG_LOG(LOG_ERROR,"rtc_int_evt_handler i2c init err");
		//sd_nvic_SystemReset();
	}

	if(uart_output_enable == false)
	{
		LibUartDisable();
	}
}

/**
 * @brief External RTC Interrupt Init Setting
 * @param None
 * @retval None
 */
static void ex_rtc_int_set(void)
{
	volatile ret_code_t err_code;
	nrfx_gpiote_in_config_t rtc_init_config;
	
	rtc_init_config.pull  = NRF_GPIO_PIN_PULLUP;
	rtc_init_config.sense = NRF_GPIOTE_POLARITY_HITOLO;
	rtc_init_config.is_watcher = 0;
	rtc_init_config.hi_accuracy = 0;
	rtc_init_config.skip_gpio_setup = false;
	
	err_code = nrfx_gpiote_in_init(RTC_INT_PIN, &rtc_init_config, rtc_int_evt_handler);
	LIB_ERR_CHECK(err_code, GPIOTE_INT_RTC_SET, __LINE__);
}

/**
 * @brief External RTC Alearm Disable Clear
 * @param pre_err 前回の処理結果を指定する
 * @retval None
 */
uint32_t ExRtcAlarmDisableClear(uint32_t pre_err)
{
	volatile ret_code_t err_code;
	volatile uint8_t buf;
	uint32_t device_err = UTC_SUCCESS;
	
	if(pre_err == UTC_SUCCESS)
	{
		/* I2Cの初期化は、消費電力対策のため毎回初期化する */
		err_code = i2c_init();
		if(err_code == NRF_SUCCESS)
		{
			// Alarm off and Alarm flag clear, clock out off;
			buf = ( RTC_ALARM_FLAG_OFF | RTC_ALARM_INT_OFF | RTC_CLOCK_OUT_OFF | RTC_12_24_SYSTEMS );
			err_code = i2c_write(RTC_SLAVE_ADD, RTC_CTRL_REG2, (uint8_t*)&buf, sizeof(buf));
			i2c_err_check(err_code,__LINE__);
			i2c_uninit();
			if(err_code != NRF_SUCCESS)
			{
				DEBUG_LOG(LOG_ERROR,"RTC Reset err. i2c write err");
				device_err = UTC_I2C_ERROR;
				SetBleErrReadCmd((uint8_t)UTC_BLE_I2C_INIT_ERROR);
				TRACE_LOG(TR_RTC_RESET_FAIL,0);
			}
			else
			{
				/* trace log */
				TRACE_LOG(TR_RTC_RESET_CMPL,0);
			}
		}
	}
	else
	{
		DEBUG_LOG(LOG_ERROR,"RTC Reset err. i2c init err");
		device_err = UTC_THROUGH_ERROR;
	}
	
	return device_err;
}

/**
 * @brief External RTC Set Date Time
 * @param pdatetime 設定する日時
 * @retval None
 */
void ExRtcSetDateTime(DATE_TIME *pdatetime)
{
	volatile  ret_code_t err_code;
	
	DEBUG_LOG( LOG_INFO, "Set RTC Time. YY %u, MM %u, DD %u, hh %u, mm %u, ss %u", pdatetime->year, pdatetime->month,pdatetime->day, pdatetime->hour, pdatetime->min, pdatetime->sec );
	
	//date format chnage BCD format.
	date_dtob_convert(pdatetime);
	err_code = i2c_init();
	if(err_code == NRF_SUCCESS)
	{
		err_code = i2c_write(RTC_SLAVE_ADD, RTC_SECONDS, (uint8_t*)&pdatetime->sec, sizeof(DATE_TIME));
		i2c_err_check(err_code,__LINE__);
		if(err_code != NRF_SUCCESS)
		{
			TRACE_LOG(TR_RTC_SET_TIME_ERROR,0);
			SetBleErrReadCmd(UTC_BLE_TIME_UPDATE_ERROR);
		}
		i2c_uninit();
	}
	else
	{
		TRACE_LOG(TR_RTC_SET_TIME_ERROR,1);
		SetBleErrReadCmd(UTC_BLE_TIME_UPDATE_ERROR);
	}
}

/**
 * @brief External RTC Get Date Time
 * @param pdatetime 取得する日時
 * @retval None
 */
ret_code_t ExRtcGetDateTime(DATE_TIME *pdatetime)
{
	volatile ret_code_t err_code;

	err_code = i2c_init();
	if(err_code == NRF_SUCCESS)
	{
		err_code = i2c_read( RTC_SLAVE_ADD, RTC_SECONDS, (uint8_t*)&pdatetime->sec, sizeof( DATE_TIME ) );
		i2c_err_check(err_code,__LINE__);
		if(err_code != NRF_SUCCESS)
		{	
			DEBUG_LOG( LOG_ERROR, "i2c_read() [0x%x] Cmd error: 0x%x", RTC_SECONDS, err_code );
			/*trace log*/
			/*I2C read error*/
			TRACE_LOG(TR_I2C_GET_TIME_ERROR,0);
			SetBleErrReadCmd(UTC_BLE_I2C_ERROR);
		}
		else
		{
			//Binary -> DEC
			date_btod_convert(pdatetime);
			DEBUG_LOG( LOG_INFO, "Get RTC Time. YY %u, MM %u, DD %u, hh %u, mm %u, ss %u", pdatetime->year, pdatetime->month,pdatetime->day, pdatetime->hour, pdatetime->min, pdatetime->sec );
		}

//debug test		
#ifdef TEST_I2C_ERROR
		ret_code_t tmp_err_code;
		tmp_err_code = NRF_ERROR_BUSY;
		i2c_err_check(tmp_err_code,__LINE__);
#endif
		i2c_uninit();
	}
	else
	{
		/*trace log*/
		TRACE_LOG(TR_I2C_GET_TIME_ERROR,1);
		/*I2C init error*/
		DEBUG_LOG(LOG_ERROR,"get rtc datetime i2c init err");
	}
	
	return err_code;
}

/**
 * @brief External RTC oscillator check
 * @param None
 * @retval true oscillator on
 * @retval false oscillator off
 */
bool ExRtcOscCheck(void)
{
	bool osc_stopped_flag;
	uint8_t rtcOSC;
	ret_code_t err_code;
	
	osc_stopped_flag = true;
	
	err_code = i2c_init();
	if(err_code == NRF_SUCCESS)
	{
		// ex RTC OSC stabilaization check
		rtcOSC = ex_rtc_os_flag_check();
		//DEBUG_LOG(LOG_DEBUG, "curr OSC flag 0x%x", rtcOSC);
		DEBUG_LOG( LOG_INFO, "curr OSC flag 0x%x", rtcOSC );
		if(RTC_OSC_CLEARED != rtcOSC)
		{
			osc_stopped_flag = true;
			while(1)
			{
				// exRTC OSC flag clear. OSC flag cleared, break. not cleared, wdt reset.
				rtcOSC = ex_rtc_os_flag_clear();
				if(RTC_OSC_CLEARED  == rtcOSC)
				{
					DEBUG_LOG(LOG_DEBUG,"exRTC OSC flag clear");
					break;
				}
				TRACE_LOG(TR_RTC_OSC_FLAG_ERROR,0);
			}			
		}
		else
		{
			osc_stopped_flag = false;
		}
		i2c_uninit();
	}
	else
	{
		TRACE_LOG(TR_RTC_OSC_FLAG_ERROR,1);
	}
	
	return osc_stopped_flag;
}

/**
 * @brief External RTC 
 * @param None
 * @retval 
 */
uint32_t ExRtcSetUp(uint32_t pre_err)
{
	ret_code_t      err;
	ret_code_t    init_err;
	uint32_t    rtc_err;

	rtc_err = UTC_SUCCESS;
	
	if(pre_err == UTC_SUCCESS)
	{
		init_err = i2c_init();
		if(init_err == NRF_SUCCESS)
		{
			/* 2022.05.16 Add RTC起動の場合、割込フラグをクリアする(自動接続対応) ++ */
			if ( g_rtc_wakeup == true )
			{
				g_rtc_wakeup = false;
				DEBUG_LOG( LOG_INFO, "!!! Clear Interrupt Flag !!!" );
				// Alarm flag clear
				ex_rtc_alarm_flag_clear();
			}
			/* 2022.05.16 Add RTC起動の場合、割込フラグをクリアする(自動接続対応) -- */
			
			/* Alarm有効化 */
			err = ex_rtc_alert_set( RTC_AALE_ENABLE | RTC_PERIODIC_INTERRUPT );
			if(err != NRF_SUCCESS)
			{
				rtc_err = 1;
			}
			if(rtc_err == UTC_SUCCESS)
			{
				/* RTC Interrupt Setup */
				ex_rtc_int_set();
				nrfx_gpiote_in_event_enable(RTC_INT_PIN, true); 
			}
			
			//register_read_test();
			
			i2c_uninit();
		}
		
		if(rtc_err != UTC_SUCCESS)
		{
			DEBUG_LOG(LOG_ERROR,"exRTC Set Up Error");
			TRACE_LOG(TR_RTC_INIT_FAIL,0);
			rtc_err = UTC_I2C_ERROR;
			SetBleErrReadCmd((uint8_t)UTC_BLE_I2C_INIT_ERROR);
		}
		else
		{
			TRACE_LOG(TR_RTC_INIT_CMPL,0);
			DEBUG_LOG(LOG_INFO,"exRTC Set Up");
		}
	}
	else
	{
		rtc_err = UTC_THROUGH_ERROR;
	}
	
	return rtc_err;
}

/**
 * @brief External RTC Setup Default Date
 * @param None
 * @retval None
 */
void ExRtcSetDefaultDateTime( void )
{
	DATE_TIME default_time;
	ret_code_t err_code;

	/* RTCデフォルト設定 */
	default_time.year	= DEFAULT_YEAR;
	default_time.month	= DEFAULT_MONTH;
	default_time.day	= DEFAULT_DAY ;
	default_time.hour	= DEFAULT_HOUR;
	default_time.min	= DEFAULT_MIN;
	default_time.sec	= DEFAULT_SEC;
	default_time.week	= DEFAULT_WEEKDAYS;
	
	//date format chnage BCD format.
	date_dtob_convert( &default_time );

	err_code = i2c_init();
	if(err_code == NRF_SUCCESS)
	{
		DEBUG_LOG( LOG_INFO, "%d.%d.%d %d:%d:%d", default_time.year, default_time.month, default_time.day, default_time.hour, default_time.min, default_time.sec );
		
		err_code = i2c_write( RTC_SLAVE_ADD, RTC_SECONDS, (uint8_t*)&default_time, sizeof(DATE_TIME));
		i2c_err_check(err_code,__LINE__);
		if(err_code != NRF_SUCCESS)
		{
			DEBUG_LOG( LOG_ERROR,"exRTC Default Time Error");
			TRACE_LOG( TR_RTC_SET_DEFTIME_ERROR, 0 );
		}
		
		i2c_uninit();
	}
	else
	{
		TRACE_LOG(TR_RTC_SET_TIME_ERROR,1);
	}
}

/* 2022.01.26 Add RawData送信中のRTC Interrupt抑止処理追加 ++ */
/**
 * @brief External RTC Interrupt Enable
 * @param None
 * @retval None
 */
void ExRtcEnableInterrupt( void )
{
	ret_code_t err_code;
	
	err_code = i2c_init();
	if(err_code == NRF_SUCCESS)
	{
		/* 割り込みを有効化 */
		err_code = ex_rtc_alert_set( RTC_AALE_ENABLE | RTC_PERIODIC_INTERRUPT );
		i2c_err_check( err_code,__LINE__ );
		if(err_code != NRF_SUCCESS)
		{
			DEBUG_LOG( LOG_ERROR,"exRTC Interrupt Enable Error");
			TRACE_LOG( TR_RTC_ENABLE_INT_ERROR, 0 );
		}
		
		i2c_uninit();

		/* RTC Interrupt Setup */
		nrfx_gpiote_in_event_enable(RTC_INT_PIN, true); 
	}
	else
	{
		TRACE_LOG( TR_RTC_ENABLE_INT_ERROR, 1 );
	}
}

/**
 * @brief External RTC Interrupt Disable
 * @param None
 * @retval None
 */
void ExRtcDisableInterrupt( void )
{
	ret_code_t err_code;
	
	err_code = i2c_init();
	if(err_code == NRF_SUCCESS)
	{
		/* 割り込みを無効化 */
		err_code = ex_rtc_alert_set( RTC_AALE_DISABLE | RTC_PERIODIC_INT_DISABLE );
		i2c_err_check( err_code,__LINE__ );
		if(err_code != NRF_SUCCESS)
		{
			DEBUG_LOG( LOG_ERROR,"exRTC Interrupt Disable Error");
			TRACE_LOG( TR_RTC_DISABLE_INT_ERROR, 0 );
		}
		
		i2c_uninit();
	}
	else
	{
		TRACE_LOG( TR_RTC_DISABLE_INT_ERROR, 1 );
	}
	/* GPIOの割り込みも無効に設定しておく */
	nrfx_gpiote_in_event_disable( RTC_INT_PIN );
}
/* 2022.01.26 Add RawData送信中のRTC Interrupt抑止処理追加 -- */

/* 2022.05.16 Add 自動接続対応 ++ */
/**
 * @brief RTC Interrupt PIN WakeUp Setting
 * @param None
 * @retval NRF_SUCCESS 成功
 */
uint32_t ExRtcWakeUpSetting( void )
{
	ret_code_t err_code;
	
	/* GPIO割込終了処理 */
	nrfx_gpiote_in_uninit( RTC_INT_PIN );
	
	/* Setup GPIO WakeUp */
	if ( NRF_GPIO->DETECTMODE != 1 )
	{
		/* latch enale */
		NRF_GPIO->DETECTMODE = 1;
	}
	nrf_gpio_cfg_input( RTC_INT_PIN, NRF_GPIO_PIN_PULLUP );
	nrf_gpio_cfg_sense_set( RTC_INT_PIN, NRF_GPIO_PIN_SENSE_LOW );
	nrf_gpio_pin_latch_clear( RTC_INT_PIN );
	
	/* RTC Setting */
	err_code = i2c_init();
	if ( err_code == NRF_SUCCESS )
	{
		/* 割り込みを有効化 */
		err_code = ex_rtc_alert_set( RTC_AALE_ENABLE | RTC_PERIODIC_INTERRUPT );
		i2c_err_check( err_code,__LINE__ );
		if ( err_code != NRF_SUCCESS )
		{
			DEBUG_LOG( LOG_ERROR,"exRTC Interrupt Enable Error");
			TRACE_LOG( TR_RTC_ENABLE_INT_ERROR, 0 );
		}
		
		i2c_uninit();
	}
	
	DEBUG_LOG( LOG_INFO, "!!! RTC WakeUp Setup Complete: %d !!!", err_code );

	return err_code;
}
/* 2022.05.16 Add 自動接続対応 -- */

/* 2022.05.16 Add 自動接続対応 ++ */
/**
 * @brief RTC WakeUp Check
 * @param None
 * @retval true RTC WakeUpで起動
 * @retval false RTC WakeUpではない
 */
bool ExRtcWakeUpCheck( void )
{
	uint32_t read_state = 0;
	bool state = false;
	
	/* RTC Interrupt確認 */
	read_state = nrf_gpio_pin_latch_get( RTC_INT_PIN );
	if ( read_state == 1 )
	{
		DEBUG_LOG( LOG_INFO, "!!! RTC Int WakeUp !!!" );
		/* RTC割込での起動 */
		g_rtc_wakeup = true;
		/* RTC WakeUp */
		state = true;
	}
	
	return state;
}
/* 2022.05.16 Add 自動接続対応 -- */

/* 2022.07.21 Add RTC異常検知のため ++ */
/**
 * @brief RTC Timer Check
 * @param p_currnet_time 取得した時間
 * @param p_flash_time Flashに保存されていた時刻
 * @retval true 問題なし
 * @retval false 時刻異常
 */
bool ExRtcCheckDateTime( DATE_TIME *p_currnet_time,  DATE_TIME *p_flash_time )
{
	bool validate_time = true;

	if ( ( p_currnet_time != NULL ) && ( p_flash_time != NULL ) )
	{
		DEBUG_LOG( LOG_INFO, "[Current] %d.%d.%d,%d:%d vs [Flash] %d.%d.%d,%d:%d",
			p_currnet_time->year, p_currnet_time->month, p_currnet_time->day, p_currnet_time->hour, p_currnet_time->min,
			p_flash_time->year, p_flash_time->month, p_flash_time->day, p_flash_time->hour, p_flash_time->min );
			
		if ( p_currnet_time->year != p_flash_time->year )
		{
			validate_time = false;
		}
		if ( p_currnet_time->month != p_flash_time->month )
		{
			validate_time = false;
		}
		if ( p_currnet_time->day != p_flash_time->day )
		{
			validate_time = false;
		}
		/* 時確認 */
		if ( p_currnet_time->hour != p_flash_time->hour )
		{
			validate_time = false;
		}
		/* 分確認 */
		if ( 59 < p_currnet_time->min )
		{
			validate_time = false;
		}
	}
#ifdef TEST_EXRTC_CHECK_DATEERR
	validate_time = false;
#endif
	
	return validate_time;
}
/* 2022.07.21 Add RTC異常検知のため -- */

