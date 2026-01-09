/**
  ******************************************************************************************
  * @file    daily_log.c
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

/* Includes --------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_fstorage.h"
#include "nrf_fstorage_sd.h"
#include "app_error.h"
#include "lib_common.h"
#include "daily_log.h"
#include "nrf_ble_gatt.h"
#include "flash_operation.h"
#include "lib_timer.h"
#include "lib_fifo.h"
#include "mode_manager.h"
#include "lib_ram_retain.h"
#include "ble_definition.h"
#include "lib_trace_log.h"

/* Definition ------------------------------------------------------------*/
/* Flash Data*/
volatile Flash_Sid_t mVolFlash_t;
volatile uint8_t mSid_over_count = 0;
volatile int8_t mSid_over_flg   = 0;

/**
 * @brief Get Last Date From Flash
 * @param date_data Date Data
 * @param sid_data SID Data
 * @retval res 0 Failed
 * @retval res 1 Success
 */
static uint16_t get_flash_lastdate(DATE_TIME *date_data, uint16_t sid_data)
{
	uint16_t res;
	struct tm time;

	res = GetLastDate(&time,sid_data);
	if(res == 1)
	{
		DEBUG_LOG(LOG_DEBUG,"Read from flash before cal yy %u, mm %u, dd %u, hh %u, m %u, s %u",time.tm_year, time.tm_mon, time.tm_mday, time.tm_hour,time.tm_min,time.tm_sec);
		
		time.tm_min = 0;
		time.tm_sec = 0;
		
		OneHourProgressTime(&time);

		date_data->year  = time.tm_year;
		date_data->month = time.tm_mon;
		date_data->day   = time.tm_mday;
		date_data->hour  = time.tm_hour;
		date_data->min   = 0;
		date_data->sec   = 0;
		InitRamDateSet(date_data);
	}
	return res;
}

/**
 * @brief Get SID Over Count
 * @param None
 * @retval mSid_over_count sid over count
 */
uint8_t GetSidOverCount(void)
{
	return mSid_over_count;
}

/**
 * @brief Get SID From Retention RAM
 * @param None
 * @retval None
 */
void GetSidFromRamRegion(void)
{
	mVolFlash_t.End_Sid = RamSidGet();
	DEBUG_LOG(LOG_DEBUG,"get ram sid %u",mVolFlash_t.End_Sid);
}

/**
 * @brief VolFlash Struct Clear
 * @param None
 * @retval None
 */
void VolFlashClear(void)
{
	memset((Flash_Sid_t*)&mVolFlash_t, 0x00, sizeof(mVolFlash_t));
}

/**
 * @brief Set Flash SID
 * @param None
 * @retval None
 */
void SetFlashSid(void)
{

	int8_t write_data;
	//Sensor Init
	mVolFlash_t.End_Sid   = GetLastSid(&write_data, (void*)&mSid_over_count);
	DEBUG_LOG(LOG_INFO,"get last sid from flash %u",mVolFlash_t.End_Sid);
	mVolFlash_t.Send_Sid  = 0;
	mVolFlash_t.Start_Sid = 0;
	mVolFlash_t.Send_Flg  = SEND_NO_DATA_STATE;
	
	if(write_data == true)
	{
		if(mVolFlash_t.End_Sid == MAX_BLE_SID)
		{
			mVolFlash_t.End_Sid = 0;
			mSid_over_count++;
		}
		else
		{
			mVolFlash_t.End_Sid++;
		}
		if((0 < mSid_over_count) && (mVolFlash_t.End_Sid < FLASH_DATA_MAX))
		{
			mSid_over_flg = 1;
		}
		RamSidSet(mVolFlash_t.End_Sid);
	}
}

/**
 * @brief Set Daily Log Send Flag
 * @param None
 * @retval None
 */
void SetDaliyLogSendFlg( void )
{
	mVolFlash_t.Send_Flg = SEND_DATA_STATE;
}

