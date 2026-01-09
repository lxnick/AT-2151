/**
  ******************************************************************************************
  * @file    lib_common.c
  * @author  k.tashiro
  * @version 1.0
  * @date    2020/09/10
  * @brief   Library Common
  ******************************************************************************************
    Revision:
    Ver            Date             Revised by        Explanation
  ******************************************************************************************
  * 1.0            2020/09/10       k.tashiro         create new
  ******************************************************************************************
*/

/* Includes --------------------------------------------------------------*/
#include "lib_common.h"
#include "lib_flash.h"
#include "lib_ram_retain.h"
#include "lib_wdt.h"

/* Definition ------------------------------------------------------------*/
#define I2C_BUS_CHECK_FREQ_100KHZ	(10)	// uint[us]
#define I2C_BUS_CHECK_RETRY_LIMIT	(3)		// unit[retry number]

#define GPIO_HIGH_LEVEL		(1)
#define GPIO_LOW_LEVEL		(0)

#define DIVIDE_WAIT_LOOP_NUM 100

/* Private variables -----------------------------------------------------*/
volatile bool g_uart_output_enable = true;
volatile bool g_acc_read_enable = true;

/* Private function prototypes -------------------------------------------*/
/**
 * @brief UICR Erase Wait
 * @param None
 * @retval None
 */
static void uicr_erase_wait(void);

/**
 * @brief UICR Write Wait
 * @param None
 * @retval None
 */
static void uicr_write_wait(void);

/**
 * @brief UICR Register Print
 * @param None
 * @retval None
 */
static void uicr_printf(void);

/**
 * @brief UART Output Enable
 * @param None
 * @retval None
 */
static void uart_output_enable(void)
{
	g_uart_output_enable = true;
}

/**
 * @brief UART Output Disable
 * @param None
 * @retval None
 */
static void uart_output_disable(void)
{
	g_uart_output_enable = false;
}

/**
 * @brief Get UART Output Status
 * @param None
 * @retval g_uart_output_enable
 *			- true 有効
 *			- false 無効
 */
bool GetUartOutputStatus(void)
{
	return g_uart_output_enable;
}

/**
 * @brief I2C Check Bus
 * @param None
 * @retval None
 */
void I2CBusCheck(void)
{
	uint32_t read_sda;
	uint8_t i;
	uint8_t retry_count = 0;
	
	/*GPIO setting, sda inpu, scl output*/
	nrf_gpio_cfg_input(I2C_SDA_PIN, NRF_GPIO_PIN_PULLUP);		//sda pin input configure
	nrf_gpio_pin_write(I2C_CLK_PIN ,GPIO_HIGH_LEVEL);
	nrf_gpio_cfg_output(I2C_CLK_PIN);
	nrf_delay_us(I2C_BUS_CHECK_FREQ_100KHZ);
	nrf_gpio_pin_write(I2C_CLK_PIN, GPIO_LOW_LEVEL);
	nrf_delay_us(I2C_BUS_CHECK_FREQ_100KHZ);
	
	/* Read SDA Pin */
	read_sda = nrf_gpio_pin_read( I2C_SDA_PIN );
	if( read_sda == GPIO_LOW_LEVEL )
	{
		/* SDA PinがLow Levelだった場合 */
		TRACE_LOG( TR_I2C_BUS_RECOVERY_HAPPEN, 0 );
		while(1)
		{
			/* Hi*/
			nrf_gpio_pin_write( I2C_CLK_PIN, GPIO_HIGH_LEVEL );
			nrf_delay_us( I2C_BUS_CHECK_FREQ_100KHZ );
			for(i = 0; i < 8; i++)
			{
				nrf_gpio_pin_write(I2C_CLK_PIN, GPIO_LOW_LEVEL);
				nrf_delay_us(I2C_BUS_CHECK_FREQ_100KHZ);
				nrf_gpio_pin_write(I2C_CLK_PIN, GPIO_HIGH_LEVEL);
				nrf_delay_us(I2C_BUS_CHECK_FREQ_100KHZ);
			}
			nrf_gpio_pin_write( I2C_CLK_PIN, GPIO_LOW_LEVEL );
			nrf_delay_us( I2C_BUS_CHECK_FREQ_100KHZ );

			read_sda = nrf_gpio_pin_read( I2C_SDA_PIN );
			if( read_sda != GPIO_LOW_LEVEL )
			{
				//check seaquence break. sda level high
				TRACE_LOG( TR_I2C_BUS_RECOVERY, 0 );
				break;
			}
			else
			{
				if( retry_count < I2C_BUS_CHECK_RETRY_LIMIT )
				{
					/* check retry count increments */
					retry_count++;
				}
				else
				{
					LIB_ERR_CHECK( I2C_BUS_CEHCK_ERROR_CODE, I2C_BUS_ERROR, __LINE__ );
				}
			}
		}
	}

	nrf_gpio_pin_write( I2C_SDA_PIN, GPIO_LOW_LEVEL );
	/* I2C SDA pin change output cfg. */
	nrf_gpio_cfg_output(I2C_SDA_PIN);
	nrf_delay_us( I2C_BUS_CHECK_FREQ_100KHZ );
	nrf_gpio_pin_write( I2C_CLK_PIN, GPIO_HIGH_LEVEL );
	nrf_delay_us( I2C_BUS_CHECK_FREQ_100KHZ );
	nrf_gpio_pin_write( I2C_SDA_PIN, GPIO_HIGH_LEVEL );
}

