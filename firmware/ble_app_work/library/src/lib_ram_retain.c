/**
  ******************************************************************************************
  * @file    lib_ram_retain.c
  * @author  k.tashiro
  * @version 1.0
  * @date    2020/09/14
  * @brief   Retain RAM operation function
  ******************************************************************************************
    Revision:
    Ver            Date             Revised by        Explanation
  ******************************************************************************************
  * 1.0            2020/09/14       k.tashiro         create new
  ******************************************************************************************
*/

/* Includes --------------------------------------------------------------*/
#include "lib_ram_retain.h"
#include "definition.h"

/* Private variables -----------------------------------------------------*/
extern Flash_Sid_t mVolFlash_t;
static RAM_SAVE_REGION *gpRamSave;

/**
 * @brief RAM Saveの領域を確保する
 * @param None
 * @retval None
 */
void MakeRamSaveRegion(void)
{
	gpRamSave = (RAM_SAVE_REGION*)DAILY_DATA_RAMSAVE_ADDRESS;
	DEBUG_LOG(LOG_INFO,"Wake ram save YY %u, MM %u, DD %u, HH %u, mm %u, ss %u",gpRamSave->year, gpRamSave->month, gpRamSave->day, gpRamSave->hour,gpRamSave->min, gpRamSave->sec);
	DEBUG_LOG(LOG_INFO,"Wake ram save age %u, sex %u, weight %u",gpRamSave->age, gpRamSave->sex, gpRamSave->weight);
	DEBUG_LOG(LOG_INFO,"Wake ram save walk %u, run %u, dash %u, SID 0x%x", gpRamSave->walk_count, gpRamSave->run_count, gpRamSave->dash_count, gpRamSave->sid);
	DEBUG_LOG(LOG_INFO,"Wake ram save offset sig 0x%x, x 0x%x, y 0x%x, z 0x%x",gpRamSave->offsetFlg,gpRamSave->offset[0],gpRamSave->offset[1],gpRamSave->offset[2]);
}

/**
 * @brief RAMに書き込まれているCurrent TimeをCheckする
 * @param pCurrDateTime 日時データへのポインタ
 * @retval true 更新あり
 * @retval false 更新なし
 */
bool CheckCurrTime(PDATE_TIME pCurrDateTime)
{
	bool ret;
	ret = true;
	
	DEBUG_LOG(LOG_DEBUG,"Check rtc time yy %u, mm %u, dd %u, hh %u", pCurrDateTime->year,pCurrDateTime->month,pCurrDateTime->day,pCurrDateTime->hour);
	DEBUG_LOG(LOG_DEBUG,"Check ram time yy %u, mm %u, dd %u, hh %u", gpRamSave->year,gpRamSave->month,gpRamSave->day,gpRamSave->hour);
	
	if(gpRamSave->year != pCurrDateTime->year)
	{
		return ret;
	}
	if(gpRamSave->month != pCurrDateTime->month)
	{
		return ret;
	}
	if(gpRamSave->day != pCurrDateTime->day)
	{
		return ret;
	}
	if(gpRamSave->hour != pCurrDateTime->hour)
	{
		return ret;
	}
	ret = false;
	return ret;
}


/**
 * @brief debug log ram save region
 * @param None
 * @retval None
 */
void DebugDisplayRamSave(void)
{
	DEBUG_LOG(LOG_DEBUG,"RAM retain YY %u, MM %u, DD %u, HH %u, mm %u, ss %u",gpRamSave->year, gpRamSave->month, gpRamSave->day, gpRamSave->hour, gpRamSave->min, gpRamSave->sec);
	DEBUG_LOG(LOG_DEBUG,"RAM retain walk %u, run %u, dash %u",gpRamSave->walk_count, gpRamSave->run_count, gpRamSave->dash_count);
	DEBUG_LOG(LOG_DEBUG,"RAM retain SID %u", gpRamSave->sid);
	DEBUG_LOG(LOG_DEBUG,"RAM retain age %u, weight %u, sex %u",gpRamSave->age,gpRamSave->weight,gpRamSave->sex);
}

/**
 * @brief Get RAM Save Walk Information
 * @param pDailyData Daily Data
 * @retval None
 */
void GetRamSaveWalkInfo(Daily_t *pDailyData)
{
	pDailyData->walk = gpRamSave->walk_count;
	pDailyData->run  = gpRamSave->run_count;
	pDailyData->dash = gpRamSave->dash_count;
}

/**
 * @brief Initialize RAM Date Set
 * @param pData date data
 * @retval None
 */
