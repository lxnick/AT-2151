/**
  ******************************************************************************************
  * @file    lib_wdt.h
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

#ifndef LIB_WDT_H_
#define LIB_WDT_H_

/* Includes --------------------------------------------------------------*/
#include <stdint.h>
#include "lib_common.h"
#include "nrf_drv_wdt.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "definition.h"
#include "lib_trace_log.h"

#ifdef __cplusplus
extern "C"{
#endif

/* Function prototypes ----------------------------------------------------*/
/**
 * @brief Watch Dog Timer Initialize and Start
 * @param None
 * @retval None
 */
void WdtStart(void);

/**
 * @brief Watch Dog Timer Reload
 * @param None
 * @retval None
 */
void WdtReload(void);

#ifdef __cplusplus
}
#endif
#endif