/**
 * @brief UART Disable
 * @param None
 * @retval None
 */
void LibUartEnable(void)
{
	NVIC_SetPriority(UARTE0_UART0_IRQn,UART_DEFAULT_CONFIG_IRQ_PRIORITY);
	NVIC_ClearPendingIRQ(UARTE0_UART0_IRQn);
	NVIC_EnableIRQ(UARTE0_UART0_IRQn);
	NRF_UARTE0->ENABLE = UARTE_ENABLE_ENABLE_Enabled;
	//NRF_UARTE0->TASKS_STARTTX = 1;		//20180813 add
}

/**
 * @brief UART Disable
 * @param None
 * @retval None
 */
void LibUartDisable(void)
{
	NVIC_DisableIRQ(UARTE0_UART0_IRQn);
	//NRF_UARTE0->TASKS_STOPTX = 1;				//20180813 add
	NRF_UARTE0->ENABLE = UARTE_ENABLE_ENABLE_Disabled;
}

/**
 * @brief Pre Sleep UART
 * @param None
 * @retval None
 */
void PreSleepUart(void)
{
	volatile uint32_t err_code;
	uint8_t  preSleepCriticalSession;
	
	err_code = sd_nvic_critical_region_enter(&preSleepCriticalSession);
	if(err_code == NRF_SUCCESS)
	{
		uart_output_disable();
		LibUartDisable();
		preSleepCriticalSession = 0;
		err_code = sd_nvic_critical_region_exit(preSleepCriticalSession);
		if(err_code != NRF_SUCCESS)
		{
			//nothing to do.
		}
	}
}

/**
 * @brief Wake Up UART
 * @param None
 * @retval None
 */
void WakeUpUart(void)
{
	uint32_t err_code;
	uint8_t  wakeupCriticalSession;
	
	err_code = sd_nvic_critical_region_enter(&wakeupCriticalSession);
	if(err_code == NRF_SUCCESS)
	{
		uart_output_enable();
		LibUartEnable();
		wakeupCriticalSession = 0;
		err_code = sd_nvic_critical_region_exit(wakeupCriticalSession);
		if(err_code != NRF_SUCCESS)
		{
			//Nothing to do.
		}
	}
}

/**
 * @brief External RTC Time Rest to RAM
 * @param pre_err 前回の処理結果を指定する
 * @param reset_reason リセット要因
 * @retval UTC_SUCCESS Success
 * @retval UTC_THROUGH_ERROR Error
 */
