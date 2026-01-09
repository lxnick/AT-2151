/**
  ******************************************************************************************
  * @file    lib_debug_uart.c
  * @author  k.tashiro
  * @version 1.0
  * @date    2020/09/08
  * @brief   Output Debug Log
  ******************************************************************************************
    Revision:
    Ver            Date             Revised by        Explanation
  ******************************************************************************************
  * 1.0            2020/09/08       k.tashiro         create new
  ******************************************************************************************
*/

/* Includes --------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#include "nrf_drv_uart.h"
#include "lib_debug_uart.h"

/* Definition ------------------------------------------------------------*/
#define DEF_MINUS_ENABLE	(1)		/** Enable Minus */
#define DEF_MINUS_DISABLE	(0)		/** Disable Minus */
#define DEF_RADIX_10		(10)

#define DEF_LOWER_ENABLE	(0)		/** Lower Case */
#define DEF_UPPER_ENABLE	(1)		/** Upper Case */

#define DEF_STR_INFO	"[I]"
#define DEF_STR_ALERT	"[A]"
#define DEF_STR_DEBUG	"[D]"
#define DEF_STR_ERROR	"[E]"

/* Private variables -----------------------------------------------------*/
static nrf_drv_uart_t g_uart_inst = NRF_DRV_UART_INSTANCE(0);

/* Private function prototypes -------------------------------------------*/
/**
 * @brief Setup Debug Level String
 * @param level Debug Log Level
 * @param str String buffer
 * @retval length String Length
 */
static uint32_t setup_debug_log( int32_t level, char *str );

/**
 * @brief Convert String to decimal
 * @param value Source String
 * @param store_str Store Data
 * @param index Store Index
 * @retval None
 */
static void convert_str_to_dec( int32_t value, char *store_str, uint32_t *index );

/**
 * @brief Convert String to Hex
 * @param value Source String
 * @param store_str Store Data
 * @param index Store Index
 * @param enabled Upper or Lower
 * @retval None
 */
static void convert_str_to_hex( int32_t value, char *store_str, uint32_t *index, int32_t enabled );

/**
 * @brief Get digit from value
 * @param radix Radix
 * @param col column
 * @retval Digit Convert value
 */
static uint32_t get_div_10(uint8_t radix, int8_t col);

/**
 * @brief UART Write
 * @param buffer 書き込みバッファ
 * @param length 書き込みバッファサイズ
 * @retval err_code エラーコード
 */
static uint32_t uart_write( const char *buffer, size_t length );

/**
 * @brief Debug Log Initialize
 * @param None
 * @retval NRF_SUCCESS Success
 */
uint32_t DebugLogInit( void )
{
	volatile ret_code_t err_code;
	nrf_drv_uart_config_t uart_config = NRF_DRV_UART_DEFAULT_CONFIG;
	
	/* Initialize Uart Config */
	uart_config.pseltxd  = NRF_LOG_BACKEND_UART_TX_PIN;
	uart_config.pselrxd  = NRF_UART_PSEL_DISCONNECTED;
	uart_config.pselcts  = NRF_UART_PSEL_DISCONNECTED;
	uart_config.pselrts  = NRF_UART_PSEL_DISCONNECTED;
	uart_config.baudrate = (nrf_uart_baudrate_t)NRF_LOG_BACKEND_UART_BAUDRATE;

	/* Initialize Uart Driver*/
	err_code = nrf_drv_uart_init(&g_uart_inst, &uart_config, NULL);
	if ( err_code != NRF_SUCCESS )
	{
		return err_code;
	}
	
	return NRF_SUCCESS;
}

/**
 * @brief Output Debug Log to UART
 * @param format Specify format
 * @retval None
 */
void DebugLog( int32_t level, const char *format, ... )
{
	va_list va;
	uint32_t ret;
	uint32_t index;
	uint32_t idx;
	char buffer[DEBUG_UART_BUFFER_SIZE] = {0};
	char *token;
	
	if ( format == NULL )
	{
		return ;
	}
	
	va_start( va, format );
	
	index = setup_debug_log(level, buffer);
	while ( *format != '\0' )
	{
		/* format identifier */
		if ( *format == '%' )
		{
			/* read on */
			format++;
			
			switch ( *format )
			{
			case 'u':
			case 'd':		/* format : %d */
				convert_str_to_dec( va_arg(va, int32_t), buffer, &index );
				break;
			case 'x':		/* format : %x */
				convert_str_to_hex( va_arg(va, int32_t), buffer, &index, DEF_LOWER_ENABLE );
				break;
			case 'X':		/* format : %X */
				convert_str_to_hex( va_arg(va, int32_t), buffer, &index, DEF_UPPER_ENABLE );
				break;
			case 's':		/* format : %s */
				token = va_arg( va, char * );
				idx = 0;
				while ( token[idx] != '\0' )
				{
					buffer[index++] = token[idx++];
				}
				break;
			case 'c':		/* format : %c */
				buffer[index++] = va_arg(va, int32_t) & 0xFF;
				break;
			default:
				break;
			}
			format++;
		}
		else
		{
			/* string put */
			buffer[index++] = *format;
			format++;
		}
	}
	
	va_end( va );
	
	/* Line feed code */
	buffer[index++] = '\r';
	buffer[index++] = '\n';

	/* Uart Output */
	ret = uart_write(buffer, index);
	if ( ret != NRF_SUCCESS ) { }
	
	return ;
}

