#include "ble_motion_service.h"
#include "sdk_common.h"

#define DEVICE_ID_VALUE  26  // 先固定，之後可換成 FICR/自訂ID

static uint32_t dev_id_char_add(ble_motion_t * p_motion)
{
    uint32_t              err_code;
    ble_add_char_params_t add_char_params;

    memset(&add_char_params, 0, sizeof(add_char_params));
    add_char_params.uuid              = MOTION_UUID_DEV_ID_CHAR;
    add_char_params.uuid_type         = p_motion->uuid_type;
    add_char_params.init_len          = sizeof(uint8_t);
    add_char_params.max_len           = sizeof(uint8_t);
    add_char_params.is_var_len        = false;
    add_char_params.char_props.read   = 1;
    add_char_params.read_access       = SEC_OPEN;

    uint8_t dev_id = DEVICE_ID_VALUE;
    add_char_params.p_init_value      = &dev_id;

    err_code = characteristic_add(p_motion->service_handle,
                                  &add_char_params,
                                  &p_motion->dev_id_handles);
    return err_code;
}

static uint32_t status_char_add(ble_motion_t * p_motion)
{
    uint32_t              err_code;
    ble_add_char_params_t add_char_params;

    memset(&add_char_params, 0, sizeof(add_char_params));
    add_char_params.uuid                = MOTION_UUID_STATUS_CHAR;
    add_char_params.uuid_type           = p_motion->uuid_type;
    add_char_params.init_len            = sizeof(uint8_t);
    add_char_params.max_len             = sizeof(uint8_t);
    add_char_params.is_var_len          = false;
    add_char_params.char_props.notify   = 1;
    add_char_params.cccd_write_access   = SEC_OPEN;
    add_char_params.read_access         = SEC_NO_ACCESS;
    add_char_params.write_access        = SEC_NO_ACCESS;

    uint8_t init = 0;
    add_char_params.p_init_value        = &init;

    err_code = characteristic_add(p_motion->service_handle,
                                  &add_char_params,
                                  &p_motion->status_handles);
    return err_code;
}

uint32_t ble_motion_init(ble_motion_t * p_motion)
{
    if (p_motion == NULL) return NRF_ERROR_NULL;

    uint32_t   err_code;
    ble_uuid_t ble_uuid;
    ble_uuid128_t base_uuid = { MOTION_UUID_BASE };

    p_motion->conn_handle = BLE_CONN_HANDLE_INVALID;

    err_code = sd_ble_uuid_vs_add(&base_uuid, &p_motion->uuid_type);
    VERIFY_SUCCESS(err_code);

    ble_uuid.type = p_motion->uuid_type;
    ble_uuid.uuid = MOTION_UUID_SERVICE;

    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
                                        &ble_uuid,
                                        &p_motion->service_handle);
    VERIFY_SUCCESS(err_code);

    err_code = dev_id_char_add(p_motion);
    VERIFY_SUCCESS(err_code);

    err_code = status_char_add(p_motion);
    VERIFY_SUCCESS(err_code);

    return NRF_SUCCESS;
}

void ble_motion_on_ble_evt(ble_evt_t const * p_ble_evt, void * p_context)
{
    ble_motion_t * p_motion = (ble_motion_t *)p_context;
    if (p_motion == NULL || p_ble_evt == NULL) return;

    switch (p_ble_evt->header.evt_id)
    {
        case BLE_GAP_EVT_CONNECTED:
            p_motion->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
            break;

        case BLE_GAP_EVT_DISCONNECTED:
            p_motion->conn_handle = BLE_CONN_HANDLE_INVALID;
            break;

        default:
            break;
    }
}

uint32_t ble_motion_status_notify(ble_motion_t * p_motion, uint8_t status)
{
    if (p_motion == NULL) return NRF_ERROR_NULL;
    if (p_motion->conn_handle == BLE_CONN_HANDLE_INVALID) return NRF_ERROR_INVALID_STATE;

    uint16_t len = sizeof(status);
    ble_gatts_hvx_params_t hvx_params;
    memset(&hvx_params, 0, sizeof(hvx_params));

    hvx_params.handle = p_motion->status_handles.value_handle;
    hvx_params.type   = BLE_GATT_HVX_NOTIFICATION;
    hvx_params.p_len  = &len;
    hvx_params.p_data = &status;

    return sd_ble_gatts_hvx(p_motion->conn_handle, &hvx_params);
}