uint32_t RamExRtcTimeReset(uint32_t pre_err, uint32_t reset_reason)
{
	bool bRamSignCheck;
	bool bPlayerRet;
	bool bPastRet;
	bool bRtcTimeRet;
	bool osc_stopped_flag;
	uint8_t age;
	uint8_t sex;
	uint8_t weight;
	DATE_TIME currDateTime;
	DATE_TIME flash_date_time;
	ret_code_t rtc_err;
	//wake up process
	
	// ram save check
	bRamSignCheck = RamSignCheck();
	if(bRamSignCheck != CHECK_VALID)
	{
		/* ram save region err */
		TRACE_LOG(TR_RAM_SIGN_ERROR,0);	
		CreateRamSaveDefault();
		/* read flash and set last sid to ram */
		SetFlashSid();
		// read player info from flash and check
		bPlayerRet = CheckPlayer(&age, &sex, &weight);
		if(bPlayerRet == CHECK_INVALID)
		{
			// plyer info correct?
			// No!! Ram save region set default player info
			SetRamPlayerInfo(DEFAULT_AGE, DEFAULT_SEX, DEFAULT_WEIGHT);
		}
		else
		{
			// Ram save region set plyaer info from flash retain player info set.
			SetRamPlayerInfo(age, sex, weight);
		}
	}
	else
	{
		DebugDisplayRamSave();
		GetSidFromRamRegion();
		SetAlgoPlayerInfo();
	}
	WdtReload();
	
	if(pre_err != UTC_SUCCESS)
	{
		return UTC_THROUGH_ERROR;
	}
	
	// RTC OSC check
	osc_stopped_flag = ExRtcOscCheck();
	WdtReload();
	// ex rtc stop?
	if(osc_stopped_flag == false)
	{
		// Get time
		rtc_err = ExRtcGetDateTime(&currDateTime);
		if(rtc_err != NRF_SUCCESS)
		{
			return UTC_I2C_ERROR;
		}
		
		if(bRamSignCheck == CHECK_VALID)
		{
			// No!! check rtc time, compare ram save region.
			bPastRet = CheckCurrTime(&currDateTime);
			if(bPastRet == CHECK_VALID)
			{
				// write daily log
				SetDailyLog(&currDateTime);
				InitRamDateSet(&currDateTime);
			}
		}
		else
		{
			/* 2022.07.21 Add 時刻チェックを追加し不正な場合初期化やFlashからの設定を行う ++ */
			/* 電源ON or WDT or Reset */
			if ( ( reset_reason == RESETPIN ) || ( reset_reason == RESETDOG ) || ( reset_reason == RESETLOCKUP ) ||
				 ( reset_reason == RESETPOR_BOR ) )
			{
				/* Flashの確認 */
				rtc_err = GetTimeFromFlashDateTime( &flash_date_time );
				if ( rtc_err == 0 )
				{
					/* Flashに設定有り */
					/* 時刻確認 */
					bRtcTimeRet = ExRtcCheckDateTime( &currDateTime, &flash_date_time );
					if ( bRtcTimeRet != CHECK_VALID )
					{
						/* 時刻異常の場合、Flashに保持されている時刻を書き込む */
						// FlashのTimeDataをRTCに書き込む
						rtc_err = SetRtcFromFlashDateTime();
						TRACE_LOG( TR_RTC_TIME_SET_FROM_FLASH, 0 );
					}
				}
				else if ( rtc_err == 1 )
				{
					/* Flashに時刻設定なしのためデフォルト時刻を設定 */
					ExRtcSetDefaultDateTime();
				}
				
				/* Flashの時刻を設定したので再度取得 */
				rtc_err = ExRtcGetDateTime(&currDateTime);
				if(rtc_err != NRF_SUCCESS)
				{
					return UTC_I2C_ERROR;
				}
			}
			/* 2022.07.21 Add 時刻チェックを追加し不正な場合初期化やFlashからの設定を行う -- */
			InitRamDateSet(&currDateTime);
		}
	}
	else
	{
		if(bRamSignCheck == CHECK_VALID)
		{
			// RAMのTimeDataをRTCに書き込む
			SetRtcFromRamDateTime();
			//trace log 
			TRACE_LOG(TR_RTC_TIME_SET_FROM_RAM,0);
		}
		else
		{
			// FlashのTimeDataをRTCに書き込む
			SetRtcFromFlashDateTime();
			TRACE_LOG(TR_RTC_TIME_SET_FROM_FLASH,0);
		}
	}
	return UTC_SUCCESS;
}

/**
 * @brief Reset Reason Check
 * @param p_reset_reason Reset Reason
 * @retval None
 */
