/**
  ******************************************************************************************
  * @file    lib_debug_uart.h
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

#ifndef LIB_DEBUG_UART_H_
#define LIB_DEBUG_UART_H_

/* Includes --------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>

/* Definition ------------------------------------------------------------*/
#define DEBUG_UART_BUFFER_SIZE		(256)		/* Debug Uart Buffer Size */

/* Debug Log Level */
#define LOG_ALERT		(1)		/**< Log level (Alert) */
#define LOG_ERROR		(2)		/**< Log level (Error) */
#define LOG_INFO 		(3)		/**< Log level (Info) */
#define LOG_DEBUG		(4)		/**< Log level (Debug) */

//#define LOG_LEVEL		LOG_DEBUG		/**< Output Log Level */
//#define LOG_LEVEL		LOG_ERROR		/**< Output Log Level */
#define LOG_LEVEL		LOG_INFO		/**< Output Log Level */
#if 1
#define DEBUG_LOG(level, ...)	DEBUG_LOG2(level, __VA_ARGS__, "")
#define DEBUG_LOG2(level, fmt, ...)                                                     \
	do{                                                                                 \
		if(level <= LOG_LEVEL){                                                         \
			DEBUG_UART_LOG(level, "[%s:%d] "fmt"%s", __func__, __LINE__, __VA_ARGS__);  \
		}                                                                               \
	}while(0)
#else
#define DEBUG_LOG(level, ...)	                 \
	do{                                          \
		if(level <= LOG_LEVEL){                  \
			DEBUG_UART_LOG(level, __VA_ARGS__);  \
		}                                        \
	}while(0)
#endif

#define DEBUG_UART_LOG(level, ...) DebugLog( level, __VA_ARGS__ )

#define DEBUG_LOG_DIRECT DebugLogDirect

/* Function prototypes ----------------------------------------------------*/
/**
 * @brief Debug Log Initialize
 * @param None
 * @retval NRF_SUCCESS Success
 */
uint32_t DebugLogInit(void);

/**
 * @brief Output Debug Log to UART
 * @param format Specify format
 * @retval None
 */
void DebugLog( int32_t level, const char *format, ... );

/**
 * @brief Debug Log Direct Output UART
 * @param buffer Send Buffer
 * @param length Send Buffer Length
 * @retval None
 */
void DebugLogDirect( const char *buffer, uint16_t length );

#endif

