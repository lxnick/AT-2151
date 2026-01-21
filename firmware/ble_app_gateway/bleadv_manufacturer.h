#ifndef MANUFACTURER_DATA_H__
#define MANUFACTURER_DATA_H__

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define COMPANY_ID          0xFFFF
#define APP_ID              0x3412
#define DEVICE_ID           0x0180

#define EVENT_ERROR        0x00
#define EVENT_BOOT         0x01
#define EVENT_HEARTBEAT    0x02
#define EVENT_BUTTON       0x03

#define BAT_FLOAT_MAX       (4.2)
#define BAT_FLOAT_MIN       (0.0)
#define BAT_UINT8_MAX       (255)
#define BAT_UINT8_MIN       (0)

#define IMU_FLOAT_MAX       (2.0)
#define IMU_FLOAT_MIN       (-2.0)
#define IMU_INT8_MAX       (127)
#define IMU_INT8_MIN       (-128)

typedef struct __attribute__((packed))
{
    uint16_t  company_id;       // 
    uint16_t  app_id;       // 
    uint16_t  device_id;    // 
    uint8_t  event;       // 

    int8_t  x;
    int8_t  y;
    int8_t  z;
    uint8_t  bat;

    uint8_t  one;
    uint8_t  two;
    uint8_t  three; 
    uint8_t  four;         

} bleadv_manufacturer_data;

float manu_bat_to_float(uint8_t bat);
uint8_t manu_bat_to_uint8(float bat);

float manu_imu_to_float(int8_t imu);
int8_t manu_imu_to_int8(float imu);

#ifdef __cplusplus
}
#endif

#endif // BLE_MOTION_ADVERTISING_H__