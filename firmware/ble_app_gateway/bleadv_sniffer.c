


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

#include "bleadv_formater.h"
#include "bleadv_sniffer.h"
#include "bleadv_queue.h"
/* ================= Configuration ================= */
#define APP_BLE_CONN_CFG_TAG    1
#define APP_BLE_OBSERVER_PRIO   3

#define BIG_SCAN_BUFFER         (1024)

/* Scan buffer (required by SoftDevice) */
//static uint8_t m_scan_buffer_data[BLE_GAP_SCAN_BUFFER_MIN];
static uint8_t m_scan_buffer_data[BIG_SCAN_BUFFER];
static ble_data_t m_scan_buffer = {
    .p_data = m_scan_buffer_data,
    .len    = sizeof(m_scan_buffer_data)
};

/* ================= BLE Event Handler ================= */

static void ble_evt_handler(ble_evt_t const * p_ble_evt, void * p_context)
{
    (void)p_context;

    ret_code_t err;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_ADV_REPORT:
        {
//            SEGGER_RTT_printf(0, "BLE_GAP_EVT_ADV_REPORT\n");          
            const ble_gap_evt_adv_report_t * r =
            &p_ble_evt->evt.gap_evt.params.adv_report;

            bleadv_packet_t pkt;

            pkt.rssi = r->rssi;
            pkt.addr_type = r->peer_addr.addr_type;
            memcpy(pkt.addr, r->peer_addr.addr, 6);

            pkt.data_len = r->data.len;
            if (pkt.data_len > ADV_DATA_MAX_LEN)
                pkt.data_len = ADV_DATA_MAX_LEN;

            memcpy(pkt.data, r->data.p_data, pkt.data_len);
            bleadv_queue_push(&pkt);

//                bleadv_data_formater(r->data.p_data,r->data.len);
            /* IMPORTANT:
            * For passive scanning, SoftDevice requires restarting scan
            * after each ADV report.
            */
            sd_ble_gap_scan_start(NULL, &m_scan_buffer);

            err = sd_ble_gap_scan_start(NULL, &m_scan_buffer);
            if (err != NRF_SUCCESS && err != NRF_ERROR_INVALID_STATE)
                SEGGER_RTT_printf(0, "scan restart err=0x%x\n", err);
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
}


int bleadv_sniffer_start(void)
{
    ble_stack_init();
    scan_start();

    return 0;
}