void InitRamDateSet(DATE_TIME *pData)
{
	gpRamSave->year  = pData->year;
	gpRamSave->month = pData->month;
	gpRamSave->day   = pData->day;
	gpRamSave->hour  = pData->hour;
	gpRamSave->min   = pData->min;
	gpRamSave->sec   = pData->sec;
	
	//future change.
	DEBUG_LOG(LOG_DEBUG,"Update RAM time using RTC Time");
	DEBUG_LOG(LOG_INFO,"Update RAM time YY %u, MM %u, DD %u, hh %u, mm %u, ss %u",gpRamSave->year,gpRamSave->month,gpRamSave->day ,gpRamSave->hour,gpRamSave->min,gpRamSave->sec);
}

/**
 * @brief RAM Sid Set
 * @param sid SID
 * @retval None
 */
void RamSidSet(uint16_t sid)
{
	gpRamSave->sid = sid;
	DEBUG_LOG(LOG_DEBUG,"set ram sid %u",gpRamSave->sid);
}

/**
 * @brief RAM Sid Clear
 * @param Noen
 * @retval None
 */
void RamSidClear(void)
{
	gpRamSave->sid = 0;
	mVolFlash_t.End_Sid = 0;
}

/**
 * @brief RAM Signatrue Check
 * @param Noen
 * @retval true 正常
 * @retval false 異常
 */
bool RamSignCheck(void)
{
	bool bret;
	int32_t ret;
	uint8_t signiture[RMA_SIG_SIZE];
	
	bret = true;
	memcpy(&signiture[0], (uint8_t*)RAM_SIG, sizeof(signiture));	
	ret = memcmp((void*)&gpRamSave->signiture, &signiture[0], sizeof(signiture));
	if(ret != 0)
	{
		DEBUG_LOG(LOG_ERROR, "RAM sign error");
		bret = false;
	}
	
	DEBUG_LOG(LOG_DEBUG,"ram save YY %u, MM %u, DD %u, HH %u",gpRamSave->year, gpRamSave->month, gpRamSave->day, gpRamSave->hour);
	DEBUG_LOG(LOG_DEBUG,"ram save mm %u, ss %u",gpRamSave->min, gpRamSave->sec);
	
	return bret;
}

/**
 * @brief RAM DataのDefault値をセットアップする
 * @param Noen
 * @retval None
 */
void CreateRamSaveDefault(void)
{
	gpRamSave->walk_count  = DEFAULT_WALK;
	gpRamSave->run_count   = DEFAULT_RUN;
	gpRamSave->dash_count  = DEFAULT_DASH;
	gpRamSave->sid         = DEFAULT_SID;
	gpRamSave->year        = DEFAULT_YEAR;
	gpRamSave->month       = DEFAULT_MONTH;
	gpRamSave->day         = DEFAULT_DAY ;
	gpRamSave->hour        = DEFAULT_HOUR;
	gpRamSave->min         = DEFAULT_MIN;
	gpRamSave->sec         = DEFAULT_SEC;
	gpRamSave->age         = DEFAULT_AGE;
	gpRamSave->weight      = DEFAULT_WEIGHT;
	gpRamSave->sex         = DEFAULT_SEX;

	/*add offset. ram save default value*/
	gpRamSave->offsetFlg  = DEFAULT_SENSOR_OFFSET_SIG;
	gpRamSave->offset[0]  = DEFAULT_SENSOR_OFFSET;
	gpRamSave->offset[1]  = DEFAULT_SENSOR_OFFSET;
	gpRamSave->offset[2]  = DEFAULT_SENSOR_OFFSET;
	
	memcpy((uint8_t*)(gpRamSave->signiture), RAM_SIG, sizeof(gpRamSave->signiture));
	
	//future change.
	DEBUG_LOG(LOG_INFO,"default ram save YY   %u, MM  %u, DD  %u, HH %u, mm %u, ss %u",gpRamSave->year, gpRamSave->month, gpRamSave->day, gpRamSave->hour,gpRamSave->min, gpRamSave->sec);
	DEBUG_LOG(LOG_INFO,"default ram save age  %u, sex %u, weight %u",gpRamSave->age, gpRamSave->sex, gpRamSave->weight);
	DEBUG_LOG(LOG_INFO,"default ram save walk %u, run %u, dash   %u, SID 0x%x", gpRamSave->walk_count, gpRamSave->run_count, gpRamSave->dash_count,gpRamSave->sid);
	DEBUG_LOG(LOG_INFO,"default ram save offset sig 0x%x, x 0x%x, y 0x%x, z 0x%x",gpRamSave->offsetFlg,gpRamSave->offset[0],gpRamSave->offset[1],gpRamSave->offset[2]);
}

