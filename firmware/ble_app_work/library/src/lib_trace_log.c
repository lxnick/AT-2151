/**
  ******************************************************************************************
  * @file    lib_trace_log.c
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

/* Includes --------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#include "nrf_fstorage.h"
#include "nrf_fstorage_sd.h"

#include "lib_common.h"
#include "definition.h"
#include "lib_ex_rtc.h"
#include "lib_trace_log.h"

/* Private variables -----------------------------------------------------*/
TRACE_DATA * g_trace_data;
TRACE_DATA_INFO g_trace_data_info = {0};
volatile uint8_t g_trace_log_buffer[TRACE_LOG_BUFFER_SIZE];

/* Private function prototypes -----------------------------------------------*/
/**
 * @brief Set Trace Log Buffer
 * @param func_no Function Number
 * @param param parameter
 * @param nested_int Interrupt
 * @retval None
 */
static void set_trace_log_buffer( uint16_t func_no, uint16_t param, uint8_t nested_int );

/**
 * @brief Setup Trace Log
 * @param date_info Date Information
 * @retval None
 */
static void setup_trace_log( DATE_TIME *date_info );

/**
 * @brief Setup Reset Reason Write Data
 * @param reason Reset Reason
 * @retval None
 */
static void setup_reason_write_data( uint32_t reason );

/**
 * @brief fstorage Event Handler
 * @param p_evt fstorage event context
 * @retval None
 */
static void fstorage_evt_handler( nrf_fstorage_evt_t * p_evt );

/**
 * @brief Trace Log Flash Area Erase
 * @param p_fstorage fds instance
 * @param page_addr erase page address
 * @param len erase page
 * @param p_context context
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 Failed
 */
static ret_code_t trace_log_flush_erase( nrf_fstorage_t *p_fstorage, uint32_t page_addr, uint32_t len, void *p_context );

/**
 * @brief Flash Read
 * @param p_fstorage fds instance
 * @param page_addr page address
 * @param len 読み出す長さ
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 Failed
 */
static ret_code_t trace_log_flush_read( nrf_fstorage_t *p_fstorage, uint32_t page_addr, uint32_t len );

/**
 * @brief Flash Write
 * @param p_fstorage fds instance
 * @param page_addr page address
 * @param buffer write buffer
 * @param len write buffer size
 * @param p_context context
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 Failed
 */
static ret_code_t trace_log_flush_write( nrf_fstorage_t *p_fstorage, uint32_t page_addr, uint8_t *buffer, uint32_t len, void *p_context );
	

NRF_FSTORAGE_DEF(nrf_fstorage_t trace_log_ex_fstorage) =
{
	/* Set a handler for fstorage events. */
	.evt_handler = fstorage_evt_handler,

	/* These below are the boundaries of the flash space assigned to this instance of fstorage.
	 * You must set these manually, even at runtime, before nrf_fstorage_init() is called.
	 * The function nrf5_flash_end_addr_get() can be used to retrieve the last address on the
	 * last page of flash available to write data. */
	.start_addr = TRACE_LOG_START_ADDR,
	.end_addr   = TRACE_LOG_END_ADDR,
};


/**
 * @brief Trace Log Initialize
 * @param None
 * @retval None
 */
void TraceLogInit( void )
{
	g_trace_data = (TRACE_DATA *)TRACE_LOG_BASE_ADDR;

	g_trace_data->signiture[0] = SIGN_DATA_ONE;
	g_trace_data->signiture[1] = SIGN_DATA_TWO;
	
	if ( ( g_trace_data->trace_log_sig[0] != TRACE_DATA_SIG_PRIMARY ) &&
		( g_trace_data->trace_log_sig[1] != TRACE_DATA_SIG_SECONDARY ) )
	{
		g_trace_data->trace_log_sig[0] = TRACE_DATA_SIG_PRIMARY;
		g_trace_data->trace_log_sig[1] = TRACE_DATA_SIG_SECONDARY;
		g_trace_data->trace_log_count = 0;
		g_trace_data->trace_log_id = 0;
		g_trace_data_info.trace_index = 0;
	}
	else
	{
		g_trace_data_info.trace_index = g_trace_data->trace_log_count;
	}
}

/**
 * @brief Trace Log Save
 * @param func_no Function Number
 * @param param Parameter
 * @retval None
 */
