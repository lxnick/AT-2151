/**
  ******************************************************************************************
  * @file    flash_operation.h
  * @author  k.tashiro
  * @version 1.0
  * @date    2020/09/17
  * @brief   Flash Function Operation
  ******************************************************************************************
    Revision:
    Ver            Date             Revised by        Explanation
  ******************************************************************************************
  * 1.0            2020/09/17       k.tashiro         create new
  ******************************************************************************************
*/

#ifndef FLASH_OPERATION_H_
#define FLASH_OPERATION_H_

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
#include "lib_flash.h"
#include "lib_fifo.h"
#include "state_control.h"
#include "ble_manager.h"
#include "mode_manager.h"
#include "lib_ram_retain.h"
#include "ble_definition.h"
#include "definition.h"
#include "lib_timer.h"

#include "daily_log.h"
#include "state_control.h"
#include "definition.h"
#include "lib_ex_rtc.h"
#include "lib_trace_log.h"


/* Definition ------------------------------------------------------------*/
#define FLASH_OP_MASK         0x40
#define FLASH_OP_MASK_OFFSET (0x40 - EVT_FLASH_OP_STATUS_CHECK)

/* Enum ------------------------------------------------------------------*/
//flash operation id
typedef enum
{
	FLASH_RTC_INT = 0x00,
	FLASH_INIT_CMD,			//0x01
	FLASH_SET_PLAYER, 		//0x02
	FLASH_SET_TIME,			//0x03
	FLASH_ACCESS_PINCODE,	//0x04
	FLASH_CHECK_PINCODE,	//0x05
	FLASH_ERASE_PINCODE,	//0x06
	//New ID
	FLASH_READ_DAILY,		//0x07
	FLASH_READ_DAILY_ONE,	//0x08
	FLASH_OP_IDLE,			//0x09
	FLASH_OP_ID_END_POINT	//0x0a
}FLASH_OPID;

//flash operation sequence id
typedef enum
{
	INIT_CMD_START				= 0x00,
	INIT_CMD_WAIT_ER_PLAYER,				//0x01 
	INIT_CMD_WAIT_WR_PLAYER, 				//0x02
	INIT_CMD_WAIT_ER_DAILY_LOG,				//0x03
	INIT_CMD_WAIT_WR_DAILY_LOG,				//0x04
	INIT_CMD_WAIT_ER_FLASH,					//0x05
	INIT_CMD_WAIT_ER_PIN_CODE,				//0x06 
	INIT_CMD_WAIT_WR_PIN_CODE,				//0x07
	INIT_CMD_WAIT_SLAVE_LATENCY_ZERO,		//0x08
	INIT_CMD_CMPL_SLAVE_LATENCY_ZERO,		//0x09
	INIT_CMD_WAIT_SLAVE_LATENCY_ONE,		//0x0a
	INIT_CMD_CMPL_SLAVE_LATENCY_ONE,		//0x0b
	INIT_CMD_PINCODE_CHECK_MATCH,
	INIT_CMD_PINCODE_CHECK_NOMATCH,
	INIT_CMD_PINCODE_ACCESS_NG,
	
	INIT_CMP_END_POINT
}FLASH_OP_STATE;

typedef enum
{
	//flash operation event
	FLASH_EVT_OP_STATUS_CHECK = 0x00,
	FLASH_EVT_INIT_CMD_START,
	FLASH_EVT_FLASH_DATA_WRITE_CMPL,
	FLASH_EVT_FLASH_DATA_ERASE_CMPL,	//Not change state
	
	FLASH_EVT_END_POINT
}FLASH_OP_EVT;

/* Struct ----------------------------------------------------------------*/
//flash operation state struct
typedef struct _flash_op
{
	uint32_t		(*exefunc[FLASH_EVT_END_POINT])(PEVT_ST);
	FLASH_OPID		op_id;
	uint8_t			seq_id;
	uint32_t		start_addr;
	nrf_fstorage_t	*p_fstorage;
	uint8_t			op[INIT_CMP_END_POINT];
	DATE_TIME		time;
}FLASH_OP, *PFLASH_OP;

//flash operation state struct register
typedef struct _flash_op_reg
{
	uint32_t			(*const exefunc[FLASH_EVT_END_POINT])(PEVT_ST);
	const FLASH_OPID	op_id;
	uint8_t				seq_id;
	uint32_t			start_addr;
	nrf_fstorage_t		*p_fstorage;
	uint8_t				op[INIT_CMP_END_POINT];
	DATE_TIME			time;
}FLASH_OP_REG, *PFLASH_OP_REG;

/* Function prototypes ----------------------------------------------------*/
/**
 * @brief Flash Operation Initialize
 * @param None
 * @retval None
 */
//void utc_flash_op_init(void);
void FlashOpInit(void);

/**
 * @brief Flash Operation Entry Point(State Controlから読み出され処理を実行する)
 * @param pEvent Event Information
 * @retval 0 Success
 */
//uint32_t utc_flash_Wrapperfunc(PEVT_ST pEvent);
uint32_t FlashWrapperfunc(PEVT_ST pEvent);

/**
 * @brief main state cnt, event flash operation all event.
 * @param pEvent Event Information
 * @retval 0 Success
 */
//uint32_t utc_flash_op_id_check(PEVT_ST pEvent);
uint32_t FlashOpIdCheck(PEVT_ST pEvent);

/**
 * @brief Flash Operation Force Initialize
 * @param None
 * @retval None
 */
//void flashOp_force_init(void);
void FlashOpForceInit(void);

/**
 * @brief main state cnt, event flash operation slave latency zero check
 * @param pEvent Event Information
 * @retval 0 Success
 */
//uint32_t utc_slave_latency_zero_check(PEVT_ST pEvent);
uint32_t FlashOpSlaveLatencyZeroCheck(PEVT_ST pEvent);

/**
 * @brief Get Flash Operation Mode
 * @param p_op_mode 現在のoperation modeを返す
 * @retval None
 */
//void getFlashOperatoinMode(uint8_t *pOp_mode);
void GetFlashOperatoinMode(uint8_t *p_op_mode);

/**
 * @brief Flash Timeout event handler
 * @param None
 * @retval None
 */
//void init_timeout_evt_handler(void);
void FlashTimeoutEvtHandler(void);

#endif
