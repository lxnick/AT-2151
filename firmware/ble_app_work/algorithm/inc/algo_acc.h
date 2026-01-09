/**
  ******************************************************************************************
  * @file    algo_acc.h
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

#ifndef ALGO_ACC_H_
#define ALGO_ACC_H_

/* Includes --------------------------------------------------------------*/
#include <stdint.h>
#include "state_control.h"
#include "lib_common.h"

#ifdef __cplusplus
extern "C"{
#endif

/* Function prototypes ---------------------------------------------------*/
/**
 * @brief Enter Sleep
 * @remark イベントが存在しない場合、Sleepに入り割り込み等を待ち受ける
 * @param None
 * @retval None
 */
//void enterSleep(void);
void EnterSleep(void);

/**
 * @brief Enter Deep Sleep
 * @remark 動作していない場合、スタンバイへ入り消費電力を低くする
 * @param pEvent Event Information
 * @retval 0 Success
 */
//uint32_t enterDeepSleep(PEVT_ST pEvent);
uint32_t EnterDeepSleep(PEVT_ST pEvent);

#ifdef __cplusplus
}
#endif

#endif

