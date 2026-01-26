/**
 * Copyright (c) 2014 - 2021, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/**
 * @brief BLE LED Button Service central and client application main file.
 *
 * This file contains the source code for a sample client application using the LED Button service.
 */

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>


#include "nrf_sdh.h"
#include "nrf_sdh_ble.h"
#include "nrf_sdh_soc.h"

#include "nrf_pwr_mgmt.h"
#include "app_timer.h"
#include "boards.h"
#include "bsp.h"
#include "bsp_btn_ble.h"
#include "ble.h"
#include "ble_hci.h"
#include "ble_advertising.h"
#include "ble_conn_params.h"
//#include "ble_db_discovery.h"
#include "ble_lbs_c.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_scan.h"

#include "nrf_delay.h"

#include "ble_gap.h"
#include "app_error.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "SEGGER_RTT.h"

#include "bleadv_sniffer.h"
#include "bleadv_scan.h"

#include "bleadv_queue.h"

#include "bleadv_formater.h"
#include "uarte_pusher.h"

#include "seq_tracker.h"

#include "led_status.h"

#define TEST_UART_DIRECT    0
#define TEST_BLE_SCAN       0

#define TRACE_ADV_INFO      1

/*
nrfjprog --memrd 0x10001208
0xFFFFFFFE NFC OFF
0xFFFFFFFF NFC ON
Disable NFC 
nrfjprog --memwr 0x10001208 --val 0xFFFFFFFE
*/
static void nfc_pin_check(void)
{
    if ((NRF_UICR->NFCPINS & UICR_NFCPINS_PROTECT_Msk)
        == (UICR_NFCPINS_PROTECT_NFC << UICR_NFCPINS_PROTECT_Pos))
    {
        SEGGER_RTT_printf(0, "NFC Locked!\n");         
        SEGGER_RTT_printf(0, "NFC pins enabled! P0.09/P0.10 unusable");         

        while (1);
    }
    else
    {
        SEGGER_RTT_printf(0, "NFC not locked\n");     
    }
}

static void timers_init(void)
{
    ret_code_t err = app_timer_init();
    APP_ERROR_CHECK(err);
}

static void power_management_init(void)
{
	ret_code_t err_code;
	err_code = nrf_pwr_mgmt_init();
	APP_ERROR_CHECK(err_code);
}


int main(void)
{
    SEGGER_RTT_Init();
    SEGGER_RTT_printf(0, "RTT Gateway!\n");    

    bleadv_sniffer_stack_init();

    nfc_pin_check();

    timers_init();
 	nrf_delay_ms(250);

    led_status_init();
 //   led_test();
 //   led_set_onfoff(0,true); 

    power_management_init();
 	nrf_delay_ms(250);  
    
 //   led_set_onfoff(1,true); 

 //   led_status_init();
 //   led_test();
 //   led_status_heartbeat_start();
//	nrf_delay_ms(250);

    bleadv_queue_init();    
    uarte_pusher_init();
//    led_set_onfoff(2,true); 

 #if TEST_BLE_SCAN   
    SEGGER_RTT_printf(0, "BLE scan!\n");   
    bleadv_scan_test();
 #else
    SEGGER_RTT_printf(0, "BLE sniffer!\n");
    bleadv_sniffer_start();
 //   led_set_onfoff(3,true); 

  //   led_status_scan_start();
    led_system_on();
    led_heartbeat_start();
 #endif   


#if TEST_UART_DIRECT

    SEGGER_RTT_printf(0, "Test UART pattern\n");  

 //   int loop_count = 0;
 //    char buffer[128];
     char sample[]="$$$x=61&y=-74&z=-21&gx=0&gy=0&gz=0&bt_addr=E4%3aC6%3aC6%3aA4%3a7B%3aCE&user_id=929c&upload_time=2026-01-22T10%3a30&battery=316###";

    while (true)
    {
 //       snprintf(buffer,sizeof(buffer), "Hello Gateway UART %d\r\n",loop_count );

        uarte_pusher_push((uint8_t *)sample, strlen(sample));
        SEGGER_RTT_printf(0, "%s\n",sample);
        nrf_delay_ms(1000);
    }

#else

    SEGGER_RTT_printf(0, "BLE snifLoop!\n");
    while (true)
    {
        bleadv_packet_t pkt;
        bleadv_format_data format;

        char buffer[128];

       if (bleadv_queue_pop(&pkt))
        {
            /* 這裡才是安全區 */
            SEGGER_RTT_printf(0, "LOOP---------------\n");            
            SEGGER_RTT_printf(0, "ADV RSSI=%d len=%d\n", pkt.rssi, pkt.data_len);

//            bleadv_dump_packet(&pkt);
//            bleadv_packet_print(&pkt);
            memset(&format,0, sizeof(bleadv_format_data) );
            bleadv_packet_format(&pkt, &format);

#if TRACE_ADV_INFO
            if (strlen(format.device_name) != 0 )
                SEGGER_RTT_printf(0, "%s\n", format.device_name);  
#endif            

            if (format.company_id != COMPANY_ID )
                continue;      

            if (format.app_id != APP_ID )
                continue;   

            if (strlen(format.device_name) == 0 )
                continue;            

//            if (! seq_tracker_accept(format.device_id, format.event))
//               continue;

            bleadv_packet_output(&format, buffer, sizeof(buffer));

//            uarte_tx_send((uint8_t *)buffer, strlen(buffer));
                uarte_pusher_push((uint8_t *)buffer, strlen(buffer));
                led_blink_uart();


 //           SEGGER_RTT_printf(0, "%s\n",buffer);

            // TODO:
            // 1. parse manufacturer data
            // 2. filter app_id / device_id
            // 3. pusher_push(pkt)
        }

        if (NRF_LOG_PROCESS() == false)
        {
            __WFE();
        }
    }
#endif    
}




