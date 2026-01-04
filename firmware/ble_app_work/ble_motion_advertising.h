#ifndef BLE_MOTION_ADVERTISING_H__
#define BLE_MOTION_ADVERTISING_H__

#include <stdint.h>
#include <stdbool.h>
#include "ble.h"
#include "ble_srv_common.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct __attribute__((packed))
{
    uint16_t company_id;   // 固定識別 (建議用你自己的，測試可用 0xFFFF)
    uint8_t  app_id;       // 應用識別 (例如 0x26)
    uint8_t  device_id;    // Device ID (26)
    uint8_t  status;       // 狀態 (0=idle,1=flip,2=alive,…)
} motion_adv_mfg_data_t;

#ifdef __cplusplus
}
#endif

#endif // BLE_MOTION_ADVERTISING_H__