/**
  ******************************************************************************************
  * @file    lib_flash.c
  * @author  k.tashiro
  * @version 1.0
  * @date    2020/09/17
  * @brief   Flash Contorl
  ******************************************************************************************
    Revision:
    Ver            Date             Revised by        Explanation
  ******************************************************************************************
  * 1.0            2020/09/17       k.tashiro         create new
  ******************************************************************************************
*/

/* Includes --------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
#include "nordic_common.h"
#include "nrf.h"
#include "nrf_fstorage.h"
#include "nrf_fstorage_sd.h"
#include "app_error.h"

#include "daily_log.h"
#include "lib_fifo.h"
#include "lib_common.h"
#include "lib_flash.h"
#include "state_control.h"
#include "ble_manager.h"
#include "ble_definition.h"
#include "mode_manager.h"
#include "lib_ram_retain.h"
#include "definition.h"
#include "lib_timer.h"
#include "flash_operation.h"
#include "lib_trace_log.h"

/* Definition ------------------------------------------------------------*/
#define  EX_FLASH_FILE_ID 0x8000

/* Private variables -----------------------------------------------------*/
volatile DAILY_WRITE_INFO tmpDailyWriteInfo;
TRACEDATA g_TrcaeRegion;
volatile uint8_t gFlash_daily_log[PAGE_SIZE];

/* Private function prototypes -------------------------------------------*/
/**
 * @brief check sid and get sid
 * @param p_fstorage  fstorage instance
 * @param *sid SIDを格納する変数へのポインタ
 * @retval ret
 *			- 0 : SIDなし
 *			- 1 : End SID
 *			- 2 : Start SID
 */
static int8_t check_sid(nrf_fstorage_t *p_fstorage, uint16_t* sid);

/**
 * @brief read word data
 * @param p_fstorage  fstorage instance
 * @param addr 読み出すAddress
 * @retval data 読み出したデータ
 */
static uint32_t read_word_data(nrf_fstorage_t *p_fstorage, uint32_t addr);

/**
 * @brief read byte data
 * @param p_fstorage  fstorage instance
 * @param addr 読み出すAddress
 * @retval data 読み出したデータ
 */
static uint8_t read_byte_data(nrf_fstorage_t *p_fstorage, uint32_t addr);

/**
 * @brief fstorage Event Handler
 * @param p_evt Event
 * @retval None
 */
static void fstorage_evt_handler(nrf_fstorage_evt_t * p_evt)
{
	uint32_t fifo_err;
	EVT_ST event;
	uint16_t tracelogId = TR_DUMMY_ID;
	uint16_t traceParam = 0;
	bool uart_output_enable;
	
	uart_output_enable = GetUartOutputStatus();
	if(uart_output_enable == false)
	{
		LibUartEnable();
	}
	
	if (p_evt->result != NRF_SUCCESS)
	{
		DEBUG_LOG(LOG_ERROR,"flash err. evt id 0x%x, address 0x%x",p_evt->id, p_evt->addr);
		SetBleErrReadCmd(UTC_BLE_FLASH_WRITE_ERROR);
		
		if(p_evt->id == NRF_FSTORAGE_EVT_WRITE_RESULT)
		{
			tracelogId = TR_FLASH_OPERATION_WRITE_FAILED;
		}
		else if(p_evt->id == NRF_FSTORAGE_EVT_ERASE_RESULT)
		{
			tracelogId = TR_FLASH_OPERATION_ERASE_FAILED;
		}
		else
		{
			traceParam = (uint16_t)0xffff;
		}
		TRACE_LOG(tracelogId, traceParam);
		
		FlashOpForceInit();
		
		if(uart_output_enable == false)
		{
			LibUartDisable();
		}
		return;
	}
	
	switch (p_evt->id)
	{
		case NRF_FSTORAGE_EVT_WRITE_RESULT:
		{
			event.evt_id = EVT_FLASH_DATA_WRITE_CMPL;
			fifo_err = PushFifo(&event);
			DEBUG_EVT_FIFO_LOG(fifo_err,event.evt_id);
			DEBUG_LOG(LOG_INFO,"FLASH WRITE CMPL");
		} break;

		case NRF_FSTORAGE_EVT_ERASE_RESULT:
		{
			event.evt_id = EVT_FLASH_DATA_ERASE_CMPL;
			fifo_err = PushFifo(&event);
			DEBUG_EVT_FIFO_LOG(fifo_err,event.evt_id);
			DEBUG_LOG(LOG_INFO,"FLASH ERASE CMPL");	
		} break;
		default:
			break;
	}
	
	if(uart_output_enable == false)
	{
		LibUartDisable();
	}
}

