/**
  ******************************************************************************************
  * @file    lib_bat.h
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

#ifndef LIB_BAT_H_
#define LIB_BAT_H_

/* Includes --------------------------------------------------------------*/
#include <stdint.h>
#include "nrf_drv_comp.h"

#include "lib_common.h"
#include "state_control.h"

#ifdef __cplusplus
extern "C"{
#endif

/* Definition ------------------------------------------------------------*/
/* ADC値を10回取得し平均値を算出する */
#define MAX_GET_BAT_VALUE		10

/* Enum ------------------------------------------------------------------*/

/* Struct ----------------------------------------------------------------*/

/* Function prototypes ----------------------------------------------------*/
/**
 * @brief Get Battery Info from Comparator
 * @param None
 * @retval None
 */
uint32_t GetBatteryInfo(void);

/**
 * @brief Get Battery Info from Comparator
 * @param None
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 Success
 */
uint32_t ReadAuthorizeBatteryInfo( PEVT_ST pEvent );

/**
 * @brief ADCの処理がタイムアウトした際のバッテリー情報読み出し処理
 * @param None
 * @retval None
 */
void ReadAuthorizeBatteryInfoTimeout( void );

#ifdef __cplusplus
}
#endif
#endif