void TraceLog( uint16_t func_no, uint16_t param )
{
	/* Set Trace Log */
	ret_code_t err_code;
	uint8_t trace_log_nested_critical;

	/* Enter Critival Session */
	err_code = sd_nvic_critical_region_enter( &trace_log_nested_critical );
	if ( err_code != NRF_SUCCESS ){ }
	
	/* Set Trace Log Buffer */
	set_trace_log_buffer( func_no, param, trace_log_nested_critical );

	trace_log_nested_critical = 0;
	/* Exit Critival Session */
	err_code = sd_nvic_critical_region_exit( trace_log_nested_critical );
	if ( err_code != NRF_SUCCESS ){ }

	return ;
}

/**
 * @brief Trace Log Flush
 * @param None
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 エラー
 */
ret_code_t TraceFlush( void )
{
	ret_code_t err_code;
	nrf_fstorage_api_t *p_fs_api;
	nrf_fstorage_t *p_fstorage;
	uint32_t start_addr;
	DATE_TIME date_info;
	
	/* Get Rtc Data */
	err_code = ExRtcGetDateTime(&date_info);
	if ( err_code != NRF_SUCCESS )
	{
		return err_code;
	}

	/* Flush Config */
	p_fs_api = &nrf_fstorage_sd;
	start_addr = TRACE_LOG_START_ADDR;
	p_fstorage = &trace_log_ex_fstorage;
	
	/* fstorage Initialize */
	err_code = nrf_fstorage_init( p_fstorage, p_fs_api, NULL );
	VALIDETE_RETCODE(err_code, FLASH_INIT_ERROR);

	/* ROM Read */
	err_code = trace_log_flush_read( p_fstorage, start_addr, TRACE_LOG_BUFFER_SIZE );
	VALIDETE_RETCODE(err_code, FLASH_READ_ERROR);
	
	/* Setup Trace Log Data */
	setup_trace_log( &date_info );
	
	/* ROM erase */
	err_code = trace_log_flush_erase( p_fstorage, start_addr, 1, NULL );
	VALIDETE_RETCODE(err_code, FLASH_ERASE_ERROR);

	/* Write ROM */
	err_code = trace_log_flush_write( p_fstorage, start_addr, (uint8_t *)&g_trace_log_buffer, TRACE_LOG_BUFFER_SIZE, NULL );
	VALIDETE_RETCODE(err_code, FLASH_WRITE_ERROR);

	/* fstorage Uninitialize */
	err_code = nrf_fstorage_uninit(p_fstorage, NULL);
	VALIDETE_RETCODE(err_code, FLASH_UNINIT_ERROR);
	
	return NRF_SUCCESS;
}

/**
 * @brief Set Reset Reason
 * @remark FlashへReset Reasonを書き込む
 * @param reason Reset Reason
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 エラー
 */
ret_code_t SetResetReason( uint32_t reason )
{
	ret_code_t err_code;
	nrf_fstorage_api_t *p_fs_api;
	nrf_fstorage_t *p_fstorage;
	uint32_t start_addr;

	/* Flush Config */
	p_fs_api = &nrf_fstorage_sd;
	start_addr = TRACE_LOG_START_ADDR;
	p_fstorage = &trace_log_ex_fstorage;

	/* fstorage Initialize */
	err_code = nrf_fstorage_init( p_fstorage, p_fs_api, NULL );
	VALIDETE_RETCODE(err_code, FLASH_INIT_ERROR);

	/* ROM Read */
	err_code = trace_log_flush_read( p_fstorage, start_addr, TRACE_LOG_BUFFER_SIZE );
	VALIDETE_RETCODE(err_code, FLASH_READ_ERROR);

	/* Setup Reset Reason Write Data */
	setup_reason_write_data( reason );
	
	/* ROM erase */
	err_code = trace_log_flush_erase( p_fstorage, start_addr, 1, NULL );
	VALIDETE_RETCODE(err_code, FLASH_ERASE_ERROR);

	/* ROM Write */
	err_code = trace_log_flush_write( p_fstorage, start_addr, (uint8_t *)&g_trace_log_buffer, TRACE_LOG_BUFFER_SIZE, NULL );
	VALIDETE_RETCODE(err_code, FLASH_WRITE_ERROR);

	/* fstorage Uninitialize */
	err_code = nrf_fstorage_uninit(p_fstorage, NULL);
	VALIDETE_RETCODE(err_code, FLASH_UNINIT_ERROR);
	
	return NRF_SUCCESS;
}