/**
 * @brief Set Daily Log Single Send Flag
 * @param None
 * @retval None
 */
void SetDaliyLogSingleSendFlg( void )
{
	mVolFlash_t.Send_SingleFlg = SEND_DATA_STATE;
}

/**
 * @brief Set Daily Log Start SID
 * @param start_id Start SID
 * @retval None
 */
void SetStartSid(uint16_t start_id)
{
	mVolFlash_t.Start_Sid = start_id;
}

/**
 * @brief Set Daily Log End SID
 * @param end_id End SID
 * @retval None
 */
void SetEndSid(uint16_t end_id)
{
	mVolFlash_t.End_Sid = end_id;
}

/**
 * @brief DailyLog Continue Configuration.
 * @param pEvent Event Information
 * @retval 0 Success
 */
uint32_t SendDailyLog(PEVT_ST pEvent)
{
	uint32_t ble_err_code;
	uint32_t fifo_err;
	uint16_t sid;
	uint16_t read_sid;
	uint16_t count;
	uint16_t notify_size;
	uint16_t daily_log_id_value_handle;
	uint8_t res;
	int8_t set_case;
	int8_t error_id;
	Daily_t daily_data;
	ble_gatts_hvx_params_t notify_data;
	static uint8_t retrycount = 0;
	uint16_t cnt_handle = BLE_CONN_HANDLE_INVALID;
	EVT_ST replyEvent;
	EVT_ST endEvent;

	GetGattsCharHandleValueID(&daily_log_id_value_handle,DAILY_LOG_ID);
	
	notify_size           = sizeof(daily_data);
	count                 = 0;
	error_id              = true;
	notify_data.handle    = daily_log_id_value_handle;
	notify_data.type      = BLE_GATT_HVX_NOTIFICATION;
	notify_data.offset    = BLE_NOTIFY_OFFSET;
	mVolFlash_t.Start_Sid = pEvent->DATA.dailyId.sid;
	EVT_ST                   event;
	
	DEBUG_LOG(LOG_INFO,"send log start %u. End %u",mVolFlash_t.Start_Sid, mVolFlash_t.End_Sid);
	
	if((0 < mSid_over_count) && (mVolFlash_t.End_Sid < 255))
	{
		if(mVolFlash_t.Start_Sid <= mVolFlash_t.End_Sid)
		{
			if((mVolFlash_t.Start_Sid + FLASH_DATA_MAX) < mVolFlash_t.End_Sid)
			{
				//error
				mVolFlash_t.Start_Sid = mVolFlash_t.End_Sid - FLASH_DATA_MAX;
				
			}
		}
		else
		{
			if(FLASH_DATA_MAX <= ((MAX_BLE_SID - mVolFlash_t.Start_Sid) + mVolFlash_t.End_Sid))
			{
				//error
				mVolFlash_t.Start_Sid = MAX_BLE_SID - (FLASH_DATA_MAX - mVolFlash_t.End_Sid) + 1;
			}
		}
	}
	else
	{
		if(mVolFlash_t.Start_Sid <= mVolFlash_t.End_Sid)
		{
			if((mVolFlash_t.Start_Sid + FLASH_DATA_MAX) < mVolFlash_t.End_Sid)
			{
				if(mVolFlash_t.End_Sid < FLASH_DATA_MAX)
				{
					//error
					mVolFlash_t.Start_Sid = 0;
					
				}
				else
				{
					//error
					mVolFlash_t.Start_Sid = mVolFlash_t.End_Sid - FLASH_DATA_MAX;
				}
			}
		}
		else
		{
			//error
			mVolFlash_t.Start_Sid = mVolFlash_t.End_Sid;
		}
	}
	
	mVolFlash_t.Send_Flg = pEvent->dailyLogSendSt;
	
	while(mVolFlash_t.Send_Flg > SEND_NO_DATA_STATE)
	{
		if((mVolFlash_t.Start_Sid !=  mVolFlash_t.End_Sid) && (error_id == true))
		{
			sid =  mVolFlash_t.Start_Sid % FLASH_DATA_MAX;
			DEBUG_LOG(LOG_DEBUG,"s mvol sid %u, sid %u",mVolFlash_t.Start_Sid,sid);
			
			/* Flash Read*/
			if(sid < FLASH_DATA_CENTER)
			{
				read_sid = sid;
				res = CheckFlash(0);
				set_case = 0;
			}
			else
			{
				read_sid = sid - FLASH_DATA_CENTER;
				res = CheckFlash(1);
				set_case = 1;
			}
			
			if(res == 0)
			{
				//No data
				DEBUG_LOG(LOG_ERROR,"No data");
				mVolFlash_t.Start_Sid = mVolFlash_t.End_Sid + 1;
				error_id = false;
			}
			else
			{
				DEBUG_LOG(LOG_DEBUG,"READ SID %u, SID %u, case %u",read_sid, sid, set_case);
				FlashRead(read_sid,&daily_data, set_case);
				/* Set Send Data*/
				notify_data.p_data = (uint8_t*)&daily_data;
				notify_data.p_len  = &notify_size;
				GetBleCntHandle(&cnt_handle);
				ble_err_code = sd_ble_gatts_hvx(cnt_handle, &notify_data);
				DEBUG_LOG(LOG_DEBUG,"continue send daily continue w %u, r %u, d %u, sid %u",daily_data.walk,daily_data.run,daily_data.dash,daily_data.sid);
				DEBUG_LOG(LOG_DEBUG,"e");
				if(ble_err_code != NRF_SUCCESS)
				{
					DEBUG_LOG(LOG_DEBUG, "read daily cmd notify err 0x%x",ble_err_code);
					/* Retry Event Setting*/
#if DEBUG_READ_LOG_RETRY_SWITCH
					retrycount++;
					if(DAILY_LOG_RETRY_COUNT < retrycount)
					{
						DEBUG_LOG(LOG_ERROR,"daily log retry count over 1. 0x%x, retry count %u",ble_err_code,retrycount);
						
						TRACE_LOG(TR_DAILY_LOG_READ_CONTINUE_SEND_ERROR,(uint16_t)retrycount);
						
						retrycount = 0;
						FlashOpForceInit();
						return 0;
					}
#endif
					DEBUG_LOG(LOG_INFO,"continue log send, retry event 1. 0x%x",ble_err_code);
					memcpy(&replyEvent,pEvent,sizeof(replyEvent));
					replyEvent.DATA.dailyId.sid = mVolFlash_t.Start_Sid;
					replyEvent.dailyLogSendSt   = mVolFlash_t.Send_Flg;    //send state save
					event.evt_id = EVT_FORCE_SLEEP;
					fifo_err = PushFifo(&event);
					DEBUG_EVT_FIFO_LOG(fifo_err,event.evt_id);
					fifo_err = PushFifo(&replyEvent);
					DEBUG_EVT_FIFO_LOG(fifo_err,replyEvent.evt_id);
					return 0;
				}
				else
				{
					retrycount = 0;
					DEBUG_LOG(LOG_DEBUG,"daily log send cmple 1. 0x%x",ble_err_code);
					if(mVolFlash_t.Start_Sid == MAX_BLE_SID)
					{
						DEBUG_LOG(LOG_DEBUG,"SID max");
						mVolFlash_t.Start_Sid = 0;
					}
					else
					{
						mVolFlash_t.Start_Sid++;
						DEBUG_LOG(LOG_DEBUG,"start %u",mVolFlash_t.Start_Sid);
					}
				}
			}
		}
		else
		{
			if(mVolFlash_t.Send_Flg == SEND_DATA_STATE)
			{
				GetRamDailyLog(&daily_data);
				mVolFlash_t.Send_Flg = SEND_FINISH_STATE;
			}
			else
			{
				memset(&daily_data, 0x00, sizeof(daily_data));
				if(mVolFlash_t.End_Sid < FLASH_DATA_MAX)
				{
					if(0 == mSid_over_count)
					{
						DEBUG_LOG(LOG_DEBUG,"over count %u.",mSid_over_count);
						daily_data.sid = 0;
					}
					else
					{
						daily_data.sid = MAX_BLE_SID - (FLASH_DATA_MAX - mVolFlash_t.End_Sid) + 1;
					}
				}
				else
				{
					daily_data.sid = mVolFlash_t.End_Sid - FLASH_DATA_MAX;
				}
				mVolFlash_t.Send_Flg = SEND_NO_DATA_STATE;
			}
			/* Set Send Data*/
			notify_data.p_data = (uint8_t*)&daily_data;
			notify_data.p_len  = &notify_size;
			GetBleCntHandle(&cnt_handle);
			ble_err_code = sd_ble_gatts_hvx(cnt_handle, &notify_data);
			DEBUG_LOG(LOG_INFO,"send continue end w %u, r %u, d %u, sid %u",daily_data.walk,daily_data.run,daily_data.dash,daily_data.sid);
			if(ble_err_code != NRF_SUCCESS)
			{
				if(mVolFlash_t.Send_Flg == SEND_FINISH_STATE)
				{
					mVolFlash_t.Send_Flg = SEND_DATA_STATE;
				}
				else
				{
					mVolFlash_t.Send_Flg = SEND_FINISH_STATE;
				}
				/* Retry Event Setting*/
				retrycount++;
#if DEBUG_READ_LOG_RETRY_SWITCH
				if(DAILY_LOG_RETRY_COUNT < retrycount)
				{
					DEBUG_LOG(LOG_ERROR,"daily log retry count over 2. 0x%x, retry count %u",ble_err_code,retrycount);
					TRACE_LOG(TR_DAILY_LOG_READ_CONTINUE_SEND_END_ERROR,(uint16_t)retrycount);
					retrycount = 0;
					FlashOpForceInit();
					return 0;
				}
#endif		
				DEBUG_LOG(LOG_INFO,"continue log send, retry event 2");
				memcpy(&replyEvent,pEvent,sizeof(replyEvent));
				replyEvent.DATA.dailyId.sid = mVolFlash_t.Start_Sid;
				replyEvent.dailyLogSendSt   = mVolFlash_t.Send_Flg;
				DEBUG_LOG(LOG_DEBUG,"state flag 0x%x",replyEvent.dailyLogSendSt);
				
					
				event.evt_id = EVT_FORCE_SLEEP;
				fifo_err = PushFifo(&event);
				DEBUG_EVT_FIFO_LOG(fifo_err,event.evt_id);
				
				fifo_err = PushFifo(&replyEvent);
				DEBUG_EVT_FIFO_LOG(fifo_err,replyEvent.evt_id);

				return 0;
			}
			else
			{
				DEBUG_LOG(LOG_DEBUG,"continue send success");
				retrycount = 0;
			}
		}
		count++;
		if(FLASH_DATA_MAX <= count)
		{
			mVolFlash_t.Send_Flg = SEND_FINISH_STATE;
			DEBUG_LOG(LOG_INFO,"send  retry event 3");
			memcpy(&endEvent,pEvent,sizeof(endEvent));
			endEvent.DATA.dailyId.sid = mVolFlash_t.Start_Sid;
			endEvent.dailyLogSendSt   = mVolFlash_t.Send_Flg;
			
			event.evt_id = EVT_FORCE_SLEEP;
			fifo_err = PushFifo(&event);
			DEBUG_EVT_FIFO_LOG(fifo_err,event.evt_id);
			
			fifo_err = PushFifo(&endEvent);
			DEBUG_EVT_FIFO_LOG(fifo_err,endEvent.evt_id);

		}
	}
	FlashOpForceInit();
	RestartForceDisconTimer();
	
	DEBUG_LOG(LOG_DEBUG,"send log end");
	
	return 0;
}

