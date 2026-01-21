#ifndef BLE_ADV_PACKET_HEADER__
#define BLE_ADV_PACKET_HEADER__

#include <stdint.h>

#define ADV_DATA_MAX_LEN 31

typedef struct
{
    int8_t   rssi;
    uint8_t addr[6];     // peer address (LSB first)
    uint8_t addr_type;
    uint8_t data_len;
    uint8_t data[ADV_DATA_MAX_LEN];
} bleadv_packet_t;


#endif