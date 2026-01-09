/**
  ******************************************************************************************
  * @file    daily_log.h
  * @author  k.tashiro
  * @version 1.0
  * @date    2020/09/25
  * @brief   Daily Log Process
  ******************************************************************************************
    Revision:
    Ver            Date             Revised by        Explanation
  ******************************************************************************************
  * 1.0            2020/09/25       k.tashiro         create new
  ******************************************************************************************
*/

#ifndef DAILY_LOG_H_
#define DAILY_LOG_H_

/* Includes --------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
#include "state_control.h"
#include "lib_ex_rtc.h"
#include "definition.h"

/* Struct ----------------------------------------------------------------*/
typedef struct {
  uint16_t Start_Sid;
  uint16_t End_Sid;
  uint16_t Send_Sid;
	uint8_t  Send_Flg;
	uint8_t  Send_SingleFlg;
} Flash_Sid_t;

typedef struct {
	uint8_t Date[8];
	int16_t walk;
	int16_t run;
	int16_t dash;
	uint16_t sid;
} Daily_t;

typedef struct _daily_write_info
{
	uint32_t start_addr;
	uint16_t sid;
} DAILY_WRITE_INFO;

/* Function prototypes ----------------------------------------------------*/
/**
 * @brief Get SID Over Count
 * @param None
 * @retval mSid_over_count sid over count
 */
//uint8_t getSidOverCount(void);
uint8_t GetSidOverCount(void);

/**
 * @brief Get SID From Retention RAM
 * @param None
 * @retval None
 */
void GetSidFromRamRegion(void);

/**
 * @brief VolFlash Struct Clear
 * @param None
 * @retval None
 */
//void mVolFlashClear(void);
void VolFlashClear(void);

/**
 * @brief Set Flash SID
 * @param None
 * @retval None
 */
//void Set_FlashSid(void);
void SetFlashSid(void);

/**
 * @brief Set Daily Log Send Flag
 * @param None
 * @retval None
 */
//void Set_DaliyLog_SendFlg(void);
void SetDaliyLogSendFlg( void );

/**
 * @brief Set Daily Log Single Send Flag
 * @param None
 * @retval None
 */
//void Set_DaliyLog_SingleSendFlg(void);
void SetDaliyLogSingleSendFlg( void );

/**
 * @brief Set Daily Log Start SID
 * @param start_id Start ID
 * @retval None
 */
//void Set_StartSid(uint16_t start_id);
void SetStartSid(uint16_t start_id);

/**
 * @brief Set Daily Log End SID
 * @param end_id End SID
 * @retval None
 */
//void Set_EndSid(uint16_t end_id);
void SetEndSid(uint16_t end_id);

/**
 * @brief DailyLog Continue Configuration.
 * @param pEvent Event Information
 * @retval 0 Success
 */
//uint32_t Send_DailyLog(PEVT_ST pEvent);
uint32_t SendDailyLog(PEVT_ST pEvent);

/**
 * @brief DailyLog Single Configuration.
 * @param pEvent Event Information
 * @retval 0 Success
 */
//uint32_t Send_SingleDailyLog(PEVT_ST pEvent);
uint32_t SendSingleDailyLog(PEVT_ST pEvent);

/**
 * @brief Set new SID
 * @param None
 * @retval None
 */
//void Set_WriteSid(void);
void SetWriteSid( void );

/**
 * @brief Set RTC From Flash Date Time
 * @param None
 * @retval ret 0 Flashからの書き込み成功
 * @retval ret 1 Flashに時刻なし
 */
//void set_rtc_from_flashDateTime(void);
uint32_t SetRtcFromFlashDateTime(void);

/* 2022.07.25 Add RTC異常時対応 ++ */
/**
 * @brief Get Time From Flash Date
 * @param None
 * @retval ret 0 Flashからの書き込み成功
 * @retval ret 1 Flashに時刻なし
 */
uint32_t GetTimeFromFlashDateTime( DATE_TIME *dateTime );
/* 2022.07.25 Add RTC異常時対応 -- */

#endif
