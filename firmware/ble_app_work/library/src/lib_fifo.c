/**
  ******************************************************************************************
  * @file    lib_fifo.c
  * @author  k.tashiro
  * @version 1.0
  * @date    2020/09/15
  * @brief   FIFO Contorl (Message AtomicFIFO)
  ******************************************************************************************
    Revision:
    Ver            Date             Revised by        Explanation
  ******************************************************************************************
  * 1.0            2020/09/15       k.tashiro         create new
  ******************************************************************************************
*/

/* Includes --------------------------------------------------------------*/
#include "lib_common.h"
#include "state_control.h"
#include "mode_manager.h"
#include "lib_ram_retain.h"
#include "lib_timer.h"
#include "lib_fifo.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"
#include "nrf_atfifo.h"
#include "nrf_atfifo_internal.h"
#include "lib_icm42607.h"
#include "definition.h"
#include "lib_trace_log.h"
#include "lib_bat.h"

/* Private variables -----------------------------------------------------*/
// Event fifo buffer
volatile EVT_ST g_fifo_buffer[EVT_FIFO_SIZE] = {0};
/* ACC/Gyro fifo buffer 2020.10.26 Modify ACC/Gyroデータに対応 */
volatile ACC_GYRO_DATA_INFO g_acc_buffer[ACC_GYRO_FIFO_SIZE] = {0};

volatile ble_gatts_hvx_params_t g_notify_buffer[2] = {0};
volatile SENSOR_FIFO_DATA_INFO g_fifo_clear_info[2] = {0};

volatile nrf_atfifo_t gEvt_fifo;
volatile nrf_atfifo_t g_acc_gyro_fifo;
volatile nrf_atfifo_t gNotifyBuf; 
volatile nrf_atfifo_t g_bat_result_fifo;
volatile nrf_atfifo_t g_fifo_count_fifo;

/**
 * @brief FIFO create
 * @param None
 * @retval None
 */
void FifoCreate(void)
{
	volatile uint32_t err_code;

	err_code = nrf_atfifo_init((nrf_atfifo_t*)&gEvt_fifo, (void*)&g_fifo_buffer, sizeof(g_fifo_buffer), sizeof(EVT_ST));
	LIB_ERR_CHECK(err_code, ATFIFO_CREATE, __LINE__);

	/* 2020.10.26 Modify ACC -> ACC/Gyro/Tempを扱う用に修正 */
	err_code = nrf_atfifo_init((nrf_atfifo_t*)&g_acc_gyro_fifo,(void*)&g_acc_buffer, sizeof(g_acc_buffer), sizeof(ACC_GYRO_DATA_INFO));
	LIB_ERR_CHECK(err_code, ATFIFO_CREATE, __LINE__);

	err_code = nrf_atfifo_init((nrf_atfifo_t*)&gNotifyBuf,(void*)&g_notify_buffer,sizeof(g_notify_buffer),sizeof(ble_gatts_hvx_params_t));
	LIB_ERR_CHECK(err_code, ATFIFO_CREATE, __LINE__);
	
	/* 2020.12.09 Add FIFO Clear ++ */
	err_code = nrf_atfifo_init((nrf_atfifo_t*)&g_fifo_count_fifo,(void*)&g_fifo_clear_info, sizeof(g_fifo_clear_info), sizeof(SENSOR_FIFO_DATA_INFO));
	LIB_ERR_CHECK(err_code, ATFIFO_CREATE, __LINE__);
	/* 2020.12.09 Add FIFO Clear -- */
	
}

/**
 * @brief Push Fifo Data(Event用)
 * @param pEvent Event Intofmation Data
 * @retval NRF_SUCCESS Success
 * @retval NRF_ERROR_NO_MEM Error
 */
uint32_t PushFifo(EVT_ST *pEvent)
{
	volatile uint32_t err_code;
	bool bFifo_err;
	EVT_ST *pPushEvent;
	nrf_atfifo_item_put_t  evt_put_context;
	
	bFifo_err = false;
	err_code = NRF_SUCCESS;
	
	pPushEvent = nrf_atfifo_item_alloc((nrf_atfifo_t*)&gEvt_fifo, &evt_put_context);
	if(pPushEvent != NULL)
	{
		memcpy(pPushEvent, pEvent, sizeof(EVT_ST));
		bFifo_err = nrf_atfifo_item_put((nrf_atfifo_t*)&gEvt_fifo, &evt_put_context);
		if(bFifo_err != true)
		{
			DEBUG_LOG(LOG_ERROR,"push critical evt 0x%x",pEvent->evt_id);
		}
	}
	else
	{
		DEBUG_LOG(LOG_INFO,"evt PushFifo No MEM");
		err_code = NRF_ERROR_NO_MEM;
		TRACE_LOG(TR_EVENT_FIFO_ERROR,(uint16_t)(pEvent->evt_id));
	}

	return err_code;

}

