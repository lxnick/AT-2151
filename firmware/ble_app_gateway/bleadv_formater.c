#include <stdbool.h>
#include <stdint.h>

#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "SEGGER_RTT.h"

#include "bleadv_packet.h"
#include "bleadv_formater.h"

#define AD_TYPE_FLAGS                    0x01
#define AD_TYPE_UUID16_INCOMPLETE        0x02
#define AD_TYPE_UUID16_COMPLETE          0x03
#define AD_TYPE_UUID32_INCOMPLETE        0x04
#define AD_TYPE_UUID32_COMPLETE          0x05
#define AD_TYPE_UUID128_INCOMPLETE       0x06
#define AD_TYPE_UUID128_COMPLETE         0x07
#define AD_TYPE_SHORT_LOCAL_NAME         0x08
#define AD_TYPE_COMPLETE_LOCAL_NAME      0x09
#define AD_TYPE_TX_POWER                 0x0A
#define AD_TYPE_APPEARANCE               0x19
#define AD_TYPE_MANUFACTURER_SPECIFIC    0xFF

static int session = 0;

void make_c_string(uint8_t* buffer, int size,  uint8_t *data, uint16_t len)
{
    if ( len >= size)
        len = size-1;

    for( int i=0; i <len; i ++)
        buffer[i] = data[i];        

    buffer[len] = 0;        
}

void make_uuid16_string(uint8_t* buffer, int size,  uint8_t *data, uint16_t len)
{
    for (uint8_t j = 0; j + 1 < len; j += 2)
    {
        uint16_t uuid = data[j] | (data[j + 1] << 8);

        int written = snprintf(
            (char*) buffer ,
            size,
            "0x%04X,",
            uuid
        );

        buffer += written;
        size -= written;

        if (written < 0 || size <= 0 )
           break;
    }  
}

void make_uuid128_string(uint8_t* buffer, int size, uint8_t* data, uint16_t len)
{
    for (int j = 15; j >= 0; j--)
    {
        int written = snprintf(
            (char*) buffer ,
            size,
            "%02X",
            data[j]);

        buffer += written;
        size -= written;

        if (written < 0 || size <= 0 )
           break;
    }
}

int bleadv_data_formater(uint8_t *data, uint16_t len)
{
    uint16_t i = 0;
    uint16_t appearance;   
    
    uint8_t string_buffer[32];
    uint8_t uuid_buffer[64];

    NRF_LOG_INFO("\nSESSION %d", session);   
    session ++;      

    while (i < len)
    {
        uint8_t field_len = data[i];
        if (field_len == 0) break;

        uint8_t type = data[i + 1];
        uint8_t *value = &data[i + 2];
        uint8_t value_len = field_len - 1;

        switch (type)
        {
        case AD_TYPE_FLAGS:
            NRF_LOG_INFO("AD_TYPE_FLAGS");        
            NRF_LOG_INFO("0x%02X", value[0]);
            break;

        case AD_TYPE_COMPLETE_LOCAL_NAME:
            NRF_LOG_INFO("AD_TYPE_COMPLETE_LOCAL_NAME"); 
            make_c_string(string_buffer, sizeof(string_buffer), value, value_len );
            NRF_LOG_INFO("%s", string_buffer);
            break;

        case AD_TYPE_SHORT_LOCAL_NAME:
            NRF_LOG_INFO("AD_TYPE_SHORT_LOCAL_NAME");   
            make_c_string(string_buffer, sizeof(string_buffer), value, value_len );         
            NRF_LOG_INFO("%s", string_buffer);
            break;

        case AD_TYPE_APPEARANCE:
            NRF_LOG_INFO("AD_TYPE_APPEARANCE");  
            appearance = value[0] | (value[1] << 8);
            NRF_LOG_INFO("%u", appearance);            
            break;

        case AD_TYPE_UUID16_COMPLETE:
            NRF_LOG_INFO("AD_TYPE_UUID16_COMPLETE");  
            make_uuid16_string(uuid_buffer, sizeof(uuid_buffer), value, value_len );
            NRF_LOG_INFO("%s", (char*) uuid_buffer);   
            break;

        case AD_TYPE_UUID16_INCOMPLETE:
            NRF_LOG_INFO("AD_TYPE_UUID16_INCOMPLETE"); 
            make_uuid16_string(uuid_buffer, sizeof(uuid_buffer), value, value_len );
            NRF_LOG_INFO("%s", uuid_buffer); 
            break;

        case AD_TYPE_UUID128_COMPLETE:
            NRF_LOG_INFO("AD_TYPE_UUID128_COMPLETE"); 
            make_uuid128_string(uuid_buffer, sizeof(uuid_buffer), value, value_len );
            NRF_LOG_INFO("%s", uuid_buffer); 
            break;

        case AD_TYPE_MANUFACTURER_SPECIFIC:
            NRF_LOG_INFO("AD_TYPE_MANUFACTURER_SPECIFIC");  
            NRF_LOG_HEXDUMP_INFO(value,value_len);
            break;

        default:
            break;
        }

        i += field_len + 1;
    }

    return 0;
}

