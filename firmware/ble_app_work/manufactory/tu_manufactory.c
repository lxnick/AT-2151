/**
  ******************************************************************************************
  * Reference source: dtm_standalone main.c
  ******************************************************************************************
*/

#include <stdint.h>
#include <stdbool.h>
#include "nrf.h"
#include "ble_dtm.h"
#include "boards.h"
#include "app_uart.h"
#include "nrf_drv_clock.h"
#include "nrf_drv_uart.h"
#include "nrf_delay.h"
#include "tu_lib_common.h"
#include "tu_lib_fifo.h"
#include "tu_manufactory.h"
#include "tu_radio_test.h"


// @note: The BLE DTM 2-wire UART standard specifies 8 data bits, 1 stop bit, no flow control.
//        These parameters are not configurable in the BLE standard.

/**@details Maximum iterations needed in the main loop between stop bit 1st byte and start bit 2nd
 * byte. DTM standard allows 5000us delay between stop bit 1st byte and start bit 2nd byte.
 * As the time is only known when a byte is received, then the time between between stop bit 1st
 * byte and stop bit 2nd byte becomes:
 *      5000us + transmission time of 2nd byte.
 *
 * Byte transmission time is (Baud rate of 19200):
 *      10bits * 1/19200 = approx. 520 us/byte (8 data bits + start & stop bit).
 *
 * Loop time on polling UART register for received byte is defined in ble_dtm.c as:
 *   UART_POLL_CYCLE = 260 us
 *
 * The max time between two bytes thus becomes (loop time: 260us / iteration):
 *      (5000us + 520us) / 260us / iteration = 21.2 iterations.
 *
 * This is rounded down to 21.
 *
 * @note If UART bit rate is changed, this value should be recalculated as well.
 */
#define MAX_ITERATIONS_NEEDED_FOR_NEXT_BYTE ((5000 + 2 * UART_POLL_CYCLE) / UART_POLL_CYCLE)

#define MAX_TEST_DATA_BYTES     (15U)                /**< max number of test bytes to be used for tx and rx. */
#define UART_TX_BUF_SIZE 256                         /**< UART TX buffer size. */
#define UART_RX_BUF_SIZE 256                         /**< UART RX buffer size. */

#define ENTRY_SLEEP_CMD         (0x5A)
#define ENTRY_REPORT_CMD        (0xA5) 
#define ENTRY_TX_ON_CMD         (0x69) 
#define ENTRY_TX_OFF_CMD        (0x96) 

static const uint8_t tu_gfw_version[] = {4,1,7};

extern int32_t TU_AnalogBattGetDirectly( void );
extern ret_code_t TU_ManufactoryRtcSetDefault(void);
extern ret_code_t TU_ManufactoryRtcGetDateTime(DATE_TIME *pdatetime);
extern ret_code_t TU_ManufactoryAccGyroCheckDeviceID(uint8_t *pdevice_id);

// Error handler for UART
void TU_uart_error_handle(app_uart_evt_t * p_event)
    
{
    if (p_event->evt_type == APP_UART_COMMUNICATION_ERROR)
    {
        APP_ERROR_HANDLER(p_event->data.error_communication);
    }
    else if (p_event->evt_type == APP_UART_FIFO_ERROR)
    {
        APP_ERROR_HANDLER(p_event->data.error_code);
    }
}

/**@brief Function for UART initialization.
 */
static void TU_uart_init(uint32_t baudrate)
{   
    uint32_t err_code;
    const app_uart_comm_params_t comm_params =
      {
          PIN_UART_RX,
          PIN_UART_TX,
          NRF_UART_PSEL_DISCONNECTED,
          NRF_UART_PSEL_DISCONNECTED,
          APP_UART_FLOW_CONTROL_DISABLED,
          false,
          baudrate
      };

    APP_UART_FIFO_INIT(&comm_params,
                       UART_RX_BUF_SIZE,
                       UART_TX_BUF_SIZE,
                       TU_uart_error_handle,
                       APP_IRQ_PRIORITY_LOWEST,
                       err_code);

    APP_ERROR_CHECK(err_code);
}

