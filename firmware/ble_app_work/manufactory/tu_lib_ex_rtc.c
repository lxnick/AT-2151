/**
  ******************************************************************************************
  * Reference source: lib_ex_rtc.c
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
#include "tu_lib_ex_rtc.h"
#include "tu_state_control.h"
#include "tu_lib_fifo.h"

/* Definition ------------------------------------------------------------*/
#define WAIT_EVENT_INTERVAL		(100000)		/* I2Cの応答待ち時間 */

#define I2C_DRV_FREQ_390K		(0x06200000)	/* Errata No.219対応 */

/* private variables -----------------------------------------------------*/
const nrf_drv_twi_t tu_gRtc_twi = NRF_DRV_TWI_INSTANCE(1);		/* SPIMとTWIMはレジスタを共有しているためSPI0とTWI1にする */
const nrf_drv_twi_config_t tu_twi_config = {
	.scl				= I2C_CLK_PIN,		/* PIN: P0.14 SCL */
	.sda				= I2C_SDA_PIN,		/* PIN: P0.15 SDA */
	.frequency			= (nrf_drv_twi_frequency_t)NRF_DRV_TWI_FREQ_100K,	/* 2022.01.11 Modify 390KHz -> 100KHz */
	.interrupt_priority	= APP_IRQ_PRIORITY_HIGH,
	.clear_bus_init		= false
};

static volatile bool tu_g_event = false;
static volatile uint16_t tu_g_res_state = 0;

/* 2022.05.16 Add 自動接続対応 ++ */
static volatile bool tu_g_rtc_wakeup = false;
/* 2022.05.16 Add 自動接続対応 -- */

/* private prototypes ----------------------------------------------------*/
/**
 * @brief Read/Write Callback function
 * @param p_event event information
 * @param p_context context
 * @retval None
 */
static void TU_twi_event_handler(nrf_drv_twi_evt_t const * p_event, void * p_context);

/**
 * @brief wait interrupt event
 * @param none
 * @retval true Interrupt Event receive
 * @retval false Interrupt Event non-receive
 */
static bool TU_wait_event( void );


/**
 * @brief Uninitialize I2C
 * @param None
 * @retval None
 */
static void TU_i2c_uninit(void)
{
	nrf_drv_twi_disable(&tu_gRtc_twi);
	nrf_drv_twi_uninit(&tu_gRtc_twi);
}

/**
 * @brief I2C Write
 * @param slave_addr Slave Address
 * @param reg_addr Register Address
 * @param ptxdata 送信データ
 * @param size 送信データサイズ
 * @retval NRF_SUCCESS Success
 */
