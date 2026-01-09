/**
  ******************************************************************************************
  * @file    lateral_wakeup.c
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

/* Includes --------------------------------------------------------------*/
#include "lateral_wakeup.h"

/* Private variables -----------------------------------------------------*/
volatile uint16_t g_th_over_count = 0;
volatile bool g_adv_status = false;
volatile uint16_t g_total_count = 0;
//volatile uint16_t sid_buf[2] ={0};

typedef struct _sid_buf_
{
	uint16_t sid_buf[2];
	uint16_t first_flg;
} SID_BUF;

volatile SID_BUF g_sid_buf = {{0,0},0};

/* *
 * @breif 靴横方向加速度検知判定
 * @param sid sequence id
 * @param lat_acc 横方向加速度
 * @retval LATERARL_ACC_DETEC 検知有
 * @retval LATERARL_ACC_NO_DETE 検知無
 * */
RESULT_LATERAL_ACC_STATUS LateralAxisWakeup( uint16_t sid, int16_t lat_acc )
{
	RESULT_LATERAL_ACC_STATUS ret = LATERARL_ACC_NO_DETECT;
	uint16_t temp_count = 0;
	uint16_t diff;

    //if(WAKEUP_TH_ACC <= lat_acc)
	if( lat_acc <= (-1*WAKEUP_TH_ACC) )
	{
		g_th_over_count++;
	}
	else
	{
		temp_count = g_th_over_count;
		g_th_over_count = 0;
	}

	if( (WAKEUP_TH_COUNT <= temp_count) && (g_adv_status == false))
	{
		if(g_total_count <= WAKEUP_TOTTAL_ACC_COUNT)
		{
			g_sid_buf.sid_buf[g_total_count] = sid;
			g_sid_buf.first_flg = 1;
		}
		g_total_count++;

		if(WAKEUP_TOTTAL_ACC_COUNT <= g_total_count)
		{
			diff = (g_sid_buf.sid_buf[1] - g_sid_buf.sid_buf[0]) % 65536;
			if(diff < WAKEUP_DIFF_TIME)
			{
			  ret = LATERARL_ACC_DETECT;
					g_adv_status = true; 
			}
			g_total_count = 0;
			g_sid_buf.sid_buf[0] = 0;
			g_sid_buf.sid_buf[1] = 0;
			g_sid_buf.first_flg  = 0;
		}
	}
	else
	{
		if(g_sid_buf.first_flg == 1)
		{
			diff = (sid - g_sid_buf.sid_buf[0]) % 65536;
			if(WAKEUP_DIFF_TIME <= diff)
			{
	    		g_total_count = 0;
	    		g_sid_buf.sid_buf[0] = 0;
	    		g_sid_buf.sid_buf[1] = 0;
	    		g_sid_buf.first_flg  = 0;
			}
		}
	}

	return ret;
}

/**
 * @brief 靴横方向加速度検知判定ステータスクリア.ADV状態の時Call
 * @param None
 * @retval None
 */
void ClearAdvStatus(void)
{
	g_adv_status = false;
	g_total_count = 0;
	g_sid_buf.sid_buf[0] = 0;
	g_sid_buf.sid_buf[1] = 0;
	g_sid_buf.first_flg  = 0;
	g_th_over_count = 0;
}