/**
 * @brief DailyLog Single Configuration.
 * @param pEvent Event Information
 * @retval 0 Success
 */
uint32_t SendSingleDailyLog(PEVT_ST pEvent)
{
	uint32_t ble_err_code;
	uint32_t fifo_err;
	Daily_t daily_data;
	uint8_t res;
	uint16_t sid;
	uint16_t read_sid;
	uint16_t daily_log_id_value_handle;
	int8_t set_case;
	int8_t send_end_data;
	uint16_t notify_size;
	ble_gatts_hvx_params_t notify_data;
	static uint8_t retrycount = 0;
	uint16_t cnt_handle = BLE_CONN_HANDLE_INVALID;
	EVT_ST sleepEvent;
	EVT_ST replyEvent;
	
	send_end_data = false;
	GetGattsCharHandleValueID(&daily_log_id_value_handle,DAILY_LOG_ID);
	
	notify_size           = sizeof(daily_data);
	notify_data.handle    = daily_log_id_value_handle;
	notify_data.type      = BLE_GATT_HVX_NOTIFICATION;
	notify_data.offset    = BLE_NOTIFY_OFFSET;
	
	mVolFlash_t.Start_Sid = pEvent->DATA.dailyId.sid;
	DEBUG_LOG(LOG_INFO,"Single Daily log read. sid %u", mVolFlash_t.Start_Sid);
	if((0 < mSid_over_count) && (mVolFlash_t.End_Sid < 255))
	{
		if(mVolFlash_t.Start_Sid <= mVolFlash_t.End_Sid)
		{
			if((mVolFlash_t.Start_Sid + FLASH_DATA_MAX) < mVolFlash_t.End_Sid)
			{
				//error
				DEBUG_LOG(LOG_ERROR,"Single Daily err 1");
				send_end_data = true;
			}
		}
		else
		{
			if(FLASH_DATA_MAX <= ((MAX_BLE_SID - mVolFlash_t.Start_Sid) + mVolFlash_t.End_Sid))
			{
				DEBUG_LOG(LOG_ERROR,"Single Daily err 2");
				send_end_data = true;
			}
		}
	}
	else
	{
		if(mVolFlash_t.Start_Sid <= mVolFlash_t.End_Sid)
		{
			if((mVolFlash_t.Start_Sid + FLASH_DATA_MAX) < mVolFlash_t.End_Sid)
			{
				if(mVolFlash_t.End_Sid == FLASH_DATA_MAX)
				{
					//error
					DEBUG_LOG(LOG_ERROR,"Single Daily err 3");
					send_end_data = true;
				}
				else
				{
					//error
					DEBUG_LOG(LOG_ERROR,"Single Daily err 4");
					send_end_data = true;
				}
			}
		}
		else
		{
			//error
			DEBUG_LOG(LOG_ERROR,"Single Daily err 5");
			send_end_data = true;
		}
	}
	mVolFlash_t.Send_SingleFlg = SEND_DATA_STATE;
	while(mVolFlash_t.Send_SingleFlg > SEND_NO_DATA_STATE)
	{
		if(send_end_data == false)
		{
			sid =  mVolFlash_t.Start_Sid % FLASH_DATA_MAX;

			/* Flash Read*/
			if(sid < FLASH_DATA_CENTER)
			{
				read_sid = sid;
				res = CheckFlash(0);
				set_case = 0;
			}
			else
			{
				read_sid = sid - FLASH_DATA_CENTER;
				res = CheckFlash(1);
				set_case = 1;
			}

			if(res == 0)
			{
				//No data
				DEBUG_LOG(LOG_ERROR,"Single Daily No data");
				send_end_data = true;
			}
			else
			{
				FlashRead(read_sid, &daily_data, set_case);
				if(daily_data.sid == mVolFlash_t.Start_Sid)
				{
					/* Set Send Data*/
					notify_data.p_data = (uint8_t*)&daily_data;
					notify_data.p_len  = &notify_size ;
					GetBleCntHandle(&cnt_handle);
					DEBUG_LOG(LOG_DEBUG,"single day log send w %u, r %u, d %u, sid %u",daily_data.walk,daily_data.run,daily_data.dash,daily_data.sid);
					ble_err_code = sd_ble_gatts_hvx(cnt_handle,&notify_data);
					if(ble_err_code != NRF_SUCCESS)
					{
						/* Retry Event Setting*/
						retrycount++;
						if(DAILY_LOG_RETRY_COUNT < retrycount)
						{
							DEBUG_LOG(LOG_ERROR,"single daily log retry count over 1. 0x%x, retry count %u", ble_err_code, retrycount);
							TRACE_LOG(TR_DAILY_LOG_READ_SINGLE_SEND_ERROR,(uint16_t)retrycount);
							retrycount = 0;
							FlashOpForceInit();
							return 0;
						}
						DEBUG_LOG(LOG_DEBUG,"single daily log notify err 0x%x. single retry1",ble_err_code);
						DEBUG_LOG(LOG_INFO,"single log event retry 1");
						memcpy(&replyEvent,pEvent,sizeof(replyEvent));
						replyEvent.DATA.dailyId.sid = mVolFlash_t.Start_Sid;
						
						sleepEvent.evt_id = EVT_FORCE_SLEEP;
						fifo_err = PushFifo(&sleepEvent);
						DEBUG_EVT_FIFO_LOG(fifo_err,sleepEvent.evt_id);
						fifo_err = PushFifo(&replyEvent);
						DEBUG_EVT_FIFO_LOG(fifo_err,replyEvent.evt_id);
						return 0;
					}
					else
					{
						retrycount = 0;
						DEBUG_LOG(LOG_INFO,"single data send notify success");
					}
					send_end_data = true;
				}
				else
				{
					send_end_data = true;
				}
			}
		}
		else
		{
			memset(&daily_data, 0,sizeof(Daily_t));
			DEBUG_LOG(LOG_INFO,"mVol end sid %u, flash max %u. 1",mVolFlash_t.End_Sid, FLASH_DATA_MAX);
			if(mVolFlash_t.End_Sid < FLASH_DATA_MAX)
			{
				if(0 < mSid_over_count)
				{
					daily_data.sid = 0;
				}
				else
				{
					daily_data.sid = MAX_BLE_SID - (FLASH_DATA_MAX - mVolFlash_t.End_Sid);
					DEBUG_LOG(LOG_INFO,"daily_data_end sid %u, flash max %u. 2",daily_data.sid, FLASH_DATA_MAX);
				}
			}
			else
			{
				daily_data.sid = mVolFlash_t.End_Sid - FLASH_DATA_MAX;
			}
			mVolFlash_t.Send_SingleFlg = SEND_NO_DATA_STATE;
			
			/* Set Send Data*/
			notify_data.p_data = (uint8_t*)&daily_data;
			notify_data.p_len  = &notify_size;
			GetBleCntHandle(&cnt_handle);
			ble_err_code = sd_ble_gatts_hvx(cnt_handle,&notify_data);
			DEBUG_LOG(LOG_INFO,"single daily log end data w %u, r %u, d %u, sid %u",daily_data.walk,daily_data.run,daily_data.dash,daily_data.sid);
			if(ble_err_code != NRF_SUCCESS)
			{
				memcpy(&replyEvent,pEvent,sizeof(replyEvent));
				/* Retry Event Setting*/
				if(mVolFlash_t.End_Sid == MAX_BLE_SID)
				{
					replyEvent.DATA.dailyId.sid = 0;
				}
				else
				{
					replyEvent.DATA.dailyId.sid = mVolFlash_t.End_Sid + 1;
				}
				
				if(DAILY_LOG_RETRY_COUNT < retrycount)
				{
					DEBUG_LOG(LOG_ERROR,"single daily log retry count over 2. 0x%x, retry count %u",ble_err_code,retrycount);
					TRACE_LOG(TR_DAILY_LOG_READ_SINGLE_SEND_END_ERROR,(uint16_t)retrycount);
					retrycount = 0;
					FlashOpForceInit();
					return 0;
				}
				DEBUG_LOG(LOG_INFO,"single log event retry 2");
				sleepEvent.evt_id = EVT_FORCE_SLEEP;
				fifo_err = PushFifo(&sleepEvent);
				DEBUG_EVT_FIFO_LOG(fifo_err,sleepEvent.evt_id);
				fifo_err = PushFifo(&replyEvent);
				DEBUG_EVT_FIFO_LOG(fifo_err,replyEvent.evt_id);

				return 0;
			}
			else
			{
				retrycount = 0;
				DEBUG_LOG(LOG_INFO,"single data end notify success");
			}
		}
	}
	FlashOpForceInit();
	RestartForceDisconTimer();
	
	return 0;
}