NRF_FSTORAGE_DEF(nrf_fstorage_t ex_fstorage) =
{
	//.p_flash_info.erase_unit = PAGE_SIZE,
	//.p_flash_info->erase_unit = PAGE_SIZE,
	/* Set a handler for fstorage events. */
	.evt_handler = fstorage_evt_handler,

	/* These below are the boundaries of the flash space assigned to this instance of fstorage.
	 * You must set these manually, even at runtime, before nrf_fstorage_init() is called.
	 * The function nrf5_flash_end_addr_get() can be used to retrieve the last address on the
	 * last page of flash available to write data. */
	.start_addr = DAILY_ADDR1,
	.end_addr   = DAILY_ADDR1 + TEST_PAGE_SIZE,//PAGE_SIZE,
};

NRF_FSTORAGE_DEF(nrf_fstorage_t ex_fstorage2) =
{
	/* Set a handler for fstorage events. */
	.evt_handler = fstorage_evt_handler,

	/* These below are the boundaries of the flash space assigned to this instance of fstorage.
	 * You must set these manually, even at runtime, before nrf_fstorage_init() is called.
	 * The function nrf5_flash_end_addr_get() can be used to retrieve the last address on the
	 * last page of flash available to write data. */
	.start_addr = DAILY_ADDR2,
	.end_addr   = DAILY_ADDR2 + TEST_PAGE_SIZE,//PAGE_SIZE,
};


NRF_FSTORAGE_DEF(nrf_fstorage_t player_pincode_fstorage) =
{
	/* Set a handler for fstorage events. */
	.evt_handler = fstorage_evt_handler,

	/* These below are the boundaries of the flash space assigned to this instance of fstorage.
	 * You must set these manually, even at runtime, before nrf_fstorage_init() is called.
	 * The function nrf5_flash_end_addr_get() can be used to retrieve the last address on the
	 * last page of flash available to write data. */
	.start_addr = PLAYER_AND_PAIRING_ADDR,
	.end_addr   = PLAYER_AND_PAIRING_ADDR + TEST_PAGE_SIZE ,//PAIRING_PAGE_SIZE,//PAIRING_DATA_SIZE,
};

NRF_FSTORAGE_DEF(nrf_fstorage_t trace_fstorage) =
{
	/* Set a handler for fstorage events. */
	.evt_handler = fstorage_evt_handler,

	/* These below are the boundaries of the flash space assigned to this instance of fstorage.
	 * You must set these manually, even at runtime, before nrf_fstorage_init() is called.
	 * The function nrf5_flash_end_addr_get() can be used to retrieve the last address on the
	 * last page of flash available to write data. */
	.start_addr = TRACE_ADDR,
	.end_addr   = TRACE_ADDR + TEST_PAGE_SIZE ,//TRACE_SIZE ,
};

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
int8_t FlashWrite(int32_t sid, Daily_t * data, int8_t set_case, uint8_t over_count)
{
	int8_t res;
	uint32_t start_addr;
	ret_code_t rc;
	nrf_fstorage_t *p_fstorage;
	nrf_fstorage_api_t *p_fs_api;
	
	p_fs_api = &nrf_fstorage_sd;
	res = 1;
	
	switch(set_case)
	{
		case 0:
			p_fstorage = &ex_fstorage;
			start_addr = DAILY_ADDR1;
			break;
		case 1:
			p_fstorage = &ex_fstorage2;
			start_addr = DAILY_ADDR2;
			break;
		default:
			res = 0;
		break;
	}
	if(res == 0)
	{
		DEBUG_LOG(LOG_ERROR,"Write Invalid Page address 0x%x",set_case);
		return res;
	}
	rc = nrf_fstorage_init(p_fstorage, p_fs_api, NULL);
	ERR_CHECK_FLASH(rc,(FLASH_INIT_ERROR|EX_FLASH_FILE_ID),__LINE__);
	
	rc = nrf_fstorage_read(p_fstorage, start_addr, (uint8_t*)&gFlash_daily_log, PAGE_SIZE);
	ERR_CHECK_FLASH(rc,(FLASH_READ_ERROR|EX_FLASH_FILE_ID),__LINE__);
	//memcpy used
	gFlash_daily_log[0] = SIGN_DATA_ONE;
	gFlash_daily_log[1] = SIGN_DATA_TWO;
	gFlash_daily_log[2] = over_count;
	memcpy((uint8_t*)&gFlash_daily_log[(sid * 16) + 4],data,sizeof(Daily_t));
	
	 /* Erase the DATA_STORAGE_PAGE before write operation */

	rc = nrf_fstorage_erase(p_fstorage, start_addr, 1, NULL);
	ERR_CHECK_FLASH(rc,(FLASH_ERASE_ERROR|EX_FLASH_FILE_ID),__LINE__);

	do{
		__NOP();
	}	while((nrf_fstorage_is_busy(p_fstorage)));

	rc = nrf_fstorage_write(p_fstorage, start_addr, (uint8_t*)&gFlash_daily_log, PAGE_SIZE, NULL);
	ERR_CHECK_FLASH(rc,(FLASH_WRITE_ERROR|EX_FLASH_FILE_ID),__LINE__);
	
	do{
		__NOP();
	}	while((nrf_fstorage_is_busy(p_fstorage)));
	
	rc = nrf_fstorage_uninit(p_fstorage, NULL);
	ERR_CHECK_FLASH(rc,(FLASH_UNINIT_ERROR|EX_FLASH_FILE_ID),__LINE__);
	return res;
}

