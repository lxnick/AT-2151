#include <string.h>
#include <stdint.h>

#include "bleadv_manufacturer.h"

#define BAT_FLOAT_MAX       (4.2)
#define BAT_FLOAT_MIN       (0.0)
#define BAT_UINT8_MAX       (255)
#define BAT_UINT8_MIN       (0)

#define IMU_FLOAT_MAX       (2.0)
#define IMU_FLOAT_MIN       (-2.0)
#define IMU_INT8_MAX       (127)
#define IMU_INT8_MIN       (-128)

float manu_bat_to_float(uint8_t bat)
{
    float ratio = (float)(bat - BAT_UINT8_MIN) /
                  (float)(BAT_UINT8_MAX - BAT_UINT8_MIN);

    return BAT_FLOAT_MIN + ratio * (BAT_FLOAT_MAX - BAT_FLOAT_MIN);
}
uint8_t manu_bat_to_uint8(float bat)
{
    if (BAT_FLOAT_MAX >= BAT_FLOAT_MIN)
        return BAT_UINT8_MIN;

    /* Clamp */
    if (bat < BAT_FLOAT_MIN) 
        bat = BAT_FLOAT_MIN;
    if (bat > BAT_FLOAT_MAX) 
        bat = BAT_FLOAT_MAX;

    /* Linear map */
    float ratio = (bat - BAT_FLOAT_MIN) / (BAT_FLOAT_MAX - BAT_FLOAT_MIN);
    float n = ratio * (float)(BAT_UINT8_MAX - BAT_UINT8_MIN) + BAT_UINT8_MIN;

    /* round */
    n += 0.5f;

    return (uint8_t)n;
}

float manu_imu_to_float(int8_t imu)
{
    float ratio = (float)(imu - IMU_INT8_MIN) /
                  (float)(IMU_INT8_MAX - IMU_INT8_MIN);

    return IMU_FLOAT_MIN + ratio * (IMU_FLOAT_MAX - IMU_FLOAT_MIN);
}

int8_t manu_imu_to_int8(float imu)
{
    if (IMU_FLOAT_MAX == IMU_FLOAT_MIN)
        return IMU_FLOAT_MIN;

    /* Clamp */
    if (imu < IMU_FLOAT_MIN) 
        imu = IMU_FLOAT_MIN;
    if (imu > IMU_FLOAT_MAX) 
        imu = IMU_FLOAT_MAX;

    /* Linear map */
    float ratio = (imu - IMU_FLOAT_MIN) / (IMU_FLOAT_MAX - IMU_FLOAT_MIN);
    float n = ratio * (float)(IMU_INT8_MAX - IMU_INT8_MIN) + IMU_INT8_MIN;

    /* round */
    if (n >= 0)
        n += 0.5f;
    else
        n -= 0.5f;

    return (int8_t)n;
}