void TU_process_dtm(void)
{
    uint32_t    current_time;
    uint32_t    dtm_error_code;
    uint32_t    msb_time          = 0;     // Time when MSB of the DTM command was read. Used to catch stray bytes from "misbehaving" testers.
    bool        is_msb_read       = false; // True when MSB of the DTM command has been read and the application is waiting for LSB.
    uint16_t    dtm_cmd_from_uart = 0;     // Packed command containing command_code:freqency:length:payload in 2:6:6:2 bits.
    uint8_t     rx_byte;                   // Last byte read from UART.
    dtm_event_t result;                    // Result of a DTM operation.
				
    TU_uart_init(DTM_BITRATE);

    dtm_error_code = dtm_init();
    if (dtm_error_code != DTM_SUCCESS)
    {
        // If DTM cannot be correctly initialized, then we just return.
        return;
    }

    for (;;)
    {
        // Will return every timeout, 625 us.
        current_time = dtm_wait();

        if (app_uart_get(&rx_byte) != NRF_SUCCESS)
        {
            // Nothing read from the UART.
            continue;
        }

        if (!is_msb_read)
        {
            // This is first byte of two-byte command.
            is_msb_read       = true;
            dtm_cmd_from_uart = ((dtm_cmd_t)rx_byte) << 8;
            msb_time          = current_time;

            // Go back and wait for 2nd byte of command word.
            continue;
        }

        // This is the second byte read; combine it with the first and process command
        if (current_time > (msb_time + MAX_ITERATIONS_NEEDED_FOR_NEXT_BYTE))
        {
            // More than ~5mS after msb: Drop old byte, take the new byte as MSB.
            // The variable is_msb_read will remains true.
            // Go back and wait for 2nd byte of the command word.
            dtm_cmd_from_uart = ((dtm_cmd_t)rx_byte) << 8;
            msb_time          = current_time;
            continue;
        }

        // 2-byte UART command received.
        is_msb_read        = false;
        dtm_cmd_from_uart |= (dtm_cmd_t)rx_byte;

        if (dtm_cmd(dtm_cmd_from_uart) != DTM_SUCCESS)
        {
            // Extended error handling may be put here.
            // Default behavior is to return the event on the UART (see below);
            // the event report will reflect any lack of success.
        }

        // Retrieve result of the operation. This implementation will busy-loop
        // for the duration of the byte transmissions on the UART.
        if (dtm_event_get(&result))
        {
            // Report command status on the UART.
            // Transmit MSB of the result.
            while (app_uart_put((result >> 8) & 0xFF));
            // Transmit LSB of the result.
            while (app_uart_put(result & 0xFF));
        }
    }
}

ret_code_t TU_Manufactory_NordicSoftDeviceEnable(void)
{
	uint32_t err_code;
	uint32_t ram_start;
	
	// softdevice enable.
	if ( !nrf_sdh_is_enabled() )
	{
		err_code = nrf_sdh_enable_request();
	}
    
    if(err_code == NRF_SUCCESS)
    {
	
        //Work around chip errata [68]
        nrf_delay_ms(WAIT_HFCLK_STABLE_MS);
        
        //Set the default BLE stack configuration.
        err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG, &ram_start);
        
        if(err_code == NRF_SUCCESS)
        {
            //softdevice ble stack enable.
            err_code = nrf_sdh_ble_enable(&ram_start);
        }
    }
    
    return err_code;
}


/** @brief Function for configuring all peripherals used in this example.
 */
static void TU_clock_init(void)
{
    // Start 64 MHz crystal oscillator.
    NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
    NRF_CLOCK->TASKS_HFCLKSTART    = 1;

    // Wait for the external oscillator to start up.
    while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0)
    {
        // Do nothing.
    }
}

static void TU_tx_on(void)
{
            uint32_t err_code = nrf_drv_clock_init();
            APP_ERROR_CHECK(err_code);
            nrf_drv_clock_lfclk_request(NULL);
            TU_clock_init();
 
            NRF_RADIO->SHORTS =0;
            NRF_RADIO->INTENCLR =1;
            nrf_radio_event_clear(NRF_RADIO_EVENT_DISABLED);
            nrf_radio_task_trigger(NRF_RADIO_TASK_DISABLE);
            while (!nrf_radio_event_check(NRF_RADIO_EVENT_DISABLED))
            {
                /* Do nothing */
            }
    
            NRF_RADIO->MODE = ((uint32_t) NRF_RADIO_MODE_NRF_1MBIT << RADIO_MODE_MODE_Pos);
 
            nrf_radio_txpower_set(NRF_RADIO_TXPOWER_POS4DBM);   //DEFAULT_TX_POWER, (NRF_RADIO_TXPOWER_0DBM);
            nrf_radio_frequency_set(2440);
            nrf_radio_task_trigger(NRF_RADIO_TASK_TXEN);
            
            while(1)
            {
                __NOP();
            }
}