/**
 * @brief flash daily log read
 * @param sid SID
 * @param data Daily Log Data
 * @param set_case 書き込むページアドレス指定
 * @retval None
 */
void FlashRead(int32_t sid, Daily_t * data, int8_t set_case)
{ 
	uint32_t page_addr;
	ret_code_t rc;
	nrf_fstorage_t *p_fstorage;
	nrf_fstorage_api_t * p_fs_api;
	p_fs_api = &nrf_fstorage_sd;
	
	switch(set_case)
	{
		case 0:
			p_fstorage = &ex_fstorage;
			page_addr = DAILY_ADDR1;
			break;
		case 1:
			p_fstorage = &ex_fstorage2;
			page_addr = DAILY_ADDR2;
			break;
		default:
			break;	
	}
	DEBUG_LOG(LOG_DEBUG,"page addr 0x%x",page_addr);
	
	rc = nrf_fstorage_init(p_fstorage, p_fs_api, NULL);
	ERR_CHECK_FLASH(rc,(FLASH_INIT_ERROR|EX_FLASH_FILE_ID),__LINE__);

	rc = nrf_fstorage_read(p_fstorage, page_addr + (sid * 16) + 4, data, sizeof(Daily_t));
	ERR_CHECK_FLASH(rc,(FLASH_READ_ERROR|EX_FLASH_FILE_ID),__LINE__);

	rc = nrf_fstorage_uninit(p_fstorage, NULL);
	ERR_CHECK_FLASH(rc,(FLASH_UNINIT_ERROR|EX_FLASH_FILE_ID),__LINE__);
}

/**
 * @brief flash daily log Clear
 * @param page クリアするページ番号
 * @retval None
 */
void FlashDailyLogPageClear(int8_t page)
{
	volatile int8_t vol_flg_state;
	uint32_t start_addr;
	ret_code_t rc;
	volatile bool bret;
	nrf_fstorage_t *p_fstorage;
	nrf_fstorage_api_t *p_fs_api;
	
	p_fs_api = &nrf_fstorage_sd;
	if( (page < 1) && (2 < page))
	{
		DEBUG_LOG(LOG_ERROR,"Daily Log Erase Page Invalid. 0x%x", page);
		return;
	}
	
	switch(page)
	{
		case 1:
			p_fstorage = &ex_fstorage;
			start_addr = DAILY_ADDR1;
			break;
		case 2:
			p_fstorage = &ex_fstorage2;
			start_addr = DAILY_ADDR2;
			break;
		default:
		break;
	}
	rc = nrf_fstorage_init(p_fstorage, p_fs_api, NULL);
	ERR_CHECK_FLASH(rc,(FLASH_INIT_ERROR|EX_FLASH_FILE_ID),__LINE__);
	
	/* Erase the DATA_STORAGE_PAGE before write operation */
	rc = nrf_fstorage_erase(p_fstorage, start_addr, 1, NULL);
	ERR_CHECK_FLASH(rc,(FLASH_ERASE_ERROR|EX_FLASH_FILE_ID),__LINE__);
	
	do{
		bret = nrf_fstorage_is_busy(p_fstorage);
	}	while(bret);
	
	rc = nrf_fstorage_uninit(p_fstorage, NULL);
	ERR_CHECK_FLASH(rc,(FLASH_UNINIT_ERROR|EX_FLASH_FILE_ID),__LINE__);
}

