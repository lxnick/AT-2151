/**
  ******************************************************************************************
  * @file    lib_trace_log.h
  * @author  k.tashiro
  * @version 1.0
  * @date    2020/09/10
  * @brief   Trace Log
  ******************************************************************************************
    Revision:
    Ver            Date             Revised by        Explanation
  ******************************************************************************************
  * 1.0            2020/09/10       k.tashiro         create new
  ******************************************************************************************
*/

#ifndef LIB_TRACE_LOG_H_
#define LIB_TRACE_LOG_H_

/* Includes --------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
#include "lib_common.h"

/* Definition ------------------------------------------------------------*/
#define DATE_SIZE					(6)		/* Date Size */
#define SIGNITURE_SIZE				(2)		/* Signiture Size */
#define TRACE_LOG_SIZE				(32)	/* Trace Log Size */
#define TRACE_LOG_MAX_ID			(255)	/* Trace ID 1-255 */
#define TRACE_LOG_MAX_ENABLE		(1)		/* Size Max Flag */

#define TRACE_LOG_BASE_ADDR			(0x2000FE00U)								/* RAM Address */
#define TRACE_LOG_START_ADDR		TRACE_ADDR									/* Start Address */
#define TRACE_LOG_END_ADDR			TRACE_LOG_START_ADDR + TEST_PAGE_SIZE		/* End Address */

#define TRACE_LOG_BUFFER_SIZE		(308)	/* MAX Buffer Size */
#define RESET_REASON_SIZE			(104)	/* Reset Reason Size */
#define RESET_REASON_DATA_SIZE		(10)	/* Reset Reason Data Size */

#define TRACE_DATA_INITIAL_VAL		(0xFF)

#define TRACE_DATA_SIG_PRIMARY		(0xDD)
#define TRACE_DATA_SIG_SECONDARY	(0xAC)

/* Trace Data Parse Mask */
#define TRACE_DATA_ROM1_MASK		(0xFF00U)
#define TRACE_DATA_ROM2_MASK		(0x00FFU)
#define RREASON_DATA_MASK1			(0xFF000000U)
#define RREASON_DATA_MASK2			(0x00FF0000U)
#define RREASON_DATA_MASK3			(0x0000FF00U)
#define RREASON_DATA_MASK4			(0x000000FFU)

#define TRACE_LOG( idx, param )		TraceLog( idx, param )
#define TRACE_LOG_FLUSH				TraceFlush

#define VALIDETE_RETCODE(statement, trace_id)           \
do                                                      \
{                                                       \
    uint32_t _err_code = (uint32_t) (statement);        \
    if (_err_code != NRF_SUCCESS)                       \
    {                                                   \
        DEBUG_LOG(LOG_ERROR,"[TraceLog] Flush Err=0x%x, TraceID=0x%x", _err_code, trace_id); \
        return _err_code;                               \
    }                                                   \
} while(0)

/* Struct ----------------------------------------------------------------*/
typedef struct _trace_log_info {
	uint8_t trace_id;				/* Trace ID */
	uint8_t nested_interrupt;		/* Nested Interrupt */
	uint16_t func_no;				/* Function No: 2byte */
	uint16_t param;					/* Param: 2byte */
} TRACE_LOG_INFO;

/* Trace Data Total: 204byte [2byte + 4byte + 6byte + (6 * 32)byte] */
typedef struct _trace_data {
	uint8_t signiture[SIGNITURE_SIZE];				/* 2byte */
	uint8_t trace_log_sig[SIGNITURE_SIZE];			/* 2byte */
	uint8_t trace_log_count;						/* 1byte */
	uint8_t trace_log_id;							/* 1byte */
	TRACE_LOG_INFO trace_data[TRACE_LOG_SIZE];		/* 6byte * 32: 192byte */
} TRACE_DATA;

/* Trace Data Information */
typedef struct _trace_data_info {
	uint32_t trace_index;
} TRACE_DATA_INFO;


/* Function prototypes -------------------------------------------*/
/**
 * @brief Trace Log Initialize
 * @param None
 * @retval None
 */
void TraceLogInit( void );

/**
 * @brief Trace Log Save
 * @param func_no Function Number
 * @param param Parameter
 * @retval None
 */
void TraceLog( uint16_t func_no, uint16_t param );

/**
 * @brief Trace Log Flush
 * @remark FlashへReset Reasonを書き込む
 * @param reason Reset Reason
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 エラー
 */
ret_code_t TraceFlush( void );

/**
 * @brief Set Reset Reason
 * @remark FlashへReset Reasonを書き込む
 * @param reason Reset Reason
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 エラー
 */
ret_code_t SetResetReason( uint32_t reason );


#endif
