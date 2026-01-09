/**
  ******************************************************************************************
  * @file    lib_flash.h
  * @author  k.tashiro
  * @version 1.0
  * @date    2020/09/14
  * @brief   Flash Peripheral Driver
  ******************************************************************************************
    Revision:
    Ver            Date             Revised by        Explanation
  ******************************************************************************************
  * 1.0            2020/09/15       k.tashiro         create new
  ******************************************************************************************
*/

#ifndef LIB_FLASH_H_
#define LIB_FLASH_H_

/* Includes --------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "lib_common.h"
#include "state_control.h"
#include "daily_log.h"
#include "definition.h"

/* Definition ------------------------------------------------------------*/
#define ERR_CHECK_FLASH ErrCheckFlash

/* Struct ----------------------------------------------------------------*/
typedef struct _trace_entry{
	uint32_t	uiTID;
	uint32_t	uiDate;			// The format is Unix time.
} TRACE_ENTRY, *PTRACE_ENTRY;

typedef struct _tracedata{
	uint32_t	uiTraceIndex;
	TRACE_ENTRY	info_entity[TRACE_MAX_DATA];
} TRACEDATA, *PTRACEDATA;

/* Function prototypes ----------------------------------------------------*/
/**
 * @brief flash daily log write
 * @param sid SID
 * @param data Daily Log Data
 * @param set_case 書き込むページアドレス指定
 * @param over_count Over Count
 * @retval res
 *			- 1 : Success
 *			- 0 : Failed
 */
//int8_t flash_write(int32_t sid, Daily_t * data, int8_t set_case, uint8_t over_count);
int8_t FlashWrite(int32_t sid, Daily_t * data, int8_t set_case, uint8_t over_count);

/**
 * @brief flash daily log read
 * @param sid SID
 * @param data Daily Log Data
 * @param set_case 書き込むページアドレス指定
 * @retval None
 */
//void flash_read(int32_t sid, Daily_t * data, int8_t set_case);
void FlashRead(int32_t sid, Daily_t * data, int8_t set_case);

/**
 * @brief flash daily log Clear
 * @param page クリアするページ番号
 * @retval None
 */
//void flash_dailyLog_page_clear(int8_t page);
void FlashDailyLogPageClear(int8_t page);

/**
 * @brief flash daily log data check
 * @param flag クリアするページ番号
 * @retval res
 *			- 1 : Success
 *			- 0 : Failed
 */
//int8_t check_flash(int8_t flg);
int8_t CheckFlash(int8_t flg);

/**
 * @brief Pairing Flash Check
 * @param data check pairing data
 * @retval res
 *			- PIN_MATCH : Match
 *			- PIN_NOT_MATCH : Not Match
 *			- PIN_NOT_EXIST : Not Exist
 */
//int8_t check_pairing(uint8_t *data);
int8_t CheckPairing(uint8_t *data);

/**
 * @brief Get Latest Sid from Flash 
 * @param write_data write data
 * @param over_data over data
 * @retval ret 最新のSID
 */
//uint16_t Get_last_Sid(int8_t *write_data, uint8_t *over_data);
uint16_t GetLastSid(int8_t *write_data, uint8_t *over_data);

/**
 * @brief Check and Get Player data
 * @param age Flashから読み出した年齢
 * @param player Flashから読み出した性別
 * @param weight Flashから読み出した体重
 * @retval ret 最新のSID
 */
//bool check_player(uint8_t *age, uint8_t *player, uint8_t *weight);
bool CheckPlayer(uint8_t *age, uint8_t *player, uint8_t *weight);

/**
 * @brief Get Latest Date from Flash 
 * @param date Timer情報を格納する変数へのポインタ
 * @param get_sid 読み出すSID
 * @retval 0 failed
 * @retval 1 success
 */
//uint8_t Get_LastDate(struct tm *date,uint16_t get_sid);
uint8_t GetLastDate(struct tm *date, uint16_t get_sid);

/**
 * @brief Error Check Flash
 * @param rc error code
 * @param traceID Trace ID
 * @param line Execution LINE
 * @retval None
 */
//void err_check_flash(ret_code_t rc, uint16_t traceID, uint16_t line);
void ErrCheckFlash(ret_code_t rc, uint16_t traceID, uint16_t line);

#endif