/**
 * @brief flash daily log data check
 * @param flag 読み出すページアドレス指定
 * @retval res
 *			- 1 : Success
 *			- 0 : Failed
 */
int8_t CheckFlash(int8_t flg)
{
	int8_t res = 1;
	uint8_t rdata[2];
	uint32_t start_addr;
	ret_code_t rc;
	nrf_fstorage_t *p_fstorage;
	nrf_fstorage_api_t * p_fs_api;
	volatile uint16_t checkdata;

	p_fs_api = &nrf_fstorage_sd;
	
	if( (flg < 0) && (1 < flg))
	{
		return 0;
	}
	
	switch(flg)
	{
		case 0:
			p_fstorage = &ex_fstorage;
			start_addr = DAILY_ADDR1;
			break;
		case 1:
			p_fstorage = &ex_fstorage2;
			start_addr = DAILY_ADDR2;
			break;
		default:
			break;
	}
	rc = nrf_fstorage_init(p_fstorage, p_fs_api, NULL);
	ERR_CHECK_FLASH(rc,(FLASH_INIT_ERROR|EX_FLASH_FILE_ID),__LINE__);
	
	rc = nrf_fstorage_read(p_fstorage, start_addr, &rdata, 2);
	ERR_CHECK_FLASH(rc,(FLASH_READ_ERROR|EX_FLASH_FILE_ID),__LINE__);
	
	checkdata = ((uint32_t)rdata[0] << 8) | (uint32_t)rdata[1] ;
	if(checkdata != CHECK_DATA)
	{
		res = 0;
	}
	
	rc = nrf_fstorage_uninit(p_fstorage, NULL);
	ERR_CHECK_FLASH(rc,(FLASH_UNINIT_ERROR|EX_FLASH_FILE_ID),__LINE__);
	
	return res;
}

/**
 * @brief Pairing Flash Check
 * @param data check pairing data
 * @retval res
 *			- PIN_MATCH : Match
 *			- PIN_NOT_MATCH : Not Match
 *			- PIN_NOT_EXIST : Not Exist
 */
int8_t CheckPairing(uint8_t *data)
{
	int8_t res;
	uint8_t page_data[PAIRING_CODE_SIZE];
	uint8_t rdata[PAIRING_DATA_SIZE];
	uint16_t write_flg;
	
	ret_code_t rc;
	nrf_fstorage_t *p_fstorage;
	nrf_fstorage_api_t * p_fs_api;
	uint32_t start_addr = PLAYER_AND_PAIRING_ADDR;

	p_fs_api = &nrf_fstorage_sd;
	p_fstorage = &player_pincode_fstorage;
	
	rc = nrf_fstorage_init(p_fstorage, p_fs_api, NULL);
	ERR_CHECK_FLASH(rc,(FLASH_INIT_ERROR|EX_FLASH_FILE_ID),__LINE__);
	
	rc = nrf_fstorage_read(p_fstorage, start_addr, &rdata, PAIRING_DATA_SIZE);
	ERR_CHECK_FLASH(rc,(FLASH_READ_ERROR|EX_FLASH_FILE_ID),__LINE__);
	DEBUG_LOG(LOG_DEBUG,"read Raw flag 1. 0x%x, 2. 0x%x",rdata[0],rdata[1]);
	
	//write_flg = ((uint16_t)rdata[0] << 8 | (uint16_t)rdata[1]);
	memcpy(&write_flg, &rdata[0],sizeof(write_flg));
	DEBUG_LOG(LOG_DEBUG,"read flag 0x%x, check 0x%x",write_flg,CHECK_DATA);
	
	if(write_flg == CHECK_DATA)
	{
		memcpy(&page_data[0],&rdata[FLASH_PARING_START_POS], sizeof(page_data));
		//future change.
		DEBUG_LOG(LOG_INFO,"FLASH SAVE  %x:%x:%x:%x:%x:%x",page_data[0],page_data[1],page_data[2],page_data[3],page_data[4],page_data[5]);
		DEBUG_LOG(LOG_INFO,"SMART PHONE %x:%x:%x:%x:%x:%x",data[0],data[1],data[2],data[3],data[4],data[5]);
		if(memcmp(data, page_data, sizeof(page_data)) == 0)
		{
			res = PIN_MATCH;
		}
		else
		{
			res = PIN_NOT_MATCH;
		}
	}
	else
	{
		res = PIN_NOT_EXIST;
	}
	
	rc = nrf_fstorage_uninit(p_fstorage, NULL);
	ERR_CHECK_FLASH(rc,(FLASH_UNINIT_ERROR|EX_FLASH_FILE_ID),__LINE__);
	
	return res;
}