void TU_nrf_power_gpregret2_set(uint32_t value)
{
    uint32_t err_code;

    err_code = sd_power_gpregret_clr(1, 0xFFFF);		//gpregret_id 0 for GPREGRET, 1 for GPREGRET2.
    APP_ERROR_CHECK(err_code);

    err_code = sd_power_gpregret_set(1, value);
    APP_ERROR_CHECK(err_code);
}

void TU_process_manufactory(void)
{
    int32_t ibatt_val = 0;
    ret_code_t err_code_rtc;
    ret_code_t err_code_AccGyro;
    ret_code_t err_code;
    uint8_t    rx_byte;
    uint8_t    accgyro_device_id = 0, accgyro_data_xyz[12];
    DATE_TIME  datetime;
    ble_gap_addr_t device_addr;
    uint32_t m_power_gpregret2 = 0;
    
    m_power_gpregret2 = NRF_POWER->GPREGRET2;
    NRF_POWER->GPREGRET2 = 0;
    
    if(m_power_gpregret2 == ENTRY_TX_ON_CMD)
        TU_tx_on();
    
//    NRF_POWER->DCDCEN = (POWER_DCDCEN_DCDCEN_Enabled << POWER_DCDCEN_DCDCEN_Pos);

    err_code_AccGyro = TU_ManufactoryAccGyroCheckDeviceID(&accgyro_device_id);
    
    // Trigger
    err_code_rtc = TU_ManufactoryRtcSetDefault();

    err_code = TU_Manufactory_NordicSoftDeviceEnable();

    // Get BLE address.
    if(err_code == NRF_SUCCESS)
        err_code = sd_ble_gap_addr_get(&device_addr);

    if(err_code != NRF_SUCCESS)
        memset(&device_addr, 0, sizeof(device_addr));

    TU_uart_init(UARTE_BAUDRATE_BAUDRATE_Baud115200);
    
    memset(accgyro_data_xyz, 0, sizeof(accgyro_data_xyz));
    
    while(1)
    {
        if (app_uart_get(&rx_byte) != NRF_SUCCESS)
        {
            // Nothing read from the UART.
            continue;
        }
        
        if(rx_byte == ENTRY_SLEEP_CMD)
        {
            nrf_gpio_cfg_output(PIN_BATT_ADC);
            nrf_gpio_pin_set(PIN_BATT_ADC);
            sd_power_system_off();
        }
        else if(rx_byte == ENTRY_REPORT_CMD)
        {
            ibatt_val = TU_AnalogBattGetDirectly();
            TU_ManufactoryRtcGetDateTime(&datetime);
            while (app_uart_put(0x1D)); //length
            while (app_uart_put(tu_gfw_version[2]));
            while (app_uart_put(tu_gfw_version[1]));
            while (app_uart_put(tu_gfw_version[0]));
            while (app_uart_put(err_code_AccGyro & 0xFF));
            while (app_uart_put(accgyro_device_id));
            while (app_uart_put(accgyro_data_xyz[0]));
            while (app_uart_put(accgyro_data_xyz[1]));
            while (app_uart_put(accgyro_data_xyz[2]));
            while (app_uart_put(accgyro_data_xyz[3]));
            while (app_uart_put(accgyro_data_xyz[4]));
            while (app_uart_put(accgyro_data_xyz[5]));
            while (app_uart_put(accgyro_data_xyz[6]));
            while (app_uart_put(accgyro_data_xyz[7]));
            while (app_uart_put(accgyro_data_xyz[8]));
            while (app_uart_put(accgyro_data_xyz[9]));
            while (app_uart_put(accgyro_data_xyz[10]));
            while (app_uart_put(accgyro_data_xyz[11]));
            while (app_uart_put(err_code_rtc & 0xFF));
            while (app_uart_put(datetime.sec & 0xFF));
            while (app_uart_put(datetime.min & 0xFF));
            while (app_uart_put(datetime.hour & 0xFF));
            while (app_uart_put((ibatt_val >> 0) & 0xFF));
            while (app_uart_put((ibatt_val >> 8) & 0xFF));
            while (app_uart_put(device_addr.addr[0]));
            while (app_uart_put(device_addr.addr[1]));
            while (app_uart_put(device_addr.addr[2]));
            while (app_uart_put(device_addr.addr[3]));
            while (app_uart_put(device_addr.addr[4]));
            while (app_uart_put(device_addr.addr[5]));
        }
        else if(rx_byte == ENTRY_TX_ON_CMD)
        {
            app_uart_close();
            TU_nrf_power_gpregret2_set(ENTRY_TX_ON_CMD);
            NVIC_SystemReset();
        }
    }
}

