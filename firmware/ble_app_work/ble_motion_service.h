#ifndef BLE_MOTION_SERVICE_H__
#define BLE_MOTION_SERVICE_H__

#include <stdint.h>
#include <stdbool.h>
#include "ble.h"
#include "ble_srv_common.h"

#ifdef __cplusplus
extern "C" {
#endif


//  #define MOTION_UUID_BASE        { 0x00,0x00,0x00,0x00,0x12,0x34,0x56,0x78,0x12,0x34,0x56,0x78,0x9A,0xBC,0xDE,0xF0 }

// 128-bit Base UUID: f0debc9a-7856-3412-7856-341200000000 (example)
#define MOTION_UUID_BASE         {0xCF, 0x71, 0x6C, 0xB2, 0x2D, 0x15, 0xBC, 0x92, \
                                          0xAA, 0x46, 0xB6, 0x66, 0xF9, 0x7D, 0xF6, 0x3F}
// 16-bit UUIDs within that base
#define MOTION_UUID_SERVICE     0x1520
#define MOTION_UUID_DEV_ID_CHAR 0x1521
#define MOTION_UUID_STATUS_CHAR 0x1522

typedef struct ble_motion_s ble_motion_t;

typedef void (*ble_motion_evt_handler_t)(ble_motion_t * p_motion, ble_evt_t const * p_ble_evt);

struct ble_motion_s
{
    uint16_t                 service_handle;
    ble_gatts_char_handles_t dev_id_handles;
    ble_gatts_char_handles_t status_handles;
    uint8_t                  uuid_type;

    uint16_t                 conn_handle;
};

uint32_t ble_motion_init(ble_motion_t * p_motion);
void     ble_motion_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context);

uint32_t ble_motion_status_notify(ble_motion_t * p_motion, uint8_t status);

#ifdef __cplusplus
}
#endif

#endif // BLE_MOTION_SERVICE_H__