/**
 * @brief Get Latest Sid from Flash 
 * @param write_data write data
 * @param over_data over data
 * @retval ret 最新のSID
 */
uint16_t GetLastSid(int8_t *write_data, uint8_t *over_data)
{
	uint16_t ret; 
	uint16_t save_sid;
	int8_t check_ret;
	int8_t end_flg;
	nrf_fstorage_t 	*p_fstorage;
	
	ret     = 0;
	end_flg = 0;
	
	p_fstorage = &ex_fstorage;
	check_ret = check_sid(p_fstorage, &ret);
	
	*write_data = true;
	if(check_ret == 0)
	{
		ret = 0;
		end_flg = 1;
		*write_data = false;
		*over_data = 0;
	}
	else if(check_ret == 1)
	{
		save_sid = ret;
	}
	else
	{
		end_flg = 1;
		*over_data = read_byte_data(p_fstorage, DAILY_ADDR1);
	}
	
	//page2
	if(end_flg == 0)
	{
		p_fstorage = &ex_fstorage2;
		check_ret = check_sid(p_fstorage, &ret);
		
		if(check_ret == 0)
		{
			ret = save_sid;
			end_flg = 1;
			*over_data = read_byte_data(p_fstorage, DAILY_ADDR1);
		}
		else if(check_ret == 1)
		{
			if(save_sid + MAX_PAGE_DATA != ret)
			{
				ret = save_sid;
				end_flg = 1;
				*over_data = read_byte_data(p_fstorage, DAILY_ADDR1);
			}
			else
			{
				end_flg = 1;
				*over_data = read_byte_data(p_fstorage, DAILY_ADDR2);
			}

		}
		else
		{
			end_flg = 1;
			*over_data = read_byte_data(p_fstorage, DAILY_ADDR2);
		}
	}
	return ret;
}

/**
 * @brief Check and Get Player data
 * @param age Flashから読み出した年齢
 * @param player Flashから読み出した性別
 * @param weight Flashから読み出した体重
 * @retval ret 最新のSID
 */
bool CheckPlayer(uint8_t *age, uint8_t *player, uint8_t *weight)
{
	uint32_t start_addr;
	ret_code_t rc;
	uint16_t check_data;
	uint8_t page_data[5];
	bool res;
	
	res = true;
	start_addr = (PLAYER_AND_PAIRING_ADDR + PLAYER_CHECK_DATA_POS);
	nrf_fstorage_t *p_fstorage;
	nrf_fstorage_api_t * p_fs_api;
	p_fs_api = &nrf_fstorage_sd;

	p_fstorage = &player_pincode_fstorage;
	
	rc = nrf_fstorage_init(p_fstorage, p_fs_api, NULL);
	ERR_CHECK_FLASH(rc,(FLASH_INIT_ERROR|EX_FLASH_FILE_ID),__LINE__);
	
	rc = nrf_fstorage_read(p_fstorage, start_addr, page_data, sizeof(page_data));
	ERR_CHECK_FLASH(rc,(FLASH_READ_ERROR|EX_FLASH_FILE_ID),__LINE__);
	check_data = ((uint16_t)page_data[0] << 8 | (uint16_t)page_data[1]);
	if(check_data == CHECK_DATA)
	{
		*age    = page_data[2];
		*player = page_data[3];
		*weight = page_data[4];
	}
	else
	{
		res = false;
	}
	
	return res;
}

/**
 * @brief Get Latest Date from Flash 
 * @param date Timer情報を格納する変数へのポインタ
 * @param get_sid 読み出すSID
 * @retval 0 failed
 * @retval 1 success
 */