void ResetReasonCheck(uint32_t *p_reset_reason)
{
	uint32_t reset_reason;
	uint32_t err_code;
	
	/* Get Reset Reason */
	err_code = sd_power_reset_reason_get(&reset_reason);
	LIB_ERR_CHECK(err_code, RESET_REASON, __LINE__);
	reset_reason = reset_reason & RESET_MASK;
	if ( p_reset_reason != NULL )
	{
		*p_reset_reason = reset_reason;
	}

	switch (reset_reason)
	{
		case RESETPIN:
			DEBUG_LOG(LOG_INFO,"Reset reason [RESET PIN] 0x%x",reset_reason);
			break;
		case RESETDOG:
			DEBUG_LOG(LOG_INFO,"Reset reason [RESET WDT] 0x%x",reset_reason);
			break;
		case RESETSREQ:
			DEBUG_LOG(LOG_INFO,"Reset reason [RESET soft reset] 0x%x",reset_reason);
			break;
		case RESETLOCKUP:
			DEBUG_LOG(LOG_INFO,"Reset reason [RESET lock up] 0x%x",reset_reason);
			break;
		case RESETSYSOFF:
			DEBUG_LOG(LOG_INFO,"Reset reason [RESET system off mode] 0x%x",reset_reason);
			break;
		case RESETSYSOFF_DEBUG:
			DEBUG_LOG(LOG_INFO,"Reset reason [RESET system off mode(debug interface)] 0x%x",reset_reason);
			break;
		case RESETPOR_BOR:
			DEBUG_LOG(LOG_INFO,"Reset reason [POR or BOR] 0x%x",reset_reason);
			break;
		default:
			DEBUG_LOG(LOG_INFO,"Reset reason [UNKNOWN] 0x%x",reset_reason);
			break;
	}

	sd_power_reset_reason_clr(RESET_RESON_REG_CLEAR);
}

/**
 * @brief GPIOTE Latch cler
 * @param None
 * @retval None
 */
void GpioteLatchClear(void)
{
	nrf_gpio_pin_latch_clear(ACC_INT1_PIN);
	nrf_gpio_pin_latch_clear(ACC_INT2_PIN);
	nrf_gpio_pin_latch_clear(RTC_INT_PIN);
}

/**
 * @brief GPIO Setting
 * @param None
 * @retval None
 */
void GpioSetting(void)
{
	uint8_t i;
	nrfx_err_t err_code;
	// unused pin number
	uint8_t unused_pin[] = {UNUSED_GPIO_24};
	
	//unused gpio pin setting
	for(i = 0; i < sizeof(unused_pin); i++)
	{
		nrf_gpio_cfg(
			unused_pin[i],
			NRF_GPIO_PIN_DIR_INPUT,
			NRF_GPIO_PIN_INPUT_DISCONNECT,
			NRF_GPIO_PIN_NOPULL,
			NRF_GPIO_PIN_D0S1,
			NRF_GPIO_PIN_NOSENSE);
	}

	// gpiote latch mode
	NRF_GPIO->DETECTMODE = (uint32_t)(GPIO_DETECTMODE_LATCH_ON);
	// gpiote initialize.
	err_code = nrfx_gpiote_init();
	LIB_ERR_CHECK(err_code,GPIO_ERROR, __LINE__);
}

/**
 * @brief Softdevice Enable
 * @param None
 * @retval None
 */
void NordicSoftDeviceEnable(void)
{
	uint32_t err_code;
	uint32_t ram_start;
	
	// softdevice enable.
	if ( !nrf_sdh_is_enabled() )
	{
		err_code = nrf_sdh_enable_request();
		LIB_ERR_CHECK(err_code, SDH_SOFTDEV, __LINE__);
		TRACE_LOG(TR_SOFTDEVICE_INIT_CMPL,0);
	}
	
	//Work around chip errata [68]
	nrf_delay_ms(WAIT_HFCLK_STABLE_MS);
	
	//Set the default BLE stack configuration.
	err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
	LIB_ERR_CHECK(err_code, BLE_STACK_DEFA, __LINE__);
	//softdevice ble stack enable.
	err_code = nrf_sdh_ble_enable(&ram_start);
	DEBUG_LOG(LOG_INFO,"ram s 0x%x",ram_start);
	LIB_ERR_CHECK(err_code, BLE_SOFTDEV, __LINE__);
	TRACE_LOG(TR_SOFTDEVICE_BLE_INIT_CMPL,0);	
}

