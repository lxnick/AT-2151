/**
  ******************************************************************************************
  * @file    lib_ram_retain.h
  * @author  k.tashiro
  * @version 1.0
  * @date    2020/09/14
  * @brief   RAM Retain
  ******************************************************************************************
    Revision:
    Ver            Date             Revised by        Explanation
  ******************************************************************************************
  * 1.0            2020/09/14       k.tashiro         create new
  ******************************************************************************************
*/

#ifndef LIB_RAM_RETAIN_H_
#define LIB_RAM_RETAIN_H_

/* Includes --------------------------------------------------------------*/
#include "nrf_fstorage.h"
#include "ble_gatts.h"

#include "lib_common.h"
#include "state_control.h"
#include "ble_manager.h"
#include "mode_manager.h"
#include "lib_fifo.h"
#include "lib_timer.h"
#include "lib_ex_rtc.h"
#include "algo_acc.h"
#include "Walk_Algo.h"
#include "daily_log.h"
#include "definition.h"

#ifdef __cplusplus
extern "C"{
#endif

/* Struct ----------------------------------------------------------------*/
typedef struct _ram_save_region
{
	uint8_t		signiture[13];		//13byte -> 36byte + 12byte = 48byte start ram address 0x20007E00
	uint8_t		padd1[3];			//3byte
	uint16_t	walk_count;			//2byte
	uint16_t	run_count;			//2byte
	uint16_t	dash_count;			//2byte
	uint16_t	sid;				//2byte
	uint8_t		year;				//1byte
	uint8_t		month;				//1byte
	uint8_t		day;				//1byte
	uint8_t		hour;				//1byte
	uint8_t		min;				//1byte
	uint8_t		sec;				//1byte
	uint8_t		age;				//1byte
	uint8_t		weight;				//1byte
	uint8_t		sex;				//1byte
	uint8_t		padd2;				//1byte
	uint16_t	offsetFlg;      	//2byte
	int32_t		offset[3];			//12byte ...add sensor offset value
} RAM_SAVE_REGION, *PRAM_SAVE_REGION;

/* Function prototypes ----------------------------------------------------*/
/**
 * @brief RAM Saveの領域を確保する
 * @param None
 * @retval None
 */
//void makeRamSaveRegion(void);
void MakeRamSaveRegion(void);

/**
 * @brief RAMに書き込まれているCurrent TimeをCheckする
 * @param pCurrDateTime 日時データへのポインタ
 * @retval true 更新あり
 * @retval false 更新なし
 */
bool CheckCurrTime(PDATE_TIME pCurrDateTime);

/**
 * @brief debug log ram save region
 * @param None
 * @retval None
 */
void DebugDisplayRamSave(void);

/**
 * @brief Get RAM Save Walk Information
 * @param pDailyData Daily Data
 * @retval None
 */
void GetRamSaveWalkInfo(Daily_t *pDailyData);

/**
 * @brief Initialize RAM Date Set
 * @param pData date data
 * @retval None
 */
void InitRamDateSet(DATE_TIME *pData);

/**
 * @brief RAM Sid Set
 * @param sid SID
 * @retval None
 */
void RamSidSet(uint16_t sid);

/**
 * @brief RAM Sid Clear
 * @param Noen
 * @retval None
 */
void RamSidClear(void);

/**
 * @brief RAM Signatrue Check
 * @param Noen
 * @retval true 正常
 * @retval false 異常
 */
bool RamSignCheck(void);

/**
 * @brief RAM DataのDefault値をセットアップする
 * @param Noen
 * @retval None
 */
void CreateRamSaveDefault(void);

/**
 * @brief RAMに保持されているSIDを読み出す
 * @param Noen
 * @retval sid 保持されているSID
 */
uint16_t RamSidGet(void);

/**
 * @brief RAMのwalk informationの領域をクリアする
 * @param Noen
 * @retval None
 */
void RamSaveRegionWalkInfoClear(void);

/**
 * @brief RAMに保持されている日時データを取得する
 * @param pSaveDateTime 取得する日時データ
 * @retval None
 */
void GetRamTime(DATE_TIME *pSaveDateTime);

/**
 * @brief RAMへプレイヤー情報を設定する
 * @param age 年齢
 * @param sex 性別
 * @param weight 体重
 * @retval None
 */
void SetRamPlayerInfo(uint8_t age, uint8_t sex, uint8_t weight);

/**
 * @brief RAMに保持されているプレイヤー情報(年齢)をalgorithmへ設定する
 * @param None
 * @retval None
 */
void SetAlgoPlayerInfo(void);

/**
 * @brief RAMに保持されているデイリー情報を取得する
 * @param pDaily_data デイリー情報を格納する変数へのポインタ
 * @retval None
 */
void GetRamDailyLog(Daily_t *pDaily_data);

/**
 * @brief RAMに保持されているSIDをインクリメントする
 * @param None
 * @retval None
 */
void RamSaveSidIncrement(void);

/**
 * @brief RAMに保持されている日時情報を更新する
 * @param pDateTime 更新する日時情報へのポインタ
 * @retval None
 */
void RamSaveTimeUpdate(uint8_t *pDateTime);

/**
 * @brief RAMに保持されている日時をRTCへ設定する
 * @param None
 * @retval None
 */
void SetRtcFromRamDateTime(void);


/**
 * @brief デイリー情報をRAMへ設定する
 * @param pDaily 設定するデイリー情報へのポインタ
 * @retval None
 */
void WalkRamSet(DAILYCOUNT *pDaily);

/**
 * @brief デイリー情報をRAMへ設定する(Debug用)
 * @param None
 * @retval None
 */
void DebugWalkRamSet(void);

/**
 * @brief set acc offset value for power retain ram function
 * @param None
 * @retval None
 */
void RamSaveOffset(uint16_t check, int32_t x_offset, int32_t y_offset, int32_t z_offset);

/**
 * @brief check acc offsetFlg and Reset
 * @param None
 * @retval None
 */
void AccOffsetCheckSystemReset(void);

/**
 * @brief get acc offset value and offset flag
 * @param x_offset X軸のOffset
 * @param y_offset Y軸のOffset
 * @param z_offset Z軸のOffset
 * @retval None
 */
void GetAccOffsetValueFromRam(int32_t *x_offset, int32_t *y_offset, int32_t *z_offset);

/**
 * @brief get acc offset flg from power retain ram
 * @param flg offset flag
 * @retval None
 */
//void get_acc_offset_flg(uint16_t *flg);
void GetAccOffsetFlgFromRam(uint16_t *flg);

/**
 * @brief clear acc offset flg in power retain ram
 * @param None
 * @retval None
 */
void ClearAccOffsetFlgToRam(void);

/**
 * @brief init acc offset value in power retain ram 
 * @param None
 * @retval None
 */
void ClearRamAccOffsetValue(void);

#ifdef __cplusplus
}
#endif

#endif