/**
 * @brief Set Trace Log Buffer
 * @param func_no Function Number
 * @param param parameter
 * @param nested_int Interrupt
 * @retval None
 */
static void set_trace_log_buffer( uint16_t func_no, uint16_t param, uint8_t nested_int )
{
	uint32_t idx;

	/* Set RAM Trace Log */
	idx = g_trace_data_info.trace_index % TRACE_LOG_SIZE;
	g_trace_data->trace_data[idx].trace_id = ((g_trace_data->trace_log_id++) % TRACE_LOG_MAX_ID) + 1;
	g_trace_data->trace_data[idx].func_no = func_no;
	g_trace_data->trace_data[idx].param = param;
	g_trace_data->trace_data[idx].nested_interrupt = nested_int;

	/* Delete oldest data */
	g_trace_data->trace_log_count = idx = (++g_trace_data_info.trace_index) % TRACE_LOG_SIZE;
	memset( &g_trace_data->trace_data[idx], 0, sizeof( TRACE_LOG_INFO ) );
	
	return ;
}

/**
 * @brief Setup Trace Log
 * @param date_info Date Information
 * @retval None
 */
static void setup_trace_log( DATE_TIME *date_info )
{
	uint32_t idx;
	uint32_t reset_reason_offset;
	
	if ( date_info != NULL )
	{
		/* Set signiture */
		g_trace_log_buffer[RESET_REASON_SIZE] = g_trace_data->signiture[0];
		g_trace_log_buffer[RESET_REASON_SIZE + 1] = g_trace_data->signiture[1];
		/* Set Data Counter Signiture */
		g_trace_log_buffer[RESET_REASON_SIZE + 2] = g_trace_data->trace_log_sig[0];
		g_trace_log_buffer[RESET_REASON_SIZE + 3] = g_trace_data->trace_log_sig[1];
		/* Set Data Counter */
		g_trace_log_buffer[RESET_REASON_SIZE + 4] = g_trace_data->trace_log_count;
		g_trace_log_buffer[RESET_REASON_SIZE + 5] = g_trace_data->trace_log_id;
		reset_reason_offset = RESET_REASON_SIZE + 6;
		
		/* Set Date */
		g_trace_log_buffer[reset_reason_offset++] = date_info->year;
		g_trace_log_buffer[reset_reason_offset++] = date_info->month;
		g_trace_log_buffer[reset_reason_offset++] = date_info->day;
		g_trace_log_buffer[reset_reason_offset++] = date_info->hour;
		g_trace_log_buffer[reset_reason_offset++] = date_info->min;
		g_trace_log_buffer[reset_reason_offset++] = date_info->sec;
		
		for ( idx = 0; idx < TRACE_LOG_SIZE; idx++ )
		{
			/* Set Trace Data */
			g_trace_log_buffer[reset_reason_offset++] = g_trace_data->trace_data[idx].trace_id;
			g_trace_log_buffer[reset_reason_offset++] = g_trace_data->trace_data[idx].nested_interrupt;
			g_trace_log_buffer[reset_reason_offset++] = ( g_trace_data->trace_data[idx].func_no & TRACE_DATA_ROM1_MASK ) >> 8;
			g_trace_log_buffer[reset_reason_offset++] = ( g_trace_data->trace_data[idx].func_no & TRACE_DATA_ROM2_MASK );
			g_trace_log_buffer[reset_reason_offset++] = ( g_trace_data->trace_data[idx].param & TRACE_DATA_ROM1_MASK ) >> 8;
			g_trace_log_buffer[reset_reason_offset++] = ( g_trace_data->trace_data[idx].param & TRACE_DATA_ROM2_MASK );
		}
	}
	return ;
}

/**
 * @brief Setup Reset Reason Write Data
 * @param reason Reset Reason
 * @retval None
 */
