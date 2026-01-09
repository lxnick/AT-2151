/**
  ******************************************************************************************
  * @file    lib_spi_function.c
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

/* Includes --------------------------------------------------------------*/
#include "nrf_drv_spi.h"
#include "nrf_gpio.h"
#include "string.h"
#include "lib_common.h"
#include "ble_definition.h"
#include "lib_spi_function.h"
#include "lib_trace_log.h"

/* Definition ------------------------------------------------------------*/
#define SPI_INSTANCE  0 /**< SPI instance index. */

#define SPI_CS_ON		0
#define SPI_CS_OFF		1

#define SPI_WAIT_EVENT_INTERVAL		(1000)		/* 応答待ち時間(ns) */

#define SPI_READ_BIT				(0x80)		/* SPI Read bit */

#define SPI_ALREADY_INIT			(1)			/* 既に初期化済み */
#define SPI_YET_USED				(0)			/* 使用中 */

/* Private variables -----------------------------------------------------*/
static const nrf_drv_spi_t gAccSpi = NRF_DRV_SPI_INSTANCE(SPI_INSTANCE);
static volatile bool g_spi_xfer_done = false;  /**< Flag used to indicate that SPI instance completed the transfer. */
/* SPI Initialize Block Counter */
static volatile uint16_t g_init_counter = 0;

/**
 * @brief SPI CS Pin Active
 * @param None
 * @retval None
 */
static void cs_pin_active(void)
{
	nrf_gpio_pin_write(SPI_SS_PIN, SPI_CS_ON);
}

/**
 * @brief SPI CS Pin Inactive
 * @param None
 * @retval None
 */
static void cs_pin_in_active(void)
{
	nrf_gpio_pin_write(SPI_SS_PIN, SPI_CS_OFF);
}

/**
 * @brief SPI Initialize Count Increment
 * @param None
 * @retval None
 */
static void init_counter_inc( void )
{
	volatile uint32_t err_code;
	uint8_t  critrical_session;
	
	err_code = sd_nvic_critical_region_enter( &critrical_session );
	if(err_code == NRF_SUCCESS)
	{
		g_init_counter++;
		critrical_session = 0;
		err_code = sd_nvic_critical_region_exit( critrical_session );
		if( err_code != NRF_SUCCESS ) {}
	}
}

/**
 * @brief SPI Initialize Count decrement
 * @param None
 * @retval None
 */
static void init_counter_dec( void )
{
	volatile uint32_t err_code;
	uint8_t  critrical_session;
	
	err_code = sd_nvic_critical_region_enter( &critrical_session );
	if(err_code == NRF_SUCCESS)
	{
		g_init_counter--;
		critrical_session = 0;
		err_code = sd_nvic_critical_region_exit( critrical_session );
		if( err_code != NRF_SUCCESS ) {}
	}
}

/**
 * @brief Initialize SPI Function
 * @param None
 * @retval NRF_SUCCESS Success
 * @retval NRF_SUCCESS以外 Failed
 */
