/**
  ******************************************************************************************
  * @file    lateral_wakeup.h
  * @author  akiteru
  * @version 1.0
  * @date    2022/03/17
  * @brief   Wake Up Algorithm
  ******************************************************************************************
    Revision:
    Ver            Date             Revised by        Explanation
  ******************************************************************************************
  * 1.0            2022/03/17       akiteru         create new
  ******************************************************************************************
*/

#ifndef LATERAL_WAKEUP_H_
#define LATERAL_WAKEUP_H_

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#define WAKEUP_TH_ACC		500		/*横方向加速度検知閾値 [mg]*/
#define WAKEUP_TH_COUNT		15		/*横方向加速度検知カウント閾値. Accデータ10ms間隔。30ms Duration相当*/
#define WAKEUP_DIFF_TIME	100

#define WAKEUP_TOTTAL_ACC_COUNT 2

typedef enum
{
	LATERARL_ACC_DETECT		= 0,	/* 横方向加速度検知   */
	LATERARL_ACC_NO_DETECT			/* 横方向加速度未検知 */
}RESULT_LATERAL_ACC_STATUS;

/* *
 * @breif 靴横方向加速度検知判定
 * @param sid sequence id
 * @param lat_acc 横方向加速度
 * @retval LATERARL_ACC_DETEC 検知有
 * @retval LATERARL_ACC_NO_DETE 検知無
 * */
RESULT_LATERAL_ACC_STATUS LateralAxisWakeup( uint16_t sid, int16_t lat_acc );

/**
 * @brief 靴横方向加速度検知判定ステータスクリア.ADV状態の時Call
 * @param None
 * @retval None
 */
void ClearAdvStatus(void);

#endif /* LATERAL_WAKEUP_H_ */
