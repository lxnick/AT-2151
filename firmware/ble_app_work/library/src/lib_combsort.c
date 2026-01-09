/**
  ******************************************************************************************
  * @file    lib_combsort.c
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

/* Includes --------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include "lib_combsort.h"

/* Definition ------------------------------------------------------------*/
//#define SORT_ASCENDING_ODER

/* Private function prototypes -------------------------------------------*/
/**
 * @brief value1とvalue2をswapする
 * @param value1 swapする値1
 * @param value2 swapする値2
 * @retval None
 */
static void comb_swap(float* value1,float* value2);

/**
 * @brief COMB Sortを実行する
 * @param values Sortする配列
 * @param length Sortする配列の長さ
 * @param comp_sort_eleven コムソート11を使用する際にtrueにする
 * @retval None
 */
void CombSort( float* values, int length, bool comp_sort_eleven )
{
	int gap = length;
	int swap_count = 0;
	
	while((gap + swap_count) > 1)
	{
		if(gap > 1)
		{
			gap = gap * 10 / 13;
			// CombSort11
			if(comp_sort_eleven == true)
			{	
				if((gap == 9)||(gap==10))
				{
					gap=11;
				}
			}
		}
		swap_count = 0;
		for(int i = (length - 1); i >= gap; i--)
		{
#ifdef SORT_ASCENDING_ODER
			// ascending order
			if( values[i - gap] > values[i] )
			{
#else
			// descending order
			if( values[i - gap] < values[i] )
			{
#endif
				comb_swap(&values[i - gap],&values[i]);
				swap_count++;
			}
		}
	}
}

/**
 * @brief value1とvalue2をswapする
 * @param value1 swapする値1
 * @param value2 swapする値2
 * @retval None
 */
static void comb_swap(float* value1,float* value2)
{
	float temp = *value1;
	*value1=*value2;
	*value2=temp;
}