/**
 * @brief RAMに保持されているSIDを読み出す
 * @param Noen
 * @retval sid 保持されているSID
 */
uint16_t RamSidGet(void)
{
	return gpRamSave->sid; 
}

/**
 * @brief RAMのwalk informationの領域をクリアする
 * @param Noen
 * @retval None
 */
void RamSaveRegionWalkInfoClear(void)
{
	DEBUG_LOG(LOG_DEBUG,"pre clear ram. walk info, w %u, r %u, d %u",gpRamSave->walk_count, gpRamSave->run_count, gpRamSave->dash_count);
	gpRamSave->walk_count = 0;
	gpRamSave->run_count  = 0;
	gpRamSave->dash_count = 0;	
}

/**
 * @brief RAMに保持されている日時データを取得する
 * @param pSaveDateTime 取得する日時データ
 * @retval None
 */
void GetRamTime(DATE_TIME *pSaveDateTime)
{
	pSaveDateTime->year  = gpRamSave->year;
	pSaveDateTime->month = gpRamSave->month;
	pSaveDateTime->day   = gpRamSave->day;
	pSaveDateTime->hour  = gpRamSave->hour;
	pSaveDateTime->min   = gpRamSave->min;
	pSaveDateTime->sec   = gpRamSave->sec;
}

/**
 * @brief RAMへプレイヤー情報を設定する
 * @param age 年齢
 * @param sex 性別
 * @param weight 体重
 * @retval None
 */
void SetRamPlayerInfo(uint8_t age, uint8_t sex, uint8_t weight)
{
	gpRamSave->age    = age;
	gpRamSave->sex    = sex;
	gpRamSave->weight = weight;
	//algo coef set
	DashCoeffSelect((uint16_t)age);
}

/**
 * @brief RAMに保持されているプレイヤー情報(年齢)をalgorithmへ設定する
 * @param None
 * @retval None
 */
void SetAlgoPlayerInfo(void)
{
	DashCoeffSelect(gpRamSave->age);
}

/**
 * @brief RAMに保持されているデイリー情報を取得する
 * @param pDaily_data デイリー情報を格納する変数へのポインタ
 * @retval None
 */
void GetRamDailyLog(Daily_t *pDaily_data)
{
	pDaily_data->walk	= gpRamSave->walk_count;
	pDaily_data->run	= gpRamSave->run_count;
	pDaily_data->dash	= gpRamSave->dash_count;
	pDaily_data->sid	= gpRamSave->sid;
	
	pDaily_data->Date[YEAR_HIGH]  = gpRamSave->year  / 10;
	pDaily_data->Date[YEAR_LOW]   = gpRamSave->year  % 10;
	pDaily_data->Date[MONTH_HIGH] = gpRamSave->month / 10;
	pDaily_data->Date[MONTH_LOW]  = gpRamSave->month % 10;
	pDaily_data->Date[DAY_HIGH]   = gpRamSave->day   / 10;
	pDaily_data->Date[DAY_LOW]    = gpRamSave->day   % 10;
	pDaily_data->Date[HOUR_HIGH]  = gpRamSave->hour  / 10;
	pDaily_data->Date[HOUR_LOW]   = gpRamSave->hour  % 10;
	
	DEBUG_LOG(LOG_DEBUG,"year  %u%u",pDaily_data->Date[YEAR_HIGH],pDaily_data->Date[YEAR_LOW]);
	DEBUG_LOG(LOG_DEBUG,"month %u%u",pDaily_data->Date[MONTH_HIGH],pDaily_data->Date[MONTH_LOW]);
	DEBUG_LOG(LOG_DEBUG,"day   %u%u",pDaily_data->Date[DAY_HIGH],pDaily_data->Date[DAY_LOW]);
	DEBUG_LOG(LOG_DEBUG,"hour  %u%u",pDaily_data->Date[HOUR_HIGH],pDaily_data->Date[HOUR_LOW]);
}

/**
 * @brief RAMに保持されているSIDをインクリメントする
 * @param None
 * @retval None
 */
void RamSaveSidIncrement(void)
{
	gpRamSave->sid++;
	DEBUG_LOG(LOG_DEBUG,"sid increments %u",gpRamSave->sid);
}

/**
 * @brief RAMに保持されている日時情報を更新する
 * @param pDateTime 更新する日時情報へのポインタ
 * @retval None
 */
