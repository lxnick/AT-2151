#ifndef BLE_ADV_FORMATER_HEADER__
#define BLE_ADV_FORMATER_HEADER__

#include <stdbool.h>
#include <stdint.h>

#include "bleadv_packet.h"

int bleadv_data_formater(uint8_t *data, uint16_t len);

void bleadv_dump_packet(bleadv_packet_t* packet);

#endif