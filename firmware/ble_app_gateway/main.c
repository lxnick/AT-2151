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
#include "ble_db_discovery.h"
#include "ble_lbs_c.h"
#include "nrf_ble_gatt.h"
#include "nrf_ble_scan.h"

#include "ble_gap.h"
#include "app_error.h"

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "SEGGER_RTT.h"

/* ================= Configuration ================= */
#define APP_BLE_CONN_CFG_TAG    1
#define APP_BLE_OBSERVER_PRIO   3

/* Scan buffer (required by SoftDevice) */
static uint8_t m_scan_buffer_data[BLE_GAP_SCAN_BUFFER_MIN];
static ble_data_t m_scan_buffer = {
    .p_data = m_scan_buffer_data,
    .len    = sizeof(m_scan_buffer_data)
};

/* ================= BLE Event Handler ================= */

static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    (void)p_context;

    switch (p_ble_evt->header.evt_id)
    {
    case BLE_GAP_EVT_ADV_REPORT:
        {
        const ble_gap_evt_adv_report_t * r =
            &p_ble_evt->evt.gap_evt.params.adv_report;

//            const ble_gap_adv_report_type_t type= r->type;

//        const ble_gap_addr_t peer_addr = r->peer_addr;
//        const ble_gap_addr_t direct_addr  = r->direct_addr;
//        const uint8_t primary_phy = r->primary_phy;
//        const uint8_t secondary_phy = r->secondary_phy;
//        const uint8_t tx_power = r->tx_power;        
            uint8_t rssi = r->rssi;   
//        const uint8_t ch_index = r->ch_index;     
//        const uint8_t set_id = r->set_id;      
//        const uint16_t data_id = r->data_id;
//        const ble_data_t data = r->data;
//        const ble_gap_aux_pointer_t aux_pointer = r->aux_pointer;

#if 0
    NRF_LOG_INFO("Type(%04x),RSSI(%d)=, Data(%d)",
                     (int)type,
                     rssi,
                     data.len);
#endif       
 
        NRF_LOG_INFO("RSSI(%d) Data length(%d)",
                   (int)rssi,
                    r->data.len);

#if  0
        NRF_LOG_INFO("MAC: %02X:%02X:%02X:%02X:%02X:%02X",
                     r->peer_addr.addr[5],
                     r->peer_addr.addr[4],
                     r->peer_addr.addr[3],
                     r->peer_addr.addr[2],
                     r->peer_addr.addr[1],
                     r->peer_addr.addr[0]);
#endif

         NRF_LOG_HEXDUMP_INFO(r->data.p_data,r->data.len);
       

        /* IMPORTANT:
         * For passive scanning, SoftDevice requires restarting scan
         * after each ADV report.
         */
        sd_ble_gap_scan_start(NULL, &m_scan_buffer);
            }
        break;

        default:
            break;
    }
}

/* ================= BLE Stack Init ================= */

static void ble_stack_init(void)
{
    ret_code_t err_code;

    err_code = nrf_sdh_enable_request();
    APP_ERROR_CHECK(err_code);

    uint32_t ram_start = 0;
    err_code = nrf_sdh_ble_default_cfg_set(APP_BLE_CONN_CFG_TAG,
                                          &ram_start);
    APP_ERROR_CHECK(err_code);

    // Enable BLE stack.
    err_code = nrf_sdh_ble_enable(&ram_start);
    APP_ERROR_CHECK(err_code);

    NRF_SDH_BLE_OBSERVER(m_ble_observer,
                         APP_BLE_OBSERVER_PRIO,
                         ble_evt_handler,
                         NULL);
}

/* ================= Scan Start ================= */

static void scan_start(void)
{
    ble_gap_scan_params_t scan_params = {
        .active        = 0,                       // Passive scan
        .interval      = 0x00A0,                  // 100 ms
        .window        = 0x0050,                  // 50 ms
        .timeout       = 0,                       // No timeout
        .scan_phys     = BLE_GAP_PHY_1MBPS,
        .filter_policy = BLE_GAP_SCAN_FP_ACCEPT_ALL,
    };

    ret_code_t err_code =
        sd_ble_gap_scan_start(&scan_params, &m_scan_buffer);

    APP_ERROR_CHECK(err_code);

    NRF_LOG_INFO("BLE ADV sniffer started");
}

/* ================= Main ================= */

int main(void)
{
//    NRF_LOG_INIT(NULL);
//    NRF_LOG_DEFAULT_BACKENDS_INIT();
//    NRF_LOG_INFO("BLE ADV Sniffer Boot");

    SEGGER_RTT_Init();
    SEGGER_RTT_printf(0, "RTT Gateway!\n");    

    ble_stack_init();
    scan_start();

    while (true)
    {
        if (NRF_LOG_PROCESS() == false)
        {
            __WFE();
        }
    }
}