void RamSaveTimeUpdate(uint8_t *pDateTime)
{
	gpRamSave->year  = *(pDateTime + YEAR_HIGH)  * 10  + *(pDateTime + YEAR_LOW);
	gpRamSave->month = *(pDateTime + MONTH_HIGH) * 10  + *(pDateTime + MONTH_LOW);
	gpRamSave->day   = *(pDateTime + DAY_HIGH)   * 10  + *(pDateTime + DAY_LOW);
	gpRamSave->hour  = *(pDateTime + HOUR_HIGH)  * 10  + *(pDateTime + HOUR_LOW);
	gpRamSave->min   = *(pDateTime + MIN_HIGH)   * 10  + *(pDateTime + MIN_LOW);
	gpRamSave->sec   = *(pDateTime + SEC_HIGH)   * 10  + *(pDateTime + SEC_LOW);
}

/**
 * @brief RAMに保持されている日時をRTCへ設定する
 * @param None
 * @retval None
 */
void SetRtcFromRamDateTime(void)
{
	DATE_TIME        dateTime;
	
	GetRamTime(&dateTime);
	DEBUG_LOG(LOG_DEBUG,"RTC set time from RAM");
	DEBUG_LOG(LOG_INFO,"RTC set time from RAM. YY %u, MM %u, DD %u, hh %u, mm %u, ss %u",dateTime.year,dateTime.month,dateTime.day,dateTime.hour,dateTime.min,dateTime.sec);
	ExRtcSetDateTime(&dateTime);
}

/**
 * @brief デイリー情報をRAMへ設定する
 * @param pDaily 設定するデイリー情報へのポインタ
 * @retval None
 */
void WalkRamSet(DAILYCOUNT *pDaily)
{
	gpRamSave->walk_count += pDaily->walk;
	gpRamSave->run_count  += pDaily->run;
	gpRamSave->dash_count += pDaily->dash;
}

/**
 * @brief デイリー情報をRAMへ設定する(Debug用)
 * @param None
 * @retval None
 */
void DebugWalkRamSet(void)
{
	gpRamSave->walk_count = 10;
	gpRamSave->run_count  = 20;
	gpRamSave->dash_count = 30;
}

/**
 * @brief set acc offset value for power retain ram function
 * @param None
 * @retval None
 */
void RamSaveOffset(uint16_t check, int32_t x_offset, int32_t y_offset, int32_t z_offset)
{
	/*set check signature*/
	gpRamSave->offsetFlg = check;
	/*set check signature and acc offset value*/
	gpRamSave->offset[0] = x_offset;
	gpRamSave->offset[1] = y_offset;
	gpRamSave->offset[2] = z_offset;
	
	//change future
	DEBUG_LOG(LOG_DEBUG,"offset x 0x%x, y 0x%x, z 0x%x",gpRamSave->offset[0],gpRamSave->offset[1],gpRamSave->offset[2]);
}

/**
 * @brief check acc offsetFlg and Reset
 * @param None
 * @retval None
 */
void AccOffsetCheckSystemReset(void)
{
	if(gpRamSave->offsetFlg == CHECK_DATA)
	{
		DEBUG_LOG( LOG_ERROR, "acc offset flag on, soft reset exec" );
		sd_nvic_SystemReset();
	}
}

/**
 * @brief get acc offset value and offset flag
 * @param x_offset X軸のOffset
 * @param y_offset Y軸のOffset
 * @param z_offset Z軸のOffset
 * @retval None
 */
void GetAccOffsetValueFromRam(int32_t *x_offset, int32_t *y_offset, int32_t *z_offset)
{
	*x_offset = gpRamSave->offset[0];
	*y_offset = gpRamSave->offset[1];
	*z_offset = gpRamSave->offset[2];
}

/**
 * @brief get acc offset flg from power retain ram
 * @param flg offset flag
 * @retval None
 */
void GetAccOffsetFlgFromRam(uint16_t *flg)
{
	*flg = gpRamSave->offsetFlg;
}

/**
 * @brief clear acc offset flg in power retain ram
 * @param None
 * @retval None
 */
void ClearAccOffsetFlgToRam(void)
{
	gpRamSave->offsetFlg = DEFAULT_SENSOR_OFFSET_SIG;
}

/**
 * @brief init acc offset value in power retain ram 
 * @param None
 * @retval None
 */
void ClearRamAccOffsetValue(void)
{
	gpRamSave->offset[0] = DEFAULT_SENSOR_OFFSET;
	gpRamSave->offset[1] = DEFAULT_SENSOR_OFFSET;
	gpRamSave->offset[2] = DEFAULT_SENSOR_OFFSET;
}