uint8_t GetLastDate(struct tm *date, uint16_t get_sid)
{
	uint32_t rdata;	
	uint32_t address;
	volatile uint32_t year_month;
	volatile uint32_t day_time;
	uint16_t page;
	uint16_t num;
	uint16_t check_sid = 0;
	int8_t ret = 0;
	nrf_fstorage_t *p_fstorage;
	
	volatile uint32_t year,month,day,time;

	page = (get_sid % 256) / MAX_PAGE_DATA;
	num = get_sid % MAX_PAGE_DATA;
	
	if(get_sid == 0) {
		p_fstorage = &ex_fstorage;
		rdata = read_word_data(p_fstorage, DAILY_ADDR1);
		if((rdata >> 16) != CHECK_DATA)
		{
			ret = 0;
			check_sid = 1;
		}
	}
	
	if(check_sid == 0)
	{
		if(page == 1)
		{
			p_fstorage = &ex_fstorage2;
			address = DAILY_ADDR2;
		}
		else
		{
			p_fstorage = &ex_fstorage;
			address = DAILY_ADDR1;
		}
			
		year_month = read_word_data(p_fstorage, address + 4 + (16 * num));
		DEBUG_LOG(LOG_INFO,"page 0x%x. num 0x%x. sid 0x%x, check_sid 0x%x",address,num,get_sid,check_sid);
		day_time = read_word_data(p_fstorage,address + 4 + (16 * num) + 4);
		year = ((year_month&0xFF000000) >> 24);
		year = year * 10 + ((year_month&0x00FF0000) >> 16);
		
		month = ((year_month&0x0000FF00) >> 8);
		month = month * 10 + (year_month&0x000000FF);
		
		day = ((day_time&0xFF000000) >> 24);
		day = day * 10 + ((day_time&0x00FF0000) >> 16);
		
		time = ((day_time&0x0000FF00) >> 8);
		time = time * 10 + (day_time&0x000000FF);
		
		date->tm_year = year;
		date->tm_mon = month;
		date->tm_mday = day;
		date->tm_hour = time;
		date->tm_min = 0;
		date->tm_sec = 0;
		
		ret = 1;
	}
		
	return ret;
}

/**
 * @brief Error Check Flash
 * @param rc error code
 * @param traceID Trace ID
 * @param line Execution LINE
 * @retval None
 */
void ErrCheckFlash(ret_code_t rc, uint16_t traceID, uint16_t line)
{
	if(rc != NRF_SUCCESS)
	{
		TRACE_LOG(TR_FLASH_OP_ERROR,traceID);
		SetBleErrReadCmd(UTC_BLE_FLASH_WRITE_ERROR);
		DEBUG_LOG(LOG_ERROR,"flash err 0x%x, trace ID 0x%x",rc,traceID);
	}
}

/**
 * @brief check sid and get sid
 * @param p_fstorage  fstorage instance
 * @param *sid SIDを格納する変数へのポインタ
 * @retval ret
 *			- 0 : SIDなし
 *			- 1 : End SID
 *			- 2 : Start SID
 */