/**
 * @brief Push Fifo Data(ACC/Gyro用)
 * @param p_data ACC/Gyro Data
 * @retval NRF_SUCCESS Success
 * @retval NRF_ERROR_NO_MEM Error
 */
uint32_t AccGyroPushFifo(ACC_GYRO_DATA_INFO *p_data)
{
	volatile uint32_t err_code;
	bool bFifo_err;
	ACC_GYRO_DATA_INFO *p_push_data;
	nrf_atfifo_item_put_t  acc_put_context;
	
	err_code = NRF_SUCCESS;
	
	p_push_data = nrf_atfifo_item_alloc((nrf_atfifo_t*)&g_acc_gyro_fifo, &acc_put_context);
	if(p_push_data != NULL)
	{
		memcpy(p_push_data, p_data, sizeof( ACC_GYRO_DATA_INFO ));
		bFifo_err = nrf_atfifo_item_put((nrf_atfifo_t*)&g_acc_gyro_fifo, &acc_put_context);
		if(bFifo_err != true)
		{
			DEBUG_LOG(LOG_ERROR,"acc push critical");
		}
	}
	else
	{
		err_code = NRF_ERROR_NO_MEM;
		TRACE_LOG(TR_ACC_FIFO_ERROR,0);
	}
	return err_code;
}

/**
 * @brief POP Fifo Data(Event用)
 * @param pEvent Event Intofmation Data
 * @retval NRF_SUCCESS Success
 * @retval NRF_ERROR_NULL Error
 */
uint32_t PopFifo(PEVT_ST pEvent)
{
	volatile uint32_t err_code;
	bool bFifo_err;
	PEVT_ST pRecEvent;
	nrf_atfifo_item_get_t  evt_get_context;
	
	bFifo_err = true;
	err_code = NRF_SUCCESS;

	pRecEvent = nrf_atfifo_item_get((nrf_atfifo_t*)&gEvt_fifo, &evt_get_context);
	if(pRecEvent != NULL)
	{
		memcpy(pEvent, pRecEvent, sizeof(EVT_ST));

		bFifo_err = nrf_atfifo_item_free((nrf_atfifo_t*)&gEvt_fifo, &evt_get_context);
		if(bFifo_err != true)
		{
			DEBUG_LOG(LOG_DEBUG,"pop critical");
		}
	}
	else
	{
		err_code = NRF_ERROR_NULL;
	}

	return err_code;
}

/**
 * @brief POP Fifo Data(ACC/Gyro用)
 * @param p_data ACC/Gyro Data
 * @retval NRF_SUCCESS Success
 * @retval NRF_ERROR_NULL Error
 */
uint32_t AccGyroPopFifo(ACC_GYRO_DATA_INFO *p_data)
{
	volatile uint32_t err_code;
	bool bFifo_err;
	ACC_GYRO_DATA_INFO *p_pop_data;
	nrf_atfifo_item_get_t acc_get_context;
	
	bFifo_err = true;
	err_code = NRF_SUCCESS;

	p_pop_data = nrf_atfifo_item_get((nrf_atfifo_t*)&g_acc_gyro_fifo, &acc_get_context);
	if(p_pop_data != NULL)
	{
		memcpy(p_data, p_pop_data, sizeof( ACC_GYRO_DATA_INFO ));
		bFifo_err = nrf_atfifo_item_free((nrf_atfifo_t*)&g_acc_gyro_fifo, &acc_get_context);
		if(bFifo_err != true)
		{
			DEBUG_LOG(LOG_DEBUG,"Acc/Gyro pop critical");
		}
	}
	else
	{
		err_code = NRF_ERROR_NULL;
	}

	return err_code;
}

/**
 * @brief Push Fifo Data(Notifiy用)
 * @param pNotify_param BLE Notify parameter
 * @retval NRF_SUCCESS Success
 * @retval NRF_ERROR_NO_MEM Error
 */
uint32_t NotifyPushFifo(ble_gatts_hvx_params_t *pNotify_param)
{
	volatile uint32_t err_code;
	bool bFifo_err;
	ble_gatts_hvx_params_t *pPushNotify_param;
	nrf_atfifo_item_put_t  notify_put_context;
	
	err_code = NRF_SUCCESS;
	
	pPushNotify_param = nrf_atfifo_item_alloc((nrf_atfifo_t*)&gNotifyBuf, &notify_put_context);
	if(pPushNotify_param != NULL)
	{
		memcpy(pPushNotify_param, pNotify_param, sizeof(ble_gatts_hvx_params_t));
		bFifo_err = nrf_atfifo_item_put((nrf_atfifo_t*)&gNotifyBuf, &notify_put_context);
		if(bFifo_err != true)
		{
			DEBUG_LOG(LOG_ERROR,"notify push critical");
		}
	}
	else
	{
		DEBUG_LOG(LOG_DEBUG,"notify push fifo NO MEM");
		err_code = NRF_ERROR_NO_MEM;
	}
	return err_code;
}