/**
 * @brief Set new SID
 * @param None
 * @retval None
 */
void SetWriteSid( void )
{
	if(mVolFlash_t.End_Sid == MAX_BLE_SID)
	{
		mSid_over_flg = 1;
		mVolFlash_t.End_Sid = 0;
		mSid_over_count++;
	}
	else
	{
		mVolFlash_t.End_Sid++;
	}
}

/**
 * @brief Set RTC From Flash Date Time
 * @param None
 * @retval ret 0 Flashからの書き込み成功
 * @retval ret 1 Flashに時刻なし
 */
uint32_t SetRtcFromFlashDateTime(void)
{
	volatile DATE_TIME dateTime;
	uint16_t res;
	int8_t write_data;
	uint16_t flash_sid;
	uint32_t ret = 0;

	uint8_t   dummy_over_count;
	
	flash_sid = GetLastSid(&write_data, &dummy_over_count);

	if(write_data == true)
	{
		res = get_flash_lastdate((DATE_TIME*)&dateTime, flash_sid);
		if(res == 1)
		{
			DEBUG_LOG(LOG_INFO,"RTC set time from flash. YY %u, MM %u, DD %u, hh %u, mm %u, ss %u",dateTime.year,dateTime.month,dateTime.day,dateTime.hour,dateTime.min,dateTime.sec);
			ExRtcSetDateTime((DATE_TIME*)&dateTime);
		}
		else
		{
			DEBUG_LOG(LOG_ERROR,"Do not read Time from flash.");
			ret = 1;
		}
	}
	else
	{
		DEBUG_LOG(LOG_ERROR,"Invalid flash data.");
		ret = 1;
	}
	return ret;
}

