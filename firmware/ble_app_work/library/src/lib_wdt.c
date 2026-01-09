/**
  ******************************************************************************************
  * @file    lib_wdt.c
  * @author  k.tashiro
  * @version 1.0
  * @date    2020/09/14
  * @brief   Watch Dog Timer Controle
  ******************************************************************************************
    Revision:
    Ver            Date             Revised by        Explanation
  ******************************************************************************************
  * 1.0            2020/09/14       k.tashiro         create new
  ******************************************************************************************
*/

/* Includes --------------------------------------------------------------*/
#include "lib_wdt.h"

/* Private variables -----------------------------------------------------*/
//wdt channel id
static nrf_drv_wdt_channel_id gWdt_channel_id;


/**
 * @brief Watch Dog Timer Interrupt Handler
 * @param None
 * @retval None
 */
static void wdt_int_evt_handler(void)
{
	__NOP();
}

/**
 * @brief Watch Dog Timer Initialize
 * @param None
 * @retval None
 */
static void wdt_initialization(void)
{
	ret_code_t err_code;

	//wdt set Reload value.
	nrfx_wdt_config_t wdtConfig = NRF_DRV_WDT_DEAFULT_CONFIG;
	wdtConfig.reload_value = WDT_TO_TIME;
	
	//wdt initialize
	err_code = nrfx_wdt_init(&wdtConfig, wdt_int_evt_handler);
	LIB_ERR_CHECK(err_code, WDT_INIT, __LINE__);

	err_code = nrf_drv_wdt_channel_alloc(&gWdt_channel_id);
	LIB_ERR_CHECK(err_code, WDT_CHANNEL, __LINE__);

}

/**
 * @brief Watch Dog Timer Initialize and Start
 * @param None
 * @retval None
 */
void WdtStart(void)
{
	wdt_initialization();
	nrf_drv_wdt_enable();
	TRACE_LOG(TR_WDT_START,0);
}

/**
 * @brief Watch Dog Timer Reload
 * @param None
 * @retval None
 */
void WdtReload(void)
{
	nrf_drv_wdt_channel_feed(gWdt_channel_id);	
}


