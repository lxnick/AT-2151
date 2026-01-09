/**
  ******************************************************************************************
  * @file    lib_combsort.h
  * @author  k.tashiro
  * @version 1.0
  * @date    2020/09/09
  * @brief   COMB Sort
  ******************************************************************************************
    Revision:
    Ver            Date             Revised by        Explanation
  ******************************************************************************************
  * 1.0            2020/09/09       k.tashiro         create new
  ******************************************************************************************
*/

#ifndef LIB_COMBSORT_H_
#define LIB_COMBSORT_H_

/* Includes --------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_delay.h"

/* Definition ------------------------------------------------------------*/
#define COMB_SORT_ELEVEN true

/**
 * @brief COMB Sortを実行する
 * @param values Sortする配列
 * @param length Sortする配列の長さ
 * @param comp_sort_eleven コムソート11を使用する際にtrueにする
 * @retval None
 */
void CombSort( float* values, int length, bool comp_sort_eleven );

#endif 