/* 2022.07.25 Add RTC異常時対応 ++ */
/**
 * @brief Get Time From Flash Date
 * @param None
 * @retval ret 0 Flashからの書き込み成功
 * @retval ret 1 Flashに時刻なし
 */
uint32_t GetTimeFromFlashDateTime( DATE_TIME *dateTime )
{
	uint16_t res;
	int8_t write_data;
	uint16_t flash_sid;
	uint32_t ret = 0;
	uint8_t  dummy_over_count;
	
	if ( dateTime != NULL )
	{
		/* SIDを取得 */
		flash_sid = GetLastSid( &write_data, &dummy_over_count );
		if( write_data == true )
		{
			res = get_flash_lastdate( dateTime, flash_sid );
			if ( res == 1 )
			{
				DEBUG_LOG( LOG_INFO,"Time from Flash. YY %u, MM %u, DD %u, hh %u, mm %u, ss %u", dateTime->year, dateTime->month, dateTime->day, dateTime->hour, dateTime->min, dateTime->sec );
			}
			else
			{
				DEBUG_LOG( LOG_ERROR,"Do not read Time from flash.");
				ret = 1;
			}
		}
		else
		{
			DEBUG_LOG( LOG_ERROR,"Invalid flash data.");
			ret = 1;
		}
	}
	else
	{
		ret = 2;
	}
	
	return ret;
}
/* 2022.07.25 Add RTC異常時対応 -- */