static int8_t check_sid(nrf_fstorage_t *p_fstorage, uint16_t* sid)
{
	int8_t ret = 0;
	uint8_t rdata[2];
	uint8_t e_data[4];
	uint8_t f_data[4];
	ret_code_t rc;
	nrf_fstorage_api_t *p_fs_api;
	uint32_t start_addr;
	volatile uint16_t s_sid,e_sid;
	volatile uint16_t checkdata;
	volatile uint8_t x;
	
	p_fs_api = &nrf_fstorage_sd;
	start_addr = p_fstorage->start_addr;
	
	rc = nrf_fstorage_init(p_fstorage, p_fs_api, NULL);
	ERR_CHECK_FLASH(rc,(FLASH_INIT_ERROR|EX_FLASH_FILE_ID),__LINE__);
	
	rc = nrf_fstorage_read(p_fstorage, start_addr, &rdata, 2);
	ERR_CHECK_FLASH(rc,(FLASH_READ_ERROR|EX_FLASH_FILE_ID),__LINE__);
	
	checkdata = ((uint32_t)rdata[0] << 8) | (uint32_t)rdata[1] ;
	if(checkdata != CHECK_DATA)
	{
		DEBUG_LOG(LOG_DEBUG, "CHECK DATA Invalid.");
		ret = 0;
	}
	else
	{
		rc = nrf_fstorage_read(p_fstorage, (start_addr + PAGE_FARST_SID_ADD), &f_data, 4);
		ERR_CHECK_FLASH(rc,(FLASH_READ_ERROR|EX_FLASH_FILE_ID),__LINE__);
		
		rc = nrf_fstorage_read(p_fstorage, (start_addr + PAGE_FINISH_SID_ADD), &e_data, 4);
		ERR_CHECK_FLASH(rc,(FLASH_READ_ERROR|EX_FLASH_FILE_ID),__LINE__);
		
		s_sid = (f_data[3] << 8) | f_data[2];
		e_sid = (e_data[3] << 8) | e_data[2];

		if( (s_sid + 127) == e_sid)
		{
			*sid = (uint16_t)e_sid;
			ret = 1;
		}
		else
		{
			*sid = (uint16_t)e_sid;
			ret = 1;
			/*2022.7.22 Flash内の歩数ログのチェック数が1つ足りないためMAX＿PAGE＿DATAに+1する. kono */
			for(x = 2; x < (MAX_PAGE_DATA+1); x++)
			{
				rc = nrf_fstorage_read(p_fstorage, (start_addr + (x * 16)), &e_data, 4);
				ERR_CHECK_FLASH(rc,(FLASH_READ_ERROR|EX_FLASH_FILE_ID),__LINE__);
				e_sid = (e_data[3] << 8) | e_data[2];

				if(s_sid + 1 == e_sid)
				{
					s_sid = e_sid;
				}
				else
				{
					if((s_sid == MAX_BLE_SID) && (e_sid == 0))
					{
						s_sid = e_sid;
					}
					else
					{
						*sid = (uint16_t)s_sid;
						ret = 2;
						//x = MAX_PAGE_DATA + 1;
						break;
					}
				}
			}
		}
	}

	rc = nrf_fstorage_uninit(p_fstorage, NULL);
	ERR_CHECK_FLASH(rc,(FLASH_UNINIT_ERROR|EX_FLASH_FILE_ID),__LINE__);
	return ret;
}

/**
 * @brief read word data
 * @param p_fstorage  fstorage instance
 * @param addr 読み出すAddress
 * @retval data 読み出したデータ
 */
static uint32_t read_word_data(nrf_fstorage_t *p_fstorage, uint32_t addr)
{
	ret_code_t rc;
	nrf_fstorage_api_t * p_fs_api;
	p_fs_api = &nrf_fstorage_sd;
	uint8_t  cData[4];
	uint32_t data;
	
	rc = nrf_fstorage_init(p_fstorage, p_fs_api, NULL);
	ERR_CHECK_FLASH(rc,(FLASH_INIT_ERROR|EX_FLASH_FILE_ID),__LINE__);
	
	rc = nrf_fstorage_read(p_fstorage, addr, cData, sizeof(cData));
	ERR_CHECK_FLASH(rc,(FLASH_READ_ERROR|EX_FLASH_FILE_ID),__LINE__);
	rc = nrf_fstorage_uninit(p_fstorage, NULL);
	ERR_CHECK_FLASH(rc,(FLASH_UNINIT_ERROR|EX_FLASH_FILE_ID),__LINE__);
	data = (((uint32_t)cData[0] << 24) | ((uint32_t)cData[1] << 16) | ((uint32_t)cData[2] << 8) | ((uint32_t)cData[3]));
	
	return data;
}

/**
 * @brief read byte data
 * @param p_fstorage  fstorage instance
 * @param addr 読み出すAddress
 * @retval data 読み出したデータ
 */
static uint8_t read_byte_data(nrf_fstorage_t *p_fstorage, uint32_t addr)
{
	ret_code_t rc;
	nrf_fstorage_api_t * p_fs_api;
	p_fs_api = &nrf_fstorage_sd;
	uint8_t data[4];
	
	rc = nrf_fstorage_init(p_fstorage, p_fs_api, NULL);
	ERR_CHECK_FLASH(rc,(FLASH_INIT_ERROR|EX_FLASH_FILE_ID),__LINE__);
	
	rc = nrf_fstorage_read(p_fstorage, addr, &data, 4);
	ERR_CHECK_FLASH(rc,(FLASH_READ_ERROR|EX_FLASH_FILE_ID),__LINE__);
	rc = nrf_fstorage_uninit(p_fstorage, NULL);
	ERR_CHECK_FLASH(rc,(FLASH_UNINIT_ERROR|EX_FLASH_FILE_ID),__LINE__);
	
	return data[2];
}


