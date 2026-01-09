/**
  ******************************************************************************************
  * @file    algo_acc.c
  * @author  k.tashiro
  * @version 1.0
  * @date    2020/09/25
  * @brief   Sleep Control Process
  ******************************************************************************************
    Revision:
    Ver            Date             Revised by        Explanation
  ******************************************************************************************
  * 1.0            2020/09/25       k.tashiro         create new
  ******************************************************************************************
*/

/* Includes --------------------------------------------------------------*/
#include "nordic_common.h"
#include "nrf_gpio.h"
#include "nrfx_gpiote.h"
#include "lib_common.h"
#include "state_control.h"
#include "mode_manager.h"
#include "lib_ram_retain.h"
#include "lib_icm42607.h"
#include "lib_fifo.h"
#include "lib_ex_rtc.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_power.h"
#include "algo_acc.h"
#include "lib_wdt.h"
#include "lib_trace_log.h"

/* Definition ------------------------------------------------------------*/
#define WALKUP_ACC_VALUE    (int16_t)3000

/* 2022.04.07 Add FPU Disable ++ */
#define FPU_EXCEPTION_MASK	0x0000009F
/* 2022.04.07 Add FPU Disable -- */

/**
 * @brief Enter Sleep
 * @remark イベントが存在しない場合、Sleepに入り割り込み等を待ち受ける
 * @param None
 * @retval None
 */
void EnterSleep(void)
{
	ret_code_t err_code;

	/* 2022.04.07 Add FPU Disable ++ */
	__set_FPSCR(__get_FPSCR() & ~(FPU_EXCEPTION_MASK));
	(void) __get_FPSCR();
	NVIC_ClearPendingIRQ(FPU_IRQn);
	/* 2022.04.07 Add FPU Disable -- */
	
	PreSleepUart();
	err_code = sd_app_evt_wait();
	WakeUpUart();
	LIB_ERR_CHECK(err_code, SLEEP_ERROR, __LINE__);
}

/**
 * @brief Enter Deep Sleep
 * @remark 動作していない場合、スタンバイへ入り消費電力を低くする
 * @param pEvent Event Information
 * @retval 0 Success
 */
uint32_t EnterDeepSleep(PEVT_ST pEvent)
{
	DEBUG_LOG(LOG_DEBUG, "start enter sleep");
	UNUSED_PARAMETER(*pEvent);
	
	volatile ret_code_t err_code;
	uint8_t i;
	uint32_t ex_device_err;
	uint32_t acc_wake_up_err;

	/*Setting RTC interrupt pin*/
	/* 2022.05.16 Delete 自動接続対応 ++ */
	//nrfx_gpiote_in_event_disable(RTC_INT_PIN);
	//ex_device_err = ExRtcAlarmDisableClear(UTC_SUCCESS);
	/* 2022.05.16 Delete 自動接続対応 -- */
	nrfx_gpiote_in_event_disable(ACC_INT1_PIN);
	nrfx_gpiote_in_event_disable(ACC_INT2_PIN);
	GpioteLatchClear();
	/* 2022.05.16 Add 自動接続対応 ++ */
	ex_device_err  = ExRtcWakeUpSetting();
	/* 2022.05.16 Add 自動接続対応 -- */

	/* 2020.10.27 Add Setting Acc Low power mode and Wake Up Sensor */
	acc_wake_up_err = AccGyroWakeUpSetting( ex_device_err );
	if(acc_wake_up_err != UTC_SUCCESS)
	{
		//trace log
		TRACE_LOG(TR_DEEP_SLEEP_ENTER,1);
		DEBUG_LOG( LOG_ERROR, "!!! AccGyroWakeUpSetting Error !!!" );
		sd_nvic_SystemReset();
	}

	DEBUG_LOG(LOG_DEBUG, "0");
	DebugDisplayRamSave();
	
	//trace log
	TRACE_LOG(TR_DEEP_SLEEP_ENTER,0);
	
	/* 2022.04.07 Add FPU Disable ++ */
	__set_FPSCR(__get_FPSCR() & ~(FPU_EXCEPTION_MASK));
	(void) __get_FPSCR();
	NVIC_ClearPendingIRQ(FPU_IRQn);
	/* 2022.04.07 Add FPU Disable -- */
	
	PreSleepUart();
	WdtReload();
	err_code = sd_power_system_off();
	GpioteLatchClear();
	for(i = 0; i < SYSTEM_OFF_ENTERY_RETRY_LIMIT; i++)
	{
		err_code = sd_power_system_off();
	}
	WakeUpUart();
	LIB_ERR_CHECK(err_code,SYSTEM_OFF_ERROR,__LINE__);
	return 0;
}