void ble_addr_to_str(const uint8_t addr[6], char * str, size_t str_len)
{
    // str_len 至少要 18 bytes: "AA:BB:CC:DD:EE:FF\0"
    snprintf(str, str_len,
             "%02X:%02X:%02X:%02X:%02X:%02X",
             addr[5], addr[4], addr[3],
             addr[2], addr[1], addr[0]);
}

void bleadv_dump_data(uint8_t *data, uint16_t len)
{
    uint16_t i = 0;
    uint16_t appearance;   
    
    uint8_t string_buffer[32];
    uint8_t uuid_buffer[64];

    SEGGER_RTT_printf(0, "Data\n");  
    session ++;      

    while (i < len)
    {
        uint8_t field_len = data[i];
        if (field_len == 0) break;

        uint8_t type = data[i + 1];
        uint8_t *value = &data[i + 2];
        uint8_t value_len = field_len - 1;

        switch (type)
        {
        case AD_TYPE_FLAGS:
            SEGGER_RTT_printf(0, "AD_TYPE_FLAGS");        
            SEGGER_RTT_printf(0, "0x%02X", value[0]);
            break;

        case AD_TYPE_COMPLETE_LOCAL_NAME:
            SEGGER_RTT_printf(0, "AD_TYPE_COMPLETE_LOCAL_NAME"); 
            make_c_string(string_buffer, sizeof(string_buffer), value, value_len );
            SEGGER_RTT_printf(0, "%s", string_buffer);
            break;

        case AD_TYPE_SHORT_LOCAL_NAME:
            SEGGER_RTT_printf(0, "AD_TYPE_SHORT_LOCAL_NAME");   
            make_c_string(string_buffer, sizeof(string_buffer), value, value_len );         
            SEGGER_RTT_printf(0, "%s", string_buffer);
            break;

        case AD_TYPE_APPEARANCE:
            SEGGER_RTT_printf(0, "AD_TYPE_APPEARANCE");  
            appearance = value[0] | (value[1] << 8);
            SEGGER_RTT_printf(0, "%u", appearance);            
            break;

        case AD_TYPE_UUID16_COMPLETE:
            SEGGER_RTT_printf(0, "AD_TYPE_UUID16_COMPLETE");  
            make_uuid16_string(uuid_buffer, sizeof(uuid_buffer), value, value_len );
            SEGGER_RTT_printf(0, "%s", (char*) uuid_buffer);   
            break;

        case AD_TYPE_UUID16_INCOMPLETE:
            SEGGER_RTT_printf(0, "AD_TYPE_UUID16_INCOMPLETE"); 
            make_uuid16_string(uuid_buffer, sizeof(uuid_buffer), value, value_len );
            SEGGER_RTT_printf(0, "%s", uuid_buffer); 
            break;

        case AD_TYPE_UUID128_COMPLETE:
            SEGGER_RTT_printf(0, "AD_TYPE_UUID128_COMPLETE"); 
            make_uuid128_string(uuid_buffer, sizeof(uuid_buffer), value, value_len );
            SEGGER_RTT_printf(0, "%s", uuid_buffer); 
            break;

        case AD_TYPE_MANUFACTURER_SPECIFIC:
            SEGGER_RTT_printf(0, "AD_TYPE_MANUFACTURER_SPECIFIC");  
            NRF_LOG_HEXDUMP_INFO(value,value_len);
            break;

        default:
            break;
        }

        i += field_len + 1;
    }

//    return 0;
}

void bleadv_dump_packet(bleadv_packet_t* pkt)
{
    char addr_str[18];

    SEGGER_RTT_printf(0, "Packet\n");    
    SEGGER_RTT_printf(0, "--rssi %d\n", pkt->rssi);    
    SEGGER_RTT_printf(0, "--addr type %d \n", pkt->addr_type);    

    ble_addr_to_str(pkt->addr, addr_str, sizeof(addr_str));
    SEGGER_RTT_printf(0, "--addr %s\n", addr_str); 

    bleadv_dump_data(pkt->data, pkt->data_len);



}