static void setup_reason_write_data( uint32_t reason )
{
	ret_code_t err_code;
	uint8_t index;
	uint32_t write_count;
	DATE_TIME date_info;

	/* Get Rtc Data */
	err_code = ExRtcGetDateTime(&date_info);
	if ( err_code != NRF_SUCCESS )
	{
		return ;
	}

	/* Get Reset Reason Index*/
	index = g_trace_log_buffer[2];
	if ( index == TRACE_DATA_INITIAL_VAL )
	{
		/* Initial Value */
		index = 0;
	}
	
	if ( index >= RESET_REASON_DATA_SIZE )
	{
		/* Reset Reason MAX count */
		index = 0;
	}
	write_count = (index * 10) + 4;
	
	/* Signiture Set */
	g_trace_log_buffer[0] = SIGN_DATA_ONE;
	g_trace_log_buffer[1] = SIGN_DATA_TWO;
	g_trace_log_buffer[2] = ++index;
	/* RTC Set */
	g_trace_log_buffer[write_count++] = date_info.year;
	g_trace_log_buffer[write_count++] = date_info.month;
	g_trace_log_buffer[write_count++] = date_info.day;
	g_trace_log_buffer[write_count++] = date_info.hour;
	g_trace_log_buffer[write_count++] = date_info.min;
	g_trace_log_buffer[write_count++] = date_info.sec;
	/* Set Reset Reason */
	g_trace_log_buffer[write_count++] = (reason & RREASON_DATA_MASK1) >> 24;
	g_trace_log_buffer[write_count++] = (reason & RREASON_DATA_MASK2) >> 16;
	g_trace_log_buffer[write_count++] = (reason & RREASON_DATA_MASK3) >> 8;
	g_trace_log_buffer[write_count++] = (reason & RREASON_DATA_MASK4);

	return ;
}

/**
 * @brief Trace Log Flash Area Erase
 * @param p_fstorage fds instance
 * @param page_addr erase page address
 * @param len erase page
 * @param p_context context
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 Failed
 */
static ret_code_t trace_log_flush_erase( nrf_fstorage_t *p_fstorage, uint32_t page_addr, uint32_t len, void *p_context )
{
	ret_code_t err_code;

	/* Erase the DATA_STORAGE_PAGE before write operation */
	err_code = nrf_fstorage_erase( p_fstorage, page_addr, len, p_context );
	if( err_code != NRF_SUCCESS )
	{
		nrf_fstorage_uninit(p_fstorage, NULL);
		return err_code;
	}

	/* Erase Wait */
	do{
		__NOP();
	}	while(nrf_fstorage_is_busy(p_fstorage));
	
	return NRF_SUCCESS;
}

/**
 * @brief Flash Read
 * @param p_fstorage fds instance
 * @param page_addr page address
 * @param len 読み出す長さ
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 Failed
 */
static ret_code_t trace_log_flush_read( nrf_fstorage_t *p_fstorage, uint32_t page_addr, uint32_t len )
{
	ret_code_t err_code;
	
	/* ROM Read */
	err_code = nrf_fstorage_read( p_fstorage, page_addr, (uint8_t*)&g_trace_log_buffer, len );
	if( err_code != NRF_SUCCESS )
	{
		nrf_fstorage_uninit(p_fstorage, NULL);
	}

	return err_code;
}

/**
 * @brief Flash Write
 * @param p_fstorage fds instance
 * @param page_addr page address
 * @param buffer write buffer
 * @param len write buffer size
 * @param p_context context
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 Failed
 */
static ret_code_t trace_log_flush_write(
	nrf_fstorage_t *p_fstorage,
	uint32_t page_addr,
	uint8_t *buffer,
	uint32_t len,
	void *p_context )
{
	ret_code_t err_code;
	
	/* Write ROM */
	err_code = nrf_fstorage_write( p_fstorage, page_addr, (uint8_t*)buffer, len, p_context );
	if( err_code != NRF_SUCCESS )
	{
		nrf_fstorage_uninit(p_fstorage, NULL);
		return err_code;
	}

	/* Write Wait */
	do{
		__NOP();
	}	while(nrf_fstorage_is_busy(p_fstorage));
	
	return NRF_SUCCESS;
}

/**
 * @brief fstorage Event Handler
 * @param p_evt fstorage event context
 * @retval None
 */
static void fstorage_evt_handler( nrf_fstorage_evt_t * p_evt )
{
}