static ret_code_t TU_i2c_write(uint8_t slave_addr, uint8_t reg_addr, uint8_t *ptxdata, uint8_t size)
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
		tu_g_event = false;
		memcpy((uint8_t*)&txbuf[1], ptxdata, size);
		err_code = nrf_drv_twi_tx(&tu_gRtc_twi, slave_addr, (uint8_t*)&txbuf[0], (size + 1), false);
		if ( err_code == NRF_SUCCESS )
		{
			/* ACK待ち */
			event = TU_wait_event();
			if ( event == false )
			{
				return NRFX_ERROR_BUSY;
			}
			switch ( tu_g_res_state )
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
static ret_code_t TU_i2c_read(uint8_t slave_addr, uint8_t reg_addr, uint8_t *prxdata, uint8_t size)
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
		tu_g_event = false;
		tu_g_res_state = 0;

		buf = reg_addr;
		
		/* 2022.01.26 Add Read Retry ++ */
		for ( i = 0; i < 10; i++ )
		{
			err_code = nrf_drv_twi_tx(&tu_gRtc_twi, slave_addr, (uint8_t*)&buf, sizeof(buf), true);
			if(err_code == NRF_SUCCESS)
			{
				/* ACK待ち */
				event = TU_wait_event();
				if ( event == false )
				{
					return NRFX_ERROR_BUSY;
				}
				switch ( tu_g_res_state )
				{
				case 1:	err_code = NRFX_ERROR_DRV_TWI_ERR_ANACK;	break;
				case 2: err_code = NRFX_ERROR_DRV_TWI_ERR_DNACK;	break;
				}

				if ( err_code == NRF_SUCCESS )
				{
					tu_g_event = false;
					tu_g_res_state = 0;
					/* Read */
					err_code = nrf_drv_twi_rx(&tu_gRtc_twi, slave_addr, prxdata, size);
					if ( err_code == NRF_SUCCESS )
					{
						/* ACK待ち */
						event = TU_wait_event();
						if ( event == false )
						{
							return NRFX_ERROR_BUSY;
						}
						switch ( tu_g_res_state )
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
static void TU_twi_event_handler(nrf_drv_twi_evt_t const * p_event, void * p_context)
{
	switch ( p_event->type )
	{
	case NRF_DRV_TWI_EVT_DONE:
		tu_g_res_state = 0;
		/* chack xfer type */
		switch ( p_event->xfer_desc.type )
		{
		case NRF_DRV_TWI_XFER_TX:
			tu_g_event = true;
			break;
		case NRF_DRV_TWI_XFER_TXTX:
			tu_g_event = true;
			break;
		case NRF_DRV_TWI_XFER_RX:
			tu_g_event = true;
			break;
		case NRF_DRV_TWI_XFER_TXRX:
			tu_g_event = true;
			break;
		default: break;
		}
		break;
	case NRF_DRV_TWI_EVT_ADDRESS_NACK:
		tu_g_event = true;
        tu_g_res_state = 1;
		break;
	case NRF_DRV_TWI_EVT_DATA_NACK:
		tu_g_event = true;
        tu_g_res_state = 2;
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
static bool TU_wait_event( void )
{
	uint32_t i;
	bool event_state = false;
	
	for ( i = 0; i < WAIT_EVENT_INTERVAL; i++ )
	{
		nrf_delay_us(1);
		if ( tu_g_event == true )
		{
			event_state = true;
			break;
		}
	}
	return event_state;
}
#if 0
static void TU_register_read_test( void )
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
 * @brief BCD convert. Binary to Dec convert
 * @param data Binary Data
 * @retval None
 */
static void TU_btod_convert(uint8_t *data)
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
static void TU_date_btod_convert(DATE_TIME *pdatetime)
{
	//sec convert.
	pdatetime->sec &= EX_RTC_SEC_GET_MASK ;
	TU_btod_convert(&pdatetime->sec);
	
	//min convert.
	pdatetime->min &= EX_RTC_MIN_GET_MASK;
	TU_btod_convert(&pdatetime->min);
	
	//hour
	pdatetime->hour &= EX_RTC_HOUR_GET_MASK;
	TU_btod_convert(&pdatetime->hour);
	
	//day
	pdatetime->day &= EX_RTC_DAY_GET_MASK;
	TU_btod_convert(&pdatetime->day);
	
	//weekend
	//Not change Binary -> Dec
	pdatetime->week &= 0x07;
	
	//month
	pdatetime->month &= EX_RTC_MONTH_GET_MASK;
	TU_btod_convert(&pdatetime->month);
	
	//year
	pdatetime->year &= EX_RTC_YEAR_GET_MASK;
	TU_btod_convert(&pdatetime->year);
}

/**
 * @brief BCD convert. Dec to Binary convert
 * @param data Dec Data
 * @retval None
 */
static void TU_dtob_convert(uint8_t *data)
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
static void TU_date_dtob_convert(DATE_TIME *pdatetime)
{
	TU_dtob_convert(&pdatetime->sec);		//sec
	TU_dtob_convert(&pdatetime->min);		//min
	TU_dtob_convert(&pdatetime->hour);		//hour
	TU_dtob_convert(&pdatetime->day);		//day
	TU_dtob_convert(&pdatetime->month);	//month
	TU_dtob_convert(&pdatetime->year);		//yert
}

static ret_code_t TU_Manufactory_i2c_init(void)
{
	ret_code_t err_code;

	/* debug test */

	err_code = nrf_drv_twi_init(&tu_gRtc_twi, &tu_twi_config, TU_twi_event_handler, NULL);
	nrf_drv_twi_enable(&tu_gRtc_twi);
	
	return err_code;
}

/**
 * @brief External RTC get oscillator stop data
 * @param None
 * @retval ret Register SecondsのOS値
 */
static ret_code_t TU_manufactory_rtc_os_flag_check(uint8_t *oscflag)
{
	volatile ret_code_t err_code;
	volatile uint8_t rxbuf;
	
	err_code = TU_i2c_read( RTC_SLAVE_ADD, RTC_CTRL_REG2, (uint8_t*)&rxbuf, sizeof( rxbuf ) );
	
	*oscflag = rxbuf & OSC_FLAG_MASK;
	
	return err_code;
}

/**
 * @brief External RTC get oscillator stop data
 * @param None
 * @retval ret Register SecondsのOS値
 */
static uint8_t TU_manufactory_rtc_os_flag_clear(void)
{
	volatile ret_code_t err_code;
	volatile uint8_t buf;
	volatile uint8_t rtccheck;
	
	err_code = TU_i2c_read( RTC_SLAVE_ADD, RTC_CTRL_REG2, (uint8_t*)&buf, sizeof( buf ) );
	
	/* 読みだしたデータを反転して書き込む */
	/* Control Register2を0クリアする */
	buf = buf & ~OSC_CLEAR_MASK;
	
	err_code = TU_i2c_write( RTC_SLAVE_ADD, RTC_CTRL_REG2, (uint8_t*)&buf, sizeof( buf ) );

	/* Register Seconds から読みだしたOSの値 */
	err_code =  TU_manufactory_rtc_os_flag_check((uint8_t *)&rtccheck);

	return rtccheck;
}

/**
 * @brief External RTC oscillator check
 * @param None
 * @retval true oscillator on
 * @retval false oscillator off
 */
ret_code_t TU_ManufactoryRtcSetDefault(void)
{
	uint8_t rtcOSC;
	ret_code_t err_code;
    DATE_TIME default_time;

	/* RTCデフォルト設定 */
	default_time.year	= DEFAULT_YEAR;
	default_time.month	= DEFAULT_MONTH;
	default_time.day	= DEFAULT_DAY ;
	default_time.hour	= DEFAULT_HOUR;
	default_time.min	= DEFAULT_MIN;
	default_time.sec	= DEFAULT_SEC;
	default_time.week	= DEFAULT_WEEKDAYS;
	
	//date format chnage BCD format.
	TU_date_dtob_convert( &default_time );
	
	err_code = TU_Manufactory_i2c_init();
	if(err_code == NRF_SUCCESS)
	{
		// ex RTC OSC stabilaization check
		err_code = TU_manufactory_rtc_os_flag_check(&rtcOSC);
        if(err_code == NRF_SUCCESS)
        {
            if(RTC_OSC_CLEARED != rtcOSC)
            {
                while(1)
                {
                    // exRTC OSC flag clear. OSC flag cleared, break. not cleared, wdt reset.
                    rtcOSC = TU_manufactory_rtc_os_flag_clear();
                    if(RTC_OSC_CLEARED  == rtcOSC)
                    {
//                        DEBUG_LOG(LOG_DEBUG,"exRTC OSC flag clear");
                        break;
                    }
                }

                // Set default Y/M/D/h/m/s
                err_code = TU_i2c_write( RTC_SLAVE_ADD, RTC_SECONDS, (uint8_t*)&default_time, sizeof(DATE_TIME));
            }

        }
		TU_i2c_uninit();
	}
	
	return err_code;
}

ret_code_t TU_ManufactoryRtcGetDateTime(DATE_TIME *pdatetime)
{
	volatile ret_code_t err_code;

	err_code = TU_Manufactory_i2c_init();
	if(err_code == NRF_SUCCESS)
	{
		err_code = TU_i2c_read( RTC_SLAVE_ADD, RTC_SECONDS, (uint8_t*)&pdatetime->sec, sizeof( DATE_TIME ) );
//		TU_i2c_err_check(err_code,__LINE__);
		if(err_code == NRF_SUCCESS)
		{
			//Binary -> DEC
			TU_date_btod_convert(pdatetime);
		}

		TU_i2c_uninit();
	}
	
	return err_code;
}


