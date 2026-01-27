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

void make_c_string(char* buffer, int size,  uint8_t *data, uint16_t len)
{
    if ( len >= size)
        len = size-1;

    for( int i=0; i <len; i ++)
        buffer[i] = data[i];        

    buffer[len] = 0;        
}

void make_uuid16_string(char* buffer, int size,  uint8_t *data, uint16_t len)
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

void make_uuid128_string(char* buffer, int size, uint8_t* data, uint16_t len)
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
    
    char string_buffer[32];
    char uuid_buffer[64];

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
    
    char string_buffer[32];
    char uuid_buffer[64];

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

bleadv_manufacturer_data backup;

void bleadv_packet_manufacture(bleadv_manufacturer_data* manu,bleadv_format_data* format )
{
    format->company_id = manu->company_id;
    format->app_id = manu->app_id;
    format->device_id = manu->device_id;
    format->event = manu->event;

//    SEGGER_RTT_printf(0, "-> ax %d\n", manu->x);   
//    SEGGER_RTT_printf(0, "-> ay %d\n", manu->y);  
//    SEGGER_RTT_printf(0, "-> az %d\n", manu->z); 
//    SEGGER_RTT_printf(0, "-> bat %d\n", manu->bat);                 

    format->ax = manu_imu_to_float( manu->x );
    format->ay = manu_imu_to_float( manu->y );  
    format->az = manu_imu_to_float( manu->z );
    format->gx = manu_imu_to_float( 0 );
    format->gy = manu_imu_to_float( 0 );
    format->gz = manu_imu_to_float( 0 );

    format->battery = manu_bat_to_float(manu->bat);
}

void bleadv_packet_format(bleadv_packet_t* packet,bleadv_format_data* format )
{
    memset(format,0, sizeof(bleadv_format_data) );

    // Get BLE packet info
    format->addr_type = packet->addr_type;
    format->rssi = packet->rssi;
    ble_addr_to_str(packet->addr, format->bt_addr, sizeof(format->bt_addr));  
    
    // Parse BLE packet data
    uint8_t *data_ptr = packet->data;
    uint16_t data_length = packet->data_len;
    uint16_t i = 0;   

    while (i < data_length)
    {
        // Length-Type-Value coding
        uint8_t field_len = data_ptr[i];
        if (field_len == 0) break;

        uint8_t type = data_ptr[i + 1];
        uint8_t *value = &data_ptr[i + 2];
        uint8_t value_len = field_len - 1;

        switch (type)
        {
        case AD_TYPE_COMPLETE_LOCAL_NAME:
            make_c_string(format->device_name, sizeof(format->device_name), value, value_len ); 
            break;             
        case AD_TYPE_SHORT_LOCAL_NAME:
            if ( strlen(format->device_name) == 0)        
                make_c_string(format->device_name, sizeof(format->device_name), value, value_len );        
            break;
        case AD_TYPE_MANUFACTURER_SPECIFIC:
            memcpy(&backup,value, sizeof(backup) );
            bleadv_packet_manufacture( (bleadv_manufacturer_data*) value, format);
            break;

        case AD_TYPE_FLAGS:
        case AD_TYPE_APPEARANCE:
        default:
            break;
        }

        i += field_len + 1;
    }

}

void bleadv_packet_output(bleadv_format_data* format, char* buffer, int size)
{ 
    int voltage = (int)(format->battery * 100);
    int ax = (int)(format->ax * 100);   
    int ay = (int)(format->ay * 100); 
    int az = (int)(format->az * 100); 

    snprintf(buffer,size,"$$$index=%d&x=%d&y=%d&z=%d&gx=%d&gy=%d&gz=%d&bt_addr=%s&user_id=%04x&upload_time=%s&battery=%d###",
           format->event,
        ax,ay,az,0,0,0,format->bt_addr,format->device_id, "2026-01-22T10:30" , voltage);   
}

void bleadv_packet_print(bleadv_packet_t* packet)
{
    bleadv_format_data format;

    memset(&format,0, sizeof(bleadv_format_data) );

    bleadv_packet_format(packet, &format);

    if (strlen(format.device_name) == 0 )
        return;    

    SEGGER_RTT_printf(0, "Packet:-------------------\n");       
 //   SEGGER_RTT_printf(0, "--RSSI %d\n", format.rssi);   
//    SEGGER_RTT_printf(0, "--ADDR TYPE %d\n", format.addr_type); 



    if (format.company_id != COMPANY_ID )
        return ;    

    if (format.app_id != APP_ID )
        return ;   

    SEGGER_RTT_printf(0, "--NAME %s\n", format.device_name); 
    SEGGER_RTT_printf(0, "--ADDR %s\n", format.bt_addr); 
    SEGGER_RTT_printf(0, "--Company:%04x, APP=%04x, DEVICE=%04x\n", format.company_id, format.app_id,format.device_id); 


    SEGGER_RTT_printf(0, "--EVENT %d\n", format.event); 

    SEGGER_RTT_printf(0, "--AX %d\n",backup.x);     
    SEGGER_RTT_printf(0, "--AY %d\n",backup.y);  
    SEGGER_RTT_printf(0, "--AZ %d\n",backup.z);  
    SEGGER_RTT_printf(0, "--BAT %d\n",backup.bat);          

    SEGGER_RTT_printf(0, "--ACCL %d,%d,%d\n", (int) (format.ax * 100), (int) (format.ay * 100), (int) (format.az * 100));   
    SEGGER_RTT_printf(0, "--GYRO %d,%d,%d\n", (int) (format.gx * 100), (int) (format.gy * 100), (int) (format.gz * 100));    
    SEGGER_RTT_printf(0, "--BAT  %d\n", (int)(format.battery * 100)); 
}


