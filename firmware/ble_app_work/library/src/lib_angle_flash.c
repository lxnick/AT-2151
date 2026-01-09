/**
  ******************************************************************************************
  * @file    lib_angle_flash.c
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

/* Includes --------------------------------------------------------------*/
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#include "lib_angle_flash.h"

#include "nrf_fstorage.h"
#include "nrf_fstorage_sd.h"

#include "lib_common.h"
#include "definition.h"

/* Private variables -----------------------------------------------------*/


/* Private function prototypes -----------------------------------------------*/
/**
 * @brief fstorage Event Handler
 * @param p_evt fstorage event context
 * @retval None
 */
static void fstorage_evt_handler( nrf_fstorage_evt_t * p_evt );

NRF_FSTORAGE_DEF(nrf_fstorage_t angle_adjust_fstorage) =
{
	/* Set a handler for fstorage events. */
	.evt_handler = fstorage_evt_handler,

	/* These below are the boundaries of the flash space assigned to this instance of fstorage.
	 * You must set these manually, even at runtime, before nrf_fstorage_init() is called.
	 * The function nrf5_flash_end_addr_get() can be used to retrieve the last address on the
	 * last page of flash available to write data. */
	.start_addr = ANGLE_ADJUST_START_ADDR,
	.end_addr   = ANGLE_ADJUST_END_ADDR,
};

/**
 * @brief Trace Log Flash Area Erase
 * @param p_fstorage fds instance
 * @param page_addr erase page address
 * @param len erase page
 * @param p_context context
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 Failed
 */
static uint32_t angle_flush_erase( nrf_fstorage_t *p_fstorage, uint32_t page_addr, uint32_t len, void *p_context )
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
 * @brief Flash Write
 * @param p_fstorage fds instance
 * @param page_addr page address
 * @param buffer write buffer
 * @param len write buffer size
 * @param p_context context
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 Failed
 */
static uint32_t angle_flush_write(
	nrf_fstorage_t *p_fstorage,
	uint32_t page_addr,
	uint8_t *buffer,
	uint32_t len,
	void *p_context )
{
	uint32_t err_code;
	
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
 * @brief Read Angle Adjust Info
 * @remark Flashから角度調整情報を読み出す
 * @param angle_rom_data Flashから読み出したデータ
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 エラー
 */
uint32_t ReadAngleAdjust( ROM_ANGLE_INFO *angle_rom_data )
{
	ret_code_t err_code;
	nrf_fstorage_api_t *p_fs_api;
	nrf_fstorage_t *p_fstorage;
	uint32_t start_addr;
	ROM_ANGLE_INFO angle_info = {0};
	
	/* Flush Config */
	p_fs_api = &nrf_fstorage_sd;
	start_addr = ANGLE_ADJUST_START_ADDR;
	p_fstorage = &angle_adjust_fstorage;
	
	/* fstorage Initialize */
	err_code = nrf_fstorage_init( p_fstorage, p_fs_api, NULL );
	ANGLE_ERR_CHECK( err_code, ANGLE_FLASH_INIT_ERR );

	/* ROM Read */
	err_code = nrf_fstorage_read( p_fstorage, start_addr, (uint8_t *)&angle_info, sizeof( angle_info ) );
	ANGLE_ERR_CHECK( err_code, ANGLE_FLASH_READ_ERR );

	/* fstorage Uninitialize */
	err_code = nrf_fstorage_uninit( p_fstorage, NULL );
	ANGLE_ERR_CHECK( err_code, ANGLE_FLASH_UNINIT_ERR );
	
	if ( angle_rom_data != NULL )
	{
		/* データを設定 */
		memcpy( angle_rom_data, &angle_info, sizeof( ROM_ANGLE_INFO ) );
	}
	
	return NRF_SUCCESS;
}

/**
 * @brief Write Angle Adjust Info
 * @remark Flashへ角度調整情報を書き出す
 * @param angle_rom_data Flashに書き込むデータ
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 エラー
 */
uint32_t WriteAngleAdjust( ROM_ANGLE_INFO *angle_rom_data )
{
	ret_code_t err_code;
	nrf_fstorage_api_t *p_fs_api;
	nrf_fstorage_t *p_fstorage;
	uint32_t start_addr;
	
	/* Flush Config */
	p_fs_api = &nrf_fstorage_sd;
	start_addr = ANGLE_ADJUST_START_ADDR;
	p_fstorage = &angle_adjust_fstorage;
	
	/* fstorage Initialize */
	err_code = nrf_fstorage_init( p_fstorage, p_fs_api, NULL );
	ANGLE_ERR_CHECK( err_code, ANGLE_FLASH_INIT_ERR );

	/* ROM erase */
	err_code = angle_flush_erase( p_fstorage, start_addr, 1, NULL );
	ANGLE_ERR_CHECK( err_code, ANGLE_FLASH_ERASE_ERR );

	/* Write ROM */
	err_code = angle_flush_write( p_fstorage, start_addr, (uint8_t *)angle_rom_data, sizeof( ROM_ANGLE_INFO ), NULL );
	ANGLE_ERR_CHECK( err_code, ANGLE_FLASH_WRITE_ERR );

	/* fstorage Uninitialize */
	err_code = nrf_fstorage_uninit( p_fstorage, NULL );
	ANGLE_ERR_CHECK( err_code, ANGLE_FLASH_UNINIT_ERR );
	
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