/**
 * @brief POP Fifo Data(Notifiy用)
 * @param pNotify_param BLE Notify parameter
 * @retval NRF_SUCCESS Success
 * @retval NRF_ERROR_NO_MEM Error
 */
uint32_t NotifyPopFifo(ble_gatts_hvx_params_t *pNotify_param)
{
	volatile uint32_t err_code;
	bool bFifo_err;
	ble_gatts_hvx_params_t *pPopNotify_param;
	nrf_atfifo_item_get_t notify_get_context;

	pPopNotify_param = nrf_atfifo_item_get((nrf_atfifo_t*)&gNotifyBuf, &notify_get_context);
	if(pPopNotify_param != NULL)
	{
		memcpy(pNotify_param, pPopNotify_param, sizeof(ble_gatts_hvx_params_t));
		bFifo_err = nrf_atfifo_item_free((nrf_atfifo_t*)&gNotifyBuf, &notify_get_context);
		if(bFifo_err != true)
		{
			DEBUG_LOG(LOG_DEBUG,"notify pop critical");
		}
	}
	else
	{
		err_code = NRF_ERROR_NO_MEM;
	}
	return err_code;
}

/**
 * @brief Push Fifo Data(FIFO Clear)
 * @param pData FIFO Count Info
 * @retval NRF_SUCCESS Success
 * @retval NRF_ERROR_NO_MEM Error
 */
uint32_t FifoClearPushFifo( SENSOR_FIFO_DATA_INFO *p_data )
{
	volatile uint32_t err_code;
	bool bFifo_err;
	SENSOR_FIFO_DATA_INFO *p_push_data;
	nrf_atfifo_item_put_t  acc_put_context;
	
	err_code = NRF_SUCCESS;
	
	p_push_data = nrf_atfifo_item_alloc((nrf_atfifo_t*)&g_fifo_count_fifo, &acc_put_context);
	if(p_push_data != NULL)
	{
		memcpy(p_push_data, p_data, sizeof( SENSOR_FIFO_DATA_INFO ));
		bFifo_err = nrf_atfifo_item_put((nrf_atfifo_t*)&g_fifo_count_fifo, &acc_put_context);
		if(bFifo_err != true)
		{
			DEBUG_LOG(LOG_ERROR,"fifo clear push critical");
		}
	}
	else
	{
		err_code = NRF_ERROR_NO_MEM;
	}
	return err_code;
}

/**
 * @brief Pop Fifo Data(FIFO Clear)
 * @param pData FIFO Count Info
 * @retval NRF_SUCCESS Success
 * @retval NRF_ERROR_NO_MEM Error
 */
uint32_t FifoClearPopFifo( SENSOR_FIFO_DATA_INFO *p_data )
{
	volatile uint32_t err_code;
	bool bFifo_err;
	SENSOR_FIFO_DATA_INFO *p_pop_data;
	nrf_atfifo_item_get_t acc_get_context;
	
	bFifo_err = true;
	err_code = NRF_SUCCESS;

	p_pop_data = nrf_atfifo_item_get((nrf_atfifo_t*)&g_fifo_count_fifo, &acc_get_context);
	if(p_pop_data != NULL)
	{
		memcpy( p_data, p_pop_data, sizeof( SENSOR_FIFO_DATA_INFO ));
		bFifo_err = nrf_atfifo_item_free((nrf_atfifo_t*)&g_fifo_count_fifo, &acc_get_context);
		if(bFifo_err != true)
		{
			DEBUG_LOG(LOG_DEBUG,"fifo clear pop critical");
		}
	}
	else
	{
		err_code = NRF_ERROR_NULL;
	}

	return err_code;
}

/**
 * @brief Clear Notify FIFO(Notifiy用)
 * @param None
 * @retval None
 */
void ClearNotifyFifo(void)
{
	ret_code_t err;
	err = nrf_atfifo_clear((nrf_atfifo_t*)&gNotifyBuf);
	if(err != NRF_SUCCESS)
	{
		DEBUG_LOG(LOG_ERROR,"notify fifo clear err");
	}
}

/**
 * @brief FIFO Event Debug Log
 * @param err_code Error Code
 * @param event Event ID
 * @retval None
 */
void DebugEvtFifoLog(uint32_t err_code, uint8_t event)
{
	if(err_code != NRF_SUCCESS)
	{
		DEBUG_LOG( LOG_ERROR, "EVT FIFO ERR 0x%x, evt 0x%x", err_code, event );
	}
}


