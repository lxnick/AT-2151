#ifndef BLE_ADV_FORMATER_HEADER__
#define BLE_ADV_FORMATER_HEADER__

#include <stdbool.h>
#include <stdint.h>

#include "bleadv_packet.h"
#include "bleadv_manufacturer.h"

#define BT_ADDR_STR_SIZE    18
#define BT_DEVICE_NAME_SIZE 30

typedef struct __attribute__((packed))
{
    // BLE Packer Info    
    int8_t rssi;
    uint8_t addr_type;
    char bt_addr[BT_ADDR_STR_SIZE];

    // BLE ADV data  
    char device_name[BT_DEVICE_NAME_SIZE];
   
   // Manufacturer data 
    uint16_t company_id;
    uint16_t app_id; 
    uint16_t device_id;
    uint16_t event;
    float   ax;
    float   ay;
    float   az;
    float   gx;
    float   gy;
    float   gz;
    float   battery;    

} bleadv_format_data;


int bleadv_data_formater(uint8_t *data, uint16_t len);

void bleadv_dump_packet(bleadv_packet_t* packet);

void bleadv_packet_print(bleadv_packet_t* packet);
//void bleadv_packet_format(bleadv_packet_t* packet,bleadv_format_data* format );

void bleadv_packet_output(bleadv_packet_t*, char* buffer, int size);


#endif