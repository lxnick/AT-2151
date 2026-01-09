/**
  ******************************************************************************************
  * @file    lib_angle_flash.h
  * @author  k.tashiro
  * @version 1.0
  * @date    2022/05/18
  * @brief   Angle Adjustment Flash Read/Write
  ******************************************************************************************
    Revision:
    Ver            Date             Revised by        Explanation
  ******************************************************************************************
  * 1.0            2022/05/18       k.tashiro         create new
  ******************************************************************************************
*/

#ifndef LIB_ANGLE_FLASH_H_
#define LIB_ANGLE_FLASH_H_

/* Includes --------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
#include "lib_common.h"

/* Definition ------------------------------------------------------------*/
#define ANGLE_ADJUST_START_ADDR		ANGLE_ADJUST_ADDR							/* Start Address */
#define ANGLE_ADJUST_END_ADDR		ANGLE_ADJUST_START_ADDR + TEST_PAGE_SIZE	/* End Address */

#define ANGLE_ADJUST_SIG			"Angle"
#define ANGLE_ADJUST_SIG_SIZE		5

#define ANGLE_ERR_CHECK(statement, trace_id)           \
do                                                      \
{                                                       \
    uint32_t _err_code = (uint32_t) (statement);        \
    if (_err_code != NRF_SUCCESS)                       \
    {                                                   \
        DEBUG_LOG(LOG_ERROR,"[AngleAdjust] Err=0x%x, TraceID=0x%x", _err_code, trace_id); \
        return _err_code;                               \
    }                                                   \
} while(0)

/* Struct ----------------------------------------------------------------*/
typedef struct _rom_angle_adjust_info
{
	/* Signiture */
	uint8_t signiture[ANGLE_ADJUST_SIG_SIZE];
	uint8_t state;		/* State */
	float x_angle;		/* X Angle */
	float y_angle;		/* Y Angle */
} ROM_ANGLE_INFO;

/* Function prototypes -------------------------------------------*/

/**
 * @brief Read Angle Adjust Info
 * @remark Flashから角度調整情報を読み出す
 * @param angle_rom_data Flashから読み出したデータ
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 エラー
 */
uint32_t ReadAngleAdjust( ROM_ANGLE_INFO *angle_rom_data );

/**
 * @brief Write Angle Adjust Info
 * @remark Flashへ角度調整情報を書き出す
 * @param angle_rom_data Flashに書き込むデータ
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 エラー
 */
uint32_t WriteAngleAdjust( ROM_ANGLE_INFO *angle_rom_data );

#endif

