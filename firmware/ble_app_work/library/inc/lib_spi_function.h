/**
  ******************************************************************************************
  * @file    lib_spi_function.h
  * @author  k.tashiro
  * @version 1.0
  * @date    2020/09/28
  * @brief   SPI function
  ******************************************************************************************
    Revision:
    Ver            Date             Revised by        Explanation
  ******************************************************************************************
  * 1.0            2020/09/28       k.tashiro         create new
  ******************************************************************************************
*/

#ifndef LIB_SPI_FUNCTION_H_
#define LIB_SPI_FUNCTION_H_

/* Includes --------------------------------------------------------------*/
#include "nrf_drv_spi.h"

#ifdef __cplusplus
extern "C"{
#endif

/* Definition ------------------------------------------------------------*/
#define SPI_ERR_CHECK	SpiErrCheck

/* Function prototypes ---------------------------------------------------*/
/**
 * @brief Initialize SPI Function
 * @param None
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 Failed
 */
//nrfx_err_t spi_init(void);
nrfx_err_t SpiInit(void);

/**
 * @brief Uninitialize SPI Function
 * @param None
 * @retval None
 */
//void spi_uninit(void);
void SpiUninit(void);

/**
 * @brief SPI Write
 * @param regAddr Address
 * @param pInData Write Data
 * @param dataSize Write Data Size
 * @retval None
 */
//nrfx_err_t spi_IO_write(uint8_t regAddr, uint8_t *pInData, uint8_t dataSize);
nrfx_err_t SpiIOWrite(uint8_t regAddr, uint8_t *pInData, uint8_t dataSize);

/**
 * @brief SPI Read
 * @param regAddr Address
 * @param pInData Read Data
 * @param dataSize Read Data Size
 * @retval NRF_SUCCESS Success
 * @retval NRF_ERROR_BUSY Busy
 */
//nrfx_err_t spi_IO_read(uint8_t regAddr, uint8_t *pOutData, uint8_t dataSize);
nrfx_err_t SpiIORead(uint8_t regAddr, uint8_t *pOutData, uint8_t dataSize);

/**
 * @brief SPI Error Check
 * @param err_code Error Code
 * @param line LINE Number
 * @retval None
 */
void SpiErrCheck(nrfx_err_t err, uint16_t line);

#ifdef __cplusplus
}
#endif
#endif