/**
 * @brief Debug Log Direct Output UART
 * @param buffer Send Buffer
 * @param length Send Buffer Length
 * @retval None
 */
void DebugLogDirect( const char *buffer, uint16_t length )
{
	nrf_drv_uart_tx(&g_uart_inst, (uint8_t *)buffer, length);
}

/**
 * @brief Setup Debug Level String
 * @param level Debug Log Level
 * @param str String buffer
 * @retval length String Length
 */
static uint32_t setup_debug_log( int32_t level, char *str )
{
	uint32_t len;

	len = 0;
	if ( str != NULL )
	{
		switch ( level )
		{
		case LOG_ALERT:
			strncpy( str, DEF_STR_ALERT, sizeof( DEF_STR_ALERT ) );
			break;
		case LOG_ERROR:
			strncpy( str, DEF_STR_ERROR, sizeof( DEF_STR_ERROR ) );
			break;
		case LOG_INFO:
			strncpy( str, DEF_STR_INFO, sizeof( DEF_STR_INFO ) );
			break;
		case LOG_DEBUG:
			strncpy( str, DEF_STR_DEBUG, sizeof( DEF_STR_DEBUG ) );
			break;
		default:
			strncpy( str, DEF_STR_DEBUG, sizeof( DEF_STR_DEBUG ) );
			break;
		}
		len = strlen( str );
		str[len++] = 0x20;
	}
	return len;
}

/**
 * @brief Convert String to decimal
 * @param value Source String
 * @param store_str Store Data
 * @param index Store Index
 * @retval None
 */
static void convert_str_to_dec( int32_t value, char *store_str, uint32_t *index )
{
	int32_t minus_flag;
	int32_t remaind_val;
	int32_t digit;
	int32_t columu;
	int32_t idx;
	
	if ( ( store_str != NULL ) && ( index != NULL ) )
	{
		idx = *index;
		
		minus_flag = DEF_MINUS_DISABLE;
		if ( value < 0 )
		{
			value *= -1;
			minus_flag = DEF_MINUS_ENABLE;
		}
		
		remaind_val = value;
		if ( value > 0 )
		{
			columu = 0;
			/* Confirm digit number */
			while ( 1 )
			{
				columu++;
				remaind_val /= 10;
				if ( !remaind_val )
				{
					break;
				}
			}
			
			if ( minus_flag == DEF_MINUS_ENABLE )
			{
				/* In case of a negative value */
				store_str[idx++] = '-';
			}
			remaind_val = value;
			while ( columu > 0 )
			{
				digit = get_div_10( DEF_RADIX_10, columu - 1 );
				store_str[idx++] = (remaind_val / digit) + 0x30;
				remaind_val %= digit;
				columu--;
			}
		}
		else
		{
			store_str[idx++] = 0x30;
			store_str[idx++] = '\0';
		}
		*index = idx;
	}
	
	return ;
}

/**
 * @brief Convert String to Hex
 * @param value Source String
 * @param store_str Store Data
 * @param index Store Index
 * @param enabled Upper or Lower
 * @retval None
 */
static void convert_str_to_hex( int32_t value, char *store_str, uint32_t *index, int32_t enabled )
{
	int32_t idx;
	int32_t result_idx;
	uint32_t remaind_val;
	char hex_table[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8' ,'9' ,'a' ,'b' ,'c', 'd', 'e', 'f'};
	char result[10];
	
	if ( ( store_str != NULL ) && ( index != NULL ) )
	{
		remaind_val = (uint32_t)value;
		if ( remaind_val != 0 )
		{
			result_idx = 0;
			while ( remaind_val > 0 )
			{
				idx = remaind_val % 16;
				remaind_val = remaind_val / 16;
				result[result_idx++] = hex_table[idx];
			}
			
			idx = *index;
			while ( result_idx > 0 )
			{
				result_idx--;
				if ( ( result[result_idx] >= 'a' ) && ( result[result_idx] <= 'f' ) )
				{
					if ( enabled == DEF_UPPER_ENABLE )
					{
						store_str[idx++] = result[result_idx] - 0x20;
					}
					else
					{
						store_str[idx++] = result[result_idx];
					}
				}
				else
				{
					store_str[idx++] = result[result_idx];
				}
			}
		}
		else
		{
			idx = *index;
			store_str[idx++] = hex_table[0];
		}
		*index = idx;
	}
	
	return ;
}

/**
 * @brief Get digit from value
 * @param radix Radix
 * @param col column
 * @retval Digit Convert value
 */
static uint32_t get_div_10(uint8_t radix, int8_t col)
{
	uint32_t ret = 0;
	int i = 0;
	ret = radix;
	if (col <= 0)
	{
		return 1;
	}
	else {
		for (i = 0; i < col - 1; i++) {
			ret = ret * radix;
		}
	}
	return ret;
}

/**
 * @brief UART Write
 * @param buffer 書き込みバッファ
 * @param length 書き込みバッファサイズ
 * @retval err_code エラーコード
 */
static uint32_t uart_write( const char *buffer, size_t length )
{
	ret_code_t err_code = NRF_SUCCESS;
	
	err_code = nrf_drv_uart_tx(&g_uart_inst, (uint8_t *)buffer, length);
	
	return err_code;
}