void TU_check_manufactory_mode(void)
{
	uint32_t cnt = 0;
	
	nrf_gpio_cfg_input(PIN_PCB, NRF_GPIO_PIN_PULLUP);
	while(cnt < TIMES_ENTRY_MANU_TEST)
	{
		if(nrf_gpio_pin_read(PIN_PCB) > 0)
			break;
		nrf_delay_ms(10);
		cnt++;
	}
	
	if(cnt >= TIMES_ENTRY_MANU_TEST)
	{
        cnt = 0;
        nrf_gpio_cfg_input(PIN_UART_RX, NRF_GPIO_PIN_PULLDOWN);
        
        while(cnt < TIMES_ENTRY_MANU_TEST) {
            if(nrf_gpio_pin_read(PIN_UART_RX) == 0)
                break;
            nrf_delay_ms(10);
            cnt++;
        }
        
        nrf_gpio_cfg_default(PIN_UART_RX);
        if(cnt >= TIMES_ENTRY_MANU_TEST)   
            TU_process_manufactory();
	}
	else
    {
		nrf_gpio_cfg_default(PIN_PCB);
    }
}

void TU_check_dtm_mode(void)
{
	uint32_t cnt = 0;
	
	nrf_gpio_cfg_input(PIN_PCB, NRF_GPIO_PIN_PULLUP);
    nrf_gpio_cfg_input(PIN_BATT_ADC, NRF_GPIO_PIN_PULLUP);
    
	while(cnt < TIMES_ENTRY_MANU_TEST) {
		if((nrf_gpio_pin_read(PIN_BATT_ADC) > 0) || (nrf_gpio_pin_read(PIN_PCB) > 0))
			break;
		nrf_delay_ms(10);
		cnt++;
	}
	
	if(cnt >= TIMES_ENTRY_MANU_TEST)
	{
        cnt = 0;
        nrf_gpio_cfg_input(PIN_UART_RX, NRF_GPIO_PIN_PULLDOWN);
        
        while(cnt < TIMES_ENTRY_MANU_TEST) {
            if(nrf_gpio_pin_read(PIN_UART_RX) == 0)
                break;
            nrf_delay_ms(10);
            cnt++;
        }
        
        nrf_gpio_cfg_default(PIN_UART_RX);
        if(cnt >= TIMES_ENTRY_MANU_TEST)   
            TU_process_dtm();
	}
	else
    {
		nrf_gpio_cfg_default(PIN_PCB);
        nrf_gpio_cfg_default(PIN_BATT_ADC);
    }
}

/**
 * @brief ACC/Gyro Read Device ID
 * @param None
 * @retval NRF_SUCCESS Success
 * @retval ACC_GYRO_SETEUP_ERROR Setup Error
 */
static nrfx_err_t TU_manufactory_acc_gyro_read_device_id( uint8_t *pdevice_id )
{
	nrfx_err_t err_code;

	/* Device ID ???? */
	err_code = SpiIORead( ICM42607_WHO_AM_I, pdevice_id, sizeof( uint8_t ) );
	if ( err_code != NRF_SUCCESS )
	{
		return ACC_GYRO_SETEUP_ERROR;
	}
	
	if ( *pdevice_id != ICM42607_DEVICE_ID )
	{
		err_code = ACC_GYRO_SETEUP_ERROR;
	}
	
	return err_code;

}

/**
 * @brief ACC/Gyro Sensor Read WakeUp Interrupt Source
 * @param None
 * @retval UTC_SUCCESS Success
 * @retval UTC_SPI_ERROR Error
 */
static ret_code_t TU_ManufactoryAccGyroCheckDeviceID(uint8_t *pdevice_id)
{
	nrfx_err_t err_code;
	
	/* SPI Function Initialize */
	err_code = SpiInit();
	if ( err_code == NRF_SUCCESS )
	{
		/* Device ID ???? */
		err_code = TU_manufactory_acc_gyro_read_device_id(pdevice_id);
	
		/* ???? */
		SpiUninit();
	}
	
	return err_code;
}

/// @}