nrfx_err_t SpiInit(void)
{
	volatile nrfx_err_t ret;
	nrf_drv_spi_config_t spi_config;
	
	/* 初期化済みかどうか確認 */
	init_counter_inc();
	if ( g_init_counter > SPI_ALREADY_INIT )
	{
		return NRF_SUCCESS;
	}

	/* SPI CS GPIO Setup */
	nrf_gpio_cfg(SPI_SS_PIN,
		NRF_GPIO_PIN_DIR_OUTPUT,
		NRF_GPIO_PIN_INPUT_CONNECT,
		NRF_GPIO_PIN_PULLUP,
		NRF_GPIO_PIN_S0S1,
		NRF_GPIO_PIN_NOSENSE);
	// SPI_SS_PIN inactive (spi ss low active).
	nrf_gpio_pin_write( SPI_SS_PIN, SPI_CS_OFF );
	/*SCK PIN Set up*/
	nrf_gpio_cfg(SPI_SCK_PIN,
		NRF_GPIO_PIN_DIR_OUTPUT,
		NRF_GPIO_PIN_INPUT_CONNECT,
		NRF_GPIO_PIN_PULLUP,
		NRF_GPIO_PIN_S0S1,
		NRF_GPIO_PIN_NOSENSE);
	/*MOSI PIN Set up*/
	nrf_gpio_cfg(SPI_MOSI_PIN,
		NRF_GPIO_PIN_DIR_OUTPUT,
		NRF_GPIO_PIN_INPUT_DISCONNECT,
		NRF_GPIO_PIN_PULLDOWN,
		NRF_GPIO_PIN_S0S1,
		NRF_GPIO_PIN_NOSENSE);
	/*MISO PIN set up*/
	nrf_gpio_cfg(SPI_MISO_PIN,
		NRF_GPIO_PIN_DIR_INPUT,
		NRF_GPIO_PIN_INPUT_CONNECT,
		NRF_GPIO_PIN_PULLUP,
		NRF_GPIO_PIN_S0S1,
		NRF_GPIO_PIN_NOSENSE);

	//SPI setting
	spi_config.sck_pin	= SPI_SCK_PIN;
	spi_config.mosi_pin	= SPI_MOSI_PIN;
	spi_config.miso_pin	= SPI_MISO_PIN;
	spi_config.ss_pin	= NRF_DRV_SPI_PIN_NOT_USED;					// ss pin unused
	
	spi_config.irq_priority	= SPI_DEFAULT_CONFIG_IRQ_PRIORITY;		//IRQ_PRIORITY
	spi_config.orc			= 0xff;
	spi_config.frequency	= NRF_DRV_SPI_FREQ_8M;					//SPI frequency 8 MHz
	spi_config.mode			= NRF_DRV_SPI_MODE_3;					//sck:active low, trigger trailing edge
	spi_config.bit_order	= NRF_DRV_SPI_BIT_ORDER_MSB_FIRST;		//MSB_first

	/* 割り込みを使用しない用に修正 */
	ret = nrf_drv_spi_init(&gAccSpi, &spi_config, NULL, NULL);
	if(ret != NRF_SUCCESS)
	{
		DEBUG_LOG( LOG_ERROR,"spi init err 0x%x",ret );
	}

	return ret;
}

/**
 * @brief Uninitialize SPI Function
 * @param None
 * @retval None
 */
void SpiUninit(void)
{
	/* 使用中かどうか確認 */
	init_counter_dec();
	if ( g_init_counter > SPI_YET_USED )
	{
		return ;
	}
	nrf_drv_spi_uninit(&gAccSpi);
}

/**
 * @brief SPI Write
 * @param regAddr Address
 * @param pInData Write Data
 * @param dataSize Write Data Size
 * @retval None
 */
nrfx_err_t SpiIOWrite(uint8_t regAddr, uint8_t *pInData, uint8_t dataSize)
{
	uint8_t txbuf[8];
	volatile nrfx_err_t ret;

	/* 書き込むデータを設定 */
	txbuf[0] = regAddr;
	memcpy(&txbuf[1], pInData, dataSize);
	/* CS Pin有効 */
	cs_pin_active();
	ret = nrf_drv_spi_transfer(&gAccSpi, &txbuf[0], (dataSize + 1), (uint8_t*)NULL, 0);
	/* CS Pin無効 */
	cs_pin_in_active();
	return ret;
}

/**
 * @brief SPI Read
 * @param regAddr Address
 * @param pInData Read Data
 * @param dataSize Read Data Size
 * @retval NRF_SUCCESS Success
 * @retval NRF_ERROR_BUSY Busy
 */
nrfx_err_t SpiIORead(uint8_t regAddr, uint8_t *pOutData, uint8_t dataSize )
{
	volatile nrfx_err_t ret;
	volatile uint8_t txdata[24] = {0};
	volatile uint8_t rxdata[24] = {0};

	/* Readする際には、Bit.0に"1"を立てる */
	txdata[0] = regAddr | SPI_READ_BIT;
	
	/* CS Pin有効 */
	cs_pin_active();
	ret = nrf_drv_spi_transfer(&gAccSpi, (void *)&txdata[0], (dataSize + 1), (void *)&rxdata[0], (dataSize + 1));
	/* CS Pin無効 */
	cs_pin_in_active();

	if(ret != NRF_SUCCESS)
	{
		DEBUG_LOG( LOG_ERROR, "spi read error. err 0x%x", ret );
	}
	else
	{
		memcpy(pOutData, (void *)&rxdata[1], dataSize);
	}
	
	return ret;
}

/**
 * @brief SPI Error Check
 * @param err_code Error Code
 * @param line LINE Number
 * @retval None
 */
void SpiErrCheck(nrfx_err_t err, uint16_t line)
{
	if(err != NRF_SUCCESS)
	{	
		DEBUG_LOG(LOG_ERROR,"spi err 0x%x. line %u",err, line);
		SetBleErrReadCmd(UTC_BLE_SPI_ERROR);
		TRACE_LOG(TR_SPI_ERROR, line);
	}
}
