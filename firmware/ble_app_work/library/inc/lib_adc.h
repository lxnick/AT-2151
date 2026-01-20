/**
  ******************************************************************************************
  * @file    lib_adc.h
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

#ifndef LIB_ADC_H_
#define LIB_ADC_H_

/* Includes --------------------------------------------------------------*/
#include <stdint.h>
#include "nrf_drv_saadc.h"
#include "nrfx_saadc.h"

#include "lib_common.h"
//#include "state_control.h"

#ifdef __cplusplus
extern "C"{
#endif

/* Definition ------------------------------------------------------------*/
/* 2022.03.30 Add 電圧値判定処理結果及び閾値 ++ */
#define BATVOLT_OK			0
#define BATVOLT_NG			1
#define THRESHOLD_BAT_VOLT	2000
/* 2022.03.30 Add 電圧値判定処理結果及び閾値 -- */

/* Enum ------------------------------------------------------------------*/

/* Struct ----------------------------------------------------------------*/

/* Function prototypes ----------------------------------------------------*/
/**
 * @brief SAADC Sampling Start
 * @param None
 * @retval None
 */
void StartADC( void );

/**
 * @brief SAADC Initialize
 * @param None
 * @retval None
 */
void AnalogInitialize( void );

/**
 * @brief SAADC Uninitialize
 * @param None
 * @retval None
 */
void AnalogUninit( void );

/**
 * @brief 起動時の電圧チェック
 * @param None
 * @retval None
 */
ret_code_t AnalogStartUpCheck( void );

float AnalogVoltageOneshot(void);

#ifdef __cplusplus
}
#endif
#endif