/**
 * @brief One Hour Progress Time
 * @param time tm構造体へのポインタ
 * @retval None
 */
void OneHourProgressTime(struct tm *time)
{
	time_t unixtime;
	
	time->tm_year = time->tm_year + 100;
	time->tm_mon = time->tm_mon - 1;
		
	unixtime = mktime(time);
	DEBUG_LOG(LOG_DEBUG,"change unix time unix time %d",unixtime);
	unixtime = unixtime + 3600;
	DEBUG_LOG(LOG_DEBUG,"+3600 unix time unix time %d",unixtime);
	localtime_r(&unixtime, time);
	
	DEBUG_LOG(LOG_DEBUG,"Read from flash after cal 1 yy %u, mm %u, dd %u, hh %u, m %u, s %u",time->tm_year, time->tm_mon, time->tm_mday, time->tm_hour,time->tm_min,time->tm_sec);
	
	time->tm_year = time->tm_year - 100;
	time->tm_mon = time->tm_mon + 1;

	DEBUG_LOG(LOG_INFO,"One Hour Read from flash after cal yy %u, mm %u, dd %u, hh %u, m %u, s %u",time->tm_year, time->tm_mon, time->tm_mday, time->tm_hour,time->tm_min,time->tm_sec);
}

/**
 * @brief Write RAM One Hour Progress Time
 * @param ram_date_time RAMに書き込む日時データ
 * @retval None
 */
void WriteRamOneHourProgressTime(DATE_TIME *ram_date_time)
{
	struct   tm time;
	
	time.tm_year = ram_date_time->year;
	time.tm_mon  = ram_date_time->month;
	time.tm_mday = ram_date_time->day;
	time.tm_hour = ram_date_time->hour;
	time.tm_min = 0;
	time.tm_sec = 0;
	OneHourProgressTime(&time);
	ram_date_time->year = time.tm_year;
	ram_date_time->month = time.tm_mon;
	ram_date_time->day = time.tm_mday;
	ram_date_time->hour = time.tm_hour;
	ram_date_time->min = time.tm_min;
	ram_date_time->sec = time.tm_sec;
	InitRamDateSet(ram_date_time);
}

/**
 * @brief UICR RegisterからCalibrationデータを読み出す
 * @param None
 * @retval None
 */
