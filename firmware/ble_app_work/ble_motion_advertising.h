#ifndef BLE_MOTION_ADVERTISING_H__
#define BLE_MOTION_ADVERTISING_H__

#include <stdint.h>
#include <stdbool.h>
#include "ble.h"
#include "ble_srv_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define APP_ID              0x3412
#define DEVICE_ID           0x1111

#define STATUS_ERROR        0x00
#define STATUS_BOOT         0x01
#define STATUS_HEARTBEAT    0x02
#define STATUS_BUTTON       0x03

#define MOTION_NONE         0x00
#define MOTION_FALLEN       0x01


typedef struct __attribute__((packed))
{
    uint16_t  app_id;       // 
    uint16_t  device_id;    // 
    uint8_t  status;       // 
    uint8_t  motion;
} motion_adv_mfg_data_t;

#ifdef __cplusplus
}
#endif

#endif // BLE_MOTION_ADVERTISING_H__