void UicrEWFunc(void)
{	
	int16_t i;
	int32_t xoffset = 0;
	int32_t yoffset = 0;
	int32_t zoffset = 0;
	uint32_t uicr_buffer[59] = {UICR_INITIAL_VALUE};
	uint32_t pselreset_0 = UICR_INITIAL_VALUE;
	uint32_t pselreset_1 = UICR_INITIAL_VALUE;
	uint32_t approtect = UICR_INITIAL_VALUE;
	/*UICR Address*/
	uint32_t uicr_address = UICR_ADRESS;
	
	DEBUG_LOG(LOG_INFO,"uicr R/E/W");	
	/*UICR Read enable*/
	NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos);	
	//while(NRF_NVMC->READY == NVMC_READY_READY_Busy){}
	uicr_erase_wait();
	/*Read UICR all register*/
	for(i = 0; i < (sizeof(uicr_buffer)/sizeof(uint32_t)); i++)
	{
		uicr_buffer[i] = *(uint32_t*)uicr_address;
		uicr_erase_wait();
		/* set next uicr register address */
		uicr_address += 0x04;
		DEBUG_LOG(LOG_DEBUG,"Read array num %d",i);
		WdtReload();
	}
	pselreset_0 = NRF_UICR->PSELRESET[0];
	uicr_erase_wait();
	pselreset_1 = NRF_UICR->PSELRESET[1];
	uicr_erase_wait();
	approtect   = NRF_UICR->APPROTECT;
	uicr_erase_wait();

	/*Erase UICR register*/
	/*Enable UICR Erase*/
	NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Een << NVMC_CONFIG_WEN_Pos);
	uicr_erase_wait();
	/*Erase UICR register*/
	NRF_NVMC->ERASEUICR = (NVMC_ERASEUICR_ERASEUICR_Erase << NVMC_ERASEUICR_ERASEUICR_Pos);
	uicr_erase_wait();
		
	/*wdt reload after uicr erase*/
	WdtReload();

	DEBUG_LOG(LOG_DEBUG,"after uicr erase");
	uicr_printf();
		
	/*Write UICR register*/
	/*Enable UICR Write*/
	NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos);
	uicr_erase_wait();
	/*Write UICR regsiter*/
	uicr_address = UICR_ADRESS;	//back uicr start address

	//set offset value for uicr register
	/*test write value*/
	/*get offset value from ram reigon*/
	GetAccOffsetValueFromRam(&xoffset, &yoffset, &zoffset);
	DEBUG_LOG(LOG_DEBUG,"get acc offset x 0x%x, y 0x%x, z 0x%x",(uint16_t)(xoffset >> 16),(uint16_t)(yoffset >> 16),(uint16_t)(zoffset >> 16));
	if(((uint16_t)(xoffset >> 16) == CHECK_DATA) && ((uint16_t)(yoffset >> 16) == CHECK_DATA) && ((uint16_t)(zoffset >> 16) == CHECK_DATA))
	{
		uicr_buffer[UICR_ACC_X_OFFSET_ADDRESS] = xoffset;
		uicr_buffer[UICR_ACC_Y_OFFSET_ADDRESS] = yoffset;
		uicr_buffer[UICR_ACC_Z_OFFSET_ADDRESS] = zoffset;
	}
	/*Write back UICR regsiter*/
	for(i = 0; i < (sizeof(uicr_buffer)/sizeof(uint32_t)); i++)
	{
		if(uicr_buffer[i] != 0xFFFFFFFF)
		{
			*(uint32_t*)uicr_address = uicr_buffer[i];
			uicr_write_wait();
		}
		/*set next uicr register address*/
		uicr_address += 0x04;
	}
	NRF_UICR->PSELRESET[0] = pselreset_0;
	uicr_write_wait();
	NRF_UICR->PSELRESET[1] = pselreset_1;
	uicr_write_wait();
	NRF_UICR->APPROTECT    = approtect;
	uicr_write_wait();

	/*wdt reload after uicr write*/
	WdtReload();

	/*Read enable*/
	NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos);	
	uicr_erase_wait();

	DEBUG_LOG(LOG_DEBUG,"after uicr write");
	uicr_printf();
	DEBUG_LOG(LOG_DEBUG,"\n");
	ClearAccOffsetFlgToRam();
	ClearRamAccOffsetValue();
	/*soft reset for valid uicr register*/
	NVIC_SystemReset();
}

/**
 * @brief ACC Value Offset Setting
 * @param None
 * @retval None
 */
void AccValueOffsetSet(void)
{
	int32_t xoffset;
	int32_t yoffset;
	int32_t zoffset;
	int16_t input_xoffset = 0;
	int16_t input_yoffset = 0;
	int16_t input_zoffset = 0;
	uint8_t valid = 0;
	
	/*Read uicr register enable*/
	NRF_NVMC->CONFIG = (NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos);
	uicr_erase_wait();
	
	/*Read uicr register*/
	xoffset = NRF_UICR->CUSTOMER[0];
	uicr_erase_wait();
	yoffset = NRF_UICR->CUSTOMER[1];
	uicr_erase_wait();
	zoffset = NRF_UICR->CUSTOMER[2];
	uicr_erase_wait();
	
	/*case of sig in uicr regsiter is valid*/
	if(((uint16_t)(xoffset >> 16) == CHECK_DATA) && ((uint16_t)(yoffset >> 16) == CHECK_DATA) && ((uint16_t)(zoffset >> 16) == CHECK_DATA))
	{
		input_xoffset = (int16_t)(xoffset & 0x0000ffff);
		input_yoffset = (int16_t)(yoffset & 0x0000ffff);
		input_zoffset = (int16_t)(zoffset & 0x0000ffff);
		valid = 1;
	}
	else
	{
		input_xoffset = 0;
		input_yoffset = 0;
		input_zoffset = 0;
		valid = 0;
		
		/*if uicr regsiter value break. each offset = 0*/
		DEBUG_LOG(LOG_ERROR,"offset uicr valid err");
		/*trace log*/
		TRACE_LOG(TR_UICR_SENSOR_VALID_ERROR, 0);
		
	}
	/*set global var sensor offset value*/
	SetAccOffset(input_xoffset,input_yoffset,input_zoffset);
	SetAccOffsetValid(valid);
}

/**
 * @brief current [A] measure (debug only)
 * @remark FW version 9.00 debug
 * @param None
 * @retval None
 */
void DebugAccReadEnable(void)
{
	uint8_t accEnableCritical;
	uint32_t err_code;
	
	err_code = sd_nvic_critical_region_enter(&accEnableCritical);
	if(err_code == NRF_SUCCESS)
	{
		g_acc_read_enable = true;
		accEnableCritical = 0;
		err_code = sd_nvic_critical_region_exit(accEnableCritical);
		if(err_code != NRF_SUCCESS)
		{
			//nothing to do.
		}
	}
}

/**
 * @brief current [A] measure (debug only)
 * @remark FW version 9.00 debug
 * @param None
 * @retval None
 */
void DebugAccReadDisable(void)
{
	uint8_t 	accDisableCritical;
	uint32_t 						err_code;
	
	err_code = sd_nvic_critical_region_enter(&accDisableCritical);
	if(err_code == NRF_SUCCESS)
	{
		g_acc_read_enable = false;
		accDisableCritical = 0;
		err_code = sd_nvic_critical_region_exit(accDisableCritical);
		if(err_code != NRF_SUCCESS)
		{
			//nothing to do.
		}
	}
}

/**
 * @brief current [A] measure (debug only)
 * @remark FW version 9.00 debug
 * @param None
 * @retval None
 */
bool DebugGetAccReadStatus(void)
{
	return g_acc_read_enable;
}

/**
 * @brief current [A] measure (debug only)
 * @remark FW version 9.00 debug
 * @param None
 * @retval None
 */
void DebugGPIOPinEnable(void)
{
	nrf_gpio_pin_write(UNUSED_GPIO_24, GPIO_LOW_LEVEL);
	nrf_gpio_cfg_output(UNUSED_GPIO_24);
}

/**
 * @brief current [A] measure (debug only)
 * @remark FW version 9.00 debug
 * @param None
 * @retval None
 */
void DebugGPIOPinOn(void)
{
	nrf_gpio_pin_write(UNUSED_GPIO_24, GPIO_HIGH_LEVEL);
}

/**
 * @brief current [A] measure (debug only)
 * @remark FW version 9.00 debug
 * @param None
 * @retval None
 */
void DebugGPIOPinOff(void)
{
	nrf_gpio_pin_write(UNUSED_GPIO_24, GPIO_LOW_LEVEL);
}

/**
 * @brief Latch Clear Wait
 * @remark Errata No.173
 * @param None
 * @retval None
 */
void WaitLatchClear(void)
{
	for(volatile uint16_t i = 0; i < 3; i++)
	{
		__NOP();
	}
}

/**
 * @brief Library Error Check
 * @param err_code Error Code
 * @param trace_id Trace ID
 * @param line LINE Number
 * @retval None
 */
void LibErrorCheck(uint32_t err_code, uint8_t trace_id, uint16_t line)
{
	if( err_code != NRF_SUCCESS )
	{
		DEBUG_LOG( LOG_ERROR, "Reset err, err_code 0x%x, TraceID 0x%x, LINE %u", err_code, trace_id, line );
		TRACE_LOG( ( TR_OTHER_ERROR_HEADER | trace_id), line );
		sd_nvic_SystemReset();
	}
}

/**
 * @brief Library Error Check
 * @param mode enable or disable
 *        - DCDC_ENABLE : DCDC Enable
 *        - DCDC_DISABLE : DCDC Disable
 * @retval None
 */
void PowerDcdcEnable( uint8_t mode )
{
	uint32_t err_code;
	
	if ( !nrf_sdh_is_enabled() )
	{
		err_code = nrf_sdh_enable_request();
		LIB_ERR_CHECK( err_code, DCDC_POWER, __LINE__ );
	}
    err_code = sd_power_dcdc_mode_set( mode );
	/* 2022.04.06 Add Power Save Mode ++ */
	err_code = sd_power_mode_set( NRF_POWER_MODE_LOWPWR );
	/* 2022.04.06 Add Power Save Mode -- */
}

/**
 * @brief UICR Erase Wait
 * @param None
 * @retval None
 */
static void uicr_erase_wait(void)
{
	int16_t i;
	
	for(i = 0; i < DIVIDE_WAIT_LOOP_NUM; i++)
	{
		if(NRF_NVMC->READY == NVMC_READY_READY_Busy)
		{
			nrf_delay_ms(WAIT_UICR_ERASE_MARGIN_MS/DIVIDE_WAIT_LOOP_NUM);
		}
		else
		{
			break;
		}
	}
	DEBUG_LOG(LOG_DEBUG,"uicr erase loop num %d",i);
	if((i == DIVIDE_WAIT_LOOP_NUM) && (NRF_NVMC->READY == NVMC_READY_READY_Busy))
	{
		DEBUG_LOG(LOG_ERROR,"uicr erase err");
		/*trace log*/
		TRACE_LOG(TR_UICR_WAIT_ERROR,0);
		NVIC_SystemReset();
	}
	
}

/**
 * @brief UICR Write Wait
 * @param None
 * @retval None
 */
static void uicr_write_wait(void)
{
	int16_t i;
	
	for(i = 0; i < DIVIDE_WAIT_LOOP_NUM; i++)
	{
		if(NRF_NVMC->READY == NVMC_READY_READY_Busy)
		{
			nrf_delay_us(WAIT_UICR_WRITE_MARGIN_US/DIVIDE_WAIT_LOOP_NUM);
		}
		else
		{
			break;
		}
	}
	DEBUG_LOG(LOG_DEBUG,"uicr write loop num %d",i);
	if((i == DIVIDE_WAIT_LOOP_NUM) && (NRF_NVMC->READY == NVMC_READY_READY_Busy))
	{
		DEBUG_LOG(LOG_ERROR,"uicr write err");	
		/*trace log*/
		TRACE_LOG(TR_UICR_WAIT_ERROR,1);
		NVIC_SystemReset();
	}
}

/**
 * @brief UICR Register Print
 * @param None
 * @retval None
 */
static void uicr_printf(void)
{
	int16_t i;
	for(i = 0; i < 15; i++)
	{
		DEBUG_LOG(LOG_DEBUG,"NRF_FW[%d] 0x%x",i, NRF_UICR->NRFFW[i]);
	}
	
	for(i = 0; i < 12; i++)
	{
		DEBUG_LOG(LOG_DEBUG,"NRF_HW[%d] 0x%x",i, NRF_UICR->NRFHW[i]);
	}
	
	for(i = 0; i < 32; i++)
	{
		DEBUG_LOG(LOG_DEBUG,"NRF_CUSTOMER[%d] 0x%x",i, NRF_UICR->CUSTOMER[i]);
	}
	
	DEBUG_LOG(LOG_DEBUG,"NRF_P0 0x%x",NRF_UICR->PSELRESET[0]);
	DEBUG_LOG(LOG_DEBUG,"NRF_P1 0x%x",NRF_UICR->PSELRESET[1]);
	DEBUG_LOG(LOG_DEBUG,"NRF_APPROTECT 0x%x\n",NRF_UICR->APPROTECT);
}

/* 2022.06.06 Add WakeUp時間帯対応 ++ */
/**
 * @brief RTC Wake Up Time Validate
 * @param None
 * @retval true ADVを発行
 * @retval false ADVを発行しない
 */
bool ValidateWakeUpTime( void )
{
	DATE_TIME current_date = {0};
	uint32_t ret;
	bool adv_exec = false;
	
	/* 現在時刻取得 */
	ret = ExRtcGetDateTime( &current_date );
	if( ret == NRF_SUCCESS )
	{
		/* 現在時刻から時間帯を検証し、0 - 6時までの間であればADVを発行しない */
		if ( current_date.hour < ADV_EXEC_TIME_RANGE_END )
		{
			/* ADVは実行しない */
			adv_exec = false;
		}
		else
		{
			/* ADVを実行する */
			adv_exec = true;
		}
	}
	
	return adv_exec;
}
/* 2022.06.06 Add WakeUp時間帯対応 -- */

